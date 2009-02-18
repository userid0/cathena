// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/db.h"
#include "../common/malloc.h"
#include "../common/mmo.h"
#include "../common/sql.h"
#include "../common/showmsg.h"
#include "../common/socket.h"
#include "../common/strlib.h"
#include "../common/timer.h"
#include "charserverdb_sql.h"
#include "inter.h"
#include "accregdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/// internal structure
typedef struct AccRegDB_SQL
{
	AccRegDB vtable;        // public interface

	CharServerDB_SQL* owner;
	Sql* accregs;           // SQL accreg storage
	
	// other settings
	const char* accreg_db;

} AccRegDB_SQL;



static bool mmo_accreg_fromsql(AccRegDB_SQL* db, struct regs* reg, int account_id)
{
	Sql* sql_handle = db->accregs;
	int i;

	memset(reg, 0, sizeof(struct regs));

	//`global_reg_value` (`type`, `account_id`, `char_id`, `str`, `value`)
	if( SQL_ERROR == Sql_Query(sql_handle, "SELECT `str`, `value` FROM `%s` WHERE `type`=2 AND `account_id`='%d'", db->accreg_db, account_id) )
		Sql_ShowDebug(sql_handle);
	for( i = 0; i < MAX_REG_NUM && SQL_SUCCESS == Sql_NextRow(sql_handle); ++i )
	{
		char* data;
		size_t len;
		struct global_reg* r = &reg->reg[i];

		Sql_GetData(sql_handle, 0, &data, &len); memcpy(r->str, data, min(len, sizeof(r->str)));
		Sql_GetData(sql_handle, 1, &data, &len); memcpy(r->value, data, min(len, sizeof(r->value)));
	}
	reg->reg_num = i;
	Sql_FreeResult(sql_handle);

	return true;
}


static bool mmo_accreg_tosql(AccRegDB_SQL* db, const struct regs* reg, int account_id)
{
	Sql* sql_handle = db->accregs;
	SqlStmt* stmt = NULL;
	int i = 0;
	bool result = false;

	if( SQL_SUCCESS != Sql_QueryStr(sql_handle, "START TRANSACTION") )
	{
		Sql_ShowDebug(sql_handle);
		return result;
	}

	// try
	do
	{

	//`global_reg_value` (`type`, `account_id`, `char_id`, `str`, `value`)
	if( SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `%s` WHERE `type`=2 AND `account_id`='%d'", db->accreg_db, account_id) )
	{
		Sql_ShowDebug(sql_handle);
		break;
	}

	if( reg->reg_num <= 0 )
	{// nothing more needed
		result = true;
		break;
	}

	stmt = SqlStmt_Malloc(sql_handle);
	if( SQL_ERROR == SqlStmt_Prepare(stmt, "INSERT INTO `%s` (`type`, `account_id`, `str`, `value`) VALUES (2,'%d',?,?)", db->accreg_db, account_id) )
		SqlStmt_ShowDebug(stmt);

	for( i = 0; i < reg->reg_num; ++i )
	{
		const struct global_reg* r = &reg->reg[i];
		if( r->str[0] == '\0' || r->value[0] == '\0' )
			continue; // should not save these

		SqlStmt_BindParam(stmt, 0, SQLDT_STRING, (void*)r->str, strnlen(r->str, sizeof(r->str)));
		SqlStmt_BindParam(stmt, 1, SQLDT_STRING, (void*)r->value, strnlen(r->value, sizeof(r->value)));

		if( SQL_ERROR == SqlStmt_Execute(stmt) )
		{
			SqlStmt_ShowDebug(stmt);
			break;
		}
	}

	if( i < reg->reg_num )
		break; // failed

	// success
	result = true;

	}
	while(0);
	// finally

	SqlStmt_Free(stmt);

	if( SQL_SUCCESS != Sql_QueryStr(sql_handle, (result == true) ? "COMMIT" : "ROLLBACK") )
	{
		Sql_ShowDebug(sql_handle);
		result = false;
	}

	return result;
}


static bool accreg_db_sql_init(AccRegDB* self)
{
	AccRegDB_SQL* db = (AccRegDB_SQL*)self;
	db->accregs = db->owner->sql_handle;
	return true;
}

static void accreg_db_sql_destroy(AccRegDB* self)
{
	AccRegDB_SQL* db = (AccRegDB_SQL*)self;
	db->accregs = NULL;
	aFree(db);
}

static bool accreg_db_sql_sync(AccRegDB* self)
{
	return true;
}

static bool accreg_db_sql_remove(AccRegDB* self, const int account_id)
{
	AccRegDB_SQL* db = (AccRegDB_SQL*)self;
	Sql* sql_handle = db->accregs;

	if( SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `%s` WHERE `type`=2 AND `account_id`='%d'", db->accreg_db, account_id) )
	{
		Sql_ShowDebug(sql_handle);
		return false;
	}

	return true;
}

static bool accreg_db_sql_save(AccRegDB* self, const struct regs* reg, int account_id)
{
	AccRegDB_SQL* db = (AccRegDB_SQL*)self;
	return mmo_accreg_tosql(db, reg, account_id);
}

static bool accreg_db_sql_load(AccRegDB* self, struct regs* reg, int account_id)
{
	AccRegDB_SQL* db = (AccRegDB_SQL*)self;
	return mmo_accreg_fromsql(db, reg, account_id);
}


/// public constructor
AccRegDB* accreg_db_sql(CharServerDB_SQL* owner)
{
	AccRegDB_SQL* db = (AccRegDB_SQL*)aCalloc(1, sizeof(AccRegDB_SQL));

	// set up the vtable
	db->vtable.init    = &accreg_db_sql_init;
	db->vtable.destroy = &accreg_db_sql_destroy;
	db->vtable.sync    = &accreg_db_sql_sync;
	db->vtable.remove  = &accreg_db_sql_remove;
	db->vtable.save    = &accreg_db_sql_save;
	db->vtable.load    = &accreg_db_sql_load;

	// initialize to default values
	db->owner = owner;
	db->accregs = NULL;

	// other settings
	db->accreg_db = db->owner->table_registry;

	return &db->vtable;
}
