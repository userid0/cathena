// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/malloc.h"
#include "../common/strlib.h"
#include "charserverdb_sql.h"

#include <string.h>
#include <stdlib.h>



/// Initializes this database engine, making it ready for use.
static bool charserver_db_sql_init(CharServerDB* self)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;
	Sql* sql_handle;
	const char* username;
	const char* password;
	const char* hostname;
	uint16      port;
	const char* database;
	const char* codepage;

	if( db->initialized )
		return true;// already initialized

	sql_handle = Sql_Malloc();
	username = db->global_db_username;
	password = db->global_db_password;
	hostname = db->global_db_hostname;
	port     = db->global_db_port;
	database = db->global_db_database;
	codepage = db->global_codepage;

	if( SQL_ERROR == Sql_Connect(sql_handle, username, password, hostname, port, database) )
	{
		Sql_ShowDebug(sql_handle);
		Sql_Free(sql_handle);
		return false;
	}

	db->sql_handle = sql_handle;
	if( codepage[0] != '\0' && SQL_ERROR == Sql_SetEncoding(sql_handle, codepage) )
		Sql_ShowDebug(sql_handle);

	// TODO DB interfaces
	if( db->castledb->init(db->castledb) && db->chardb->init(db->chardb) && db->guilddb->init(db->guilddb) && db->homundb->init(db->homundb) && db->petdb->init(db->petdb) && rank_db_sql_init(db->rankdb) && db->statusdb->init(db->statusdb) )
		db->initialized = true;

	return db->initialized;
}



/// Destroys this database engine, releasing all allocated memory (including itself).
static void charserver_db_sql_destroy(CharServerDB* self)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;

	db->castledb->destroy(db->castledb);
	db->castledb = NULL;
	db->chardb->destroy(db->chardb);
	db->chardb = NULL;
	db->guilddb->destroy(db->guilddb);
	db->guilddb = NULL;
	db->homundb->destroy(db->homundb);
	db->homundb = NULL;
	db->petdb->destroy(db->petdb);
	db->petdb = NULL;
	rank_db_sql_destroy(db->rankdb);
	db->rankdb = NULL;
	db->statusdb->destroy(db->statusdb);
	db->statusdb = NULL;
	// TODO DB interfaces
	Sql_Free(db->sql_handle);
	db->sql_handle = NULL;
	aFree(db);
}



/// Gets a property from this database engine.
static bool charserver_db_sql_get_property(CharServerDB* self, const char* key, char* buf, size_t buflen)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;
	const char* signature;

	signature = "engine.";
	if( strncmpi(key, signature, strlen(signature)) == 0 )
	{
		key += strlen(signature);
		if( strcmpi(key, "name") == 0 )
			safesnprintf(buf, buflen, "sql");
		else
		if( strcmpi(key, "version") == 0 )
			safesnprintf(buf, buflen, "%d", CHARSERVERDB_SQL_VERSION);
		else
		if( strcmpi(key, "comment") == 0 )
			safesnprintf(buf, buflen, "CharServerDB SQL engine");
		else
			return false;// not found
		return true;
	}

	signature = "sql.";
	if( strncmpi(key, signature, strlen(signature)) == 0 )
	{
		key += strlen(signature);
		if( strcmpi(key, "db_hostname") == 0 )
			safesnprintf(buf, buflen, "%s", db->global_db_hostname);
		else
		if( strcmpi(key, "db_port") == 0 )
			safesnprintf(buf, buflen, "%d", db->global_db_port);
		else
		if( strcmpi(key, "db_username") == 0 )
			safesnprintf(buf, buflen, "%s", db->global_db_username);
		else
		if(	strcmpi(key, "db_password") == 0 )
			safesnprintf(buf, buflen, "%s", db->global_db_password);
		else
		if( strcmpi(key, "db_database") == 0 )
			safesnprintf(buf, buflen, "%s", db->global_db_database);
		else
		if( strcmpi(key, "codepage") == 0 )
			safesnprintf(buf, buflen, "%s", db->global_codepage);
		else
			return false;// not found
		return true;
	}

	// TODO DB interface properties

	return false;// not found
}



/// Sets a property in this database engine.
static bool charserver_db_sql_set_property(CharServerDB* self, const char* key, const char* value)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;
	const char* signature;


	signature = "sql.";
	if( strncmp(key, signature, strlen(signature)) == 0 )
	{
		key += strlen(signature);
		if( strcmpi(key, "db_hostname") == 0 )
			safestrncpy(db->global_db_hostname, value, sizeof(db->global_db_hostname));
		else
		if( strcmpi(key, "db_port") == 0 )
			db->global_db_port = (uint16)strtoul(value, NULL, 10);
		else
		if( strcmpi(key, "db_username") == 0 )
			safestrncpy(db->global_db_username, value, sizeof(db->global_db_username));
		else
		if( strcmpi(key, "db_password") == 0 )
			safestrncpy(db->global_db_password, value, sizeof(db->global_db_password));
		else
		if( strcmpi(key, "db_database") == 0 )
			safestrncpy(db->global_db_database, value, sizeof(db->global_db_database));
		else
		if( strcmpi(key, "codepage") == 0 )
			safestrncpy(db->global_codepage, value, sizeof(db->global_codepage));
		else
			return false;// not found
		return true;
	}

	// TODO DB interface properties

	return false;// not found
}



/// TODO
static CastleDB* charserver_db_sql_castledb(CharServerDB* self)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;

	return db->castledb;
}



/// TODO
static CharDB* charserver_db_sql_chardb(CharServerDB* self)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;

	return db->chardb;
}



/// TODO
static GuildDB* charserver_db_sql_guilddb(CharServerDB* self)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;

	return db->guilddb;
}



/// TODO
static HomunDB* charserver_db_sql_homundb(CharServerDB* self)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;

	return db->homundb;
}



/// TODO
static PetDB* charserver_db_sql_petdb(CharServerDB* self)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;

	return db->petdb;
}



/// Returns the database interface that handles rankings.
static RankDB* charserver_db_sql_rankdb(CharServerDB* self)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;

	return db->rankdb;
}



/// TODO
static StatusDB* charserver_db_sql_statusdb(CharServerDB* self)
{
	CharServerDB_SQL* db = (CharServerDB_SQL*)self;

	return db->statusdb;
}



/// constructor
CharServerDB* charserver_db_sql(void)
{
	CharServerDB_SQL* db;

	CREATE(db, CharServerDB_SQL, 1);
	db->vtable.init         = charserver_db_sql_init;
	db->vtable.destroy      = charserver_db_sql_destroy;
	db->vtable.get_property = charserver_db_sql_get_property;
	db->vtable.set_property = charserver_db_sql_set_property;
	db->vtable.castledb     = charserver_db_sql_castledb;
	db->vtable.chardb       = charserver_db_sql_chardb;
	db->vtable.guilddb      = charserver_db_sql_guilddb;
	db->vtable.homundb      = charserver_db_sql_homundb;
	db->vtable.petdb        = charserver_db_sql_petdb;
	db->vtable.rankdb       = charserver_db_sql_rankdb;
	db->vtable.statusdb     = charserver_db_sql_statusdb;
	// TODO DB interfaces

	// initialize to default values
	db->sql_handle = NULL;
	db->initialized = false;

	db->castledb = castle_db_sql(db);
	db->chardb = char_db_sql(db);
	db->guilddb = guild_db_sql(db);
	db->homundb = homun_db_sql(db);
	db->petdb = pet_db_sql(db);
	db->rankdb = rank_db_sql(db);
	db->statusdb = status_db_sql(db);

	// global sql settings
	safestrncpy(db->global_db_hostname, "127.0.0.1", sizeof(db->global_db_hostname));
	db->global_db_port = 3306;
	safestrncpy(db->global_db_username, "ragnarok", sizeof(db->global_db_username));
	safestrncpy(db->global_db_password, "ragnarok", sizeof(db->global_db_password));
	safestrncpy(db->global_db_database, "ragnarok", sizeof(db->global_db_database));
	safestrncpy(db->global_codepage, "", sizeof(db->global_codepage));
	// other settings
	safestrncpy(db->table_chars, "char", sizeof(db->table_chars));

	return &db->vtable;
}
