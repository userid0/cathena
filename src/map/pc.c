// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>

#include "../common/socket.h" // [Valaris]
#include "../common/timer.h"
#include "../common/nullpo.h"
#include "../common/showmsg.h"
#include "../common/malloc.h"
#include "../common/core.h"

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
#include "atcommand.h"
#include "log.h"

#ifndef TXT_ONLY // mail system [Valaris]
#include "mail.h"
#endif

#define PVP_CALCRANK_INTERVAL 1000	// PVP順位計算の間隔
static unsigned int exp_table[MAX_PC_CLASS][2][MAX_LEVEL];
static unsigned int max_level[MAX_PC_CLASS][2];
static short statp[MAX_LEVEL];

// h-files are for declarations, not for implementations... [Shinomori]
struct skill_tree_entry skill_tree[MAX_PC_CLASS][MAX_SKILL_TREE];
// timer for night.day implementation
int day_timer_tid;
int night_timer_tid;

struct fame_list smith_fame_list[MAX_FAME_LIST];
struct fame_list chemist_fame_list[MAX_FAME_LIST];
struct fame_list taekwon_fame_list[MAX_FAME_LIST];

static unsigned int equip_pos[11]={0x0080,0x0008,0x0040,0x0004,0x0001,0x0200,0x0100,0x0010,0x0020,0x0002,0x8000};

static struct gm_account *gm_account = NULL;
static int GM_num = 0;

#define MOTD_LINE_SIZE 128
char motd_text[MOTD_LINE_SIZE][256]; // Message of the day buffer [Valaris]

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
		skill_unit_move(&sd->bl,gettick(),1);
	}
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
	switch(sd->class_&MAPID_UPPERMASK){
		case MAPID_BLACKSMITH: // Blacksmith
            clif_fame_blacksmith(sd,count);
            break;
		case MAPID_ALCHEMIST: // Alchemist
            clif_fame_alchemist(sd,count);
            break;
		case MAPID_TAEKWON: // Taekwon
            clif_fame_taekwon(sd,count);
            break;	
	}
	//FIXME: Is this needed? It places unnecessary stress on the char server.... >.< [Skotlex]
	chrif_save(sd,0); // Save to allow up-to-date fame list refresh
	chrif_reqfamelist(); // Refresh the fame list
}

// Check whether a player ID is in the Top 10 fame list of its job
int pc_istop10fame(int char_id,int job) {
    int i;
	switch(job){
	case MAPID_BLACKSMITH: // Blacksmith
	    for(i=0;i<MAX_FAME_LIST;i++){
			if(smith_fame_list[i].id==char_id)
			    return smith_fame_list[i].fame;
		}
		break;
	case MAPID_ALCHEMIST: // Alchemist
	    for(i=0;i<MAX_FAME_LIST;i++){
	        if(chemist_fame_list[i].id==char_id)
	            return chemist_fame_list[i].fame;
		}
		break;
	case MAPID_TAEKWON: // Taekwon
	    for(i=0;i<MAX_FAME_LIST;i++){
	        if(taekwon_fame_list[i].id==char_id)
	            return taekwon_fame_list[i].fame;
		}
		break;
	}

	return 0;
}

int pc_setrestartvalue(struct map_session_data *sd,int type) {
	//?生や養子の場合の元の職業を算出する

	nullpo_retr(0, sd);

	//-----------------------
	// 死亡した
	if(sd->special_state.restart_full_recover ||	// オシリスカ?ド
		sd->state.snovice_flag == 4) {				// [Celest]
		sd->status.hp=sd->status.max_hp;
		sd->status.sp=sd->status.max_sp;
		if (sd->state.snovice_flag == 4) {
			sd->state.snovice_flag = 0;
			sc_start(&sd->bl,SkillStatusChangeTable[MO_STEELBODY],100,1,skill_get_time(MO_STEELBODY,1));
		}
	}
	else {
		if((sd->class_&MAPID_BASEMASK) == MAPID_NOVICE && !(sd->class_&JOBL_2) && battle_config.restart_hp_rate < 50) { //ノビは半分回復
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

	if(type&2 && (sd->class_&MAPID_UPPERMASK) != MAPID_NOVICE && battle_config.zeny_penalty > 0 && !map[sd->bl.m].flag.nozenypenalty) {
		int zeny = (int)((double)sd->status.zeny * (double)battle_config.zeny_penalty / 10000.);
		if(zeny < 1) zeny = 1;
		sd->status.zeny -= zeny;
		if(sd->status.zeny < 0) sd->status.zeny = 0;
		clif_updatestatus(sd,SP_ZENY);
	}

	return 0;
}

/*==========================================
	Determines if the GM can give / drop / trade / vend items [Lupus]
    Args: GM Level (current player GM level)
 *  Returns
		1 = this GM can't do it
		0 = this one can do it
 *------------------------------------------
 */
int pc_can_give_items(int level) {
	return (	level >= battle_config.gm_cant_drop_min_lv
			&&	level <= battle_config.gm_cant_drop_max_lv);
}

/*==========================================
 * saveに必要なステ?タス修正を行なう
 *------------------------------------------
 */
int pc_makesavestatus(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if (sd->state.finalsave)
		return 0; //Nothing to change.
	
	// 秒ﾌ色は色?弊害が多いので保存?象にはしない
	if(!battle_config.save_clothcolor)
		sd->status.clothes_color=0;

	// 死亡?態だったのでhpを1、位置をセ?ブ場所に?更
	if(!sd->state.waitingdisconnect) {
		sd->status.option = sd->sc.option; //Since the option saved is in 
		if(pc_isdead(sd)){
			pc_setrestartvalue(sd,0);
			memcpy(&sd->status.last_point,&sd->status.save_point,sizeof(sd->status.last_point));
		} else {
			sd->status.last_point.map = sd->mapindex;
			sd->status.last_point.x = sd->bl.x;
			sd->status.last_point.y = sd->bl.y;
		}

		// セ?ブ禁止マップだったので指定位置に移動
		if(map[sd->bl.m].flag.nosave){
			struct map_data *m=&map[sd->bl.m];
			if(m->save.map)
				memcpy(&sd->status.last_point,&m->save,sizeof(sd->status.last_point));
			else
				memcpy(&sd->status.last_point,&sd->status.save_point,sizeof(sd->status.last_point));
		}
	}

	return 0;
}

/*==========================================
 * 接?暫ﾌ初期化
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
	sd->canlog_tick  = gettick();
	sd->state.waitingdisconnect = 0;

	return 0;
}

int pc_equippoint(struct map_session_data *sd,int n)
{
	int ep = 0;
	//?生や養子の場合の元の職業を算出する

	nullpo_retr(0, sd);

	if(sd->inventory_data[n]) {
		ep = sd->inventory_data[n]->equip;
		if(sd->inventory_data[n]->look == 1 || sd->inventory_data[n]->look == 2 || sd->inventory_data[n]->look == 6) {
			if(ep == 2 && (pc_checkskill(sd,AS_LEFT) > 0 || (sd->class_&MAPID_UPPERMASK) == MAPID_ASSASSIN))
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
		sd->inventory_data[i] = id?itemdb_search(id):NULL;
	}
	return 0;
}

int pc_calcweapontype(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->weapontype1 != W_FIST && sd->weapontype2 == W_FIST)
		sd->status.weapon = sd->weapontype1;
	else if(sd->weapontype1 == W_FIST && sd->weapontype2 != W_FIST)// 左手武器 Only
		sd->status.weapon = sd->weapontype2;
	else if(sd->weapontype1 == W_DAGGER && sd->weapontype2 == W_DAGGER)// ?短?
		sd->status.weapon = MAX_WEAPON_TYPE+1;
	else if(sd->weapontype1 == W_1HSWORD && sd->weapontype2 == W_1HSWORD)// ??手?
		sd->status.weapon = MAX_WEAPON_TYPE+2;
	else if(sd->weapontype1 == W_1HAXE && sd->weapontype2 == W_1HAXE)// ??手斧
		sd->status.weapon = MAX_WEAPON_TYPE+3;
	else if( (sd->weapontype1 == W_DAGGER && sd->weapontype2 == W_1HSWORD) ||
		(sd->weapontype1 == W_1HSWORD && sd->weapontype2 == W_DAGGER) ) // 短? - ?手?
		sd->status.weapon = MAX_WEAPON_TYPE+4;
	else if( (sd->weapontype1 == W_DAGGER && sd->weapontype2 == W_1HAXE) ||
		(sd->weapontype1 == W_1HAXE && sd->weapontype2 == W_DAGGER) ) // 短? - 斧
		sd->status.weapon = MAX_WEAPON_TYPE+5;
	else if( (sd->weapontype1 == W_1HSWORD && sd->weapontype2 == W_1HAXE) ||
		(sd->weapontype1 == W_1HAXE && sd->weapontype2 == W_1HSWORD) ) // ?手? - 斧
		sd->status.weapon = MAX_WEAPON_TYPE+6;
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

static int pc_isAllowedCardOn(struct map_session_data *sd,int s,int eqindex,int flag)  {
	int i;
	struct item *item = &sd->status.inventory[eqindex];
	struct item_data *data;
	if (	//Crafted/made/hatched items.
		item->card[0]==0x00ff ||
		item->card[0]==0x00fe ||
		item->card[0]==(short)0xff00
	)
		return 1;
	
	for (i=0;i<s;i++)	{
		if (item->card[i] &&
			(data = itemdb_exists(item->card[i])) &&
			data->flag.no_equip&flag
		)
			return 0;
	}
	return 1;              
}

int pc_isequip(struct map_session_data *sd,int n)
{
	struct item_data *item;
	//?生や養子の場合の元の職業を算出する

	nullpo_retr(0, sd);

	item = sd->inventory_data[n];

	if( battle_config.gm_allequip>0 && pc_isGM(sd)>=battle_config.gm_allequip )
		return 1;

	if(item == NULL)
		return 0;
	if(item->elv && sd->status.base_level < (unsigned int)item->elv)
		return 0;
	if(item->sex != 2 && sd->status.sex != item->sex)
		return 0;
	if(map[sd->bl.m].flag.pvp && item->flag.no_equip&1)
		return 0;
	if(map_flag_gvg(sd->bl.m) && item->flag.no_equip&2)
		return 0; 
	if(map[sd->bl.m].flag.restricted && item->flag.no_equip&map[sd->bl.m].zone)
		return 0;
	if (sd->sc.count) {
			
		if((item->equip & 0x0002 || item->equip & 0x0020) && item->type == 4 && sd->sc.data[SC_STRIPWEAPON].timer != -1) // Also works with left-hand weapons [DracoRPG]
			return 0;
		if(item->equip & 0x0020 && item->type == 5 && sd->sc.data[SC_STRIPSHIELD].timer != -1) // Also works with left-hand weapons [DracoRPG]
			return 0;
		if(item->equip & 0x0010 && sd->sc.data[SC_STRIPARMOR].timer != -1)
			return 0;
		if(item->equip & 0x0100 && sd->sc.data[SC_STRIPHELM].timer != -1)
			return 0;

		if (sd->sc.data[SC_SPIRIT].timer != -1 && sd->sc.data[SC_SPIRIT].val2 == SL_SUPERNOVICE) {
			//Spirit of Super Novice equip bonuses. [Skotlex]
			if (sd->status.base_level > 90 && item->equip & 0x301)
				return 1; //Can equip all helms

			if (sd->status.base_level > 96 && item->equip & 0x022 && item->type == 4)
				switch(item->look) { //In weapons, the look determines type of weapon.
					case W_DAGGER: //Level 4 Knives are equippable.. this means all knives, I'd guess?
					case W_1HSWORD: //All 1H swords
					case W_1HAXE: //All 1H Axes
					case W_MACE: //All Maces
					case W_STAFF: //All Staffs
						return 1;
				}
		}
	}
	//Not equipable by class. [Skotlex]
	if (!(1<<(sd->class_&MAPID_BASEMASK)&item->class_base[(sd->class_&JOBL_2_1)?1:((sd->class_&JOBL_2_2)?2:0)]))
		return 0;
	
	//Not equipable by upper class. [Skotlex]
	if(!(1<<((sd->class_&JOBL_UPPER)?1:((sd->class_&JOBL_BABY)?2:0))&item->class_upper))
		return 0;

	return 1;
}

/*==========================================
 * session idに問題無し
 * char鯖から送られてきたステ?タスを設定
 *------------------------------------------
 */
int pc_authok(struct map_session_data *sd, int login_id2, time_t connect_until_time, struct mmo_charstatus *st)
{
	struct party *p;
	struct guild *g;
	int i;
	unsigned long tick = gettick();

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

	//Set the map-server used job id. [Skotlex]
	i = pc_jobid2mapid(sd->status.class_);
	if (i == -1) { //Invalid class?
		if (battle_config.error_log)
			ShowError("pc_authok: Invalid class %d for player %s (%d:%d). Class was changed to novice.\n", sd->status.class_, sd->status.name, sd->status.account_id, sd->status.char_id);
		sd->status.class_ = JOB_NOVICE;
		sd->class_ = MAPID_NOVICE;
	} else
		sd->class_ = i; 
	//Initializations to null/0 unneeded since map_session_data was filled with 0 upon allocation.
	// 基本的な初期化
	sd->state.connect_new = 1;

	sd->speed = DEFAULT_WALK_SPEED;
	sd->followtimer = -1; // [MouseJstr]
	sd->skillitem = -1;
	sd->skillitemlv = -1;
	sd->invincible_timer = -1;
	
	sd->canregen_tick = tick;
	sd->canuseitem_tick = tick;
	
	for(i = 0; i < MAX_SKILL_LEVEL; i++)
		sd->spirit_timer[i] = -1;

	if (battle_config.item_auto_get)
		sd->state.autoloot = 10000;

	if (battle_config.disp_experience)
		sd->state.showexp = 1;
	if (battle_config.disp_zeny)
		sd->state.showzeny = 1;
	
	if (battle_config.display_delay_skill_fail)
		sd->state.showdelay = 1;
		
	// Request all registries.
	intif_request_registry(sd,7);

	// アイテムチェック
	pc_setinventorydata(sd);
	pc_checkitem(sd);
	
	//Set here because we need the inventory data for weapon sprite parsing.
	status_set_viewdata(&sd->bl, sd->status.class_);
	status_change_init(&sd->bl);
	unit_dataset(&sd->bl);
	
	// pet
	sd->pet_hungry_timer = -1;

	if ((battle_config.atc_gmonly == 0 || pc_isGM(sd)) &&
	    (pc_isGM(sd) >= get_atcommand_level(AtCommand_Hide)))
		sd->status.option &= (OPTION_MASK | OPTION_INVISIBLE);
	else
		sd->status.option &= OPTION_MASK;

	sd->sc.option = sd->status.option; //This is the actual option used in battle.
	// パ?ティ??係の初期化
	sd->party_x = -1;
	sd->party_y = -1;
	sd->guild_x = -1;
	sd->guild_y = -1;

	// イベント?係の初期化
	for(i = 0; i < MAX_EVENTTIMER; i++)
		sd->eventtimer[i] = -1;

	sd->npc_timer_id = -1;
	
	// Moved PVP timer initialisation before set_pos
	sd->pvp_timer = -1;

	// 位置の設定
	if ((i=pc_setpos(sd,sd->status.last_point.map, sd->status.last_point.x, sd->status.last_point.y, 0)) != 0) {
		if(battle_config.error_log)
			ShowError ("Last_point_map %s - id %d not found (error code %d)\n", mapindex_id2name(sd->status.last_point.map), sd->status.last_point.map, i);

		// try warping to a default map instead (church graveyard)
		if (pc_setpos(sd, mapindex_name2id(MAP_PRONTERA), 273, 354, 0) != 0) {
			// if we fail again
			clif_authfail_fd(sd->fd, 0);
			return 1;
		}
	}

	// pet
	if (sd->status.pet_id > 0)
		intif_request_petdata(sd->status.account_id, sd->status.char_id, sd->status.pet_id);

	// パ?ティ、ギルドデ?タの要求
	if (sd->status.party_id > 0 && (p = party_search(sd->status.party_id)) == NULL)
		party_request_info(sd->status.party_id);
	if (sd->status.guild_id > 0)
	{
		if ((g = guild_search(sd->status.guild_id)) == NULL)
			guild_request_info(sd->status.guild_id);
		else if (strcmp(sd->status.name,g->master) == 0)
		{	//Block Guild Skills to prevent logout/login reuse exploiting. [Skotlex]
			guild_block_skill(sd, 300000);
			//Also set the Guild Master flag.
			sd->state.gmaster_flag = g;
		}
	}

	// 通知

	clif_authok(sd);
	map_addiddb(&sd->bl);
	if (map_charid2nick(sd->status.char_id) == NULL)
		map_addchariddb(sd->status.char_id, sd->status.name);

	// Notify everyone that this char logged in [Skotlex].
	clif_foreachclient(clif_friendslist_toggle_sub, sd->status.account_id, sd->status.char_id, 1);
	
	//Prevent S. Novices from getting the no-death bonus just yet. [Skotlex]
	sd->die_counter=-1;
	//Until the reg values arrive, set them to not require trigger...
	sd->state.event_death = 1;
	sd->state.event_kill_pc = 1;
	sd->state.event_disconnect = 1;
	sd->state.event_kill_mob = 1;

	// ステ?タス初期計算など
	status_calc_pc(sd,1);
			
	sd->state.auth = 1; //Do not auth him until the initial stats have been placed.
	{	//Add IP field
		unsigned char *ip = (unsigned char *) &session[sd->fd]->client_addr.sin_addr;
		if (pc_isGM(sd))
			ShowInfo("GM Character '"CL_WHITE"%s"CL_RESET"' logged in. (Acc. ID: '"CL_WHITE"%d"CL_RESET"', Connection: '"CL_WHITE"%d"CL_RESET"', Packet Ver: '"CL_WHITE"%d"CL_RESET"', IP: '"CL_WHITE"%d.%d.%d.%d"CL_RESET"', GM Level '"CL_WHITE"%d"CL_RESET"').\n", sd->status.name, sd->status.account_id, sd->fd, sd->packet_ver, ip[0],ip[1],ip[2],ip[3], pc_isGM(sd));
		else
			ShowInfo("Character '"CL_WHITE"%s"CL_RESET"' logged in. (Account ID: '"CL_WHITE"%d"CL_RESET"', Connection: '"CL_WHITE"%d"CL_RESET"', Packet Ver: '"CL_WHITE"%d"CL_RESET"', IP: '"CL_WHITE"%d.%d.%d.%d"CL_RESET"').\n", sd->status.name, sd->status.account_id, sd->fd, sd->packet_ver, ip[0],ip[1],ip[2],ip[3]);
	}
	
	// Send friends list
	clif_friendslist_send(sd);

	if (battle_config.display_version == 1){
		char buf[256];
		sprintf(buf, "eAthena SVN version: %s", get_svn_revision());
		clif_displaymessage(sd->fd, buf);
	}

	// Message of the Day [Valaris]
	{
		int ln;
		for(ln=0; motd_text[ln][0] && ln < MOTD_LINE_SIZE; ln++) {
			if (battle_config.motd_type)
				clif_disp_onlyself(sd,motd_text[ln],strlen(motd_text[ln]));
			else
				clif_displaymessage(sd->fd, motd_text[ln]);
		}
	}

#ifndef TXT_ONLY
	if(mail_server_enable)
		mail_check(sd,1); // check mail at login [Valaris]
#endif

	// message of the limited time of the account
	if (connect_until_time != 0) { // don't display if it's unlimited or unknow value
		char tmpstr[1024];
		strftime(tmpstr, sizeof(tmpstr) - 1, msg_txt(501), localtime(&connect_until_time)); // "Your account time limit is: %d-%m-%Y %H:%M:%S."
		clif_wis_message(sd->fd, wisp_server_name, tmpstr, strlen(tmpstr)+1);
	}

	if(sd->status.manner < 0) //Needed or manner will always be negative.
		sc_start(&sd->bl,SC_NOCHAT,100,0,0);
	return 0;
}

/*==========================================
 * Closes a connection because it failed to be authenticated from the char server.
 *------------------------------------------
 */
int pc_authfail(struct map_session_data *sd) {

	if (sd->state.auth) //Temporary debug. [Skotlex]
		ShowDebug("pc_authfail: Received auth fail for already authentified client (account id %d)!\n", sd->bl.id);

	if (!sd->fd)
		ShowDebug("pc_authfail: Received auth fail for a player with no connection (account id %d)!\n", sd->bl.id);

	clif_authfail_fd(sd->fd, 0);
	return 0;
}

/*==========================================
 * Invoked once after the char/account/account2 registry variables are received. [Skotlex]
 *------------------------------------------
 */
int pc_reg_received(struct map_session_data *sd)
{
	int i,j;
	char feel_var[3][NAME_LENGTH] = {"PC_FEEL_SUN","PC_FEEL_MOON","PC_FEEL_STAR"};
	char hate_var[3][NAME_LENGTH] = {"PC_HATE_MOB_SUN","PC_HATE_MOB_MOON","PC_HATE_MOB_STAR"};
	
	pc_clean_skilltree(sd); //Clean skill tree before loading reg-based skills
	sd->change_level = pc_readglobalreg(sd,"jobchange_level");
	sd->die_counter = pc_readglobalreg(sd,"PC_DIE_COUNTER");
	if (!sd->die_counter && (sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE)
		status_calc_pc(sd, 0); //Check +10 to all stats bonus.
	if (pc_checkskill(sd, TK_MISSION)) {
		sd->mission_mobid = pc_readglobalreg(sd,"TK_MISSION_ID");
		sd->mission_count = pc_readglobalreg(sd,"TK_MISSION_COUNT");
	}
	
	//SG map and mob read [Komurka]
	for(i=0;i<3;i++) //for now - someone need to make reading from txt/sql
	{
		if ((j = pc_readglobalreg(sd,feel_var[i]))!=0) {
			sd->feel_map[i].index = j;
			sd->feel_map[i].m = map_mapindex2mapid(j);
		} else {
			sd->feel_map[i].index = 0;
			sd->feel_map[i].m = -1;
		}
		sd->hate_mob[i] = pc_readglobalreg(sd,hate_var[i])  - 1;
		
	}

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
		sd->state.event_kill_pc = pc_readglobalreg(sd, script_config.kill_pc_event_name);
		sd->state.event_kill_mob = pc_readglobalreg(sd, script_config.kill_mob_event_name);
		sd->state.event_disconnect = pc_readglobalreg(sd, script_config.logout_event_name);
	// if script triggers are not required
	} else {
		sd->state.event_death = 1;
		sd->state.event_kill_pc = 1;
		sd->state.event_disconnect = 1;
		sd->state.event_kill_mob = 1;
	}

	if (script_config.event_script_type == 0) {
		struct npc_data *npc;
		if ((npc = npc_name2id(script_config.login_event_name))) {
			run_script(npc->u.scr.script,0,sd->bl.id,npc->bl.id); // PCLoginNPC
			ShowStatus("Event '"CL_WHITE"%s"CL_RESET"' executed.\n", script_config.login_event_name);
		}
	} else {
		ShowStatus("%d '"CL_WHITE"%s"CL_RESET"' events executed.\n",
			npc_event_doall_id(script_config.login_event_name, sd->bl.id), script_config.login_event_name);
	}

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
				!(inf2&(INF2_WEDDING_SKILL|INF2_SPIRIT_SKILL)) //Do not count wedding/link skills. [Skotlex]
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
 * ?えられるスキルの計算
 *------------------------------------------
 */
int pc_calc_skilltree(struct map_session_data *sd)
{
	int i,id=0,flag;
	int c=0;

	nullpo_retr(0, sd);
	i = pc_calc_skilltree_normalize_job(sd);
	c = pc_mapid2jobid(i, sd->status.sex);
	if (c == -1) { //Unable to normalize job??
		if (battle_config.error_log)
			ShowError("pc_calc_skilltree: Unable to normalize job %d for character %s (%d:%d)\n", i, sd->status.name, sd->status.account_id, sd->status.char_id);
		return 1;
	}
	
	for(i=0;i<MAX_SKILL;i++){ 
		if (sd->status.skill[i].flag != 13) //Don't touch plagiarized skills
			sd->status.skill[i].id=0; //First clear skills.
	}
	for(i=0;i<MAX_SKILL;i++){ 
		if (sd->status.skill[i].flag && sd->status.skill[i].flag != 13){ //Restore original level of skills after deleting earned skills.	
			sd->status.skill[i].lv=(sd->status.skill[i].flag==1)?0:sd->status.skill[i].flag-2;
			sd->status.skill[i].flag=0;
		}
		if(sd->sc.count && sd->sc.data[SC_SPIRIT].timer != -1 && sd->sc.data[SC_SPIRIT].val2 == SL_BARDDANCER && i >= DC_HUMMING && i<= DC_SERVICEFORYOU)
		{ //Enable Bard/Dancer spirit linked skills.
			if (sd->status.sex) { //Link dancer skills to bard.
				sd->status.skill[i].id=i;
				sd->status.skill[i].lv=sd->status.skill[i-8].lv; // Set the level to the same as the linking skill
				sd->status.skill[i].flag=1; // Tag it as a non-savable, non-uppable, bonus skill
			} else { //Link bard skills to dancer.
				sd->status.skill[i-8].id=i-8;
				sd->status.skill[i-8].lv=sd->status.skill[i].lv; // Set the level to the same as the linking skill
				sd->status.skill[i-8].flag=1; // Tag it as a non-savable, non-uppable, bonus skill
			}
		}
	}

	if (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill){
		for(i=0;i<MAX_SKILL;i++){
			if (skill_get_inf2(i)&(INF2_NPC_SKILL|INF2_GUILD_SKILL)) //Only skills you can't have are npc/guild ones
				continue;
			if (skill_get_max(i) > 0)
				sd->status.skill[i].id=i;
		}
		return 0;
	}
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
				else if (pc_checkskill(sd, NV_BASIC) < 9 && id != NV_BASIC && !(skill_get_inf2(id)&INF2_QUEST_SKILL))
					f=0; // Do not unlock normal skills when Basic Skills is not maxed out (can happen because of skill reset)
			}
			if(sd->status.skill[id].id==0 ){
				if(skill_get_inf2(id)&INF2_SPIRIT_SKILL)
				{	//Spirit skills cannot be learned, they will only show up on your tree when you get buffed.
					if (sd->sc.count && sd->sc.data[SC_SPIRIT].timer != -1)
					{	//Enable Spirit Skills. [Skotlex]
						sd->status.skill[id].id=id;
						sd->status.skill[id].lv=1;
						sd->status.skill[id].flag=1; //So it is not saved, and tagged as a "bonus" skill.
					}
					flag=1;
				} else if (f){
					sd->status.skill[id].id=id;
					flag=1;
				}
			}
		}
	} while(flag);
	if ((sd->class_&MAPID_UPPERMASK) == MAPID_TAEKWON && sd->status.base_level >= 90 && pc_istop10fame(sd->char_id, MAPID_TAEKWON)) {
		//Grant all Taekwon Tree, but only as bonus skills in case they drop from ranking. [Skotlex]
		for(i=0;i < MAX_SKILL_TREE && (id=skill_tree[c][i].id)>0;i++){
			if ((skill_get_inf2(id)&(INF2_QUEST_SKILL|INF2_WEDDING_SKILL)))
				continue; //Do not include Quest/Wedding skills.
			if(sd->status.skill[id].id==0 ){
				sd->status.skill[id].id=id;
				sd->status.skill[id].flag=1; //So it is not saved, and tagged as a "bonus" skill.
			} else
				sd->status.skill[id].flag=sd->status.skill[id].lv+2; 
			sd->status.skill[id].lv= skill_tree_get_max(id, sd->status.class_);
		}
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
	int skill_point;
	int c = sd->class_;
	
	if (!battle_config.skillup_limit || !(sd->class_&JOBL_2) || (sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE)
		return c; //Only Normalize non-first classes (and non-super novice)
	
	skill_point = pc_calc_skillpoint(sd);
	if(skill_point < 9)
		c = MAPID_NOVICE;
	else if (sd->status.skill_point >= (int)sd->status.job_level
		&& ((sd->change_level > 0 && skill_point < sd->change_level+8) || skill_point < 58)) {
		//Send it to first class.
		c &= MAPID_BASEMASK;
	}
	if (sd->class_&JOBL_UPPER) //Convert to Upper
		c |= JOBL_UPPER;
	else if (sd->class_&JOBL_BABY) //Convert to Baby
		c |= JOBL_BABY;

	return c;
}

/*==========================================
 * 重量アイコンの確認
 *------------------------------------------
 */
int pc_checkweighticon(struct map_session_data *sd)
{
	int flag=0;

	nullpo_retr(0, sd);

	//Consider the battle option 50% criteria....
	if(sd->weight*100 >= sd->max_weight*battle_config.natural_heal_weight_rate)
		flag=1;
	if(sd->weight*10 >= sd->max_weight*9)
		flag=2;

	if(flag==1){
		if(sd->sc.data[SC_WEIGHT50].timer==-1)
			sc_start(&sd->bl,SC_WEIGHT50,100,0,0);
	}else{
		if(sd->sc.data[SC_WEIGHT50].timer!=-1)
			status_change_end(&sd->bl,SC_WEIGHT50,-1);
	} 
	if(flag==2){
		if(sd->sc.data[SC_WEIGHT90].timer==-1)
			sc_start(&sd->bl,SC_WEIGHT90,100,0,0);
	}else{
		if(sd->sc.data[SC_WEIGHT90].timer!=-1)
			status_change_end(&sd->bl,SC_WEIGHT90,-1);
	}
	return 0;
}

int pc_disguise(struct map_session_data *sd, int class_) {
	if (!class_ && !sd->disguise)
		return 0;
	if (class_ && (sd->disguise || pc_isriding(sd)))
		return 0;

	pc_stop_walking(sd, 0);
	clif_clearchar(&sd->bl, 0);

	if (!class_) {
		sd->disguise = 0;
		class_ = sd->status.class_;
	} else
		sd->disguise=class_;

	status_set_viewdata(&sd->bl, class_);
	clif_changeoption(&sd->bl);
	clif_spawn(&sd->bl);
	return 1;
}

static int pc_bonus_autospell(struct s_autospell *spell, int max, short id, short lv, short rate, short card_id) {
	int i;
	for (i = 0; i < max && spell[i].id; i++) {
		if (spell[i].card_id == card_id &&
			spell[i].id == id && spell[i].lv == lv)
		{
			if (!battle_config.autospell_stacking)
				return 0;
			rate += spell[i].rate;
			break;
		}
	}
	if (i == max) {
		if (battle_config.error_log)
			ShowWarning("pc_bonus: Reached max (%d) number of autospells per character!\n", max);
		return 0;
	}
	spell[i].id = id;
	spell[i].lv = lv;
	spell[i].rate = rate;
	spell[i].card_id = card_id;
	return 1;
}

static int pc_bonus_item_drop(struct s_add_drop *drop, short *count, short id, short group, int race, int rate) {
	int i;
	//Apply config rate adjustment settings.
	if (rate >= 0) { //Absolute drop.
		if (battle_config.item_rate_adddrop != 100)
			rate = rate*battle_config.item_rate_adddrop/100;
		if (rate < battle_config.item_drop_adddrop_min)
			rate = battle_config.item_drop_adddrop_min;
		else if (rate > battle_config.item_drop_adddrop_max)
			rate = battle_config.item_drop_adddrop_max;
	} else { //Relative drop, max/min limits are applied at drop time.
		if (battle_config.item_rate_adddrop != 100)
			rate = rate*battle_config.item_rate_adddrop/100;
		if (rate > -1)
			rate = -1;
	}
	for(i = 0; i < *count; i++) {
		if(
			(id && drop[i].id == id) ||
			(group && drop[i].group == group)
		) {
			drop[i].race |= race;
			if(drop[i].rate > 0 && rate > 0)
			{	//Both are absolute rates.
				if (drop[i].rate < rate)
					drop[i].rate = rate;
			} else
			if(drop[i].rate < 0 && rate < 0) {
				//Both are relative rates.
				if (drop[i].rate > rate)
					drop[i].rate = rate;
			} else if (rate < 0) //Give preference to relative rate.
					drop[i].rate = rate;
			return 1;
		}
	}
	if(*count >= MAX_PC_BONUS) {
		if (battle_config.error_log)
			ShowWarning("pc_bonus: Reached max (%d) number of added drops per character!\n", MAX_PC_BONUS);
		return 0;
	}
	drop[*count].id = id;
	drop[*count].group = group;
	drop[*count].race |= race;
	drop[*count].rate = rate;
	(*count)++;
	return 1;
}

/*==========================================
 * ? 備品による能力等のボ?ナス設定
 *------------------------------------------
 */
int pc_bonus(struct map_session_data *sd,int type,int val)
{
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
		if(sd->state.lr_flag != 2) {
			sd->special_state.infinite_endure = 1;
			if (sd->sc.data[SC_ENDURE].timer == -1)
				sc_start(&sd->bl,SC_ENDURE,100,11,600000);
		}
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
			sd->state.perfect_hiding=1;
		}
		break;
	case SP_DISGUISE: // Disguise script for items [Valaris]
		if(sd->state.lr_flag!=2)
			pc_disguise(sd, val);
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
		//if(sd->state.lr_flag != 2 && sd->long_attack_atk_rate < val)
		//	sd->long_attack_atk_rate = val;
		if(sd->state.lr_flag != 2)	//[Lupus] it should stack, too. As any other cards rate bonuses
			sd->long_attack_atk_rate+=val;
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
			sd->right_weapon.hp_drain[RC_NONBOSS].value += val;
			sd->right_weapon.hp_drain[RC_BOSS].value += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->right_weapon.hp_drain[RC_NONBOSS].value += val;
			sd->right_weapon.hp_drain[RC_BOSS].value += val;
		}
		break;
	case SP_SP_DRAIN_VALUE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.sp_drain[RC_NONBOSS].value += val;
			sd->right_weapon.sp_drain[RC_BOSS].value += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.sp_drain[RC_NONBOSS].value += val;
			sd->left_weapon.sp_drain[RC_BOSS].value += val;
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
	default:
		if(battle_config.error_log)
			ShowWarning("pc_bonus: unknown type %d %d !\n",type,val);
		break;
	}
	return 0;
}

/*==========================================
 * ? 備品による能力等のボ?ナス設定
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
		if (type2 < SC_COMMON_MIN || type2 > SC_COMMON_MAX) {
			ShowWarning("pc_bonus2 (Add Effect): %d is not supported.\n", type2);
			break;
		}
		if(sd->state.lr_flag != 2)
			sd->addeff[type2-SC_COMMON_MIN]+=val;
		else
			sd->arrow_addeff[type2-SC_COMMON_MIN]+=val;
		break;
	case SP_ADDEFF2:
		if (type2 < SC_COMMON_MIN || type2 > SC_COMMON_MAX) {
			ShowWarning("pc_bonus2 (Add Effect2): %d is not supported.\n", type2);
			break;
		}
		if(sd->state.lr_flag != 2)
			sd->addeff2[type2-SC_COMMON_MIN]+=val;
		else
			sd->arrow_addeff2[type2-SC_COMMON_MIN]+=val;
		break;
	case SP_RESEFF:
		if (type2 < SC_COMMON_MIN || type2 > SC_COMMON_MAX) {
			ShowWarning("pc_bonus2 (Resist Effect): %d is not supported.\n", type2);
			break;
		}
		if(sd->state.lr_flag != 2)
			sd->reseff[type2-SC_COMMON_MIN]+=val;
		break;
	case SP_MAGIC_ADDELE:
		if(sd->state.lr_flag != 2)
			sd->magic_addele[type2]+=val;
		break;
	case SP_MAGIC_ADDRACE:
		if(sd->state.lr_flag != 2)
			sd->magic_addrace[type2]+=val;
		break;
	case SP_MAGIC_ADDSIZE:
		if(sd->state.lr_flag != 2)
			sd->magic_addsize[type2]+=val;
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
			for(i=0;i<sd->add_mdmg_count;i++) {
				if(sd->add_mdmg[i].class_ == type2) {
					sd->add_mdmg[i].rate += val;
					break;
				}
			}
			if(i >= sd->add_mdmg_count && sd->add_mdmg_count < MAX_PC_BONUS) {
				sd->add_mdmg[sd->add_mdmg_count].class_ = type2;
				sd->add_mdmg[sd->add_mdmg_count].rate += val;
				sd->add_mdmg_count++;
			}
		}
		break;
	case SP_ADD_DEF_CLASS:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_def_count;i++) {
				if(sd->add_def[i].class_ == type2) {
					sd->add_def[i].rate += val;
					break;
				}
			}
			if(i >= sd->add_def_count && sd->add_def_count < MAX_PC_BONUS) {
				sd->add_def[sd->add_def_count].class_ = type2;
				sd->add_def[sd->add_def_count].rate += val;
				sd->add_def_count++;
			}
		}
		break;
	case SP_ADD_MDEF_CLASS:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_mdef_count;i++) {
				if(sd->add_mdef[i].class_ == type2) {
					sd->add_mdef[i].rate += val;
					break;
				}
			}
			if(i >= sd->add_mdef_count && sd->add_mdef_count < MAX_PC_BONUS) {
				sd->add_mdef[sd->add_mdef_count].class_ = type2;
				sd->add_mdef[sd->add_mdef_count].rate += val;
				sd->add_mdef_count++;
			}
		}
		break;
	case SP_HP_DRAIN_RATE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.hp_drain[RC_NONBOSS].rate += type2;
			sd->right_weapon.hp_drain[RC_NONBOSS].per += val;
			sd->right_weapon.hp_drain[RC_BOSS].rate += type2;
			sd->right_weapon.hp_drain[RC_BOSS].per += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.hp_drain[RC_NONBOSS].rate += type2;
			sd->left_weapon.hp_drain[RC_NONBOSS].per += val;
			sd->left_weapon.hp_drain[RC_BOSS].rate += type2;
			sd->left_weapon.hp_drain[RC_BOSS].per += val;
		}
		break;
	case SP_HP_DRAIN_VALUE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.hp_drain[RC_NONBOSS].value += type2;
			sd->right_weapon.hp_drain[RC_NONBOSS].type = val;
			sd->right_weapon.hp_drain[RC_BOSS].value += type2;
			sd->right_weapon.hp_drain[RC_BOSS].type = val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.hp_drain[RC_NONBOSS].value += type2;
			sd->left_weapon.hp_drain[RC_NONBOSS].type = val;
			sd->left_weapon.hp_drain[RC_BOSS].value += type2;
			sd->left_weapon.hp_drain[RC_BOSS].type = val;
		}
		break;
	case SP_SP_DRAIN_RATE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.sp_drain[RC_NONBOSS].rate += type2;
			sd->right_weapon.sp_drain[RC_NONBOSS].per += val;
			sd->right_weapon.sp_drain[RC_BOSS].rate += type2;
			sd->right_weapon.sp_drain[RC_BOSS].per += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.sp_drain[RC_NONBOSS].rate += type2;
			sd->left_weapon.sp_drain[RC_NONBOSS].per += val;
			sd->left_weapon.sp_drain[RC_BOSS].rate += type2;
			sd->left_weapon.sp_drain[RC_BOSS].per += val;
		}
		break;
	case SP_SP_DRAIN_VALUE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.sp_drain[RC_NONBOSS].value += type2;
			sd->right_weapon.sp_drain[RC_NONBOSS].type = val;
			sd->right_weapon.sp_drain[RC_BOSS].value += type2;
			sd->right_weapon.sp_drain[RC_BOSS].type = val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.sp_drain[RC_NONBOSS].value += type2;
			sd->left_weapon.sp_drain[RC_NONBOSS].type = val;
			sd->left_weapon.sp_drain[RC_BOSS].value += type2;
			sd->left_weapon.sp_drain[RC_BOSS].type = val;
		}
		break;
	case SP_SP_VANISH_RATE:
		if(sd->state.lr_flag != 2) {
			sd->sp_vanish_rate += type2;
			sd->sp_vanish_per += val;
		}
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
		if (type2 < SC_COMMON_MIN || type2 > SC_COMMON_MAX) {
			ShowWarning("pc_bonus2 (Add Effect when hit): %d is not supported.\n", type2);
			break;
		}
		if(sd->state.lr_flag != 2) {
			sd->addeff3[type2-SC_COMMON_MIN]+=val;
			sd->addeff3_type[type2-SC_COMMON_MIN]=1;
		}
		break;
	case SP_ADDEFF_WHENHIT_SHORT:
		if (type2 < SC_COMMON_MIN || type2 > SC_COMMON_MAX) {
			ShowWarning("pc_bonus2 (Add Effect when hit short): %d is not supported.\n", type2);
			break;
		}
		if(sd->state.lr_flag != 2) {
			sd->addeff3[type2-SC_COMMON_MIN]+=val;
			sd->addeff3_type[type2-SC_COMMON_MIN]=0;
		}
		break;
	case SP_SKILL_ATK:
		for (i = 0; i < MAX_PC_BONUS && sd->skillatk[i].id != 0 && sd->skillatk[i].id != type2; i++);
		if (i == MAX_PC_BONUS)
		{	//Better mention this so the array length can be updated. [Skotlex]
			ShowDebug("run_script: bonus2 bSkillAtk reached it's limit (%d skills per character), bonus skill %d (+%d%%) lost.\n", MAX_PC_BONUS, type2, val);
			break;
		}
		if(sd->state.lr_flag != 2) {
			if (sd->skillatk[i].id == type2)
				sd->skillatk[i].val += val;
			else {
				sd->skillatk[i].id = type2;
				sd->skillatk[i].val = val;
			}
		}
		break;
	case SP_ADD_SKILL_BLOW:
		for (i = 0; i < MAX_PC_BONUS && sd->skillblown[i].id != 0 && sd->skillblown[i].id != type2; i++);
		if (i == MAX_PC_BONUS)
		{	//Better mention this so the array length can be updated. [Skotlex]
			ShowDebug("run_script: bonus2 bSkillBlown reached it's limit (%d skills per character), bonus skill %d (+%d%%) lost.\n", MAX_PC_BONUS, type2, val);
			break;
		}
		if(sd->state.lr_flag != 2) {
			if (sd->skillblown[i].id == type2)
				sd->skillblown[i].val += val;
			else {
				sd->skillblown[i].id = type2;
				sd->skillblown[i].val = val;
			}
		}
		break;
	case SP_ADD_DAMAGE_BY_CLASS:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_dmg_count;i++) {
				if(sd->add_dmg[i].class_ == type2) {
					sd->add_dmg[i].rate += val;
					break;
				}
			}
			if(i >= sd->add_dmg_count && sd->add_dmg_count < MAX_PC_BONUS) {
				sd->add_dmg[sd->add_dmg_count].class_ = type2;
				sd->add_dmg[sd->add_dmg_count].rate += val;
				sd->add_dmg_count++;
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
		if (sd->state.lr_flag != 2)
			pc_bonus_item_drop(sd->add_drop, &sd->add_drop_count, type2, 0, (1<<10)|(1<<11), val);
		break;
	case SP_ADD_MONSTER_DROP_ITEMGROUP:
		if (sd->state.lr_flag != 2)
			pc_bonus_item_drop(sd->add_drop, &sd->add_drop_count, 0, type2, (1<<10)|(1<<11), val);
		break;
	case SP_SP_LOSS_RATE:
		if(sd->state.lr_flag != 2) {
			sd->sp_loss_value = type2;
			sd->sp_loss_rate = val;
		}
		break;
	case SP_HP_DRAIN_VALUE_RACE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.hp_drain[type2].value += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.hp_drain[type2].value += val;
		}
		break;
	case SP_SP_DRAIN_VALUE_RACE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.sp_drain[type2].value += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.sp_drain[type2].value += val;
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
	nullpo_retr(0, sd);

	switch(type){
	case SP_ADD_MONSTER_DROP_ITEM:
		if(sd->state.lr_flag != 2)
			pc_bonus_item_drop(sd->add_drop, &sd->add_drop_count, type2, 0, 1<<type3, val);
		break;
	case SP_AUTOSPELL:
		if(sd->state.lr_flag != 2)
			pc_bonus_autospell(sd->autospell, MAX_PC_BONUS, type2, type3, val, current_equip_card_id);
		break;
	case SP_AUTOSPELL_WHENHIT:
		if(sd->state.lr_flag != 2)
			pc_bonus_autospell(sd->autospell2, MAX_PC_BONUS, type2, type3, val, current_equip_card_id);
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
			sd->right_weapon.sp_drain[RC_NONBOSS].rate += type2;
			sd->right_weapon.sp_drain[RC_NONBOSS].per += type3;
			sd->right_weapon.sp_drain[RC_NONBOSS].type = val;
			sd->right_weapon.sp_drain[RC_BOSS].rate += type2;
			sd->right_weapon.sp_drain[RC_BOSS].per += type3;
			sd->right_weapon.sp_drain[RC_BOSS].type = val;

		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.sp_drain[RC_NONBOSS].rate += type2;
			sd->left_weapon.sp_drain[RC_NONBOSS].per += type3;
			sd->left_weapon.sp_drain[RC_NONBOSS].type = val;
			sd->left_weapon.sp_drain[RC_BOSS].rate += type2;
			sd->left_weapon.sp_drain[RC_BOSS].per += type3;
			sd->left_weapon.sp_drain[RC_BOSS].type = val;
		}
		break;
	case SP_HP_DRAIN_RATE_RACE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.hp_drain[type2].rate += type3;
			sd->right_weapon.hp_drain[type2].per += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.hp_drain[type2].rate += type3;
			sd->left_weapon.hp_drain[type2].per += val;
		}
		break;
	case SP_SP_DRAIN_RATE_RACE:
		if(!sd->state.lr_flag) {
			sd->right_weapon.sp_drain[type2].rate += type3;
			sd->right_weapon.sp_drain[type2].per += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->left_weapon.sp_drain[type2].rate += type3;
			sd->left_weapon.sp_drain[type2].per += val;
		}
		break;
	case SP_ADD_MONSTER_DROP_ITEMGROUP:
		if (sd->state.lr_flag != 2)
			pc_bonus_item_drop(sd->add_drop, &sd->add_drop_count, 0, type2, 1<<type3, val);
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
	nullpo_retr(0, sd);

	switch(type){
	case SP_AUTOSPELL:
		if(sd->state.lr_flag != 2)
			pc_bonus_autospell(sd->autospell, MAX_PC_BONUS, (val?type2:-type2), type3, type4, current_equip_card_id);
		break;

	case SP_AUTOSPELL_WHENHIT:
		if(sd->state.lr_flag != 2)
			pc_bonus_autospell(sd->autospell2, MAX_PC_BONUS, (val?type2:-type2), type3, type4, current_equip_card_id);
		break;
	default:
		if(battle_config.error_log)
			ShowWarning("pc_bonus4: unknown type %d %d %d %d %d!\n",type,type2,type3,type4,val);
		break;
	}

	return 0;
}

/*==========================================
 * スクリプトによるスキル所得
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
	if(!flag && (sd->status.skill[id].id == id || level == 0)){	// クエスト所得ならここで?件を確認して送信する
		sd->status.skill[id].lv=level;
		status_calc_pc(sd,0);
		clif_skillinfoblock(sd);
	}
	else if(flag==2 && (sd->status.skill[id].id == id || level == 0)){	// クエスト所得ならここで?件を確認して送信する
		sd->status.skill[id].lv+=level;
		status_calc_pc(sd,0);
		clif_skillinfoblock(sd);
	}
	else if(sd->status.skill[id].lv < level){	// ?えられるがlvが小さいなら
		if(sd->status.skill[id].id==id)
			sd->status.skill[id].flag=sd->status.skill[id].lv+2;	// lvを記憶
		else {
			sd->status.skill[id].id=id;
			sd->status.skill[id].flag=1;	// cardスキルとする
		}
		sd->status.skill[id].lv=level;
	}

	return 0;
}
/*==========================================
 * カ?ド?入
 *------------------------------------------
 */
int pc_insert_card(struct map_session_data *sd,int idx_card,int idx_equip)
{
	int i, ep;
	int nameid, cardid;

	nullpo_retr(0, sd);

	if(idx_card < 0 || idx_card >= MAX_INVENTORY || !sd->inventory_data[idx_card])
		return 0; //Invalid card index.
			
	if(idx_equip < 0 || idx_equip >= MAX_INVENTORY || !sd->inventory_data[idx_equip])
		return 0; //Invalid item index.
	
	nameid=sd->status.inventory[idx_equip].nameid;
	cardid=sd->status.inventory[idx_card].nameid;
	ep=sd->inventory_data[idx_card]->equip;

	if( nameid <= 0 || cardid <= 0 ||
		(sd->inventory_data[idx_equip]->type!=4 && sd->inventory_data[idx_equip]->type!=5)||	// ? 備じゃない
		sd->inventory_data[idx_card]->type!=6 || // Prevent Hack [Ancyker]
		sd->status.inventory[idx_equip].identify==0 ||		// 未鑑定
		sd->status.inventory[idx_equip].card[0]==0x00ff ||		// 製造武器
		sd->status.inventory[idx_equip].card[0]==0x00fe ||
		sd->status.inventory[idx_equip].card[0]==(short)0xff00 ||
		!(sd->inventory_data[idx_equip]->equip&ep) ||					// ? 備個所違い
		(sd->inventory_data[idx_equip]->type==4 && ep==32) ||			// ? 手武器と盾カ?ド
		sd->status.inventory[idx_equip].equip){

		clif_insert_card(sd,idx_equip,idx_card,1);
		return 0;
	}
	for(i=0;i<sd->inventory_data[idx_equip]->slot;i++){
		if( sd->status.inventory[idx_equip].card[i] == 0){
		// 空きスロットが� ったので差し?む
			sd->status.inventory[idx_equip].card[i]=cardid;

		// カ?ドは減らす
			clif_insert_card(sd,idx_equip,idx_card,0);
			pc_delitem(sd,idx_card,1,1);
			return 0;
		}
	}
	clif_insert_card(sd,idx_equip,idx_card,1);
	return 0;
}

//
// アイテム物
//

/*==========================================
 * スキルによる買い値修正
 *------------------------------------------
 */
int pc_modifybuyvalue(struct map_session_data *sd,int orig_value)
{
	int skill,val = orig_value,rate1 = 0,rate2 = 0;
	if((skill=pc_checkskill(sd,MC_DISCOUNT))>0)	// ディスカウント
		rate1 = 5+skill*2-((skill==10)? 1:0);
	if((skill=pc_checkskill(sd,RG_COMPULSION))>0)	// コムパルションディスカウント
		rate2 = 5+skill*4;
	if(rate1 < rate2) rate1 = rate2;
	if(rate1)
		val = (int)((double)orig_value*(double)(100-rate1)/100.);
	if(val < 0) val = 0;
	if(orig_value > 0 && val < 1) val = 1;

	return val;
}

/*==========================================
 * スキルによる?り値修正
 *------------------------------------------
 */
int pc_modifysellvalue(struct map_session_data *sd,int orig_value)
{
	int skill,val = orig_value,rate = 0;
	if((skill=pc_checkskill(sd,MC_OVERCHARGE))>0)	// オ?バ?チャ?ジ
		rate = 5+skill*2-((skill==10)? 1:0);
	if(rate)
		val = (int)((double)orig_value*(double)(100+rate)/100.);
	if(val < 0) val = 0;
	if(orig_value > 0 && val < 1) val = 1;

	return val;
}

/*==========================================
 * アイテムを買った暫ﾉ、新しいアイテム欄を使うか、
 * 3万個制限にかかるか確認
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
 * 空きアイテム欄の個?
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
 * お金を?う
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
 * お金を得る
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
	if(zeny > 0 && sd->state.showzeny){
		char output[255];
		sprintf(output, "Gained %dz.", zeny);
		clif_disp_onlyself(sd,output,strlen(output));
	}

	return 0;
}

/*==========================================
 * アイテムを探して、インデックスを返す
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
 * アイテム追加。個?のみitem構造?の?字を無視
 *------------------------------------------
 */
int pc_additem(struct map_session_data *sd,struct item *item_data,int amount)
{
	struct item_data *data;
	int i;
	long w;

	nullpo_retr(1, sd);
	nullpo_retr(1, item_data);

	if(item_data->nameid <= 0 || amount <= 0)
		return 1;
	data = itemdb_search(item_data->nameid);
	w = data->weight*amount;
	if(w + (long)sd->weight > (long)sd->max_weight || w + (long)sd->weight < 0) //Weight overflow check?
		return 2;

	i = MAX_INVENTORY;

	if (!itemdb_isequip2(data)){
		// 装 備品ではないので、既所有品なら個数のみ変化させる
		for (i = 0; i < MAX_INVENTORY; i++)
			if(sd->status.inventory[i].nameid == item_data->nameid &&
				sd->status.inventory[i].card[0] == item_data->card[0] &&
				sd->status.inventory[i].card[1] == item_data->card[1] &&
				sd->status.inventory[i].card[2] == item_data->card[2] &&
				sd->status.inventory[i].card[3] == item_data->card[3])
			{
				if (amount < 0 || amount > MAX_AMOUNT || sd->status.inventory[i].amount + amount > MAX_AMOUNT)
					return 5;
				sd->status.inventory[i].amount += amount;
				clif_additem(sd,i,amount,0);
				break;
			}
	}
	if (i >= MAX_INVENTORY){
		// 装 備品か未所有品だったので空き欄へ追加
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
	sd->weight += (int)w;
	clif_updatestatus(sd,SP_WEIGHT);

	return 0;
}

/*==========================================
 * アイテムを減らす
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
 * アイテムを落す
 *------------------------------------------
 */
int pc_dropitem(struct map_session_data *sd,int n,int amount)
{
	nullpo_retr(1, sd);

	if(n < 0 || n >= MAX_INVENTORY)
		return 0;

	if(amount <= 0)
		return 0;

	if (sd->status.inventory[n].nameid <= 0 ||
	    sd->status.inventory[n].amount < amount ||
	    sd->trade_partner != 0 || sd->vender_id != 0 ||
	    sd->status.inventory[n].amount <= 0)
		return 0;

	if (map[sd->bl.m].flag.nodrop) {
		clif_displaymessage (sd->fd, msg_txt(271));
		return 0; //Can't drop items in nodrop mapflag maps.
	}
	
	if (!pc_candrop(sd,sd->status.inventory[n].nameid)) {
		clif_displaymessage (sd->fd, msg_txt(263));
		return 0;
	}
	
	//Logs items, dropped by (P)layers [Lupus]
	if(log_config.pick > 0 )
		log_pick(sd, "P", 0, sd->status.inventory[n].nameid, -amount, (struct item*)&sd->status.inventory[n]);
	//Logs

	if (!map_addflooritem(&sd->status.inventory[n], amount, sd->bl.m, sd->bl.x, sd->bl.y, NULL, NULL, NULL, 2))
		return 0;
	
	pc_delitem(sd, n, amount, 0);
	return 1;
}

/*==========================================
 * アイテムを拾う
 *------------------------------------------
 */
int pc_takeitem(struct map_session_data *sd,struct flooritem_data *fitem)
{
	int flag=0;
	unsigned int tick = gettick();
	struct map_session_data *first_sd = NULL,*second_sd = NULL,*third_sd = NULL;
	struct party *p=NULL;

	nullpo_retr(0, sd);
	nullpo_retr(0, fitem);

	if(!check_distance_bl(&fitem->bl, &sd->bl, 2) && sd->ud.skillid!=BS_GREED)
		return 0;	// 距離が遠い

	if (sd->status.party_id)
		p = party_search(sd->status.party_id);
	
	if(fitem->first_get_id > 0 && fitem->first_get_id != sd->bl.id) {
		first_sd = map_id2sd(fitem->first_get_id);
		if(DIFF_TICK(tick,fitem->first_get_tick) < 0) {
			if (!(p && p->item&1 &&
				first_sd && first_sd->status.party_id == sd->status.party_id
			)) {
				clif_additem(sd,0,0,6);
				return 0;
			}
		}
		else if(fitem->second_get_id > 0 && fitem->second_get_id != sd->bl.id) {
			second_sd = map_id2sd(fitem->second_get_id);
			if(DIFF_TICK(tick, fitem->second_get_tick) < 0) {
				if(!(p && p->item&1 &&
					((first_sd && first_sd->status.party_id == sd->status.party_id) ||
					(second_sd && second_sd->status.party_id == sd->status.party_id))
				)) {
					clif_additem(sd,0,0,6);
					return 0;
				}
			}
			else if(fitem->third_get_id > 0 && fitem->third_get_id != sd->bl.id) {
				third_sd = map_id2sd(fitem->third_get_id);
				if(DIFF_TICK(tick,fitem->third_get_tick) < 0) {
					if(!(p && p->item&1 &&
						((first_sd && first_sd->status.party_id == sd->status.party_id) ||
						(second_sd && second_sd->status.party_id == sd->status.party_id) ||
						(third_sd && third_sd->status.party_id == sd->status.party_id))
					)) {
						clif_additem(sd,0,0,6);
						return 0;
					}
				}
			}
		}
	}
	//This function takes care of giving the item to whoever should have it
	//considering party-share options.
	if ((flag = party_share_loot(p,sd,&fitem->item_data))) {
		clif_additem(sd,0,0,flag);
		return 1;
	}

	//Display pickup animation.
	pc_stop_attack(sd);

	clif_takeitem(&sd->bl,&fitem->bl);
	if(itemdb_autoequip(fitem->item_data.nameid) != 0){
		pc_equipitem(sd, fitem->item_data.nameid, fitem->item_data.equip);
	}
	map_clearflooritem(fitem->bl.id);
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
	//Not consumable item
	if(item->type != 0 && item->type != 2)
		return 0;
	if(!item->script) //if it has no script, you can't really consume it!
		return 0;
	//Anodyne (can't use Anodyne's Endure at GVG)
	if(nameid == 605 && map_flag_gvg(sd->bl.m))
		return 0;
	//Fly Wing (can't use at GVG and when noteleport flag is on)
	if(nameid == 601 && (map[sd->bl.m].flag.noteleport || map_flag_gvg(sd->bl.m))) {
		clif_skill_teleportmessage(sd,0);
		return 0;
	}
	//Fly Wing/Butterfly Wing (can't use when you in duel) [LuzZza]
	if((nameid == 601 || nameid == 602) && (!battle_config.duel_allow_teleport && sd->duel_group)) {
		clif_displaymessage(sd->fd, "Duel: Can't use this item in duel.");
		return 0;
	}
	//Butterfly Wing (can't use noreturn flag is on and if duel)
	if(nameid == 602 && map[sd->bl.m].flag.noreturn)
		return 0;
	//Dead Branch & Bloody Branch & Porings Box (can't use at GVG and when nobranch flag is on)
	if((nameid == 604 || nameid == 12103 || nameid == 12109) && (map[sd->bl.m].flag.nobranch || map_flag_gvg(sd->bl.m)))
		return 0;

	//Anodyne/Aleovera not usable while sitting.
	if ((nameid == 605 || nameid == 606) && pc_issit(sd))
		return 0;
	  
	//added item_noequip.txt items check by Maya&[Lupus]
	if (
		(map[sd->bl.m].flag.pvp && item->flag.no_equip&1) || // PVP
		(map_flag_gvg(sd->bl.m) && item->flag.no_equip&2) || // GVG
		(map[sd->bl.m].flag.restricted && item->flag.no_equip&map[sd->bl.m].zone) // Zone restriction
	)
		return 0;

	//Gender check
	if(item->sex != 2 && sd->status.sex != item->sex)
		return 0;
	//Required level check
	if(item->elv && sd->status.base_level < (unsigned int)item->elv)
		return 0;

	//Not equipable by class. [Skotlex]
	if (!(
		(1<<(sd->class_&MAPID_BASEMASK)) &
		(item->class_base[sd->class_&JOBL_2_1?1:(sd->class_&JOBL_2_2?2:0)])
	))
		return 0;
	
	//Not usable by upper class. [Skotlex]
	if(!(
		(1<<(sd->class_&JOBL_UPPER?1:(sd->class_&JOBL_BABY?2:0))) &
		item->class_upper
	))
		return 0;

	//Dead Branch & Bloody Branch & Porings Box
	if((log_config.branch > 0) && (nameid == 604 || nameid == 12103 || nameid == 12109))
		log_branch(sd);

	return 1;
}

/*==========================================
 * アイテムを使う
 *------------------------------------------
 */
int pc_useitem(struct map_session_data *sd,int n)
{
	unsigned int tick = gettick();
	int amount;
	unsigned char *script;

	nullpo_retr(0, sd);

	if(sd->status.inventory[n].nameid <= 0 ||
		sd->status.inventory[n].amount <= 0)
		return 0;

	if(!pc_isUseitem(sd,n))
		return 0;

	 //Prevent mass item usage. [Skotlex]
	if(DIFF_TICK(sd->canuseitem_tick, tick) > 0)
		return 0;

	if (sd->sc.count && (
		sd->sc.data[SC_BERSERK].timer!=-1 ||
		sd->sc.data[SC_MARIONETTE].timer!=-1 ||
		sd->sc.data[SC_GRAVITATION].timer!=-1 ||
		//Cannot use Potions/Healing items while under Gospel.
		(sd->sc.data[SC_GOSPEL].timer!=-1 && sd->sc.data[SC_GOSPEL].val4 == BCT_SELF && sd->inventory_data[n]->type == 0)
	))
		return 0;
	
	sd->itemid = sd->status.inventory[n].nameid;
	sd->itemindex = n;
	amount = sd->status.inventory[n].amount;
	script = sd->inventory_data[n]->script;
	//Check if the item is to be consumed inmediately [Skotlex]
	if (sd->inventory_data[n]->flag.delay_consume)
		clif_useitemack(sd,n,amount,1);
	else {
		clif_useitemack(sd,n,amount-1,1);
		//Logs (C)onsumable items [Lupus]
		if(log_config.pick > 0 )
			log_pick(sd, "C", 0, sd->status.inventory[n].nameid, -1, &sd->status.inventory[n]);
		//Logs
		pc_delitem(sd,n,1,1);
	}
	if(sd->status.inventory[n].card[0]==0x00fe &&
		pc_istop10fame(MakeDWord(sd->status.inventory[n].card[2],sd->status.inventory[n].card[3]), MAPID_ALCHEMIST))
	{
	    potion_flag = 2; // Famous player's potions have 50% more efficiency
		 if (sd->sc.data[SC_SPIRIT].timer != -1 && sd->sc.data[SC_SPIRIT].val2 == SL_ROGUE)
			 potion_flag = 3; //Even more effective potions.
	}

	sd->canuseitem_tick= tick + battle_config.item_use_interval; //Update item use time.
	run_script(script,0,sd->bl.id,0);
	potion_flag = 0;
	return 1;
}

/*==========================================
 * カ?トアイテム追加。個?のみitem構造?の?字を無視
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
		// 装 備品ではないので、既所有品なら個数のみ変化させる
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
		// 装 備品か未所有品だったので空き欄へ追加
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
 * カ?トアイテムを減らす
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
 * カ?トへアイテム移動
 *------------------------------------------
 */
int pc_putitemtocart(struct map_session_data *sd,int idx,int amount) {
	struct item *item_data;

	nullpo_retr(0, sd);

	if (idx < 0 || idx >= MAX_CART) //Invalid index check [Skotlex]
		return 1;
	
	item_data = &sd->status.inventory[idx];

	if (item_data->nameid==0 || amount < 1 || item_data->amount<amount || sd->vender_id)
		return 1;

	if (pc_cart_additem(sd,item_data,amount) == 0)
		return pc_delitem(sd,idx,amount,0);

	return 1;
}

/*==========================================
 * カ?ト?のアイテム?確認(個?の差分を返す)
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
 * カ?トからアイテム移動
 *------------------------------------------
 */

int pc_getitemfromcart(struct map_session_data *sd,int idx,int amount)
{
	struct item *item_data;
	int flag;

	nullpo_retr(0, sd);

	if (idx < 0 || idx >= MAX_CART) //Invalid index check [Skotlex]
		return 1;
	
	item_data=&sd->status.cart[idx];

	if( item_data->nameid==0 || amount < 1 || item_data->amount<amount || sd->vender_id )
		return 1;
	if((flag = pc_additem(sd,item_data,amount)) == 0)
		return pc_cart_delitem(sd,idx,amount,0);

	clif_additem(sd,0,0,flag);
	return 1;
}

/*==========================================
 * スティル品公開
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
//** pc.c:
int pc_steal_item(struct map_session_data *sd,struct block_list *bl)
{
	int i,j,skill,itemid,flag;
	struct mob_data *md;
	struct item tmp_item;

	if(!sd || !bl || bl->type != BL_MOB)
		return 0;
	
	md=(struct mob_data *)bl;

	if(md->state.steal_flag>battle_config.skill_steal_max_tries || status_get_mode(bl)&MD_BOSS || md->master_id ||
		(md->class_>=1324 && md->class_<1364) || // prevent stealing from treasure boxes [Valaris]
		map[md->bl.m].flag.nomobloot ||        // check noloot map flag [Lorky]
		md->sc.data[SC_STONE].timer != -1 || md->sc.data[SC_FREEZE].timer != -1 //status change check
  	)
		return 0;
	
	skill = battle_config.skill_steal_type == 1
		? (sd->paramc[4] - md->db->dex)/2 + pc_checkskill(sd,TF_STEAL)*6 + 10
		: sd->paramc[4] - md->db->dex + pc_checkskill(sd,TF_STEAL)*3 + 10;

	if (skill < 1)
		return 0;

	if(md->state.steal_flag < battle_config.skill_steal_max_tries)
		md->state.steal_flag++; //increase steal tries number

	for(i = 0; i<MAX_MOB_DROP; i++)//Pick one mobs drop slot.
	{
		itemid = md->db->dropitem[i].nameid;
		if(itemid <= 0 || (itemid>4000 && itemid<5000 && pc_checkskill(sd,TF_STEAL) <= 5))
			continue;
		if(rand() % 10000 > ((md->db->dropitem[i].p * skill) / 100 + sd->add_steal_rate))
			break;
	}
	if (i == MAX_MOB_DROP)
		return 0;

	md->state.steal_flag = 255; //you can't steal from this mob any more
	
	memset(&tmp_item,0,sizeof(tmp_item));
	tmp_item.nameid = itemid;
	tmp_item.amount = 1;
	tmp_item.identify = !itemdb_isequip3(itemid);
	flag = pc_additem(sd,&tmp_item,1);

	if(battle_config.show_steal_in_same_party)
		party_foreachsamemap(pc_show_steal,sd,AREA_SIZE,sd,tmp_item.nameid,flag?1:0);
	if(flag)
		clif_additem(sd,0,0,flag);
	else
	{	//Only invoke logs if item was successfully added (otherwise logs lie about actual item transaction)
		//Logs items, Stolen from mobs [Lupus]
		if(log_config.pick > 0 ) {
			log_pick((struct map_session_data*)md, "M", md->class_, itemid, -1, NULL);
			log_pick(sd, "P", 0, itemid, 1, NULL);
		}
		
		if(log_config.steal) {	//this drop log contains ALL stolen items [Lupus]
			int log_item[10]; //for stolen items logging Lupus
			memset(&log_item,0,sizeof(log_item));
			log_item[i] = itemid; //i == monster's drop slot
			log_drop(sd, md->class_, log_item);
		}

		//A Rare Steal Global Announce by Lupus
		if(md->db->dropitem[i].p<=battle_config.rare_drop_announce) {
			struct item_data *i_data;
			char message[128];
			i_data = itemdb_search(itemid);
			sprintf (message, msg_txt(542), (sd->status.name != NULL)?sd->status.name :"GM", md->db->jname, i_data->jname, (float)md->db->dropitem[i].p/100);
			//MSG: "'%s' stole %s's %s (chance: %%%0.02f)"
			intif_GMmessage(message,strlen(message)+1,0);
		}
	}
	return 1;
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
			if (md->sc.data && (md->sc.data[SC_STONE].timer != -1 || md->sc.data[SC_FREEZE].timer != -1))
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
 * Set's a player position.
 * Return values:
 * 0 - Success.
 * 1 - Invalid map index.
 * 2 - Map not in this map-server, and failed to locate alternate map-server.
 * 3 - Failed to warp player because it was in transition between maps.
 *------------------------------------------
 */
int pc_setpos(struct map_session_data *sd,unsigned short mapindex,int x,int y,int clrtype)
{
	int m;

	nullpo_retr(0, sd);

	if (!mapindex || !mapindex_id2name(mapindex)) {
		ShowDebug("pc_setpos: Passed mapindex(%d) is invalid!\n", mapindex);
		return 1;
	}
	if(sd->state.auth && sd->bl.prev == NULL)
	{	//Should NOT move a character while it is not in a map (changing between maps, for example)
		//state.auth helps identifies if this is the initial setpos rather than a normal map-change set pos.
		if (battle_config.etc_log)
			ShowInfo("pc_setpos failed: Attempted to relocate player %s (%d:%d) while it is still between maps.\n", sd->status.name, sd->status.account_id, sd->status.char_id);
		return 3;
	}

	m=map_mapindex2mapid(mapindex);

	if (sd->mapindex != mapindex)
	{	//Misc map-changing settings
		party_send_dot_remove(sd); //minimap dot fix [Kevin]
		guild_send_dot_remove(sd);
		skill_clear_group(&sd->bl, 1|(battle_config.traps_setting&2));
		if (sd->sc.count)
		{ //Cancel some map related stuff.
			if (sd->sc.data[SC_WARM].timer != -1)
				status_change_end(&sd->bl,SC_WARM,-1);
			if (sd->sc.data[SC_SUN_COMFORT].timer != -1)
				status_change_end(&sd->bl,SC_SUN_COMFORT,-1);
			if (sd->sc.data[SC_MOON_COMFORT].timer != -1)
				status_change_end(&sd->bl,SC_MOON_COMFORT,-1);
			if (sd->sc.data[SC_STAR_COMFORT].timer != -1)
				status_change_end(&sd->bl,SC_STAR_COMFORT,-1);
			if (sd->sc.data[SC_KNOWLEDGE].timer != -1) {
				delete_timer(sd->sc.data[SC_KNOWLEDGE].timer, status_change_timer);
				sd->sc.data[SC_KNOWLEDGE].timer = add_timer(gettick() + skill_get_time(SG_KNOWLEDGE, sd->sc.data[SC_KNOWLEDGE].val1), status_change_timer, sd->bl.id, SC_KNOWLEDGE);
			}
		}
	}

	if(m<0){
		if(sd->mapindex){
			int ip,port;
			if(map_mapname2ipport(mapindex,&ip,&port)==0){
				unit_remove_map(&sd->bl,clrtype);
				sd->mapindex = mapindex;
				sd->bl.x=x;
				sd->bl.y=y;
				sd->state.waitingdisconnect=1;
				pc_clean_skilltree(sd);
				if(sd->status.pet_id > 0 && sd->pd) {
					unit_remove_map(&sd->pd->bl, clrtype);
					intif_save_petdata(sd->status.account_id,&sd->pet);
				}
				chrif_save(sd,1);
				chrif_changemapserver(sd, mapindex, x, y, ip, (short)port);
				return 0;
			}
		}
		return 2;
	}

	if(x <0 || x >= map[m].xs || y <0 || y >= map[m].ys)
		x=y=0;
	if((x==0 && y==0) ||
		(map_getcell(m, x, y, CELL_CHKNOPASS) &&
		!map_getcell(m, x, y, CELL_CHKICEWALL) &&
#ifdef CELL_NOSTACK
		!map_getcell(m, x, y, CELL_CHKSTACK) &&
#endif
		!map_getcell(m, x, y, CELL_CHKMOONLIT))
	){ //It is allowed on top of Moonlight/icewall tiles to prevent force-warping 'cheats' [Skotlex]
		if(x||y) {
			if(battle_config.error_log)
				ShowError("pc_setpos: attempt to place player on non-walkable tile (%s-%d,%d)\n",mapindex_id2name(mapindex),x,y);
		}
		do {
			x=rand()%(map[m].xs-2)+1;
			y=rand()%(map[m].ys-2)+1;
		} while(map_getcell(m,x,y,CELL_CHKNOPASS));
	}

	if(sd->bl.prev != NULL){
		unit_remove_map(&sd->bl, clrtype);
		if(sd->status.pet_id > 0 && sd->pd)
			unit_remove_map(&sd->pd->bl, clrtype);
		clif_changemap(sd,map[m].index,x,y); // [MouseJstr]
	}
	
	sd->mapindex =  mapindex;
	sd->bl.m = m;
	sd->bl.x = sd->ud.to_x = x;
	sd->bl.y = sd->ud.to_y = y;

	if(sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 0) {
		sd->pd->bl.m = m;
		sd->pd->bl.x = sd->pd->ud.to_x = x;
		sd->pd->bl.y = sd->pd->ud.to_y = y;
		sd->pd->ud.dir = sd->ud.dir;
	}

	return 0;
}

/*==========================================
 * PCのランダムワ?プ
 *------------------------------------------
 */
int pc_randomwarp(struct map_session_data *sd, int type) {
	int x,y,i=0;
	int m;

	nullpo_retr(0, sd);

	m=sd->bl.m;

	if (map[sd->bl.m].flag.noteleport)	// テレポ?ト禁止
		return 0;

	do{
		x=rand()%(map[m].xs-2)+1;
		y=rand()%(map[m].ys-2)+1;
	}while(map_getcell(m,x,y,CELL_CHKNOPASS) && (i++)<1000 );

	if (i < 1000)
		return pc_setpos(sd,map[sd->bl.m].index,x,y,type);

	return 0;
}

/*==========================================
 * 現在位置のメモ
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
		if (sd->status.memo_point[j].map == map[sd->bl.m].index) {
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
	sd->status.memo_point[i].map = map[sd->bl.m].index;
	sd->status.memo_point[i].x = sd->bl.x;
	sd->status.memo_point[i].y = sd->bl.y;

	clif_skill_memo(sd, 0);

	return 1;
}

//
// 武器??
//
/*==========================================
 * スキルの?索 所有していた場合Lvが返る
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
 * 武器?更によるスキルの??チェック
 * 引?：
 *   struct map_session_data *sd	セッションデ?タ
 *   int nameid						?備品ID
 * 返り値：
 *   0		?更なし
 *   -1		スキルを解除
 *------------------------------------------
 */
int pc_checkallowskill(struct map_session_data *sd)
{
	const int scw_list[] = {
		SC_TWOHANDQUICKEN,
		SC_ONEHAND,
		SC_AURABLADE,
		SC_PARRYING,
		SC_SPEARSQUICKEN,
		SC_ADRENALINE,
		SC_ADRENALINE2,
		SC_GATLINGFEVER
	};
	const int scs_list[] = {
		SC_AUTOGUARD,
		SC_DEFENDER,
		SC_REFLECTSHIELD
	};
	int i;
	nullpo_retr(0, sd);

	if(!sd->sc.count)
		return 0;
	
	for (i = 0; i < sizeof(scw_list)/sizeof(scw_list[0]); i++)
	{	// Skills requiring specific weapon types
		if(sd->sc.data[scw_list[i]].timer!=-1 && !(skill_get_weapontype(StatusSkillChangeTable[scw_list[i]])&(1<<sd->status.weapon)))
		status_change_end(&sd->bl,scw_list[i],-1);
	}
	
	if(sd->sc.data[SC_SPURT].timer!=-1 && sd->status.weapon)
		// Spurt requires bare hands (feet, in fact xD)
		status_change_end(&sd->bl,SC_SPURT,-1);
	
	if(sd->status.shield <= 0) { // Skills requiring a shield
		for (i = 0; i < sizeof(scs_list)/sizeof(scs_list[0]); i++)
			if(sd->sc.data[scs_list[i]].timer!=-1)	// Guard
				status_change_end(&sd->bl,scs_list[i],-1);
	}
	return 0;
}

/*==========================================
 * ? 備品のチェック
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
 * Convert's from the client's lame Job ID system
 * to the map server's 'makes sense' system. [Skotlex]
 *------------------------------------------
 */
int pc_jobid2mapid(unsigned short b_class)
{
	int class_ = 0;
	if (b_class >= JOB_BABY && b_class <= JOB_SUPER_BABY)
	{
		if (b_class == JOB_SUPER_BABY)
			b_class = JOB_SUPER_NOVICE;
		else
			b_class -= JOB_BABY;
		class_|= JOBL_BABY;
	}
	else if (b_class >= JOB_NOVICE_HIGH && b_class <= JOB_PALADIN2)
	{
		b_class -= JOB_NOVICE_HIGH;
		class_|= JOBL_UPPER;
	}
	if (b_class >= JOB_KNIGHT && b_class <= JOB_KNIGHT2)
		class_|= JOBL_2_1;
	else if (b_class >= JOB_CRUSADER && b_class <= JOB_CRUSADER2)
		class_|= JOBL_2_2;
	switch (b_class)
	{
		case JOB_NOVICE:
		case JOB_SWORDMAN:
		case JOB_MAGE:
		case JOB_ARCHER:
		case JOB_ACOLYTE:
		case JOB_MERCHANT:
		case JOB_THIEF:
			class_ |= b_class;
			break;
		case JOB_KNIGHT:
		case JOB_KNIGHT2:
		case JOB_CRUSADER:
		case JOB_CRUSADER2:
			class_ |= MAPID_SWORDMAN;
			break;
		case JOB_PRIEST:
		case JOB_MONK:
			class_ |= MAPID_ACOLYTE;
			break;
		case JOB_WIZARD:
		case JOB_SAGE:
			class_ |= MAPID_MAGE;
			break;
		case JOB_BLACKSMITH:
		case JOB_ALCHEMIST:
			class_ |= MAPID_MERCHANT;
			break;
		case JOB_HUNTER:
		case JOB_BARD:
		case JOB_DANCER:
			class_ |= MAPID_ARCHER;
			break;
		case JOB_ASSASSIN:
		case JOB_ROGUE:
			class_ |= MAPID_THIEF;
			break;
			
		case JOB_STAR_GLADIATOR:
		case JOB_STAR_GLADIATOR2:
			class_ |= JOBL_2_1;
			class_ |= MAPID_TAEKWON;
			break;	
		case JOB_SOUL_LINKER:
			class_ |= JOBL_2_2;
		case JOB_TAEKWON:
			class_ |= MAPID_TAEKWON;
			break;
		case JOB_WEDDING:
			class_ = MAPID_WEDDING;
			break;
		case JOB_SUPER_NOVICE: //Super Novices are considered 2-1 novices. [Skotlex]
			class_ |= JOBL_2_1;
			break;
		case JOB_GUNSLINGER:
			class_ |= MAPID_GUNSLINGER;
			break;
		case JOB_NINJA:
			class_ |= MAPID_NINJA;
			break;
		case JOB_XMAS:
			class_ = MAPID_XMAS;
			break;
		default:
			ShowError("pc_jobid2mapid: Unrecognized job %d!\n", b_class);
			return -1;
	}
	return class_;
}

//Reverts the map-style class id to the client-style one.
int pc_mapid2jobid(unsigned short class_, int sex) {
	switch(class_) {
		case MAPID_NOVICE:
			return JOB_NOVICE;
		case MAPID_SWORDMAN:
			return JOB_SWORDMAN;
		case MAPID_MAGE:
			return JOB_MAGE;
		case MAPID_ARCHER:
			return JOB_ARCHER;
		case MAPID_ACOLYTE:
			return JOB_ACOLYTE;
		case MAPID_MERCHANT:
			return JOB_MERCHANT;
		case MAPID_THIEF:
			return JOB_THIEF;
		case MAPID_TAEKWON:
			return JOB_TAEKWON;
		case MAPID_WEDDING:
			return JOB_WEDDING;
		case MAPID_GUNSLINGER:
			return JOB_GUNSLINGER;
		case MAPID_NINJA:
			return JOB_NINJA;
		case MAPID_XMAS: // [Valaris]
			return JOB_XMAS;
	//2_1 classes
		case MAPID_SUPER_NOVICE:
			return JOB_SUPER_NOVICE;
		case MAPID_KNIGHT:
			return JOB_KNIGHT;
		case MAPID_WIZARD:
			return JOB_WIZARD;
		case MAPID_HUNTER:
			return JOB_HUNTER;
		case MAPID_PRIEST:
			return JOB_PRIEST;
		case MAPID_BLACKSMITH:
			return JOB_BLACKSMITH;
		case MAPID_ASSASSIN:
			return JOB_ASSASSIN;
		case MAPID_STAR_GLADIATOR:
			return JOB_STAR_GLADIATOR;
	//2_2 classes
		case MAPID_CRUSADER:
			return JOB_CRUSADER;
		case MAPID_SAGE:
			return JOB_SAGE;
		case MAPID_BARDDANCER:
			return sex?JOB_BARD:JOB_DANCER;
		case MAPID_MONK:
			return JOB_MONK;
		case MAPID_ALCHEMIST:
			return JOB_ALCHEMIST;
		case MAPID_ROGUE:
			return JOB_ROGUE;
		case MAPID_SOUL_LINKER:
			return JOB_SOUL_LINKER;
	//1-1: advanced
		case MAPID_NOVICE_HIGH:
			return JOB_NOVICE_HIGH;
		case MAPID_SWORDMAN_HIGH:
			return JOB_SWORDMAN_HIGH;
		case MAPID_MAGE_HIGH:
			return JOB_MAGE_HIGH;
		case MAPID_ARCHER_HIGH:
			return JOB_ARCHER_HIGH;
		case MAPID_ACOLYTE_HIGH:
			return JOB_ACOLYTE_HIGH;
		case MAPID_MERCHANT_HIGH:
			return JOB_MERCHANT_HIGH;
		case MAPID_THIEF_HIGH:
			return JOB_THIEF_HIGH;
	//2_1 advanced
		case MAPID_LORD_KNIGHT:
			return JOB_LORD_KNIGHT;
		case MAPID_HIGH_WIZARD:
			return JOB_HIGH_WIZARD;
		case MAPID_SNIPER:
			return JOB_SNIPER;
		case MAPID_HIGH_PRIEST:
			return JOB_HIGH_PRIEST;
		case MAPID_WHITESMITH:
			return JOB_WHITESMITH;
		case MAPID_ASSASSIN_CROSS:
			return JOB_ASSASSIN_CROSS;
	//2_2 advanced
		case MAPID_PALADIN:
			return JOB_PALADIN;
		case MAPID_PROFESSOR:
			return JOB_PROFESSOR;
		case MAPID_CLOWNGYPSY:
			return sex?JOB_CLOWN:JOB_GYPSY;
		case MAPID_CHAMPION:
			return JOB_CHAMPION;
		case MAPID_CREATOR:
			return JOB_CREATOR;
		case MAPID_STALKER:
			return JOB_STALKER;
	//1-1 baby
		case MAPID_BABY:
			return JOB_BABY;
		case MAPID_BABY_SWORDMAN:
			return JOB_BABY_SWORDMAN;
		case MAPID_BABY_MAGE:
			return JOB_BABY_MAGE;
		case MAPID_BABY_ARCHER:
			return JOB_BABY_ARCHER;
		case MAPID_BABY_ACOLYTE:
			return JOB_BABY_ACOLYTE;
		case MAPID_BABY_MERCHANT:
			return JOB_BABY_MERCHANT;
		case MAPID_BABY_THIEF:
			return JOB_BABY_THIEF;
	//2_1 baby
		case MAPID_SUPER_BABY:
			return JOB_SUPER_BABY;
		case MAPID_BABY_KNIGHT:
			return JOB_BABY_KNIGHT;
		case MAPID_BABY_WIZARD:
			return JOB_BABY_WIZARD;
		case MAPID_BABY_HUNTER:
			return JOB_BABY_HUNTER;
		case MAPID_BABY_PRIEST:
			return JOB_BABY_PRIEST;
		case MAPID_BABY_BLACKSMITH:
			return JOB_BABY_BLACKSMITH;
		case MAPID_BABY_ASSASSIN:
			return JOB_BABY_ASSASSIN;
	//2_2 baby
		case MAPID_BABY_CRUSADER:
			return JOB_BABY_CRUSADER;
		case MAPID_BABY_SAGE:
			return JOB_BABY_SAGE;
		case MAPID_BABY_BARDDANCER:
			return sex?JOB_BABY_BARD:JOB_BABY_DANCER;
		case MAPID_BABY_MONK:
			return JOB_BABY_MONK;
		case MAPID_BABY_ALCHEMIST:
			return JOB_BABY_ALCHEMIST;
		case MAPID_BABY_ROGUE:
			return JOB_BABY_ROGUE;
		default:
			ShowError("pc_mapid2jobid: Unrecognized job %d!\n", class_);
			return -1;
	}
}

/*====================================================
 * This function return the name of the job (by [Yor])
 *----------------------------------------------------
 */
char * job_name(int class_) {
	switch (class_) {
	case JOB_NOVICE:
	case JOB_SWORDMAN:
	case JOB_MAGE:
	case JOB_ARCHER:
	case JOB_ACOLYTE:
	case JOB_MERCHANT:
	case JOB_THIEF:
		return msg_txt(550 - JOB_NOVICE+class_);
		
	case JOB_KNIGHT:
	case JOB_PRIEST:
	case JOB_WIZARD:
	case JOB_BLACKSMITH:
	case JOB_HUNTER:
	case JOB_ASSASSIN:
		return msg_txt(557 - JOB_KNIGHT+class_);
		
	case JOB_KNIGHT2:
		return msg_txt(557);
		
	case JOB_CRUSADER:
	case JOB_MONK:
	case JOB_SAGE:
	case JOB_ROGUE:
	case JOB_ALCHEMIST:
	case JOB_BARD:
	case JOB_DANCER:
		return msg_txt(563 - JOB_CRUSADER+class_);
			
	case JOB_CRUSADER2:
		return msg_txt(563);
		
	case JOB_WEDDING:
	case JOB_SUPER_NOVICE:

	case JOB_XMAS:
		return msg_txt(570 - JOB_WEDDING+class_);
		
	case JOB_NOVICE_HIGH:
	case JOB_SWORDMAN_HIGH:
	case JOB_MAGE_HIGH:
	case JOB_ARCHER_HIGH:
	case JOB_ACOLYTE_HIGH:
	case JOB_MERCHANT_HIGH:
	case JOB_THIEF_HIGH:
		return msg_txt(575 - JOB_NOVICE_HIGH+class_);

	case JOB_LORD_KNIGHT:
	case JOB_HIGH_PRIEST:
	case JOB_HIGH_WIZARD:
	case JOB_WHITESMITH:
	case JOB_SNIPER:
	case JOB_ASSASSIN_CROSS:
		return msg_txt(582 - JOB_LORD_KNIGHT+class_);
		
	case JOB_LORD_KNIGHT2:
		return msg_txt(582);
		
	case JOB_PALADIN:
	case JOB_CHAMPION:
	case JOB_PROFESSOR:
	case JOB_STALKER:
	case JOB_CREATOR:
	case JOB_CLOWN:
	case JOB_GYPSY:
		return msg_txt(588 - JOB_PALADIN + class_);
		
	case JOB_PALADIN2:
		return msg_txt(588);

	case JOB_BABY:
	case JOB_BABY_SWORDMAN:
	case JOB_BABY_MAGE:
	case JOB_BABY_ARCHER:
	case JOB_BABY_ACOLYTE:
	case JOB_BABY_MERCHANT:
	case JOB_BABY_THIEF:
		return msg_txt(595 - JOB_BABY + class_);
		
	case JOB_BABY_KNIGHT:
	case JOB_BABY_PRIEST:
	case JOB_BABY_WIZARD:
	case JOB_BABY_BLACKSMITH:
	case JOB_BABY_HUNTER:
	case JOB_BABY_ASSASSIN:
		return msg_txt(602 - JOB_BABY_KNIGHT + class_);
		
	case JOB_BABY_KNIGHT2:
		return msg_txt(602);
		
	case JOB_BABY_CRUSADER:
	case JOB_BABY_MONK:
	case JOB_BABY_SAGE:
	case JOB_BABY_ROGUE:
	case JOB_BABY_ALCHEMIST:
	case JOB_BABY_BARD:
	case JOB_BABY_DANCER:
		return msg_txt(608 - JOB_BABY_CRUSADER +class_);
		
	case JOB_BABY_CRUSADER2:
		return msg_txt(608);
		
	case JOB_SUPER_BABY:
		return msg_txt(615);
		
	case JOB_TAEKWON:
		return msg_txt(616);
	case JOB_STAR_GLADIATOR:
	case JOB_STAR_GLADIATOR2:
		return msg_txt(617);
	case JOB_SOUL_LINKER:
		return msg_txt(618);
		
	case JOB_GUNSLINGER:
		return msg_txt(619);
	case JOB_NINJA:
		return msg_txt(620);
	
	default:
		return msg_txt(650);
	}
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

	if ((tsd = map_id2sd(sd->followtarget)) == NULL || pc_isdead(tsd))
		return 0;

	// either player or target is currently detached from map blocks (could be teleporting),
	// but still connected to this map, so we'll just increment the timer and check back later
	if (sd->bl.prev != NULL && tsd->bl.prev != NULL &&
		sd->ud.skilltimer == -1 && sd->ud.attacktimer == -1 && sd->ud.walktimer == -1)
	{
		if((sd->bl.m == tsd->bl.m) && unit_can_reach_bl(&sd->bl,&tsd->bl, AREA_SIZE, 0, NULL, NULL)) {
			if (!check_distance_bl(&sd->bl, &tsd->bl, 5))
				unit_walktobl(&sd->bl, &tsd->bl, 5, 0);
		} else
			pc_setpos(sd, tsd->mapindex, tsd->bl.x, tsd->bl.y, 3);
	}
	sd->followtimer = add_timer(
		tick + 1000,	// increase time a bit to loosen up map's load
		pc_follow_timer, sd->bl.id, 0);
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
	if (bl == NULL /*|| bl->type != BL_PC*/)
		return 1;
	if (sd->followtimer != -1)
		pc_stop_following(sd);

	sd->followtarget = target_id;
	pc_follow_timer(-1,gettick(),sd->bl.id,0);

	return 0;
}

int pc_checkbaselevelup(struct map_session_data *sd)
{
	unsigned int next = pc_nextbaseexp(sd);

	nullpo_retr(0, sd);

	if(sd->status.base_exp >= next && next > 0){
		sd->status.base_exp -= next;
		//Kyoki pointed out that the max overcarry exp is the exp needed for the previous level -1. [Skotlex]
		if(!battle_config.multi_level_up && sd->status.base_exp > next-1)
			sd->status.base_exp = next-1;

		sd->status.base_level ++;

		if (battle_config.pet_lv_rate && sd->pd)	//<Skotlex> update pet's level
			status_calc_pet(sd,0);
		if (battle_config.use_statpoint_table)
			next = statp[sd->status.base_level] - statp[sd->status.base_level-1];
		else //Estimated way.
			next = (sd->status.base_level+14) / 5 ;
		if (sd->status.status_point > USHRT_MAX - next)
			sd->status.status_point = USHRT_MAX;
		else	
			sd->status.status_point += next;
		clif_updatestatus(sd,SP_STATUSPOINT);
		clif_updatestatus(sd,SP_BASELEVEL);
		clif_updatestatus(sd,SP_NEXTBASEEXP);
		status_calc_pc(sd,0);
		pc_heal(sd,sd->status.max_hp,sd->status.max_sp);

		//スパノビはキリエ、イムポ、マニピ、グロ、サフラLv1がかかる
		if((sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE)
		{
			sc_start(&sd->bl,SkillStatusChangeTable[PR_KYRIE],100,1,skill_get_time(PR_KYRIE,1));
			sc_start(&sd->bl,SkillStatusChangeTable[PR_IMPOSITIO],100,1,skill_get_time(PR_IMPOSITIO,1));
			sc_start(&sd->bl,SkillStatusChangeTable[PR_MAGNIFICAT],100,1,skill_get_time(PR_MAGNIFICAT,1));
			sc_start(&sd->bl,SkillStatusChangeTable[PR_GLORIA],100,1,skill_get_time(PR_GLORIA,1));
			sc_start(&sd->bl,SkillStatusChangeTable[PR_SUFFRAGIUM],100,1,skill_get_time(PR_SUFFRAGIUM,1));
		} else
		if((sd->class_&MAPID_UPPERMASK) == MAPID_TAEKWON)
		{
			sc_start(&sd->bl,SkillStatusChangeTable[AL_INCAGI],100,10,skill_get_time(AL_INCAGI,10));
			sc_start(&sd->bl,SkillStatusChangeTable[AL_BLESSING],100,10,skill_get_time(AL_BLESSING,10));
		}
		clif_misceffect(&sd->bl,0);
		//LORDALFA - LVLUPEVENT
		if (script_config.event_script_type == 0) {
			struct npc_data *npc;
			if ((npc = npc_name2id(script_config.baselvup_event_name))) {
				run_script(npc->u.scr.script,0,sd->bl.id,npc->bl.id); // PCLvlUPNPC
				ShowStatus("Event '"CL_WHITE"%s"CL_RESET"' executed.\n",script_config.baselvup_event_name);
			}
		} else {
				ShowStatus("%d '"CL_WHITE"%s"CL_RESET"' events executed.\n",
				npc_event_doall_id(script_config.baselvup_event_name, sd->bl.id), script_config.baselvup_event_name);
		}

		return 1;
		}

	return 0;
}

int pc_checkjoblevelup(struct map_session_data *sd)
{
	unsigned int next = pc_nextjobexp(sd);

	nullpo_retr(0, sd);

	if(sd->status.job_exp >= next && next > 0){
		sd->status.job_exp -= next;
		//Kyoki pointed out that the max overcarry exp is the exp needed for the previous level -1. [Skotlex]
		if(!battle_config.multi_level_up && sd->status.job_exp > next-1)
			sd->status.job_exp = next-1;

		sd->status.job_level ++;
		  
		clif_updatestatus(sd,SP_JOBLEVEL);
		clif_updatestatus(sd,SP_NEXTJOBEXP);
		sd->status.skill_point ++;
		clif_updatestatus(sd,SP_SKILLPOINT);
		status_calc_pc(sd,0);

		clif_misceffect(&sd->bl,1);
		if (pc_checkskill(sd, SG_DEVIL) && !pc_nextjobexp(sd))
			clif_status_change(&sd->bl,SI_DEVIL, 1); //Permanent blind effect from SG_DEVIL.

		if (script_config.event_script_type == 0) {
			struct npc_data *npc;
			if ((npc = npc_name2id(script_config.joblvup_event_name))) {
				run_script(npc->u.scr.script,0,sd->bl.id,npc->bl.id); // PCLvlUPNPC
				ShowStatus("Event '"CL_WHITE"%s"CL_RESET"' executed.\n",script_config.joblvup_event_name);
			}
		} else {
				ShowStatus("%d '"CL_WHITE"%s"CL_RESET"' events executed.\n",
				npc_event_doall_id(script_config.joblvup_event_name, sd->bl.id), script_config.joblvup_event_name);
		}

		return 1;
	}

	return 0;
}

/*==========================================
 * ??値取得
 *------------------------------------------
 */
int pc_gainexp(struct map_session_data *sd,unsigned int base_exp,unsigned int job_exp)
{
	char output[256];
	float nextbp=0, nextjp=0;
	unsigned int nextb=0, nextj=0;
	nullpo_retr(0, sd);

	if(sd->bl.prev == NULL || pc_isdead(sd))
		return 0;

	if(!battle_config.pvp_exp && map[sd->bl.m].flag.pvp)  // [MouseJstr]
		return 0; // no exp on pvp maps

	if(sd->status.guild_id>0){	// ギルドに上納
		base_exp-=guild_payexp(sd,base_exp);
	}

	nextb = pc_nextbaseexp(sd);
	nextj = pc_nextjobexp(sd);
	
		
	if(sd->state.showexp || battle_config.max_exp_gain_rate){
		if (nextb > 0)
			nextbp = (float) base_exp / (float) nextb;
		if (nextj > 0)
			nextjp = (float) job_exp / (float) nextj;

		if(battle_config.max_exp_gain_rate) {
			if (nextbp > battle_config.max_exp_gain_rate/1000.) {
				//Note that this value should never be greater than the original
				//base_exp, therefore no overflow checks are needed. [Skotlex]
				base_exp = (unsigned int)(battle_config.max_exp_gain_rate/1000.*nextb);
				if (sd->state.showexp)
					nextbp = (float) base_exp / (float) nextb;
			}
			if (nextjp > battle_config.max_exp_gain_rate/1000.) {
				job_exp = (unsigned int)(battle_config.max_exp_gain_rate/1000.*nextj);
				if (sd->state.showexp)
					nextjp = (float) job_exp / (float) nextj;
			}
		}
	}
	
	//Overflow checks... think we'll ever really need'em? [Skotlex]
	if (base_exp > 0 && sd->status.base_exp > UINT_MAX - base_exp)
		sd->status.base_exp = UINT_MAX;
	else if (base_exp < 0 && sd->status.base_exp > base_exp)
		sd->status.base_exp = 0;
	else
		sd->status.base_exp += base_exp;
	
	while(pc_checkbaselevelup(sd)) ;

	clif_updatestatus(sd,SP_BASEEXP);

	//Overflow checks... think we'll ever really need'em? [Skotlex]
	if (job_exp > 0 && sd->status.job_exp > UINT_MAX - job_exp)
		sd->status.job_exp = UINT_MAX;
	else if (job_exp < 0 && sd->status.job_exp > job_exp)
		sd->status.job_exp = 0;
	else
		sd->status.job_exp += job_exp;

	while(pc_checkjoblevelup(sd)) ;

	clif_updatestatus(sd,SP_JOBEXP);

	if(sd->state.showexp){
		sprintf(output,
			"Experience Gained Base:%u (%.2f%%) Job:%u (%.2f%%)",base_exp,nextbp*(float)100,job_exp,nextjp*(float)100);
		clif_disp_onlyself(sd,output,strlen(output));
	}

	return 1;
}

/*==========================================
 * Returns max level for this character.
 *------------------------------------------
 */

unsigned int pc_maxbaselv(struct map_session_data *sd) {
  	return max_level[sd->status.class_][0];
};

unsigned int pc_maxjoblv(struct map_session_data *sd) {
  	return max_level[sd->status.class_][1];
};

/*==========================================
 * base level側必要??値計算
 *------------------------------------------
 */
unsigned int pc_nextbaseexp(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->status.base_level>=pc_maxbaselv(sd) || sd->status.base_level<=0)
		return 0;

	return exp_table[sd->status.class_][0][sd->status.base_level-1];
}

/*==========================================
 * job level側必要??値計算
 *------------------------------------------
 */
unsigned int pc_nextjobexp(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->status.job_level>=pc_maxjoblv(sd) || sd->status.job_level<=0)
		return 0;
	return exp_table[sd->status.class_][1][sd->status.job_level-1];
}

/*==========================================

 * 必要ステ?タスポイント計算
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
 * 能力値成長
 *------------------------------------------
 */
int pc_statusup(struct map_session_data *sd,int type)
{
	int max, need,val = 0;

	nullpo_retr(0, sd);

	max = pc_maxparameter(sd);

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
 * 能力値成長
 *------------------------------------------
 */
int pc_statusup2(struct map_session_data *sd,int type,int val)
{
	int max;
	nullpo_retr(0, sd);

	max = pc_maxparameter(sd);
	
	if(type<SP_STR || type>SP_LUK){
		clif_statusupack(sd,type,0,0);
		return 1;
	}
	switch(type){
	case SP_STR:
		if(sd->status.str + val >= max)
			val = max;
		else if(sd->status.str + val < 1)
			val = 1;
		else
			val += sd->status.str;
		sd->status.str = val;
		break;
	case SP_AGI:
		if(sd->status.agi + val >= max)
			val = max;
		else if(sd->status.agi + val < 1)
			val = 1;
		else
			val += sd->status.agi;
		sd->status.agi = val;
		break;
	case SP_VIT:
		if(sd->status.vit + val >= max)
			val = max;
		else if(sd->status.vit + val < 1)
			val = 1;
		else
			val += sd->status.vit;
		sd->status.vit = val;
		break;
	case SP_INT:
		if(sd->status.int_ + val >= max)
			val = max;
		else if(sd->status.int_ + val < 1)
			val = 1;
		else
			val += sd->status.int_;
		sd->status.int_ = val;
		break;
	case SP_DEX:
		if(sd->status.dex + val >= max)
			val = max;
		else if(sd->status.dex + val < 1)
			val = 1;
		else
			val += sd->status.dex;
		sd->status.dex = val;
		break;
	case SP_LUK:
		if(sd->status.luk + val >= max)
			val = max;
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
 * スキルポイント割り振り
 *------------------------------------------
 */
int pc_skillup(struct map_session_data *sd,int skill_num)
{
	nullpo_retr(0, sd);

	if( skill_num>=GD_SKILLBASE){
		guild_skillup(sd,skill_num,0);
		return 0;
	}
	if (skill_num < 0 || skill_num >= MAX_SKILL)
		return 0;
	
	if(sd->status.skill_point>0 &&
		sd->status.skill[skill_num].id &&
		sd->status.skill[skill_num].flag==0 && //Don't allow raising while you have granted skills. [Skotlex]
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
		if (sd->status.skill[i].flag && sd->status.skill[i].flag != 13){	// cardスキルなら、
			sd->status.skill[i].lv=(sd->status.skill[i].flag==1)?0:sd->status.skill[i].flag-2;	// 本?のlvに
			sd->status.skill[i].flag=0;	// flagは0にしておく
		}
	}

	if (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill){
		// 全てのスキル
		for(i=0;i<MAX_SKILL;i++){
			if(!(skill_get_inf2(i)&(INF2_NPC_SKILL|INF2_GUILD_SKILL))) //Get ALL skills except npc/guild ones. [Skotlex]
				if (i!=SG_DEVIL) //and except SG_DEVIL [Komurka]
					sd->status.skill[i].lv=skill_get_max(i); //Nonexistant skills should return a max of 0 anyway.
		}
	}
	else {
		int inf2;
		for(i=0;i < MAX_SKILL_TREE && (id=skill_tree[sd->status.class_][i].id)>0;i++){
			inf2 = skill_get_inf2(id);
			if(sd->status.skill[id].id==0 &&
				(!(inf2&INF2_QUEST_SKILL) || battle_config.quest_skill_learn) &&
				!(inf2&(INF2_WEDDING_SKILL|INF2_SPIRIT_SKILL)) &&
				(id!=SG_DEVIL))
			 {
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

	if (type != 3) //Also reset skills
		pc_resetskill(sd, 0);

	if(type == 1){
	sd->status.skill_point=0;
	sd->status.base_level=1;
	sd->status.job_level=1;
	sd->status.base_exp=sd->status.base_exp=0;
	sd->status.job_exp=sd->status.job_exp=0;
	if(sd->sc.option !=0)
		sd->sc.option = 0;

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

	if ((type == 1 || type == 2 || type == 3) && sd->status.party_id) {
		//Send map-change packet to do a level range check and break party settings. [Skotlex]
		party_send_movemap(sd);
	}
	status_calc_pc(sd,0);
	clif_skillinfoblock(sd);

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
		int stat;
		stat = statp[sd->status.base_level];
		if (sd->class_&JOBL_UPPER)
			stat+=52;	// extra 52+48=100 stat points
		
		if (stat > USHRT_MAX)
			sd->status.status_point = USHRT_MAX;
		else
			sd->status.status_point = stat;
	} else { //Use new stat-calculating equation [Skotlex]
#define sumsp(a) (((a-1)/10 +2)*(5*((a-1)/10 +1) + (a-1)%10) -10)
		int add=0;
		add += sumsp(sd->status.str);
		add += sumsp(sd->status.agi);
		add += sumsp(sd->status.vit);
		add += sumsp(sd->status.int_);
		add += sumsp(sd->status.dex);
		add += sumsp(sd->status.luk);
		if (add > USHRT_MAX - sd->status.status_point)
			sd->status.status_point = USHRT_MAX;
		else
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
 * if flag is 1, perform block resync and status_calc call.
 *------------------------------------------
 */
int pc_resetskill(struct map_session_data* sd, int flag)
{
	int i, skill, inf2, skill_point=0;
	nullpo_retr(0, sd);

	if (pc_checkskill(sd, SG_DEVIL) &&  !pc_nextjobexp(sd))
		clif_status_load(&sd->bl, SI_DEVIL, 0); //Remove perma blindness due to skill-reset. [Skotlex]
	
	for (i = 1; i < MAX_SKILL; i++) {
		if ((skill = sd->status.skill[i].lv) > 0) {
			inf2 = skill_get_inf2(i);	
			if ((!(inf2&INF2_QUEST_SKILL) || battle_config.quest_skill_learn) &&
				!(inf2&(INF2_WEDDING_SKILL|INF2_SPIRIT_SKILL))) //Avoid reseting wedding/linker skills.
			{
					if (!sd->status.skill[i].flag)
						skill_point += skill;
					else if (sd->status.skill[i].flag > 2 && sd->status.skill[i].flag != 13)
						skill_point += (sd->status.skill[i].flag - 2);
					sd->status.skill[i].lv = 0;
					sd->status.skill[i].flag = 0;
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
	
	if (sd->status.skill_point > USHRT_MAX - skill_point)
		sd->status.skill_point = USHRT_MAX;
	else
		sd->status.skill_point += skill_point;
	
	if (flag) {
		clif_updatestatus(sd,SP_SKILLPOINT);
		clif_skillinfoblock(sd);
		status_calc_pc(sd,0);
	}

	return 0;
}

/*==========================================
 * /resetfeel [Komurka]
 *------------------------------------------
 */
int pc_resetfeel(struct map_session_data* sd)
{
	int i;
	char feel_var[3][NAME_LENGTH] = {"PC_FEEL_SUN","PC_FEEL_MOON","PC_FEEL_STAR"};
	nullpo_retr(0, sd);

	for (i=0; i<3; i++)
	{
		sd->feel_map[i].m = -1;
		sd->feel_map[i].index = 0;
		pc_setglobalreg(sd,feel_var[i],0);
	}

	return 0;
}

static int pc_respawn(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd = map_id2sd(id);
	if (sd && pc_isdead(sd))
	{	//Auto-respawn [Skotlex]
		pc_setstand(sd);
		pc_setrestartvalue(sd,3);
		pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,0);
	}
	return 0;
}

/*==========================================
 * Damages a player's SP, returns remaining SP. [Skotlex]
 * damage is absolute damage, rate is % damage (100 = 100%)
 * if rate is positive, it is % of current sp, if negative, it is % of max 
 * Returns remaining SP, or -1 if the player did not have enough SP to substract from.
 *------------------------------------------
 */
int pc_damage_sp(struct map_session_data *sd, int damage, int rate)
{
	if (!sd->status.sp)
		return 0;
	
	if (rate > 0)
		damage += (rate*sd->status.sp)/100;
	else if (rate < 0)
		damage -= (rate*sd->status.max_sp)/100;

	
	if (sd->status.sp >= damage){
		sd->status.sp -= damage;
		clif_updatestatus(sd,SP_SP);
		return sd->status.sp;
	}
	sd->status.sp = 0;
	clif_updatestatus(sd,SP_SP);
	return -1;
}
/*==========================================
 * pcにダメ?ジを?える
 *------------------------------------------
 */
int pc_damage(struct block_list *src,struct map_session_data *sd,int damage)
{
	int i=0,j=0, resurrect_flag=0;

	nullpo_retr(0, sd);

	// ?に死んでいたら無?
	if(pc_isdead(sd))
		return 0;
	// 座ってたら立ち上がる
	if(pc_issit(sd)) {
		pc_setstand(sd);
		skill_gangsterparadise(sd,0);
		skill_rest(sd,0);
	}

	// 演奏/ダンスの中?
	if(damage > sd->status.max_hp>>2)
		skill_stop_dancing(&sd->bl);

	if (damage < sd->status.hp)
		sd->status.hp-=damage;
	else
		sd->status.hp = 0;
	
	if(sd->status.pet_id > 0 && sd->pd && sd->petDB && battle_config.pet_damage_support)
		pet_target_check(sd,src,1);

	clif_updatestatus(sd,SP_HP);

 
	if (sd->status.hp<sd->status.max_hp>>2) { //25% HP left effects.
		if(sd->status.hp>0 && sd->sc.data[SC_AUTOBERSERK].timer != -1 &&
			(sd->sc.data[SC_PROVOKE].timer==-1 || sd->sc.data[SC_PROVOKE].val2==0 ))
			sc_start4(&sd->bl,SC_PROVOKE,100,10,1,0,0,0);

		for(i = 0; i < 5; i++)
			if (sd->devotion[i]){
				struct map_session_data *devsd = map_id2sd(sd->devotion[i]);
				if (devsd) status_change_end(&devsd->bl,SC_DEVOTION,-1);
				sd->devotion[i] = 0;
			}
	}

	if(sd->status.hp>0){
		sd->canlog_tick = gettick();
		return damage;
	}

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

	// Leave duel if you die [LuzZza]
	if(battle_config.duel_autoleave_when_die) {
		if(sd->duel_group > 0)
			duel_leave(sd->duel_group, sd);
		if(sd->duel_invite > 0)
			duel_reject(sd->duel_invite, sd);
	}

	pc_stop_attack(sd);
	pc_stop_walking(sd,0);
	unit_skillcastcancel(&sd->bl,0);
	skill_stop_dancing(&sd->bl); //You should stop dancing when dead... [Skotlex]
	if (sd->sc.data[SC_GOSPEL].timer != -1 && sd->sc.data[SC_GOSPEL].val4 == BCT_SELF)
	{	//Remove Gospel [Skotlex]
		struct skill_unit_group *sg = (struct skill_unit_group *)sd->sc.data[SC_GOSPEL].val3;
		if (sg)
			skill_delunitgroup(&sd->bl, sg);
	}
	clif_clearchar_area(&sd->bl,1);

	if (src && src->type == BL_PC) {
		struct map_session_data *ssd = (struct map_session_data *)src;
		if (ssd) {
			if (sd->state.event_death)
				pc_setglobalreg(sd,"killerrid",(ssd->status.account_id));
			if (ssd->state.event_kill_pc) {
				pc_setglobalreg(ssd, "killedrid", sd->bl.id);
				if (script_config.event_script_type == 0) {
					struct npc_data *npc;
					if ((npc = npc_name2id(script_config.kill_pc_event_name))) {
						run_script(npc->u.scr.script,0,ssd->bl.id,npc->bl.id); // PCKillPC [Lance]
						ShowStatus("Event '"CL_WHITE"%s"CL_RESET"' executed.\n", script_config.kill_pc_event_name);
					}
				} else {
					ShowStatus("%d '"CL_WHITE"%s"CL_RESET"' events executed.\n",
						npc_event_doall_id(script_config.kill_pc_event_name, ssd->bl.id), script_config.kill_pc_event_name);
				}
			}
			if (battle_config.pk_mode && ssd->status.manner >= 0 && battle_config.manner_system) {
				ssd->status.manner -= 5;
				if(ssd->status.manner < 0)
					sc_start(src,SC_NOCHAT,100,0,0);

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
	} else {
		pc_setglobalreg(sd, "killerrid", 0);
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
	/*if(sd->status.karma > 0) {
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
		|| (battle_config.bone_drop==1 && map[sd->bl.m].flag.pvp)){	// ドクロドロップ
		struct item item_tmp;
		memset(&item_tmp,0,sizeof(item_tmp));
		item_tmp.nameid=7420; //PVP Skull item ID
		item_tmp.identify=1;
		item_tmp.card[0]=0x00fe;
		item_tmp.card[1]=0;
		item_tmp.card[2]=GetWord(sd->char_id,0); // CharId
		item_tmp.card[3]=GetWord(sd->char_id,1);
		map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
	}

	// activate Steel body if a super novice dies at 99+% exp [celest]
	if ((sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE) {
		if ((i=pc_nextbaseexp(sd))<=0)
			i=sd->status.base_exp;
		if (i>0 && (j=sd->status.base_exp*1000/i)>=990 && j<1000)
			sd->state.snovice_flag = 4;
	}
	
	pc_setdead(sd);
	skill_unit_move(&sd->bl,gettick(),4);
	if (battle_config.clear_unit_ondeath)
		skill_clear_unitgroup(&sd->bl); //orn
	
	pc_setglobalreg(sd,"PC_DIE_COUNTER",++sd->die_counter); //死にカウンタ?書き?み
	 // changed penalty options, added death by player if pk_mode [Valaris]
	if(battle_config.death_penalty_type && sd->state.snovice_flag != 4
		&& (sd->class_&MAPID_UPPERMASK) != MAPID_NOVICE	// only novices will receive no penalty
		&& !map[sd->bl.m].flag.nopenalty && !map_flag_gvg(sd->bl.m)
		&& !(sd->sc.count && sd->sc.data[SC_BABY].timer!=-1))
	{
		unsigned int base_penalty =0;
		if (battle_config.death_penalty_base > 0) {
			switch (battle_config.death_penalty_type) {
				case 1:
					base_penalty += (unsigned int) ((double)pc_nextbaseexp(sd) * (double)battle_config.death_penalty_base/10000);
				break;
				case 2:
					base_penalty = (unsigned int) ((double)sd->status.base_exp * (double)battle_config.death_penalty_base/10000);
				break;
			}
			if(base_penalty) {
			  	if (battle_config.pk_mode && src && src->type==BL_PC)
					base_penalty*=2;
				if(sd->status.base_exp < base_penalty)
					sd->status.base_exp = 0;
				else
					sd->status.base_exp -= base_penalty;
				clif_updatestatus(sd,SP_BASEEXP);
			}
		}
		if(battle_config.death_penalty_job > 0)
	  	{
			base_penalty = 0;
			switch (battle_config.death_penalty_type) {
				case 1:
					base_penalty = (unsigned int) ((double)pc_nextjobexp(sd) * (double)battle_config.death_penalty_job/10000);
				break;
				case 2:
					base_penalty = (unsigned int) ((double)sd->status.job_exp * (double)battle_config.death_penalty_job/10000);
				break;
			}
			if(base_penalty) {
			  	if (battle_config.pk_mode && src && src->type==BL_PC)
					base_penalty*=2;
				if(sd->status.job_exp < base_penalty)
					sd->status.job_exp = 0;
				else
					sd->status.job_exp -= base_penalty;
				clif_updatestatus(sd,SP_JOBEXP);
			}
		}
	}
	if(src && src->type==BL_MOB) {
		struct mob_data *md=(struct mob_data *)src;
		if(md && md->target_id != 0 && md->target_id==sd->bl.id) {
			// reset target id when player dies
			mob_unlocktarget(md,gettick());
		}
		if(battle_config.mobs_level_up && md && md->hp &&
			md->level < pc_maxbaselv(sd) &&
			!md->guardian_data && !md->special_state.ai// Guardians/summons should not level. [Skotlex]
		) { 	// monster level up [Valaris]
			clif_misceffect(&md->bl,0);
			md->level++;
			md->hp+=(int) (sd->status.max_hp*.1);
			if (battle_config.show_mob_hp)
				clif_charnameack (0, &md->bl);
		}
	}
	//Clear these data here so that SC_BABY check may work. [Skotlex]
	resurrect_flag = (sd->sc.data[SC_KAIZEL].timer != -1)?sd->sc.data[SC_KAIZEL].val1:0; //Auto-resurrect later in the code.
	status_change_clear(&sd->bl,0);	// ステ?タス異常を解除する
	clif_updatestatus(sd,SP_HP);
	status_calc_pc(sd,0);
	sd->canregen_tick = gettick();
	

	//ナイトメアモ?ドアイテムドロップ
	if(map[sd->bl.m].flag.pvp_nightmaredrop){ // Moved this outside so it works when PVP isnt enabled and during pk mode [Ancyker]
		for(j=0;j<MAX_DROP_PER_MAP;j++){
			int id = map[sd->bl.m].drop_list[j].drop_id;
			int type = map[sd->bl.m].drop_list[j].drop_type;
			int per = map[sd->bl.m].drop_list[j].drop_per;
			if(id == 0)
				continue;
			if(id == -1){//ランダムドロップ
				int eq_num=0,eq_n[MAX_INVENTORY];
				memset(eq_n,0,sizeof(eq_n));
				//先ず?備しているアイテム?をカウント
				for(i=0;i<MAX_INVENTORY;i++){
					int k;
					if( (type == 1 && !sd->status.inventory[i].equip)
						|| (type == 2 && sd->status.inventory[i].equip)
						||  type == 3){
						//InventoryIndexを格納
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
					int n = eq_n[rand()%eq_num];//該?アイテムの中からランダム
					if(rand()%10000 < per){
						if(sd->status.inventory[n].equip)
							pc_unequipitem(sd,n,3);
						pc_dropitem(sd,n,1);
					}
				}
			}
			else if(id > 0){
				for(i=0;i<MAX_INVENTORY;i++){
					if(sd->status.inventory[i].nameid == id//ItemIDが一致していて
						&& rand()%10000 < per//ドロップ率判定もOKで
						&& ((type == 1 && !sd->status.inventory[i].equip)//タイプ判定もOKならドロップ
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
		//ランキング計算
		if (!map[sd->bl.m].flag.pvp_nocalcrank) {
			sd->pvp_point -= 5;
			sd->pvp_lost++;
			if (src && src->type == BL_PC) {
				struct map_session_data *ssd = (struct map_session_data *)src;
				if (ssd) { ssd->pvp_point++; ssd->pvp_won++; }
			}
		}
		// ?制送還
		if( sd->pvp_point < 0 ){
			sd->pvp_point=0;
			add_timer(gettick()+1000, pc_respawn,sd->bl.id,0);
			return damage;
		}
	}
	//GvG
	if(map_flag_gvg(sd->bl.m)){
		add_timer(gettick()+1000, pc_respawn,sd->bl.id,0);
		return damage;
	}

	if (sd->state.snovice_flag == 4 || resurrect_flag) {
		if (sd->state.snovice_flag == 4 || sd->special_state.restart_full_recover) {
			sd->status.hp = sd->status.max_hp;
			sd->status.sp = sd->status.max_sp;
		} else { //10% life per each level in Kaizel
			sd->status.hp = 10*resurrect_flag*sd->status.max_hp/100;
		}
		clif_skill_nodamage(&sd->bl,&sd->bl,ALL_RESURRECTION,1,1);
		pc_setstand(sd);
		clif_updatestatus(sd, SP_HP);
		clif_updatestatus(sd, SP_SP);
		clif_resurrection(&sd->bl, 1);
		sd->state.snovice_flag = 0;
		if(battle_config.pc_invincible_time)
			pc_setinvincibletimer(sd, battle_config.pc_invincible_time);
		if (resurrect_flag)
			sc_start(&sd->bl,SkillStatusChangeTable[PR_KYRIE],100,10,skill_get_time2(SL_KAIZEL, resurrect_flag));
		else
			sc_start(&sd->bl,SkillStatusChangeTable[MO_STEELBODY],100,1,skill_get_time(MO_STEELBODY,1));
		return 0;
	}

	return damage;
}

//
// script? 連
//
/*==========================================
 * script用PCステ?タス?み出し
 *------------------------------------------
 */
int pc_readparam(struct map_session_data *sd,int type)
{
	int val=0;

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
		val= sd->status.class_;
		break;
	case SP_BASEJOB: //Base job, extracting upper type.
		val= pc_mapid2jobid(sd->class_&MAPID_UPPERMASK, sd->status.sex);
		break;
	case SP_UPPER:
		val= sd->class_&JOBL_UPPER?1:(sd->class_&JOBL_BABY?2:0);
		break;
	case SP_BASECLASS: //Extract base class tree. [Skotlex]
		val= pc_mapid2jobid(sd->class_&MAPID_BASEMASK, sd->status.sex);
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
 * script用PCステ?タス設定
 *------------------------------------------
 */
int pc_setparam(struct map_session_data *sd,int type,int val)
{
	int i = 0;

	nullpo_retr(0, sd);

	switch(type){
	case SP_BASELEVEL:
		if ((unsigned int)val > pc_maxbaselv(sd)) //Capping to max
			val = pc_maxbaselv(sd);
		if ((unsigned int)val > sd->status.base_level) {
			int stat=0;
			for (i = 1; i <= (int)((unsigned int)val - sd->status.base_level); i++)
				stat += (sd->status.base_level + i + 14) / 5 ;
			if (sd->status.status_point > USHRT_MAX - stat)
				
				sd->status.status_point = USHRT_MAX;
			else
				sd->status.status_point += stat;
		}
		sd->status.base_level = (unsigned int)val;
		sd->status.base_exp = 0;
		clif_updatestatus(sd, SP_BASELEVEL);
		clif_updatestatus(sd, SP_NEXTBASEEXP);
		clif_updatestatus(sd, SP_STATUSPOINT);
		clif_updatestatus(sd, SP_BASEEXP);
		status_calc_pc(sd, 0);
		pc_heal(sd, sd->status.max_hp, sd->status.max_sp);
		break;
	case SP_JOBLEVEL:
		if ((unsigned int)val >= sd->status.job_level) {
			if ((unsigned int)val > pc_maxjoblv(sd)) val = pc_maxjoblv(sd);
			if (sd->status.skill_point > USHRT_MAX - val + sd->status.job_level)
				sd->status.skill_point = USHRT_MAX;
			else
				sd->status.skill_point += val-sd->status.job_level;
			clif_updatestatus(sd, SP_SKILLPOINT);
			clif_misceffect(&sd->bl, 1);
		}
		sd->status.job_level = (unsigned int)val;
		sd->status.job_exp = 0;
		clif_updatestatus(sd, SP_JOBLEVEL);
		clif_updatestatus(sd, SP_NEXTJOBEXP);
		clif_updatestatus(sd, SP_JOBEXP);
		status_calc_pc(sd, 0);
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
			// MAX_ZENY 以下なら代入
			sd->status.zeny = val;
		} else {
			sd->status.zeny = MAX_ZENY;
			/* Could someone explain the comments below? I have no idea what they are trying to do... 
			 * if you want to give someone so much zeny, just set their zeny to the max. [Skotlex]
			if(sd->status.zeny > val) {
				// Zeny が減少しているなら代入
				sd->status.zeny = val;
			} else if(sd->status.zeny <= MAX_ZENY) {
				// Zeny が増加していて、現在の値がMAX_ZENY 以下ならMAX_ZENY
				sd->status.zeny = MAX_ZENY;
			} else {
				// Zeny が増加していて、現在の値がMAX_ZENY より下なら増加分を無視
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
 * HP/SP回復
 *------------------------------------------
 */
int pc_heal(struct map_session_data *sd,int hp,int sp)
{
//	if(battle_config.battle_log)
//		printf("heal %d %d\n",hp,sp);

	nullpo_retr(0, sd);

//Uneeded as the hp range adjustment below will auto-adap itself and make hp = max. [Skotlex]
//	if(hp > 0 && pc_checkoverhp(sd))
//		hp = 0;

//	if(sp > 0 && pc_checkoversp(sd))
//		sp = 0;

	if(hp > sd->status.max_hp - sd->status.hp)
		hp = sd->status.max_hp - sd->status.hp;
	sd->status.hp+=hp;
		
	if(sp > sd->status.max_sp - sd->status.sp)
		sp = sd->status.max_sp - sd->status.sp;
	sd->status.sp+=sp;

	if(sd->status.sp <= 0)
		sd->status.sp = 0;

	if(sd->status.hp <= 0) {
		sd->status.hp = 0;
		pc_damage(NULL,sd,1);
		hp = 0;
	}
	
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	if(sd->status.hp>=sd->status.max_hp>>2 && sd->sc.data[SC_AUTOBERSERK].timer != -1 &&
		(sd->sc.data[SC_PROVOKE].timer!=-1 && sd->sc.data[SC_PROVOKE].val2==1 ))
			status_change_end(&sd->bl,SC_PROVOKE,-1); //End auto berserk.

	return hp + sp;
}

/*==========================================
 * HP/SP回復
 *------------------------------------------
 */
int pc_itemheal(struct map_session_data *sd,int hp,int sp)
{
	int bonus, type;
//	if(battle_config.battle_log)
//		printf("heal %d %d\n",hp,sp);

	nullpo_retr(0, sd);

	if(hp > 0 && pc_checkoverhp(sd))
		hp = 0;

	if(sp > 0 && pc_checkoversp(sd))
		sp = 0;

	if(hp > 0) {
		bonus = (sd->paramc[2]<<1) + 100 + pc_checkskill(sd,SM_RECOVERY)*10
			+ pc_checkskill(sd,AM_LEARNINGPOTION)*5;
		// A potion produced by an Alchemist in the Fame Top 10 gets +50% effect [DracoRPG]
		bonus += (potion_flag==2)?50:(potion_flag==3?100:0);
		if ((type = itemdb_group(sd->itemid)) > 0 && type <= 7)
			bonus = bonus * (100+sd->itemhealrate[type - 1]) / 100;
		if(bonus != 100)
			hp = hp * bonus / 100;
	}
	if(sp > 0) {
		bonus = (sd->paramc[3]<<1) + 100 + pc_checkskill(sd,MG_SRECOVERY)*10
			+ pc_checkskill(sd,AM_LEARNINGPOTION)*5;
		bonus += (potion_flag==2)?50:(potion_flag==3?100:0);
		if(bonus != 100)
			sp = sp * bonus / 100;
	}
	if(hp > sd->status.max_hp - sd->status.hp)
		sd->status.hp = sd->status.max_hp;
	else
		sd->status.hp+=hp;
		
	if(sp > sd->status.max_sp - sd->status.sp)
		sd->status.sp = sd->status.max_sp;
	else
		sd->status.sp += sp;

	if(sd->status.hp <= 0) {
		sd->status.hp = 0;
		pc_damage(NULL,sd,1);
		hp = 0;
	}
	if(sd->status.sp <= 0)
		sd->status.sp = 0;
	
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	return 0;
}

/*==========================================
 * HP/SP回復
 *------------------------------------------
 */
int pc_percentheal(struct map_session_data *sd,int hp,int sp)
{
	nullpo_retr(0, sd);
/*	Shouldn't be needed, these functions are proof of bad coding xP
	if(pc_checkoverhp(sd)) {
		if(hp > 0)
			hp = 0;
	}
	if(pc_checkoversp(sd)) {
		if(sp > 0)
			sp = 0;
	}
*/
	if(hp) {
		if(hp >= 100)
			sd->status.hp = sd->status.max_hp;
		else if(hp <= -100) {
			sd->status.hp = 0;
			pc_damage(NULL,sd,1);
		}
		else if (hp > 0) {
			hp = sd->status.max_hp*hp/100;
			if (sd->status.max_hp - sd->status.hp < hp)
				sd->status.hp = sd->status.max_hp;
			else
				sd->status.hp += hp;
		}
		else { //hp < 0
			hp = sd->status.max_hp*hp/100;
			if (sd->status.hp <= -hp) {
				sd->status.hp = 0;
				pc_damage(NULL,sd,1);
			} else
				sd->status.hp += hp;
		}
	}
	if(sp) {
		if(sp >= 100)
			sd->status.sp = sd->status.max_sp;
		else if(sp <= -100)
			sd->status.sp = 0;
		else if(sp > 0) {
			sp = sd->status.max_sp*sp/100;
			if (sd->status.max_sp - sd->status.sp < sp)
				sd->status.sp = sd->status.max_sp;
			else
				sd->status.sp += sp;
		} else { //sp < 0
			sp = sd->status.max_sp*sp/100;
			if (sd->status.sp <= -sp)
				sd->status.sp = 0;
			else
				sd->status.sp += sp;
		}
	}
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	return 0;
}

/*==========================================
 * 職?更
 * 引?	job 職業 0〜23
 *		upper 通常 0, ?生 1, 養子 2, そのまま -1
 * Rewrote to make it tidider [Celest]
 *------------------------------------------
 */
int pc_jobchange(struct map_session_data *sd,int job, int upper)
{
	int i, fame_flag=0;
	int b_class;

	nullpo_retr(0, sd);

	if (job < 0)
		return 1;

	//Normalize job.
	b_class = pc_jobid2mapid(job);
	if (b_class == -1)
		return 1;
	switch (upper) {
		case 1:
			b_class|= JOBL_UPPER; 
			break;
		case 2:
			b_class|= JOBL_BABY;
			break;
	}
	//This will automatically adjust bard/dancer classes to the correct gender
	//That is, if you try to jobchange into dancer, it will turn you to bard.	
	job = pc_mapid2jobid(b_class, sd->status.sex);
	if (job == -1)
		return 1;
	
	if ((unsigned short)b_class == sd->class_)
		return 1; //Nothing to change.
	// check if we are changing from 1st to 2nd job
	if (b_class&JOBL_2) {
	  if (!(sd->class_&JOBL_2))
			sd->change_level = sd->status.job_level;
	  else if (!sd->change_level)
			sd->change_level = 40; //Assume 40?
	}

	pc_setglobalreg (sd, "jobchange_level", sd->change_level);

	sd->status.class_ = job;
	status_set_viewdata(&sd->bl, job);
	fame_flag = pc_istop10fame(sd->status.char_id,sd->class_&MAPID_UPPERMASK);
	sd->class_ = (unsigned short)b_class;
	sd->status.job_level=1;
	sd->status.job_exp=0;
	clif_updatestatus(sd,SP_JOBLEVEL);
	clif_updatestatus(sd,SP_JOBEXP);
	clif_updatestatus(sd,SP_NEXTJOBEXP);

	for(i=0;i<11;i++) {
		if(sd->equip_index[i] >= 0)
			if(!pc_isequip(sd,sd->equip_index[i]))
				pc_unequipitem(sd,sd->equip_index[i],2);	// ?備外し
	}

	clif_changelook(&sd->bl,LOOK_BASE,sd->vd.class_); // move sprite update to prevent client crashes with incompatible equipment [Valaris]

	if(sd->vd.cloth_color)
		clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->vd.cloth_color);
	
	if(battle_config.muting_players && sd->status.manner < 0 && battle_config.manner_system)
		clif_changestatus(&sd->bl,SP_MANNER,sd->status.manner);

	
	if(pc_isriding(sd)) //Remove Peco Status to prevent display <> class problems.
		pc_setoption(sd,sd->sc.option&~OPTION_RIDING);

	status_calc_pc(sd,0);
	pc_checkallowskill(sd);
	pc_equiplookall(sd);
	clif_equiplist(sd);

	//if you were previously famous, not anymore.
	if (fame_flag) {
		chrif_save(sd,0);
		chrif_reqfamelist();
	} else if (sd->status.fame > 0) {
		//It may be that now they are famous?
 		switch (sd->class_&MAPID_UPPERMASK) {
			case MAPID_BLACKSMITH:
			case MAPID_ALCHEMIST:
			case MAPID_TAEKWON:
				chrif_save(sd,0);
				chrif_reqfamelist();
			break;
		}
	}

	return 0;
}

/*==========================================
 * 見た目?更
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
 * 見た目?更
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
 * 付?品(鷹,ペコ,カ?ト)設定
 *------------------------------------------
 */
int pc_setoption(struct map_session_data *sd,int type)
{
	nullpo_retr(0, sd);
	if (type&OPTION_RIDING && !(sd->sc.option&OPTION_RIDING) && (sd->class_&MAPID_BASEMASK) == MAPID_SWORDMAN)
	{	//We are going to mount. [Skotlex]
		switch (sd->status.class_)
		{
			case JOB_KNIGHT:
				clif_changelook(&sd->bl,LOOK_BASE,JOB_KNIGHT2);
				break;
			case JOB_CRUSADER:
				clif_changelook(&sd->bl,LOOK_BASE,JOB_CRUSADER2);
				break;
			case JOB_LORD_KNIGHT:
				clif_changelook(&sd->bl,LOOK_BASE,JOB_LORD_KNIGHT2);
				break;
			case JOB_PALADIN:
				clif_changelook(&sd->bl,LOOK_BASE,JOB_PALADIN2);
				break;
			case JOB_BABY_KNIGHT:
				clif_changelook(&sd->bl,LOOK_BASE,JOB_BABY_KNIGHT2);
				break;
			case JOB_BABY_CRUSADER:
				clif_changelook(&sd->bl,LOOK_BASE,JOB_BABY_CRUSADER2);
				break;
		}
		clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->vd.cloth_color);
		clif_status_load(&sd->bl,SI_RIDING,1);
		status_calc_pc(sd,0); //Mounting/Umounting affects walk and attack speeds.
	}
	else if (!(type&OPTION_RIDING) && sd->sc.option&OPTION_RIDING && (sd->class_&MAPID_BASEMASK) == MAPID_SWORDMAN)
	{	//We are going to dismount.
		if (sd->vd.class_ != sd->status.class_) {
			clif_changelook(&sd->bl,LOOK_BASE,sd->status.class_);
			clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->vd.cloth_color);
		}
		clif_status_load(&sd->bl,SI_RIDING,0);
		status_calc_pc(sd,0); //Mounting/Umounting affects walk and attack speeds.
	}
	if (type&OPTION_FALCON && !(sd->sc.option&OPTION_FALCON)) //Falcon ON
		clif_status_load(&sd->bl,SI_FALCON,1);
	else if (!(type&OPTION_FALCON) && sd->sc.option&OPTION_FALCON) //Falcon OFF
		clif_status_load(&sd->bl,SI_FALCON,0);

	//SG flying [Komurka]
	if (type&OPTION_FLYING && !(sd->sc.option&OPTION_FLYING)) //Flying ON
	{
		if (sd->status.class_==JOB_STAR_GLADIATOR)
			clif_changelook(&sd->bl,LOOK_BASE,JOB_STAR_GLADIATOR2);
	}
	else if (!(type&OPTION_FLYING) && sd->sc.option&OPTION_FLYING) //Flying OFF
		if (sd->vd.class_ != sd->status.class_)
			clif_changelook(&sd->bl,LOOK_BASE,sd->status.class_);

	sd->sc.option=type;
	clif_changeoption(&sd->bl);
	status_calc_pc(sd,0);
	return 0;
}

/*==========================================
 * カ?ト設定
 *------------------------------------------
 */
int pc_setcart(struct map_session_data *sd,int type)
{
	int cart[6]={0x0000,OPTION_CART1,OPTION_CART2,OPTION_CART3,OPTION_CART4,OPTION_CART5};
	int option;
	nullpo_retr(0, sd);
	
	if (type < 0 || type > 5)
		return 0; //Never trust the values sent by the client! [Skotlex]

	if(pc_checkskill(sd,MC_PUSHCART)>0){ // プッシュカ?トスキル所持
		option = sd->sc.option;
		//This should preserve the current option, only modifying the cart bit.
		option&=~(OPTION_CART1|OPTION_CART2|OPTION_CART3|OPTION_CART4|OPTION_CART5);
		option|=cart[type];
		if(!pc_iscarton(sd)){ // カ?トを付けていない
			pc_setoption(sd,option);
			clif_cart_itemlist(sd);
			clif_cart_equiplist(sd);
			clif_updatestatus(sd,SP_CARTINFO);
			clif_status_change(&sd->bl,SI_INCREASEAGI,0); //0x0c is 12, Increase Agi??
		}
		else{
			pc_setoption(sd,option);
		}
	}

	return 0;
}

/*==========================================
 * 鷹設定
 *------------------------------------------
 */
int pc_setfalcon(struct map_session_data *sd)
{
	if(pc_checkskill(sd,HT_FALCON)>0){	// ファルコンマスタリ?スキル所持
		pc_setoption(sd,sd->sc.option|OPTION_FALCON);
	}

	return 0;
}

/*==========================================
 * ペコペコ設定
 *------------------------------------------
 */
int pc_setriding(struct map_session_data *sd)
{
	if((pc_checkskill(sd,KN_RIDING)>0)){ // ライディングスキル所持
		pc_setoption(sd,sd->sc.option|OPTION_RIDING);
	}
	return 0;
}

/*==========================================
 * アイテムドロップ可不可判定
 *------------------------------------------
 */
int pc_candrop(struct map_session_data *sd,int item_id)
{
	int level = pc_isGM(sd);
	if ( pc_can_give_items(level) ) //check if this GM level can drop items
		return 0;
	return (itemdb_isdropable(item_id, level));
}

/*==========================================
 * script用??の値を?む
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
 * script用??の値を設定
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
 * script用文字列??の値を?む
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
 * script用文字列??の値を設定
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

int pc_readregistry(struct map_session_data *sd,char *reg,int type) {
	struct global_reg *sd_reg;
	int i,max;
	
	nullpo_retr(0, sd);
	switch (type) {
	case 3: //Char reg
		sd_reg = sd->save_reg.global;
		max = sd->save_reg.global_num;
	break;
	case 2: //Account reg
		sd_reg = sd->save_reg.account;
		max = sd->save_reg.account_num;
	break;
	case 1: //Account2 reg
		sd_reg = sd->save_reg.account2;
		max = sd->save_reg.account2_num;
	break;
	default:
		return 0;
	}
	if (max == -1) {
		if (battle_config.error_log)
			ShowError("pc_readregistry: Trying to read reg value %s (type %d) before it's been loaded!\n", reg, type);
		//This really shouldn't happen, so it's possible the data was lost somewhere, we should request it again.
		intif_request_registry(sd,type==3?4:type);
		return 0;
	}
	for(i=0;i<max;i++){
		if(strcmp(sd_reg[i].str,reg)==0)
			return atoi(sd_reg[i].value);
	}
	return 0;
}

char* pc_readregistry_str(struct map_session_data *sd,char *reg,int type) {
	struct global_reg *sd_reg;
	int i,max;
	
	nullpo_retr(0, sd);
	switch (type) {
	case 3: //Char reg
		sd_reg = sd->save_reg.global;
		max = sd->save_reg.global_num;
	break;
	case 2: //Account reg
		sd_reg = sd->save_reg.account;
		max = sd->save_reg.account_num;
	break;
	case 1: //Account2 reg
		sd_reg = sd->save_reg.account2;
		max = sd->save_reg.account2_num;
	break;
	default:
		return NULL;
	}
	if (max == -1) {
		if (battle_config.error_log)
			ShowError("pc_readregistry: Trying to read reg value %s (type %d) before it's been loaded!\n", reg, type);
		//This really shouldn't happen, so it's possible the data was lost somewhere, we should request it again.
		intif_request_registry(sd,type==3?4:type);
		return NULL;
	}
	for(i=0;i<max;i++){
		if(strcmp(sd_reg[i].str,reg)==0)
			return sd_reg[i].value;
	}
	return NULL;
}

int pc_setregistry(struct map_session_data *sd,char *reg,int val,int type) {
	struct global_reg *sd_reg;
	int i,*max, regmax;

	nullpo_retr(0, sd);
	if (type == 3) { //Some special character reg values...
		if(strcmp(reg,"PC_DIE_COUNTER") == 0 && sd->die_counter != val){
			sd->die_counter = val;
	//		status_calc_pc(sd,0); //I doubt this is needed....
		} else if(strcmp(reg,script_config.die_event_name) == 0){
			sd->state.event_death = val;
		} else if(strcmp(reg,script_config.kill_pc_event_name) == 0){
			sd->state.event_kill_pc = val;
		} else if(strcmp(reg,script_config.kill_mob_event_name) == 0){
			sd->state.event_kill_mob = val;
		} else if(strcmp(reg,script_config.logout_event_name) == 0){
			sd->state.event_disconnect = val;
		}
	}
	switch (type) {
	case 3: //Char reg
		sd_reg = sd->save_reg.global;
		max = &sd->save_reg.global_num;
		regmax = GLOBAL_REG_NUM;
	break;
	case 2: //Account reg
		sd_reg = sd->save_reg.account;
		max = &sd->save_reg.account_num;
		regmax = ACCOUNT_REG_NUM;
	break;
	case 1: //Account2 reg
		sd_reg = sd->save_reg.account2;
		max = &sd->save_reg.account2_num;
		regmax = ACCOUNT_REG2_NUM;
	break;
	default:
		return 0;
	}
	if (*max == -1) {
		if(battle_config.error_log)
			ShowError("pc_setregistry : refusing to set %s (type %d) until vars are received.\n", reg, type);
		return 1;
	}
	
	// delete reg
	if (val == 0) {
		for(i = 0; i < *max; i++) {
			if (strcmp(sd_reg[i].str, reg) == 0) {
				if (i != *max - 1)
					memcpy(&sd_reg[i], &sd_reg[*max - 1], sizeof(struct global_reg));
				memset(&sd_reg[*max - 1], 0, sizeof(struct global_reg));
				(*max)--;
				sd->state.reg_dirty |= 1<<(type-1); //Mark this registry as "need to be saved"
				break;
			}
		}
		return 0;
	}
	// change value if found
	for(i = 0; i < *max; i++) {
		if (strcmp(sd_reg[i].str, reg) == 0) {
			sprintf(sd_reg[i].value, "%d", val); 
			sd->state.reg_dirty |= 1<<(type-1);
			return 0;
		}
	}

	// add value if not found
	if (i < regmax) {
		memset(&sd_reg[i], 0, sizeof(struct global_reg));
		strncpy(sd_reg[i].str, reg, 32);
		sprintf(sd_reg[i].value, "%d", val); 
		(*max)++;
		sd->state.reg_dirty |= 1<<(type-1);
		return 0;
	}

	if(battle_config.error_log)
		ShowError("pc_setregistry : couldn't set %s, limit of registries reached (%d)\n", reg, regmax);

	return 1;
}

int pc_setregistry_str(struct map_session_data *sd,char *reg,char *val,int type) {
	struct global_reg *sd_reg;
	int i,*max, regmax;

	nullpo_retr(0, sd);
	if (reg[strlen(reg)-1] != '$') {
		if(battle_config.error_log)
			ShowError("pc_setregistry_str : reg %s must be string (end in '$') to use this!\n", reg);
		return 1;
	}

	switch (type) {
	case 3: //Char reg
		sd_reg = sd->save_reg.global;
		max = &sd->save_reg.global_num;
		regmax = GLOBAL_REG_NUM;
	break;
	case 2: //Account reg
		sd_reg = sd->save_reg.account;
		max = &sd->save_reg.account_num;
		regmax = ACCOUNT_REG_NUM;
	break;
	case 1: //Account2 reg
		sd_reg = sd->save_reg.account2;
		max = &sd->save_reg.account2_num;
		regmax = ACCOUNT_REG2_NUM;
	break;
	default:
		return 0;
	}
	if (*max == -1) {
		if(battle_config.error_log)
			ShowError("pc_setregistry_str : refusing to set %s (type %d) until vars are received.\n", reg, type);
		return 1;
	}
	
	// delete reg
	if (strcmp(val,"")==0) {
		for(i = 0; i < *max; i++) {
			if (strcmp(sd_reg[i].str, reg) == 0) {
				if (i != *max - 1)
					memcpy(&sd_reg[i], &sd_reg[*max - 1], sizeof(struct global_reg));
				memset(&sd_reg[*max - 1], 0, sizeof(struct global_reg));
				(*max)--;
				sd->state.reg_dirty |= 1<<(type-1); //Mark this registry as "need to be saved"
				if (type!=3) intif_saveregistry(sd,type);
				break;
			}
		}
		return 0;
	}
	// change value if found
	for(i = 0; i < *max; i++) {
		if (strcmp(sd_reg[i].str, reg) == 0) {
			strncpy(sd_reg[i].value, val, 256);
			sd->state.reg_dirty |= 1<<(type-1); //Mark this registry as "need to be saved"
			if (type!=3) intif_saveregistry(sd,type);
			return 0;
		}
	}

	// add value if not found
	if (i < regmax) {
		memset(&sd_reg[i], 0, sizeof(struct global_reg));
		strncpy(sd_reg[i].str, reg, 32);
		strncpy(sd_reg[i].value, val, 256);
		(*max)++;
		sd->state.reg_dirty |= 1<<(type-1); //Mark this registry as "need to be saved"
		if (type!=3) intif_saveregistry(sd,type);
		return 0;
	}

	if(battle_config.error_log)
		ShowError("pc_setregistry : couldn't set %s, limit of registries reached (%d)\n", reg, regmax);

	return 1;
}

/*==========================================
 * イベントタイマ??理
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
 * イベントタイマ?追加
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
 * イベントタイマ?削除
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
 * イベントタイマ?カウント値追加
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
 * イベントタイマ?全削除
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
// ? 備物
//
/*==========================================
 * アイテムを?備する
 *------------------------------------------
 */
int pc_equipitem(struct map_session_data *sd,int n,int pos)
{
	int i,nameid, arrow;
	struct item_data *id;
	//?生や養子の場合の元の職業を算出する

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
	if(sd->sc.count && sd->sc.data[SC_BERSERK].timer!=-1){
		clif_equipitemack(sd,n,0,0);	// fail
		return 0;
	}

	if(pos==0x88){ // アクセサリ用例外?理
		int epor=0;
		if(sd->equip_index[0] >= 0)
			epor |= sd->status.inventory[sd->equip_index[0]].equip;
		if(sd->equip_index[1] >= 0)
			epor |= sd->status.inventory[sd->equip_index[1]].equip;
		epor &= 0x88;
		pos = epor == 0x08 ? 0x80 : 0x08;
	}

	// 二刀流?理
	if ((pos==0x22) // 一?、?備要求箇所が二刀流武器かチェックする
	 &&	(id->equip==2)	// ? 手武器
	 &&	(pc_checkskill(sd, AS_LEFT) > 0 || (sd->class_&MAPID_UPPERMASK) == MAPID_ASSASSIN) ) // 左手修?有
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
	// 弓矢?備
	if(pos==0x8000){
		clif_arrowequip(sd,n);
		clif_arrow_fail(sd,3);	// 3=矢が?備できました
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

	pc_checkallowskill(sd);	// ?備品でスキルか解除されるかチェック
	if (itemdb_look(sd->status.inventory[n].nameid) == 11 && (arrow >= 0)){	// Added by RoVeRT
		clif_arrowequip(sd,arrow);
		sd->status.inventory[arrow].equip=32768;
	}
	status_calc_pc(sd,0);
	//OnEquip script [Skotlex]
	if (sd->inventory_data[n]) {
		int i;
		struct item_data *data;
		if (sd->inventory_data[n]->equip_script)
			run_script(sd->inventory_data[n]->equip_script,0,sd->bl.id,0);
		if(sd->status.inventory[n].card[0]==0x00ff ||
			sd->status.inventory[n].card[0]==0x00fe ||
			sd->status.inventory[n].card[0]==(short)0xff00)
			; //No cards
		else
		for(i=0;i<sd->inventory_data[n]->slot; i++)
		{
			if (!sd->status.inventory[n].card[i])
				continue;
			data = itemdb_exists(sd->status.inventory[n].card[i]);
			if (data && data->equip_script)
				run_script(data->equip_script,0,sd->bl.id,0);
		}
	}
	return 0;
}

/*==========================================
 * ? 備した物を外す
 * type:
 * 0 - only unequip
 * 1 - calculate status after unequipping
 * 2 - force unequip
 *------------------------------------------
 */
int pc_unequipitem(struct map_session_data *sd,int n,int flag)
{
	int i;
	nullpo_retr(0, sd);

// -- moonsoul	(if player is berserk then cannot unequip)
//
	if(!(flag&2) && sd->sc.count && (sd->sc.data[SC_BLADESTOP].timer!=-1 || sd->sc.data[SC_BERSERK].timer!=-1)){
		clif_unequipitemack(sd,n,0,0);
		return 0;
	}

	if(battle_config.battle_log)
		ShowInfo("unequip %d %x:%x\n",n,pc_equippoint(sd,n),sd->status.inventory[n].equip);

	if(!sd->status.inventory[n].equip){ //Nothing to unequip
		clif_unequipitemack(sd,n,0,0);
		return 0;
	}
	for(i=0;i<11;i++) {
		if(sd->status.inventory[n].equip & equip_pos[i])
			sd->equip_index[i] = -1;
	}

	if(sd->status.inventory[n].equip & 0x0002) {
		sd->weapontype1 = 0;
		sd->status.weapon = sd->weapontype2;
		pc_calcweapontype(sd);
		clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
		if(sd->sc.data[SC_DANCING].timer!=-1) //When unequipping, stop dancing. [Skotlex]
			skill_stop_dancing(&sd->bl);
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
		skill_enchant_elemental_end(&sd->bl,-1);  //武器持ち誓えは無?件で?性付?解除
	if(flag&1)
		status_calc_pc(sd,0);

	if(sd->sc.count && sd->sc.data[SC_SIGNUMCRUCIS].timer != -1 && !battle_check_undead(RC_DEMIHUMAN,sd->def_ele))
		status_change_end(&sd->bl,SC_SIGNUMCRUCIS,-1);

	//OnUnEquip script [Skotlex]
	if (sd->inventory_data[n]) {
		struct item_data *data;
		if (sd->inventory_data[n]->unequip_script)
			run_script(sd->inventory_data[n]->unequip_script,0,sd->bl.id,0);
		if(sd->status.inventory[n].card[0]==0x00ff ||
			sd->status.inventory[n].card[0]==0x00fe ||
			sd->status.inventory[n].card[0]==(short)0xff00)
			; //No cards
		else
		for(i=0;i<sd->inventory_data[n]->slot; i++)
		{
			if (!sd->status.inventory[n].card[i])
				continue;
			data = itemdb_exists(sd->status.inventory[n].card[i]);
			if (data && data->unequip_script)
				run_script(data->unequip_script,0,sd->bl.id,0);
		}
	}

	return 0;
}

/*==========================================
 * アイテムのindex番?を詰めたり
 * ? 備品の?備可能チェックを行なう
 *------------------------------------------
 */
int pc_checkitem(struct map_session_data *sd)
{
	int i,j,k,id,calc_flag = 0;
	struct item_data *it=NULL;

	nullpo_retr(0, sd);

	if (sd->vender_id) //Avoid reorganizing items when we are vending, as that leads to exploits (pointed out by End of Exam)
		return 0;
	
	// 所持品空き詰め
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

	// カ?ト?空き詰め
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

	// ? 備位置チェック

	for(i=0;i<MAX_INVENTORY;i++){

		it=sd->inventory_data[i];

		if(sd->status.inventory[i].nameid==0)
			continue;
		if(sd->status.inventory[i].equip & ~pc_equippoint(sd,i)) {
			sd->status.inventory[i].equip=0;
			calc_flag = 1;
		}
		//?備制限チェック
		if(sd->status.inventory[i].equip && it) {
			if (map[sd->bl.m].flag.pvp && it->flag.no_equip&1)
			{  //PVP check for forbiden items. optimized by [Lupus]
				sd->status.inventory[i].equip=0;
				calc_flag = 1;
			} else
			if (map_flag_gvg(sd->bl.m) && it->flag.no_equip&2)
			{  //GvG optimized by [Lupus]
				sd->status.inventory[i].equip=0;
				calc_flag = 1;
			} else
			if(map[sd->bl.m].flag.restricted && it->flag.no_equip&map[sd->bl.m].zone)
			{ // Restricted zone by [Komurka]
				sd->status.inventory[i].equip=0;
				calc_flag = 1;
			}
			if (!calc_flag) { //Check cards
				int flag;
				flag = (map[sd->bl.m].flag.restricted?map[sd->bl.m].zone:0)
					| (map[sd->bl.m].flag.pvp?1:0)
					| (map_flag_gvg(sd->bl.m)?2:0);
				if (flag && !pc_isAllowedCardOn(sd,it->slot,i,flag))
					calc_flag = 1;
			}
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
 * PVP順位計算用(foreachinarea)
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
 * PVP順位計算
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
	map_foreachinmap(pc_calc_pvprank_sub,sd->bl.m,BL_PC,sd);
	if(old!=sd->pvp_rank || sd->pvp_lastusers!=m->users)
		clif_pvpset(sd,sd->pvp_rank,sd->pvp_lastusers=m->users,0);
	return sd->pvp_rank;
}
/*==========================================
 * PVP順位計算(timer)
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
 * sdは結婚しているか(?婚の場合は相方のchar_idを返す)
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
 * sdがdstsdと結婚(dstsd→sdの結婚?理も同暫ﾉ行う)
 *------------------------------------------
 */
int pc_marriage(struct map_session_data *sd,struct map_session_data *dstsd)
{
	if(sd == NULL || dstsd == NULL ||
		sd->status.partner_id > 0 || dstsd->status.partner_id > 0 ||
		sd->class_&JOBL_BABY)
		return -1;
	sd->status.partner_id = dstsd->status.char_id;
	dstsd->status.partner_id = sd->status.char_id;
	return 0;
}

/*==========================================
 * sdが離婚(相手はsd->status.partner_idに依る)(相手も同暫ﾉ離婚?結婚指輪自動?奪)
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
		for (i = 0; i < MAX_INVENTORY; i++) {
			if (sd->status.inventory[i].nameid == WEDDING_RING_M || sd->status.inventory[i].nameid == WEDDING_RING_F)
				pc_delitem(sd, i, 1, 0);
			if (p_sd->status.inventory[i].nameid == WEDDING_RING_M || p_sd->status.inventory[i].nameid == WEDDING_RING_F)
				pc_delitem(p_sd, i, 1, 0);
		}
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
 * sdの相方のmap_session_dataを返す
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
	if (sd && sd->class_&JOBL_BABY && sd->status.father > 0)
		// charid2sd returns NULL if not found
		return map_charid2sd(sd->status.father);

	return NULL;
}

struct map_session_data *pc_get_mother (struct map_session_data *sd)
{
	if (sd && sd->class_&JOBL_BABY && sd->status.mother > 0)
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
// 自然回復物
//
/*==========================================
 * SP回復量計算
 *------------------------------------------
 */
static unsigned int natural_heal_prev_tick,natural_heal_diff_tick;
static int pc_spheal(struct map_session_data *sd)
{
	int a = natural_heal_diff_tick;
	
	nullpo_retr(0, sd);
	
	if(pc_issit(sd))
		a += a;
	if (sd->sc.count) {
		if (sd->sc.data[SC_MAGNIFICAT].timer!=-1)	// マグニフィカ?ト
			a += a;
		if (sd->sc.data[SC_REGENERATION].timer != -1)
			a *= sd->sc.data[SC_REGENERATION].val1;
	}
	// Re-added back to status_calc
	//if((skill = pc_checkskill(sd,HP_MEDITATIO)) > 0) //Increase natural SP regen with Meditatio [DracoRPG]
		//a += a*skill*3/100;
	
	if (sd->status.guild_id > 0) {
		struct guild_castle *gc = guild_mapindex2gc(sd->mapindex);	// Increased guild castle regen [Valaris]
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
 * HP回復量計算
 *------------------------------------------
 */
static int pc_hpheal(struct map_session_data *sd)
{
	int a = natural_heal_diff_tick;

	nullpo_retr(0, sd);

	if(pc_issit(sd))
		a += a;
	if (sd->sc.count) {
		if (sd->sc.data[SC_MAGNIFICAT].timer != -1)	// Modified by RoVeRT
			a += a;
		if (sd->sc.data[SC_REGENERATION].timer != -1)
			a *= sd->sc.data[SC_REGENERATION].val1;
	}
	if (sd->status.guild_id > 0) {
		struct guild_castle *gc = guild_mapindex2gc(sd->mapindex);	// Increased guild castle regen [Valaris]
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

	if (sd->no_regen & 1)
		return 0;

	if(pc_checkoverhp(sd)) {
		sd->hp_sub = sd->inchealhptick = 0;
		return 0;
	}

	bhp=sd->status.hp;
	hp_flag = (pc_checkskill(sd,SM_MOVINGRECOVERY) > 0 && sd->ud.walktimer != -1);

	if(sd->ud.walktimer == -1) {
		inc_num = pc_hpheal(sd);
		if(sd->sc.data[SC_TENSIONRELAX].timer!=-1 ){	// テンションリラックス
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
			bonus = sd->nshealhp;
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

	if (sd->no_regen & 2)
		return 0;

	if(pc_checkoversp(sd)) {
		sd->sp_sub = sd->inchealsptick = 0;
		return 0;
	}

	bsp=sd->status.sp;

	inc_num = pc_spheal(sd);
	if(sd->sc.data[SC_EXPLOSIONSPIRITS].timer == -1 || (sd->sc.data[SC_SPIRIT].timer!=-1 && sd->sc.data[SC_SPIRIT].val2 == SL_MONK))
		sd->sp_sub += inc_num;
	if(sd->ud.walktimer == -1)
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
			if(sd->doridori_counter && (sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE) {
				bonus = sd->nshealsp*2;
				sd->doridori_counter = 0;
			} else
				bonus = sd->nshealsp;
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
		if(sd->doridori_counter && pc_checkskill(sd,TK_HPTIME) > 0) {
		  	//TK_HPTIME doridori provided bonus [Dralnu]
			bonus_hp += sd->nsshealhp;
			if (!sd->nsshealsp) //If there's sp regen, this gets clear in the next function. [Skotlex]
				sd->doridori_counter = 0;
		}
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
		if(sd->doridori_counter && pc_checkskill(sd,TK_SPTIME) > 0) {
			//TK_SPTIME doridori provided bonus [Dralnu]
			bonus_sp += sd->nsshealsp;
			sd->doridori_counter = 0;
		}
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
 * HP/SP 自然回復 各クライアント
 *------------------------------------------
 */

static int pc_natural_heal_sub(struct map_session_data *sd,va_list ap) {
	int tick;

	nullpo_retr(0, sd);
	tick = va_arg(ap,int);

// -- moonsoul (if conditions below altered to disallow natural healing if under berserk status)
	if (pc_isdead(sd) || pc_ishiding(sd) ||
	//-- cannot regen for 5 minutes after using Berserk --- [Celest]
		(sd->sc.count && (
			(sd->sc.data[SC_POISON].timer != -1 && sd->sc.data[SC_SLOWPOISON].timer == -1) ||
			(sd->sc.data[SC_DPOISON].timer != -1 && sd->sc.data[SC_SLOWPOISON].timer == -1) ||
			sd->sc.data[SC_BERSERK].timer != -1 ||
			sd->sc.data[SC_TRICKDEAD].timer != -1 ||
			sd->sc.data[SC_BLEEDING].timer != -1
		))
	) { //Cannot heal neither natural or special.
		sd->hp_sub = sd->inchealhptick = sd->inchealspirithptick = 0;
		sd->sp_sub = sd->inchealsptick = sd->inchealspiritsptick = 0;
	} else {
		if (DIFF_TICK (tick, sd->canregen_tick)<0 ||
			sd->weight*100/sd->max_weight >= battle_config.natural_heal_weight_rate) { //Cannot heal natural HP/SP
			sd->hp_sub = sd->inchealhptick = 0;
			sd->sp_sub = sd->inchealsptick = 0;
		} else { //natural heal
			pc_natural_heal_hp(sd);
			if(sd->sc.count && (
				sd->sc.data[SC_EXTREMITYFIST].timer != -1 ||
				sd->sc.data[SC_DANCING].timer != -1
			))	//No SP natural heal.
				sd->sp_sub = sd->inchealsptick = 0;
			else
				pc_natural_heal_sp(sd);
			sd->canregen_tick = tick;
		}
		//Sitting Healing
		if (sd->nsshealhp)
			pc_spirit_heal_hp(sd);
		if (sd->nsshealsp)
			pc_spirit_heal_sp(sd);
	}
	if (sd->hp_loss_value > 0 || sd->sp_loss_value > 0)
		pc_bleeding(sd);
	else
		sd->hp_loss_tick = sd->sp_loss_tick = 0;

	return 0;
}

/*==========================================
 * HP/SP自然回復 (interval timer??)
 *------------------------------------------
 */
int pc_natural_heal(int tid,unsigned int tick,int id,int data)
{
	natural_heal_diff_tick = DIFF_TICK(tick,natural_heal_prev_tick);
	clif_foreachclient(pc_natural_heal_sub, tick);

	natural_heal_prev_tick = tick;
	return 0;
}

/*==========================================
 * セ?ブポイントの保存
 *------------------------------------------
 */
int pc_setsavepoint(struct map_session_data *sd, short mapindex,int x,int y)
{
	nullpo_retr(0, sd);

	sd->status.save_point.map = mapindex;
	sd->status.save_point.x = x;
	sd->status.save_point.y = y;

	return 0;
}

/*==========================================
 * 自動セ?ブ 各クライアント
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

		chrif_save(sd,0);
		save_flag=1;
		last_save_fd = sd->fd;
	}

	return 0;
}

/*==========================================
 * 自動セ?ブ (timer??)
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
	int i = 0;
	RFIFOHEAD(fd);
	if (gm_account != NULL)
		aFree(gm_account);
	GM_num = 0;
	gm_account = (struct gm_account *) aMallocA(((RFIFOW(fd,2) - 4) / 5)*sizeof(struct gm_account));
	for (i = 4; i < RFIFOW(fd,2); i += 5) {
		gm_account[GM_num].account_id = RFIFOL(fd,i);
		gm_account[GM_num].level = (int)RFIFOB(fd,i+4);
		//printf("GM account: %d -> level %d\n", gm_account[GM_num].account_id, gm_account[GM_num].level);
		GM_num++;
	}
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
	struct map_session_data *pl_sd;

	if (data == 0 && battle_config.day_duration <= 0)	// if we want a day
		return 0;
	
	if (night_flag != 0) {
		int i;
		night_flag = 0; // 0=day, 1=night [Yor]

		for(i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = (struct map_session_data *) session[i]->session_data) && pl_sd->state.auth && pl_sd->fd)
			{
				if (pl_sd->state.night) {
					clif_status_load(&pl_sd->bl, SI_NIGHT, 0); //New night effect by dynamix [Skotlex]
					pl_sd->state.night = 0;
				}
			}
		}

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
			if (session[i] && (pl_sd = (struct map_session_data *) session[i]->session_data) && pl_sd->state.auth && pl_sd->fd)
			{
				if (!pl_sd->state.night && map[pl_sd->bl.m].flag.nightenabled) {
					clif_status_load(&pl_sd->bl, SI_NIGHT, 1); //New night effect by dynamix [Skotlex]
					pl_sd->state.night = 1;
				}
			}
		}
		strcpy(tmp_soutput, (data == 0) ? msg_txt(503) : msg_txt(59)); // The night has fallen...
		intif_GMmessage(tmp_soutput, strlen(tmp_soutput) + 1, 0);
	}

	return 0;
}

void pc_setstand(struct map_session_data *sd){
	nullpo_retv(sd);

	if(sd->sc.count && sd->sc.data[SC_TENSIONRELAX].timer!=-1)
		status_change_end(&sd->bl,SC_TENSIONRELAX,-1);

	sd->state.dead_sit = sd->vd.dead_sit = 0;
}

int pc_split_str(char *str,char **val,int num)
{
	int i;

	for (i=0; i<num && str; i++){
		val[i] = str;
		str = strchr(str,',');
		if (str && i<num-1) //Do not remove a trailing comma.
			*str++=0;
	}
	return i;
}

int pc_split_atoi(char *str,int *val, char sep, int max)
{
	int i,j;
	for (i=0; i<max; i++) {
		if (!str) break;
		val[i] = atoi(str);
		str = strchr(str,sep);
		if (str)
			*str++=0;
	}
	//Zero up the remaining.
	for(j=i; j < max; j++)
		val[j] = 0;
	return i;
}

int pc_split_atoui(char *str,unsigned int *val, char sep, int max)
{
	static int warning=0;
	int i,j;
	double f;
	for (i=0; i<max; i++) {
		if (!str) break;
		f = atof(str);
		if (f < 0)
			val[i] = 0;
		else if (f > UINT_MAX) {
			val[i] = UINT_MAX;
			if (!warning) {
				warning = 1;
				ShowWarning("pc_readdb (exp.txt): Required exp per level is capped to %u\n", UINT_MAX);
			}
		} else
			val[i] = (unsigned int)f;
		str = strchr(str,sep);
		if (str)
			*str++=0;
	}
	//Zero up the remaining.
	for(j=i; j < max; j++)
		val[j] = 0;
	return i;
}

//
// 初期化物
//
/*==========================================
 * 設定ファイル?み?む
 * exp.txt 必要??値
 * job_db1.txt 重量,hp,sp,攻?速度
 * job_db2.txt job能力値ボ?ナス
 * skill_tree.txt 各職?のスキルツリ?
 * attr_fix.txt ?性修正テ?ブル
 * size_fix.txt サイズ補正テ?ブル
 * refine_db.txt 精?デ?タテ?ブル
 *------------------------------------------
 */
int pc_readdb(void)
{
	int i,j,k;
	FILE *fp;
	char line[24000],*p;

	// 必要??値?み?み
	memset(exp_table,0,sizeof(exp_table));
	memset(max_level,0,sizeof(max_level));
	sprintf(line, "%s/exp.txt", db_path);
	fp=fopen(line, "r");
	if(fp==NULL){
		ShowError("can't read %s\n", line);
		return 1;
	}
	while(fgets(line, sizeof(line)-1, fp)){
		int jobs[MAX_PC_CLASS], job_count, job;
		int type;
		unsigned int max;
		char *split[4];
		if(line[0]=='/' && line[1]=='/')
			continue;
		if (pc_split_str(line,split,4) < 4)
			continue;
		
		job_count = pc_split_atoi(split[1],jobs,':',MAX_PC_CLASS);
		if (job_count < 1)
			continue;
		job = jobs[0];
		if (!pcdb_checkid(job)) {
			ShowError("pc_readdb: Invalid job ID %d.\n", job);
			continue;
		}
		type = atoi(split[2]);
		if (type < 0 || type > 1) {
			ShowError("pc_readdb: Invalid type %d (must be 0 for base levels, 1 for job levels).\n", type);
			continue;
		}
		max = atoi(split[0]);
		if (max > MAX_LEVEL) {
			ShowWarning("pc_readdb: Specified max level %d for job %d is beyond server's limit (%d).\n ", max, job, MAX_LEVEL);
			max = MAX_LEVEL;
		}
		//We send one less and then one more because the last entry in the exp array should hold 0.
		max_level[job][type] = pc_split_atoui(split[3], exp_table[job][type],',',max-1)+1;
		//Reverse check in case the array has a bunch of trailing zeros... [Skotlex]
		//The reasoning behind the -2 is this... if the max level is 5, then the array
		//should look like this:
	   //0: x, 1: x, 2: x: 3: x 4: 0 <- last valid value is at 3.
		while ((i = max_level[job][type]-2) >= 0 && exp_table[job][type][i] <= 0)
			max_level[job][type]--;
		if (max_level[job][type] < max) {
			ShowWarning("pc_readdb: Specified max %d for job %d, but that job's exp table only goes up to level %d.\n", max, job, max_level[job][type]);
			ShowNotice("(You may still reach lv %d through scripts/gm-commands)\n");
			max_level[job][type] = max;
		}
//		ShowDebug("%s - Class %d: %d\n", type?"Job":"Base", job, max_level[job][type]);
		for (i = 1; i < job_count; i++) {
			job = jobs[i];
			if (!pcdb_checkid(job)) {
				ShowError("pc_readdb: Invalid job ID %d.\n", job);
				continue;
			}
			memcpy(exp_table[job][type], exp_table[jobs[0]][type], sizeof(exp_table[0][0]));
			max_level[job][type] = max;
//			ShowDebug("%s - Class %d: %d\n", type?"Job":"Base", job, max_level[job][type]);
		}
	}
	fclose(fp);
	for (i = 0; i < MAX_PC_CLASS; i++) {
		if (!pcdb_checkid(i)) continue;
		if (i == JOB_WEDDING || i == JOB_XMAS)
			continue; //Classes that do not need exp tables.
		if (!max_level[i][0])
			ShowWarning("Class %s (%d) does not has a base exp table.\n", job_name(i), i);
		if (!max_level[i][1])
			ShowWarning("Class %s (%d) does not has a job exp table.\n", job_name(i), i);
	}
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","exp.txt");

	// スキルツリ?
	memset(skill_tree,0,sizeof(skill_tree));
	sprintf(line, "%s/skill_tree.txt", db_path);
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
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","skill_tree.txt");

	// ?性修正テ?ブル
	for(i=0;i<4;i++)
		for(j=0;j<10;j++)
			for(k=0;k<10;k++)
				attr_fix_table[i][j][k]=100;

	sprintf(line, "%s/attr_fix.txt", db_path);
	fp=fopen(line,"r");
	if(fp==NULL){
		ShowError("can't read %s\n", line);
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
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","attr_fix.txt");

	// スキルツリ?
	memset(statp,0,sizeof(statp));
	i=1;
	j=45;	// base points
	sprintf(line, "%s/statpoint.txt", db_path);
	fp=fopen(line,"r");
	if(fp == NULL){
		ShowStatus("Can't read '"CL_WHITE"%s"CL_RESET"'... Generating DB.\n",line);
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
		ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","statpoint.txt");
	}
	// generate the remaining parts of the db if necessary
	for (; i < MAX_LEVEL; i++) {
		j += (i+15)/5;
		statp[i] = j;		
	}

	return 0;
}

// Read MOTD on startup. [Valaris]
int pc_read_motd(void) {
	FILE *fp;
	int ln=0,i=0;

	memset(motd_text,0,sizeof(motd_text));
	if ((fp = fopen(motd_txt, "r")) != NULL) {
		while ((ln < MOTD_LINE_SIZE) && fgets(motd_text[ln], sizeof(motd_text[ln])-1, fp) != NULL) {
			if(motd_text[ln][0] == '/' && motd_text[ln][1] == '/')
				continue;
			for(i=0; motd_text[ln][i]; i++) {
				if (motd_text[ln][i] == '\r' || motd_text[ln][i]== '\n') {
					if(i)
						motd_text[ln][i]=0;
					else
						motd_text[ln][0]=' ';
					ln++;
					break;
				}
			}
		}
		fclose(fp);
	}
	else if(battle_config.error_log)
		ShowWarning("In function pc_read_motd() -> File '"CL_WHITE"%s"CL_RESET"' not found.\n", motd_txt);
	
	return 0;
}

/*==========================================
 * pc? 係初期化
 *------------------------------------------
 */
void do_final_pc(void) {
	if (gm_account)
		aFree(gm_account);
	return;
}
int do_init_pc(void) {
	pc_readdb();
	pc_read_motd(); // Read MOTD [Valaris]

	add_timer_func_list(pc_natural_heal, "pc_natural_heal");
	add_timer_func_list(pc_invincible_timer, "pc_invincible_timer");
	add_timer_func_list(pc_eventtimer, "pc_eventtimer");
	add_timer_func_list(pc_calc_pvprank_timer, "pc_calc_pvprank_timer");
	add_timer_func_list(pc_autosave, "pc_autosave");
	add_timer_func_list(pc_spiritball_timer, "pc_spiritball_timer");
	add_timer_func_list(pc_follow_timer, "pc_follow_timer");
	natural_heal_prev_tick = gettick();
	add_timer_interval(natural_heal_prev_tick + NATURAL_HEAL_INTERVAL, pc_natural_heal, 0, 0, NATURAL_HEAL_INTERVAL);
	add_timer(gettick() + autosave_interval, pc_autosave, 0, 0);

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
