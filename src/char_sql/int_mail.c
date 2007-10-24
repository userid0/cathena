// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/mmo.h"
#include "../common/malloc.h"
#include "../common/showmsg.h"
#include "../common/socket.h"
#include "../common/strlib.h"
#include "../common/sql.h"
#include "char.h"
#include "inter.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static time_t calc_times(void)
{
	time_t temp = time(NULL);
	return mktime(localtime(&temp));
}


static int mail_fromsql(int char_id, struct mail_data* md)
{
	int i, j;
	struct mail_message *msg;
	struct item *item;
	char *data;
	StringBuf buf;

	memset(md, 0, sizeof(struct mail_data));
	md->amount = 0;
	md->full = false;

	StringBuf_Init(&buf);
	StringBuf_AppendStr(&buf, "SELECT `id`,`send_name`,`send_id`,`dest_name`,`dest_id`,`title`,`message`,`time`,`read_flag`,"
		"`zeny`,`amount`,`nameid`,`refine`,`attribute`,`identify`");
	for (i = 0; i < MAX_SLOTS; i++)
		StringBuf_Printf(&buf, ",`card%d`", i);
	StringBuf_Printf(&buf, " FROM `%s` WHERE `dest_id`='%d' ORDER BY `id` LIMIT %d", mail_db, char_id, MAIL_MAX_INBOX + 1);

	if( SQL_ERROR == Sql_Query(sql_handle, StringBuf_Value(&buf)) )
		Sql_ShowDebug(sql_handle);

	StringBuf_Destroy(&buf);

	for (i = 0; i < MAIL_MAX_INBOX && SQL_SUCCESS == Sql_NextRow(sql_handle); ++i )
	{
		msg = &md->msg[i];
		Sql_GetData(sql_handle, 0, &data, NULL); msg->id = atoi(data);
		Sql_GetData(sql_handle, 1, &data, NULL); safestrncpy(msg->send_name, data, NAME_LENGTH);
		Sql_GetData(sql_handle, 2, &data, NULL); msg->send_id = atoi(data);
		Sql_GetData(sql_handle, 3, &data, NULL); safestrncpy(msg->dest_name, data, NAME_LENGTH);
		Sql_GetData(sql_handle, 4, &data, NULL); msg->dest_id = atoi(data);
		Sql_GetData(sql_handle, 5, &data, NULL); safestrncpy(msg->title, data, MAIL_TITLE_LENGTH);
		Sql_GetData(sql_handle, 6, &data, NULL); safestrncpy(msg->body, data, MAIL_BODY_LENGTH);
		Sql_GetData(sql_handle, 7, &data, NULL); msg->timestamp = atoi(data);
		Sql_GetData(sql_handle, 8, &data, NULL); msg->read = atoi(data);
		Sql_GetData(sql_handle, 9, &data, NULL); msg->zeny = atoi(data);
		item = &msg->item;
		Sql_GetData(sql_handle,10, &data, NULL); item->amount = (short)atoi(data);
		Sql_GetData(sql_handle,11, &data, NULL); item->nameid = atoi(data);
		Sql_GetData(sql_handle,12, &data, NULL); item->refine = atoi(data);
		Sql_GetData(sql_handle,13, &data, NULL); item->attribute = atoi(data);
		Sql_GetData(sql_handle,14, &data, NULL); item->identify = atoi(data);

		for (j = 0; j < MAX_SLOTS; j++)
		{
			Sql_GetData(sql_handle, 15 + j, &data, NULL);
			item->card[j] = atoi(data);
		}
	}

	md->full = ( Sql_NumRows(sql_handle) > MAIL_MAX_INBOX );

	md->amount = i;
	md->changed = false;
	Sql_FreeResult(sql_handle);

	md->unchecked = 0;
	md->unread = 0;
	for (i = 0; i < md->amount; i++)
	{
		msg = &md->msg[i];
		if( msg->read == 0 )
		{
			if ( SQL_ERROR == Sql_Query(sql_handle, "UPDATE `%s` SET `read_flag` = '1' WHERE `id` = '%d'", mail_db, msg->id) )
				Sql_ShowDebug(sql_handle);

			md->unchecked++;
		}
		else if ( msg->read == 1 )
			md->unread++;

		msg->read = (msg->read < 2)?0:1;
	}

	ShowInfo("mail load complete from DB - id: %d (total: %d)\n", char_id, md->amount);
	return 1;
}

/// Stores a single message in the database.
/// Returns the message's ID if successful (or 0 if it fails).
static int mail_savemessage(struct mail_message* msg)
{
	StringBuf buf;
	SqlStmt* stmt;
	int j;

	// build message save query
	StringBuf_Init(&buf);
	StringBuf_Printf(&buf, "INSERT INTO `%s` (`send_name`, `send_id`, `dest_name`, `dest_id`, `title`, `message`, `time`, `read_flag`, `zeny`, `amount`, `nameid`, `refine`, `attribute`, `identify`", mail_db);
	for (j = 0; j < MAX_SLOTS; j++)
		StringBuf_Printf(&buf, ", `card%d`", j);
	StringBuf_Printf(&buf, ") VALUES (?, '%d', ?, '%d', ?, ?, '%d', '0', '%d', '%d', '%d', '%d', '%d', '%d'",
		msg->send_id, msg->dest_id, msg->timestamp, msg->zeny, msg->item.amount, msg->item.nameid, msg->item.refine, msg->item.attribute, msg->item.identify);
	for (j = 0; j < MAX_SLOTS; j++)
		StringBuf_Printf(&buf, ", '%d'", msg->item.card[j]);
	StringBuf_AppendStr(&buf, ")");

	// prepare and execute query
	stmt = SqlStmt_Malloc(sql_handle);
	if( SQL_SUCCESS != SqlStmt_PrepareStr(stmt, StringBuf_Value(&buf))
	||  SQL_SUCCESS != SqlStmt_BindParam(stmt, 0, SQLDT_STRING, msg->send_name, strnlen(msg->send_name, NAME_LENGTH))
	||  SQL_SUCCESS != SqlStmt_BindParam(stmt, 1, SQLDT_STRING, msg->dest_name, strnlen(msg->dest_name, NAME_LENGTH))
	||  SQL_SUCCESS != SqlStmt_BindParam(stmt, 2, SQLDT_STRING, msg->title, strnlen(msg->title, MAIL_TITLE_LENGTH))
	||  SQL_SUCCESS != SqlStmt_BindParam(stmt, 3, SQLDT_STRING, msg->body, strnlen(msg->body, MAIL_BODY_LENGTH))
	||  SQL_SUCCESS != SqlStmt_Execute(stmt) )
	{
		Sql_ShowDebug(sql_handle);
		j = 0;
	} else
		j = (int)Sql_LastInsertId(sql_handle);

	StringBuf_Destroy(&buf);

	// return the ID of the new mail
	return j;
}

/// Retrieves a single message from the database.
/// Returns true if the operation succeeds (or false if it fails).
static bool mail_loadmessage(int char_id, int mail_id, struct mail_message* msg)
{
	int j;
	StringBuf buf;

	StringBuf_Init(&buf);
	StringBuf_AppendStr(&buf, "SELECT `id`,`send_name`,`send_id`,`dest_name`,`dest_id`,`title`,`message`,`time`,`read_flag`,"
		"`zeny`,`amount`,`nameid`,`refine`,`attribute`,`identify`");
	for( j = 0; j < MAX_SLOTS; j++ )
		StringBuf_Printf(&buf, ",`card%d`", j);
	StringBuf_Printf(&buf, " FROM `%s` WHERE `dest_id` = '%d' AND `id` = '%d'", mail_db, char_id, mail_id);

	if( SQL_ERROR == Sql_Query(sql_handle, StringBuf_Value(&buf))
	||  SQL_SUCCESS != Sql_NextRow(sql_handle) )
	{
		Sql_ShowDebug(sql_handle);
		Sql_FreeResult(sql_handle);
		StringBuf_Destroy(&buf);
		return false;
	}
	else
	{
		char* data;

		Sql_GetData(sql_handle, 0, &data, NULL); msg->id = atoi(data);
		Sql_GetData(sql_handle, 1, &data, NULL); safestrncpy(msg->send_name, data, NAME_LENGTH);
		Sql_GetData(sql_handle, 2, &data, NULL); msg->send_id = atoi(data);
		Sql_GetData(sql_handle, 3, &data, NULL); safestrncpy(msg->dest_name, data, NAME_LENGTH);
		Sql_GetData(sql_handle, 4, &data, NULL); msg->dest_id = atoi(data);
		Sql_GetData(sql_handle, 5, &data, NULL); safestrncpy(msg->title, data, MAIL_TITLE_LENGTH);
		Sql_GetData(sql_handle, 6, &data, NULL); safestrncpy(msg->body, data, MAIL_BODY_LENGTH);
		Sql_GetData(sql_handle, 7, &data, NULL); msg->timestamp = atoi(data);
		Sql_GetData(sql_handle, 8, &data, NULL); msg->read = atoi(data);
		Sql_GetData(sql_handle, 9, &data, NULL); msg->zeny = atoi(data);
		Sql_GetData(sql_handle,10, &data, NULL); msg->item.amount = (short)atoi(data);
		Sql_GetData(sql_handle,11, &data, NULL); msg->item.nameid = atoi(data);
		Sql_GetData(sql_handle,12, &data, NULL); msg->item.refine = atoi(data);
		Sql_GetData(sql_handle,13, &data, NULL); msg->item.attribute = atoi(data);
		Sql_GetData(sql_handle,14, &data, NULL); msg->item.identify = atoi(data);
		for( j = 0; j < MAX_SLOTS; j++ )
		{
			Sql_GetData(sql_handle,15 + j, &data, NULL);
			msg->item.card[j] = atoi(data);
		}
	}

	StringBuf_Destroy(&buf);
	Sql_FreeResult(sql_handle);

	ShowDebug("Loaded message (had read flag %d)\n", msg->read);
	if (msg->read == 1)
	{
		msg->read = 0;
	}
	else
		msg->read = 1;

	return true;
}

/*==========================================
 * Client Inbox Request
 *------------------------------------------*/
static void mapif_Mail_sendinbox(int fd, int char_id, unsigned char flag)
{
	struct mail_data md;
	mail_fromsql(char_id, &md);

	//FIXME: dumping the whole structure like this is unsafe [ultramage]
	WFIFOHEAD(fd, sizeof(md) + 9);
	WFIFOW(fd,0) = 0x3848;
	WFIFOW(fd,2) = sizeof(md) + 9;
	WFIFOL(fd,4) = char_id;
	WFIFOB(fd,8) = flag;
	memcpy(WFIFOP(fd,9),&md,sizeof(md));
	WFIFOSET(fd,WFIFOW(fd,2));
}

static void mapif_parse_Mail_requestinbox(int fd)
{
	mapif_Mail_sendinbox(fd, RFIFOL(fd,2), RFIFOB(fd,6));
}

/*==========================================
 * 'Mail read' Mark
 *------------------------------------------*/
static void mapif_parse_Mail_read(int fd)
{
	int mail_id = RFIFOL(fd,2);
	if( SQL_ERROR == Sql_Query(sql_handle, "UPDATE `%s` SET `read_flag` = '2' WHERE `id` = '%d'", mail_db, mail_id) )
		Sql_ShowDebug(sql_handle);
}

/*==========================================
 * Client Attachment Request
 *------------------------------------------*/
static bool mail_DeleteAttach(int mail_id)
{
	StringBuf buf;
	int i;

	StringBuf_Init(&buf);
	StringBuf_Printf(&buf, "UPDATE `%s` SET `zeny` = '0', `nameid` = '0', `amount` = '0', `refine` = '0', `attribute` = '0', `identify` = '0'", mail_db);
	for (i = 0; i < MAX_SLOTS; i++)
		StringBuf_Printf(&buf, ", `card%d` = '0'", i);
	StringBuf_Printf(&buf, " WHERE `id` = '%d'", mail_id);

	if( SQL_ERROR == Sql_Query(sql_handle, StringBuf_Value(&buf)) )
	{
		Sql_ShowDebug(sql_handle);
		StringBuf_Destroy(&buf);

		return false;
	}

	StringBuf_Destroy(&buf);
	return true;
}

static void mapif_Mail_getattach(int fd, int char_id, int mail_id)
{
	struct mail_message msg;

	if( !mail_loadmessage(char_id, mail_id, &msg) )
		return;

	if( (msg.item.nameid < 1 || msg.item.amount < 1) && msg.zeny < 1 )
		return; // No Attachment

	if( !mail_DeleteAttach(mail_id) )
		return;

	WFIFOHEAD(fd, sizeof(struct item) + 12);
	WFIFOW(fd,0) = 0x384a;
	WFIFOW(fd,2) = sizeof(struct item) + 12;
	WFIFOL(fd,4) = char_id;
	WFIFOL(fd,8) = (msg.zeny > 0)?msg.zeny:0;
	memcpy(WFIFOP(fd,12), &msg.item, sizeof(struct item));
	WFIFOSET(fd,WFIFOW(fd,2));
}

static void mapif_parse_Mail_getattach(int fd)
{
	mapif_Mail_getattach(fd, RFIFOL(fd,2), RFIFOL(fd,6));
}

/*==========================================
 * Delete Mail
 *------------------------------------------*/
static void mapif_Mail_delete(int fd, int char_id, int mail_id)
{
	bool failed = false;
	if ( SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `%s` WHERE `id` = '%d'", mail_db, mail_id) )
	{
		Sql_ShowDebug(sql_handle);
		failed = true;
	}

	WFIFOHEAD(fd,11);
	WFIFOW(fd,0) = 0x384b;
	WFIFOL(fd,2) = char_id;
	WFIFOL(fd,6) = mail_id;
	WFIFOB(fd,10) = failed;
	WFIFOSET(fd,11);
}

static void mapif_parse_Mail_delete(int fd)
{
	mapif_Mail_delete(fd, RFIFOL(fd,2), RFIFOL(fd,6));
}

/*==========================================
 * Return Mail
 *------------------------------------------*/
static void mapif_Mail_return(int fd, int char_id, int mail_id)
{
	struct mail_message msg;
	int new_mail = 0;

	if( mail_loadmessage(char_id, mail_id, &msg) )
	{
		if( SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `%s` WHERE `id` = '%d'", mail_db, mail_id) )
			Sql_ShowDebug(sql_handle);
		else
		{
			char temp_[MAIL_TITLE_LENGTH];

			// swap sender and receiver
			swap(msg.send_id, msg.dest_id);
			safestrncpy(temp_, msg.send_name, NAME_LENGTH);
			safestrncpy(msg.send_name, msg.dest_name, NAME_LENGTH);
			safestrncpy(msg.dest_name, temp_, NAME_LENGTH);

			// set reply message title
			snprintf(temp_, MAIL_TITLE_LENGTH, "RE:%s", msg.title);
			safestrncpy(msg.title, temp_, MAIL_TITLE_LENGTH);

			msg.timestamp = (unsigned int)calc_times();

			new_mail = mail_savemessage(&msg);
		}
	}

	WFIFOHEAD(fd,14);
	WFIFOW(fd,0) = 0x384c;
	WFIFOL(fd,2) = char_id;
	WFIFOL(fd,6) = mail_id;
	WFIFOL(fd,10) = new_mail;
	WFIFOSET(fd,14);
}

static void mapif_parse_Mail_return(int fd)
{
	mapif_Mail_return(fd, RFIFOL(fd,2), RFIFOL(fd,6));
}

static void mapif_Mail_send(int fd, struct mail_message* msg)
{
	int len = strlen(msg->title);

	WFIFOHEAD(fd,len);
	WFIFOW(fd,0) = 0x384d;
	WFIFOW(fd,2) = len + 16;
	WFIFOL(fd,4) = msg->send_id;
	WFIFOL(fd,8) = msg->id;
	WFIFOL(fd,12) = msg->dest_id;
	safestrncpy((char*)WFIFOP(fd,16), msg->title, len);
	WFIFOSET(fd,WFIFOW(fd,2));
}

static void mapif_parse_Mail_send(int fd)
{
	struct mail_message msg;
	int mail_id = 0, account_id = 0;

	if(RFIFOW(fd,2) != 8 + sizeof(struct mail_message))
		return;

	memcpy(&msg, RFIFOP(fd,8), sizeof(struct mail_message));

	account_id = RFIFOL(fd,4);

	if( !msg.dest_id )
	{
		// Try to find the Dest Char by Name
		char esc_name[NAME_LENGTH*2+1];

		Sql_EscapeStringLen(sql_handle, esc_name, msg.dest_name, strnlen(msg.dest_name, NAME_LENGTH));
		if ( SQL_ERROR == Sql_Query(sql_handle, "SELECT `account_id`, `char_id` FROM `%s` WHERE `name` = '%s'", char_db, esc_name) )
			Sql_ShowDebug(sql_handle);
		else
		if ( SQL_SUCCESS == Sql_NextRow(sql_handle) )
		{
			char *data;
			Sql_GetData(sql_handle, 0, &data, NULL);
			if (atoi(data) != account_id)
			{ // Cannot send mail to char in the same account
				Sql_GetData(sql_handle, 1, &data, NULL);
				msg.dest_id = atoi(data);
			}
		}
		Sql_FreeResult(sql_handle);
	}

	if( msg.dest_id > 0 )
		mail_id = mail_savemessage(&msg);

	msg.id = mail_id;
	mapif_Mail_send(fd, &msg);
}

/*==========================================
 * Packets From Map Server
 *------------------------------------------*/
int inter_mail_parse_frommap(int fd)
{
	switch(RFIFOW(fd,0))
	{
		case 0x3048: mapif_parse_Mail_requestinbox(fd); break;
		case 0x3049: mapif_parse_Mail_read(fd); break;
		case 0x304a: mapif_parse_Mail_getattach(fd); break;
		case 0x304b: mapif_parse_Mail_delete(fd); break;
		case 0x304c: mapif_parse_Mail_return(fd); break;
		case 0x304d: mapif_parse_Mail_send(fd); break;
		default:
			return 0;
	}
	return 1;
}

int inter_mail_sql_init(void)
{
	return 1;
}

void inter_mail_sql_final(void)
{
	return;
}
