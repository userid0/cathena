// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _INT_GUILD_H_
#define _INT_GUILD_H_

#include "guilddb.h"
#include "castledb.h"

int inter_guild_init(GuildDB* gdb, CastleDB* cdb);
void inter_guild_final(void);
int inter_guild_parse_frommap(int fd);
void inter_guild_delete(int guild_id);
void inter_guild_mapif_init(int fd);
int inter_guild_leave(int guild_id, int account_id, int char_id);
int inter_guild_sex_changed(int guild_id,int account_id,int char_id, int gender);
void mapif_parse_BreakGuild(int fd, int guild_id);

#endif /* _INT_GUILD_H_ */
