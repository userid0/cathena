// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/malloc.h"
#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/nullpo.h"
#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/utils.h"

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct s_mercenary_db mercenary_db[MAX_MERCENARY_CLASS]; // Mercenary Database

int merc_search_index(int class_)
{
	int i;
	ARR_FIND(0, MAX_MERCENARY_CLASS, i, mercenary_db[i].class_ == class_);
	return (i == MAX_MERCENARY_CLASS)?-1:i;
}

bool merc_class(int class_)
{
	return (bool)(merc_search_index(class_) > -1);
}

struct view_data * merc_get_viewdata(int class_)
{
	int i = merc_search_index(class_);
	if( i < 0 )
		return 0;

	return &mercenary_db[i].vd;
}

int merc_create(struct map_session_data *sd, int class_, unsigned int lifetime)
{
	struct s_mercenary merc;
	struct s_mercenary_db *db;
	int i;
	nullpo_retr(1,sd);

	if( (i = merc_search_index(class_)) < 0 )
		return 0;

	db = &mercenary_db[i];
	memset(&merc,0,sizeof(struct s_mercenary));

	merc.char_id = sd->status.char_id;
	merc.class_ = class_;
	merc.hp = db->status.max_hp;
	merc.sp = db->status.max_sp;
	merc.remain_life_time = lifetime;

	// Request Char Server to create this mercenary
	intif_mercenary_create(&merc);

	return 1;
}

int mercenary_save(struct mercenary_data *md)
{
	const struct TimerData * td = get_timer(md->contract_timer);

	md->mercenary.hp = md->battle_status.hp;
	md->mercenary.sp = md->battle_status.sp;
	if( td != NULL )
		md->mercenary.remain_life_time = DIFF_TICK(td->tick, gettick());
	else
	{
		md->mercenary.remain_life_time = 0;
		ShowWarning("mercenary_save : mercenary without timer (tid %d)\n", md->contract_timer);
	}

	intif_mercenary_save(&md->mercenary);
	return 1;
}

static int merc_contract_end(int tid, unsigned int tick, int id, intptr data)
{
	struct map_session_data *sd;
	struct mercenary_data *md;

	if( (sd = map_id2sd(id)) == NULL )
		return 1;
	if( (md = sd->md) == NULL )
		return 1;

	if( md->contract_timer != tid )
	{
		ShowError("merc_contract_end %d != %d.\n", md->contract_timer, tid);
		return 0;
	}

	md->contract_timer = INVALID_TIMER;
	merc_delete(md, 0); // Mercenary soldier's duty hour is over.

	return 0;
}

int merc_delete(struct mercenary_data *md, int reply)
{
	struct map_session_data *sd = md->master;
	md->mercenary.remain_life_time = 0;

	merc_contract_stop(md);

	if( !sd )
		return unit_free(&md->bl, 1);
	clif_mercenary_message(sd->fd, reply);
	
	return unit_remove_map(&md->bl, 1);
}

void merc_contract_stop(struct mercenary_data *md)
{
	nullpo_retv(md);
	if( md->contract_timer != INVALID_TIMER )
		delete_timer(md->contract_timer, merc_contract_end);
	md->contract_timer = INVALID_TIMER;
}

void merc_contract_init(struct mercenary_data *md)
{
	if( md->contract_timer == INVALID_TIMER )
		md->contract_timer = add_timer(gettick() + md->mercenary.remain_life_time, merc_contract_end, md->master->bl.id, 0);

	md->regen.state.block = 0;
}

int merc_data_received(struct s_mercenary *merc, bool flag)
{
	struct map_session_data *sd;
	struct mercenary_data *md;
	struct s_mercenary_db *db;
	int i = merc_search_index(merc->class_);

	if( (sd = map_charid2sd(merc->char_id)) == NULL )
		return 0;
	if( !flag || i < 0 )
	{ // Not created - loaded - DB info
		sd->status.mer_id = 0;
		return 0;
	}

	sd->status.mer_id = merc->mercenary_id;
	db = &mercenary_db[i];

	if( !sd->md )
	{
		sd->md = md = (struct mercenary_data*)aCalloc(1,sizeof(struct mercenary_data));
		md->bl.type = BL_MER;
		md->bl.id = npc_get_new_npc_id();

		md->master = sd;
		md->db = db;
		memcpy(&md->mercenary, merc, sizeof(struct s_mercenary));
		status_set_viewdata(&md->bl, md->mercenary.class_);
		status_change_init(&md->bl);
		unit_dataset(&md->bl);
		md->ud.dir = sd->ud.dir;

		md->bl.m = sd->bl.m;
		md->bl.x = sd->bl.x;
		md->bl.y = sd->bl.y;
		unit_calc_pos(&md->bl, sd->bl.x, sd->bl.y, sd->ud.dir);
		md->bl.x = md->ud.to_x;
		md->bl.y = md->ud.to_y;

		map_addiddb(&md->bl);
		status_calc_mercenary(md,1);
		md->contract_timer = INVALID_TIMER;
		merc_contract_init(md);
	}
	else
		memcpy(&sd->md->mercenary, merc, sizeof(struct s_mercenary));

	md = sd->md;
	if( md && md->bl.prev == NULL && sd->bl.prev != NULL )
	{
		map_addblock(&md->bl);
		clif_spawn(&md->bl);
		clif_mercenary_info(sd);
		clif_mercenary_skillblock(sd);
	}

	return 1;
}

int read_mercenarydb(void)
{
	FILE *fp;
	char line[1024], *p;
	char *str[26];
	int i, j = 0, k = 0, ele;
	struct s_mercenary_db *db;
	struct status_data *status;

	sprintf(line, "%s/%s", db_path, "mercenary_db.txt");
	memset(mercenary_db,0,sizeof(mercenary_db));

	fp = fopen(line, "r");
	if( !fp )
	{
		ShowError("read_mercenarydb : can't read mercenary_db.txt\n");
		return -1;
	}

	while( fgets(line, sizeof(line), fp) && j < MAX_MERCENARY_CLASS )
	{
		k++;
		if( line[0] == '/' && line[1] == '/' )
			continue;

		i = 0;
		p = strtok(line, ",");
		while( p != NULL && i < 26 )
		{
			str[i++] = p;
			p = strtok(NULL, ",");
		}
		if( i < 26 )
		{
			ShowError("read_mercenarydb : Incorrect number of columns at mercenary_db.txt line %d.\n", k);
			continue;
		}

		db = &mercenary_db[j];
		db->class_ = atoi(str[0]);
		strncpy(db->sprite, str[1], NAME_LENGTH);
		strncpy(db->name, str[2], NAME_LENGTH);
		db->lv = atoi(str[3]);

		status = &db->status;
		db->vd.class_ = db->class_;

		status->max_hp = atoi(str[4]);
		status->max_sp = atoi(str[5]);
		status->rhw.range = atoi(str[6]);
		status->rhw.atk = atoi(str[7]);
		status->rhw.atk2 = atoi(str[8]);
		status->def = atoi(str[9]);
		status->mdef = atoi(str[10]);
		status->str = atoi(str[11]);
		status->agi = atoi(str[12]);
		status->vit = atoi(str[13]);
		status->int_ = atoi(str[14]);
		status->dex = atoi(str[15]);
		status->luk = atoi(str[16]);
		db->range2 = atoi(str[17]);
		db->range3 = atoi(str[18]);
		status->size = atoi(str[19]);
		status->race = atoi(str[20]);
	
		ele = atoi(str[21]);
		status->def_ele = ele%10;
		status->ele_lv = ele/20;
		if( status->def_ele >= ELE_MAX )
		{
			ShowWarning("Mercenary %d has invalid element type %d (max element is %d)\n", db->class_, status->def_ele, ELE_MAX - 1);
			status->def_ele = ELE_NEUTRAL;
		}
		if( status->ele_lv < 1 || status->ele_lv > 4 )
		{
			ShowWarning("Mercenary %d has invalid element level %d (max is 4)\n", db->class_, status->ele_lv);
			status->ele_lv = 1;
		}

		status->speed = atoi(str[22]);
		status->adelay = atoi(str[23]);
		status->amotion = atoi(str[24]);
		status->dmotion = atoi(str[25]);

		j++;
	}

	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' mercenaries in '"CL_WHITE"db/mercenary_db.txt"CL_RESET"'.\n",j);

	return 0;
}

int read_mercenary_skilldb(void)
{
	FILE *fp;
	char line[1024], *p;
	char *str[3];
	struct s_mercenary_db *db;
	int i, j = 0, k = 0, class_;
	int skillid, skilllv;

	sprintf(line, "%s/%s", db_path, "mercenary_skill_db.txt");
	fp = fopen(line, "r");
	if( !fp )
	{
		ShowError("read_mercenary_skilldb : can't read mercenary_skill_db.txt\n");
		return -1;
	}

	while( fgets(line, sizeof(line), fp) )
	{
		k++;
		if( line[0] == '/' && line[1] == '/' )
			continue;

		i = 0;
		p = strtok(line, ",");
		while( p != NULL && i < 3 )
		{
			str[i++] = p;
			p = strtok(NULL, ",");
		}
		if( i < 3 )
		{
			ShowError("read_mercenary_skilldb : Incorrect number of columns at mercenary_skill_db.txt line %d.\n", k);
			continue;
		}

		class_ = atoi(str[0]);
		ARR_FIND(0, MAX_MERCENARY_CLASS, i, class_ == mercenary_db[i].class_);
		if( i == MAX_MERCENARY_CLASS )
		{
			ShowError("read_mercenary_skilldb : Class not found in mercenary_db for skill entry, line %d.\n", k);
			continue;
		}
		
		skillid = atoi(str[1]);
		if( skillid < MC_SKILLBASE || skillid >= MC_SKILLBASE + MAX_MERCSKILL )
		{
			ShowError("read_mercenary_skilldb : Skill out of range, line %d.\n", k);
			continue;
		}

		db = &mercenary_db[i];
		skilllv = atoi(str[2]);

		i = skillid - MC_SKILLBASE;
		db->skill[i].id = skillid;
		db->skill[i].lv = skilllv;
		j++;
	}

	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"db/mercenary_skill_db.txt"CL_RESET"'.\n",j);
	return 0;
}

int do_init_mercenary(void)
{
	read_mercenarydb();
	read_mercenary_skilldb();
	
	//add_timer_func_list(mercenary_contract, "mercenary_contract");
	return 0;
}

int do_final_mercenary(void);
