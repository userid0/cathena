// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/db.h"
#include "../common/lock.h"
#include "../common/malloc.h"
#include "../common/mapindex.h"
#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/socket.h"
#include "../common/strlib.h"
#include "../common/timer.h"
#include "char.h"
#include "inter.h"
#include "regdb.h"
#include "charserverdb_txt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// temporary stuff
extern CharServerDB* charserver;
extern int autosave_interval;
extern bool mmo_charreg_tostr(const struct regs* reg, char* str);
extern bool mmo_charreg_fromstr(struct regs* reg, const char* str);


/// internal structure
typedef struct CharDB_TXT
{
	CharDB vtable;       // public interface

	CharServerDB_TXT* owner;
	DBMap* chars;        // in-memory character storage
	int next_char_id;    // auto_increment
	int save_timer;      // save timer id

	char char_db[1024];  // character data storage file
	bool case_sensitive; // how to look up usernames

} CharDB_TXT;

/// internal structure
typedef struct CharDBIterator_TXT
{
	CharDBIterator vtable;      // public interface

	DBIterator* iter;
	int account_id;
	bool has_account_id;
} CharDBIterator_TXT;

/// internal functions
static bool char_db_txt_init(CharDB* self);
static void char_db_txt_destroy(CharDB* self);
static bool char_db_txt_create(CharDB* self, struct mmo_charstatus* status);
static bool char_db_txt_remove(CharDB* self, const int char_id);
static bool char_db_txt_save(CharDB* self, const struct mmo_charstatus* status);
static bool char_db_txt_load_num(CharDB* self, struct mmo_charstatus* ch, int char_id);
static bool char_db_txt_load_str(CharDB* self, struct mmo_charstatus* ch, const char* name);
static bool char_db_txt_load_slot(CharDB* self, struct mmo_charstatus* status, int account_id, int slot);
static bool char_db_txt_id2name(CharDB* self, int char_id, char name[NAME_LENGTH]);
static bool char_db_txt_name2id(CharDB* self, const char* name, int* char_id, int* account_id);
static bool char_db_txt_slot2id(CharDB* self, int account_id, int slot, int* char_id);
static CharDBIterator* char_db_txt_iterator(CharDB* self);
static CharDBIterator* char_db_txt_characters(CharDB* self, int account_id);
static void char_db_txt_iter_destroy(CharDBIterator* self);
static bool char_db_txt_iter_next(CharDBIterator* self, struct mmo_charstatus* ch);

int mmo_char_fromstr(CharDB* chars, const char *str, struct mmo_charstatus *p, struct regs* reg);
static int mmo_char_sync_timer(int tid, unsigned int tick, int id, intptr data);
static void mmo_char_sync(CharDB_TXT* db);

/// public constructor
CharDB* char_db_txt(CharServerDB_TXT* owner)
{
	CharDB_TXT* db = (CharDB_TXT*)aCalloc(1, sizeof(CharDB_TXT));

	// set up the vtable
	db->vtable.init      = &char_db_txt_init;
	db->vtable.destroy   = &char_db_txt_destroy;
	db->vtable.create    = &char_db_txt_create;
	db->vtable.remove    = &char_db_txt_remove;
	db->vtable.save      = &char_db_txt_save;
	db->vtable.load_num  = &char_db_txt_load_num;
	db->vtable.load_str  = &char_db_txt_load_str;
	db->vtable.load_slot = &char_db_txt_load_slot;
	db->vtable.id2name   = &char_db_txt_id2name;
	db->vtable.name2id   = &char_db_txt_name2id;
	db->vtable.slot2id   = &char_db_txt_slot2id;
	db->vtable.iterator  = &char_db_txt_iterator;
	db->vtable.characters = &char_db_txt_characters;

	// initialize to default values
	db->owner = owner;
	db->chars = NULL;
	db->next_char_id = START_CHAR_NUM;
	db->save_timer = INVALID_TIMER;
	// other settings
	safestrncpy(db->char_db, "save/athena.txt", sizeof(db->char_db));
	db->case_sensitive = false;

	return &db->vtable;
}


/* ------------------------------------------------------------------------- */


static bool char_db_txt_init(CharDB* self)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	FriendDB* friends = db->owner->frienddb;
	HotkeyDB* hotkeys = db->owner->hotkeydb;
	CharRegDB* charregs = db->owner->charregdb;
	DBMap* chars;

	char line[65536];
	int line_count = 0;
	int ret;
	FILE* fp;

	// create chars database
	db->chars = idb_alloc(DB_OPT_RELEASE_DATA);
	chars = db->chars;

	// open data file
	fp = fopen(db->char_db, "r");
	if( fp == NULL )
	{
		ShowError("Characters file not found: %s.\n", db->char_db);
		log_char("Characters file not found: %s.\n", db->char_db);
		log_char("Id for the next created character: %d.\n", db->next_char_id);
		return false;
	}

	// load data file
	while( fgets(line, sizeof(line), fp) != NULL )
	{
		int char_id, n;
		struct mmo_charstatus* ch;
		struct regs reg;
		line_count++;

		if( line[0] == '/' && line[1] == '/' )
			continue;

		n = 0;
		if( sscanf(line, "%d\t%%newid%%%n", &char_id, &n) == 1 && n > 0 && (line[n] == '\n' || line[n] == '\r') )
		{// auto-increment
			if( char_id > db->next_char_id )
				db->next_char_id = char_id;
			continue;
		}

		// allocate memory for the char entry
		ch = (struct mmo_charstatus*)aMalloc(sizeof(struct mmo_charstatus));

		// parse char data
		ret = mmo_char_fromstr(self, line, ch, &reg);

		charregs->save(charregs, &reg, ch->char_id); // Initialize char regs
		//friends->load(friends, &ch->friends, ch->char_id); // Initialize friends list
		//hotkeys->load(hotkeys, &ch->hotkeys, ch->char_id); // Initialize hotkey list

		if( ret <= 0 )
		{
			ShowError("mmo_char_init: in characters file, unable to read the line #%d.\n", line_count);
			ShowError("               -> Character saved in log file.\n");
			switch( ret )
			{
			case  0: log_char("Unable to get a character in the next line - Basic structure of line (before inventory) is incorrect (character not readed):\n"); break;
			case -1: log_char("Duplicate character id in the next character line (character not readed):\n"); break;
			case -2: log_char("Duplicate character name in the next character line (character not readed):\n"); break;
			case -3: log_char("Invalid memo point structure in the next character line (character not readed):\n"); break;
			case -4: log_char("Invalid inventory item structure in the next character line (character not readed):\n"); break;
			case -5: log_char("Invalid cart item structure in the next character line (character not readed):\n"); break;
			case -6: log_char("Invalid skill structure in the next character line (character not readed):\n"); break;
			case -7: log_char("Invalid register structure in the next character line (character not readed):\n"); break;
			default: break;
			}
			log_char("%s", line);

			aFree(ch);
			continue;
		}

		// record entry in db
		idb_put(chars, ch->char_id, ch);

		if( ch->char_id >= db->next_char_id )
			db->next_char_id = ch->char_id + 1;
	}

	// close data file
	fclose(fp);

	ShowStatus("mmo_char_init: %d characters read in %s.\n", chars->size(chars), db->char_db);
	log_char("mmo_char_init: %d characters read in %s.\n", chars->size(chars), db->char_db);
	log_char("Id for the next created character: %d.\n", db->next_char_id);

	// initialize data saving timer
	add_timer_func_list(mmo_char_sync_timer, "mmo_char_sync_timer");
	db->save_timer = add_timer_interval(gettick() + 1000, mmo_char_sync_timer, 0, (intptr)db, autosave_interval);

	return true;
}

static void char_db_txt_destroy(CharDB* self)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;

	// stop saving timer
	delete_timer(db->save_timer, mmo_char_sync_timer);

	// write data
	mmo_char_sync(db);

	// delete chars database
	chars->destroy(chars, NULL);
	db->chars = NULL;

	// delete entire structure
	aFree(db);
}

static bool char_db_txt_create(CharDB* self, struct mmo_charstatus* cd)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;
	struct mmo_charstatus* tmp;

	// decide on the char id to assign
	int char_id = ( cd->char_id != -1 ) ? cd->char_id : db->next_char_id;

	// check if the char_id is free
	tmp = idb_get(chars, char_id);
	if( tmp != NULL )
	{// error condition - entry already present
		ShowError("char_db_txt_create: cannot create character %d:'%s', this id is already occupied by %d:'%s'!\n", char_id, cd->name, char_id, tmp->name);
		return false;
	}

	// copy the data and store it in the db
	tmp = (struct mmo_charstatus*)aMalloc(sizeof(struct mmo_charstatus));
	memcpy(tmp, cd, sizeof(struct mmo_charstatus));
	tmp->char_id = char_id;
	idb_put(chars, char_id, tmp);

	// increment the auto_increment value
	if( char_id >= db->next_char_id )
		db->next_char_id = char_id + 1;

	// flush data
	mmo_char_sync(db);

	// write output
	cd->char_id = char_id;

	return true;
}

static bool char_db_txt_remove(CharDB* self, const int char_id)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;

	struct mmo_charstatus* tmp = (struct mmo_charstatus*)idb_remove(chars, char_id);
	if( tmp == NULL )
	{// error condition - entry not present
		ShowError("char_db_txt_remove: no such character with id %d\n", char_id);
		return false;
	}

	return true;
}

static bool char_db_txt_save(CharDB* self, const struct mmo_charstatus* ch)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;
	int char_id = ch->char_id;

	// retrieve previous data
	struct mmo_charstatus* tmp = idb_get(chars, char_id);
	if( tmp == NULL )
	{// error condition - entry not found
		return false;
	}
	
	// overwrite with new data
	memcpy(tmp, ch, sizeof(struct mmo_charstatus));

	return true;
}

static bool char_db_txt_load_num(CharDB* self, struct mmo_charstatus* ch, int char_id)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;

	// retrieve data
	struct mmo_charstatus* tmp = idb_get(chars, char_id);
	if( tmp == NULL )
	{// entry not found
		return false;
	}

	// store it
	memcpy(ch, tmp, sizeof(struct mmo_charstatus));

	return true;
}

static bool char_db_txt_load_str(CharDB* self, struct mmo_charstatus* ch, const char* name)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;

	// retrieve data
	struct DBIterator* iter = chars->iterator(chars);
	struct mmo_charstatus* tmp;
	int (*compare)(const char* str1, const char* str2) = ( db->case_sensitive ) ? strcmp : stricmp;

	for( tmp = (struct mmo_charstatus*)iter->first(iter,NULL); iter->exists(iter); tmp = (struct mmo_charstatus*)iter->next(iter,NULL) )
		if( compare(name, tmp->name) == 0 )
			break;
	iter->destroy(iter);

	if( tmp == NULL )
	{// entry not found
		return false;
	}

	// store it
	memcpy(ch, tmp, sizeof(struct mmo_charstatus));

	return true;
}

static bool char_db_txt_load_slot(CharDB* self, struct mmo_charstatus* ch, int account_id, int slot)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;

	// retrieve data
	struct DBIterator* iter = chars->iterator(chars);
	struct mmo_charstatus* tmp;

	for( tmp = (struct mmo_charstatus*)iter->first(iter,NULL); iter->exists(iter); tmp = (struct mmo_charstatus*)iter->next(iter,NULL) )
		if( account_id == tmp->account_id && slot == tmp->slot )
			break;
	iter->destroy(iter);

	if( tmp == NULL )
	{// entry not found
		return false;
	}

	// store it
	memcpy(ch, tmp, sizeof(struct mmo_charstatus));

	return true;
}

static bool char_db_txt_id2name(CharDB* self, int char_id, char name[NAME_LENGTH])
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;

	// retrieve data
	struct mmo_charstatus* tmp = idb_get(chars, char_id);
	if( tmp == NULL )
	{// entry not found
		return false;
	}

	if( name != NULL )
		safestrncpy(name, tmp->name, sizeof(name));
	
	return true;
}

static bool char_db_txt_name2id(CharDB* self, const char* name, int* char_id, int* account_id)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;

	// retrieve data
	struct DBIterator* iter = chars->iterator(chars);
	struct mmo_charstatus* tmp;
	int (*compare)(const char* str1, const char* str2) = ( db->case_sensitive ) ? strcmp : stricmp;

	for( tmp = (struct mmo_charstatus*)iter->first(iter,NULL); iter->exists(iter); tmp = (struct mmo_charstatus*)iter->next(iter,NULL) )
		if( compare(name, tmp->name) == 0 )
			break;
	iter->destroy(iter);

	if( tmp == NULL )
	{// entry not found
		return false;
	}

	if( char_id != NULL )
		*char_id = tmp->char_id;
	if( account_id != NULL )
		*account_id = tmp->account_id;

	return true;
}

static bool char_db_txt_slot2id(CharDB* self, int account_id, int slot, int* char_id)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;

	// retrieve data
	struct DBIterator* iter = chars->iterator(chars);
	struct mmo_charstatus* tmp;

	for( tmp = (struct mmo_charstatus*)iter->first(iter,NULL); iter->exists(iter); tmp = (struct mmo_charstatus*)iter->next(iter,NULL) )
		if( account_id == tmp->account_id && slot == tmp->slot )
			break;
	iter->destroy(iter);

	if( tmp == NULL )
	{// entry not found
		return false;
	}

	if( char_id != NULL )
		*char_id = tmp->char_id;

	return true;
}

/// Returns an iterator over all the characters.
static CharDBIterator* char_db_txt_iterator(CharDB* self)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;
	CharDBIterator_TXT* iter = (CharDBIterator_TXT*)aCalloc(1, sizeof(CharDBIterator_TXT));

	// set up the vtable
	iter->vtable.destroy = &char_db_txt_iter_destroy;
	iter->vtable.next    = &char_db_txt_iter_next;

	// fill data
	iter->iter = db_iterator(chars);
	iter->has_account_id = false;

	return &iter->vtable;
}

/// Returns an iterator over all the characters of the account.
static CharDBIterator* char_db_txt_characters(CharDB* self, int account_id)
{
	CharDB_TXT* db = (CharDB_TXT*)self;
	DBMap* chars = db->chars;
	CharDBIterator_TXT* iter = (CharDBIterator_TXT*)aCalloc(1, sizeof(CharDBIterator_TXT));

	// set up the vtable
	iter->vtable.destroy = &char_db_txt_iter_destroy;
	iter->vtable.next    = &char_db_txt_iter_next;

	// fill data
	iter->iter = db_iterator(chars);
	iter->account_id = account_id;
	iter->has_account_id = true;

	return &iter->vtable;
}

/// Destroys this iterator, releasing all allocated memory (including itself).
static void char_db_txt_iter_destroy(CharDBIterator* self)
{
	CharDBIterator_TXT* iter = (CharDBIterator_TXT*)self;
	dbi_destroy(iter->iter);
	aFree(iter);
}

/// Fetches the next character.
static bool char_db_txt_iter_next(CharDBIterator* self, struct mmo_charstatus* ch)
{
	CharDBIterator_TXT* iter = (CharDBIterator_TXT*)self;
	struct mmo_charstatus* tmp;

	while( true )
	{
		tmp = (struct mmo_charstatus*)dbi_next(iter->iter);
		if( tmp == NULL )
			return false;// not found

		if( iter->has_account_id && iter->account_id != tmp->account_id )
			continue;// wrong account, try next

		memcpy(ch, tmp, sizeof(struct mmo_charstatus));
		return true;
	}
}


//-------------------------------------------------------------------------
// Function to set the character from the line (at read of characters file)
//-------------------------------------------------------------------------
int mmo_char_fromstr(CharDB* chars, const char *str, struct mmo_charstatus *p, struct regs* reg)
{
	char tmp_str[3][128]; //To avoid deleting chars with too long names.
	int tmp_int[256];
	unsigned int tmp_uint[2]; //To read exp....
	int next, len, i, j;

	// initilialise character
	memset(p, '\0', sizeof(struct mmo_charstatus));
	
// Char structure of version 1500 (homun + mapindex maps)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%d,%d,%d\t%d,%d,%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0],
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], &tmp_int[26], &tmp_int[44],
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		&tmp_int[45], &tmp_int[35], &tmp_int[36],
		&tmp_int[46], &tmp_int[37], &tmp_int[38], &tmp_int[39], 
		&tmp_int[40], &tmp_int[41], &tmp_int[42], &tmp_int[43], &next) != 48)
	{
	tmp_int[44] = 0; //Hom ID.
// Char structure of version 1488 (fame field addition)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%127[^,],%d,%d\t%127[^,],%d,%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0],
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], &tmp_int[26],
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		tmp_str[1], &tmp_int[35], &tmp_int[36],
		tmp_str[2], &tmp_int[37], &tmp_int[38], &tmp_int[39], 
		&tmp_int[40], &tmp_int[41], &tmp_int[42], &tmp_int[43], &next) != 47)
	{
	tmp_int[43] = 0; //Fame
// Char structure of version 1363 (family data addition)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%127[^,],%d,%d\t%127[^,],%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0], //
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], &tmp_int[26],
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		tmp_str[1], &tmp_int[35], &tmp_int[36], //
		tmp_str[2], &tmp_int[37], &tmp_int[38], &tmp_int[39], 
		&tmp_int[40], &tmp_int[41], &tmp_int[42], &next) != 46)
	{
	tmp_int[40] = 0; // father
	tmp_int[41] = 0; // mother
	tmp_int[42] = 0; // child
// Char structure version 1008 (marriage partner addition)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%127[^,],%d,%d\t%127[^,],%d,%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0], //
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], &tmp_int[26],
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		tmp_str[1], &tmp_int[35], &tmp_int[36], //
		tmp_str[2], &tmp_int[37], &tmp_int[38], &tmp_int[39], &next) != 43)
	{
	tmp_int[39] = 0; // partner id
// Char structure version 384 (pet addition)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%127[^,],%d,%d\t%127[^,],%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0], //
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], &tmp_int[26],
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		tmp_str[1], &tmp_int[35], &tmp_int[36], //
		tmp_str[2], &tmp_int[37], &tmp_int[38], &next) != 42)
	{
	tmp_int[26] = 0; // pet id
// Char structure of a version 1 (original data structure)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%127[^,],%d,%d\t%127[^,],%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0], //
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], //
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		tmp_str[1], &tmp_int[35], &tmp_int[36], //
		tmp_str[2], &tmp_int[37], &tmp_int[38], &next) != 41)
	{
		ShowError("Char-loading: Unrecognized character data version, info lost!\n");
		ShowDebug("Character info: %s\n", str);
		return 0;
	}
	}	// Char structure version 384 (pet addition)
	}	// Char structure version 1008 (marriage partner addition)
	}	// Char structure of version 1363 (family data addition)
	}	// Char structure of version 1488 (fame field addition)
	//Convert save data from string to integer for older formats
		tmp_int[45] = mapindex_name2id(tmp_str[1]);
		tmp_int[46] = mapindex_name2id(tmp_str[2]);
	}	// Char structure of version 1500 (homun + mapindex maps)

	memcpy(p->name, tmp_str[0], NAME_LENGTH); //Overflow protection [Skotlex]
	p->char_id = tmp_int[0];
	p->account_id = tmp_int[1];
	p->slot = tmp_int[2];
	p->class_ = tmp_int[3];
	p->base_level = tmp_int[4];
	p->job_level = tmp_int[5];
	p->base_exp = tmp_uint[0];
	p->job_exp = tmp_uint[1];
	p->zeny = tmp_int[8];
	p->hp = tmp_int[9];
	p->max_hp = tmp_int[10];
	p->sp = tmp_int[11];
	p->max_sp = tmp_int[12];
	p->str = tmp_int[13];
	p->agi = tmp_int[14];
	p->vit = tmp_int[15];
	p->int_ = tmp_int[16];
	p->dex = tmp_int[17];
	p->luk = tmp_int[18];
	p->status_point = min(tmp_int[19], USHRT_MAX);
	p->skill_point = min(tmp_int[20], USHRT_MAX);
	p->option = tmp_int[21];
	p->karma = tmp_int[22];
	p->manner = tmp_int[23];
	p->party_id = tmp_int[24];
	p->guild_id = tmp_int[25];
	p->pet_id = tmp_int[26];
	p->hair = tmp_int[27];
	p->hair_color = tmp_int[28];
	p->clothes_color = tmp_int[29];
	p->weapon = tmp_int[30];
	p->shield = tmp_int[31];
	p->head_top = tmp_int[32];
	p->head_mid = tmp_int[33];
	p->head_bottom = tmp_int[34];
	p->last_point.x = tmp_int[35];
	p->last_point.y = tmp_int[36];
	p->save_point.x = tmp_int[37];
	p->save_point.y = tmp_int[38];
	p->partner_id = tmp_int[39];
	p->father = tmp_int[40];
	p->mother = tmp_int[41];
	p->child = tmp_int[42];
	p->fame = tmp_int[43];
	p->hom_id = tmp_int[44];
	p->last_point.map = tmp_int[45];
	p->save_point.map = tmp_int[46];

#ifndef TXT_SQL_CONVERT
	// Some checks
	//TODO: just a check is needed here (loading all data is excessive)
	if( chars->id2name(chars, p->char_id, NULL) )
	{
		ShowError(CL_RED"mmmo_auth_init: a character has an identical id to another.\n");
		ShowError("               character id #%d -> new character not read.\n", p->char_id);
		ShowError("               Character saved in log file."CL_RESET"\n");
		return -1;
	}
	if( chars->name2id(chars, p->name, NULL, NULL) )
	{
		ShowError(CL_RED"mmmo_auth_init: a character name already exists.\n");
		ShowError("               character name '%s' -> new character not read.\n", p->name);
		ShowError("               Character saved in log file."CL_RESET"\n");
		return -2;
	}

	if (strcmpi(wisp_server_name, p->name) == 0) {
		ShowWarning("mmo_auth_init: ******WARNING: character name has wisp server name.\n");
		ShowWarning("               Character name '%s' = wisp server name '%s'.\n", p->name, wisp_server_name);
		ShowWarning("               Character readed. Suggestion: change the wisp server name.\n");
		log_char("mmo_auth_init: ******WARNING: character name has wisp server name: Character name '%s' = wisp server name '%s'.\n",
		          p->name, wisp_server_name);
	}
#endif //TXT_SQL_CONVERT
	if (str[next] == '\n' || str[next] == '\r')
		return 1;	// 新規データ

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		//mapindex memo format
		if (sscanf(str+next, "%d,%d,%d%n", &tmp_int[2], &tmp_int[0], &tmp_int[1], &len) != 3)
		{	//Old string-based memo format.
			if (sscanf(str+next, "%[^,],%d,%d%n", tmp_str[0], &tmp_int[0], &tmp_int[1], &len) != 3)
				return -3;
			tmp_int[2] = mapindex_name2id(tmp_str[0]);
		}
		if (i < MAX_MEMOPOINTS)
	  	{	//Avoid overflowing (but we must also read through all saved memos)
			p->memo_point[i].x = tmp_int[0];
			p->memo_point[i].y = tmp_int[1];
			p->memo_point[i].map = tmp_int[2];
		}
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if(sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d%[0-9,-]%n",
		      &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		      &tmp_int[4], &tmp_int[5], &tmp_int[6], tmp_str[0], &len) == 8)
		{
			p->inventory[i].id = tmp_int[0];
			p->inventory[i].nameid = tmp_int[1];
			p->inventory[i].amount = tmp_int[2];
			p->inventory[i].equip = tmp_int[3];
			p->inventory[i].identify = tmp_int[4];
			p->inventory[i].refine = tmp_int[5];
			p->inventory[i].attribute = tmp_int[6];

			for(j = 0; j < MAX_SLOTS && tmp_str[0] && sscanf(tmp_str[0], ",%d%[0-9,-]",&tmp_int[0], tmp_str[0]) > 0; j++)
				p->inventory[i].card[j] = tmp_int[0];

			next += len;
			if (str[next] == ' ')
				next++;
		} else // invalid structure
			return -4;
	}
	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if(sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d%[0-9,-]%n",
		      &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		      &tmp_int[4], &tmp_int[5], &tmp_int[6], tmp_str[0], &len) == 8)
		{
			p->cart[i].id = tmp_int[0];
			p->cart[i].nameid = tmp_int[1];
			p->cart[i].amount = tmp_int[2];
			p->cart[i].equip = tmp_int[3];
			p->cart[i].identify = tmp_int[4];
			p->cart[i].refine = tmp_int[5];
			p->cart[i].attribute = tmp_int[6];
			
			for(j = 0; j < MAX_SLOTS && tmp_str && sscanf(tmp_str[0], ",%d%[0-9,-]",&tmp_int[0], tmp_str[0]) > 0; j++)
				p->cart[i].card[j] = tmp_int[0];
			
			next += len;
			if (str[next] == ' ')
				next++;
		} else // invalid structure
			return -5;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if (sscanf(str + next, "%d,%d%n", &tmp_int[0], &tmp_int[1], &len) != 2)
			return -6;
		p->skill[tmp_int[0]].id = tmp_int[0];
		p->skill[tmp_int[0]].lv = tmp_int[1];
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	// parse character regs
	if( !mmo_charreg_fromstr(reg, str + next) )
		return -7;

	return 1;
}

//-------------------------------------------------
// Function to create the character line (for save)
//-------------------------------------------------
int mmo_char_tostr(char *str, struct mmo_charstatus *p, const struct regs* reg)
{
	int i,j;
	char *str_p = str;

	// character data
	str_p += sprintf(str_p,
		"%d\t%d,%d\t%s\t%d,%d,%d\t%u,%u,%d" //Up to Zeny field
		"\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d" //Up to Skill Point
		"\t%d,%d,%d\t%d,%d,%d,%d" //Up to hom id
		"\t%d,%d,%d\t%d,%d,%d,%d,%d" //Up to head bottom
		"\t%d,%d,%d\t%d,%d,%d" //last point + save point
		",%d,%d,%d,%d,%d\t",	//Family info
		p->char_id, p->account_id, p->slot, p->name, //
		p->class_, p->base_level, p->job_level,
		p->base_exp, p->job_exp, p->zeny,
		p->hp, p->max_hp, p->sp, p->max_sp,
		p->str, p->agi, p->vit, p->int_, p->dex, p->luk,
		p->status_point, p->skill_point,
		p->option, p->karma, p->manner,	//
		p->party_id, p->guild_id, p->pet_id, p->hom_id,
		p->hair, p->hair_color, p->clothes_color,
		p->weapon, p->shield, p->head_top, p->head_mid, p->head_bottom,
		p->last_point.map, p->last_point.x, p->last_point.y, //
		p->save_point.map, p->save_point.x, p->save_point.y,
		p->partner_id,p->father,p->mother,p->child,p->fame);
	for(i = 0; i < MAX_MEMOPOINTS; i++)
		if (p->memo_point[i].map) {
			str_p += sprintf(str_p, "%d,%d,%d ", p->memo_point[i].map, p->memo_point[i].x, p->memo_point[i].y);
		}
	*(str_p++) = '\t';

	// inventory
	for(i = 0; i < MAX_INVENTORY; i++)
		if (p->inventory[i].nameid) {
			str_p += sprintf(str_p,"%d,%d,%d,%d,%d,%d,%d",
				p->inventory[i].id,p->inventory[i].nameid,p->inventory[i].amount,p->inventory[i].equip,
				p->inventory[i].identify,p->inventory[i].refine,p->inventory[i].attribute);
			for(j=0; j<MAX_SLOTS; j++)
				str_p += sprintf(str_p,",%d",p->inventory[i].card[j]);
			str_p += sprintf(str_p," ");
		}
	*(str_p++) = '\t';

	// cart
	for(i = 0; i < MAX_CART; i++)
		if (p->cart[i].nameid) {
			str_p += sprintf(str_p,"%d,%d,%d,%d,%d,%d,%d",
				p->cart[i].id,p->cart[i].nameid,p->cart[i].amount,p->cart[i].equip,
				p->cart[i].identify,p->cart[i].refine,p->cart[i].attribute);
			for(j=0; j<MAX_SLOTS; j++)
				str_p += sprintf(str_p,",%d",p->cart[i].card[j]);
			str_p += sprintf(str_p," ");
		}
	*(str_p++) = '\t';

	// skills
	for(i = 0; i < MAX_SKILL; i++)
		if (p->skill[i].id && p->skill[i].flag != 1) {
			str_p += sprintf(str_p, "%d,%d ", p->skill[i].id, (p->skill[i].flag == 0) ? p->skill[i].lv : p->skill[i].flag-2);
		}
	*(str_p++) = '\t';

	// registry
	if( reg != NULL )
	{
		mmo_charreg_tostr(reg, str_p);
		str_p += strlen(str_p);
	}
	*(str_p++) = '\t';

	*str_p = '\0';
	return 0;
}

/// Dumps the entire char db (+ associated data) to disk
static void mmo_char_sync(CharDB_TXT* db)
{
	CharRegDB* charregs = db->owner->charregdb;
	int lock;
	FILE *fp;
	void* data;
	struct DBIterator* iter;

	// Data save
	fp = lock_fopen(db->char_db, &lock);
	if( fp == NULL )
	{
		ShowWarning("Server cannot save characters.\n");
		log_char("WARNING: Server cannot save characters.\n");
		return;
	}

	iter = db->chars->iterator(db->chars);
	for( data = iter->first(iter,NULL); iter->exists(iter); data = iter->next(iter,NULL) )
	{
		struct mmo_charstatus* ch = (struct mmo_charstatus*) data;
		char line[65536]; // ought to be big enough
		struct regs reg;

		charregs->load(charregs, &reg, ch->char_id);
		mmo_char_tostr(line, ch, &reg);
		fprintf(fp, "%s\n", line);
	}
	fprintf(fp, "%d\t%%newid%%\n", db->next_char_id);
	iter->destroy(iter);

	lock_fclose(fp, db->char_db, &lock);
}

/// Periodic data saving function
int mmo_char_sync_timer(int tid, unsigned int tick, int id, intptr data)
{
	CharDB_TXT* db = (CharDB_TXT*)data;
	FriendDB* friends = db->owner->frienddb;
	HotkeyDB* hotkeys = db->owner->hotkeydb;

	if (save_log)
		ShowInfo("Saving all files...\n");

	mmo_char_sync(db);

	friends->sync(friends);

#ifdef HOTKEY_SAVING
	hotkeys->sync(hotkeys);
#endif

	inter_save();
	return 0;
}
