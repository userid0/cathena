// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/db.h"
#include "../common/malloc.h"
#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/strlib.h"
#include "charserverdb_txt.h"
#include "guilddb.h"
#include <stdio.h>
#include <string.h>

#define START_GUILD_NUM 1

/// internal structure
typedef struct GuildDB_TXT
{
	GuildDB vtable;      // public interface

	CharServerDB_TXT* owner;
	DBMap* guilds;       // in-memory guild storage
	int next_guild_id;   // auto_increment

	const char* guild_db; // guild data storage file

} GuildDB_TXT;

/// internal functions
static bool guild_db_txt_init(GuildDB* self);
static void guild_db_txt_destroy(GuildDB* self);
static bool guild_db_txt_sync(GuildDB* self);
static bool guild_db_txt_create(GuildDB* self, struct guild* g);
static bool guild_db_txt_remove(GuildDB* self, const int guild_id);
static bool guild_db_txt_save(GuildDB* self, const struct guild* g);
static bool guild_db_txt_load_num(GuildDB* self, struct guild* g, int guild_id);
static bool guild_db_txt_name2id(GuildDB* self, const char* name, int* guild_id);

static bool mmo_guild_fromstr(struct guild* g, char* str);
static bool mmo_guild_tostr(const struct guild* g, char* str);
static bool mmo_guild_sync(GuildDB_TXT* db);

/// public constructor
GuildDB* guild_db_txt(CharServerDB_TXT* owner)
{
	GuildDB_TXT* db = (GuildDB_TXT*)aCalloc(1, sizeof(GuildDB_TXT));

	// set up the vtable
	db->vtable.init      = &guild_db_txt_init;
	db->vtable.destroy   = &guild_db_txt_destroy;
	db->vtable.sync      = &guild_db_txt_sync;
	db->vtable.create    = &guild_db_txt_create;
	db->vtable.remove    = &guild_db_txt_remove;
	db->vtable.save      = &guild_db_txt_save;
	db->vtable.load_num  = &guild_db_txt_load_num;
	db->vtable.name2id   = &guild_db_txt_name2id;

	// initialize to default values
	db->owner = owner;
	db->guilds = NULL;
	db->next_guild_id = START_GUILD_NUM;

	// other settings
	db->guild_db = db->owner->file_guilds;

	return &db->vtable;
}


/* ------------------------------------------------------------------------- */


static bool guild_db_txt_init(GuildDB* self)
{
	GuildDB_TXT* db = (GuildDB_TXT*)self;
	DBMap* guilds;

	char line[16384];
	FILE* fp;

	// create guild database
	db->guilds = idb_alloc(DB_OPT_RELEASE_DATA);
	guilds = db->guilds;

	// open data file
	fp = fopen(db->guild_db, "r");
	if( fp == NULL )
		return 1;

	// load data file
/*
	while( fgets(line, sizeof(line), fp) )
	{
		j = 0;
		if (sscanf(line, "%d\t%%newid%%\n%n", &i, &j) == 1 && j > 0 && guild_newid <= i) {
			guild_newid = i;
			continue;
		}

		g = (struct guild *) aCalloc(sizeof(struct guild), 1);
		if(g == NULL){
			ShowFatalError("int_guild: out of memory!\n");
			exit(EXIT_FAILURE);
		}
		if (inter_guild_fromstr(line, g) == 0 && g->guild_id > 0)
		{
			if (g->guild_id >= guild_newid)
				guild_newid = g->guild_id + 1;
			idb_put(guild_db, g->guild_id, g);
			guild_check_empty(g);
			guild_calcinfo(g);
		} else {
			ShowError("int_guild: broken data [%s] line %d\n", guild_txt, c);
			aFree(g);
		}
		c++;
	}
*/

	// close data file
	fclose(fp);

	return true;
}

static void guild_db_txt_destroy(GuildDB* self)
{
	GuildDB_TXT* db = (GuildDB_TXT*)self;
	DBMap* guilds = db->guilds;

	// write data
	mmo_guild_sync(db);

	// delete guild database
	guilds->destroy(guilds, NULL);
	db->guilds = NULL;

	// delete entire structure
	aFree(db);
}

static bool guild_db_txt_sync(GuildDB* self)
{
	GuildDB_TXT* db = (GuildDB_TXT*)self;
	return mmo_guild_sync(db);
}

static bool guild_db_txt_create(GuildDB* self, struct guild* g)
{
/*
*/
}

static bool guild_db_txt_remove(GuildDB* self, const int guild_id)
{
/*
*/
}

static bool guild_db_txt_save(GuildDB* self, const struct guild* g)
{
/*
*/
}

static bool guild_db_txt_load_num(GuildDB* self, struct guild* g, int guild_id)
{
/*
	return (struct guild*)idb_get(guild_db, guild_id);
*/
}

static bool guild_db_txt_name2id(GuildDB* self, const char* name, int* guild_id)
{
/*
	DBIterator* iter;
	struct guild* g;

	iter = guild_db->iterator(guild_db);
	for( g = (struct guild*)iter->first(iter,NULL); iter->exists(iter); g = (struct guild*)iter->next(iter,NULL) )
	{
		if (strcmpi(g->name, str) == 0)
			break;
	}
	iter->destroy(iter);

	return g;
*/
}

/// parses the guild data string into a guild data structure
static bool mmo_guild_fromstr(struct guild* g, char* str)
{
	int i, c;

	memset(g, 0, sizeof(struct guild));

	{// load guild base info
		int guildid;
		char name[256]; // only 24 used
		char master[256]; // only 24 used
		int guildlv;
		int max_member;
		unsigned int exp;
		int skpoint;
		char mes1[256]; // only 60 used
		char mes2[256]; // only 120 used
		int len;

		if( sscanf(str, "%d\t%[^\t]\t%[^\t]\t%d,%d,%u,%d,%*d\t%[^\t]\t%[^\t]\t%n",
				   &guildid, name, master, &guildlv, &max_member, &exp, &skpoint, mes1, mes2, &len) < 9 )
			return false;

		// remove '#'
		mes1[strlen(mes1)-1] = '\0';
		mes2[strlen(mes2)-1] = '\0';

		g->guild_id = guildid;
		g->guild_lv = guildlv;
		g->max_member = max_member;
		g->exp = exp;
		g->skill_point = skpoint;
		safestrncpy(g->name, name, sizeof(g->name));
		safestrncpy(g->master, master, sizeof(g->master));
		safestrncpy(g->mes1, mes1, sizeof(g->mes1));
		safestrncpy(g->mes2, mes2, sizeof(g->mes2));

		str+= len;
	}

	{// load guild member info
		int accountid;
		int charid;
		int hair, hair_color, gender;
		int class_, lv;
		unsigned int exp;
		int exp_payper;
		int position;
		char name[256]; // only 24 used
		int len;
		int i;

		for( i = 0; i < g->max_member; i++ )
		{
			struct guild_member* m = &g->member[i];
			if (sscanf(str, "%d,%d,%d,%d,%d,%d,%d,%u,%d,%d\t%[^\t]\t%n",
					   &accountid, &charid, &hair, &hair_color, &gender,
					   &class_, &lv, &exp, &exp_payper, &position,
					   name, &len) < 11)
				return false;

			m->account_id = accountid;
			m->char_id = charid;
			m->hair = hair;
			m->hair_color = hair_color;
			m->gender = gender;
			m->class_ = class_;
			m->lv = lv;
			m->exp = exp;
			m->exp_payper = exp_payper;
			m->position = position;
			safestrncpy(m->name, name, NAME_LENGTH);

			str+= len;
		}
	}

	{// load guild position info
		int mode, exp_mode;
		char name[256]; // only 24 used
		int len;
		int i = 0;
		int j;

		while (sscanf(str, "%d,%d%n", &mode, &exp_mode, &j) == 2 && str[j] == '\t')
		{
			struct guild_position *p = &g->position[i];
			if (sscanf(str, "%d,%d\t%[^\t]\t%n", &mode, &exp_mode, name, &len) < 3)
				return false;

			p->mode = mode;
			p->exp_mode = exp_mode;
			name[strlen(name)-1] = 0;
			safestrncpy(p->name, name, NAME_LENGTH);

			i++;
			str+= len;
		}
	}

	{// load guild emblem
		int emblemlen;
		int emblemid;
		char emblem[4096];
		int len;
		char* pstr;

		emblemid = 0;
		if( sscanf(str, "%d,%d,%[^\t]\t%n", &emblemlen, &emblemid, emblem, &len) < 3 )
		if( sscanf(str, "%d,%[^\t]\t%n", &emblemlen, emblem, &len) < 2 ) //! pre-svn format
			return false;

		g->emblem_len = emblemlen;
		g->emblem_id = emblemid;
		for(i = 0, pstr = emblem; i < g->emblem_len; i++, pstr += 2) {
			int c1 = pstr[0], c2 = pstr[1], x1 = 0, x2 = 0;
			if (c1 >= '0' && c1 <= '9') x1 = c1 - '0';
			if (c1 >= 'a' && c1 <= 'f') x1 = c1 - 'a' + 10;
			if (c1 >= 'A' && c1 <= 'F') x1 = c1 - 'A' + 10;
			if (c2 >= '0' && c2 <= '9') x2 = c2 - '0';
			if (c2 >= 'a' && c2 <= 'f') x2 = c2 - 'a' + 10;
			if (c2 >= 'A' && c2 <= 'F') x2 = c2 - 'A' + 10;
			g->emblem_data[i] = (x1<<4) | x2;
		}

		str+= len;
	}

	{// load guild alliance info
		int guildid;
		int opposition;
		char name[256]; // only 24 used
		int len;

		if (sscanf(str, "%d\t%n", &c, &len) < 1)
			return false;
		str+= len;

		for(i = 0; i < c; i++)
		{
			struct guild_alliance* a = &g->alliance[i];
			if (sscanf(str, "%d,%d\t%[^\t]\t%n", &guildid, &opposition, name, &len) < 3)
				return false;

			a->guild_id = guildid;
			a->opposition = opposition;
			safestrncpy(a->name, name, NAME_LENGTH);

			str+= len;
		}
	}

	{// load guild expulsion info
		int accountid;
		char name[256]; // only 24 used
		char message[256]; // only 40 used
		int len;
		int i;

		if (sscanf(str, "%d\t%n", &c, &len) < 1)
			return false;
		str+= len;

		for(i = 0; i < c; i++)
		{
			struct guild_expulsion *e = &g->expulsion[i];
			if (sscanf(str, "%d,%*d,%*d,%*d\t%[^\t]\t%*[^\t]\t%[^\t]\t%n", &accountid, name, message, &len) < 3)
				return false;

			e->account_id = accountid;
			safestrncpy(e->name, name, sizeof(e->name));
			message[strlen(message)-1] = 0; // remove '#'
			safestrncpy(e->mes, message, sizeof(e->mes));

			str+= len;
		}
	}

	{// load guild skill info
		int skillid;
		int skilllv;
		int len;
		int i;

		for(i = 0; i < MAX_GUILDSKILL; i++)
		{
			if (sscanf(str, "%d,%d %n", &skillid, &skilllv, &len) < 2)
				break;
			g->skill[i].id = skillid;
			g->skill[i].lv = skilllv;

			str+= len;
		}
		str = strchr(str, '\t');
	}

	return true;
}

/// serializes the guild data structure into the provided string
static bool mmo_guild_tostr(const struct guild* g, char* str)
{
	int i, c;
	int len;

	// save guild base info
	len = sprintf(str, "%d\t%s\t%s\t%d,%d,%u,%d,%d\t%s#\t%s#\t",
	              g->guild_id, g->name, g->master, g->guild_lv, g->max_member, g->exp, g->skill_point, 0, g->mes1, g->mes2);

	// save guild member info
	for( i = 0; i < g->max_member; i++ )
	{
		const struct guild_member* m = &g->member[i];
		len += sprintf(str + len, "%d,%d,%d,%d,%d,%d,%d,%u,%d,%d\t%s\t",
		               m->account_id, m->char_id,
		               m->hair, m->hair_color, m->gender,
		               m->class_, m->lv, m->exp, m->exp_payper, m->position,
		               ((m->account_id > 0) ? m->name : "-"));
	}

	// save guild position info
	for( i = 0; i < MAX_GUILDPOSITION; i++ )
	{
		const struct guild_position* p = &g->position[i];
		len += sprintf(str + len, "%d,%d\t%s#\t", p->mode, p->exp_mode, p->name);
	}

	// save guild emblem
	len += sprintf(str + len, "%d,%d,", g->emblem_len, g->emblem_id);
	for( i = 0; i < g->emblem_len; i++ )
		len += sprintf(str + len, "%02x", (unsigned char)(g->emblem_data[i]));
	len += sprintf(str + len, "$\t");

	// save guild alliance info
	c = 0;
	for( i = 0; i < MAX_GUILDALLIANCE; i++ )
		if( g->alliance[i].guild_id > 0 )
			c++;
	len += sprintf(str + len, "%d\t", c);
	for( i = 0; i < MAX_GUILDALLIANCE; i++ )
	{
		const struct guild_alliance* a = &g->alliance[i];
		if( a->guild_id == 0 )
			continue;

		len += sprintf(str + len, "%d,%d\t%s\t", a->guild_id, a->opposition, a->name);
	}

	// save guild expulsion info
	c = 0;
	for( i = 0; i < MAX_GUILDEXPULSION; i++ )
		if( g->expulsion[i].account_id > 0 )
			c++;
	len += sprintf(str + len, "%d\t", c);
	for( i = 0; i < MAX_GUILDEXPULSION; i++ )
	{
		const struct guild_expulsion* e = &g->expulsion[i];
		if( e->account_id == 0 )
			continue;

		len += sprintf(str + len, "%d,%d,%d,%d\t%s\t%s\t%s#\t", e->account_id, 0, 0, 0, e->name, "#", e->mes );
	}

	// save guild skill info
	for( i = 0; i < MAX_GUILDSKILL; i++ )
		len += sprintf(str + len, "%d,%d ", g->skill[i].id, g->skill[i].lv);
	len += sprintf(str + len, "\t");

	return true;
}

static bool mmo_guild_sync(GuildDB_TXT* db)
{
/*
	FILE *fp;
	int lock;
	DBIterator* iter;
	struct guild* g;

	// save guild data
	fp = lock_fopen(guild_txt, &lock);
	if( fp == NULL )
	{
		ShowError("int_guild: can't write [%s] !!! data is lost !!!\n", guild_txt);
		return 1;
	}

	iter = guild_db->iterator(guild_db);
	for( g = (struct guild*)iter->first(iter,NULL); iter->exists(iter); g = (struct guild*)iter->next(iter,NULL) )
	{
		char line[16384];
		inter_guild_tostr(line, g);
		fprintf(fp, "%s\n", line);
	}
	iter->destroy(iter);

//	fprintf(fp, "%d\t%%newid%%\n", guild_newid);
	lock_fclose(fp, guild_txt, &lock);
*/

	return true;
}
