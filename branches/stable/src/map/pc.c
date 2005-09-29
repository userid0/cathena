#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "socket.h" // [Valaris]
#include "timer.h"
#include "db.h"

#include "malloc.h"
#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "status.h"
#include "npc.h"
#include "mob.h"
#include "pet.h"
#include "itemdb.h"
#include "script.h"
#include "battle.h"
#include "skill.h"
#include "party.h"
#include "guild.h"
#include "chat.h"
#include "trade.h"
#include "storage.h"
#include "vending.h"
#include "nullpo.h"
#include "atcommand.h"
#include "log.h"
#include "showmsg.h"
#include "core.h"

#ifndef TXT_ONLY // mail system [Valaris]
#include "mail.h"
#endif

#define PVP_CALCRANK_INTERVAL 1000	// PVP���ʌv�Z�̊Ԋu

static int exp_table[14][MAX_LEVEL];
static short statp[MAX_LEVEL];

// h-files are for declarations, not for implementations... [Shinomori]
struct skill_tree_entry skill_tree[MAX_PC_CLASS][MAX_SKILL_TREE];
// timer for night.day implementation
int day_timer_tid;
int night_timer_tid;

static int dirx[8]={0,-1,-1,-1,0,1,1,1};
static int diry[8]={1,1,0,-1,-1,-1,0,1};

struct fame_list smith_fame_list[10];
struct fame_list chemist_fame_list[10];

static unsigned int equip_pos[11]={0x0080,0x0008,0x0040,0x0004,0x0001,0x0200,0x0100,0x0010,0x0020,0x0002,0x8000};

static struct gm_account *gm_account = NULL;
static int GM_num = 0;

int pc_isGM(struct map_session_data *sd) {
	int i;

	nullpo_retr(0, sd);

	if(sd->bl.type!=BL_PC )
		return 0;


	//For console [Wizputer] //Unfortunately the console is "broken" because it shares fd 0 with disconnected players. [Skotlex]
//	if ( sd->fd == 0 )
//	    return 99;

	for(i = 0; i < GM_num; i++)
		if (gm_account[i].account_id == sd->status.account_id)
			return gm_account[i].level;
	return 0;

}

int pc_iskiller(struct map_session_data *src, struct map_session_data *target) {
	nullpo_retr(0, src);

	if(src->bl.type!=BL_PC )
		return 0;
	if (src->special_state.killer)
		return 1;

	if(target->bl.type!=BL_PC )
		return 0;
	
	if (target->special_state.killable)
		return 1;

	return 0;
}


int pc_set_gm_level(int account_id, int level) {
    int i;
    for (i = 0; i < GM_num; i++) {
        if (account_id == gm_account[i].account_id) {
            gm_account[i].level = level;
            return 0;
        }
    }

    GM_num++;
    gm_account = (struct gm_account *) aRealloc(gm_account, sizeof(struct gm_account) * GM_num);
    gm_account[GM_num - 1].account_id = account_id;
    gm_account[GM_num - 1].level = level;
    return 0;
}

static int distance(int x0, int y0, int x1, int y1) {
	int dx, dy;

	dx = abs(x0-x1);
	dy = abs(y0-y1);
	return dx>dy ? dx : dy;
}

static int pc_invincible_timer(int tid,unsigned int tick,int id,int data) {
	struct map_session_data *sd;

	if( (sd=(struct map_session_data *)map_id2sd(id)) == NULL || sd->bl.type!=BL_PC )
		return 1;

	if(sd->invincible_timer != tid){
		if(battle_config.error_log)
			ShowError("invincible_timer %d != %d\n",sd->invincible_timer,tid);
		return 0;
	}
	sd->invincible_timer=-1;
	skill_unit_move(&sd->bl,tick,1);

	return 0;
}

int pc_setinvincibletimer(struct map_session_data *sd,int val) {
	nullpo_retr(0, sd);

	if(sd->invincible_timer != -1)
		delete_timer(sd->invincible_timer,pc_invincible_timer);
	sd->invincible_timer = add_timer(gettick()+val,pc_invincible_timer,sd->bl.id,0);
	return 0;
}

int pc_delinvincibletimer(struct map_session_data *sd) {
	nullpo_retr(0, sd);

	if(sd->invincible_timer != -1) {
		delete_timer(sd->invincible_timer,pc_invincible_timer);
		sd->invincible_timer = -1;
	}
	skill_unit_move(&sd->bl,gettick(),1);
	return 0;
}

static int pc_spiritball_timer(int tid,unsigned int tick,int id,int data) {
	struct map_session_data *sd;

	if( (sd=(struct map_session_data *)map_id2sd(id)) == NULL || sd->bl.type!=BL_PC )
		return 1;

	if(sd->spirit_timer[0] != tid){
		if(battle_config.error_log)
			ShowError("spirit_timer %d != %d\n",sd->spirit_timer[0],tid);
		return 0;
	}

	if(sd->spiritball <= 0) {
		if(battle_config.error_log)
			ShowError("Spiritballs are already 0 when pc_spiritball_timer gets called");
		sd->spiritball = 0;
		return 0;
	}

	sd->spiritball--;
	// I leave this here as bad example [Shinomori]
	//memcpy( &sd->spirit_timer[0], &sd->spirit_timer[1], sizeof(sd->spirit_timer[0]) * sd->spiritball );
	memmove( sd->spirit_timer+0, sd->spirit_timer+1, (sd->spiritball)*sizeof(int) );
	sd->spirit_timer[sd->spiritball]=-1;

	clif_spiritball(sd);

	return 0;
}

int pc_addspiritball(struct map_session_data *sd,int interval,int max) {
	nullpo_retr(0, sd);

	if(max > MAX_SKILL_LEVEL)
		max = MAX_SKILL_LEVEL;
	if(sd->spiritball < 0)
		sd->spiritball = 0;

	if(sd->spiritball >= max) {
		if(sd->spirit_timer[0] != -1)
			delete_timer(sd->spirit_timer[0],pc_spiritball_timer);
		// I leave this here as bad example [Shinomori]
		//memcpy( &sd->spirit_timer[0], &sd->spirit_timer[1], sizeof(sd->spirit_timer[0]) * (sd->spiritball - 1));
		memmove( sd->spirit_timer+0, sd->spirit_timer+1, (sd->spiritball - 1)*sizeof(int) );
		//sd->spirit_timer[sd->spiritball-1] = -1; // intentionally, but will be overwritten
	} else
		sd->spiritball++;

	sd->spirit_timer[sd->spiritball-1] = add_timer(gettick()+interval,pc_spiritball_timer,sd->bl.id,0);
	clif_spiritball(sd);

	return 0;
}

int pc_delspiritball(struct map_session_data *sd,int count,int type) {
	int i;

	nullpo_retr(0, sd);

	if(sd->spiritball <= 0) {
		sd->spiritball = 0;
		return 0;
	}

	if(count > sd->spiritball)
		count = sd->spiritball;
	sd->spiritball -= count;
	if(count > MAX_SKILL_LEVEL)
		count = MAX_SKILL_LEVEL;

	for(i=0;i<count;i++) {
		if(sd->spirit_timer[i] != -1) {
			delete_timer(sd->spirit_timer[i],pc_spiritball_timer);
			sd->spirit_timer[i] = -1;
		}
	}
	for(i=count;i<MAX_SKILL_LEVEL;i++) {
		sd->spirit_timer[i-count] = sd->spirit_timer[i];
		sd->spirit_timer[i] = -1;
	}

	if(!type)
		clif_spiritball(sd);

	return 0;
}

// Increases a player's  and displays a notice to him
void pc_addfame(struct map_session_data *sd,int count) {
    nullpo_retv(sd);
	sd->status.fame += count;
	if(sd->status.fame > MAX_FAME)
	    sd->status.fame = MAX_FAME;
	switch(pc_calc_base_job2(sd->status.class_)){
		case JOB_BLACKSMITH: // Blacksmith
            clif_fame_blacksmith(sd,count);
            break;
		case JOB_ALCHEMIST: // Alchemist
            clif_fame_alchemist(sd,count);
            break;
	}
	chrif_save(sd); // Save to allow up-to-date fame list refresh
	chrif_reqfamelist(); // Refresh the fame list
}

// Check whether a player ID is in the Top 10 fame list of its job
int pc_istop10fame(int char_id,int job) {
    int i;
	switch(job){
	case JOB_BLACKSMITH: // Blacksmith
	    for(i=0;i<10;i++){
			if(smith_fame_list[i].id==char_id)
			    return 1;
		}
		break;
	case JOB_ALCHEMIST: // Alchemist
	    for(i=0;i<10;i++){
	        if(chemist_fame_list[i].id==char_id)
	            return 1;
		}
		break;
	}

	return 0;
}

int pc_setrestartvalue(struct map_session_data *sd,int type) {
	//?����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	int s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job2(sd->status.class_);

	//-----------------------
	// ���S����
	if(sd->special_state.restart_full_recover ||	// �I�V���X�J?�h
		sd->state.snovice_flag == 4) {				// [Celest]
		sd->status.hp=sd->status.max_hp;
		sd->status.sp=sd->status.max_sp;
		if (sd->state.snovice_flag == 4) {
			sd->state.snovice_flag = 0;
			status_change_start(&sd->bl,SkillStatusChangeTable[MO_STEELBODY],1,0,0,0,skill_get_time(MO_STEELBODY,1),0 );
		}
	}
	else {
		if(s_class == JOB_NOVICE && battle_config.restart_hp_rate < 50) { //�m�r�͔�����
			sd->status.hp=(sd->status.max_hp)/2;
		}
		else {
			if(battle_config.restart_hp_rate <= 0)
				sd->status.hp = 1;
			else {
				sd->status.hp = sd->status.max_hp * battle_config.restart_hp_rate /100;
				if(sd->status.hp <= 0)
					sd->status.hp = 1;
			}
		}
		if(battle_config.restart_sp_rate > 0) {
			int sp = sd->status.max_sp * battle_config.restart_sp_rate /100;
			if(sd->status.sp < sp)
				sd->status.sp = sp;
		}
	}
	if(type&1)
		clif_updatestatus(sd,SP_HP);
	if(type&1)
		clif_updatestatus(sd,SP_SP);

	/* removed exp penalty on spawn [Valaris] */

	if(type&2 && s_class != JOB_NOVICE && battle_config.zeny_penalty > 0 && !map[sd->bl.m].flag.nozenypenalty) {
		int zeny = (int)((double)sd->status.zeny * (double)battle_config.zeny_penalty / 10000.);
		if(zeny < 1) zeny = 1;
		sd->status.zeny -= zeny;
		if(sd->status.zeny < 0) sd->status.zeny = 0;
		clif_updatestatus(sd,SP_ZENY);
	}

	return 0;
}

/*==========================================
 * ��?�J���v���g�^�C�v�錾 (�K�v�ȕ��̂�)
 *------------------------------------------
 */
static int pc_walktoxy_sub(struct map_session_data *);

/*==========================================
 * save�ɕK�v�ȃX�e?�^�X�C�����s�Ȃ�
 *------------------------------------------
 */
int pc_makesavestatus(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	// �b̐F�͐F?���Q�������̂ŕۑ�?�ۂɂ͂��Ȃ�
	if(!battle_config.save_clothcolor)
		sd->status.clothes_color=0;

	// ���S?�Ԃ������̂�hp��1�A�ʒu���Z?�u�ꏊ��?�X
	if(!sd->state.waitingdisconnect) {
		if(pc_isdead(sd)){
			pc_setrestartvalue(sd,0);
			memcpy(&sd->status.last_point,&sd->status.save_point,sizeof(sd->status.last_point));
		} else {
			memcpy(sd->status.last_point.map, sd->mapname, MAP_NAME_LENGTH-1);
			sd->status.last_point.x = sd->bl.x;
			sd->status.last_point.y = sd->bl.y;
		}

		// �Z?�u�֎~�}�b�v�������̂Ŏw��ʒu�Ɉړ�
		if(map[sd->bl.m].flag.nosave){
			struct map_data *m=&map[sd->bl.m];
			if(strcmp(m->save.map,"SavePoint")==0)
				memcpy(&sd->status.last_point,&sd->status.save_point,sizeof(sd->status.last_point));
			else
				memcpy(&sd->status.last_point,&m->save,sizeof(sd->status.last_point));
		}
	}

	//�}�i?�|�C���g���v���X�������ꍇ0��
	if(battle_config.muting_players && sd->status.manner > 0)
		sd->status.manner = 0;

	return 0;
}

/*==========================================
 * ��?�b̏�����
 *------------------------------------------
 */
int pc_setnewpc(struct map_session_data *sd, int account_id, int char_id, int login_id1, unsigned int client_tick, int sex, int fd) {
	nullpo_retr(0, sd);

	sd->bl.id        = account_id;
	sd->char_id      = char_id;
	sd->login_id1    = login_id1;
	sd->login_id2    = 0; // at this point, we can not know the value :(
	sd->client_tick  = client_tick;
	sd->sex          = sex;
	sd->state.auth   = 0;
	sd->bl.type      = BL_PC;
	sd->canact_tick  = sd->canmove_tick = gettick();
	sd->canlog_tick  = gettick();
	sd->state.waitingdisconnect = 0;

	return 0;
}

int pc_equippoint(struct map_session_data *sd,int n)
{
	int ep = 0;
	//?����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	int s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job2(sd->status.class_);

	if(sd->inventory_data[n]) {
		ep = sd->inventory_data[n]->equip;
		if(sd->inventory_data[n]->look == 1 || sd->inventory_data[n]->look == 2 || sd->inventory_data[n]->look == 6) {
			if(ep == 2 && (pc_checkskill(sd,AS_LEFT) > 0 || s_class == JOB_ASSASSIN))
				return 34;
		}
	}
	return ep;
}

int pc_setinventorydata(struct map_session_data *sd)
{
	int i,id;

	nullpo_retr(0, sd);

	for(i=0;i<MAX_INVENTORY;i++) {
		id = sd->status.inventory[i].nameid;
		sd->inventory_data[i] = itemdb_search(id);
	}
	return 0;
}

int pc_calcweapontype(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->weapontype1 != 0 &&	sd->weapontype2 == 0)
		sd->status.weapon = sd->weapontype1;
	if(sd->weapontype1 == 0 &&	sd->weapontype2 != 0)// ���蕐�� Only
		sd->status.weapon = sd->weapontype2;
	else if(sd->weapontype1 == 1 && sd->weapontype2 == 1)// ?�Z?
		sd->status.weapon = 0x11;
	else if(sd->weapontype1 == 2 && sd->weapontype2 == 2)// ??��?
		sd->status.weapon = 0x12;
	else if(sd->weapontype1 == 6 && sd->weapontype2 == 6)// ??�蕀
		sd->status.weapon = 0x13;
	else if( (sd->weapontype1 == 1 && sd->weapontype2 == 2) ||
		(sd->weapontype1 == 2 && sd->weapontype2 == 1) ) // �Z? - ?��?
		sd->status.weapon = 0x14;
	else if( (sd->weapontype1 == 1 && sd->weapontype2 == 6) ||
		(sd->weapontype1 == 6 && sd->weapontype2 == 1) ) // �Z? - ��
		sd->status.weapon = 0x15;
	else if( (sd->weapontype1 == 2 && sd->weapontype2 == 6) ||
		(sd->weapontype1 == 6 && sd->weapontype2 == 2) ) // ?��? - ��
		sd->status.weapon = 0x16;
	else
		sd->status.weapon = sd->weapontype1;

	return 0;
}

int pc_setequipindex(struct map_session_data *sd)
{
	int i,j;

	nullpo_retr(0, sd);

	for(i=0;i<11;i++)
		sd->equip_index[i] = -1;

	for(i=0;i<MAX_INVENTORY;i++) {
		if(sd->status.inventory[i].nameid <= 0)
			continue;
		if(sd->status.inventory[i].equip) {
			for(j=0;j<11;j++)
				if(sd->status.inventory[i].equip & equip_pos[j])
					sd->equip_index[j] = i;
			if(sd->status.inventory[i].equip & 0x0002) {
				if(sd->inventory_data[i])
					sd->weapontype1 = sd->inventory_data[i]->look;
				else
					sd->weapontype1 = 0;
			}
			if(sd->status.inventory[i].equip & 0x0020) {
				if(sd->inventory_data[i]) {
					if(sd->inventory_data[i]->type == 4) {
						if(sd->status.inventory[i].equip == 0x0020)
							sd->weapontype2 = sd->inventory_data[i]->look;
						else
							sd->weapontype2 = 0;
					}
					else
						sd->weapontype2 = 0;
				}
				else
					sd->weapontype2 = 0;
			}
		}
	}
	pc_calcweapontype(sd);

	return 0;
}

int pc_isequip(struct map_session_data *sd,int n)
{
	struct item_data *item;
	struct status_change *sc_data;
	//?����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����

	nullpo_retr(0, sd);

	item = sd->inventory_data[n];
	sc_data = status_get_sc_data(&sd->bl);

	if( battle_config.gm_allequip>0 && pc_isGM(sd)>=battle_config.gm_allequip )
		return 1;

	if(item == NULL)
		return 0;
	if(item->elv > 0 && sd->status.base_level < item->elv)
		return 0;
	if(item->sex != 2 && sd->status.sex != item->sex)
		return 0;
	if(map[sd->bl.m].flag.pvp && (item->flag.no_equip&1)) //optimized by Lupus
		return 0;
	if(map[sd->bl.m].flag.gvg && (item->flag.no_equip>1)) //optimized by Lupus
		return 0;
	if((item->equip & 0x0002 || item->equip & 0x0020) && item->type == 4 && sc_data && sc_data[SC_STRIPWEAPON].timer != -1) // Also works with left-hand weapons [DracoRPG]
		return 0;
	if(item->equip & 0x0020 && item->type == 5 && sc_data && sc_data[SC_STRIPSHIELD].timer != -1) // Also works with left-hand weapons [DracoRPG]
		return 0;
	if(item->equip & 0x0010 && sc_data && sc_data[SC_STRIPARMOR].timer != -1)
		return 0;
	if(item->equip & 0x0100 && sc_data && sc_data[SC_STRIPHELM].timer != -1)
		return 0;

	//Taekwon Class
	if (sd->status.class_ == JOB_TAEKWON) { //[Lupus]
		//cannot equip ANY weapons.
		//Any equipments that can be equipped by 1) either by everybody, 2) or everybody except Novice, can be equipped 
		if (item->type == 4 || (item->class_ & 126) != 126) //126 = all 1st classes but Novice
			return 0;

	} else
	//Star Gladiator
	if (sd->status.class_ == JOB_STAR_GLADIATOR || sd->status.class_ == JOB_STAR_GLADIATOR2) { //[Lupus]
		//They can equip anything TK Boy can equip, plus,
		//2nd-class specific items as well. In addition, they can equip these specific equipments:
		//All book-type items, Manteau, Mink Coat, Mirror Shield, Sharp Headgear, Majestic Goat, Boots. 
		if ( (item->nameid>=1550 && item->nameid<1599) //a Book type
			|| item->nameid==2505 || item->nameid==2506 //Manteau[0] [1]
			|| item->nameid==2107 || item->nameid==2108 //Mirror Shield [0] [1]
			|| item->nameid==2405 || item->nameid==2406 //Boots [0] [1]
			|| item->nameid==2256 || item->nameid==2258 //Majestic Goat | Spiky Band
			)
			return 1; //You can equip these exceptions
		if ( item->type == 4 || !(((item->class_ & 126) == 126) || ((item->class_ & 2088832) == 2088832)) )	//2088832 = all 2nd classes but Super Novice
			return 0;
	} else
	//Soul Linker
	if (sd->status.class_ == JOB_SOUL_LINKER) { //[Lupus]
		//They can only equip equipments that Wizards can equip.
		if ((1<< JOB_WIZARD & item->class_) == 0)
			return 0;

	} else
	//Other Common Classes
	if ((1<<(pc_calc_base_job(sd->status.class_).job) & item->class_) == 0) //[Skotlex]
		return 0;

	return 1;
}

//�����j��
int pc_break_equip(struct map_session_data *sd, unsigned short where)
{
	int i, j;

	nullpo_retr(-1, sd);
	if (sd->unbreakable_equip & where)
		return 0;
	if (sd->unbreakable >= rand()%100)
		return 0;
	if (where == EQP_WEAPON && (sd->status.weapon == 0 ||	//Bare fists should not break :P
		sd->status.weapon == 6 || sd->status.weapon == 7 || sd->status.weapon == 8 || // Axes and Maces can't be broken [DracoRPG]
		sd->status.weapon == 10 || sd->status.weapon == 15 //Rods and Books can't be broken [Skotlex]
		))
		return 0;
	switch (where) {
		case EQP_WEAPON:
			i = SC_CP_WEAPON;
			break;
		case EQP_ARMOR:
			i = SC_CP_ARMOR;
			break;
		case EQP_SHIELD:
			i = SC_CP_SHIELD;
			break;
		case EQP_HELM:
			i = SC_CP_HELM;
			break;
		default:
			return 0;
	}
	if (sd->sc_count && sd->sc_data[i].timer != -1)
		return 0;

	for (i = 0; i < 11; i++) {
		if ((j = sd->equip_index[i]) > 0 && sd->status.inventory[j].attribute != 1 &&
			((where == EQP_HELM && i == 6) ||
			(where == EQP_ARMOR && i == 7) ||
			(where == EQP_WEAPON && (i == 8 || i == 9) && sd->inventory_data[j]->type == 4) ||
			(where == EQP_SHIELD && i == 9 && sd->inventory_data[j]->type == 5)))
		{
			sd->status.inventory[j].attribute = 1;
			pc_unequipitem(sd, j, 3);
			clif_equiplist(sd);
			return 1;
		}
	}

	return 1;
}

/*==========================================
 * session id�ɖ�薳��
 * char�I���瑗���Ă����X�e?�^�X��ݒ�
 *------------------------------------------
 */
int pc_authok(int id, int login_id2, time_t connect_until_time, struct mmo_charstatus *st)
{
	struct map_session_data *sd = NULL;

	struct party *p;
	struct guild *g;
	int i;
	unsigned long tick = gettick();

	sd = map_id2sd(id);
	if (sd == NULL)
	{
		ShowDebug("pc_authok: Player with account %d not found!\n", id);
		return 1;	
	//	nullpo_retr(1, sd);
	}

	if (sd->state.auth) //Temporary debug. [Skotlex]
	{
		ShowDebug("pc_authok: Received auth ok for already authorized client (account id %d)!\n", sd->bl.id);
		return 1;
	}
		
	sd->login_id2 = login_id2;
	memcpy(&sd->status, st, sizeof(*st));

	if (sd->status.sex != sd->sex) {
		clif_authfail_fd(sd->fd, 0);
		return 1;
	}

	//Initializations to null/0 unneeded since map_session_data was filled with 0 upon allocation.
//	memset(&sd->state, 0, sizeof(sd->state));
	// ��{�I�ȏ�����
	sd->state.connect_new = 1;
//	sd->bl.prev = sd->bl.next = NULL;

//	sd->weapontype1 = sd->weapontype2 = 0;
	sd->view_class = sd->status.class_;
	sd->speed = DEFAULT_WALK_SPEED;
//	sd->state.dead_sit = 0;
//	sd->dir = 0;
//	sd->head_dir = 0;
	sd->state.auth = 1;
	sd->walktimer = -1;
	sd->next_walktime = -1;
	sd->attacktimer = -1;
	sd->followtimer = -1; // [MouseJstr]
	sd->skilltimer = -1;
	sd->skillitem = -1;
	sd->skillitemlv = -1;
	sd->invincible_timer = -1;
	
//	sd->deal_locked = 0;
//	sd->trade_partner = 0;

//	sd->inchealhptick = 0;
//	sd->inchealsptick = 0;
//	sd->hp_sub = 0;
//	sd->sp_sub = 0;
//	sd->inchealspirithptick = 0;
//	sd->inchealspiritsptick = 0;
	sd->canact_tick = tick;
	sd->canmove_tick = tick;
	sd->canregen_tick = tick;
	sd->attackabletime = tick;
//	sd->reg = NULL;
//	sd->reg_num = 0;
//	sd->doridori_counter = 0;
	sd->change_level = pc_readglobalreg(sd,"jobchange_level");

//#ifndef TXT_ONLY // mail system [Valaris]
//	if(battle_config.mail_system)
//		sd->mail_counter = 0;
//#endif
//	sd->spiritball = 0;
	for(i = 0; i < MAX_SKILL_LEVEL; i++)
		sd->spirit_timer[i] = -1;
	for(i = 0; i < MAX_SKILLTIMERSKILL; i++)
		sd->skilltimerskill[i].timer = -1;
//	sd->timerskill_count=0;

//	memset(sd->blockskill,0,sizeof(sd->blockskill));

//	memset(&sd->dev,0,sizeof(struct square));
//	for(i = 0; i < 5; i++) {
//		sd->dev.val1[i] = 0;
//		sd->dev.val2[i] = 0;
//	}

	if (battle_config.item_auto_get)
		sd->state.autoloot = 1;
	
	// �A�J�E���g??�̑��M�v��
	intif_request_accountreg(sd);

	// �A�C�e���`�F�b�N
	pc_setinventorydata(sd);
	pc_checkitem(sd);

	// pet
//	sd->petDB = NULL;
//	sd->pd = NULL;
	sd->pet_hungry_timer = -1;
//	memset(&sd->pet, 0, sizeof(struct s_pet));

	// �X�e?�^�X�ُ�̏�����
	for(i = 0; i < MAX_STATUSCHANGE; i++) {
		sd->sc_data[i].timer=-1;
//		sd->sc_data[i].val1 = sd->sc_data[i].val2 = sd->sc_data[i].val3 = sd->sc_data[i].val4 = 0;
	}
	sd->sc_count=0;
	if ((battle_config.atc_gmonly == 0 || pc_isGM(sd)) &&
	    (pc_isGM(sd) >= get_atcommand_level(AtCommand_Hide)))
		sd->status.option &= (OPTION_MASK | OPTION_HIDE);
	else
		sd->status.option &= OPTION_MASK;

	// �X�L�����j�b�g?�W�̏�����
//	memset(sd->skillunit, 0, sizeof(sd->skillunit));
//	memset(sd->skillunittick, 0, sizeof(sd->skillunittick));	

	// �p?�e�B??�W�̏�����
//	sd->party_sended = 0;
//	sd->party_invite = 0;
	sd->party_x = -1;
	sd->party_y = -1;
	sd->party_hp = -1;

	// �M���h?�W�̏�����
//	sd->guild_sended = 0;
//	sd->guild_invite = 0;
//	sd->guild_alliance = 0;

	// �C�x���g?�W�̏�����
//	memset(sd->eventqueue, 0, sizeof(sd->eventqueue));
	for(i = 0; i < MAX_EVENTTIMER; i++)
		sd->eventtimer[i] = -1;
//	sd->eventcount=0;

	// �ʒu�̐ݒ�
	if (pc_setpos(sd,sd->status.last_point.map, sd->status.last_point.x, sd->status.last_point.y, 0) != 0) {
		if(battle_config.error_log)
			ShowError ("Last_point_map %s not found\n", sd->status.last_point.map);

		// try warping to a default map instead
		if (pc_setpos(sd, "prontera.gat", 273, 354, 0) != 0) {
			// if we fail again
			clif_authfail_fd(sd->fd, 0);
			return 1;
		}
	}

	// pet
	if (sd->status.pet_id > 0)
		intif_request_petdata(sd->status.account_id, sd->status.char_id, sd->status.pet_id);

	// �p?�e�B�A�M���h�f?�^�̗v��
	if (sd->status.party_id > 0 && (p = party_search(sd->status.party_id)) == NULL)
		party_request_info(sd->status.party_id);
	if (sd->status.guild_id > 0)
	{
		if ((g = guild_search(sd->status.guild_id)) == NULL)
			guild_request_info(sd->status.guild_id);
		else if (strcmp(sd->status.name,g->master) == 0)
		{	//Block Guild Skills to prevent logout/login reuse exploiting. [Skotlex]
			int skill_num[] = { GD_BATTLEORDER, GD_REGENERATION, GD_RESTORE, GD_EMERGENCYCALL };
			for (i = 0; i < 4; i++)
				if (guild_checkskill(g, skill_num[i]))
					pc_blockskill_start(sd, skill_num[i], 300000);
		}
	}

	// pvp�̐ݒ�
//	sd->pvp_rank = 0;
//	sd->pvp_point = 0;
	sd->pvp_timer = -1;
//	sd->pvp_won = 0;
//	sd->pvp_lost = 0;

	// �ʒm

	clif_authok(sd);
	map_addnickdb(sd);
	if (map_charid2nick(sd->status.char_id) == NULL)
		map_addchariddb(sd->status.char_id, sd->status.name);

	// Notify everyone that this char logged in [Skotlex].
	clif_foreachclient(clif_friendslist_toggle_sub, sd->status.account_id, sd->status.char_id, 1);
	
	//�X�p�m�r�p���ɃJ�E���^?�̃X�N���v�g??�����?�ݏo����sd�ւ̃Z�b�g
	sd->die_counter = pc_readglobalreg(sd,"PC_DIE_COUNTER");

	if ((i = pc_checkskill(sd,RG_PLAGIARISM)) > 0) {
		sd->cloneskill_id = pc_readglobalreg(sd,"CLONE_SKILL");
		if (sd->cloneskill_id > 0) {
			sd->status.skill[sd->cloneskill_id].id = sd->cloneskill_id;
			sd->status.skill[sd->cloneskill_id].lv = pc_readglobalreg(sd,"CLONE_SKILL_LV");
			if (i < sd->status.skill[sd->cloneskill_id].lv)
				sd->status.skill[sd->cloneskill_id].lv = i;
			sd->status.skill[sd->cloneskill_id].flag = 13;	//cloneskill flag			
			clif_skillinfoblock(sd);
		}
	}

	// Automated script events
	if (script_config.event_requires_trigger) {
		sd->state.event_death = pc_readglobalreg(sd, script_config.die_event_name);
		sd->state.event_kill = pc_readglobalreg(sd, script_config.kill_event_name);
		sd->state.event_disconnect = pc_readglobalreg(sd, script_config.logout_event_name);
	// if script triggers are not required
	} else {
		sd->state.event_death = 1;
		sd->state.event_kill = 1;
		sd->state.event_disconnect = 1;
	}

	if (night_flag && !map[sd->bl.m].flag.indoors) {
		char tmpstr[1024];
		strcpy(tmpstr, msg_txt(500)); // Actually, it's the night...
		clif_wis_message(sd->fd, wisp_server_name, tmpstr, strlen(tmpstr)+1);
		clif_weather1(sd->fd, 474 + battle_config.night_darkness_level);
	}

	// �X�e?�^�X�����v�Z�Ȃ�
	status_calc_pc(sd,1);
	{	//Add IP field
		unsigned char *ip = (unsigned char *) &session[sd->fd]->client_addr.sin_addr;
		if (pc_isGM(sd))
			ShowInfo("GM Character '"CL_WHITE"%s"CL_RESET"' logged in. (Acc. ID: '"CL_WHITE"%d"CL_RESET"', Connection: '"CL_WHITE"%d"CL_RESET"', Packet Ver: '"CL_WHITE"%d"CL_RESET"', IP: '"CL_WHITE"%d.%d.%d.%d"CL_RESET"', GM Level '"CL_WHITE"%d"CL_RESET"').\n", sd->status.name, sd->status.account_id, sd->fd, sd->packet_ver, ip[0],ip[1],ip[2],ip[3], pc_isGM(sd));
		else
			ShowInfo("Character '"CL_WHITE"%s"CL_RESET"' logged in. (Account ID: '"CL_WHITE"%d"CL_RESET"', Connection: '"CL_WHITE"%d"CL_RESET"', Packet Ver: '"CL_WHITE"%d"CL_RESET"', IP: '"CL_WHITE"%d.%d.%d.%d"CL_RESET"').\n", sd->status.name, sd->status.account_id, sd->fd, sd->packet_ver, ip[0],ip[1],ip[2],ip[3]);
	}
	
	if (script_config.event_script_type == 0) {
		struct npc_data *npc;
		//printf("pc: OnPCLogin event done. (%d events)\n", npc_event_doall("OnPCLogin") );
		if ((npc = npc_name2id(script_config.login_event_name))) {
			run_script(npc->u.scr.script,0,sd->bl.id,npc->bl.id); // PCLoginNPC
			ShowStatus("Event '"CL_WHITE"%s"CL_RESET"' executed.\n", script_config.login_event_name);
		}
	} else {
		ShowStatus("%d '"CL_WHITE"%s"CL_RESET"' events executed.\n",
			npc_event_doall_id(script_config.login_event_name, sd->bl.id), script_config.login_event_name);
	}

	// Send friends list
	clif_friendslist_send(sd);

	if (battle_config.display_version == 1){
		char buf[256];
		sprintf(buf, "eAthena SVN version: %s", get_svn_revision());
		clif_displaymessage(sd->fd, buf);
	}

	// Message of the Day�̑��M
	{
		char buf[256];
		FILE *fp;
		if ((fp = fopen(motd_txt, "r")) != NULL) {
			while (fgets(buf, sizeof(buf)-1, fp) != NULL) {
				int i;
				for(i=0; buf[i]; i++) {
					if (buf[i] == '\r' || buf[i]== '\n') {
						buf[i] = 0;
						break;
					}
				}
				if (battle_config.motd_type)
					clif_disp_onlyself(sd,buf,strlen(buf));
				else
					clif_displaymessage(sd->fd, buf);
			}
			fclose(fp);
		}
		else if(battle_config.error_log) {
			ShowWarning("In function pc_atuhok() -> File '"CL_WHITE"%s"CL_RESET"' not found.\n", motd_txt);
		}
	}

#ifndef TXT_ONLY
	if(battle_config.mail_system)
		mail_check(sd,1); // check mail at login [Valaris]
#endif

	// message of the limited time of the account
	if (connect_until_time != 0) { // don't display if it's unlimited or unknow value
		char tmpstr[1024];
		strftime(tmpstr, sizeof(tmpstr) - 1, msg_txt(501), localtime(&connect_until_time)); // "Your account time limit is: %d-%m-%Y %H:%M:%S."
		clif_wis_message(sd->fd, wisp_server_name, tmpstr, strlen(tmpstr)+1);
	}

	return 0;
}

/*==========================================
 * session id�ɖ�肠��Ȃ̂Ō�n��
 *------------------------------------------
 */
int pc_authfail(int id) {
	struct map_session_data *sd;

	sd = map_id2sd(id);
	if (sd == NULL)
		return 1;

	if (sd->state.auth) //Temporary debug. [Skotlex]
		ShowDebug("pc_authfail: Received auth fail for already authentified client (account id %d!)\n", sd->bl.id);

	clif_authfail_fd(sd->fd, 0);

	return 0;
}

static int pc_calc_skillpoint(struct map_session_data* sd)
{
	int  i,skill,inf2,skill_point=0;

	nullpo_retr(0, sd);

	for(i=1;i<MAX_SKILL;i++){
		if( (skill = pc_checkskill(sd,i)) > 0) {
			inf2 = skill_get_inf2(i);
			if((!(inf2&INF2_QUEST_SKILL) || battle_config.quest_skill_learn) &&
				!(inf2&INF2_WEDDING_SKILL) //Do not count wedding skills. [Skotlex]
				) {
				if(!sd->status.skill[i].flag)
					skill_point += skill;
				else if(sd->status.skill[i].flag > 2 && sd->status.skill[i].flag != 13) {
					skill_point += (sd->status.skill[i].flag - 2);
				}
			}
		}
	}

	return skill_point;
}

/*==========================================
 * ?������X�L���̌v�Z
 *------------------------------------------
 */
int pc_calc_skilltree(struct map_session_data *sd)
{
	int i,id=0,flag;
	int c=0;

	nullpo_retr(0, sd);

	c = pc_calc_skilltree_normalize_job(sd);

	for(i=0;i<MAX_SKILL;i++){
		if (sd->status.skill[i].flag != 13)
		        sd->status.skill[i].id=0;
		if (sd->status.skill[i].flag && sd->status.skill[i].flag != 13){	// card�X�L���Ȃ�A
			sd->status.skill[i].lv=(sd->status.skill[i].flag==1)?0:sd->status.skill[i].flag-2;	// �{?��lv��
			sd->status.skill[i].flag=0;	// flag��0�ɂ��Ă���
		}
	}

	if (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill){
		for(i=1;i<158;i++)
			sd->status.skill[i].id=i;
		for(i=210;i<291;i++)
			sd->status.skill[i].id=i;
		for(i=304;i<331;i++)
			sd->status.skill[i].id=i;
		for(i=334;i<338;i++)			
			sd->status.skill[i].id=i;
		for(i=355;i<473;i++)
			sd->status.skill[i].id=i;
		for(i=475;i<492;i++)
			sd->status.skill[i].id=i;
	} else {
            do {
                flag=0;
                for(i=0;i < MAX_SKILL_TREE && (id=skill_tree[c][i].id)>0;i++){
                    int j,f=1;
                    if(!battle_config.skillfree) {
						for(j=0;j<5;j++) {
							if( skill_tree[c][i].need[j].id &&
								pc_checkskill(sd,skill_tree[c][i].need[j].id) <
								skill_tree[c][i].need[j].lv) {
								f=0;
								break;
							}
						}
						if (sd->status.job_level < skill_tree[c][i].joblv)
							f=0;
						else if (((id >= SM_SWORD && id <= TF_DETOXIFY) || (id >= TK_RUN && id <= TK_HIGHJUMP)) && pc_checkskill(sd, NV_BASIC) < 9)
							f=0; // Do not unlock job1 skills when Basic Skills is not maxed out (can happen because of skill reset)
					}
					if(f && sd->status.skill[id].id==0 ){
						sd->status.skill[id].id=id;
						flag=1;
					}
				}
			} while(flag);
	}
	return 0;
}

// Make sure all the skills are in the correct condition
// before persisting to the backend.. [MouseJstr]
int pc_clean_skilltree(struct map_session_data *sd) {
	int i;
	for (i = 0; i < MAX_SKILL; i++){
		if (sd->status.skill[i].flag == 13){
			sd->status.skill[i].id = 0;
			sd->status.skill[i].lv = 0;
			sd->status.skill[i].flag = 0;
		}
	}

	return 0;
}

int pc_calc_skilltree_normalize_job(struct map_session_data *sd) {
	struct pc_base_job s_class = pc_calc_base_job(sd->status.class_);
	int c = s_class.job;

	if (!battle_config.skillup_limit || !((c >= JOB_NOVICE && c < JOB_SUPER_NOVICE) || c == JOB_TAEKWON))
		return sd->status.class_;
	else {
		int skill_point = pc_calc_skillpoint(sd);
		if(skill_point < 9)
			c = JOB_NOVICE;
		else if (((c > JOB_THIEF && c < JOB_SUPER_NOVICE) || c == JOB_TAEKWON) && sd->status.skill_point >= sd->status.job_level && ((sd->change_level > 0 && skill_point < sd->change_level+8) || skill_point < 58)) {
			switch(c) {
				case JOB_KNIGHT:
				case JOB_KNIGHT2:
				case JOB_CRUSADER:
				case JOB_CRUSADER2:
					c = JOB_SWORDMAN;
					break;
				case JOB_PRIEST:
				case JOB_MONK:
					c = JOB_ACOLYTE;
					break;
				case JOB_WIZARD:
				case JOB_SAGE:
					c = JOB_MAGE;
					break;
				case JOB_BLACKSMITH:
				case JOB_ALCHEMIST:
					c = JOB_MERCHANT;
					break;
				case JOB_HUNTER:
				case JOB_BARD:
				case JOB_DANCER:
					c = JOB_ARCHER;
					break;
				case JOB_ASSASSIN:
				case JOB_ROGUE:
					c = JOB_THIEF;
					break;
				case JOB_STAR_GLADIATOR:
				case JOB_STAR_GLADIATOR2:
				case JOB_SOUL_LINKER:
					c = JOB_TAEKWON;
					break;
			}
		}
		if (s_class.upper == 1)
		    c += JOB_NOVICE_HIGH;
		else if (s_class.upper == 2)
		    c += JOB_BABY;

		return c;
	}
}

/*==========================================
 * �d�ʃA�C�R���̊m�F
 *------------------------------------------
 */
int pc_checkweighticon(struct map_session_data *sd)
{
	int flag=0;

	nullpo_retr(0, sd);

	if(sd->weight*2 >= sd->max_weight)
		flag=1;
	if(sd->weight*10 >= sd->max_weight*9)
		flag=2;

	if(flag==1){
		if(sd->sc_data[SC_WEIGHT50].timer==-1)
			status_change_start(&sd->bl,SC_WEIGHT50,0,0,0,0,0,0);
	}else{
		status_change_end(&sd->bl,SC_WEIGHT50,-1);
	}
	if(flag==2){
		if(sd->sc_data[SC_WEIGHT90].timer==-1)
			status_change_start(&sd->bl,SC_WEIGHT90,0,0,0,0,0,0);
	}else{
		status_change_end(&sd->bl,SC_WEIGHT90,-1);
	}
	return 0;
}

/*==========================================
 * ? ���i�ɂ��\�͓��̃{?�i�X�ݒ�
 *------------------------------------------
 */
int pc_bonus(struct map_session_data *sd,int type,int val)
{
	int i;
	nullpo_retr(0, sd);

	switch(type){
	case SP_STR:
	case SP_AGI:
	case SP_VIT:
	case SP_INT:
	case SP_DEX:
	case SP_LUK:
		if(sd->state.lr_flag != 2)
			sd->parame[type-SP_STR]+=val;
		break;
	case SP_ATK1:
		if(!sd->state.lr_flag)
			sd->right_weapon.watk+=val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.watk+=val;
		break;
	case SP_ATK2:
		if(!sd->state.lr_flag)
			sd->right_weapon.watk2+=val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.watk2+=val;
		break;
	case SP_BASE_ATK:
		if(sd->state.lr_flag != 2)
			sd->base_atk+=val;
		break;
	case SP_MATK1:
		if(sd->state.lr_flag != 2)
			sd->matk1 += val;
		break;
	case SP_MATK2:
		if(sd->state.lr_flag != 2)
			sd->matk2 += val;
		break;
	case SP_MATK:
		if(sd->state.lr_flag != 2) {
			sd->matk1 += val;
			sd->matk2 += val;
		}
		break;
	case SP_DEF1:
		if(sd->state.lr_flag != 2)
			sd->def+=val;
		break;
	case SP_DEF2:
		if(sd->state.lr_flag != 2)
			sd->def2+=val;
		break;
	case SP_MDEF1:
		if(sd->state.lr_flag != 2)
			sd->mdef+=val;
		break;
	case SP_MDEF2:
		if(sd->state.lr_flag != 2)
			sd->mdef+=val;
		break;
	case SP_HIT:
		if(sd->state.lr_flag != 2)
			sd->hit+=val;
		else
			sd->arrow_hit+=val;
		break;
	case SP_FLEE1:
		if(sd->state.lr_flag != 2)
			sd->flee+=val;
		break;
	case SP_FLEE2:
		if(sd->state.lr_flag != 2)
			sd->flee2+=val*10;
		break;
	case SP_CRITICAL:
		if(sd->state.lr_flag != 2)
			sd->critical+=val*10;
		else
			sd->arrow_cri += val*10;
		break;
	case SP_ATKELE:
		if(!sd->state.lr_flag)
			sd->right_weapon.atk_ele=val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.atk_ele=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_ele=val;
		break;
	case SP_DEFELE:
		if(sd->state.lr_flag != 2)
			sd->def_ele=val;
		break;
	case SP_MAXHP:
		if(sd->state.lr_flag != 2)
			sd->status.max_hp+=val;
		break;
	case SP_MAXSP:
		if(sd->state.lr_flag != 2)
			sd->status.max_sp+=val;
		break;
	case SP_CASTRATE:
		if(sd->state.lr_flag != 2)
			sd->castrate+=val;
		break;
	case SP_MAXHPRATE:
		if(sd->state.lr_flag != 2)
			sd->hprate+=val;
		break;
	case SP_MAXSPRATE:
		if(sd->state.lr_flag != 2)
			sd->sprate+=val;
		break;
	case SP_SPRATE:
		if(sd->state.lr_flag != 2)
			sd->dsprate+=val;
		break;
	case SP_ATTACKRANGE:
		if(!sd->state.lr_flag)
			sd->attackrange += val;
		else if(sd->state.lr_flag == 1)
			sd->attackrange_ += val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_range += val;
		break;
	case SP_ADD_SPEED:	//Raw increase
		if(sd->state.lr_flag != 2)
			sd->speed -= val;
		break;
	case SP_SPEED_RATE:	//Non stackable increase
		if(sd->state.lr_flag != 2 && sd->speed_rate > 100-val)
			sd->speed_rate = 100-val;
		break;
	case SP_SPEED_ADDRATE:	//Stackable increase
		if(sd->state.lr_flag != 2)
			sd->speed_add_rate = sd->speed_add_rate * (100-val)/100;
		break;
	case SP_ASPD:	//Raw increase
		if(sd->state.lr_flag != 2)
			sd->aspd -= val*10;
		break;
	case SP_ASPD_RATE:	//Non stackable increase
		if(sd->state.lr_flag != 2 && sd->aspd_rate > 100-val)
			sd->aspd_rate = 100-val;
		break;
	case SP_ASPD_ADDRATE:	//Stackable increase - Made it linear as per rodatazone
		if(sd->state.lr_flag != 2)
			sd->aspd_add_rate -= val;
		break;
	case SP_HP_RECOV_RATE:
		if(sd->state.lr_flag != 2)
			sd->hprecov_rate += val;
		break;
	case SP_SP_RECOV_RATE:
		if(sd->state.lr_flag != 2)
			sd->sprecov_rate += val;
		break;
	case SP_CRITICAL_DEF:
		if(sd->state.lr_flag != 2)
			sd->critical_def += val;
		break;
	case SP_NEAR_ATK_DEF:
		if(sd->state.lr_flag != 2)
			sd->near_attack_def_rate += val;
		break;
	case SP_LONG_ATK_DEF:
		if(sd->state.lr_flag != 2)
			sd->long_attack_def_rate += val;
		break;
	case SP_DOUBLE_RATE:
		if(sd->state.lr_flag == 0 && sd->double_rate < val)
			sd->double_rate = val;
		break;
	case SP_DOUBLE_ADD_RATE:
		if(sd->state.lr_flag == 0)
			sd->double_add_rate += val;
		break;
	case SP_MATK_RATE:
		if(sd->state.lr_flag != 2)
			sd->matk_rate += val;
		break;
	case SP_IGNORE_DEF_ELE:
		if(!sd->state.lr_flag)
			sd->right_weapon.ignore_def_ele |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.ignore_def_ele |= 1<<val;
		break;
	case SP_IGNORE_DEF_RACE:
		if(!sd->state.lr_flag)
			sd->right_weapon.ignore_def_race |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.ignore_def_race |= 1<<val;
		break;
	case SP_ATK_RATE:
		if(sd->state.lr_flag != 2)
			sd->atk_rate += val;
		break;
	case SP_MAGIC_ATK_DEF:
		if(sd->state.lr_flag != 2)
			sd->magic_def_rate += val;
		break;
	case SP_MISC_ATK_DEF:
		if(sd->state.lr_flag != 2)
			sd->misc_def_rate += val;
		break;
	case SP_IGNORE_MDEF_ELE:
		if(sd->state.lr_flag != 2)
			sd->ignore_mdef_ele |= 1<<val;
		break;
	case SP_IGNORE_MDEF_RACE:
		if(sd->state.lr_flag != 2)
			sd->ignore_mdef_race |= 1<<val;
		break;
	case SP_PERFECT_HIT_RATE:
		if(sd->state.lr_flag != 2 && sd->perfect_hit < val)
			sd->perfect_hit = val;
		break;
	case SP_PERFECT_HIT_ADD_RATE:
		if(sd->state.lr_flag != 2)
			sd->perfect_hit_add += val;
		break;
	case SP_CRITICAL_RATE:
		if(sd->state.lr_flag != 2)
			sd->critical_rate+=val;
		break;
	case SP_DEF_RATIO_ATK_ELE:
		if(!sd->state.lr_flag)
			sd->right_weapon.def_ratio_atk_ele |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.def_ratio_atk_ele |= 1<<val;
		break;
	case SP_DEF_RATIO_ATK_RACE:
		if(!sd->state.lr_flag)
			sd->right_weapon.def_ratio_atk_race |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.def_ratio_atk_race |= 1<<val;
		break;
	case SP_HIT_RATE:
		if(sd->state.lr_flag != 2)
			sd->hit_rate += val;
		break;
	case SP_FLEE_RATE:
		if(sd->state.lr_flag != 2)
			sd->flee_rate += val;
		break;
	case SP_FLEE2_RATE:
		if(sd->state.lr_flag != 2)
			sd->flee2_rate += val;
		break;
	case SP_DEF_RATE:
		if(sd->state.lr_flag != 2)
			sd->def_rate += val;
		break;
	case SP_DEF2_RATE:
		if(sd->state.lr_flag != 2)
			sd->def2_rate += val;
		break;
	case SP_MDEF_RATE:
		if(sd->state.lr_flag != 2)
			sd->mdef_rate += val;
		break;
	case SP_MDEF2_RATE:
		if(sd->state.lr_flag != 2)
			sd->mdef2_rate += val;
		break;
	case SP_RESTART_FULL_RECOVER:
		if(sd->state.lr_flag != 2)
			sd->special_state.restart_full_recover = 1;
		break;
	case SP_NO_CASTCANCEL:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_castcancel = 1;
		break;
	case SP_NO_CASTCANCEL2:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_castcancel2 = 1;
		break;
	case SP_NO_SIZEFIX:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_sizefix = 1;
		break;
	case SP_NO_MAGIC_DAMAGE:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_magic_damage = 1;
		break;
	case SP_NO_WEAPON_DAMAGE:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_weapon_damage = 1;
		break;
	case SP_NO_GEMSTONE:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_gemstone = 1;
		break;
	case SP_INFINITE_ENDURE:
		if(sd->state.lr_flag != 2)
			sd->special_state.infinite_endure = 1;
		break;
	case SP_INTRAVISION: // Maya Purple Card effect allowing to see Hiding/Cloaking people [DracoRPG]
		if(sd->state.lr_flag != 2)
			sd->special_state.intravision = 1;
		break;
	case SP_SPLASH_RANGE:
		if(sd->state.lr_flag != 2 && sd->splash_range < val)
			sd->splash_range = val;
		break;
	case SP_SPLASH_ADD_RANGE:
		if(sd->state.lr_flag != 2)
			sd->splash_add_range += val;
		break;
	case SP_SHORT_WEAPON_DAMAGE_RETURN:
		if(sd->state.lr_flag != 2)
			sd->short_weapon_damage_return += val;
		break;
	case SP_LONG_WEAPON_DAMAGE_RETURN:
		if(sd->state.lr_flag != 2)
			sd->long_weapon_damage_return += val;
		break;
	case SP_MAGIC_DAMAGE_RETURN: //AppleGirl Was Here
		if(sd->state.lr_flag != 2)
			sd->magic_damage_return += val;
		break;
	case SP_ALL_STATS:	// [Valaris]
		if(sd->state.lr_flag!=2) {
			sd->parame[SP_STR-SP_STR]+=val;
			sd->parame[SP_AGI-SP_STR]+=val;
			sd->parame[SP_VIT-SP_STR]+=val;
			sd->parame[SP_INT-SP_STR]+=val;
			sd->parame[SP_DEX-SP_STR]+=val;
			sd->parame[SP_LUK-SP_STR]+=val;
		}
		break;
	case SP_AGI_VIT:	// [Valaris]
		if(sd->state.lr_flag!=2) {
			sd->parame[SP_AGI-SP_STR]+=val;
			sd->parame[SP_VIT-SP_STR]+=val;
		}
		break;
	case SP_AGI_DEX_STR:	// [Valaris]
		if(sd->state.lr_flag!=2) {
			sd->parame[SP_AGI-SP_STR]+=val;
			sd->parame[SP_DEX-SP_STR]+=val;
			sd->parame[SP_STR-SP_STR]+=val;
		}
		break;
	case SP_PERFECT_HIDE: // [Valaris]
		if(sd->state.lr_flag!=2) {
			sd->perfect_hiding=1;
		}
		break;
	case SP_DISGUISE: // Disguise script for items [Valaris]
		if(sd->state.lr_flag!=2 && !sd->disguiseflag && !pc_isriding(sd)) {
			clif_clearchar(&sd->bl, 0);
			sd->disguise=val;
			clif_changeoption(&sd->bl);
			clif_spawnpc(sd);
		}
		break;
	case SP_UNBREAKABLE:
		if(sd->state.lr_flag!=2) {
			sd->unbreakable += val;
		}
		break;
	case SP_UNBREAKABLE_WEAPON:
		if(sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_WEAPON;
		break;
	case SP_UNBREAKABLE_ARMOR:
		if(sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_ARMOR;
		break;
	case SP_UNBREAKABLE_HELM:
		if(sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_HELM;
		break;
	case SP_UNBREAKABLE_SHIELD:
		if(sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_SHIELD;
		break;
	case SP_CLASSCHANGE: // [Valaris]
		if(sd->state.lr_flag !=2){
			sd->classchange=val;
		}
		break;
	case SP_LONG_ATK_RATE:
		if(sd->state.lr_flag != 2 && sd->long_attack_atk_rate < val)
			sd->long_attack_atk_rate = val;
		break;
	case SP_BREAK_WEAPON_RATE:
		if(sd->state.lr_flag != 2)
			sd->break_weapon_rate+=val;
		break;
	case SP_BREAK_ARMOR_RATE:
		if(sd->state.lr_flag != 2)
			sd->break_armor_rate+=val;
		break;
	case SP_ADD_STEAL_RATE:
		if(sd->state.lr_flag != 2)
			sd->add_steal_rate+=val;
		break;
	case SP_DELAYRATE:
		if(sd->state.lr_flag != 2)
			sd->delayrate+=val;
		break;
	case SP_CRIT_ATK_RATE:
		if(sd->state.lr_flag != 2)
			sd->crit_atk_rate += val;
		break;
	case SP_NO_REGEN:
		if(sd->state.lr_flag != 2)
			sd->no_regen = val;
		break;
	case SP_UNSTRIPABLE_WEAPON:
		if(sd->state.lr_flag != 2)
			sd->unstripable_equip |= EQP_WEAPON;
		break;
	case SP_UNSTRIPABLE:
	case SP_UNSTRIPABLE_ARMOR:
		if(sd->state.lr_flag != 2)
			sd->unstripable_equip |= EQP_ARMOR;
		break;
	case SP_UNSTRIPABLE_HELM:
		if(sd->state.lr_flag != 2)
			sd->unstripable_equip |= EQP_HELM;
		break;
	case SP_UNSTRIPABLE_SHIELD:
		if(sd->state.lr_flag != 2)
			sd->unstripable_equip |= EQP_SHIELD;
		break;
	case SP_HP_DRAIN_VALUE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.hp_drain_value += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.hp_drain_value += val;
		}
		break;
	case SP_SP_DRAIN_VALUE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.sp_drain_value += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.sp_drain_value += val;
		}
		break;
	case SP_SP_GAIN_VALUE:
		if(!sd->state.lr_flag)
			sd->sp_gain_value += val;
		break;
	case SP_IGNORE_DEF_MOB:	// 0:normal monsters only, 1:affects boss monsters as well
		if(!sd->state.lr_flag)
			sd->right_weapon.ignore_def_mob |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.ignore_def_mob |= 1<<val;
		break;
	case SP_HP_GAIN_VALUE:
		if(!sd->state.lr_flag)
			sd->hp_gain_value += val;
		break;
	case SP_DAMAGE_WHEN_UNEQUIP:
		if(!sd->state.lr_flag) {
			for (i=0; i<11; i++) {
				if (sd->inventory_data[current_equip_item_index]->equip & equip_pos[i]) {
					sd->unequip_losehp[i] += val;
					break;
				}
			}
		}
		break;
	case SP_LOSESP_WHEN_UNEQUIP:
		if(!sd->state.lr_flag) {
			for (i=0; i<11; i++) {
				if (sd->inventory_data[current_equip_item_index]->equip & equip_pos[i]) {
					sd->unequip_losesp[i] += val;
					break;
				}
			}
		}
		break;
	default:
		if(battle_config.error_log)
			ShowWarning("pc_bonus: unknown type %d %d !\n",type,val);
		break;
	}
	return 0;
}

/*==========================================
 * ? ���i�ɂ��\�͓��̃{?�i�X�ݒ�
 *------------------------------------------
 */
int pc_bonus2(struct map_session_data *sd,int type,int type2,int val)
{
	int i;

	nullpo_retr(0, sd);

	switch(type){
	case SP_ADDELE:
		if(!sd->state.lr_flag)
			sd->right_weapon.addele[type2]+=val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.addele[type2]+=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_addele[type2]+=val;
		break;
	case SP_ADDRACE:
		if(!sd->state.lr_flag)
			sd->right_weapon.addrace[type2]+=val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.addrace[type2]+=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_addrace[type2]+=val;
		break;
	case SP_ADDSIZE:
		if(!sd->state.lr_flag)
			sd->right_weapon.addsize[type2]+=val;
		else if(sd->state.lr_flag == 1)
			sd->left_weapon.addsize[type2]+=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_addsize[type2]+=val;
		break;
	case SP_SUBELE:
		if(sd->state.lr_flag != 2)
			sd->subele[type2]+=val;
		break;
	case SP_SUBRACE:
		if(sd->state.lr_flag != 2)
			sd->subrace[type2]+=val;
		break;
	case SP_ADDEFF:
		if(sd->state.lr_flag != 2)
			sd->addeff[type2]+=val;
		else
			sd->arrow_addeff[type2]+=val;
		break;
	case SP_ADDEFF2:
		if(sd->state.lr_flag != 2)
			sd->addeff2[type2]+=val;
		else
			sd->arrow_addeff2[type2]+=val;
		break;
	case SP_RESEFF:
		if(sd->state.lr_flag != 2)
			sd->reseff[type2]+=val;
		break;
	case SP_MAGIC_ADDELE:
		if(sd->state.lr_flag != 2)
			sd->magic_addele[type2]+=val;
		break;
	case SP_MAGIC_ADDRACE:
		if(sd->state.lr_flag != 2)
			sd->magic_addrace[type2]+=val;
		break;
	case SP_MAGIC_SUBRACE:
		if(sd->state.lr_flag != 2)
			sd->magic_subrace[type2]+=val;
		break;
	case SP_ADD_DAMAGE_CLASS:
		if(!sd->state.lr_flag) {
			for(i=0;i<sd->right_weapon.add_damage_class_count;i++) {
				if(sd->right_weapon.add_damage_classid[i] == type2) {
					sd->right_weapon.add_damage_classrate[i] += val;
					break;
				}
			}
			if(i >= sd->right_weapon.add_damage_class_count && sd->right_weapon.add_damage_class_count < 10) {
				sd->right_weapon.add_damage_classid[sd->right_weapon.add_damage_class_count] = type2;
				sd->right_weapon.add_damage_classrate[sd->right_weapon.add_damage_class_count] += val;
				sd->right_weapon.add_damage_class_count++;
			}
		}
		else if(sd->state.lr_flag == 1) {
			for(i=0;i<sd->left_weapon.add_damage_class_count;i++) {
				if(sd->left_weapon.add_damage_classid[i] == type2) {
					sd->left_weapon.add_damage_classrate[i] += val;
					break;
				}
			}
			if(i >= sd->left_weapon.add_damage_class_count && sd->left_weapon.add_damage_class_count < 10) {
				sd->left_weapon.add_damage_classid[sd->left_weapon.add_damage_class_count] = type2;
				sd->left_weapon.add_damage_classrate[sd->left_weapon.add_damage_class_count] += val;
				sd->left_weapon.add_damage_class_count++;
			}
		}
		break;
	case SP_ADD_MAGIC_DAMAGE_CLASS:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_magic_damage_class_count;i++) {
				if(sd->add_magic_damage_classid[i] == type2) {
					sd->add_magic_damage_classrate[i] += val;
					break;
				}
			}
			if(i >= sd->add_magic_damage_class_count && sd->add_magic_damage_class_count < 10) {
				sd->add_magic_damage_classid[sd->add_magic_damage_class_count] = type2;
				sd->add_magic_damage_classrate[sd->add_magic_damage_class_count] += val;
				sd->add_magic_damage_class_count++;
			}
		}
		break;
	case SP_ADD_DEF_CLASS:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_def_class_count;i++) {
				if(sd->add_def_classid[i] == type2) {
					sd->add_def_classrate[i] += val;
					break;
				}
			}
			if(i >= sd->add_def_class_count && sd->add_def_class_count < 10) {
				sd->add_def_classid[sd->add_def_class_count] = type2;
				sd->add_def_classrate[sd->add_def_class_count] += val;
				sd->add_def_class_count++;
			}
		}
		break;
	case SP_ADD_MDEF_CLASS:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_mdef_class_count;i++) {
				if(sd->add_mdef_classid[i] == type2) {
					sd->add_mdef_classrate[i] += val;
					break;
				}
			}
			if(i >= sd->add_mdef_class_count && sd->add_mdef_class_count < 10) {
				sd->add_mdef_classid[sd->add_mdef_class_count] = type2;
				sd->add_mdef_classrate[sd->add_mdef_class_count] += val;
				sd->add_mdef_class_count++;
			}
		}
		break;
	case SP_HP_DRAIN_RATE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.hp_drain_rate += type2;
			sd->right_weapon.hp_drain_per += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.hp_drain_rate += type2;
			sd->left_weapon.hp_drain_per += val;
		}
		break;
	case SP_HP_DRAIN_VALUE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.hp_drain_value += type2;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.hp_drain_value += type2;
		}
		sd->sp_drain_type = val;
		break;
	case SP_SP_DRAIN_RATE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.sp_drain_rate += type2;
			sd->right_weapon.sp_drain_per += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.sp_drain_rate += type2;
			sd->left_weapon.sp_drain_per += val;
		}
		sd->sp_drain_type = 0;
		break;
	case SP_SP_DRAIN_VALUE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.sp_drain_value += type2;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.sp_drain_value += type2;
		}
		sd->sp_drain_type = val;
		break;
	case SP_GET_ZENY_NUM:
		if(sd->state.lr_flag != 2 && sd->get_zeny_rate < val)
		{
			sd->get_zeny_rate = val;
			sd->get_zeny_num = type2;
		}
		break;
	case SP_ADD_GET_ZENY_NUM:
		if(sd->state.lr_flag != 2)
		{
			sd->get_zeny_rate += val;
			sd->get_zeny_num += type2;
		}
		break;
	case SP_WEAPON_COMA_ELE:
		if(sd->state.lr_flag != 2)
			sd->weapon_coma_ele[type2] += val;
		break;
	case SP_WEAPON_COMA_RACE:
		if(sd->state.lr_flag != 2)
			sd->weapon_coma_race[type2] += val;
		break;
	case SP_RANDOM_ATTACK_INCREASE:	// [Valaris]
		if(sd->state.lr_flag !=2){
			sd->random_attack_increase_add = type2;
			sd->random_attack_increase_per += val;
		}
		break;
	case SP_WEAPON_ATK:
		if(sd->state.lr_flag != 2)
			sd->weapon_atk[type2]+=val;
		break;
	case SP_WEAPON_ATK_RATE:
		if(sd->state.lr_flag != 2)
			sd->weapon_atk_rate[type2]+=val;
		break;
	case SP_CRITICAL_ADDRACE:
		if(sd->state.lr_flag != 2)
			sd->critaddrace[type2] += val*10;
		break;
	case SP_ADDEFF_WHENHIT:
		if(sd->state.lr_flag != 2) {
			sd->addeff3[type2]+=val;
			sd->addeff3_type[type2]=1;
		}
		break;
	case SP_ADDEFF_WHENHIT_SHORT:
		if(sd->state.lr_flag != 2) {
			sd->addeff3[type2]+=val;
			sd->addeff3_type[type2]=0;
		}
		break;
	case SP_SKILL_ATK:
		for (i = 0; i < 5 && sd->skillatk[i][0] != 0 && sd->skillatk[i][0] != type2; i++);
		if (i == 5)
		{	//Better mention this so the array length can be updated. [Skotlex]
			ShowDebug("run_script: bonus2 bSkillAtk reached it's limit (5 skills per character), bonus skill %d (+%d%%) lost.\n", type2, val);
			break;
		}
		if(sd->state.lr_flag != 2) {
			if (sd->skillatk[i][0] == type2)
				sd->skillatk[i][1] += val;
			else {
				sd->skillatk[i][0] = type2;
				sd->skillatk[i][1] = val;
			}
		}
		break;
	case SP_ADD_DAMAGE_BY_CLASS:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_damage_class_count2;i++) {
				if(sd->add_damage_classid2[i] == type2) {
					sd->add_damage_classrate2[i] += val;
					break;
				}
			}
			if(i >= sd->add_damage_class_count2 && sd->add_damage_class_count2 < 10) {
				sd->add_damage_classid2[sd->add_damage_class_count2] = type2;
				sd->add_damage_classrate2[sd->add_damage_class_count2] += val;
				sd->add_damage_class_count2++;				
			}			
		}
		break;
	case SP_HP_LOSS_RATE:
		if(sd->state.lr_flag != 2) {
			sd->hp_loss_value = type2;
			sd->hp_loss_rate = val;
		}
		break;
	case SP_ADDRACE2:
		if (!(type2 > 0 && type2 < MAX_MOB_RACE_DB))
			break;
		if(sd->state.lr_flag != 2)
			sd->right_weapon.addrace2[type2] += val;
		else
			sd->left_weapon.addrace2[type2] += val;
		break;
	case SP_SUBSIZE:
		if(sd->state.lr_flag != 2)
			sd->subsize[type2]+=val;
		break;
	case SP_SUBRACE2:
		if(sd->state.lr_flag != 2)
			sd->subrace2[type2]+=val;
		break;
	case SP_ADD_ITEM_HEAL_RATE:
		if(sd->state.lr_flag != 2)
			sd->itemhealrate[type2 - 1] += val;
		break;
	case SP_EXP_ADDRACE:
		if(sd->state.lr_flag != 2)
			sd->expaddrace[type2]+=val;
		break;
	case SP_SP_GAIN_RACE:
		if(sd->state.lr_flag != 2)
			sd->sp_gain_race[type2]+=val;
		break;
	case SP_ADD_MONSTER_DROP_ITEM:
		if (sd->state.lr_flag != 2) {
			for(i = 0; i < sd->monster_drop_item_count; i++) {
				if(sd->monster_drop_itemid[i] == type2) {
					sd->monster_drop_race[i] |= (1<<10)|(1<<11);
					if(sd->monster_drop_itemrate[i] < val)
						sd->monster_drop_itemrate[i] = val;
					break;
				}
			}
			if(i >= sd->monster_drop_item_count && sd->monster_drop_item_count < 10) {
				sd->monster_drop_itemid[sd->monster_drop_item_count] = type2;
				// all monsters, including boss and non boss monsters
				sd->monster_drop_race[sd->monster_drop_item_count] |= (1<<10)|(1<<11);
				sd->monster_drop_itemrate[sd->monster_drop_item_count] = val;
				sd->monster_drop_item_count++;
			}
		}
		break;
	case SP_ADD_MONSTER_DROP_ITEMGROUP:
		if (sd->state.lr_flag != 2) {
			for(i = 0; i < sd->monster_drop_item_count; i++) {
				if(sd->monster_drop_itemgroup[i] == type2) {
					sd->monster_drop_itemid[i] = 0;
					sd->monster_drop_race[i] |= (1<<10)|(1<<11);
					if(sd->monster_drop_itemrate[i] < val)
						sd->monster_drop_itemrate[i] = val;
					break;
				}
			}
			if(i >= sd->monster_drop_item_count && sd->monster_drop_item_count < 10) {
				sd->monster_drop_itemgroup[sd->monster_drop_item_count] = type2;
				sd->monster_drop_itemid[sd->monster_drop_item_count] = 0;
				// all monsters, including boss and non boss monsters
				sd->monster_drop_race[sd->monster_drop_item_count] |= (1<<10)|(1<<11);
				sd->monster_drop_itemrate[sd->monster_drop_item_count] = val;
				sd->monster_drop_item_count++;
			}
		}
		break;
	case SP_SP_LOSS_RATE:
		if(sd->state.lr_flag != 2) {
			sd->sp_loss_value = type2;
			sd->sp_loss_rate = val;
		}
		break;

	default:
		if(battle_config.error_log)
			ShowWarning("pc_bonus2: unknown type %d %d %d!\n",type,type2,val);
		break;
	}
	return 0;
}

int pc_bonus3(struct map_session_data *sd,int type,int type2,int type3,int val)
{
	int i;
	nullpo_retr(0, sd);

	switch(type){
	case SP_ADD_MONSTER_DROP_ITEM:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->monster_drop_item_count;i++) {
				if(sd->monster_drop_itemid[i] == type2) {
					sd->monster_drop_race[i] |= 1<<type3;
					if(sd->monster_drop_itemrate[i] < val)
						sd->monster_drop_itemrate[i] = val;
					break;
				}
			}
			if(i >= sd->monster_drop_item_count && sd->monster_drop_item_count < 10) {
				sd->monster_drop_itemid[sd->monster_drop_item_count] = type2;
				sd->monster_drop_race[sd->monster_drop_item_count] |= 1<<type3;
				sd->monster_drop_itemrate[sd->monster_drop_item_count] = val;
				sd->monster_drop_item_count++;
			}
		}
		break;
	case SP_AUTOSPELL:
		if(sd->state.lr_flag != 2) {
			for (i = 0; i < 10; i++) {
				if (sd->autospell_id[i] == 0 ||
					(sd->autospell_id[i] == type2 && sd->autospell_lv[i] < type3) ||
					(sd->autospell_id[i] == type2 && sd->autospell_lv[i] == type3 && sd->autospell_rate[i] < val))
				{
					sd->autospell_id[i] = type2;
					sd->autospell_lv[i] = type3;
					sd->autospell_rate[i] = val;
					break;
				}
			}
		}
		break;
	case SP_AUTOSPELL_WHENHIT:
		if(sd->state.lr_flag != 2) {
			for (i = 0; i < 10; i++) {
				if (sd->autospell2_id[i] == 0 ||
					(sd->autospell2_id[i] == type2 && sd->autospell2_lv[i] < type3) ||
					(sd->autospell2_id[i] == type2 && sd->autospell2_lv[i] == type3 && sd->autospell2_rate[i] < val))
				{
					sd->autospell2_id[i] = type2;
					sd->autospell2_lv[i] = type3;
					sd->autospell2_rate[i] = val;
					break;
				}
			}
		}
		break;
	case SP_HP_LOSS_RATE:
		if(sd->state.lr_flag != 2) {
			sd->hp_loss_value = type2;
			sd->hp_loss_rate = type3;
			sd->hp_loss_type = val;
		}
		break;
	case SP_SP_DRAIN_RATE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.sp_drain_rate += type2;
			sd->right_weapon.sp_drain_per += type3;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.sp_drain_rate += type2;
			sd->left_weapon.sp_drain_per += type3;
		}
		sd->sp_drain_type = val;
		break;
	case SP_ADD_MONSTER_DROP_ITEMGROUP:
		if (sd->state.lr_flag != 2) {
			for(i = 0; i < sd->monster_drop_item_count; i++) {
				if(sd->monster_drop_itemgroup[i] == type2) {
					sd->monster_drop_itemid[i] = 0;
					sd->monster_drop_race[i] |= 1<<type3;
					if(sd->monster_drop_itemrate[i] < val)
						sd->monster_drop_itemrate[i] = val;
					break;
				}
			}
			if(i >= sd->monster_drop_item_count && sd->monster_drop_item_count < 10) {
				sd->monster_drop_itemgroup[sd->monster_drop_item_count] = type2;
				sd->monster_drop_itemid[sd->monster_drop_item_count] = 0;
				// all monsters, including boss and non boss monsters
				sd->monster_drop_race[sd->monster_drop_item_count] |= 1<<type3;
				sd->monster_drop_itemrate[sd->monster_drop_item_count] = val;
				sd->monster_drop_item_count++;
			}
		}
		break;

	default:
		if(battle_config.error_log)
			ShowWarning("pc_bonus3: unknown type %d %d %d %d!\n",type,type2,type3,val);
		break;
	}

	return 0;
}

int pc_bonus4(struct map_session_data *sd,int type,int type2,int type3,int type4,int val)
{
	int i;
	nullpo_retr(0, sd);

	switch(type){
	case SP_AUTOSPELL:
		if(sd->state.lr_flag != 2) {
			for (i = 0; i < 10; i++) {
				if (sd->autospell_id[i] == 0 ||
					(sd->autospell_id[i] == type2 && sd->autospell_lv[i] < type3) ||
					(sd->autospell_id[i] == type2 && sd->autospell_lv[i] == type3 && sd->autospell_rate[i] < type4))
				{
					sd->autospell_id[i] = (val) ? type2 : -type2;		// val = 0: self, 1: enemy
					sd->autospell_lv[i] = type3;
					sd->autospell_rate[i] = type4;
					break;
				}
			}
		}			
		break;
	case SP_AUTOSPELL_WHENHIT:
		if(sd->state.lr_flag != 2) {
			for (i = 0; i < 10; i++) {
				if (sd->autospell2_id[i] == 0 ||
					(sd->autospell2_id[i] == type2 && sd->autospell2_lv[i] < type3) ||
					(sd->autospell2_id[i] == type2 && sd->autospell2_lv[i] == type3 && sd->autospell2_rate[i] < type4))
				{
					sd->autospell2_id[i] = (val) ? type2 : -type2;		// val = 0: self, 1: enemy
					sd->autospell2_lv[i] = type3;
					sd->autospell2_rate[i] = type4;
					break;
				}
			}
		}
		break;
	default:
		if(battle_config.error_log)
			ShowWarning("pc_bonus4: unknown type %d %d %d %d %d!\n",type,type2,type3,type4,val);
		break;
	}

	return 0;
}

/*==========================================
 * �X�N���v�g�ɂ��X�L������
 *------------------------------------------
 */
int pc_skill(struct map_session_data *sd,int id,int level,int flag)
{
	nullpo_retr(0, sd);

	if(level>MAX_SKILL_LEVEL){
		if(battle_config.error_log)
			ShowError("support card skill only!\n");
		return 0;
	}
	if(!flag && (sd->status.skill[id].id == id || level == 0)){	// �N�G�X�g�����Ȃ炱����?�����m�F���đ��M����
		sd->status.skill[id].lv=level;
		status_calc_pc(sd,0);
		clif_skillinfoblock(sd);
	}
	else if(flag==2 && (sd->status.skill[id].id == id || level == 0)){	// �N�G�X�g�����Ȃ炱����?�����m�F���đ��M����
		sd->status.skill[id].lv+=level;
		status_calc_pc(sd,0);
		clif_skillinfoblock(sd);
	}
	else if(sd->status.skill[id].lv < level){	// ?�����邪lv���������Ȃ�
		if(sd->status.skill[id].id==id)
			sd->status.skill[id].flag=sd->status.skill[id].lv+2;	// lv���L��
		else {
			sd->status.skill[id].id=id;
			sd->status.skill[id].flag=1;	// card�X�L���Ƃ���
		}
		sd->status.skill[id].lv=level;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_blockskill_end(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd = map_id2sd(id);
	if (data <= 0 || data >= MAX_SKILL)
		return 0;
	if (sd) sd->blockskill[data] = 0;
	
	return 1;
}
int pc_blockskill_start (struct map_session_data *sd, int skillid, int tick)
{
	nullpo_retr (-1, sd);

	if (skillid >= 10000 && skillid < 10015)
		skillid -= 9500;
	else if (skillid < 1 || skillid > MAX_SKILL)
		return -1;

	sd->blockskill[skillid] = 1;
	return add_timer(gettick()+tick,pc_blockskill_end,sd->bl.id,skillid);
}

/*==========================================
 * �J?�h?��
 *------------------------------------------
 */
int pc_insert_card(struct map_session_data *sd,int idx_card,int idx_equip)
{
	nullpo_retr(0, sd);

	if(idx_card >= 0 && idx_card < MAX_INVENTORY && idx_equip >= 0 && idx_equip < MAX_INVENTORY && sd->inventory_data[idx_card]) {
		int i;
		int nameid=sd->status.inventory[idx_equip].nameid;
		int cardid=sd->status.inventory[idx_card].nameid;
		int ep=sd->inventory_data[idx_card]->equip;

		if( nameid <= 0 || sd->inventory_data[idx_equip] == NULL ||
			(sd->inventory_data[idx_equip]->type!=4 && sd->inventory_data[idx_equip]->type!=5)||	// ? ������Ȃ�
			( sd->status.inventory[idx_equip].identify==0 ) ||		// ���Ӓ�
			(sd->inventory_data[idx_card]->type!=6)|| // Prevent Hack [Ancyker]
			( sd->status.inventory[idx_equip].card[0]==0x00ff) ||		// ��������
			( sd->status.inventory[idx_equip].card[0]==0x00fe) ||
			( (sd->inventory_data[idx_equip]->equip&ep)==0 ) ||					// ? �����Ⴂ
			( sd->inventory_data[idx_equip]->type==4 && ep==32) ||			// ? �蕐��Ə��J?�h
			( sd->status.inventory[idx_equip].card[0]==(short)0xff00) || sd->status.inventory[idx_equip].equip){

			clif_insert_card(sd,idx_equip,idx_card,1);
			return 0;
		}
		for(i=0;i<sd->inventory_data[idx_equip]->slot;i++){
			if( sd->status.inventory[idx_equip].card[i] == 0){
			// �󂫃X���b�g���������̂ō���?��
				sd->status.inventory[idx_equip].card[i]=cardid;

			// �J?�h�͌��炷
				clif_insert_card(sd,idx_equip,idx_card,0);
				pc_delitem(sd,idx_card,1,1);
				return 0;
			}
		}
	}
	else
		clif_insert_card(sd,idx_equip,idx_card,1);

	return 0;
}

//
// �A�C�e����
//

/*==========================================
 * �X�L���ɂ�锃���l�C��
 *------------------------------------------
 */
int pc_modifybuyvalue(struct map_session_data *sd,int orig_value)
{
	int skill,val = orig_value,rate1 = 0,rate2 = 0;
	if((skill=pc_checkskill(sd,MC_DISCOUNT))>0)	// �f�B�X�J�E���g
		rate1 = 5+skill*2-((skill==10)? 1:0);
	if((skill=pc_checkskill(sd,RG_COMPULSION))>0)	// �R���p���V�����f�B�X�J�E���g
		rate2 = 5+skill*4;
	if(rate1 < rate2) rate1 = rate2;
	if(rate1)
		val = (int)((double)orig_value*(double)(100-rate1)/100.);
	if(val < 0) val = 0;
	if(orig_value > 0 && val < 1) val = 1;

	return val;
}

/*==========================================
 * �X�L���ɂ��?��l�C��
 *------------------------------------------
 */
int pc_modifysellvalue(struct map_session_data *sd,int orig_value)
{
	int skill,val = orig_value,rate = 0;
	if((skill=pc_checkskill(sd,MC_OVERCHARGE))>0)	// �I?�o?�`��?�W
		rate = 5+skill*2-((skill==10)? 1:0);
	if(rate)
		val = (int)((double)orig_value*(double)(100+rate)/100.);
	if(val < 0) val = 0;
	if(orig_value > 0 && val < 1) val = 1;

	return val;
}

/*==========================================
 * �A�C�e���𔃂����bɁA�V�����A�C�e�������g�����A
 * 3�������ɂ����邩�m�F
 *------------------------------------------
 */
int pc_checkadditem(struct map_session_data *sd,int nameid,int amount)
{
	int i;

	nullpo_retr(0, sd);

	if(itemdb_isequip(nameid))
		return ADDITEM_NEW;

	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==nameid){
			if(sd->status.inventory[i].amount+amount > MAX_AMOUNT)
				return ADDITEM_OVERAMOUNT;
			return ADDITEM_EXIST;
		}
	}

	if(amount > MAX_AMOUNT)
		return ADDITEM_OVERAMOUNT;
	return ADDITEM_NEW;
}

/*==========================================
 * �󂫃A�C�e�����̌�?
 *------------------------------------------
 */
int pc_inventoryblank(struct map_session_data *sd)
{
	int i,b;

	nullpo_retr(0, sd);

	for(i=0,b=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==0)
			b++;
	}

	return b;
}

/*==========================================
 * ������?��
 *------------------------------------------
 */
int pc_payzeny(struct map_session_data *sd,int zeny)
{
	double z;

	nullpo_retr(0, sd);

	z = (double)sd->status.zeny;
	if(sd->status.zeny<zeny || z - (double)zeny > MAX_ZENY)
		return 1;
	sd->status.zeny-=zeny;
	clif_updatestatus(sd,SP_ZENY);

	return 0;
}

/*==========================================
 * �����𓾂�
 *------------------------------------------
 */
int pc_getzeny(struct map_session_data *sd,int zeny)
{
	double z;

	nullpo_retr(0, sd);

	z = (double)sd->status.zeny;
	if(z + (double)zeny > MAX_ZENY) {
		zeny = 0;
		sd->status.zeny = MAX_ZENY;
	}
	sd->status.zeny+=zeny;
	clif_updatestatus(sd,SP_ZENY);

	return 0;
}

/*==========================================
 * �A�C�e����T���āA�C���f�b�N�X��Ԃ�
 *------------------------------------------
 */
int pc_search_inventory(struct map_session_data *sd,int item_id)
{
	int i;

	nullpo_retr(-1, sd);

	for(i=0;i<MAX_INVENTORY;i++) {
		if(sd->status.inventory[i].nameid == item_id &&
		 (sd->status.inventory[i].amount > 0 || item_id == 0))
			return i;
	}

	return -1;
}

/*==========================================
 * �A�C�e���ǉ��B��?�̂�item�\��?��?���𖳎�
 *------------------------------------------
 */
int pc_additem(struct map_session_data *sd,struct item *item_data,int amount)
{
	struct item_data *data;
	int i,w;

	nullpo_retr(1, sd);
	nullpo_retr(1, item_data);

	if(item_data->nameid <= 0 || amount <= 0)
		return 1;
	data = itemdb_search(item_data->nameid);
	if((w = data->weight*amount) + sd->weight > sd->max_weight)
		return 2;

	i = MAX_INVENTORY;

	if (!itemdb_isequip2(data)){
		// �� ���i�ł͂Ȃ��̂ŁA�����L�i�Ȃ���̂ݕω�������
		for (i = 0; i < MAX_INVENTORY; i++)
			if(sd->status.inventory[i].nameid == item_data->nameid &&
				sd->status.inventory[i].card[0] == item_data->card[0] &&
				sd->status.inventory[i].card[1] == item_data->card[1] &&
				sd->status.inventory[i].card[2] == item_data->card[2] &&
				sd->status.inventory[i].card[3] == item_data->card[3])
			{
				if (sd->status.inventory[i].amount + amount > MAX_AMOUNT)
					return 5;
				sd->status.inventory[i].amount += amount;
				clif_additem(sd,i,amount,0);
				break;
			}
	}
	if (i >= MAX_INVENTORY){
		// �� ���i�������L�i�������̂ŋ󂫗��֒ǉ�
		i = pc_search_inventory(sd,0);
		if(i >= 0) {
			// clear equips field first, just in case
			if (item_data->equip != 0)
				item_data->equip = 0;
			memcpy(&sd->status.inventory[i], item_data, sizeof(sd->status.inventory[0]));
			sd->status.inventory[i].amount = amount;
			sd->inventory_data[i] = data;
			clif_additem(sd,i,amount,0);
		}
		else return 4;
	}
	sd->weight += w;
	clif_updatestatus(sd,SP_WEIGHT);

	return 0;
}

/*==========================================
 * �A�C�e�������炷
 *------------------------------------------
 */
int pc_delitem(struct map_session_data *sd,int n,int amount,int type)
{
	nullpo_retr(1, sd);

	if(sd->status.inventory[n].nameid==0 || amount <= 0 || sd->status.inventory[n].amount<amount || sd->inventory_data[n] == NULL)
		return 1;

	sd->status.inventory[n].amount -= amount;
	sd->weight -= sd->inventory_data[n]->weight*amount ;
	if(sd->status.inventory[n].amount<=0){
		if(sd->status.inventory[n].equip)
			pc_unequipitem(sd,n,3);
		memset(&sd->status.inventory[n],0,sizeof(sd->status.inventory[0]));
		sd->inventory_data[n] = NULL;
	}
	if(!(type&1))
		clif_delitem(sd,n,amount);
	if(!(type&2))
		clif_updatestatus(sd,SP_WEIGHT);

	return 0;
}

/*==========================================
 * �A�C�e���𗎂�
 *------------------------------------------
 */
int pc_dropitem(struct map_session_data *sd,int n,int amount)
{
	nullpo_retr(1, sd);

	if(n < 0 || n >= MAX_INVENTORY)
		return 1;

	if(amount <= 0)
		return 1;

	if (sd->status.inventory[n].nameid <= 0 ||
	    sd->status.inventory[n].amount < amount ||
	    sd->trade_partner != 0 || sd->vender_id != 0 ||
	    sd->status.inventory[n].amount <= 0)
		return 1;
	
	if (!pc_candrop(sd,sd->status.inventory[n].nameid))
	{	//The client does not likes being silently ignored, so we send it a del of 0 qty
		clif_delitem(sd,n,0);
		clif_displaymessage (sd->fd, msg_txt(263));
		return 1;
	}

	//Logs items, dropped by (P)layers [Lupus]
	if(log_config.pick > 0 )
		log_pick(sd, "P", 0, sd->status.inventory[n].nameid, -amount, (struct item*)&sd->status.inventory[n]);
	//Logs

	if (map_addflooritem(&sd->status.inventory[n], amount, sd->bl.m, sd->bl.x, sd->bl.y, NULL, NULL, NULL, 0) != 0)
		pc_delitem(sd, n, amount, 0);
	else
		clif_delitem(sd,n,0);

	return 0;
}

/*==========================================
 * �A�C�e�����E��
 *------------------------------------------
 */
int pc_takeitem(struct map_session_data *sd,struct flooritem_data *fitem)
{
	int flag;
	unsigned int tick = gettick();
	struct map_session_data *first_sd = NULL,*second_sd = NULL,*third_sd = NULL;

	nullpo_retr(0, sd);
	nullpo_retr(0, fitem);

	if(distance(fitem->bl.x,fitem->bl.y,sd->bl.x,sd->bl.y)>2)
		return 0;	// ����������

	if(fitem->first_get_id > 0) {
		first_sd = map_id2sd(fitem->first_get_id);
		if(tick < fitem->first_get_tick) {
			if(fitem->first_get_id != sd->bl.id && !(first_sd && first_sd->status.party_id == sd->status.party_id)) {
				clif_additem(sd,0,0,6);
				return 0;
			}
		}
		else if(fitem->second_get_id > 0) {
			second_sd = map_id2sd(fitem->second_get_id);
			if(tick < fitem->second_get_tick) {
				if(fitem->first_get_id != sd->bl.id && fitem->second_get_id != sd->bl.id &&
					!(first_sd && first_sd->status.party_id == sd->status.party_id) && !(second_sd && second_sd->status.party_id == sd->status.party_id)) {
					clif_additem(sd,0,0,6);
					return 0;
				}
			}
			else if(fitem->third_get_id > 0) {
				third_sd = map_id2sd(fitem->third_get_id);
				if(tick < fitem->third_get_tick) {
					if(fitem->first_get_id != sd->bl.id && fitem->second_get_id != sd->bl.id && fitem->third_get_id != sd->bl.id &&
						!(first_sd && first_sd->status.party_id == sd->status.party_id) && !(second_sd && second_sd->status.party_id == sd->status.party_id) &&
						!(third_sd && third_sd->status.party_id == sd->status.party_id)) {
						clif_additem(sd,0,0,6);
						return 0;
					}
				}
			}
		}
	}
	if((flag = pc_additem(sd,&fitem->item_data,fitem->item_data.amount)))
		// �d��over�Ŏ擾���s
		clif_additem(sd,0,0,flag);
	else {
		/* �擾���� */
		if(sd->attacktimer != -1)
			pc_stopattack(sd);

		//Logs items, taken by (P)layers [Lupus]
		if(log_config.pick > 0 )
			log_pick(sd, "P", 0, fitem->item_data.nameid, fitem->item_data.amount, (struct item*)&fitem->item_data);
		//Logs

		clif_takeitem(&sd->bl,&fitem->bl);
		map_clearflooritem(fitem->bl.id);
	}
	return 0;
}

int pc_isUseitem(struct map_session_data *sd,int n)
{
	struct item_data *item;
	int nameid;

	nullpo_retr(0, sd);

	item = sd->inventory_data[n];
	nameid = sd->status.inventory[n].nameid;

	if(item == NULL)
		return 0;
	if(item->type != 0 && item->type != 2)
		return 0;
	if((nameid == 605) && map[sd->bl.m].flag.gvg)
		return 0;
	if(nameid == 601 && (map[sd->bl.m].flag.noteleport || map[sd->bl.m].flag.gvg)) {
		clif_skill_teleportmessage(sd,0);
		return 0;
	}
	if(nameid == 602 && map[sd->bl.m].flag.noreturn)
		return 0;
	//Dead Branch & Bloody Branch
	if((nameid == 604 || nameid == 12103) && (map[sd->bl.m].flag.nobranch || map[sd->bl.m].flag.gvg))
		return 0;
	if(item->sex != 2 && sd->status.sex != item->sex)
		return 0;
	if(item->elv > 0 && sd->status.base_level < item->elv)
		return 0;

	//Phew, this way is much cleaner to check for valid jobs :3 [Skotlex]
	//Note that items for mounted chars alone will never be usable as items for non-mounted chars apply to both mounted/non-mounted chars.
	if ((1<<(pc_calc_base_job2(sd->status.class_)) & item->class_) == 0)
		return 0;
	//Dead Branch & Bloody Branch
	if((log_config.branch > 0) && (nameid == 604 || nameid == 12103))
		log_branch(sd);

	return 1;
}

/*==========================================
 * �A�C�e�����g��
 *------------------------------------------
 */
int pc_useitem(struct map_session_data *sd,int n)
{
	int amount;

	nullpo_retr(1, sd);

	if(n >=0 && n < MAX_INVENTORY) {
		unsigned char *script;
		sd->itemid = sd->status.inventory[n].nameid;
		sd->itemindex = n;
		amount = sd->status.inventory[n].amount;
		if(sd->status.inventory[n].nameid <= 0 ||
			sd->status.inventory[n].amount <= 0 ||
			gettick() < sd->canuseitem_tick || //Prevent mass item usage. [Skotlex]
			sd->sc_data[SC_BERSERK].timer!=-1 ||
			sd->sc_data[SC_MARIONETTE].timer!=-1 ||
			sd->sc_data[SC_GRAVITATION].timer!=-1 ||
			(pc_issit(sd) && (sd->itemid == 605 || sd->itemid == 606)) ||
			//added item_noequip.txt items check by Maya&[Lupus]
			(map[sd->bl.m].flag.pvp && (sd->inventory_data[n]->flag.no_equip&1) ) || // PVP
			(map[sd->bl.m].flag.gvg && (sd->inventory_data[n]->flag.no_equip>1) ) || // GVG
			!pc_isUseitem(sd,n) ) {
			clif_useitemack(sd,n,0,0);
			return 1;
		}
		script = sd->inventory_data[n]->use_script;
		amount = sd->status.inventory[n].amount;
		//Check if the item is to be consumed inmediately [Skotlex]
		if (sd->inventory_data[n]->flag.delay_consume)
			clif_useitemack(sd,n,amount,1);
		else {
			clif_useitemack(sd,n,amount-1,1);

			//Logs (C)onsumable items [Lupus]
			if(log_config.pick > 0 ) {
				log_pick(sd, "C", 0, sd->status.inventory[n].nameid, -1, &sd->status.inventory[n]);
			}
			//Logs

			pc_delitem(sd,n,1,1);
		}
		if(sd->status.inventory[n].card[0]==0x00fe && pc_istop10fame(MakeDWord(sd->status.inventory[n].card[2],sd->status.inventory[n].card[3]),JOB_ALCHEMIST))
		    sd->state.potion_flag = 2; // Famous player's potions have 50% more efficiency
		sd->canuseitem_tick= gettick() + battle_config.item_use_interval; //Update item use time.
		run_script(script,0,sd->bl.id,0);
		sd->state.potion_flag = 0;
	}

	return 0;
}

/*==========================================
 * �J?�g�A�C�e���ǉ��B��?�̂�item�\��?��?���𖳎�
 *------------------------------------------
 */
int pc_cart_additem(struct map_session_data *sd,struct item *item_data,int amount)
{
	struct item_data *data;
	int i,w;

	nullpo_retr(1, sd);
	nullpo_retr(1, item_data);

	if(item_data->nameid <= 0 || amount <= 0)
		return 1;
	data = itemdb_search(item_data->nameid);

	if(!itemdb_cancartstore(item_data->nameid, pc_isGM(sd)))
	{	//Check item trade restrictions	[Skotlex]
		clif_displaymessage (sd->fd, msg_txt(264));
		return 1;
	}

	if((w=data->weight*amount) + sd->cart_weight > sd->cart_max_weight)
		return 1;

	i=MAX_CART;
	if(!itemdb_isequip2(data)){
		// �� ���i�ł͂Ȃ��̂ŁA�����L�i�Ȃ���̂ݕω�������
		for(i=0;i<MAX_CART;i++){
			if(sd->status.cart[i].nameid==item_data->nameid &&
				sd->status.cart[i].card[0] == item_data->card[0] && sd->status.cart[i].card[1] == item_data->card[1] &&
				sd->status.cart[i].card[2] == item_data->card[2] && sd->status.cart[i].card[3] == item_data->card[3]){
				if(sd->status.cart[i].amount+amount > MAX_AMOUNT)
					return 1;
				sd->status.cart[i].amount+=amount;
				clif_cart_additem(sd,i,amount,0);
				break;
			}
		}
	}
	if(i >= MAX_CART){
		// �� ���i�������L�i�������̂ŋ󂫗��֒ǉ�
		for(i=0;i<MAX_CART;i++){
			if(sd->status.cart[i].nameid==0){
				memcpy(&sd->status.cart[i],item_data,sizeof(sd->status.cart[0]));
				sd->status.cart[i].amount=amount;
				sd->cart_num++;
				clif_cart_additem(sd,i,amount,0);
				break;
			}
		}
		if(i >= MAX_CART)
			return 1;
	}
	sd->cart_weight += w;
	clif_updatestatus(sd,SP_CARTINFO);

	return 0;
}

/*==========================================
 * �J?�g�A�C�e�������炷
 *------------------------------------------
 */
int pc_cart_delitem(struct map_session_data *sd,int n,int amount,int type)
{
	nullpo_retr(1, sd);

	if(sd->status.cart[n].nameid==0 ||
	   sd->status.cart[n].amount<amount)
		return 1;

	sd->status.cart[n].amount -= amount;
	sd->cart_weight -= itemdb_weight(sd->status.cart[n].nameid)*amount ;
	if(sd->status.cart[n].amount <= 0){
		memset(&sd->status.cart[n],0,sizeof(sd->status.cart[0]));
		sd->cart_num--;
	}
	if(!type) {
		clif_cart_delitem(sd,n,amount);
		clif_updatestatus(sd,SP_CARTINFO);
	}

	return 0;
}

/*==========================================
 * �J?�g�փA�C�e���ړ�
 *------------------------------------------
 */
int pc_putitemtocart(struct map_session_data *sd,int idx,int amount) {
	struct item *item_data;

	nullpo_retr(0, sd);
	nullpo_retr(0, item_data = &sd->status.inventory[idx]);

	if (item_data->nameid==0 || item_data->amount<amount || sd->vender_id)
		return 1;

	if (pc_cart_additem(sd,item_data,amount) == 0)
		return pc_delitem(sd,idx,amount,0);

	return 1;
}

/*==========================================
 * �J?�g?�̃A�C�e��?�m�F(��?�̍�����Ԃ�)
 *------------------------------------------
 */
int pc_cartitem_amount(struct map_session_data *sd,int idx,int amount)
{
	struct item *item_data;

	nullpo_retr(-1, sd);
	nullpo_retr(-1, item_data=&sd->status.cart[idx]);

	if( item_data->nameid==0 || !item_data->amount)
		return -1;
	return item_data->amount-amount;
}
/*==========================================
 * �J?�g����A�C�e���ړ�
 *------------------------------------------
 */

int pc_getitemfromcart(struct map_session_data *sd,int idx,int amount)
{
	struct item *item_data;
	int flag;

	nullpo_retr(0, sd);
	nullpo_retr(0, item_data=&sd->status.cart[idx]);

	if( item_data->nameid==0 || item_data->amount<amount || sd->vender_id )
		return 1;
	if((flag = pc_additem(sd,item_data,amount)) == 0)
		return pc_cart_delitem(sd,idx,amount,0);

	clif_additem(sd,0,0,flag);
	return 1;
}

/*==========================================
 * �X�e�B���i���J
 *------------------------------------------
 */
int pc_show_steal(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	int itemid;
	int type;

	struct item_data *item=NULL;
	char output[100];

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=va_arg(ap,struct map_session_data *));

	itemid=va_arg(ap,int);
	type=va_arg(ap,int);

	if(!type){
		if((item=itemdb_exists(itemid))==NULL)
			sprintf(output,"%s stole an Unknown Item.",sd->status.name);
		else
			sprintf(output,"%s stole %s.",sd->status.name,item->jname);
		clif_displaymessage( ((struct map_session_data *)bl)->fd, output);
	}else{
		sprintf(output,"%s has not stolen the item because of being overweight.",sd->status.name);
		clif_displaymessage( ((struct map_session_data *)bl)->fd, output);
	}

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
//** pc.c: Small Steal Item fix by fritz
int pc_steal_item(struct map_session_data *sd,struct block_list *bl)
{
	int log_item[10]; //for stolen items logging Lupus

	if(sd != NULL && bl != NULL && bl->type == BL_MOB) {
		int i,skill,itemid,flag;
		struct mob_data *md;
		md=(struct mob_data *)bl;

		if(!md->state.steal_flag && md->db->mexp <= 0 && !(md->db->mode&0x20) &&
			(!md->master_id) && //Prevent stealing from summoned creatures. [Skotlex]
			(!(md->class_>=1324 && md->class_<1364)) && // prevent stealing from treasure boxes [Valaris]
			(!map[md->bl.m].flag.nomobloot))        // check noloot map flag [Lorky]
		{
			if (md->sc_data && (md->sc_data[SC_STONE].timer != -1 || md->sc_data[SC_FREEZE].timer != -1))
				return 0;
			skill = battle_config.skill_steal_type == 1
				? (sd->paramc[4] - md->db->dex)/2 + pc_checkskill(sd,TF_STEAL)*6 + 10
				: sd->paramc[4] - md->db->dex + pc_checkskill(sd,TF_STEAL)*3 + 10;

			if(0 < skill)
			{
				i = rand()%10; //8 -> 10 Lupus
				itemid = md->db->dropitem[i].nameid;

				if(itemid > 0 && (itemdb_type(itemid) != 6 || pc_checkskill(sd,TF_STEAL) > 5))
				{
					//fixed rate. From Freya [Lupus]
					if (rand() % 10000 < ((md->db->dropitem[i].p * skill) / 100 + sd->add_steal_rate))
					{
						struct item tmp_item;
						memset(&tmp_item,0,sizeof(tmp_item));
						tmp_item.nameid = itemid;
						tmp_item.amount = 1;
						tmp_item.identify = !itemdb_isequip3(itemid);
						flag = pc_additem(sd,&tmp_item,1);

						//Logs items, Stolen from mobs [Lupus]
						if(log_config.pick > 0 ) {
							log_pick((struct map_session_data*)md, "M", md->class_, itemid, -1, NULL);
							log_pick(sd, "P", 0, itemid, 1, NULL);
						}
						//Logs

						//this drop log contains ALL stolen items [Lupus]
						if(log_config.steal) { //we check were there any drops.. and if not - don't write the log
							memset(&log_item,0,sizeof(log_item));
							log_item[i] = itemid; //i == monster's drop slot
							log_drop(sd, md->class_, log_item);
						}

						//A Rare Steal Global Announce by Lupus
						if(md->db->dropitem[i].p<=battle_config.rare_drop_announce) {
							struct item_data *i_data;
							char message[128];
							i_data = itemdb_exists(itemid);
							sprintf (message, msg_txt(542), (sd->status.name != NULL)?sd->status.name :"GM", md->db->jname, i_data->jname, (float)md->db->dropitem[i].p/100);
							//MSG: "'%s' stole %s's %s (chance: %%%0.02f)"
							intif_GMmessage(message,strlen(message)+1,0);
						}

						if(battle_config.show_steal_in_same_party)
						{
							party_foreachsamemap(pc_show_steal,sd,1,sd,tmp_item.nameid,0);
						}
							if(flag)
						{
							if(battle_config.show_steal_in_same_party)
							{
								party_foreachsamemap(pc_show_steal,sd,1,sd,tmp_item.nameid,1);
							}
								clif_additem(sd,0,0,flag);
						}
						md->state.steal_flag = 1;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_steal_coin(struct map_session_data *sd,struct block_list *bl)
{
	if(sd != NULL && bl != NULL && bl->type == BL_MOB) {
		int rate,skill;
		struct mob_data *md=(struct mob_data *)bl;
		if(md && !md->state.steal_coin_flag) {
			if (md->sc_data && (md->sc_data[SC_STONE].timer != -1 || md->sc_data[SC_FREEZE].timer != -1))
				return 0;
			skill = pc_checkskill(sd,RG_STEALCOIN)*10;
			rate = skill + (sd->status.base_level - md->db->lv)*3 + sd->paramc[4]*2 + sd->paramc[5]*2;
			if(rand()%1000 < rate) {
				pc_getzeny(sd,md->db->lv*10 + rand()%100);
				md->state.steal_coin_flag = 1;
				return 1;
			}
		}
	}

	return 0;
}
//
//
//
/*==========================================
 * PC�̈ʒu�ݒ�
 *------------------------------------------
 */
int pc_setpos(struct map_session_data *sd,char *mapname_org,int x,int y,int clrtype)
{
	char mapname[MAP_NAME_LENGTH];
	int m;

	nullpo_retr(0, sd);

	if(sd->chatID)	// �`���b�g����o��
		chat_leavechat(sd);
	if(sd->trade_partner)	// �����?����
		trade_tradecancel(sd);
	if(sd->state.storage_flag == 1)
		storage_storage_quit(sd);	// �q�ɂ��J���Ă�Ȃ�ۑ�����
	else if (sd->state.storage_flag == 2)
		storage_guild_storage_quit(sd,0);

	if(sd->party_invite>0)	// �p?�e�B?�U�����ۂ���
		party_reply_invite(sd,sd->party_invite_account,0);
	if(sd->guild_invite>0)	// �M���h?�U�����ۂ���
		guild_reply_invite(sd,sd->guild_invite,0);
	if(sd->guild_alliance>0)	// �M���h����?�U�����ۂ���
		guild_reply_reqalliance(sd,sd->guild_alliance_account,0);

	skill_castcancel(&sd->bl,0);	// �r����?
	pc_stop_walking(sd,0);		// ?�s��?
	pc_stopattack(sd);			// �U?��?

	if(pc_issit(sd)) {
		pc_setstand(sd);
		skill_gangsterparadise(sd,0);
	}

	if (sd->sc_count) {
		if(sd->sc_data[SC_TRICKDEAD].timer != -1)
			status_change_end(&sd->bl, SC_TRICKDEAD, -1);
		if(sd->sc_data[SC_BLADESTOP].timer!=-1)
			status_change_end(&sd->bl,SC_BLADESTOP,-1);
		if(sd->sc_data[SC_DANCING].timer!=-1) // clear dance effect when warping [Valaris]
			skill_stop_dancing(&sd->bl);
		if (sd->sc_data[SC_BASILICA].timer!=-1) {
			struct skill_unit_group *sg = (struct skill_unit_group *)sd->sc_data[SC_BASILICA].val4;
			if (sg && sg->src_id == sd->bl.id)
				skill_delunitgroup (sg);
			status_change_end(&sd->bl,SC_BASILICA,-1);
		}
		if (sd->sc_data[SC_DEVOTION].timer!=-1)
			status_change_end(&sd->bl,SC_DEVOTION,-1);
	}

	if(sd->status.option&2)
		status_change_end(&sd->bl, SC_HIDING, -1);
	if(pc_iscloaking(sd))
		status_change_end(&sd->bl, SC_CLOAKING, -1);
	if(pc_ischasewalk(sd))
		status_change_end(&sd->bl, SC_CHASEWALK, -1);

	if(sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 0) {
		pet_stopattack(sd->pd);
		pet_changestate(sd->pd,MS_IDLE,0);
	}

	memcpy(mapname, mapname_org, MAP_NAME_LENGTH-1);
	mapname[MAP_NAME_LENGTH-1]= '\0';
	if(strstr(mapname,".gat")==NULL && strstr(mapname,".afm")==NULL && strlen(mapname)<MAP_NAME_LENGTH-5){	//It has to be -5 for a .gat to fit!(5th is \0) [Skotlex]
		strcat(mapname,".gat");
	}

	m=map_mapname2mapid(mapname);

	if(m<0){
		if(sd->mapname[0]){
			int ip,port;
			if(map_mapname2ipport(mapname,&ip,&port)==0){
				skill_stop_dancing(&sd->bl);
				skill_unit_move(&sd->bl,gettick(),4);
				clif_clearchar_area(&sd->bl,clrtype&0xffff);
				skill_gangsterparadise(sd,0);
				map_delblock(&sd->bl);
				if(sd->status.pet_id > 0 && sd->pd) {
					if(sd->pd->bl.m != m && sd->pet.intimate <= 0) {
						pet_remove_map(sd);
						intif_delete_petdata(sd->status.pet_id);
						sd->status.pet_id = 0;
						sd->pd = NULL;
						sd->petDB = NULL;
						if(battle_config.pet_status_support)
							status_calc_pc(sd,2);
					}
					else if(sd->pet.intimate > 0) {
						pet_stopattack(sd->pd);
						pet_changestate(sd->pd,MS_IDLE,0);
						clif_clearchar_area(&sd->pd->bl,clrtype&0xffff);
						map_delblock(&sd->pd->bl);
					}
				}
				memcpy(sd->mapname, mapname, MAP_NAME_LENGTH-1);
				sd->bl.x=x;
				sd->bl.y=y;
				sd->state.waitingdisconnect=1;
				pc_clean_skilltree(sd);

				if(sd->status.pet_id > 0 && sd->pd)
					intif_save_petdata(sd->status.account_id,&sd->pet);
				//The storage close routines save the char data. [Skotlex]
				if (!sd->state.storage_flag)
					chrif_save(sd);
				else if (sd->state.storage_flag == 1)
				{
					storage_storageclose(sd);
					storage_delete(sd->status.account_id);
				}
				else if (sd->state.storage_flag == 2)
					storage_guild_storageclose(sd);
					
				chrif_changemapserver(sd, mapname, x, y, ip, (short)port);
				return 0;
			}
		}
#if 0
		clif_authfail_fd(sd->fd,0);	// cancel
		clif_setwaitclose(sd->fd);
#endif
		return 1;
	}

	if(x <0 || x >= map[m].xs || y <0 || y >= map[m].ys)
		x=y=0;
	if((x==0 && y==0) || map_getcell(m,x,y,CELL_CHKNOPASS)){
		if(x||y) {
			if(battle_config.error_log)
				ShowError("pc_setpos: stacked (%d,%d)\n",x,y);
		}
		do {
			x=rand()%(map[m].xs-2)+1;
			y=rand()%(map[m].ys-2)+1;
		} while(map_getcell(m,x,y,CELL_CHKNOPASS));
	}

	if(sd->mapname[0] && sd->bl.prev != NULL){
		skill_unit_move(&sd->bl,gettick(),4);
		clif_clearchar_area(&sd->bl,clrtype&0xffff);
		skill_gangsterparadise(sd,0);
		
		map_delblock(&sd->bl);
		// pet
		if(sd->status.pet_id > 0 && sd->pd) {
			if(sd->pd->bl.m != m && sd->pet.intimate <= 0) {
				pet_remove_map(sd);
				intif_delete_petdata(sd->status.pet_id);
				sd->status.pet_id = 0;
				sd->pd = NULL;
				sd->petDB = NULL;
				if(battle_config.pet_status_support)
					status_calc_pc(sd,2);
				pc_clean_skilltree(sd);

				//The storage functions will save the char. [Skotlex]
				if (!sd->state.storage_flag)
					chrif_save(sd);
				else if (sd->state.storage_flag == 1)
					storage_storageclose(sd);
				else if (sd->state.storage_flag == 2)
					storage_guild_storageclose(sd);
			}
			else if(sd->pet.intimate > 0) {
				pet_stopattack(sd->pd);
				pet_changestate(sd->pd,MS_IDLE,0);
				clif_clearchar_area(&sd->pd->bl,clrtype&0xffff);
				map_delblock(&sd->pd->bl);
			}
		}
		clif_changemap(sd,map[m].name,x,y); // [MouseJstr]
	}
		
	if (strcmp(sd->mapname,mapname)!=0) //minimap dot fix [Kevin]
	{
		party_send_dot_remove(sd);
		guild_send_dot_remove(sd);
	}

	memcpy(sd->mapname, mapname, MAP_NAME_LENGTH-1);
	sd->bl.m = m;
	sd->to_x = x;
	sd->to_y = y;

	// moved and changed dance effect stopping

	sd->bl.x =  x;
	sd->bl.y =  y;

	if(sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 0) {
		sd->pd->bl.m = m;
		sd->pd->bl.x = sd->pd->to_x = x;
		sd->pd->bl.y = sd->pd->to_y = y;
		sd->pd->dir = sd->dir;
	}

//	map_addblock(&sd->bl);	/// �u���b�N�o?��spawn��
//	clif_spawnpc(sd);

	return 0;
}

/*==========================================
 * PC�̃����_����?�v
 *------------------------------------------
 */
int pc_randomwarp(struct map_session_data *sd, int type) {
	int x,y,i=0;
	int m;

	nullpo_retr(0, sd);

	m=sd->bl.m;

	if (map[sd->bl.m].flag.noteleport)	// �e���|?�g�֎~
		return 0;

	do{
		x=rand()%(map[m].xs-2)+1;
		y=rand()%(map[m].ys-2)+1;
	}while(map_getcell(m,x,y,CELL_CHKNOPASS) && (i++)<1000 );

	if (i < 1000)
		pc_setpos(sd,map[m].name,x,y,type);

	return 0;
}

/*==========================================
 * ���݈ʒu�̃���
 *------------------------------------------
 */
int pc_memo(struct map_session_data *sd, int i) {
	int skill;
	int j;

	nullpo_retr(0, sd);

	skill = pc_checkskill(sd, AL_WARP);

	if (i >= MIN_PORTAL_MEMO)
		i -= MIN_PORTAL_MEMO;
	else if (map[sd->bl.m].flag.nomemo || (map[sd->bl.m].flag.nowarpto && battle_config.any_warp_GM_min_level > pc_isGM(sd))) {
		clif_skill_teleportmessage(sd, 1);
		return 0;
	}

	if (skill < 1) {
		clif_skill_memo(sd,2);
	}

	if (skill < 2 || i < -1 || i > 2) {
		clif_skill_memo(sd, 1);
		return 0;
	}

	for(j = 0 ; j < 3; j++) {
		if (strcmp(sd->status.memo_point[j].map, map[sd->bl.m].name) == 0) {
			i = j;
			break;
		}
	}

	if (i == -1) {
		for(i = skill - 3; i >= 0; i--) {
			memcpy(&sd->status.memo_point[i+1],&sd->status.memo_point[i],
				sizeof(struct point));
		}
		i = 0;
	}
	memcpy(sd->status.memo_point[i].map, map[sd->bl.m].name, MAP_NAME_LENGTH-1);
	sd->status.memo_point[i].x = sd->bl.x;
	sd->status.memo_point[i].y = sd->bl.y;

	clif_skill_memo(sd, 0);

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_can_reach(struct map_session_data *sd,int x,int y)
{
	struct walkpath_data wpd;

	nullpo_retr(0, sd);

	if( sd->bl.x==x && sd->bl.y==y )	// �����}�X
		return 1;

	// ��Q������
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	return (path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,x,y,0)!=-1)?1:0;
}

//
// ? �s��
//
/*==========================================
 * ����1?�ɂ�����jԂ��v�Z
 *------------------------------------------
 */
static int calc_next_walk_step(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->walkpath.path_pos>=sd->walkpath.path_len)
		return -1;
	if(sd->walkpath.path[sd->walkpath.path_pos]&1)
		return sd->speed*14/10;

	return sd->speed;
}

/*==========================================
 * ��?�i��(timer??)
 *------------------------------------------
 */
static int pc_walk(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	int i, x, y, dx, dy;
	int moveblock;

	if ((sd = map_id2sd(id)) == NULL)
		return 0;

	if(sd->walktimer != tid){
		if(battle_config.error_log)
			ShowError("pc_walk %d != %d\n", sd->walktimer, tid);
		return 0;
	}

	sd->walktimer = -1;
	if (sd->walkpath.path_pos >= sd->walkpath.path_len ||
		sd->walkpath.path_pos != data)
		return 0;

	//?�����̂ő����̃^�C�}?��������
	sd->inchealspirithptick = 0;
	sd->inchealspiritsptick = 0;

	sd->walkpath.path_half ^= 1;
	if (sd->walkpath.path_half == 0) { // �}�X�ڒ��S�֓r
		sd->walkpath.path_pos++;
		if (sd->state.change_walk_target) {
			pc_walktoxy_sub(sd);
			return 0;
		}
	} else { // �}�X�ڋ��E�֓r
		if (sd->walkpath.path[sd->walkpath.path_pos] >= 8)
			return 1;
		x = sd->bl.x;
		y = sd->bl.y;
		if (map_getcell(sd->bl.m,x,y,CELL_CHKNOPASS)) {
			pc_stop_walking(sd,1);
			return 0;
		}
		sd->dir = sd->head_dir = sd->walkpath.path[sd->walkpath.path_pos];
		dx = dirx[(int)sd->dir];
		dy = diry[(int)sd->dir];
		if (map_getcell(sd->bl.m,x,y,CELL_CHKNOPASS)) {
			pc_walktoxy_sub(sd);
			return 0;
		}
		if (skill_check_moonlit (&sd->bl,x+dx,y+dy)) {
			pc_stop_walking(sd,1);
			return 0;
		}
		moveblock = ( x/BLOCK_SIZE != (x+dx)/BLOCK_SIZE || y/BLOCK_SIZE != (y+dy)/BLOCK_SIZE);

		sd->walktimer = 1;	// temporarily set (so that in clif_set007x the player will still appear as walking)
		map_foreachinmovearea(clif_pcoutsight, sd->bl.m,
			x-AREA_SIZE, y-AREA_SIZE, x+AREA_SIZE, y+AREA_SIZE,
			dx, dy, 0, sd);
		x += dx;
		y += dy;

		sd->walktimer = -1;	// set back so not to disturb future pc_stop_walking calls
		skill_unit_move(&sd->bl,tick,2);
		if (moveblock) map_delblock(&sd->bl);
		sd->bl.x = x;
		sd->bl.y = y;
		if (moveblock) map_addblock(&sd->bl);
		skill_unit_move(&sd->bl,tick,3);

		sd->walktimer = 1;	// temporarily set (so that in clif_set007x the player will still appear as walking)
		map_foreachinmovearea (clif_pcinsight, sd->bl.m,
			x-AREA_SIZE, y-AREA_SIZE, x+AREA_SIZE, y+AREA_SIZE,
			-dx, -dy, 0, sd);
		sd->walktimer = -1;	// set back so not to disturb future pc_stop_walking calls

		if (sd->status.party_id > 0) {	// �p?�e�B�̂g�o���ʒm?��
			struct party *p = party_search(sd->status.party_id);
			if (p != NULL) {
				int p_flag = 0;
				map_foreachinmovearea (party_send_hp_check, sd->bl.m,
					x-AREA_SIZE, y-AREA_SIZE, x+AREA_SIZE, y+AREA_SIZE,
					-dx, -dy, BL_PC, sd->status.party_id, &p_flag);
				if (p_flag)
					sd->party_hp = -1;
			}
		}

		if (pc_iscloaking(sd))	// �N��?�L���O�̏���?��
			skill_check_cloaking(&sd->bl);
		/* ��f�B�{?�V����?�� */
		if (sd->sc_count) {
			if (sd->sc_data[SC_DANCING].timer != -1)
				skill_unit_move_unit_group((struct skill_unit_group *)sd->sc_data[SC_DANCING].val2, sd->bl.m, dx, dy);
		}
		if (map_getcell(sd->bl.m,x,y,CELL_CHKNPC))
			npc_touch_areanpc(sd,sd->bl.m,x,y);
		else
			sd->areanpc_id = 0;
	}

	if ((i = calc_next_walk_step(sd)) > 0) {
		i = i>>1;
		if (i < 1 && sd->walkpath.path_half == 0)
			i = 1;
		sd->walktimer = add_timer (tick+i, pc_walk, id, sd->walkpath.path_pos);
	}

	if (battle_config.disp_hpmeter)
		clif_hpmeter(sd);

	return 0;
}

/*==========================================
 * �ړ��\���m�F���āA�\�Ȃ�?�s�J�n
 *------------------------------------------
 */
static int pc_walktoxy_sub (struct map_session_data *sd)
{
	struct walkpath_data wpd;
	int i;

	nullpo_retr(1, sd);

	if (path_search(&wpd, sd->bl.m, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y, 0))
		return 1;
	memcpy(&sd->walkpath, &wpd, sizeof(wpd));

	clif_walkok(sd);
	sd->state.change_walk_target = 0;

	if ((i = calc_next_walk_step(sd)) > 0){
		i = i >> 2;
		sd->walktimer = add_timer(gettick()+i, pc_walk, sd->bl.id, 0);
	}
	clif_movechar(sd);

	return 0;
}

/*==========================================
 * pc? �s�v��
 *------------------------------------------
 */
int pc_walktoxy (struct map_session_data *sd, int x, int y)
{
	nullpo_retr(0, sd);

	sd->to_x = x;
	sd->to_y = y;

	if (sd->walktimer != -1 /*&& sd->state.change_walk_target == 0*/)
	{	//There was a timer-mismatch here. pc_walktoxy_sub does not clears previous pc_walk timers! [Skotlex]
		sd->state.change_walk_target = 1;
	} else {
		pc_walktoxy_sub(sd);
	}

	if (sd->state.gmaster_flag > 0) {
		struct guild *g = (struct guild *)sd->state.gmaster_flag;
		if (g) {
			int skill, guildflag = 0;
			if ((skill = guild_checkskill(g, GD_LEADERSHIP)) > 0) guildflag |= skill<<12;
			if ((skill = guild_checkskill(g, GD_GLORYWOUNDS)) > 0) guildflag |= skill<<8;
			if ((skill = guild_checkskill(g, GD_SOULCOLD)) > 0) guildflag |= skill<<4;
			if ((skill = guild_checkskill(g, GD_HAWKEYES)) > 0) guildflag |= skill;
			if (guildflag)
				map_foreachinarea (skill_guildaura_sub, sd->bl.m,
					sd->bl.x-2, sd->bl.y-2, sd->bl.x+2, sd->bl.y+2, BL_PC,
					sd->bl.id, sd->status.guild_id, &guildflag);
		}
	}

	return 0;
}

/*==========================================
 * ? �s��~
 *------------------------------------------
 */
int pc_stop_walking (struct map_session_data *sd, int type)
{
	nullpo_retr(0, sd);

	if (sd->walktimer != -1) {
		delete_timer(sd->walktimer, pc_walk);
		sd->walktimer = -1;
	}
	sd->walkpath.path_len = 0;
	sd->to_x = sd->bl.x;
	sd->to_y = sd->bl.y;
	if (type & 0x01)
		clif_fixpos(&sd->bl);
	return 0;
}

/*==========================================
 * Random walk
 *------------------------------------------
 */
int pc_randomwalk (struct map_session_data *sd, int tick)
{
	const int retrycount = 20;
	nullpo_retr(0, sd);

	if (DIFF_TICK(sd->next_walktime, tick) < 0) {
		int i, x, y, d;
		d = rand() % 7 + 5;
		for (i = 0; i < retrycount; i++) {	// Search of a movable place
			int r = rand();
			x = sd->bl.x + r % (d*2+1) - d;
			y = sd->bl.y + r / (d*2+1) % (d*2+1) - d;
			if ((map_getcell(sd->bl.m, x, y, CELL_CHKPASS)) &&
				pc_walktoxy(sd, x, y) == 0)
				break;
		}
		// Working on this part later [celest]
		/*for(i=c=0;i<sd->walkpath.path_len;i++){	// The next walk start time is calculated.
			if(sd->walkpath.path[i]&1)
				c+=sd->speed*14/10;
			else
				c+=sd->speed;
		}
		sd->next_walktime = (d=tick+rand()%3000+c);
		return d;*/
		return 1;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_movepos(struct map_session_data *sd,int dst_x,int dst_y,int checkpath)
{
	int moveblock;
	int dx,dy;
	int tick = gettick();

	struct walkpath_data wpd;

	nullpo_retr(0, sd);

	if(checkpath && path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,dst_x,dst_y,0))
		return 1;

	sd->dir = sd->head_dir = map_calc_dir(&sd->bl, dst_x,dst_y);

	dx = dst_x - sd->bl.x;
	dy = dst_y - sd->bl.y;

	moveblock = ( sd->bl.x/BLOCK_SIZE != dst_x/BLOCK_SIZE || sd->bl.y/BLOCK_SIZE != dst_y/BLOCK_SIZE);

	map_foreachinmovearea(clif_pcoutsight,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,dx,dy,0,sd);

	skill_unit_move(&sd->bl,tick,2);
	if(moveblock) map_delblock(&sd->bl);
	sd->bl.x = dst_x;
	sd->bl.y = dst_y;
	if(moveblock) map_addblock(&sd->bl);
	skill_unit_move(&sd->bl,tick,3);

	map_foreachinmovearea(clif_pcinsight,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,-dx,-dy,0,sd);

	if(sd->status.party_id>0){	// �p?�e�B�̂g�o���ʒm?��
		struct party *p=party_search(sd->status.party_id);
		if(p!=NULL){
			int flag=0;
			map_foreachinmovearea(party_send_hp_check,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,-dx,-dy,BL_PC,sd->status.party_id,&flag);
			if(flag)
				sd->party_hp=-1;
		}
	}

	if (pc_iscloaking(sd)) // �N��?�L���O�̏���?��
		skill_check_cloaking(&sd->bl);

	if(map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKNPC))
		npc_touch_areanpc(sd,sd->bl.m,sd->bl.x,sd->bl.y);
	else
		sd->areanpc_id=0;
	return 0;
}

//
// ����??
//
/*==========================================
 * �X�L����?�� ���L���Ă����ꍇLv���Ԃ�
 *------------------------------------------
 */
int pc_checkskill(struct map_session_data *sd,int skill_id)
{
	if(sd == NULL) return 0;
	if( skill_id>=10000 ){
		struct guild *g;
		if( sd->status.guild_id>0 && (g=guild_search(sd->status.guild_id))!=NULL)
			return guild_checkskill(g,skill_id);
		return 0;
	}

	if(sd->status.skill[skill_id].id == skill_id)
		return (sd->status.skill[skill_id].lv);

	return 0;
}

/*==========================================
 * ����?�X�ɂ��X�L����??�`�F�b�N
 * ��?�F
 *   struct map_session_data *sd	�Z�b�V�����f?�^
 *   int nameid						?���iID
 * �Ԃ�l�F
 *   0		?�X�Ȃ�
 *   -1		�X�L��������
 *------------------------------------------
 */
int pc_checkallowskill(struct map_session_data *sd)
{
	nullpo_retr(0, sd);
	nullpo_retr(0, sd->sc_data);

	// Skills requiring specific weapon types
	if(!(skill_get_weapontype(KN_TWOHANDQUICKEN)&(1<<sd->status.weapon)) && sd->sc_data[SC_TWOHANDQUICKEN].timer!=-1) { // Two-Hand Quicken requires a Two-handed sword
		status_change_end(&sd->bl,SC_TWOHANDQUICKEN,-1);
	}
	if(!(skill_get_weapontype(LK_AURABLADE)&(1<<sd->status.weapon)) && sd->sc_data[SC_AURABLADE].timer!=-1) { // Aura Blade requires any weapon but bare fists
		status_change_end(&sd->bl,SC_AURABLADE,-1);
	}
	if(!(skill_get_weapontype(LK_PARRYING)&(1<<sd->status.weapon)) && sd->sc_data[SC_PARRYING].timer!=-1) {	// Parrying requires a Two-handed sword
		status_change_end(&sd->bl,SC_PARRYING,-1);
	}
	if(!(skill_get_weapontype(CR_SPEARQUICKEN)&(1<<sd->status.weapon)) && sd->sc_data[SC_SPEARSQUICKEN].timer!=-1){	// Spear Quicken requires a Two-handed spear
		status_change_end(&sd->bl,SC_SPEARSQUICKEN,-1);
	}
	if(!(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)) && sd->sc_data[SC_ADRENALINE].timer!=-1){	// Adrenaline Rush requires an Axe or a Mace
		status_change_end(&sd->bl,SC_ADRENALINE,-1);
	}

	if(sd->status.shield <= 0) { // Skills requiring a shield
		if(sd->sc_data[SC_AUTOGUARD].timer!=-1){	// Guard
			status_change_end(&sd->bl,SC_AUTOGUARD,-1);
		}
		if(sd->sc_data[SC_DEFENDER].timer!=-1){	// Defending Aura
			status_change_end(&sd->bl,SC_DEFENDER,-1);
		}
		if(sd->sc_data[SC_REFLECTSHIELD].timer!=-1){ // Shield Reflect
			status_change_end(&sd->bl,SC_REFLECTSHIELD,-1);
		}
	}

	return 0;
}

/*==========================================
 * ? ���i�̃`�F�b�N
 *------------------------------------------
 */
int pc_checkequip(struct map_session_data *sd,int pos)
{
	int i;

	nullpo_retr(-1, sd);

	for(i=0;i<11;i++){
		if(pos & equip_pos[i])
			return sd->equip_index[i];
	}

	return -1;
}

/*==========================================
 * ?���E��{�q�E�̌��̐E�Ƃ�Ԃ�
 *------------------------------------------
 */
struct pc_base_job pc_calc_base_job(int b_class)
{
	struct pc_base_job bj;
	if(b_class < JOB_NOVICE_HIGH || (b_class >= JOB_TAEKWON && b_class <= JOB_SOUL_LINKER)){
		if (b_class == JOB_KNIGHT2)
			bj.job = JOB_KNIGHT;
		else if (b_class == JOB_CRUSADER2)
			bj.job = JOB_CRUSADER;
		else
			bj.job = b_class;
		bj.upper = 0;
	}else if(b_class >= JOB_NOVICE_HIGH && b_class <= JOB_PALADIN2){ //High Jobs
		if (b_class == JOB_LORD_KNIGHT2)
			bj.job = JOB_KNIGHT;
		else if (b_class == JOB_PALADIN2)
			bj.job = JOB_CRUSADER;
		else
			bj.job = b_class - JOB_NOVICE_HIGH;
		bj.upper = 1;
	}else{	//Baby Classes
		if (b_class == JOB_SUPER_BABY)
			bj.job = JOB_SUPER_NOVICE;
		else if (b_class == JOB_BABY_KNIGHT2)
			bj.job = JOB_KNIGHT;
		else if (b_class == JOB_BABY_CRUSADER2)
			bj.job = JOB_CRUSADER;
		else
			bj.job = b_class - JOB_BABY;
		bj.upper = 2;
	}

	if(bj.job == JOB_NOVICE){
		bj.type = 0;
	}else if(bj.job <= JOB_THIEF || bj.job == JOB_TAEKWON){
		bj.type = 1;
	}else{
		bj.type = 2;
	}

	return bj;
}

/*==========================================
 * For quick calculating [Celest]
 *------------------------------------------
 */
int pc_calc_base_job2 (int b_class)
{
	if(b_class < JOB_NOVICE_HIGH || (b_class >= JOB_TAEKWON && b_class <= JOB_SOUL_LINKER))
	{
		if (b_class == JOB_KNIGHT2)
			return JOB_KNIGHT;
		if (b_class == JOB_CRUSADER2)
			return JOB_CRUSADER;
		return b_class;
	}
	if(b_class >= JOB_NOVICE_HIGH && b_class < JOB_BABY)
	{
		if (b_class == JOB_LORD_KNIGHT2)
			return JOB_KNIGHT;
		if (b_class == JOB_PALADIN2)
			return JOB_CRUSADER;
		return b_class - JOB_NOVICE_HIGH;
	}
	//Baby Classes
	{
		if (b_class == JOB_SUPER_BABY)
			return JOB_SUPER_NOVICE;
		if (b_class == JOB_BABY_KNIGHT2)
			return JOB_KNIGHT;
		if (b_class == JOB_BABY_CRUSADER2)
			return JOB_CRUSADER;
		return b_class - JOB_BABY;
	}
}

int pc_calc_upper(int b_class)
{
	if(b_class < JOB_NOVICE_HIGH || (b_class >= JOB_TAEKWON && b_class <= JOB_SOUL_LINKER))
		return 0;
	else if(b_class >= JOB_NOVICE_HIGH && b_class < JOB_BABY)
		return 1;
	return 2;
}

/*==========================================
 * PC�̍U? (timer??)
 *------------------------------------------
 */
int pc_attack_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	struct block_list *bl;
	struct status_change *sc_data;
	short *opt;
	int dist,skill,range;

	sd=map_id2sd(id);
	if(sd == NULL)
		return 0;

	//Should we disable this line? Ctrl+click and then going away "IS" idling... [Skotlex]
	sd->idletime = last_tick;

	if(sd->attacktimer != tid){
		if(battle_config.error_log)
			ShowError("pc_attack_timer %d != %d\n",sd->attacktimer,tid);
		return 0;
	}
	sd->attacktimer=-1;

	if(sd->bl.prev == NULL)
		return 0;

	bl=map_id2bl(sd->attacktarget);
	if(bl==NULL || bl->prev == NULL)
		return 0;

	if(bl->type == BL_PC) {
		if (pc_isdead((struct map_session_data *)bl))
			return 0;
		else if (pc_ishiding((struct map_session_data *)bl))
			return 0;
	}

	// ����map�łȂ��Ȃ�U?���Ȃ�
	// PC������łĂ��U?���Ȃ�
	if(sd->bl.m != bl->m || pc_isdead(sd))
		return 0;

	if( sd->opt1>0 || sd->status.option&2 || pc_ischasewalk(sd))	// �ُ�ȂǂōU?�ł��Ȃ�
		return 0;

	if (sd->sc_count &&
 		(sd->sc_data[SC_AUTOCOUNTER].timer != -1 ||
		sd->sc_data[SC_BLADESTOP].timer != -1 ||
		sd->sc_data[SC_GRAVITATION].timer != -1 ||
		(sd->sc_data[SC_GOSPEL].timer != -1 && sd->sc_data[SC_GOSPEL].val4 == BCT_SELF)))
			return 0;

	if((opt = status_get_option(bl)) != NULL && *opt&0x42)
		return 0;
	if((sc_data = status_get_sc_data(bl)) != NULL) {
		if (sc_data[SC_TRICKDEAD].timer != -1 ||
			sc_data[SC_BASILICA].timer != -1)
		return 0;
	}

	if(sd->skilltimer != -1 && pc_checkskill(sd,SA_FREECAST) <= 0)
		return 0;

	if(!battle_config.sdelay_attack_enable && sd->canact_tick > tick && pc_checkskill(sd,SA_FREECAST) <= 0)
	{
		clif_skill_fail(sd,1,4,0);
		return 0;
	}

	if(sd->status.weapon == 11 && sd->equip_index[10] < 0)
	{
		clif_arrow_fail(sd,0);
		return 0;
	}

	dist = distance(sd->bl.x,sd->bl.y,bl->x,bl->y);
	range = sd->attackrange;
	if(sd->status.weapon != 11) range++;
	if( dist > range ){	// ? ���Ȃ��̂ňړ�
		if(pc_can_reach(sd,bl->x,bl->y))
			clif_movetoattack(sd,bl);
		return 0;
	}

	if(!battle_check_range(&sd->bl,bl,range) ) {
		if(pc_can_reach(sd,bl->x,bl->y) && sd->canmove_tick < tick && (sd->sc_data[SC_ANKLE].timer == -1 || sd->sc_data[SC_SPIDERWEB].timer == -1))
			pc_walktoxy(sd,bl->x,bl->y);
		sd->attackabletime = tick + (sd->aspd<<1);
	}
	else {
		if(battle_config.pc_attack_direction_change)
			sd->dir=sd->head_dir=map_calc_dir(&sd->bl, bl->x,bl->y );	// �����ݒ�

		if(sd->walktimer != -1)
			pc_stop_walking(sd,1);

		if(sd->attackabletime <= tick) {
			map_freeblock_lock();
			sd->attacktarget_lv = battle_weapon_attack(&sd->bl,bl,tick,0);
			
			if(!(battle_config.pc_cloak_check_type&2) && sd->sc_data[SC_CLOAKING].timer != -1)
				status_change_end(&sd->bl,SC_CLOAKING,-1);
			
			if(sd->status.pet_id > 0 && sd->pd && sd->petDB && battle_config.pet_attack_support)
				pet_target_check(sd,bl,0);

			map_freeblock_unlock();
			
			if(sd->skilltimer != -1 && (skill = pc_checkskill(sd,SA_FREECAST)) > 0 ) // �t��?�L���X�g
				sd->attackabletime = tick + ((sd->aspd<<1)*(150 - skill*5)/100);
			else
				sd->attackabletime = tick + (sd->aspd<<1);
		}
	}

	if(sd->state.attack_continue) {
		sd->attacktimer=add_timer(sd->attackabletime,pc_attack_timer,sd->bl.id,0);
	}

	return 0;
}

/*==========================================
 * �U?�v��
 * type��1�Ȃ�??�U?
 *------------------------------------------
 */
int pc_attack(struct map_session_data *sd,int target_id,int type)
{
	struct block_list *bl;

	nullpo_retr(0, sd);

	bl=map_id2bl(target_id);
	if(bl==NULL)
		return 1;

	if(bl->type==BL_NPC) { // monster npcs [Valaris]
		//npc_click(sd,RFIFOL(sd->fd,2));
		npc_click(sd,target_id); // submitted by leinsirk10 [Celest]
		return 0;
	}

	if(battle_check_target(&sd->bl,bl,BCT_ENEMY) <= 0)
		return 1;
	if(sd->attacktimer != -1)
	{	//Just change target/type. [Skotlex]
		sd->attacktarget=target_id;
		sd->state.attack_continue=type;
		return 0;
	}
	
	sd->attacktarget=target_id;
	sd->state.attack_continue=type;

	if(sd->attackabletime > gettick()){	//Do attack next time it is possible. [Skotlex]
		sd->attacktimer=add_timer(sd->attackabletime,pc_attack_timer,sd->bl.id,0);
	} else { //Attack NOW.
		pc_attack_timer(-1,gettick(),sd->bl.id,0);
	}

	return 0;
}

/*==========================================
 * ??�U?��~
 *------------------------------------------
 */
int pc_stopattack(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->attacktimer != -1) {
		delete_timer(sd->attacktimer,pc_attack_timer);
		sd->attacktimer=-1;
	}
	sd->attacktarget=0;
	sd->state.attack_continue=0;

	return 0;
}

int pc_follow_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd, *tsd;

	sd = map_id2sd(id);
	nullpo_retr(0, sd);

	if (sd->followtimer != tid){
		if(battle_config.error_log)
			ShowError("pc_follow_timer %d != %d\n",sd->followtimer,tid);
		sd->followtimer = -1;
		return 0;
	}

	sd->followtimer = -1;
	if (pc_isdead(sd))
		return 0;

	if ((tsd = map_id2sd(sd->followtarget)) != NULL)
	{
		if (pc_isdead(tsd))
			return 0;

		// either player or target is currently detached from map blocks (could be teleporting),
		// but still connected to this map, so we'll just increment the timer and check back later
		if (sd->bl.prev != NULL && tsd->bl.prev != NULL &&
			sd->skilltimer == -1 && sd->attacktimer == -1 && sd->walktimer == -1)
		{
			if((sd->bl.m == tsd->bl.m) && pc_can_reach(sd,tsd->bl.x,tsd->bl.y)) {
				if (distance(sd->bl.x,sd->bl.y,tsd->bl.x,tsd->bl.y) > 5)
					pc_walktoxy(sd,tsd->bl.x,tsd->bl.y);
			} else
				pc_setpos(sd, tsd->mapname, tsd->bl.x, tsd->bl.y, 3);
		}
		sd->followtimer = add_timer(
			tick + sd->aspd + rand() % 1000,	// increase time a bit to loosen up map's load
			pc_follow_timer, sd->bl.id, 0);
	}
	return 0;
}

int pc_stop_following (struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if (sd->followtimer != -1) {
		delete_timer(sd->followtimer,pc_follow_timer);
		sd->followtimer = -1;
	}
	sd->followtarget = -1;

	return 0;
}

int pc_follow(struct map_session_data *sd,int target_id)
{
	struct block_list *bl = map_id2bl(target_id);
	if (bl == NULL || bl->type != BL_PC)
		return 1;
	if (sd->followtimer != -1)
		pc_stop_following(sd);

	sd->followtarget = target_id;
	pc_follow_timer(-1,gettick(),sd->bl.id,0);

	return 0;
}

int pc_checkbaselevelup(struct map_session_data *sd)
{
	int next = pc_nextbaseexp(sd);

	nullpo_retr(0, sd);

	if(sd->status.base_exp >= next && next > 0){
		int s_class = pc_calc_base_job2(sd->status.class_);

		// base�����x���A�b�v?��
		sd->status.base_exp -= next;

		sd->status.base_level ++;
		if (battle_config.pet_lv_rate && sd->pd)	//<Skotlex> update pet's level
			status_calc_pet(sd,0);
		sd->status.status_point += (sd->status.base_level+14) / 5 ;
		clif_updatestatus(sd,SP_STATUSPOINT);
		clif_updatestatus(sd,SP_BASELEVEL);
		clif_updatestatus(sd,SP_NEXTBASEEXP);
		status_calc_pc(sd,0);
		pc_heal(sd,sd->status.max_hp,sd->status.max_sp);

		//�X�p�m�r�̓L���G�A�C���|�A�}�j�s�A�O���A�T�t��Lv1��������
		if(s_class == JOB_SUPER_NOVICE){
			status_change_start(&sd->bl,SkillStatusChangeTable[PR_KYRIE],1,0,0,0,skill_get_time(PR_KYRIE,1),0 );
			status_change_start(&sd->bl,SkillStatusChangeTable[PR_IMPOSITIO],1,0,0,0,skill_get_time(PR_IMPOSITIO,1),0 );
			status_change_start(&sd->bl,SkillStatusChangeTable[PR_MAGNIFICAT],1,0,0,0,skill_get_time(PR_MAGNIFICAT,1),0 );
			status_change_start(&sd->bl,SkillStatusChangeTable[PR_GLORIA],1,0,0,0,skill_get_time(PR_GLORIA,1),0 );
			status_change_start(&sd->bl,SkillStatusChangeTable[PR_SUFFRAGIUM],1,0,0,0,skill_get_time(PR_SUFFRAGIUM,1),0 );
		}

		clif_misceffect(&sd->bl,0);
		//���x���A�b�v�����̂Ńp?�e�B?�����X�V����
		//(������?�`�F�b�N)
		party_send_movemap(sd);
//LORDALFA - LVLUPEVENT
	if (script_config.event_script_type == 0) {
	struct npc_data *npc;
	if ((npc = npc_name2id("PCBaseUpEvent"))) {
	run_script(npc->u.scr.script,0,sd->bl.id,npc->bl.id); // PCLvlUPNPC
	ShowStatus("Event '"CL_WHITE"PCBaseUpEvent"CL_RESET"' executed.\n");
	}
	} else {
			ShowStatus("%d '"CL_WHITE"%s"CL_RESET"' events executed.\n",
			npc_event_doall_id("PCBaseUpEvent", sd->bl.id), "PCBaseUpEvent");
	}
//LORDALFA - LVLUPEVENT

		return 1;
	}

	return 0;
}

int pc_checkjoblevelup(struct map_session_data *sd)
{
	int next = pc_nextjobexp(sd);

	nullpo_retr(0, sd);

	if(sd->status.job_exp >= next && next > 0){
		// job�����x���A�b�v?��
		sd->status.job_exp -= next;
		sd->status.job_level ++;
		clif_updatestatus(sd,SP_JOBLEVEL);
		clif_updatestatus(sd,SP_NEXTJOBEXP);
		sd->status.skill_point ++;
		clif_updatestatus(sd,SP_SKILLPOINT);
		status_calc_pc(sd,0);

		clif_misceffect(&sd->bl,1);
		return 1;
	}

	return 0;
}

/*==========================================
 * ??�l�擾
 *------------------------------------------
 */
int pc_gainexp(struct map_session_data *sd,int base_exp,int job_exp)
{
	char output[256];
	float nextbp=0, nextjp=0;
	int nextb=0, nextj=0;
	nullpo_retr(0, sd);

	if(sd->bl.prev == NULL || pc_isdead(sd))
		return 0;

	if((battle_config.pvp_exp == 0) && map[sd->bl.m].flag.pvp)  // [MouseJstr]
		return 0; // no exp on pvp maps

	if(sd->status.guild_id>0){	// �M���h�ɏ�[
		base_exp-=guild_payexp(sd,base_exp);
		if(base_exp < 0)
			base_exp = 0;
	}

	if(!battle_config.multi_level_up && pc_nextbaseafter(sd) && sd->status.base_exp+base_exp >= pc_nextbaseafter(sd)) {
		base_exp = pc_nextbaseafter(sd) - sd->status.base_exp;
		if (base_exp < 0)
			base_exp = 0;
	}
	nextb = pc_nextbaseexp(sd);
	nextj = pc_nextjobexp(sd);
	if (nextb > 0)
		nextbp = (float) base_exp / (float) nextb;
	if (nextj > 0)
		nextjp = (float) job_exp / (float) nextj;

	sd->status.base_exp += base_exp;
	if(sd->status.base_exp < 0)
		sd->status.base_exp = 0;

	while(pc_checkbaselevelup(sd)) ;

	clif_updatestatus(sd,SP_BASEEXP);
	if(!battle_config.multi_level_up && pc_nextjobafter(sd) && sd->status.job_exp+job_exp >= pc_nextjobafter(sd)) {
		job_exp = pc_nextjobafter(sd) - sd->status.job_exp;
		if (job_exp < 0)
			job_exp = 0;
	}

	sd->status.job_exp += job_exp;
	if(sd->status.job_exp < 0)
		sd->status.job_exp = 0;

	while(pc_checkjoblevelup(sd)) ;

	clif_updatestatus(sd,SP_JOBEXP);

	if(battle_config.disp_experience && !sd->noexp){
		sprintf(output,
			"Experienced Gained Base:%d (%.2f%%) Job:%d (%.2f%%)",base_exp,nextbp*(float)100,job_exp,nextjp*(float)100);
		clif_disp_onlyself(sd,output,strlen(output));
	}

	return 0;
}

/*==========================================
 * base level���K�v??�l�v�Z
 *------------------------------------------
 */
int pc_nextbaseexp(struct map_session_data *sd)
{
	int i;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	if(sd->status.base_level>=MAX_LEVEL || sd->status.base_level<=0)
		return 0;

	s_class = pc_calc_base_job(sd->status.class_);

	if (s_class.upper == 1)
	{	//Uppper Classes
		i = 4 + s_class.type; //4+0: High Novice, 4+1: High First Class, 4+2: Advanced Classes
	} else { //Baby/Normal classes
		if (s_class.job == JOB_SUPER_NOVICE)
			i = 3; //Super Novice/Super Baby
		else
			i = s_class.type; //0: Novice, 1: First Class, 2: Second Class
	}
	return exp_table[i][sd->status.base_level-1];
}

/*==========================================
 * job level���K�v??�l�v�Z
 *------------------------------------------
 */
int pc_nextjobexp(struct map_session_data *sd)
{
	int i;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	if(sd->status.job_level>=MAX_LEVEL || sd->status.job_level<=0)
		return 0;

	s_class = pc_calc_base_job(sd->status.class_);

	if (s_class.upper == 1)
	{	//Uppper Classes
		i = 11 + s_class.type; //11+0: High Novice, 11+1: High First Class, 11+2: Advanced Classes
	} else { //Baby/Normal classes
		if (s_class.job == JOB_SUPER_NOVICE)
			i = 10; //Super Novice/Super Baby
		else
			i = 7+s_class.type; //7+0: Novice, 7+1: First Class, 7+2: Second Class
	}

	return exp_table[i][sd->status.job_level-1];
}

/*==========================================
 * base level after next [Valaris]
 *------------------------------------------
 */
int pc_nextbaseafter(struct map_session_data *sd)
{
	int i;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	if(sd->status.base_level>=MAX_LEVEL || sd->status.base_level<=0)
		return 0;

	s_class = pc_calc_base_job(sd->status.class_);

	if (s_class.upper == 1)
	{	//Uppper Classes
		i = 4 + s_class.type; //4+0: High Novice, 4+1: High First Class, 4+2: Advanced Classes
	} else { //Baby/Normal classes
		if (s_class.job == JOB_SUPER_NOVICE)
			i = 3; //Super Novice/Super Baby
		else
			i = s_class.type; //0: Novice, 1: First Class, 2: Second Class
	}

	return exp_table[i][sd->status.base_level];
}

/*==========================================
 * job level after next [Valaris]
 *------------------------------------------
 */
int pc_nextjobafter(struct map_session_data *sd)
{
	int i;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	if(sd->status.job_level>=MAX_LEVEL || sd->status.job_level<=0)
		return 0;

	s_class = pc_calc_base_job(sd->status.class_);

	if (s_class.upper == 1)
	{	//Uppper Classes
		i = 11 + s_class.type; //11+0: High Novice, 11+1: High First Class, 11+2: Advanced Classes
	} else { //Baby/Normal classes
		if (s_class.job == JOB_SUPER_NOVICE)
			i = 10; //Super Novice/Super Baby
		else
			i = 7+s_class.type; //7+0: Novice, 7+1: First Class, 7+2: Second Class
	}

	return exp_table[i][sd->status.job_level];
}
/*==========================================

 * �K�v�X�e?�^�X�|�C���g�v�Z
 *------------------------------------------
 */
int pc_need_status_point(struct map_session_data *sd,int type)
{
	int val;

	nullpo_retr(-1, sd);

	if(type<SP_STR || type>SP_LUK)
		return -1;
	val =
		type==SP_STR ? sd->status.str :
		type==SP_AGI ? sd->status.agi :
		type==SP_VIT ? sd->status.vit :
		type==SP_INT ? sd->status.int_:
		type==SP_DEX ? sd->status.dex : sd->status.luk;

	return (val+9)/10+1;
}

/*==========================================
 * �\�͒l����
 *------------------------------------------
 */
int pc_statusup(struct map_session_data *sd,int type)
{
	int max, need,val = 0;

	nullpo_retr(0, sd);

	max = (pc_calc_upper(sd->status.class_)==2) ? 80 : battle_config.max_parameter;

	need=pc_need_status_point(sd,type);
	if(type<SP_STR || type>SP_LUK || need<0 || need>sd->status.status_point){
		clif_statusupack(sd,type,0,0);
		return 1;
	}
	switch(type){
	case SP_STR:
		if(sd->status.str >= max) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.str;
		break;
	case SP_AGI:
		if(sd->status.agi >= max) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.agi;
		break;
	case SP_VIT:
		if(sd->status.vit >= max) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.vit;
		break;
	case SP_INT:
		if(sd->status.int_ >= max) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.int_;
		break;
	case SP_DEX:
		if(sd->status.dex >= max) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.dex;
		break;
	case SP_LUK:
		if(sd->status.luk >= max) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.luk;
		break;
	}
	sd->status.status_point-=need;
	if(need!=pc_need_status_point(sd,type)){
		clif_updatestatus(sd,type-SP_STR+SP_USTR);
	}
	clif_updatestatus(sd,SP_STATUSPOINT);
	clif_updatestatus(sd,type);
	status_calc_pc(sd,0);
	clif_statusupack(sd,type,1,val);

	return 0;
}

/*==========================================
 * �\�͒l����
 *------------------------------------------
 */
int pc_statusup2(struct map_session_data *sd,int type,int val)
{
	nullpo_retr(0, sd);

	if(type<SP_STR || type>SP_LUK){
		clif_statusupack(sd,type,0,0);
		return 1;
	}
	switch(type){
	case SP_STR:
		if(sd->status.str + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.str + val < 1)
			val = 1;
		else
			val += sd->status.str;
		sd->status.str = val;
		break;
	case SP_AGI:
		if(sd->status.agi + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.agi + val < 1)
			val = 1;
		else
			val += sd->status.agi;
		sd->status.agi = val;
		break;
	case SP_VIT:
		if(sd->status.vit + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.vit + val < 1)
			val = 1;
		else
			val += sd->status.vit;
		sd->status.vit = val;
		break;
	case SP_INT:
		if(sd->status.int_ + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.int_ + val < 1)
			val = 1;
		else
			val += sd->status.int_;
		sd->status.int_ = val;
		break;
	case SP_DEX:
		if(sd->status.dex + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.dex + val < 1)
			val = 1;
		else
			val += sd->status.dex;
		sd->status.dex = val;
		break;
	case SP_LUK:
		if(sd->status.luk + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.luk + val < 1)
			val = 1;
		else
			val = sd->status.luk + val;
		sd->status.luk = val;
		break;
	}
	clif_updatestatus(sd,type-SP_STR+SP_USTR);
	clif_updatestatus(sd,type);
	status_calc_pc(sd,0);
	clif_statusupack(sd,type,1,val);

	return 0;
}

/*==========================================
 * �X�L���|�C���g����U��
 *------------------------------------------
 */
int pc_skillup(struct map_session_data *sd,int skill_num)
{
	nullpo_retr(0, sd);

	if( skill_num>=10000 ){
		guild_skillup(sd,skill_num,0);
		return 0;
	}

	if( sd->status.skill_point>0 &&
		sd->status.skill[skill_num].id!=0 &&
		//sd->status.skill[skill_num].lv < skill_get_max(skill_num) ) - celest
		sd->status.skill[skill_num].lv < skill_tree_get_max(skill_num, sd->status.class_) )
	{
		sd->status.skill[skill_num].lv++;
		sd->status.skill_point--;
		status_calc_pc(sd,0);
		clif_skillup(sd,skill_num);
		clif_updatestatus(sd,SP_SKILLPOINT);
		clif_skillinfoblock(sd);
	}

	return 0;
}

/*==========================================
 * /allskill
 *------------------------------------------
 */
int pc_allskillup(struct map_session_data *sd)
{
	int i,id;

	nullpo_retr(0, sd);

	for(i=0;i<MAX_SKILL;i++){
		sd->status.skill[i].id=0;
		if (sd->status.skill[i].flag && sd->status.skill[i].flag != 13){	// card�X�L���Ȃ�A
			sd->status.skill[i].lv=(sd->status.skill[i].flag==1)?0:sd->status.skill[i].flag-2;	// �{?��lv��
			sd->status.skill[i].flag=0;	// flag��0�ɂ��Ă���
		}
	}

	if (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill){
		// �S�ẴX�L��
		for(i=0;i<MAX_SKILL;i++){
			if(!(skill_get_inf2(i)&INF2_NPC_SKILL)) //Get ALL skills except npc ones. [Skotlex]
				sd->status.skill[i].lv=skill_get_max(i); //Nonexistant skills should return a max of 0 anyway.
		}
	}
	else {
		int inf2;
		for(i=0;i < MAX_SKILL_TREE && (id=skill_tree[sd->status.class_][i].id)>0;i++){
			inf2 = skill_get_inf2(id);
			if(sd->status.skill[id].id==0 && (!(inf2&INF2_QUEST_SKILL) || battle_config.quest_skill_learn) && !(inf2&INF2_WEDDING_SKILL)) {
				sd->status.skill[id].id = id;	// celest
				sd->status.skill[id].lv = skill_tree_get_max(id, sd->status.class_);	// celest
			}
		}
	}
	status_calc_pc(sd,0);

	return 0;
}

/*==========================================
 * /resetlvl
 *------------------------------------------
 */
int pc_resetlvl(struct map_session_data* sd,int type)
{
	int  i;

	nullpo_retr(0, sd);

	for(i=1;i<MAX_SKILL;i++){
		sd->status.skill[i].lv = 0;
	}

	if(type == 1){
	sd->status.skill_point=0;
	sd->status.base_level=1;
	sd->status.job_level=1;
	sd->status.base_exp=sd->status.base_exp=0;
	sd->status.job_exp=sd->status.job_exp=0;
	if(sd->status.option !=0)
		sd->status.option = 0;

	sd->status.str=1;
	sd->status.agi=1;
	sd->status.vit=1;
	sd->status.int_=1;
	sd->status.dex=1;
	sd->status.luk=1;
	if(sd->status.class_ == JOB_NOVICE_HIGH)
		sd->status.status_point=100;	// not 88 [celest]
		// give platinum skills upon changing
		pc_skill(sd,142,1,0);
		pc_skill(sd,143,1,0);
	}

	if(type == 2){
		sd->status.skill_point=0;
		sd->status.base_level=1;
		sd->status.job_level=1;
		sd->status.base_exp=0;
		sd->status.job_exp=0;
	}
	if(type == 3){
		sd->status.base_level=1;
		sd->status.base_exp=0;
	}
	if(type == 4){
		sd->status.job_level=1;
		sd->status.job_exp=0;
	}

	clif_updatestatus(sd,SP_STATUSPOINT);
	clif_updatestatus(sd,SP_STR);
	clif_updatestatus(sd,SP_AGI);
	clif_updatestatus(sd,SP_VIT);
	clif_updatestatus(sd,SP_INT);
	clif_updatestatus(sd,SP_DEX);
	clif_updatestatus(sd,SP_LUK);
	clif_updatestatus(sd,SP_BASELEVEL);
	clif_updatestatus(sd,SP_JOBLEVEL);
	clif_updatestatus(sd,SP_STATUSPOINT);
	clif_updatestatus(sd,SP_NEXTBASEEXP);
	clif_updatestatus(sd,SP_NEXTJOBEXP);
	clif_updatestatus(sd,SP_SKILLPOINT);

	clif_updatestatus(sd,SP_USTR);	// Updates needed stat points - Valaris
	clif_updatestatus(sd,SP_UAGI);
	clif_updatestatus(sd,SP_UVIT);
	clif_updatestatus(sd,SP_UINT);
	clif_updatestatus(sd,SP_UDEX);
	clif_updatestatus(sd,SP_ULUK);	// End Addition

	for(i=0;i<11;i++) { // unequip items that can't be equipped by base 1 [Valaris]
		if(sd->equip_index[i] >= 0)
			if(!pc_isequip(sd,sd->equip_index[i]))
				pc_unequipitem(sd,sd->equip_index[i],2);
	}

	clif_skillinfoblock(sd);
	status_calc_pc(sd,0);

	return 0;
}
/*==========================================
 * /resetstate
 *------------------------------------------
 */
int pc_resetstate(struct map_session_data* sd)
{
	nullpo_retr(0, sd);
	
	if (battle_config.use_statpoint_table)
	{	// New statpoint table used here - Dexity
		int lv;
		// allow it to just read the last entry [celest]
		lv = sd->status.base_level < MAX_LEVEL ? sd->status.base_level : MAX_LEVEL - 1;
		
		sd->status.status_point = statp[lv];
		if (pc_calc_upper(sd->status.class_)==1) //Upper Jobs are Upper 1! (2 is for baby classes) [Skotlex]
			sd->status.status_point+=52;	// extra 52+48=100 stat points
	} else { //Use new stat-calculating equation [Skotlex]
#define sumsp(a) (((a-1)/10 +2)*(5*((a-1)/10 +1) + (a-1)%10) -10)
		int add=0;
		add += sumsp(sd->status.str);
		add += sumsp(sd->status.agi);
		add += sumsp(sd->status.vit);
		add += sumsp(sd->status.int_);
		add += sumsp(sd->status.dex);
		add += sumsp(sd->status.luk);
		sd->status.status_point+=add;
	}

	sd->status.str=1;
	sd->status.agi=1;
	sd->status.vit=1;
	sd->status.int_=1;
	sd->status.dex=1;
	sd->status.luk=1;

	clif_updatestatus(sd,SP_STR);
	clif_updatestatus(sd,SP_AGI);
	clif_updatestatus(sd,SP_VIT);
	clif_updatestatus(sd,SP_INT);
	clif_updatestatus(sd,SP_DEX);
	clif_updatestatus(sd,SP_LUK);

	clif_updatestatus(sd,SP_USTR);	// Updates needed stat points - Valaris
	clif_updatestatus(sd,SP_UAGI);
	clif_updatestatus(sd,SP_UVIT);
	clif_updatestatus(sd,SP_UINT);
	clif_updatestatus(sd,SP_UDEX);
	clif_updatestatus(sd,SP_ULUK);	// End Addition
	
	clif_updatestatus(sd,SP_STATUSPOINT);
	status_calc_pc(sd,0);

	return 0;
}

/*==========================================
 * /resetskill
 *------------------------------------------
 */
int pc_resetskill(struct map_session_data* sd)
{
	int i, skill, inf2;
	nullpo_retr(0, sd);

	for (i = 1; i < MAX_SKILL; i++) {
		if ((skill = sd->status.skill[i].lv) > 0) {
			inf2 = skill_get_inf2(i);	
			if ((!(inf2&INF2_QUEST_SKILL) || battle_config.quest_skill_learn) &&
				!(inf2&INF2_WEDDING_SKILL)) //Avoid reseting wedding skills.
			{
					if (!sd->status.skill[i].flag)
						sd->status.skill_point += skill;
					else if (sd->status.skill[i].flag > 2 && sd->status.skill[i].flag != 13)
						sd->status.skill_point += (sd->status.skill[i].flag - 2);
					sd->status.skill[i].lv = 0;
			}
			else if (battle_config.quest_skill_reset && (inf2&INF2_QUEST_SKILL))
			{
				sd->status.skill[i].lv = 0;
				sd->status.skill[i].flag = 0;
			}
		} else {
			sd->status.skill[i].lv = 0;
		}
	}
	clif_updatestatus(sd,SP_SKILLPOINT);
	clif_skillinfoblock(sd);
	status_calc_pc(sd,0);

	return 0;
}

/*==========================================
 * pc�Ƀ_��?�W��?����
 *------------------------------------------
 */
int pc_damage(struct block_list *src,struct map_session_data *sd,int damage, int delay)
{
	int i=0,j=0;
	int s_class;

	nullpo_retr(0, sd);

	//?����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	s_class = pc_calc_base_job2(sd->status.class_);
	// ?�Ɏ���ł����疳?
	if(pc_isdead(sd))
		return 0;
	// �����Ă��痧���オ��
	if(pc_issit(sd)) {
		pc_setstand(sd);
		skill_gangsterparadise(sd,0);
	}

	// ? ���Ă����瑫���~�߂�
	if (sd->sc_data) {
		if (sd->sc_data[SC_ENDURE].timer != -1 && (src != NULL && src->type == BL_MOB) && !map[sd->bl.m].flag.gvg) {
			if (!sd->special_state.infinite_endure && (--sd->sc_data[SC_ENDURE].val2) < 0) 
				status_change_end(&sd->bl, SC_ENDURE, -1);
		} else if (delay) {
			pc_stop_walking (sd,1);
			if (sd->canmove_tick < gettick())
				sd->canmove_tick = gettick() + delay;
		}
		if (sd->sc_data[SC_GRAVITATION].timer != -1 &&
			sd->sc_data[SC_GRAVITATION].val3 == BCT_SELF) {
			struct skill_unit_group *sg = (struct skill_unit_group *)sd->sc_data[SC_GRAVITATION].val4;
			if (sg) {
				skill_delunitgroup(sg);
				status_change_end(&sd->bl, SC_GRAVITATION, -1);
			}
		}

	}

	// ���t/�_���X�̒�?
	if(damage > sd->status.max_hp>>2)
		skill_stop_dancing(&sd->bl);

	sd->status.hp-=damage;
	if(sd->status.pet_id > 0 && sd->pd && sd->petDB && battle_config.pet_damage_support)
		pet_target_check(sd,src,1);

	if (sd->sc_data[SC_TRICKDEAD].timer != -1)
		status_change_end(&sd->bl, SC_TRICKDEAD, -1);
	if(sd->status.option&2)
		status_change_end(&sd->bl, SC_HIDING, -1);
	if(pc_iscloaking(sd))
		status_change_end(&sd->bl, SC_CLOAKING, -1);
	if(pc_ischasewalk(sd))
		status_change_end(&sd->bl, SC_CHASEWALK, -1);

	if(sd->status.hp>0){
		// �܂������Ă���Ȃ�HP�X�V
		clif_updatestatus(sd,SP_HP);

		//if(sd->status.hp<sd->status.max_hp>>2 && pc_checkskill(sd,SM_AUTOBERSERK)>0 &&
		if(sd->status.hp<sd->status.max_hp>>2 && sd->sc_data[SC_AUTOBERSERK].timer != -1 &&
			(sd->sc_data[SC_PROVOKE].timer==-1 || sd->sc_data[SC_PROVOKE].val2==0 ))
			// �I?�g�o?�T?�N?��
			status_change_start(&sd->bl,SC_PROVOKE,10,1,0,0,0,0);

		sd->canlog_tick = gettick();

		if(sd->status.party_id>0) {	// on-the-fly party hp updates [Valaris]
			struct party *p=party_search(sd->status.party_id);
			if(p!=NULL) clif_party_hp(p,sd);
		}	// end addition [Valaris]

		return damage;
	}
	sd->status.hp = 0;
	//pc_setdead(sd);
	if(sd->vender_id)
		vending_closevending(sd);

	if(sd->status.pet_id > 0 && sd->pd && 
		!map[sd->bl.m].flag.nopenalty) {
		if(sd->petDB) {
			sd->pet.intimate -= sd->petDB->die;
			if(sd->pet.intimate < 0)
				sd->pet.intimate = 0;
			clif_send_petdata(sd,1,sd->pet.intimate);
		}
	}

	pc_stop_walking(sd,0);
	skill_castcancel(&sd->bl,0);	// �r���̒��~
	skill_stop_dancing(&sd->bl); //You should stop dancing when dead... [Skotlex]
	if (sd->sc_data[SC_GOSPEL].timer != -1 && sd->sc_data[SC_GOSPEL].val4 == BCT_SELF)
	{	//Remove Gospel [Skotlex]
		struct skill_unit_group *sg = (struct skill_unit_group *)sd->sc_data[SC_GOSPEL].val3;
		if (sg)
			skill_delunitgroup(sg);
	}
	clif_clearchar_area(&sd->bl,1);

	if (src && src->type == BL_PC) {
		struct map_session_data *ssd = (struct map_session_data *)src;
		if (ssd) {
			if (sd->state.event_death)
				pc_setglobalreg(sd,"killerrid",(ssd->status.account_id));
			if (ssd->state.event_kill) {
				if (script_config.event_script_type == 0) {
					struct npc_data *npc;
					if ((npc = npc_name2id(script_config.kill_event_name))) {
						run_script(npc->u.scr.script,0,sd->bl.id,npc->bl.id); // PCKillNPC
						ShowStatus("Event '"CL_WHITE"%s"CL_RESET"' executed.\n", script_config.kill_event_name);
					}
				} else {
					ShowStatus("%d '"CL_WHITE"%s"CL_RESET"' events executed.\n",
						npc_event_doall_id(script_config.kill_event_name, sd->bl.id), script_config.kill_event_name);
				}
			}
			if (battle_config.pk_mode && ssd->status.manner >= 0) {
				ssd->status.manner -= 5;
				if(ssd->status.manner < 0)
					status_change_start(src,SC_NOCHAT,0,0,0,0,0,0);

			// PK/Karma system code (not enabled yet) [celest]
			// originally from Kade Online, so i don't know if any of these is correct ^^;
			// note: karma is measured REVERSE, so more karma = more 'evil' / less honourable,
			// karma going down = more 'good' / more honourable.
			// The Karma System way...
				/*if (sd->status.karma > ssd->status.karma) {	// If player killed was more evil
					sd->status.karma--;
					ssd->status.karma--;
				}
				else if (sd->status.karma < ssd->status.karma)	// If player killed was more good
					ssd->status.karma++;*/

			// or the PK System way...
				/* if (sd->status.karma > 0)	// player killed is dishonourable?
					ssd->status.karma--; // honour points earned
				sd->status.karma++;	// honour points lost */
				// To-do: Receive exp on certain occasions
			}
		}
	}

	if (sd->state.event_death) {
		if (script_config.event_script_type == 0) {
			struct npc_data *npc;
			if ((npc = npc_name2id(script_config.die_event_name))) {
				run_script(npc->u.scr.script,0,sd->bl.id,npc->bl.id); // PCDeathNPC
				ShowStatus("Event '"CL_WHITE"%s"CL_RESET"' executed.\n", script_config.die_event_name);
			}
		} else {
			ShowStatus("%d '"CL_WHITE"%s"CL_RESET"' events executed.\n",
				npc_event_doall_id(script_config.die_event_name, sd->bl.id), script_config.die_event_name);
		}
	}

// PK/Karma system code (not enabled yet) [celest]
	/*if(sd->status.karma < 0) {
		int eq_num=0,eq_n[MAX_INVENTORY];
		memset(eq_n,0,sizeof(eq_n));
		for(i=0;i<MAX_INVENTORY;i++){
			int k;
			for(k=0;k<MAX_INVENTORY;k++){
				if(eq_n[k] <= 0){
					eq_n[k]=i;
					break;
				}
			}
			eq_num++;
		}
		if(eq_num > 0){
			int n = eq_n[rand()%eq_num];
			if(rand()%10000 < sd->status.karma){
				if(sd->status.inventory[n].equip)
					pc_unequipitem(sd,n,0);
				pc_dropitem(sd,n,1);
			}
		}
	}*/

	if(battle_config.bone_drop==2
		|| (battle_config.bone_drop==1 && map[sd->bl.m].flag.pvp)){	// �h�N���h���b�v
		struct item item_tmp;
		memset(&item_tmp,0,sizeof(item_tmp));
		item_tmp.nameid=7005;
		item_tmp.identify=1;
		item_tmp.card[0]=0x00fe;
		item_tmp.card[1]=0;
		item_tmp.card[2]=GetWord(sd->char_id,0); // CharId
		item_tmp.card[3]=GetWord(sd->char_id,1);
		map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
	}

	// activate Steel body if a super novice dies at 99+% exp [celest]
	if (s_class == JOB_SUPER_NOVICE) {
		if ((i=pc_nextbaseexp(sd))<=0)
			i=sd->status.base_exp;
		if (i>0 && (j=sd->status.base_exp*1000/i)>=990 && j<=1000)
			sd->state.snovice_flag = 4;
	}

	for(i = 0; i < 5; i++)
		if (sd->devotion[i]){
			struct map_session_data *devsd = map_id2sd(sd->devotion[i]);
			if (devsd) status_change_end(&devsd->bl,SC_DEVOTION,-1);
		}

	pc_setdead(sd);
	skill_unit_move(&sd->bl,gettick(),4);
	if(sd->sc_data[SC_BLADESTOP].timer!=-1)//���n�͎��O�ɉ���
		status_change_end(&sd->bl,SC_BLADESTOP,-1);
	pc_setglobalreg(sd,"PC_DIE_COUNTER",++sd->die_counter); //���ɃJ�E���^?����?��
	status_change_clear(&sd->bl,0);	// �X�e?�^�X�ُ����������
	clif_updatestatus(sd,SP_HP);
	status_calc_pc(sd,0);
	sd->canregen_tick = gettick();

	if(battle_config.death_penalty_type>0) { // changed penalty options, added death by player if pk_mode [Valaris]
		if(pc_calc_base_job2(sd->status.class_) != JOB_NOVICE && !map[sd->bl.m].flag.nopenalty && !map[sd->bl.m].flag.gvg &&	// only novices will recieve no penalty
			!(sd->sc_count && sd->sc_data[SC_BABY].timer!=-1)) {
			if(battle_config.death_penalty_type==1 && battle_config.death_penalty_base > 0)
				sd->status.base_exp -= (int) ((double)pc_nextbaseexp(sd) * (double)battle_config.death_penalty_base/10000);
				if(battle_config.pk_mode && src && src->type==BL_PC)
					sd->status.base_exp -= (int) ((double)pc_nextbaseexp(sd) * (double)battle_config.death_penalty_base/10000);
			else if(battle_config.death_penalty_type==2 && battle_config.death_penalty_base > 0) {
				if(pc_nextbaseexp(sd) > 0)
					sd->status.base_exp -= (int) ((double)sd->status.base_exp * (double)battle_config.death_penalty_base/10000);
					if(battle_config.pk_mode && src && src->type==BL_PC)
						sd->status.base_exp -= (int) ((double)sd->status.base_exp * (double)battle_config.death_penalty_base/10000);
			}
			if(sd->status.base_exp < 0)
				sd->status.base_exp = 0;
			clif_updatestatus(sd,SP_BASEEXP);

			if(battle_config.death_penalty_type==1 && battle_config.death_penalty_job > 0)
				sd->status.job_exp -= (int) ((double)pc_nextjobexp(sd) * (double)battle_config.death_penalty_job/10000);
					if(battle_config.pk_mode && src && src->type==BL_PC)
					sd->status.job_exp -= (int) ((double)pc_nextjobexp(sd) * (double)battle_config.death_penalty_job/10000);
			else if(battle_config.death_penalty_type==2 && battle_config.death_penalty_job > 0) {
				if(pc_nextjobexp(sd) > 0)
					sd->status.job_exp -= (int) ((double)sd->status.job_exp * (double)battle_config.death_penalty_job/10000);
					if(battle_config.pk_mode && src && src->type==BL_PC)
						sd->status.job_exp -= (int) ((double)sd->status.job_exp * (double)battle_config.death_penalty_job/10000);
			}
			if(sd->status.job_exp < 0)
				sd->status.job_exp = 0;
			clif_updatestatus(sd,SP_JOBEXP);
		}
	}
	if(src && src->type==BL_MOB) {
		struct mob_data *md=(struct mob_data *)src;
		if(md && md->target_id != 0 && md->target_id==sd->bl.id) { // reset target id when player dies
			md->target_id=0;
			mob_changestate(md,MS_WALK,0);
		}
		if(battle_config.mobs_level_up && md && md->state.state!=MS_DEAD &&
			md->level < battle_config.max_base_level && !md->guardian_data // Guardians should not level. [Skotlex]
		) { 	// monster level up [Valaris]
			clif_misceffect(&md->bl,0);
			md->level++;
			md->hp+=(int) (sd->status.max_hp*.1);
			if (battle_config.show_mob_hp)
				clif_charnameack (0, &md->bl);
		}
	}

	//�i�C�g���A��?�h�A�C�e���h���b�v
	if(map[sd->bl.m].flag.pvp_nightmaredrop){ // Moved this outside so it works when PVP isnt enabled and during pk mode [Ancyker]
		for(j=0;j<MAX_DROP_PER_MAP;j++){
			int id = map[sd->bl.m].drop_list[j].drop_id;
			int type = map[sd->bl.m].drop_list[j].drop_type;
			int per = map[sd->bl.m].drop_list[j].drop_per;
			if(id == 0)
				continue;
			if(id == -1){//�����_���h���b�v
				int eq_num=0,eq_n[MAX_INVENTORY];
				memset(eq_n,0,sizeof(eq_n));
				//�悸?�����Ă���A�C�e��?���J�E���g
				for(i=0;i<MAX_INVENTORY;i++){
					int k;
					if( (type == 1 && !sd->status.inventory[i].equip)
						|| (type == 2 && sd->status.inventory[i].equip)
						||  type == 3){
						//InventoryIndex���i�[
						for(k=0;k<MAX_INVENTORY;k++){
							if(eq_n[k] <= 0){
								eq_n[k]=i;
								break;
							}
						}
						eq_num++;
					}
				}
				if(eq_num > 0){
					int n = eq_n[rand()%eq_num];//�Y?�A�C�e���̒����烉���_��
					if(rand()%10000 < per){
						if(sd->status.inventory[n].equip)
							pc_unequipitem(sd,n,3);
						pc_dropitem(sd,n,1);
					}
				}
			}
			else if(id > 0){
				for(i=0;i<MAX_INVENTORY;i++){
					if(sd->status.inventory[i].nameid == id//ItemID����v���Ă���
						&& rand()%10000 < per//�h���b�v�������OK��
						&& ((type == 1 && !sd->status.inventory[i].equip)//�^�C�v�����OK�Ȃ�h���b�v
							|| (type == 2 && sd->status.inventory[i].equip)
							|| type == 3) ){
						if(sd->status.inventory[i].equip)
							pc_unequipitem(sd,i,3);
						pc_dropitem(sd,i,1);
						break;
					}
				}
			}
		}
	}
	// pvp
	if( map[sd->bl.m].flag.pvp && !battle_config.pk_mode){ // disable certain pvp functions on pk_mode [Valaris]
		//�����L���O�v�Z
		if (!map[sd->bl.m].flag.pvp_nocalcrank) {
			sd->pvp_point -= 5;
			sd->pvp_lost++;
			if (src && src->type == BL_PC) {
				struct map_session_data *ssd = (struct map_session_data *)src;
				if (ssd) { ssd->pvp_point++; ssd->pvp_won++; }
			}
		//} //fixed wrong '{' placement by Lupus
			pc_setdead(sd);
		}
		// ?������
		if( sd->pvp_point < 0 ){
			sd->pvp_point=0;
			pc_setstand(sd);
			pc_setrestartvalue(sd,3);
			pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,0);
		}
	}
	//GvG
	if(map[sd->bl.m].flag.gvg){
		pc_setstand(sd);
		pc_setrestartvalue(sd,3);
		pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,0);
	}

	return damage;
}

//
// script? �A
//
/*==========================================
 * script�pPC�X�e?�^�X?�ݏo��
 *------------------------------------------
 */
int pc_readparam(struct map_session_data *sd,int type)
{
	int val=0;
	struct pc_base_job s_class;

	s_class = pc_calc_base_job(sd->status.class_);

	nullpo_retr(0, sd);

	switch(type){
	case SP_SKILLPOINT:
		val= sd->status.skill_point;
		break;
	case SP_STATUSPOINT:
		val= sd->status.status_point;
		break;
	case SP_ZENY:
		val= sd->status.zeny;
		break;
	case SP_BASELEVEL:
		val= sd->status.base_level;
		break;
	case SP_JOBLEVEL:
		val= sd->status.job_level;
		break;
	case SP_CLASS:
		if(val>=24 && val < 45)
			val+=3978;
		else
		val= sd->status.class_;
		break;
	case SP_BASEJOB:
		val= s_class.job;
		break;
	case SP_UPPER:
		val= s_class.upper;
		break;
	case SP_SEX:
		val= sd->sex;
		break;
	case SP_WEIGHT:
		val= sd->weight;
		break;
	case SP_MAXWEIGHT:
		val= sd->max_weight;
		break;
	case SP_BASEEXP:
		val= sd->status.base_exp;
		break;
	case SP_JOBEXP:
		val= sd->status.job_exp;
		break;
	case SP_NEXTBASEEXP:
		val= pc_nextbaseexp(sd);
		break;
	case SP_NEXTJOBEXP:
		val= pc_nextjobexp(sd);
		break;
	case SP_HP:
		val= sd->status.hp;
		break;
	case SP_MAXHP:
		val= sd->status.max_hp;
		break;
	case SP_SP:
		val= sd->status.sp;
		break;
	case SP_MAXSP:
		val= sd->status.max_sp;
		break;
	case SP_STR:
		val= sd->status.str;
		break;
	case SP_AGI:
		val= sd->status.agi;
		break;
	case SP_VIT:
		val= sd->status.vit;
		break;
	case SP_INT:
		val= sd->status.int_;
		break;
	case SP_DEX:
		val= sd->status.dex;
		break;
	case SP_LUK:
		val= sd->status.luk;
		break;
	case SP_KARMA:	// celest
		val = sd->status.karma;
		break;
	case SP_MANNER:
		val = sd->status.manner;
		break;
	case SP_FAME:
		val= sd->status.fame;
		break;
	}

	return val;
}

/*==========================================
 * script�pPC�X�e?�^�X�ݒ�
 *------------------------------------------
 */
int pc_setparam(struct map_session_data *sd,int type,int val)
{
	int i = 0,up_level = battle_config.max_job_level;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job(sd->status.class_);

	switch(type){
	case SP_BASELEVEL:
		if ((val+ sd->status.base_level) > battle_config.max_base_level) //Capping to max
			val = battle_config.max_base_level - sd->status.base_level;
		if (val > sd->status.base_level) {
			for (i = 1; i <= (val - sd->status.base_level); i++)
				sd->status.status_point += (sd->status.base_level + i + 14) / 5 ;
		}
		sd->status.base_level = val;
		sd->status.base_exp = 0;
		clif_updatestatus(sd, SP_BASELEVEL);
		clif_updatestatus(sd, SP_NEXTBASEEXP);
		clif_updatestatus(sd, SP_STATUSPOINT);
		clif_updatestatus(sd, SP_BASEEXP);
		status_calc_pc(sd, 0);
		pc_heal(sd, sd->status.max_hp, sd->status.max_sp);
		break;
	case SP_JOBLEVEL:
		if (s_class.job == JOB_NOVICE) //Novice & Baby Novice have 10 Job Levels only
			up_level = 10;
		else if (s_class.job == JOB_SUPER_NOVICE) //Super Novice & Super Baby can go up to 99
			up_level = battle_config.max_sn_level;
		else if (s_class.upper == 1 && s_class.type == 2) //3rd Job has 70 Job Levels
			up_level = battle_config.max_adv_level;
		if (val >= sd->status.job_level) {
			if (val > up_level) val = up_level;
			sd->status.skill_point += (val-sd->status.job_level);
			sd->status.job_level = val;
			sd->status.job_exp = 0;
			clif_updatestatus(sd, SP_JOBLEVEL);
			clif_updatestatus(sd, SP_NEXTJOBEXP);
			clif_updatestatus(sd, SP_JOBEXP);
			clif_updatestatus(sd, SP_SKILLPOINT);
			status_calc_pc(sd, 0);
			clif_misceffect(&sd->bl, 1);
		} else {
			sd->status.job_level = val;
			sd->status.job_exp = 0;
			clif_updatestatus(sd, SP_JOBLEVEL);
			clif_updatestatus(sd, SP_NEXTJOBEXP);
			clif_updatestatus(sd, SP_JOBEXP);
			status_calc_pc(sd, 0);
		}
		clif_updatestatus(sd,type);
		break;
	case SP_SKILLPOINT:
		sd->status.skill_point = val;
		break;
	case SP_STATUSPOINT:
		sd->status.status_point = val;
		break;
	case SP_ZENY:
		if(val <= MAX_ZENY) {
			// MAX_ZENY �ȉ��Ȃ���
			sd->status.zeny = val;
		} else {
			sd->status.zeny = MAX_ZENY;
			/* Could someone explain the comments below? I have no idea what they are trying to do... 
			 * if you want to give someone so much zeny, just set their zeny to the max. [Skotlex]
			if(sd->status.zeny > val) {
				// Zeny ���������Ă���Ȃ���
				sd->status.zeny = val;
			} else if(sd->status.zeny <= MAX_ZENY) {
				// Zeny ���������Ă��āA���݂̒l��MAX_ZENY �ȉ��Ȃ�MAX_ZENY
				sd->status.zeny = MAX_ZENY;
			} else {
				// Zeny ���������Ă��āA���݂̒l��MAX_ZENY ��艺�Ȃ瑝�����𖳎�
				;
			}
			*/
		}
		break;
	case SP_BASEEXP:
		if(pc_nextbaseexp(sd) > 0) {
			sd->status.base_exp = val;
			if(sd->status.base_exp < 0)
				sd->status.base_exp=0;
			pc_checkbaselevelup(sd);
		}
		break;
	case SP_JOBEXP:
		if(pc_nextjobexp(sd) > 0) {
			sd->status.job_exp = val;
			if(sd->status.job_exp < 0)
				sd->status.job_exp=0;
			pc_checkjoblevelup(sd);
		}
		break;
	case SP_SEX:
		sd->sex = val;
		break;
	case SP_WEIGHT:
		sd->weight = val;
		break;
	case SP_MAXWEIGHT:
		sd->max_weight = val;
		break;
	case SP_HP:
		sd->status.hp = val;
		break;
	case SP_MAXHP:
		sd->status.max_hp = val;
		break;
	case SP_SP:
		sd->status.sp = val;
		break;
	case SP_MAXSP:
		sd->status.max_sp = val;
		break;
	case SP_STR:
		sd->status.str = val;
		break;
	case SP_AGI:
		sd->status.agi = val;
		break;
	case SP_VIT:
		sd->status.vit = val;
		break;
	case SP_INT:
		sd->status.int_ = val;
		break;
	case SP_DEX:
		sd->status.dex = val;
		break;
	case SP_LUK:
		sd->status.luk = val;
		break;
	case SP_KARMA:
		sd->status.karma = val;
		break;
	case SP_MANNER:
		sd->status.manner = val;
		break;
	case SP_FAME:
		sd->status.fame = val;
		break;
	}
	clif_updatestatus(sd,type);

	return 0;
}

/*==========================================
 * HP/SP��
 *------------------------------------------
 */
int pc_heal(struct map_session_data *sd,int hp,int sp)
{
//	if(battle_config.battle_log)
//		printf("heal %d %d\n",hp,sp);

	nullpo_retr(0, sd);

	if(pc_checkoverhp(sd)) {
		if(hp > 0)
			hp = 0;
	}
	if(pc_checkoversp(sd)) {
		if(sp > 0)
			sp = 0;
	}

	if(sd->sc_count && sd->sc_data[SC_BERSERK].timer!=-1) //�o?�T?�N���͉񕜂����Ȃ��炵��
		return 0;

	if(hp+sd->status.hp>sd->status.max_hp)
		hp=sd->status.max_hp-sd->status.hp;
	if(sp+sd->status.sp>sd->status.max_sp)
		sp=sd->status.max_sp-sd->status.sp;
	sd->status.hp+=hp;
	if(sd->status.hp <= 0) {
		sd->status.hp = 0;
		pc_damage(NULL,sd,1,0);
		hp = 0;
	}
	sd->status.sp+=sp;
	if(sd->status.sp <= 0)
		sd->status.sp = 0;
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	if(sd->status.party_id>0) {	// on-the-fly party hp updates [Valaris]
		struct party *p=party_search(sd->status.party_id);
		if(p!=NULL) clif_party_hp(p,sd);
	}	// end addition [Valaris]

	return hp + sp;
}

/*==========================================
 * HP/SP��
 *------------------------------------------
 */
int pc_itemheal(struct map_session_data *sd,int hp,int sp)
{
	int bonus, type;
//	if(battle_config.battle_log)
//		printf("heal %d %d\n",hp,sp);

	nullpo_retr(0, sd);

	if(sd->sc_count && sd->sc_data[SC_GOSPEL].timer!=-1) //�o?�T?�N���͉񕜂����Ȃ��炵��
		return 0;

	if(sd->state.potion_flag==1) {
		sd->potion_hp = hp;
		sd->potion_sp = sp;
		return 0;
	}

	if(pc_checkoverhp(sd)) {
		if(hp > 0)
			hp = 0;
	}
	if(pc_checkoversp(sd)) {
		if(sp > 0)
			sp = 0;
	}

	if(hp > 0) {
		bonus = (sd->paramc[2]<<1) + 100 + pc_checkskill(sd,SM_RECOVERY)*10
			+ pc_checkskill(sd,AM_LEARNINGPOTION)*5 + (sd->state.potion_flag == 2)*50; // A potion produced by an Alchemist in the Fame Top 10 gets +50% effect [DracoRPG]
		if ((type = itemdb_group(sd->itemid)) > 0 && type <= 7)
			bonus = bonus * (100+sd->itemhealrate[type - 1]) / 100;
		if(bonus != 100)
			hp = hp * bonus / 100;
	}
	if(sp > 0) {
		bonus = (sd->paramc[3]<<1) + 100 + pc_checkskill(sd,MG_SRECOVERY)*10
			+ pc_checkskill(sd,AM_LEARNINGPOTION)*5 + (sd->state.potion_flag == 2)*50; // A potion produced by an Alchemist in the Fame Top 10 gets +50% effect [DracoRPG]
		if(bonus != 100)
			sp = sp * bonus / 100;
	}
	if(hp+sd->status.hp>sd->status.max_hp)
		hp=sd->status.max_hp-sd->status.hp;
	if(sp+sd->status.sp>sd->status.max_sp)
		sp=sd->status.max_sp-sd->status.sp;
	sd->status.hp+=hp;
	if(sd->status.hp <= 0) {
		sd->status.hp = 0;
		pc_damage(NULL,sd,1,0);
		hp = 0;
	}
	sd->status.sp+=sp;
	if(sd->status.sp <= 0)
		sd->status.sp = 0;
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	return 0;
}

/*==========================================
 * HP/SP��
 *------------------------------------------
 */
int pc_percentheal(struct map_session_data *sd,int hp,int sp)
{
	nullpo_retr(0, sd);

	if(sd->state.potion_flag==1) {
		sd->potion_per_hp = hp;
		sd->potion_per_sp = sp;
		return 0;
	}

	if(pc_checkoverhp(sd)) {
		if(hp > 0)
			hp = 0;
	}
	if(pc_checkoversp(sd)) {
		if(sp > 0)
			sp = 0;
	}
	if(hp) {
		if(hp >= 100) {
			sd->status.hp = sd->status.max_hp;
		}
		else if(hp <= -100) {
			sd->status.hp = 0;
			pc_damage(NULL,sd,1,0);
		}
		else {
			sd->status.hp += sd->status.max_hp*hp/100;
			if(sd->status.hp > sd->status.max_hp)
				sd->status.hp = sd->status.max_hp;
			if(sd->status.hp <= 0) {
				sd->status.hp = 0;
				pc_damage(NULL,sd,1,0);
				hp = 0;
			}
		}
	}
	if(sp) {
		if(sp >= 100) {
			sd->status.sp = sd->status.max_sp;
		}
		else if(sp <= -100) {
			sd->status.sp = 0;
		}
		else {
			sd->status.sp += sd->status.max_sp*sp/100;
			if(sd->status.sp > sd->status.max_sp)
				sd->status.sp = sd->status.max_sp;
			if(sd->status.sp < 0)
				sd->status.sp = 0;
		}
	}
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	return 0;
}

/*==========================================
 * �E?�X
 * ��?	job �E�� 0�`23
 *		upper �ʏ� 0, ?�� 1, �{�q 2, ���̂܂� -1
 * Rewrote to make it tidider [Celest]
 *------------------------------------------
 */
int pc_jobchange(struct map_session_data *sd,int job, int upper)
{
	int i;
	int b_class = 0;
	//?����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	struct pc_base_job s_class = pc_calc_base_job(sd->status.class_);

	nullpo_retr(0, sd);

	if (job < 0)
		return 1;
	if (upper < 0 || upper > 2) //����?�����ǂ�����?����
		upper = s_class.upper;

	b_class = job;	//�ʏ�E�Ȃ�job���̂܂��
	if (job < JOB_SUPER_NOVICE) {
		if (upper == 1)
			b_class += JOB_NOVICE_HIGH;
		else if (upper == 2)	//�{�q�Ɍ����͂Ȃ����ǂǂ������ŏR���邩�炢����
			b_class += JOB_BABY;
	} else if (job == JOB_SUPER_NOVICE) {
		if (upper == 1)	//?���ɃX�p�m�r�͑��݂��Ȃ��̂ł�?��
			return 1;
		else if (upper == 2)
			b_class = JOB_SUPER_BABY;
	} else if (job < JOB_SUPER_BABY-JOB_NOVICE_HIGH+JOB_SUPER_NOVICE+2) {
	// Min is SuperNovice +1 -> Becomes Novice High [Skotlex]
	// Max is SuperBaby-NoviceHigh+1 -> Becomes Super Baby
		b_class += JOB_NOVICE_HIGH - JOB_SUPER_NOVICE -1;
	} else if (job >= JOB_TAEKWON && job <= JOB_SOUL_LINKER) {
		if (upper > 0)
			return 1;
	} else if (job < JOB_NOVICE_HIGH || job > JOB_SOUL_LINKER) //Invalid value
		return 1;

	job = pc_calc_base_job2 (b_class); // check base class [celest]

	if((sd->status.sex == 0 && job == JOB_BARD) || (sd->status.sex == 1 && job == JOB_DANCER))
		return 1;

	// check if we are changing from 1st to 2nd job
	if ((job >= JOB_KNIGHT && job <= JOB_CRUSADER2) || (job >= JOB_STAR_GLADIATOR && job <= JOB_SOUL_LINKER)) {
		if ((s_class.job > JOB_NOVICE && s_class.job < JOB_KNIGHT) || s_class.job == JOB_TAEKWON)
			sd->change_level = sd->status.job_level;
		else
			sd->change_level = 40;
	}
 	else
 		sd->change_level = 0;

	pc_setglobalreg (sd, "jobchange_level", sd->change_level);

	sd->status.class_ = sd->view_class = b_class;

	sd->status.job_level=1;
	sd->status.job_exp=0;
	clif_updatestatus(sd,SP_JOBLEVEL);
	clif_updatestatus(sd,SP_JOBEXP);
	clif_updatestatus(sd,SP_NEXTJOBEXP);

	for(i=0;i<11;i++) {
		if(sd->equip_index[i] >= 0)
			if(!pc_isequip(sd,sd->equip_index[i]))
				pc_unequipitem(sd,sd->equip_index[i],2);	// ?���O��
	}

	clif_changelook(&sd->bl,LOOK_BASE,sd->view_class); // move sprite update to prevent client crashes with incompatible equipment [Valaris]

	if(battle_config.save_clothcolor &&
		sd->status.clothes_color > 0 &&
		(sd->view_class != JOB_WEDDING || !battle_config.wedding_ignorepalette)
		)
		clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->status.clothes_color);
	if(battle_config.muting_players && sd->status.manner < 0)
		clif_changestatus(&sd->bl,SP_MANNER,sd->status.manner);

	status_calc_pc(sd,0);
	pc_checkallowskill(sd);
	pc_equiplookall(sd);
	clif_equiplist(sd);

	if(pc_isriding(sd)) {	// remove peco status if changing into invalid class [Valaris]
		if(!(pc_checkskill(sd,KN_RIDING)))
			pc_setoption(sd,sd->status.option|-0x0000);
		else
			pc_setriding(sd);
	}

	return 0;
}

/*==========================================
 * ������?�X
 *------------------------------------------
 */
int pc_equiplookall(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

#if PACKETVER < 4
	clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
	clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
#else
	clif_changelook(&sd->bl,LOOK_WEAPON,0);
	clif_changelook(&sd->bl,LOOK_SHOES,0);
#endif
	clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
	clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
	clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);

	return 0;
}

/*==========================================
 * ������?�X
 *------------------------------------------
 */
int pc_changelook(struct map_session_data *sd,int type,int val)
{
	nullpo_retr(0, sd);

	switch(type){
	case LOOK_HAIR:	//Use the battle_config limits! [Skotlex]
		if (val < battle_config.min_hair_style)
			val = battle_config.min_hair_style;
		else if (val > battle_config.max_hair_style)
			val = battle_config.max_hair_style;
		if (sd->status.hair != val)
		{
			sd->status.hair=val;
			if (sd->status.guild_id) //Update Guild Window. [Skotlex]
				intif_guild_change_memberinfo(sd->status.guild_id,sd->status.account_id,sd->status.char_id,
				GMI_HAIR,&sd->status.hair,sizeof(sd->status.hair));
		}
		break;
	case LOOK_WEAPON:
		sd->status.weapon=val;
		break;
	case LOOK_HEAD_BOTTOM:
		sd->status.head_bottom=val;
		break;
	case LOOK_HEAD_TOP:
		sd->status.head_top=val;
		break;
	case LOOK_HEAD_MID:
		sd->status.head_mid=val;
		break;
	case LOOK_HAIR_COLOR:	//Use the battle_config limits! [Skotlex]
		if (val < battle_config.min_hair_color)
			val = battle_config.min_hair_color;
		else if (val > battle_config.max_hair_color)
			val = battle_config.max_hair_color;
		if (sd->status.hair_color != val)
		{
			sd->status.hair_color=val;
			if (sd->status.guild_id) //Update Guild Window. [Skotlex]
				intif_guild_change_memberinfo(sd->status.guild_id,sd->status.account_id,sd->status.char_id,
				GMI_HAIR_COLOR,&sd->status.hair_color,sizeof(sd->status.hair_color));
		}
		break;
	case LOOK_CLOTHES_COLOR:	//Use the battle_config limits! [Skotlex]
		if (val < battle_config.min_cloth_color)
			val = battle_config.min_cloth_color;
		else if (val > battle_config.max_cloth_color)
			val = battle_config.max_cloth_color;
		sd->status.clothes_color=val;
		break;
	case LOOK_SHIELD:
		sd->status.shield=val;
		break;
	case LOOK_SHOES:
		break;
	}
	clif_changelook(&sd->bl,type,val);

	return 0;
}

/*==========================================
 * �t?�i(��,�y�R,�J?�g)�ݒ�
 *------------------------------------------
 */
int pc_setoption(struct map_session_data *sd,int type)
{
	nullpo_retr(0, sd);
	if (type&0x0020 && !(sd->status.option&0x0020))
	{	//We are going to mount. [Skotlex]
		switch (sd->status.class_)
		{
			case JOB_KNIGHT:
				sd->status.class_ = sd->view_class = JOB_KNIGHT2;
				break;
			case JOB_CRUSADER:
				sd->status.class_ = sd->view_class = JOB_CRUSADER2;
				break;
			case JOB_LORD_KNIGHT:
				sd->status.class_ = sd->view_class = JOB_LORD_KNIGHT2;
				break;
			case JOB_PALADIN:
				sd->status.class_ = sd->view_class = JOB_PALADIN2;
				break;
			case JOB_BABY_KNIGHT:
				sd->status.class_ = sd->view_class = JOB_BABY_KNIGHT2;
				break;
			case JOB_BABY_CRUSADER:
				sd->status.class_ = sd->view_class = JOB_BABY_CRUSADER2;
				break;
		}
	}
	else if (!(type&0x0020) && sd->status.option&0x0020)
	{	//We are going to dismount.
		switch (sd->status.class_)
		{
			case JOB_KNIGHT2:
				sd->status.class_ = sd->view_class = JOB_KNIGHT;
				break;
			case JOB_CRUSADER2:
				sd->status.class_ = sd->view_class = JOB_CRUSADER;
				break;
			case JOB_LORD_KNIGHT2:
				sd->status.class_ = sd->view_class = JOB_LORD_KNIGHT;
				break;
			case JOB_PALADIN2:
				sd->status.class_ = sd->view_class = JOB_PALADIN;
				break;
			case JOB_BABY_KNIGHT2:
				sd->status.class_ = sd->view_class = JOB_BABY_KNIGHT;
				break;
			case JOB_BABY_CRUSADER2:
				sd->status.class_ = sd->view_class = JOB_BABY_CRUSADER;
				break;
		}
	}
	sd->status.option=type;
	clif_changeoption(&sd->bl);
	status_calc_pc(sd,0);
	return 0;
}

/*==========================================
 * �J?�g�ݒ�
 *------------------------------------------
 */
int pc_setcart(struct map_session_data *sd,int type)
{
	int cart[6]={0x0000,0x0008,0x0080,0x0100,0x0200,0x0400};
	int option, i;
	nullpo_retr(0, sd);
	
	if (type < 0 || type > 5)
		return 0; //Never trust the values sent by the client! [Skotlex]

	option = sd->status.option;
	for (i = 0; i < 6; i++)
	{	//This should preserve the current option, only modifying the cart bit.
		if (i == type)
			option |= cart[i];
		else
			option &= ~cart[i];
	}
	if(pc_checkskill(sd,MC_PUSHCART)>0){ // �v�b�V���J?�g�X�L������
		if(!pc_iscarton(sd)){ // �J?�g��t���Ă��Ȃ�
			pc_setoption(sd,option);
			clif_cart_itemlist(sd);
			clif_cart_equiplist(sd);
			clif_updatestatus(sd,SP_CARTINFO);
			clif_status_change(&sd->bl,0x0c,0);
		}
		else{
			pc_setoption(sd,option);
		}
	}

	return 0;
}

/*==========================================
 * ��ݒ�
 *------------------------------------------
 */
int pc_setfalcon(struct map_session_data *sd)
{
	if(pc_checkskill(sd,HT_FALCON)>0){	// �t�@���R���}�X�^��?�X�L������
		pc_setoption(sd,sd->status.option|0x0010);
	}

	return 0;
}

/*==========================================
 * �y�R�y�R�ݒ�
 *------------------------------------------
 */
int pc_setriding(struct map_session_data *sd)
{
	if((pc_checkskill(sd,KN_RIDING)>0)){ // ���C�f�B���O�X�L������
		pc_setoption(sd,sd->status.option|0x0020);
	}
	return 0;
}

/*==========================================
 * �A�C�e���h���b�v�s����
 *------------------------------------------
 */
int pc_candrop(struct map_session_data *sd,int item_id)
{
	int level = pc_isGM(sd);
	if (level > 0 && level < battle_config.gm_can_drop_lv)
		return 0;
	return (itemdb_isdropable(item_id, level));
}

/*==========================================
 * script�p??�̒l��?��
 *------------------------------------------
 */
int pc_readreg(struct map_session_data *sd,int reg)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->reg_num;i++)
		if(sd->reg[i].index==reg)
			return sd->reg[i].data;

	return 0;
}
/*==========================================
 * script�p??�̒l��ݒ�
 *------------------------------------------
 */
int pc_setreg(struct map_session_data *sd,int reg,int val)
{
	int i;

	nullpo_retr(0, sd);

	for (i = 0; i < sd->reg_num; i++) {
		if (sd->reg[i].index == reg){
			sd->reg[i].data = val;
			return 0;
		}
	}
	sd->reg_num++;
	sd->reg = (struct script_reg *) aRealloc(sd->reg, sizeof(*(sd->reg)) * sd->reg_num);
	memset(sd->reg + (sd->reg_num - 1), 0, sizeof(struct script_reg));
	sd->reg[i].index = reg;
	sd->reg[i].data = val;

	return 0;
}

/*==========================================
 * script�p������??�̒l��?��
 *------------------------------------------
 */
char *pc_readregstr(struct map_session_data *sd,int reg)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->regstr_num;i++)
		if(sd->regstr[i].index==reg)
			return sd->regstr[i].data;

	return NULL;
}
/*==========================================
 * script�p������??�̒l��ݒ�
 *------------------------------------------
 */
int pc_setregstr(struct map_session_data *sd,int reg,char *str)
{
	int i;

	nullpo_retr(0, sd);

	if(strlen(str)+1 >= sizeof(sd->regstr[0].data)){
		ShowWarning("pc_setregstr: string too long !\n");
		return 0;
	}

	for(i=0;i<sd->regstr_num;i++)
		if(sd->regstr[i].index==reg){
			strcpy(sd->regstr[i].data,str);
			return 0;
		}

	sd->regstr_num++;
	sd->regstr = (struct script_regstr *) aRealloc(sd->regstr, sizeof(sd->regstr[0]) * sd->regstr_num);
	if(sd->regstr==NULL){
		ShowFatalError("out of memory : pc_setreg\n");
		exit(1);
	}
	memset(sd->regstr + (sd->regstr_num - 1), 0, sizeof(struct script_regstr));
	sd->regstr[i].index = reg;
	strcpy(sd->regstr[i].data, str);

	return 0;
}

/*==========================================
 * script�p�O��?�o��??�̒l��?��
 *------------------------------------------
 */
int pc_readglobalreg(struct map_session_data *sd,char *reg)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->status.global_reg_num;i++){
		if(strcmp(sd->status.global_reg[i].str,reg)==0)
			return sd->status.global_reg[i].value;
	}

	return 0;
}

/*==========================================
 * script�p�O��?�o��??�̒l��ݒ�
 *------------------------------------------
 */
int pc_setglobalreg(struct map_session_data *sd,char *reg,int val)
{
	int i;

	nullpo_retr(0, sd);

	//PC_DIE_COUNTER���X�N���v�g�Ȃǂ�?�X���ꂽ�b�?��
	if(strcmp(reg,"PC_DIE_COUNTER") == 0 && sd->die_counter != val){
		sd->die_counter = val;
		status_calc_pc(sd,0);
	} else if(strcmp(reg,script_config.die_event_name) == 0){
		sd->state.event_death = val;
	} else if(strcmp(reg,script_config.kill_event_name) == 0){
		sd->state.event_kill = val;
	} else if(strcmp(reg,script_config.logout_event_name) == 0){
		sd->state.event_disconnect = val;
	}

	// delete reg
	if (val == 0) {
		for(i = 0; i < sd->status.global_reg_num; i++) {
			if (strcmp(sd->status.global_reg[i].str, reg) == 0) {
				if (i != sd->status.global_reg_num - 1)
					memcpy(&sd->status.global_reg[i], &sd->status.global_reg[sd->status.global_reg_num - 1], sizeof(struct global_reg));
				memset(&sd->status.global_reg[sd->status.global_reg_num - 1], 0, sizeof(struct global_reg));
				sd->status.global_reg_num--;
				break;
			}
		}
		return 0;
	}

	// change value if found
	for(i = 0; i < sd->status.global_reg_num; i++) {
		if (strcmp(sd->status.global_reg[i].str, reg) == 0) {
			sd->status.global_reg[i].value = val;
			return 0;
		}
	}
	// add value if not found
	if (sd->status.global_reg_num < GLOBAL_REG_NUM) {
		memset(&sd->status.global_reg[i], 0, sizeof(struct global_reg));
		strncpy(sd->status.global_reg[i].str, reg, 32);
		sd->status.global_reg[i].value = val;
		sd->status.global_reg_num++;
		return 0;
	}

	if(battle_config.error_log)
		ShowError("pc_setglobalreg : couldn't set %s (GLOBAL_REG_NUM = %d)\n", reg, GLOBAL_REG_NUM);

	return 1;
}

/*==========================================
 * script�p�A�J�E���g??�̒l��?��
 *------------------------------------------
 */
int pc_readaccountreg(struct map_session_data *sd,char *reg)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->status.account_reg_num;i++){
		if(strcmp(sd->status.account_reg[i].str,reg)==0)
			return sd->status.account_reg[i].value;
	}

	return 0;
}
/*==========================================
 * script�p�A�J�E���g??�̒l��ݒ�
 *------------------------------------------
 */
int pc_setaccountreg(struct map_session_data *sd,char *reg,int val)
{
	int i;

	nullpo_retr(0, sd);

	if (sd->status.account_reg_num == -1) {
		if(battle_config.error_log)
			ShowError("pc_setaccountreg : refusing to set until vars are received\n");
		return 1;
	}
	
	sd->state.accreg_dirty = 1; //Mark the registry dirty until saved. [Skotlex]

	// delete reg
	if (val == 0) {
		for(i = 0; i < sd->status.account_reg_num; i++) {
			if (strcmp(sd->status.account_reg[i].str, reg) == 0) {
				if (i != sd->status.account_reg_num - 1)
					memcpy(&sd->status.account_reg[i], &sd->status.account_reg[sd->status.account_reg_num - 1], sizeof(struct global_reg));
				memset(&sd->status.account_reg[sd->status.account_reg_num - 1], 0, sizeof(struct global_reg));
				sd->status.account_reg_num--;
				intif_saveaccountreg(sd);
				break;
			}
		}
		return 0;
	}

	// change value if found
	for(i = 0; i < sd->status.account_reg_num; i++) {
		if (strcmp(sd->status.account_reg[i].str, reg) == 0) {
			sd->status.account_reg[i].value = val;
			intif_saveaccountreg(sd);
			return 0;
		}
	}
	// add value if not found
	if (sd->status.account_reg_num < ACCOUNT_REG_NUM) {
		memset(&sd->status.account_reg[i], 0, sizeof(struct global_reg));
		strncpy(sd->status.account_reg[i].str, reg, 32);
		sd->status.account_reg[i].value = val;
		sd->status.account_reg_num++;
		intif_saveaccountreg(sd);
		return 0;
	}

	if(battle_config.error_log)
		ShowError("pc_setaccountreg : couldn't set %s (ACCOUNT_REG_NUM = %d)\n", reg, ACCOUNT_REG_NUM);

	return 1;
}
/*==========================================
 * script�p�A�J�E���g??2�̒l��?��
 *------------------------------------------
 */
int pc_readaccountreg2(struct map_session_data *sd,char *reg)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->status.account_reg2_num;i++){
		if(strcmp(sd->status.account_reg2[i].str,reg)==0)
			return sd->status.account_reg2[i].value;
	}

	return 0;
}
/*==========================================
 * script�p�A�J�E���g??2�̒l��ݒ�
 *------------------------------------------
 */
int pc_setaccountreg2(struct map_session_data *sd,char *reg,int val)
{
	int i;

	nullpo_retr(1, sd);

	// delete reg
	if (val == 0) {
		for(i = 0; i < sd->status.account_reg2_num; i++) {
			if (strcmp(sd->status.account_reg2[i].str, reg) == 0) {
				if (i != sd->status.account_reg2_num - 1)
					memcpy(&sd->status.account_reg2[i], &sd->status.account_reg2[sd->status.account_reg2_num - 1], sizeof(struct global_reg));
				memset(&sd->status.account_reg2[sd->status.account_reg2_num - 1], 0, sizeof(struct global_reg));
				sd->status.account_reg2_num--;
				chrif_saveaccountreg2(sd);
				break;
			}
		}
		return 0;
	}

	// change value if found
	for(i = 0; i < sd->status.account_reg2_num; i++) {
		if (strcmp(sd->status.account_reg2[i].str, reg) == 0) {
			sd->status.account_reg2[i].value = val;
			chrif_saveaccountreg2(sd);
			return 0;
		}
	}
	// add value if not found
	if (sd->status.account_reg2_num < ACCOUNT_REG2_NUM) {
		memset(&sd->status.account_reg2[i], 0, sizeof(struct global_reg));
		strncpy(sd->status.account_reg2[i].str, reg, 32);
		sd->status.account_reg2[i].value = val;
		sd->status.account_reg2_num++;
		chrif_saveaccountreg2(sd);
		return 0;
	}

	if (battle_config.error_log)
		ShowError("pc_setaccountreg2 : couldn't set %s (ACCOUNT_REG2_NUM = %d)\n", reg, ACCOUNT_REG2_NUM);

	return 1;
}

/*==========================================
 * �C�x���g�^�C�}??��
 *------------------------------------------
 */
int pc_eventtimer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	char *p = (char *)data;
	int i;
	if(sd==NULL)
		return 0;

	for(i=0;i < MAX_EVENTTIMER;i++){
		if( sd->eventtimer[i]==tid ){
			sd->eventtimer[i]=-1;
			npc_event(sd,p,0);
			break;
		}
	}
	if (p) aFree(p);
	if(i==MAX_EVENTTIMER) {
		if(battle_config.error_log)
			ShowError("pc_eventtimer: no such event timer\n");
	}

	return 0;
}

/*==========================================
 * �C�x���g�^�C�}?�ǉ�
 *------------------------------------------
 */
int pc_addeventtimer(struct map_session_data *sd,int tick,const char *name)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]==-1 )
			break;
	if(i<MAX_EVENTTIMER){
		char *evname = aStrdup(name);
		//char *evname=(char *)aMallocA((strlen(name)+1)*sizeof(char));
		//memcpy(evname,name,(strlen(name)+1));
		sd->eventtimer[i]=add_timer(gettick()+tick,
			pc_eventtimer,sd->bl.id,(int)evname);
		sd->eventcount++;
	}

	return 0;
}

/*==========================================
 * �C�x���g�^�C�}?�폜
 *------------------------------------------
 */
int pc_deleventtimer(struct map_session_data *sd,const char *name)
{
	int i;

	nullpo_retr(0, sd);

	if (sd->eventcount <= 0)
		return 0;

	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]!=-1 ) {
			char *p = (char *)(get_timer(sd->eventtimer[i])->data);
			if(p && strcmp(p, name)==0) {
				delete_timer(sd->eventtimer[i],pc_eventtimer);
				sd->eventtimer[i]=-1;
				sd->eventcount--;
				aFree(p);
				break;
			}
		}

	return 0;
}

/*==========================================
 * �C�x���g�^�C�}?�J�E���g�l�ǉ�
 *------------------------------------------
 */
int pc_addeventtimercount(struct map_session_data *sd,const char *name,int tick)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]!=-1 && strcmp(
			(char *)(get_timer(sd->eventtimer[i])->data), name)==0 ){
				addtick_timer(sd->eventtimer[i],tick);
				break;
		}

	return 0;
}

/*==========================================
 * �C�x���g�^�C�}?�S�폜
 *------------------------------------------
 */
int pc_cleareventtimer(struct map_session_data *sd)
{
	int i;

	nullpo_retr(0, sd);

	if (sd->eventcount <= 0)
		return 0;

	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]!=-1 ){
			char *p = (char *)(get_timer(sd->eventtimer[i])->data);
			delete_timer(sd->eventtimer[i],pc_eventtimer);
			sd->eventtimer[i]=-1;
			if (p) aFree(p);
		}

	return 0;
}

//
// ? ����
//
/*==========================================
 * �A�C�e����?������
 *------------------------------------------
 */
int pc_equipitem(struct map_session_data *sd,int n,int pos)
{
	int i,nameid, arrow;
	struct item_data *id;
	//?����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����

	nullpo_retr(0, sd);

	nameid = sd->status.inventory[n].nameid;
	id = sd->inventory_data[n];
	pos = pc_equippoint(sd,n);
	if(battle_config.battle_log)
		ShowInfo("equip %d(%d) %x:%x\n",nameid,n,id->equip,pos);
	if(!pc_isequip(sd,n) || !pos || sd->status.inventory[n].attribute==1 ) { // [Valaris]
		clif_equipitemack(sd,n,0,0);	// fail
		return 0;
	}

// -- moonsoul (if player is berserk then cannot equip)
//
	if(sd->sc_count && sd->sc_data[SC_BERSERK].timer!=-1){
		clif_equipitemack(sd,n,0,0);	// fail
		return 0;
	}

	if(pos==0x88){ // �A�N�Z�T���p��O?��
		int epor=0;
		if(sd->equip_index[0] >= 0)
			epor |= sd->status.inventory[sd->equip_index[0]].equip;
		if(sd->equip_index[1] >= 0)
			epor |= sd->status.inventory[sd->equip_index[1]].equip;
		epor &= 0x88;
		pos = epor == 0x08 ? 0x80 : 0x08;
	}

	// �񓁗�?��
	if ((pos==0x22) // ��?�A?���v���ӏ����񓁗����킩�`�F�b�N����
	 &&	(id->equip==2)	// ? �蕐��
	 &&	(pc_checkskill(sd, AS_LEFT) > 0 || pc_calc_base_job2(sd->status.class_) == JOB_ASSASSIN) ) // ����C?�L
	{
		int tpos=0;
		if(sd->equip_index[8] >= 0)
			tpos |= sd->status.inventory[sd->equip_index[8]].equip;
		if(sd->equip_index[9] >= 0)
			tpos |= sd->status.inventory[sd->equip_index[9]].equip;
		tpos &= 0x02;
		pos = tpos == 0x02 ? 0x20 : 0x02;
	}

	arrow=pc_search_inventory(sd,pc_checkequip(sd,9));	// Added by RoVeRT
	for(i=0;i<11;i++) {
		if(sd->equip_index[i] >= 0 && sd->status.inventory[sd->equip_index[i]].equip&pos) {
			pc_unequipitem(sd,sd->equip_index[i],2);
		}
	}
	// �|��?��
	if(pos==0x8000){
		clif_arrowequip(sd,n);
		clif_arrow_fail(sd,3);	// 3=�?���ł��܂���
	}
	else
		clif_equipitemack(sd,n,pos,1);

	for(i=0;i<11;i++) {
		if(pos & equip_pos[i])
			sd->equip_index[i] = n;
	}
	sd->status.inventory[n].equip=pos;

	if(sd->status.inventory[n].equip & 0x0002) {
		if(sd->inventory_data[n])
			sd->weapontype1 = sd->inventory_data[n]->look;
		else
			sd->weapontype1 = 0;
		pc_calcweapontype(sd);
		clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
	}
	if(sd->status.inventory[n].equip & 0x0020) {
		if(sd->inventory_data[n]) {
			if(sd->inventory_data[n]->type == 4) {
				sd->status.shield = 0;
				if(sd->status.inventory[n].equip == 0x0020)
					sd->weapontype2 = sd->inventory_data[n]->look;
				else
					sd->weapontype2 = 0;
			}
			else if(sd->inventory_data[n]->type == 5) {
				sd->status.shield = sd->inventory_data[n]->look;
				sd->weapontype2 = 0;
			}
		}
		else
			sd->status.shield = sd->weapontype2 = 0;
		pc_calcweapontype(sd);
		clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
	}
	if(sd->status.inventory[n].equip & 0x0001) {
		if(sd->inventory_data[n])
			sd->status.head_bottom = sd->inventory_data[n]->look;
		else
			sd->status.head_bottom = 0;
		clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
	}
	if(sd->status.inventory[n].equip & 0x0100) {
		if(sd->inventory_data[n])
			sd->status.head_top = sd->inventory_data[n]->look;
		else
			sd->status.head_top = 0;
		clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
	}
	if(sd->status.inventory[n].equip & 0x0200) {
		if(sd->inventory_data[n])
			sd->status.head_mid = sd->inventory_data[n]->look;
		else
			sd->status.head_mid = 0;
		clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);
	}
	if(sd->status.inventory[n].equip & 0x0040)
		clif_changelook(&sd->bl,LOOK_SHOES,0);

	pc_checkallowskill(sd);	// ?���i�ŃX�L������������邩�`�F�b�N
	if (itemdb_look(sd->status.inventory[n].nameid) == 11 && (arrow >= 0)){	// Added by RoVeRT
		clif_arrowequip(sd,arrow);
		sd->status.inventory[arrow].equip=32768;
	}
	status_calc_pc(sd,0);

	if(sd->special_state.infinite_endure) {
		if(sd->sc_data[SC_ENDURE].timer == -1)
			status_change_start(&sd->bl,SC_ENDURE,10,1,0,0,0,0);
	}
	else {
		if(sd->sc_count && sd->sc_data[SC_ENDURE].timer != -1 && sd->sc_data[SC_ENDURE].val2)
			status_change_end(&sd->bl,SC_ENDURE,-1);
	}

	if(sd->sc_count) {
		if (sd->sc_data[SC_SIGNUMCRUCIS].timer != -1 && !battle_check_undead(7,sd->def_ele))
			status_change_end(&sd->bl,SC_SIGNUMCRUCIS,-1);
		if(sd->sc_data[SC_DANCING].timer!=-1 && (sd->status.weapon != 13 && sd->status.weapon !=14))
			skill_stop_dancing(&sd->bl);
	}

	return 0;
}

/*==========================================
 * ? �����������O��
 * type:
 * 0 - only unequip
 * 1 - calculate status after unequipping
 * 2 - force unequip
 *------------------------------------------
 */
int pc_unequipitem(struct map_session_data *sd,int n,int flag)
{
	short hp = 0, sp = 0;
	nullpo_retr(0, sd);

// -- moonsoul	(if player is berserk then cannot unequip)
//
	if(!(flag&2) && sd->sc_count && (sd->sc_data[SC_BLADESTOP].timer!=-1 || sd->sc_data[SC_BERSERK].timer!=-1)){
		clif_unequipitemack(sd,n,0,0);
		return 0;
	}

	if(battle_config.battle_log)
		ShowInfo("unequip %d %x:%x\n",n,pc_equippoint(sd,n),sd->status.inventory[n].equip);
	if(sd->status.inventory[n].equip){
		int i;
		for(i=0;i<11;i++) {
			if(sd->status.inventory[n].equip & equip_pos[i]) {
				sd->equip_index[i] = -1;
				if(sd->unequip_losehp[i] > 0) {
					hp += sd->unequip_losehp[i];
					sd->unequip_losehp[i] = 0;
				}
				if(sd->unequip_losesp[i] > 0) {
					sp += sd->unequip_losesp[i];
					sd->unequip_losesp[i] = 0;
				}
			}
		}
		if(sd->status.inventory[n].equip & 0x0002) {
			sd->weapontype1 = 0;
			sd->status.weapon = sd->weapontype2;
			pc_calcweapontype(sd);
			clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
		}
		if(sd->status.inventory[n].equip & 0x0020) {
			sd->status.shield = sd->weapontype2 = 0;
			pc_calcweapontype(sd);
			clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
		}
		if(sd->status.inventory[n].equip & 0x0001) {
			sd->status.head_bottom = 0;
			clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
		}
		if(sd->status.inventory[n].equip & 0x0100) {
			sd->status.head_top = 0;
			clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
		}
		if(sd->status.inventory[n].equip & 0x0200) {
			sd->status.head_mid = 0;
			clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);
		}
		if(sd->status.inventory[n].equip & 0x0040)
			clif_changelook(&sd->bl,LOOK_SHOES,0);

		clif_unequipitemack(sd,n,sd->status.inventory[n].equip,1);
		sd->status.inventory[n].equip=0;
		if(flag&1)
			pc_checkallowskill(sd);
		if(sd->weapontype1 == 0 && sd->weapontype2 == 0)
			skill_enchant_elemental_end(&sd->bl,-1);  //���펝�������͖�?����?���t?����
	} else {
		clif_unequipitemack(sd,n,0,0);
	}

	if(flag&1) {
		status_calc_pc(sd,0);
		if(sd->sc_count && sd->sc_data[SC_SIGNUMCRUCIS].timer != -1 && !battle_check_undead(7,sd->def_ele))
			status_change_end(&sd->bl,SC_SIGNUMCRUCIS,-1);
	}

	if (hp > 0 || sp > 0) {
		if (hp > sd->status.hp)
			hp = sd->status.hp;
		if (sp > sd->status.sp)
			sp = sd->status.sp;
		pc_heal(sd,-hp,-sp);
	}

	return 0;
}

/*==========================================
 * �A�C�e����index��?���l�߂���
 * ? ���i��?���\�`�F�b�N���s�Ȃ�
 *------------------------------------------
 */
int pc_checkitem(struct map_session_data *sd)
{
	int i,j,k,id,calc_flag = 0;
	struct item_data *it=NULL;

	nullpo_retr(0, sd);

	// �����i�󂫋l��
	for(i=j=0;i<MAX_INVENTORY;i++){
		if( (id=sd->status.inventory[i].nameid)==0)
			continue;
		if( battle_config.item_check && !itemdb_available(id) ){
			if(battle_config.error_log)
				ShowWarning("illegal item id %d in %d[%s] inventory.\n",id,sd->bl.id,sd->status.name);
			pc_delitem(sd,i,sd->status.inventory[i].amount,3);
			continue;
		}
		if(i>j){
			memcpy(&sd->status.inventory[j],&sd->status.inventory[i],sizeof(struct item));
			sd->inventory_data[j] = sd->inventory_data[i];
		}
		j++;
	}
	if(j < MAX_INVENTORY)
		memset(&sd->status.inventory[j],0,sizeof(struct item)*(MAX_INVENTORY-j));
	for(k=j;k<MAX_INVENTORY;k++)
		sd->inventory_data[k] = NULL;

	// �J?�g?�󂫋l��
	for(i=j=0;i<MAX_CART;i++){
		if( (id=sd->status.cart[i].nameid)==0 )
			continue;
		if( battle_config.item_check &&  !itemdb_available(id) ){
			if(battle_config.error_log)
				ShowWarning("illeagal item id %d in %d[%s] cart.\n",id,sd->bl.id,sd->status.name);
			pc_cart_delitem(sd,i,sd->status.cart[i].amount,1);
			continue;
		}
		if(i>j){
			memcpy(&sd->status.cart[j],&sd->status.cart[i],sizeof(struct item));
		}
		j++;
	}
	if(j < MAX_CART)
		memset(&sd->status.cart[j],0,sizeof(struct item)*(MAX_CART-j));

	// ? ���ʒu�`�F�b�N

	for(i=0;i<MAX_INVENTORY;i++){

		it=sd->inventory_data[i];

		if(sd->status.inventory[i].nameid==0)
			continue;
		if(sd->status.inventory[i].equip & ~pc_equippoint(sd,i)) {
			sd->status.inventory[i].equip=0;
			calc_flag = 1;
		}
		//?�������`�F�b�N
		if(sd->status.inventory[i].equip && map[sd->bl.m].flag.pvp && (it->flag.no_equip&1)){//PVP check for forbiden items. optimized by [Lupus]
			sd->status.inventory[i].equip=0;
			calc_flag = 1;
		}else if(sd->status.inventory[i].equip && map[sd->bl.m].flag.gvg && (it->flag.no_equip>1)){//GvG optimized by [Lupus]
			sd->status.inventory[i].equip=0;
			calc_flag = 1;
		}
	}

	pc_setequipindex(sd);
	if(calc_flag)
		status_calc_pc(sd,2);

	return 0;
}

int pc_checkoverhp(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->status.hp == sd->status.max_hp)
		return 1;
	if(sd->status.hp > sd->status.max_hp) {
		sd->status.hp = sd->status.max_hp;
		clif_updatestatus(sd,SP_HP);
		return 2;
	}

	return 0;
}

int pc_checkoversp(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->status.sp == sd->status.max_sp)
		return 1;
	if(sd->status.sp > sd->status.max_sp) {
		sd->status.sp = sd->status.max_sp;
		clif_updatestatus(sd,SP_SP);
		return 2;
	}

	return 0;
}

/*==========================================
 * PVP���ʌv�Z�p(foreachinarea)
 *------------------------------------------
 */
int pc_calc_pvprank_sub(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd1,*sd2=NULL;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd1=(struct map_session_data *)bl);
	nullpo_retr(0, sd2=va_arg(ap,struct map_session_data *));

	if( sd1->pvp_point > sd2->pvp_point )
		sd2->pvp_rank++;
	return 0;
}
/*==========================================
 * PVP���ʌv�Z
 *------------------------------------------
 */
int pc_calc_pvprank(struct map_session_data *sd)
{
	int old;
	struct map_data *m;

	nullpo_retr(0, sd);
	nullpo_retr(0, m=&map[sd->bl.m]);

	old=sd->pvp_rank;

	if( !(m->flag.pvp) )
		return 0;
	sd->pvp_rank=1;
	map_foreachinarea(pc_calc_pvprank_sub,sd->bl.m,0,0,m->xs,m->ys,BL_PC,sd);
	if(old!=sd->pvp_rank || sd->pvp_lastusers!=m->users)
		clif_pvpset(sd,sd->pvp_rank,sd->pvp_lastusers=m->users,0);
	return sd->pvp_rank;
}
/*==========================================
 * PVP���ʌv�Z(timer)
 *------------------------------------------
 */
int pc_calc_pvprank_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=NULL;
	if(battle_config.pk_mode) // disable pvp ranking if pk_mode on [Valaris]
		return 0;

	sd=map_id2sd(id);
	if(sd==NULL)
		return 0;
	sd->pvp_timer=-1;
	if( pc_calc_pvprank(sd)>0 )
		sd->pvp_timer=add_timer(
			gettick()+PVP_CALCRANK_INTERVAL,
			pc_calc_pvprank_timer,id,data);
	return 0;
}

/*==========================================
 * sd�͌������Ă��邩(?���̏ꍇ�͑�����char_id��Ԃ�)
 *------------------------------------------
 */
int pc_ismarried(struct map_session_data *sd)
{
	if(sd == NULL)
		return -1;
	if(sd->status.partner_id > 0)
		return sd->status.partner_id;
	else
		return 0;
}
/*==========================================
 * sd��dstsd�ƌ���(dstsd��sd�̌���?�������bɍs��)
 *------------------------------------------
 */
int pc_marriage(struct map_session_data *sd,struct map_session_data *dstsd)
{
	if(sd == NULL || dstsd == NULL ||
		sd->status.partner_id > 0 || dstsd->status.partner_id > 0 ||
		pc_calc_upper(sd->status.class_) == 2)
		return -1;
	sd->status.partner_id = dstsd->status.char_id;
	dstsd->status.partner_id = sd->status.char_id;
	return 0;
}

/*==========================================
 * sd������(�����sd->status.partner_id�Ɉ˂�)(��������bɗ���?�����w�֎���?�D)
 *------------------------------------------
 */
int pc_divorce(struct map_session_data *sd)
{
	struct map_session_data *p_sd;
	if (sd == NULL || !pc_ismarried(sd))
		return -1;

	if ((p_sd = map_charid2sd(sd->status.partner_id)) != NULL) {
		int i;
		if (p_sd->status.partner_id != sd->status.char_id || sd->status.partner_id != p_sd->status.char_id) {
			ShowWarning("pc_divorce: Illegal partner_id sd=%d p_sd=%d\n", sd->status.partner_id, p_sd->status.partner_id);
			return -1;
		}
		sd->status.partner_id = 0;
		p_sd->status.partner_id = 0;
		for (i = 0; i < MAX_INVENTORY; i++)
			if (sd->status.inventory[i].nameid == WEDDING_RING_M || sd->status.inventory[i].nameid == WEDDING_RING_F)
				pc_delitem(sd, i, 1, 0);
		for (i = 0; i < MAX_INVENTORY; i++)
			if (p_sd->status.inventory[i].nameid == WEDDING_RING_M || p_sd->status.inventory[i].nameid == WEDDING_RING_F)
				pc_delitem(p_sd, i, 1, 0);
		clif_divorced(sd, p_sd->status.name);
		clif_divorced(p_sd, sd->status.name);
	} else {
		ShowError("pc_divorce: p_sd nullpo\n");
		return -1;
	}
	return 0;
}

/*==========================================
 * sd - father dstsd - mother jasd - child
 */
int pc_adoption(struct map_session_data *sd,struct map_session_data *dstsd, struct map_session_data *jasd)
{       
	int j;          
	if (sd == NULL || dstsd == NULL || jasd == NULL ||
		sd->status.partner_id <= 0 || dstsd->status.partner_id <= 0 ||
		sd->status.partner_id != dstsd->status.char_id || dstsd->status.partner_id != sd->status.char_id ||
		sd->status.child > 0 || dstsd->status.child || jasd->status.father > 0 || jasd->status.mother > 0)
			return -1;
	jasd->status.father = sd->status.char_id;
	jasd->status.mother = dstsd->status.char_id;
	sd->status.child = jasd->status.char_id;
	dstsd->status.child = jasd->status.char_id;

	for (j=0; j < MAX_INVENTORY; j++) {
		if(jasd->status.inventory[j].nameid>0 && jasd->status.inventory[j].equip!=0)
			pc_unequipitem(jasd, j, 3);
	}
	if (pc_jobchange(jasd, 4023, 0) == 0)
	{	//Success, and give Junior the Baby skills. [Skotlex]
		pc_skill(jasd,WE_BABY,1,0);
		pc_skill(jasd,WE_CALLPARENT,1,0);
		clif_displaymessage(jasd->fd, msg_txt(12)); // Your job has been changed.
		//We should also grant the parent skills to the parents [Skotlex]
		pc_skill(sd,WE_CALLBABY,1,0);
		pc_skill(dstsd,WE_CALLBABY,1,0);
	} else {
		clif_displaymessage(jasd->fd, msg_txt(155)); // Impossible to change your job.
		return -1;
	}
	return 0;
}

/*==========================================
 * sd�̑�����map_session_data��Ԃ�
 *------------------------------------------
 */
struct map_session_data *pc_get_partner(struct map_session_data *sd)
{
	//struct map_session_data *p_sd = NULL;
	//char *nick;
	//if(sd == NULL || !pc_ismarried(sd))
	//	return NULL;
	//nick=map_charid2nick(sd->status.partner_id);
	//if (nick==NULL)
	//	return NULL;
	//if((p_sd=map_nick2sd(nick)) == NULL )
	//	return NULL;

	if (sd && pc_ismarried(sd))
		// charid2sd returns NULL if not found
		return map_charid2sd(sd->status.partner_id);

	return NULL;
}

struct map_session_data *pc_get_father (struct map_session_data *sd)
{
	if (sd && pc_calc_upper(sd->status.class_) == 2 && sd->status.father > 0)
		// charid2sd returns NULL if not found
		return map_charid2sd(sd->status.father);

	return NULL;
}

struct map_session_data *pc_get_mother (struct map_session_data *sd)
{
	if (sd && pc_calc_upper(sd->status.class_) == 2 && sd->status.mother > 0)
		// charid2sd returns NULL if not found
		return map_charid2sd(sd->status.mother);

	return NULL;
}

struct map_session_data *pc_get_child (struct map_session_data *sd)
{
	if (sd && pc_ismarried(sd) && sd->status.child > 0)
		// charid2sd returns NULL if not found
		return map_charid2sd(sd->status.child);

	return NULL;
}

//
// ���R�񕜕�
//
/*==========================================
 * SP�񕜗ʌv�Z
 *------------------------------------------
 */
static int natural_heal_tick,natural_heal_prev_tick,natural_heal_diff_tick;
static int pc_spheal(struct map_session_data *sd)
{
	int a = natural_heal_diff_tick;
	
	nullpo_retr(0, sd);
	
	if(pc_issit(sd))
		a += a;
	if (sd->sc_count) {
		if (sd->sc_data[SC_MAGNIFICAT].timer!=-1)	// �}�O�j�t�B�J?�g
			a += a;
		if (sd->sc_data[SC_REGENERATION].timer != -1)
			a *= sd->sc_data[SC_REGENERATION].val1;
	}
	// Re-added back to status_calc
	//if((skill = pc_checkskill(sd,HP_MEDITATIO)) > 0) //Increase natural SP regen with Meditatio [DracoRPG]
		//a += a*skill*3/100;
	
	if (sd->status.guild_id > 0) {
		struct guild_castle *gc = guild_mapname2gc(sd->mapname);	// Increased guild castle regen [Valaris]
		if(gc)	{
			struct guild *g = guild_search(sd->status.guild_id);
			if(g && g->guild_id == gc->guild_id)
				a += a;
		}	// end addition [Valaris]
	}

	if (map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKREGEN))
		a += a;

	return a;
}

/*==========================================
 * HP�񕜗ʌv�Z
 *------------------------------------------
 */
static int pc_hpheal(struct map_session_data *sd)
{
	int a = natural_heal_diff_tick;

	nullpo_retr(0, sd);

	if(pc_issit(sd))
		a += a;
	if (sd->sc_count) {
		if (sd->sc_data[SC_MAGNIFICAT].timer != -1)	// Modified by RoVeRT
			a += a;
		if (sd->sc_data[SC_REGENERATION].timer != -1)
			a *= sd->sc_data[SC_REGENERATION].val1;
	}
	if (sd->status.guild_id > 0) {
		struct guild_castle *gc = guild_mapname2gc(sd->mapname);	// Increased guild castle regen [Valaris]
		if(gc)	{
			struct guild *g = guild_search(sd->status.guild_id);
			if(g && g->guild_id == gc->guild_id)
				a += a;
		}	// end addition [Valaris]
	}

	if (map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKREGEN))
		a += a;

	return a;
}

static int pc_natural_heal_hp(struct map_session_data *sd)
{
	int bhp;
	int inc_num,bonus,hp_flag;

	nullpo_retr(0, sd);

	if (sd->sc_count && sd->sc_data[SC_TRICKDEAD].timer != -1)		// Modified by RoVeRT
		return 0;

	if (sd->no_regen & 1)
		return 0;

	if(pc_checkoverhp(sd)) {
		sd->hp_sub = sd->inchealhptick = 0;
		return 0;
	}

	bhp=sd->status.hp;
	hp_flag = (pc_checkskill(sd,SM_MOVINGRECOVERY) > 0 && sd->walktimer != -1);

	if(sd->walktimer == -1) {
		inc_num = pc_hpheal(sd);
		if(sd->sc_data[SC_TENSIONRELAX].timer!=-1 ){	// �e���V���������b�N�X
			sd->hp_sub += 2*inc_num;
			sd->inchealhptick += 3*natural_heal_diff_tick;
		} else {
			sd->hp_sub += inc_num;
			sd->inchealhptick += natural_heal_diff_tick;
		}
	}
	else if(hp_flag) {
		inc_num = pc_hpheal(sd);
		sd->hp_sub += inc_num;
		sd->inchealhptick = 0;
	}
	else {
		sd->hp_sub = sd->inchealhptick = 0;
		return 0;
	}

	if(sd->hp_sub >= battle_config.natural_healhp_interval) {
		bonus = sd->nhealhp;
		if(hp_flag) {
			bonus >>= 2;
			if(bonus <= 0) bonus = 1;
		}
		while(sd->hp_sub >= battle_config.natural_healhp_interval) {
			sd->hp_sub -= battle_config.natural_healhp_interval;
			if(sd->status.hp + bonus <= sd->status.max_hp)
				sd->status.hp += bonus;
			else {
				sd->status.hp = sd->status.max_hp;
				sd->hp_sub = sd->inchealhptick = 0;
			}
		}
	}
	if(bhp!=sd->status.hp)
		clif_updatestatus(sd,SP_HP);

	if(sd->nshealhp > 0) {
		if(sd->inchealhptick >= battle_config.natural_heal_skill_interval && sd->status.hp < sd->status.max_hp) {
			if(sd->doridori_counter && pc_checkskill(sd,TK_HPTIME)) //TK_HPTIME doridori provided bonus [Dralnu]
				bonus = sd->nshealhp+30;
			else
			bonus = sd->nshealhp;
			sd->doridori_counter = 0;
			while(sd->inchealhptick >= battle_config.natural_heal_skill_interval) {
				sd->inchealhptick -= battle_config.natural_heal_skill_interval;
				if(sd->status.hp + bonus <= sd->status.max_hp)
					sd->status.hp += bonus;
				else {
					bonus = sd->status.max_hp - sd->status.hp;
					sd->status.hp = sd->status.max_hp;
					sd->hp_sub = sd->inchealhptick = 0;
				}
				clif_heal(sd->fd,SP_HP,bonus);
			}
		}
	}
	else sd->inchealhptick = 0;

	return 0;
}

static int pc_natural_heal_sp(struct map_session_data *sd)
{
	int bsp;
	int inc_num,bonus;

	nullpo_retr(0, sd);

	if (sd->sc_count && (sd->sc_data[SC_TRICKDEAD].timer != -1 ||	// Modified by RoVeRT
		sd->sc_data[SC_BERSERK].timer != -1 ||
		sd->sc_data[SC_BLEEDING].timer != -1))
		return 0;

	if (sd->no_regen & 2)
		return 0;

	if(pc_checkoversp(sd)) {
		sd->sp_sub = sd->inchealsptick = 0;
		return 0;
	}

	bsp=sd->status.sp;

	inc_num = pc_spheal(sd);
	if(sd->sc_data[SC_EXPLOSIONSPIRITS].timer == -1)
		sd->sp_sub += inc_num;
	if(sd->walktimer == -1)
		sd->inchealsptick += natural_heal_diff_tick;
	else sd->inchealsptick = 0;

	if(sd->sp_sub >= battle_config.natural_healsp_interval){
		bonus = sd->nhealsp;;
		while(sd->sp_sub >= battle_config.natural_healsp_interval){
			sd->sp_sub -= battle_config.natural_healsp_interval;
			if(sd->status.sp + bonus <= sd->status.max_sp)
				sd->status.sp += bonus;
			else {
				sd->status.sp = sd->status.max_sp;
				sd->sp_sub = sd->inchealsptick = 0;
			}
		}
	}

	if(bsp != sd->status.sp)
		clif_updatestatus(sd,SP_SP);

	if(sd->nshealsp > 0) {
		if(sd->inchealsptick >= battle_config.natural_heal_skill_interval && sd->status.sp < sd->status.max_sp) {
			if(sd->doridori_counter && pc_calc_base_job2(sd->status.class_) == JOB_SUPER_NOVICE)
				bonus = sd->nshealsp*2;
			if(sd->doridori_counter && pc_checkskill(sd,TK_SPTIME)) //TK_SPTIME doridori provided bonus [Dralnu]
				bonus = sd->nshealsp+3;
			else
			bonus = sd->nshealsp;
			sd->doridori_counter = 0;
			while(sd->inchealsptick >= battle_config.natural_heal_skill_interval) {
				sd->inchealsptick -= battle_config.natural_heal_skill_interval;
				if(sd->status.sp + bonus <= sd->status.max_sp)
					sd->status.sp += bonus;
				else {
					bonus = sd->status.max_sp - sd->status.sp;
					sd->status.sp = sd->status.max_sp;
					sd->sp_sub = sd->inchealsptick = 0;
				}
				clif_heal(sd->fd,SP_SP,bonus);
			}
		}
	}
	else sd->inchealsptick = 0;

	return 0;
}

static int pc_spirit_heal_hp(struct map_session_data *sd)
{
	int bonus_hp,interval = battle_config.natural_heal_skill_interval;

	nullpo_retr(0, sd);

	if(pc_checkoverhp(sd)) {
		sd->inchealspirithptick = 0;
		return 0;
	}

	sd->inchealspirithptick += natural_heal_diff_tick;

	if(sd->weight*100/sd->max_weight >= battle_config.natural_heal_weight_rate)
		interval += interval;

	if(sd->inchealspirithptick >= interval) {
		bonus_hp = sd->nsshealhp;
		while(sd->inchealspirithptick >= interval) {
			if(pc_issit(sd)) {
				sd->inchealspirithptick -= interval;
				if(sd->status.hp < sd->status.max_hp) {
					if(sd->status.hp + bonus_hp <= sd->status.max_hp)
						sd->status.hp += bonus_hp;
					else {
						bonus_hp = sd->status.max_hp - sd->status.hp;
						sd->status.hp = sd->status.max_hp;
					}
					clif_heal(sd->fd,SP_HP,bonus_hp);
					sd->inchealspirithptick = 0;
				}
			}else{
				sd->inchealspirithptick -= natural_heal_diff_tick;
				break;
			}
		}
	}

	return 0;
}
static int pc_spirit_heal_sp(struct map_session_data *sd)
{
	int bonus_sp,interval = battle_config.natural_heal_skill_interval;

	nullpo_retr(0, sd);

	if(pc_checkoversp(sd)) {
		sd->inchealspiritsptick = 0;
		return 0;
	}

	sd->inchealspiritsptick += natural_heal_diff_tick;

	if(sd->weight*100/sd->max_weight >= battle_config.natural_heal_weight_rate)
		interval += interval;

	if(sd->inchealspiritsptick >= interval) {
		bonus_sp = sd->nsshealsp;
		while(sd->inchealspiritsptick >= interval) {
			if(pc_issit(sd)) {
				sd->inchealspiritsptick -= interval;
				if(sd->status.sp < sd->status.max_sp) {
					if(sd->status.sp + bonus_sp <= sd->status.max_sp)
						sd->status.sp += bonus_sp;
					else {
						bonus_sp = sd->status.max_sp - sd->status.sp;
						sd->status.sp = sd->status.max_sp;
					}
					clif_heal(sd->fd,SP_SP,bonus_sp);
					sd->inchealspiritsptick = 0;
				}
			}else{
				sd->inchealspiritsptick -= natural_heal_diff_tick;
				break;
			}
		}
	}

	return 0;
}

static int pc_bleeding (struct map_session_data *sd)
{
	int hp = 0, sp = 0;
	nullpo_retr(0, sd);

	if (sd->hp_loss_value > 0) {
		sd->hp_loss_tick += natural_heal_diff_tick;
		if (sd->hp_loss_tick >= sd->hp_loss_rate) {
			do {
				hp += sd->hp_loss_value;
				sd->hp_loss_tick -= sd->hp_loss_rate;
			} while (sd->hp_loss_tick >= sd->hp_loss_rate);
			sd->hp_loss_tick = 0;
		}
	}
	
	if (sd->sp_loss_value > 0) {
		sd->sp_loss_tick += natural_heal_diff_tick;
		if (sd->sp_loss_tick >= sd->sp_loss_rate) {
			do {
				sp += sd->sp_loss_value;
				sd->sp_loss_tick -= sd->sp_loss_rate;
			} while (sd->sp_loss_tick >= sd->sp_loss_rate);
			sd->sp_loss_tick = 0;
		}
	}

	if (hp > 0 || sp > 0)
		pc_heal(sd,-hp,-sp);

	return 0;
}

/*==========================================
 * HP/SP ���R�� �e�N���C�A���g
 *------------------------------------------
 */

static int pc_natural_heal_sub(struct map_session_data *sd,va_list ap) {
	int skill;
	int tick;

	nullpo_retr(0, sd);
	tick = va_arg(ap,int);

// -- moonsoul (if conditions below altered to disallow natural healing if under berserk status)
	if ((battle_config.natural_heal_weight_rate > 100 || sd->weight*100/sd->max_weight < battle_config.natural_heal_weight_rate) &&
		!pc_isdead(sd) &&
		!pc_ishiding(sd) &&
	//-- cannot regen for 5 minutes after using Berserk --- [Celest]
		DIFF_TICK (tick, sd->canregen_tick)>=0 &&
		(sd->sc_data && !(sd->sc_data[SC_POISON].timer != -1 && sd->sc_data[SC_SLOWPOISON].timer == -1) &&
		sd->sc_data[SC_BERSERK].timer == -1 ))
	{
		pc_natural_heal_hp(sd);
		if( sd->sc_data && sd->sc_data[SC_EXTREMITYFIST].timer == -1 &&	//���C��?�Ԃł�SP���񕜂��Ȃ�
			sd->sc_data[SC_DANCING].timer == -1 && //�_���X?�Ԃł�SP���񕜂��Ȃ�
			sd->sc_data[SC_BERSERK].timer == -1 )   //�o?�T?�N?�Ԃł�SP���񕜂��Ȃ�
			pc_natural_heal_sp(sd);
		sd->canregen_tick = tick;
	} else {
		sd->hp_sub = sd->inchealhptick = 0;
		sd->sp_sub = sd->inchealsptick = 0;
	}
	if((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0 && !pc_ishiding(sd) &&
		sd->sc_data[SC_POISON].timer == -1 && sd->sc_data[SC_BERSERK].timer == -1){
		pc_spirit_heal_hp(sd);
		pc_spirit_heal_sp(sd);
	}
	else {
		sd->inchealspirithptick = 0;
		sd->inchealspiritsptick = 0;
	}
	if (sd->hp_loss_value > 0 || sd->sp_loss_value > 0)
		pc_bleeding(sd);
	else
		sd->hp_loss_tick = sd->sp_loss_tick = 0;

	return 0;
}

/*==========================================
 * HP/SP���R�� (interval timer??)
 *------------------------------------------
 */
int pc_natural_heal(int tid,unsigned int tick,int id,int data)
{
	natural_heal_tick = tick;
	natural_heal_diff_tick = DIFF_TICK(natural_heal_tick,natural_heal_prev_tick);
	clif_foreachclient(pc_natural_heal_sub, tick);

	natural_heal_prev_tick = tick;
	return 0;
}

/*==========================================
 * �Z?�u�|�C���g�̕ۑ�
 *------------------------------------------
 */
int pc_setsavepoint(struct map_session_data *sd,char *mapname,int x,int y)
{
	nullpo_retr(0, sd);

	memcpy(sd->status.save_point.map, mapname, MAP_NAME_LENGTH-1);
	sd->status.save_point.x = x;
	sd->status.save_point.y = y;

	return 0;
}

/*==========================================
 * �����Z?�u �e�N���C�A���g
 *------------------------------------------
 */
static int last_save_fd,save_flag;
static int pc_autosave_sub(struct map_session_data *sd,va_list ap)
{
	nullpo_retr(0, sd);

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd);

	if(save_flag==0 && sd->fd>last_save_fd && !sd->state.waitingdisconnect)
	{
		// pet
		if(sd->status.pet_id > 0 && sd->pd)
			intif_save_petdata(sd->status.account_id,&sd->pet);

		chrif_save(sd);
		save_flag=1;
		last_save_fd = sd->fd;
	}

	return 0;
}

/*==========================================
 * �����Z?�u (timer??)
 *------------------------------------------
 */
int pc_autosave(int tid,unsigned int tick,int id,int data)
{
	int interval;

	save_flag=0;
	clif_foreachclient(pc_autosave_sub);
	if(save_flag==0)
		last_save_fd=0;

	interval = autosave_interval/(clif_countusers()+1);
	if(interval <= 0)
		interval = 1;
	add_timer(gettick()+interval,pc_autosave,0,0);

	return 0;
}

int pc_read_gm_account(int fd)
{
#ifdef TXT_ONLY
        int i = 0;
#endif
	if (gm_account != NULL)
		aFree(gm_account);
	GM_num = 0;
#ifdef TXT_ONLY
	gm_account = (struct gm_account *) aCallocA(sizeof(struct gm_account) * ((RFIFOW(fd,2) - 4) / 5), 1);
	for (i = 4; i < RFIFOW(fd,2); i = i + 5) {
		gm_account[GM_num].account_id = RFIFOL(fd,i);
		gm_account[GM_num].level = (int)RFIFOB(fd,i+4);
		//printf("GM account: %d -> level %d\n", gm_account[GM_num].account_id, gm_account[GM_num].level);
		GM_num++;
	}
#else
	sprintf (tmp_sql, "SELECT `%s`,`%s` FROM `%s` WHERE `%s`>='%d'",gm_db_account_id,gm_db_level,gm_db,gm_db_level,lowest_gm_level);
	if(mysql_query(&lmysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&lmysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		return 0;
	}
	lsql_res = mysql_store_result(&lmysql_handle);
	if (lsql_res) {
	    gm_account = (struct gm_account *) aCallocA(sizeof(struct gm_account) * mysql_num_rows(lsql_res), 1);
	    while ((lsql_row = mysql_fetch_row(lsql_res))) {
	        gm_account[GM_num].account_id = atoi(lsql_row[0]);
		    gm_account[GM_num].level = atoi(lsql_row[1]);
		    ShowNotice("GM account: %d -> level %d\n", gm_account[GM_num].account_id, gm_account[GM_num].level);
		    GM_num++;
	    }
    }

    mysql_free_result(lsql_res);
#endif /* TXT_ONLY */
	return GM_num;
}

/*================================================
 * timer to do the day [Yor]
 * data: 0 = called by timer, 1 = gmcommand/script
 *------------------------------------------------
 */
int map_day_timer(int tid, unsigned int tick, int id, int data)
{
	char tmp_soutput[1024];
	if (data == 0 && battle_config.day_duration <= 0)	// if we want a day
		return 0;
	
	if (night_flag != 0) {
		int i;
		night_flag = 0; // 0=day, 1=night [Yor]
		for (i = 0; i < map_num; i++)
			clif_clearweather(i);

		strcpy(tmp_soutput, (data == 0) ? msg_txt(502) : msg_txt(60)); // The day has arrived!
		intif_GMmessage(tmp_soutput, strlen(tmp_soutput) + 1, 0);
	}

	return 0;
}

/*================================================
 * timer to do the night [Yor]
 * data: 0 = called by timer, 1 = gmcommand/script
 *------------------------------------------------
 */
int map_night_timer(int tid, unsigned int tick, int id, int data)
{
	char tmp_soutput[1024];
	struct map_session_data *pl_sd;

	if (data == 0 && battle_config.night_duration <= 0)	// if we want a night
		return 0;
	
	if (night_flag == 0) {
		int i;
		night_flag = 1; // 0=day, 1=night [Yor]
		for(i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = (struct map_session_data *) session[i]->session_data) && pl_sd->state.auth  && !map[pl_sd->bl.m].flag.indoors)
				clif_weather1(i, 474 + battle_config.night_darkness_level);
		}
		strcpy(tmp_soutput, (data == 0) ? msg_txt(503) : msg_txt(59)); // The night has fallen...
		intif_GMmessage(tmp_soutput, strlen(tmp_soutput) + 1, 0);
	}

	return 0;
}

void pc_setstand(struct map_session_data *sd){
	nullpo_retv(sd);

	if(sd->sc_count && sd->sc_data[SC_TENSIONRELAX].timer!=-1)
		status_change_end(&sd->bl,SC_TENSIONRELAX,-1);

	sd->state.dead_sit = 0;
}

//
// ��������
//
/*==========================================
 * �ݒ�t�@�C��?��?��
 * exp.txt �K�v??�l
 * job_db1.txt �d��,hp,sp,�U?���x
 * job_db2.txt job�\�͒l�{?�i�X
 * skill_tree.txt �e�E?�̃X�L���c��?
 * attr_fix.txt ?���C���e?�u��
 * size_fix.txt �T�C�Y�␳�e?�u��
 * refine_db.txt ��?�f?�^�e?�u��
 *------------------------------------------
 */
int pc_readdb(void)
{
	int i,j,k;
	FILE *fp;
	char line[1024],*p;

	// �K�v??�l?��?��
	memset(exp_table,0,sizeof(exp_table));
	fp=fopen("db/exp.txt","r");
	if(fp==NULL){
		ShowError("can't read db/exp.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line)-1, fp)){
		int bn,b1,b2,b3,b4,b5,b6,jn,j1,j2,j3,j4,j5,j6;
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(sscanf(line,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",&bn,&b1,&b2,&b3,&b4,&b5,&b6,&jn,&j1,&j2,&j3,&j4,&j5,&j6)!=14)
			continue;
		exp_table[0][i]=bn;
		exp_table[1][i]=b1;
		exp_table[2][i]=b2;
		exp_table[3][i]=b3;
		exp_table[4][i]=b4;
		exp_table[5][i]=b5;
		exp_table[6][i]=b6;
		exp_table[7][i]=jn;
		exp_table[8][i]=j1;
		exp_table[9][i]=j2;
		exp_table[10][i]=j3;
		exp_table[11][i]=j4;
		exp_table[12][i]=j5;
		exp_table[13][i]=j6;
		i++;
//We can't assume max_base_level is the max between all columns. [Skotlex]
//		if(i >= battle_config.max_base_level)
//			break;
	}
	if (i > battle_config.max_base_level)
	{	//Empty Base level columns
		for (j = battle_config.max_base_level; j < i && exp_table[0][j]>0; j++)
		{
			exp_table[0][j]=0;
			exp_table[1][j]=0;
			exp_table[2][j]=0;
			exp_table[3][j]=0;
			exp_table[4][j]=0;
			exp_table[5][j]=0;
			exp_table[6][j]=0;
		}
	}
	if (i > battle_config.max_sn_level)
	{	//Empty SN job exp columns
		for (j = battle_config.max_sn_level; j < i && exp_table[10][j]>0; j++)
			exp_table[10][j]=0;
	}
	if (i > battle_config.max_adv_level)
	{	//Empty Adv Jobs columns
		for (j = battle_config.max_adv_level; j < i && exp_table[13][j]>0; j++)
			exp_table[13][j]=0;
	}
	if (i > battle_config.max_job_level)
	{	//Empty normal Job columns
		for (j = battle_config.max_job_level; j < i &&
			(exp_table[8][j]>0 || exp_table[9][j]>0 || exp_table[12][j]>0); j++)
		{
			exp_table[8][j]=0; //1st Job
			exp_table[9][j]=0; //2nd Job
			exp_table[12][j]=0; //Adv 1st Job
		}
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","db/exp.txt");

	// �X�L���c��?
	memset(skill_tree,0,sizeof(skill_tree));
	fp=fopen("db/skill_tree.txt","r");
	if(fp==NULL){
		ShowError("can't read db/skill_tree.txt\n");
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
		if (atoi(split[0]) >= MAX_PC_CLASS)
			continue;
		k = atoi(split[1]); //This is to avoid adding two lines for the same skill. [Skotlex]
		for(j = 0; j < MAX_SKILL_TREE && skill_tree[atoi(split[0])][j].id && skill_tree[atoi(split[0])][j].id != k; j++);
		if (j == MAX_SKILL_TREE)
		{
			ShowWarning("Unable to load skill %d into job %d's tree. Maximum number of skills per class has been reached.\n", k, atoi(split[0]));
			continue;
		}
		skill_tree[atoi(split[0])][j].id=k;
		skill_tree[atoi(split[0])][j].max=atoi(split[2]);
		if (f) skill_tree[atoi(split[0])][j].joblv=atoi(split[3]);

		for(k=0;k<5;k++){
			skill_tree[atoi(split[0])][j].need[k].id=atoi(split[k*2+m]);
			skill_tree[atoi(split[0])][j].need[k].lv=atoi(split[k*2+m+1]);
		}
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","db/skill_tree.txt");

	// ?���C���e?�u��
	for(i=0;i<4;i++)
		for(j=0;j<10;j++)
			for(k=0;k<10;k++)
				attr_fix_table[i][j][k]=100;
	fp=fopen("db/attr_fix.txt","r");
	if(fp==NULL){
		ShowError("can't read db/attr_fix.txt\n");
		return 1;
	}
	while(fgets(line, sizeof(line)-1, fp)){
		char *split[10];
		int lv,n;
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<3 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		lv=atoi(split[0]);
		n=atoi(split[1]);

		for(i=0;i<n;){
			if( !fgets(line, sizeof(line)-1, fp) )
				break;
			if(line[0]=='/' && line[1]=='/')
				continue;

			for(j=0,p=line;j<n && p;j++){
				while(*p==32 && *p>0)
					p++;
				attr_fix_table[lv-1][i][j]=atoi(p);
				if(battle_config.attr_recover == 0 && attr_fix_table[lv-1][i][j] < 0)
					attr_fix_table[lv-1][i][j] = 0;
				p=strchr(p,',');
				if(p) *p++=0;
			}

			i++;
		}
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","db/attr_fix.txt");

	// �X�L���c��?
	memset(statp,0,sizeof(statp));
	i=1;
	j=45;	// base points
	fp=fopen("db/statpoint.txt","r");
	if(fp == NULL){
		ShowStatus("Can't read '"CL_WHITE"%s"CL_RESET"'... Generating DB.\n","db/statpoint.txt");
		//return 1;
	} else {
		while(fgets(line, sizeof(line)-1, fp)){
			if(line[0]=='/' && line[1]=='/')
				continue;
			if ((j=atoi(line))<0)
				j=0;
			if (i >= MAX_LEVEL)
				break;
			statp[i]=j;			
			i++;
		}
		fclose(fp);
		ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","db/statpoint.txt");
	}
	// generate the remaining parts of the db if necessary
	for (; i < MAX_LEVEL; i++) {
		j += (i+15)/5;
		statp[i] = j;		
	}

	return 0;
}

/*==========================================
 * pc? �W������
 *------------------------------------------
 */
void do_final_pc(void) {
	if (gm_account)
		aFree(gm_account);
	return;
}
int do_init_pc(void) {
	pc_readdb();

	add_timer_func_list(pc_walk, "pc_walk");
	add_timer_func_list(pc_attack_timer, "pc_attack_timer");
	add_timer_func_list(pc_natural_heal, "pc_natural_heal");
	add_timer_func_list(pc_invincible_timer, "pc_invincible_timer");
	add_timer_func_list(pc_eventtimer, "pc_eventtimer");
	add_timer_func_list(pc_calc_pvprank_timer, "pc_calc_pvprank_timer");
	add_timer_func_list(pc_autosave, "pc_autosave");
	add_timer_func_list(pc_spiritball_timer, "pc_spiritball_timer");
	add_timer_func_list(pc_blockskill_end, "pc_blockskill_end");
	add_timer_func_list(pc_follow_timer, "pc_follow_timer");	
	add_timer_interval((natural_heal_prev_tick = gettick() + NATURAL_HEAL_INTERVAL), pc_natural_heal, 0, 0, NATURAL_HEAL_INTERVAL);
	add_timer(gettick() + autosave_interval, pc_autosave, 0, 0);
#ifndef TXT_ONLY
	pc_read_gm_account(0);
#endif /* not TXT_ONLY */

	if (battle_config.day_duration > 0 && battle_config.night_duration > 0) {
		int day_duration = battle_config.day_duration;
		int night_duration = battle_config.night_duration;
		// add night/day timer (by [yor])
		add_timer_func_list(map_day_timer, "map_day_timer"); // by [yor]
		add_timer_func_list(map_night_timer, "map_night_timer"); // by [yor]

		if (!battle_config.night_at_start) {
			night_flag = 0; // 0=day, 1=night [Yor]
			day_timer_tid = add_timer_interval(gettick() + day_duration + night_duration, map_day_timer, 0, 0, day_duration + night_duration);
			night_timer_tid = add_timer_interval(gettick() + day_duration, map_night_timer, 0, 0, day_duration + night_duration);
		} else {
			night_flag = 1; // 0=day, 1=night [Yor]
			day_timer_tid = add_timer_interval(gettick() + night_duration, map_day_timer, 0, 0, day_duration + night_duration);
			night_timer_tid = add_timer_interval(gettick() + day_duration + night_duration, map_night_timer, 0, 0, day_duration + night_duration);
		}
	}

	return 0;
}
