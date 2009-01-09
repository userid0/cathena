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
#include "maildb.h"
#include <stdio.h>
#include <string.h>

#define START_MAIL_NUM 1


/// internal structure
typedef struct MailDB_TXT
{
	MailDB vtable;             // public interface

	CharServerDB_TXT* owner;
	DBMap* mails;              // in-memory mail storage
	unsigned int next_mail_id; // auto_increment

	char mail_db[1024];        // mail data storage file

} MailDB_TXT;

/// internal functions
static bool mail_db_txt_init(MailDB* self);
static void mail_db_txt_destroy(MailDB* self);
static bool mail_db_txt_sync(MailDB* self);
static bool mail_db_txt_create(MailDB* self, struct mail_message* msg);
static bool mail_db_txt_remove(MailDB* self, const int mail_id);
static bool mail_db_txt_save(MailDB* self, const struct mail_message* msg);
static bool mail_db_txt_load(MailDB* self, struct mail_message* msg, const int mail_id);
static bool mail_db_txt_loadall(MailDB* self, struct mail_data* md, const int char_id);

static bool mmo_mail_tostr(const struct mail_message* msg, char* str);
static bool mmo_mail_fromstr(struct mail_message* msg, const char* str);
static bool mmo_mail_sync(MailDB_TXT* db);

/// public constructor
MailDB* mail_db_txt(CharServerDB_TXT* owner)
{
	MailDB_TXT* db = (MailDB_TXT*)aCalloc(1, sizeof(MailDB_TXT));

	// set up the vtable
	db->vtable.init      = &mail_db_txt_init;
	db->vtable.destroy   = &mail_db_txt_destroy;
	db->vtable.sync      = &mail_db_txt_sync;
	db->vtable.create    = &mail_db_txt_create;
	db->vtable.remove    = &mail_db_txt_remove;
	db->vtable.save      = &mail_db_txt_save;
	db->vtable.load      = &mail_db_txt_load;
	db->vtable.loadall   = &mail_db_txt_loadall;

	// initialize to default values
	db->owner = owner;
	db->mails = NULL;
	db->next_mail_id = START_MAIL_NUM;
	// other settings
	safestrncpy(db->mail_db, "save/mail.txt", sizeof(db->mail_db));

	return &db->vtable;
}


/* ------------------------------------------------------------------------- */


static bool mail_db_txt_init(MailDB* self)
{
	MailDB_TXT* db = (MailDB_TXT*)self;
	DBMap* mails;

	char line[8192];
	FILE *fp;

	// create mail database
	db->mails = idb_alloc(DB_OPT_RELEASE_DATA);
	mails = db->mails;

	// open data file
	fp = fopen(db->mail_db, "r");
	if( fp == NULL )
	{
		ShowError("Mail file not found: %s.\n", db->mail_db);
		return false;
	}

	// load data file
	while( fgets(line, sizeof(line), fp) )
	{
		int mail_id, n;
		struct mail_message* msg;

		n = 0;
		if( sscanf(line, "%d\t%%newid%%%n", &mail_id, &n) == 1 && n > 0 && (line[n] == '\n' || line[n] == '\r') )
		{// auto-increment
			if( mail_id > db->next_mail_id )
				db->next_mail_id = mail_id;
			continue;
		}

		msg = (struct mail_message*)aMalloc(sizeof(struct mail_message));
		if( !mmo_mail_fromstr(msg, line) )
		{
			ShowError("mail_db_txt_init: skipping invalid data: %s", line);
			aFree(msg);
			continue;
		}
	
		// record entry in db
		idb_put(mails, msg->id, msg);

		if( msg->id >= db->next_mail_id )
			db->next_mail_id = msg->id + 1;
	}

	// close data file
	fclose(fp);

	return true;
}

static void mail_db_txt_destroy(MailDB* self)
{
	MailDB_TXT* db = (MailDB_TXT*)self;
	DBMap* mails = db->mails;

	// write data
	mmo_mail_sync(db);

	// delete mail database
	mails->destroy(mails, NULL);
	db->mails = NULL;

	// delete entire structure
	aFree(db);
}

static bool mail_db_txt_sync(MailDB* self)
{
	MailDB_TXT* db = (MailDB_TXT*)self;
	return mmo_mail_sync(db);
}

static bool mail_db_txt_create(MailDB* self, struct mail_message* msg)
{
	MailDB_TXT* db = (MailDB_TXT*)self;
	DBMap* mails = db->mails;
	struct mail_message* tmp;

	// decide on the mail id to assign
	int mail_id = ( msg->id != -1 ) ? msg->id : db->next_mail_id;

	// check if the mail id is free
	tmp = idb_get(mails, mail_id);
	if( tmp != NULL )
	{// error condition - entry already present
		ShowError("mail_db_txt_create: cannot create mail %d:'%s', this id is already occupied by %d:'%s'!\n", mail_id, msg->title, mail_id, tmp->title);
		return false;
	}

	// copy the data and store it in the db
	CREATE(tmp, struct mail_message, 1);
	memcpy(tmp, msg, sizeof(struct mail_message));
	tmp->id = mail_id;
	idb_put(mails, mail_id, tmp);

	// increment the auto_increment value
	if( mail_id >= db->next_mail_id )
		db->next_mail_id = mail_id + 1;

	// flush data
	mmo_mail_sync(db);

	// write output
	msg->id = mail_id;

	return true;
}

static bool mail_db_txt_remove(MailDB* self, const int mail_id)
{
	MailDB_TXT* db = (MailDB_TXT*)self;
	DBMap* mails = db->mails;

	struct mail_message* tmp = (struct mail_message*)idb_remove(mails, mail_id);
	if( tmp == NULL )
	{// error condition - entry not present
		ShowError("mail_db_txt_remove: no such mail with id %d\n", mail_id);
		return false;
	}

	return true;
}

static bool mail_db_txt_save(MailDB* self, const struct mail_message* msg)
{
	MailDB_TXT* db = (MailDB_TXT*)self;
	DBMap* mails = db->mails;
	int mail_id = msg->id;

	// retrieve previous data
	struct mail_message* tmp = idb_get(mails, mail_id);
	if( tmp == NULL )
	{// error condition - entry not found
		return false;
	}
	
	// overwrite with new data
	memcpy(tmp, msg, sizeof(struct mail_message));

	return true;
}

static bool mail_db_txt_load(MailDB* self, struct mail_message* msg, const int mail_id)
{
	MailDB_TXT* db = (MailDB_TXT*)self;
	DBMap* mails = db->mails;

	// retrieve data
	struct mail_message* tmp = idb_get(mails, mail_id);
	if( tmp == NULL )
	{// entry not found
		return false;
	}

	// store it
	memcpy(msg, tmp, sizeof(struct mail_message));

	return true;
}

static bool mail_db_txt_loadall(MailDB* self, struct mail_data* md, const int char_id)
{
	MailDB_TXT* db = (MailDB_TXT*)self;
	struct mail_message* msg;
	DBIterator* iter;
	void* data;
	int i;

	memset(md, 0, sizeof(struct mail_data));
	i = 0;

	iter = db->mails->iterator(db->mails);
	for( data = iter->first(iter, NULL); iter->exists(iter); data = iter->next(iter, NULL) )
	{
		msg = (struct mail_message*)data;

		if( msg->dest_id != char_id )
			continue;

		if( md->amount < MAIL_MAX_INBOX )
		{
			memcpy(&md->msg[md->amount], msg, sizeof(struct mail_message));
			md->amount++;
		}

		++i; // total message count
	}
	iter->destroy(iter);

	md->full = ( i > MAIL_MAX_INBOX );

	// process retrieved data
	md->unchecked = 0;
	md->unread = 0;
	for( i = 0; i < md->amount; ++i )
	{
		msg = &md->msg[i];

		if( msg->status == MAIL_NEW )
		{// change to 'unread'
			msg->status = MAIL_UNREAD;
			md->unchecked++;
		}
		else
		if( msg->status == MAIL_UNREAD )
			md->unread++;
	}

	return true;
}

static bool mmo_mail_fromstr(struct mail_message* msg, const char* str)
{
	char esc_title[MAIL_TITLE_LENGTH*2+1];
	char esc_body[MAIL_BODY_LENGTH*2+1];
	int len, n;
	int i;

	if( sscanf(str, "%u,%d,%[^\t]\t%d,%[^\t]\t%[^\t]\t%[^\t]\t%u,%lu,%d," "%d,%d,%d,%u,%u,%u,%u%n",
	    &msg->id, &msg->send_id, msg->send_name, &msg->dest_id, msg->dest_name,
		esc_title, esc_body, &msg->status, (unsigned long*)&msg->timestamp, &msg->zeny,
		&msg->item.id, &msg->item.nameid, &msg->item.amount, &msg->item.equip, &msg->item.identify, &msg->item.refine, &msg->item.attribute, &len) != 17 )
		return false;

	for( i = 0; i < MAX_SLOTS; i++ )
	{
		if( sscanf(str + len, ",%d%n", &msg->item.card[i], &n) != 1 )
			return false;

		len += n;
	}

	sv_unescape_c(msg->title, esc_title, strlen(esc_title));
	sv_unescape_c(msg->body, esc_body, strlen(esc_body));

	return true;
}

static bool mmo_mail_tostr(const struct mail_message* msg, char* str)
{
	char esc_title[MAIL_TITLE_LENGTH*2+1];
	char esc_body[MAIL_BODY_LENGTH*2+1];
	int len;
	int i;

	sv_escape_c(esc_title, msg->title, strlen(msg->title), NULL);
	sv_escape_c(esc_body, msg->body, strlen(msg->body), NULL);

	len = sprintf(str, "%u,%d,%s\t%d,%s\t%s\t%s\t%u,%lu,%d," "%d,%d,%d,%u,%u,%u,%u",
	              msg->id, msg->send_id, msg->send_name, msg->dest_id, msg->dest_name,
	              esc_title, esc_body, msg->status, (unsigned long)msg->timestamp, msg->zeny,
	              msg->item.id, msg->item.nameid, msg->item.amount, msg->item.equip, msg->item.identify, msg->item.refine, msg->item.attribute);

	for( i = 0; i < MAX_SLOTS; i++ )
		len += sprintf(str+len, ",%d", msg->item.card[i]);

	strcat(str+len, "\t");

	return true;
}

static bool mmo_mail_sync(MailDB_TXT* db)
{
	DBIterator* iter;
	void* data;
	FILE *fp;
	int lock;

	fp = lock_fopen(db->mail_db, &lock);
	if( fp == NULL )
	{
		ShowError("mmo_mail_sync: can't write [%s] !!! data is lost !!!\n", db->mail_db);
		return false;
	}

	iter = db->mails->iterator(db->mails);
	for( data = iter->first(iter,NULL); iter->exists(iter); data = iter->next(iter,NULL) )
	{
		struct mail_message* msg = (struct mail_message*) data;
		char line[8192];

		mmo_mail_tostr(msg, line);
		fprintf(fp, "%s\n", line);
	}
	iter->destroy(iter);

	lock_fclose(fp, db->mail_db, &lock);

	return true;
}
