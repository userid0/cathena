// $Id: mmo.h,v 1.3 2004/09/25 20:12:25 PoW Exp $
// Original : mmo.h 2003/03/14 12:07:02 Rev.1.7

#ifndef	_MMO_H_
#define	_MMO_H_

#include <time.h>
#include "utils.h" // _WIN32

#if ! defined(Assert)
#if defined(RELEASE)
#define Assert(EX)
#else
// extern "C" {
#include <assert.h>
// }
#ifndef DEFCPP
#if defined(_WIN32)
#include <crtdbg.h>
#endif
#endif
#define Assert(EX) assert(EX)
#endif
#endif /* ! defined(Assert) */

#ifdef CYGWIN
// txtやlogなどの書き出すファイルの改行コード
#define RETCODE	"\r\n"	// (CR/LF：Windows系)
#else
#define RETCODE "\n"	// (LF：Unix系）
#endif

#define RET RETCODE

#define FIFOSIZE_SERVERLINK	128*1024

// set to 0 to not check IP of player between each server.
// set to another value if you want to check (1)
#define CMP_AUTHFIFO_IP 1

#define CMP_AUTHFIFO_LOGIN2 1

#define MAX_MAP_PER_SERVER 512
#define MAX_INVENTORY 100
#define MAX_AMOUNT 30000
#define MAX_ZENY 1000000000	// 1G zeny
#define MAX_FAME 1000000000 // 1G fame point [DracoRPG]
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

//For character names, title names, guilds, maps, etc.
//Includes null-terminator as it is the length of the array.
#define NAME_LENGTH 24
//For item names, which tend to have much longer names.
#define ITEM_NAME_LENGTH 24
//For Map Names, which the client considers to be 16 in length
#define MAP_NAME_LENGTH 16

//..
#define MAX_FRIENDS 20
#define MAX_MEMOPOINTS 10



#define CHAR_CONF_NAME  "conf/char_athena.conf"

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

struct point{
	char map[MAP_NAME_LENGTH];
	short x,y;
};

struct skill {
	unsigned short id,lv,flag;
};

struct global_reg {
	char str[32];
	int value;
};

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
	char name[NAME_LENGTH];
	char rename_flag;
	char incuvate;
};

struct friend {
	int friend_id;
	char friend_name[NAME_LENGTH];
};

struct mmo_charstatus {
	int char_id;
	int account_id;
	int partner_id;
	int father;
	int mother;
	int child;

	int base_exp,job_exp,zeny;

	short class_;
	short status_point,skill_point;
	int hp,max_hp,sp,max_sp;
	short option,karma,manner;
	short hair,hair_color,clothes_color;
	int party_id,guild_id,pet_id;
	int fame;

	short weapon,shield;
	short head_top,head_mid,head_bottom;

	char name[NAME_LENGTH];
	unsigned int base_level,job_level;
	short str,agi,vit,int_,dex,luk;
	unsigned char char_num,sex;

	unsigned long mapip;
	unsigned int mapport;

	struct point last_point,save_point,memo_point[MAX_MEMOPOINTS];
	struct item inventory[MAX_INVENTORY],cart[MAX_CART];
	struct skill skill[MAX_SKILL];
	int global_reg_num;
	struct global_reg global_reg[GLOBAL_REG_NUM];
	int account_reg_num;
	struct global_reg account_reg[ACCOUNT_REG_NUM];
	int account_reg2_num;
	struct global_reg account_reg2[ACCOUNT_REG2_NUM];


         //struct friend friends[MAX_FRIENDS];
	// Friends list vars
	int friend_id[MAX_FRIENDS];
	char friend_name[MAX_FRIENDS][NAME_LENGTH];
};

struct storage {
	int dirty;
	int account_id;
	short storage_status;
	short storage_amount;
	struct item storage_[MAX_STORAGE];
};

struct guild_storage {
	int guild_id;
	short storage_status;
	short storage_amount;
	struct item storage_[MAX_GUILD_STORAGE];
};

struct map_session_data;

struct gm_account {
	int account_id;
	int level;
};

struct party_member {
	int account_id;
	char name[NAME_LENGTH],map[MAP_NAME_LENGTH];
	int leader,online,lv;
	struct map_session_data *sd;
};

struct party {
	int party_id;
	char name[NAME_LENGTH];
	int exp;
	int item, itemc;
	struct party_member member[MAX_PARTY];
};

struct guild_member {
	int account_id, char_id;
	short hair,hair_color,gender,class_,lv;
	int exp,exp_payper;
	short online,position;
	int rsv1,rsv2;
	char name[NAME_LENGTH];
	struct map_session_data *sd;
};

struct guild_position {
	char name[NAME_LENGTH];
	int mode;
	int exp_mode;
};

struct guild_alliance {
	int opposition;
	int guild_id;
	char name[NAME_LENGTH];
};

struct guild_explusion {
	char name[NAME_LENGTH];
	char mes[40];
	char acc[40];
	int account_id;
	int rsv1,rsv2,rsv3;
};

struct guild_skill {
	int id,lv;
};

struct guild {
	int guild_id;
	short guild_lv, connect_member, max_member, average_lv;
	int exp,next_exp,skill_point,castle_id;
	char name[NAME_LENGTH],master[NAME_LENGTH];
	struct guild_member member[MAX_GUILD];
	struct guild_position position[MAX_GUILDPOSITION];
	char mes1[60],mes2[120];
	int emblem_len,emblem_id;
	char emblem_data[2048];
	struct guild_alliance alliance[MAX_GUILDALLIANCE];
	struct guild_explusion explusion[MAX_GUILDEXPLUSION];
	struct guild_skill skill[MAX_GUILDSKILL];
#ifndef TXT_ONLY
	unsigned char save_flag;
	int save_timer;
#endif
};

struct guild_castle {
	int castle_id;
	char map_name[MAP_NAME_LENGTH];
	char castle_name[NAME_LENGTH];
	char castle_event[NAME_LENGTH];
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
struct square {
	int val1[5];
	int val2[5];
};

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
	GD_KAFRACONTRACT=10001,
	GD_GUARDIANRESEARCH=10002,
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

#ifndef __WIN32
	#ifndef strcmpi
		#define strcmpi strcasecmp
	#endif
	#ifndef stricmp
		#define stricmp strcasecmp
	#endif
	#ifndef strncmpi
		#define strncmpi strncasecmp
	#endif
	#ifndef strnicmp
		#define strnicmp strncasecmp
	#endif
#else
	#define snprintf _snprintf
	#define vsnprintf _vsnprintf
	#ifndef strncmpi
		#define strncmpi strnicmp
	#endif
#endif

#endif	// _MMO_H_
