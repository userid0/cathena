// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/db.h"
#include "../common/lock.h"
#include "../common/malloc.h"
#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/strlib.h"
#include "../common/utils.h"
#include "petdb.h"
#include <stdio.h>
#include <string.h>

#define START_PET_NUM 1


/// internal structure
typedef struct PetDB_TXT
{
	PetDB vtable;       // public interface

	DBMap* pets;         // in-memory pet storage
	int next_pet_id;     // auto_increment

	char pet_db[1024];   // pet data storage file
	bool case_sensitive; // how to look up names

} PetDB_TXT;

/// internal functions
static bool pet_db_txt_init(PetDB* self);
static void pet_db_txt_destroy(PetDB* self);
static bool pet_db_txt_sync(PetDB* self);
static bool pet_db_txt_create(PetDB* self, struct s_pet* pd);
static bool pet_db_txt_remove(PetDB* self, const int pet_id);
static bool pet_db_txt_save(PetDB* self, const struct s_pet* pd);
static bool pet_db_txt_load_num(PetDB* self, struct s_pet* pd, int pet_id);
static bool pet_db_txt_load_str(PetDB* self, struct s_pet* pd, const char* name);

static bool mmo_pet_fromstr(struct s_pet* pd, char* str);
static bool mmo_pet_tostr(const struct s_pet* pd, char* str);
static bool mmo_pet_sync(PetDB_TXT* db);

/// public constructor
PetDB* pet_db_txt(void)
{
	PetDB_TXT* db = (PetDB_TXT*)aCalloc(1, sizeof(PetDB_TXT));

	// set up the vtable
	db->vtable.init      = &pet_db_txt_init;
	db->vtable.destroy   = &pet_db_txt_destroy;
	db->vtable.sync      = &pet_db_txt_sync;
	db->vtable.create    = &pet_db_txt_create;
	db->vtable.remove    = &pet_db_txt_remove;
	db->vtable.save      = &pet_db_txt_save;
	db->vtable.load_num  = &pet_db_txt_load_num;
	db->vtable.load_str  = &pet_db_txt_load_str;

	// initialize to default values
	db->pets = NULL;
	db->next_pet_id = START_PET_NUM;
	// other settings
	safestrncpy(db->pet_db, "save/pet.txt", sizeof(db->pet_db));

	return &db->vtable;
}


/* ------------------------------------------------------------------------- */


static bool pet_db_txt_init(PetDB* self)
{
	PetDB_TXT* db = (PetDB_TXT*)self;
	DBMap* pets;

	char line[8192];
	FILE *fp;

	// create pet database
	db->pets = idb_alloc(DB_OPT_RELEASE_DATA);
	pets = db->pets;

	// open data file
	fp = fopen(db->pet_db, "r");
	if( fp == NULL )
	{
		ShowError("Pet file not found: %s.\n", db->pet_db);
		return false;
	}

	// load data file
	while( fgets(line, sizeof(line), fp) )
	{
		int pet_id, n;
		struct s_pet p;
		struct s_pet* tmp;

		n = 0;
		if( sscanf(line, "%d\t%%newid%%%n", &pet_id, &n) == 1 && n > 0 && (line[n] == '\n' || line[n] == '\r') )
		{// auto-increment
			if( pet_id > db->next_pet_id )
				db->next_pet_id = pet_id;
			continue;
		}

		if( !mmo_pet_fromstr(&p, line) )
		{
			ShowError("pet_db_txt_init: skipping invalid data: %s", line);
			continue;
		}
	
		// record entry in db
		tmp = (struct s_pet*)aMalloc(sizeof(struct s_pet));
		memcpy(tmp, &p, sizeof(struct s_pet));
		idb_put(pets, p.pet_id, tmp);

		if( p.pet_id >= db->next_pet_id )
			db->next_pet_id = p.pet_id + 1;
	}

	// close data file
	fclose(fp);

	return true;
}

static void pet_db_txt_destroy(PetDB* self)
{
	PetDB_TXT* db = (PetDB_TXT*)self;

}

static bool pet_db_txt_sync(PetDB* self)
{
	PetDB_TXT* db = (PetDB_TXT*)self;

}

static bool pet_db_txt_create(PetDB* self, struct s_pet* pd)
{
	PetDB_TXT* db = (PetDB_TXT*)self;

}

static bool pet_db_txt_remove(PetDB* self, const int pet_id)
{
	PetDB_TXT* db = (PetDB_TXT*)self;
/*
	struct s_pet *p;
	p = (struct s_pet*)idb_get(pet_db,pet_id);
	if( p == NULL)
		return 1;
	else {
		idb_remove(pet_db,pet_id);
		ShowInfo("Deleted pet (pet_id: %d)\n",pet_id);
	}
	return 0;
*/
}

static bool pet_db_txt_save(PetDB* self, const struct s_pet* pd)
{
	PetDB_TXT* db = (PetDB_TXT*)self;

}

static bool pet_db_txt_load_num(PetDB* self, struct s_pet* pd, int pet_id)
{
	PetDB_TXT* db = (PetDB_TXT*)self;

}

static bool pet_db_txt_load_str(PetDB* self, struct s_pet* pd, const char* name)
{
	PetDB_TXT* db = (PetDB_TXT*)self;

}

static bool mmo_pet_fromstr(struct s_pet* p, char* str)
{
	int tmp_int[16];
	char tmp_str[256];

	memset(p, 0, sizeof(p));

	if( sscanf(str, "%d,%d,%[^\t]\t%d,%d,%d,%d,%d,%d,%d,%d,%d",
		&tmp_int[0], &tmp_int[1], tmp_str, &tmp_int[2],
		&tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6],
		&tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10]) != 12 )
		return false;

	p->pet_id = tmp_int[0];
	p->class_ = tmp_int[1];
	memcpy(p->name,tmp_str,NAME_LENGTH);
	p->account_id = tmp_int[2];
	p->char_id = tmp_int[3];
	p->level = tmp_int[4];
	p->egg_id = tmp_int[5];
	p->equip = tmp_int[6];
	p->intimate = tmp_int[7];
	p->hungry = tmp_int[8];
	p->rename_flag = tmp_int[9];
	p->incuvate = tmp_int[10];

	p->hungry = cap_value(p->hungry, 0, 100);
	p->intimate = cap_value(p->intimate, 0, 1000);

	return true;
}

static bool mmo_pet_tostr(const struct s_pet* p, char* str)
{
	sprintf(str, "%d,%d,%s\t%d,%d,%d,%d,%d,%d,%d,%d,%d",
		p->pet_id, p->class_, p->name, p->account_id, p->char_id, p->level, p->egg_id,
		p->equip, p->intimate, p->hungry, p->rename_flag, p->incuvate);

	return true;
}

static bool mmo_pet_sync(PetDB_TXT* db)
{
	DBIterator* iter;
	void* data;
	FILE *fp;
	int lock;

	fp = lock_fopen(db->pet_db, &lock);
	if( fp == NULL )
	{
		ShowError("mmo_pet_sync: can't write [%s] !!! data is lost !!!\n", db->pet_db);
		return false;
	}

	iter = db->pets->iterator(db->pets);
	for( data = iter->first(iter,NULL); iter->exists(iter); data = iter->next(iter,NULL) )
	{
		struct s_pet* pd = (struct s_pet*) data;
		char line[8192];

		mmo_pet_tostr(pd, line);
		fprintf(fp, "%s\n", line);
	}
	iter->destroy(iter);

	lock_fclose(fp, db->pet_db, &lock);

	return true;
}
