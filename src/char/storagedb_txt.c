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
#include "charserverdb_txt.h"
#include "storagedb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/// internal structure
typedef struct StorageDB_TXT
{
	// public interface
	StorageDB vtable;

	// state
	CharServerDB_TXT* owner;
	DBMap* inventories;
	DBMap* carts;
	DBMap* storages;
	DBMap* guildstorages;
	bool dirty;

	// settings
	const char* storage_db;
	const char* guildstorage_db;

} StorageDB_TXT;


static DBMap* type2db(StorageDB_TXT* db, enum storage_type type)
{
	switch( type )
	{
	case STORAGE_INVENTORY: return db->inventories;
	case STORAGE_CART     : return db->carts;
	case STORAGE_KAFRA    : return db->storages;
	case STORAGE_GUILD    : return db->guildstorages;
	default:
		return NULL;
	}
}


static const char* type2file(StorageDB_TXT* db, enum storage_type type)
{
	switch( type )
	{
	case STORAGE_KAFRA    : return db->storage_db;
	case STORAGE_GUILD    : return db->guildstorage_db;
	default:
		return NULL;
	}
}


bool mmo_storage_fromstr(struct item* s, size_t size, const char* str)
{
	int count;
	int fields[1+MAX_STORAGE][2]; //FIXME: this should be 'size' dependent
	int tmp_int[7+MAX_SLOTS+1];
	int len,i,j;

	memset(s, 0, size * sizeof(*s)); //clean up memory

	// extract space-separated item blocks from str
	count = sv_parse(str, strlen(str), 0, ' ', (int*)fields, 2*ARRAYLENGTH(fields), (e_svopt)(SV_TERMINATE_LF|SV_TERMINATE_CRLF)) - 1;

	// parse individual item blocks
	for( i = 0; i < count; ++i )
	{
		const char* p = &str[fields[i+1][0]];

		if( sscanf(p, "%d,%d,%d,%d,%d,%d,%d%n",
		    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6],
			&len) != 7 )
			return false;

		p += len;

		j = 0;
		while( *p != ' ' && *p != '\0' )
		{
			if( sscanf(p, ",%d%n", &tmp_int[7+j], &len) != 1 )
				return false;

			p += len;

			if( j == MAX_SLOTS )
				continue; // discard card slots over max

			j++;
		}

		if( i == size )
			continue; // discard items over max

		s[i].id = tmp_int[0];
		s[i].nameid = tmp_int[1];
		s[i].amount = tmp_int[2];
		s[i].equip = tmp_int[3];
		s[i].identify = tmp_int[4];
		s[i].refine = tmp_int[5];
		s[i].attribute = tmp_int[6];
		for( j = 0; j < MAX_SLOTS; ++j )
			s[i].card[j] = tmp_int[7+j];
	}

	return true;
}


bool mmo_storage_tostr(const struct item* s, size_t size, char* str)
{
	size_t i,j;
	char* p = str;

	for( i = 0; i < size; i++ )
	{
		if( s[i].nameid == 0 || s[i].amount == 0 )
			continue;

		p += sprintf(p, "%d,%d,%d,%d,%d,%d,%d",
			s[i].id, s[i].nameid, s[i].amount, s[i].equip,
			s[i].identify, s[i].refine, s[i].attribute);

		for( j = 0; j < MAX_SLOTS; j++ )
			p += sprintf(p, ",%d", s[i].card[j]);

		p += sprintf(p, " ");
	}

	*p = '\0';

	return true;
}


/// Loads data from the data file associated with the storage type.
/// Assumes that the file format is:
/// <id>,<count> \tab {<id>,<nameid>,<amount>,<equip>,<identify>,<refine>,<attribute> \space}*
/// @see mmo_storage_tostr
/// @param size Maximum number of entries to load
static bool mmo_storagedb_load(StorageDB_TXT* db, enum storage_type type, size_t size)
{
	DBMap* storages = type2db(db, type);
	const char* file = type2file(db, type);
	char line[65536];
	FILE* fp;

	// open data file
	fp = fopen(file, "r");
	if( fp == NULL )
	{
		ShowError("mmo_storagedb_load: Cannot open file %s!\n", file);
		return false;
	}

	while( fgets(line, sizeof(line), fp) )
	{
		int id;
		int n;

		struct item* s = (struct item*)aCalloc(size, sizeof(struct item));
		if( s == NULL )
		{
			ShowFatalError("mmo_storagedb_load: out of memory!\n");
			exit(EXIT_FAILURE);
		}

		// load id
		if( sscanf(line, "%d,%*d\t%n", &id, &n) != 1 )
		{
			aFree(s);
			continue;
		}

		// load storage for this id
		if( !mmo_storage_fromstr(s, size, line + n) )
		{
			ShowError("mmo_storagedb_load: File %s, broken line data: %s\n", file, line);
			aFree(s);
			continue;
		}

		db->vtable.save(&db->vtable, s, size, type, id);
		aFree(s);
	}

	fclose(fp);

	return true;
}


static bool mmo_storagedb_sync(StorageDB_TXT* db, enum storage_type type)
{
	DBMap* storages = type2db(db, type);
	const char* file = type2file(db, type);
	DBIterator* iter;
	DBKey key;
	void* data;
	FILE* fp;
	int lock;

	fp = lock_fopen(file, &lock);
	if( fp == NULL )
	{
		ShowError("mmo_storagedb_sync: can't write [%s] !!! data is lost !!!\n", file);
		return false;
	}

	iter = db_iterator(storages);
	for( data = iter->first(iter,&key); iter->exists(iter); data = iter->next(iter,&key) )
	{
		int id = key.i;
		struct item* s = (struct item*) data;
		char line[65536];
		size_t size;

		ARR_FIND(0, SIZE_MAX, size, s[size].amount == 0); // determine size
		mmo_storage_tostr(s, size, line);
		fprintf(fp, "%d,%d\t%s\n", id, 0, line);
	}
	iter->destroy(iter);

	lock_fclose(fp, file, &lock);

	db->dirty = false;
	return true;
}


static bool storage_db_txt_init(StorageDB* self)
{
	StorageDB_TXT* db = (StorageDB_TXT*)self;

	// create storage databases
	if( db->inventories == NULL )
		db->inventories = idb_alloc(DB_OPT_RELEASE_DATA);
	if( db->carts == NULL )
		db->carts = idb_alloc(DB_OPT_RELEASE_DATA);
	if( db->storages == NULL )
		db->storages = idb_alloc(DB_OPT_RELEASE_DATA);
	if( db->guildstorages == NULL )
		db->guildstorages = idb_alloc(DB_OPT_RELEASE_DATA);
	db_clear(db->inventories);
	db_clear(db->carts);
	db_clear(db->storages);
	db_clear(db->guildstorages);

	if( !mmo_storagedb_load(db, STORAGE_KAFRA, MAX_STORAGE)
	||  !mmo_storagedb_load(db, STORAGE_GUILD, MAX_GUILD_STORAGE)
	)
		return false;

	return true;
}


static void storage_db_txt_destroy(StorageDB* self)
{
	StorageDB_TXT* db = (StorageDB_TXT*)self;

	// delete storage databases
	if( db->inventories != NULL )
	{
		db_destroy(db->inventories);
		db->inventories = NULL;
	}
	if( db->carts != NULL )
	{
		db_destroy(db->carts);
		db->carts = NULL;
	}
	if( db->storages != NULL )
	{
		db_destroy(db->storages);
		db->storages = NULL;
	}
	if( db->guildstorages != NULL )
	{
		db_destroy(db->guildstorages);
		db->guildstorages = NULL;
	}

	// delete entire structure
	aFree(db);
}


static bool storage_db_txt_sync(StorageDB* self)
{
	StorageDB_TXT* db = (StorageDB_TXT*)self;
	bool result = true;

	if( mmo_storagedb_sync(db, STORAGE_KAFRA) == false )
		result = false;
	if( mmo_storagedb_sync(db, STORAGE_GUILD) == false )
		result = false;

	return result;
}


static bool storage_db_txt_remove(StorageDB* self, enum storage_type type, const int id)
{
	StorageDB_TXT* db = (StorageDB_TXT*)self;
	DBMap* storages = type2db(db,type);

	idb_remove(storages, id);

	db->dirty = true;
	db->owner->p.request_sync(db->owner);
	return true;
}


/// Writes provided data into storage cache.
/// If data contains 0 items, any existing entry in cache is destroyed instead.
static bool storage_db_txt_save(StorageDB* self, const struct item* s, size_t size, enum storage_type type, int id)
{
	StorageDB_TXT* db = (StorageDB_TXT*)self;
	DBMap* storages = type2db(db,type);
	struct item* tmp;
	size_t i, k;

	if( s == NULL )
		size = 0;

	// remove old storage
	idb_remove(storages, id);

	// fill storage (compact array + append zero terminator)
	tmp = (struct item*)aMalloc((size+1)*sizeof(*s));
	k = 0;
	for( i = 0; i < size; ++i )
	{
		if( s[i].amount <= 0 )
			continue;
		memcpy(&tmp[k], &s[i], sizeof(*s));
		++k;
	}

	if( k > 0 )
	{// has items
		if( k != size )
			tmp = (struct item*)aRealloc(tmp, (k+1)*sizeof(*s));
		memset(tmp+k, 0x00, sizeof(*s));
		idb_put(storages, id, tmp);
	}
	else
	{// no data to store
		aFree(tmp);
	}

	db->dirty = true;
	db->owner->p.request_sync(db->owner);
	return true;
}


/// Loads storage data into the provided data structure.
/// If data doesn't exist, the destination is zeroed instead.
static bool storage_db_txt_load(StorageDB* self, struct item* s, size_t size, enum storage_type type, int id)
{
	StorageDB_TXT* db = (StorageDB_TXT*)self;
	DBMap* storages = type2db(db,type);
	struct item* tmp;
	size_t i = 0;
	
	tmp = (struct item*)idb_get(storages, id);

	if( tmp != NULL )
		ARR_FIND(0, size, i, tmp[i].amount == 0);
	if( i > 0 )
		memcpy(s, tmp, i*sizeof(*s));
	if( i != size )
		memset(s+i, 0x00, (size-i)*sizeof(*s));

	return true;
}


/// Returns an iterator over all storages.
static CSDBIterator* storage_db_txt_iterator(StorageDB* self, enum storage_type type)
{
	StorageDB_TXT* db = (StorageDB_TXT*)self;
	return csdb_txt_iterator(db_iterator(type2db(db,type)));
}


/// public constructor
StorageDB* storage_db_txt(CharServerDB_TXT* owner)
{
	StorageDB_TXT* db = (StorageDB_TXT*)aCalloc(1, sizeof(StorageDB_TXT));

	// set up the vtable
	db->vtable.init      = &storage_db_txt_init;
	db->vtable.destroy   = &storage_db_txt_destroy;
	db->vtable.sync      = &storage_db_txt_sync;
	db->vtable.remove    = &storage_db_txt_remove;
	db->vtable.save      = &storage_db_txt_save;
	db->vtable.load      = &storage_db_txt_load;
	db->vtable.iterator  = &storage_db_txt_iterator;

	// initialize to default values
	db->owner = owner;
	db->inventories = NULL;
	db->carts = NULL;
	db->storages = NULL;
	db->guildstorages = NULL;
	db->dirty = false;

	// other settings
	db->storage_db = db->owner->file_storages;
	db->guildstorage_db = db->owner->file_guild_storages;

	return &db->vtable;
}
