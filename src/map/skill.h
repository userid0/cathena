// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _SKILL_H_
#define _SKILL_H_

#include "../common/mmo.h" // MAX_SKILL, struct square
#include "map.h" // struct block_list
struct map_session_data;
struct homun_data;
struct skill_unit;
struct skill_unit_group;
struct status_change_entry;

#define MAX_SKILL_DB			MAX_SKILL
#define MAX_SKILL_PRODUCE_DB	150
#define MAX_PRODUCE_RESOURCE	12
#define MAX_SKILL_ARROW_DB		150
#define MAX_ARROW_RESOURCE		5
#define MAX_SKILL_ABRA_DB		350

#define MAX_SKILL_LEVEL 100

//Constants to identify the skill's inf value:
enum e_skill_inf
{
	INF_ATTACK_SKILL  = 0x01,
	INF_GROUND_SKILL  = 0x02,
	INF_SELF_SKILL    = 0x04, // Skills casted on self where target is automatically chosen
	// 0x08 not assigned
	INF_SUPPORT_SKILL = 0x10,
	INF_TARGET_TRAP   = 0x20,
};

//Constants to identify a skill's nk value (damage properties)
//The NK value applies only to non INF_GROUND_SKILL skills
//when determining skill castend function to invoke.
enum e_skill_nk
{
	NK_NO_DAMAGE      = 0x01,
	NK_SPLASH         = 0x02|0x04, // 0x4 = splash & split
	NK_SPLASHSPLIT    = 0x04,
	NK_NO_CARDFIX_ATK = 0x08,
	NK_NO_ELEFIX      = 0x10,
	NK_IGNORE_DEF     = 0x20,
	NK_IGNORE_FLEE    = 0x40,
	NK_NO_CARDFIX_DEF = 0x80,
};

//A skill with 3 would be no damage + splash: area of effect.
//Constants to identify a skill's inf2 value.
enum e_skill_inf2
{
	INF2_QUEST_SKILL    = 0x0001,
	INF2_NPC_SKILL      = 0x0002, //NPC skills are those that players can't have in their skill tree.
	INF2_WEDDING_SKILL  = 0x0004,
	INF2_SPIRIT_SKILL   = 0x0008,
	INF2_GUILD_SKILL    = 0x0010,
	INF2_SONG_DANCE     = 0x0020,
	INF2_ENSEMBLE_SKILL = 0x0040,
	INF2_TRAP           = 0x0080,
	INF2_TARGET_SELF    = 0x0100, //Refers to ground placed skills that will target the caster as well (like Grandcross)
	INF2_NO_TARGET_SELF = 0x0200,
	INF2_PARTY_ONLY     = 0x0400,
	INF2_GUILD_ONLY     = 0x0800,
	INF2_NO_ENEMY       = 0x1000,
};

//Walk intervals at which chase-skills are attempted to be triggered.
#define WALK_SKILL_INTERVAL 5

// Flags passed to skill_attack/skill_area_sub
enum e_skill_display
{
	SD_LEVEL     = 0x1000, // skill_attack will send -1 instead of skill level (affects display of some skills)
	SD_ANIMATION = 0x2000, // skill_attack will use '5' instead of the skill's 'type' (this makes skills show an animation)
	SD_SPLASH    = 0x4000, // skill_area_sub will count targets in skill_area_temp[2]
	SD_PREAMBLE  = 0x8000, // skill_area_sub will transmit a 'magic' damage packet (-30000 dmg) for the first target selected
};

#define MAX_SKILL_ITEM_REQUIRE	10
struct skill_condition {
	int weapon,ammo,ammo_qty,hp,sp,zeny,spiritball,mhp,state;
	int itemid[MAX_SKILL_ITEM_REQUIRE],amount[MAX_SKILL_ITEM_REQUIRE];
};

// スキルデ?タベ?ス
struct s_skill_db {
	char name[NAME_LENGTH];
	char desc[40];
	int range[MAX_SKILL_LEVEL],hit,inf,element[MAX_SKILL_LEVEL],nk,splash[MAX_SKILL_LEVEL],max;
	int num[MAX_SKILL_LEVEL];
	int cast[MAX_SKILL_LEVEL],walkdelay[MAX_SKILL_LEVEL],delay[MAX_SKILL_LEVEL];
	int upkeep_time[MAX_SKILL_LEVEL],upkeep_time2[MAX_SKILL_LEVEL];
	int castcancel,cast_def_rate;
	int inf2,maxcount[MAX_SKILL_LEVEL],skill_type;
	int blewcount[MAX_SKILL_LEVEL];
	int hp[MAX_SKILL_LEVEL],sp[MAX_SKILL_LEVEL],mhp[MAX_SKILL_LEVEL],hp_rate[MAX_SKILL_LEVEL],sp_rate[MAX_SKILL_LEVEL],zeny[MAX_SKILL_LEVEL];
	int weapon,ammo,ammo_qty[MAX_SKILL_LEVEL],state,spiritball[MAX_SKILL_LEVEL];
	int itemid[MAX_SKILL_ITEM_REQUIRE],amount[MAX_SKILL_ITEM_REQUIRE];
	int castnodex[MAX_SKILL_LEVEL], delaynodex[MAX_SKILL_LEVEL];
	int nocast;
	int unit_id[2];
	int unit_layout_type[MAX_SKILL_LEVEL];
	int unit_range[MAX_SKILL_LEVEL];
	int unit_interval;
	int unit_target;
	int unit_flag;
};
extern struct s_skill_db skill_db[MAX_SKILL_DB];

#define MAX_SKILL_UNIT_LAYOUT	50
#define MAX_SQUARE_LAYOUT		5	// 11*11のユニット配置が最大
#define MAX_SKILL_UNIT_COUNT ((MAX_SQUARE_LAYOUT*2+1)*(MAX_SQUARE_LAYOUT*2+1))
struct s_skill_unit_layout {
	int count;
	int dx[MAX_SKILL_UNIT_COUNT];
	int dy[MAX_SKILL_UNIT_COUNT];
};

#define MAX_SKILLTIMERSKILL 15
struct skill_timerskill {
	int timer;
	int src_id;
	int target_id;
	int map;
	short x,y;
	short skill_id,skill_lv;
	int type; // a BF_ type (NOTE: some places use this as general-purpose storage...)
	int flag;
};

#define MAX_SKILLUNITGROUP 25
struct skill_unit_group {
	int src_id;
	int party_id;
	int guild_id;
	int bg_id;
	int map;
	int target_flag; //Holds BCT_* flag for battle_check_target
	int bl_flag;	//Holds BL_* flag for map_foreachin* functions
	unsigned int tick;
	int limit,interval;

	short skill_id,skill_lv;
	int val1,val2,val3;
	char *valstr;
	int unit_id;
	int group_id;
	int unit_count,alive_count;
	struct skill_unit *unit;
	struct {
		unsigned ammo_consume : 1;
		unsigned magic_power : 1;
		unsigned song_dance : 2; //0x1 Song/Dance, 0x2 Ensemble
	} state;
};

struct skill_unit {
	struct block_list bl;

	struct skill_unit_group *group;

	int limit;
	int val1,val2;
	short alive,range;
};

#define MAX_SKILLUNITGROUPTICKSET 25
struct skill_unit_group_tickset {
	unsigned int tick;
	int id;
};


enum {
	UF_DEFNOTENEMY   = 0x0001,	// If 'defunit_not_enemy' is set, the target is changed to 'friend'
	UF_NOREITERATION = 0x0002,	// Spell cannot be stacked
	UF_NOFOOTSET     = 0x0004,	// Spell cannot be cast near/on targets
	UF_NOOVERLAP     = 0x0008,	// Spell effects do not overlap
	UF_PATHCHECK     = 0x0010,	// Only cells with a shootable path will be placed
	UF_NOPC          = 0x0020,	// May not target players
	UF_NOMOB         = 0x0040,	// May not target mobs
	UF_SKILL         = 0x0080,	// May target skills
	UF_DANCE         = 0x0100,	// Dance
	UF_ENSEMBLE      = 0x0200,	// Duet
	UF_SONG          = 0x0400,	// Song
	UF_DUALMODE      = 0x0800,	// Spells should trigger both ontimer and onplace/onout/onleft effects.
};

// アイテム作成デ?タベ?ス
struct s_skill_produce_db {
	int nameid, trigger;
	int req_skill,req_skill_lv,itemlv;
	int mat_id[MAX_PRODUCE_RESOURCE],mat_amount[MAX_PRODUCE_RESOURCE];
};
extern struct s_skill_produce_db skill_produce_db[MAX_SKILL_PRODUCE_DB];

// 矢作成デ?タベ?ス
struct s_skill_arrow_db {
	int nameid, trigger;
	int cre_id[MAX_ARROW_RESOURCE],cre_amount[MAX_ARROW_RESOURCE];
};
extern struct s_skill_arrow_db skill_arrow_db[MAX_SKILL_ARROW_DB];

// アブラカダブラデ?タベ?ス
struct s_skill_abra_db {
	int skillid;
	int req_lv;
	int per;
};
extern struct s_skill_abra_db skill_abra_db[MAX_SKILL_ABRA_DB];

extern int enchant_eff[5];
extern int deluge_eff[5];

int do_init_skill(void);
int do_final_skill(void);

//Returns the cast type of the skill: ground cast, castend damage, castend no damage
enum { CAST_GROUND, CAST_DAMAGE, CAST_NODAMAGE };
int skill_get_casttype(int id); //[Skotlex]
// スキルデ?タベ?スへのアクセサ
//
int skill_get_index( int id );
int	skill_get_type( int id );
int	skill_get_hit( int id );
int	skill_get_inf( int id );
int	skill_get_ele( int id , int lv );
int	skill_get_nk( int id );
int	skill_get_max( int id );
int	skill_get_range( int id , int lv );
int	skill_get_range2(struct block_list *bl, int id, int lv);
int	skill_get_splash( int id , int lv );
int	skill_get_hp( int id ,int lv );
int	skill_get_mhp( int id ,int lv );
int	skill_get_sp( int id ,int lv );
int	skill_get_state(int id);
int	skill_get_zeny( int id ,int lv );
int	skill_get_num( int id ,int lv );
int	skill_get_cast( int id ,int lv );
int	skill_get_delay( int id ,int lv );
int	skill_get_walkdelay( int id ,int lv );
int	skill_get_time( int id ,int lv );
int	skill_get_time2( int id ,int lv );
int	skill_get_castnodex( int id ,int lv );
int	skill_get_castdef( int id );
int	skill_get_weapontype( int id );
int	skill_get_ammotype( int id );
int	skill_get_ammo_qty( int id, int lv );
int	skill_get_nocast( int id );
int	skill_get_unit_id(int id,int flag);
int	skill_get_inf2( int id );
int	skill_get_castcancel( int id );
int	skill_get_maxcount( int id ,int lv );
int	skill_get_blewcount( int id ,int lv );
int	skill_get_unit_flag( int id );
int	skill_get_unit_target( int id );
int	skill_tree_get_max( int id, int b_class );	// Celest
const char*	skill_get_name( int id ); 	// [Skotlex]
const char*	skill_get_desc( int id ); 	// [Skotlex]

int skill_name2id(const char* name);

int skill_isammotype(struct map_session_data *sd, int skill);
int skill_castend_id(int tid, unsigned int tick, int id, intptr data);
int skill_castend_pos(int tid, unsigned int tick, int id, intptr data);
int skill_castend_map( struct map_session_data *sd,short skill_num, const char *map);

int skill_cleartimerskill(struct block_list *src);
int skill_addtimerskill(struct block_list *src,unsigned int tick,int target,int x,int y,int skill_id,int skill_lv,int type,int flag);

// 追加?果
int skill_additional_effect( struct block_list* src, struct block_list *bl,int skillid,int skilllv,int attack_type,int dmg_lv,unsigned int tick);
int skill_counter_additional_effect( struct block_list* src, struct block_list *bl,int skillid,int skilllv,int attack_type,unsigned int tick);
int skill_blown(struct block_list* src, struct block_list* target, int count, int direction, int flag);
int skill_break_equip(struct block_list *bl, unsigned short where, int rate, int flag);
int skill_strip_equip(struct block_list *bl, unsigned short where, int rate, int lv, int time);
// ユニットスキル
struct skill_unit_group* skill_id2group(int group_id);
struct skill_unit_group *skill_unitsetting(struct block_list* src, short skillid, short skilllv, short x, short y, int flag);
struct skill_unit *skill_initunit (struct skill_unit_group *group, int idx, int x, int y, int val1, int val2);
int skill_delunit(struct skill_unit *unit);
struct skill_unit_group *skill_initunitgroup(struct block_list* src, int count, short skillid, short skilllv, int unit_id, int limit, int interval);
int skill_delunitgroup_(struct skill_unit_group *group, const char* file, int line, const char* func);
#define skill_delunitgroup(group) skill_delunitgroup_(group,__FILE__,__LINE__,__func__)
int skill_clear_unitgroup(struct block_list *src);
int skill_clear_group(struct block_list *bl, int flag);

int skill_unit_ondamaged(struct skill_unit *src,struct block_list *bl,int damage,unsigned int tick);

int skill_castfix( struct block_list *bl, int skill_id, int skill_lv);
int skill_castfix_sc( struct block_list *bl, int time);
int skill_delayfix( struct block_list *bl, int skill_id, int skill_lv);

// Skill conditions check and remove [Inkfish]
int skill_check_condition_castbegin(struct map_session_data *sd, short skill, short lv);
int skill_check_condition_castend(struct map_session_data *sd, short skill, short lv);
int skill_consume_requirement(struct map_session_data *sd, short skill, short lv, short type);
struct skill_condition skill_get_requirement(struct map_session_data *sd, short skill, short lv);

int skill_check_pc_partner(struct map_session_data *sd, short skill_id, short* skill_lv, int range, int cast_flag);
// -- moonsoul	(added skill_check_unit_cell)
int skill_check_unit_cell(int skillid,int m,int x,int y,int unit_id);
int skill_unit_out_all( struct block_list *bl,unsigned int tick,int range);
int skill_unit_move(struct block_list *bl,unsigned int tick,int flag);
int skill_unit_move_unit_group( struct skill_unit_group *group, int m,int dx,int dy);

struct skill_unit_group *skill_check_dancing( struct block_list *src );

// Guild skills [celest]
int skill_guildaura_sub (struct block_list *bl,va_list ap);

// 詠唱キャンセル
int skill_castcancel(struct block_list *bl,int type);

int skill_sit (struct map_session_data *sd, int type);
void skill_brandishspear(struct block_list* src, struct block_list* bl, int skillid, int skilllv, unsigned int tick, int flag);
void skill_repairweapon(struct map_session_data *sd, int idx);
void skill_identify(struct map_session_data *sd,int idx);
void skill_weaponrefine(struct map_session_data *sd,int idx); // [Celest]
int skill_autospell(struct map_session_data *md,int skillid);

int skill_calc_heal(struct block_list *src, struct block_list *target, int skill_id, int skill_lv, bool heal);

bool skill_check_cloaking(struct block_list *bl, struct status_change_entry *sce);

// ステ?タス異常
int skill_enchant_elemental_end(struct block_list *bl, int type);
int skillnotok(int skillid, struct map_session_data *sd);
int skillnotok_hom(int skillid, struct homun_data *hd);
int skillnotok_mercenary(int skillid, struct mercenary_data *md);

int skill_chastle_mob_changetarget(struct block_list *bl,va_list ap);

// アイテム作成
int skill_can_produce_mix( struct map_session_data *sd, int nameid, int trigger, int qty);
int skill_produce_mix( struct map_session_data *sd, int skill_id, int nameid, int slot1, int slot2, int slot3, int qty );

int skill_arrow_create( struct map_session_data *sd,int nameid);

// mobスキルのため
int skill_castend_nodamage_id( struct block_list *src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );
int skill_castend_damage_id( struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );
int skill_castend_pos2( struct block_list *src, int x,int y,int skillid,int skilllv,unsigned int tick,int flag);

int skill_blockpc_start (struct map_session_data*,int,int);
int skill_blockhomun_start (struct homun_data*,int,int);
int skill_blockmerc_start (struct mercenary_data*,int,int);

// スキル攻?一括?理
int skill_attack( int attack_type, struct block_list* src, struct block_list *dsrc,struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );

void skill_reload(void);

enum {
	ST_NONE,
	ST_HIDING,
	ST_CLOAKING,
	ST_HIDDEN,
	ST_RIDING,
	ST_FALCON,
	ST_CART,
	ST_SHIELD,
	ST_SIGHT,
	ST_EXPLOSIONSPIRITS,
	ST_CARTBOOST,
	ST_RECOV_WEIGHT_RATE,
	ST_MOVE_ENABLE,
	ST_WATER,
};

enum e_skill {
	NV_BASIC = 1,

	SM_SWORD,
	SM_TWOHAND,
	SM_RECOVERY,
	SM_BASH,
	SM_PROVOKE,
	SM_MAGNUM,
	SM_ENDURE,

	MG_SRECOVERY,
	MG_SIGHT,
	MG_NAPALMBEAT,
	MG_SAFETYWALL,
	MG_SOULSTRIKE,
	MG_COLDBOLT,
	MG_FROSTDIVER,
	MG_STONECURSE,
	MG_FIREBALL,
	MG_FIREWALL,
	MG_FIREBOLT,
	MG_LIGHTNINGBOLT,
	MG_THUNDERSTORM,

	AL_DP,
	AL_DEMONBANE,
	AL_RUWACH,
	AL_PNEUMA,
	AL_TELEPORT,
	AL_WARP,
	AL_HEAL,
	AL_INCAGI,
	AL_DECAGI,
	AL_HOLYWATER,
	AL_CRUCIS,
	AL_ANGELUS,
	AL_BLESSING,
	AL_CURE,

	MC_INCCARRY,
	MC_DISCOUNT,
	MC_OVERCHARGE,
	MC_PUSHCART,
	MC_IDENTIFY,
	MC_VENDING,
	MC_MAMMONITE,

	AC_OWL,
	AC_VULTURE,
	AC_CONCENTRATION,
	AC_DOUBLE,
	AC_SHOWER,

	TF_DOUBLE,
	TF_MISS,
	TF_STEAL,
	TF_HIDING,
	TF_POISON,
	TF_DETOXIFY,

	ALL_RESURRECTION,

	KN_SPEARMASTERY,
	KN_PIERCE,
	KN_BRANDISHSPEAR,
	KN_SPEARSTAB,
	KN_SPEARBOOMERANG,
	KN_TWOHANDQUICKEN,
	KN_AUTOCOUNTER,
	KN_BOWLINGBASH,
	KN_RIDING,
	KN_CAVALIERMASTERY,

	PR_MACEMASTERY,
	PR_IMPOSITIO,
	PR_SUFFRAGIUM,
	PR_ASPERSIO,
	PR_BENEDICTIO,
	PR_SANCTUARY,
	PR_SLOWPOISON,
	PR_STRECOVERY,
	PR_KYRIE,
	PR_MAGNIFICAT,
	PR_GLORIA,
	PR_LEXDIVINA,
	PR_TURNUNDEAD,
	PR_LEXAETERNA,
	PR_MAGNUS,

	WZ_FIREPILLAR,
	WZ_SIGHTRASHER,
	WZ_FIREIVY,
	WZ_METEOR,
	WZ_JUPITEL,
	WZ_VERMILION,
	WZ_WATERBALL,
	WZ_ICEWALL,
	WZ_FROSTNOVA,
	WZ_STORMGUST,
	WZ_EARTHSPIKE,
	WZ_HEAVENDRIVE,
	WZ_QUAGMIRE,
	WZ_ESTIMATION,

	BS_IRON,
	BS_STEEL,
	BS_ENCHANTEDSTONE,
	BS_ORIDEOCON,
	BS_DAGGER,
	BS_SWORD,
	BS_TWOHANDSWORD,
	BS_AXE,
	BS_MACE,
	BS_KNUCKLE,
	BS_SPEAR,
	BS_HILTBINDING,
	BS_FINDINGORE,
	BS_WEAPONRESEARCH,
	BS_REPAIRWEAPON,
	BS_SKINTEMPER,
	BS_HAMMERFALL,
	BS_ADRENALINE,
	BS_WEAPONPERFECT,
	BS_OVERTHRUST,
	BS_MAXIMIZE,

	HT_SKIDTRAP,
	HT_LANDMINE,
	HT_ANKLESNARE,
	HT_SHOCKWAVE,
	HT_SANDMAN,
	HT_FLASHER,
	HT_FREEZINGTRAP,
	HT_BLASTMINE,
	HT_CLAYMORETRAP,
	HT_REMOVETRAP,
	HT_TALKIEBOX,
	HT_BEASTBANE,
	HT_FALCON,
	HT_STEELCROW,
	HT_BLITZBEAT,
	HT_DETECTING,
	HT_SPRINGTRAP,

	AS_RIGHT,
	AS_LEFT,
	AS_KATAR,
	AS_CLOAKING,
	AS_SONICBLOW,
	AS_GRIMTOOTH,
	AS_ENCHANTPOISON,
	AS_POISONREACT,
	AS_VENOMDUST,
	AS_SPLASHER,

	NV_FIRSTAID,
	NV_TRICKDEAD,
	SM_MOVINGRECOVERY,
	SM_FATALBLOW,
	SM_AUTOBERSERK,
	AC_MAKINGARROW,
	AC_CHARGEARROW,
	TF_SPRINKLESAND,
	TF_BACKSLIDING,
	TF_PICKSTONE,
	TF_THROWSTONE,
	MC_CARTREVOLUTION,
	MC_CHANGECART,
	MC_LOUD,
	AL_HOLYLIGHT,
	MG_ENERGYCOAT,

	NPC_PIERCINGATT,
	NPC_MENTALBREAKER,
	NPC_RANGEATTACK,
	NPC_ATTRICHANGE,
	NPC_CHANGEWATER,
	NPC_CHANGEGROUND,
	NPC_CHANGEFIRE,
	NPC_CHANGEWIND,
	NPC_CHANGEPOISON,
	NPC_CHANGEHOLY,
	NPC_CHANGEDARKNESS,
	NPC_CHANGETELEKINESIS,
	NPC_CRITICALSLASH,
	NPC_COMBOATTACK,
	NPC_GUIDEDATTACK,
	NPC_SELFDESTRUCTION,
	NPC_SPLASHATTACK,
	NPC_SUICIDE,
	NPC_POISON,
	NPC_BLINDATTACK,
	NPC_SILENCEATTACK,
	NPC_STUNATTACK,
	NPC_PETRIFYATTACK,
	NPC_CURSEATTACK,
	NPC_SLEEPATTACK,
	NPC_RANDOMATTACK,
	NPC_WATERATTACK,
	NPC_GROUNDATTACK,
	NPC_FIREATTACK,
	NPC_WINDATTACK,
	NPC_POISONATTACK,
	NPC_HOLYATTACK,
	NPC_DARKNESSATTACK,
	NPC_TELEKINESISATTACK,
	NPC_MAGICALATTACK,
	NPC_METAMORPHOSIS,
	NPC_PROVOCATION,
	NPC_SMOKING,
	NPC_SUMMONSLAVE,
	NPC_EMOTION,
	NPC_TRANSFORMATION,
	NPC_BLOODDRAIN,
	NPC_ENERGYDRAIN,
	NPC_KEEPING,
	NPC_DARKBREATH,
	NPC_DARKBLESSING,
	NPC_BARRIER,
	NPC_DEFENDER,
	NPC_LICK,
	NPC_HALLUCINATION,
	NPC_REBIRTH,
	NPC_SUMMONMONSTER,

	RG_SNATCHER,
	RG_STEALCOIN,
	RG_BACKSTAP,
	RG_TUNNELDRIVE,
	RG_RAID,
	RG_STRIPWEAPON,
	RG_STRIPSHIELD,
	RG_STRIPARMOR,
	RG_STRIPHELM,
	RG_INTIMIDATE,
	RG_GRAFFITI,
	RG_FLAGGRAFFITI,
	RG_CLEANER,
	RG_GANGSTER,
	RG_COMPULSION,
	RG_PLAGIARISM,

	AM_AXEMASTERY,
	AM_LEARNINGPOTION,
	AM_PHARMACY,
	AM_DEMONSTRATION,
	AM_ACIDTERROR,
	AM_POTIONPITCHER,
	AM_CANNIBALIZE,
	AM_SPHEREMINE,
	AM_CP_WEAPON,
	AM_CP_SHIELD,
	AM_CP_ARMOR,
	AM_CP_HELM,
	AM_BIOETHICS,
	AM_BIOTECHNOLOGY,
	AM_CREATECREATURE,
	AM_CULTIVATION,
	AM_FLAMECONTROL,
	AM_CALLHOMUN,
	AM_REST,
	AM_DRILLMASTER,
	AM_HEALHOMUN,
	AM_RESURRECTHOMUN,

	CR_TRUST,
	CR_AUTOGUARD,
	CR_SHIELDCHARGE,
	CR_SHIELDBOOMERANG,
	CR_REFLECTSHIELD,
	CR_HOLYCROSS,
	CR_GRANDCROSS,
	CR_DEVOTION,
	CR_PROVIDENCE,
	CR_DEFENDER,
	CR_SPEARQUICKEN,

	MO_IRONHAND,
	MO_SPIRITSRECOVERY,
	MO_CALLSPIRITS,
	MO_ABSORBSPIRITS,
	MO_TRIPLEATTACK,
	MO_BODYRELOCATION,
	MO_DODGE,
	MO_INVESTIGATE,
	MO_FINGEROFFENSIVE,
	MO_STEELBODY,
	MO_BLADESTOP,
	MO_EXPLOSIONSPIRITS,
	MO_EXTREMITYFIST,
	MO_CHAINCOMBO,
	MO_COMBOFINISH,

	SA_ADVANCEDBOOK,
	SA_CASTCANCEL,
	SA_MAGICROD,
	SA_SPELLBREAKER,
	SA_FREECAST,
	SA_AUTOSPELL,
	SA_FLAMELAUNCHER,
	SA_FROSTWEAPON,
	SA_LIGHTNINGLOADER,
	SA_SEISMICWEAPON,
	SA_DRAGONOLOGY,
	SA_VOLCANO,
	SA_DELUGE,
	SA_VIOLENTGALE,
	SA_LANDPROTECTOR,
	SA_DISPELL,
	SA_ABRACADABRA,
	SA_MONOCELL,
	SA_CLASSCHANGE,
	SA_SUMMONMONSTER,
	SA_REVERSEORCISH,
	SA_DEATH,
	SA_FORTUNE,
	SA_TAMINGMONSTER,
	SA_QUESTION,
	SA_GRAVITY,
	SA_LEVELUP,
	SA_INSTANTDEATH,
	SA_FULLRECOVERY,
	SA_COMA,

	BD_ADAPTATION,
	BD_ENCORE,
	BD_LULLABY,
	BD_RICHMANKIM,
	BD_ETERNALCHAOS,
	BD_DRUMBATTLEFIELD,
	BD_RINGNIBELUNGEN,
	BD_ROKISWEIL,
	BD_INTOABYSS,
	BD_SIEGFRIED,
	BD_RAGNAROK,

	BA_MUSICALLESSON,
	BA_MUSICALSTRIKE,
	BA_DISSONANCE,
	BA_FROSTJOKER,
	BA_WHISTLE,
	BA_ASSASSINCROSS,
	BA_POEMBRAGI,
	BA_APPLEIDUN,

	DC_DANCINGLESSON,
	DC_THROWARROW,
	DC_UGLYDANCE,
	DC_SCREAM,
	DC_HUMMING,
	DC_DONTFORGETME,
	DC_FORTUNEKISS,
	DC_SERVICEFORYOU,

	NPC_RANDOMMOVE,
	NPC_SPEEDUP,
	NPC_REVENGE,

	WE_MALE,
	WE_FEMALE,
	WE_CALLPARTNER,

	ITM_TOMAHAWK,

	NPC_DARKCROSS,
	NPC_GRANDDARKNESS,
	NPC_DARKSTRIKE,
	NPC_DARKTHUNDER,
	NPC_STOP,
	NPC_WEAPONBRAKER,
	NPC_ARMORBRAKE,
	NPC_HELMBRAKE,
	NPC_SHIELDBRAKE,
	NPC_UNDEADATTACK,
	NPC_CHANGEUNDEAD,
	NPC_POWERUP,
	NPC_AGIUP,
	NPC_SIEGEMODE,
	NPC_CALLSLAVE,
	NPC_INVISIBLE,
	NPC_RUN,

	LK_AURABLADE,
	LK_PARRYING,
	LK_CONCENTRATION,
	LK_TENSIONRELAX,
	LK_BERSERK,
	LK_FURY,
	HP_ASSUMPTIO,
	HP_BASILICA,
	HP_MEDITATIO,
	HW_SOULDRAIN,
	HW_MAGICCRASHER,
	HW_MAGICPOWER,
	PA_PRESSURE,
	PA_SACRIFICE,
	PA_GOSPEL,
	CH_PALMSTRIKE,
	CH_TIGERFIST,
	CH_CHAINCRUSH,
	PF_HPCONVERSION,
	PF_SOULCHANGE,
	PF_SOULBURN,
	ASC_KATAR,
	ASC_HALLUCINATION,
	ASC_EDP,
	ASC_BREAKER,
	SN_SIGHT,
	SN_FALCONASSAULT,
	SN_SHARPSHOOTING,
	SN_WINDWALK,
	WS_MELTDOWN,
	WS_CREATECOIN,
	WS_CREATENUGGET,
	WS_CARTBOOST,
	WS_SYSTEMCREATE,
	ST_CHASEWALK,
	ST_REJECTSWORD,
	ST_STEALBACKPACK,
	CR_ALCHEMY,
	CR_SYNTHESISPOTION,
	CG_ARROWVULCAN,
	CG_MOONLIT,
	CG_MARIONETTE,
	LK_SPIRALPIERCE,
	LK_HEADCRUSH,
	LK_JOINTBEAT,
	HW_NAPALMVULCAN,
	CH_SOULCOLLECT,
	PF_MINDBREAKER,
	PF_MEMORIZE,
	PF_FOGWALL,
	PF_SPIDERWEB,
	ASC_METEORASSAULT,
	ASC_CDP,
	WE_BABY,
	WE_CALLPARENT,
	WE_CALLBABY,

	TK_RUN,
	TK_READYSTORM,
	TK_STORMKICK,
	TK_READYDOWN,
	TK_DOWNKICK,
	TK_READYTURN,
	TK_TURNKICK,
	TK_READYCOUNTER,
	TK_COUNTER,
	TK_DODGE,
	TK_JUMPKICK,
	TK_HPTIME,
	TK_SPTIME,
	TK_POWER,
	TK_SEVENWIND,
	TK_HIGHJUMP,
	SG_FEEL,
	SG_SUN_WARM,
	SG_MOON_WARM,
	SG_STAR_WARM,
	SG_SUN_COMFORT,
	SG_MOON_COMFORT,
	SG_STAR_COMFORT,
	SG_HATE,
	SG_SUN_ANGER,
	SG_MOON_ANGER,
	SG_STAR_ANGER,
	SG_SUN_BLESS,
	SG_MOON_BLESS,
	SG_STAR_BLESS,
	SG_DEVIL,
	SG_FRIEND,
	SG_KNOWLEDGE,
	SG_FUSION,
	SL_ALCHEMIST,
	AM_BERSERKPITCHER,
	SL_MONK,
	SL_STAR,
	SL_SAGE,
	SL_CRUSADER,
	SL_SUPERNOVICE,
	SL_KNIGHT,
	SL_WIZARD,
	SL_PRIEST,
	SL_BARDDANCER,
	SL_ROGUE,
	SL_ASSASIN,
	SL_BLACKSMITH,
	BS_ADRENALINE2,
	SL_HUNTER,
	SL_SOULLINKER,
	SL_KAIZEL,
	SL_KAAHI,
	SL_KAUPE,
	SL_KAITE,
	SL_KAINA,
	SL_STIN,
	SL_STUN,
	SL_SMA,
	SL_SWOO,
	SL_SKE,
	SL_SKA,

	SM_SELFPROVOKE,
	NPC_EMOTION_ON,
	ST_PRESERVE,
	ST_FULLSTRIP,
	WS_WEAPONREFINE,
	CR_SLIMPITCHER,
	CR_FULLPROTECTION,
	PA_SHIELDCHAIN,
	HP_MANARECHARGE,
	PF_DOUBLECASTING,
	HW_GANBANTEIN,
	HW_GRAVITATION,
	WS_CARTTERMINATION,
	WS_OVERTHRUSTMAX,
	CG_LONGINGFREEDOM,
	CG_HERMODE,
	CG_TAROTCARD,
	CR_ACIDDEMONSTRATION,
	CR_CULTIVATION,
	ITEM_ENCHANTARMS,
	TK_MISSION,
	SL_HIGH,
	KN_ONEHAND,
	AM_TWILIGHT1,
	AM_TWILIGHT2,
	AM_TWILIGHT3,
	HT_POWER,
	GS_GLITTERING,
	GS_FLING,
	GS_TRIPLEACTION,
	GS_BULLSEYE,
	GS_MADNESSCANCEL,
	GS_ADJUSTMENT,
	GS_INCREASING,
	GS_MAGICALBULLET,
	GS_CRACKER,
	GS_SINGLEACTION,
	GS_SNAKEEYE,
	GS_CHAINACTION,
	GS_TRACKING,
	GS_DISARM,
	GS_PIERCINGSHOT,
	GS_RAPIDSHOWER,
	GS_DESPERADO,
	GS_GATLINGFEVER,
	GS_DUST,
	GS_FULLBUSTER,
	GS_SPREADATTACK,
	GS_GROUNDDRIFT,
	NJ_TOBIDOUGU,
	NJ_SYURIKEN,
	NJ_KUNAI,
	NJ_HUUMA,
	NJ_ZENYNAGE,
	NJ_TATAMIGAESHI,
	NJ_KASUMIKIRI,
	NJ_SHADOWJUMP,
	NJ_KIRIKAGE,
	NJ_UTSUSEMI,
	NJ_BUNSINJYUTSU,
	NJ_NINPOU,
	NJ_KOUENKA,
	NJ_KAENSIN,
	NJ_BAKUENRYU,
	NJ_HYOUSENSOU,
	NJ_SUITON,
	NJ_HYOUSYOURAKU,
	NJ_HUUJIN,
	NJ_RAIGEKISAI,
	NJ_KAMAITACHI,
	NJ_NEN,
	NJ_ISSEN,

	NPC_EARTHQUAKE = 653,
	NPC_FIREBREATH,
	NPC_ICEBREATH,
	NPC_THUNDERBREATH,
	NPC_ACIDBREATH,
	NPC_DARKNESSBREATH,
	NPC_DRAGONFEAR,
	NPC_BLEEDING,
	NPC_PULSESTRIKE,
	NPC_HELLJUDGEMENT,
	NPC_WIDESILENCE,
	NPC_WIDEFREEZE,
	NPC_WIDEBLEEDING,
	NPC_WIDESTONE,
	NPC_WIDECONFUSE,
	NPC_WIDESLEEP,
	NPC_WIDESIGHT,
	NPC_EVILLAND,
	NPC_MAGICMIRROR,
	NPC_SLOWCAST,
	NPC_CRITICALWOUND,
	NPC_EXPULSION,
	NPC_STONESKIN,
	NPC_ANTIMAGIC,
	NPC_WIDECURSE,
	NPC_WIDESTUN,
	NPC_VAMPIRE_GIFT,
	NPC_WIDESOULDRAIN,

	ALL_INCCARRY,
	NPC_TALK,
	NPC_HELLPOWER,
	NPC_WIDEHELLDIGNITY,
	NPC_INVINCIBLE,
	NPC_INVINCIBLEOFF,
	NPC_ALLHEAL,
	GM_SANDMAN,
	CASH_BLESSING,
	CASH_INCAGI,
	CASH_ASSUMPTIO,
	ALL_CATCRY,
	ALL_PARTYFLEE,
	ALL_ANGEL_PROTECT,
	ALL_DREAM_SUMMERNIGHT,
	NPC_CHANGEUNDEAD2,
	ALL_REVERSEORCISH,
	ALL_WEWISH,
	ALL_SONKRAN,

	KN_CHARGEATK = 1001,
	CR_SHRINK,
	AS_SONICACCEL,
	AS_VENOMKNIFE,
	RG_CLOSECONFINE,
	WZ_SIGHTBLASTER,
	SA_CREATECON,
	SA_ELEMENTWATER,
	HT_PHANTASMIC,
	BA_PANGVOICE,
	DC_WINKCHARM,
	BS_UNFAIRLYTRICK,
	BS_GREED,
	PR_REDEMPTIO,
	MO_KITRANSLATION,
	MO_BALKYOUNG,
	SA_ELEMENTGROUND,
	SA_ELEMENTFIRE,
	SA_ELEMENTWIND,

	RK_ENCHANTBLADE = 2001,
	RK_SONICWAVE,
	RK_DEATHBOUND,
	RK_HUNDREDSPEAR,
	RK_WINDCUTTER,
	RK_IGNITIONBREAK,
	RK_DRAGONTRAINING,
	RK_DRAGONBREATH,
	RK_DRAGONHOWLING,
	RK_RUNEMASTERY,
	RK_MILLENNIUMSHIELD,
	RK_CRUSHSTRIKE,
	RK_REFRESH,
	RK_GIANTGROWTH,
	RK_STONEHARDSKIN,
	RK_VITALITYACTIVATION,
	RK_STORMBLAST,
	RK_FIGHTINGSPIRIT,
	RK_ABUNDANCE,
	RK_PHANTOMTHRUST,

	GC_VENOMIMPRESS,
	GC_CROSSIMPACT,
	GC_DARKILLUSION,
	GC_RESEARCHNEWPOISON,
	GC_CREATENEWPOISON,
	GC_ANTIDOTE,
	GC_POISONINGWEAPON,
	GC_WEAPONBLOCKING,
	GC_COUNTERSLASH,
	GC_WEAPONCRUSH,
	GC_VENOMPRESSURE,
	GC_POISONSMOKE,
	GC_CLOAKINGEXCEED,
	GC_PHANTOMMENACE,
	GC_HALLUCINATIONWALK,
	GC_ROLLINGCUTTER,
	GC_CROSSRIPPERSLASHER,

	AB_JUDEX,
	AB_ANCILLA,
	AB_ADORAMUS,
	AB_CLEMENTIA,
	AB_CANTO,
	AB_CHEAL,
	AB_EPICLESIS,
	AB_PRAEFATIO,
	AB_ORATIO,
	AB_LAUDAAGNUS,
	AB_LAUDARAMUS,
	AB_EUCHARISTICA,
	AB_RENOVATIO,
	AB_HIGHNESSHEAL,
	AB_CLEARANCE,
	AB_EXPIATIO,
	AB_DUPLELIGHT,
	AB_DUPLELIGHT_MELEE,
	AB_DUPLELIGHT_MAGIC,
	AB_SILENTIUM,

	WL_WHITEIMPRISON = 2201,
	WL_SOULEXPANSION,
	WL_FROSTMISTY,
	WL_JACKFROST,
	WL_MARSHOFABYSS,
	WL_RECOGNIZEDSPELL,
	WL_SIENNAEXECRATE,
	WL_RADIUS,
	WL_STASIS,
	WL_DRAINLIFE,
	WL_CRIMSONROCK,
	WL_HELLINFERNO,
	WL_COMET,
	WL_CHAINLIGHTNING,
	WL_CHAINLIGHTNING_ATK,
	WL_EARTHSTRAIN,
	WL_TETRAVORTEX,
	WL_TETRAVORTEX_FIRE,
	WL_TETRAVORTEX_WATER,
	WL_TETRAVORTEX_WIND,
	WL_TETRAVORTEX_GROUND,
	WL_SUMMONFB,
	WL_SUMMONBL,
	WL_SUMMONWB,
	WL_SUMMON_ATK_FIRE,
	WL_SUMMON_ATK_WIND,
	WL_SUMMON_ATK_WATER,
	WL_SUMMON_ATK_GROUND,
	WL_SUMMONSTONE,
	WL_RELEASE,
	WL_READING_SB,
	WL_FREEZE_SP,

	RA_ARROWSTORM,
	RA_FEARBREEZE,
	RA_RANGERMAIN,
	RA_AIMEDBOLT,
	RA_DETONATOR,
	RA_ELECTRICSHOCKER,
	RA_CLUSTERBOMB,
	RA_WUGMASTERY,
	RA_WUGRIDER,
	RA_WUGDASH,
	RA_WUGSTRIKE,
	RA_WUGBITE,
	RA_TOOTHOFWUG,
	RA_SENSITIVEKEEN,
	RA_CAMOUFLAGE,
	RA_RESEARCHTRAP,
	RA_MAGENTATRAP,
	RA_COBALTTRAP,
	RA_MAIZETRAP,
	RA_VERDURETRAP,
	RA_FIRINGTRAP,
	RA_ICEBOUNDTRAP,

	NC_MADOLICENCE,
	NC_BOOSTKNUCKLE,
	NC_PILEBUNKER,
	NC_VULCANARM,
	NC_FLAMELAUNCHER,
	NC_COLDSLOWER,
	NC_ARMSCANNON,
	NC_ACCELERATION,
	NC_HOVERING,
	NC_F_SIDESLIDE,
	NC_B_SIDESLIDE,
	NC_MAINFRAME,
	NC_SELFDESTRUCTION,
	NC_SHAPESHIFT,
	NC_EMERGENCYCOOL,
	NC_INFRAREDSCAN,
	NC_ANALYZE,
	NC_MAGNETICFIELD,
	NC_NEUTRALBARRIER,
	NC_STEALTHFIELD,
	NC_REPAIR,
	NC_TRAININGAXE,
	NC_RESEARCHFE,
	NC_AXEBOOMERANG,
	NC_POWERSWING,
	NC_AXETORNADO,
	NC_SILVERSNIPER,
	NC_MAGICDECOY,
	NC_DISJOINT,

	SC_FATALMENACE,
	SC_REPRODUCE,
	SC_AUTOSHADOWSPELL,
	SC_SHADOWFORM,
	SC_TRIANGLESHOT,
	SC_BODYPAINT,
	SC_INVISIBILITY,
	SC_DEADLYINFECT,
	SC_ENERVATION,
	SC_GROOMY,
	SC_IGNORANCE,
	SC_LAZINESS,
	SC_UNLUCKY,
	SC_WEAKNESS,
	SC_STRIPACCESSARY,
	SC_MANHOLE,
	SC_DIMENSIONDOOR,
	SC_CHAOSPANIC,
	SC_MAELSTROM,
	SC_BLOODYLUST,
	SC_FEINTBOMB,

	LG_CANNONSPEAR = 2307,
	LG_BANISHINGPOINT,
	LG_TRAMPLE,
	LG_SHIELDPRESS,
	LG_REFLECTDAMAGE,
	LG_PINPOINTATTACK,
	LG_FORCEOFVANGUARD,
	LG_RAGEBURST,
	LG_SHIELDSPELL,
	LG_EXEEDBREAK,
	LG_OVERBRAND,
	LG_PRESTIGE,
	LG_BANDING,
	LG_MOONSLASHER,
	LG_RAYOFGENESIS,
	LG_PIETY,
	LG_EARTHDRIVE,
	LG_HESPERUSLIT,
	LG_INSPIRATION,

	SR_DRAGONCOMBO,
	SR_SKYNETBLOW,
	SR_EARTHSHAKER,
	SR_FALLENEMPIRE,
	SR_TIGERCANNON,
	SR_HELLGATE,
	SR_RAMPAGEBLASTER,
	SR_CRESCENTELBOW,
	SR_CURSEDCIRCLE,
	SR_LIGHTNINGWALK,
	SR_KNUCKLEARROW,
	SR_WINDMILL,
	SR_RAISINGDRAGON,
	SR_GENTLETOUCH,
	SR_ASSIMILATEPOWER,
	SR_POWERVELOCITY,
	SR_CRESCENTELBOW_AUTOSPELL,
	SR_GATEOFHELL,
	SR_GENTLETOUCH_QUIET,
	SR_GENTLETOUCH_CURE,
	SR_GENTLETOUCH_ENERGYGAIN,
	SR_GENTLETOUCH_CHANGE,
	SR_GENTLETOUCH_REVITALIZE,

	WA_SWING_DANCE = 2350,
	WA_SYMPHONY_OF_LOVER,
	WA_MOONLIT_SERENADE,
	MI_RUSH_WINDMILL = 2381,
	MI_ECHOSONG,
	MI_HARMONIZE,
	WM_LESSON = 2412,
	WM_METALICSOUND,
	WM_REVERBERATION,
	WM_REVERBERATION_MELEE,
	WM_REVERBERATION_MAGIC,
	WM_DOMINION_IMPULSE,
	WM_SEVERE_RAINSTORM,
	WM_POEMOFNETHERWORLD,
	WM_VOICEOFSIREN,
	WM_DEADHILLHERE,
	WM_LULLABY_DEEPSLEEP,
	WM_SIRCLEOFNATURE,
	WM_RANDOMIZESPELL,
	WM_GLOOMYDAY,
	WM_GREAT_ECHO,
	WM_SONG_OF_MANA,
	WM_DANCE_WITH_WUG,
	WM_SOUND_OF_DESTRUCTION,
	WM_SATURDAY_NIGHT_FEVER,
	WM_LERADS_DEW,
	WM_MELODYOFSINK,
	WM_BEYOND_OF_WARCRY,
	WM_UNLIMITED_HUMMING_VOICE,

	SO_FIREWALK = 2443,
	SO_ELECTRICWALK,
	SO_SPELLFIST,
	SO_EARTHGRAVE,
	SO_DIAMONDDUST,
	SO_POISON_BUSTER,
	SO_PSYCHIC_WAVE,
	SO_CLOUD_KILL,
	SO_STRIKING,
	SO_WARMER,
	SO_VACUUM_EXTREME,
	SO_VARETYR_SPEAR,
	SO_ARRULLO,
	SO_EL_CONTROL,
	SO_SUMMON_AGNI,
	SO_SUMMON_AQUA,
	SO_SUMMON_VENTUS,
	SO_SUMMON_TERA,
	SO_EL_ACTION,
	SO_EL_ANALYSIS,
	SO_EL_SYMPATHY,
	SO_EL_CURE,
	SO_FIRE_INSIGNIA,
	SO_WATER_INSIGNIA,
	SO_WIND_INSIGNIA,
	SO_EARTH_INSIGNIA,

	GN_TRAINING_SWORD = 2474,
	GN_REMODELING_CART,
	GN_CART_TORNADO,
	GN_CARTCANNON,
	GN_CARTBOOST,
	GN_THORNS_TRAP,
	GN_BLOOD_SUCKER,
	GN_SPORE_EXPLOSION,
	GN_WALLOFTHORN,
	GN_CRAZYWEED,
	GN_CRAZYWEED_ATK,
	GN_DEMONIC_FIRE,
	GN_FIRE_EXPANSION,
	GN_FIRE_EXPANSION_SMOKE_POWDER,
	GN_FIRE_EXPANSION_TEAR_GAS,
	GN_FIRE_EXPANSION_ACID,
	GN_HELLS_PLANT,
	GN_HELLS_PLANT_ATK,
	GN_MANDRAGORA,
	GN_SLINGITEM,
	GN_CHANGEMATERIAL,
	GN_MIX_COOKING,
	GN_MAKEBOMB,
	GN_S_PHARMACY,
	GN_SLINGITEM_RANGEMELEEATK,

	AB_SECRAMENT,
	WM_SEVERE_RAINSTORM_MELEE,
	SR_HOWLINGOFLION,
	SR_RIDEINLIGHTNING,
	LG_OVERBRAND_BRANDISH,
	LG_OVERBRAND_PLUSATK,

	ALL_ODINS_RECALL = 2533,
	RETURN_TO_ELDICASTES,
	ALL_BUYING_STORE,

	HLIF_HEAL = 8001,
	HLIF_AVOID,
	HLIF_BRAIN,
	HLIF_CHANGE,
	HAMI_CASTLE,
	HAMI_DEFENCE,
	HAMI_SKIN,
	HAMI_BLOODLUST,
	HFLI_MOON,
	HFLI_FLEET,
	HFLI_SPEED,
	HFLI_SBR44,
	HVAN_CAPRICE,
	HVAN_CHAOTIC,
	HVAN_INSTRUCT,
	HVAN_EXPLOSION,

	MS_BASH = 8201,
	MS_MAGNUM,
	MS_BOWLINGBASH,
	MS_PARRYING,
	MS_REFLECTSHIELD,
	MS_BERSERK,
	MA_DOUBLE,
	MA_SHOWER,
	MA_SKIDTRAP,
	MA_LANDMINE,
	MA_SANDMAN,
	MA_FREEZINGTRAP,
	MA_REMOVETRAP,
	MA_CHARGEARROW,
	MA_SHARPSHOOTING,
	ML_PIERCE,
	ML_BRANDISH,
	ML_SPIRALPIERCE,
	ML_DEFENDER,
	ML_AUTOGUARD,
	ML_DEVOTION,
	MER_MAGNIFICAT,
	MER_QUICKEN,
	MER_SIGHT,
	MER_CRASH,
	MER_REGAIN,
	MER_TENDER,
	MER_BENEDICTION,
	MER_RECUPERATE,
	MER_MENTALCURE,
	MER_COMPRESS,
	MER_PROVOKE,
	MER_AUTOBERSERK,
	MER_DECAGI,
	MER_SCAPEGOAT,
	MER_LEXDIVINA,
	MER_ESTIMATION,
	MER_KYRIE,
	MER_BLESSING,
	MER_INCAGI,

	EL_CIRCLE_OF_FIRE = 8401,
	EL_FIRE_CLOAK,
	EL_FIRE_MANTLE,
	EL_WATER_SCREEN,
	EL_WATER_DROP,
	EL_WATER_BARRIER,
	EL_WIND_STEP,
	EL_WIND_CURTAIN,
	EL_ZEPHYR,
	EL_SOLID_SKIN,
	EL_STONE_SHIELD,
	EL_POWER_OF_GAIA,
	EL_PYROTECHNIC,
	EL_HEATER,
	EL_TROPIC,
	EL_AQUAPLAY,
	EL_COOLER,
	EL_CHILLY_AIR,
	EL_GUST,
	EL_BLAST,
	EL_WILD_STORM,
	EL_PETROLOGY,
	EL_CURSED_SOIL,
	EL_UPHEAVAL,
	EL_FIRE_ARROW,
	EL_FIRE_BOMB,
	EL_FIRE_BOMB_ATK,
	EL_FIRE_WAVE,
	EL_FIRE_WAVE_ATK,
	EL_ICE_NEEDLE,
	EL_WATER_SCREW,
	EL_WATER_SCREW_ATK,
	EL_TIDAL_WEAPON,
	EL_WIND_SLASH,
	EL_HURRICANE,
	EL_HURRICANE_ATK,
	EL_TYPOON_MIS,
	EL_TYPOON_MIS_ATK,
	EL_STONE_HAMMER,
	EL_ROCK_CRUSHER,
	EL_ROCK_CRUSHER_ATK,
	EL_STONE_RAIN,
};

/// The client view ids for land skills.
enum {
	UNT_SAFETYWALL = 0x7e,
	UNT_FIREWALL,
	UNT_WARP_WAITING,
	UNT_WARP_ACTIVE,
	UNT_BENEDICTIO, //TODO
	UNT_SANCTUARY,
	UNT_MAGNUS,
	UNT_PNEUMA,
	UNT_DUMMYSKILL, //These show no effect on the client
	UNT_FIREPILLAR_WAITING,
	UNT_FIREPILLAR_ACTIVE,
	UNT_HIDDEN_TRAP, //TODO
	UNT_TRAP, //TODO
	UNT_HIDDEN_WARP_NPC, //TODO
	UNT_USED_TRAPS,
	UNT_ICEWALL,
	UNT_QUAGMIRE,
	UNT_BLASTMINE,
	UNT_SKIDTRAP,
	UNT_ANKLESNARE,
	UNT_VENOMDUST,
	UNT_LANDMINE,
	UNT_SHOCKWAVE,
	UNT_SANDMAN,
	UNT_FLASHER,
	UNT_FREEZINGTRAP,
	UNT_CLAYMORETRAP,
	UNT_TALKIEBOX,
	UNT_VOLCANO,
	UNT_DELUGE,
	UNT_VIOLENTGALE,
	UNT_LANDPROTECTOR,
	UNT_LULLABY,
	UNT_RICHMANKIM,
	UNT_ETERNALCHAOS,
	UNT_DRUMBATTLEFIELD,
	UNT_RINGNIBELUNGEN,
	UNT_ROKISWEIL,
	UNT_INTOABYSS,
	UNT_SIEGFRIED,
	UNT_DISSONANCE,
	UNT_WHISTLE,
	UNT_ASSASSINCROSS,
	UNT_POEMBRAGI,
	UNT_APPLEIDUN,
	UNT_UGLYDANCE,
	UNT_HUMMING,
	UNT_DONTFORGETME,
	UNT_FORTUNEKISS,
	UNT_SERVICEFORYOU,
	UNT_GRAFFITI,
	UNT_DEMONSTRATION,
	UNT_CALLFAMILY,
	UNT_GOSPEL,
	UNT_BASILICA,
	UNT_MOONLIT,
	UNT_FOGWALL,
	UNT_SPIDERWEB,
	UNT_GRAVITATION,
	UNT_HERMODE,
	UNT_KAENSIN, //TODO
	UNT_SUITON,
	UNT_TATAMIGAESHI,
	UNT_KAEN,
	UNT_GROUNDDRIFT_WIND,
	UNT_GROUNDDRIFT_DARK,
	UNT_GROUNDDRIFT_POISON,
	UNT_GROUNDDRIFT_WATER,
	UNT_GROUNDDRIFT_FIRE,
	UNT_DEATHWAVE, //TODO
	UNT_WATERATTACK, //TODO
	UNT_WINDATTACK, //TODO
	UNT_EARTHQUAKE, //TODO
	UNT_EVILLAND,
	UNT_DARK_RUNNER, //TODO
	UNT_DARK_TRANSFER, //TODO
	UNT_EPICLESIS, //TODO
	UNT_EARTHSTRAIN, //TODO
	UNT_MANHOLE, //TODO
	UNT_DIMENSIONDOOR, //TODO
	UNT_CHAOSPANIC, //TODO
	UNT_MAELSTROM, //TODO
	UNT_BLOODYLUST, //TODO
	UNT_FEINTBOMB, //TODO
	UNT_MAGENTATRAP, //TODO
	UNT_COBALTTRAP, //TODO
	UNT_MAIZETRAP, //TODO
	UNT_VERDURETRAP, //TODO
	UNT_FIRINGTRAP, //TODO
	UNT_ICEBOUNDTRAP, //TODO
	UNT_ELECTRICSHOCKER, //TODO
	UNT_CLUSTERBOMB, //TODO
	UNT_REVERBERATION, //TODO
	UNT_SEVERE_RAINSTORM, //TODO
	UNT_FIREWALK, //TODO
	UNT_ELECTRICWALK, //TODO
	UNT_POEMOFNETHERWORLD, //TODO
	UNT_PSYCHIC_WAVE, //TODO
	UNT_CLOUD_KILL, //TODO
	UNT_POISONSMOKE, //TODO
	UNT_NEUTRALBARRIER, //TODO
	UNT_STEALTHFIELD, //TODO
	UNT_WARMER, //TODO
	UNT_THORNS_TRAP, //TODO
	UNT_WALLOFTHORN, //TODO
	UNT_DEMONIC_FIRE, //TODO
	UNT_FIRE_EXPANSION_SMOKE_POWDER, //TODO
	UNT_FIRE_EXPANSION_TEAR_GAS, //TODO
	UNT_HELLS_PLANT, //TODO
	UNT_VACUUM_EXTREME, //TODO
	UNT_BANDING, //TODO
	UNT_FIRE_MANTLE, //TODO
	UNT_WATER_BARRIER, //TODO
	UNT_ZEPHYR, //TODO
	UNT_POWER_OF_GAIA, //TODO
	UNT_FIRE_INSIGNIA, //TODO
	UNT_WATER_INSIGNIA, //TODO
	UNT_WIND_INSIGNIA, //TODO
	UNT_EARTH_INSIGNIA, //TODO

	UNT_MAX = 0x190
};

#endif /* _SKILL_H_ */
