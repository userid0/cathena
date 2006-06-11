// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

// original code from athena
// SQL conversion by hack

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "char.h"
#include "../common/strlib.h"
#include "../common/showmsg.h"
// #include "int_storage.h"
#include "inter.h"
#include "int_guild.h"
#include "mmo.h"
#include "socket.h"
#include "db.h"
#include "malloc.h"

//Guild cache
static struct dbt *guild_db_;

struct guild_castle castles[MAX_GUILDCASTLE];

static unsigned int guild_exp[100];

#define GS_BASIC 0x01
#define GS_MEMBER 0x02
#define GS_POSITION 0x04
#define GS_ALLIANCE 0x08
#define GS_EXPULSION 0x10
#define GS_SKILL 0x20
#define GS_MASK 0x7F
#define GS_REMOVE 0x80

int mapif_parse_GuildLeave(int fd,int guild_id,int account_id,int char_id,int flag,const char *mes);
int mapif_guild_broken(int guild_id,int flag);
int guild_check_empty(struct guild *g);
int guild_calcinfo(struct guild *g);
int mapif_guild_basicinfochanged(int guild_id,int type,const void *data,int len);
int mapif_guild_info(int fd,struct guild *g);
int guild_break_sub(int key,void *data,va_list ap);
int inter_guild_tosql(struct guild *g,int flag);


#define mysql_query(_x, _y)  debug_mysql_query(__FILE__, __LINE__, _x, _y)

static int guild_save(DBKey key, void *data, va_list ap) {
	struct guild *g = (struct guild*) data;
	int *last_id = va_arg(ap, int *);
	int *state = va_arg(ap, int *);
	
	if ((*state) == 0 && g->guild_id == (*last_id))
		(*state)++; //Save next guild in the list.
	else if (g->save_flag&GS_MASK && (*state) == 1) {
	   inter_guild_tosql(g, g->save_flag&GS_MASK);
		g->save_flag &= ~GS_MASK;

		//Some guild saved.
		(*last_id) = g->guild_id;
		(*state)++;
	}
	
   if(g->save_flag == GS_REMOVE) { //Nothing to save, guild is ready for removal.
		if (save_log)
			ShowInfo("Guild Unloaded (%d - %s)\n", g->guild_id, g->name);
		db_remove(guild_db_, key);
   }
	return 0;
}

static int guild_save_timer(int tid, unsigned int tick, int id, int data) {
	static int last_id = 0; //To know in which guild we were.
	int state = 0; //0: Have not reached last guild. 1: Reached last guild, ready for save. 2: Some guild saved, don't do further saving.
	if (!last_id) //Save the first guild in the list.
		state = 1;
	guild_db_->foreach(guild_db_, guild_save, &last_id, &state);
	if (state != 2) //Reached the end of the guild db without saving.
		last_id = 0; //Reset guild saved, return to beginning.

	state = guild_db_->size(guild_db_);
	if (state < 1) state = 1; //Calculate the time slot for the next save.
	add_timer(tick  + autosave_interval/state, guild_save_timer, 0, 0);
	return 0;
}

// Save guild into sql
int inter_guild_tosql(struct guild *g,int flag)
{
	// GS_BASIC `guild` (`guild_id`, `name`,`master`,`guild_lv`,`connect_member`,`max_member`,`average_lv`,`exp`,`next_exp`,`skill_point`,`mes1`,`mes2`,`emblem_len`,`emblem_id`,`emblem_data`)
	// GS_MEMBER `guild_member` (`guild_id`,`account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`rsv1`,`rsv2`,`name`)
	// GS_POSITION `guild_position` (`guild_id`,`position`,`name`,`mode`,`exp_mode`)
	// GS_ALLIANCE `guild_alliance` (`guild_id`,`opposition`,`alliance_id`,`name`)
	// GS_EXPULSION `guild_expulsion` (`guild_id`,`name`,`mes`,`acc`,`account_id`,`rsv1`,`rsv2`,`rsv3`)
	// GS_SKILL `guild_skill` (`guild_id`,`id`,`lv`) 

	// temporary storage for str convertion. They must be twice the size of the
	// original string to ensure no overflows will occur. [Skotlex]
	char t_name[NAME_LENGTH*2],
		t_master[NAME_LENGTH*2],
		t_mes1[120],
		t_mes2[240],
		t_member[NAME_LENGTH*2],
		t_position[NAME_LENGTH*2],
		t_alliance[NAME_LENGTH*2],
		t_ename[NAME_LENGTH*2],
		t_emes[80],
		t_info[240];
	char emblem_data[4096];
	int i=0;

	if (g->guild_id<=0 && g->guild_id != -1) return 0;
	
#ifdef NOISY
	ShowInfo("Save guild request ("CL_BOLD"%d"CL_RESET" - flag 0x%x).",g->guild_id, flag);
#endif

	jstrescapecpy(t_name, g->name);
	
	t_info[0]='\0';
	// Insert new guild to sqlserver
	if (flag&GS_BASIC || g->guild_id == -1){
		int len=0;
		strcat(t_info, " guild");
					
		for(i=0;i<g->emblem_len;i++){
			len+=sprintf(emblem_data+len,"%02x",(unsigned char)(g->emblem_data[i]));
			//printf("%02x",(unsigned char)(g->emblem_data[i]));
		}
		emblem_data[len] = '\0';
		//printf("- emblem_len = %d \n",g->emblem_len);
		if (g->guild_id == -1) //Insert
			sprintf(tmp_sql,"INSERT INTO `%s` "
				"(`name`,`master`,`guild_lv`,`connect_member`,`max_member`,`average_lv`,`exp`,`next_exp`,`skill_point`,`mes1`,`mes2`,`emblem_len`,`emblem_id`,`emblem_data`,`char_id`) "
				"VALUES ('%s', '%s', '%d', '%d', '%d', '%d', '%u', '%u', '%d', '%s', '%s', '%d', '%d', '%s', '%d')",
				guild_db,t_name,jstrescapecpy(t_master,g->master),
				g->guild_lv,g->connect_member,g->max_member,g->average_lv,g->exp,g->next_exp,g->skill_point,
				jstrescapecpy(t_mes1,g->mes1),jstrescapecpy(t_mes2,g->mes2),g->emblem_len,g->emblem_id,emblem_data,
				g->member[0].char_id);
		else //Update
			sprintf(tmp_sql,"UPDATE `%s` SET"
				" `guild_id`=%d, `name`='%s', `master`='%s',`guild_lv`=%d, `connect_member`=%d,`max_member`=%d, "
				"`average_lv`=%d,`exp`=%u,`next_exp`=%u,`skill_point`=%d,`mes1`='%s',`mes2`='%s',"
				"`emblem_len`=%d,`emblem_id`=%d,`emblem_data`='%s',`char_id`=%d WHERE `guild_id`=%d",
				guild_db, g->guild_id,t_name,jstrescapecpy(t_master,g->master),
				g->guild_lv,g->connect_member,g->max_member,g->average_lv,g->exp,g->next_exp,g->skill_point,
				jstrescapecpy(t_mes1,g->mes1),jstrescapecpy(t_mes2,g->mes2),g->emblem_len,g->emblem_id,emblem_data,
				g->member[0].char_id, g->guild_id);
		
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
			ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
			if (g->guild_id == -1)
				return 0; //Failed to create guild!
		} else if (g->guild_id == -1) { //New guild
			if(mysql_field_count(&mysql_handle) == 0 &&
				mysql_insert_id(&mysql_handle) != 0)
				g->guild_id = (int)mysql_insert_id(&mysql_handle);
			else
				return 0; //Failed to get ID??
		}
	}

	if (flag&GS_MEMBER){
		struct guild_member *m;
		strcat(t_info, " members");
		// Re-writing from scratch (Aru)
		sprintf(tmp_sql,"DELETE from `%s` where `guild_id` = '%d'",
			guild_member_db, g->guild_id);
		if(mysql_query(&mysql_handle, tmp_sql))
		{
			ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
			ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		}
		sprintf(tmp_sql,"UPDATE `%s` SET `guild_id` = '0' WHERE `guild_id` = '%d'",
			char_db, g->guild_id);
		if(mysql_query(&mysql_handle, tmp_sql))
		{
			ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
			ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		}
		for(i=0;i<g->max_member;i++){
			m = &g->member[i];
			if(m->account_id) {
				//Since nothing references guild member table as foreign keys, it's safe to use REPLACE INTO
				sprintf(tmp_sql,"REPLACE INTO `%s` (`guild_id`,`account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`name`) "
					"VALUES ('%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%s')",
				       guild_member_db, g->guild_id, m->account_id,m->char_id,
				       m->hair,m->hair_color,m->gender,
				       m->class_,m->lv,m->exp,m->exp_payper,m->online,m->position,
				       jstrescapecpy(t_member,m->name));
				if(mysql_query(&mysql_handle, tmp_sql))
				{
					ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
					ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
				}
				sprintf(tmp_sql,"UPDATE `%s` SET `guild_id` = '%d' WHERE `char_id` = '%d'",
					char_db, g->guild_id, m->char_id);
				if(mysql_query(&mysql_handle, tmp_sql))
				{
					ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
					ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
				}
			}
		}
	}

	if (flag&GS_POSITION){
		strcat(t_info, " positions");
		//printf("- Insert guild %d to guild_position\n",g->guild_id);
		for(i=0;i<MAX_GUILDPOSITION;i++){
			struct guild_position *p = &g->position[i];
			sprintf(tmp_sql,"REPLACE INTO `%s` (`guild_id`,`position`,`name`,`mode`,`exp_mode`) VALUES ('%d','%d', '%s','%d','%d')",
				guild_position_db, g->guild_id, i, jstrescapecpy(t_position,p->name),p->mode,p->exp_mode);
			//printf(" %s\n",tmp_sql);
			if(mysql_query(&mysql_handle, tmp_sql) ) {
				ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
				ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
			}
		}
	}

	if (flag&GS_ALLIANCE){
		sprintf(tmp_sql, "DELETE FROM `%s` WHERE `guild_id`='%d' OR `alliance_id`='%d'",guild_alliance_db, g->guild_id,g->guild_id);
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
			ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		}
		else
		{
		//printf("- Insert guild %d to guild_alliance\n",g->guild_id);
		for(i=0;i<MAX_GUILDALLIANCE;i++){
			struct guild_alliance *a=&g->alliance[i];
			if(a->guild_id>0){
				sprintf(tmp_sql,"REPLACE INTO `%s` (`guild_id`,`opposition`,`alliance_id`,`name`) "
					"VALUES ('%d','%d','%d','%s')",
					guild_alliance_db, g->guild_id,a->opposition,a->guild_id,jstrescapecpy(t_alliance,a->name));
				//printf(" %s\n",tmp_sql);
				if(mysql_query(&mysql_handle, tmp_sql) ) {
					ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
					ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
				}
				sprintf(tmp_sql,"REPLACE INTO `%s` (`guild_id`,`opposition`,`alliance_id`,`name`) "
					"VALUES ('%d','%d','%d','%s')",
					guild_alliance_db, a->guild_id,a->opposition,g->guild_id,t_name);
				//printf(" %s\n",tmp_sql);
				if(mysql_query(&mysql_handle, tmp_sql) ) {
					ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
					ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
				}
			}
		}
		}
	}

	if (flag&GS_EXPULSION){
		strcat(t_info, " expulsions");
		//printf("- Insert guild %d to guild_expulsion\n",g->guild_id);
		for(i=0;i<MAX_GUILDEXPLUSION;i++){
			struct guild_explusion *e=&g->explusion[i];
			if(e->account_id>0){
				sprintf(tmp_sql,"REPLACE INTO `%s` (`guild_id`,`name`,`mes`,`acc`,`account_id`,`rsv1`,`rsv2`,`rsv3`) "
					"VALUES ('%d','%s','%s','%s','%d','%d','%d','%d')",
					guild_expulsion_db, g->guild_id,
					jstrescapecpy(t_ename,e->name),jstrescapecpy(t_emes,e->mes),e->acc,e->account_id,e->rsv1,e->rsv2,e->rsv3 );
				//printf(" %s\n",tmp_sql);
				if(mysql_query(&mysql_handle, tmp_sql) ) {
					ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
					ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
				}
			}
		}
	}

	if (flag&GS_SKILL){
		strcat(t_info, " skills");
		//printf("- Insert guild %d to guild_skill\n",g->guild_id);
		for(i=0;i<MAX_GUILDSKILL;i++){
			if (g->skill[i].id>0 && g->skill[i].lv>0){
				sprintf(tmp_sql,"REPLACE INTO `%s` (`guild_id`,`id`,`lv`) VALUES ('%d','%d','%d')",
					guild_skill_db, g->guild_id,g->skill[i].id,g->skill[i].lv);
				//printf("%s\n",tmp_sql);
				if(mysql_query(&mysql_handle, tmp_sql) ) {
					ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
					ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
				}
			}
		}
	}

	if (save_log)
		ShowInfo("Saved guild (%d - %s):%s\n",g->guild_id,g->name,t_info);
	return 1;
}

// Read guild from sql
struct guild * inter_guild_fromsql(int guild_id)
{
	int i;
	char emblem_data[4096];
	char *pstr;
	struct guild *g;

	if (guild_id<=0) return NULL;

	g = idb_get(guild_db_,guild_id);
	if (g) return g;

	g = (struct guild*)aCalloc(sizeof(struct guild), 1);

#ifdef NOISY
	ShowInfo("Guild load request (%d)...\n", guild_id);
#endif
	
	sprintf(tmp_sql,"SELECT `name`,`master`,`guild_lv`,`connect_member`,`max_member`,`average_lv`,`exp`,`next_exp`,`skill_point`,`mes1`,`mes2`,`emblem_len`,`emblem_id`,`emblem_data` "
		"FROM `%s` WHERE `guild_id`='%d'",guild_db, guild_id);
	//printf("  %s\n",tmp_sql);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		aFree(g);
		return NULL;
	}

	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res==NULL || mysql_num_rows(sql_res)<1) {
		//Guild does not exists.
		if (sql_res) mysql_free_result(sql_res);
		aFree(g);
		return NULL;
	}
	
	sql_row = mysql_fetch_row(sql_res);
	if (sql_row==NULL) {
		mysql_free_result(sql_res);
		aFree(g);
		return NULL;
	}

	g->guild_id=guild_id;
	strncpy(g->name,sql_row[0],NAME_LENGTH-1);
	strncpy(g->master,sql_row[1],NAME_LENGTH-1);
	g->guild_lv=atoi(sql_row[2]);
	g->connect_member=atoi(sql_row[3]);
	if (atoi(sql_row[4]) > MAX_GUILD) // Fix reduction of MAX_GUILD [PoW]
		g->max_member = MAX_GUILD;
	else
		g->max_member = atoi(sql_row[4]);
	g->average_lv=atoi(sql_row[5]);
	g->exp=(unsigned int)atof(sql_row[6]);
	g->next_exp=(unsigned int)atof(sql_row[7]);
	g->skill_point=atoi(sql_row[8]);
	//There shouldn't be a need to copy the very last char, as it's the \0 [Skotlex]
	strncpy(g->mes1,sql_row[9],59);
	strncpy(g->mes2,sql_row[10],119);
	g->emblem_len=atoi(sql_row[11]);
	g->emblem_id=atoi(sql_row[12]);
	strncpy(emblem_data,sql_row[13],4096);
	for(i=0,pstr=emblem_data;i<g->emblem_len;i++,pstr+=2){
		int c1=pstr[0],c2=pstr[1],x1=0,x2=0;
		if(c1>='0' && c1<='9')x1=c1-'0';
		if(c1>='a' && c1<='f')x1=c1-'a'+10;
		if(c1>='A' && c1<='F')x1=c1-'A'+10;
		if(c2>='0' && c2<='9')x2=c2-'0';
		if(c2>='a' && c2<='f')x2=c2-'a'+10;
		if(c2>='A' && c2<='F')x2=c2-'A'+10;
		g->emblem_data[i]=(x1<<4)|x2;
	}
	mysql_free_result(sql_res);

	//printf("- Read guild_member %d from sql \n",guild_id);
	sprintf(tmp_sql,"SELECT `guild_id`,`account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`rsv1`,`rsv2`,`name` "
		"FROM `%s` WHERE `guild_id`='%d'  ORDER BY `position`", guild_member_db, guild_id);
	//printf("  %s\n",tmp_sql);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		aFree(g);
		return NULL;
	}
	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res) {
		for(i=0;((sql_row = mysql_fetch_row(sql_res))&&i<g->max_member);i++){
			struct guild_member *m = &g->member[i];
			m->account_id=atoi(sql_row[1]);
			m->char_id=atoi(sql_row[2]);
			m->hair=atoi(sql_row[3]);
			m->hair_color=atoi(sql_row[4]);
			m->gender=atoi(sql_row[5]);
			m->class_=atoi(sql_row[6]);
			m->lv=atoi(sql_row[7]);
			m->exp=(unsigned int)atof(sql_row[8]);
			m->exp_payper=atoi(sql_row[9]);
			m->online=atoi(sql_row[10]);
			if (atoi(sql_row[11]) >= MAX_GUILDPOSITION) // Fix reduction of MAX_GUILDPOSITION [PoW]
				m->position = MAX_GUILDPOSITION - 1;
			else
				m->position = atoi(sql_row[11]);

			strncpy(m->name,sql_row[14],NAME_LENGTH-1);
		}
		mysql_free_result(sql_res);
	}

	//printf("- Read guild_position %d from sql \n",guild_id);
	sprintf(tmp_sql,"SELECT `guild_id`,`position`,`name`,`mode`,`exp_mode` FROM `%s` WHERE `guild_id`='%d'",guild_position_db, guild_id);
	//printf("  %s\n",tmp_sql);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		aFree(g);
		return NULL;
	}
	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res) {
		for(i=0;((sql_row = mysql_fetch_row(sql_res))&&i<MAX_GUILDPOSITION);i++){
			int position = atoi(sql_row[1]);
			struct guild_position *p = &g->position[position];
			strncpy(p->name,sql_row[2],NAME_LENGTH-1);
			p->mode=atoi(sql_row[3]);
			p->exp_mode=atoi(sql_row[4]);
		}
		mysql_free_result(sql_res);
	}

	//printf("- Read guild_alliance %d from sql \n",guild_id);
	sprintf(tmp_sql,"SELECT `guild_id`,`opposition`,`alliance_id`,`name` FROM `%s` WHERE `guild_id`='%d'",guild_alliance_db, guild_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		aFree(g);
		return NULL;
	}
	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res) {
		for(i=0;((sql_row = mysql_fetch_row(sql_res))&&i<MAX_GUILDALLIANCE);i++){
			struct guild_alliance *a = &g->alliance[i];
			a->opposition=atoi(sql_row[1]);
			a->guild_id=atoi(sql_row[2]);
			strncpy(a->name,sql_row[3],NAME_LENGTH-1);
		}
		mysql_free_result(sql_res);
	}

	//printf("- Read guild_expulsion %d from sql \n",guild_id);
	sprintf(tmp_sql,"SELECT `guild_id`,`name`,`mes`,`acc`,`account_id`,`rsv1`,`rsv2`,`rsv3` FROM `%s` WHERE `guild_id`='%d'",guild_expulsion_db, guild_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		aFree(g);
		return NULL;
	}
	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res) {
		for(i=0;((sql_row = mysql_fetch_row(sql_res))&&i<MAX_GUILDEXPLUSION);i++){
			struct guild_explusion *e = &g->explusion[i];

			strncpy(e->name,sql_row[1],NAME_LENGTH-1);
			//No need to copy char 40, the null terminator. [Skotlex]
			strncpy(e->mes,sql_row[2],39);
			strncpy(e->acc,sql_row[3],39);
			e->account_id=atoi(sql_row[4]);
			e->rsv1=atoi(sql_row[5]);
			e->rsv2=atoi(sql_row[6]);
			e->rsv3=atoi(sql_row[7]);
		}
		mysql_free_result(sql_res);
	}

	//printf("- Read guild_skill %d from sql \n",guild_id);
	sprintf(tmp_sql,"SELECT `guild_id`,`id`,`lv` FROM `%s` WHERE `guild_id`='%d' ORDER BY `id`",guild_skill_db, guild_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		aFree(g);
		return NULL;
	}
	
	for(i = 0; i < MAX_GUILDSKILL; i++)
	{	//Skill IDs must always be initialized. [Skotlex]
		g->skill[i].id = i + GD_SKILLBASE;
	}

	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res) {
		while ((sql_row = mysql_fetch_row(sql_res))){
			int id = atoi(sql_row[1])-GD_SKILLBASE;
			if (id >= 0 && id < MAX_GUILDSKILL)
			//I know this seems ridiculous, but the skills HAVE to be placed on their 'correct' array slot or things break x.x [Skotlex]
				g->skill[id].lv=atoi(sql_row[2]);
		}
		mysql_free_result(sql_res);
	}

	idb_put(guild_db_, guild_id, g); //Add to cache
	g->save_flag |= GS_REMOVE; //But set it to be removed, in case it is not needed for long.
	
	if (save_log)
		ShowInfo("Guild loaded (%d - %s)\n", guild_id, g->name);

	return g;
}

int inter_guildcastle_tosql(struct guild_castle *gc){
	// `guild_castle` (`castle_id`, `guild_id`, `economy`, `defense`, `triggerE`, `triggerD`, `nextTime`, `payTime`, `createTime`, `visibleC`, `visibleG0`, `visibleG1`, `visibleG2`, `visibleG3`, `visibleG4`, `visibleG5`, `visibleG6`, `visibleG7`)

	if (gc==NULL) return 0;
	#ifdef GUILD_DEBUG
ShowDebug("Save guild_castle (%d)\n", gc->castle_id);
	#endif

//	sql_query("DELETE FROM `%s` WHERE `castle_id`='%d'",guild_castle_db, gc->castle_id);

	sprintf(tmp_sql,"REPLACE INTO `%s` "
		"(`castle_id`, `guild_id`, `economy`, `defense`, `triggerE`, `triggerD`, `nextTime`, `payTime`, `createTime`,"
		"`visibleC`, `visibleG0`, `visibleG1`, `visibleG2`, `visibleG3`, `visibleG4`, `visibleG5`, `visibleG6`, `visibleG7`,"
		"`Ghp0`, `Ghp1`, `Ghp2`, `Ghp3`, `Ghp4`, `Ghp5`, `Ghp6`, `Ghp7`)"
		"VALUES ('%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d')",
		guild_castle_db, gc->castle_id, gc->guild_id,  gc->economy, gc->defense, gc->triggerE, gc->triggerD, gc->nextTime, gc->payTime,
		gc->createTime, gc->visibleC,
		gc->guardian[0].visible, gc->guardian[1].visible, gc->guardian[2].visible, gc->guardian[3].visible, gc->guardian[4].visible, gc->guardian[5].visible, gc->guardian[6].visible, gc->guardian[7].visible,
		gc->guardian[0].hp, gc->guardian[1].hp, gc->guardian[2].hp, gc->guardian[3].hp, gc->guardian[4].hp, gc->guardian[5].hp, gc->guardian[6].hp, gc->guardian[7].hp);

	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}


	memcpy(&castles[gc->castle_id],gc,sizeof(struct guild_castle));

	return 0;
}


// Read guild_castle from sql
int inter_guildcastle_fromsql(int castle_id,struct guild_castle *gc)
{
	static int castles_init=0;
	if (gc==NULL) return 0;
	if (castle_id==-1) return 0;

	if(!castles_init)
	{
		int i;
		for(i=0;i<MAX_GUILDCASTLE;i++)
			castles[i].castle_id=-1;
		castles_init = 1;
	}

	if(castles[castle_id].castle_id == castle_id)
	{
		memcpy(gc,&castles[castle_id],sizeof(struct guild_castle));
		return 1;
	}

	memset(gc,0,sizeof(struct guild_castle));
	sprintf(tmp_sql,"SELECT `castle_id`, `guild_id`, `economy`, `defense`, `triggerE`, `triggerD`, `nextTime`, `payTime`, `createTime`, "
		"`visibleC`, `visibleG0`, `visibleG1`, `visibleG2`, `visibleG3`, `visibleG4`, `visibleG5`, `visibleG6`, `visibleG7`,"
		"`Ghp0`, `Ghp1`, `Ghp2`, `Ghp3`, `Ghp4`, `Ghp5`, `Ghp6`, `Ghp7`"
        	" FROM `%s` WHERE `castle_id`='%d'",guild_castle_db, castle_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		return 0;
	}
	// ARU: This needs to be set even if there are no SQL results
	gc->castle_id=castle_id;
	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res!=NULL && mysql_num_rows(sql_res)>0) {
		sql_row = mysql_fetch_row(sql_res);
		if (sql_row==NULL){
			mysql_free_result(sql_res);
			return 1; //Assume empty castle.
		}
		gc->guild_id =  atoi (sql_row[1]);
		gc->economy = atoi (sql_row[2]);
		gc->defense = atoi (sql_row[3]);
		gc->triggerE = atoi (sql_row[4]);
		gc->triggerD = atoi (sql_row[5]);
		gc->nextTime = atoi (sql_row[6]);
		gc->payTime = atoi (sql_row[7]);
		gc->createTime = atoi (sql_row[8]);
		gc->visibleC = atoi (sql_row[9]);
		gc->guardian[0].visible = atoi (sql_row[10]);
		gc->guardian[1].visible = atoi (sql_row[11]);
		gc->guardian[2].visible = atoi (sql_row[12]);
		gc->guardian[3].visible = atoi (sql_row[13]);
		gc->guardian[4].visible = atoi (sql_row[14]);
		gc->guardian[5].visible = atoi (sql_row[15]);
		gc->guardian[6].visible = atoi (sql_row[16]);
		gc->guardian[7].visible = atoi (sql_row[17]);
		gc->guardian[0].hp = atoi (sql_row[18]);
		gc->guardian[1].hp = atoi (sql_row[19]);
		gc->guardian[2].hp = atoi (sql_row[20]);
		gc->guardian[3].hp = atoi (sql_row[21]);
		gc->guardian[4].hp = atoi (sql_row[22]);
		gc->guardian[5].hp = atoi (sql_row[23]);
		gc->guardian[6].hp = atoi (sql_row[24]);
		gc->guardian[7].hp = atoi (sql_row[25]);
	
		if (save_log)
			ShowInfo("Loaded Castle %d (guild %d)\n",castle_id,gc->guild_id);

	}
	mysql_free_result(sql_res) ; //resource free

	memcpy(&castles[castle_id],gc,sizeof(struct guild_castle));

	return 1;
}


// Read exp_guild.txt
int inter_guild_ReadEXP(void)
{
	int i;
	FILE *fp;
	char line[1024];
	for (i=0;i<100;i++) guild_exp[i]=0;

	sprintf(line, "%s/exp_guild.txt", db_path);
	fp=fopen(line,"r");
	if(fp==NULL){
		ShowError("can't read %s\n", line);
		return 1;
	}
	i=0;
	while(fgets(line,256,fp) && i<100){
		if(line[0]=='/' && line[1]=='/')
			continue;
		guild_exp[i]=(unsigned int)atof(line);
		i++;
	}
	fclose(fp);

	return 0;
}


int inter_guild_CharOnline(int char_id, int guild_id) {
   
   struct guild *g;
   int i;
   
	if (guild_id == -1) {
		//Get guild_id from the database
		sprintf (tmp_sql , "SELECT guild_id FROM `%s` WHERE char_id='%d'",char_db,char_id);
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
			ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
			return 0;
		}

		sql_res = mysql_store_result(&mysql_handle) ;
		if(sql_res == NULL)
			return 0; //Eh? No guild?
		
		sql_row = mysql_fetch_row(sql_res);
		guild_id = sql_row?atoi(sql_row[0]):0;
		mysql_free_result(sql_res);
	}
	if (guild_id == 0)
		return 0; //No guild...
	
	g = inter_guild_fromsql(guild_id);
	if(!g) {
		ShowError("Character %d's guild %d not found!\n", char_id, guild_id);
		return 0;
	}

	//Member has logged in before saving, tell saver not to delete
	if(g->save_flag & GS_REMOVE)
		g->save_flag &= ~GS_REMOVE;

	//Set member online
	for(i=0; i<g->max_member; i++) {
		if (g->member[i].char_id == char_id) {
			g->member[i].online = 1;
			break;
		}
	}
	return 1;
}

int inter_guild_CharOffline(int char_id, int guild_id) {
	struct guild *g=NULL;
	int online_count=0, i;

	if (guild_id == -1) {
		//Get guild_id from the database
		sprintf (tmp_sql , "SELECT guild_id FROM `%s` WHERE char_id='%d'",char_db,char_id);
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
			ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
			return 0;
		}

		sql_res = mysql_store_result(&mysql_handle) ;
		if(sql_res == NULL)
			return 0; //Eh? No guild?
		
		sql_row = mysql_fetch_row(sql_res);
		guild_id = sql_row?atoi(sql_row[0]):0;
		mysql_free_result(sql_res);
	}
	if (guild_id == 0)
		return 0; //No guild...
	
	//Character has a guild, set character offline and check if they were the only member online
	g = inter_guild_fromsql(guild_id);
	if (g == NULL) //Guild not found?
		return 0;

	//Set member offline
	for(i=0; i<g->max_member; i++) {
		if(g->member[i].char_id == char_id)
			g->member[i].online = 0;
		if(g->member[i].online)
			online_count++;
	}

	if(online_count == 0)
		g->save_flag |= GS_REMOVE;

	return 1;
}

// Initialize guild sql
int inter_guild_sql_init(void)
{
	//Initialize the guild cache
   guild_db_= db_alloc(__FILE__,__LINE__,DB_INT,DB_OPT_RELEASE_DATA,sizeof(int));

   //Read exp file
	inter_guild_ReadEXP();
   
	add_timer_func_list(guild_save_timer, "guild_save_timer");
	add_timer(gettick() + 10000, guild_save_timer, 0, 0);
	return 0;
}

static int guild_db_final(DBKey key, void *data, va_list ap)
{
	struct guild *g = (struct guild*)data;
	if (g->save_flag&GS_MASK) {
	   inter_guild_tosql(g, g->save_flag&GS_MASK);
		return 1;
	}
	return 0;
}

void inter_guild_sql_final(void)
{
	guild_db_->destroy(guild_db_, guild_db_final);
	return;
}

// Get guild_id by its name. Returns 0 if not found, -1 on error.
int search_guildname(char *str)
{
	int guild_id;

	//Lookup guilds with the same name
	sprintf (tmp_sql , "SELECT guild_id FROM `%s` WHERE name='%s'",guild_db,jstrescape(str));
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		return -1;
	}

	sql_res = mysql_store_result(&mysql_handle) ;
	if(sql_res) sql_row = mysql_fetch_row(sql_res);
	
	guild_id = (sql_row&&sql_res&&sql_row[0])?atoi(sql_row[0]):0;
	mysql_free_result(sql_res);
	return guild_id;
}

// Check if guild is empty
int guild_check_empty(struct guild *g)
{
	int i;
	for(i=0;i<g->max_member;i++){
		if(g->member[i].account_id>0){
			return 0;
		}
	}
	//Let the calling function handle the guild removal in case they need
	//to do something else with it before freeing the data. [Skotlex]
	return 1;
}

unsigned int guild_nextexp(int level)
{
	if (level == 0)
		return 1;
	if (level < 100 && level > 0) // Change by hack
		return guild_exp[level-1];

	return 0;
}

// �M���h�X�L�����E�邩�m�F
int guild_checkskill(struct guild *g,int id) {

	int idx = id - GD_SKILLBASE;


	if(idx < 0 || idx >= MAX_GUILDSKILL)

		return 0;

	return g->skill[idx].lv;

}

// �M���h�̏��̍Čv�Z
int guild_calcinfo(struct guild *g)
{
	int i,c;
	unsigned int nextexp;
	struct guild before=*g;

	// �X�L��ID�̐ݒ�
	for(i=0;i<MAX_GUILDSKILL;i++)
		g->skill[i].id=i+GD_SKILLBASE;

	// �M���h���x��
	if(g->guild_lv<=0) g->guild_lv=1;
	nextexp = guild_nextexp(g->guild_lv);
	if(nextexp > 0) {
		while(g->exp >= nextexp && nextexp > 0){	//fixed guild exp overflow [Kevin]
			g->exp-=nextexp;
			g->guild_lv++;
			g->skill_point++;
			nextexp = guild_nextexp(g->guild_lv);
		}
	}

	// �M���h�̎��̌o���l
	g->next_exp = guild_nextexp(g->guild_lv);

	// �����o����i�M���h�g���K�p�j
	g->max_member = 16 + guild_checkskill(g, GD_EXTENSION) * 6; //Guild Extention skill - currently adds 6 to max per skill lv.
	if(g->max_member > MAX_GUILD)
	{	
		ShowError("Guild %d:%s has capacity for too many guild members (%d), max supported is %d\n", g->guild_id, g->name, g->max_member, MAX_GUILD);
		g->max_member = MAX_GUILD;
	}
	
	// ���σ��x���ƃI�����C���l��
	g->average_lv=0;
	g->connect_member=0;
	for(i=c=0;i<g->max_member;i++){
		if(g->member[i].account_id>0){
			g->average_lv+=g->member[i].lv;
			c++;

			if(g->member[i].online>0)
				g->connect_member++;
		}
	}
	if(c) g->average_lv/=c;

	// �S�f�[�^�𑗂�K�v���E�肻��
	if(g->max_member!=before.max_member	||
		g->guild_lv!=before.guild_lv		||
		g->skill_point!=before.skill_point	){
		mapif_guild_info(-1,g);
		return 1;
	}

	return 0;
}

//-------------------------------------------------------------------
// map server�ւ̒ʐM

// �M���h�쐬��
int mapif_guild_created(int fd,int account_id,struct guild *g)
{
	WFIFOW(fd,0)=0x3830;
	WFIFOL(fd,2)=account_id;
	if(g!=NULL){
		WFIFOL(fd,6)=g->guild_id;
		ShowInfo("int_guild: Guild created (%d - %s)\n",g->guild_id,g->name);
	}else{
		WFIFOL(fd,6)=0;
	}
	WFIFOSET(fd,10);
	return 0;
}
// �M���h��񌩂��炸
int mapif_guild_noinfo(int fd,int guild_id)
{
	unsigned char buf[12];
	WBUFW(buf,0)=0x3831;
	WBUFW(buf,2)=8;
	WBUFL(buf,4)=guild_id;
	ShowWarning("int_guild: info not found %d\n",guild_id);
	if(fd<0)
		mapif_sendall(buf,8);
	else
		mapif_send(fd,buf,8);
	return 0;
}
// �M���h���܂Ƃߑ���
int mapif_guild_info(int fd,struct guild *g)
{
	unsigned char buf[8+sizeof(struct guild)];
	WBUFW(buf,0)=0x3831;
	memcpy(buf+4,g,sizeof(struct guild));
	WBUFW(buf,2)=4+sizeof(struct guild);
	if(fd<0)
		mapif_sendall(buf,WBUFW(buf,2));
	else
		mapif_send(fd,buf,WBUFW(buf,2));
	return 0;
}

// �����o�ǉ���
int mapif_guild_memberadded(int fd,int guild_id,int account_id,int char_id,int flag)
{
	WFIFOW(fd,0)=0x3832;
	WFIFOL(fd,2)=guild_id;
	WFIFOL(fd,6)=account_id;
	WFIFOL(fd,10)=char_id;
	WFIFOB(fd,14)=flag;
	WFIFOSET(fd,15);
	return 0;
}
// �E��/�Ǖ��ʒm
int mapif_guild_leaved(int guild_id,int account_id,int char_id,int flag,
	const char *name,const char *mes)
{
	unsigned char buf[128];
	WBUFW(buf, 0)=0x3834;
	WBUFL(buf, 2)=guild_id;
	WBUFL(buf, 6)=account_id;
	WBUFL(buf,10)=char_id;
	WBUFB(buf,14)=flag;
	memcpy(WBUFP(buf,15),mes,40);
	memcpy(WBUFP(buf,55),name,NAME_LENGTH);
	mapif_sendall(buf,55+NAME_LENGTH);
//	mapif_sendall(buf,79);
	ShowInfo("int_guild: guild leaved (%d - %d: %s - %s)\n",guild_id,account_id,name,mes);
	return 0;
}

// �I�����C����Ԃ�Lv�X�V�ʒm
int mapif_guild_memberinfoshort(struct guild *g,int idx)
{
	unsigned char buf[32];
	WBUFW(buf, 0)=0x3835;
	WBUFL(buf, 2)=g->guild_id;
	WBUFL(buf, 6)=g->member[idx].account_id;
	WBUFL(buf,10)=g->member[idx].char_id;
	WBUFB(buf,14)=(unsigned char)g->member[idx].online;
	WBUFW(buf,15)=g->member[idx].lv;
	WBUFW(buf,17)=g->member[idx].class_;
	mapif_sendall(buf,19);
	return 0;
}

// ���U�ʒm
int mapif_guild_broken(int guild_id,int flag)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x3836;
	WBUFL(buf,2)=guild_id;
	WBUFB(buf,6)=flag;
	mapif_sendall(buf,7);
	ShowInfo("int_guild: Guild broken (%d)\n",guild_id);
	return 0;
}

// �M���h������
int mapif_guild_message(int guild_id,int account_id,char *mes,int len, int sfd)
{
	unsigned char buf[512];
	WBUFW(buf,0)=0x3837;
	WBUFW(buf,2)=len+12;
	WBUFL(buf,4)=guild_id;
	WBUFL(buf,8)=account_id;
	memcpy(WBUFP(buf,12),mes,len);
	mapif_sendallwos(sfd, buf,len+12);
	return 0;
}

// �M���h��{���ύX�ʒm
int mapif_guild_basicinfochanged(int guild_id,int type,const void *data,int len)
{
	unsigned char buf[2048];
	WBUFW(buf, 0)=0x3839;
	WBUFW(buf, 2)=len+10;
	WBUFL(buf, 4)=guild_id;
	WBUFW(buf, 8)=type;
	memcpy(WBUFP(buf,10),data,len);
	mapif_sendall(buf,len+10);
	return 0;
}
// �M���h�����o���ύX�ʒm
int mapif_guild_memberinfochanged(int guild_id,int account_id,int char_id,
	int type,const void *data,int len)
{
	unsigned char buf[2048];
	WBUFW(buf, 0)=0x383a;
	WBUFW(buf, 2)=len+18;
	WBUFL(buf, 4)=guild_id;
	WBUFL(buf, 8)=account_id;
	WBUFL(buf,12)=char_id;
	WBUFW(buf,16)=type;
	memcpy(WBUFP(buf,18),data,len);
	mapif_sendall(buf,len+18);
	return 0;
}
// �M���h�X�L���A�b�v�ʒm
int mapif_guild_skillupack(int guild_id,int skill_num,int account_id)
{
	unsigned char buf[16];
	WBUFW(buf, 0)=0x383c;
	WBUFL(buf, 2)=guild_id;
	WBUFL(buf, 6)=skill_num;
	WBUFL(buf,10)=account_id;
	mapif_sendall(buf,14);
	return 0;
}
// �M���h����/�G�Βʒm
int mapif_guild_alliance(int guild_id1,int guild_id2,int account_id1,int account_id2,
	int flag,const char *name1,const char *name2)
{
	unsigned char buf[128];
	WBUFW(buf, 0)=0x383d;
	WBUFL(buf, 2)=guild_id1;
	WBUFL(buf, 6)=guild_id2;
	WBUFL(buf,10)=account_id1;
	WBUFL(buf,14)=account_id2;
	WBUFB(buf,18)=flag;
	memcpy(WBUFP(buf,19),name1,NAME_LENGTH);
	memcpy(WBUFP(buf,19+NAME_LENGTH),name2,NAME_LENGTH);
	mapif_sendall(buf,19+2*NAME_LENGTH);
//	memcpy(WBUFP(buf,43),name2,NAME_LENGTH);
//	mapif_sendall(buf,67);
	return 0;
}

// �M���h��E�ύX�ʒm
int mapif_guild_position(struct guild *g,int idx)
{
	unsigned char buf[128];
	WBUFW(buf,0)=0x383b;
	WBUFW(buf,2)=sizeof(struct guild_position)+12;
	WBUFL(buf,4)=g->guild_id;
	WBUFL(buf,8)=idx;
	memcpy(WBUFP(buf,12),&g->position[idx],sizeof(struct guild_position));
	mapif_sendall(buf,WBUFW(buf,2));
	return 0;
}

// �M���h���m�ύX�ʒm
int mapif_guild_notice(struct guild *g)
{
	unsigned char buf[256];
	WBUFW(buf,0)=0x383e;
	WBUFL(buf,2)=g->guild_id;
	memcpy(WBUFP(buf,6),g->mes1,60);
	memcpy(WBUFP(buf,66),g->mes2,120);
	mapif_sendall(buf,186);
	return 0;
}
// �M���h�G���u�����ύX�ʒm
int mapif_guild_emblem(struct guild *g)
{
	unsigned char buf[2048];
	WBUFW(buf,0)=0x383f;
	WBUFW(buf,2)=g->emblem_len+12;
	WBUFL(buf,4)=g->guild_id;
	WBUFL(buf,8)=g->emblem_id;
	memcpy(WBUFP(buf,12),g->emblem_data,g->emblem_len);
	mapif_sendall(buf,WBUFW(buf,2));
	return 0;
}
int mapif_guild_master_changed(struct guild *g, int position)
{
	unsigned char buf[12];
	WBUFW(buf,0)=0x3843;
	WBUFL(buf,2)=g->guild_id;
	WBUFL(buf,6)=position;
	mapif_sendall(buf,10);
	return 0;
}

int mapif_guild_castle_dataload(int castle_id,int index,int value)      // <Agit>
{
	unsigned char buf[16];
	WBUFW(buf, 0)=0x3840;
	WBUFW(buf, 2)=castle_id;
	WBUFB(buf, 4)=index;
	WBUFL(buf, 5)=value;
	mapif_sendall(buf,9);
	return 0;
}

int mapif_guild_castle_datasave(int castle_id,int index,int value)      // <Agit>
{
	unsigned char buf[16];
	WBUFW(buf, 0)=0x3841;
	WBUFW(buf, 2)=castle_id;
	WBUFB(buf, 4)=index;
	WBUFL(buf, 5)=value;
	mapif_sendall(buf,9);
	return 0;
}

int mapif_guild_castle_alldataload(int fd) {
	struct guild_castle* gc = (struct guild_castle *)aMalloc(sizeof(struct guild_castle));
	int i, len = 4;

	WFIFOW(fd,0) = 0x3842;
	sprintf(tmp_sql,"SELECT * FROM `%s` ORDER BY `castle_id`", guild_castle_db);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}
	sql_res = mysql_store_result(&mysql_handle);
	if (sql_res) {
		for(i = 0; ((sql_row = mysql_fetch_row(sql_res)) && i < MAX_GUILDCASTLE); i++) {
			memset(gc, 0, sizeof(struct guild_castle));
			gc->castle_id = atoi(sql_row[0]);
			gc->guild_id =  atoi(sql_row[1]);
			gc->economy = atoi(sql_row[2]);
			gc->defense = atoi(sql_row[3]);
			gc->triggerE = atoi(sql_row[4]);
			gc->triggerD = atoi(sql_row[5]);
			gc->nextTime = atoi(sql_row[6]);
			gc->payTime = atoi(sql_row[7]);
			gc->createTime = atoi(sql_row[8]);
			gc->visibleC = atoi(sql_row[9]);
			gc->guardian[0].visible = atoi(sql_row[10]);
			gc->guardian[1].visible = atoi(sql_row[11]);
			gc->guardian[2].visible = atoi(sql_row[12]);
			gc->guardian[3].visible = atoi(sql_row[13]);
			gc->guardian[4].visible = atoi(sql_row[14]);
			gc->guardian[5].visible = atoi(sql_row[15]);
			gc->guardian[6].visible = atoi(sql_row[16]);
			gc->guardian[7].visible = atoi(sql_row[17]);
			gc->guardian[0].visible = atoi(sql_row[18]);
			gc->guardian[1].visible = atoi(sql_row[19]);
			gc->guardian[2].visible = atoi(sql_row[20]);
			gc->guardian[3].visible = atoi(sql_row[21]);
			gc->guardian[4].visible = atoi(sql_row[22]);
			gc->guardian[5].visible = atoi(sql_row[23]);
			gc->guardian[6].visible = atoi(sql_row[24]);
			gc->guardian[7].visible = atoi(sql_row[25]);
			memcpy(WFIFOP(fd,len), gc, sizeof(struct guild_castle));
			len += sizeof(struct guild_castle);
		}
		mysql_free_result(sql_res);
	}
	WFIFOW(fd,2) = len;
	WFIFOSET(fd,len);
	
	aFree(gc);

	return 0;
}


//-------------------------------------------------------------------
// map server����̒ʐM


// �M���h�쐬�v��
int mapif_parse_CreateGuild(int fd,int account_id,char *name,struct guild_member *master)
{
	struct guild *g;
	int i=0;
#ifdef NOISY
	ShowInfo("Creating Guild (%s)\n", name);
#endif
	if(search_guildname(name) != 0){
		ShowInfo("int_guild: guild with same name exists [%s]\n",name);
		mapif_guild_created(fd,account_id,NULL);
		return 0;
	}
	// Check Authorised letters/symbols in the name of the character
	if (char_name_option == 1) { // only letters/symbols in char_name_letters are authorised
		for (i = 0; i < NAME_LENGTH && name[i]; i++)
			if (strchr(char_name_letters, name[i]) == NULL) {
				mapif_guild_created(fd,account_id,NULL);
				return 0;
			}
	} else if (char_name_option == 2) { // letters/symbols in char_name_letters are forbidden
		for (i = 0; i < NAME_LENGTH && name[i]; i++)
			if (strchr(char_name_letters, name[i]) != NULL) {
				mapif_guild_created(fd,account_id,NULL);
				return 0;
			}
	}

	g = (struct guild *)aMalloc(sizeof(struct guild));
	memset(g,0,sizeof(struct guild));

	memcpy(g->name,name,NAME_LENGTH);
	memcpy(g->master,master->name,NAME_LENGTH);
	memcpy(&g->member[0],master,sizeof(struct guild_member));

	g->position[0].mode=0x11;
	strcpy(g->position[0].name,"GuildMaster");
	strcpy(g->position[MAX_GUILDPOSITION-1].name,"Newbie");
	for(i=1;i<MAX_GUILDPOSITION-1;i++)
		sprintf(g->position[i].name,"Position %d",i+1);

	// Initialize guild property
	g->max_member=16;
	g->average_lv=master->lv;
	for(i=0;i<MAX_GUILDSKILL;i++)
		g->skill[i].id=i + GD_SKILLBASE;
	g->guild_id= -1; //Request to create guild.
	if (!inter_guild_tosql(g,GS_BASIC|GS_POSITION|GS_SKILL)) {
		//Failed to Create guild....
		ShowError("Failed to create Guild %s (Guild Master: %s)\n", g->name, g->master);
		mapif_guild_created(fd,account_id,NULL);
		aFree(g);
		return 0;
	}
	ShowInfo("Created Guild %d - %s (Guild Master: %s)\n", g->guild_id, g->name, g->master);
	//Add to cache
	idb_put(guild_db_, g->guild_id, g);

	// Report to client
	mapif_guild_created(fd,account_id,g);
	mapif_guild_info(fd,g);

	if(log_inter)
		inter_log("guild %s (id=%d) created by master %s (id=%d)" RETCODE,
			name, g->guild_id, master->name, master->account_id );

	return 0;
}
// Return guild info to client
int mapif_parse_GuildInfo(int fd,int guild_id)
{
	struct guild * g = inter_guild_fromsql(guild_id); //We use this because on start-up the info of castle-owned guilds is requied. [Skotlex]
	if(g){
		if (!guild_calcinfo(g))
			mapif_guild_info(fd,g);
		//inter_guild_tosql(g,GS_BASIC); // Change guild
	}else
		mapif_guild_noinfo(fd,guild_id);
	return 0;
}
// Add member to guild
int mapif_parse_GuildAddMember(int fd,int guild_id,struct guild_member *m)
{
	struct guild * g;
	int i;

	g = inter_guild_fromsql(guild_id);
	if(g==NULL){
		mapif_guild_memberadded(fd,guild_id,m->account_id,m->char_id,1);
		return 0;
	}

	for(i=0;i<g->max_member;i++){
		if(g->member[i].account_id==0){

			memcpy(&g->member[i],m,sizeof(struct guild_member));
			mapif_guild_memberadded(fd,guild_id,m->account_id,m->char_id,0);
			if (!guild_calcinfo(g)) //Send members if it was not invoked.
				mapif_guild_info(fd,g);
			g->save_flag |= (GS_BASIC|GS_MEMBER);
			if (g->save_flag&GS_REMOVE)
				g->save_flag&=~GS_REMOVE;
			return 0;
		}
	}
	mapif_guild_memberadded(fd,guild_id,m->account_id,m->char_id,1);
	return 0;
}
// Delete member from guild
int mapif_parse_GuildLeave(int fd,int guild_id,int account_id,int char_id,int flag,const char *mes)
{
	struct guild * g = inter_guild_fromsql(guild_id);

	if(g){
		int i;
		for(i=0;i<g->max_member;i++){
			if( g->member[i].account_id==account_id &&
				g->member[i].char_id==char_id){
				if(flag){	// �Ǖ��̏ꍇ�Ǖ����X�g�ɓ����
					int j;
					for(j=0;j<MAX_GUILDEXPLUSION;j++){
						if(g->explusion[j].account_id==0)
							break;
					}
					if(j==MAX_GUILDEXPLUSION){	// ��t�Ȃ̂ŌÂ��̂�����
						for(j=0;j<MAX_GUILDEXPLUSION-1;j++)
							g->explusion[j]=g->explusion[j+1];
						j=MAX_GUILDEXPLUSION-1;
					}
					g->explusion[j].account_id=account_id;
					memcpy(g->explusion[j].acc,"dummy",NAME_LENGTH-1);
					memcpy(g->explusion[j].name,g->member[i].name,NAME_LENGTH-1);
					memcpy(g->explusion[j].mes,mes,40);
				}

				mapif_guild_leaved(guild_id,account_id,char_id,flag,g->member[i].name,mes);
				memset(&g->member[i],0,sizeof(struct guild_member));

				if(!guild_check_empty(g)) {
					break;
				}
				//Guild empty? break it.	
				mapif_parse_BreakGuild(-1,guild_id); //Break the guild.
				return 0;
			}
		}
		//Update member info.
		if (!guild_calcinfo(g))
			mapif_guild_info(fd,g);
		g->save_flag |= (GS_BASIC|GS_MEMBER|GS_EXPULSION);
	}else{
		sprintf(tmp_sql, "UPDATE `%s` SET `guild_id`='0' WHERE `account_id`='%d' AND `char_id`='%d'",char_db, account_id,char_id);
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
			ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		}
		/* mapif_guild_leaved(guild_id,account_id,char_id,flag,g->member[i].name,mes);	*/
	}

	return 0;
}
// Change member info
int mapif_parse_GuildChangeMemberInfoShort(int fd,int guild_id,
	int account_id,int char_id,int online,int lv,int class_)
{
	// Could speed up by manipulating only guild_member
	struct guild * g;
	int i,alv,c;
	int prev_count;

	g = inter_guild_fromsql(guild_id);
	if(g==NULL)
		return 0;
	
	prev_count = g->connect_member;
	g->connect_member=0;

	for(i=0,alv=0,c=0;i<g->max_member;i++){
		if(g->member[i].account_id==account_id &&
			g->member[i].char_id==char_id){

			g->member[i].online=online;
			g->member[i].lv=lv;
			g->member[i].class_=class_;
			mapif_guild_memberinfoshort(g,i);
		}
		if( g->member[i].account_id>0 ){
			alv+=g->member[i].lv;
			c++;
		}
		if( g->member[i].online )
			g->connect_member++;
	}

	if (c)
	{
		alv= alv/c;
		if (g->connect_member != prev_count || g->average_lv != alv)
		{
			g->average_lv=alv;
			g->save_flag |= GS_BASIC; //FIXME: Save the base guild just because the avl/connect count changed?
		}
		if (g->save_flag & GS_REMOVE)
			g->save_flag &= ~GS_REMOVE;
	}
	g->save_flag |= GS_MEMBER; //Update guild member data
	return 0;
}

// BreakGuild
int mapif_parse_BreakGuild(int fd,int guild_id)
{
	struct guild * g;
	
	g = inter_guild_fromsql(guild_id);
	if(g==NULL)
		return 0;

	// Delete guild from sql
	//printf("- Delete guild %d from guild\n",guild_id);
	sprintf(tmp_sql, "DELETE FROM `%s` WHERE `guild_id`='%d'",guild_db, guild_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}

	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `guild_id` = '%d'", guild_member_db, guild_id);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}

	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `guild_id` = '%d'", guild_castle_db, guild_id);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}

	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `guild_id` = '%d'", guild_storage_db, guild_id);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}

	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `guild_id` = '%d' OR `alliance_id` = '%d'", guild_alliance_db, guild_id, guild_id);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}

	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `guild_id` = '%d'", guild_position_db, guild_id);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}

	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `guild_id` = '%d'", guild_skill_db, guild_id);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}

	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `guild_id` = '%d'", guild_expulsion_db, guild_id);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}

	//printf("- Update guild %d of char\n",guild_id);
	sprintf(tmp_sql, "UPDATE `%s` SET `guild_id`='0' WHERE `guild_id`='%d'",char_db, guild_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
	}

	mapif_guild_broken(guild_id,0);
	
	if(log_inter)
		inter_log("guild %s (id=%d) broken" RETCODE,g->name,guild_id);
	
	//Remove the guild from memory. [Skotlex]
	g = idb_remove(guild_db_, guild_id);
	return 0;
}

// �M���h���b�Z�[�W���M
int mapif_parse_GuildMessage(int fd,int guild_id,int account_id,char *mes,int len)
{
	return mapif_guild_message(guild_id,account_id,mes,len, fd);
}
// �M���h��{�f�[�^�ύX�v��
int mapif_parse_GuildBasicInfoChange(int fd,int guild_id,
	int type,const char *data,int len)
{
	struct guild * g;
//	int dd=*((int *)data);
	short dw=*((short *)data);
	g = inter_guild_fromsql(guild_id);
	if(g==NULL)
		return 0;

	switch(type){
	case GBI_GUILDLV: {
		ShowDebug("GBI_GUILDLV\n");
			if(dw>0 && g->guild_lv+dw<=50){
				g->guild_lv+=dw;
				g->skill_point+=dw;
			}else if(dw<0 && g->guild_lv+dw>=1)
				g->guild_lv+=dw;
			mapif_guild_info(-1,g);
			g->save_flag |= GS_BASIC;
		} return 0;
	default:
		ShowError("int_guild: GuildBasicInfoChange: Unknown type %d\n",type);
		break;
	}
	mapif_guild_basicinfochanged(guild_id,type,data,len);
	return 0;
}

// �M���h�����o�f�[�^�ύX�v��
int mapif_parse_GuildMemberInfoChange(int fd,int guild_id,int account_id,int char_id,
	int type,const char *data,int len)
{
	// Could make some improvement in speed, because only change guild_member
	int i;
	struct guild * g;

	g = inter_guild_fromsql(guild_id);
	if(g==NULL)
		return 0;

	for(i=0;i<g->max_member;i++)
		if(	g->member[i].account_id==account_id &&
			g->member[i].char_id==char_id )
				break;
	if(i==g->max_member){
		ShowWarning("int_guild: GuildMemberChange: Not found %d,%d in guild (%d - %s)\n",
			account_id,char_id,guild_id,g->name);
		return 0;
	}
	switch(type){
	case GMI_POSITION:	// ��E
	  {
	    g->member[i].position=*((int *)data);
	    mapif_guild_memberinfochanged(guild_id,account_id,char_id,type,data,len);
	    g->save_flag |= (GS_BASIC|GS_MEMBER);
	    break;
	  }
	case GMI_EXP:
	{	// EXP
		unsigned int exp, old_exp=g->member[i].exp;
		g->member[i].exp=*((unsigned int *)data);
		if (g->member[i].exp > (signed int)old_exp && old_exp < INT_MAX)
		{
			exp = g->member[i].exp - old_exp;
			if (guild_exp_rate != 100)
				exp = exp*guild_exp_rate/100;
			if (exp > UINT_MAX - g->exp)
				g->exp = UINT_MAX;
			else
				g->exp+=exp;
			guild_calcinfo(g);
			mapif_guild_basicinfochanged(guild_id,GBI_EXP,&g->exp,4);
			g->save_flag |= GS_BASIC;
		}
		mapif_guild_memberinfochanged(guild_id,account_id,char_id,type,data,len);
		g->save_flag |= GS_MEMBER;
		break;
	}
	case GMI_HAIR:
	{
		g->member[i].hair=*((int *)data);
		mapif_guild_memberinfochanged(guild_id,account_id,char_id,type,data,len);
		g->save_flag |= GS_MEMBER; //Save new data.
		break;
	}
	case GMI_HAIR_COLOR:
	{
		g->member[i].hair_color=*((int *)data);
		mapif_guild_memberinfochanged(guild_id,account_id,char_id,type,data,len);
		g->save_flag |= GS_MEMBER; //Save new data.
		break;
	}
	case GMI_GENDER:
	{
		g->member[i].gender=*((int *)data);
		mapif_guild_memberinfochanged(guild_id,account_id,char_id,type,data,len);
		g->save_flag |= GS_MEMBER; //Save new data.
		break;
	}
	case GMI_CLASS:
	{
		g->member[i].class_=*((int *)data);
		mapif_guild_memberinfochanged(guild_id,account_id,char_id,type,data,len);
		g->save_flag |= GS_MEMBER; //Save new data.
		break;
	}
	case GMI_LEVEL:
	{
		g->member[i].lv=*((int *)data);
		mapif_guild_memberinfochanged(guild_id,account_id,char_id,type,data,len);
		g->save_flag |= GS_MEMBER; //Save new data.
		break;
	}
	default:
	  ShowError("int_guild: GuildMemberInfoChange: Unknown type %d\n",type);
	  break;
	}
	return 0;
}

int inter_guild_sex_changed(int guild_id,int account_id,int char_id, int gender)
{
	return mapif_parse_GuildMemberInfoChange(0, guild_id, account_id, char_id, GMI_GENDER, (const char*)&gender, sizeof(gender));
}

// �M���h��E���ύX�v��
int mapif_parse_GuildPosition(int fd,int guild_id,int idx,struct guild_position *p)
{
	// Could make some improvement in speed, because only change guild_position
	struct guild * g;

	g = inter_guild_fromsql(guild_id);
	if(g==NULL || idx<0 || idx>=MAX_GUILDPOSITION)
		return 0;

	memcpy(&g->position[idx],p,sizeof(struct guild_position));
	mapif_guild_position(g,idx);
	ShowInfo("int_guild: position data changed (Guild %d, position %d)\n",guild_id, idx);
	g->save_flag |= GS_POSITION; // Change guild_position
	return 0;
}
// �M���h�X�L���A�b�v�v��
int mapif_parse_GuildSkillUp(int fd,int guild_id,int skill_num,int account_id)
{
	// Could make some improvement in speed, because only change guild_position
	struct guild * g;
	int idx = skill_num - GD_SKILLBASE;

	g = inter_guild_fromsql(guild_id);
	if(g == NULL || idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;

	if(g->skill_point>0 && g->skill[idx].id>0 &&
		g->skill[idx].lv<10 ){
		g->skill[idx].lv++;
		g->skill_point--;
		if (!guild_calcinfo(g))
			mapif_guild_info(-1,g);
		mapif_guild_skillupack(guild_id,skill_num,account_id);
		ShowDebug("int_guild: skill %d up\n",skill_num);
		g->save_flag |= (GS_BASIC|GS_SKILL); // Change guild & guild_skill
	}

	return 0;
}

//Manual deletion of an alliance when partnering guild does not exists. [Skotlex]
static int mapif_parse_GuildDeleteAlliance(struct guild *g, int guild_id, int account_id1, int account_id2, int flag)
{
	int i;
	char name[NAME_LENGTH];
	for(i=0;i<MAX_GUILDALLIANCE;i++)
		if(g->alliance[i].guild_id == guild_id)
		{
			strcpy(name, g->alliance[i].name);
			g->alliance[i].guild_id=0;
			break;
		}
	if (i == MAX_GUILDALLIANCE)
		return -1;
	
	mapif_guild_alliance(g->guild_id,guild_id,account_id1,account_id2,flag,g->name,name);
	g->save_flag |= GS_ALLIANCE;
	return 0;
}
// �M���h�����v��
int mapif_parse_GuildAlliance(int fd,int guild_id1,int guild_id2,
	int account_id1,int account_id2,int flag)
{
	// Could speed up
	struct guild *g[2];
	int j,i;
	g[0] = inter_guild_fromsql(guild_id1);
	g[1] = inter_guild_fromsql(guild_id2);

	if(g[0] && g[1]==NULL && (flag&0x8)) //Requested to remove an alliance with a not found guild.
		return mapif_parse_GuildDeleteAlliance(g[0], guild_id2,
			account_id1, account_id2, flag); //Try to do a manual removal of said guild.
		
	if(g[0]==NULL || g[1]==NULL)
		return 0;

	if(!(flag&0x8)){
		for(i=0;i<2-(flag&1);i++){
			for(j=0;j<MAX_GUILDALLIANCE;j++)
				if(g[i]->alliance[j].guild_id==0){
					g[i]->alliance[j].guild_id=g[1-i]->guild_id;
					memcpy(g[i]->alliance[j].name,g[1-i]->name,NAME_LENGTH-1);
					g[i]->alliance[j].opposition=flag&1;
					break;
				}
		}
	}else{	// �֌W����
		for(i=0;i<2-(flag&1);i++){
			for(j=0;j<MAX_GUILDALLIANCE;j++)
				if(g[i]->alliance[j].guild_id==g[1-i]->guild_id &&
					g[i]->alliance[j].opposition==(flag&1)){
					g[i]->alliance[j].guild_id=0;
					break;
				}
		}
	}
	mapif_guild_alliance(guild_id1,guild_id2,account_id1,account_id2,flag,
		g[0]->name,g[1]->name);
	g[0]->save_flag |= GS_ALLIANCE;
	g[1]->save_flag |= GS_ALLIANCE;
	return 0;
}
// �M���h���m�ύX�v��
int mapif_parse_GuildNotice(int fd,int guild_id,const char *mes1,const char *mes2)
{
	struct guild *g;

	g = inter_guild_fromsql(guild_id);
	if(g==NULL)
		return 0;

	memcpy(g->mes1,mes1,60);
	memcpy(g->mes2,mes2,120);
	g->save_flag |= GS_BASIC;	//Change mes of guild
	return mapif_guild_notice(g);
}
// �M���h�G���u�����ύX�v��
int mapif_parse_GuildEmblem(int fd,int len,int guild_id,int dummy,const char *data)
{
	struct guild * g;

	g = inter_guild_fromsql(guild_id);
	if(g==NULL)
		return 0;
	memcpy(g->emblem_data,data,len);
	g->emblem_len=len;
	g->emblem_id++;
	g->save_flag |= GS_BASIC;	//Change guild
	return mapif_guild_emblem(g);
}

int mapif_parse_GuildCastleDataLoad(int fd,int castle_id,int index)     // <Agit>
{
	struct guild_castle gc;
	if (!inter_guildcastle_fromsql(castle_id, &gc)) {
		return mapif_guild_castle_dataload(castle_id,0,0);
	}
	switch(index){
	case 1: return mapif_guild_castle_dataload(gc.castle_id,index,gc.guild_id); break;
	case 2: return mapif_guild_castle_dataload(gc.castle_id,index,gc.economy); break;
	case 3: return mapif_guild_castle_dataload(gc.castle_id,index,gc.defense); break;
	case 4: return mapif_guild_castle_dataload(gc.castle_id,index,gc.triggerE); break;
	case 5: return mapif_guild_castle_dataload(gc.castle_id,index,gc.triggerD); break;
	case 6: return mapif_guild_castle_dataload(gc.castle_id,index,gc.nextTime); break;
	case 7: return mapif_guild_castle_dataload(gc.castle_id,index,gc.payTime); break;
	case 8: return mapif_guild_castle_dataload(gc.castle_id,index,gc.createTime); break;
	case 9: return mapif_guild_castle_dataload(gc.castle_id,index,gc.visibleC); break;
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
		return mapif_guild_castle_dataload(gc.castle_id,index,gc.guardian[index-10].visible); break;
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
		return mapif_guild_castle_dataload(gc.castle_id,index,gc.guardian[index-18].hp); break;
	default:
		ShowError("mapif_parse_GuildCastleDataLoad ERROR!! (Not found index=%d)\n", index);
		return 0;
	}
}

int mapif_parse_GuildCastleDataSave(int fd,int castle_id,int index,int value)   // <Agit>
{
	struct guild_castle gc;
	if(!inter_guildcastle_fromsql(castle_id, &gc))
		return mapif_guild_castle_datasave(castle_id,index,value);

	switch(index){
	case 1:
		if( gc.guild_id!=value ){
			int gid=(value)?value:gc.guild_id;
			struct guild *g=idb_get(guild_db_, gid);
			if(log_inter)
				inter_log("guild %s (id=%d) %s castle id=%d" RETCODE,
					(g)?g->name:"??" ,gid, (value)?"occupy":"abandon", castle_id);
		}
		gc.guild_id = value;
		if(gc.guild_id == 0) {
			//Delete guardians.
			memset(&gc.guardian, 0, sizeof(gc.guardian));
		}
		break;
	case 2: gc.economy = value; break;
	case 3: gc.defense = value; break;
	case 4: gc.triggerE = value; break;
	case 5: gc.triggerD = value; break;
	case 6: gc.nextTime = value; break;
	case 7: gc.payTime = value; break;
	case 8: gc.createTime = value; break;
	case 9: gc.visibleC = value; break;
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
		gc.guardian[index-10].visible = value; break;
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
		gc.guardian[index-18].hp = value; break;	// end additions [Valaris]
	default:
		ShowError("mapif_parse_GuildCastleDataSave ERROR!! (Not found index=%d)\n", index);
		return 0;
	}
	inter_guildcastle_tosql(&gc);
	mapif_guild_castle_datasave(gc.castle_id,index,value);
	return 0;
}

int mapif_parse_GuildMasterChange(int fd, int guild_id, const char* name, int len)
{
	struct guild * g;
	struct guild_member gm;
	int pos;

	g = inter_guild_fromsql(guild_id);
	
	if(g==NULL || len > NAME_LENGTH)
		return 0;
	
	for (pos = 0; pos < g->max_member && strncmp(g->member[pos].name, name, len); pos++);

	if (pos == g->max_member)
		return 0; //Character not found??
	
	memcpy(&gm, &g->member[pos], sizeof (struct guild_member));
	memcpy(&g->member[pos], &g->member[0], sizeof(struct guild_member));
	memcpy(&g->member[0], &gm, sizeof(struct guild_member));

	g->member[pos].position = g->member[0].position;
	g->member[0].position = 0; //Position 0: guild Master.
	strncpy(g->master, name, len);
	if (len < NAME_LENGTH)
		g->master[len] = '\0';

	ShowInfo("int_guild: Guildmaster Changed to %s (Guild %d - %s)\n",g->master, guild_id, g->name);
	g->save_flag |= (GS_BASIC|GS_POSITION); //Save main data and member data.
	return mapif_guild_master_changed(g, pos);
}

// �M���h�`�F�b�N�v��
int mapif_parse_GuildCheck(int fd,int guild_id,int account_id,int char_id)
{
	// What does this mean? Check if belong to another guild?
	return 0;
}

// map server ����̒ʐM
// �E�P�p�P�b�g�̂݉�͂��邱��
// �E�p�P�b�g���f�[�^��inter.c�ɃZ�b�g���Ă�������
// �E�p�P�b�g���`�F�b�N��ARFIFOSKIP�͌Ăяo�����ōs����̂ōs���Ă͂Ȃ�Ȃ�
// �E�G���[�Ȃ�0(false)�A�����łȂ��Ȃ�1(true)���������Ȃ���΂Ȃ�Ȃ�
int inter_guild_parse_frommap(int fd)
{
	switch(RFIFOW(fd,0)){
	case 0x3030: mapif_parse_CreateGuild(fd,RFIFOL(fd,4),(char*)RFIFOP(fd,8),(struct guild_member *)RFIFOP(fd,32)); break;
	case 0x3031: mapif_parse_GuildInfo(fd,RFIFOL(fd,2)); break;
	case 0x3032: mapif_parse_GuildAddMember(fd,RFIFOL(fd,4),(struct guild_member *)RFIFOP(fd,8)); break;
	case 0x3033: mapif_parse_GuildMasterChange(fd,RFIFOL(fd,4),(const char*)RFIFOP(fd,8),RFIFOW(fd,2)-8); break;
	case 0x3034: mapif_parse_GuildLeave(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOB(fd,14),(const char*)RFIFOP(fd,15)); break;
	case 0x3035: mapif_parse_GuildChangeMemberInfoShort(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOB(fd,14),RFIFOW(fd,15),RFIFOW(fd,17)); break;
	case 0x3036: mapif_parse_BreakGuild(fd,RFIFOL(fd,2)); break;
	case 0x3037: mapif_parse_GuildMessage(fd,RFIFOL(fd,4),RFIFOL(fd,8),(char*)RFIFOP(fd,12),RFIFOW(fd,2)-12); break;
	case 0x3038: mapif_parse_GuildCheck(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10)); break;
	case 0x3039: mapif_parse_GuildBasicInfoChange(fd,RFIFOL(fd,4),RFIFOW(fd,8),(const char*)RFIFOP(fd,10),RFIFOW(fd,2)-10); break;
	case 0x303A: mapif_parse_GuildMemberInfoChange(fd,RFIFOL(fd,4),RFIFOL(fd,8),RFIFOL(fd,12),RFIFOW(fd,16),(const char*)RFIFOP(fd,18),RFIFOW(fd,2)-18); break;
	case 0x303B: mapif_parse_GuildPosition(fd,RFIFOL(fd,4),RFIFOL(fd,8),(struct guild_position *)RFIFOP(fd,12)); break;
	case 0x303C: mapif_parse_GuildSkillUp(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10)); break;
	case 0x303D: mapif_parse_GuildAlliance(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOL(fd,14),RFIFOB(fd,18)); break;
	case 0x303E: mapif_parse_GuildNotice(fd,RFIFOL(fd,2),(const char*)RFIFOP(fd,6),(const char*)RFIFOP(fd,66)); break;
	case 0x303F: mapif_parse_GuildEmblem(fd,RFIFOW(fd,2)-12,RFIFOL(fd,4),RFIFOL(fd,8),(const char*)RFIFOP(fd,12)); break;
	case 0x3040: mapif_parse_GuildCastleDataLoad(fd,RFIFOW(fd,2),RFIFOB(fd,4)); break;
	case 0x3041: mapif_parse_GuildCastleDataSave(fd,RFIFOW(fd,2),RFIFOB(fd,4),RFIFOL(fd,5)); break;

	default:
		return 0;
	}
	return 1;
}

int inter_guild_mapif_init(int fd)
{
	return mapif_guild_castle_alldataload(fd);
}

// �T�[�o�[����E�ޗv���i�L�����폜�p�j
int inter_guild_leave(int guild_id,int account_id,int char_id)
{
	return mapif_parse_GuildLeave(-1,guild_id,account_id,char_id,0,"** Character Deleted **");
}

int inter_guild_broken(int guild_id)
{
	return mapif_guild_broken(guild_id, 0);
}
