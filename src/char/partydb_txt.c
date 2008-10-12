// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/db.h"
#include "../common/lock.h"
#include "../common/malloc.h"
#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/strlib.h"
#include "chardb.h"
#include "int_party.h"
#include "partydb.h"
#include <stdio.h>
#include <string.h>

#define START_PARTY_NUM 1

// temp
extern CharDB* chars;


/// internal structure
typedef struct PartyDB_TXT
{
	PartyDB vtable;      // public interface

	DBMap* parties;      // in-memory character storage
	int next_party_id;   // auto_increment

	char party_db[1024]; // character data storage file

} PartyDB_TXT;

/// internal functions
static bool party_db_txt_init(PartyDB* self);
static void party_db_txt_destroy(PartyDB* self);
static bool party_db_txt_sync(PartyDB* self);
static bool party_db_txt_create(PartyDB* self, struct party_data* p);
static bool party_db_txt_remove(PartyDB* self, const int party_id);
static bool party_db_txt_save(PartyDB* self, const struct party_data* p);
static bool party_db_txt_load_num(PartyDB* self, struct party_data* p, int party_id);
static bool party_db_txt_load_str(PartyDB* self, struct party_data* p, const char* name);

static bool mmo_party_fromstr(PartyDB_TXT* db, struct party* p, char* str);
static bool mmo_party_tostr(PartyDB_TXT* db, const struct party* p, char* str);
static bool mmo_party_sync(PartyDB_TXT* db);

/// public constructor
PartyDB* party_db_txt(void)
{
	PartyDB_TXT* db = (PartyDB_TXT*)aCalloc(1, sizeof(PartyDB_TXT));

	// set up the vtable
	db->vtable.init      = &party_db_txt_init;
	db->vtable.destroy   = &party_db_txt_destroy;
	db->vtable.sync      = &party_db_txt_sync;
	db->vtable.create    = &party_db_txt_create;
	db->vtable.remove    = &party_db_txt_remove;
	db->vtable.save      = &party_db_txt_save;
	db->vtable.load_num  = &party_db_txt_load_num;
	db->vtable.load_str  = &party_db_txt_load_str;

	// initialize to default values
	db->parties = NULL;
	db->next_party_id = START_PARTY_NUM;
	// other settings
	safestrncpy(db->party_db, "save/party.txt", sizeof(db->party_db));

	return &db->vtable;
}


/* ------------------------------------------------------------------------- */


static bool party_db_txt_init(PartyDB* self)
{
	PartyDB_TXT* db = (PartyDB_TXT*)self;
	DBMap* parties;

	char line[8192];
	FILE *fp;

	// create party database
	db->parties = idb_alloc(DB_OPT_RELEASE_DATA);
	parties = db->parties;

	// open data file
	fp = fopen(db->party_db, "r");
	if( fp == NULL )
	{
		ShowError("Party file not found: %s.\n", db->party_db);
		return false;
	}

	// load data file
	while( fgets(line, sizeof(line), fp) != NULL )
	{
		int party_id, n;
		struct party_data p;
		struct party_data* tmp;

		n = 0;
		if( sscanf(line, "%d\t%%newid%%%n", &party_id, &n) == 1 && n > 0 && (line[n] == '\n' || line[n] == '\r') )
		{// auto-increment
			if( party_id > db->next_party_id )
				db->next_party_id = party_id;
			continue;
		}

		if( !mmo_party_fromstr(db, &p.party, line) )
		{
			ShowError("party_db_txt_init: skipping invalid data: %s", line);
			continue;
		}

		//init state
		int_party_calc_state(&p);

		// record entry in db
		tmp = (struct party_data*)aMalloc(sizeof(struct party_data));
		memcpy(tmp, &p, sizeof(struct party_data));
		idb_put(parties, p.party.party_id, tmp);

		if( db->next_party_id < p.party.party_id)
			db->next_party_id = p.party.party_id + 1;
	}

	// close data file
	fclose(fp);

	return true;
}

static void party_db_txt_destroy(PartyDB* self)
{
	PartyDB_TXT* db = (PartyDB_TXT*)self;
	DBMap* parties = db->parties;

	// write data
	mmo_party_sync(db);

	// delete party database
	parties->destroy(parties, NULL);
	db->parties = NULL;

	// delete entire structure
	aFree(db);
}

static bool party_db_txt_sync(PartyDB* self)
{
	PartyDB_TXT* db = (PartyDB_TXT*)self;
	DBMap* parties = db->parties;

	return mmo_party_sync(db);
}

static bool party_db_txt_create(PartyDB* self, struct party_data* p)
{
	PartyDB_TXT* db = (PartyDB_TXT*)self;
	DBMap* parties = db->parties;

	//TODO
//	p->party.party_id = party_newid++;
//	idb_put(party_db, p->party.party_id, p);

	// flush data
	mmo_party_sync(db);

	return true;
}

static bool party_db_txt_remove(PartyDB* self, const int party_id)
{
	PartyDB_TXT* db = (PartyDB_TXT*)self;
	DBMap* parties = db->parties;

	struct party_data* tmp = idb_remove(parties, party_id);
	if( tmp == NULL )
	{// error condition - entry not present
		ShowError("party_db_txt_remove: no such party with id %d\n", party_id);
		return false;
	}

	return true;
}

static bool party_db_txt_save(PartyDB* self, const struct party_data* p)
{
	PartyDB_TXT* db = (PartyDB_TXT*)self;
	DBMap* parties = db->parties;
	int party_id = p->party.party_id;

	// retrieve previous data
	struct party_data* tmp = idb_get(parties, party_id);
	if( tmp == NULL )
	{// error condition - entry not found
		return false;
	}
	
	// overwrite with new data
	memcpy(tmp, p, sizeof(struct party_data));

	return true;
}

static bool party_db_txt_load_num(PartyDB* self, struct party_data* p, int party_id)
{
	PartyDB_TXT* db = (PartyDB_TXT*)self;
	DBMap* parties = db->parties;
	struct mmo_charstatus cd;
	int i;

	// retrieve data
	struct party_data* tmp = idb_get(parties, party_id);
	if( tmp == NULL )
	{// entry not found
		return false;
	}

	// store it
	memcpy(p, tmp, sizeof(struct party_data));

	//Lookup players for rest of data.
	for( i = 0; i < MAX_PARTY; i++ )
	{
		struct party_member* m = &p->party.member[i];

		if( !chars->load_num(chars, &cd, m->char_id) || cd.account_id != m->account_id )
			continue;

		safestrncpy(m->name, cd.name, sizeof(m->name));
		m->class_ = cd.class_;
		m->map = cd.last_point.map;
		m->lv = cd.base_level;
	}

	return true;
}

static bool party_db_txt_load_str(PartyDB* self, struct party_data* p, const char* name)
{
	PartyDB_TXT* db = (PartyDB_TXT*)self;
	DBMap* parties = db->parties;

	// retrieve data
	struct DBIterator* iter = parties->iterator(parties);
	struct party_data* tmp;

	for( tmp = (struct party_data*)iter->first(iter,NULL); iter->exists(iter); tmp = (struct party_data*)iter->next(iter,NULL) )
		if( strcmp(name, tmp->party.name) == 0 )
			break;
	iter->destroy(iter);

	if( tmp == NULL )
	{// entry not found
		return false;
	}

	// store it
	memcpy(p, tmp, sizeof(struct party_data));

	return true;
}


static bool mmo_party_fromstr(PartyDB_TXT* db, struct party* p, char* str)
{
	int i, j;
	int party_id;
	char name[256];
	int exp;
	int item;
	
	memset(p, 0, sizeof(struct party));

	if( sscanf(str, "%d\t%255[^\t]\t%d,%d\t", &party_id, name, &exp, &item) != 4 )
		return false;

	p->party_id = party_id;
	safestrncpy(p->name, name, sizeof(p->name));
	p->exp = exp ? 1:0;
	p->item = item;

	for( j = 0; j < 3 && str != NULL; j++ )
		str = strchr(str + 1, '\t');

	for( i = 0; i < MAX_PARTY; i++ )
	{
		struct party_member* m = &p->member[i];
		int account_id;
		int char_id;
		int leader;

		if( str == NULL )
			return false;

		if( sscanf(str + 1, "%d,%d,%d\t", &account_id, &char_id, &leader) != 3 )
			return false;

		m->account_id = account_id;
		m->char_id = char_id; 
		m->leader = leader ? 1:0;

		str = strchr(str + 1, '\t');
	}

	return true;
}

static bool mmo_party_tostr(PartyDB_TXT* db, const struct party* p, char* str)
{
	int i, len;

	// write basic data
	len = sprintf(str, "%d\t%s\t%d,%d\t", p->party_id, p->name, p->exp, p->item);

	// write party member data
	for( i = 0; i < MAX_PARTY; i++ )
	{
		const struct party_member* m = &p->member[i];
		len += sprintf(str + len, "%d,%d,%d\t", m->account_id, m->char_id, m->leader);
	}

	return true;
}

static bool mmo_party_sync(PartyDB_TXT* db)
{
	FILE *fp;
	int lock;
	struct DBIterator* iter;
	struct party_data* p;

	fp = lock_fopen(db->party_db, &lock);
	if( fp == NULL )
	{
		ShowError("mmo_party_sync: can't write [%s] !!! data is lost !!!\n", db->party_db);
		return false;
	}

	iter = db->parties->iterator(db->parties);
	for( p = (struct party_data*)iter->first(iter,NULL); iter->exists(iter); p = (struct party_data*)iter->next(iter,NULL) )
	{
		char buf[8192]; // ought to be big enough ^^
		mmo_party_tostr(db, &p->party, buf);
		fprintf(fp, "%s\n", buf);
	}
	iter->destroy(iter);

	lock_fclose(fp, db->party_db, &lock);

	return true;
}
