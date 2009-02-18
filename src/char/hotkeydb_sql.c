// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/malloc.h"
#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/sql.h"
#include "../common/strlib.h"
#include "charserverdb_sql.h"
#include "hotkeydb.h"
#include <stdlib.h>
#include <string.h>


/// internal structure
typedef struct HotkeyDB_SQL
{
	HotkeyDB vtable;    // public interface

	CharServerDB_SQL* owner;
	Sql* hotkeys;       // SQL hotkey storage

	// other settings
	const char* hotkey_db;

} HotkeyDB_SQL;



static bool mmo_hotkeylist_fromsql(HotkeyDB_SQL* db, hotkeylist* list, int char_id)
{
	Sql* sql_handle = db->hotkeys;
	SqlStmt* stmt = SqlStmt_Malloc(sql_handle);
	struct hotkey tmp_hotkey;
	int hotkey_num;
	bool result = false;

	do
	{

	//`hotkey` (`char_id`, `hotkey`, `type`, `itemskill_id`, `skill_lvl`)
	if( SQL_ERROR == SqlStmt_Prepare(stmt, "SELECT `hotkey`, `type`, `itemskill_id`, `skill_lvl` FROM `%s` WHERE `char_id`=%d", db->hotkey_db, char_id)
	||	SQL_ERROR == SqlStmt_Execute(stmt)
	||	SQL_ERROR == SqlStmt_BindColumn(stmt, 0, SQLDT_INT,    &hotkey_num,      0, NULL, NULL)
	||	SQL_ERROR == SqlStmt_BindColumn(stmt, 1, SQLDT_UCHAR,  &tmp_hotkey.type, 0, NULL, NULL)
	||	SQL_ERROR == SqlStmt_BindColumn(stmt, 2, SQLDT_UINT,   &tmp_hotkey.id,   0, NULL, NULL)
	||	SQL_ERROR == SqlStmt_BindColumn(stmt, 3, SQLDT_USHORT, &tmp_hotkey.lv,   0, NULL, NULL) )
	{
		SqlStmt_ShowDebug(stmt);
		break;
	}

	while( SQL_SUCCESS == SqlStmt_NextRow(stmt) )
	{
		if( hotkey_num >= 0 && hotkey_num < MAX_HOTKEYS )
			memcpy(&(*list)[hotkey_num], &tmp_hotkey, sizeof(tmp_hotkey));
		else
			ShowWarning("mmo_hotkeylist_fromsql: ignoring invalid hotkey (hotkey=%d,type=%u,id=%u,lv=%u) of character with CID=%d\n", hotkey_num, tmp_hotkey.type, tmp_hotkey.id, tmp_hotkey.lv, char_id);
	}

	result = true;

	}
	while(0);

	return result;
}


static bool mmo_hotkeylist_tosql(HotkeyDB_SQL* db, const hotkeylist* list, int char_id)
{
	Sql* sql_handle = db->hotkeys;
	StringBuf buf;
	bool result = false;
	int i, count;

	StringBuf_Init(&buf);

	do
	{

	StringBuf_Printf(&buf, "REPLACE INTO `%s` (`char_id`, `hotkey`, `type`, `itemskill_id`, `skill_lvl`) VALUES ", db->hotkey_db);
	for( i = 0, count = 0; i < MAX_HOTKEYS; ++i )
	{
		if( count != 0 )
			StringBuf_AppendStr(&buf, ",");

		StringBuf_Printf(&buf, "('%d','%d','%u','%u','%u')", char_id, i, (*list)[i].type, (*list)[i].id , (*list)[i].lv);
		count++;
	}

	if( count > 0 )
	{
		if( SQL_ERROR == Sql_QueryStr(sql_handle, StringBuf_Value(&buf)) )
		{
			Sql_ShowDebug(sql_handle);
			break;
		}
	}

	result = true;

	}
	while(0);

	StringBuf_Destroy(&buf);

	return result;
}


static bool hotkey_db_sql_init(HotkeyDB* self)
{
	HotkeyDB_SQL* db = (HotkeyDB_SQL*)self;
	db->hotkeys = db->owner->sql_handle;
	return true;
}

static void hotkey_db_sql_destroy(HotkeyDB* self)
{
	HotkeyDB_SQL* db = (HotkeyDB_SQL*)self;
	db->hotkeys = NULL;
	aFree(db);
}

static bool hotkey_db_sql_sync(HotkeyDB* self)
{
	return true;
}

static bool hotkey_db_sql_remove(HotkeyDB* self, const int char_id)
{
	HotkeyDB_SQL* db = (HotkeyDB_SQL*)self;
	Sql* sql_handle = db->hotkeys;

	if( SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `%s` WHERE `char_id`='%d'", db->hotkey_db, char_id) )
	{
		Sql_ShowDebug(sql_handle);
		return false;
	}

	return true;
}

static bool hotkey_db_sql_save(HotkeyDB* self, const hotkeylist* list, const int char_id)
{
	HotkeyDB_SQL* db = (HotkeyDB_SQL*)self;
	return mmo_hotkeylist_tosql(db, list, char_id);
}

static bool hotkey_db_sql_load(HotkeyDB* self, hotkeylist* list, const int char_id)
{
	HotkeyDB_SQL* db = (HotkeyDB_SQL*)self;
	return mmo_hotkeylist_fromsql(db, list, char_id);
}


/// public constructor
HotkeyDB* hotkey_db_sql(CharServerDB_SQL* owner)
{
	HotkeyDB_SQL* db = (HotkeyDB_SQL*)aCalloc(1, sizeof(HotkeyDB_SQL));

	// set up the vtable
	db->vtable.init      = &hotkey_db_sql_init;
	db->vtable.destroy   = &hotkey_db_sql_destroy;
	db->vtable.sync      = &hotkey_db_sql_sync;
	db->vtable.remove    = &hotkey_db_sql_remove;
	db->vtable.save      = &hotkey_db_sql_save;
	db->vtable.load      = &hotkey_db_sql_load;

	// initialize to default values
	db->owner = owner;
	db->hotkeys = NULL;

	// other settings
	db->hotkey_db = db->owner->table_hotkeys;

	return &db->vtable;
}
