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
#define MAX_STATUSCHANGE 200
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
#define MAX_FAMELIST 10

#define DEFAULT_AUTOSAVE_INTERVAL 60*1000

#define OPTION_HIDE 0x40

enum { BL_NUL, BL_PC, BL_NPC, BL_MOB, BL_ITEM, BL_CHAT, BL_SKILL , BL_PET };
enum { WARP, SHOP, SCRIPT, MONS };

struct block_list
{
	struct block_list *next;
	struct block_list *prev;
	unsigned long id;
	unsigned short m;
	unsigned short x;
	unsigned short y;
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
	unsigned short rx;
	unsigned short ry;
	unsigned short len;
	unsigned short x[MAX_WALKPATH];
	unsigned short y[MAX_WALKPATH];
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
	long val1;
	long val2;
	long val3;
	long val4;
};
struct vending {
	unsigned short index;
	unsigned short amount;
	unsigned long value;
};

struct skill_unit_group;

struct skill_unit
{
	struct block_list bl;
	struct skill_unit_group *group;

	long limit;
	long val1;
	long val2;
	short alive;
	short range;
};
struct skill_unit_group
{
	unsigned long src_id;
	unsigned long party_id;
	unsigned long guild_id;
	unsigned short map;
	long target_flag;
	unsigned long tick;
	long limit;
	long interval;

	unsigned short skill_id;
	unsigned short skill_lv;
	long val1;
	long val2;
	long val3;
	char *valstr;
	unsigned char unit_id;
	long group_id;
	long unit_count,alive_count;
	struct skill_unit *unit;
};
struct skill_unit_group_tickset
{
	unsigned short skill_id;
	unsigned long tick;
};
struct skill_timerskill
{
	unsigned long src_id;
	unsigned long target_id;
	unsigned short map;
	unsigned short x;
	unsigned short y;
	unsigned short skill_id;
	unsigned short skill_lv;
	int timer;
	long type;
	long flag;
};

struct npc_data;
struct pet_db;
struct item_data;
struct square;

struct map_session_data
{
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
													
		unsigned intravision : 1;					// 40
		unsigned ignoreAll : 1;						// 41
		unsigned nodelay :1;						// 42
		unsigned noexp :1;							// 43
		unsigned detach :1;							// 44
		unsigned __unused : 3;						// 3 bit left
	} state;
	
	unsigned long char_id;
	unsigned long login_id1;
	unsigned long login_id2;
	unsigned char sex;
	unsigned long packet_ver;  // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])

	struct guild * gmaster_flag;

	struct mmo_charstatus status;

	struct item_data *inventory_data[MAX_INVENTORY];

	short equip_index[11];
	unsigned short unbreakable_equip;
	unsigned short unbreakable;	// chance to prevent equipment breaking [celest]

	unsigned long weight;
	unsigned long max_weight;
	unsigned long cart_weight;
	unsigned long cart_max_weight;
	unsigned short cart_num;
	unsigned short cart_max_num;
	char mapname[24];
	int fd;
	int new_fd;
	unsigned short to_x;
	unsigned short to_y;
	unsigned short speed;
	unsigned short prev_speed;
	short opt1;
	short opt2;
	short opt3;
	unsigned char dir;
	unsigned char head_dir;
	unsigned long client_tick;
	unsigned long server_tick;
	struct walkpath_data walkpath;
	int walktimer;
	int next_walktime;

	unsigned long npc_id;
	unsigned long areanpc_id;
	unsigned long npc_shopid;
	unsigned long npc_pos;
	unsigned long npc_menu;
	unsigned long npc_amount;
	unsigned long npc_stack;
	unsigned long npc_stackmax;
	char *npc_script;
	char *npc_scriptroot;
	char *npc_stackbuf;
	char npc_str[256];
	unsigned long chatID;
	time_t idletime;

	struct{
		char name[24];
	} ignore[MAX_IGNORE_LIST];


	int attacktimer;
	unsigned long attacktarget;
	unsigned short attacktarget_lv;
	unsigned long attackabletime;

        int followtimer; // [MouseJstr]
        int followtarget;

	time_t emotionlasttime; // to limit flood with emotion packets

	unsigned short attackrange;
	unsigned short attackrange_;
	int skilltimer;
	unsigned long skilltarget;
	unsigned short skillx;
	unsigned short skilly;
	unsigned short skillid;
	unsigned short skilllv;
	unsigned short skillitem;
	unsigned short skillitemlv;
	unsigned short skillid_old;
	unsigned short skilllv_old;
	unsigned short skillid_dance;
	unsigned short skilllv_dance;
	struct skill_unit_group skillunit[MAX_SKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	struct skill_timerskill skilltimerskill[MAX_SKILLTIMERSKILL];
	char blockskill[MAX_SKILL];	// [celest]	
	unsigned short cloneskill_id;
	unsigned short cloneskill_lv;
	unsigned long potion_hp;
	unsigned long potion_sp;
	unsigned long potion_per_hp;
	unsigned long potion_per_sp;

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

	unsigned short view_class;
	unsigned short weapontype1;
	unsigned short weapontype2;
	
	unsigned short disguise_id; // [Valaris]

	long paramb[6];
	long paramc[6];
	long parame[6];
	long paramcard[6];
	unsigned short  hit;
	unsigned short  flee;
	unsigned short  flee2;
	long aspd;
	long amotion;
	long dmotion;
	unsigned short watk;
	unsigned short watk2;
	long atkmods[3];
	unsigned short  def;
	unsigned short  def2;
	unsigned short  mdef;
	unsigned short  mdef2;
	long critical;
	unsigned short  matk1;
	unsigned short  matk2;
	long atk_ele;
	long def_ele;
	long star;
	long overrefine;
	long castrate;
	long delayrate;
	long hprate;
	long sprate;
	long dsprate;
	long addele[10];
	long addrace[12];
	long addsize[3];
	long subele[10];
	long subrace[12];
	long addeff[10];
	long addeff2[10];
	long reseff[10];
	long watk_;
	long watk_2;
	long atkmods_[3];
	long addele_[10];
	long addrace_[12];
	long addsize_[3];	//二刀流のために追加
	long atk_ele_;
	long star_;
	long overrefine_;				//二刀流のために追加
	unsigned short base_atk;
	long atk_rate;
	long weapon_atk[16];
	long weapon_atk_rate[16];
	long arrow_atk;
	long arrow_ele;
	long arrow_cri;
	long arrow_hit;
	unsigned short arrow_range;
	long arrow_addele[10],arrow_addrace[12],arrow_addsize[3],arrow_addeff[10],arrow_addeff2[10];
	long nhealhp,nhealsp,nshealhp,nshealsp,nsshealhp,nsshealsp;
	long aspd_rate,speed_rate,hprecov_rate,sprecov_rate,critical_def,double_rate;
	long near_attack_def_rate,long_attack_def_rate,magic_def_rate,misc_def_rate;
	long matk_rate,ignore_def_ele,ignore_def_race,ignore_def_ele_,ignore_def_race_;
	long ignore_mdef_ele,ignore_mdef_race;
	long magic_addele[10],magic_addrace[12],magic_subrace[12];
	long perfect_hit,get_zeny_num;
	long critical_rate,hit_rate,flee_rate,flee2_rate,def_rate,def2_rate,mdef_rate,mdef2_rate;
	long def_ratio_atk_ele,def_ratio_atk_ele_,def_ratio_atk_race,def_ratio_atk_race_;
	long add_damage_class_count,add_damage_class_count_,add_magic_damage_class_count;
	short add_damage_classid[10],add_damage_classid_[10],add_magic_damage_classid[10];
	long add_damage_classrate[10],add_damage_classrate_[10],add_magic_damage_classrate[10];
	short add_def_class_count,add_mdef_class_count;
	short add_def_classid[10],add_mdef_classid[10];
	long add_def_classrate[10],add_mdef_classrate[10];
	short monster_drop_item_count;
	short monster_drop_itemid[10];
	long monster_drop_race[10];
	long monster_drop_itemrate[10];
	long double_add_rate;
	long speed_add_rate;
	long aspd_add_rate;
	long perfect_hit_add;
	long get_zeny_add_num;
	short splash_range;
	short splash_add_range;
	short autospell_id[10];
	short autospell_lv[10];
	short autospell_rate[10];
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
	long short_weapon_damage_return;
	long long_weapon_damage_return;
	long weapon_coma_ele[10];
	long weapon_coma_race[12];
	short break_weapon_rate;
	short break_armor_rate;
	short add_steal_rate;
	//--- 02/15's new card effects [celest]
	long crit_atk_rate;
	long critaddrace[12];
	short no_regen;
	long addeff3[10];
	short addeff3_type[10];
	short autospell2_id[10],autospell2_lv[10],autospell2_rate[10];
	long skillatk[2];
	unsigned short unstripable_equip;
	short add_damage_classid2[10],add_damage_class_count2;
	long add_damage_classrate2[10];
	short sp_gain_value, hp_gain_value;
	short sp_drain_type;
	short ignore_def_mob, ignore_def_mob_;

	unsigned long hp_loss_tick;
	unsigned long hp_loss_rate;
	short hp_loss_value;
	short hp_loss_type;

	unsigned long sp_loss_tick;
	unsigned long sp_loss_rate;
	short sp_loss_value;

	long addrace2[12],addrace2_[12];
	long subsize[3];
	short unequip_losehp[11];
	short unequip_losesp[11];
	long itemid;
	long itemhealrate[7];
	//--- 03/15's new card effects
	long expaddrace[12];
	long subrace2[12];
	short sp_gain_race[12];
	short monster_drop_itemgroup[10];

	int setitem_hash;
	//--- end effects
	short spiritball;
	short spiritball_old;
	long spirit_timer[MAX_SKILL_LEVEL];
	long magic_damage_return; // AppleGirl Was Here
	long random_attack_increase_add;
	long random_attack_increase_per; // [Valaris]
	long perfect_hiding; // [Valaris]
	long classchange; // [Valaris]
	
	long die_counter;
	short doridori_counter;
	char potion_success_counter;

	long reg_num;
	struct script_reg *reg;
	long regstr_num;
	struct script_regstr *regstr;

	struct status_change sc_data[MAX_STATUSCHANGE];
	struct square dev;

	unsigned long trade_partner;
	unsigned long deal_item_index[10];
	unsigned short deal_item_amount[10];
	unsigned long deal_zeny;
	unsigned short deal_locked;

	unsigned long party_sended;
	unsigned long party_invite;
	unsigned long party_invite_account;
	long party_hp;
	short party_x;
	short party_y;

	unsigned long guild_sended;
	unsigned long guild_invite;
	unsigned long guild_invite_account;
	unsigned long guild_emblem_id;
	unsigned long guild_alliance;
	unsigned long guild_alliance_account;
	unsigned long guildspy; // [Syrus22]
	unsigned long partyspy; // [Syrus22]

	unsigned long vender_id;
	unsigned short vend_num;
	char message[80];
	struct vending vending[MAX_VENDING];

	long catch_target_class;
	struct s_pet pet;
	struct pet_db *petDB;
	struct pet_data *pd;
	long pet_hungry_timer;

	long pvp_point;
	long pvp_rank;
	long pvp_timer;
	unsigned long pvp_lastusers;
	int pvp_won, pvp_lost;

	char eventqueue[MAX_EVENTQUEUE][50];
	long eventtimer[MAX_EVENTTIMER];
	unsigned short eventcount; // [celest]

	unsigned short change_level;	// [celest]

#ifndef TXT_ONLY
	unsigned long mail_counter;	// mail counter for mail system [Valaris]
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
	unsigned short nameid;
	long value;
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
	// ここにメンバを追加してはならない(shop_itemが可変長の為)

	short arenaflag;
	void *chatdb;

	npc_data() : chatdb(NULL)	{} 
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
		unsigned soul_change_flag : 1; // Celest
		unsigned special_mob_ai : 3;
		unsigned _unused : 5;
	} state;
	unsigned long provoke_id; // Celest
	int timer;
	short to_x;
	short to_y;
	short target_dir;
	short speed;
	int hp;
	unsigned long target_id;
	unsigned long attacked_id;
	short attacked_count;
	short target_lv;
	struct walkpath_data walkpath;
	unsigned long next_walktime;
	unsigned long attackabletime;
	unsigned long last_deadtime;
	unsigned long last_spawntime;
	unsigned long last_thinktime;
	unsigned long canmove_tick;
	short move_fail_count;
	struct mob_damage
	{
		unsigned long fromid;
		long dmg;
	} dmglog[DAMAGELOG_SIZE];
	struct item *lootitem;
	short lootitem_count;

	struct status_change sc_data[MAX_STATUSCHANGE];
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
	unsigned long master_id;
	int master_dist;
	unsigned long exclusion_src;
	unsigned long exclusion_party;
	unsigned long  exclusion_guild;
	struct skill_timerskill skilltimerskill[MAX_MOBSKILLTIMERSKILL];
	struct skill_unit_group skillunit[MAX_MOBSKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	char npc_event[50];
	unsigned char size;
	short recall_flag;
	int recallmob_count;
	short recallcount;
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
	unsigned long target_id;
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

enum { ATK_LUCKY=1,ATK_FLEE,ATK_DEF };	// 囲まれペナルティ計算用

// 装備コード
enum {
	EQP_WEAPON		= 1,		// 右手
	EQP_ARMOR		= 2,		// 体
	EQP_SHIELD		= 4,		// 左手
	EQP_HELM		= 8,		// 頭上段
};
















// map_getcell()/map_setcell()で使用されるフラグ
typedef enum { 
	CELL_CHKWALL=0,		// 壁(セルタイプ1)
	CELL_CHKWATER,		// 水場(セルタイプ3)
	CELL_CHKGROUND,		// 地面障害物(セルタイプ5)
	CELL_CHKPASS,		// 通過可能(セルタイプ1,5以外)
	CELL_CHKNOPASS,		// 通過不可(セルタイプ1,5)
	CELL_CHKHOLE,		// a hole in morroc desert
	CELL_GETTYPE,		// セルタイプを返す
	CELL_CHKNPC=0x10,	// タッチタイプのNPC(セルタイプ0x80フラグ)
	CELL_SETNPC,		// タッチタイプのNPCをセット
	CELL_CLRNPC,		// タッチタイプのNPCをclear suru
	CELL_CHKBASILICA,	// バジリカ(セルタイプ0x40フラグ)
	CELL_SETBASILICA,	// バジリカをセット
	CELL_CLRBASILICA,	// バジリカをクリア
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
	char mapname[24];
	struct mapgat	*gat;	// NULLなら下のmap_data_other_serverとして扱う

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
	size_t npc_num;
	size_t users;
	struct {
//		unsigned alias : 1;
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
		unsigned gvg_dungeon : 1; // celest
		unsigned nozenypenalty : 1;
		unsigned notrade : 1;
		unsigned noskill : 1;
		unsigned nowarp : 1;
		unsigned nowarpto : 1;
		unsigned nopvp : 1;
		unsigned noicewall : 1;
		unsigned snow : 1;
		unsigned clouds : 1;
		unsigned fog : 1;
		unsigned fireworks : 1;
		unsigned sakura : 1;
		unsigned leaves : 1;
		unsigned rain : 1;
		unsigned indoors : 1;
		unsigned nogo : 1;
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
	struct mapgat *gat;	// NULL固定にして判断
	unsigned long ip;
	unsigned short port;
	struct map_data* map;
};

struct flooritem_data {
	struct block_list bl;
	short subx;
	short suby;
	int cleartimer;
	unsigned long first_get_id;
	unsigned long second_get_id;
	unsigned long third_get_id;
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
	SP_SP_GAIN_RACE, SP_SUBRACE2, SP_ADDEFF_WHENHIT_SHORT,	// 2031-2033
	SP_UNSTRIPABLE_WEAPON,SP_UNSTRIPABLE_ARMOR,SP_UNSTRIPABLE_HELM,SP_UNSTRIPABLE_SHIELD,  // 2034-2037
	SP_INTRAVISION, SP_ADD_MONSTER_DROP_ITEMGROUP, SP_SP_LOSS_RATE // 2038-2040
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
extern size_t map_num;
extern int autosave_interval;
extern int agit_flag;
extern int night_flag; // 0=day, 1=night [Yor]

// gat?ﾖｧ
int map_getcell(unsigned short m,unsigned short x, unsigned short y,cell_t cellchk);
int map_getcellp(struct map_data* m,unsigned short x, unsigned short y,cell_t cellchk);
void map_setcell(unsigned short m,unsigned short x, unsigned short y,int cellck);


extern int map_read_flag; // 0: grfｫﾕｫ｡ｫ､ｫ� 1: ｫｭｫ罩ﾃｫｷｫ� 2: ｫｭｫ罩ﾃｫｷｫ�(?��)
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

// 鯖全体情報
void map_setusers(int fd);
int map_getusers(void);
// block削除関連
int map_freeblock( void *bl );
int map_freeblock_lock(void);
int map_freeblock_unlock(void);
// block関連
int map_addblock(struct block_list *);
int map_delblock(struct block_list *);
void map_foreachinarea(int (*)(struct block_list*,va_list),int,int,int,int,int,int,...);
// -- moonsoul (added map_foreachincell)
void map_foreachincell(int (*)(struct block_list*,va_list),int,int,int,int,...);
void map_foreachinmovearea(int (*)(struct block_list*,va_list),int,int,int,int,int,int,int,int,...);
void map_foreachinpath(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int range,int type,...); // Celest
int map_countnearpc(int,int,int);
//block関連に追加
int map_count_oncell(int m,int x,int y);
struct skill_unit *map_find_skill_unit_oncell(struct block_list *,int x,int y,unsigned short skill_id,struct skill_unit *);
// 一時的object関連
int map_addobject(struct block_list *);
int map_delobject(int);
int map_delobjectnofree(int id);
void map_foreachobject(int (*)(struct block_list*,va_list),int,...);
//
int map_quit(struct map_session_data *);
// npc
int map_addnpc(size_t m, struct npc_data *nd);

// 床アイテム関連
int map_clearflooritem_timer(int tid,unsigned long tick,int id,int data);
#define map_clearflooritem(id) map_clearflooritem_timer(0,0,id,1)
int map_addflooritem(struct item *,int,int,int,int,struct map_session_data *,struct map_session_data *,struct map_session_data *,int);
int map_searchrandfreecell(int,int,int,int);

// キャラid＝＞キャラ名 変換関連
void map_addchariddb(unsigned long charid,char *name);
void map_delchariddb(unsigned long charid);
int map_reqchariddb(struct map_session_data * sd,unsigned long charid);
char * map_charid2nick(int);
struct map_session_data * map_charid2sd(unsigned long id);
struct map_session_data * map_id2sd(unsigned long id);
struct block_list * map_id2bl(unsigned long id);

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

// その他
int map_check_dir(int s_dir,int t_dir);
int map_calc_dir( struct block_list *src,int x,int y);

// path.cより
int path_search(struct walkpath_data*,int,int,int,int,int,int);
int path_search_long(struct shootpath_data *spd,unsigned short m,unsigned short x0,unsigned short y0,unsigned short x1,unsigned short y1);
int path_blownpos(int m,int x0,int y0,int dx,int dy,int count);

int map_who(int fd);

int cleanup_sub(struct block_list *bl, va_list ap);

void map_helpscreen(); // [Valaris]
int map_delmap(char *mapname);

extern char *INTER_CONF_NAME;
extern char *LOG_CONF_NAME;
extern char *MAP_CONF_NAME;
extern char *BATTLE_CONF_FILENAME;
extern char *ATCOMMAND_CONF_FILENAME;
extern char *CHARCOMMAND_CONF_FILENAME;
extern char *SCRIPT_CONF_NAME;
extern char *MSG_CONF_NAME;
extern char *GRF_PATH_FILENAME;

#ifndef TXT_ONLY

// MySQL
#ifdef __WIN32
#include <my_global.h>
#include <my_sys.h>
#endif
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
extern char item_db2_db[32];
extern char mob_db_db[32];
extern char mob_db2_db[32];
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
