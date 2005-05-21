// $Id: pet.c,v 1.4 2004/09/25 05:32:18 MouseJstr Exp $
#include "base.h"
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
#include "utils.h"




struct castend_delay { //[Skotlex] For passing skill info after casting
	struct pet_data *src;
	unsigned long target_id;
	unsigned short skill_id;
	unsigned short skill_lv;
	short x;
	short y;
};
struct delay_item_drop {
	unsigned short m,x,y;
	unsigned short nameid,amount;
	struct map_session_data *first_sd,*second_sd,*third_sd;
};

struct delay_item_drop2 {
	int m,x,y;
	struct item item_data;
	struct map_session_data *first_sd,*second_sd,*third_sd;
};


#define MIN_PETTHINKTIME 100

struct pet_db pet_db[MAX_PET_DB];

static int dirx[8]={0,-1,-1,-1,0,1,1,1};
static int diry[8]={1,1,0,-1,-1,-1,0,1};

static int pet_timer(int tid,unsigned long tick,int id,int data);
static int pet_walktoxy_sub(struct pet_data &pd);

static int distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}

static int calc_next_walk_step(struct pet_data &pd)
{
	if(pd.walkpath.path_pos>=pd.walkpath.path_len)
		return -1;
	if(pd.walkpath.path[pd.walkpath.path_pos]&1)
		return pd.speed*14/10;
	return pd.speed;
}

static int pet_performance_val(struct map_session_data &sd)
{
	if(sd.pet.intimate > 900 && sd.petDB)
		return (sd.petDB->s_perfor > 0)? 4:3;
	else if(sd.pet.intimate > 750)
		return 2;
	else
		return 1;
}

int pet_hungry_val(struct map_session_data &sd)
{
	if(sd.pet.hungry > 90)
		return 4;
	else if(sd.pet.hungry > 75)
		return 3;
	else if(sd.pet.hungry > 25)
		return 2;
	else if(sd.pet.hungry > 10)
		return 1;
	else
		return 0;
}

static int pet_can_reach(struct pet_data &pd,int x,int y)
{
	struct walkpath_data wpd;

	if( pd.bl.x==x && pd.bl.y==y )	// �����}�X
		return 1;

	// ��Q������
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	return (path_search(wpd,pd.bl.m,pd.bl.x,pd.bl.y,x,y,0)!=-1)?1:0;
}

static int pet_calc_pos(struct pet_data &pd,int tx,int ty,int dir)
{
	int x,y,dx,dy;
	int i,j=0,k;

	pd.to_x = tx;
	pd.to_y = ty;

	if(dir >= 0 && dir < 8) {
		dx = -dirx[dir]*2;
		dy = -diry[dir]*2;
		x = tx + dx;
		y = ty + dy;
		if(!(j=pet_can_reach(pd,x,y))) {
			if(dx > 0) x--;
			else if(dx < 0) x++;
			if(dy > 0) y--;
			else if(dy < 0) y++;
			if(!(j=pet_can_reach(pd,x,y))) {
				for(i=0;i<12;i++) {
					k = rand()%8;
					dx = -dirx[k]*2;
					dy = -diry[k]*2;
					x = tx + dx;
					y = ty + dy;
					if((j=pet_can_reach(pd,x,y)))
						break;
					else {
						if(dx > 0) x--;
						else if(dx < 0) x++;
						if(dy > 0) y--;
						else if(dy < 0) y++;
						if((j=pet_can_reach(pd,x,y)))
							break;
					}
				}
				if(!j) {
					x = tx;
					y = ty;
					if(!pet_can_reach(pd,x,y))
						return 1;
				}
			}
		}
	}
	else
		return 1;

	pd.to_x = x;
	pd.to_y = y;
	return 0;
}

static int pet_unlocktarget(struct pet_data &pd)
{
	pd.target_id=0;
	return 0;
}

static int pet_attack(struct pet_data &pd,unsigned int tick,int data)
{
	struct mob_data *md;
//	int mode,race;
	short range;

//	pd.state.state=MS_IDLE;

	md=(struct mob_data *)map_id2bl(pd.target_id);
	if(md == NULL || pd.bl.m != md->bl.m || md->bl.prev == NULL ||
		distance(pd.bl.x,pd.bl.y,md->bl.x,md->bl.y) > 13)
//	|| md->bl.type != BL_MOB
	{
		pet_unlocktarget(pd);
		return 0;
	}
/*
	mode=mob_db[pd.class_].mode;
	race=mob_db[pd.class_].race;
	if(mob_db[pd.class_].mexp <= 0 && !(mode&0x20) && (md->option & 0x06 && race != 4 && race != 6) ) {
		pd.target_id=0;
		return 0;
	}
*/
	range = mob_db[pd.class_].range + 1;
	if(distance(pd.bl.x,pd.bl.y,md->bl.x,md->bl.y) > range)
		return 0;
	if(battle_config.monster_attack_direction_change)
		pd.dir=map_calc_dir(pd.bl, md->bl.x,md->bl.y );

	clif_fixpetpos(&pd);

	pd.target_lv = battle_weapon_attack(&pd.bl,&md->bl,tick,0);

	pd.attackabletime = tick + status_get_adelay(&pd.bl);

	pd.timer=add_timer(pd.attackabletime,pet_timer,pd.bl.id,0);
	pd.state.state=MS_ATTACK;
	return 0;
}

static int petskill_use(struct pet_data &pd, struct block_list &target, short skill_id, short skill_lv, unsigned int tick);
static int petskill_castend(struct pet_data &pd,unsigned long tick,int data);
static int petskill_castend2(struct pet_data &pd, struct block_list &target, short skill_id, short skill_lv, short skill_x, short skill_y, unsigned int tick);

/*==========================================
 * Pet Attack Skill [Skotlex]
 *------------------------------------------
 */
static int pet_attackskill(struct pet_data &pd, unsigned int tick, int data)
{
	//short mode,race;
	struct mob_data *md;

//	pd.state.state=MS_IDLE;

	md=(struct mob_data *)map_id2bl(pd.target_id);
	if(md == NULL || pd.bl.m != md->bl.m || md->bl.prev == NULL ||
		NULL==pd.a_skill ||
		distance(pd.bl.x,pd.bl.y,md->bl.x,md->bl.y) > 13)
//			|| md->bl.type != BL_MOB || (!agit_flag && md->class_ >= 1285 && md->class_ <= 1288)) // Cannot attack Guardians outside of WoE
	{
		pet_unlocktarget(pd);
		return 0;
	}
/*
	mode=mob_db[pd.class_].mode;
	race=mob_db[pd.class_].race;
	if(mob_db[pd.class_].mexp <= 0 && !(mode&0x20) && (md->option & 0x06 && race != 4 && race != 6) ) {
		pd.target_id=0;
		return 0;
	}
*/
	petskill_use(pd, md->bl, pd.a_skill->id, pd.a_skill->lv, tick);
	return 0;
}

/*==========================================
 * Pet Skill Use [Skotlex]
 *------------------------------------------
 */
static int petskill_use(struct pet_data &pd, struct block_list &target, short skill_id, short skill_lv, unsigned int tick)
{
	int casttime;
	struct castend_delay *dat;
	
	if(battle_config.monster_attack_direction_change)
		pd.dir=map_calc_dir(pd.bl, target.x, target.y );
	clif_fixpetpos(&pd);

	//Casting time
	casttime=skill_castfix(&pd.bl, skill_get_cast(skill_id, skill_lv));
		
	pd.attackabletime = tick;
	pd.state.state=MS_ATTACK;

	if (casttime > 0)
	{
		pd.attackabletime += casttime;
		pet_stop_walking(pd,1);
		
		dat = (struct castend_delay *)aCalloc(1, sizeof(struct castend_delay));
		dat->src = &pd;
		dat->target_id = target.id;
		dat->skill_id = skill_id;
		dat->skill_lv = skill_lv;
		dat->x = target.x;
		dat->y = target.y;

		pd.state.state=MS_ATTACK;
		pd.state.casting_flag = 1;
		if (skill_get_inf(skill_id) & 2) //Area Skill
			clif_skillcasting( &pd.bl, pd.bl.id, 0, dat->x, dat->y, skill_id, casttime);
		else
			clif_skillcasting( &pd.bl, pd.bl.id, dat->target_id, 0,0, skill_id,casttime);
		
		pd.timer = add_timer(pd.attackabletime,pet_timer,pd.bl.id,(int)dat);
	} else {
		petskill_castend2(pd, target, skill_id, skill_lv, target.x, target.y, tick);
	}	
	return 0;
}

/*==========================================
 * Pet Attack Cast End [Skotlex]
 *------------------------------------------
 */
static int petskill_castend(struct pet_data &pd,unsigned long tick,int data)
{
	struct castend_delay *dat = (struct castend_delay *)data;
	if(dat)
	{
		struct block_list *target = map_id2bl(dat->target_id);
		pd.state.state = MS_IDLE;
		pd.state.casting_flag = 0;
		if (target && &pd == dat->src && target->prev != NULL)
			petskill_castend2(pd, *target, dat->skill_id, dat->skill_lv, dat->x, dat->y, tick);
		aFree(dat);
	}
	return 0;
}

/*==========================================
 * Pet Attack Cast End2 [Skotlex]
 *------------------------------------------
 */
static int petskill_castend2(struct pet_data &pd, struct block_list &target, short skill_id, short skill_lv, short skill_x, short skill_y, unsigned int tick)
{	//Invoked after the casting time has passed.
	short delaytime =0, range;
	
	pd.state.state=MS_IDLE;
	
	if (skill_get_inf(skill_id) & 2)
	{	//Area skill
		skill_castend_pos2(&pd.bl, skill_x, skill_y, skill_id, skill_lv, tick,0);
	} else { //Targeted Skill
		//Skills with inf = 4 (cast on self) have view range (assumed party skills)
		range = (skill_get_inf(skill_id) & 4?battle_config.area_size:skill_get_range(skill_id, skill_lv));
		if(range < 0)
			range = status_get_range(&pd.bl) - (range + 1);
		if(distance(pd.bl.x, pd.bl.y, target.x, target.y) > range)
			return 0; 
		switch( skill_get_nk(skill_id) )
		{
			case 0:
			case 2: //Damage Attack Skill
				skill_castend_damage_id(&pd.bl,&target,skill_id,skill_lv,tick,0);
				break;
			case 1: //Non Damage Attack Skill
				if(
				(skill_id==AL_HEAL || skill_id==ALL_RESURRECTION) && battle_check_undead(status_get_race(&target),status_get_elem_type(&target)) )
					skill_castend_damage_id(&pd.bl, &target, skill_id, skill_lv, tick, 0);
				else
				{
				  skill_castend_nodamage_id(&pd.bl,&target, skill_id, skill_lv,tick, 0);
				}
				break;
		}
	}

	delaytime = skill_delayfix(&pd.bl,skill_get_delay(skill_id, skill_lv));
	if (delaytime < MIN_PETTHINKTIME)
		delaytime = status_get_adelay(&pd.bl);
	pd.attackabletime = tick + delaytime; 
	if (pd.target_id) //Resume attacking
		pd.state.state=MS_ATTACK;
	pd.timer=add_timer(pd.attackabletime,pet_timer,pd.bl.id,0);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pet_walk(struct pet_data &pd,unsigned long tick,int data)
{
	int moveblock;
	int i;
	int x,y,dx,dy;

	pd.state.state=MS_IDLE;
	if(pd.walkpath.path_pos >= pd.walkpath.path_len || pd.walkpath.path_pos != data)
		return 0;

	pd.walkpath.path_half ^= 1;
	if(pd.walkpath.path_half==0){
		pd.walkpath.path_pos++;
		if(pd.state.change_walk_target){
			pet_walktoxy_sub(pd);
			return 0;
		}
	}
	else {
		if(pd.walkpath.path[pd.walkpath.path_pos] >= 8)
			return 1;

		x = pd.bl.x;
		y = pd.bl.y;
/*		ctype = map_getcell(pd.bl.m,x,y);
		if(ctype == 1 || ctype == 5) {
			pet_stop_walking(pd,1);
			return 0;
		}*/
		pd.dir=pd.walkpath.path[pd.walkpath.path_pos];
		dx = dirx[pd.dir];
		dy = diry[pd.dir];

		if(map_getcell(pd.bl.m,x+dx,y+dy,CELL_CHKNOPASS)){
			pet_walktoxy_sub(pd);
			return 0;
		}

		moveblock = ( x/BLOCK_SIZE != (x+dx)/BLOCK_SIZE || y/BLOCK_SIZE != (y+dy)/BLOCK_SIZE);

		pd.state.state=MS_WALK;
		map_foreachinmovearea(clif_petoutsight,pd.bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,&pd);

		x += dx;
		y += dy;

		if(moveblock) map_delblock(pd.bl);
		pd.bl.x = x;
		pd.bl.y = y;
		if(moveblock) map_addblock(pd.bl);

		map_foreachinmovearea(clif_petinsight,pd.bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,&pd);
		pd.state.state=MS_IDLE;
	}
	if((i=calc_next_walk_step(pd))>0){
		i = i>>1;
		if(i < 1 && pd.walkpath.path_half == 0)
			i = 1;
		pd.timer=add_timer(tick+i,pet_timer,pd.bl.id,pd.walkpath.path_pos);
		pd.state.state=MS_WALK;

		if(pd.walkpath.path_pos >= pd.walkpath.path_len)
			clif_fixpetpos(&pd);
	}
	return 0;
}

int pet_stopattack(struct pet_data &pd)
{
	pd.target_id=0;
	if(pd.state.state == MS_ATTACK)
		pet_changestate(pd,MS_IDLE,0);

	return 0;
}

int pet_target_check(struct map_session_data &sd,struct block_list *bl,int type)
{
	struct pet_data *pd;
	struct mob_data *md;
	int rate,mode,race;

	pd = sd.pd;

	if(bl && pd && bl->type == BL_MOB && sd.pet.intimate > 900 && sd.pet.hungry > 0 && pd->class_ != status_get_class(bl)
		&& pd->state.state != MS_DELAY) {
		mode=mob_db[pd->class_].mode;
		race=mob_db[pd->class_].race;
		md=(struct mob_data *)bl;
		if(md->bl.type != BL_MOB || pd->bl.m != md->bl.m ||
			md->bl.prev == NULL ||
			distance(pd->bl.x,pd->bl.y,md->bl.x,md->bl.y) > 13 || 
			(md->class_ >= 1285 && md->class_ <= 1288)) // Cannot attack Guardians/Emperium
			return 0;
		if(mob_db[pd->class_].mexp <= 0 && !(mode&0x20) && (md->option & 0x06 && race!=4 && race!=6) )
			return 0;
		if(!type) {
			rate = sd.petDB->attack_rate;
			rate = rate * (150 - (sd.pet.intimate - 1000))/100;
			if(battle_config.pet_support_rate != 100)
				rate = rate*battle_config.pet_support_rate/100;
			if(sd.petDB->attack_rate > 0 && rate <= 0)
				rate = 1;
		}
		else {
			rate = sd.petDB->defence_attack_rate;
			rate = rate * (150 - (sd.pet.intimate - 1000))/100;
			if(battle_config.pet_support_rate != 100)
				rate = rate*battle_config.pet_support_rate/100;
			if(sd.petDB->defence_attack_rate > 0 && rate <= 0)
				rate = 1;
		}
		if(rand()%10000 < rate) 
		{
			if(pd->target_id == 0 || rand()%10000 < sd.petDB->change_target_rate)
				pd->target_id = bl->id;
		}
	}
	return 0;
}
/*==========================================
 * Pet SC Check [Skotlex]
 *------------------------------------------
 */
int pet_sc_check(struct map_session_data &sd, int type)
{	
	struct pet_data *pd;

	pd = sd.pd;
	if (pd == NULL ||
		(battle_config.pet_equip_required && pd->equip_id == 0) ||
		pd->recovery == NULL ||
		pd->recovery->timer != -1 ||
		pd->recovery->type != type)
		return 1;

	pd->recovery->timer = add_timer(gettick()+pd->recovery->delay*1000,pet_recovery_timer,sd.bl.id,0);
	
	return 0;
}

int pet_changestate(struct pet_data &pd,int state,int type)
{
	unsigned long tick;
	int i;

	if(pd.timer != -1)
	{
		delete_timer(pd.timer,pet_timer);
		pd.timer=-1;
	}
	pd.state.state=state;
	if( pd.state.casting_flag )
	{	//Skotlex: Cancel casting
		pd.state.casting_flag = 0;
		clif_skillcastcancel(&pd.bl);
	}
	switch(state)
	{
		case MS_WALK:
			if((i=calc_next_walk_step(pd)) > 0){
				i = i>>2;
				pd.timer=add_timer(gettick()+i,pet_timer,pd.bl.id,0);
			} else
				pd.state.state=MS_IDLE;
			break;
		case MS_ATTACK:
			tick = gettick();
			i=DIFF_TICK(pd.attackabletime,tick);
			if(i>0 && i<2000)
				pd.timer=add_timer(pd.attackabletime,pet_timer,pd.bl.id,0);
			else
				pd.timer=add_timer(tick+1,pet_timer,pd.bl.id,0);
			break;
		case MS_DELAY:
				pd.timer=add_timer(gettick()+type,pet_timer,pd.bl.id,0);
			break;
	}
	return 0;
}

static int pet_timer(int tid,unsigned long tick,int id,int data)
{
	struct pet_data *pd;

	pd=(struct pet_data*)map_id2bl(id);
	if(pd == NULL || pd->bl.type != BL_PET)
		return 1;

	if(pd->timer != tid){
		if(battle_config.error_log)
			ShowMessage("pet_timer %d != %d\n",pd->timer,tid);
		return 0;
	}
	pd->timer=-1;

	if(pd->bl.prev == NULL)
		return 1;

	switch(pd->state.state){
		case MS_WALK:
			pet_walk(*pd,tick,data);
			break;
		case MS_ATTACK:
			if (pd->msd == NULL) //Is this even possible?
				break;
			if (pc_isdead(*pd->msd))
			{	//Stop attacking when master died.
				pet_stopattack(*pd);
				break;
			}
			if (pd->state.casting_flag) 
			{	//There is a skill being cast.
				petskill_castend(*pd, tick, data);
				break;
			}
			if (battle_config.pet_status_support &&
				pd->a_skill &&
				(!battle_config.pet_equip_required || pd->equip_id > 0) &&
				(rand()%100 < (pd->a_skill->rate +pd->msd->pet.intimate*pd->a_skill->bonusrate/1000))
				)
			{	//Skotlex: Use pet's skill 
				pet_attackskill(*pd,tick,data);
				break;
			}
			pet_attack(*pd,tick,data);
			break;
		case MS_DELAY:
			pet_changestate(*pd,MS_IDLE,0);
			break;
		default:
			if(battle_config.error_log)
				ShowMessage("pet_timer : %d ?\n",pd->state.state);
			break;
	}

	return 0;
}

static int pet_walktoxy_sub(struct pet_data &pd)
{
	struct walkpath_data wpd;

	if(path_search(wpd,pd.bl.m,pd.bl.x,pd.bl.y,pd.to_x,pd.to_y,0))
		return 1;
	memcpy(&pd.walkpath,&wpd,sizeof(wpd));

	pd.state.change_walk_target=0;
	pet_changestate(pd,MS_WALK,0);
	clif_movepet(&pd);
	return 0;
}

int pet_walktoxy(struct pet_data &pd,int x,int y)
{
	struct walkpath_data wpd;

	if(pd.state.state == MS_WALK && path_search(wpd,pd.bl.m,pd.bl.x,pd.bl.y,x,y,0))
		return 1;

	pd.to_x=x;
	pd.to_y=y;

	if(pd.state.state == MS_WALK) {
		pd.state.change_walk_target=1;
	} else {
		return pet_walktoxy_sub(pd);
	}
	return 0;
}

int pet_stop_walking(struct pet_data &pd,int type)
{
	if(pd.state.state == MS_WALK || pd.state.state == MS_IDLE) {
		pd.walkpath.path_len=0;
		pd.to_x=pd.bl.x;
		pd.to_y=pd.bl.y;
	}
	if(type&0x01)
		clif_fixpetpos(&pd);
	if(type&~0xff)
		pet_changestate(pd,MS_DELAY,type>>8);
	else
		pet_changestate(pd,MS_IDLE,0);
	return 0;
}

static int pet_hungry(int tid,unsigned long tick,int id,int data)
{
	struct map_session_data *sd;
	int interval,t;

	sd=map_id2sd(id);
	if(sd==NULL)
		return 1;

	Assert((sd->status.pet_id == 0 || sd->pd == 0) || sd->pd->msd == sd); 

	if(sd->pet_hungry_timer != tid){
		if(battle_config.error_log)
			ShowMessage("pet_hungry_timer %d != %d\n",sd->pet_hungry_timer,tid);
		return 0;
	}
	
	sd->pet_hungry_timer = -1;

	if(!sd->status.pet_id || !sd->pd || !sd->petDB)
		return 1;

	sd->pet.hungry--;
	t = sd->pet.intimate;
	if(sd->pet.hungry < 0) {
		if(sd->pd->target_id > 0)
			pet_stopattack(*(sd->pd));
		sd->pet.hungry = 0;
		sd->pet.intimate -= battle_config.pet_hungry_friendly_decrease;
		if(sd->pet.intimate <= 0) {
			sd->pet.intimate = 0;
			if(battle_config.pet_status_support && t > 0) {
				if(sd->bl.prev != NULL)
					status_calc_pc(*sd,0);
				else
					status_calc_pc(*sd,2);
			}
		}
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

int pet_hungry_timer_delete(struct map_session_data &sd)
{
	if(sd.pet_hungry_timer != -1) {
		delete_timer(sd.pet_hungry_timer,pet_hungry);
		sd.pet_hungry_timer = -1;
	}
	return 0;
}

int pet_remove_map(struct map_session_data &sd)
{
	if(sd.status.pet_id && sd.pd)
	{
		struct pet_data *pd=sd.pd; // [Valaris]
		//[Skotlex] clear bonus data
		if (pd->status)
		{
			aFree(pd->status);
			pd->status = NULL;
		}
		if (pd->a_skill)
		{
			aFree(pd->a_skill);
			pd->a_skill = NULL;
		}
		if (pd->s_skill)
		{
			if (pd->s_skill->timer != -1)
				delete_timer(pd->s_skill->timer, pet_skill_support_timer);
			aFree(pd->s_skill);
			pd->s_skill = NULL;
		}
		if(pd->bonus)
		{
			if (pd->bonus->timer != -1)
				delete_timer(pd->bonus->timer, pet_skill_bonus_timer);
			aFree(pd->bonus);
			pd->bonus = NULL;
		}
		if (pd->loot)
		{
			if (pd->loot->item)
				aFree(pd->loot->item);
			aFree (pd->loot);
			pd->loot = NULL;
		}
		pd->state.skillbonus=-1;
		if(sd.perfect_hiding==1) sd.perfect_hiding=0;	// end additions

		pet_changestate(*(sd.pd),MS_IDLE,0);
		if(sd.pet_hungry_timer != -1)
			pet_hungry_timer_delete(sd);
		clif_clearchar_area(&sd.pd->bl,0);

		map_delblock(sd.pd->bl);
		map_deliddb(sd.pd->bl);
		aFree(sd.pd);
		sd.pd = NULL;
	}
	return 0;
}

int pet_performance(struct map_session_data &sd)
{
	struct pet_data *pd;
	nullpo_retr(0, pd=sd.pd);

	Assert((sd.status.pet_id == 0 || sd.pd == 0) || sd.pd->msd == sd); 

	pet_stop_walking(*pd,2000<<8);
	clif_pet_performance(&pd->bl,rand()%pet_performance_val(sd) + 1);
	// ���[�g����Item�𗎂Ƃ�����
	pet_lootitem_drop(*pd,NULL);

	return 0;
}

int pet_return_egg(struct map_session_data &sd)
{
	struct item tmp_item;
	int flag;

	if(sd.status.pet_id && sd.pd) {

		// ���[�g����Item�𗎂Ƃ�����
		pet_lootitem_drop(*(sd.pd),&sd);

		pet_remove_map(sd);
		sd.status.pet_id = 0;
		sd.pd = NULL;

		if(sd.petDB == NULL)
			return 1;
		sd.pet.incuvate = 1;
		memset(&tmp_item,0,sizeof(tmp_item));
		tmp_item.nameid = sd.petDB->EggID;
		tmp_item.identify = 1;
		tmp_item.card[0] = 0xff00;
		tmp_item.card[1] = GetWord(sd.pet.pet_id,0);
		tmp_item.card[2] = GetWord(sd.pet.pet_id,1);
		tmp_item.card[3] = sd.pet.rename_flag;
		if((flag = pc_additem(sd,tmp_item,1))) {
			clif_additem(&sd,0,0,flag);
			map_addflooritem(tmp_item,1,sd.bl.m,sd.bl.x,sd.bl.y,NULL,NULL,NULL,0);
		}
		if(battle_config.pet_status_support && sd.pet.intimate > 0) {
			if(sd.bl.prev != NULL)
				status_calc_pc(sd,0);
			else
				status_calc_pc(sd,2);
		}

		intif_save_petdata(sd.status.account_id,sd.pet);
		pc_makesavestatus(sd);
		chrif_save(sd);
		storage_storage_save(sd);

		sd.petDB = NULL;
	}

	return 0;
}

int pet_data_init(struct map_session_data &sd)
{
	struct pet_data *pd;
	int i=0,interval=0;

	if(sd.status.account_id != sd.pet.account_id || sd.status.char_id != sd.pet.char_id ||
		sd.status.pet_id != sd.pet.pet_id) {
		sd.status.pet_id = 0;
		return 1;
	}

	i = search_petDB_index(sd.pet.class_,PET_CLASS);
	if(i < 0) {
		sd.status.pet_id = 0;
		return 1;
	}
	sd.petDB = &pet_db[i];
	sd.pd = pd = (struct pet_data *)aCalloc(1,sizeof(struct pet_data));

	pd->bl.m = sd.bl.m;
//	pd->bl.prev = pd->bl.next = NULL;
	pd->bl.x = pd->to_x = sd.bl.x;
	pd->bl.y = pd->to_y = sd.bl.y;
	pet_calc_pos(*pd,sd.bl.x,sd.bl.y,sd.dir);
	pd->bl.x = pd->to_x;
	pd->bl.y = pd->to_y;
	pd->bl.id = npc_get_new_npc_id();
//	memcpy(pd->name,sd->pet.name,24);
	pd->namep = sd.pet.name;
	pd->class_ = sd.pet.class_;
	pd->equip_id = sd.pet.equip_id;
	pd->dir = sd.dir;
	pd->speed = sd.petDB->speed;
	pd->bl.subtype = MONS;
	pd->bl.type = BL_PET;
	pd->state.state = MS_IDLE;
//	pd->state.change_walk_target = 0;
	pd->timer = -1;
//	pd->target_id = 0;
//	pd->move_fail_count = 0;
	pd->next_walktime = pd->attackabletime = pd->last_thinktime = gettick();
	pd->msd = &sd;
	
	map_addiddb(pd->bl);

	// initialise
	if (battle_config.pet_lv_rate)
	{ 	//Skotlex
		pd->status = (struct pet_data::pet_status *) aCalloc(1,sizeof(struct pet_data::pet_status));
		status_calc_pet(sd,true);
	}

	pd->state.skillbonus = -1;
	if (battle_config.pet_status_support) //Skotlex
		run_script(pet_db[i].script,0,sd.bl.id,0);

	if(sd.pet_hungry_timer != -1)
		pet_hungry_timer_delete(sd);
	if(battle_config.pet_hungry_delay_rate != 100)
		interval = (sd.petDB->hungry_delay*battle_config.pet_hungry_delay_rate)/100;
	else
		interval = sd.petDB->hungry_delay;
	if(interval <= 0)
		interval = 1;
	sd.pet_hungry_timer = add_timer(gettick()+interval,pet_hungry,sd.bl.id,0);

	return 0;
}

int pet_birth_process(struct map_session_data &sd)
{

	Assert((sd.status.pet_id == 0 || sd.pd == 0) || sd.pd->msd == sd); 

	if(sd.status.pet_id && sd.pet.incuvate == 1) {
		sd.status.pet_id = 0;
		return 1;
	}

	sd.pet.incuvate = 0;
	sd.pet.account_id = sd.status.account_id;
	sd.pet.char_id = sd.status.char_id;
	sd.status.pet_id = sd.pet.pet_id;
	if(pet_data_init(sd)) {
		sd.status.pet_id = 0;
		sd.pet.incuvate = 1;
		sd.pet.account_id = 0;
		sd.pet.char_id = 0;
		return 1;
	}

	intif_save_petdata(sd.status.account_id,sd.pet);
	pc_makesavestatus(sd);
	chrif_save(sd);
	storage_storage_save(sd);
	map_addblock(sd.pd->bl);
	clif_spawnpet(sd.pd);
	clif_send_petdata(&sd,0,0);
	clif_send_petdata(&sd,5,0x14);
	clif_pet_equip(sd.pd,sd.pet.equip_id);
	clif_send_petstatus(&sd);

	Assert((sd.status.pet_id == 0 || sd.pd == 0) || sd.pd->msd == sd); 

	return 0;
}

int pet_recv_petdata(unsigned long account_id,struct s_pet &p,int flag)
{
	struct map_session_data *sd;

	sd = map_id2sd(account_id);
	if(sd == NULL)
		return 1;
	if(flag == 1) {
		sd->status.pet_id = 0;
		return 1;
	}
	memcpy(&sd->pet,&p,sizeof(struct s_pet));
	if(sd->pet.incuvate == 1)
		pet_birth_process(*sd);
	else {
		pet_data_init(*sd);
		if(sd->pd && sd->bl.prev != NULL) {
			map_addblock(sd->pd->bl);
			clif_spawnpet(sd->pd);
			clif_send_petdata(sd,0,0);
			clif_send_petdata(sd,5,0x14);
//			clif_pet_equip(sd->pd,sd->pet.equip);
			clif_send_petstatus(sd);
		}
	}
	if(battle_config.pet_status_support && sd->pet.intimate > 0) {
		if(sd->bl.prev != NULL)
			status_calc_pc(*sd,0);
		else
			status_calc_pc(*sd,2);
	}

	return 0;
}

int pet_select_egg(struct map_session_data &sd,short egg_index)
{
	if(sd.status.inventory[egg_index].card[0] == 0xff00)
	{
		intif_request_petdata(sd.status.account_id, sd.status.char_id, MakeDWord(sd.status.inventory[egg_index].card[1], sd.status.inventory[egg_index].card[2]) );
		pc_delitem(sd,egg_index,1,0);
	}
	else {
		if(battle_config.error_log)
			ShowMessage("wrong egg item inventory %d\n",egg_index);
	}
	return 0;
}

int pet_catch_process1(struct map_session_data &sd,int target_class)
{
	sd.catch_target_class = target_class;
	clif_catch_process(&sd);

	return 0;
}

int pet_catch_process2(struct map_session_data &sd,int target_id)
{
	struct mob_data *md;
	int i=0,pet_catch_rate=0;

	if(sd.itemid > 0)
	{	//Consume the pet lure [Skotlex]
		if ((i = sd.itemindex) == -1 ||
			sd.status.inventory[i].nameid != sd.itemid ||
			!sd.inventory_data[i]->flag.delay_consume ||
			sd.status.inventory[i].amount < 1	
			)
		{	//Something went wrong, items moved or they tried an exploit.
			clif_pet_rulet(&sd,0);
			return 1;
		}
		//Delete the item
		sd.itemid = sd.itemindex = -1;
		pc_delitem(sd,i,1,0);
	}
	
	md=(struct mob_data*)map_id2bl(target_id);
	if(!md){
		clif_pet_rulet(&sd,0);
		return 1;
	}

	i = search_petDB_index(md->class_,PET_CLASS);
	if(md == NULL || md->bl.type != BL_MOB || md->bl.prev == NULL || i < 0 || sd.catch_target_class != md->class_) {
		clif_pet_rulet(&sd,0);
		return 1;
	}

	//target_id�ɂ��G��������
//	if(battle_config.etc_log)
//		ShowMessage("mob_id = %d, mob_class = %d\n",md->bl.id,md->class_);
		//�����̏ꍇ
	pet_catch_rate = (pet_db[i].capture + (sd.status.base_level - mob_db[md->class_].lv)*30 + sd.paramc[5]*20)*(200 - md->hp*100/mob_db[md->class_].max_hp)/100;
	if(pet_catch_rate < 1) pet_catch_rate = 1;
	if(battle_config.pet_catch_rate != 100)
		pet_catch_rate = (pet_catch_rate*battle_config.pet_catch_rate)/100;

	if(rand()%10000 < pet_catch_rate) {
		mob_remove_map(md,0);
		mob_setdelayspawn(md->bl.id);
		clif_pet_rulet(&sd,1);
//		if(battle_config.etc_log)
//			ShowMessage("rulet success %d\n",target_id);
		intif_create_pet(
			sd.status.account_id,sd.status.char_id,
			pet_db[i].class_,mob_db[pet_db[i].class_].lv,
			pet_db[i].EggID,0,pet_db[i].intimate,100,
			0,1,
			pet_db[i].jname);
	}
	else
		clif_pet_rulet(&sd,0);

	return 0;
}

int pet_get_egg(unsigned long account_id,unsigned long pet_id,int flag)
{
	struct map_session_data *sd;
	struct item tmp_item;
	int i=0,ret=0;

	if(!flag) {
		sd = map_id2sd(account_id);
		if(sd == NULL)
			return 1;

		i = search_petDB_index(sd->catch_target_class,PET_CLASS);
		if(i >= 0) {
			memset(&tmp_item,0,sizeof(tmp_item));
			tmp_item.nameid = pet_db[i].EggID;
			tmp_item.identify = 1;
			tmp_item.card[0] = 0xff00;
			tmp_item.card[1] = GetWord(pet_id,0);
			tmp_item.card[2] = GetWord(pet_id,1);
			tmp_item.card[3] = sd->pet.rename_flag;
			if((ret = pc_additem(*sd,tmp_item,1))) {
				clif_additem(sd,0,0,ret);
				map_addflooritem(tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
			}
		}
		else
			intif_delete_petdata(pet_id);
	}

	return 0;
}

int pet_menu(struct map_session_data &sd,int menunum)
{
	switch(menunum) {
		case 0:
			clif_send_petstatus(&sd);
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

int pet_change_name(struct map_session_data &sd, const char *name)
{
	int i;
	

	if((sd.pd == NULL) || (sd.pet.rename_flag == 1 && battle_config.pet_rename == 0))
		return 1;

	for(i=0;i<24 && name[i];i++){
		if( !(name[i]&0xe0) || name[i]==0x7f)
			return 1;
	}

	pet_stop_walking(*(sd.pd),1);
	memcpy(sd.pet.name,name,24);
	//memcpy(sd->pd->name,name,24);
	clif_clearchar_area(&sd.pd->bl,0);
	clif_spawnpet(sd.pd);
	clif_send_petdata(&sd,0,0);
	clif_send_petdata(&sd,5,0x14);
	sd.pet.rename_flag = 1;
	clif_pet_equip(sd.pd,sd.pet.equip_id);
	clif_send_petstatus(&sd);

	return 0;
}

int pet_equipitem(struct map_session_data &sd,int index)
{
	unsigned short nameid;

	nameid = sd.status.inventory[index].nameid;
	if(sd.petDB == NULL)
		return 1;
	if(sd.petDB->AcceID == 0 || nameid != sd.petDB->AcceID || sd.pet.equip_id != 0)
	{
		clif_equipitemack(&sd,0,0,0);
		return 1;
	}
	else
	{
		pc_delitem(sd,index,1,0);
		sd.pet.equip_id = sd.pd->equip_id = nameid;
		status_calc_pc(sd,0);
		clif_pet_equip(sd.pd,nameid);
		if (battle_config.pet_equip_required)
		{ 	//Skotlex: start support timers if needd
			if (sd.pd->s_skill && sd.pd->s_skill->timer == -1)
				sd.pd->s_skill->timer=add_timer(gettick()+sd.pd->s_skill->delay*1000, pet_skill_support_timer, sd.bl.id, 0);
			if (sd.pd->bonus && sd.pd->bonus->timer == -1)
				sd.pd->bonus->timer=add_timer(gettick()+sd.pd->bonus->delay*1000, pet_skill_bonus_timer, sd.bl.id, 0);
		}
	}
	return 0;
}

int pet_unequipitem(struct map_session_data &sd)
{
	struct item tmp_item;
	unsigned short nameid;
	int flag;

	if(sd.petDB == NULL)
		return 1;
	if(sd.pet.equip_id == 0)
		return 1;

	nameid = sd.pet.equip_id;
	sd.pet.equip_id = sd.pd->equip_id = 0;
	status_calc_pc(sd,0);
	clif_pet_equip(sd.pd,0);
	memset(&tmp_item,0,sizeof(tmp_item));
	tmp_item.nameid = nameid;
	tmp_item.identify = 1;
	if((flag = pc_additem(sd,tmp_item,1))) {
		clif_additem(&sd,0,0,flag);
		map_addflooritem(tmp_item,1,sd.bl.m,sd.bl.x,sd.bl.y,NULL,NULL,NULL,0);
	}
	if (battle_config.pet_equip_required)
	{ 	//Skotlex: halt support timers if needed
		if (sd.pd->s_skill && sd.pd->s_skill->timer != -1)
		{
			if (sd.pd->s_skill->id == 0)
				delete_timer(sd.pd->s_skill->timer, pet_heal_timer);
			else
				delete_timer(sd.pd->s_skill->timer, pet_skill_support_timer);
		}
		if (sd.pd->bonus && sd.pd->bonus->timer != -1)
			delete_timer(sd.pd->bonus->timer, pet_skill_bonus_timer);
	}

	return 0;
}

int pet_food(struct map_session_data &sd)
{
	int i,k,t;

	if(sd.petDB == NULL)
		return 1;
	i=pc_search_inventory(sd,sd.petDB->FoodID);
	if(i < 0) {
		clif_pet_food(&sd,sd.petDB->FoodID,0);
		return 1;
	}
	pc_delitem(sd,i,1,0);
	t = sd.pet.intimate;
	if(sd.pet.hungry > 90)
		sd.pet.intimate -= sd.petDB->r_full;
	else if(sd.pet.hungry > 75) {
		if(battle_config.pet_friendly_rate != 100)
			k = (sd.petDB->r_hungry * battle_config.pet_friendly_rate)/100;
		else
			k = sd.petDB->r_hungry;
		k = k >> 1;
		if(k <= 0)
			k = 1;
		sd.pet.intimate += k;
	}
	else {
		if(battle_config.pet_friendly_rate != 100)
			k = (sd.petDB->r_hungry * battle_config.pet_friendly_rate)/100;
		else
			k = sd.petDB->r_hungry;
		sd.pet.intimate += k;
	}
	if(sd.pet.intimate <= 0) {
		sd.pet.intimate = 0;
		if(battle_config.pet_status_support && t > 0) {
			if(sd.bl.prev != NULL)
				status_calc_pc(sd,0);
			else
				status_calc_pc(sd,2);
		}
	}
	else if(sd.pet.intimate > 1000)
		sd.pet.intimate = 1000;
	sd.pet.hungry += sd.petDB->fullness;
	if(sd.pet.hungry > 100)
		sd.pet.hungry = 100;

	clif_send_petdata(&sd,2,sd.pet.hungry);
	clif_send_petdata(&sd,1,sd.pet.intimate);
	clif_pet_food(&sd,sd.petDB->FoodID,1);

	return 0;
}

static int pet_randomwalk(struct pet_data &pd,unsigned long tick)
{
	const int retrycount=20;
	int speed;

	speed = status_get_speed(&pd.bl);

	if(DIFF_TICK(pd.next_walktime,tick) < 0){
		int i,x,y,c,d=12-pd.move_fail_count;
		if(d<5) d=5;
		for(i=0;i<retrycount;i++){
			int r=rand();
			x=pd.bl.x+r%(d*2+1)-d;
			y=pd.bl.y+r/(d*2+1)%(d*2+1)-d;
			if((map_getcell(pd.bl.m,x,y,CELL_CHKPASS))&&( pet_walktoxy(pd,x,y)==0)){
				pd.move_fail_count=0;
				break;
			}
			if(i+1>=retrycount){
				pd.move_fail_count++;
				if(pd.move_fail_count>1000){
					if(battle_config.error_log)
						ShowMessage("PET cant move. hold position %d, class_ = %d\n",pd.bl.id,pd.class_);
					pd.move_fail_count=0;
					pet_changestate(pd,MS_DELAY,60000);
					return 0;
				}
			}
		}
		for(i=c=0;i<pd.walkpath.path_len;i++){
			if(pd.walkpath.path[i]&1)
				c+=speed*14/10;
			else
				c+=speed;
		}
		pd.next_walktime = tick+rand()%3000+3000+c;

		return 1;
	}
	return 0;
}

static int pet_ai_sub_hard(struct pet_data &pd,unsigned long tick)
{
	struct map_session_data *sd = pd.msd;
	struct mob_data *md = NULL;
	int dist,i=0,dx,dy,ret;
	int mode,race;

	if( pd.bl.prev == NULL || sd == NULL || sd->bl.prev == NULL )
		return 0;
	if( sd->status.pet_id == 0 || sd->pd == 0 || sd->pd->msd == sd)
		return 0;

	if(DIFF_TICK(tick,pd.last_thinktime) < MIN_PETTHINKTIME)
		return 0;

	pd.last_thinktime=tick;

	if(pd.state.state == MS_DELAY || pd.bl.m != sd->bl.m)
		return 0;

	// �y�b�g�ɂ�郋�[�g
	if(!pd.target_id && pd.loot && pd.loot->count < pd.loot->max && DIFF_TICK(gettick(),pd.loot->loottick)>0)
	{
		map_foreachinarea(pet_ai_sub_hard_lootsearch,pd.bl.m,
						  pd.bl.x-AREA_SIZE*2,pd.bl.y-AREA_SIZE*2,
						  pd.bl.x+AREA_SIZE*2,pd.bl.y+AREA_SIZE*2,
						  BL_ITEM,&pd,&i);
	}

	if(sd->pet.intimate > 0)
	{
		dist = distance(sd->bl.x,sd->bl.y,pd.bl.x,pd.bl.y);
		if(dist > 12)
		{
			if(pd.target_id > 0)
				pet_unlocktarget(pd);
			if(pd.timer != -1 && pd.state.state == MS_WALK && distance(pd.to_x,pd.to_y,sd->bl.x,sd->bl.y) < 3)
				return 0;
			pd.speed = (sd->speed>>1);
			if(pd.speed <= 0)
				pd.speed = 1;
			pet_calc_pos(pd,sd->bl.x,sd->bl.y,sd->dir);
			if(pet_walktoxy(pd,pd.to_x,pd.to_y))
				pet_randomwalk(pd,tick);
		}
		else if(pd.target_id - MAX_FLOORITEM > 0)
		{	//Mob targeted
			mode=mob_db[pd.class_].mode;
			race=mob_db[pd.class_].race;
			md=(struct mob_data *)map_id2bl(pd.target_id);
			if( md == NULL /*|| md->bl.type != BL_MOB*/ || 
				pd.bl.m != md->bl.m || md->bl.prev == NULL ||
				distance(pd.bl.x,pd.bl.y,md->bl.x,md->bl.y) > 13)
			{
				pet_unlocktarget(pd);
			}
//			else if(mob_db[pd->class_].mexp <= 0 && !(mode&0x20) && (md->option & 0x06 && race!=4 && race!=6) )
//				pet_unlocktarget(pd);
			else if(!battle_check_range(&pd.bl,&md->bl,mob_db[pd.class_].range && !pd.state.casting_flag))
			{	//Skotlex Don't interrupt a casting spell when targed moved
				if(pd.timer != -1 && pd.state.state == MS_WALK && distance(pd.to_x,pd.to_y,md->bl.x,md->bl.y) < 2)
					return 0;
				if( !pet_can_reach(pd,md->bl.x,md->bl.y))
					pet_unlocktarget(pd);
				else
				{
					i=0;
					pd.speed = status_get_speed(&pd.bl);
					do
					{
						if(i==0)
						{	// �ŏ���AEGIS�Ɠ������@�Ō���
							dx=md->bl.x - pd.bl.x;
							dy=md->bl.y - pd.bl.y;
							if(dx<0)		dx++;
							else if(dx>0)	dx--;
							if(dy<0)		dy++;
							else if(dy>0)	dy--;
						}
						else
						{	// ���߂Ȃ�Athena��(�����_��)
							dx=md->bl.x - pd.bl.x + rand()%3 - 1;
							dy=md->bl.y - pd.bl.y + rand()%3 - 1;
						}
						ret=pet_walktoxy(pd,pd.bl.x+dx,pd.bl.y+dy);
						i++;
					} while(ret && i<5);

					if(ret)
					{	// �ړ��s�\�ȏ�����̍U���Ȃ�2������
						if(dx<0)		dx=2;
						else if(dx>0)	dx=-2;
						if(dy<0)		dy=2;
						else if(dy>0)	dy=-2;
						pet_walktoxy(pd,pd.bl.x+dx,pd.bl.y+dy);
					}
				}
			}
			else
			{
				if(pd.state.state==MS_WALK)
					pet_stop_walking(pd,1);
				if(pd.state.state==MS_ATTACK)
					return 0;
				pet_changestate(pd,MS_ATTACK,0);
			}
		}
		else if(pd.target_id > 0 && pd.loot)
		{	//Item Targeted, attempt loot
			struct block_list *bl_item = map_id2bl(pd.target_id);
			dist=distance(pd.bl.x,pd.bl.y,bl_item->x,bl_item->y);
			if(bl_item == NULL || bl_item->type != BL_ITEM || bl_item->m != pd.bl.m || dist>=5)
			{
				 // �������邩�A�C�e�����Ȃ��Ȃ���
 				pet_unlocktarget(pd);
			}
			else if(dist)
			{
				if(pd.timer != -1 && pd.state.state!=MS_ATTACK && (DIFF_TICK(pd.next_walktime,tick)<0 || distance(pd.to_x,pd.to_y,bl_item->x,bl_item->y) <= 0))
					return 0; // ���Ɉړ���

				pd.next_walktime=tick+500;
				//dx=bl_item->x - pd.bl.x;
				//dy=bl_item->y - pd.bl.y;
				//ret=pet_walktoxy(pd,pd->bl.x+dx,pd->bl.y+dy);
				ret=pet_walktoxy(pd, bl_item->x, bl_item->y);
			}
			else
			{	// �A�C�e���܂ł��ǂ蒅����
				if(pd.state.state==MS_ATTACK)
					return 0; // �U����

				if(pd.state.state==MS_WALK)
				{	// ���s���Ȃ��~
					pet_stop_walking(pd,1);
				}

				if(pd.loot && pd.loot->count < pd.loot->max)
				{
					struct flooritem_data *fitem = (struct flooritem_data *)bl_item;
					memcpy(&pd.loot->item[pd.loot->count++], &fitem->item_data, sizeof(struct item));
					pd.loot->weight += itemdb_search(fitem->item_data.nameid)->weight*fitem->item_data.amount;
					map_clearflooritem(bl_item->id);
				}
				pet_unlocktarget(pd);
			}
		}
		else
		{
			if(dist <= 3 || (pd.timer != -1 && pd.state.state == MS_WALK && distance(pd.to_x,pd.to_y,sd->bl.x,sd->bl.y) < 3) )
				return 0;
			pd.speed = status_get_speed(&pd.bl);
			pet_calc_pos(pd,sd->bl.x,sd->bl.y,sd->dir);
			if(pet_walktoxy(pd,pd.to_x,pd.to_y))
				pet_randomwalk(pd,tick);
		}
	}
	else
	{
		pd.speed = status_get_speed(&pd.bl);
		if(pd.state.state == MS_ATTACK)
			pet_stopattack(pd);
		pet_randomwalk(pd,tick);
	}
	return 0;
}

static int pet_ai_sub_foreachclient(struct map_session_data &sd,va_list ap)
{
	unsigned long tick;

	nullpo_retr(0, ap);
	tick=(unsigned long)va_arg(ap,int);

	if(sd.status.pet_id && sd.pd && sd.petDB)
		pet_ai_sub_hard(*sd.pd, tick);

	return 0;
}

static int pet_ai_hard(int tid,unsigned long tick,int id,int data)
{
	clif_foreachclient(pet_ai_sub_foreachclient,tick);

	return 0;
}

int pet_ai_sub_hard_lootsearch(struct block_list *bl,va_list ap)
{
	struct pet_data* pd;
	int dist,*itc;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, pd=va_arg(ap,struct pet_data *));
	nullpo_retr(0, itc=va_arg(ap,int *));

	if(!pd->target_id){
		struct flooritem_data *fitem = (struct flooritem_data *)bl;
		struct map_session_data *sd = NULL;
		// ���[�g������
		if(fitem && fitem->first_get_id>0)
			sd = map_id2sd(fitem->first_get_id);

		if(pd->loot == NULL || pd->loot->item == NULL || (pd->loot->count >= pd->loot->max) || (sd && sd->pd != pd))
			return 0;
		if(bl->m == pd->bl.m && (dist=distance(pd->bl.x,pd->bl.y,bl->x,bl->y))<5){
			if( pet_can_reach(*pd,bl->x,bl->y)		// ���B�\������
				 && rand()%1000<1000/(++(*itc)) ){	// �͈͓�PC�œ��m���ɂ���
				pd->target_id=bl->id;
			}
		}
	}
	return 0;
}
int pet_lootitem_drop(struct pet_data &pd,struct map_session_data *sd)
{
	size_t i,flag=0;
	if(pd.loot)
	{
		for(i=0;i<pd.loot->count;i++)
		{
			struct delay_item_drop2 *ditem;

			ditem=(struct delay_item_drop2 *)aCalloc(1,sizeof(struct delay_item_drop2));
			memcpy(&ditem->item_data,&pd.loot->item[i],sizeof(struct item));
			ditem->m = pd.bl.m;
			ditem->x = pd.bl.x;
			ditem->y = pd.bl.y;
			ditem->first_sd = 0;
			ditem->second_sd = 0;
			ditem->third_sd = 0;
			// ���Ƃ��Ȃ��Œ���PC��Item����
			if(sd){
				if((flag = pc_additem(*sd,ditem->item_data,ditem->item_data.amount))){
					clif_additem(sd,0,0,flag);
					map_addflooritem(ditem->item_data,ditem->item_data.amount,ditem->m,ditem->x,ditem->y,ditem->first_sd,ditem->second_sd,ditem->third_sd,0);
				}
				aFree(ditem);
			}
			else
				add_timer(gettick()+540+i,pet_delay_item_drop2,(int)ditem,0);
		}
		//The smart thing to do is use pd->loot->max (thanks for pointing it out, Shinomori)
		memset(pd.loot->item,0,pd.loot->max * sizeof(struct item));
		pd.loot->count = 0;
		pd.loot->weight = 0;
		pd.loot->loottick = gettick()+10000;	//	10*1000ms�̊ԏE��Ȃ�
	}
	return 1;
}

int pet_delay_item_drop2(int tid,unsigned long tick,int id,int data)
{
	struct delay_item_drop2 *ditem;

	ditem=(struct delay_item_drop2 *)id;

	map_addflooritem(ditem->item_data,ditem->item_data.amount,ditem->m,ditem->x,ditem->y,ditem->first_sd,ditem->second_sd,ditem->third_sd,0);

	aFree(ditem);
	return 0;
}

/*==========================================
 * pet bonus giving skills [Valaris] / Rewritten by [Skotlex]
 *------------------------------------------
 */ 
int pet_skill_bonus_timer(int tid,unsigned long tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	struct pet_data *pd;
	int timer = 0;

	if(sd == NULL || sd->pd==NULL || sd->pd->bonus == NULL)
		return 1;
	
	pd=sd->pd;

	if(pd->bonus->timer != tid) {
		if(battle_config.error_log)
			printf("pet_skill_bonus_timer %d != %d\n",pd->bonus->timer,tid);
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
		timer = (pd->bonus->delay - pd->bonus->duration)*1000;	// the duration until pet bonuses will be reactivated again
		if (timer < 0) //Always active bonus
			timer = MIN_PETTHINKTIME; 
	}

	// add/remove our bonuses
	status_calc_pc(*sd, 0);

	// wait for the next timer
	pd->bonus->timer=add_timer(tick+timer,pet_skill_bonus_timer,sd->bl.id,0);
	
	return 0;
}

int pet_recovery_timer(int tid,unsigned long tick,int id,int data)
{
	struct map_session_data *sd=(struct map_session_data*)map_id2bl(id);
	struct pet_data *pd;
	
	if(sd==NULL || sd->pd == NULL || sd->pd->recovery == NULL)
		return 1;
	
	pd=sd->pd;

	if(pd->recovery->timer != tid) {
		if(battle_config.error_log)
			printf("pet_recovery_timer %d != %d\n",pd->recovery->timer,tid);
		return 0;
	}

	if(sd->sc_data && sd->sc_data[pd->recovery->type].timer != -1)
	{
		status_change_end(&sd->bl,pd->recovery->type,-1);
		clif_emotion(&pd->bl, 33);
	}

	pd->recovery->timer = -1;
	
	return 0;
}

int pet_heal_timer(int tid,unsigned long tick,int id,int data)
{
	struct map_session_data *sd=(struct map_session_data*)map_id2bl(id);
	struct pet_data *pd;
	
	if(sd==NULL || sd->bl.type!=BL_PC || sd->pd == NULL)
		return 1;
	
	pd=sd->pd;

	if(pd->s_skill->timer != tid) {
		if(battle_config.error_log)
			printf("pet_heal_timer %d != %d\n",pd->s_skill->timer,tid);
		return 0;
	}
	
	if(pc_isdead(*sd) ||
		pd->state.casting_flag || //Another skill is in effect
		pd->state.state == MS_WALK || //Better wait until the pet stops moving
		sd->status.hp > sd->status.max_hp * pd->s_skill->hp/100 ||
		sd->status.sp > sd->status.max_sp * pd->s_skill->sp/100)
	{	//Wait (how long? 30x Min PetThinkTime (3sec) is fair enough?)
		pd->s_skill->timer=add_timer(gettick()+30*MIN_PETTHINKTIME,pet_heal_timer,sd->bl.id,0);
		return 0;
	}

	if (pd->state.state == MS_ATTACK)
			pet_stopattack(*pd);
	clif_skill_nodamage(&pd->bl,&sd->bl,AL_HEAL,pd->s_skill->lv,1);
	pc_heal(sd,pd->s_skill->lv,0);
	
	pd->s_skill->timer=add_timer(tick+pd->s_skill->delay*1000,pet_heal_timer,sd->bl.id,0);
	
	return 0;
}
/*
int pet_mag_timer(int tid,unsigned long tick,int id,int data)
{
	struct pet_data *pd;
	struct map_session_data *sd=(struct map_session_data*)map_id2bl(id);
	
	if(sd==NULL || sd->bl.type!=BL_PC)
		return 1;
	
	pd=sd->pd;

	if(pd==NULL || pd->bl.type!=BL_PET)
		return 1;

	if(pd->skillbonustimer != tid)
		return 0;

	if(sd->status.hp < sd->status.max_hp * pd->skilltype/100 && sd->status.sp < sd->status.max_sp * pd->skillduration/100) {
		clif_skill_nodamage(&pd->bl,&sd->bl,PR_MAGNIFICAT,pd->skillval,1);
		status_change_start(&sd->bl,SkillStatusChangeTable[PR_MAGNIFICAT],pd->skillval,0,0,0,skill_get_time(PR_MAGNIFICAT,pd->skillval),0 );
  	}
	
	pd->skillbonustimer=add_timer(gettick()+pd->skilltimer*1000,pet_mag_timer,sd->bl.id,0);
	
	return 0;
}
*/
/*
int pet_skillattack_timer(int tid,unsigned long tick,int id,int data)
{
	struct mob_data *md;
	struct map_session_data *sd=(struct map_session_data*)map_id2bl(id);
	struct pet_data *pd;

	if(sd==NULL || sd->bl.type!=BL_PC)
		return 1;
	
	pd=sd->pd;

	if(pd==NULL || pd->bl.type!=BL_PET)
		return 1;

	if(pd->skillbonustimer != tid)
		return 0;

	md=(struct mob_data *)map_id2bl(sd->attacktarget);
	if(md == NULL || md->bl.type != BL_MOB || pd->bl.m != md->bl.m || md->bl.prev == NULL ||
		distance(pd->bl.x,pd->bl.y,md->bl.x,md->bl.y) > 6) {
		pd->target_id=0;
		pd->skillbonustimer=add_timer(gettick()+100,pet_skillattack_timer,sd->bl.id,pd->skillduration);
		return 0;
	}

	if(md && rand()%100 < sd->pet.intimate*pd->skilltimer/100 ) {
		switch(pd->skilltype)
		{
		case SM_PROVOKE:
		//case NPC_POISON: poison is not handled there
			skill_castend_nodamage_id(&pd->bl,&md->bl,pd->skilltype,pd->skillval,tick,0);
			break;
		case BS_HAMMERFALL:
			skill_castend_pos2(&pd->bl,md->bl.x,md->bl.y,pd->skilltype,pd->skillval,tick,0);
			break;
		case WZ_HEAVENDRIVE:
			skill_castend_pos2(&pd->bl,md->bl.x,md->bl.y,pd->skilltype,pd->skillval+rand()%100,tick,0);
			break;
		default:
if(pd->skilltype<0)
printf("pet_skillattack_timer negative skill trap 1\n");

			skill_castend_damage_id(&pd->bl,&md->bl,pd->skilltype,pd->skillval,tick,0);
			break;
		}
		pd->skillbonustimer=add_timer(gettick()+1000,pet_skillattack_timer,sd->bl.id,0);
		return 0;
	}

	pd->skillbonustimer=add_timer(gettick()+100,pet_skillattack_timer,sd->bl.id,0);
	
	return 0;
}
*/
/*==========================================
 * pet support skills [Skotlex]
 *------------------------------------------
 */ 
int pet_skill_support_timer(int tid,unsigned long tick,int id,int data)
{
	struct map_session_data *sd=(struct map_session_data*)map_id2bl(id);
	struct pet_data *pd;
	short rate = 100;	
	if(sd==NULL || sd->bl.type!=BL_PC || sd->pd == NULL)
		return 1;
	
	pd=sd->pd;

	if(pd->s_skill->timer != tid) {
		if(battle_config.error_log)
			printf("pet_skill_support_timer %d != %d\n",pd->s_skill->timer,tid);
		return 0;
	}
	
	if(pc_isdead(*sd) ||
		(rate = sd->status.sp*100/sd->status.max_sp) > pd->s_skill->sp ||
		(rate = sd->status.hp*100/sd->status.max_hp) > pd->s_skill->hp ||
		(rate = pd->state.casting_flag) || //Another skill is in effect
		(rate = pd->state.state) == MS_WALK) //Better wait until the pet stops moving (MS_WALK is 2)
	{  //Wait (how long? 1 sec for every 10% of remaining)
		pd->s_skill->timer=add_timer(gettick()+(rate>10?rate:10)*100,pet_skill_support_timer,sd->bl.id,0);
		return 0;
	}

	if (pd->state.state == MS_ATTACK)
		pet_stopattack(*pd);
	petskill_use(*pd, sd->bl, pd->s_skill->id, pd->s_skill->lv, tick);

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
	unsigned short nameid;
	size_t i,k,j=0;
	int lines;
	char *filename[]={"db/pet_db.txt","db/pet_db2.txt"};
	char *str[32],*p,*np;
	
	memset(pet_db,0,sizeof(pet_db));
	for(i=0;i<2;i++){
		fp=savefopen(filename[i],"r");
		if(fp==NULL){
			if(i>0)
				continue;
			ShowMessage("can't read %s\n",filename[i]);
			return -1;
		}
		lines = 0;
		while(fgets(line,1020,fp)){
			
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
			memcpy(pet_db[j].name,str[1],24);
			memcpy(pet_db[j].jname,str[2],24);
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
	read_petdb();

	add_timer_func_list(pet_timer,"pet_timer");
	add_timer_func_list(pet_hungry,"pet_hungry");
	add_timer_func_list(pet_ai_hard,"pet_ai_hard");
	add_timer_func_list(pet_skill_bonus_timer,"pet_skill_bonus_timer"); // [Valaris]
	add_timer_func_list(pet_skill_support_timer, "pet_skill_support_timer"); // [Skotlex]
	add_timer_func_list(pet_recovery_timer,"pet_recovery_timer"); // [Valaris]
//	add_timer_func_list(pet_mag_timer,"pet_mag_timer"); // [Valaris]
	add_timer_func_list(pet_heal_timer,"pet_heal_timer"); // [Valaris]
//	add_timer_func_list(pet_skillattack_timer,"pet_skillattack_timer"); // [Valaris]
	add_timer_interval(gettick()+MIN_PETTHINKTIME,MIN_PETTHINKTIME,pet_ai_hard,0,0);

	return 0;
}

int do_final_pet(void) {
	int i;
	for(i = 0;i < MAX_PET_DB; i++) {
		if(pet_db[i].script) {
			aFree(pet_db[i].script);
			pet_db[i].script=NULL;
		}
	}
	return 0;
}
