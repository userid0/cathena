// $Id: clif.h 1952 2004-10-23 14:05:01Z Yor $
#ifndef _CLIF_H_
#define _CLIF_H_

#include "base.h"
#include "map.h"

#define MAX_PACKET_DB		0x235
#define MAX_PACKET_VER		18

struct packet_db {
	short len;
	int (*func)(int,struct map_session_data &);
	short pos[20];
};
extern struct packet_db packet_db[MAX_PACKET_VER + 1][MAX_PACKET_DB];

extern struct Clif_Config {
	int enable_packet_db;
	unsigned long packet_db_ver;
	int prefer_packet_db;
	int connect_cmd;
} clif_config;

void clif_setip(unsigned long ip);
void clif_setport(unsigned short port);

unsigned long  clif_getip(void);
unsigned short clif_getport(void);
int clif_countusers(void);

int clif_authok(struct map_session_data &sd);
int clif_authfail_fd(int fd, unsigned long type);
int clif_charselectok(unsigned long id);
int clif_dropflooritem(struct flooritem_data &fitem);
int clif_clearflooritem(struct flooritem_data &fitem, int fd=0);
int clif_clearchar(struct block_list &bl, unsigned char type);
int clif_clearchar_delay(unsigned long tick, struct block_list &bl, int type);
#define clif_clearchar_area(bl,type) clif_clearchar(bl,type)
int clif_clearchar_id(int fd, unsigned long id, unsigned char type);
int clif_spawnpc(struct map_session_data &sd);	//area
int clif_spawnnpc(struct npc_data &nd);	// area
int clif_spawnmob(struct mob_data &md);	// area
int clif_spawnpet(struct pet_data &pd);	// area
int clif_walkok(struct map_session_data &sd);	// self
int clif_movechar(struct map_session_data &sd);	// area
int clif_movemob(struct mob_data &md);	//area
int clif_movepet(struct pet_data &pd);	//area
int clif_movenpc(struct npc_data &nd);	// [Valaris]
int clif_changemap(struct map_session_data &sd, const char *mapname, unsigned short x, unsigned short y);	//self
int clif_changemapserver(struct map_session_data &sd, const char *mapname, unsigned short x, unsigned short y, unsigned long ip, unsigned short port);	//self
int clif_fixpos(struct block_list &bl);	// area
int clif_fixmobpos(struct mob_data &md);
int clif_fixpcpos(struct map_session_data &sd);
int clif_fixpetpos(struct pet_data &pd);
int clif_fixnpcpos(struct npc_data &nd); // [Valaris]
int clif_npcbuysell(struct map_session_data &sd, unsigned long id);	//self
int clif_buylist(struct map_session_data&,struct npc_data&);	//self
int clif_selllist(struct map_session_data&);	//self
int clif_scriptmes(struct map_session_data &sd, unsigned long npcid, const char *mes);	//self
int clif_scriptnext(struct map_session_data &sd,unsigned long npcid);	//self
int clif_scriptclose(struct map_session_data &sd, unsigned long npcid);	//self
int clif_scriptmenu(struct map_session_data &sd, unsigned long npcid, const char *mes);	//self
int clif_scriptinput(struct map_session_data &sd, unsigned long npcid);	//self
int clif_scriptinputstr(struct map_session_data &sd,unsigned long npcid);	// self
int clif_cutin(struct map_session_data &sd, const char *image, unsigned char type);	//self
int clif_viewpoint(struct map_session_data &sd, unsigned long npc_id, unsigned long id, unsigned long x, unsigned long y, unsigned char type, unsigned long color);	//self
int clif_additem(struct map_session_data &sd, unsigned short n, unsigned short amount, unsigned char fail);	//self
int clif_delitem(struct map_session_data &sd,unsigned short n,unsigned short amount);	//self
int clif_updatestatus(struct map_session_data &sd,unsigned short type);	//self
int clif_changestatus(struct block_list &bl,unsigned short type,unsigned long val);	//area
int clif_damage(struct block_list *src,struct block_list *dst,unsigned long tick,unsigned long sdelay,unsigned long ddelay,unsigned short damage,unsigned short div,unsigned char type,unsigned short damage2);	// area
#define clif_takeitem(src,dst) clif_damage(src,dst,0,0,0,0,0,1,0)
int clif_changelook(struct block_list *bl,unsigned char type, unsigned short val);	// area
int clif_arrowequip(struct map_session_data &sd,unsigned short val); //self
int clif_arrow_fail(struct map_session_data &sd,unsigned short type); //self
int clif_arrow_create_list(struct map_session_data &sd);	//self
int clif_statusupack(struct map_session_data &sd,unsigned short type,unsigned char ok,unsigned char val);	// self
int clif_equipitemack(struct map_session_data &sd,unsigned short n,unsigned short pos,unsigned char ok);	// self
int clif_unequipitemack(struct map_session_data &sd,unsigned short n,unsigned short pos,unsigned char ok);	// self
int clif_misceffect(struct block_list &bl, unsigned long type);	// area
int clif_misceffect2(struct block_list &bl, unsigned long type);
int clif_changeoption(struct block_list*);	// area
int clif_useitemack(struct map_session_data &sd,unsigned short index,unsigned short amount,unsigned char ok);	// self
int clif_GlobalMessage(struct block_list *bl,const char *message);
int clif_createchat(struct map_session_data &sd,unsigned char fail);	// self
int clif_dispchat(struct chat_data &cd,int fd);	// area or fd
int clif_joinchatfail(struct map_session_data &sd,unsigned char fail);	// self
int clif_joinchatok(struct map_session_data&,struct chat_data&);	// self
int clif_addchat(struct chat_data&,struct map_session_data&);	// chat
int clif_changechatowner(struct chat_data&,struct map_session_data&);	// chat
int clif_clearchat(struct chat_data &cd,int fd);	// area or fd
int clif_leavechat(struct chat_data&,struct map_session_data&);	// chat
int clif_changechatstatus(struct chat_data&);	// chat
int clif_refresh(struct map_session_data&);	// self

int clif_fame_blacksmith(struct map_session_data &sd, unsigned long points);
int clif_fame_alchemist(struct map_session_data &sd, unsigned long points);

int clif_emotion(struct block_list &bl,unsigned char type);
int clif_talkiebox(struct block_list &bl,const char* talkie);
int clif_wedding_effect(struct block_list &bl);
int clif_divorced(struct map_session_data &sd, const char *name);

//int clif_callpartner(struct map_session_data &sd);
int clif_adopt_process(struct map_session_data &sd);
int clif_sitting(struct map_session_data &sd);
int clif_soundeffect(struct map_session_data &sd,struct block_list &bl,const char *name,unsigned char type);
int clif_soundeffectall(struct block_list &bl, const char *name, unsigned char type);

// trade
int clif_traderequest(struct map_session_data &sd,const char *name);
int clif_tradestart(struct map_session_data &sd,unsigned char type);
int clif_tradeadditem(struct map_session_data &sd,struct map_session_data &tsd,unsigned short index,unsigned long amount);
int clif_tradeitemok(struct map_session_data &sd,unsigned short index,unsigned char fail);
int clif_tradedeal_lock(struct map_session_data &sd,unsigned char fail);
int clif_tradecancelled(struct map_session_data &sd);
int clif_tradecompleted(struct map_session_data &sd,unsigned char fail);

// storage
#include "storage.h"
int clif_storageitemlist(struct map_session_data &sd,struct pc_storage &stor);
int clif_storageequiplist(struct map_session_data &sd,struct pc_storage &stor);
int clif_updatestorageamount(struct map_session_data &sd,struct pc_storage &stor);
int clif_storageitemadded(struct map_session_data &sd,struct pc_storage &stor,unsigned short index,unsigned long amount);
int clif_storageitemremoved(struct map_session_data &sd,unsigned short index,unsigned long amount);
int clif_storageclose(struct map_session_data &sd);
int clif_guildstorageitemlist(struct map_session_data &sd,struct guild_storage &stor);
int clif_guildstorageequiplist(struct map_session_data &sd,struct guild_storage &stor);
int clif_updateguildstorageamount(struct map_session_data &sd,struct guild_storage &stor);
int clif_guildstorageitemadded(struct map_session_data &sd,struct guild_storage &stor,unsigned short index,unsigned long amount);

int clif_pcinsight(struct block_list &bl,va_list ap);	// map_forallinmovearea callback
int clif_pcoutsight(struct block_list &bl,va_list ap);	// map_forallinmovearea callback
int clif_mobinsight(struct block_list &bl,va_list ap);	// map_forallinmovearea callback
int clif_moboutsight(struct block_list &bl,va_list ap);	// map_forallinmovearea callback
int clif_petoutsight(struct block_list &bl,va_list ap);
int clif_petinsight(struct block_list &bl,va_list ap);
int clif_npcoutsight(struct block_list &bl,va_list ap);
int clif_npcinsight(struct block_list &bl,va_list ap);

int clif_class_change(struct block_list *bl,unsigned short class_,unsigned char type);
int clif_mob_class_change(struct mob_data &md,unsigned short class_);
int clif_mob_equip(struct mob_data &md,unsigned short nameid); // [Valaris]

int clif_skillinfo(struct map_session_data &sd,unsigned short skillid, short type,short range);
int clif_skillinfoblock(struct map_session_data &sd);
int clif_skillup(struct map_session_data &sd,unsigned short skill_num);

int clif_skillcasting(struct block_list* bl,unsigned long src_id,unsigned long dst_id,unsigned short dst_x,unsigned short dst_y,unsigned short skill_id,unsigned long casttime);
int clif_skillcastcancel(struct block_list* bl);
int clif_skill_fail(struct map_session_data &sd,unsigned short skill_id,unsigned char type,unsigned short btype);
int clif_skill_damage(struct block_list *src,struct block_list *dst,unsigned long tick,unsigned long sdelay,unsigned long ddelay,unsigned long damage,unsigned short div,unsigned short skill_id,unsigned short skill_lv,int type);
int clif_skill_damage2(struct block_list *src,struct block_list *dst,unsigned long tick,unsigned long sdelay,unsigned long ddelay,unsigned long damage,unsigned short div,unsigned short skill_id,unsigned short skill_lv,int type);
int clif_skill_nodamage(struct block_list *src,struct block_list *dst,unsigned short skill_id,unsigned short heal,unsigned char fail);
int clif_skill_poseffect(struct block_list *src,unsigned short skill_id,unsigned short val,unsigned short x,unsigned short y,unsigned long tick);
int clif_skill_estimation(struct map_session_data &sd,struct block_list *dst);
int clif_skill_warppoint(struct map_session_data &sd,unsigned short skill_id,const char *map1,const char *map2,const char *map3,const char *map4);
int clif_skill_memo(struct map_session_data &sd,unsigned char flag);
int clif_skill_teleportmessage(struct map_session_data &sd,unsigned short flag);
int clif_skill_produce_mix_list(struct map_session_data &sd,int trigger);

int clif_produceeffect(struct map_session_data &sd,unsigned short flag,unsigned short nameid);

int clif_skill_setunit(struct skill_unit *unit);
int clif_skill_delunit(struct skill_unit *unit);

int clif_01ac(struct block_list &bl);

int clif_autospell(struct map_session_data &sd,unsigned short skilllv);
int clif_devotion(struct map_session_data &sd,unsigned long target_id);
int clif_spiritball(struct map_session_data &sd);
int clif_combo_delay(struct block_list &src,unsigned long wait);
int clif_bladestop(struct block_list &src,struct block_list &dst,unsigned long bool_);
int clif_changemapcell(unsigned short m,unsigned short x,unsigned short y,unsigned short cell_type,int type);

int clif_status_change(struct block_list &bl,unsigned short type,unsigned char flag);

int clif_wis_message(int fd,const char *nick,const char *mes,size_t mes_len);
int clif_wis_end(int fd, unsigned short flag);

int clif_solved_charname(struct map_session_data &sd, unsigned long char_id);
int clif_update_mobhp(struct mob_data &md);

int clif_use_card(struct map_session_data &sd, size_t idx);
int clif_insert_card(struct map_session_data &sd,unsigned short idx_equip,unsigned short idx_card,unsigned char flag);

int clif_itemlist(struct map_session_data &sd);
int clif_equiplist(struct map_session_data &sd);

int clif_cart_additem(struct map_session_data &sd,unsigned short n,unsigned long amount,unsigned char fail);
int clif_cart_delitem(struct map_session_data &sd,unsigned short n,unsigned long amount);
int clif_cart_itemlist(struct map_session_data &sd);
int clif_cart_equiplist(struct map_session_data &sd);

int clif_item_identify_list(struct map_session_data &sd);
int clif_item_identified(struct map_session_data &sd,unsigned short idx,unsigned char flag);
int clif_item_repair_list(struct map_session_data &sd);
int clif_item_refine_list(struct map_session_data &sd);

int clif_item_skill(struct map_session_data &sd,unsigned short skillid,unsigned short skilllv,const char *name);

int clif_mvp_effect(struct map_session_data &sd);
int clif_mvp_item(struct map_session_data &sd,unsigned short nameid);
int clif_mvp_exp(struct map_session_data &sd, unsigned long exp);
int clif_changed_dir(struct block_list &bl);

// vending
int clif_openvendingreq(struct map_session_data &sd,unsigned short num);
int clif_showvendingboard(struct block_list &bl,const char *message,int fd);
int clif_closevendingboard(struct block_list &bl,int fd);
int clif_vendinglist(struct map_session_data &sd,unsigned long id,struct vending vending[]);
int clif_buyvending(struct map_session_data &sd,unsigned short index,unsigned short amount,unsigned char fail);
int clif_openvending(struct map_session_data &sd,unsigned long id,struct vending *vending);
int clif_vendingreport(struct map_session_data &sd,unsigned short index,unsigned short amount);

int clif_movetoattack(struct map_session_data &sd,struct block_list &bl);

// party
int clif_party_created(struct map_session_data &sd,unsigned char flag);
int clif_party_info(struct party &p,int fd);
int clif_party_invite(struct map_session_data &sd,struct map_session_data &tsd);
int clif_party_inviteack(struct map_session_data &sd,const char *nick,unsigned char flag);
int clif_party_option(struct party &p,struct map_session_data *sd,int flag);
int clif_party_leaved(struct party &p,struct map_session_data *sd,unsigned long account_id,char *name,unsigned char flag);
int clif_party_message(struct party &p,unsigned long account_id,const char *mes,size_t len);
int clif_party_move(struct party &p,struct map_session_data &sd,unsigned char online);
int clif_party_xy(struct party &p,struct map_session_data &sd);
int clif_party_hp(struct party &p,struct map_session_data &sd);
int clif_hpmeter(struct map_session_data &sd);

// guild
int clif_guild_created(struct map_session_data &sd,unsigned char flag);
int clif_guild_belonginfo(struct map_session_data &sd,struct guild &g);
int clif_guild_basicinfo(struct map_session_data &sd);
int clif_guild_allianceinfo(struct map_session_data &sd);
int clif_guild_memberlist(struct map_session_data &sd);
int clif_guild_skillinfo(struct map_session_data &sd);
int clif_guild_memberlogin_notice(struct guild &g,unsigned long idx,unsigned long flag);
int clif_guild_invite(struct map_session_data &sd,struct guild &g);
int clif_guild_inviteack(struct map_session_data &sd,unsigned char flag);
int clif_guild_leave(struct map_session_data &sd,const char *name,const char *mes);
int clif_guild_explusion(struct map_session_data &sd,const char *name,const char *mes,unsigned long account_id);
int clif_guild_positionchanged(struct guild &g,unsigned long idx);
int clif_guild_memberpositionchanged(struct guild &g,unsigned long idx);
int clif_guild_emblem(struct map_session_data &sd,struct guild &g);
int clif_guild_notice(struct map_session_data &sd,struct guild &g);
int clif_guild_message(struct guild &g,unsigned long account_id,const char *mes,size_t len);
int clif_guild_skillup(struct map_session_data &sd,unsigned short skillid,unsigned short skilllv);
int clif_guild_reqalliance(struct map_session_data &sd,unsigned long account_id,const char *name);
int clif_guild_allianceack(struct map_session_data &sd,unsigned long flag);
int clif_guild_delalliance(struct map_session_data &sd,unsigned long guild_id,unsigned long flag);
int clif_guild_oppositionack(struct map_session_data &sd,unsigned char flag);
int clif_guild_broken(struct map_session_data &sd,unsigned long flag);


// atcommand
int clif_displaymessage(int fd,const char* mes);
int clif_disp_onlyself(struct map_session_data &sd,const char *mes);
int clif_GMmessage(struct block_list *bl,const char* mes,int flag);
int clif_heal(int fd,unsigned short type,unsigned short val);
int clif_resurrection(struct block_list &bl,unsigned short type);
int clif_set0199(int fd,unsigned short type);
int clif_pvpset(struct map_session_data &sd,unsigned long pvprank,unsigned long pvpnum,int type);
int clif_send0199(unsigned short map,unsigned short type);
int clif_refine(int fd,struct map_session_data &sd,unsigned short fail,unsigned short index,unsigned short val);

//petsystem
int clif_catch_process(struct map_session_data &sd);
int clif_pet_rulet(struct map_session_data &sd,unsigned char data);
int clif_sendegg(struct map_session_data &sd);
int clif_send_petdata(struct map_session_data &sd,unsigned char type,unsigned long param);
int clif_send_petstatus(struct map_session_data &sd);
int clif_pet_emotion(struct pet_data &pd,unsigned long param);
int clif_pet_performance(struct block_list &bl,unsigned long param);
int clif_pet_equip(struct pet_data &pd,unsigned short nameid);
int clif_pet_food(struct map_session_data &sd,unsigned short foodid,unsigned char fail);

//friends list
int clif_friendslist_send(struct map_session_data &sd);
int clif_friendslist_reqack(struct map_session_data &sd, const char *name, unsigned short type);

int clif_specialeffect(struct block_list &bl,unsigned long type, int flag); // special effects [Valaris]
int clif_message(struct block_list &bl, const char* msg); // messages (from mobs/npcs) [Valaris]

int clif_GM_kickack(struct map_session_data &sd,unsigned long id);
int clif_GM_kick(struct map_session_data &sd,struct map_session_data &tsd,int type);
int clif_GM_silence(struct map_session_data &sd,struct map_session_data &tsd,int type);
int clif_timedout(struct map_session_data &sd);

int clif_foreachclient(int (*)(struct map_session_data&,va_list),...);
int clif_disp_overhead(struct map_session_data &sd, const char* mes);


int clif_terminate(int fd);

int do_final_clif(void);
int do_init_clif(void);

//Fix for minimap [Kevin]
int clif_party_xy_remove(struct map_session_data &sd);

#endif


