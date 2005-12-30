// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _PARTY_H_
#define _PARTY_H_

#include <stdarg.h>

extern int party_share_level;
struct party;
struct map_session_data;
struct block_list;

void do_init_party(void);
void do_final_party(void);
struct party *party_search(int party_id);
struct party* party_searchname(char *str);

int party_create(struct map_session_data *sd,char *name, int item, int item2);
int party_created(int account_id,int char_id,int fail,int party_id,char *name);
int party_request_info(int party_id);
int party_invite(struct map_session_data *sd,int account_id);
int party_member_added(int party_id,int account_id,int char_id,int flag);
int party_leave(struct map_session_data *sd);
int party_removemember(struct map_session_data *sd,int account_id,char *name);
int party_member_leaved(int party_id,int account_id,int char_id);
int party_reply_invite(struct map_session_data *sd,int account_id,int flag);
int party_recv_noinfo(int party_id);
int party_recv_info(struct party *sp);
int party_recv_movemap(int party_id,int account_id,int char_id, unsigned short map,int online,int lv);
int party_broken(int party_id);
int party_optionchanged(int party_id,int account_id,int exp,int item,int flag);
int party_changeoption(struct map_session_data *sd,int exp,int item);
int party_send_movemap(struct map_session_data *sd);
int party_send_logout(struct map_session_data *sd);
int party_send_message(struct map_session_data *sd,char *mes,int len);
int party_recv_message(int party_id,int account_id,char *mes,int len);
int party_check_conflict(struct map_session_data *sd);
int party_skill_check(struct map_session_data *sd, int party_id, int skillid, int skilllv);
int party_send_xy_clear(struct party *p);
void party_exp_share_check(struct map_session_data *sd, struct party *p);
int party_exp_share(struct party *p,int map,int base_exp,int job_exp,int zeny);
int party_send_dot_remove(struct map_session_data *sd);
int party_sub_count(struct block_list *bl, va_list ap);
void party_foreachsamemap(int (*func)(struct block_list *,va_list),struct map_session_data *sd,int type,...);


#endif
