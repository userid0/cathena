#ifndef _INT_GUILD_H_
#define _INT_GUILD_H_

int inter_guild_parse_frommap(int fd);
int inter_guild_sql_init();
void inter_guild_sql_final();
int inter_guild_mapif_init(int fd);
int inter_guild_leave(int guild_id,int account_id,int char_id);
int inter_guild_broken(int guild_id);
int inter_guild_sex_changed(int guild_id,int account_id,int char_id, int gender);
int inter_guild_CharOnline(int char_id, int guild_id);
int inter_guild_CharOffline(int char_id, int guild_id);

#endif
