// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "timer.h"
#include "socket.h"
#include "nullpo.h"
#include "malloc.h"
#include "pc.h"
#include "status.h"
#include "map.h"
#include "intif.h"
#include "clif.h"
#include "chrif.h"
#include "pet.h"
#include "itemdb.h"
#include "battle.h"
#include "mob.h"
#include "npc.h"
#include "script.h"
#include "skill.h"
#include "showmsg.h"

#define MIN_PETTHINKTIME 100

struct pet_db pet_db[MAX_PET_DB];

static int dirx[8]={0,-1,-1,-1,0,1,1,1};
static int diry[8]={1,1,0,-1,-1,-1,0,1};

static int pet_performance_val(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd); 

	if(sd->pet.intimate > 900)
		return (sd->petDB->s_perfor > 0)? 4:3;
	else if(sd->pet.intimate > 750)
		return 2;
	else
		return 1;
}

int pet_hungry_val(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd); 

	if(sd->pet.hungry > 90)
		return 4;
	else if(sd->pet.hungry > 75)
		return 3;
	else if(sd->pet.hungry > 25)
		return 2;
	else if(sd->pet.hungry > 10)
		return 1;
	else
		return 0;
}

static int pet_calc_pos(struct pet_data *pd,int tx,int ty,int dir)
{
	int x,y,dx,dy;
	int i,k;

	nullpo_retr(0, pd);

	pd->ud.to_x = tx;
	pd->ud.to_y = ty;

	if(dir < 0 || dir >= 8)
	 return 1;
	
	dx = -dirx[dir]*2;
	dy = -diry[dir]*2;
	x = tx + dx;
	y = ty + dy;
	if(!unit_can_reach_pos(&pd->bl,x,y,0)) {
		if(dx > 0) x--;
		else if(dx < 0) x++;
		if(dy > 0) y--;
		else if(dy < 0) y++;
		if(!unit_can_reach_pos(&pd->bl,x,y,0)) {
			for(i=0;i<12;i++) {
				k = rand()%8;
				dx = -dirx[k]*2;
				dy = -diry[k]*2;
				x = tx + dx;
				y = ty + dy;
				if(unit_can_reach_pos(&pd->bl,x,y,0))
					break;
				else {
					if(dx > 0) x--;
					else if(dx < 0) x++;
					if(dy > 0) y--;
					else if(dy < 0) y++;
					if(unit_can_reach_pos(&pd->bl,x,y,0))
						break;
				}
			}
			if(i>=12) {
				x = tx;
				y = ty;
				if(!unit_can_reach_pos(&pd->bl,x,y,0))
					return 1;
			}
		}
	}
	pd->ud.to_x = x;
	pd->ud.to_y = y;
	return 0;
}

int pet_unlocktarget(struct pet_data *pd)
{
	nullpo_retr(0, pd);

	pd->target_id=0;
	pet_stop_attack(pd);
	pet_stop_walking(pd,1);
	return 0;
}

/*==========================================
 * Pet Attack Skill [Skotlex]
 *------------------------------------------
 */
int pet_attackskill(struct pet_data *pd, int target_id)
{
	struct block_list *bl;

	if (!battle_config.pet_status_support || !pd->a_skill || 
		(battle_config.pet_equip_required && !pd->equip))
		return 0;

	if (rand()%100 < (pd->a_skill->rate +pd->msd->pet.intimate*pd->a_skill->bonusrate/1000))
	{	//Skotlex: Use pet's skill 
		bl=map_id2bl(target_id);
		if(bl == NULL || pd->bl.m != bl->m || bl->prev == NULL || status_isdead(bl) ||
			!check_distance_bl(&pd->bl, bl, pd->db->range3))
			return 0;
		if (skill_get_inf(pd->a_skill->id) & INF_GROUND_SKILL)
			unit_skilluse_pos(&pd->bl, bl->x, bl->y, pd->a_skill->id, pd->a_skill->lv);
		else
			unit_skilluse_id(&pd->bl, bl->id, pd->a_skill->id, pd->a_skill->lv);
		return 1; //Skill invoked.
	}
	return 0;
}

int pet_target_check(struct map_session_data *sd,struct block_list *bl,int type)
{
	struct pet_data *pd;
	int rate;

	pd = sd->pd;
	
	Assert((pd->msd == 0) || (pd->msd->pd == pd));

	if(bl == NULL || bl->type != BL_MOB || bl->prev == NULL ||
		sd->pet.intimate < battle_config.pet_support_min_friendly ||
		sd->pet.hungry < 1 ||
		pd->class_ == status_get_class(bl))
		return 0;

	if(pd->bl.m != bl->m ||
		!check_distance_bl(&pd->bl, bl, pd->db->range2))
		return 0;

	if (!status_check_skilluse(&pd->bl, bl, 0, 0))
		return 0;

	if(!type) {
		rate = sd->petDB->attack_rate;
		rate = rate * pd->rate_fix/1000;
		if(sd->petDB->attack_rate > 0 && rate <= 0)
			rate = 1;
	} else {
		rate = sd->petDB->defence_attack_rate;
		rate = rate * pd->rate_fix/1000;
		if(sd->petDB->defence_attack_rate > 0 && rate <= 0)
			rate = 1;
	}
	if(rand()%10000 < rate) 
	{
		if(pd->target_id == 0 || rand()%10000 < sd->petDB->change_target_rate)
			pd->target_id = bl->id;
	}

	return 0;
}
/*==========================================
 * Pet SC Check [Skotlex]
 *------------------------------------------
 */
int pet_sc_check(struct map_session_data *sd, int type)
{	
	struct pet_data *pd;

	nullpo_retr(0, sd);
	pd = sd->pd;

	if (pd == NULL ||
		(battle_config.pet_equip_required && pd->equip == 0) ||
		pd->recovery == NULL ||
		pd->recovery->timer != -1 ||
		pd->recovery->type != type)
		return 1;

	pd->recovery->timer = add_timer(gettick()+pd->recovery->delay*1000,pet_recovery_timer,sd->bl.id,0);
	
	return 0;
}

static int pet_hungry(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	int interval,t;


	sd=map_id2sd(id);
	if(sd==NULL)
		return 1;

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd); 

	if(sd->pet_hungry_timer != tid){
		if(battle_config.error_log)
			ShowError("pet_hungry_timer %d != %d\n",sd->pet_hungry_timer,tid);
		return 0;
	}
	sd->pet_hungry_timer = -1;
	if(!sd->status.pet_id || !sd->pd || !sd->petDB)
		return 1;

	if (sd->pet.intimate <= 0)
		return 1; //You lost the pet already, the rest is irrelevant.
	
	sd->pet.hungry--;
	t = sd->pet.intimate;
	if(sd->pet.hungry < 0) {
		pet_stop_attack(sd->pd);
		sd->pet.hungry = 0;
		sd->pet.intimate -= battle_config.pet_hungry_friendly_decrease;
		if(sd->pet.intimate <= 0) {
			sd->pet.intimate = 0;
			sd->pd->speed = sd->pd->db->speed;
			if(battle_config.pet_status_support && t > 0) {
				if(sd->bl.prev != NULL)
					status_calc_pc(sd,0);
				else
					status_calc_pc(sd,2);
			}
		}
		status_calc_pet(sd, 0);
		clif_send_petdata(sd,1,sd->pet.intimate);
	}
	clif_send_petdata(sd,2,sd->pet.hungry);

	if(battle_config.pet_hungry_delay_rate != 100)
		interval = (sd->petDB->hungry_delay*battle_config.pet_hungry_delay_rate)/100;
	else
		interval = sd->petDB->hungry_delay;
	if(interval <= 0)
		interval = 1;
	sd->pet_hungry_timer = add_timer(tick+interval,pet_hungry,sd->bl.id,0);

	return 0;
}

int search_petDB_index(int key,int type)
{
	int i;

	for(i=0;i<MAX_PET_DB;i++) {
		if(pet_db[i].class_ <= 0)
			continue;
		switch(type) {
			case PET_CLASS:
				if(pet_db[i].class_ == key)
					return i;
				break;
			case PET_CATCH:
				if(pet_db[i].itemID == key)
					return i;
				break;
			case PET_EGG:
				if(pet_db[i].EggID == key)
					return i;
				break;
			case PET_EQUIP:
				if(pet_db[i].AcceID == key)
					return i;
				break;
			case PET_FOOD:
				if(pet_db[i].FoodID == key)
					return i;
				break;
			default:
				return -1;
		}
	}
	return -1;
}

int pet_hungry_timer_delete(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->pet_hungry_timer != -1) {
		delete_timer(sd->pet_hungry_timer,pet_hungry);
		sd->pet_hungry_timer = -1;
	}

	return 0;
}

struct delay_item_drop {
	int m,x,y;
	int nameid,amount;
	struct map_session_data *first_sd,*second_sd,*third_sd;
};

struct delay_item_drop2 {
	int m,x,y;
	struct item item_data;
	struct map_session_data *first_sd,*second_sd,*third_sd;
};

int pet_performance(struct map_session_data *sd)
{
	struct pet_data *pd;

	nullpo_retr(0, sd);
	nullpo_retr(0, pd=sd->pd);

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd); 

	pet_stop_walking(pd,2000<<8);
	clif_pet_performance(&pd->bl,rand()%pet_performance_val(sd) + 1);
	// ���[�g����Item�𗎂Ƃ�����
	pet_lootitem_drop(pd,NULL);

	return 0;
}

int pet_return_egg(struct map_session_data *sd)
{
	struct item tmp_item;
	int flag;

	nullpo_retr(0, sd);

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd); 

	if(sd->status.pet_id && sd->pd) {
		// ���[�g����Item�𗎂Ƃ�����
		pet_lootitem_drop(sd->pd,sd);
		if(sd->petDB == NULL)
			return 1;
		memset(&tmp_item,0,sizeof(tmp_item));
		tmp_item.nameid = sd->petDB->EggID;
		tmp_item.identify = 1;
		tmp_item.card[0] = (short)0xff00;
		tmp_item.card[1] = GetWord(sd->pet.pet_id,0);
		tmp_item.card[2] = GetWord(sd->pet.pet_id,1);
		tmp_item.card[3] = sd->pet.rename_flag;
		if((flag = pc_additem(sd,&tmp_item,1))) {
			clif_additem(sd,0,0,flag);
			map_addflooritem(&tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
		sd->pet.incuvate = 1;
		intif_save_petdata(sd->status.account_id,&sd->pet);
		unit_free(&sd->pd->bl);
		if(battle_config.pet_status_support && sd->pet.intimate > 0)
			status_calc_pc(sd,0);
		memset(&sd->pet, 0, sizeof(struct s_pet));
		sd->status.pet_id = 0;
		sd->petDB = NULL;
	}

	return 0;
}

int pet_data_init(struct map_session_data *sd)
{
	struct pet_data *pd;
	int i=0,interval=0;

	nullpo_retr(1, sd);

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd); 

	if(sd->status.account_id != sd->pet.account_id || sd->status.char_id != sd->pet.char_id) {
		sd->status.pet_id = 0;
		return 1;
	}
	if (sd->status.pet_id != sd->pet.pet_id) {
		if (sd->status.pet_id) {
			//Wrong pet?? Set incuvate to no and send it back for saving.
			sd->pet.incuvate = 1;
			intif_save_petdata(sd->status.account_id,&sd->pet);
			sd->status.pet_id = 0;
			return 1;
		}
		//The pet_id value was lost? odd... restore it.
		sd->status.pet_id = sd->pet.pet_id;
	}
	
	i = search_petDB_index(sd->pet.class_,PET_CLASS);
	if(i < 0) {
		sd->status.pet_id = 0;
		return 1;
	}
	sd->petDB = &pet_db[i];
	sd->pd = pd = (struct pet_data *)aCalloc(1,sizeof(struct pet_data));
	pd->bl.m = sd->bl.m;
	pd->bl.x = sd->bl.x;
	pd->bl.y = sd->bl.y;
	pet_calc_pos(pd,sd->bl.x,sd->bl.y,sd->ud.dir);
	pd->bl.x = pd->ud.to_x;
	pd->bl.y = pd->ud.to_y;
	pd->bl.id = npc_get_new_npc_id();
	memcpy(pd->name, sd->pet.name, NAME_LENGTH-1);
	pd->class_ = sd->pet.class_;
	pd->db = mob_db(pd->class_);
	pd->equip = sd->pet.equip;
	pd->speed = sd->petDB->speed;
	pd->bl.subtype = MONS;
	pd->bl.type = BL_PET;
	pd->msd = sd;
	status_set_viewdata(&pd->bl,pd->class_);
	unit_dataset(&sd->pd->bl);
	pd->ud.dir = sd->ud.dir;
	pd->last_thinktime = gettick();

	map_addiddb(&pd->bl);

	// initialise
	if (battle_config.pet_lv_rate)	//[Skotlex]
		pd->status = (struct pet_status *) aCalloc(1,sizeof(struct pet_status));

	status_calc_pet(sd,1);

	pd->state.skillbonus = -1;
	if (battle_config.pet_status_support) //Skotlex
		run_script(pet_db[i].script,0,sd->bl.id,0);

	if(sd->pet_hungry_timer != -1)
		pet_hungry_timer_delete(sd);
	if(battle_config.pet_hungry_delay_rate != 100)
		interval = (sd->petDB->hungry_delay*battle_config.pet_hungry_delay_rate)/100;
	else
		interval = sd->petDB->hungry_delay;
	if(interval <= 0)
		interval = 1;
	sd->pet_hungry_timer = add_timer(gettick()+interval,pet_hungry,sd->bl.id,0);
	return 0;
}

int pet_birth_process(struct map_session_data *sd)
{
	nullpo_retr(1, sd);

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd); 

	if(sd->status.pet_id && sd->pet.incuvate == 1) {
		sd->status.pet_id = 0;
		return 1;
	}

	sd->pet.incuvate = 0;
	sd->pet.account_id = sd->status.account_id;
	sd->pet.char_id = sd->status.char_id;
	sd->status.pet_id = sd->pet.pet_id;
	if(pet_data_init(sd)) {
		sd->status.pet_id = 0;
		sd->pet.incuvate = 1;
		sd->pet.account_id = 0;
		sd->pet.char_id = 0;
		return 1;
	}

	intif_save_petdata(sd->status.account_id,&sd->pet);
	chrif_save(sd,0); //FIXME: As before, is it REALLY Needed to save the char for hatching a pet? [Skotlex]

	map_addblock(&sd->pd->bl);
	clif_spawn(&sd->pd->bl);
	clif_send_petdata(sd,0,0);
	clif_send_petdata(sd,5,battle_config.pet_hair_style);
	clif_pet_equip(sd->pd,sd->pet.equip);
	clif_send_petstatus(sd);

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd); 

	return 0;
}

int pet_recv_petdata(int account_id,struct s_pet *p,int flag)
{
	struct map_session_data *sd;

	sd = map_id2sd(account_id);
	if(sd == NULL)
		return 1;
	if(flag == 1) {
		sd->status.pet_id = 0;
		return 1;
	}
	memcpy(&sd->pet,p,sizeof(struct s_pet));
	if(sd->pet.incuvate == 1) {
		int i;
		//Delete egg from inventory. [Skotlex]
		for (i = 0; i < MAX_INVENTORY; i++) {
			if(sd->status.inventory[i].card[0] == (short)0xff00 &&
				p->pet_id == MakeDWord(sd->status.inventory[i].card[1], sd->status.inventory[i].card[2]))
				break;
		}
		if(i >= MAX_INVENTORY) {
		  	if (battle_config.error_log)
				ShowError("pet_recv_petdata: Hatching pet (%d:%s) aborted, couldn't find egg in inventory for removal!\n",p->pet_id, p->name);
			sd->status.pet_id = 0;
			memset(&sd->pet,0,sizeof(struct s_pet));
			sd->pet.incuvate = 1;
			return 1;
		}
		if (!pet_birth_process(sd)) //Pet hatched. Delete egg.
			pc_delitem(sd,i,1,0);
	} else {
		pet_data_init(sd);
		if(sd->pd && sd->bl.prev != NULL) {
			map_addblock(&sd->pd->bl);
			clif_spawn(&sd->pd->bl);
			clif_send_petdata(sd,0,0);
			clif_send_petdata(sd,5,battle_config.pet_hair_style);
//			clif_pet_equip(sd->pd,sd->pet.equip);
			clif_send_petstatus(sd);
		}
	}
	if(battle_config.pet_status_support && sd->pet.intimate > 0) {
		if(sd->bl.prev != NULL)
			status_calc_pc(sd,0);
		else
			status_calc_pc(sd,2);
	}

	return 0;
}

int pet_select_egg(struct map_session_data *sd,short egg_index)
{
	nullpo_retr(0, sd);

	if(egg_index < 0 || egg_index >= MAX_INVENTORY)
		return 0; //Forged packet!

	if(sd->status.inventory[egg_index].card[0] == (short)0xff00)
		intif_request_petdata(sd->status.account_id, sd->status.char_id, MakeDWord(sd->status.inventory[egg_index].card[1], sd->status.inventory[egg_index].card[2]) );
	else {
		if(battle_config.error_log)
			ShowError("wrong egg item inventory %d\n",egg_index);
	}
	return 0;
}

int pet_catch_process1(struct map_session_data *sd,int target_class)
{
	nullpo_retr(0, sd);

	sd->catch_target_class = target_class;
	clif_catch_process(sd);

	return 0;
}

int pet_catch_process2(struct map_session_data *sd,int target_id)
{
	struct mob_data *md;
	int i=0,pet_catch_rate=0;

	nullpo_retr(1, sd);

	md=(struct mob_data*)map_id2bl(target_id);
	if(!md || md->bl.type != BL_MOB || md->bl.prev == NULL){
		//Abort capture.
		sd->catch_target_class = -1;
		sd->itemid = sd->itemindex = -1;
		return 1;
	}
	
	if (sd->menuskill_id != SA_TAMINGMONSTER) 
	{	//Exploit?
		clif_pet_rulet(sd,0);
		sd->catch_target_class = -1;
		return 1;
	}
	
	if (sd->menuskill_lv > 0)
	{	//Consume the pet lure [Skotlex]
		i=pc_search_inventory(sd,sd->menuskill_lv);
		if (i < 0)
		{	//they tried an exploit?
			clif_pet_rulet(sd,0);
			sd->catch_target_class = -1;
			return 1;
		}
		//Delete the item
		if (sd->itemid == sd->menuskill_lv)
			sd->itemid = sd->itemindex = -1;
		sd->menuskill_id = sd->menuskill_lv = 0;
		pc_delitem(sd,i,1,0);
	}

	i = search_petDB_index(md->class_,PET_CLASS);
	//catch_target_class == 0 is used for universal lures. [Skotlex]
	//for now universal lures do not include bosses.
	if (sd->catch_target_class == 0 && !(md->db->mode&0x20))
		sd->catch_target_class = md->class_;
	if(i < 0 || sd->catch_target_class != md->class_) {
		clif_emotion(&md->bl, 7);	//mob will do /ag if wrong lure is used on them.
		clif_pet_rulet(sd,0);
		sd->catch_target_class = -1;
		return 1;
	}

	//target_id�ɂ��G��������
//	if(battle_config.etc_log)
//		printf("mob_id = %d, mob_class = %d\n",md->bl.id,md->class_);
		//�����̏ꍇ
	pet_catch_rate = (pet_db[i].capture + (sd->status.base_level - md->db->lv)*30 + sd->paramc[5]*20)*(200 - md->hp*100/md->db->max_hp)/100;
	if(pet_catch_rate < 1) pet_catch_rate = 1;
	if(battle_config.pet_catch_rate != 100)
		pet_catch_rate = (pet_catch_rate*battle_config.pet_catch_rate)/100;

	if(rand()%10000 < pet_catch_rate) {
		unit_remove_map(&md->bl,0);
		clif_pet_rulet(sd,1);
//		if(battle_config.etc_log)
//			printf("rulet success %d\n",target_id);
		intif_create_pet(sd->status.account_id,sd->status.char_id,pet_db[i].class_,mob_db(pet_db[i].class_)->lv,
			pet_db[i].EggID,0,pet_db[i].intimate,100,0,1,pet_db[i].jname);
	}
	else
	{
		sd->catch_target_class = -1;
		clif_pet_rulet(sd,0);
	}

	return 0;
}

int pet_get_egg(int account_id,int pet_id,int flag)
{	//This function is invoked when a new pet has been created, and at no other time!
	struct map_session_data *sd;
	struct item tmp_item;
	int i=0,ret=0;

	if(!flag) {
		sd = map_id2sd(account_id);
		if(sd == NULL)
			return 1;

		i = search_petDB_index(sd->catch_target_class,PET_CLASS);
		sd->catch_target_class = -1;
		
		if(i >= 0) {
			memset(&tmp_item,0,sizeof(tmp_item));
			tmp_item.nameid = pet_db[i].EggID;
			tmp_item.identify = 1;
			tmp_item.card[0] = (short)0xff00;
			tmp_item.card[1] = GetWord(pet_id,0);
			tmp_item.card[2] = GetWord(pet_id,1);
			tmp_item.card[3] = 0; //New pets are not named.
			if((ret = pc_additem(sd,&tmp_item,1))) {
				clif_additem(sd,0,0,ret);
				map_addflooritem(&tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
			}
		}
		else
			intif_delete_petdata(pet_id);
	}

	return 0;
}

int pet_menu(struct map_session_data *sd,int menunum)
{
	nullpo_retr(0, sd);
	if (sd->pd == NULL)
		return 1;
	
	//You lost the pet already.
	if(sd->pet.intimate <= 0)
		return 1;
	
	switch(menunum) {
		case 0:
			clif_send_petstatus(sd);
			break;
		case 1:
			pet_food(sd);
			break;
		case 2:
			pet_performance(sd);
			break;
		case 3:
			pet_return_egg(sd);
			break;
		case 4:
			pet_unequipitem(sd);
			break;
	}
	return 0;
}

int pet_change_name(struct map_session_data *sd,char *name)
{
	int i;
	
	nullpo_retr(1, sd);

	if((sd->pd == NULL) || (sd->pet.rename_flag == 1 && battle_config.pet_rename == 0))
		return 1;

	for(i=0;i<NAME_LENGTH && name[i];i++){
		if( !(name[i]&0xe0) || name[i]==0x7f)
			return 1;
	}

	pet_stop_walking(sd->pd,1);
	
	memcpy(sd->pet.name, name, NAME_LENGTH-1);
	memcpy(sd->pd->name, name, NAME_LENGTH-1);
	
	clif_clearchar_area(&sd->pd->bl,0);
	clif_spawn(&sd->pd->bl);
	clif_send_petdata(sd,0,0);
	clif_send_petdata(sd,5,battle_config.pet_hair_style);
	sd->pet.rename_flag = 1;
	clif_pet_equip(sd->pd,sd->pet.equip);
	clif_send_petstatus(sd);

	return 0;
}

int pet_equipitem(struct map_session_data *sd,int index)
{
	int nameid;

	nullpo_retr(1, sd);

	nameid = sd->status.inventory[index].nameid;
	if(sd->petDB == NULL)
		return 1;
	if(sd->petDB->AcceID == 0 || nameid != sd->petDB->AcceID || sd->pet.equip != 0) {
		clif_equipitemack(sd,0,0,0);
		return 1;
	}
	else {
		pc_delitem(sd,index,1,0);
		sd->pet.equip = sd->pd->equip = nameid;
		status_calc_pc(sd,0);
		clif_pet_equip(sd->pd,nameid);
		if (battle_config.pet_equip_required)
		{ 	//Skotlex: start support timers if needd
			if (sd->pd->s_skill && sd->pd->s_skill->timer == -1)
			{
				if (sd->pd->s_skill->id)
					sd->pd->s_skill->timer=add_timer(gettick()+sd->pd->s_skill->delay*1000, pet_skill_support_timer, sd->bl.id, 0);
				else
					sd->pd->s_skill->timer=add_timer(gettick()+sd->pd->s_skill->delay*1000, pet_heal_timer, sd->bl.id, 0);
			}
			if (sd->pd->bonus && sd->pd->bonus->timer == -1)
				sd->pd->bonus->timer=add_timer(gettick()+sd->pd->bonus->delay*1000, pet_skill_bonus_timer, sd->bl.id, 0);
		}
	}

	return 0;
}

int pet_unequipitem(struct map_session_data *sd)
{
	struct item tmp_item;
	int nameid,flag;

	nullpo_retr(1, sd);

	if(sd->petDB == NULL)
		return 1;
	if(sd->pet.equip == 0)
		return 1;

	nameid = sd->pet.equip;
	sd->pet.equip = sd->pd->equip = 0;
	status_calc_pc(sd,0);
	clif_pet_equip(sd->pd,0);
	memset(&tmp_item,0,sizeof(tmp_item));
	tmp_item.nameid = nameid;
	tmp_item.identify = 1;
	if((flag = pc_additem(sd,&tmp_item,1))) {
		clif_additem(sd,0,0,flag);
		map_addflooritem(&tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
	}
	if (battle_config.pet_equip_required)
	{ 	//Skotlex: halt support timers if needed
		if (sd->pd->s_skill && sd->pd->s_skill->timer != -1)
		{
			if (sd->pd->s_skill->id)
				delete_timer(sd->pd->s_skill->timer, pet_skill_support_timer);
			else
				delete_timer(sd->pd->s_skill->timer, pet_heal_timer);
			sd->pd->s_skill->timer = -1;
		}
		if (sd->pd->bonus && sd->pd->bonus->timer != -1)
		{
			delete_timer(sd->pd->bonus->timer, pet_skill_bonus_timer);
			sd->pd->bonus->timer = -1;
		}
	}

	return 0;
}

int pet_food(struct map_session_data *sd)
{
	int i,k;

	nullpo_retr(1, sd);

	if(sd->petDB == NULL)
		return 1;
	i=pc_search_inventory(sd,sd->petDB->FoodID);
	if(i < 0) {
		clif_pet_food(sd,sd->petDB->FoodID,0);
		return 1;
	}
	pc_delitem(sd,i,1,0);

	if(sd->pet.hungry > 90)
		sd->pet.intimate -= sd->petDB->r_full;
	else {
		if(battle_config.pet_friendly_rate != 100)
			k = (sd->petDB->r_hungry * battle_config.pet_friendly_rate)/100;
		else
			k = sd->petDB->r_hungry;
		if(sd->pet.hungry > 75) {
			k = k >> 1;
			if(k <= 0)
				k = 1;
		}
		sd->pet.intimate += k;
	}
	if(sd->pet.intimate <= 0) {
		sd->pet.intimate = 0;
		pet_stop_attack(sd->pd);
		sd->pd->speed = sd->pd->db->speed;
	
		if(battle_config.pet_status_support) {
			if(sd->bl.prev != NULL)
				status_calc_pc(sd,0);
			else
				status_calc_pc(sd,2);
		}
	}
	else if(sd->pet.intimate > 1000)
		sd->pet.intimate = 1000;
	status_calc_pet(sd, 0);
	sd->pet.hungry += sd->petDB->fullness;
	if(sd->pet.hungry > 100)
		sd->pet.hungry = 100;

	clif_send_petdata(sd,2,sd->pet.hungry);
	clif_send_petdata(sd,1,sd->pet.intimate);
	clif_pet_food(sd,sd->petDB->FoodID,1);

	return 0;
}

static int pet_randomwalk(struct pet_data *pd,unsigned int tick)
{
	const int retrycount=20;
	int speed;

	nullpo_retr(0, pd);

	Assert((pd->msd == 0) || (pd->msd->pd == pd));

	speed = status_get_speed(&pd->bl);

	if(DIFF_TICK(pd->next_walktime,tick) < 0 && unit_can_move(&pd->bl)) {
		int i,x,y,c,d=12-pd->move_fail_count;
		if(d<5) d=5;
		for(i=0;i<retrycount;i++){
			int r=rand();
			x=pd->bl.x+r%(d*2+1)-d;
			y=pd->bl.y+r/(d*2+1)%(d*2+1)-d;
			if(map_getcell(pd->bl.m,x,y,CELL_CHKPASS) && unit_walktoxy(&pd->bl,x,y,0)){
				pd->move_fail_count=0;
				break;
			}
			if(i+1>=retrycount){
				pd->move_fail_count++;
				if(pd->move_fail_count>1000){
					if(battle_config.error_log)
						ShowWarning("PET cant move. hold position %d, class = %d\n",pd->bl.id,pd->class_);
					pd->move_fail_count=0;
					pd->ud.canmove_tick = tick + 60000;
					return 0;
				}
			}
		}
		for(i=c=0;i<pd->ud.walkpath.path_len;i++){
			if(pd->ud.walkpath.path[i]&1)
				c+=speed*14/10;
			else
				c+=speed;
		}
		pd->next_walktime = tick+rand()%3000+3000+c;

		return 1;
	}
	return 0;
}

static int pet_ai_sub_hard(struct pet_data *pd,unsigned int tick)
{
	struct map_session_data *sd;
	struct block_list *target = NULL;

	sd = pd->msd;

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd); 

	if(pd->bl.prev == NULL || sd == NULL || sd->bl.prev == NULL)
		return 0;

	if(DIFF_TICK(tick,pd->last_thinktime) < MIN_PETTHINKTIME)
		return 0;
	pd->last_thinktime=tick;

	if(pd->ud.attacktimer != -1 || pd->ud.skilltimer != -1 || pd->bl.m != sd->bl.m)
		return 0;

	if(pd->ud.walktimer != -1 && pd->ud.walkpath.path_pos <= 3)
		return 0; //No thinking when you just started to walk.

	if(sd->pet.intimate <= 0) {
		//Pet should just... well, random walk.
		pet_randomwalk(pd,tick);
		return 0;
	}
	
	if (!check_distance_bl(&sd->bl, &pd->bl, pd->db->range2)) {
		//Master too far, chase.
		if(pd->target_id)
			pet_unlocktarget(pd);
		if(pd->ud.walktimer != -1 && pd->ud.target == sd->bl.id)
			return 0; //Already walking to him
		
		pd->speed = (sd->speed>>1);
		if(pd->speed <= 0)
			pd->speed = 1;
		if (!unit_walktobl(&pd->bl, &sd->bl, 3, 0));
			pet_randomwalk(pd,tick);
		return 0;
	}

	//Return speed to normal.
	if (pd->speed != sd->petDB->speed)
		pd->speed = sd->petDB->speed;
	
	if (pd->target_id) {
		target= map_id2bl(pd->target_id);
		if (!target || pd->bl.m != target->m || status_isdead(target) ||
			!check_distance_bl(&pd->bl, target, pd->db->range3))
		{
			target = NULL;
			pet_unlocktarget(pd);
		}
	}
	
	// �y�b�g�ɂ�郋�[�g
	if(!target && pd->loot && pd->loot->count < pd->loot->max && DIFF_TICK(tick,pd->ud.canact_tick)>0) {
		//Use half the pet's range of sight.
		int itc=0;
		map_foreachinrange(pet_ai_sub_hard_lootsearch,&pd->bl,
			pd->db->range2/2, BL_ITEM,pd,&itc);
	}
	if (!target) {
	//Just walk around.
		if (check_distance_bl(&sd->bl, &pd->bl, 3))
			return 0; //Already next to master.

		if(pd->ud.walktimer != -1 && check_distance_blxy(&sd->bl, pd->ud.to_x,pd->ud.to_y, 3))
			return 0; //Already walking to him

		pet_calc_pos(pd,sd->bl.x,sd->bl.y,sd->ud.dir);
		if(!unit_walktoxy(&pd->bl,pd->ud.to_x,pd->ud.to_y,0))
			pet_randomwalk(pd,tick);

		return 0;
	}
	
	if(pd->ud.target == target->id &&
		(pd->ud.attacktimer != -1 || pd->ud.walktimer != -1))
		return 0; //Target already locked.

	if (target->type != BL_ITEM) 
	{ //enemy targetted
		if(!battle_check_range(&pd->bl,target,pd->db->range))
		{	//Chase
			if(!unit_walktobl(&pd->bl, target, pd->db->range, 2))
				pet_unlocktarget(pd); //Unreachable target.
			return 0;
		}
		//Continuous attack.
		unit_attack(&pd->bl, pd->target_id, 1);
	} else {	//Item Targeted, attempt loot
		if (!check_distance_bl(&pd->bl, target, 1))
		{	//Out of range
			if(!unit_walktobl(&pd->bl, target, 0, 1)) //Unreachable target.
				pet_unlocktarget(pd);
			return 0;
		} else{
			struct flooritem_data *fitem = (struct flooritem_data *)target;
			if(pd->loot->count < pd->loot->max){
				memcpy(&pd->loot->item[pd->loot->count++],&fitem->item_data,sizeof(pd->loot->item[0]));
				pd->loot->weight += itemdb_search(fitem->item_data.nameid)->weight*fitem->item_data.amount;
				map_clearflooritem(target->id);
			} 
			//Target is unlocked regardless of whether it was picked or not.
			pet_unlocktarget(pd);
		}
	}
	return 0;
}

static int pet_ai_sub_foreachclient(struct map_session_data *sd,va_list ap)
{
	unsigned int tick;
	tick=va_arg(ap,unsigned int);
	if(sd->status.pet_id && sd->pd && sd->petDB)
		pet_ai_sub_hard(sd->pd,tick);

	return 0;
}

static int pet_ai_hard(int tid,unsigned int tick,int id,int data)
{
	clif_foreachclient(pet_ai_sub_foreachclient,tick);

	return 0;
}

int pet_ai_sub_hard_lootsearch(struct block_list *bl,va_list ap)
{
	struct pet_data* pd;
	struct flooritem_data *fitem = (struct flooritem_data *)bl;
	int sd_id =0;
	int *itc;

	pd=va_arg(ap,struct pet_data *);
	itc=va_arg(ap,int *);

	sd_id = fitem->first_get_id;
	// Removed [Valaris]
	//if((pd->lootitem_weight + (itemdb_search(fitem->item_data.))->weight * fitem->item_data.amount) > battle_config.pet_weight)
	//	return 0;

	if(pd->loot == NULL || pd->loot->item == NULL || (pd->loot->count >= pd->loot->max) ||
	 	(sd_id && pd->msd && pd->msd->bl.id != sd_id))
		return 0;
	if(bl->m == pd->bl.m && unit_can_reach_bl(&pd->bl,bl, pd->db->range2, 1, NULL, NULL)
		&& rand()%1000<1000/(++(*itc)))
		pd->target_id=bl->id;
	return 0;
}

int pet_lootitem_drop(struct pet_data *pd,struct map_session_data *sd)
{
	int i,flag=0;
	struct delay_item_drop2 *ditem_floor, ditem;
	if(pd && pd->loot && pd->loot->count) {
		memset(&ditem, 0, sizeof(struct delay_item_drop2));
		ditem.m = pd->bl.m;
		ditem.x = pd->bl.x;
		ditem.y = pd->bl.y;
		ditem.first_sd = 0;
		ditem.second_sd = 0;
		ditem.third_sd = 0;
		for(i=0;i<pd->loot->count;i++) {
			memcpy(&ditem.item_data,&pd->loot->item[i],sizeof(pd->loot->item[0]));
			// ���Ƃ��Ȃ��Œ���PC��Item����
			if(sd){
				if((flag = pc_additem(sd,&ditem.item_data,ditem.item_data.amount))){
					clif_additem(sd,0,0,flag);
					map_addflooritem(&ditem.item_data,ditem.item_data.amount,ditem.m,ditem.x,ditem.y,ditem.first_sd,ditem.second_sd,ditem.third_sd,0);
				}
			}
			else {
				ditem_floor=(struct delay_item_drop2 *)aCalloc(1,sizeof(struct delay_item_drop2));
				memcpy(ditem_floor, &ditem, sizeof(struct delay_item_drop2));
				add_timer(gettick()+540+i,pet_delay_item_drop2,(int)ditem_floor,0);
			}
		}
		//The smart thing to do is use pd->loot->max (thanks for pointing it out, Shinomori)
		memset(pd->loot->item,0,pd->loot->max * sizeof(struct item));
		pd->loot->count = 0;
		pd->loot->weight = 0;
		pd->ud.canact_tick = gettick()+10000;	//	10*1000ms�̊ԏE��Ȃ�
	}
	return 1;
}

int pet_delay_item_drop2(int tid,unsigned int tick,int id,int data)
{
	struct delay_item_drop2 *ditem;

	ditem=(struct delay_item_drop2 *)id;

	map_addflooritem(&ditem->item_data,ditem->item_data.amount,ditem->m,ditem->x,ditem->y,ditem->first_sd,ditem->second_sd,ditem->third_sd,0);

	aFree(ditem);
	return 0;
}

/*==========================================
 * pet bonus giving skills [Valaris] / Rewritten by [Skotlex]
 *------------------------------------------
 */ 
int pet_skill_bonus_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	struct pet_data *pd;
	int timer = 0;

	if(sd == NULL || sd->pd==NULL || sd->pd->bonus == NULL)
		return 1;
	
	pd=sd->pd;

	if(pd->bonus->timer != tid) {
		if(battle_config.error_log)
		{
			ShowError("pet_skill_bonus_timer %d != %d\n",pd->bonus->timer,tid);
			pd->bonus->timer = -1;
		}
		return 0;
	}
	
	// determine the time for the next timer
	if (pd->state.skillbonus == 0) {
		// pet bonuses are not active at the moment, so,
		pd->state.skillbonus = 1;
		timer = pd->bonus->duration*1000;	// the duration for pet bonuses to be in effect
	} else if (pd->state.skillbonus == 1) {
		// pet bonuses are already active, so,
		pd->state.skillbonus = 0;
		timer = pd->bonus->delay*1000;	// the duration until pet bonuses will be reactivated again
		if (timer <= 0) //Always active bonus
			timer = MIN_PETTHINKTIME; 
	}

	// add/remove our bonuses
	status_calc_pc(sd, 0);

	// wait for the next timer
	pd->bonus->timer=add_timer(tick+timer,pet_skill_bonus_timer,sd->bl.id,0);
	
	return 0;
}

/*==========================================
 * pet recovery skills [Valaris] / Rewritten by [Skotlex]
 *------------------------------------------
 */ 
int pet_recovery_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	struct pet_data *pd;
	
	if(sd==NULL || sd->pd == NULL || sd->pd->recovery == NULL)
		return 1;
	
	pd=sd->pd;

	if(pd->recovery->timer != tid) {
		if(battle_config.error_log)
			ShowError("pet_recovery_timer %d != %d\n",pd->recovery->timer,tid);
		return 0;
	}

	if(sd->sc.count && sd->sc.data[pd->recovery->type].timer != -1)
	{	//Display a heal animation? 
		//Detoxify is chosen for now.
		clif_skill_nodamage(&pd->bl,&sd->bl,TF_DETOXIFY,1,1);
		status_change_end(&sd->bl,pd->recovery->type,-1);
		clif_emotion(&pd->bl, 33);
	}

	pd->recovery->timer = -1;
	
	return 0;
}

int pet_heal_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	struct pet_data *pd;
	short rate = 100;
	
	if(sd==NULL || sd->pd == NULL || sd->pd->s_skill == NULL)
		return 1;
	
	pd=sd->pd;

	if(pd->s_skill->timer != tid) {
		if(battle_config.error_log)
			ShowError("pet_heal_timer %d != %d\n",pd->s_skill->timer,tid);
		return 0;
	}
	
	if(pc_isdead(sd) ||
		(rate = sd->status.sp*100/sd->status.max_sp) > pd->s_skill->sp ||
		(rate = sd->status.hp*100/sd->status.max_hp) > pd->s_skill->hp ||
		(rate = (pd->ud.skilltimer != -1)) //Another skill is in effect
	) {  //Wait (how long? 1 sec for every 10% of remaining)
		pd->s_skill->timer=add_timer(gettick()+(rate>10?rate:10)*100,pet_heal_timer,sd->bl.id,0);
		return 0;
	}
	pet_stop_attack(pd);
	pet_stop_walking(pd,1);
	clif_skill_nodamage(&pd->bl,&sd->bl,AL_HEAL,pd->s_skill->lv,1);
	battle_heal(&pd->bl, &sd->bl, pd->s_skill->lv,0, 0);
	pd->s_skill->timer=add_timer(tick+pd->s_skill->delay*1000,pet_heal_timer,sd->bl.id,0);
	return 0;
}

/*==========================================
 * pet support skills [Skotlex]
 *------------------------------------------
 */ 
int pet_skill_support_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	struct pet_data *pd;
	short rate = 100;	
	if(sd==NULL || sd->pd == NULL || sd->pd->s_skill == NULL)
		return 1;
	
	pd=sd->pd;
	
	if(pd->s_skill->timer != tid) {
		if(battle_config.error_log)
			ShowError("pet_skill_support_timer %d != %d\n",pd->s_skill->timer,tid);
		return 0;
	}
	
	if(pc_isdead(sd) ||
		(rate = sd->status.sp*100/sd->status.max_sp) > pd->s_skill->sp ||
		(rate = sd->status.hp*100/sd->status.max_hp) > pd->s_skill->hp ||
		(rate = (pd->ud.skilltimer != -1)) //Another skill is in effect
	) {  //Wait (how long? 1 sec for every 10% of remaining)
		pd->s_skill->timer=add_timer(gettick()+(rate>10?rate:10)*100,pet_skill_support_timer,sd->bl.id,0);
		return 0;
	}
	
	pet_stop_attack(pd);
	pet_stop_walking(pd,1);
	
	if (skill_get_inf(pd->s_skill->id) & INF_GROUND_SKILL)
		unit_skilluse_pos(&pd->bl, sd->bl.x, sd->bl.y, pd->s_skill->id, pd->s_skill->lv);
	else
		unit_skilluse_id(&pd->bl, sd->bl.id, pd->s_skill->id, pd->s_skill->lv);

	pd->s_skill->timer=add_timer(tick+pd->s_skill->delay*1000,pet_skill_support_timer,sd->bl.id,0);
	return 0;
}

/*==========================================
 *�y�b�g�f�[�^�ǂݍ���
 *------------------------------------------
 */ 
int read_petdb()
{
	FILE *fp;
	char line[1024];
	int nameid,i,k; 
	int j=0;
	int lines;
	char *filename[]={"pet_db.txt","pet_db2.txt"};
	char *str[32],*p,*np;


//Remove any previous scripts in case reloaddb was invoked.	
	for(j =0; j < MAX_PET_DB; j++)
		if (pet_db[j].script) {
			aFree(pet_db[j].script);
			pet_db[j].script = NULL;
		}
	j = 0;
	memset(pet_db,0,sizeof(pet_db));
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
		while(fgets(line,1020,fp) && j < MAX_PET_DB){
			
			lines++;

			if(line[0] == '/' && line[1] == '/')
				continue;

			for(k=0,p=line;k<20;k++){
				if((np=strchr(p,','))!=NULL){
					str[k]=p;
					*np=0;
					p=np+1;
				} else {
					str[k]=p;
					p+=strlen(p);
				}
			}

			nameid=atoi(str[0]);
			if(nameid<=0 || nameid>2000)
				continue;
		
			//MobID,Name,JName,ItemID,EggID,AcceID,FoodID,"Fullness (1��̉a�ł̖����x������%)","HungryDeray (/min)","R_Hungry (�󕠎��a���e���x������%)","R_Full (�ƂĂ��������a���e���x������%)","Intimate (�ߊl���e���x%)","Die (���S���e���x������%)","Capture (�ߊl��%)",(Name)
			pet_db[j].class_ = nameid;
			memcpy(pet_db[j].name,str[1],NAME_LENGTH-1);
			memcpy(pet_db[j].jname,str[2],NAME_LENGTH-1);
			pet_db[j].itemID=atoi(str[3]);
			pet_db[j].EggID=atoi(str[4]);
			pet_db[j].AcceID=atoi(str[5]);
			pet_db[j].FoodID=atoi(str[6]);
			pet_db[j].fullness=atoi(str[7]);
			pet_db[j].hungry_delay=atoi(str[8])*1000;
			pet_db[j].r_hungry=atoi(str[9]);
			if(pet_db[j].r_hungry <= 0)
				pet_db[j].r_hungry=1;
			pet_db[j].r_full=atoi(str[10]);
			pet_db[j].intimate=atoi(str[11]);
			pet_db[j].die=atoi(str[12]);
			pet_db[j].capture=atoi(str[13]);
			pet_db[j].speed=atoi(str[14]);
			pet_db[j].s_perfor=(char)atoi(str[15]);
			pet_db[j].talk_convert_class=atoi(str[16]);
			pet_db[j].attack_rate=atoi(str[17]);
			pet_db[j].defence_attack_rate=atoi(str[18]);
			pet_db[j].change_target_rate=atoi(str[19]);
			pet_db[j].script = NULL;
			if((np=strchr(p,'{'))==NULL)
				continue;
			pet_db[j].script = parse_script((unsigned char *) np,lines);
			j++;
		}
		if (j >= MAX_PET_DB)
			ShowWarning("read_petdb: Reached max number of pets [%d]. Remaining pets were not read.\n ", MAX_PET_DB);
		fclose(fp);
		ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' pets in '"CL_WHITE"%s"CL_RESET"'.\n",j,filename[i]);
	}
	return 0;
}

/*==========================================
 * �X�L���֌W����������
 *------------------------------------------
 */
int do_init_pet(void)
{
	memset(pet_db,0,sizeof(pet_db));
	read_petdb();

	add_timer_func_list(pet_hungry,"pet_hungry");
	add_timer_func_list(pet_ai_hard,"pet_ai_hard");
	add_timer_func_list(pet_skill_bonus_timer,"pet_skill_bonus_timer"); // [Valaris]
	add_timer_func_list(pet_delay_item_drop2,"pet_delay_item_drop2");	
	add_timer_func_list(pet_skill_support_timer, "pet_skill_support_timer"); // [Skotlex]
	add_timer_func_list(pet_recovery_timer,"pet_recovery_timer"); // [Valaris]
	add_timer_func_list(pet_heal_timer,"pet_heal_timer"); // [Valaris]
	add_timer_interval(gettick()+MIN_PETTHINKTIME,pet_ai_hard,0,0,MIN_PETTHINKTIME);

	return 0;
}

int do_final_pet(void) {
	int i;
	for(i = 0;i < MAX_PET_DB; i++) {
		if(pet_db[i].script) {
			aFree(pet_db[i].script);
			pet_db[i].script = NULL;
		}
	}
	return 0;
}
