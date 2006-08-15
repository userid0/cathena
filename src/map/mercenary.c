#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/nullpo.h"
#include "../common/mmo.h"
#include "../common/showmsg.h"

#include "log.h"
#include "clif.h"
#include "chrif.h"
#include "intif.h"
#include "itemdb.h"
#include "map.h"
#include "pc.h"
#include "status.h"
#include "skill.h"
#include "mob.h"
#include "pet.h"
#include "battle.h"
#include "party.h"
#include "guild.h"
#include "atcommand.h"
#include "script.h"
#include "npc.h"
#include "trade.h"
#include "unit.h"

#include "mercenary.h"
#include "charsave.h"

static int dirx[8]={0,-1,-1,-1,0,1,1,1};	//[orn]
static int diry[8]={1,1,0,-1,-1,-1,0,1};	//[orn]

//Better equiprobability than rand()% [orn]
#define rand(a, b) a+(int) ((float)(b-a+1)*rand()/(RAND_MAX+1.0))

struct homunculus_db homunculus_db[MAX_HOMUNCULUS_CLASS];	//[orn]
struct skill_tree_entry hskill_tree[MAX_HOMUNCULUS_CLASS][MAX_SKILL_TREE];

void merc_load_exptables(void);
int mercskill_castend_id( int tid, unsigned int tick, int id,int data );
static int merc_hom_hungry(int tid,unsigned int tick,int id,int data);

static unsigned long hexptbl[MAX_LEVEL+1];

void merc_load_exptables(void)
{
	FILE *fp;
	char line[1024];
	int i,k; 
	int j=0;
	int lines;
	char *filename[]={"exp_homun.txt","exp_homun2.txt"};
	char *str[32],*h,*nh;


	j = 0;
	memset(hexptbl,0,sizeof(hexptbl));
	for(i=0;i<2;i++){
		sprintf(line, "%s/%s", db_path, filename[i]);
		fp=fopen(line,"r");
		if(fp==NULL){
			if(i>0)
				continue;
			ShowError("can't read %s\n",line);
			return ;
		}
		lines = 0;
		while(fgets(line,sizeof(line)-1,fp) && j <= MAX_LEVEL){
			
			lines++;

			if(line[0] == '/' && line[1] == '/')
				continue;

			for(k=0,h=line;k<20;k++){
				if((nh=strchr(h,','))!=NULL){
					str[k]=h;
					*nh=0;
					h=nh+1;
				} else {
					str[k]=h;
					h+=strlen(h);
				}
			}

			hexptbl[j]= atoi(str[0]);
			
			j++;
		}
		if (j >= MAX_LEVEL)
			ShowWarning("read_hexptbl: Reached max level in exp_homun [%d]. Remaining lines were not read.\n ", MAX_HOMUNCULUS_CLASS);
		fclose(fp);
		ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n",filename[i]);
	}
	
	return ;

}


void merc_damage(struct homun_data *hd,struct block_list *src,int hp,int sp)
{
	nullpo_retv(hd);
	clif_hominfo(hd->master,hd,0);
}

int merc_hom_dead(struct homun_data *hd, struct block_list *src)
{
	struct map_session_data *sd = hd->master;
	if (!sd)
	{
		clif_emotion(&hd->bl, 16) ;	//wah
		return 7;
	}

	//Delete timers when dead.
	merc_hom_hungry_timer_delete(hd);
	merc_natural_heal_timer_delete(hd);
	sd->homunculus.hp = 0 ;
	clif_hominfo(sd,hd,0); // Send dead flag

	if(!merc_hom_decrease_intimacy(hd, 100)) { // Intimacy was < 100
		clif_emotion(&sd->bl, 23) ;	//omg
		return 7; //Delete from memory.
	}

	clif_send_homdata(hd->master,SP_INTIMATE,hd->master->homunculus.intimacy / 100);
	clif_emotion(&hd->bl, 16) ;	//wah
	clif_emotion(&sd->bl, 28) ;	//sob
	return 3; //Remove from map.
}

//Vaporize a character's homun. If flag, HP needs to be 80% or above.
int merc_hom_vaporize(struct map_session_data *sd, int flag)
{
	struct homun_data *hd;

	nullpo_retr(0, sd);

	hd = sd->hd;
	if (!hd || sd->homunculus.vaporize)
		return 0;
	
	if (flag && hd->battle_status.hp < (hd->battle_status.max_hp*80/100))
		return 0;

	//Delete timers when vaporized.
	merc_hom_hungry_timer_delete(hd);
	merc_natural_heal_timer_delete(hd);
	sd->homunculus.vaporize = 1;
	clif_hominfo(sd, sd->hd, 0);
	merc_save(hd);
	return unit_remove_map(&hd->bl, 0);
}

//delete a homunculus, completely "killing it". 
//Emote is the emotion the master should use, send negative to disable.
int merc_hom_delete(struct homun_data *hd, int emote)
{
	struct map_session_data *sd;
	nullpo_retr(0, hd);
	sd = hd->master;

	if (!sd)
		return unit_free(&hd->bl);

	if (emote >= 0)
		clif_emotion(&sd->bl, emote);

	//This makes it be deleted right away.
	sd->homunculus.intimacy = 0;
	// Send homunculus_dead to client
	sd->homunculus.hp = 0;
	clif_hominfo(sd, hd, 0);
	return unit_free(&hd->bl);
}

int merc_hom_calc_skilltree(struct map_session_data *sd)
{
	int i,id=0 ;
	int j,f=1;
	int c=0;

	nullpo_retr(0, sd);
	c = sd->homunculus.class_ - 6001 ;
	
	for(i=0;i < MAX_SKILL_TREE && (id = hskill_tree[c][i].id) > 0;i++){
		if(sd->homunculus.hskill[id-HM_SKILLBASE-1].id)
			continue; //Skill already known.
		if(!battle_config.skillfree) {
			for(j=0;j<5;j++) {
				if( hskill_tree[c][i].need[j].id &&
					merc_hom_checkskill(sd,hskill_tree[c][i].need[j].id) <
					hskill_tree[c][i].need[j].lv) {
					f=0;
					break;
				}
			}
		}
		if (f){
			sd->homunculus.hskill[id-HM_SKILLBASE-1].id = id ;
		}
	}

	return 0;
}

int merc_hom_checkskill(struct map_session_data *sd,int skill_id)
{
	int i = skill_id - HM_SKILLBASE - 1 ;
	if(sd == NULL) return 0;

	if(sd->homunculus.hskill[i].id == skill_id)
		return (sd->homunculus.hskill[i].lv);

	return 0;
}

int merc_skill_tree_get_max(int id, int b_class){
	int i, skillid;
	for(i=0;(skillid=hskill_tree[b_class-6001][i].id)>0;i++)
		if (id == skillid) return hskill_tree[b_class-6001][i].max;
	return skill_get_max (id);
}

void merc_hom_skillup(struct homun_data *hd,int skillnum)
{
	int i = 0 ;
	nullpo_retv(hd);

	if( hd->master->homunculus.vaporize == 0) {
		i = skillnum - HM_SKILLBASE - 1 ;
		if( hd->master->homunculus.skillpts > 0 &&
			hd->master->homunculus.hskill[i].id &&
			( hd->master->homunculus.hskill[i].flag == 0 ) && //Don't allow raising while you have granted skills. [Skotlex]
			hd->master->homunculus.hskill[i].lv < merc_skill_tree_get_max(skillnum, hd->master->homunculus.class_)
			)
		{
			hd->master->homunculus.hskill[i].lv++ ;
			hd->master->homunculus.skillpts-- ;
			status_calc_homunculus(hd,1) ;
			clif_homskillup(hd->master, skillnum) ;
			clif_hominfo(hd->master,hd,0) ;
			clif_homskillinfoblock(hd->master) ;
		}
	}
}

int merc_hom_levelup(struct homun_data *hd)
{
	int growth_str, growth_agi, growth_vit, growth_int, growth_dex, growth_luk ;
	int growth_max_hp, growth_max_sp ;
	char output[256] ;

	if (hd->master->homunculus.level == MAX_LEVEL) return 0 ;
	
	hd->master->homunculus.level++ ;
	if ( ( (hd->master->homunculus.level) % 3 ) == 0 ) hd->master->homunculus.skillpts++ ;	//1 skillpoint each 3 base level

	hd->master->homunculus.exp -= hd->exp_next ;
	hd->exp_next = hexptbl[hd->master->homunculus.level - 1] ;
	
	if ( hd->homunculusDB->gmaxHP <= hd->homunculusDB->gminHP ) 
		growth_max_hp = hd->homunculusDB->gminHP ;
	else
		growth_max_hp = rand(hd->homunculusDB->gminHP, hd->homunculusDB->gmaxHP) ;
	if ( hd->homunculusDB->gmaxSP <= hd->homunculusDB->gminSP ) 
		growth_max_sp = hd->homunculusDB->gminSP ;
	else
		growth_max_sp = rand(hd->homunculusDB->gminSP, hd->homunculusDB->gmaxSP) ;
	if ( hd->homunculusDB->gmaxSTR <= hd->homunculusDB->gminSTR ) 
		growth_str = hd->homunculusDB->gminSTR ;
	else
		growth_str = rand(hd->homunculusDB->gminSTR, hd->homunculusDB->gmaxSTR) ;
	if ( hd->homunculusDB->gmaxAGI <= hd->homunculusDB->gminAGI ) 
		growth_agi = hd->homunculusDB->gminAGI ;
	else
		growth_agi = rand(hd->homunculusDB->gminAGI, hd->homunculusDB->gmaxAGI) ;
	if ( hd->homunculusDB->gmaxVIT <= hd->homunculusDB->gminVIT ) 
		growth_vit = hd->homunculusDB->gminVIT ;
	else
		growth_vit = rand(hd->homunculusDB->gminVIT, hd->homunculusDB->gmaxVIT) ;
	if ( hd->homunculusDB->gmaxDEX <= hd->homunculusDB->gminDEX ) 
		growth_dex = hd->homunculusDB->gminDEX ;
	else
		growth_dex = rand(hd->homunculusDB->gminDEX, hd->homunculusDB->gmaxDEX) ;
	if ( hd->homunculusDB->gmaxINT <= hd->homunculusDB->gminINT ) 
		growth_int = hd->homunculusDB->gminINT ;
	else
		growth_int = rand(hd->homunculusDB->gminINT, hd->homunculusDB->gmaxINT) ;
	if ( hd->homunculusDB->gmaxLUK <= hd->homunculusDB->gminLUK ) 
		growth_luk = hd->homunculusDB->gminLUK ;
	else
		growth_luk = rand(hd->homunculusDB->gminLUK, hd->homunculusDB->gmaxLUK) ;

	hd->base_status.max_hp += growth_max_hp ;
	hd->base_status.max_sp += growth_max_sp ;
	hd->master->homunculus.max_hp = hd->base_status.max_hp ;
	hd->master->homunculus.max_sp = hd->base_status.max_sp ;
	hd->master->homunculus.str += growth_str ;
	hd->master->homunculus.agi += growth_agi ;
	hd->master->homunculus.vit += growth_vit ;
	hd->master->homunculus.dex += growth_dex ;
	hd->master->homunculus.int_ += growth_int ;
	hd->master->homunculus.luk += growth_luk ;
	
	if ( battle_config.homunculus_show_growth ) {
		sprintf(output,
				"Growth : hp:%d sp:%d str(%.2f) agi(%.2f) vit(%.2f) int(%.2f) dex(%.2f) luk(%.2f) ", growth_max_hp, growth_max_sp, growth_str/(float)10, growth_agi/(float)10, growth_vit/(float)10, growth_int/(float)10, growth_dex/(float)10, growth_luk/(float)10 ) ;
			clif_disp_onlyself(hd->master,output,strlen(output));
	}
	
	hd->base_status.str = (int) (hd->master->homunculus.str / 10) ;
	hd->base_status.agi = (int) (hd->master->homunculus.agi / 10) ;
	hd->base_status.vit = (int) (hd->master->homunculus.vit / 10) ;
	hd->base_status.dex = (int) (hd->master->homunculus.dex / 10) ;
	hd->base_status.int_ = (int) (hd->master->homunculus.int_ / 10) ;
	hd->base_status.luk = (int) (hd->master->homunculus.luk / 10) ;
			
	memcpy(&hd->battle_status, &hd->base_status, sizeof(struct status_data)) ;
	status_calc_homunculus(hd,1) ;

	status_percent_heal(&hd->bl, 100, 100);
//	merc_save(hd) ;	//not necessary
	
	return 1 ;
}

int merc_hom_evolution(struct homun_data *hd)
{
	struct map_session_data *sd;
	short x,y;
	nullpo_retr(0, hd);

	if(!hd->homunculusDB->evo_class)
	{
		clif_emotion(&hd->bl, 4) ;	//swt
		return 0 ;
	}
	sd = hd->master;
	if (!sd) return 0;

	//TODO: Clean this up to avoid free'ing/realloc'ing. 
	sd->homunculus.class_ = hd->homunculusDB->evo_class;
	x = hd->bl.x;
	y = hd->bl.y;
	merc_hom_vaporize(sd, 0);
	unit_free(&hd->bl);
	merc_call_homunculus(hd->master, x, y);
	clif_emotion(&sd->bl, 21) ;	//no1
	clif_misceffect2(&hd->bl,568);
	return 1 ;
}

int merc_hom_gainexp(struct homun_data *hd,int exp)
{
	if(hd->master->homunculus.vaporize)
		return 1;

	if( hd->exp_next == 0 ) {
		hd->master->homunculus.exp = 0 ;
		return 0;
	}

	hd->master->homunculus.exp += exp;

	if(hd->master->homunculus.exp < hd->exp_next) {
		clif_hominfo(hd->master,hd,0);
		return 0;
	}

 	//levelup
	do
	{
		merc_hom_levelup(hd) ;
	}
	while(hd->master->homunculus.exp > hd->exp_next && hd->exp_next != 0 );
		
	if( hd->exp_next == 0 ) {
		hd->master->homunculus.exp = 0 ;
	}

	status_calc_homunculus(hd,1);
	clif_misceffect2(&hd->bl,568);
	status_calc_homunculus(hd,1);
	return 0;
}

// Return then new value
int merc_hom_increase_intimacy(struct homun_data * hd, unsigned int value)
{
	if (hd->master->homunculus.intimacy + value <= 100000)
		hd->master->homunculus.intimacy += value;
	else
		hd->master->homunculus.intimacy = 100000;
	return hd->master->homunculus.intimacy;
}

// Return 0 if decrease fails or intimacy became 0 else the new value
int merc_hom_decrease_intimacy(struct homun_data * hd, unsigned int value)
{
	if (hd->master->homunculus.intimacy >= value)
		hd->master->homunculus.intimacy -= value;
	else
		hd->master->homunculus.intimacy = 0;

	return hd->master->homunculus.intimacy;
}

void merc_hom_heal(struct homun_data *hd,int hp,int sp)
{
	clif_hominfo(hd->master,hd,0);
}

static unsigned int natural_heal_prev_tick,natural_heal_diff_tick;
static void merc_natural_heal_hp(struct homun_data *hd)
{

	nullpo_retv(hd);

//	ShowDebug("merc_natural_heal_hp (1) : homunculus = %s | hd->ud.walktimer = %d |\n", hd->name, hd->ud.walktimer) ;
	if(hd->ud.walktimer == -1) {
		hd->inchealhptick += natural_heal_diff_tick;
	}
	else {
		hd->inchealhptick = 0;
		return;
	}

//	ShowDebug("merc_natural_heal_hp (2) : homunculus = %s | hd->regenhp = %d |\n", hd->name, hd->regenhp) ;
	if (hd->battle_status.hp != hd->battle_status.max_hp) {
		if ((unsigned int)status_heal(&hd->bl, hd->regenhp, 0, 1) < hd->regenhp)
		{	//At full.
			hd->inchealhptick = 0;
			return;
		}
	}

	return;
}

static void merc_natural_heal_sp(struct homun_data *hd)
{

	nullpo_retv(hd);

//	ShowDebug("merc_natural_heal_sp (1) : homunculus = %s | hd->regensp = %d |\n", hd->name, hd->regensp) ;
	if (hd->battle_status.sp != hd->battle_status.max_sp) {
		if ((unsigned int)status_heal(&hd->bl, 0, hd->regensp, 1) < hd->regensp)
		{	//At full.
			hd->inchealsptick = 0;
			return;
		}
	}

	return;
}

/*==========================================
 * HP/SP natural heal
 *------------------------------------------
 */

//static int merc_natural_heal_sub(struct homun_data *hd,va_list ap) {
static int merc_natural_heal_sub(struct homun_data *hd,int tick) {
//	int tick;

	nullpo_retr(0, hd);
//	tick = va_arg(ap,int);

// -- moonsoul (if conditions below altered to disallow natural healing if under berserk status)
	if (  status_isdead(&hd->bl) ||
		( ( hd->sc.count ) &&
			( (hd->sc.data[SC_POISON].timer != -1 ) || ( hd->sc.data[SC_BLEEDING].timer != -1 ) )
		)
	) { //Cannot heal neither natural or special.
		hd->hp_sub = hd->inchealhptick = 0;
		hd->sp_sub = hd->inchealsptick = 0;
	} else {	//natural heal
		merc_natural_heal_hp(hd);
		merc_natural_heal_sp(hd);
	}

	return 0;
}

/*==========================================
 * orn
 *------------------------------------------
 */
int merc_natural_heal(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	struct homun_data * hd;

	sd=map_id2sd(id);
	if(!sd)
		return 1;

	if(!sd->status.hom_id || !(hd = sd->hd))
		return 1;

	if(hd->natural_heal_timer != tid){
		if(battle_config.error_log)
			ShowError("merc_natural_heal %d != %d\n",hd->natural_heal_timer,tid);
		return 1;
	}

	hd->natural_heal_timer = -1;

	if(sd->homunculus.vaporize || sd->homunculus.hp == 0)
		return 1;
	
	natural_heal_diff_tick = DIFF_TICK(tick,natural_heal_prev_tick);
	merc_natural_heal_sub(hd, tick);
	
	natural_heal_prev_tick = tick;
	sd->hd->natural_heal_timer = add_timer(gettick()+battle_config.natural_healhp_interval, merc_natural_heal,sd->bl.id,0);

	return 0;
}

void merc_save(struct homun_data *hd)
{
	// copy data that must be saved in homunculus struct ( hp / sp )
	TBL_PC * sd = hd->master;
	if((sd->homunculus.hp = hd->battle_status.hp) > sd->homunculus.max_hp )
		sd->homunculus.hp = sd->homunculus.max_hp;
	if((sd->homunculus.sp = hd->battle_status.sp) > sd->homunculus.max_sp )
		sd->homunculus.sp = sd->homunculus.max_sp;
	intif_homunculus_requestsave(sd->status.account_id, &sd->homunculus) ;
}

static int merc_calc_pos(struct homun_data *hd,int tx,int ty,int dir)	//[orn]
{
	int x,y,dx,dy;
	int i,k;

	nullpo_retr(0, hd);

	hd->ud.to_x = tx;
	hd->ud.to_y = ty;

	if(dir < 0 || dir >= 8)
	 return 1;
	
	dx = -dirx[dir]*2;
	dy = -diry[dir]*2;
	x = tx + dx;
	y = ty + dy;
	if(!unit_can_reach_pos(&hd->bl,x,y,0)) {
		if(dx > 0) x--;
		else if(dx < 0) x++;
		if(dy > 0) y--;
		else if(dy < 0) y++;
		if(!unit_can_reach_pos(&hd->bl,x,y,0)) {
			for(i=0;i<12;i++) {
				k = rand(1, 8);
//				k = rand()%8;
				dx = -dirx[k]*2;
				dy = -diry[k]*2;
				x = tx + dx;
				y = ty + dy;
				if(unit_can_reach_pos(&hd->bl,x,y,0))
					break;
				else {
					if(dx > 0) x--;
					else if(dx < 0) x++;
					if(dy > 0) y--;
					else if(dy < 0) y++;
					if(unit_can_reach_pos(&hd->bl,x,y,0))
						break;
				}
			}
			if(i>=12) {
				x = tx;
				y = ty;
				if(!unit_can_reach_pos(&hd->bl,x,y,0))
					return 1;
			}
		}
	}
	hd->ud.to_x = x;
	hd->ud.to_y = y;
	return 0;
}

int merc_menu(struct map_session_data *sd,int menunum)
{
	nullpo_retr(0, sd);
	if (sd->hd == NULL)
		return 1;
	
	switch(menunum) {
		case 0:
			break;
		case 1:
			merc_hom_food(sd, sd->hd);
			break;
		case 2:
			merc_hom_delete(sd->hd, -1);
			break;
		default:
			ShowError("merc_menu : unknown menu choice : %d\n", menunum) ;
			break;
	}
	return 0;
}

int merc_hom_food(struct map_session_data *sd, struct homun_data *hd)
{
	int i, k, emotion;

	if(sd->homunculus.vaporize)
		return 1 ;

	k=hd->homunculusDB->foodID;
	i=pc_search_inventory(sd,k);
	if(i < 0) {
		clif_hom_food(sd,k,0);
		return 1;
	}
	pc_delitem(sd,i,1,0);

	if ( sd->homunculus.hunger >= 91 ) {
		merc_hom_decrease_intimacy(hd, 50);
		emotion = 16;
	} else if ( sd->homunculus.hunger >= 76 ) {
		merc_hom_decrease_intimacy(hd, 5);
		emotion = 19;
	} else if ( sd->homunculus.hunger >= 26 ) {
		merc_hom_increase_intimacy(hd, 75);
		emotion = 2;
	} else if ( sd->homunculus.hunger >= 11 ) {
		merc_hom_increase_intimacy(hd, 100);
		emotion = 2;
	} else {
		merc_hom_increase_intimacy(hd, 25);
		emotion = 2;
	}

	sd->homunculus.hunger += 10;	//dunno increase value for each food
	if(sd->homunculus.hunger > 100)
		sd->homunculus.hunger = 100;

	clif_emotion(&hd->bl,emotion) ;
	clif_send_homdata(sd,SP_HUNGRY,sd->homunculus.hunger);
	clif_send_homdata(sd,SP_INTIMATE,sd->homunculus.intimacy / 100);
	clif_hom_food(sd,hd->homunculusDB->foodID,1);
       	
	// Too much food :/
	if(sd->homunculus.intimacy == 0)
		return merc_hom_delete(sd->hd, 23); //omg  

	return 0;
}

static int merc_hom_hungry(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	struct homun_data *hd;

	sd=map_id2sd(id);
	if(!sd)
		return 1;

	if(!sd->status.hom_id || !sd->hd)
		return 1;

	hd = sd->hd;
	if(hd->hungry_timer != tid){
		if(battle_config.error_log)
			ShowError("merc_hom_hungry_timer %d != %d\n",hd->hungry_timer,tid);
		return 0 ;
	}

	hd->hungry_timer = -1;
	
	sd->homunculus.hunger-- ;
	if(sd->homunculus.hunger <= 10) {
		clif_emotion(&hd->bl, 6) ;	//an
	} else if(sd->homunculus.hunger == 25) {
		clif_emotion(&hd->bl, 20) ;	//hmm
	} else if(sd->homunculus.hunger == 75) {
		clif_emotion(&hd->bl, 33) ;	//ok
	}  
	
	if(sd->homunculus.hunger < 0) {
		sd->homunculus.hunger = 0;
		// Delete the homunculus if intimacy <= 100
		if ( !merc_hom_decrease_intimacy(hd, 100) )
			return merc_hom_delete(sd->hd, 23); //omg  
		clif_send_homdata(sd,SP_INTIMATE,sd->homunculus.intimacy / 100);
	}

	clif_send_homdata(sd,SP_HUNGRY,sd->homunculus.hunger);
	hd->hungry_timer = add_timer(tick+hd->homunculusDB->hungryDelay,merc_hom_hungry,sd->bl.id,0); //simple Fix albator
	return 0;
}

int merc_hom_hungry_timer_delete(struct homun_data *hd)
{
	nullpo_retr(0, hd);
	if(hd->hungry_timer != -1) {
		delete_timer(hd->hungry_timer,merc_hom_hungry);
		hd->hungry_timer = -1;
	}

	return 1;
}

int merc_natural_heal_timer_delete(struct homun_data *hd)
{
	nullpo_retr(0, hd);
	if(hd->natural_heal_timer != -1) {
		delete_timer(hd->natural_heal_timer,merc_natural_heal);
		hd->natural_heal_timer = -1;
	}

	return 1;
}

int search_homunculusDB_index(int key,int type)
{
	int i;

	for(i=0;i<MAX_HOMUNCULUS_CLASS;i++) {
		if(homunculus_db[i].class_ <= 0)
			continue;
		switch(type) {
			case HOMUNCULUS_CLASS:
				if(homunculus_db[i].class_ == key)
					return i;
				break;
			case HOMUNCULUS_FOOD:
				if(homunculus_db[i].foodID == key)
					return i;
				break;
			default:
				return -1;
		}
	}
	return -1;
}

// Create homunc structure
int merc_hom_create(struct map_session_data *sd)
{
	struct homun_data *hd;
	int i = 0 ;

	nullpo_retr(1, sd);

	Assert((sd->status.hom_id == 0 || sd->hd == 0) || sd->hd->master == sd); 

	i = search_homunculusDB_index(sd->homunculus.class_,HOMUNCULUS_CLASS);
	if(i < 0) {
		sd->status.hom_id = 0;
		return 1;
	}
	sd->hd = hd = (struct homun_data *)aCalloc(1,sizeof(struct homun_data));
	hd->homunculusDB = &homunculus_db[i];
	hd->master = sd;

	hd->bl.m=sd->bl.m;
	hd->bl.x=sd->bl.x;
	hd->bl.y=sd->bl.y - 1 ;
	hd->bl.subtype = MONS;
	hd->bl.type=BL_HOM;
	hd->bl.id= npc_get_new_npc_id();
	hd->bl.prev=NULL;
	hd->bl.next=NULL;
	hd->exp_next=hexptbl[hd->master->homunculus.level - 1];
	hd->ud.attacktimer=-1;
	hd->ud.attackabletime=gettick();
	hd->target_id = 0 ;

	for(i=0;i<MAX_STATUSCHANGE;i++) {
		hd->sc.data[i].timer=-1;
		hd->sc.data[i].val1 = hd->sc.data[i].val2 = hd->sc.data[i].val3 = hd->sc.data[i].val4 = 0;
	}

	hd->base_status.hp = hd->master->homunculus.hp ;
	hd->base_status.max_hp = hd->master->homunculus.max_hp ;
	hd->base_status.sp = hd->master->homunculus.sp ;
	hd->base_status.max_sp = hd->master->homunculus.max_sp ;
	hd->base_status.str = (int) (hd->master->homunculus.str / 10) ;
	hd->base_status.agi = (int) (hd->master->homunculus.agi / 10) ;
	hd->base_status.vit = (int) (hd->master->homunculus.vit / 10) ;
	hd->base_status.int_ = (int) (hd->master->homunculus.int_ / 10) ;
	hd->base_status.dex = (int) (hd->master->homunculus.dex / 10) ;
	hd->base_status.luk = (int) (hd->master->homunculus.luk / 10) ;
			
	memcpy(&hd->battle_status, &hd->base_status, sizeof(struct status_data)) ;

	status_set_viewdata(&hd->bl, hd->master->homunculus.class_);
	status_change_init(&hd->bl);
	hd->ud.dir = sd->ud.dir;
	unit_dataset(&hd->bl);
	
	map_addiddb(&hd->bl);
	status_calc_homunculus(hd,1);

	// Timers
	merc_hom_init_timers(hd);
	return 0;
}

void merc_hom_init_timers(struct homun_data * hd)
{
	if (hd->hungry_timer == -1)
		hd->hungry_timer = add_timer(gettick()+hd->homunculusDB->hungryDelay,merc_hom_hungry,hd->master->bl.id,0);
	if (hd->natural_heal_timer == -1)
	{
		hd->natural_heal_timer = add_timer(gettick()+battle_config.natural_healhp_interval, merc_natural_heal,hd->master->bl.id,0);
	}
}

int merc_call_homunculus(struct map_session_data *sd, short x, short y)
{
	struct homun_data *hd;

	if (!sd->status.hom_id) //Create a new homun.
		return merc_create_homunculus(sd, 6000 + rand(1, 8)) ;

	if (!sd->homunculus.vaporize)
		return 0; //Can't use this if homun wasn't vaporized.

	// If homunc not yet loaded, load it
	if (!sd->hd)
		merc_hom_create(sd);
	else
		merc_hom_init_timers(sd->hd);

	hd = sd->hd;
	sd->homunculus.vaporize = 0;
	if (hd->bl.prev == NULL)
	{	//Spawn him
		hd->bl.x = x;
		hd->bl.y = y;
		map_addblock(&hd->bl);
		clif_spawn(&hd->bl);
		clif_send_homdata(sd,SP_ACK,0);
		clif_hominfo(sd,hd,1);
		clif_hominfo(sd,hd,0); // send this x2. dunno why, but kRO does that [blackhole89]
		clif_homskillinfoblock(sd);
		merc_save(hd); 
	} else
		//Warp him to master.
		unit_warp(&hd->bl,sd->bl.m, x, y,0);
	return 1;
}

// Albator
// Recv data of an homunculus after it loading
int merc_hom_recv_data(int account_id, struct s_homunculus *sh, int flag)
{
	struct map_session_data *sd ;

	sd = map_id2sd(account_id);
	if(sd == NULL)
		return 0;
	
	if(flag == 0) {
		sd->status.hom_id = 0;
		return 0;
	}
	memcpy(&sd->homunculus, sh, sizeof(struct s_homunculus));

	if(sd->homunculus.hp && sd->homunculus.vaporize!=1)
	{
		merc_hom_create(sd);
		
		if ( sd->hd && sd->bl.prev != NULL) {
			map_addblock(&sd->hd->bl);
			clif_spawn(&sd->hd->bl);
			clif_hominfo(sd,sd->hd,1);
			clif_hominfo(sd,sd->hd,0); // send this x2. dunno why, but kRO does that [blackhole89]
			clif_homskillinfoblock(sd);
			clif_hominfo(sd,sd->hd,0);
			clif_send_homdata(sd,SP_ACK,0);
		}
	}

	return 1;

}

int merc_create_homunculus(struct map_session_data *sd, int class_)
{
	int i=0 ;

	nullpo_retr(1, sd);

	i = search_homunculusDB_index(class_,HOMUNCULUS_CLASS);
	if(i < 0) {
		sd->status.hom_id = 0;
		return 0;
	}
	memcpy(sd->homunculus.name, homunculus_db[i].name, NAME_LENGTH-1);

	sd->homunculus.class_ = class_;
	sd->homunculus.level=1;
	sd->homunculus.intimacy = 500;
	sd->homunculus.hunger = 50;
	sd->homunculus.exp = 0;
	sd->homunculus.rename_flag = 0;
	sd->homunculus.skillpts = 0;
	sd->homunculus.char_id = sd->status.char_id;
	sd->homunculus.vaporize = 0; // albator
	
	sd->homunculus.hp = sd->homunculus.max_hp = homunculus_db[i].basemaxHP ;
	sd->homunculus.sp = sd->homunculus.max_sp = homunculus_db[i].basemaxSP ;
	sd->homunculus.str = homunculus_db[i].baseSTR * 10;
	sd->homunculus.agi = homunculus_db[i].baseAGI * 10;
	sd->homunculus.vit = homunculus_db[i].baseVIT * 10;
	sd->homunculus.int_ = homunculus_db[i].baseINT * 10;
	sd->homunculus.dex = homunculus_db[i].baseDEX * 10;
	sd->homunculus.luk = homunculus_db[i].baseLUK * 10;
	
	for(i=0;i<MAX_HOMUNSKILL;i++)
		sd->homunculus.hskill[i].id = sd->homunculus.hskill[i].lv = sd->homunculus.hskill[i].flag = 0;

	intif_homunculus_create(sd->status.account_id, &sd->homunculus); // request homunculus creation
	
	return 1;
}

int merc_revive_homunculus(struct map_session_data *sd, unsigned char per, short x, short y)
{
	struct homun_data *hd;
	nullpo_retr(0, sd);
	if (!sd->status.hom_id || sd->homunculus.vaporize)
		return 0;

	if (!sd->hd) //Load homun data;
		merc_hom_create(sd);
	else
		merc_hom_init_timers(sd->hd);
	
	hd = sd->hd;

	if (!status_isdead(&hd->bl))
		return 0;

	if (!hd->bl.prev)
	{	//Add it back to the map.
		hd->bl.m = sd->bl.m;
		hd->bl.x = x;
		hd->bl.y = y;
		map_addblock(&sd->hd->bl);
		clif_spawn(&sd->hd->bl);
	}
	return status_revive(&hd->bl, per, 0);
}

void merc_homun_revive(struct homun_data *hd, unsigned int hp, unsigned int sp)
{
	struct map_session_data *sd = hd->master;
	if (!sd) return;
	clif_send_homdata(sd,SP_ACK,0);
	clif_hominfo(sd,hd,1);
	clif_hominfo(sd,hd,0);
	clif_homskillinfoblock(sd);
}

int read_homunculusdb()
{
	FILE *fp;
	char line[1024], *p;
	int i,k,l; 
	int j=0;
	int c = 0 ;
	int lines;
	char *filename[]={"homunculus_db.txt","homunculus_db2.txt"};
	char *str[36],*h,*nh;


	j = 0;
	memset(homunculus_db,0,sizeof(homunculus_db));
	for(i=0;i<2;i++){
		sprintf(line, "%s/%s", db_path, filename[i]);
		fp=fopen(line,"r");
		if(fp==NULL){
			if(i>0)
				continue;
			ShowError("can't read %s\n",line);
			return -1;
		}
		lines = 0;
		while(fgets(line,sizeof(line)-1,fp) && j < MAX_HOMUNCULUS_CLASS){
			
			lines++;

			if(line[0] == '/' && line[1] == '/')
				continue;

			for(k=0,h=line;k<36;k++){
				if((nh=strchr(h,','))!=NULL){
					str[k]=h;
					*nh=0;
					h=nh+1;
				} else {
					str[k]=h;
					h+=strlen(h);
				}
			}

			if(atoi(str[0]) < 6001 || atoi(str[0]) > 6099)
				continue;
		
			//Class,Homunculus,HP,SP,ATK,MATK,HIT,CRI,DEF,MDEF,FLEE,ASPD,STR,AGI,VIT,INT,DEX,LUK
			homunculus_db[j].class_ = atoi(str[0]);
			memcpy(homunculus_db[j].name,str[1],NAME_LENGTH-1);
			homunculus_db[j].basemaxHP = atoi(str[2]);
			homunculus_db[j].basemaxSP = atoi(str[3]);
			homunculus_db[j].baseSTR = atoi(str[4]);
			homunculus_db[j].baseAGI = atoi(str[5]);
			homunculus_db[j].baseVIT = atoi(str[6]);
			homunculus_db[j].baseINT = atoi(str[7]);
			homunculus_db[j].baseDEX = atoi(str[8]);
			homunculus_db[j].baseLUK = atoi(str[9]);
			homunculus_db[j].baseIntimacy = atoi(str[10]);
			homunculus_db[j].baseHungry = atoi(str[11]);
			homunculus_db[j].hungryDelay = atoi(str[12]);
			homunculus_db[j].foodID = atoi(str[13]);
			homunculus_db[j].gminHP = atoi(str[14]);
			homunculus_db[j].gmaxHP = atoi(str[15]);
			homunculus_db[j].gminSP = atoi(str[16]);
			homunculus_db[j].gmaxSP = atoi(str[17]);
			homunculus_db[j].gminSTR = atoi(str[18]);
			homunculus_db[j].gmaxSTR = atoi(str[19]);
			homunculus_db[j].gminAGI = atoi(str[20]);
			homunculus_db[j].gmaxAGI = atoi(str[21]);
			homunculus_db[j].gminVIT = atoi(str[22]);
			homunculus_db[j].gmaxVIT = atoi(str[23]);
			homunculus_db[j].gminINT = atoi(str[24]);
			homunculus_db[j].gmaxINT = atoi(str[25]);
			homunculus_db[j].gminDEX = atoi(str[26]);
			homunculus_db[j].gmaxDEX = atoi(str[27]);
			homunculus_db[j].gminLUK = atoi(str[28]);
			homunculus_db[j].gmaxLUK = atoi(str[29]);
			homunculus_db[j].evo_class = atoi(str[30]);
			homunculus_db[j].baseASPD = atoi(str[31]);
			homunculus_db[j].size = atoi(str[32]);
			homunculus_db[j].race = atoi(str[33]);
			homunculus_db[j].element = atoi(str[34]);
			homunculus_db[j].accessID = atoi(str[35]);

			j++;
		}
		if (j > MAX_HOMUNCULUS_CLASS)
			ShowWarning("read_homunculusdb: Reached max number of homunculus [%d]. Remaining homunculus were not read.\n ", MAX_HOMUNCULUS_CLASS);
		fclose(fp);
		ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' homunculus in '"CL_WHITE"db/%s"CL_RESET"'.\n",j,filename[i]);
	}

	memset(hskill_tree,0,sizeof(hskill_tree));
	sprintf(line, "%s/homun_skill_tree.txt", db_path);
	fp=fopen(line,"r");
	if(fp==NULL){
		ShowError("can't read %s\n", line);
		return 1;
	}

	while(fgets(line, sizeof(line)-1, fp)){
		char *split[50];
		int f=0, m=3;
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<14 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j<13)
			continue;
		if (j == 14) {
			f=1;	// MinJobLvl has been added
			m++;
		}
		// check for bounds [celest]
		c = atoi(split[0]) ;
		l = c - 6001 ;
		if ( l >= MAX_HOMUNCULUS_CLASS )
			continue;
		k = atoi(split[1]); //This is to avoid adding two lines for the same skill. [Skotlex]
		for(j = 0; j < MAX_SKILL_TREE && hskill_tree[l][j].id && hskill_tree[l][j].id != k; j++);
		if (j == MAX_SKILL_TREE)
		{
			ShowWarning("Unable to load skill %d into homunculus %d's tree. Maximum number of skills per class has been reached.\n", k, l);
			continue;
		}
		hskill_tree[l][j].id=k;
		hskill_tree[l][j].max=atoi(split[2]);
		if (f) hskill_tree[l][j].joblv=atoi(split[3]);

		for(k=0;k<5;k++){
			hskill_tree[l][j].need[k].id=atoi(split[k*2+m]);
			hskill_tree[l][j].need[k].lv=atoi(split[k*2+m+1]);
		}
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","homun_skill_tree.txt");


	return 0;
}

int do_init_merc (void)
{
	merc_load_exptables();
	memset(homunculus_db,0,sizeof(homunculus_db));	//[orn]
	read_homunculusdb();	//[orn]
	// Add homunc timer function to timer func list [Toms]
	natural_heal_prev_tick = gettick();
	add_timer_func_list(merc_natural_heal, "merc_natural_heal");
	add_timer_func_list(merc_hom_hungry, "merc_hom_hungry");
	return 0;
}

int do_final_merc (void);
