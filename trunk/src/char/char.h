// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _CHAR_H_
#define _CHAR_H_

#include "../common/mmo.h"
#include "../common/mapindex.h"

#define START_CHAR_NUM 150000
#define MAX_MAP_SERVERS 30

#define CHAR_CONF_NAME	"conf/char_athena.conf"

#define LOGIN_LAN_CONF_NAME	"conf/subnet_athena.conf"

#define DEFAULT_AUTOSAVE_INTERVAL 300*1000

struct mmo_map_server{
	long ip;
	short port;
	int users;
	unsigned short map[MAX_MAP_PER_SERVER];
};

int search_character_index(char* character_name);
char * search_character_name(int index);

int mapif_sendall(unsigned char *buf, unsigned int len);
int mapif_sendallwos(int fd,unsigned char *buf, unsigned int len);
int mapif_send(int fd,unsigned char *buf, unsigned int len);

int char_married(int pl1,int pl2);
int char_child(int parent_id, int child_id);

int char_log(char *fmt, ...);

int request_accreg2(int account_id, int char_id);
int char_parse_Registry(int account_id, int char_id, unsigned char *buf, int len);
int save_accreg2(unsigned char *buf, int len);
int char_account_reg_reply(int fd,int account_id,int char_id);

extern int char_name_option;
extern char char_name_letters[];
extern int autosave_interval;
extern char db_path[];
extern int guild_exp_rate;
#endif
