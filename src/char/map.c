// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/db.h"
#include "../common/malloc.h"
#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/socket.h"
#include "../common/strlib.h"
#include "char.h"
#include "chardb.h"
#include "charlog.h"
#include "int_status.h"
#include "int_storage.h"
#include "inter.h"
#include "map.h"
#include <string.h>

//temporary imports
extern void set_char_online(int map_id, int char_id, int account_id);
extern void set_char_offline(int char_id, int account_id);
#include "char.h"
extern struct mmo_charstatus *char_dat;
extern int char_num, char_max;
extern int login_fd;
struct online_char_data {
	int account_id;
	int char_id;
	int fd;
	int waiting_disconnect;
	short server; // -2: unknown server, -1: not connected, 0+: id of server
};
extern DBMap* online_char_db; // int account_id -> struct online_char_data*
struct auth_node {
	int account_id;
	int char_id;
	uint32 login_id1;
	uint32 login_id2;
	uint32 ip;
	int sex;
	time_t expiration_time; // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
	int gmlevel;
};
extern DBMap* auth_db; // int account_id -> struct auth_node*
extern int char_db_setoffline(DBKey key, void* data, va_list ap);
extern int char_send_fame_list(int fd);  
extern void* create_online_char_data(DBKey key, va_list args);
extern int online_check;
//Custom limits for the fame lists. [Skotlex]
extern int fame_list_size_chemist;
extern int fame_list_size_smith;
extern int fame_list_size_taekwon;
// Char-server-side stored fame lists [DracoRPG]
extern struct fame_list smith_fame_list[MAX_FAME_LIST];
extern struct fame_list chemist_fame_list[MAX_FAME_LIST];
extern struct fame_list taekwon_fame_list[MAX_FAME_LIST];
extern void char_update_fame_list(int type, int index, int fame);
extern void set_all_offline(int id);
extern void char_read_fame_list(void);
extern int mmo_char_tosql(int char_id, struct mmo_charstatus* p);
extern int mmo_char_fromsql(int char_id, struct mmo_charstatus* p, bool load_everything);
extern DBMap* char_db_;
extern int divorce_char_sql(int partner_id1, int partner_id2);



struct mmo_map_server server[MAX_MAP_SERVERS];


// Searches for the mapserver that has a given map (and optionally ip/port, if not -1).
// If found, returns the server's index in the 'server' array (otherwise returns -1).
int search_mapserver(unsigned short map, uint32 ip, uint16 port)
{
	int i, j;
	
	for(i = 0; i < MAX_MAP_SERVERS; i++)
	{
		if (server[i].fd > 0
		&& (ip == (uint32)-1 || server[i].ip == ip)
		&& (port == (uint16)-1 || server[i].port == port))
		{
			for (j = 0; server[i].map[j]; j++)
				if (server[i].map[j] == map)
					return i;
		}
	}

	return -1;
}


int parse_frommap(int fd)
{
	int i, j;
	int id;

	ARR_FIND( 0, MAX_MAP_SERVERS, id, server[id].fd == fd );
	if(id == MAX_MAP_SERVERS)
		set_eof(fd);
	if(session[fd]->flag.eof) {
		if (id < MAX_MAP_SERVERS) {
			unsigned char buf[16384];
			ShowStatus("Map-server %d (session #%d) has disconnected.\n", id, fd);
			//Notify other map servers that this one is gone. [Skotlex]
			WBUFW(buf,0) = 0x2b20;
			WBUFL(buf,4) = htonl(server[id].ip);
			WBUFW(buf,8) = htons(server[id].port);
			j = 0;
			for(i = 0; i < MAX_MAP_PER_SERVER; i++)
				if (server[id].map[i])
					WBUFW(buf,10+(j++)*4) = server[id].map[i];
			if (j > 0) {
				WBUFW(buf,2) = j * 4 + 10;
				mapif_sendallwos(fd, buf, WBUFW(buf,2));
			}
#ifndef TXT_ONLY
			memset(&server[id], 0, sizeof(struct mmo_map_server));
			if( SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `ragsrvinfo` WHERE `index`='%d'", server[id].fd) )
				Sql_ShowDebug(sql_handle);
#endif
			server[id].fd = -1;
			online_char_db->foreach(online_char_db,char_db_setoffline,id); //Tag relevant chars as 'in disconnected' server.
		}
		do_close(fd);
		/* TODO: move to plugin
		create_online_files();
		*/
		return 0;
	}

	while(RFIFOREST(fd) >= 2)
	{
		//ShowDebug("Received packet 0x%4x (%d bytes) from map-server (connection %d)\n", RFIFOW(fd, 0), RFIFOREST(fd), fd);

		switch(RFIFOW(fd,0))
		{

		case 0x2afa: // Receiving map names list from the map-server
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;

			memset(server[id].map, 0, sizeof(server[id].map));
			j = 0;
			for(i = 4; i < RFIFOW(fd,2); i += 4) {
				server[id].map[j] = RFIFOW(fd,i);
				j++;
			}

			ShowStatus("Map-Server %d connected: %d maps, from IP %d.%d.%d.%d port %d.\n",
						id, j, CONVIP(server[id].ip), server[id].port);
			ShowStatus("Map-server %d loading complete.\n", id);
#ifdef TXT_ONLY
			char_log("Map-Server %d connected: %d maps, from IP %d.%d.%d.%d port %d. Map-server %d loading complete.\n",
						id, j, CONVIP(server[id].ip), server[id].port, id);
#endif

			// send name for wisp to player
			WFIFOHEAD(fd, 3 + NAME_LENGTH);
			WFIFOW(fd,0) = 0x2afb;
			WFIFOB(fd,2) = 0;
			memcpy(WFIFOP(fd,3), wisp_server_name, NAME_LENGTH);
			WFIFOSET(fd,3+NAME_LENGTH);

			char_send_fame_list(fd); //Send fame list.

			{
			unsigned char buf[16384];
			int x;
			if (j == 0) {
				ShowWarning("Map-server %d has NO maps.\n", id);
#ifdef TXT_ONLY
				char_log("WARNING: Map-server %d has NO maps.\n", id);
#endif
			} else {
				// Transmitting maps information to the other map-servers
				WBUFW(buf,0) = 0x2b04;
				WBUFW(buf,2) = j * 4 + 10;
				WBUFL(buf,4) = htonl(server[id].ip);
				WBUFW(buf,8) = htons(server[id].port);
				memcpy(WBUFP(buf,10), RFIFOP(fd,4), j * 4);
				mapif_sendallwos(fd, buf, WBUFW(buf,2));
			}
			// Transmitting the maps of the other map-servers to the new map-server
			for(x = 0; x < MAX_MAP_SERVERS; x++) {
				if (server[x].fd > 0 && x != id) {
					WFIFOHEAD(fd,10 +4*MAX_MAP_PER_SERVER);
					WFIFOW(fd,0) = 0x2b04;
					WFIFOL(fd,4) = htonl(server[x].ip);
					WFIFOW(fd,8) = htons(server[x].port);
					j = 0;
					for(i = 0; i < MAX_MAP_PER_SERVER; i++)
						if (server[x].map[i])
							WFIFOW(fd,10+(j++)*4) = server[x].map[i];
					if (j > 0) {
						WFIFOW(fd,2) = j * 4 + 10;
						WFIFOSET(fd,WFIFOW(fd,2));
					}
				}
			}
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
		break;

		case 0x2afc: //Packet command is now used for sc_data request. [Skotlex]
			if (RFIFOREST(fd) < 10)
				return 0;
		{
#ifdef ENABLE_SC_SAVING
#ifdef TXT_ONLY
			int aid, cid;
			struct scdata *data;
			aid = RFIFOL(fd,2);
			cid = RFIFOL(fd,6);
			data = status_search_scdata(aid, cid);
			if (data->count > 0)
			{	//Deliver status change data.
				WFIFOW(fd,0) = 0x2b1d;
				WFIFOW(fd,2) = 14 + data->count*sizeof(struct status_change_data);
				WFIFOL(fd,4) = aid;
				WFIFOL(fd,8) = cid;
				WFIFOW(fd,12) = data->count;
				for (i = 0; i < data->count; i++)
					memcpy(WFIFOP(fd,14+i*sizeof(struct status_change_data)), &data->data[i], sizeof(struct status_change_data));
				WFIFOSET(fd, WFIFOW(fd,2));
				status_delete_scdata(aid, cid); //Data sent, so it needs be discarded now.
			}
#else
			int aid, cid;
			aid = RFIFOL(fd,2);
			cid = RFIFOL(fd,6);
			if( SQL_ERROR == Sql_Query(sql_handle, "SELECT type, tick, val1, val2, val3, val4 from `%s` WHERE `account_id` = '%d' AND `char_id`='%d'",
				scdata_db, aid, cid) )
			{
				Sql_ShowDebug(sql_handle);
				break;
			}
			if( Sql_NumRows(sql_handle) > 0 )
			{
				struct status_change_data scdata;
				int count;
				char* data;

				WFIFOHEAD(fd,14+50*sizeof(struct status_change_data));
				WFIFOW(fd,0) = 0x2b1d;
				WFIFOL(fd,4) = aid;
				WFIFOL(fd,8) = cid;
				for( count = 0; count < 50 && SQL_SUCCESS == Sql_NextRow(sql_handle); ++count )
				{
					Sql_GetData(sql_handle, 0, &data, NULL); scdata.type = atoi(data);
					Sql_GetData(sql_handle, 1, &data, NULL); scdata.tick = atoi(data);
					Sql_GetData(sql_handle, 2, &data, NULL); scdata.val1 = atoi(data);
					Sql_GetData(sql_handle, 3, &data, NULL); scdata.val2 = atoi(data);
					Sql_GetData(sql_handle, 4, &data, NULL); scdata.val3 = atoi(data);
					Sql_GetData(sql_handle, 5, &data, NULL); scdata.val4 = atoi(data);
					memcpy(WFIFOP(fd, 14+count*sizeof(struct status_change_data)), &scdata, sizeof(struct status_change_data));
				}
				if (count >= 50)
					ShowWarning("Too many status changes for %d:%d, some of them were not loaded.\n", aid, cid);
				if (count > 0)
				{
					WFIFOW(fd,2) = 14 + count*sizeof(struct status_change_data);
					WFIFOW(fd,12) = count;
					WFIFOSET(fd,WFIFOW(fd,2));

					//Clear the data once loaded.
					if( SQL_ERROR == Sql_Query(sql_handle, "DELETE FROM `%s` WHERE `account_id` = '%d' AND `char_id`='%d'", scdata_db, aid, cid) )
						Sql_ShowDebug(sql_handle);
				}
			}
			Sql_FreeResult(sql_handle);
#endif
#endif
			RFIFOSKIP(fd, 10);
		}
		break;
		
		case 0x2afe: //set MAP user count
			if (RFIFOREST(fd) < 4)
				return 0;
			if (RFIFOW(fd,2) != server[id].users) {
				server[id].users = RFIFOW(fd,2);
				ShowInfo("User Count: %d (Server: %d)\n", server[id].users, id);
			}
			RFIFOSKIP(fd, 4);
			break;

		case 0x2aff: //set MAP users
			if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;

			//TODO: When data mismatches memory, update guild/party online/offline states.
			server[id].users = RFIFOW(fd,4);
			online_char_db->foreach(online_char_db,char_db_setoffline,id); //Set all chars from this server as 'unknown'
			for(i = 0; i < server[id].users; i++)
			{
				struct online_char_data* character;
				int aid = RFIFOL(fd,6+i*8);
				int cid = RFIFOL(fd,6+i*8+4);
				character = (struct online_char_data*)idb_ensure(online_char_db, aid, create_online_char_data);
				if (online_check && character->server > -1 && character->server != id)
				{
					ShowNotice("Set map user: Character (%d:%d) marked on map server %d, but map server %d claims to have (%d:%d) online!\n",
						character->account_id, character->char_id, character->server, id, aid, cid);
					mapif_disconnectplayer(server[character->server].fd, character->account_id, character->char_id, 2);
				}
				character->char_id = cid;
				character->server = id;
			}
			//If any chars remain in -2, they will be cleaned in the cleanup timer.
			RFIFOSKIP(fd,6+i*8);
		break;

		case 0x2b01: // Receive character data from map-server for saving
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		{
			int aid = RFIFOL(fd,4), cid = RFIFOL(fd,8), size = RFIFOW(fd,2);
#ifndef TXT_ONLY
			struct online_char_data* character;
#endif

			// verify data size
			if (size - 13 != sizeof(struct mmo_charstatus))
			{
				ShowError("parse_from_map (save-char): Size mismatch! %d != %d\n", size-13, sizeof(struct mmo_charstatus));
				RFIFOSKIP(fd,size);
				break;
			}

#ifdef TXT_ONLY
			// look up character
			ARR_FIND( 0, char_num, i, char_dat[i].account_id == aid && char_dat[i].char_id == cid );
			if( i < char_num )
			{
				memcpy(&char_dat[i], RFIFOP(fd,13), sizeof(struct mmo_charstatus));
				storage_save(char_dat[i].account_id, &char_dat[i].storage);
			}
#else
			//Check account only if this ain't final save. Final-save goes through because of the char-map reconnect
			if (RFIFOB(fd,12) || (
				(character = (struct online_char_data*)idb_get(online_char_db, aid)) != NULL &&
				character->char_id == cid))
			{
				struct mmo_charstatus char_dat;
				memcpy(&char_dat, RFIFOP(fd,13), sizeof(struct mmo_charstatus));
				mmo_char_tosql(cid, &char_dat);
			} else {	//This may be valid on char-server reconnection, when re-sending characters that already logged off.
				ShowError("parse_from_map (save-char): Received data for non-existant/offline character (%d:%d).\n", aid, cid);
				set_char_online(id, cid, aid);
			}
#endif

			if (RFIFOB(fd,12))
			{// Flag? Set character offline after saving [Skotlex]
				set_char_offline(cid,aid);
				WFIFOHEAD(fd,10);
				WFIFOW(fd,0) = 0x2b21; //Save ack only needed on final save.
				WFIFOL(fd,2) = aid;
				WFIFOL(fd,6) = cid;
				WFIFOSET(fd,10);
			}
		}
			RFIFOSKIP(fd,RFIFOW(fd,2));
		break;

		case 0x2b02: // req char selection
			if( RFIFOREST(fd) < 18 )
				return 0;
		{
			struct auth_node* node;

			int account_id = RFIFOL(fd,2);
			uint32 login_id1 = RFIFOL(fd,6);
			uint32 login_id2 = RFIFOL(fd,10);
			uint32 ip = RFIFOL(fd,14);
			RFIFOSKIP(fd,18);

			// create temporary auth entry
			CREATE(node, struct auth_node, 1);
			node->account_id = account_id;
			node->char_id = 0;
			node->login_id1 = login_id1;
			node->login_id2 = login_id2;
			//node->sex = 0;
			node->ip = ntohl(ip);
			//node->expiration_time = 0; // unlimited/unknown time by default (not display in map-server)
			//node->gmlevel = 0;
			idb_put(auth_db, account_id, node);

			//Set char to "@ char select" in online db [Kevin]
			set_char_online(-3, 99, account_id);

			WFIFOHEAD(fd,7);
			WFIFOW(fd,0) = 0x2b03;
			WFIFOL(fd,2) = account_id;
			WFIFOB(fd,6) = 0;
			WFIFOSET(fd,7);
		}
		break;

		case 0x2b05: // request "change map server"
			if (RFIFOREST(fd) < 35)
				return 0;
		{
			int account_id = RFIFOL(fd,2);
			int login_id1 = RFIFOL(fd,6);
			int login_id2 = RFIFOL(fd,10);
			int char_id = RFIFOL(fd,14);
			unsigned short mapindex = RFIFOW(fd,18);
			short x = RFIFOW(fd,20);
			short y = RFIFOW(fd,22);
			uint32 server_ip = RFIFOL(fd,24);
			uint16 server_port = RFIFOW(fd,28);
			char sex = RFIFOB(fd,30);
			uint32 client_ip = RFIFOL(fd,31);

			int map_id, map_fd = -1;
			struct online_char_data* data;
			struct mmo_charstatus* char_data;

			map_id = search_mapserver(mapindex, ntohl(server_ip), ntohs(server_port)); //Locate mapserver by ip and port.
			if (map_id >= 0)
				map_fd = server[map_id].fd;

#ifdef TXT_ONLY
			ARR_FIND( 0, char_num, i, char_dat[i].account_id == account_id && char_dat[i].char_id == char_id );
			char_data = i < char_num ? &char_dat[i] : NULL;
#else
			//Char should just had been saved before this packet, so this should be safe. [Skotlex]
			char_data = (struct mmo_charstatus*)uidb_get(char_db_,RFIFOL(fd,14));
			if (char_data == NULL) 
			{	//Really shouldn't happen.
				struct mmo_charstatus char_dat;
				mmo_char_fromsql(RFIFOL(fd,14), &char_dat, true);
				char_data = (struct mmo_charstatus*)uidb_get(char_db_,RFIFOL(fd,14));
			}
#endif

			if (map_fd >= 0 && session[map_fd] && char_data) 
			{	//Send the map server the auth of this player.
				struct auth_node* node;

				//Update the "last map" as this is where the player must be spawned on the new map server.
				char_data->last_point.map = mapindex;
				char_data->last_point.x = x;
				char_data->last_point.y = y;
				char_data->sex = sex;

				// create temporary auth entry
				CREATE(node, struct auth_node, 1);
				node->account_id = account_id;
				node->char_id = char_id;
				node->login_id1 = login_id1;
				node->login_id2 = login_id2;
				node->sex = sex;
				//node->expiration_time = 0; // FIXME
				node->ip = ntohl(client_ip);
				idb_put(auth_db, account_id, node);

				data = (struct online_char_data*)idb_ensure(online_char_db, account_id, create_online_char_data);
				data->char_id = char_data->char_id;
				data->server = map_id; //Update server where char is.

				//Reply with an ack.
				WFIFOHEAD(fd,30);
				WFIFOW(fd,0) = 0x2b06;
				memcpy(WFIFOP(fd,2), RFIFOP(fd,2), 28);
				WFIFOSET(fd,30);
			} else { //Reply with nak
				WFIFOHEAD(fd,30);
				WFIFOW(fd,0) = 0x2b06;
				memcpy(WFIFOP(fd,2), RFIFOP(fd,2), 28);
				WFIFOL(fd,6) = 0; //Set login1 to 0.
				WFIFOSET(fd,30);
			}
			RFIFOSKIP(fd,35);
		}
		break;

		case 0x2b08: // char name request
			if (RFIFOREST(fd) < 6)
				return 0;

			WFIFOHEAD(fd,30);
			WFIFOW(fd,0) = 0x2b09;
			WFIFOL(fd,2) = RFIFOL(fd,2);
			char_loadName((int)RFIFOL(fd,2), (char*)WFIFOP(fd,6));
			WFIFOSET(fd,30);

			RFIFOSKIP(fd,6);
		break;

		case 0x2b0c: // Map server send information to change an email of an account -> login-server
			if (RFIFOREST(fd) < 86)
				return 0;
			if (login_fd > 0) { // don't send request if no login-server
				WFIFOHEAD(login_fd,86);
				memcpy(WFIFOP(login_fd,0), RFIFOP(fd,0),86); // 0x2722 <account_id>.L <actual_e-mail>.40B <new_e-mail>.40B
				WFIFOW(login_fd,0) = 0x2722;
				WFIFOSET(login_fd,86);
			}
			RFIFOSKIP(fd, 86);
		break;

		case 0x2b0e: // Request from map-server to change an account's status (will just be forwarded to login server)
			if (RFIFOREST(fd) < 44)
				return 0;
		{
			int result = 0; // 0-login-server request done, 1-player not found, 2-gm level too low, 3-login-server offline
#ifdef TXT_ONLY
			char character_name[NAME_LENGTH];
#else
			char esc_name[NAME_LENGTH*2+1];
#endif

			int acc = RFIFOL(fd,2); // account_id of who ask (-1 if server itself made this request)
			const char* name = (char*)RFIFOP(fd,6); // name of the target character
			int type = RFIFOW(fd,30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban
			short year = RFIFOW(fd,32);
			short month = RFIFOW(fd,34);
			short day = RFIFOW(fd,36);
			short hour = RFIFOW(fd,38);
			short minute = RFIFOW(fd,40);
			short second = RFIFOW(fd,42);
			RFIFOSKIP(fd,44);

#ifdef TXT_ONLY
			safestrncpy(character_name, name, NAME_LENGTH);
			i = search_character_index(character_name);
			if( i < 0 )
			{
				result = 1; // 1-player not found
			}
#else
			Sql_EscapeStringLen(sql_handle, esc_name, name, strnlen(name, NAME_LENGTH));
			if( SQL_ERROR == Sql_Query(sql_handle, "SELECT `account_id`,`name` FROM `%s` WHERE `name` = '%s'", char_db, esc_name) )
				Sql_ShowDebug(sql_handle);
			else
			if( Sql_NumRows(sql_handle) == 0 )
			{
				result = 1; // 1-player not found
			}
			else
			if( SQL_SUCCESS != Sql_NextRow(sql_handle) )
				Sql_ShowDebug(sql_handle);
				//FIXME: set proper result value?
#endif
			else
			{
				char name[NAME_LENGTH];
				int account_id;

#ifdef TXT_ONLY
				account_id = char_dat[i].account_id;
				safestrncpy(name, char_dat[i].name, NAME_LENGTH);
#else
				char* data;

				Sql_GetData(sql_handle, 0, &data, NULL); account_id = atoi(data);
				Sql_GetData(sql_handle, 1, &data, NULL); safestrncpy(name, data, sizeof(name));
#endif

				if( login_fd <= 0 )
					result = 3; // 3-login-server offline
				//FIXME: need to move this check to login server [ultramage]
//				else
//				if( acc != -1 && isGM(acc) < isGM(account_id) )
//					result = 2; // 2-gm level too low
				else
				switch( type ) {
				case 1: // block
						WFIFOHEAD(login_fd,10);
						WFIFOW(login_fd,0) = 0x2724;
						WFIFOL(login_fd,2) = account_id;
						WFIFOL(login_fd,6) = 5; // new account status
						WFIFOSET(login_fd,10);
				break;
				case 2: // ban
						WFIFOHEAD(login_fd,18);
						WFIFOW(login_fd, 0) = 0x2725;
						WFIFOL(login_fd, 2) = account_id;
						WFIFOW(login_fd, 6) = year;
						WFIFOW(login_fd, 8) = month;
						WFIFOW(login_fd,10) = day;
						WFIFOW(login_fd,12) = hour;
						WFIFOW(login_fd,14) = minute;
						WFIFOW(login_fd,16) = second;
						WFIFOSET(login_fd,18);
				break;
				case 3: // unblock
						WFIFOHEAD(login_fd,10);
						WFIFOW(login_fd,0) = 0x2724;
						WFIFOL(login_fd,2) = account_id;
						WFIFOL(login_fd,6) = 0; // new account status
						WFIFOSET(login_fd,10);
				break;
				case 4: // unban
						WFIFOHEAD(login_fd,6);
						WFIFOW(login_fd,0) = 0x272a;
						WFIFOL(login_fd,2) = account_id;
						WFIFOSET(login_fd,6);
				break;
				case 5: // changesex
						WFIFOHEAD(login_fd,6);
						WFIFOW(login_fd,0) = 0x2727;
						WFIFOL(login_fd,2) = account_id;
						WFIFOSET(login_fd,6);
				break;
				}
			}
#ifndef TXT_ONLY
			Sql_FreeResult(sql_handle);
#endif
			// send answer if a player ask, not if the server ask
			if( acc != -1 && type != 5) { // Don't send answer for changesex
				WFIFOHEAD(fd,34);
				WFIFOW(fd, 0) = 0x2b0f;
				WFIFOL(fd, 2) = acc;
				safestrncpy((char*)WFIFOP(fd,6), name, NAME_LENGTH);
				WFIFOW(fd,30) = type;
				WFIFOW(fd,32) = result;
				WFIFOSET(fd,34);
			}
		}
		break;

		case 0x2b10: // Update and send fame ranking list
			if (RFIFOREST(fd) < 11)
				return 0;
		{
			int cid = RFIFOL(fd, 2);
			int fame = RFIFOL(fd, 6);
			char type = RFIFOB(fd, 10);
			int size;
			struct fame_list* list;
			int player_pos;
			int fame_pos;

			switch(type)
			{
				case 1:  size = fame_list_size_smith;   list = smith_fame_list;   break;
				case 2:  size = fame_list_size_chemist; list = chemist_fame_list; break;
				case 3:  size = fame_list_size_taekwon; list = taekwon_fame_list; break;
				default: size = 0;                      list = NULL;              break;
			}

			ARR_FIND(0, size, player_pos, list[player_pos].id == cid);// position of the player
			ARR_FIND(0, size, fame_pos, list[fame_pos].fame <= fame);// where the player should be

			if( player_pos == size && fame_pos == size )
				;// not on list and not enough fame to get on it
			else if( fame_pos == player_pos )
			{// same position
				list[player_pos].fame = fame;
				char_update_fame_list(type, player_pos, fame);
			}
			else
			{// move in the list
				if( player_pos == size )
				{// new ranker - not in the list
					ARR_MOVE(size - 1, fame_pos, list, struct fame_list);
					list[fame_pos].id = cid;
					list[fame_pos].fame = fame;
					char_loadName(cid, list[fame_pos].name);
				}
				else
				{// already in the list
					if( fame_pos == size )
						--fame_pos;// move to the end of the list
					ARR_MOVE(player_pos, fame_pos, list, struct fame_list);
					list[fame_pos].fame = fame;
				}
				char_send_fame_list(-1);
			}

			RFIFOSKIP(fd,11);
		}
		break;

		// Divorce chars
		case 0x2b11:
			if( RFIFOREST(fd) < 10 )
				return 0;

#ifdef TXT_ONLY
			//TODO!
#else
			divorce_char_sql(RFIFOL(fd,2), RFIFOL(fd,6));
#endif
			RFIFOSKIP(fd,10);
		break;

		case 0x2b16: // Receive rates [Wizputer]
			if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd,8))
				return 0;
#ifndef TXT_ONLY
		{
			char motd[256];
			char esc_motd[sizeof(motd)*2+1];
			char esc_server_name[sizeof(server_name)*2+1];

			strncpy(motd, (char*)RFIFOP(fd,10), 255); //First copy it to make sure the motd fits.
			motd[255] = '\0';
			Sql_EscapeString(sql_handle, esc_motd, motd);
			Sql_EscapeString(sql_handle, esc_server_name, server_name);

			if( SQL_ERROR == Sql_Query(sql_handle, "INSERT INTO `ragsrvinfo` SET `index`='%d',`name`='%s',`exp`='%d',`jexp`='%d',`drop`='%d',`motd`='%s'",
				fd, esc_server_name, RFIFOW(fd,2), RFIFOW(fd,4), RFIFOW(fd,6), esc_motd) )
				Sql_ShowDebug(sql_handle);
		}
#endif
			RFIFOSKIP(fd,RFIFOW(fd,8));
		break;

		case 0x2b17: // Character disconnected set online 0 [Wizputer]
			if (RFIFOREST(fd) < 6)
				return 0;
			set_char_offline(RFIFOL(fd,2),RFIFOL(fd,6));
			RFIFOSKIP(fd,10);
		break;

		case 0x2b18: // Reset all chars to offline [Wizputer]
			set_all_offline(id);
			RFIFOSKIP(fd,2);
		break;
		
		case 0x2b19: // Character set online [Wizputer]
			if (RFIFOREST(fd) < 10)
				return 0;
			set_char_online(id, RFIFOL(fd,2),RFIFOL(fd,6));
			RFIFOSKIP(fd,10);
		break;

		case 0x2b1a: // Build and send fame ranking lists [DracoRPG]
			if (RFIFOREST(fd) < 2)
				return 0;
			char_read_fame_list();
			char_send_fame_list(-1);
			RFIFOSKIP(fd,2);
		break;

		case 0x2b1c: //Request to save status change data. [Skotlex]
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		{
#ifdef ENABLE_SC_SAVING
			int aid = RFIFOL(fd,4);
			int cid = RFIFOL(fd,8);
			int count = RFIFOW(fd,12);

#ifdef TXT_ONLY
			struct scdata* data = status_search_scdata(aid, cid);
			if (data->count != count)
			{
				data->count = count;
				data->data = (struct status_change_data*)aRealloc(data->data, count*sizeof(struct status_change_data));
			}
			for (i = 0; i < count; i++)
				memcpy (&data->data[i], RFIFOP(fd, 14+i*sizeof(struct status_change_data)), sizeof(struct status_change_data));
#else //SQL
			if( count > 0 )
			{
				struct status_change_data data;
				StringBuf buf;
				int i;

				StringBuf_Init(&buf);
				StringBuf_Printf(&buf, "INSERT INTO `%s` (`account_id`, `char_id`, `type`, `tick`, `val1`, `val2`, `val3`, `val4`) VALUES ", scdata_db);
				for( i = 0; i < count; ++i )
				{
					memcpy (&data, RFIFOP(fd, 14+i*sizeof(struct status_change_data)), sizeof(struct status_change_data));
					if( i > 0 )
						StringBuf_AppendStr(&buf, ", ");
					StringBuf_Printf(&buf, "('%d','%d','%hu','%d','%d','%d','%d','%d')", aid, cid,
						data.type, data.tick, data.val1, data.val2, data.val3, data.val4);
				}
				if( SQL_ERROR == Sql_QueryStr(sql_handle, StringBuf_Value(&buf)) )
					Sql_ShowDebug(sql_handle);
				StringBuf_Destroy(&buf);
			}
#endif
#endif
			RFIFOSKIP(fd, RFIFOW(fd,2));
		}
		break;

		case 0x2b23: // map-server alive packet
			WFIFOHEAD(fd,2);
			WFIFOW(fd,0) = 0x2b24;
			WFIFOSET(fd,2);
			RFIFOSKIP(fd,2);
		break;

		case 0x2b26: // auth request from map-server
			if (RFIFOREST(fd) < 19)
				return 0;
		{
			int account_id;
			int char_id;
			int login_id1;
			char sex;
			uint32 ip;
			struct auth_node* node;
			struct mmo_charstatus* cd;

			account_id = RFIFOL(fd,2);
			char_id    = RFIFOL(fd,6);
			login_id1  = RFIFOL(fd,10);
			sex        = RFIFOB(fd,14);
			ip         = ntohl(RFIFOL(fd,15));
			RFIFOSKIP(fd,19);

			node = (struct auth_node*)idb_get(auth_db, account_id);
#ifdef TXT_ONLY
			cd = search_character(account_id, char_id);
#else
			cd = (struct mmo_charstatus*)uidb_get(char_db_,char_id);
			if( cd == NULL )
			{	//Really shouldn't happen.
				struct mmo_charstatus char_dat;
				mmo_char_fromsql(char_id, &char_dat, true);
				cd = (struct mmo_charstatus*)uidb_get(char_db_,char_id);
			}
#endif
			if( node != NULL && cd != NULL &&
				node->account_id == account_id &&
				node->char_id == char_id &&
				node->login_id1 == login_id1 &&
				node->sex == sex &&
				node->ip == ip )
			{// auth ok
				cd->sex = sex;

				WFIFOHEAD(fd,24 + sizeof(struct mmo_charstatus));
				WFIFOW(fd,0) = 0x2afd;
				WFIFOW(fd,2) = 24 + sizeof(struct mmo_charstatus);
				WFIFOL(fd,4) = account_id;
				WFIFOL(fd,8) = node->login_id1;
				WFIFOL(fd,12) = node->login_id2;
				WFIFOL(fd,16) = (uint32)node->expiration_time; // FIXME: will wrap to negative after "19-Jan-2038, 03:14:07 AM GMT"
				WFIFOL(fd,20) = node->gmlevel;
#ifdef TXT_ONLY
				storage_load(cd->account_id, &cd->storage); //FIXME: storage is used as a temp buffer here
#endif
				memcpy(WFIFOP(fd,24), cd, sizeof(struct mmo_charstatus));
				WFIFOSET(fd, WFIFOW(fd,2));

				// only use the auth once and mark user online
				idb_remove(auth_db, account_id);
				set_char_online(id, char_id, account_id);
			}
			else
			{// auth failed
				WFIFOHEAD(fd,19);
				WFIFOW(fd,0) = 0x2b27;
				WFIFOL(fd,2) = account_id;
				WFIFOL(fd,6) = char_id;
				WFIFOL(fd,10) = login_id1;
				WFIFOB(fd,14) = sex;
				WFIFOL(fd,15) = htonl(ip);
				WFIFOSET(fd,19);
			}
		}
		break;

		case 0x2736: // ip address update
			if (RFIFOREST(fd) < 6) return 0;
			server[id].ip = ntohl(RFIFOL(fd, 2));
			ShowInfo("Updated IP address of map-server #%d to %d.%d.%d.%d.\n", id, CONVIP(server[id].ip));
			RFIFOSKIP(fd,6);
		break;

		default:
		{
			// inter server - packet
			int r = inter_parse_frommap(fd);
			if (r == 1) break;		// processed
			if (r == 2) return 0;	// need more packet

			// no inter server packet. no char server packet -> disconnect
			ShowError("Unknown packet 0x%04x from map server, disconnecting.\n", RFIFOW(fd,0));
			set_eof(fd);
			return 0;
		}
		} // switch
	} // while
	
	return 0;
}
