// $Id: mmo.h,v 1.3 2004/09/25 20:12:25 PoW Exp $
// Original : mmo.h 2003/03/14 12:07:02 Rev.1.7

#ifndef	_MMO_H_
#define	_MMO_H_

#include "base.h"
#include "socket.h"

#define FIFOSIZE_SERVERLINK	128*1024

// set to 0 to not check IP of player between each server.
// set to another value if you want to check (1)
#define CMP_AUTHFIFO_IP 1
#define CMP_AUTHFIFO_LOGIN2 1



#define MAX_MAP_PER_SERVER 512
#define MAX_INVENTORY 100
#define MAX_AMOUNT 30000
#define MAX_ZENY 1000000000	// 1G zeny
#define MAX_CART 100
#define MAX_SKILL 650
#define GLOBAL_REG_NUM 96
#define ACCOUNT_REG_NUM 16
#define ACCOUNT_REG2_NUM 16
#define DEFAULT_WALK_SPEED 150
#define MIN_WALK_SPEED 0
#define MAX_WALK_SPEED 1000
#define MAX_STORAGE 300
#define MAX_GUILD_STORAGE 1000
#define MAX_PARTY 12
#define MAX_GUILD 16+10*6	// increased max guild members to accomodate for +6 increase for extension levels [Lupus]
#define MAX_GUILDPOSITION 20	// increased max guild positions to accomodate for all members [Valaris] (removed) [PoW]
#define MAX_GUILDEXPLUSION 32
#define MAX_GUILDALLIANCE 16
#define MAX_GUILDSKILL	15 // increased max guild skills because of new skills [Sara-chan]
#define MAX_GUILDCASTLE 24	// increased to include novice castles [Valaris]
#define MAX_GUILDLEVEL 50

#define MIN_HAIR_STYLE battle_config.min_hair_style
#define MAX_HAIR_STYLE battle_config.max_hair_style
#define MIN_HAIR_COLOR battle_config.min_hair_color
#define MAX_HAIR_COLOR battle_config.max_hair_color
#define MIN_CLOTH_COLOR battle_config.min_cloth_color
#define MAX_CLOTH_COLOR battle_config.max_cloth_color

// for produce
#define MIN_ATTRIBUTE 0
#define MAX_ATTRIBUTE 4
#define ATTRIBUTE_NORMAL 0
#define MIN_STAR 0
#define MAX_STAR 3

#define MIN_PORTAL_MEMO 0
#define MAX_PORTAL_MEMO 2

#define MAX_STATUS_TYPE 5

#define WEDDING_RING_M 2634
#define WEDDING_RING_F 2635

#define CHAR_CONF_NAME  "conf/char_athena.conf"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

enum {
	GBI_EXP			=1,		// ギルドのEXP
	GBI_GUILDLV		=2,		// ギルドのLv
	GBI_SKILLPOINT	=3,		// ギルドのスキルポイント
	GBI_SKILLLV		=4,		// ギルドスキルLv

	GMI_POSITION	=0,		// メンバーの役職変更
	GMI_EXP			=1,		// メンバーのEXP

};

enum {
	GD_SKILLBASE=10000,
	GD_APPROVAL=10000,
	GD_KAFRACONTACT=10001,
	GD_GUARDIANRESEARCH=10002,
	GD_CHARISMA=10003,
	GD_GUARDUP=10003,
	GD_EXTENSION=10004,
	GD_GLORYGUILD=10005,
	GD_LEADERSHIP=10006,
	GD_GLORYWOUNDS=10007,
	GD_SOULCOLD=10008,
	GD_HAWKEYES=10009,
	GD_BATTLEORDER=10010,
	GD_REGENERATION=10011,
	GD_RESTORE=10012,
	GD_EMERGENCYCALL=10013,
	GD_DEVELOPMENT=10014,
};

/////////////////////////////////////////////////////////////////////////////
// simplified buffer functions with moving buffer pointer 
// to enable secure continous inserting of data to a buffer
// no pointer checking is implemented here, so make sure the calls are correct
/////////////////////////////////////////////////////////////////////////////

extern inline void _L_tobuffer(const unsigned long *valin, unsigned char **buf)
{	
	*(*buf)++ = (unsigned char)((*valin & 0x000000FF)          );
	*(*buf)++ = (unsigned char)((*valin & 0x0000FF00)  >> 0x08 );
	*(*buf)++ = (unsigned char)((*valin & 0x00FF0000)  >> 0x10 );
	*(*buf)++ = (unsigned char)((*valin & 0xFF000000)  >> 0x18 );
}

extern inline void _W_tobuffer(const unsigned short *valin, unsigned char **buf)
{	
	*(*buf)++ = (unsigned char)((*valin & 0x00FF)          );
	*(*buf)++ = (unsigned char)((*valin & 0xFF00)  >> 0x08 );
}

extern inline void _B_tobuffer(const unsigned char *valin, unsigned char **buf)
{	
	*(*buf)++ = (unsigned char)((*valin & 0xFF)          );
}

extern inline void _S_tobuffer(const char *valin, unsigned char **buf, const size_t sz)
{	
	strncpy((char*)(*buf), (char*)valin, sz);
	(*buf) += sz;
}
extern inline void S_tobuffer(const char *valin, unsigned char **buf, const size_t sz)
{	
	strncpy((char*)buf, (char*)valin, sz);
}

extern inline void _L_frombuffer(unsigned long *valin, const unsigned char **buf)
{	
	*valin = ( ((unsigned long)((*buf)[0]))        )
			|( ((unsigned long)((*buf)[1])) << 0x08)
			|( ((unsigned long)((*buf)[2])) << 0x10)
			|( ((unsigned long)((*buf)[3])) << 0x18);
	(*buf) += 4;
}

extern inline void _W_frombuffer(unsigned short *valin, const unsigned char **buf)
{	
	*valin = ( ((unsigned short)((*buf)[0]))        )
			|( ((unsigned short)((*buf)[1])) << 0x08);
	(*buf) += 2;
}

extern inline void _B_frombuffer(unsigned char *valin, const unsigned char **buf)
{	
	*valin	= *(*buf)++;
}

extern inline void _S_frombuffer(char *valin, const unsigned char **buf, const size_t sz)
{	
	strncpy((char*)valin, (char*)(*buf), sz);
	(*buf) += sz;
}
extern inline void S_frombuffer(char *valin, const unsigned char *buf, const size_t sz)
{	
	strncpy((char*)valin, (char*)buf, sz);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// predeclaration
struct map_session_data;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct item {
	int id;
	short nameid;
	short amount;
	unsigned short equip;
	char identify;
	char refine;
	char attribute;
	short card[4];
};
extern inline void _item_tobuffer(const struct item *p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->id),			buf);
	_W_tobuffer( (unsigned short*)(&p->nameid),		buf);
	_W_tobuffer( (unsigned short*)(&p->amount),		buf);
	_W_tobuffer(                  (&p->equip),		buf);
	_B_tobuffer( (unsigned char*) (&p->identify),	buf);
	_B_tobuffer( (unsigned char*) (&p->refine),		buf);
	_B_tobuffer( (unsigned char*) (&p->attribute),	buf);
	_W_tobuffer( (unsigned short*)(&p->card[0]),	buf);
	_W_tobuffer( (unsigned short*)(&p->card[1]),	buf);
	_W_tobuffer( (unsigned short*)(&p->card[2]),	buf);
	_W_tobuffer( (unsigned short*)(&p->card[3]),	buf);
}
extern inline void item_tobuffer(const struct item *p, unsigned char *buf)
{
	_item_tobuffer(p, &buf);
}

extern inline void _item_frombuffer(struct item *p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->id),		buf);
	_W_frombuffer( (unsigned short*)(&p->nameid),	buf);
	_W_frombuffer( (unsigned short*)(&p->amount),	buf);
	_W_frombuffer(                  (&p->equip),		buf);
	_B_frombuffer( (unsigned char*) (&p->identify),	buf);
	_B_frombuffer( (unsigned char*) (&p->refine),	buf);
	_B_frombuffer( (unsigned char*) (&p->attribute),buf);
	_W_frombuffer( (unsigned short*)(&p->card[0]),	buf);
	_W_frombuffer( (unsigned short*)(&p->card[1]),	buf);
	_W_frombuffer( (unsigned short*)(&p->card[2]),	buf);
	_W_frombuffer( (unsigned short*)(&p->card[3]),	buf);
}
extern inline void item_frombuffer(struct item *p, const unsigned char *buf)
{
	_item_frombuffer(p, &buf);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct point{
	char map[24];
	short x;
	short y;
};
extern inline void _point_tobuffer(const struct point *p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_S_tobuffer(                   p->map,		buf, 24);
	_W_tobuffer( (unsigned short*)(&p->x),		buf);
	_W_tobuffer( (unsigned short*)(&p->y),		buf);
}
extern inline void point_tobuffer(const struct point *p, unsigned char *buf)
{
	_point_tobuffer(p, &buf);
}

extern inline void _point_frombuffer(struct point *p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_S_frombuffer(                   p->map,	buf, 24);
	_W_frombuffer( (unsigned short*)(&p->x),		buf);
	_W_frombuffer( (unsigned short*)(&p->y),		buf);
}
extern inline void point_frombuffer(struct point *p, const unsigned char *buf)
{
	_point_frombuffer(p, &buf);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct skill {
	unsigned short id;
	unsigned short lv;
	unsigned short flag;
};
extern inline void _skill_tobuffer(const struct skill *p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_W_tobuffer( &(p->id),		buf);
	_W_tobuffer( &(p->lv),		buf);
	_W_tobuffer( &(p->flag),	buf);
}
extern inline void skill_tobuffer(const struct skill *p, unsigned char *buf)
{
	_skill_tobuffer(p, &buf);
}

extern inline void _skill_frombuffer(struct skill *p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_W_frombuffer( &(p->id),	buf);
	_W_frombuffer( &(p->lv),	buf);
	_W_frombuffer( &(p->flag),	buf);
}
extern inline void skill_frombuffer(struct skill *p, const unsigned char *buf)
{
	_skill_frombuffer(p, &buf);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct global_reg {
	char str[32];
	int value;
};
extern inline void _global_reg_tobuffer(const struct global_reg *p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_S_tobuffer(                   p->str,		buf, 32);
	_L_tobuffer( (unsigned long*) (&p->value),	buf);
}
extern inline void global_reg_tobuffer(const struct global_reg *p, unsigned char *buf)
{
	_global_reg_tobuffer(p, &buf);
}

extern inline void _global_reg_frombuffer(struct global_reg *p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_S_frombuffer(                   p->str,	buf, 32);
	_L_frombuffer( (unsigned long*) (&p->value),	buf);
}
extern inline void global_reg_frombuffer(struct global_reg *p, const unsigned char *buf)
{
	_global_reg_frombuffer(p, &buf);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct s_pet {
	int account_id;
	int char_id;
	int pet_id;
	short class_;
	short level;
	short egg_id;//pet egg id
	short equip;//pet equip name_id
	short intimate;//pet friendly
	short hungry;//pet hungry
	char name[24];
	char rename_flag;
	char incuvate;
};
extern inline void _s_pet_tobuffer(const struct s_pet *p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->account_id),	buf);
	_L_tobuffer( (unsigned long*) (&p->char_id),		buf);
	_L_tobuffer( (unsigned long*) (&p->pet_id),		buf);
	_W_tobuffer( (unsigned short*)(&p->class_),		buf);
	_W_tobuffer( (unsigned short*)(&p->level),		buf);
	_W_tobuffer( (unsigned short*)(&p->egg_id),		buf);
	_W_tobuffer( (unsigned short*)(&p->equip),		buf);
	_W_tobuffer( (unsigned short*)(&p->intimate),	buf);
	_W_tobuffer( (unsigned short*)(&p->hungry),		buf);
	_S_tobuffer(                  (p->name),		buf, 24);
	_B_tobuffer( (unsigned char*) (&p->rename_flag),	buf);
	_B_tobuffer( (unsigned char*) (&p->incuvate),	buf);
}
extern inline void s_pet_tobuffer(const struct s_pet *p, unsigned char *buf)
{
	_s_pet_tobuffer(p, &buf);
}
extern inline void _s_pet_frombuffer(struct s_pet *p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->account_id),	buf);
	_L_frombuffer( (unsigned long*) (&p->char_id),		buf);
	_L_frombuffer( (unsigned long*) (&p->pet_id),		buf);
	_W_frombuffer( (unsigned short*)(&p->class_),		buf);
	_W_frombuffer( (unsigned short*)(&p->level),			buf);
	_W_frombuffer( (unsigned short*)(&p->egg_id),		buf);
	_W_frombuffer( (unsigned short*)(&p->equip),			buf);
	_W_frombuffer( (unsigned short*)(&p->intimate),		buf);
	_W_frombuffer( (unsigned short*)(&p->hungry),		buf);
	_S_frombuffer(                  (p->name),			buf, 24);
	_B_frombuffer( (unsigned char*) (&p->rename_flag),	buf);
	_B_frombuffer( (unsigned char*) (&p->incuvate),		buf);
}
extern inline void s_pet_frombuffer(struct s_pet *p, const unsigned char *buf)
{
	_s_pet_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct mmo_charstatus {
	int char_id;
	int account_id;
	int partner_id;

	int base_exp;
	int job_exp;
	int zeny;

	short class_;
	short status_point;
	short skill_point;
	int hp;
	int max_hp;
	int sp;
	int max_sp;
	short option;
	short karma;
	short manner;
	short hair;
	short hair_color;
	short clothes_color;
	int party_id;
	int guild_id;
	int pet_id;

	short weapon;
	short shield;
	short head_top;
	short head_mid;
	short head_bottom;

	char name[24];
	unsigned short base_level;
	unsigned short job_level;
	short str;
	short agi;
	short vit;
	short int_;
	short dex;
	short luk;
	unsigned char char_num;
	unsigned char sex;
	unsigned long mapip;
	unsigned int mapport;

	struct point last_point;
	struct point save_point;
	struct point memo_point[10];
	struct item inventory[MAX_INVENTORY];
	struct item cart[MAX_CART];
	struct skill skill[MAX_SKILL];
	int global_reg_num;
	struct global_reg global_reg[GLOBAL_REG_NUM];
	int account_reg_num;
	struct global_reg account_reg[ACCOUNT_REG_NUM];
	int account_reg2_num;
	struct global_reg account_reg2[ACCOUNT_REG2_NUM];

	// Friends list vars
	int friend_id[20];
	char friend_name[20][24];
};
extern inline void _mmo_charstatus_tobuffer(const struct mmo_charstatus *p, unsigned char **buf)
{
	size_t i;
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->char_id),			buf);
	_L_tobuffer( (unsigned long*) (&p->account_id),		buf);
	_L_tobuffer( (unsigned long*) (&p->partner_id),		buf);

	_L_tobuffer( (unsigned long*) (&p->base_exp),		buf);
	_L_tobuffer( (unsigned long*) (&p->job_exp),			buf);
	_L_tobuffer( (unsigned long*) (&p->zeny),			buf);

	_W_tobuffer( (unsigned short*)(&p->class_),			buf);
	_W_tobuffer( (unsigned short*)(&p->status_point),	buf);
	_W_tobuffer( (unsigned short*)(&p->skill_point),		buf);

	_L_tobuffer( (unsigned long*) (&p->hp),				buf);
	_L_tobuffer( (unsigned long*) (&p->max_hp),			buf);
	_L_tobuffer( (unsigned long*) (&p->sp),				buf);
	_L_tobuffer( (unsigned long*) (&p->max_sp),			buf);

	_W_tobuffer( (unsigned short*)(&p->option),			buf);
	_W_tobuffer( (unsigned short*)(&p->karma),			buf);
	_W_tobuffer( (unsigned short*)(&p->manner),			buf);
	_W_tobuffer( (unsigned short*)(&p->hair),			buf);
	_W_tobuffer( (unsigned short*)(&p->hair_color),		buf);
	_W_tobuffer( (unsigned short*)(&p->clothes_color),	buf);

	_L_tobuffer( (unsigned long*) (&p->party_id),		buf);
	_L_tobuffer( (unsigned long*) (&p->guild_id),		buf);
	_L_tobuffer( (unsigned long*) (&p->pet_id),			buf);

	_W_tobuffer( (unsigned short*)(&p->option),			buf);
	_W_tobuffer( (unsigned short*)(&p->karma),			buf);
	_W_tobuffer( (unsigned short*)(&p->manner),			buf);
	_W_tobuffer( (unsigned short*)(&p->hair),			buf);

	_W_tobuffer( (unsigned short*)(&p->weapon),			buf);
	_W_tobuffer( (unsigned short*)(&p->shield),			buf);
	_W_tobuffer( (unsigned short*)(&p->head_top),		buf);
	_W_tobuffer( (unsigned short*)(&p->head_mid),		buf);
	_W_tobuffer( (unsigned short*)(&p->head_bottom),		buf);

	_S_tobuffer(                   p->name,				buf, 24);


	_W_tobuffer(                 (&p->base_level),		buf);
	_W_tobuffer(                 (&p->job_level),		buf);
	_W_tobuffer( (unsigned short*)(&p->str),				buf);
	_W_tobuffer( (unsigned short*)(&p->agi),				buf);
	_W_tobuffer( (unsigned short*)(&p->vit),				buf);
	_W_tobuffer( (unsigned short*)(&p->int_),			buf);
	_W_tobuffer( (unsigned short*)(&p->dex),				buf);
	_W_tobuffer( (unsigned short*)(&p->luk),				buf);

	_B_tobuffer(                 (&p->char_num),		buf);
	_B_tobuffer(                 (&p->sex),				buf);

	_L_tobuffer(                 (&p->mapip),			buf);
	_L_tobuffer( (unsigned long*) (&p->mapport),			buf);


	_point_tobuffer(             &(p->last_point),		buf);
	_point_tobuffer(             &(p->save_point),		buf);
	for(i=0; i<10; i++)
		_point_tobuffer(           p->memo_point+i,		buf);

	for(i=0; i<MAX_INVENTORY; i++)
		_item_tobuffer(            p->inventory+i,		buf);

	for(i=0; i<MAX_CART; i++)
		_item_tobuffer(            p->cart+i,			buf);

	for(i=0; i<MAX_SKILL; i++)
		_skill_tobuffer(           p->skill+i,			buf);

	_L_tobuffer( (unsigned long*) (&p->global_reg_num),	buf);
	for(i=0; i<GLOBAL_REG_NUM; i++)
		_global_reg_tobuffer(      p->global_reg+i,		buf);


	_L_tobuffer( (unsigned long*) (&p->account_reg_num),	buf);
	for(i=0; i<ACCOUNT_REG_NUM; i++)
		_global_reg_tobuffer(      p->account_reg+i,	buf);

	_L_tobuffer( (unsigned long*) (&p->account_reg2_num),buf);
	for(i=0; i<ACCOUNT_REG2_NUM; i++)
		_global_reg_tobuffer(      p->account_reg2+i,	buf);

	// Friends list vars
	for(i=0; i<20; i++)
		_L_tobuffer((unsigned long*) (&p->friend_id[i]),	buf);
	for(i=0; i<20; i++)
		_S_tobuffer(               p->friend_name[i],	buf, 24);
}
extern inline void mmo_charstatus_tobuffer(const struct mmo_charstatus *p, unsigned char *buf)
{
	_mmo_charstatus_tobuffer(p, &buf);
}
extern inline void _mmo_charstatus_frombuffer(struct mmo_charstatus *p, const unsigned char **buf)
{
	size_t i;
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->char_id),		buf);
	_L_frombuffer( (unsigned long*) (&p->account_id),	buf);
	_L_frombuffer( (unsigned long*) (&p->partner_id),	buf);

	_L_frombuffer( (unsigned long*) (&p->base_exp),		buf);
	_L_frombuffer( (unsigned long*) (&p->job_exp),		buf);
	_L_frombuffer( (unsigned long*) (&p->zeny),			buf);

	_W_frombuffer( (unsigned short*)(&p->class_),		buf);
	_W_frombuffer( (unsigned short*)(&p->status_point),	buf);
	_W_frombuffer( (unsigned short*)(&p->skill_point),	buf);

	_L_frombuffer( (unsigned long*) (&p->hp),			buf);
	_L_frombuffer( (unsigned long*) (&p->max_hp),		buf);
	_L_frombuffer( (unsigned long*) (&p->sp),			buf);
	_L_frombuffer( (unsigned long*) (&p->max_sp),		buf);

	_W_frombuffer( (unsigned short*)(&p->option),		buf);
	_W_frombuffer( (unsigned short*)(&p->karma),			buf);
	_W_frombuffer( (unsigned short*)(&p->manner),		buf);
	_W_frombuffer( (unsigned short*)(&p->hair),			buf);
	_W_frombuffer( (unsigned short*)(&p->hair_color),	buf);
	_W_frombuffer( (unsigned short*)(&p->clothes_color),	buf);

	_L_frombuffer( (unsigned long*) (&p->party_id),		buf);
	_L_frombuffer( (unsigned long*) (&p->guild_id),		buf);
	_L_frombuffer( (unsigned long*) (&p->pet_id),		buf);

	_W_frombuffer( (unsigned short*)(&p->option),		buf);
	_W_frombuffer( (unsigned short*)(&p->karma),			buf);
	_W_frombuffer( (unsigned short*)(&p->manner),		buf);
	_W_frombuffer( (unsigned short*)(&p->hair),			buf);

	_W_frombuffer( (unsigned short*)(&p->weapon),		buf);
	_W_frombuffer( (unsigned short*)(&p->shield),		buf);
	_W_frombuffer( (unsigned short*)(&p->head_top),		buf);
	_W_frombuffer( (unsigned short*)(&p->head_mid),		buf);
	_W_frombuffer( (unsigned short*)(&p->head_bottom),	buf);

	_S_frombuffer(                   p->name,			buf, 24);


	_W_frombuffer(                 (&p->base_level),	buf);
	_W_frombuffer(                 (&p->job_level),		buf);
	_W_frombuffer( (unsigned short*)(&p->str),			buf);
	_W_frombuffer( (unsigned short*)(&p->agi),			buf);
	_W_frombuffer( (unsigned short*)(&p->vit),			buf);
	_W_frombuffer( (unsigned short*)(&p->int_),			buf);
	_W_frombuffer( (unsigned short*)(&p->dex),			buf);
	_W_frombuffer( (unsigned short*)(&p->luk),			buf);

	_B_frombuffer(                 (&p->char_num),		buf);
	_B_frombuffer(                 (&p->sex),			buf);

	_L_frombuffer(                 (&p->mapip),			buf);
	_L_frombuffer( (unsigned long*) (&p->mapport),		buf);


	_point_frombuffer(             &(p->last_point),	buf);
	_point_frombuffer(             &(p->save_point),	buf);
	for(i=0; i<10; i++)
		_point_frombuffer(           p->memo_point+i,	buf);

	for(i=0; i<MAX_INVENTORY; i++)
		_item_frombuffer(            p->inventory+i,	buf);

	for(i=0; i<MAX_CART; i++)
		_item_frombuffer(            p->cart+i,			buf);

	for(i=0; i<MAX_SKILL; i++)
		_skill_frombuffer(           p->skill+i,		buf);

	_L_frombuffer( (unsigned long*) (&p->global_reg_num),buf);
	for(i=0; i<GLOBAL_REG_NUM; i++)
		_global_reg_frombuffer(      p->global_reg+i,	buf);


	_L_frombuffer( (unsigned long*) (&p->account_reg_num),buf);
	for(i=0; i<ACCOUNT_REG_NUM; i++)
		_global_reg_frombuffer(      p->account_reg+i,	buf);

	_L_frombuffer( (unsigned long*) (&p->account_reg2_num),buf);
	for(i=0; i<ACCOUNT_REG2_NUM; i++)
		_global_reg_frombuffer(      p->account_reg2+i,	buf);

	// Friends list vars
	for(i=0; i<20; i++)
		_L_frombuffer((unsigned long*) (&p->friend_id[i]),buf);
	for(i=0; i<20; i++)
		_S_frombuffer(               p->friend_name[i],	buf, 24);
}
extern inline void mmo_charstatus_frombuffer(struct mmo_charstatus *p, const unsigned char *buf)
{
	_mmo_charstatus_frombuffer(p, &buf);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct pc_storage {
	int dirty;
	int account_id;
	short storage_status;
	short storage_amount;
	struct item storage[MAX_STORAGE];
};
extern inline void _pc_storage_tobuffer(const struct pc_storage *p, unsigned char **buf)
{
	size_t i;
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->dirty),			buf);
	_L_tobuffer( (unsigned long*) (&p->account_id),		buf);
	_W_tobuffer( (unsigned short*)(&p->storage_status),	buf);
	_W_tobuffer( (unsigned short*)(&p->storage_amount),	buf);
	for(i=0;i<MAX_STORAGE;i++)
		_item_tobuffer(p->storage+i,		buf);
}
extern inline void pc_storage_tobuffer(const struct pc_storage *p, unsigned char *buf)
{
	_pc_storage_tobuffer(p, &buf);
}
extern inline void _pc_storage_frombuffer(struct pc_storage *p, const unsigned char **buf)
{
	size_t i;
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->dirty),			buf);
	_L_frombuffer( (unsigned long*) (&p->account_id),	buf);
	_W_frombuffer( (unsigned short*)(&p->storage_status),buf);
	_W_frombuffer( (unsigned short*)(&p->storage_amount),buf);
	for(i=0;i<MAX_STORAGE;i++)
		_item_frombuffer(p->storage+i,	buf);
}
extern inline void pc_storage_frombuffer(struct pc_storage *p, const unsigned char *buf)
{
	_pc_storage_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct guild_storage {
	int guild_id;
	short storage_status;
	short storage_amount;
	struct item storage[MAX_GUILD_STORAGE];
};
extern inline void _guild_storage_tobuffer(const struct guild_storage *p, unsigned char **buf)
{
	size_t i;
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->guild_id),			buf);
	_W_tobuffer( (unsigned short*)(&p->storage_status),		buf);
	_W_tobuffer( (unsigned short*)(&p->storage_amount),		buf);
	for(i=0;i<MAX_STORAGE;i++)
		_item_tobuffer(p->storage+i,		buf);
}
extern inline void guild_storage_tobuffer(const struct guild_storage *p, unsigned char *buf)
{
	_guild_storage_tobuffer(p, &buf);
}
extern inline void _guild_storage_frombuffer(struct guild_storage *p, const unsigned char **buf)
{
	size_t i;
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->guild_id),			buf);
	_W_frombuffer( (unsigned short*)(&p->storage_status),	buf);
	_W_frombuffer( (unsigned short*)(&p->storage_amount),	buf);
	for(i=0;i<MAX_STORAGE;i++)
		_item_frombuffer(p->storage+i,		buf);
}
extern inline void guild_storage_frombuffer(struct guild_storage *p, const unsigned char *buf)
{
	_guild_storage_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct gm_account {
	int account_id;
	int level;
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct party_member {
	int account_id;
	char name[24];
	char map[24];
	int leader;
	int online;
	unsigned short lv;
	struct map_session_data *sd;
};
extern inline void _party_member_tobuffer(const struct party_member *p, unsigned char **buf)
{	
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->account_id),		buf);
	_S_tobuffer(                  (p->name),			buf, 24);
	_S_tobuffer(                  (p->map),				buf, 24);
	_L_tobuffer( (unsigned long*) (&p->leader),			buf);
	_L_tobuffer( (unsigned long*) (&p->online),			buf);
	_W_tobuffer(                 (&p->lv),				buf);
	//_L_tobuffer( &(p->sd),				buf);
	(*buf)+=4;
	// skip the map_session_data *
}
extern inline void party_member_tobuffer(const struct party_member *p, unsigned char *buf)
{
	_party_member_tobuffer(p, &buf);
}
extern inline void _party_member_frombuffer(struct party_member *p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->account_id),	buf);
	_S_frombuffer(                  (p->name),			buf, 24);
	_S_frombuffer(                  (p->map),			buf, 24);
	_L_frombuffer( (unsigned long*) (&p->leader),		buf);
	_L_frombuffer( (unsigned long*) (&p->online),		buf);
	_W_frombuffer(                 (&p->lv),			buf);
	//_L_frombuffer( &(p->sd),			buf);
	(*buf)+=4; p->sd = NULL; 
	// skip the map_session_data *
}
extern inline void party_member_frombuffer(struct party_member *p, const unsigned char *buf)
{
	_party_member_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct party {
	int party_id;
	char name[24];
	int exp;
	int item;
	int itemc;
	struct party_member member[MAX_PARTY];
};
extern inline void _party_tobuffer(const struct party *p, unsigned char **buf)
{
	size_t i;
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->party_id),	buf);
	_S_tobuffer(                  (p->name),		buf, 24);
	_L_tobuffer( (unsigned long*) (&p->exp),			buf);
	_L_tobuffer( (unsigned long*) (&p->item),		buf);
	for(i=0; i< MAX_PARTY; i++)
		_party_member_tobuffer(p->member+i, buf);
}
extern inline void party_tobuffer(const struct party *p, unsigned char *buf)
{
	_party_tobuffer(p, &buf);
}
extern inline void _party_frombuffer(struct party *p, const unsigned char **buf)
{
	size_t i;
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->party_id),	buf);
	_S_frombuffer(                  (p->name),		buf, 24);
	_L_frombuffer( (unsigned long*) (&p->exp),		buf);
	_L_frombuffer( (unsigned long*) (&p->item),		buf);
	for(i=0; i< MAX_PARTY; i++)
		_party_member_frombuffer(p->member+i, buf);
}
extern inline void party_frombuffer(struct party *p, const unsigned char *buf)
{
	_party_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct guild_member {
	int account_id;
	int char_id;
	short hair;
	short hair_color;
	unsigned char gender;
	short class_;
	unsigned short lv;
	int exp;
	int exp_payper;
	short online;
	short position;
	int rsv1;
	int rsv2;
	char name[24];
	struct map_session_data *sd;
};
extern inline void _guild_member_tobuffer(const struct guild_member *p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->account_id),	buf);
	_L_tobuffer( (unsigned long*) (&p->char_id),		buf);
	_W_tobuffer( (unsigned short*)(&p->hair),		buf);
	_W_tobuffer( (unsigned short*)(&p->hair_color),	buf);
	_B_tobuffer(                 (&p->gender),		buf);
	_W_tobuffer( (unsigned short*)(&p->class_),		buf);
	_W_tobuffer(                 (&p->lv),			buf);
	_L_tobuffer( (unsigned long*) (&p->exp),			buf);
	_L_tobuffer( (unsigned long*) (&p->exp_payper),	buf);
	_W_tobuffer( (unsigned short*)(&p->online),		buf);
	_W_tobuffer( (unsigned short*)(&p->position),	buf);
	_L_tobuffer( (unsigned long*) (&p->rsv1),		buf);
	_L_tobuffer( (unsigned long*) (&p->rsv2),		buf);
	_S_tobuffer(                  (p->name),		buf, 24);
	//_L_tobuffer( &(p->sd),			buf);
	(*buf)+=4;
	// skip the struct map_session_data *
}
extern inline void guild_member_tobuffer(const struct guild_member *p, unsigned char *buf)
{
	_guild_member_tobuffer(p, &buf);
}
extern inline void _guild_member_frombuffer(struct guild_member *p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->account_id),	buf);
	_L_frombuffer( (unsigned long*) (&p->char_id),		buf);
	_W_frombuffer( (unsigned short*)(&p->hair),		buf);
	_W_frombuffer( (unsigned short*)(&p->hair_color),	buf);
	_B_frombuffer(                 (&p->gender),		buf);
	_W_frombuffer( (unsigned short*)(&p->class_),		buf);
	_W_frombuffer(                 (&p->lv),			buf);
	_L_frombuffer( (unsigned long*) (&p->exp),			buf);
	_L_frombuffer( (unsigned long*) (&p->exp_payper),	buf);
	_W_frombuffer( (unsigned short*)(&p->online),		buf);
	_W_frombuffer( (unsigned short*)(&p->position),	buf);
	_L_frombuffer( (unsigned long*) (&p->rsv1),		buf);
	_L_frombuffer( (unsigned long*) (&p->rsv2),		buf);
	_S_frombuffer(                  (p->name),		buf, 24);
	//_L_frombuffer( &(p->sd),		buf);
	(*buf)+=4; p->sd = NULL; 
	// skip the struct map_session_data *
}
extern inline void guild_member_frombuffer(struct guild_member *p, const unsigned char *buf)
{
	_guild_member_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct guild_position {
	char name[24];
	int mode;
	int exp_mode;
};
extern inline void _guild_position_tobuffer(const struct guild_position*p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_S_tobuffer(                  (p->name),		buf, 24);
	_L_tobuffer( (unsigned long*) (&p->mode),		buf);
	_L_tobuffer( (unsigned long*) (&p->exp_mode),	buf);
}
extern inline void guild_position_tobuffer(const struct guild_position*p, unsigned char *buf)
{
	_guild_position_tobuffer(p, &buf);
}
extern inline void _guild_position_frombuffer(struct guild_position*p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_S_frombuffer(                  (p->name),		buf, 24);
	_L_frombuffer( (unsigned long*) (&p->mode),		buf);
	_L_frombuffer( (unsigned long*) (&p->exp_mode),	buf);
}
extern inline void guild_position_frombuffer(struct guild_position*p, const unsigned char *buf)
{
	_guild_position_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct guild_alliance {
	int opposition;
	int guild_id;
	char name[24];
};
extern inline void _guild_alliance_tobuffer(const struct guild_alliance*p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->opposition),	buf);
	_L_tobuffer( (unsigned long*) (&p->guild_id),	buf);
	_S_tobuffer(                  (p->name),		buf, 24);
}
extern inline void guild_alliance_tobuffer(const struct guild_alliance*p, unsigned char *buf)
{
	_guild_alliance_tobuffer(p, &buf);
}
extern inline void _guild_alliance_frombuffer(struct guild_alliance*p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->opposition),buf);
	_L_frombuffer( (unsigned long*) (&p->guild_id),	buf);
	_S_frombuffer(                  (p->name),		buf, 24);
}
extern inline void guild_alliance_frombuffer(struct guild_alliance*p, const unsigned char *buf)
{
	_guild_alliance_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct guild_explusion {
	char name[24];
	char mes[40];
	char acc[40];
	int account_id;
	int rsv1;
	int rsv2;
	int rsv3;
};
extern inline void _guild_explusion_tobuffer(const struct guild_explusion*p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_S_tobuffer(                  (p->name),		buf, 24);
	_S_tobuffer(                  (p->mes),			buf, 40);
	_S_tobuffer(                  (p->acc),			buf, 40);
	_L_tobuffer( (unsigned long*) (&p->account_id),	buf);
	_L_tobuffer( (unsigned long*) (&p->rsv1),		buf);
	_L_tobuffer( (unsigned long*) (&p->rsv2),		buf);
	_L_tobuffer( (unsigned long*) (&p->rsv3),		buf);
}
extern inline void guild_explusion_tobuffer(const struct guild_explusion*p, unsigned char *buf)
{
	_guild_explusion_tobuffer(p, &buf);
}
extern inline void _guild_explusion_frombuffer(struct guild_explusion*p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_S_frombuffer(                  (p->name),		buf, 24);
	_S_frombuffer(                  (p->mes),		buf, 40);
	_S_frombuffer(                  (p->acc),		buf, 40);
	_L_frombuffer( (unsigned long*) (&p->account_id),buf);
	_L_frombuffer( (unsigned long*) (&p->rsv1),		buf);
	_L_frombuffer( (unsigned long*) (&p->rsv2),		buf);
	_L_frombuffer( (unsigned long*) (&p->rsv3),		buf);
}
extern inline void guild_explusion_frombuffer(struct guild_explusion*p, const unsigned char *buf)
{
	_guild_explusion_frombuffer(p, &buf);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct guild_skill {
	int id;
	int lv;
};
extern inline void _guild_skill_tobuffer(const struct guild_skill*p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->id),	buf);
	_L_tobuffer( (unsigned long*) (&p->lv),	buf);
}
extern inline void guild_skill_tobuffer(const struct guild_skill*p, unsigned char *buf)
{
	_guild_skill_tobuffer(p, &buf);
}
extern inline void _guild_skill_frombuffer(struct guild_skill*p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->id),buf);
	_L_frombuffer( (unsigned long*) (&p->lv),buf);
}
extern inline void guild_skill_frombuffer(struct guild_skill*p, const unsigned char *buf)
{
	_guild_skill_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct guild {
	int guild_id;
	unsigned short guild_lv;
	unsigned short connect_member;
	unsigned short max_member; 
	unsigned short average_lv;
	int exp;
	int next_exp;
	int skill_point;
	int castle_id;
	char name[24];
	char master[24];
	struct guild_member member[MAX_GUILD];
	struct guild_position position[MAX_GUILDPOSITION];
	char mes1[60];
	char mes2[120];
	int emblem_len;
	int emblem_id;
	char emblem_data[2048];
	struct guild_alliance alliance[MAX_GUILDALLIANCE];
	struct guild_explusion explusion[MAX_GUILDEXPLUSION];
	struct guild_skill skill[MAX_GUILDSKILL];
};
extern inline void _guild_tobuffer(const struct guild* p, unsigned char **buf)
{
	size_t i;
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->guild_id),		buf);
	_W_tobuffer(                 (&p->guild_lv),		buf);
	_W_tobuffer(                 (&p->connect_member),	buf);
	_W_tobuffer(                 (&p->max_member),		buf);
	_W_tobuffer(                 (&p->average_lv),		buf);
	_L_tobuffer( (unsigned long*) (&p->exp),				buf);
	_L_tobuffer( (unsigned long*) (&p->next_exp),		buf);
	_L_tobuffer( (unsigned long*) (&p->skill_point),		buf);
	_L_tobuffer( (unsigned long*) (&p->castle_id),		buf);
	_S_tobuffer(                  (p->name),			buf, 24);
	_S_tobuffer(                  (p->master),			buf, 24);
	for(i=0; i< MAX_GUILD; i++)
		_guild_member_tobuffer(p->member+i,		buf);
	for(i=0; i< MAX_GUILDPOSITION; i++)
		_guild_position_tobuffer(p->position+i,	buf);
	_S_tobuffer(                  (p->mes1),			buf, 60);
	_S_tobuffer(                  (p->mes2),			buf, 120);
	_L_tobuffer( (unsigned long*) (&p->emblem_len),		buf);
	_L_tobuffer( (unsigned long*) (&p->emblem_id),		buf);
	_S_tobuffer(                  (p->emblem_data),		buf, 2048);
	for(i=0; i< MAX_GUILDALLIANCE; i++)
		_guild_alliance_tobuffer(p->alliance+i,	buf);
	for(i=0; i< MAX_GUILDEXPLUSION; i++)
		_guild_explusion_tobuffer(p->explusion+i,buf);
	for(i=0; i< MAX_GUILDSKILL; i++)
		_guild_skill_tobuffer(p->skill+i,		buf);
}
extern inline void guild_tobuffer(const struct guild* p, unsigned char *buf)
{
	_guild_tobuffer(p, &buf);
}
extern inline void _guild_frombuffer(struct guild* p, const unsigned char **buf)
{
	size_t i;
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->guild_id),		buf);
	_W_frombuffer(                 (&p->guild_lv),		buf);
	_W_frombuffer(                 (&p->connect_member),	buf);
	_W_frombuffer(                 (&p->max_member),		buf);
	_W_frombuffer(                 (&p->average_lv),		buf);
	_L_frombuffer( (unsigned long*) (&p->exp),				buf);
	_L_frombuffer( (unsigned long*) (&p->next_exp),		buf);
	_L_frombuffer( (unsigned long*) (&p->skill_point),		buf);
	_L_frombuffer( (unsigned long*) (&p->castle_id),		buf);
	_S_frombuffer(                  (p->name),			buf, 24);
	_S_frombuffer(                  (p->master),			buf, 24);
	for(i=0; i< MAX_GUILD; i++)
		_guild_member_frombuffer(p->member+i,		buf);
	for(i=0; i< MAX_GUILDPOSITION; i++)
		_guild_position_frombuffer(p->position+i,	buf);
	_S_frombuffer(                  (p->mes1),			buf, 60);
	_S_frombuffer(                  (p->mes2),			buf, 120);
	_L_frombuffer( (unsigned long*) (&p->emblem_len),		buf);
	_L_frombuffer( (unsigned long*) (&p->emblem_id),		buf);
	_S_frombuffer(                  (p->emblem_data),		buf, 2048);
	for(i=0; i< MAX_GUILDALLIANCE; i++)
		_guild_alliance_frombuffer(p->alliance+i,	buf);
	for(i=0; i< MAX_GUILDEXPLUSION; i++)
		_guild_explusion_frombuffer(p->explusion+i,buf);
	for(i=0; i< MAX_GUILDSKILL; i++)
		_guild_skill_frombuffer(p->skill+i,		buf);
}
extern inline void guild_frombuffer(struct guild* p, const unsigned char *buf)
{
	_guild_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct guild_castle {
	int castle_id;
	char map_name[24];
	char castle_name[24];
	char castle_event[24];
	int guild_id;
	int economy;
	int defense;
	int triggerE;
	int triggerD;
	int nextTime;
	int payTime;
	int createTime;
	int visibleC;
	int visibleG0;
	int visibleG1;
	int visibleG2;
	int visibleG3;
	int visibleG4;
	int visibleG5;
	int visibleG6;
	int visibleG7;
	int Ghp0;	// added Guardian HP [Valaris]
	int Ghp1;
	int Ghp2;
	int Ghp3;
	int Ghp4;
	int Ghp5;
	int Ghp6;
	int Ghp7;	
	int GID0;	
	int GID1;
	int GID2;
	int GID3;
	int GID4;
	int GID5;
	int GID6;
	int GID7;	// end addition [Valaris]
};
extern inline void _guild_castle_tobuffer(const struct guild_castle* p, unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_tobuffer( (unsigned long*) (&p->castle_id),	buf);
	_S_tobuffer(                  (p->map_name),	buf, 24);
	_S_tobuffer(                  (p->castle_name),	buf, 24);
	_S_tobuffer(                  (p->castle_event),buf, 24);
	_L_tobuffer( (unsigned long*) (&p->guild_id),	buf);
	_L_tobuffer( (unsigned long*) (&p->economy),		buf);
	_L_tobuffer( (unsigned long*) (&p->defense),		buf);
	_L_tobuffer( (unsigned long*) (&p->triggerE),	buf);
	_L_tobuffer( (unsigned long*) (&p->triggerD),	buf);
	_L_tobuffer( (unsigned long*) (&p->nextTime),	buf);
	_L_tobuffer( (unsigned long*) (&p->payTime),		buf);
	_L_tobuffer( (unsigned long*) (&p->createTime),	buf);
	_L_tobuffer( (unsigned long*) (&p->visibleC),	buf);
	_L_tobuffer( (unsigned long*) (&p->visibleG0),	buf);
	_L_tobuffer( (unsigned long*) (&p->visibleG1),	buf);
	_L_tobuffer( (unsigned long*) (&p->visibleG2),	buf);
	_L_tobuffer( (unsigned long*) (&p->visibleG3),	buf);
	_L_tobuffer( (unsigned long*) (&p->visibleG4),	buf);
	_L_tobuffer( (unsigned long*) (&p->visibleG5),	buf);
	_L_tobuffer( (unsigned long*) (&p->visibleG6),	buf);
	_L_tobuffer( (unsigned long*) (&p->visibleG7),	buf);
	_L_tobuffer( (unsigned long*) (&p->Ghp0),		buf);
	_L_tobuffer( (unsigned long*) (&p->Ghp1),		buf);
	_L_tobuffer( (unsigned long*) (&p->Ghp2),		buf);
	_L_tobuffer( (unsigned long*) (&p->Ghp3),		buf);
	_L_tobuffer( (unsigned long*) (&p->Ghp4),		buf);
	_L_tobuffer( (unsigned long*) (&p->Ghp5),		buf);
	_L_tobuffer( (unsigned long*) (&p->Ghp6),		buf);
	_L_tobuffer( (unsigned long*) (&p->Ghp7),		buf);
	_L_tobuffer( (unsigned long*) (&p->GID0),		buf);
	_L_tobuffer( (unsigned long*) (&p->GID1),		buf);
	_L_tobuffer( (unsigned long*) (&p->GID2),		buf);
	_L_tobuffer( (unsigned long*) (&p->GID3),		buf);
	_L_tobuffer( (unsigned long*) (&p->GID4),		buf);
	_L_tobuffer( (unsigned long*) (&p->GID5),		buf);
	_L_tobuffer( (unsigned long*) (&p->GID6),		buf);
	_L_tobuffer( (unsigned long*) (&p->GID7),		buf);
}
extern inline void guild_castle_tobuffer(const struct guild_castle* p, unsigned char *buf)
{
	_guild_castle_tobuffer(p, &buf);
}
extern inline void _guild_castle_frombuffer(struct guild_castle* p, const unsigned char **buf)
{
	if( NULL==p || NULL==buf)	return;
	_L_frombuffer( (unsigned long*) (&p->castle_id),	buf);
	_S_frombuffer(                  (p->map_name),	buf, 24);
	_S_frombuffer(                  (p->castle_name),	buf, 24);
	_S_frombuffer(                  (p->castle_event),buf, 24);
	_L_frombuffer( (unsigned long*) (&p->guild_id),	buf);
	_L_frombuffer( (unsigned long*) (&p->economy),		buf);
	_L_frombuffer( (unsigned long*) (&p->defense),		buf);
	_L_frombuffer( (unsigned long*) (&p->triggerE),	buf);
	_L_frombuffer( (unsigned long*) (&p->triggerD),	buf);
	_L_frombuffer( (unsigned long*) (&p->nextTime),	buf);
	_L_frombuffer( (unsigned long*) (&p->payTime),		buf);
	_L_frombuffer( (unsigned long*) (&p->createTime),	buf);
	_L_frombuffer( (unsigned long*) (&p->visibleC),	buf);
	_L_frombuffer( (unsigned long*) (&p->visibleG0),	buf);
	_L_frombuffer( (unsigned long*) (&p->visibleG1),	buf);
	_L_frombuffer( (unsigned long*) (&p->visibleG2),	buf);
	_L_frombuffer( (unsigned long*) (&p->visibleG3),	buf);
	_L_frombuffer( (unsigned long*) (&p->visibleG4),	buf);
	_L_frombuffer( (unsigned long*) (&p->visibleG5),	buf);
	_L_frombuffer( (unsigned long*) (&p->visibleG6),	buf);
	_L_frombuffer( (unsigned long*) (&p->visibleG7),	buf);
	_L_frombuffer( (unsigned long*) (&p->Ghp0),		buf);
	_L_frombuffer( (unsigned long*) (&p->Ghp1),		buf);
	_L_frombuffer( (unsigned long*) (&p->Ghp2),		buf);
	_L_frombuffer( (unsigned long*) (&p->Ghp3),		buf);
	_L_frombuffer( (unsigned long*) (&p->Ghp4),		buf);
	_L_frombuffer( (unsigned long*) (&p->Ghp5),		buf);
	_L_frombuffer( (unsigned long*) (&p->Ghp6),		buf);
	_L_frombuffer( (unsigned long*) (&p->Ghp7),		buf);
	_L_frombuffer( (unsigned long*) (&p->GID0),		buf);
	_L_frombuffer( (unsigned long*) (&p->GID1),		buf);
	_L_frombuffer( (unsigned long*) (&p->GID2),		buf);
	_L_frombuffer( (unsigned long*) (&p->GID3),		buf);
	_L_frombuffer( (unsigned long*) (&p->GID4),		buf);
	_L_frombuffer( (unsigned long*) (&p->GID5),		buf);
	_L_frombuffer( (unsigned long*) (&p->GID6),		buf);
	_L_frombuffer( (unsigned long*) (&p->GID7),		buf);
}
extern inline void guild_castle_frombuffer(struct guild_castle* p, const unsigned char *buf)
{
	_guild_castle_frombuffer(p, &buf);
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
struct square {
	int val1[5];
	int val2[5];
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



#endif	// _MMO_H_
