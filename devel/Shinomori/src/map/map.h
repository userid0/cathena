// $Id: map.h,v 1.8 2004/09/25 11:39:17 MouseJstr Exp $
#ifndef _MAP_H_
#define _MAP_H_

#include <stdarg.h>
#include "mmo.h"

#define MAX_PC_CLASS (1+6+6+1+6+1+1+1+1+4023)
#define PC_CLASS_BASE 0
#define PC_CLASS_BASE2 (PC_CLASS_BASE + 4001)
#define PC_CLASS_BASE3 (PC_CLASS_BASE2 + 22)
#define MAX_NPC_PER_MAP 512
#define BLOCK_SIZE 8 // Never zero
#define AREA_SIZE battle_config.area_size
#define LOCAL_REG_NUM 16
#define LIFETIME_FLOORITEM 60
#define DAMAGELOG_SIZE 30
#define LOOTITEM_SIZE 10
#define MAX_SKILL_LEVEL 100
#define MAX_STATUSCHANGE 210
#define MAX_SKILLUNITGROUP 32
#define MAX_MOBSKILLUNITGROUP 8
#define MAX_SKILLUNITGROUPTICKSET 32
#define MAX_SKILLTIMERSKILL 32
#define MAX_MOBSKILLTIMERSKILL 10
#define MAX_MOBSKILL 32
#define MAX_EVENTQUEUE 2
#define MAX_EVENTTIMER 32
#define NATURAL_HEAL_INTERVAL 500
#define MAX_FLOORITEM 500000
#define MAX_LEVEL 255
#define MAX_WALKPATH 32
#define MAX_DROP_PER_MAP 48
#define MAX_IGNORE_LIST 80
#define MAX_VENDING 12

#define DEFAULT_AUTOSAVE_INTERVAL 60*1000

#define OPTION_HIDE 0x40

enum { BL_NUL, BL_PC, BL_NPC, BL_MOB, BL_ITEM, BL_CHAT, BL_SKILL , BL_PET };
enum { WARP, SHOP, SCRIPT, MONS };

struct block_list {
	struct block_list *next,*prev;
	int id;
	short m;
	short x;
	short y;
	unsigned char type;
	unsigned char subtype;
};

struct walkpath_data {
	unsigned char path_len;
	unsigned char path_pos;
	unsigned char path_half;
	unsigned char path[MAX_WALKPATH];
};
struct shootpath_data {
	int rx,ry,len;
	int x[MAX_WALKPATH];
	int y[MAX_WALKPATH];
};

struct script_reg {
	int index;
	int data;
};
struct script_regstr {
	int index;
	char data[256];
};
struct status_change {
	int timer;
	int val1;
	int val2;
	int val3;
	int val4;
};
struct vending {
	short index;
	unsigned short amount;
	unsigned int value;
};

struct skill_unit_group;
struct skill_unit {
	struct block_list bl;

	struct skill_unit_group *group;

	int limit;
	int val1;
	int val2;
	short alive;
	short range;
};
struct skill_unit_group {
	int src_id;
	int party_id;
	int guild_id;
	int map;
	int target_flag;
	unsigned long tick;
	int limit;
	int interval;

	unsigned short skill_id;
	unsigned short skill_lv;
	int val1;
	int val2;
	int val3;
	char *valstr;
	int unit_id;
	int group_id;
	int unit_count,alive_count;
	struct skill_unit *unit;
};
struct skill_unit_group_tickset {
	unsigned long tick;
	int id;
};
struct skill_timerskill {
	int timer;
	int src_id;
	int target_id;
	int map;
	short x;
	short y;
	unsigned short skill_id;
	unsigned short skill_lv;
	int type;
	int flag;
};

struct npc_data;
struct pet_db;
struct item_data;
struct square;

struct map_session_data {
	struct block_list bl;
	struct {
		unsigned auth : 1;							// 0
		unsigned change_walk_target : 1;			// 1
		unsigned attack_continue : 1;				// 2
		unsigned menu_or_input : 1;					// 3
		unsigned dead_sit : 2;						// 4,5
		unsigned skillcastcancel : 1;				// 6
		unsigned waitingdisconnect : 1;				// 7 - byte 1
		unsigned lr_flag : 2;						// 8,9 
		unsigned connect_new : 1;					// 10
		unsigned arrow_atk : 1;						// 11
		unsigned attack_type : 3;					// 12,13,14
		unsigned skill_flag : 1;					// 15 - byte 2
		unsigned gangsterparadise : 1;				// 16 
		unsigned produce_flag : 1;					// 17
		unsigned make_arrow_flag : 1;				// 18
		unsigned potionpitcher_flag : 1;			// 19
		unsigned storage_flag : 1;					// 20
		unsigned snovice_flag : 4;					// 21,22,23,24 - byte 3

		// originally by Qamera, adapted by celest
		unsigned event_death : 1;					// 25
		unsigned event_kill : 1;					// 26
		unsigned event_disconnect : 1;				// 27
		unsigned event_onconnect : 1;				// 28

		unsigned killer : 1;						// 29
		unsigned killable : 1;						// 30
		unsigned restart_full_recover : 1;			// 31 - byte 4
		unsigned no_castcancel : 1;					// 32
		unsigned no_castcancel2 : 1;				// 33
		unsigned no_sizefix : 1;					// 34
		unsigned no_magic_damage : 1;				// 35
		unsigned no_weapon_damage : 1;				// 36
		unsigned no_gemstone : 1;					// 37
		unsigned infinite_endure : 1;				// 38
		unsigned autoloot : 1; //by Upa-Kun			// 39  - byte 5
													// 0 bit left
	} state;

	int gmaster_flag;
	int char_id;
	int login_id1;
	int login_id2;
	int sex;
	int packet_ver;  // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
	struct mmo_charstatus status;
	struct item_data *inventory_data[MAX_INVENTORY];
	short equip_index[11];
	unsigned short unbreakable_equip;
	unsigned short unbreakable;	// chance to prevent equipment breaking [celest]
	int weight;
	int max_weight;
	int cart_weight;
	int cart_max_weight;
	int cart_num;
	int cart_max_num;
	char mapname[24];
	int fd;
	int new_fd;
	short to_x;
	short to_y;
	short speed;
	short prev_speed;
	short opt1;
	short opt2;
	short opt3;
	char dir;
	char head_dir;
	unsigned long client_tick;
	unsigned long server_tick;
	struct walkpath_data walkpath;
	int walktimer;
	int next_walktime;
	int npc_id;
	int areanpc_id;
	int npc_shopid;
	int npc_pos;
	int npc_menu;
	int npc_amount;
	int npc_stack;
	int npc_stackmax;
	char *npc_script;
	char *npc_scriptroot;
	char *npc_stackbuf;
	char npc_str[256];
	unsigned int chatID;
	time_t idletime;

	struct{
		char name[24];
	} ignore[MAX_IGNORE_LIST];
	int ignoreAll;

	int attacktimer;
	int attacktarget;
	short attacktarget_lv;
	unsigned int attackabletime;
	
	int followtimer; // [MouseJstr]
	int followtarget;

	time_t emotionlasttime; // to limit flood with emotion packets

	short attackrange;
	short attackrange_;
	int skilltimer;
	int skilltarget;
	short skillx;
	short skilly;
	short skillid;
	short skilllv;
	short skillitem;
	short skillitemlv;
	short skillid_old;
	short skilllv_old;
	short skillid_dance;
	short skilllv_dance;
	struct skill_unit_group skillunit[MAX_SKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	struct skill_timerskill skilltimerskill[MAX_SKILLTIMERSKILL];
	char blockskill[MAX_SKILL];	// [celest]	
	unsigned short cloneskill_id;
	unsigned short cloneskill_lv;
	int potion_hp;
	int potion_sp;
	int potion_per_hp;
	int potion_per_sp;

	int invincible_timer;
	unsigned long canact_tick;
	unsigned long canmove_tick;
	unsigned long canlog_tick;
	unsigned long canregen_tick;
	unsigned long hp_sub;
	unsigned long sp_sub;
	unsigned long inchealhptick;
	unsigned long inchealsptick;
	unsigned long inchealspirithptick;
	unsigned long inchealspiritsptick;
// -- moonsoul (new tick for berserk self-damage)
//	unsigned long berserkdamagetick;
	int fame;

	short view_class;
	short weapontype1,weapontype2;
	short disguiseflag,disguise; // [Valaris]
	int paramb[6];
	int paramc[6];
	int parame[6];
	int paramcard[6];
	int hit;
	int flee;
	int flee2;
	int aspd;
	int amotion;
	int dmotion;
	int watk;
	int watk2;
	int atkmods[3];
	int def;
	int def2;
	int mdef;
	int mdef2;
	int critical;
	int matk1;
	int matk2;
	int atk_ele;
	int def_ele;
	int star;
	int overrefine;
	int castrate;
	int delayrate;
	int hprate;
	int sprate;
	int dsprate;
	int addele[10];
	int addrace[12];
	int addsize[3];
	int subele[10];
	int subrace[12];
	int addeff[10];
	int addeff2[10];
	int reseff[10];
	int watk_;
	int watk_2;
	int atkmods_[3];
	int addele_[10];
	int addrace_[12];
	int addsize_[3];	//ìÒìÅó¨ÇÃÇΩÇﬂÇ…í«â¡
	int atk_ele_;
	int star_;
	int overrefine_;				//ìÒìÅó¨ÇÃÇΩÇﬂÇ…í«â¡
	int base_atk;
	int atk_rate;
	int weapon_atk[16];
	int weapon_atk_rate[16];
	int arrow_atk;
	int arrow_ele;
	int arrow_cri;
	int arrow_hit;
	int arrow_range;
	int arrow_addele[10],arrow_addrace[12],arrow_addsize[3],arrow_addeff[10],arrow_addeff2[10];
	int nhealhp,nhealsp,nshealhp,nshealsp,nsshealhp,nsshealsp;
	int aspd_rate,speed_rate,hprecov_rate,sprecov_rate,critical_def,double_rate;
	int near_attack_def_rate,long_attack_def_rate,magic_def_rate,misc_def_rate;
	int matk_rate,ignore_def_ele,ignore_def_race,ignore_def_ele_,ignore_def_race_;
	int ignore_mdef_ele,ignore_mdef_race;
	int magic_addele[10],magic_addrace[12],magic_subrace[12];
	int perfect_hit,get_zeny_num;
	int critical_rate,hit_rate,flee_rate,flee2_rate,def_rate,def2_rate,mdef_rate,mdef2_rate;
	int def_ratio_atk_ele,def_ratio_atk_ele_,def_ratio_atk_race,def_ratio_atk_race_;
	int add_damage_class_count,add_damage_class_count_,add_magic_damage_class_count;
	short add_damage_classid[10],add_damage_classid_[10],add_magic_damage_classid[10];
	int add_damage_classrate[10],add_damage_classrate_[10],add_magic_damage_classrate[10];
	short add_def_class_count,add_mdef_class_count;
	short add_def_classid[10],add_mdef_classid[10];
	int add_def_classrate[10],add_mdef_classrate[10];
	short monster_drop_item_count;
	short monster_drop_itemid[10];
	int monster_drop_race[10];
	int monster_drop_itemrate[10];
	int double_add_rate;
	int speed_add_rate;
	int aspd_add_rate;
	int perfect_hit_add;
	int get_zeny_add_num;
	short splash_range;
	short splash_add_range;
	unsigned short autospell_id;
	unsigned short autospell_lv;
	short autospell_rate;
	short hp_drain_rate;
	short hp_drain_per;
	short sp_drain_rate;
	short sp_drain_per;
	short hp_drain_rate_;
	short hp_drain_per_;
	short sp_drain_rate_;
	short sp_drain_per_;
	short hp_drain_value;
	short sp_drain_value;
	short hp_drain_value_;
	short sp_drain_value_;
	int short_weapon_damage_return;
	int long_weapon_damage_return;
	int weapon_coma_ele[10];
	int weapon_coma_race[12];
	short break_weapon_rate;
	short break_armor_rate;
	short add_steal_rate;
	//--- 02/15's new card effects [celest]
	int crit_atk_rate;
	int critaddrace[12];
	short no_regen;
	int addeff3[10];
	short autospell2_id,autospell2_lv,autospell2_rate,autospell2_type;
	int skillatk[2];
	unsigned short unstripable_equip;
	short add_damage_classid2[10],add_damage_class_count2;
	int add_damage_classrate2[10];
	short sp_gain_value, hp_gain_value;
	short sp_drain_type;
	short ignore_def_mob, ignore_def_mob_;
	unsigned long hp_loss_tick;
	int hp_loss_rate;
	short hp_loss_value, hp_loss_type;
	int addrace2[6],addrace2_[6];
	int subsize[3];
	short unequip_losehp[11];
	short unequip_losesp[11];
	int itemid;
	int itemhealrate[6];
	//--- 03/15's new card effects
	int expaddrace[6];
	int subrace2[6];
	short sp_gain_race[6];

	short spiritball;
	short spiritball_old;
	int spirit_timer[MAX_SKILL_LEVEL];
	int magic_damage_return; // AppleGirl Was Here
	int random_attack_increase_add;
	int random_attack_increase_per; // [Valaris]
	int perfect_hiding; // [Valaris]
	int classchange; // [Valaris]

	int die_counter;
	short doridori_counter;

	int reg_num;
	struct script_reg *reg;
	int regstr_num;
	struct script_regstr *regstr;

	struct status_change sc_data[MAX_STATUSCHANGE];
//!!	short sc_count;
	struct square dev;

	int trade_partner;
	int deal_item_index[10];
	int deal_item_amount[10];
	int deal_zeny;
	short deal_locked;

	int party_sended;
	int party_invite;
	int party_invite_account;
	int party_hp;
	int party_x;
	int party_y;

	int guild_sended;
	int guild_invite;
	int guild_invite_account;
	int guild_emblem_id;
	int guild_alliance;
	int guild_alliance_account;
	int guildspy; // [Syrus22]
	int partyspy; // [Syrus22]

	int vender_id;
	int vend_num;
	char message[80];
	struct vending vending[MAX_VENDING];

	int catch_target_class;
	struct s_pet pet;
	struct pet_db *petDB;
	struct pet_data *pd;
	int pet_hungry_timer;

	int pvp_point;
	int pvp_rank;
	int pvp_timer;
	int pvp_lastusers;

	char eventqueue[MAX_EVENTQUEUE][50];
	int eventtimer[MAX_EVENTTIMER];
	unsigned short eventcount; // [celest]

	unsigned short change_level;	// [celest]

#ifndef TXT_ONLY
	int mail_counter;	// mail counter for mail system [Valaris]
#endif

};

struct npc_timerevent_list {
	int timer;
	int pos;
};
struct npc_label_list {
	char labelname[24];
	int pos;
};
struct npc_item_list {
	int nameid;
	int value;
};
struct npc_reference{
	char *script;
	struct npc_label_list *label_list;
	int label_list_num;	
	size_t refcnt;			//reference counter
};
struct npc_data {
	struct block_list bl;
	short n;
	short class_;
	short dir;
	short speed;
	char name[24];
	char exname[24];
	int chat_id;
	short opt1;
	short opt2;
	short opt3;
	short option;
	short flag;
	int walktimer; // [Valaris]
	short to_x;
	short to_y; // [Valaris]
	struct walkpath_data walkpath;
	unsigned long next_walktime;
	unsigned long canmove_tick;

	struct { // [Valaris]
		unsigned state : 8;
		unsigned change_walk_target : 1;
		unsigned walk_easy : 1;
	} state;

	union {
		struct {
			struct npc_reference *ref; // char pointer with reference counter
			short xs;
			short ys;
			int guild_id;
			int timer;
			int timerid;
			int timeramount;
			int nexttimer;
			int rid;
			unsigned long timertick;
			struct npc_timerevent_list *timer_event;
		} scr;
		struct npc_item_list shop_item[1];
		struct {
			short xs;
			short ys;
			short x;
			short y;
			char name[16];
		} warp;
	} u;
	// Ç±Ç±Ç…ÉÅÉìÉoÇí«â¡ÇµÇƒÇÕÇ»ÇÁÇ»Ç¢(shop_itemÇ™â¬ïœí∑ÇÃà◊)

//	char eventqueue[MAX_EVENTQUEUE][50];
//	int eventtimer[MAX_EVENTTIMER];
	short arenaflag;

	void *chatdb;
};

struct mob_data {
	struct block_list bl;
	short n;
	short base_class;
	short class_;
	short dir;
	short mode;
	short level;
	short m;
	short x0;
	short y0;
	short xs;
	short ys;
	char name[24];
	int spawndelay1;
	int spawndelay2;
	struct {
		unsigned state : 8;
		unsigned skillstate : 8;
		unsigned targettype : 1;
		unsigned steal_flag : 1;
		unsigned steal_coin_flag : 1;
		unsigned skillcastcancel : 1;
		unsigned master_check : 1;
		unsigned change_walk_target : 1;
		unsigned walk_easy : 1;
		unsigned special_mob_ai : 3;
		unsigned soul_change_flag : 1; // Celest
		int provoke_flag; // Celest
	} state;
	int timer;
	short to_x;
	short to_y;
	short target_dir;
	short speed;
	int hp;
	int target_id;
	int attacked_id;
	short target_lv;
	struct walkpath_data walkpath;
	unsigned long next_walktime;
	unsigned long attackabletime;
	unsigned long last_deadtime;
	unsigned long last_spawntime;
	unsigned long last_thinktime;
	unsigned long canmove_tick;
	short move_fail_count;
	struct {
		int id;
		int dmg;
	} dmglog[DAMAGELOG_SIZE];
	struct item *lootitem;
	short lootitem_count;

	struct status_change sc_data[MAX_STATUSCHANGE];
//!!	short sc_count;
	short opt1;
	short opt2;
	short opt3;
	short option;
	short min_chase;
	int guild_id;
	int deletetimer;

	int skilltimer;
	int skilltarget;
	short skillx;
	short skilly;
	short skillid;
	short skilllv;
	short skillidx;
	unsigned int skilldelay[MAX_MOBSKILL];
	int def_ele;
	int master_id;
	int master_dist;
	int exclusion_src;
	int exclusion_party;
	int exclusion_guild;
	struct skill_timerskill skilltimerskill[MAX_MOBSKILLTIMERSKILL];
	struct skill_unit_group skillunit[MAX_MOBSKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	char npc_event[50];
	unsigned char size;
	int owner;
};

struct pet_data {
	struct block_list bl;
	short n;
	short class_;
	short dir;
	short speed;
	char name[24];
	struct {
		unsigned state : 8 ;
		unsigned skillstate : 8 ;
		unsigned change_walk_target : 1 ;
		short skillbonus;
	} state;
	int timer;
	short to_x;
	short to_y;
	short equip;
	struct walkpath_data walkpath;
	int target_id;
	short target_lv;
	int move_fail_count;
	unsigned int attackabletime;
	unsigned int next_walktime;
	unsigned int last_thinktime;
	int skilltype;
	int skillval;
	int skilltimer;
	int skillduration; // [Valaris]
	int skillbonustype;
	int skillbonusval;
	int skillbonustimer;
//	int skillbonusduration; // [Valaris]
	struct item *lootitem;
	short loot; // [Valaris]
	short lootmax; // [Valaris]
	short lootitem_count;
	short lootitem_weight;
	int lootitem_timer;
	struct skill_timerskill skilltimerskill[MAX_MOBSKILLTIMERSKILL]; // [Valaris]
	struct skill_unit_group skillunit[MAX_MOBSKILLUNITGROUP]; // [Valaris]
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET]; // [Valaris]
	struct map_session_data *msd;
};

enum { MS_IDLE,MS_WALK,MS_ATTACK,MS_DEAD,MS_DELAY };

enum { NONE_ATTACKABLE,ATTACKABLE };

enum { ATK_LUCKY=1,ATK_FLEE,ATK_DEF};	// àÕÇ‹ÇÍÉyÉiÉãÉeÉBåvéZóp

// ëïîıÉRÅ[Éh
enum {
	EQP_WEAPON		= 0x0002,		// âEéË
	EQP_ARMOR		= 0x0010,		// ëÃ
	EQP_SHIELD		= 0x0020,		// ç∂éË
	EQP_HELM		= 0x0100,		// ì™è„íi
};
















// map_getcell()/map_setcell()Ç≈égópÇ≥ÇÍÇÈÉtÉâÉO
typedef enum { 
	CELL_CHKWALL=0,		// ï«(ÉZÉãÉ^ÉCÉv1)
	CELL_CHKWATER,		// êÖèÍ(ÉZÉãÉ^ÉCÉv3)
	CELL_CHKGROUND,		// ínñ è·äQï®(ÉZÉãÉ^ÉCÉv5)
	CELL_CHKPASS,		// í âﬂâ¬î\(ÉZÉãÉ^ÉCÉv1,5à»äO)
	CELL_CHKNOPASS,		// í âﬂïsâ¬(ÉZÉãÉ^ÉCÉv1,5)
	CELL_CHKHOLE,		// a hole in morroc desert
	CELL_GETTYPE,		// ÉZÉãÉ^ÉCÉvÇï‘Ç∑
	CELL_CHKNPC=0x10,	// É^ÉbÉ`É^ÉCÉvÇÃNPC(ÉZÉãÉ^ÉCÉv0x80ÉtÉâÉO)
	CELL_SETNPC,		// É^ÉbÉ`É^ÉCÉvÇÃNPCÇÉZÉbÉg
	CELL_CLRNPC,		// É^ÉbÉ`É^ÉCÉvÇÃNPCÇclear suru
	CELL_CHKBASILICA,	// ÉoÉWÉäÉJ(ÉZÉãÉ^ÉCÉv0x40ÉtÉâÉO)
	CELL_SETBASILICA,	// ÉoÉWÉäÉJÇÉZÉbÉg
	CELL_CLRBASILICA,	// ÉoÉWÉäÉJÇÉNÉäÉA
	CELL_CHKMOONLIT,
	CELL_SETMOONLIT,
	CELL_CLRMOONLIT,
	CELL_CHKREGEN,
	CELL_SETREGEN,
	CELL_CLRREGEN
} cell_t;

// CELL
#define CELL_MASK		0x07	// 3 bit for cell mask

// celests new stuff
//#define CELL_MOONLIT	0x100
//#define CELL_REGEN		0x200

enum {
	GAT_NONE		= 0,	// normal ground
	GAT_WALL		= 1,	// not passable and blocking
	GAT_UNUSED1		= 2,
	GAT_WATER		= 3,	// water
	GAT_UNUSED2		= 4,
	GAT_GROUND		= 5,	// not passable but can shoot/cast over it
	GAT_HOLE		= 6,	// holes in morroc desert
	GAT_UNUSED3		= 7,
};

struct mapgat // values from .gat & 
{
	unsigned char type : 3;		// 3bit used for land,water,wall,(hole) (values 0,1,3,5,6 used)
								// providing 4 bit space and interleave two cells in x dimension
								// would not waste memory too much; will implement it later on a new map model
	unsigned char npc : 4;		// 4bit counter for npc touchups, can hold 15 touchups;
	unsigned char basilica : 1;	// 1bit for basilica (is on/off for basilica enough, what about two casting priests?)
	unsigned char moonlit : 1;	// 1bit for moonlit
	unsigned char regen : 1;	// 1bit for regen
	unsigned char _unused : 6;	// 6 bits left
};
// will alloc a short now


struct map_data {
	char name[24];
	struct mapgat	*gat;	// NULLÇ»ÇÁâ∫ÇÃmap_data_other_serverÇ∆ÇµÇƒàµÇ§

	char *alias; // [MouseJstr]
	struct block_list **block;
	struct block_list **block_mob;
	int *block_count;
	int *block_mob_count;
	int m;
	unsigned short xs;
	unsigned short ys;
	unsigned short bxs;
	unsigned short bys;
	int npc_num;
	int users;
	struct {
		unsigned alias : 1;
		unsigned nomemo : 1;
		unsigned noteleport : 1;
		unsigned noreturn : 1;
		unsigned monster_noteleport : 1;
		unsigned nosave : 1;
		unsigned nobranch : 1;
		unsigned nopenalty : 1;
		unsigned pvp : 1;
		unsigned pvp_noparty : 1;
		unsigned pvp_noguild : 1;
		unsigned pvp_nightmaredrop :1;
		unsigned pvp_nocalcrank : 1;
		unsigned gvg : 1;
		unsigned gvg_noparty : 1;
		unsigned nozenypenalty : 1;
		unsigned notrade : 1;
		unsigned noskill : 1;
		unsigned nowarp : 1;
		unsigned nowarpto : 1;
		unsigned nopvp : 1; // [Valaris]
		unsigned noicewall : 1; // [Valaris]
		unsigned snow : 1; // [Valaris]
		unsigned fog : 1; // [Valaris]
		unsigned sakura : 1; // [Valaris]
		unsigned leaves : 1; // [Valaris]
		unsigned rain : 1; // [Valaris]
		unsigned indoors : 1; // celest
		unsigned nogo : 1; // [Valaris]
	} flag;
	struct point save;
	struct npc_data *npc[MAX_NPC_PER_MAP];
	struct {
		int drop_id;
		int drop_type;
		int drop_per;
	} drop_list[MAX_DROP_PER_MAP];
};
struct map_data_other_server {
	char name[24];
	struct mapgat *gat;	// NULLå≈íËÇ…ÇµÇƒîªíf
	unsigned long ip;
	unsigned short port;
	struct map_data* map;
};

struct flooritem_data {
	struct block_list bl;
	short subx;
	short suby;
	int cleartimer;
	int first_get_id;
	int second_get_id;
	int third_get_id;
	unsigned long first_get_tick;
	unsigned long second_get_tick;
	unsigned long third_get_tick;
	struct item item_data;
};

enum {
	SP_SPEED,SP_BASEEXP,SP_JOBEXP,SP_KARMA,SP_MANNER,SP_HP,SP_MAXHP,SP_SP,	// 0-7
	SP_MAXSP,SP_STATUSPOINT,SP_0a,SP_BASELEVEL,SP_SKILLPOINT,SP_STR,SP_AGI,SP_VIT,	// 8-15
	SP_INT,SP_DEX,SP_LUK,SP_CLASS,SP_ZENY,SP_SEX,SP_NEXTBASEEXP,SP_NEXTJOBEXP,	// 16-23
	SP_WEIGHT,SP_MAXWEIGHT,SP_1a,SP_1b,SP_1c,SP_1d,SP_1e,SP_1f,	// 24-31
	SP_USTR,SP_UAGI,SP_UVIT,SP_UINT,SP_UDEX,SP_ULUK,SP_26,SP_27,	// 32-39
	SP_28,SP_ATK1,SP_ATK2,SP_MATK1,SP_MATK2,SP_DEF1,SP_DEF2,SP_MDEF1,	// 40-47
	SP_MDEF2,SP_HIT,SP_FLEE1,SP_FLEE2,SP_CRITICAL,SP_ASPD,SP_36,SP_JOBLEVEL,	// 48-55
	SP_UPPER,SP_PARTNER,SP_CART,SP_FAME,SP_UNBREAKABLE,	//56-60
	SP_CARTINFO=99,	// 99

	SP_BASEJOB=119,	// 100+19 - celest

	// original 1000-
	SP_ATTACKRANGE=1000,	SP_ATKELE,SP_DEFELE,	// 1000-1002
	SP_CASTRATE, SP_MAXHPRATE, SP_MAXSPRATE, SP_SPRATE, // 1003-1006
	SP_ADDELE, SP_ADDRACE, SP_ADDSIZE, SP_SUBELE, SP_SUBRACE, // 1007-1011
	SP_ADDEFF, SP_RESEFF,	// 1012-1013
	SP_BASE_ATK,SP_ASPD_RATE,SP_HP_RECOV_RATE,SP_SP_RECOV_RATE,SP_SPEED_RATE, // 1014-1018
	SP_CRITICAL_DEF,SP_NEAR_ATK_DEF,SP_LONG_ATK_DEF, // 1019-1021
	SP_DOUBLE_RATE, SP_DOUBLE_ADD_RATE, SP_MATK, SP_MATK_RATE, // 1022-1025
	SP_IGNORE_DEF_ELE,SP_IGNORE_DEF_RACE, // 1026-1027
	SP_ATK_RATE,SP_SPEED_ADDRATE,SP_ASPD_ADDRATE, // 1028-1030
	SP_MAGIC_ATK_DEF,SP_MISC_ATK_DEF, // 1031-1032
	SP_IGNORE_MDEF_ELE,SP_IGNORE_MDEF_RACE, // 1033-1034
	SP_MAGIC_ADDELE,SP_MAGIC_ADDRACE,SP_MAGIC_SUBRACE, // 1035-1037
	SP_PERFECT_HIT_RATE,SP_PERFECT_HIT_ADD_RATE,SP_CRITICAL_RATE,SP_GET_ZENY_NUM,SP_ADD_GET_ZENY_NUM, // 1038-1042
	SP_ADD_DAMAGE_CLASS,SP_ADD_MAGIC_DAMAGE_CLASS,SP_ADD_DEF_CLASS,SP_ADD_MDEF_CLASS, // 1043-1046
	SP_ADD_MONSTER_DROP_ITEM,SP_DEF_RATIO_ATK_ELE,SP_DEF_RATIO_ATK_RACE,SP_ADD_SPEED, // 1047-1050
	SP_HIT_RATE,SP_FLEE_RATE,SP_FLEE2_RATE,SP_DEF_RATE,SP_DEF2_RATE,SP_MDEF_RATE,SP_MDEF2_RATE, // 1051-1057
	SP_SPLASH_RANGE,SP_SPLASH_ADD_RANGE,SP_AUTOSPELL,SP_HP_DRAIN_RATE,SP_SP_DRAIN_RATE, // 1058-1062
	SP_SHORT_WEAPON_DAMAGE_RETURN,SP_LONG_WEAPON_DAMAGE_RETURN,SP_WEAPON_COMA_ELE,SP_WEAPON_COMA_RACE, // 1063-1066
	SP_ADDEFF2,SP_BREAK_WEAPON_RATE,SP_BREAK_ARMOR_RATE,SP_ADD_STEAL_RATE, // 1067-1070
	SP_MAGIC_DAMAGE_RETURN,SP_RANDOM_ATTACK_INCREASE,SP_ALL_STATS,SP_AGI_VIT,SP_AGI_DEX_STR,SP_PERFECT_HIDE, // 1071-1076
	SP_DISGUISE,SP_CLASSCHANGE, // 1077-1078
	SP_HP_DRAIN_VALUE,SP_SP_DRAIN_VALUE, // 1079-1080
	SP_WEAPON_ATK,SP_WEAPON_ATK_RATE, // 1081-1082
	SP_DELAYRATE,	// 1083
	
	SP_RESTART_FULL_RECORVER=2000,SP_NO_CASTCANCEL,SP_NO_SIZEFIX,SP_NO_MAGIC_DAMAGE,SP_NO_WEAPON_DAMAGE,SP_NO_GEMSTONE, // 2000-2005
	SP_NO_CASTCANCEL2,SP_INFINITE_ENDURE,SP_UNBREAKABLE_WEAPON,SP_UNBREAKABLE_ARMOR, SP_UNBREAKABLE_HELM, // 2006-2010
	SP_UNBREAKABLE_SHIELD, SP_LONG_ATK_RATE, // 2011-2012

	SP_CRIT_ATK_RATE, SP_CRITICAL_ADDRACE, SP_NO_REGEN, SP_ADDEFF_WHENHIT, SP_AUTOSPELL_WHENHIT, // 2013-2017
	SP_SKILL_ATK, SP_UNSTRIPABLE, SP_ADD_DAMAGE_BY_CLASS, // 2018-2020
	SP_SP_GAIN_VALUE, SP_IGNORE_DEF_MOB, SP_HP_LOSS_RATE, SP_ADDRACE2, SP_HP_GAIN_VALUE, // 2021-2025
	SP_SUBSIZE, SP_DAMAGE_WHEN_UNEQUIP, SP_ADD_ITEM_HEAL_RATE, SP_LOSESP_WHEN_UNEQUIP, SP_EXP_ADDRACE,	// 2026-2030
	SP_SP_GAIN_RACE, SP_SUBRACE2,
};

enum {
	LOOK_BASE,LOOK_HAIR,LOOK_WEAPON,LOOK_HEAD_BOTTOM,LOOK_HEAD_TOP,LOOK_HEAD_MID,LOOK_HAIR_COLOR,LOOK_CLOTHES_COLOR,LOOK_SHIELD,LOOK_SHOES
};




struct chat_data {
	struct block_list bl;

	char pass[8];   /* password */
	char title[61]; /* room title MAX 60 */
	unsigned char limit;     /* join limit */
	unsigned char trigger;
	unsigned char users;     /* current users */
	unsigned char pub;       /* room attribute */
	struct map_session_data *usersd[20];
	struct block_list *owner_;
	struct block_list **owner;
	char npc_event[50];
};

extern struct map_data map[];
extern int map_num;
extern int autosave_interval;
extern int agit_flag;
extern int night_flag; // 0=day, 1=night [Yor]

// gat?÷ß
int map_getcell(int,int,int,cell_t);
int map_getcellp(struct map_data*,int,int,cell_t);
void map_setcell(int,int,int,int);


extern int map_read_flag; // 0: grf´’´°´§´Î 1: ´≠´„´√´∑´Â 2: ´≠´„´√´∑´Â(?ıÍ)
enum {
	READ_FROM_GAT, 
	READ_FROM_AFM,
	READ_FROM_BITMAP, 
	READ_FROM_BITMAP_COMPRESSED
};

extern char motd_txt[];
extern char help_txt[];

extern char talkie_mes[];

extern char wisp_server_name[];

// éIëSëÃèÓïÒ
void map_setusers(int);
int map_getusers(void);
// blockçÌèúä÷òA
int map_freeblock( void *bl );
int map_freeblock_lock(void);
int map_freeblock_unlock(void);
// blockä÷òA
int map_addblock(struct block_list *);
int map_delblock(struct block_list *);
void map_foreachinarea(int (*)(struct block_list*,va_list),int,int,int,int,int,int,...);
// -- moonsoul (added map_foreachincell)
void map_foreachincell(int (*)(struct block_list*,va_list),int,int,int,int,...);
void map_foreachinmovearea(int (*)(struct block_list*,va_list),int,int,int,int,int,int,int,int,...);
void map_foreachinpath(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int range,int type,...); // Celest
int map_countnearpc(int,int,int);
//blockä÷òAÇ…í«â¡
int map_count_oncell(int m,int x,int y);
struct skill_unit *map_find_skill_unit_oncell(struct block_list *,int x,int y,unsigned short skill_id,struct skill_unit *);
// àÍéûìIobjectä÷òA
int map_addobject(struct block_list *);
int map_delobject(int);
int map_delobjectnofree(int id);
void map_foreachobject(int (*)(struct block_list*,va_list),int,...);
//
int map_quit(struct map_session_data *);
// npc
int map_addnpc(int,struct npc_data *);

// è∞ÉAÉCÉeÉÄä÷òA
int map_clearflooritem_timer(int tid,unsigned long tick,int id,int data);
#define map_clearflooritem(id) map_clearflooritem_timer(0,0,id,1)
int map_addflooritem(struct item *,int,int,int,int,struct map_session_data *,struct map_session_data *,struct map_session_data *,int);
int map_searchrandfreecell(int,int,int,int);

// ÉLÉÉÉâidÅÅÅÑÉLÉÉÉâñº ïœä∑ä÷òA
void map_addchariddb(int charid,char *name);
void map_delchariddb(int charid);
int map_reqchariddb(struct map_session_data * sd,int charid);
char * map_charid2nick(int);

struct map_session_data * map_id2sd(int);
struct block_list * map_id2bl(int);
int map_mapname2mapid(char*);
int map_mapname2ipport(char *name, unsigned long *ip,unsigned short *port);
int map_setipport(char *name, unsigned long ip, unsigned short port);
int map_eraseipport(char *name, unsigned long ip,unsigned short port);
int map_eraseallipport(void);
void map_addiddb(struct block_list *);
void map_deliddb(struct block_list *bl);
int map_foreachiddb(int (*)(void*,void*,va_list),...);
void map_addnickdb(struct map_session_data *);
struct map_session_data * map_nick2sd(char*);
int compare_item(struct item *a, struct item *b);

// ÇªÇÃëº
int map_check_dir(int s_dir,int t_dir);
int map_calc_dir( struct block_list *src,int x,int y);

// path.cÇÊÇË
int path_search(struct walkpath_data*,int,int,int,int,int,int);
int path_search_long(struct shootpath_data *,int,int,int,int,int);
int path_blownpos(int m,int x0,int y0,int dx,int dy,int count);

int map_who(int fd);

int cleanup_sub(struct block_list *bl, va_list ap);

void map_helpscreen(); // [Valaris]
int map_delmap(char *mapname);

#ifndef TXT_ONLY

// MySQL
#include <mysql.h>

void char_online_check(void); // [Valaris]
void char_offline(struct map_session_data *sd);

extern MYSQL mmysql_handle;
extern char tmp_sql[65535];
extern MYSQL_RES* sql_res ;
extern MYSQL_ROW	sql_row ;

extern MYSQL lmysql_handle;
extern char tmp_lsql[65535];
extern MYSQL_RES* lsql_res ;
extern MYSQL_ROW	lsql_row ;

extern MYSQL mail_handle;
extern MYSQL_RES* 	mail_res ;
extern MYSQL_ROW	mail_row ;
extern char tmp_msql[65535];

extern int db_use_sqldbs;

extern char item_db_db[32];
extern char mob_db_db[32];
extern char login_db[32];

extern char login_db_level[32];
extern char login_db_account_id[32];

extern char gm_db[32];
extern char gm_db_level[32];
extern char gm_db_account_id[32];

extern int lowest_gm_level;
extern int read_gm_interval;

extern char char_db[32];
#endif /* not TXT_ONLY */

#endif
