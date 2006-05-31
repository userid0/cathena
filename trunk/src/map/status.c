// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <limits.h>

#include "pc.h"
#include "map.h"
#include "pet.h"
#include "npc.h"
#include "mob.h"
#include "clif.h"
#include "guild.h"
#include "skill.h"
#include "itemdb.h"
#include "battle.h"
#include "chrif.h"
#include "status.h"
#include "script.h"
#include "unit.h"
#include "mercenary.h"

#include "../common/timer.h"
#include "../common/nullpo.h"
#include "../common/showmsg.h"
#include "../common/malloc.h"

int SkillStatusChangeTable[MAX_SKILL]; //Stores the status that should be associated to this skill.
int StatusIconChangeTable[SC_MAX]; //Stores the icon that should be associated to this status change.
int StatusSkillChangeTable[SC_MAX]; //Stores the skill that should be considered associated to this status change. 
unsigned long StatusChangeFlagTable[SC_MAX]; //Stores the flag specifying what this SC changes.

static int max_weight_base[MAX_PC_CLASS];
static int hp_coefficient[MAX_PC_CLASS];
static int hp_coefficient2[MAX_PC_CLASS];
static int hp_sigma_val[MAX_PC_CLASS][MAX_LEVEL];
static int sp_coefficient[MAX_PC_CLASS];
static int aspd_base[MAX_PC_CLASS][MAX_WEAPON_TYPE];	//[blackhole89]
static int refinebonus[MAX_REFINE_BONUS][3];	// 精錬ボーナステーブル(refine_db.txt)
int percentrefinery[5][MAX_REFINE+1];	// 精錬成功率(refine_db.txt)
static int atkmods[3][MAX_WEAPON_TYPE];	// 武器ATKサイズ修正(size_fix.txt)
static char job_bonus[MAX_PC_CLASS][MAX_LEVEL];

static struct status_data dummy_status;
int current_equip_item_index; //Contains inventory index of an equipped item. To pass it into the EQUP_SCRIPT [Lupus]
int current_equip_card_id; //To prevent card-stacking (from jA) [Skotlex]
//we need it for new cards 15 Feb 2005, to check if the combo cards are insrerted into the CURRENT weapon only
//to avoid cards exploits

//Caps values to min/max
#define cap_value(a, min, max) (a>max?max:a<min?min:a)

//Initializes the StatusIconChangeTable variable. May seem somewhat slower than directly defining the array,
//but it is much less prone to errors. [Skotlex]
void initChangeTables(void) {
	int i;
	for (i = 0; i < SC_MAX; i++)
		StatusIconChangeTable[i] = SI_BLANK;
	for (i = 0; i < MAX_SKILL; i++)
		SkillStatusChangeTable[i] = -1;
	memset(StatusSkillChangeTable, 0, sizeof(StatusSkillChangeTable));
	memset(StatusChangeFlagTable, 0, sizeof(StatusChangeFlagTable));

	//First we define the skill for common ailments. These are used in 
	//skill_additional_effect through sc cards. [Skotlex]
	StatusSkillChangeTable[SC_STONE] =     MG_STONECURSE;
	StatusSkillChangeTable[SC_FREEZE] =    MG_FROSTDIVER;
	StatusSkillChangeTable[SC_STUN] =      NPC_STUNATTACK;
	StatusSkillChangeTable[SC_SLEEP] =     NPC_SLEEPATTACK;
	StatusSkillChangeTable[SC_POISON] =    NPC_POISON;
	StatusSkillChangeTable[SC_CURSE] =     NPC_CURSEATTACK;
	StatusSkillChangeTable[SC_SILENCE] =   NPC_SILENCEATTACK;
	StatusSkillChangeTable[SC_CONFUSION] = DC_WINKCHARM;
	StatusSkillChangeTable[SC_BLIND] =     NPC_BLINDATTACK;
	StatusSkillChangeTable[SC_BLEEDING] =  LK_HEADCRUSH;
	StatusSkillChangeTable[SC_DPOISON] =   NPC_POISON;

	//These are the status-change flags for the common ailments.
	StatusChangeFlagTable[SC_STONE] =     SCB_DEF_ELE;
	StatusChangeFlagTable[SC_FREEZE] =    SCB_DEF_ELE;
//	StatusChangeFlagTable[SC_STUN] =      SCB_NONE;
//	StatusChangeFlagTable[SC_SLEEP] =     SCB_NONE;
	StatusChangeFlagTable[SC_POISON] =    SCB_DEF2;
	StatusChangeFlagTable[SC_CURSE] =     SCB_LUK|SCB_BATK|SCB_WATK|SCB_SPEED;
//	StatusChangeFlagTable[SC_SILENCE] =   SCB_NONE;
//	StatusChangeFlagTable[SC_CONFUSION] = SCB_NONE;
	StatusChangeFlagTable[SC_BLIND] =     SCB_HIT|SCB_FLEE;
//	StatusChangeFlagTable[SC_BLEEDING] =  SCB_NONE;
//	StatusChangeFlagTable[SC_DPOISON] =   SCB_NONE;

	//The icons for the common ailments
//	StatusIconChangeTable[SC_STONE] =     SI_BLANK;
//	StatusIconChangeTable[SC_FREEZE] =    SI_BLANK;
//	StatusIconChangeTable[SC_STUN] =      SI_BLANK;
//	StatusIconChangeTable[SC_SLEEP] =     SI_BLANK;
//	StatusIconChangeTable[SC_POISON] =    SI_BLANK;
//	StatusIconChangeTable[SC_CURSE] =     SI_BLANK;
//	StatusIconChangeTable[SC_SILENCE] =   SI_BLANK;
//	StatusIconChangeTable[SC_CONFUSION] = SI_BLANK;
//	StatusIconChangeTable[SC_BLIND] =     SI_BLANK;
	StatusIconChangeTable[SC_BLEEDING] =  SI_BLEEDING;
//	StatusIconChangeTable[SC_DPOISON] =   SI_BLANK;

	
#define set_sc(skill, sc, icon, flag) \
	if (SkillStatusChangeTable[skill]==-1) SkillStatusChangeTable[skill] = sc; \
	if (StatusSkillChangeTable[sc]==0) StatusSkillChangeTable[sc] = skill; \
	if (StatusIconChangeTable[sc]==SI_BLANK) StatusIconChangeTable[sc] = icon; \
	StatusChangeFlagTable[sc] |= flag;

//This one is for sc's that already were defined.
#define add_sc(skill, sc) \
	if (SkillStatusChangeTable[skill]==-1) SkillStatusChangeTable[skill] = sc; \
	if (StatusSkillChangeTable[sc]==0) StatusSkillChangeTable[sc] = skill;
		

	add_sc(SM_BASH, SC_STUN);
	set_sc(SM_PROVOKE, SC_PROVOKE, SI_PROVOKE, SCB_DEF|SCB_DEF2|SCB_BATK|SCB_WATK);
	add_sc(SM_MAGNUM, SC_WATK_ELEMENT);
	set_sc(SM_ENDURE, SC_ENDURE, SI_ENDURE, SCB_MDEF|SCB_DSPD);
	add_sc(MG_SIGHT, SC_SIGHT);
	add_sc(MG_SAFETYWALL, SC_SAFETYWALL);
	add_sc(MG_FROSTDIVER, SC_FREEZE);
	add_sc(MG_STONECURSE, SC_STONE);
	add_sc(AL_RUWACH, SC_RUWACH);
	set_sc(AL_INCAGI, SC_INCREASEAGI, SI_INCREASEAGI, SCB_AGI|SCB_SPEED);
	set_sc(AL_DECAGI, SC_DECREASEAGI, SI_DECREASEAGI, SCB_AGI|SCB_SPEED);
	set_sc(AL_CRUCIS, SC_SIGNUMCRUCIS, SI_SIGNUMCRUCIS, SCB_DEF);
	set_sc(AL_ANGELUS, SC_ANGELUS, SI_ANGELUS, SCB_DEF2);
	set_sc(AL_BLESSING, SC_BLESSING, SI_BLESSING, SCB_STR|SCB_INT|SCB_DEX);
	set_sc(AC_CONCENTRATION, SC_CONCENTRATE, SI_CONCENTRATE, SCB_AGI|SCB_DEX);
	set_sc(TF_HIDING, SC_HIDING, SI_HIDING, SCB_SPEED);
	add_sc(TF_POISON, SC_POISON);
	set_sc(KN_TWOHANDQUICKEN, SC_TWOHANDQUICKEN, SI_TWOHANDQUICKEN, SCB_ASPD);
	add_sc(KN_AUTOCOUNTER, SC_AUTOCOUNTER);
	set_sc(PR_IMPOSITIO, SC_IMPOSITIO, SI_IMPOSITIO, SCB_WATK);
	set_sc(PR_SUFFRAGIUM, SC_SUFFRAGIUM, SI_SUFFRAGIUM, SCB_NONE);
	set_sc(PR_ASPERSIO, SC_ASPERSIO, SI_ASPERSIO, SCB_ATK_ELE);
	set_sc(PR_BENEDICTIO, SC_BENEDICTIO, SI_BENEDICTIO, SCB_DEF_ELE);
	set_sc(PR_SLOWPOISON, SC_SLOWPOISON, SI_SLOWPOISON, SCB_NONE);
	set_sc(PR_KYRIE, SC_KYRIE,	SI_KYRIE, SCB_NONE);
	set_sc(PR_MAGNIFICAT, SC_MAGNIFICAT, SI_MAGNIFICAT, SCB_NONE);
	set_sc(PR_GLORIA, SC_GLORIA, SI_GLORIA, SCB_LUK);
	add_sc(PR_LEXDIVINA, SC_SILENCE);
	set_sc(PR_LEXAETERNA, SC_AETERNA, SI_AETERNA, SCB_NONE);
	add_sc(WZ_METEOR, SC_STUN);
	add_sc(WZ_VERMILION, SC_BLIND);
	add_sc(WZ_FROSTNOVA, SC_FREEZE);
	add_sc(WZ_STORMGUST, SC_FREEZE);
	set_sc(WZ_QUAGMIRE, SC_QUAGMIRE, SI_QUAGMIRE, SCB_AGI|SCB_DEX|SCB_ASPD);
	set_sc(BS_ADRENALINE, SC_ADRENALINE, SI_ADRENALINE, SCB_ASPD);
	set_sc(BS_WEAPONPERFECT, SC_WEAPONPERFECTION, SI_WEAPONPERFECTION, SCB_NONE);
	set_sc(BS_OVERTHRUST, SC_OVERTHRUST, SI_OVERTHRUST, SCB_NONE);
	set_sc(BS_MAXIMIZE, SC_MAXIMIZEPOWER, SI_MAXIMIZEPOWER, SCB_NONE);
	add_sc(HT_LANDMINE, SC_STUN);
	add_sc(HT_ANKLESNARE, SC_ANKLE);
	add_sc(HT_SANDMAN, SC_SLEEP);
	add_sc(HT_FLASHER, SC_BLIND);
	add_sc(HT_FREEZINGTRAP, SC_FREEZE);
	set_sc(AS_CLOAKING, SC_CLOAKING,	SI_CLOAKING, SCB_CRI|SCB_SPEED);
	add_sc(AS_SONICBLOW, SC_STUN);
	set_sc(AS_GRIMTOOTH, SC_SLOWDOWN, SI_BLANK, SCB_SPEED);
	set_sc(AS_ENCHANTPOISON, SC_ENCPOISON,	SI_ENCPOISON, SCB_ATK_ELE);
	set_sc(AS_POISONREACT, SC_POISONREACT, SI_POISONREACT, SCB_NONE);
	add_sc(AS_VENOMDUST, SC_POISON);
	add_sc(AS_SPLASHER, SC_SPLASHER);
	set_sc(NV_TRICKDEAD, SC_TRICKDEAD, SI_TRICKDEAD, SCB_NONE);
	set_sc(SM_AUTOBERSERK, SC_AUTOBERSERK, SI_STEELBODY, SCB_NONE);
	add_sc(TF_SPRINKLESAND, SC_BLIND);
	add_sc(TF_THROWSTONE, SC_STUN);
	set_sc(MC_LOUD, SC_LOUD, SI_LOUD, SCB_STR);
	set_sc(MG_ENERGYCOAT, SC_ENERGYCOAT, SI_ENERGYCOAT, SCB_NONE);
	set_sc(NPC_EMOTION, SC_MODECHANGE, SI_BLANK, SCB_MODE);
	add_sc(NPC_EMOTION_ON, SC_MODECHANGE);
	set_sc(NPC_ATTRICHANGE, SC_ELEMENTALCHANGE, SI_BLANK, SCB_DEF_ELE);
	add_sc(NPC_CHANGEWATER, SC_ELEMENTALCHANGE);
	add_sc(NPC_CHANGEGROUND, SC_ELEMENTALCHANGE);
	add_sc(NPC_CHANGEFIRE, SC_ELEMENTALCHANGE);
	add_sc(NPC_CHANGEWIND, SC_ELEMENTALCHANGE);
	add_sc(NPC_CHANGEPOISON, SC_ELEMENTALCHANGE);
	add_sc(NPC_CHANGEHOLY, SC_ELEMENTALCHANGE);
	add_sc(NPC_CHANGEDARKNESS, SC_ELEMENTALCHANGE);
	add_sc(NPC_CHANGETELEKINESIS, SC_ELEMENTALCHANGE);
	add_sc(NPC_POISON, SC_POISON);
	add_sc(NPC_BLINDATTACK, SC_BLIND);
	add_sc(NPC_SILENCEATTACK, SC_SILENCE);
	add_sc(NPC_STUNATTACK, SC_STUN);
	add_sc(NPC_PETRIFYATTACK, SC_STONE);
	add_sc(NPC_CURSEATTACK, SC_CURSE);
	add_sc(NPC_SLEEPATTACK, SC_SLEEP);
	set_sc(NPC_KEEPING, SC_KEEPING, SI_BLANK, SCB_DEF);
	add_sc(NPC_DARKBLESSING, SC_COMA);
	set_sc(NPC_BARRIER, SC_BARRIER, SI_BLANK, SCB_MDEF);
	add_sc(NPC_LICK, SC_STUN);
	set_sc(NPC_HALLUCINATION, SC_HALLUCINATION, SI_HALLUCINATION, SCB_NONE);
	add_sc(NPC_REBIRTH, SC_KAIZEL);
	add_sc(RG_RAID, SC_STUN);
	set_sc(RG_STRIPWEAPON, SC_STRIPWEAPON, SI_STRIPWEAPON, SCB_WATK);
	set_sc(RG_STRIPSHIELD, SC_STRIPSHIELD, SI_STRIPSHIELD, SCB_DEF);
	set_sc(RG_STRIPARMOR, SC_STRIPARMOR, SI_STRIPARMOR, SCB_VIT);
	set_sc(RG_STRIPHELM, SC_STRIPHELM, SI_STRIPHELM, SCB_INT);
	add_sc(AM_ACIDTERROR, SC_BLEEDING);
	set_sc(AM_CP_WEAPON, SC_CP_WEAPON, SI_CP_WEAPON, SCB_NONE);
	set_sc(AM_CP_SHIELD, SC_CP_SHIELD, SI_CP_SHIELD, SCB_NONE);
	set_sc(AM_CP_ARMOR, SC_CP_ARMOR, SI_CP_ARMOR, SCB_NONE);
	set_sc(AM_CP_HELM, SC_CP_HELM, SI_CP_HELM, SCB_NONE);
	set_sc(CR_AUTOGUARD, SC_AUTOGUARD, SI_AUTOGUARD, SCB_NONE);
	add_sc(CR_SHIELDCHARGE, SC_STUN);
	set_sc(CR_REFLECTSHIELD, SC_REFLECTSHIELD, SI_REFLECTSHIELD, SCB_NONE);
	add_sc(CR_HOLYCROSS, SC_BLIND);
	add_sc(CR_GRANDCROSS, SC_BLIND);
	set_sc(CR_DEVOTION, SC_DEVOTION, SI_DEVOTION, SCB_NONE);
	set_sc(CR_PROVIDENCE, SC_PROVIDENCE, SI_PROVIDENCE, SCB_PC);
	set_sc(CR_DEFENDER, SC_DEFENDER, SI_DEFENDER, SCB_SPEED|SCB_ASPD);
	set_sc(CR_SPEARQUICKEN, SC_SPEARQUICKEN, SI_SPEARQUICKEN, SCB_ASPD);
	set_sc(MO_STEELBODY, SC_STEELBODY, SI_STEELBODY, SCB_DEF|SCB_MDEF|SCB_ASPD|SCB_SPEED);
	add_sc(MO_BLADESTOP, SC_BLADESTOP_WAIT);
	add_sc(MO_BLADESTOP, SC_BLADESTOP);
	set_sc(MO_EXPLOSIONSPIRITS, SC_EXPLOSIONSPIRITS, SI_EXPLOSIONSPIRITS, SCB_CRI);
	add_sc(MO_EXTREMITYFIST, SC_EXTREMITYFIST);
	add_sc(SA_MAGICROD, SC_MAGICROD);
	set_sc(SA_AUTOSPELL, SC_AUTOSPELL, SI_AUTOSPELL, SCB_NONE);
	set_sc(SA_FLAMELAUNCHER, SC_FIREWEAPON, SI_FIREWEAPON, SCB_ATK_ELE);
	set_sc(SA_FROSTWEAPON, SC_WATERWEAPON, SI_WATERWEAPON, SCB_ATK_ELE);
	set_sc(SA_LIGHTNINGLOADER, SC_WINDWEAPON, SI_WINDWEAPON, SCB_ATK_ELE);
	set_sc(SA_SEISMICWEAPON, SC_EARTHWEAPON, SI_EARTHWEAPON, SCB_ATK_ELE);
	set_sc(SA_VOLCANO, SC_VOLCANO, SI_BLANK, SCB_WATK);
	set_sc(SA_DELUGE, SC_DELUGE, SI_BLANK, SCB_MAXHP);
	set_sc(SA_VIOLENTGALE, SC_VIOLENTGALE, SI_BLANK, SCB_FLEE);
	add_sc(SA_LANDPROTECTOR, SC_LANDPROTECTOR);
	add_sc(SA_REVERSEORCISH, SC_ORCISH);
	add_sc(SA_COMA, SC_COMA);
	add_sc(BD_RICHMANKIM, SC_RICHMANKIM);
	set_sc(BD_ETERNALCHAOS, SC_ETERNALCHAOS, SI_BLANK, SCB_DEF2);
	set_sc(BD_DRUMBATTLEFIELD, SC_DRUMBATTLE, SI_BLANK, SCB_WATK|SCB_DEF);
	set_sc(BD_RINGNIBELUNGEN, SC_NIBELUNGEN, SI_BLANK, SCB_WATK);
	add_sc(BD_ROKISWEIL, SC_ROKISWEIL);
	add_sc(BD_INTOABYSS, SC_INTOABYSS);
	set_sc(BD_SIEGFRIED, SC_SIEGFRIED, SI_BLANK, SCB_PC);
	add_sc(BA_FROSTJOKE, SC_FREEZE);
	set_sc(BA_WHISTLE, SC_WHISTLE, SI_BLANK, SCB_FLEE|SCB_FLEE2);
	set_sc(BA_ASSASSINCROSS, SC_ASSNCROS, SI_BLANK, SCB_ASPD);
	add_sc(BA_POEMBRAGI, SC_POEMBRAGI);
	set_sc(BA_APPLEIDUN, SC_APPLEIDUN, SI_BLANK, SCB_MAXHP);
	add_sc(DC_SCREAM, SC_STUN);
	set_sc(DC_HUMMING, SC_HUMMING, SI_BLANK, SCB_HIT);
	set_sc(DC_DONTFORGETME, SC_DONTFORGETME, SI_BLANK, SCB_SPEED|SCB_ASPD);
	set_sc(DC_FORTUNEKISS, SC_FORTUNE, SI_BLANK, SCB_CRI);
	set_sc(DC_SERVICEFORYOU, SC_SERVICE4U, SI_BLANK, SCB_MAXSP|SCB_PC);
	add_sc(NPC_DARKCROSS, SC_BLIND);
	add_sc(NPC_GRANDDARKNESS, SC_BLIND);
	add_sc(NPC_STOP, SC_STOP);
	set_sc(NPC_BREAKWEAPON, SC_BROKENWEAPON, SI_BROKENWEAPON, SCB_NONE);
	set_sc(NPC_BREAKARMOR, SC_BROKENARMOR, SI_BROKENARMOR, SCB_NONE);
	set_sc(NPC_POWERUP, SC_INCHITRATE, SI_BLANK, SCB_HIT);
	set_sc(NPC_AGIUP, SC_INCFLEERATE, SI_BLANK, SCB_FLEE);
	add_sc(NPC_INVISIBLE, SC_CLOAKING);
	set_sc(LK_AURABLADE, SC_AURABLADE, SI_AURABLADE, SCB_NONE);
	set_sc(LK_PARRYING, SC_PARRYING, SI_PARRYING, SCB_NONE);
	set_sc(LK_CONCENTRATION, SC_CONCENTRATION, SI_CONCENTRATION, SCB_BATK|SCB_WATK|SCB_HIT|SCB_DEF|SCB_DEF2|SCB_DSPD);
	set_sc(LK_TENSIONRELAX, SC_TENSIONRELAX, SI_TENSIONRELAX, SCB_NONE);
	set_sc(LK_BERSERK, SC_BERSERK, SI_BERSERK, SCB_DEF|SCB_DEF2|SCB_MDEF|SCB_MDEF2|SCB_FLEE|SCB_SPEED|SCB_ASPD|SCB_MAXHP);
//	set_sc(LK_FURY, SC_FURY, SI_FURY, SCB_NONE); //Unused skill
	set_sc(HP_ASSUMPTIO, SC_ASSUMPTIO, SI_ASSUMPTIO, SCB_NONE);
	add_sc(HP_BASILICA, SC_BASILICA);
	set_sc(HW_MAGICPOWER, SC_MAGICPOWER, SI_MAGICPOWER, SCB_MATK);
	add_sc(PA_SACRIFICE, SC_SACRIFICE);
	set_sc(PA_GOSPEL, SC_GOSPEL, SI_BLANK, SCB_SPEED|SCB_ASPD);
	add_sc(PA_GOSPEL, SC_SCRESIST);
	add_sc(CH_TIGERFIST, SC_STOP);
	set_sc(ASC_EDP, SC_EDP, SI_EDP, SCB_NONE);
	set_sc(SN_SIGHT, SC_TRUESIGHT, SI_TRUESIGHT, SCB_STR|SCB_AGI|SCB_VIT|SCB_INT|SCB_DEX|SCB_LUK|SCB_CRI|SCB_HIT);
	set_sc(SN_WINDWALK, SC_WINDWALK, SI_WINDWALK, SCB_FLEE|SCB_SPEED);
	set_sc(WS_MELTDOWN, SC_MELTDOWN, SI_MELTDOWN, SCB_NONE);
	set_sc(WS_CARTBOOST, SC_CARTBOOST, SI_CARTBOOST, SCB_SPEED);
	set_sc(ST_CHASEWALK, SC_CHASEWALK, SI_CHASEWALK, SCB_SPEED);
	set_sc(ST_REJECTSWORD, SC_REJECTSWORD, SI_REJECTSWORD, SCB_NONE);
	add_sc(ST_REJECTSWORD, SC_AUTOCOUNTER);
	set_sc(CG_MOONLIT, SC_MOONLIT, SI_MOONLIT, SCB_NONE);
	set_sc(CG_MARIONETTE, SC_MARIONETTE, SI_MARIONETTE, SCB_STR|SCB_AGI|SCB_VIT|SCB_INT|SCB_DEX|SCB_LUK);
	set_sc(CG_MARIONETTE, SC_MARIONETTE2, SI_MARIONETTE2, SCB_STR|SCB_AGI|SCB_VIT|SCB_INT|SCB_DEX|SCB_LUK);
	add_sc(LK_SPIRALPIERCE, SC_STOP);
	add_sc(LK_HEADCRUSH, SC_BLEEDING);
	set_sc(LK_JOINTBEAT, SC_JOINTBEAT, SI_JOINTBEAT, SCB_BATK|SCB_DEF2|SCB_SPEED|SCB_ASPD);
	add_sc(HW_NAPALMVULCAN, SC_CURSE);
	set_sc(PF_MINDBREAKER, SC_MINDBREAKER, SI_BLANK, SCB_MATK|SCB_MDEF2);
	add_sc(PF_MEMORIZE, SC_MEMORIZE);
	add_sc(PF_FOGWALL, SC_FOGWALL);
	set_sc(PF_SPIDERWEB, SC_SPIDERWEB, SI_BLANK, SCB_FLEE);
	add_sc(WE_BABY, SC_BABY);
	set_sc(TK_RUN, SC_RUN, SI_RUN, SCB_SPEED);
	set_sc(TK_RUN, SC_SPURT, SI_SPURT, SCB_STR);
	set_sc(TK_READYSTORM, SC_READYSTORM, SI_READYSTORM, SCB_NONE);
	set_sc(TK_READYDOWN, SC_READYDOWN, SI_READYDOWN, SCB_NONE);
	add_sc(TK_DOWNKICK, SC_STUN);
	set_sc(TK_READYTURN, SC_READYTURN, SI_READYTURN, SCB_NONE);
	set_sc(TK_READYCOUNTER,SC_READYCOUNTER, SI_READYCOUNTER, SCB_NONE);
	set_sc(TK_DODGE, SC_DODGE, SI_DODGE, SCB_NONE);
	set_sc(TK_SPTIME, SC_TKREST, SI_TKREST, SCB_NONE);
	set_sc(TK_SEVENWIND, SC_GHOSTWEAPON, SI_GHOSTWEAPON, SCB_ATK_ELE);
	set_sc(TK_SEVENWIND, SC_SHADOWWEAPON, SI_SHADOWWEAPON, SCB_ATK_ELE);
	set_sc(SG_SUN_WARM, SC_WARM, SI_WARM, SCB_NONE);
	add_sc(SG_MOON_WARM, SC_WARM);
	add_sc(SG_STAR_WARM, SC_WARM);
	set_sc(SG_SUN_COMFORT, SC_SUN_COMFORT, SI_SUN_COMFORT, SCB_DEF2);
	set_sc(SG_MOON_COMFORT, SC_MOON_COMFORT, SI_MOON_COMFORT, SCB_FLEE);
	set_sc(SG_STAR_COMFORT, SC_STAR_COMFORT, SI_STAR_COMFORT, SCB_ASPD);
	add_sc(SG_FRIEND, SC_SKILLRATE_UP);
	set_sc(SG_KNOWLEDGE, SC_KNOWLEDGE, SI_BLANK, SCB_PC);
	set_sc(SG_FUSION, SC_FUSION, SI_BLANK, SCB_SPEED);
	set_sc(BS_ADRENALINE2, SC_ADRENALINE2, SI_ADRENALINE2, SCB_ASPD);
	set_sc(SL_KAIZEL, SC_KAIZEL, SI_KAIZEL, SCB_NONE);
	set_sc(SL_KAAHI, SC_KAAHI, SI_KAAHI, SCB_NONE);
	set_sc(SL_KAUPE, SC_KAUPE, SI_KAUPE, SCB_NONE);
	set_sc(SL_KAITE, SC_KAITE, SI_KAITE, SCB_NONE);
	add_sc(SL_STUN, SC_STUN);
	set_sc(SL_SWOO, SC_SWOO, SI_BLANK, SCB_SPEED);
	set_sc(SL_SKE, SC_SKE, SI_BLANK, SCB_BATK|SCB_WATK|SCB_DEF|SCB_DEF2);
	set_sc(SL_SKA, SC_SKA, SI_BLANK, SCB_DEF|SCB_MDEF|SCB_ASPD);
	set_sc(SL_SMA, SC_SMA, SI_SMA, SCB_NONE);
	set_sc(ST_PRESERVE, SC_PRESERVE, SI_PRESERVE, SCB_NONE);
	set_sc(PF_DOUBLECASTING, SC_DOUBLECAST, SI_DOUBLECAST, SCB_NONE);
	set_sc(HW_GRAVITATION, SC_GRAVITATION, SI_BLANK, SCB_ASPD);
	add_sc(WS_CARTTERMINATION, SC_STUN);
	set_sc(WS_OVERTHRUSTMAX, SC_MAXOVERTHRUST, SI_MAXOVERTHRUST, SCB_NONE);
	set_sc(CG_LONGINGFREEDOM, SC_LONGING, SI_BLANK, SCB_SPEED|SCB_ASPD);
	add_sc(CG_HERMODE, SC_HERMODE);
	set_sc(SL_HIGH, SC_SPIRIT, SI_SPIRIT, SCB_PC);
	set_sc(KN_ONEHAND, SC_ONEHAND, SI_ONEHAND, SCB_ASPD);
	set_sc(CR_SHRINK, SC_SHRINK, SI_SHRINK, SCB_NONE);
	set_sc(RG_CLOSECONFINE, SC_CLOSECONFINE2, SI_CLOSECONFINE2, SCB_NONE);
	set_sc(RG_CLOSECONFINE, SC_CLOSECONFINE, SI_CLOSECONFINE, SCB_FLEE);
	set_sc(WZ_SIGHTBLASTER, SC_SIGHTBLASTER, SI_SIGHTBLASTER, SCB_NONE);
	set_sc(DC_WINKCHARM, SC_WINKCHARM, SI_WINKCHARM, SCB_NONE);
	add_sc(MO_BALKYOUNG, SC_STUN);
	add_sc(SA_ELEMENTWATER, SC_ELEMENTALCHANGE);
	add_sc(SA_ELEMENTFIRE, SC_ELEMENTALCHANGE);
	add_sc(SA_ELEMENTGROUND, SC_ELEMENTALCHANGE);
	add_sc(SA_ELEMENTWIND, SC_ELEMENTALCHANGE);

	//Until they're at right position - gs_set_sc- [Vicious] / some of these don't seem to have a status icon adequate [blackhole89]
	set_sc(GS_MADNESSCANCEL, SC_MADNESSCANCEL, SI_MADNESSCANCEL, SCB_BATK|SCB_ASPD);
	set_sc(GS_ADJUSTMENT, SC_ADJUSTMENT, SI_ADJUSTMENT, SCB_HIT|SCB_FLEE);
	set_sc(GS_INCREASING, SC_INCREASING, SI_ACCURACY, SCB_AGI|SCB_DEX|SCB_HIT);
	set_sc(GS_GATLINGFEVER, SC_GATLINGFEVER, SI_GATLINGFEVER, SCB_FLEE|SCB_SPEED|SCB_ASPD);
	set_sc(GS_FLING, SC_FLING, SI_BLANK, SCB_DEF|SCB_DEF2);

	//Uncomment and update when you plan on implementing.
//	set_sc(NJ_TATAMIGAESHI, SC_TATAMIGAESHI, SI_BLANK);
//	set_sc(NJ_UTSUSEMI,             SC_UTSUSEMI,            SI_MAEMI);
//	set_sc(NJ_KAENSIN,              SC_KAENSIN,             SI_BLANK);
	set_sc(NJ_SUITON, SC_SUITON, SI_BLANK, SCB_AGI);
	set_sc(NJ_NEN, SC_NEN, SI_NEN, SCB_STR|SCB_INT);
 	set_sc(HLIF_AVOID, SC_AVOID, SI_BLANK, SCB_SPEED);
	set_sc(HLIF_CHANGE, SC_CHANGE, SI_BLANK, SCB_INT);
	set_sc(HAMI_BLOODLUST, SC_BLOODLUST, SI_BLANK, SCB_BATK|SCB_WATK);
	set_sc(HFLI_FLEET, SC_FLEET, SI_BLANK, SCB_ASPD|SCB_BATK|SCB_WATK);

	// Storing the target job rather than simply SC_SPIRIT simplifies code later on.
	SkillStatusChangeTable[SL_ALCHEMIST] =   MAPID_ALCHEMIST,
	SkillStatusChangeTable[SL_MONK] =        MAPID_MONK,
	SkillStatusChangeTable[SL_STAR] =        MAPID_STAR_GLADIATOR,
	SkillStatusChangeTable[SL_SAGE] =        MAPID_SAGE,
	SkillStatusChangeTable[SL_CRUSADER] =    MAPID_CRUSADER,
	SkillStatusChangeTable[SL_SUPERNOVICE] = MAPID_SUPER_NOVICE,
	SkillStatusChangeTable[SL_KNIGHT] =      MAPID_KNIGHT,
	SkillStatusChangeTable[SL_WIZARD] =      MAPID_WIZARD,
	SkillStatusChangeTable[SL_PRIEST] =      MAPID_PRIEST,
	SkillStatusChangeTable[SL_BARDDANCER] =  MAPID_BARDDANCER,
	SkillStatusChangeTable[SL_ROGUE] =       MAPID_ROGUE,
	SkillStatusChangeTable[SL_ASSASIN] =     MAPID_ASSASSIN,
	SkillStatusChangeTable[SL_BLACKSMITH] =  MAPID_BLACKSMITH,
	SkillStatusChangeTable[SL_HUNTER] =      MAPID_HUNTER,
	SkillStatusChangeTable[SL_SOULLINKER] =  MAPID_SOUL_LINKER,

	//Status that don't have a skill associated.
	StatusIconChangeTable[SC_WEIGHT50] = SI_WEIGHT50;
	StatusIconChangeTable[SC_WEIGHT90] = SI_WEIGHT90;
	StatusIconChangeTable[SC_ASPDPOTION0] = SI_ASPDPOTION;
	StatusIconChangeTable[SC_ASPDPOTION1] = SI_ASPDPOTION;
	StatusIconChangeTable[SC_ASPDPOTION2] = SI_ASPDPOTION;
	StatusIconChangeTable[SC_ASPDPOTION3] = SI_ASPDPOTION;
	StatusIconChangeTable[SC_SPEEDUP0] = SI_SPEEDPOTION;
	StatusIconChangeTable[SC_SPEEDUP1] = SI_SPEEDPOTION;
	StatusIconChangeTable[SC_MIRACLE] = SI_SPIRIT;
	
	//Other SC which are not necessarily associated to skills.
	StatusChangeFlagTable[SC_ASPDPOTION0] = SCB_ASPD;
	StatusChangeFlagTable[SC_ASPDPOTION1] = SCB_ASPD;
	StatusChangeFlagTable[SC_ASPDPOTION2] = SCB_ASPD;
	StatusChangeFlagTable[SC_ASPDPOTION3] = SCB_ASPD;
	StatusChangeFlagTable[SC_SPEEDUP0] = SCB_SPEED;
	StatusChangeFlagTable[SC_SPEEDUP1] = SCB_SPEED;
	StatusChangeFlagTable[SC_ATKPOTION] = SCB_BATK;
	StatusChangeFlagTable[SC_MATKPOTION] = SCB_MATK;
	StatusChangeFlagTable[SC_INCALLSTATUS] |= SCB_STR|SCB_AGI|SCB_VIT|SCB_INT|SCB_DEX|SCB_LUK;
	StatusChangeFlagTable[SC_INCSTR] |= SCB_STR;
	StatusChangeFlagTable[SC_INCAGI] |= SCB_AGI;
	StatusChangeFlagTable[SC_INCVIT] |= SCB_VIT;
	StatusChangeFlagTable[SC_INCINT] |= SCB_INT;
	StatusChangeFlagTable[SC_INCDEX] |= SCB_DEX;
	StatusChangeFlagTable[SC_INCLUK] |= SCB_LUK;
	StatusChangeFlagTable[SC_INCHIT] |= SCB_HIT;
	StatusChangeFlagTable[SC_INCHITRATE] |= SCB_HIT;
	StatusChangeFlagTable[SC_INCFLEE] |= SCB_FLEE;
	StatusChangeFlagTable[SC_INCFLEERATE] |= SCB_FLEE;
	StatusChangeFlagTable[SC_INCMHPRATE] |= SCB_MAXHP;
	StatusChangeFlagTable[SC_INCMSPRATE] |= SCB_MAXSP;
	StatusChangeFlagTable[SC_INCATKRATE] |= SCB_BATK|SCB_WATK;
	StatusChangeFlagTable[SC_INCMATKRATE] |= SCB_MATK;
	StatusChangeFlagTable[SC_INCDEFRATE] |= SCB_DEF;
	StatusChangeFlagTable[SC_STRFOOD] |= SCB_STR;
	StatusChangeFlagTable[SC_AGIFOOD] |= SCB_AGI;
	StatusChangeFlagTable[SC_VITFOOD] |= SCB_VIT;
	StatusChangeFlagTable[SC_INTFOOD] |= SCB_INT;
	StatusChangeFlagTable[SC_DEXFOOD] |= SCB_DEX;
	StatusChangeFlagTable[SC_LUKFOOD] |= SCB_LUK;
	StatusChangeFlagTable[SC_HITFOOD] |= SCB_HIT;
	StatusChangeFlagTable[SC_FLEEFOOD] |= SCB_FLEE;
	StatusChangeFlagTable[SC_BATKFOOD] |= SCB_BATK;
	StatusChangeFlagTable[SC_WATKFOOD] |= SCB_WATK;
	StatusChangeFlagTable[SC_MATKFOOD] |= SCB_MATK;

	//Guild skills don't fit due to their range being beyond MAX_SKILL
	StatusIconChangeTable[SC_GUILDAURA] =    SI_GUILDAURA;
	StatusChangeFlagTable[SC_GUILDAURA] |= SCB_STR|SCB_AGI|SCB_VIT|SCB_DEX;
	StatusIconChangeTable[SC_BATTLEORDERS] = SI_BATTLEORDERS;
	StatusChangeFlagTable[SC_BATTLEORDERS] |= SCB_STR|SCB_INT|SCB_DEX;
#undef set_sc
#undef add_sc
	if (!battle_config.display_hallucination) //Disable Hallucination.
		StatusIconChangeTable[SC_HALLUCINATION] = SI_BLANK;
}

static void initDummyData(void) {
	memset(&dummy_status, 0, sizeof(dummy_status));
	dummy_status.hp = 
	dummy_status.max_hp = 
	dummy_status.max_sp = 
	dummy_status.str =
	dummy_status.agi =
	dummy_status.vit =
	dummy_status.int_ =
	dummy_status.dex =
	dummy_status.luk =
	dummy_status.hit = 1;
	dummy_status.speed = 2000;
	dummy_status.adelay = 4000;
	dummy_status.amotion = 2000;
	dummy_status.dmotion = 2000;
	dummy_status.ele_lv = 1; //Min elemental level.
	dummy_status.mode = MD_CANMOVE;
}

/*==========================================
 * 精錬ボーナス
 *------------------------------------------
 */
int status_getrefinebonus(int lv,int type)
{
	if (lv >= 0 && lv < 5 && type >= 0 && type < 3)
		return refinebonus[lv][type];
	return 0;
}

//Inflicts damage on the target with the according walkdelay.
//If flag&1, damage is passive and does not triggers cancelling status changes.
//If flag&2, fail if target does not has enough to substract.
//If flag&4, if killed, mob must not give exp/loot.
int status_damage(struct block_list *src,struct block_list *target,int hp, int sp, int walkdelay, int flag)
{
	struct status_data *status;
	struct status_change *sc;

	if(sp && target->type != BL_PC)
		sp = 0; //Only players get SP damage.
	
	if (hp < 0) { //Assume absorbed damage.
		status_heal(target, -hp, 0, 1);
		hp = 0;
	}

	if (sp < 0) {
		status_heal(target, 0, -sp, 1);
		sp = 0;
	}
	
	if (!hp && !sp)
		return 0;

	
	if (target->type == BL_SKILL)
		return skill_unit_ondamaged((struct skill_unit *)target, src, hp, gettick());
	
	status = status_get_status_data(target);
	
	if (status == &dummy_status || !status->hp || !target->prev)
		return 0; //Invalid targets: no damage, dead, not on a map.

	sc = status_get_sc(target);

	if (sc && !sc->count)
		sc = NULL;

	if (hp && !(flag&1)) {
		if (sc) {
			if (sc->data[SC_FREEZE].timer != -1)
				status_change_end(target,SC_FREEZE,-1);
			if (sc->data[SC_STONE].timer!=-1 && sc->opt1 == OPT1_STONE)
				status_change_end(target,SC_STONE,-1);
			if (sc->data[SC_SLEEP].timer != -1)
				status_change_end(target,SC_SLEEP,-1);
			if (sc->data[SC_WINKCHARM].timer != -1)
				status_change_end(target,SC_WINKCHARM,-1);
			if (sc->data[SC_CONFUSION].timer != -1)
				status_change_end(target, SC_CONFUSION, -1);
			if (sc->data[SC_TRICKDEAD].timer != -1)
				status_change_end(target, SC_TRICKDEAD, -1);
			if (sc->data[SC_HIDING].timer != -1)
				status_change_end(target, SC_HIDING, -1);
			if (sc->data[SC_CLOAKING].timer != -1)
				status_change_end(target, SC_CLOAKING, -1);
			if (sc->data[SC_CHASEWALK].timer != -1)
				status_change_end(target, SC_CHASEWALK, -1);
			if (sc->data[SC_ENDURE].timer != -1 && !sc->data[SC_ENDURE].val4) {
				//Endure count is only reduced by non-players on non-gvg maps.
				//val4 signals infinite endure. [Skotlex]
				if (src && src->type != BL_PC && !map_flag_gvg(target->m)
					&& --(sc->data[SC_ENDURE].val2) < 0)
					status_change_end(target, SC_ENDURE, -1);
			}
			if (sc->data[SC_GRAVITATION].timer != -1 &&
				sc->data[SC_GRAVITATION].val3 == BCT_SELF) {
				struct skill_unit_group *sg = (struct skill_unit_group *)sc->data[SC_GRAVITATION].val4;
				if (sg) {
					skill_delunitgroup(target,sg);
					sc->data[SC_GRAVITATION].val4 = 0;
					status_change_end(target, SC_GRAVITATION, -1);
				}
			}
			if (sc->data[SC_DEVOTION].val1 && src && battle_getcurrentskill(src) != PA_PRESSURE)
			{
				struct map_session_data *sd2 = map_id2sd(sc->data[SC_DEVOTION].val1);
				if (sd2 && sd2->devotion[sc->data[SC_DEVOTION].val2] == target->id)
				{
					clif_damage(src, &sd2->bl, gettick(), 0, 0, hp, 0, 0, 0);
					status_fix_damage(NULL, &sd2->bl, hp, 0);
					return 0;
				}
				status_change_end(target, SC_DEVOTION, -1);
			}
			if(sc->data[SC_DANCING].timer != -1 && hp > (signed int)status->max_hp>>2)
				skill_stop_dancing(target);
		}
		unit_skillcastcancel(target, 2);
	}

	if (hp >= (signed int)status->hp) {
		if (flag&2) return 0;
		hp = status->hp;
	}

	if (sp > (signed int)status->sp) {
		if (flag&2) return 0;
		sp = status->sp;
	}
	
	status->hp-= hp;
	status->sp-= sp;
	
	if (sc && hp && status->hp) {
		if (sc->data[SC_AUTOBERSERK].timer != -1 &&
			(sc->data[SC_PROVOKE].timer==-1 || !sc->data[SC_PROVOKE].val2) &&
			status->hp < status->max_hp>>2)
			sc_start4(target,SC_PROVOKE,100,10,1,0,0,0);
	}
	
	switch (target->type)
	{
		case BL_MOB:
			mob_damage((TBL_MOB*)target, src, hp);
			break;
		case BL_PC:
			pc_damage((TBL_PC*)target,src,hp,sp);
			break;
		case BL_HOMUNCULUS:
			merc_damage((TBL_HOMUNCULUS*)target,src,hp,sp);
	}

	if (status->hp)
  	{	//Still lives!
		if (walkdelay)
			unit_set_walkdelay(target, gettick(), walkdelay, 0);
		return hp+sp;
	}

	status->hp = 1; //To let the dead function cast skills and all that.
	//NOTE: These dead functions should return: [Skotlex]
	//0: Death cancelled, auto-revived.
	//Non-zero: Standard death. Clear status, cancel move/attack, etc
	//&2: Also remove object from map.
	//&4: Also delete object from memory.
	switch (target->type)
	{
		case BL_MOB:
			flag = mob_dead((TBL_MOB*)target, src, flag&4?3:0);
			break;
		case BL_PC:
			flag = pc_dead((TBL_PC*)target,src);
			break;
		case BL_HOMUNCULUS:
			flag = merc_dead((TBL_HOMUNCULUS*)target,src);
			break;
		default:	//Unhandled case, do nothing to object.
			flag = 0;
			break;
	}

	if(!flag) //Death cancelled.
		return hp+sp;
  
	//Normal death
	status->hp = 0;
	if (battle_config.clear_unit_ondeath &&
		battle_config.clear_unit_ondeath&target->type)
		skill_clear_unitgroup(target);
	status_change_clear(target,0);

	if(flag&2) //remove the unit from the map.
		unit_remove_map(target,1);
	else { //These are handled by unit_remove_map.
		unit_stop_attack(target);
		unit_stop_walking(target,0);
		unit_skillcastcancel(target,0);
		clif_clearchar_area(target,1);
		skill_unit_move(target,gettick(),4);
		skill_cleartimerskill(target);
	}

	if(flag&4) { //Delete from memory.
		map_delblock(target);
		unit_free(target);
	}

		
	return hp+sp;
}

//Heals a character. If flag&1, this is forced healing (otherwise stuff like Berserk can block it)
//If flag&2, when the player is healed, show the HP/SP heal effect.
int status_heal(struct block_list *bl,int hp,int sp, int flag)
{
	struct status_data *status;
	struct status_change *sc;

	status = status_get_status_data(bl);

	if (status == &dummy_status || !status->hp)
		return 0;
	
	sc = status_get_sc(bl);
	if (sc && !sc->count)
		sc = NULL;

	if (hp < 0) {
		status_damage(NULL, bl, -hp, 0, 0, 1);
		hp = 0;
	}
	
	if(hp) {
		if (!(flag&1) && sc && sc->data[SC_BERSERK].timer!=-1)
			hp = 0;

		
		if(hp > (signed int)(status->max_hp - status->hp))
			hp = status->max_hp - status->hp;
	}

	if(sp < 0) {
		status_damage(NULL, bl, 0, -sp, 0, 1);
		sp = 0;
	}

	if(sp) {
		if(sp > (signed int)(status->max_sp - status->sp))
			sp = status->max_sp - status->sp;
	}

	if(!sp && !hp) return 0;

	status->hp+= hp;
	status->sp+= sp;

	if(hp && sc &&
		sc->data[SC_AUTOBERSERK].timer != -1 &&
		sc->data[SC_PROVOKE].timer!=-1 &&
		sc->data[SC_PROVOKE].val2==1 &&
		status->hp>=status->max_hp>>2
	)	//End auto berserk.
		status_change_end(bl,SC_PROVOKE,-1);

	switch(bl->type) {
	case BL_MOB:
		mob_heal((TBL_MOB*)bl,hp);
		break;
	case BL_PC:
		pc_heal((TBL_PC*)bl,hp,sp,flag&2?1:0);
		break;
	case BL_HOMUNCULUS:
		merc_heal((TBL_HOMUNCULUS*)bl,hp,sp);
		break;
	}
	return hp+sp;
}

//Does percentual non-flinching damage/heal. If mob is killed this way,
//no exp/drops will be awarded if there is no src (or src is target)
//If rates are > 0, percent is of current HP/SP
//If rates are < 0, percent is of max HP/SP
//If flag, this is heal, otherwise it is damage.
int status_percent_change(struct block_list *src,struct block_list *target,signed char hp_rate, signed char sp_rate, int flag)
{
	struct status_data *status;
	unsigned int hp  =0, sp = 0;

	status = status_get_status_data(target);
	
	if (hp_rate > 0)
		hp = (hp_rate*status->hp)/100;
	else if (hp_rate < 0)
		hp = (-hp_rate)*status->max_hp/100;
	if (hp_rate && !hp)
		hp = 1;

	if (sp_rate > 0)
		sp = (sp_rate*status->sp)/100;
	else if (sp_rate < 0)
		sp = (-sp_rate)*status->max_sp/100;
	if (sp_rate && !sp)
		sp = 1;

	if (flag) return status_heal(target, hp, sp, 0);
	return status_damage(src, target, hp, sp, 0, (!src||src==target?5:1));
}

int status_revive(struct block_list *bl, unsigned char per_hp, unsigned char per_sp)
{
	struct status_data *status;
	unsigned int hp, sp;
	if (!status_isdead(bl)) return 0;

	status = status_get_status_data(bl);
	if (status == &dummy_status)
		return 0; //Invalid target.
	
	hp = status->max_hp * per_hp/100;
	sp = status->max_sp * per_hp/100;

	if(hp > status->max_hp - status->hp)
		hp = status->max_hp - status->hp;

	if(sp > status->max_sp - status->sp)
		sp = status->max_sp - status->sp;
	
	status->hp += hp;
	status->sp += sp;

	clif_resurrection(bl, 1);
	switch (bl->type) {
		case BL_MOB:
			mob_revive((TBL_MOB*)bl, hp);
			break;
		case BL_PC:
			pc_revive((TBL_PC*)bl, hp, sp);
			break;
	}
	return 1;
}
/*==========================================
 * Checks whether the src can use the skill on the target,
 * taking into account status/option of both source/target. [Skotlex]
 * flag:
 * 	0 - Trying to use skill on target.
 * 	1 - Cast bar is done.
 * 	2 - Skill already pulled off, check is due to ground-based skills or splash-damage ones.
 * src MAY be null to indicate we shouldn't check it, this is a ground-based skill attack.
 * target MAY Be null, in which case the checks are only to see 
 * whether the source can cast or not the skill on the ground.
 *------------------------------------------
 */
int status_check_skilluse(struct block_list *src, struct block_list *target, int skill_num, int flag)
{
	int mode, race, hide_flag;
	struct status_change *sc=NULL, *tsc;

	mode = src?status_get_mode(src):MD_CANATTACK;

	if (src && status_isdead(src))
		return 0;
	
	if (!skill_num) { //Normal attack checks.
		if (!(mode&MD_CANATTACK))
			return 0; //This mode is only needed for melee attacking.
		//Dead state is not checked for skills as some skills can be used 
		//on dead characters, said checks are left to skill.c [Skotlex]
		if (target && status_isdead(target))
			return 0;
	}

	if (skill_num == PA_PRESSURE && flag) {
	//Gloria Avoids pretty much everything....
		tsc = target?status_get_sc(target):NULL;
		if(tsc) {
			if (tsc->option&OPTION_HIDE)
				return 0;
			if (tsc->count && tsc->data[SC_TRICKDEAD].timer != -1)
				return 0;
		}
		return 1;
	}

	if (((src && map_getcell(src->m,src->x,src->y,CELL_CHKBASILICA)) ||
		(target && target != src && map_getcell(target->m,target->x,target->y,CELL_CHKBASILICA)))
		&& !(mode&MD_BOSS))
	{	//Basilica Check
		if (!skill_num) return 0;
		race = skill_get_inf(skill_num);
		if (race&INF_ATTACK_SKILL)
			return 0;
		if (race&INF_GROUND_SKILL && skill_get_unit_target(skill_num)&BCT_ENEMY)
			return 0;
	}	

	if (src) sc = status_get_sc(src);
	
	if(sc && sc->opt1 >0 && (battle_config.sc_castcancel || flag != 1))
		//When sc do not cancel casting, the spell should come out.
		return 0;
	
	if(sc && sc->count)
	{
		if (
			(sc->data[SC_TRICKDEAD].timer != -1 && skill_num != NV_TRICKDEAD)
			|| (sc->data[SC_AUTOCOUNTER].timer != -1 && !flag)
			|| (sc->data[SC_GOSPEL].timer != -1 && sc->data[SC_GOSPEL].val4 == BCT_SELF && skill_num != PA_GOSPEL)
			|| (sc->data[SC_GRAVITATION].timer != -1 && sc->data[SC_GRAVITATION].val3 == BCT_SELF && skill_num != HW_GRAVITATION)
		)
			return 0;

		if (sc->data[SC_WINKCHARM].timer != -1 && target && target->type == BL_PC && !flag)
		{	//Prevents skill usage against players?
			clif_emotion(src, 3);
			return 0;
		}
			
		if (sc->data[SC_BLADESTOP].timer != -1) {
			switch (sc->data[SC_BLADESTOP].val1)
			{
				case 5: if (skill_num == MO_EXTREMITYFIST) break;
				case 4: if (skill_num == MO_CHAINCOMBO) break;
				case 3: if (skill_num == MO_INVESTIGATE) break;
				case 2: if (skill_num == MO_FINGEROFFENSIVE) break;
				default: return 0;
			}
		}
		if (skill_num && //Do not block item-casted skills.
			(src->type != BL_PC || ((TBL_PC*)src)->skillitem != skill_num)
		) {	//Skills blocked through status changes...
			if (!flag && ( //Blocked only from using the skill (stuff like autospell may still go through
				(sc->data[SC_MARIONETTE].timer != -1 && skill_num != CG_MARIONETTE) ||
				(sc->data[SC_MARIONETTE2].timer != -1 && skill_num == CG_MARIONETTE) ||
				sc->data[SC_SILENCE].timer != -1 || 
				sc->data[SC_STEELBODY].timer != -1 ||
				sc->data[SC_BERSERK].timer != -1
			))
				return 0;
			//Skill blocking.
			if (
				(sc->data[SC_VOLCANO].timer != -1 && skill_num == WZ_ICEWALL) ||
				(sc->data[SC_ROKISWEIL].timer != -1 && skill_num != BD_ADAPTATION && !(mode&MD_BOSS)) ||
				(sc->data[SC_HERMODE].timer != -1 && skill_get_inf(skill_num) & INF_SUPPORT_SKILL) ||
				sc->data[SC_NOCHAT].timer != -1
			)
				return 0;

			if (flag!=2 && sc->data[SC_DANCING].timer != -1)
			{
				if (skill_num != BD_ADAPTATION && skill_num != CG_LONGINGFREEDOM
					&& skill_num != BA_MUSICALSTRIKE && skill_num != DC_THROWARROW)
					return 0;
				if (sc->data[SC_DANCING].val1 == CG_HERMODE && skill_num == BD_ADAPTATION)
					return 0;	//Can't amp out of Wand of Hermode :/ [Skotlex]
			}
		}
	}

	if (sc && sc->option)
	{
		if (sc->option&OPTION_HIDE && skill_num != TF_HIDING && skill_num != AS_GRIMTOOTH
			&& skill_num != RG_BACKSTAP && skill_num != RG_RAID && skill_num != NJ_SHADOWJUMP
			&& skill_num != NJ_KIRIKAGE)
			return 0;
//		if (sc->option&OPTION_CLOAK && skill_num == TF_HIDING)
//			return 0; //Latest reports indicate Hiding is usable while Cloaking. [Skotlex]
		if (sc->option&OPTION_CHASEWALK && skill_num != ST_CHASEWALK)
			return 0;
	}
	if (target == NULL || target == src) //No further checking needed.
		return 1;

	tsc = status_get_sc(target);
	if(tsc && tsc->count)
	{	
		if (!(mode & MD_BOSS) && tsc->data[SC_TRICKDEAD].timer != -1)
			return 0;
		if(skill_num == WZ_STORMGUST && tsc->data[SC_FREEZE].timer != -1)
			return 0;
		if(skill_num == PR_LEXAETERNA && (tsc->data[SC_FREEZE].timer != -1 || (tsc->data[SC_STONE].timer != -1 && tsc->opt1 == OPT1_STONE)))
			return 0;
	}

	race = src?status_get_race(src):0; 
	//If targetting, cloak+hide protect you, otherwise only hiding does.
	hide_flag = flag?OPTION_HIDE:(OPTION_HIDE|OPTION_CLOAK|OPTION_CHASEWALK);
		
 	//You cannot hide from ground skills.
	if(skill_get_pl(skill_num) == 2)
		hide_flag &= ~OPTION_HIDE;
	
	switch (target->type)
	{
	case BL_PC:
		{
			struct map_session_data *sd = (struct map_session_data*) target;
			if (pc_isinvisible(sd))
				return 0;
			if (tsc->option&hide_flag
				&& (sd->state.perfect_hiding || !(race == RC_INSECT || race == RC_DEMON || mode&MD_DETECTOR))
				&& !(mode&MD_BOSS))
				return 0;
		}
		break;
	case BL_ITEM:	//Allow targetting of items to pick'em up (or in the case of mobs, to loot them).
		//TODO: Would be nice if this could be used to judge whether the player can or not pick up the item it targets. [Skotlex]
		if (mode&MD_LOOTER)
			return 1;
		else
			return 0;
	default:
		//Check for chase-walk/hiding/cloaking opponents.
		if (tsc && !(mode&MD_BOSS))
		{
			if (tsc->option&hide_flag && !(race == RC_INSECT || race == RC_DEMON || mode&MD_DETECTOR))
				return 0;
		}
	}
	return 1;
}

void status_calc_bl(struct block_list *bl, unsigned long flag);

static int status_base_atk(struct block_list *bl, struct status_data *status)
{
	int flag = 0, str, dex, dstr;
	if (bl->type == BL_PC)
	switch(((TBL_PC*)bl)->status.weapon){
		case W_BOW:
		case W_MUSICAL: 
		case W_WHIP:
		case W_REVOLVER:
		case W_RIFLE:
		case W_SHOTGUN:
		case W_GATLING:
		case W_GRENADE:
			flag = 1;
	}
	if (flag) {
		str = status->dex;
		dex = status->str;
	} else {
		str = status->str;
		dex = status->dex;
	}
	dstr = str/10;
	return str + dstr*dstr + dex/5 + status->luk/5;
}


//Fills in the misc data that can be calculated from the other status info (except for level)
void status_calc_misc(struct status_data *status, int level)
{
	status->matk_min = status->int_+(status->int_/7)*(status->int_/7);
	status->matk_max = status->int_+(status->int_/5)*(status->int_/5);

	status->hit = level + status->dex;
	status->flee = level + status->agi;
	status->def2 = status->vit;
	status->mdef2 = status->int_ + (status->vit>>1);
	
	status->cri = status->luk*3 + 10;
	status->flee2 = status->luk + 10;
}

//Skotlex: Calculates the initial status for the given mob
//first will only be false when the mob leveled up or got a GuardUp level.
int status_calc_mob(struct mob_data* md, int first)
{
	struct status_data *status;
	struct block_list *mbl = NULL;
	int flag=0;

	//Check if we need custom base-status
	if (battle_config.mobs_level_up && md->level != md->db->lv)
		flag|=1;
	
	if (md->special_state.size)
		flag|=2;

	if (md->guardian_data && md->guardian_data->guardup_lv)
		flag|=4;
	
	if (battle_config.slaves_inherit_speed && md->master_id)
		flag|=8;
	
	if (md->master_id && md->special_state.ai>1)
		flag|=16;
		
	if (!flag)
	{ //No special status required.
		if (md->base_status) {
			aFree(md->base_status);
			md->base_status = NULL;
		}
		if(first)
			memcpy(&md->status, &md->db->status, sizeof(struct status_data));
		return 0;
	}
	if (!md->base_status)
		md->base_status = aCalloc(1, sizeof(struct status_data));
	
	status = md->base_status;
	memcpy(status, &md->db->status, sizeof(struct status_data));
	

	if (flag&(8|16))
		mbl = map_id2bl(md->master_id);

	if (flag&8 && mbl) {
		struct status_data *mstatus = status_get_base_status(mbl);
		if (mstatus)
			status->speed = mstatus->speed;
	}
		
	if (flag&16 && mbl)
	{	//Max HP setting from Summon Flora/marine Sphere
		struct unit_data *ud = unit_bl2ud(mbl);
		if (ud)
		{	// different levels of HP according to skill level
			if (ud->skillid == AM_SPHEREMINE) {
				status->max_hp = 2000 + 400*ud->skilllv;
				status->mode|= MD_CANMOVE; //Needed for the skill
			} else { //AM_CANNIBALIZE
				status->max_hp = 1500 + 200*ud->skilllv + 10*status_get_lv(mbl);
				status->mode|= MD_CANATTACK|MD_AGGRESSIVE;
			}
			status->hp = status->max_hp;
		}
	}

	if (flag&1)
	{	// increase from mobs leveling up [Valaris]
		int diff = md->level - md->db->lv;
		status->str+= diff;
		status->agi+= diff;
		status->vit+= diff;
		status->int_+= diff;
		status->dex+= diff;
		status->luk+= diff;
		status->max_hp += diff*status->vit;
		status->max_sp += diff*status->int_;
		status->hp = status->max_hp;
		status->sp = status->max_sp;
		status->speed -= diff;
	}
	
	
	if (flag&2)
	{	// change for sized monsters [Valaris]
		if (md->special_state.size==1) {
			status->max_hp>>=1;
			status->max_sp>>=1;
			if (!status->max_hp) status->max_hp = 1;
			if (!status->max_sp) status->max_sp = 1;
			status->hp=status->max_hp;
			status->sp=status->max_sp;
			status->str>>=1;
			status->agi>>=1;
			status->vit>>=1;
			status->int_>>=1;
			status->dex>>=1;
			status->luk>>=1;
			if (!status->str) status->str = 1;
			if (!status->agi) status->agi = 1;
			if (!status->vit) status->vit = 1;
			if (!status->int_) status->int_ = 1;
			if (!status->dex) status->dex = 1;
			if (!status->luk) status->luk = 1;
		} else if (md->special_state.size==2) {
			status->max_hp<<=1;
			status->max_sp<<=1;
			status->hp=status->max_hp;
			status->sp=status->max_sp;
			status->str<<=1;
			status->agi<<=1;
			status->vit<<=1;
			status->int_<<=1;
			status->dex<<=1;
			status->luk<<=1;
		}
	}

	status->batk = status_base_atk(&md->bl, status);
	status_calc_misc(status, md->level);

	if(flag&4)
	{	// Strengthen Guardians - custom value +10% / lv
		struct guild_castle *gc;
		gc=guild_mapname2gc(map[md->bl.m].name);
		if (!gc)
			ShowError("status_calc_mob: No castle set at map %s\n", map[md->bl.m].name);
		else {
			status->max_hp += 2000 * gc->defense;
			status->max_sp += 200 * gc->defense;
			if (md->guardian_data->number < MAX_GUARDIANS) //Spawn with saved HP
				status->hp = gc->guardian[md->guardian_data->number].hp;
			else //Emperium
				status->hp = status->max_hp;
			status->sp = status->max_sp;
		}
		status->batk += status->batk * 10*md->guardian_data->guardup_lv/100;
		status->rhw.atk += status->rhw.atk * 10*md->guardian_data->guardup_lv/100;
		status->rhw.atk2 += status->rhw.atk2 * 10*md->guardian_data->guardup_lv/100;
		status->aspd_rate -= 10*md->guardian_data->guardup_lv;
	}

	if(!battle_config.enemy_str)
		status->batk = 0;
		
	if(battle_config.enemy_critical_rate != 100)
		status->cri = status->cri*battle_config.enemy_critical_rate/100;
	if (!status->cri && battle_config.enemy_critical_rate)
		status->cri = 1;

	if (!battle_config.enemy_perfect_flee)
		status->flee2 = 0;

	//Initial battle status
	if (!first) {
		status_cpy(&md->status, status);
		if (md->sc.count)
			status_calc_bl(&md->bl, SCB_ALL);
	} else
		memcpy(&md->status, status, sizeof(struct status_data));
	return 1;
}

//Skotlex: Calculates the stats of the given pet.
int status_calc_pet(struct pet_data *pd, int first)
{
	struct map_session_data *sd;
	int lv;
	
	nullpo_retr(0, pd);
	sd = pd->msd;
	if(!sd || sd->status.pet_id == 0 || sd->pd == NULL)
		return 0;

	if (first) {
		memcpy(&pd->status, &pd->db->status, sizeof(struct status_data));
		pd->status.speed = pd->petDB->speed;
	}

	if (battle_config.pet_lv_rate)
	{
		lv =sd->status.base_level*battle_config.pet_lv_rate/100;
		if (lv < 0)
			lv = 1;
		if (lv != sd->pet.level || first)
		{
			struct status_data *bstat = &pd->db->status, *status = &pd->status;
			sd->pet.level = lv;
			if (!first) //Lv Up animation
				clif_misceffect(&pd->bl, 0);
			status->rhw.atk = (bstat->rhw.atk*lv)/pd->db->lv;
			status->rhw.atk2 = (bstat->rhw.atk2*lv)/pd->db->lv;
			status->str = (bstat->str*lv)/pd->db->lv;
			status->agi = (bstat->agi*lv)/pd->db->lv;
			status->vit = (bstat->vit*lv)/pd->db->lv;
			status->int_ = (bstat->int_*lv)/pd->db->lv;
			status->dex = (bstat->dex*lv)/pd->db->lv;
			status->luk = (bstat->luk*lv)/pd->db->lv;
		
			if(status->rhw.atk > battle_config.pet_max_atk1)
				status->rhw.atk = battle_config.pet_max_atk1;
			if(status->rhw.atk2 > battle_config.pet_max_atk2)
				status->rhw.atk2 = battle_config.pet_max_atk2;

			status->str = cap_value(status->str,1,battle_config.pet_max_stats);
			status->agi = cap_value(status->agi,1,battle_config.pet_max_stats);
			status->vit = cap_value(status->vit,1,battle_config.pet_max_stats);
			status->int_= cap_value(status->int_,1,battle_config.pet_max_stats);
			status->dex = cap_value(status->dex,1,battle_config.pet_max_stats);
			status->luk = cap_value(status->luk,1,battle_config.pet_max_stats);

			status->batk = status_base_atk(&pd->bl, &pd->status);
			status_calc_misc(&pd->status, lv);
			if (!battle_config.pet_str)
				pd->status.batk = 0;
			if (!first)	//Not done the first time because the pet is not visible yet
				clif_send_petstatus(sd);
		}
	} else if (first) {
		pd->status.batk = status_base_atk(&pd->bl, &pd->status);
		status_calc_misc(&pd->status, pd->db->lv);
		if (!battle_config.pet_str)
			pd->status.batk = 0;
	}
	
	//Support rate modifier (1000 = 100%)
	pd->rate_fix = 1000*(sd->pet.intimate - battle_config.pet_support_min_friendly)/(1000- battle_config.pet_support_min_friendly) +500;
	if(battle_config.pet_support_rate != 100)
		pd->rate_fix = pd->rate_fix*battle_config.pet_support_rate/100;
	return 1;
}	

int status_calc_homunculus(struct homun_data *hd, int first)
{
	struct status_data *status = &hd->base_status;
	int lv, i;
	/* very proprietary */
	lv=hd->level;
	memset(status, 0, sizeof(struct status_data));
	switch(hd->class_)
	{
	case 6001:	//LIF ~ int,dex,vit
		status->str = 3+lv/7;
		status->agi = 3+2*lv/5;
		status->vit = 4+lv;
		status->int_ = 4+3*lv/4;
		status->dex = 4+2*lv/3;
		status->luk = 3+lv/4;
		for(i=8001;i<8005;++i)
		{
			hd->hskill[i-8001].id=i;
			//hd->hskill[i-8001].level=1;
		}
		break;
	case 6003:	//FILIR ~ str,agi,dex
		status->str = 4+3*lv/4;
		status->agi = 4+2*lv/3;
		status->vit = 3+2*lv/5;
		status->int_ = 3+lv/4;
		status->dex = 4+lv;
		status->luk = 3+lv/7;
		for(i=8009;i<8013;++i)
		{
			hd->hskill[i-8009].id=i;
			//hd->hskill[i-8009].level=1;
		}
		break;
	case 6002:	//AMISTR ~ str,vit,luk
		status->str = 4+lv;
		status->agi = 3+lv/4;
		status->vit = 3+3*lv/4;
		status->int_ = 3+lv/10;
		status->dex = 3+2*lv/5;
		status->luk = 4+2*lv/3;
		for(i=8005;i<8009;++i)
		{
			hd->hskill[i-8005].id=i;
			//hd->hskill[i-8005].level=1;
		}
		break;
	case 6004:	//VANILMIRTH ~ int,dex,luk
		status->str = 3+lv/4;
		status->agi = 3+lv/7;
		status->vit = 3+2*lv/5;
		status->int_ = 4+lv;
		status->dex = 4+2*lv/3;
		status->luk = 4+3*lv/4;
		for(i=8013;i<8017;++i)
		{
			hd->hskill[i-8013].id=i;
			//hd->hskill[i-8013].level=1;
		}
		break;
	default:
		if (battle_config.error_log)
			ShowError("status_calc_homun: Unknown class %d\n", hd->class_);
		memcpy(status, &dummy_status, sizeof(struct status_data));
		break;
	}
	status->hp = 10; //Revive HP/SP?
	status->sp = 0;
	status->max_hp=500+lv*10+lv*lv;
	status->max_sp=300+lv*11+lv*lv*90/100;
	status->speed=0x96;
	status->batk = status_base_atk(&hd->bl, status);
	status_calc_misc(status, hd->level);

	// hp recovery
	hd->regenhp = 1 + (status->vit/5) + (status->max_hp/200);

	// sp recovery
	hd->regensp = 1 + (status->int_/6) + (status->max_sp/100);
	if(status->int_ >= 120)
		hd->regensp += ((status->int_-120)>>1) + 4;

	status->amotion = 1800 - (1800 * status->agi / 250 + 1800 * status->dex / 1000);
	status->amotion	-= 200;
	status->dmotion=status->amotion;
	status_calc_bl(&hd->bl, SCB_ALL);
	return 1;
}

static unsigned int status_base_pc_maxhp(struct map_session_data* sd, struct status_data *status)
{
	unsigned int val;
	val = (3500 + sd->status.base_level*hp_coefficient2[sd->status.class_]
		+ hp_sigma_val[sd->status.class_][sd->status.base_level-1])/100
		* (100 + status->vit)/100 + sd->param_equip[2];
	if (sd->class_&JOBL_UPPER)
		val += val * 30/100;
	else if (sd->class_&JOBL_BABY)
		val -= val * 30/100;
	if ((sd->class_&MAPID_UPPERMASK) == MAPID_TAEKWON && sd->status.base_level >= 90 && pc_famerank(sd->char_id, MAPID_TAEKWON))
		val *= 3; //Triple max HP for top ranking Taekwons over level 90.
	return val;
}

static unsigned int status_base_pc_maxsp(struct map_session_data* sd, struct status_data *status)
{
	unsigned int val;
	val = (1000 + sd->status.base_level*sp_coefficient[sd->status.class_])/100
		* (100 + status->int_)/100 + sd->param_equip[3];
	if (sd->class_&JOBL_UPPER)
		val += val * 30/100;
	else if (sd->class_&JOBL_BABY)
		val -= val * 30/100;
	if ((sd->class_&MAPID_UPPERMASK) == MAPID_TAEKWON && sd->status.base_level >= 90 && pc_famerank(sd->char_id, MAPID_TAEKWON))
		val *= 3; //Triple max SP for top ranking Taekwons over level 90.
	
	return val;
}


//Calculates player data from scratch without counting SC adjustments.
//Should be invoked whenever players raise stats, learn passive skills or change equipment.
int status_calc_pc(struct map_session_data* sd,int first)
{
	static int calculating = 0; //Check for recursive call preemption. [Skotlex]
	struct status_data b_status, *status;
	struct weapon_atk b_lhw;
	struct skill b_skill[MAX_SKILL];
		
	int b_weight,b_max_weight;
	int b_paramcard[6];
	int i,index;
	int skill,refinedef=0;

	if (++calculating > 10) //Too many recursive calls!
		return -1;

	memcpy(&b_status, &sd->battle_status, sizeof(struct status_data));
	memcpy(&b_lhw, &sd->battle_lhw, sizeof(struct weapon_atk));
	b_status.lhw = &b_lhw;
	
	memcpy(b_skill,&sd->status.skill,sizeof(b_skill));
	b_weight = sd->weight;
	b_max_weight = sd->max_weight;
	
	pc_calc_skilltree(sd);	// スキルツリ?の計算
	
	sd->max_weight = max_weight_base[sd->status.class_]+sd->status.str*300;

	if(first&1) {
		//Load Hp/SP from char-received data.
		sd->battle_status.hp = sd->status.hp;
		sd->battle_status.sp = sd->status.sp;
		sd->battle_status.lhw = &sd->battle_lhw;
		sd->base_status.lhw = &sd->base_lhw;
		
		sd->weight=0;
		for(i=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid==0 || sd->inventory_data[i] == NULL)
				continue;
			sd->weight += sd->inventory_data[i]->weight*sd->status.inventory[i].amount;
		}
		sd->cart_max_weight=battle_config.max_cart_weight;
		sd->cart_weight=0;
		sd->cart_max_num=MAX_CART;
		sd->cart_num=0;
		for(i=0;i<MAX_CART;i++){
			if(sd->status.cart[i].nameid==0)
				continue;
			sd->cart_weight+=itemdb_weight(sd->status.cart[i].nameid)*sd->status.cart[i].amount;
			sd->cart_num++;
		}
	}

	status = &sd->base_status;
	// these are not zeroed. [zzo]
	sd->hprate=100;
	sd->sprate=100;
	sd->castrate=100;
	sd->delayrate=100;
	sd->dsprate=100;
	sd->speed_rate = 100;
	sd->hprecov_rate = 100;
	sd->sprecov_rate = 100;
	sd->atk_rate = sd->matk_rate = 100;
	sd->critical_rate = sd->hit_rate = sd->flee_rate = sd->flee2_rate = 100;
	sd->def_rate = sd->def2_rate = sd->mdef_rate = sd->mdef2_rate = 100;

	// zeroed arays, order follows the order in map.h.
	// add new arrays to the end of zeroed area in map.h (see comments) and size here. [zzo]
	memset (sd->param_bonus, 0, sizeof(sd->param_bonus)
		+ sizeof(sd->param_equip)
		+ sizeof(sd->subele)
		+ sizeof(sd->subrace)
		+ sizeof(sd->subrace2)
		+ sizeof(sd->subsize)
		+ sizeof(sd->addeff)
		+ sizeof(sd->addeff2)
		+ sizeof(sd->reseff)
		+ sizeof(sd->weapon_coma_ele)
		+ sizeof(sd->weapon_coma_race)
		+ sizeof(sd->weapon_atk)
		+ sizeof(sd->weapon_atk_rate)
		+ sizeof(sd->arrow_addele) 
		+ sizeof(sd->arrow_addrace)
		+ sizeof(sd->arrow_addsize)
		+ sizeof(sd->arrow_addeff)
		+ sizeof(sd->arrow_addeff2)
		+ sizeof(sd->magic_addele)
		+ sizeof(sd->magic_addrace)
		+ sizeof(sd->magic_addsize)
		+ sizeof(sd->critaddrace)
		+ sizeof(sd->expaddrace)
		+ sizeof(sd->itemhealrate)
		+ sizeof(sd->addeff3)
		+ sizeof(sd->addeff3_type)
		+ sizeof(sd->sp_gain_race)
		);

	memset (&sd->right_weapon.overrefine, 0, sizeof(sd->right_weapon) - sizeof(sd->right_weapon.atkmods));
	memset (&sd->left_weapon.overrefine, 0, sizeof(sd->left_weapon) - sizeof(sd->left_weapon.atkmods));

	memset(&sd->special_state,0,sizeof(sd->special_state));
	memset(&status->max_hp, 0, sizeof(struct status_data)-(sizeof(status->hp)+sizeof(status->sp)+sizeof(status->lhw)));
	memset(status->lhw, 0, sizeof(struct weapon_atk));

	//FIXME: Most of these stuff should be calculated once, but how do I fix the memset above to do that? [Skotlex]
	status->speed = DEFAULT_WALK_SPEED;
	status->aspd_rate = 100;
	status->mode = MD_CANMOVE|MD_CANATTACK|MD_LOOTER|MD_ASSIST|MD_AGGRESSIVE|MD_CASTSENSOR;
	status->size = (sd->class_&JOBL_BABY)?0:1;
	if (battle_config.character_size && pc_isriding(sd)) { //[Lupus]
		if (sd->class_&JOBL_BABY) {
			if (battle_config.character_size&2)
				status->size++;
		} if(battle_config.character_size&1)
			status->size++;
	}
	status->aspd_rate = 100;
	status->ele_lv = 1;
	status->race = RC_DEMIHUMAN;

	//zero up structures...
	memset(&sd->autospell,0,sizeof(sd->autospell)
		+ sizeof(sd->autospell2)
		+ sizeof(sd->skillatk)
		+ sizeof(sd->skillblown)
		+ sizeof(sd->add_def)
		+ sizeof(sd->add_mdef)
		+ sizeof(sd->add_dmg)
		+ sizeof(sd->add_mdmg)
		+ sizeof(sd->add_drop)
	);
	
	// vars zeroing. ints, shorts, chars. in that order.
	memset (&sd->arrow_atk, 0,sizeof(sd->arrow_atk)
		+ sizeof(sd->arrow_ele)
		+ sizeof(sd->arrow_cri)
		+ sizeof(sd->arrow_hit)
		+ sizeof(sd->nhealhp)
		+ sizeof(sd->nhealsp)
		+ sizeof(sd->nshealhp)
		+ sizeof(sd->nshealsp)
		+ sizeof(sd->nsshealhp)
		+ sizeof(sd->nsshealsp)
		+ sizeof(sd->critical_def)
		+ sizeof(sd->double_rate)
		+ sizeof(sd->long_attack_atk_rate)
		+ sizeof(sd->near_attack_def_rate)
		+ sizeof(sd->long_attack_def_rate)
		+ sizeof(sd->magic_def_rate)
		+ sizeof(sd->misc_def_rate)
		+ sizeof(sd->ignore_mdef_ele)
		+ sizeof(sd->ignore_mdef_race)
		+ sizeof(sd->perfect_hit)
		+ sizeof(sd->perfect_hit_add)
		+ sizeof(sd->get_zeny_rate)
		+ sizeof(sd->get_zeny_num)
		+ sizeof(sd->double_add_rate)
		+ sizeof(sd->short_weapon_damage_return)
		+ sizeof(sd->long_weapon_damage_return)
		+ sizeof(sd->magic_damage_return)
		+ sizeof(sd->random_attack_increase_add)
		+ sizeof(sd->random_attack_increase_per)
		+ sizeof(sd->break_weapon_rate)
		+ sizeof(sd->break_armor_rate)
		+ sizeof(sd->crit_atk_rate)
		+ sizeof(sd->hp_loss_rate)
		+ sizeof(sd->sp_loss_rate)
		+ sizeof(sd->classchange)
		+ sizeof(sd->speed_add_rate)
		+ sizeof(sd->aspd_add_rate)
		+ sizeof(sd->setitem_hash)
		+ sizeof(sd->setitem_hash2)
		// shorts
		+ sizeof(sd->splash_range)
		+ sizeof(sd->splash_add_range)
		+ sizeof(sd->add_steal_rate)
		+ sizeof(sd->hp_loss_value)
		+ sizeof(sd->sp_loss_value)
		+ sizeof(sd->hp_loss_type)
		+ sizeof(sd->hp_gain_value)
		+ sizeof(sd->sp_gain_value)
		+ sizeof(sd->add_drop_count)
		+ sizeof(sd->unbreakable)
		+ sizeof(sd->unbreakable_equip)
		+ sizeof(sd->unstripable_equip)
		+ sizeof(sd->no_regen)
		+ sizeof(sd->add_def_count)
		+ sizeof(sd->add_mdef_count)
		+ sizeof(sd->add_dmg_count)
		+ sizeof(sd->add_mdmg_count)
		);

	for(i=0;i<10;i++) {
		current_equip_item_index = index = sd->equip_index[i]; //We pass INDEX to current_equip_item_index - for EQUIP_SCRIPT (new cards solution) [Lupus]
		if(index < 0)
			continue;
		if(i == 9 && sd->equip_index[8] == index)
			continue;
		if(i == 5 && sd->equip_index[4] == index)
			continue;
		if(i == 6 && (sd->equip_index[5] == index || sd->equip_index[4] == index))
			continue;

		if(sd->inventory_data[index]) {
			int j,c;
			struct item_data *data;
	
			//Card script execution.
			if(sd->status.inventory[index].card[0]==0x00ff ||
				sd->status.inventory[index].card[0]==0x00fe ||
				sd->status.inventory[index].card[0]==(short)0xff00)
				continue;
			for(j=0;j<sd->inventory_data[index]->slot;j++){	
				current_equip_card_id= c= sd->status.inventory[index].card[j];
				if(!c)
					continue;
				data = itemdb_exists(c);
				if(!data)
					continue;
				if(first&1 && data->equip_script)
			  	{	//Execute equip-script on login
					run_script(data->equip_script,0,sd->bl.id,0);
					if (!calculating)
						return 1;
				}
				if(!data->script)
					continue;
				if(data->flag.no_equip) { //Card restriction checks.
					if(map[sd->bl.m].flag.restricted && data->flag.no_equip&map[sd->bl.m].zone)
						continue;
					if(map[sd->bl.m].flag.pvp && data->flag.no_equip&1)
						continue;
					if(map_flag_gvg(sd->bl.m) && data->flag.no_equip&2) 
						continue;
				}
				if(i == 8 && sd->status.inventory[index].equip == 0x20)
				{	//Left hand status.
					sd->state.lr_flag = 1;
					run_script(data->script,0,sd->bl.id,0);
					sd->state.lr_flag = 0;
				} else
					run_script(data->script,0,sd->bl.id,0);
				if (!calculating) //Abort, run_script his function. [Skotlex]
					return 1;
			}
		}
	}
	
	if(sd->status.pet_id > 0 && battle_config.pet_status_support && sd->pet.intimate > 0)
	{ // Pet
		struct pet_data *pd=sd->pd;
		if(pd && (!battle_config.pet_equip_required || pd->equip > 0) &&
			pd->state.skillbonus == 1 && pd->bonus) //Skotlex: Readjusted for pets
			pc_bonus(sd,pd->bonus->type, pd->bonus->val);
	}
	memcpy(b_paramcard,sd->param_bonus,sizeof(b_paramcard));
	memset(sd->param_bonus, 0, sizeof(sd->param_bonus));
	
	// ?備品によるステ?タス?化はここで?行
	for(i=0;i<10;i++) {
		current_equip_item_index = index = sd->equip_index[i]; //We pass INDEX to current_equip_item_index - for EQUIP_SCRIPT (new cards solution) [Lupus]
		if(index < 0)
			continue;
		if(i == 9 && sd->equip_index[8] == index)
			continue;
		if(i == 5 && sd->equip_index[4] == index)
			continue;
		if(i == 6 && (sd->equip_index[5] == index || sd->equip_index[4] == index))
			continue;
		if(!sd->inventory_data[index])
			continue;
		
		status->def += sd->inventory_data[index]->def;

		if(first&1 && sd->inventory_data[index]->equip_script)
	  	{	//Execute equip-script on login
			run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
			if (!calculating)
				return 1;
		}

		if(sd->inventory_data[index]->type == 4) {
			int r,wlv = sd->inventory_data[index]->wlv;
			struct weapon_data *wd;
			struct weapon_atk *wa;
			
			if (wlv >= MAX_REFINE_BONUS) 
				wlv = MAX_REFINE_BONUS - 1;
			if(i == 8 && sd->status.inventory[index].equip == 0x20) {
				wd = &sd->left_weapon; // Left-hand weapon
				wa = status->lhw;
			} else {
				wd = &sd->right_weapon;
				wa = &status->rhw;
			}
			wa->atk += sd->inventory_data[index]->atk;
			wa->atk2 = (r=sd->status.inventory[index].refine)*refinebonus[wlv][0];
			if((r-=refinebonus[wlv][2])>0) //Overrefine bonus.
				wd->overrefine = r*refinebonus[wlv][1];

			wa->range += sd->inventory_data[index]->range;
			if(sd->inventory_data[index]->script) {
				if (wd == &sd->left_weapon) {
					sd->state.lr_flag = 1;
					run_script(sd->inventory_data[index]->script,0,sd->bl.id,0);
					sd->state.lr_flag = 0;
				} else
					run_script(sd->inventory_data[index]->script,0,sd->bl.id,0);
				if (!calculating) //Abort, run_script retriggered this. [Skotlex]
					return 1;
			}

			if(sd->status.inventory[index].card[0]==0x00ff)
			{	// Forged weapon
				wd->star += (sd->status.inventory[index].card[1]>>8);
				if(wd->star >= 15) wd->star = 40; // 3 Star Crumbs now give +40 dmg
				if(pc_famerank( MakeDWord(sd->status.inventory[index].card[2],sd->status.inventory[index].card[3]) ,MAPID_BLACKSMITH))
					wd->star += 10;
				
				if (!wa->ele) //Do not overwrite element from previous bonuses.
					wa->ele = (sd->status.inventory[index].card[1]&0x0f);
			}
		}
		else if(sd->inventory_data[index]->type == 5) {
			refinedef += sd->status.inventory[index].refine*refinebonus[0][0];
			if(sd->inventory_data[index]->script) {
				run_script(sd->inventory_data[index]->script,0,sd->bl.id,0);
				if (!calculating) //Abort, run_script retriggered this. [Skotlex]
					return 1;
			}
		}
	}

	if(sd->equip_index[10] >= 0){ // 矢
		index = sd->equip_index[10];
		if(sd->inventory_data[index]){		// Arrows
			sd->state.lr_flag = 2;
			run_script(sd->inventory_data[index]->script,0,sd->bl.id,0);
			sd->state.lr_flag = 0;
			sd->arrow_atk += sd->inventory_data[index]->atk;
		}
	}
	
	//Store equipment script bonuses 
	memcpy(sd->param_equip,sd->param_bonus,sizeof(sd->param_equip));
	//We store card bonuses here because Improve Concentration is the only SC
	//that will not take it into consideration when buffing you up.
	memcpy(sd->param_bonus, b_paramcard, sizeof(sd->param_bonus));
	
	status->def += (refinedef+50)/100;

	if(status->rhw.range < 1) status->rhw.range = 1;
	if(status->lhw->range < 1) status->lhw->range = 1;
	if(status->rhw.range < status->lhw->range)
		status->rhw.range = status->lhw->range;

	sd->double_rate += sd->double_add_rate;
	sd->perfect_hit += sd->perfect_hit_add;
	sd->splash_range += sd->splash_add_range;
	if(sd->aspd_add_rate)	
		sd->aspd_rate += sd->aspd_add_rate;
	if(sd->speed_add_rate)	
		sd->speed_rate += sd->speed_add_rate;

	// Damage modifiers from weapon type
	sd->right_weapon.atkmods[0] = atkmods[0][sd->weapontype1];
	sd->right_weapon.atkmods[1] = atkmods[1][sd->weapontype1];
	sd->right_weapon.atkmods[2] = atkmods[2][sd->weapontype1];
	sd->left_weapon.atkmods[0] = atkmods[0][sd->weapontype2];
	sd->left_weapon.atkmods[1] = atkmods[1][sd->weapontype2];
	sd->left_weapon.atkmods[2] = atkmods[2][sd->weapontype2];

// ----- STATS CALCULATION -----

	// Job bonuses
	for(i=0;i<(int)sd->status.job_level && i<MAX_LEVEL;i++){
		if(!job_bonus[sd->status.class_][i])
			continue;
		switch(job_bonus[sd->status.class_][i]) {
			case 1:
				status->str++;
				break;
			case 2:
				status->agi++;
				break;
			case 3:
				status->vit++;
				break;
			case 4:
				status->int_++;
				break;
			case 5:
				status->dex++;
				break;
			case 6:
				status->luk++;
				break;
		}
	}

	// If a Super Novice has never died and is at least joblv 70, he gets all stats +10
	if((sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE && sd->die_counter == 0 && sd->status.job_level >= 70){
		status->str += 10;
		status->agi += 10;
		status->vit += 10;
		status->int_+= 10;
		status->dex += 10;
		status->luk += 10;
	}

	// Absolute modifiers from passive skills
	if(pc_checkskill(sd,BS_HILTBINDING)>0)
		status->str++;
	if((skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0)
		status->int_ += (skill+1)/2; // +1 INT / 2 lv
	if((skill=pc_checkskill(sd,AC_OWL))>0)
		status->dex += skill;

	// Bonuses from cards and equipment as well as base stat, remember to avoid overflows.
	i = status->str + sd->status.str + b_paramcard[0] + sd->param_equip[0];
	status->str = i<0?0:(i>USHRT_MAX?USHRT_MAX:i);
	i = status->agi + sd->status.agi + b_paramcard[1] + sd->param_equip[1];
	status->agi = i<0?0:(i>USHRT_MAX?USHRT_MAX:i);
	i = status->vit + sd->status.vit + b_paramcard[2] + sd->param_equip[2];
	status->vit = i<0?0:(i>USHRT_MAX?USHRT_MAX:i);
	i = status->int_+ sd->status.int_+ b_paramcard[3] + sd->param_equip[3];
	status->int_ = i<0?0:(i>USHRT_MAX?USHRT_MAX:i);
	i = status->dex + sd->status.dex + b_paramcard[4] + sd->param_equip[4];
	status->dex = i<0?0:(i>USHRT_MAX?USHRT_MAX:i);
	i = status->luk + sd->status.luk + b_paramcard[5] + sd->param_equip[5];
	status->luk = i<0?0:(i>USHRT_MAX?USHRT_MAX:i);
	
// ------ BASE ATTACK CALCULATION ------

	// Basic Base ATK value
	status->batk += status_base_atk(&sd->bl,status);
	// weapon-type bonus (FIXME: Why is the weapon_atk bonus applied to base attack?)
	if (sd->status.weapon < MAX_WEAPON_TYPE && sd->weapon_atk[sd->status.weapon])
		status->batk += sd->weapon_atk[sd->status.weapon];
	// Absolute modifiers from passive skills
	if((skill=pc_checkskill(sd,BS_HILTBINDING))>0)
		status->batk += 4;

// ----- MATK CALCULATION -----

	// Basic MATK value
	status->matk_max += status->int_+(status->int_/5)*(status->int_/5);
	status->matk_min += status->int_+(status->int_/7)*(status->int_/7);

// ----- CRIT CALCULATION -----

	// Basic Crit value
	status->cri += (status->luk*3)+10;

// ----- HIT CALCULATION -----

	// Basic Hit value
	status->hit += status->dex + sd->status.base_level;

	// Absolute modifiers from passive skills
	if((skill=pc_checkskill(sd,BS_WEAPONRESEARCH))>0)
		status->hit += skill*2;
	if((skill=pc_checkskill(sd,AC_VULTURE))>0){
		status->hit += skill;
		if(sd->status.weapon == W_BOW)
			status->rhw.range += skill;
	}
	if(sd->status.weapon >= W_REVOLVER && sd->status.weapon <= W_GRENADE)
  	{
		if((skill=pc_checkskill(sd,GS_SINGLEACTION))>0)
			status->hit += 2*skill;
		if((skill=pc_checkskill(sd,GS_SNAKEEYE))>0) {
			status->hit += skill;
			status->rhw.range += skill;
		}
	}

// ----- FLEE CALCULATION -----

	// Basic Flee value
	status->flee += status->agi + sd->status.base_level;

	// Absolute modifiers from passive skills
	if((skill=pc_checkskill(sd,TF_MISS))>0)
		status->flee += skill*(sd->class_&JOBL_2 && (sd->class_&MAPID_BASEMASK) == MAPID_THIEF? 4 : 3);
	if((skill=pc_checkskill(sd,MO_DODGE))>0)
		status->flee += (skill*3)>>1;

// ----- PERFECT DODGE CALCULATION -----

	// Basic Perfect Dodge value
	status->flee2 += status->luk+10;

// ----- VIT-DEF CALCULATION -----

	// Basic VIT-DEF value
	status->def2 += status->vit;

// ----- EQUIPMENT-DEF CALCULATION -----

	// Apply relative modifiers from equipment
	if(sd->def_rate != 100)
		status->def = status->def * sd->def_rate/100;

	if (!battle_config.weapon_defense_type && status->def > battle_config.max_def)
	{
		status->def2 += battle_config.over_def_bonus*(status->def -battle_config.max_def);
		status->def = (unsigned char)battle_config.max_def;
	}

// ----- INT-MDEF CALCULATION -----

	// Basic INT-MDEF value
	status->mdef2 += status->int_ + (status->vit>>1);

// ----- EQUIPMENT-MDEF CALCULATION -----

	// Apply relative modifiers from equipment
	if(sd->mdef_rate != 100)
		status->mdef = status->mdef * sd->mdef_rate/100;

	if (!battle_config.magic_defense_type && status->mdef > battle_config.max_def)
	{
		status->mdef2 += battle_config.over_def_bonus*(status->mdef -battle_config.max_def);
		status->mdef = (unsigned char)battle_config.max_def;
	}
	
// ----- WALKING SPEED CALCULATION -----

	// Relative modifiers from passive skills
	if(pc_isriding(sd) && pc_checkskill(sd,KN_RIDING)>0)
		status->speed -= status->speed * 25/100;
	if(pc_iscarton(sd) && (skill=pc_checkskill(sd,MC_PUSHCART))>0)
		status->speed += status->speed * (100-10*skill)/100;

// ----- ASPD CALCULATION -----
// Unlike other stats, ASPD rate modifiers from skills/SCs/items/etc are first all added together, then the final modifier is applied

	// Basic ASPD value
	if (sd->status.weapon < MAX_WEAPON_TYPE)
		i = aspd_base[sd->status.class_][sd->status.weapon]-(status->agi*4+status->dex)*aspd_base[sd->status.class_][sd->status.weapon]/1000;
	else
		i = (
			(aspd_base[sd->status.class_][sd->weapontype1]
			-(status->agi*4+status->dex)*aspd_base[sd->status.class_][sd->weapontype1]/1000) +
			(aspd_base[sd->status.class_][sd->weapontype2]
			-(status->agi*4+status->dex)*aspd_base[sd->status.class_][sd->weapontype2]/1000)
			) *2/3; //From what I read in rodatazone, 2/3 should be more accurate than 0.7 -> 140 / 200; [Skotlex]

	status->amotion = cap_value(i,battle_config.max_aspd,2000);

	// Relative modifiers from passive skills
	if((skill=pc_checkskill(sd,SA_ADVANCEDBOOK))>0 && sd->status.weapon == W_BOOK)
		status->aspd_rate -= (skill/2);
	if((skill = pc_checkskill(sd,SG_DEVIL)) > 0 && !pc_nextjobexp(sd))
		status->aspd_rate -= (skill*3);
	if((skill=pc_checkskill(sd,GS_SINGLEACTION))>0 &&
		(sd->status.weapon >= W_REVOLVER && sd->status.weapon <= W_GRENADE))
		status->aspd_rate -= (skill/2);
	if(pc_isriding(sd))
		status->aspd_rate += 50-10*pc_checkskill(sd,KN_CAVALIERMASTERY);
	
	status->adelay = 2*status->amotion;
	

// ----- DMOTION -----
//
	i =  800-status->agi*4;
	status->dmotion = cap_value(i, 400, 800);

// ----- HP MAX CALCULATION -----

	// Basic MaxHP value
	//We hold the standard Max HP here to make it faster to recalculate on vit changes.
	sd->status.max_hp = status_base_pc_maxhp(sd,status);
	status->max_hp += sd->status.max_hp;

	if((sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE && sd->status.base_level >= 99)
		status->max_hp += 2000;

	// Absolute modifiers from passive skills
	if((skill=pc_checkskill(sd,CR_TRUST))>0)
		status->max_hp += skill*200;

// ----- SP MAX CALCULATION -----

	// Basic MaxSP value
	sd->status.max_sp = status_base_pc_maxsp(sd,status);
	status->max_sp += sd->status.max_sp;

	// Absolute modifiers from passive skills
	if((skill=pc_checkskill(sd,SL_KAINA))>0)
		status->max_sp += 30*skill;

	if(status->sp>status->max_sp)
		status->sp=status->max_sp;

// ----- RESPAWN HP/SP -----
// 
	//Calc respawn hp and store it on base_status
	if (sd->special_state.restart_full_recover)
	{
		status->hp = status->max_hp;
		status->sp = status->max_sp;
	} else {
		if((sd->class_&MAPID_BASEMASK) == MAPID_NOVICE && !(sd->class_&JOBL_2) 
			&& battle_config.restart_hp_rate < 50) 
			status->hp=status->max_hp>>1;
		else 
			status->hp=status->max_hp * battle_config.restart_hp_rate/100;
		if(status->hp < 0)
			status->hp = 1;

		status->sp = status->max_sp * battle_config.restart_sp_rate /100;
	}

// ----- MISC CALCULATIONS -----

	// Weight
	if((skill=pc_checkskill(sd,MC_INCCARRY))>0)
		sd->max_weight += 2000*skill;
	if(pc_isriding(sd) && pc_checkskill(sd,KN_RIDING)>0)
		sd->max_weight += 10000;
	if(sd->sc.data[SC_KNOWLEDGE].timer != -1)
		sd->max_weight += sd->max_weight*sd->sc.data[SC_KNOWLEDGE].val1/10;
	
	// Skill SP cost
	if((skill=pc_checkskill(sd,HP_MANARECHARGE))>0 )
		sd->dsprate -= 4*skill;

	if(sd->sc.count){
		if(sd->sc.data[SC_SERVICE4U].timer!=-1)
			sd->dsprate -= sd->sc.data[SC_SERVICE4U].val3;
	}

	if(sd->dsprate < 0) sd->dsprate = 0;

	// Anti-element and anti-race
	if((skill=pc_checkskill(sd,CR_TRUST))>0)
		sd->subele[6] += skill*5;
	if((skill=pc_checkskill(sd,BS_SKINTEMPER))>0) {
		sd->subele[0] += skill;
		sd->subele[3] += skill*4;
	}
	if((skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){
		skill = skill*4;
		sd->right_weapon.addrace[RC_DRAGON]+=skill;
		sd->left_weapon.addrace[RC_DRAGON]+=skill;
		sd->magic_addrace[RC_DRAGON]+=skill;
		sd->subrace[RC_DRAGON]+=skill;
	}

	if(sd->sc.count){
     	if(sd->sc.data[SC_CONCENTRATE].timer!=-1)
		{	//Update the card-bonus data
			sd->sc.data[SC_CONCENTRATE].val3 = sd->param_bonus[1]; //Agi
			sd->sc.data[SC_CONCENTRATE].val4 = sd->param_bonus[4]; //Dex
		}
     	if(sd->sc.data[SC_SIEGFRIED].timer!=-1){
			sd->subele[1] += sd->sc.data[SC_SIEGFRIED].val2;
			sd->subele[2] += sd->sc.data[SC_SIEGFRIED].val2;
			sd->subele[3] += sd->sc.data[SC_SIEGFRIED].val2;
			sd->subele[4] += sd->sc.data[SC_SIEGFRIED].val2;
			sd->subele[5] += sd->sc.data[SC_SIEGFRIED].val2;
			sd->subele[6] += sd->sc.data[SC_SIEGFRIED].val2;
			sd->subele[7] += sd->sc.data[SC_SIEGFRIED].val2;
			sd->subele[8] += sd->sc.data[SC_SIEGFRIED].val2;
			sd->subele[9] += sd->sc.data[SC_SIEGFRIED].val2;
		}
		if(sd->sc.data[SC_PROVIDENCE].timer!=-1){
			sd->subele[6] += sd->sc.data[SC_PROVIDENCE].val2;
			sd->subrace[RC_DEMON] += sd->sc.data[SC_PROVIDENCE].val2;
		}
	}

	status_cpy(&sd->battle_status, status);
	status_calc_bl(&sd->bl, SCB_ALL); //Status related changes.
	status = &sd->battle_status; //Need to compare versus this.
	
// ----- CLIENT-SIDE REFRESH -----
	if(memcmp(b_skill,sd->status.skill,sizeof(sd->status.skill)))
		clif_skillinfoblock(sd);
	if(b_status.speed != status->speed)
		clif_updatestatus(sd,SP_SPEED);
	if(b_weight != sd->weight)
		clif_updatestatus(sd,SP_WEIGHT);
	if(b_max_weight != sd->max_weight) {
		clif_updatestatus(sd,SP_MAXWEIGHT);
		pc_checkweighticon(sd);
	}
	if(b_status.str != status->str)
		clif_updatestatus(sd,SP_STR);
	if(b_status.agi != status->agi)
		clif_updatestatus(sd,SP_AGI);
	if(b_status.vit != status->vit)
		clif_updatestatus(sd,SP_VIT);
	if(b_status.int_ != status->int_)
		clif_updatestatus(sd,SP_INT);
	if(b_status.dex != status->dex)
		clif_updatestatus(sd,SP_DEX);
	if(b_status.luk != status->luk)
		clif_updatestatus(sd,SP_LUK);
	if(b_status.hit != status->hit)
		clif_updatestatus(sd,SP_HIT);
	if(b_status.flee != status->flee)
		clif_updatestatus(sd,SP_FLEE1);
	if(b_status.amotion != status->amotion)
		clif_updatestatus(sd,SP_ASPD);
	if(b_status.rhw.atk != status->rhw.atk ||
		b_status.lhw->atk != status->lhw->atk ||
		b_status.batk != status->batk)
		clif_updatestatus(sd,SP_ATK1);
	if(b_status.def != status->def)
		clif_updatestatus(sd,SP_DEF1);
	if(b_status.rhw.atk2 != status->rhw.atk2 ||
		b_status.lhw->atk2 != status->lhw->atk2)
		clif_updatestatus(sd,SP_ATK2);
	if(b_status.def2 != status->def2)
		clif_updatestatus(sd,SP_DEF2);
	if(b_status.flee2 != status->flee2)
		clif_updatestatus(sd,SP_FLEE2);
	if(b_status.cri != status->cri)
		clif_updatestatus(sd,SP_CRITICAL);
	if(b_status.matk_max != status->matk_max)
		clif_updatestatus(sd,SP_MATK1);
	if(b_status.matk_min != status->matk_min)
		clif_updatestatus(sd,SP_MATK2);
	if(b_status.mdef != status->mdef)
		clif_updatestatus(sd,SP_MDEF1);
	if(b_status.mdef2 != status->mdef2)
		clif_updatestatus(sd,SP_MDEF2);
	if(b_status.rhw.range != status->rhw.range)
		clif_updatestatus(sd,SP_ATTACKRANGE);
	if(b_status.max_hp != status->max_hp)
		clif_updatestatus(sd,SP_MAXHP);
	if(b_status.max_sp != status->max_sp)
		clif_updatestatus(sd,SP_MAXSP);
	if(b_status.hp != status->hp)
		clif_updatestatus(sd,SP_HP);
	if(b_status.sp != status->sp)
		clif_updatestatus(sd,SP_SP);

	calculating = 0;
	return 0;
}

static unsigned short status_calc_str(struct block_list *,struct status_change *,int);
static unsigned short status_calc_agi(struct block_list *,struct status_change *,int);
static unsigned short status_calc_vit(struct block_list *,struct status_change *,int);
static unsigned short status_calc_int(struct block_list *,struct status_change *,int);
static unsigned short status_calc_dex(struct block_list *,struct status_change *,int);
static unsigned short status_calc_luk(struct block_list *,struct status_change *,int);
static unsigned short status_calc_batk(struct block_list *,struct status_change *,int);
static unsigned short status_calc_watk(struct block_list *,struct status_change *,int);
static unsigned short status_calc_matk(struct block_list *,struct status_change *,int);
static unsigned short status_calc_hit(struct block_list *,struct status_change *,int);
static unsigned short status_calc_critical(struct block_list *,struct status_change *,int);
static unsigned short status_calc_flee(struct block_list *,struct status_change *,int);
static unsigned short status_calc_flee2(struct block_list *,struct status_change *,int);
static unsigned char status_calc_def(struct block_list *,struct status_change *,int);
static unsigned short status_calc_def2(struct block_list *,struct status_change *,int);
static unsigned char status_calc_mdef(struct block_list *,struct status_change *,int);
static unsigned short status_calc_mdef2(struct block_list *,struct status_change *,int);
static unsigned short status_calc_speed(struct block_list *,struct status_change *,int);
static short status_calc_aspd_rate(struct block_list *,struct status_change *,int);
static unsigned short status_calc_dmotion(struct block_list *bl, struct status_change *sc, int dmotion);
static unsigned int status_calc_maxhp(struct block_list *,struct status_change *,unsigned int);
static unsigned int status_calc_maxsp(struct block_list *,struct status_change *,unsigned int);
static unsigned char status_calc_element(struct block_list *bl, struct status_change *sc, int element);
static unsigned char status_calc_element_lv(struct block_list *bl, struct status_change *sc, int lv);
static unsigned short status_calc_mode(struct block_list *bl, struct status_change *sc, int mode);

//Calculates some attributes that depends on modified stats from status changes.
void status_calc_bl_sub_pc(struct map_session_data *sd, unsigned long flag)
{
	struct status_data *status = &sd->battle_status, *b_status = &sd->base_status;
	int skill;

	if(flag&(SCB_MAXHP|SCB_VIT))
	{
		flag|=SCB_MAXHP; //Ensures client-side refresh
		
		status->max_hp = status_base_pc_maxhp(sd,status);
		status->max_hp += b_status->max_hp - sd->status.max_hp;
		
		status->max_hp = status_calc_maxhp(&sd->bl, &sd->sc, status->max_hp);
		// Apply relative modifiers from equipment
		if(sd->hprate!=100)
			status->max_hp = status->max_hp * sd->hprate/100;
		if(battle_config.hp_rate != 100)
			status->max_hp = status->max_hp * battle_config.hp_rate/100;
		
		if(status->max_hp > (unsigned int)battle_config.max_hp)
			status->max_hp = battle_config.max_hp;
		else if(!status->max_hp)
			status->max_hp = 1;
	
		if(status->hp > status->max_hp) {
			status->hp = status->max_hp;
			clif_updatestatus(sd,SP_HP);
		}

		sd->nhealhp = 1 + (status->vit/5) + (status->max_hp/200);

		// Apply relative modifiers from equipment
		if(sd->hprecov_rate != 100)
			sd->nhealhp = sd->nhealhp*sd->hprecov_rate/100;

		if(sd->nhealhp < 1) sd->nhealhp = 1;
		else if(sd->nhealhp > SHRT_MAX) sd->nhealhp = SHRT_MAX;

		// Skill-related HP recovery
		if((skill=pc_checkskill(sd,SM_RECOVERY)) > 0)
			sd->nshealhp = skill*5 + (status->max_hp*skill/500);
		// Skill-related HP recovery (only when sit)
		if((skill=pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0)
			sd->nsshealhp = skill*4 + (status->max_hp*skill/500);
		if((skill=pc_checkskill(sd,TK_HPTIME)) > 0 && sd->state.rest)
			sd->nsshealhp = skill*30 + (status->max_hp*skill/500);

		if(sd->nshealhp > SHRT_MAX) sd->nshealhp = SHRT_MAX;
		if(sd->nsshealhp > SHRT_MAX) sd->nsshealhp = SHRT_MAX;
	}

	if(flag&(SCB_MAXSP|SCB_INT))
	{	
		flag|=SCB_MAXSP;
		
		status->max_sp = status_base_pc_maxsp(sd,status);
		status->max_sp += b_status->max_sp - sd->status.max_sp;
		
		if((skill=pc_checkskill(sd,HP_MEDITATIO))>0)
			status->max_sp += status->max_sp * skill/100;
		if((skill=pc_checkskill(sd,HW_SOULDRAIN))>0)
			status->max_sp += status->max_sp * 2*skill/100;

		status->max_sp = status_calc_maxsp(&sd->bl, &sd->sc, status->max_sp);
		
		// Apply relative modifiers from equipment
		if(sd->sprate!=100)
			status->max_sp = status->max_sp * sd->sprate/100;
		if(battle_config.sp_rate != 100)
			status->max_sp = status->max_sp * battle_config.sp_rate/100;
		
		if(status->max_sp > (unsigned int)battle_config.max_sp)
			status->max_sp = battle_config.max_sp;
		else if(!status->max_sp)
			status->max_sp = 1;
		
		if(status->sp > status->max_sp) {
			status->sp = status->max_sp;
			clif_updatestatus(sd,SP_SP);
		}

		sd->nhealsp = 1 + (status->int_/6) + (status->max_sp/100);
		if(status->int_ >= 120)
			sd->nhealsp += ((status->int_-120)>>1) + 4;

		// Relative modifiers from passive skills
		if((skill=pc_checkskill(sd,HP_MEDITATIO)) > 0)
			sd->nhealsp += sd->nhealsp * 3*skill/100;

		// Apply relative modifiers from equipment
		if(sd->sprecov_rate != 100)
			sd->nhealsp = sd->nhealsp*sd->sprecov_rate/100;

		if(sd->nhealsp > SHRT_MAX) sd->nhealsp = SHRT_MAX;
		else if(sd->nhealsp < 1) sd->nhealsp = 1;

		// Skill-related SP recovery
		if((skill=pc_checkskill(sd,MG_SRECOVERY)) > 0)
			sd->nshealsp = skill*3 + (status->max_sp*skill/500);
		if((skill=pc_checkskill(sd,NJ_NINPOU)) > 0)
			sd->nshealsp = skill*3 + (status->max_sp*skill/500);
		// Skill-related SP recovery (only when sit)
		if((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0)
			sd->nsshealsp = skill*2 + (status->max_sp*skill/500);
		if((skill=pc_checkskill(sd,TK_SPTIME)) > 0 && sd->state.rest)
		{
			sd->nsshealsp = skill*3 + (status->max_sp*skill/500);
			if ((skill=pc_checkskill(sd,SL_KAINA)) > 0) //Power up Enjoyable Rest
				sd->nsshealsp += (30+10*skill)*sd->nsshealsp/100;
		}
		if(sd->nshealsp > SHRT_MAX) sd->nshealsp = SHRT_MAX;
		if(sd->nsshealsp > SHRT_MAX) sd->nsshealsp = SHRT_MAX;
	}

	if(flag&SCB_MATK) {
		//New matk
		status->matk_min = status->int_+(status->int_/7)*(status->int_/7);
		status->matk_max = status->int_+(status->int_/5)*(status->int_/5);

		//Bonuses from previous matk
		status->matk_max += b_status->matk_max - (b_status->int_+(b_status->int_/5)*(b_status->int_/5));
		status->matk_min += b_status->matk_min - (b_status->int_+(b_status->int_/7)*(b_status->int_/7));

		status->matk_min = status_calc_matk(&sd->bl, &sd->sc, status->matk_min);
		status->matk_max = status_calc_matk(&sd->bl, &sd->sc, status->matk_max);
		if(sd->matk_rate != 100){
			status->matk_max = status->matk_max * sd->matk_rate/100;
			status->matk_min = status->matk_min * sd->matk_rate/100;
		}

		if(sd->sc.data[SC_MAGICPOWER].timer!=-1) { //Store current matk values
			sd->sc.data[SC_MAGICPOWER].val3 = status->matk_min;
			sd->sc.data[SC_MAGICPOWER].val4 = status->matk_max;
		}
	}
	
	if(flag&SCB_HIT) {
		if(sd->hit_rate != 100)
			status->hit = status->hit * sd->hit_rate/100;

		if(status->hit < 1) status->hit = 1;
	}

	if(flag&SCB_FLEE) {
		if(sd->flee_rate != 100)
			status->flee = status->flee * sd->flee_rate/100;

		if(status->flee < 1) status->flee = 1;
	}

	if(flag&SCB_DEF2) {
		if(sd->def2_rate != 100)
			status->def2 = status->def2 * sd->def2_rate/100;

		if(status->def2 < 1) status->def2 = 1;
	}

	if(flag&SCB_MDEF2) {
		if(sd->mdef2_rate != 100)
			status->mdef2 = status->mdef2 * sd->mdef2_rate/100;

		if(status->mdef2 < 1) status->mdef2 = 1;
	}

	if(flag&SCB_SPEED) {
		if(sd->speed_rate != 100)
			status->speed = status->speed*sd->speed_rate/100;

		if(status->speed < battle_config.max_walk_speed)
			status->speed = battle_config.max_walk_speed;

		if ((skill=pc_checkskill(sd,SA_FREECAST))>0) {
			//Store casting walk speed for quick restoration. [Skotlex]
			sd->prev_speed = status->speed * (175-5*skill)/100;
			if(sd->ud.skilltimer != -1) { //Swap speed.
				skill = status->speed;
				status->speed = sd->prev_speed;
				sd->prev_speed = skill;
			}
		}
	}

	if(flag&(SCB_ASPD|SCB_AGI|SCB_DEX)) {
		if (sd->status.weapon < MAX_WEAPON_TYPE)
			skill = aspd_base[sd->status.class_][sd->status.weapon]-(status->agi*4+status->dex)*aspd_base[sd->status.class_][sd->status.weapon]/1000;
		else
			skill = (
				(aspd_base[sd->status.class_][sd->weapontype1]
				-(status->agi*4+status->dex)*aspd_base[sd->status.class_][sd->weapontype1]/1000) +
				(aspd_base[sd->status.class_][sd->weapontype2]
				-(status->agi*4+status->dex)*aspd_base[sd->status.class_][sd->weapontype2]/1000)
				) *2/3; //From what I read in rodatazone, 2/3 should be more accurate than 0.7 -> 140 / 200; [Skotlex]

		status->aspd_rate = status_calc_aspd_rate(&sd->bl, &sd->sc , b_status->aspd_rate);
		
		// Apply all relative modifiers
		if(status->aspd_rate != 100)
			skill = skill *status->aspd_rate/100;

		status->amotion = cap_value(skill,battle_config.max_aspd,2000);

		status->adelay = 2*status->amotion;
		if ((skill=pc_checkskill(sd,SA_FREECAST))>0) {
			//Store casting adelay for quick restoration. [Skotlex]
			sd->prev_adelay = status->adelay*(150-5*skill)/100;
			if(sd->ud.skilltimer != -1) { //Swap adelay.
				skill = status->adelay;
				status->adelay = sd->prev_adelay;
				sd->prev_adelay = skill;
			}
		}

	}
	
	if(flag&(SCB_AGI|SCB_DSPD)) {
		//Even though people insist this is too slow, packet data reports this is the actual real equation.
		skill = 800-status->agi*4;
		status->dmotion = cap_value(skill, 400, 800);

		if(battle_config.pc_damage_delay_rate != 100)
			status->dmotion  = status->dmotion*battle_config.pc_damage_delay_rate/100;
		status->dmotion = status_calc_dmotion(&sd->bl, &sd->sc, b_status->dmotion);
	}


	if(flag&SCB_CRI)
	{
		if(sd->critical_rate != 100)
			status->cri = status->cri * sd->critical_rate/100;

		if(status->cri < 10) status->cri = 10;
	}

	if(flag&SCB_FLEE2) {
		if(sd->flee2_rate != 100)
			status->flee2 = status->flee2 * sd->flee2_rate/100;

		if(status->flee2 < 10) status->flee2 = 10;
	}
	if (flag == SCB_ALL)
		return; //Refresh is done on invoking function (status_calc_pc)

	if(flag&SCB_SPEED)
		clif_updatestatus(sd,SP_SPEED);
	if(flag&SCB_STR)
		clif_updatestatus(sd,SP_STR);
	if(flag&SCB_AGI)
		clif_updatestatus(sd,SP_AGI);
	if(flag&SCB_VIT)
		clif_updatestatus(sd,SP_VIT);
	if(flag&SCB_DEX)
		clif_updatestatus(sd,SP_DEX);
	if(flag&SCB_LUK)
		clif_updatestatus(sd,SP_LUK);
	if(flag&SCB_HIT)
		clif_updatestatus(sd,SP_HIT);
	if(flag&SCB_FLEE)
		clif_updatestatus(sd,SP_FLEE1);
	if(flag&SCB_ASPD)
		clif_updatestatus(sd,SP_ASPD);
	if(flag&(SCB_BATK|SCB_WATK))
		clif_updatestatus(sd,SP_ATK1);
	if(flag&SCB_DEF)
		clif_updatestatus(sd,SP_DEF1);
	if(flag&SCB_WATK)
		clif_updatestatus(sd,SP_ATK2);
	if(flag&SCB_DEF2)
		clif_updatestatus(sd,SP_DEF2);
	if(flag&SCB_FLEE2)
		clif_updatestatus(sd,SP_FLEE2);
	if(flag&SCB_CRI)
		clif_updatestatus(sd,SP_CRITICAL);
	if(flag&SCB_MATK) {
		clif_updatestatus(sd,SP_MATK1);
		clif_updatestatus(sd,SP_MATK2);
	}
	if(flag&SCB_MDEF)
		clif_updatestatus(sd,SP_MDEF1);
	if(flag&SCB_MDEF2)
		clif_updatestatus(sd,SP_MDEF2);
	if(flag&SCB_RANGE)
		clif_updatestatus(sd,SP_ATTACKRANGE);
	if(flag&SCB_MAXHP)
		clif_updatestatus(sd,SP_MAXHP);
	if(flag&SCB_MAXSP)
		clif_updatestatus(sd,SP_MAXSP);
}

void status_calc_bl(struct block_list *bl, unsigned long flag)
{
	struct status_data *b_status, *status;
	struct status_change *sc;
	int temp;
	TBL_PC *sd;
	b_status = status_get_base_status(bl);
	status = status_get_status_data(bl);
	sc = status_get_sc(bl);
	
	if (!b_status || !status)
		return;

	BL_CAST(BL_PC,bl,sd);

	if(sd && flag&SCB_PC)
	{	//Recalc everything.
		status_calc_pc(sd,0);
		return;
	}
	
	if(!sd && (!sc || !sc->count)) { //No difference.
		status_cpy(status, b_status);
		return;
	}
	
	if(flag&SCB_STR) {
		status->str = status_calc_str(bl, sc, b_status->str);
		flag|=SCB_BATK;
	}

	if(flag&SCB_AGI) {
		status->agi = status_calc_agi(bl, sc, b_status->agi);
		flag|=SCB_FLEE;
	}

	if(flag&SCB_VIT) {
		status->vit = status_calc_vit(bl, sc, b_status->vit);
		flag|=SCB_DEF2|SCB_MDEF2;
	}

	if(flag&SCB_INT) {
		status->int_ = status_calc_int(bl, sc, b_status->int_);
		flag|=SCB_MATK|SCB_MDEF2;
	}

	if(flag&SCB_DEX) {
		status->dex = status_calc_dex(bl, sc, b_status->dex);
		flag|=SCB_BATK|SCB_HIT;
	}

	if(flag&SCB_LUK) {
		status->luk = status_calc_luk(bl, sc, b_status->luk);
		flag|=SCB_CRI|SCB_FLEE2;
	}

	if(flag&SCB_BATK && b_status->batk) {
		status->batk = status_base_atk(bl,status);
		temp = b_status->batk - status_base_atk(bl,b_status);
		if (temp)
			status->batk += temp;
		status->batk = status_calc_batk(bl, sc, status->batk);
	}

	if(flag&SCB_WATK) {
		status->rhw.atk = status_calc_watk(bl, sc, b_status->rhw.atk);
		status->rhw.atk2 = status_calc_watk(bl, sc, b_status->rhw.atk2);
		if(status->lhw && b_status->lhw && b_status->lhw->atk) {
			if (sd) sd->state.lr_flag = 1;
			status->lhw->atk = status_calc_watk(bl, sc, b_status->lhw->atk);
			status->lhw->atk2 = status_calc_watk(bl, sc, b_status->lhw->atk2);
			if (sd) sd->state.lr_flag = 0;
		}
	}

	if(flag&SCB_HIT) {
		status->hit = b_status->hit - b_status->dex + status->dex;
		status->hit = status_calc_hit(bl, sc, status->hit);
	}

	if(flag&SCB_FLEE) {
		status->flee = b_status->flee - b_status->agi + status->agi;
		status->flee = status_calc_flee(bl, sc, status->flee);
	}

	if(flag&SCB_DEF)
		status->def = status_calc_def(bl, sc, b_status->def);

	if(flag&SCB_DEF2) {
		status->def2 = status->vit;
		temp = b_status->def2 - b_status->vit;
		if (temp)
			status->def2+=temp;
		status->def2 = status_calc_def2(bl, sc, status->def2);
	}

	if(flag&SCB_MDEF)
		status->mdef = status_calc_mdef(bl, sc, b_status->mdef);
		
	if(flag&SCB_MDEF2) {
		status->mdef2 = status->int_ + (status->vit>>1);
		temp = b_status->mdef2 -(b_status->int_ + (b_status->vit>>1));
		if (temp)
			status->mdef2+=temp;
		status->mdef2 = status_calc_mdef2(bl, sc, status->mdef2);
	}

	if(flag&SCB_SPEED)
		status->speed = status_calc_speed(bl, sc, b_status->speed);

	if(flag&SCB_CRI && b_status->cri) {
		status->cri = status->luk*3 + 10;
		temp = b_status->cri - (b_status->luk*3 + 10);
		if (temp)
			status->cri += temp;
		status->cri = status_calc_critical(bl, sc, status->cri);
	}

	if(flag&SCB_FLEE2 && b_status->flee2) {
		status->flee2 = status->luk + 10;
		temp = b_status->flee2 - b_status->flee2 + 10;
		if (temp)
			status->flee2 += temp;
		status->flee2 = status_calc_flee2(bl, sc, status->flee2);
	}

	if(flag&SCB_ATK_ELE) {
		status->rhw.ele = status_calc_attack_element(bl, sc, b_status->rhw.ele);
		if(status->lhw && b_status->lhw) {
			if (sd) sd->state.lr_flag = 1;
			status->lhw->ele = status_calc_attack_element(bl, sc, b_status->lhw->ele);
			if (sd) sd->state.lr_flag = 0;
		}
	}

	if(flag&SCB_DEF_ELE) {
		status->def_ele = status_calc_element(bl, sc, b_status->def_ele);
		status->ele_lv = status_calc_element_lv(bl, sc, b_status->ele_lv);
	}

	if(flag&SCB_MODE)
	{
		status->mode = status_calc_mode(bl, sc, b_status->mode);
		//Since mode changed, reset their state.
		unit_stop_attack(bl);
		unit_stop_walking(bl,0);
	}

// No status changes alter these yet.
//	if(flag&SCB_SIZE)
// if(flag&SCB_RACE)
// if(flag&SCB_RANGE)

	if(sd) {
		//The remaining are handled quite different by players, so use their own function.
		status_calc_bl_sub_pc(sd, flag);
		return;
	}
	
	if(flag&SCB_MAXHP) {
		status->max_hp = status_calc_maxhp(bl, sc, b_status->max_hp);
		if (status->hp > status->max_hp) //FIXME: Should perhaps a status_zap should be issued?
			status->hp = status->max_hp;
	}

	if(flag&SCB_MAXSP) {
		status->max_sp = status_calc_maxsp(bl, sc, b_status->max_sp);
		if (status->sp > status->max_sp)
			status->sp = status->max_sp;
	}

	if(flag&SCB_MATK) {
		status->matk_min = status->int_+(status->int_/7)*(status->int_/7);
		status->matk_max = status->int_+(status->int_/5)*(status->int_/5);
		status->matk_min = status_calc_matk(bl, sc, status->matk_min);
		status->matk_max = status_calc_matk(bl, sc, status->matk_max);
		if(sc->data[SC_MAGICPOWER].timer!=-1) { //Store current matk values
			sc->data[SC_MAGICPOWER].val3 = status->matk_min;
			sc->data[SC_MAGICPOWER].val4 = status->matk_max;
		}
	}
	
	if(flag&SCB_ASPD) {
		status->aspd_rate = status_calc_aspd_rate(bl, sc , b_status->aspd_rate);
		temp = status->aspd_rate*b_status->amotion/100;
		status->amotion = cap_value(temp, battle_config.monster_max_aspd, 2000);
		
		temp = status->aspd_rate*b_status->adelay/100;
		status->adelay = cap_value(temp, battle_config.monster_max_aspd<<1, 4000);
	}

	if(flag&SCB_DSPD)
		status->dmotion = status_calc_dmotion(bl, sc, b_status->dmotion);
}
/*==========================================
 * Apply shared stat mods from status changes [DracoRPG]
 *------------------------------------------
 */
static unsigned short status_calc_str(struct block_list *bl, struct status_change *sc, int str)
{
	if(!sc || !sc->count)
		return str;
	
	if(sc->data[SC_INCALLSTATUS].timer!=-1)
		str += sc->data[SC_INCALLSTATUS].val1;
	if(sc->data[SC_INCSTR].timer!=-1)
		str += sc->data[SC_INCSTR].val1;
	if(sc->data[SC_STRFOOD].timer!=-1)
		str += sc->data[SC_STRFOOD].val1;
	if(sc->data[SC_BATTLEORDERS].timer!=-1)
		str += 5;
	if(sc->data[SC_GUILDAURA].timer != -1 && ((sc->data[SC_GUILDAURA].val4>>12)&0xF))
		str += (sc->data[SC_GUILDAURA].val4>>12)&0xF;
	if(sc->data[SC_LOUD].timer!=-1)
		str += 4;
	if(sc->data[SC_TRUESIGHT].timer!=-1)
		str += 5;
	if(sc->data[SC_SPURT].timer!=-1)
		str += 10;
	if(sc->data[SC_NEN].timer!=-1)
		str += sc->data[SC_NEN].val1;
	if(sc->data[SC_BLESSING].timer != -1){
		if(sc->data[SC_BLESSING].val2)
			str += sc->data[SC_BLESSING].val2;
		else
			str >>= 1;
	}
	if(sc->data[SC_MARIONETTE].timer!=-1)
		str -= (sc->data[SC_MARIONETTE].val3>>16)&0xFF;
	if(sc->data[SC_MARIONETTE2].timer!=-1)
		str += (sc->data[SC_MARIONETTE2].val3>>16)&0xFF;
	if(sc->data[SC_SPIRIT].timer!=-1 && sc->data[SC_SPIRIT].val2 == SL_HIGH && str < 50)
		str = 50;

	return cap_value(str,1,USHRT_MAX);
}

static unsigned short status_calc_agi(struct block_list *bl, struct status_change *sc, int agi)
{
	if(!sc || !sc->count)
		return agi;

	if(sc->data[SC_CONCENTRATE].timer!=-1 && sc->data[SC_QUAGMIRE].timer == -1)
		agi += (agi-sc->data[SC_CONCENTRATE].val3)*sc->data[SC_CONCENTRATE].val2/100;
	if(sc->data[SC_INCALLSTATUS].timer!=-1)
		agi += sc->data[SC_INCALLSTATUS].val1;
	if(sc->data[SC_INCAGI].timer!=-1)
		agi += sc->data[SC_INCAGI].val1;
	if(sc->data[SC_AGIFOOD].timer!=-1)
		agi += sc->data[SC_AGIFOOD].val1;
	if(sc->data[SC_GUILDAURA].timer != -1 && ((sc->data[SC_GUILDAURA].val4>>4)&0xF))
		agi += (sc->data[SC_GUILDAURA].val4>>4)&0xF;
	if(sc->data[SC_TRUESIGHT].timer!=-1)
		agi += 5;
	if(sc->data[SC_INCREASEAGI].timer!=-1)
		agi += 2 + sc->data[SC_INCREASEAGI].val1;
	if(sc->data[SC_INCREASING].timer!=-1)
		agi += 4;	// added based on skill updates [Reddozen]
	if(sc->data[SC_DECREASEAGI].timer!=-1)
		agi -= 2 + sc->data[SC_DECREASEAGI].val1;
	if(sc->data[SC_QUAGMIRE].timer!=-1)
		agi -= sc->data[SC_QUAGMIRE].val2;
	if(sc->data[SC_SUITON].timer!=-1)
		agi -= sc->data[SC_SUITON].val2;
	if(sc->data[SC_MARIONETTE].timer!=-1)
		agi -= (sc->data[SC_MARIONETTE].val3>>8)&0xFF;
	if(sc->data[SC_MARIONETTE2].timer!=-1)
		agi += (sc->data[SC_MARIONETTE2].val3>>8)&0xFF;
	if(sc->data[SC_SPIRIT].timer!=-1 && sc->data[SC_SPIRIT].val2 == SL_HIGH && agi < 50)
		agi = 50;

	return cap_value(agi,1,USHRT_MAX);
}

static unsigned short status_calc_vit(struct block_list *bl, struct status_change *sc, int vit)
{
	if(!sc || !sc->count)
		return vit;

	if(sc->data[SC_INCALLSTATUS].timer!=-1)
		vit += sc->data[SC_INCALLSTATUS].val1;
	if(sc->data[SC_INCVIT].timer!=-1)
		vit += sc->data[SC_INCVIT].val1;
	if(sc->data[SC_VITFOOD].timer!=-1)
		vit += sc->data[SC_VITFOOD].val1;
	if(sc->data[SC_GUILDAURA].timer != -1 && ((sc->data[SC_GUILDAURA].val4>>8)&0xF))
		vit += (sc->data[SC_GUILDAURA].val4>>8)&0xF;
	if(sc->data[SC_TRUESIGHT].timer!=-1)
		vit += 5;
	if(sc->data[SC_STRIPARMOR].timer!=-1)
		vit -= vit * sc->data[SC_STRIPARMOR].val2/100;
	if(sc->data[SC_MARIONETTE].timer!=-1)
		vit -= sc->data[SC_MARIONETTE].val3&0xFF;
	if(sc->data[SC_MARIONETTE2].timer!=-1)
		vit += sc->data[SC_MARIONETTE2].val3&0xFF;
	if(sc->data[SC_SPIRIT].timer!=-1 && sc->data[SC_SPIRIT].val2 == SL_HIGH && vit < 50)
		vit = 50;

	return cap_value(vit,1,USHRT_MAX);
}

static unsigned short status_calc_int(struct block_list *bl, struct status_change *sc, int int_)
{
	if(!sc || !sc->count)
		return int_;

	if(sc->data[SC_INCALLSTATUS].timer!=-1)
		int_ += sc->data[SC_INCALLSTATUS].val1;
	if(sc->data[SC_INCINT].timer!=-1)
		int_ += sc->data[SC_INCINT].val1;
	if(sc->data[SC_INTFOOD].timer!=-1)
		int_ += sc->data[SC_INTFOOD].val1;
	if(sc->data[SC_BATTLEORDERS].timer!=-1)
		int_ += 5;
	if(sc->data[SC_TRUESIGHT].timer!=-1)
		int_ += 5;
	if(sc->data[SC_BLESSING].timer != -1){
		if (sc->data[SC_BLESSING].val2)
			int_ += sc->data[SC_BLESSING].val2;
		else
			int_ >>= 1;
	}
	if(sc->data[SC_STRIPHELM].timer!=-1)
		int_ -= int_ * sc->data[SC_STRIPHELM].val2/100;
	if(sc->data[SC_NEN].timer!=-1)
		int_ += sc->data[SC_NEN].val1;
	if(sc->data[SC_CHANGE].timer!=-1)
		int_ += 60;
	if(sc->data[SC_MARIONETTE].timer!=-1)
		int_ -= (sc->data[SC_MARIONETTE].val4>>16)&0xFF;
	if(sc->data[SC_MARIONETTE2].timer!=-1)
		int_ += (sc->data[SC_MARIONETTE2].val4>>16)&0xFF;
	if(sc->data[SC_SPIRIT].timer!=-1 && sc->data[SC_SPIRIT].val2 == SL_HIGH && int_ < 50)
		int_ = 50;

	return cap_value(int_,1,USHRT_MAX);
}

static unsigned short status_calc_dex(struct block_list *bl, struct status_change *sc, int dex)
{
	if(!sc || !sc->count)
		return dex;

	if(sc->data[SC_CONCENTRATE].timer!=-1 && sc->data[SC_QUAGMIRE].timer == -1)
		dex += (dex-sc->data[SC_CONCENTRATE].val4)*sc->data[SC_CONCENTRATE].val2/100;
	if(sc->data[SC_INCALLSTATUS].timer!=-1)
		dex += sc->data[SC_INCALLSTATUS].val1;
	if(sc->data[SC_INCDEX].timer!=-1)
		dex += sc->data[SC_INCDEX].val1;
	if(sc->data[SC_DEXFOOD].timer!=-1)
		dex += sc->data[SC_DEXFOOD].val1;
	if(sc->data[SC_BATTLEORDERS].timer!=-1)
		dex += 5;
	if(sc->data[SC_GUILDAURA].timer != -1 && (sc->data[SC_GUILDAURA].val4&0xF))
		dex += sc->data[SC_GUILDAURA].val4&0xF;
	if(sc->data[SC_TRUESIGHT].timer!=-1)
		dex += 5;
	if(sc->data[SC_QUAGMIRE].timer!=-1)
		dex -= sc->data[SC_QUAGMIRE].val2;
	if(sc->data[SC_BLESSING].timer != -1){
		if (sc->data[SC_BLESSING].val2)
			dex += sc->data[SC_BLESSING].val2;
		else
			dex >>= 1;
	}
	if(sc->data[SC_INCREASING].timer!=-1)
		dex += 4;	// added based on skill updates [Reddozen]
	if(sc->data[SC_MARIONETTE].timer!=-1)
		dex -= (sc->data[SC_MARIONETTE].val4>>8)&0xFF;
	if(sc->data[SC_MARIONETTE2].timer!=-1)
		dex += (sc->data[SC_MARIONETTE2].val4>>8)&0xFF;
	if(sc->data[SC_SPIRIT].timer!=-1 && sc->data[SC_SPIRIT].val2 == SL_HIGH && dex < 50)
		dex  = 50;

	return cap_value(dex,1,USHRT_MAX);
}

static unsigned short status_calc_luk(struct block_list *bl, struct status_change *sc, int luk)
{
	if(!sc || !sc->count)
		return luk;

	if(sc->data[SC_CURSE].timer!=-1)
		return 0;
	if(sc->data[SC_INCALLSTATUS].timer!=-1)
		luk += sc->data[SC_INCALLSTATUS].val1;
	if(sc->data[SC_INCLUK].timer!=-1)
		luk += sc->data[SC_INCLUK].val1;
	if(sc->data[SC_LUKFOOD].timer!=-1)
		luk += sc->data[SC_LUKFOOD].val1;
	if(sc->data[SC_TRUESIGHT].timer!=-1)
		luk += 5;
	if(sc->data[SC_GLORIA].timer!=-1)
		luk += 30;
	if(sc->data[SC_MARIONETTE].timer!=-1)
		luk -= sc->data[SC_MARIONETTE].val4&0xFF;
	if(sc->data[SC_MARIONETTE2].timer!=-1)
		luk += sc->data[SC_MARIONETTE2].val4&0xFF;
	if(sc->data[SC_SPIRIT].timer!=-1 && sc->data[SC_SPIRIT].val2 == SL_HIGH && luk < 50)
		luk = 50;

	return cap_value(luk,1,USHRT_MAX);
}

static unsigned short status_calc_batk(struct block_list *bl, struct status_change *sc, int batk)
{
	if(!sc || !sc->count)
		return batk;

	if(sc->data[SC_ATKPOTION].timer!=-1)
		batk += sc->data[SC_ATKPOTION].val1;
	if(sc->data[SC_BATKFOOD].timer!=-1)
		batk += sc->data[SC_BATKFOOD].val1;
	if(sc->data[SC_INCATKRATE].timer!=-1)
		batk += batk * sc->data[SC_INCATKRATE].val1/100;
	if(sc->data[SC_PROVOKE].timer!=-1)
		batk += batk * sc->data[SC_PROVOKE].val3/100;
	if(sc->data[SC_CONCENTRATION].timer!=-1)
		batk += batk * sc->data[SC_CONCENTRATION].val2/100;
	if(sc->data[SC_SKE].timer!=-1)
		batk += batk * 3;
	if(sc->data[SC_BLOODLUST].timer!=-1)
		batk += batk * sc->data[SC_BLOODLUST].val2/100;
	if(sc->data[SC_FLEET].timer!=-1)
		batk += batk * sc->data[SC_FLEET].val3/100;
	if(sc->data[SC_JOINTBEAT].timer!=-1 && sc->data[SC_JOINTBEAT].val2==4)
		batk -= batk * 25/100;
	if(sc->data[SC_CURSE].timer!=-1)
		batk -= batk * 25/100;
//Curse shouldn't effect on this?  <- Curse OR Bleeding??
//	if(sc->data[SC_BLEEDING].timer != -1)
//		batk -= batk * 25/100;
	if(sc->data[SC_MADNESSCANCEL].timer!=-1)
		batk += 100;
	return cap_value(batk,0,USHRT_MAX);
}

static unsigned short status_calc_watk(struct block_list *bl, struct status_change *sc, int watk)
{
	if(!sc || !sc->count)
		return watk;

	if(sc->data[SC_IMPOSITIO].timer!=-1)
		watk += sc->data[SC_IMPOSITIO].val2;
	if(sc->data[SC_WATKFOOD].timer!=-1)
		watk += sc->data[SC_WATKFOOD].val1;
	if(sc->data[SC_DRUMBATTLE].timer!=-1)
		watk += sc->data[SC_DRUMBATTLE].val2;
	if(sc->data[SC_VOLCANO].timer!=-1)
		watk += sc->data[SC_VOLCANO].val2;
	if(sc->data[SC_INCATKRATE].timer!=-1)
		watk += watk * sc->data[SC_INCATKRATE].val1/100;
	if(sc->data[SC_PROVOKE].timer!=-1)
		watk += watk * sc->data[SC_PROVOKE].val3/100;
	if(sc->data[SC_CONCENTRATION].timer!=-1)
		watk += watk * sc->data[SC_CONCENTRATION].val2/100;
	if(sc->data[SC_SKE].timer!=-1)
		watk += watk * 3;
	if(sc->data[SC_NIBELUNGEN].timer!=-1) {
		if (bl->type != BL_PC)
			watk += sc->data[SC_NIBELUNGEN].val2;
		else {
			TBL_PC *sd = (TBL_PC*)bl;
			int index = sd->equip_index[sd->state.lr_flag?8:9];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->wlv == 4)
				watk += sc->data[SC_NIBELUNGEN].val2;
		}
	}
	if(sc->data[SC_BLOODLUST].timer!=-1)
		watk += watk * sc->data[SC_BLOODLUST].val2/100;
	if(sc->data[SC_FLEET].timer!=-1)
		watk += watk * sc->data[SC_FLEET].val3/100;
	if(sc->data[SC_CURSE].timer!=-1)
		watk -= watk * 25/100;
	if(sc->data[SC_STRIPWEAPON].timer!=-1)
		watk -= watk * sc->data[SC_STRIPWEAPON].val2/100;
	return cap_value(watk,0,USHRT_MAX);
}

static unsigned short status_calc_matk(struct block_list *bl, struct status_change *sc, int matk)
{
	if(!sc || !sc->count)
		return matk;

	if(sc->data[SC_MATKPOTION].timer!=-1)
		matk += sc->data[SC_MATKPOTION].val1;
	if(sc->data[SC_MATKFOOD].timer!=-1)
		matk += sc->data[SC_MATKFOOD].val1;
	if(sc->data[SC_MAGICPOWER].timer!=-1)
		matk += matk * 5*sc->data[SC_MAGICPOWER].val1/100;
	if(sc->data[SC_MINDBREAKER].timer!=-1)
		matk += matk * 20*sc->data[SC_MINDBREAKER].val1/100;
	if(sc->data[SC_INCMATKRATE].timer!=-1)
		matk += matk * sc->data[SC_INCMATKRATE].val1/100;

	return cap_value(matk,0,USHRT_MAX);
}

static unsigned short status_calc_critical(struct block_list *bl, struct status_change *sc, int critical)
{
	if(!sc || !sc->count)
		return critical;

	if (sc->data[SC_EXPLOSIONSPIRITS].timer!=-1)
		critical += sc->data[SC_EXPLOSIONSPIRITS].val2;
	if (sc->data[SC_FORTUNE].timer!=-1)
		critical += sc->data[SC_FORTUNE].val2;
	if (sc->data[SC_TRUESIGHT].timer!=-1)
		critical += sc->data[SC_TRUESIGHT].val2;
	if(sc->data[SC_CLOAKING].timer!=-1)
		critical += critical;

	return cap_value(critical,0,USHRT_MAX);
}

static unsigned short status_calc_hit(struct block_list *bl, struct status_change *sc, int hit)
{
	
	if(!sc || !sc->count)
		return hit;

	if(sc->data[SC_INCHIT].timer != -1)
		hit += sc->data[SC_INCHIT].val1;
	if(sc->data[SC_HITFOOD].timer!=-1)
		hit += sc->data[SC_HITFOOD].val1;
	if(sc->data[SC_TRUESIGHT].timer != -1)
		hit += sc->data[SC_TRUESIGHT].val3;
	if(sc->data[SC_HUMMING].timer!=-1)
		hit += sc->data[SC_HUMMING].val2;
	if(sc->data[SC_CONCENTRATION].timer != -1)
		hit += sc->data[SC_CONCENTRATION].val3;
	if(sc->data[SC_INCHITRATE].timer != -1)
		hit += hit * sc->data[SC_INCHITRATE].val1/100;
	if(sc->data[SC_BLIND].timer != -1)
		hit -= hit * 25/100;
	if(sc->data[SC_ADJUSTMENT].timer!=-1)
		hit += 30;
	if(sc->data[SC_INCREASING].timer!=-1)
		hit += 20; // RockmanEXE; changed based on updated [Reddozen]
	
	return cap_value(hit,0,USHRT_MAX);
}

static unsigned short status_calc_flee(struct block_list *bl, struct status_change *sc, int flee)
{
	if (bl->type == BL_PC && map_flag_gvg(bl->m)) //GVG grounds flee penalty, placed here because it's "like" a status change. [Skotlex]
		flee -= flee * battle_config.gvg_flee_penalty/100;

	if(!sc || !sc->count)
		return flee;
	if(sc->data[SC_INCFLEE].timer!=-1)
		flee += sc->data[SC_INCFLEE].val1;
	if(sc->data[SC_FLEEFOOD].timer!=-1)
		flee += sc->data[SC_FLEEFOOD].val1;
	if(sc->data[SC_WHISTLE].timer!=-1)
		flee += sc->data[SC_WHISTLE].val2;
	if(sc->data[SC_WINDWALK].timer!=-1)
		flee += sc->data[SC_WINDWALK].val2;
	if(sc->data[SC_INCFLEERATE].timer!=-1)
		flee += flee * sc->data[SC_INCFLEERATE].val1/100;
	if(sc->data[SC_VIOLENTGALE].timer!=-1)
		flee += flee * sc->data[SC_VIOLENTGALE].val2/100;
	if(sc->data[SC_MOON_COMFORT].timer!=-1) //SG skill [Komurka]
		flee += sc->data[SC_MOON_COMFORT].val2;
	if(sc->data[SC_CLOSECONFINE].timer!=-1)
		flee += 10;
	if(sc->data[SC_SPIDERWEB].timer!=-1)
		flee -= flee * 50/100;
	if(sc->data[SC_BERSERK].timer!=-1)
		flee -= flee * 50/100;
	if(sc->data[SC_BLIND].timer!=-1)
		flee -= flee * 25/100;
	if(sc->data[SC_ADJUSTMENT].timer!=-1)
		flee += 30;
	if(sc->data[SC_GATLINGFEVER].timer!=-1)
		flee -= sc->data[SC_GATLINGFEVER].val1*5;

	return cap_value(flee,0,USHRT_MAX);
}

static unsigned short status_calc_flee2(struct block_list *bl, struct status_change *sc, int flee2)
{
	if(!sc || !sc->count)
		return flee2;
	if(sc->data[SC_WHISTLE].timer!=-1)
		flee2 += sc->data[SC_WHISTLE].val3*10;
	return cap_value(flee2,0,USHRT_MAX);
}

static unsigned char status_calc_def(struct block_list *bl, struct status_change *sc, int def)
{
	if(!sc || !sc->count)
		return def;
	if(sc->data[SC_BERSERK].timer!=-1)
		return 0;
	if(sc->data[SC_KEEPING].timer!=-1)
		return 100;
	if(sc->data[SC_SKA].timer != -1)
		return rand()%100; //Reports indicate SKA actually randomizes defense.
	if(sc->data[SC_STEELBODY].timer!=-1)
		return 90;
	if(sc->data[SC_DRUMBATTLE].timer!=-1)
		def += sc->data[SC_DRUMBATTLE].val3;
	if(sc->data[SC_INCDEFRATE].timer!=-1)
		def += def * sc->data[SC_INCDEFRATE].val1/100;
	if(sc->data[SC_SIGNUMCRUCIS].timer!=-1)
		def -= def * sc->data[SC_SIGNUMCRUCIS].val2/100;
	if(sc->data[SC_CONCENTRATION].timer!=-1)
		def -= def * sc->data[SC_CONCENTRATION].val4/100;
	if(sc->data[SC_SKE].timer!=-1)
		def -= def * 50/100;
	if(sc->data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC) // Provoke doesn't alter player defense.
		def -= def * sc->data[SC_PROVOKE].val4/100;
	if(sc->data[SC_STRIPSHIELD].timer!=-1)
		def -= def * sc->data[SC_STRIPSHIELD].val2/100;
	if (sc->data[SC_FLING].timer!=-1)
		def -= def * (sc->data[SC_FLING].val2)/100;
	return cap_value(def,0,UCHAR_MAX);
}

static unsigned short status_calc_def2(struct block_list *bl, struct status_change *sc, int def2)
{
	if(!sc || !sc->count)
		return def2;
	
	if(sc->data[SC_BERSERK].timer!=-1)
		return 0;
	if(sc->data[SC_ETERNALCHAOS].timer!=-1)
		return 0;
	if(sc->data[SC_SUN_COMFORT].timer!=-1)
		def2 += sc->data[SC_SUN_COMFORT].val2;
	if(sc->data[SC_ANGELUS].timer!=-1)
		def2 += def2 * sc->data[SC_ANGELUS].val2/100;
	if(sc->data[SC_CONCENTRATION].timer!=-1)
		def2 -= def2 * sc->data[SC_CONCENTRATION].val4/100;
	if(sc->data[SC_POISON].timer!=-1)
		def2 -= def2 * 25/100;
	if(sc->data[SC_SKE].timer!=-1)
		def2 -= def2 * 50/100;
	if(sc->data[SC_PROVOKE].timer!=-1)
		def2 -= def2 * sc->data[SC_PROVOKE].val4/100;
	if(sc->data[SC_JOINTBEAT].timer!=-1){
		if(sc->data[SC_JOINTBEAT].val2==3)
			def2 -= def2 * 50/100;
		else if(sc->data[SC_JOINTBEAT].val2==4)
			def2 -= def2 * 25/100;
	}
	if(sc->data[SC_FLING].timer!=-1)
		def2 -= def2 * (sc->data[SC_FLING].val3)/100;

	return cap_value(def2,0,USHRT_MAX);
}

static unsigned char status_calc_mdef(struct block_list *bl, struct status_change *sc, int mdef)
{
	if(!sc || !sc->count)
		return mdef;

	if(sc->data[SC_BERSERK].timer!=-1)
		return 0;
	if(sc->data[SC_BARRIER].timer!=-1)
		return 100;
	if(sc->data[SC_STEELBODY].timer!=-1)
		return 90;
	if(sc->data[SC_SKA].timer != -1) // [marquis007]
		return 90;
	if(sc->data[SC_ENDURE].timer!=-1 && sc->data[SC_ENDURE].val4 == 0)
		mdef += sc->data[SC_ENDURE].val1;

	return cap_value(mdef,0,UCHAR_MAX);
}

static unsigned short status_calc_mdef2(struct block_list *bl, struct status_change *sc, int mdef2)
{
	if(!sc || !sc->count)
		return mdef2;
	if(sc->data[SC_BERSERK].timer!=-1)
		return 0;
	if(sc->data[SC_MINDBREAKER].timer!=-1)
		mdef2 -= mdef2 * 12*sc->data[SC_MINDBREAKER].val1/100;

	return cap_value(mdef2,0,USHRT_MAX);
}

static unsigned short status_calc_speed(struct block_list *bl, struct status_change *sc, int speed)
{
	if(!sc || !sc->count)
		return speed;
	if(sc->data[SC_CURSE].timer!=-1)
		speed += 450;
	if(sc->data[SC_SWOO].timer != -1) // [marquis007]
		speed += 450; //Let's use Curse's slow down momentarily (exact value unknown)
	if(sc->data[SC_WEDDING].timer!=-1)
		speed += 300;
	if(sc->data[SC_SPEEDUP1].timer!=-1)
		speed -= speed*50/100;
	else if(sc->data[SC_SPEEDUP0].timer!=-1)
		speed -= speed*25/100;
	else if(sc->data[SC_INCREASEAGI].timer!=-1)
		speed -= speed * 25/100;
	else if(sc->data[SC_CARTBOOST].timer!=-1)
		speed -= speed * 20/100;
	else if(sc->data[SC_BERSERK].timer!=-1)
		speed -= speed * 20/100;
	else if(sc->data[SC_AVOID].timer!=-1)
		speed -= speed * sc->data[SC_AVOID].val2/100;
	else if(sc->data[SC_WINDWALK].timer!=-1)
		speed -= speed * sc->data[SC_WINDWALK].val3/100;
	if(sc->data[SC_SLOWDOWN].timer!=-1)
		speed += speed * 50/100;
	if(sc->data[SC_DECREASEAGI].timer!=-1)
		speed += speed * 25/100;
	if(sc->data[SC_STEELBODY].timer!=-1)
		speed += speed * 25/100;
	if(sc->data[SC_QUAGMIRE].timer!=-1)
		speed += speed * 50/100;
	if(sc->data[SC_DONTFORGETME].timer!=-1)
		speed += speed * sc->data[SC_DONTFORGETME].val3/100;
	if(sc->data[SC_DEFENDER].timer!=-1)
		speed += speed * (35-5*sc->data[SC_DEFENDER].val1)/100;
	if(sc->data[SC_GOSPEL].timer!=-1 && sc->data[SC_GOSPEL].val4 == BCT_ENEMY)
		speed += speed * 25/100;
	if(sc->data[SC_JOINTBEAT].timer!=-1) {
		if (sc->data[SC_JOINTBEAT].val2 == 0)
			speed += speed * 50/100;
		else if (sc->data[SC_JOINTBEAT].val2 == 2)
			speed += speed * 30/100;
	}
	if(sc->data[SC_CLOAKING].timer!=-1)
		speed = speed*(
			(sc->data[SC_CLOAKING].val4&2?25:0) //Wall speed bonus
			+sc->data[SC_CLOAKING].val3) /100; //Normal adjustment bonus.
	
	if(sc->data[SC_DANCING].timer!=-1 && sc->data[SC_DANCING].val3&0xFFFF)
		speed += speed*(sc->data[SC_DANCING].val3&0xFFFF)/100;
	if(sc->data[SC_LONGING].timer!=-1)
		speed += speed*sc->data[SC_LONGING].val2/100;
	if(sc->data[SC_HIDING].timer!=-1 && sc->data[SC_HIDING].val3)
		speed += speed*sc->data[SC_HIDING].val3/100;
	if(sc->data[SC_CHASEWALK].timer!=-1)
		speed = speed * sc->data[SC_CHASEWALK].val3/100;
	if(sc->data[SC_RUN].timer!=-1)
		speed -= speed * 25/100;
	if(sc->data[SC_FUSION].timer != -1)
		speed -= speed * 25/100;
	if(sc->data[SC_GATLINGFEVER].timer!=-1)
		speed += speed * 25/100;
	
	return cap_value(speed,0,USHRT_MAX);
}

static short status_calc_aspd_rate(struct block_list *bl, struct status_change *sc, int aspd_rate)
{
	int i;
	if(!sc || !sc->count)
		return aspd_rate;

	if(sc->data[SC_QUAGMIRE].timer==-1 && sc->data[SC_DONTFORGETME].timer==-1)
	{
		int max = 0;
		if(sc->data[SC_STAR_COMFORT].timer!=-1)
			max = sc->data[SC_STAR_COMFORT].val2;
		if((sc->data[SC_TWOHANDQUICKEN].timer!=-1 ||
			sc->data[SC_ONEHAND].timer!=-1 ||
			sc->data[SC_BERSERK].timer!=-1
			) && max < 30)
			max = 30;
		
		if(sc->data[SC_MADNESSCANCEL].timer!=-1 && max < 20)
			max = 20;
		
		if(sc->data[SC_ADRENALINE2].timer!=-1 &&
			max < sc->data[SC_ADRENALINE2].val2)
			max = sc->data[SC_ADRENALINE2].val2;
		
		if(sc->data[SC_ADRENALINE].timer!=-1 &&
			max < sc->data[SC_ADRENALINE].val2)
			max = sc->data[SC_ADRENALINE].val2;
		
		if(sc->data[SC_SPEARQUICKEN].timer!=-1 &&
			max < sc->data[SC_SPEARQUICKEN].val2)
			max = sc->data[SC_SPEARQUICKEN].val2;

		if(sc->data[SC_GATLINGFEVER].timer!=-1 &&
			max < sc->data[SC_GATLINGFEVER].val2)
			max = sc->data[SC_GATLINGFEVER].val2;
		
		if(sc->data[SC_FLEET].timer!=-1 &&
			max < sc->data[SC_FLEET].val2)
			max = sc->data[SC_FLEET].val2;

		if(sc->data[SC_ASSNCROS].timer!=-1 &&
			max < sc->data[SC_ASSNCROS].val2)
		{
			if (bl->type!=BL_PC)
				max = sc->data[SC_ASSNCROS].val2;
			else
			switch(((TBL_PC*)bl)->status.weapon)
			{
				case W_BOW:
				case W_REVOLVER:
				case W_RIFLE:
				case W_SHOTGUN:
				case W_GATLING:
				case W_GRENADE:
					break;
				default:
					max = sc->data[SC_ASSNCROS].val2;
			}
		}
		aspd_rate -= max;
	}
	if(sc->data[i=SC_ASPDPOTION3].timer!=-1 ||
		sc->data[i=SC_ASPDPOTION2].timer!=-1 ||
		sc->data[i=SC_ASPDPOTION1].timer!=-1 ||
		sc->data[i=SC_ASPDPOTION0].timer!=-1)
		aspd_rate -= sc->data[i].val2;
	if(sc->data[SC_DONTFORGETME].timer!=-1)
		aspd_rate += sc->data[SC_DONTFORGETME].val2;
	if(sc->data[SC_LONGING].timer!=-1)
		aspd_rate += sc->data[SC_LONGING].val2;
	if(sc->data[SC_STEELBODY].timer!=-1)
		aspd_rate += 25;
	if(sc->data[SC_SKA].timer!=-1)
		aspd_rate += 25;
	if(sc->data[SC_DEFENDER].timer != -1)
		aspd_rate += 25 -sc->data[SC_DEFENDER].val1*5;
	if(sc->data[SC_GOSPEL].timer!=-1 && sc->data[SC_GOSPEL].val4 == BCT_ENEMY)
		aspd_rate += 25;
	if(sc->data[SC_GRAVITATION].timer!=-1)
		aspd_rate += sc->data[SC_GRAVITATION].val2;
//Curse shouldn't effect on this?
//		if(sc->data[SC_BLEEDING].timer != -1)
//			aspd_rate += 25;
	if(sc->data[SC_JOINTBEAT].timer!=-1) {
		if (sc->data[SC_JOINTBEAT].val2 == 1)
			aspd_rate += 25;
		else if (sc->data[SC_JOINTBEAT].val2 == 2)
			aspd_rate += 10;
	}

	return cap_value(aspd_rate,0,SHRT_MAX);
}

static unsigned short status_calc_dmotion(struct block_list *bl, struct status_change *sc, int dmotion)
{
	if(!sc || !sc->count || map_flag_gvg(bl->m))
		return dmotion;
		
	if (sc->data[SC_ENDURE].timer!=-1 ||
		sc->data[SC_CONCENTRATION].timer!=-1)
		return 0;

	return cap_value(dmotion,0,USHRT_MAX);
}

static unsigned int status_calc_maxhp(struct block_list *bl, struct status_change *sc, unsigned int maxhp)
{
	if(!sc || !sc->count)
		return maxhp;

	if(sc->data[SC_INCMHPRATE].timer!=-1)
		maxhp += maxhp * sc->data[SC_INCMHPRATE].val1/100;
	if(sc->data[SC_APPLEIDUN].timer!=-1)
		maxhp += maxhp * sc->data[SC_APPLEIDUN].val2/100;
	if(sc->data[SC_DELUGE].timer!=-1)
		maxhp += maxhp * sc->data[SC_DELUGE].val2/100;
	if(sc->data[SC_BERSERK].timer!=-1)
		maxhp += maxhp * 2;

	return cap_value(maxhp,1,UINT_MAX);
}

static unsigned int status_calc_maxsp(struct block_list *bl, struct status_change *sc, unsigned int maxsp)
{
	if(!sc || !sc->count)
		return maxsp;
	if(sc->data[SC_INCMSPRATE].timer!=-1)
		maxsp += maxsp * sc->data[SC_INCMSPRATE].val1/100;
	if(sc->data[SC_SERVICE4U].timer!=-1)
		maxsp += maxsp * sc->data[SC_SERVICE4U].val2/100;

	return cap_value(maxsp,1,UINT_MAX);
}

static unsigned char status_calc_element(struct block_list *bl, struct status_change *sc, int element)
{
	if(!sc || !sc->count)
		return element;
	if( sc->data[SC_FREEZE].timer!=-1 )	
		return ELE_WATER;
	if( sc->data[SC_STONE].timer!=-1 && sc->opt1 == OPT1_STONE)
		return ELE_EARTH;
	if( sc->data[SC_BENEDICTIO].timer!=-1 )
		return ELE_HOLY;
	if( sc->data[SC_ELEMENTALCHANGE].timer!=-1)
		return sc->data[SC_ELEMENTALCHANGE].val3;
	return cap_value(element,0,UCHAR_MAX);
}

static unsigned char status_calc_element_lv(struct block_list *bl, struct status_change *sc, int lv)
{
	if(!sc || !sc->count)
		return lv;
	if(sc->data[SC_ELEMENTALCHANGE].timer!=-1)
		return sc->data[SC_ELEMENTALCHANGE].val4;
	return cap_value(lv,0,UCHAR_MAX);
}


unsigned char status_calc_attack_element(struct block_list *bl, struct status_change *sc, int element)
{
	if(!sc || !sc->count)
		return element;
	if( sc->data[SC_WATERWEAPON].timer!=-1)
		return ELE_WATER;
	if( sc->data[SC_EARTHWEAPON].timer!=-1)
		return ELE_EARTH;
	if( sc->data[SC_FIREWEAPON].timer!=-1)
		return ELE_FIRE;
	if( sc->data[SC_WINDWEAPON].timer!=-1)
		return ELE_WIND;
	if( sc->data[SC_ENCPOISON].timer!=-1)
		return ELE_POISON;
	if( sc->data[SC_ASPERSIO].timer!=-1)
		return ELE_HOLY;
	if( sc->data[SC_SHADOWWEAPON].timer!=-1)
		return ELE_DARK;
	if( sc->data[SC_GHOSTWEAPON].timer!=-1)
		return ELE_GHOST;
	return cap_value(element,0,UCHAR_MAX);
}

static unsigned short status_calc_mode(struct block_list *bl, struct status_change *sc, int mode)
{
	if(!sc || !sc->count)
		return mode;
	if(sc->data[SC_MODECHANGE].timer!=-1) {
		if (sc->data[SC_MODECHANGE].val2)
			mode = sc->data[SC_MODECHANGE].val2; //Set mode
		if (sc->data[SC_MODECHANGE].val3)
			mode|= sc->data[SC_MODECHANGE].val3; //Add mode
		if (sc->data[SC_MODECHANGE].val4)
			mode&=~sc->data[SC_MODECHANGE].val4; //Del mode
	}
	return cap_value(mode,0,USHRT_MAX);
}

/*==========================================
 * Quick swap of adelay/speed when starting ending SA_FREECAST
 *------------------------------------------
 */
void status_freecast_switch(struct map_session_data *sd)
{
	struct status_data *status;
	unsigned short b_speed,tmp;

	status = &sd->battle_status;

	b_speed = status->speed;

	tmp = status->speed;
	status->speed = sd->prev_speed;
	sd->prev_speed = tmp;

	tmp = status->adelay;
	status->adelay = sd->prev_adelay;
	sd->prev_adelay = tmp;

	if(b_speed != status->speed)
		clif_updatestatus(sd,SP_SPEED);
}

/*==========================================
 * 対象のClassを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int status_get_class(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB)	//Class used on all code should be the view class of the mob.
		return ((struct mob_data *)bl)->vd->class_;
	if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->status.class_;
	if(bl->type==BL_PET)
		return ((struct pet_data *)bl)->class_;
	return 0;
}
/*==========================================
 * 対象のレベルを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int status_get_lv(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB)
		return ((TBL_MOB*)bl)->level;
	if(bl->type==BL_PC)
		return ((TBL_PC*)bl)->status.base_level;
	if(bl->type==BL_PET)
		return ((TBL_PET*)bl)->msd->pet.level;
	if(bl->type==BL_HOMUNCULUS)
		return ((TBL_HOMUNCULUS*)bl)->level;
	return 1;
}

struct status_data *status_get_status_data(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
		
	switch (bl->type) {
		case BL_PC:
			return &((TBL_PC*)bl)->battle_status;
		case BL_MOB:
			return &((TBL_MOB*)bl)->status;
		case BL_PET:
			return &((TBL_PET*)bl)->status;
		case BL_HOMUNCULUS:
			return &((TBL_HOMUNCULUS*)bl)->battle_status;
		default:
			return &dummy_status;
	}
}

struct status_data *status_get_base_status(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	switch (bl->type) {
		case BL_PC:
			return &((TBL_PC*)bl)->base_status;
		case BL_MOB:
			return ((TBL_MOB*)bl)->base_status?
				((TBL_MOB*)bl)->base_status:
				&((TBL_MOB*)bl)->db->status;
		case BL_PET:
			return &((TBL_PET*)bl)->db->status;
		case BL_HOMUNCULUS:
			return &((TBL_HOMUNCULUS*)bl)->base_status;
		default:
			return NULL;
	}
}

unsigned short status_get_lwatk(struct block_list *bl)
{
	struct status_data *status = status_get_status_data(bl);
	return status->lhw?status->lhw->atk:0;
}

unsigned short status_get_lwatk2(struct block_list *bl)
{
	struct status_data *status = status_get_status_data(bl);
	return status->lhw?status->lhw->atk2:0;
}

unsigned char status_get_def(struct block_list *bl)
{
	struct unit_data *ud;
	struct status_data *status = status_get_status_data(bl);
	int def = status?status->def:0;
	ud = unit_bl2ud(bl);
	if (ud && ud->skilltimer != -1)
		def -= def * skill_get_castdef(ud->skillid)/100;
	if(def < 0) def = 0;
	return def;
}

unsigned short status_get_speed(struct block_list *bl)
{
	if(bl->type==BL_NPC)//Only BL with speed data but no status_data [Skotlex]
		return ((struct npc_data *)bl)->speed;
	return status_get_status_data(bl)->speed;
}

unsigned char status_get_attack_lelement(struct block_list *bl)
{
	struct status_data *status = status_get_status_data(bl);
	return status->lhw?status->lhw->ele:0;
}

int status_get_party_id(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->status.party_id;
	if(bl->type==BL_PET)
		return ((struct pet_data *)bl)->msd->status.party_id;
	if(bl->type==BL_MOB){
		struct mob_data *md=(struct mob_data *)bl;
		if( md->master_id>0 )
		{
			struct map_session_data *msd;
			if (md->special_state.ai && (msd = map_id2sd(md->master_id)) != NULL)
				return msd->status.party_id;
			return -md->master_id;
		}
		return 0; //No party.
	}
	if(bl->type==BL_SKILL)
		return ((struct skill_unit *)bl)->group->party_id;
	return 0;
}

int status_get_guild_id(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->status.guild_id;
	if(bl->type==BL_PET)
		return ((struct pet_data *)bl)->msd->status.guild_id;
	if(bl->type==BL_MOB)
	{
		struct map_session_data *msd;
		struct mob_data *md = (struct mob_data *)bl;
		if (md->guardian_data)	//Guardian's guild [Skotlex]
			return md->guardian_data->guild_id;
		if (md->special_state.ai && (msd = map_id2sd(md->master_id)) != NULL)
			return msd->status.guild_id; //Alchemist's mobs [Skotlex]
		return 0; //No guild.
	}
	if (bl->type == BL_NPC && bl->subtype == SCRIPT)
		return ((TBL_NPC*)bl)->u.scr.guild_id;
	if(bl->type==BL_SKILL)
		return ((struct skill_unit *)bl)->group->guild_id;
	return 0;
}

int status_get_mexp(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB)
		return ((struct mob_data *)bl)->db->mexp;
	if(bl->type==BL_PET)
		return ((struct pet_data *)bl)->db->mexp;
	return 0;
}
int status_get_race2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type == BL_MOB)
		return ((struct mob_data *)bl)->db->race2;
	if(bl->type==BL_PET)
		return ((struct pet_data *)bl)->db->race2;
	return 0;
}
int status_isdead(struct block_list *bl)
{
	nullpo_retr(0, bl);
	return status_get_status_data(bl)->hp == 0;
}

int status_isimmune(struct block_list *bl)
{
	struct status_change *sc =status_get_sc(bl);
	if (bl->type == BL_PC &&
		((struct map_session_data *)bl)->special_state.no_magic_damage)
		return 1;
	if (sc && sc->count && sc->data[SC_HERMODE].timer != -1)
		return 1;
	return 0;
}

struct view_data *status_get_viewdata(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	switch (bl->type)
	{
		case BL_PC:
			return &((TBL_PC*)bl)->vd;
		case BL_MOB:
			return ((TBL_MOB*)bl)->vd;
		case BL_PET:
			return &((TBL_PET*)bl)->vd;
		case BL_NPC:
			return ((TBL_NPC*)bl)->vd;
		case BL_HOMUNCULUS: //[blackhole89]
			return ((struct homun_data*)bl)->vd;
	}
	return NULL;
}

void status_set_viewdata(struct block_list *bl, int class_)
{
	struct view_data* vd;
	nullpo_retv(bl);
	if (mobdb_checkid(class_) || mob_is_clone(class_))
		vd =  mob_get_viewdata(class_);
	else if (npcdb_checkid(class_) || (bl->type == BL_NPC && class_ == WARP_CLASS))
		vd = npc_get_viewdata(class_);
	else
		vd = NULL;

	switch (bl->type) {
	case BL_PC:
		{
			TBL_PC* sd = (TBL_PC*)bl;
			if (pcdb_checkid(class_)) {
				if (sd->sc.option&OPTION_RIDING)
				switch (class_)
				{	//Adapt class to a Mounted one.
				case JOB_KNIGHT:
					class_ = JOB_KNIGHT2;
					break;
				case JOB_CRUSADER:
					class_ = JOB_CRUSADER2;
					break;
				case JOB_LORD_KNIGHT:
					class_ = JOB_LORD_KNIGHT2;
					break;
				case JOB_PALADIN:
					class_ = JOB_PALADIN2;
					break;
				case JOB_BABY_KNIGHT:
					class_ = JOB_BABY_KNIGHT2;
					break;
				case JOB_BABY_CRUSADER:
					class_ = JOB_BABY_CRUSADER2;
					break;
				}
				if (class_ == JOB_WEDDING)
					sd->sc.option|=OPTION_WEDDING;
				else if (sd->sc.option&OPTION_WEDDING)
					sd->sc.option&=~OPTION_WEDDING; //If not going to display it, then remove the option.
				sd->vd.class_ = class_;
				clif_get_weapon_view(sd, &sd->vd.weapon, &sd->vd.shield);
				sd->vd.head_top = sd->status.head_top;
				sd->vd.head_mid = sd->status.head_mid;
				sd->vd.head_bottom = sd->status.head_bottom;
				sd->vd.hair_style = sd->status.hair;
				sd->vd.hair_color = sd->status.hair_color;
				sd->vd.cloth_color = sd->status.clothes_color;
				sd->vd.sex = sd->status.sex;
			} else if (vd)
				memcpy(&sd->vd, vd, sizeof(struct view_data));
			else if (battle_config.error_log)
				ShowError("status_set_viewdata (PC): No view data for class %d\n", class_);
		}
	break;
	case BL_MOB:
		{
			TBL_MOB* md = (TBL_MOB*)bl;
			if (vd)
				md->vd = vd;
			else if (battle_config.error_log)
				ShowError("status_set_viewdata (MOB): No view data for class %d\n", class_);
		}
	break;
	case BL_PET:
		{
			TBL_PET* pd = (TBL_PET*)bl;
			if (vd) {
				memcpy(&pd->vd, vd, sizeof(struct view_data));
				if (!pcdb_checkid(vd->class_)) {
					pd->vd.hair_style = battle_config.pet_hair_style;
					if(pd->equip) {
						pd->vd.head_bottom = itemdb_viewid(pd->equip);
						if (!pd->vd.head_bottom)
							pd->vd.head_bottom = pd->equip;
					}
				}
			} else if (battle_config.error_log)
				ShowError("status_set_viewdata (PET): No view data for class %d\n", class_);
		}
	break;
	case BL_NPC:
		{
			TBL_NPC* nd = (TBL_NPC*)bl;
			if (vd)
				nd->vd = vd;
			else if (battle_config.error_log)
				ShowError("status_set_viewdata (NPC): No view data for class %d\n", class_);
		}
	break;
	case BL_HOMUNCULUS:		//[blackhole89]
		{
			struct homun_data *hd = (struct homun_data*)bl;
			if (vd)
				hd->vd = vd;
			else if (battle_config.error_log)
				ShowError("status_set_viewdata (HOMUNCULUS): No view data for class %d\n", class_);
		}
		break;
	}
	vd = status_get_viewdata(bl);
	if (vd && vd->cloth_color && (
		(vd->class_==JOB_WEDDING && !battle_config.wedding_ignorepalette)
		|| (vd->class_==JOB_XMAS && !battle_config.xmas_ignorepalette)
	))
		vd->cloth_color = 0;
}

struct status_change *status_get_sc(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	switch (bl->type) {
	case BL_MOB:
		return &((TBL_MOB*)bl)->sc;
	case BL_PC:
		return &((TBL_PC*)bl)->sc;
	case BL_NPC:
		return &((TBL_NPC*)bl)->sc;
	case BL_HOMUNCULUS: //[blackhole89]
		return &((struct homun_data*)bl)->sc;
	}
	return NULL;
}

void status_change_init(struct block_list *bl)
{
	struct status_change *sc = status_get_sc(bl);
	int i;
	nullpo_retv(sc);
	memset(sc, 0, sizeof (struct status_change));
	for (i=0; i< SC_MAX; i++)
		sc->data[i].timer = -1;
}

//Returns defense against the specified status change.
//Return range is 0 (no resist) to 10000 (inmunity)
int status_get_sc_def(struct block_list *bl, int type)
{
	int sc_def;
	struct status_change* sc;
	nullpo_retr(0, bl);

	//Status that are blocked by Golden Thief Bug card or Wand of Hermod
	if (status_isimmune(bl))
	switch (type)
	{
	case SC_DECREASEAGI:
	case SC_SILENCE:
	case SC_COMA:
	case SC_INCREASEAGI:
	case SC_BLESSING:
	case SC_SLOWPOISON:
	case SC_IMPOSITIO:
	case SC_AETERNA:
	case SC_SUFFRAGIUM:
	case SC_BENEDICTIO:
	case SC_PROVIDENCE:
	case SC_KYRIE:
	case SC_ASSUMPTIO:
	case SC_ANGELUS:
	case SC_MAGNIFICAT:
	case SC_GLORIA:
	case SC_WINDWALK:
	case SC_MAGICROD:
	case SC_HALLUCINATION:
	case SC_STONE:
	case SC_QUAGMIRE:
		return 10000;
	}
	
	switch (type)
	{
	//Note that stats that are *100/3 were simplified to *33
	case SC_STONE:
	case SC_FREEZE:
	case SC_DECREASEAGI:
	case SC_COMA:
		sc_def = 300 +100*status_get_mdef(bl) +33*status_get_luk(bl);
		break;
	case SC_SLEEP:
	case SC_CONFUSION:
		sc_def = 300 +100*status_get_int(bl) +33*status_get_luk(bl);
		break;
// Removed since it collides with normal sc.
//	case SP_DEF1:	// def
//		sc_def = 300 +100*status_get_def(bl) +33*status_get_luk(bl);
//		break;
	case SC_STUN:
	case SC_POISON:
	case SC_DPOISON:
	case SC_SILENCE:
	case SC_BLEEDING:
	case SC_STOP:
		sc_def = 300 +100*status_get_vit(bl) +33*status_get_luk(bl);
		break;
	case SC_BLIND:
		sc_def = 300 +100*status_get_int(bl) +33*status_get_vit(bl);
		break;
	case SC_CURSE:
		sc_def = 300 +100*status_get_luk(bl) +33*status_get_vit(bl);
		break;
	default:
		return 0; //Effect that cannot be reduced? Likely a buff.
	}

	if (bl->type == BL_PC) {
		if (battle_config.pc_sc_def_rate != 100)
			sc_def = sc_def*battle_config.pc_sc_def_rate/100;
	} else
	if (battle_config.mob_sc_def_rate != 100)
		sc_def = sc_def*battle_config.mob_sc_def_rate/100;
	
	sc = status_get_sc(bl);
	if (sc && sc->count)
	{
		if (sc->data[SC_SCRESIST].timer != -1)
			sc_def += 100*sc->data[SC_SCRESIST].val1; //Status resist
		else if (sc->data[SC_SIEGFRIED].timer != -1)
			sc_def += 100*sc->data[SC_SIEGFRIED].val3; //Status resistance.
	}

	if(bl->type == BL_PC) {
		if (sc_def > battle_config.pc_max_sc_def)
			sc_def = battle_config.pc_max_sc_def;
	} else if (sc_def > battle_config.mob_max_sc_def)
		sc_def = battle_config.mob_max_sc_def;
	
	return sc_def;
}

//Reduces tick delay based on type and character defenses.
int status_get_sc_tick(struct block_list *bl, int type, int tick)
{
	struct map_session_data *sd;
	int rate=0, min=0;
	//If rate is positive, it is a % reduction (10000 -> 100%)
	//if it is negative, it is an absolute reduction in ms.
	sd = bl->type == BL_PC?(struct map_session_data *)bl:NULL;
	switch (type) {
		case SC_DECREASEAGI:		/* 速度減少 */
			if (sd)	// Celest
				tick>>=1;
		break;
		case SC_ADRENALINE:			/* アドレナリンラッシュ */
		case SC_ADRENALINE2:
		case SC_WEAPONPERFECTION:	/* ウェポンパ?フェクション */
		case SC_OVERTHRUST:			/* オ?バ?スラスト */
			if(sd && pc_checkskill(sd,BS_HILTBINDING)>0)
				tick += tick / 10;
		break;
		case SC_STONE:				/* 石化 */
			rate = -200*status_get_mdef(bl);
		break;
		case SC_FREEZE:				/* 凍結 */
			rate = 100*status_get_mdef(bl);
		break;
		case SC_STUN:	//Reduction in duration is the same as reduction in rate.
			rate = 300 +100*status_get_vit(bl) +33*status_get_luk(bl);
		break;
		case SC_DPOISON:			/* 猛毒 */
		case SC_POISON:				/* 毒 */
			rate = 100*status_get_vit(bl) + 20*status_get_luk(bl);
		break;
		case SC_SILENCE:			/* 沈?（レックスデビ?ナ） */
		case SC_CONFUSION:
		case SC_CURSE:
			rate = 100*status_get_vit(bl);
		break;
		case SC_BLIND:				/* 暗? */
			rate = 10*status_get_lv(bl) + 7*status_get_int(bl);
			min = 5000; //Minimum 5 secs?
		break;
		case SC_BLEEDING:
			rate = 20*status_get_lv(bl) +100*status_get_vit(bl);
			min = 10000; //Need a min of 10 secs for it to hurt at least once.
		break;
		case SC_SWOO:
			if (status_get_mode(bl)&MD_BOSS)
				tick /= 5; //TODO: Reduce skill's duration. But for how long?
		break;
		case SC_ANKLE:
			if(status_get_mode(bl)&MD_BOSS) // Lasts 5 times less on bosses
				tick /= 5;
			rate = -100*status_get_agi(bl);
		// Minimum trap time of 3+0.03*skilllv seconds [celest]
		// Changed to 3 secs and moved from skill.c [Skotlex]
			min = 3000;
		break;
		case SC_SPIDERWEB:
			if (map[bl->m].flag.pvp)
				tick /=2;
		break;
		case SC_STOP:
		// Unsure of this... but I get a feeling that agi reduces this
		// (it was on Tiger Fist Code, but at -1 ms per 10 agi....
			rate = -100*status_get_agi(bl);
		break;
	}
	if (rate) {
		if (bl->type == BL_PC) {
			if (battle_config.pc_sc_def_rate != 100)
				rate = rate*battle_config.pc_sc_def_rate/100;
			if (battle_config.pc_max_sc_def != 10000)
				min = tick*(10000-battle_config.pc_max_sc_def)/10000;
		} else {
			if (battle_config.mob_sc_def_rate != 100)
				rate = rate*battle_config.mob_sc_def_rate/100;
			if (battle_config.mob_max_sc_def != 10000)
				min = tick*(10000-battle_config.mob_max_sc_def)/10000;
		}
		
		if (rate >0)
			tick -= tick*rate/10000;
		else
			tick += rate;
	}
	return tick<min?min:tick;
}
/*==========================================
 * Starts a status change.
 * type = type, val1~4 depend on the type.
 * rate = base success rate. 10000 = 100%
 * Tick is base duration
 * flag:
 * &1: Cannot be avoided (it has to start)
 * &2: Tick should not be reduced (by vit, luk, lv, etc)
 * &4: sc_data loaded, no value has to be altered.
 * &8: rate should not be reduced
 *------------------------------------------
 */
int status_change_start(struct block_list *bl,int type,int rate,int val1,int val2,int val3,int val4,int tick,int flag)
{
	struct map_session_data *sd = NULL;
	struct status_change* sc;
	struct status_data *status;
	int opt_flag , calc_flag = 0, undead_flag;

	nullpo_retr(0, bl);
	sc=status_get_sc(bl);
	status = status_get_status_data(bl);

	if (!sc || !status || status_isdead(bl))
		return 0;
	
	switch (bl->type)
	{
		case BL_PC:
			sd=(struct map_session_data *)bl;
			break;
		case BL_MOB:
			if (((struct mob_data*)bl)->class_ == MOBID_EMPERIUM && type != SC_SAFETYWALL)
				return 0; //Emperium can't be afflicted by status changes.
			break;
	}

	if(type < 0 || type >= SC_MAX) {
		if(battle_config.error_log)
			ShowError("status_change_start: invalid status change (%d)!\n", type);
		return 0;
	}

	//Check rate
	if (!(flag&(4|1))) {
		int def;
		def = flag&8?0:status_get_sc_def(bl, type); //recycling race to store the sc_def value.
		//sd resistance applies even if the flag is &8
		if(sd && SC_COMMON_MIN<=type && type<=SC_COMMON_MAX && sd->reseff[type-SC_COMMON_MIN] > 0)
			def+= sd->reseff[type-SC_COMMON_MIN];

		if (def)
			rate -= rate*def/10000;

		if (!(rand()%10000 < rate))
			return 0;
	}
	
	//SC duration reduction.
	if(!(flag&(2|4)) && tick) {
		tick = status_get_sc_tick(bl, type, tick);
		if (tick <= 0)
			return 0;
	}

	undead_flag=battle_check_undead(status->race,status->def_ele);

	//Check for inmunities / sc fails
	switch (type) {
		case SC_FREEZE:
		case SC_STONE:
			//Undead are inmune to Freeze/Stone
			if (undead_flag && !(flag&1))
				return 0;
		case SC_SLEEP:
		case SC_STUN:
			if (sc->opt1)
				return 0; //Cannot override other opt1 status changes. [Skotlex]
		break;
		case SC_CURSE:
			//Dark Elementals are inmune to curse.
			if (status->def_ele == ELE_DARK && !(flag&1))
				return 0;
		break;
		case SC_COMA:
			//Dark elementals and Demons are inmune to coma.
			if((status->def_ele == ELE_DARK || status->race == RC_DEMON) && !(flag&1))
				return 0;
		break;
		case SC_SIGNUMCRUCIS:
			//Only affects demons and undead.
			if(status->race != RC_DEMON && !undead_flag)
				return 0;
			break;
		case SC_AETERNA:
		  if (sc->data[SC_STONE].timer != -1 || sc->data[SC_FREEZE].timer != -1)
			  return 0;
		break;
		case SC_OVERTHRUST:
			if (sc->data[SC_MAXOVERTHRUST].timer != -1)
				return 0; //Overthrust can't take effect if under Max Overthrust. [Skotlex]
		break;
		case SC_ADRENALINE:
		 	if (sd && !(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)))
				return 0;
			if (sc->data[SC_QUAGMIRE].timer!=-1 ||
				sc->data[SC_DONTFORGETME].timer!=-1 ||
				sc->data[SC_DECREASEAGI].timer!=-1
			)
				return 0;
		break;
		case SC_ADRENALINE2:
			if (sd && !(skill_get_weapontype(BS_ADRENALINE2)&(1<<sd->status.weapon)))
				return 0;
			if (sc->data[SC_QUAGMIRE].timer!=-1 ||
				sc->data[SC_DONTFORGETME].timer!=-1 ||
				sc->data[SC_DECREASEAGI].timer!=-1
			)
				return 0;
		break;
		case SC_ONEHAND:
		case SC_TWOHANDQUICKEN:
			if(sc->data[SC_DECREASEAGI].timer!=-1)
				return 0;
		case SC_CONCENTRATE:
		case SC_INCREASEAGI:
		case SC_SPEARQUICKEN:
		case SC_TRUESIGHT:
		case SC_WINDWALK:
		case SC_CARTBOOST:
		case SC_ASSNCROS:
			if (sc->data[SC_QUAGMIRE].timer!=-1 || sc->data[SC_DONTFORGETME].timer!=-1)
				return 0;
		break;
		case SC_CLOAKING:
		//Avoid cloaking with no wall and low skill level. [Skotlex]
		//Due to the cloaking card, we have to check the wall versus to known skill level rather than the used one. [Skotlex]
//			if (sd && skilllv < 3 && skill_check_cloaking(bl,&sd->sc))
			if (sd && pc_checkskill(sd, AS_CLOAKING)< 3 && skill_check_cloaking(bl, &sd->sc))
				return 0;
		break;
		case SC_MODECHANGE:
		{
			int mode;
			struct status_data *bstatus = status_get_base_status(bl);
			if (!bstatus) return 0;
			mode = val2?val2:bstatus->mode; //Base mode
			if (val3) mode|= val3; //Add mode
			if (val4) mode&=~val4; //Del mode
			if (mode == bstatus->mode) { //No change.
				if (sc->data[type].timer != -1) //Abort previous status
					return status_change_end(bl, type, -1);
				return 0;
			}
		}
	}

	//Check for BOSS resistances
	if(status->mode&MD_BOSS && !(flag&1)) {
		 if (type>=SC_COMMON_MIN && type <= SC_COMMON_MAX)
			 return 0;
		 switch (type) {
			case SC_BLESSING:
			  if (!undead_flag && status->race != RC_DEMON)
				  break;
			case SC_QUAGMIRE:
			case SC_DECREASEAGI:
			case SC_SIGNUMCRUCIS:
			case SC_PROVOKE:
			case SC_ROKISWEIL:
			case SC_COMA:
			case SC_GRAVITATION:
				return 0;
		}
	}
	//Before overlapping fail, one must check for status cured.
	switch (type) {
	case SC_BLESSING:
		if ((!undead_flag && status->race!=RC_DEMON) || bl->type == BL_PC) {
			if (sc->data[SC_CURSE].timer!=-1)
				status_change_end(bl,SC_CURSE,-1);
			if (sc->data[SC_STONE].timer!=-1 && sc->opt1 == OPT1_STONE)
				status_change_end(bl,SC_STONE,-1);
		}
		break;
	case SC_INCREASEAGI:
		if(sc->data[SC_DECREASEAGI].timer!=-1 )
			status_change_end(bl,SC_DECREASEAGI,-1);
		break;
	case SC_DONTFORGETME:
		//is this correct? Maybe all three should stop the same subset of SCs...
		if(sc->data[SC_ASSNCROS].timer!=-1 )
			status_change_end(bl,SC_ASSNCROS,-1);
	case SC_QUAGMIRE:
		if(sc->data[SC_CONCENTRATE].timer!=-1 )
			status_change_end(bl,SC_CONCENTRATE,-1);
		if(sc->data[SC_TRUESIGHT].timer!=-1 )
			status_change_end(bl,SC_TRUESIGHT,-1);
		if(sc->data[SC_WINDWALK].timer!=-1 )
			status_change_end(bl,SC_WINDWALK,-1);
		//Also blocks the ones below...
	case SC_DECREASEAGI:
		if(sc->data[SC_INCREASEAGI].timer!=-1 )
			status_change_end(bl,SC_INCREASEAGI,-1);
		if(sc->data[SC_ADRENALINE].timer!=-1 )
			status_change_end(bl,SC_ADRENALINE,-1);
		if(sc->data[SC_ADRENALINE2].timer!=-1 )
			status_change_end(bl,SC_ADRENALINE2,-1);
		if(sc->data[SC_SPEARQUICKEN].timer!=-1 )
			status_change_end(bl,SC_SPEARQUICKEN,-1);
		if(sc->data[SC_TWOHANDQUICKEN].timer!=-1 )
			status_change_end(bl,SC_TWOHANDQUICKEN,-1);
		if(sc->data[SC_CARTBOOST].timer!=-1 )
			status_change_end(bl,SC_CARTBOOST,-1);
		if(sc->data[SC_ONEHAND].timer!=-1 )
			status_change_end(bl,SC_ONEHAND,-1);
		break;
	case SC_ONEHAND:
	  	//Removes the Aspd potion effect, as reported by Vicious. [Skotlex]
		if(sc->data[SC_ASPDPOTION0].timer!=-1)
			status_change_end(bl,SC_ASPDPOTION0,-1);
		if(sc->data[SC_ASPDPOTION1].timer!=-1)
			status_change_end(bl,SC_ASPDPOTION1,-1);
		if(sc->data[SC_ASPDPOTION2].timer!=-1)
			status_change_end(bl,SC_ASPDPOTION2,-1);
		if(sc->data[SC_ASPDPOTION3].timer!=-1)
			status_change_end(bl,SC_ASPDPOTION3,-1);
		break;
	case SC_MAXOVERTHRUST:
	  	//Cancels Normal Overthrust. [Skotlex]
		if (sc->data[SC_OVERTHRUST].timer != -1)
			status_change_end(bl, SC_OVERTHRUST, -1);
		break;
	case SC_KYRIE:
		// -- moonsoul (added to undo assumptio status if target has it)
		if(sc->data[SC_ASSUMPTIO].timer!=-1 )
			status_change_end(bl,SC_ASSUMPTIO,-1);
		break;
	case SC_DELUGE:
		if (sc->data[SC_FOGWALL].timer != -1 && sc->data[SC_BLIND].timer != -1)
			status_change_end(bl,SC_BLIND,-1);
		break;
	case SC_SILENCE:
		if (sc->data[SC_GOSPEL].timer!=-1 && sc->data[SC_GOSPEL].val4 == BCT_SELF)
		  	//Clear Gospel [Skotlex]
			status_change_end(bl,SC_GOSPEL,-1);
		break;
	case SC_HIDING:
		if(sc->data[SC_CLOSECONFINE].timer != -1)
			status_change_end(bl, SC_CLOSECONFINE, -1);
		if(sc->data[SC_CLOSECONFINE2].timer != -1)
			status_change_end(bl, SC_CLOSECONFINE2, -1);
		break;
	case SC_BERSERK:
		if(battle_config.berserk_cancels_buffs)
		{
			if (sc->data[SC_ONEHAND].timer != -1)
				status_change_end(bl,SC_ONEHAND,-1);
			if (sc->data[SC_TWOHANDQUICKEN].timer != -1)
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if (sc->data[SC_CONCENTRATION].timer != -1)
				status_change_end(bl,SC_CONCENTRATION,-1);
			if (sc->data[SC_PARRYING].timer != -1)
				status_change_end(bl,SC_PARRYING,-1);
			if (sc->data[SC_AURABLADE].timer != -1)
				status_change_end(bl,SC_AURABLADE,-1);
		}
		break;
	case SC_ASSUMPTIO:
		if(sc->data[SC_KYRIE].timer!=-1)
			status_change_end(bl,SC_KYRIE,-1);
		break;
	case SC_CARTBOOST:
		if(sc->data[SC_DECREASEAGI].timer!=-1 )
		{	//Cancel Decrease Agi, but take no further effect [Skotlex]
			status_change_end(bl,SC_DECREASEAGI,-1);
			return 0;
		}
		break;
	case SC_FUSION:
		if(sc->data[SC_SPIRIT].timer!=-1 )
			status_change_end(bl,SC_SPIRIT,-1);
		break;
	case SC_ADJUSTMENT:
		if(sc->data[SC_MADNESSCANCEL].timer != -1)
			status_change_end(bl,SC_MADNESSCANCEL,-1);
		break;
	case SC_MADNESSCANCEL:
		if(sc->data[SC_ADJUSTMENT].timer!=-1)
			status_change_end(bl,SC_ADJUSTMENT,-1);
		break;
	}
	//Check for overlapping fails
	if(sc->data[type].timer != -1){
		switch (type) {
			case SC_ADRENALINE:
			case SC_ADRENALINE2:
			case SC_WEAPONPERFECTION:
			case SC_OVERTHRUST:
				if (sc->data[type].val2 > val2)
					return 0;
			break;
			case SC_WARM:
			{	//Fetch the Group, half the attack interval. [Skotlex]
				struct skill_unit_group *group = (struct skill_unit_group *)sc->data[type].val4;
				if (group)
					group->interval/=2;
				return 1;
			}
			case SC_STUN:
			case SC_SLEEP:
			case SC_POISON:
			case SC_CURSE:
			case SC_SILENCE:
			case SC_CONFUSION:
			case SC_BLIND:
			case SC_BLEEDING:
			case SC_DPOISON:
			case SC_COMBO: //You aren't supposed to change the combo (and it gets turned off when you trigger it)
			case SC_CLOSECONFINE2: //Can't be re-closed in.
			case SC_MARIONETTE:
			case SC_MARIONETTE2:
				return 0;
			case SC_DANCING:
			case SC_DEVOTION:
			case SC_ASPDPOTION0:
			case SC_ASPDPOTION1:
			case SC_ASPDPOTION2:
			case SC_ASPDPOTION3:
			case SC_ATKPOTION:
			case SC_MATKPOTION:
				break;
			case SC_GOSPEL:
				 //Must not override a casting gospel char.
				if(sc->data[type].val4 == BCT_SELF)
					return 0;
				if(sc->data[type].val1 > val1)
					return 1;
				break;
			case SC_ENDURE:
				if(sc->data[type].val4 && !val4)
					return 1; //Don't let you override infinite endure.
				if(sc->data[type].val1 > val1)
					return 1;
				break;
			case SC_KAAHI:
				if(sc->data[type].val1 > val1)
					return 1;
				//Delete timer if it exists.
				if (sc->data[type].val4 != -1) {
					delete_timer(sc->data[type].val4,kaahi_heal_timer);
					sc->data[type].val4=-1;
				}
				break;
			default:
				if(sc->data[type].val1 > val1)
					return 1; //Return true to not mess up skill animations. [Skotlex
			}
		(sc->count)--;
		delete_timer(sc->data[type].timer, status_change_timer);
		sc->data[type].timer = -1;
	}

	calc_flag = StatusChangeFlagTable[type];
	if(!(flag&4)) //Do not parse val settings when loading SCs
	switch(type){
		case SC_ENDURE:				/* インデュア */
			val2 = 7; // Hit-count [Celest]
			break;
		case SC_AUTOBERSERK:
			if (status->hp < status->max_hp>>2 &&
				(sc->data[SC_PROVOKE].timer==-1 || sc->data[SC_PROVOKE].val2==0))
					sc_start4(bl,SC_PROVOKE,100,10,1,0,0,60000);
			break;
		
		case SC_SIGNUMCRUCIS:
			val2 = 10 + val1*2; //Def reduction
			clif_emotion(bl,4);
			break;
		case SC_MAXIMIZEPOWER:
			val2 = tick>0?tick:60000;
			break;
		case SC_EDP:	// [Celest]
			val2 = val1 + 2; //Chance to Poison enemies.
			break;
		case SC_POISONREACT:
			val2=val1/2 + val1%2; // Number of counters [Celest]
			break;
		case SC_MAGICROD:
			val2 = val1*20; //SP gained
			break;
		case SC_KYRIE:
			val2 = status->max_hp * (val1 * 2 + 10) / 100; //%Max HP to absorb
			val3 = (val1 / 2 + 5); //Hits
			break;
		case SC_MAGICPOWER:
			//val1: Skill lv
			val2 = 1; //Lasts 1 invocation
			//val3 will store matk_min (needed in case you use ground-spells)
			//val4 will store matk_max
			break;
		case SC_SACRIFICE:
			val2 = 5; //Lasts 5 hits
			break;
		case SC_ENCPOISON:
			val2= 25+5*val1;	//Poisoning Chance (2.5+5%)
		case SC_ASPERSIO:
		case SC_FIREWEAPON:
		case SC_WATERWEAPON:
		case SC_WINDWEAPON:
		case SC_EARTHWEAPON:
		case SC_SHADOWWEAPON:
		case SC_GHOSTWEAPON:
			skill_enchant_elemental_end(bl,type);
			break;
		case SC_ELEMENTALCHANGE:
			//Val1 is skill level, val2 is skill that invoked this.
			if (!val3) //Val 3 holds the element, when not given, a random one is picked.
				val3 = rand()%ELE_MAX;
			val4 =1+rand()%4; //Elemental Lv is always a random value between  1 and 4.
			break;
		case SC_PROVIDENCE:
			val2=val1*5; //Race/Ele resist
			break;
		case SC_REFLECTSHIELD:
			val2=10+val1*3; //% Dmg reflected
			break;
		case SC_STRIPWEAPON:
			if (bl->type != BL_PC) //Watk reduction
				val2 = 5*val1;
			break;
		case SC_STRIPSHIELD:
			if (bl->type != BL_PC) //Def reduction
				val2 = 3*val1;
			break;
		case SC_STRIPARMOR:
			if (bl->type != BL_PC) //Vit reduction
				val2 = 8*val1;
			break;
		case SC_STRIPHELM:
			if (bl->type != BL_PC) //Int reduction
				val2 = 8*val1;
			break;
		case SC_AUTOSPELL:
			//Val1 Skill LV of Autospell
			//Val2 Skill ID to cast
			//Val3 Max Lv to cast
			val4 = 5 + val1*2; //Chance of casting
			break;
		case SC_VOLCANO:
			if (status->def_ele == ELE_FIRE)
				val2 = val1*10; //Watk increase
			else
				val2 = 0;
			break;
		case SC_VIOLENTGALE:
			if (status->def_ele == ELE_WIND)
				val2 = val1*3; //Flee increase
			else
				val2 = 0;
			break;
		case SC_DELUGE:
			if(status->def_ele == ELE_WATER)
				val2 = deluge_eff[val1-1]; //HP increase
			else
				val2 = 0;
			break;
		case SC_SUITON:
			if (status_get_class(bl) != JOB_NINJA) {
				//Is there some kind of formula behind this?
				switch ((val1+1)/3) {
				case 3:
					val2 = 8;
				break;
				case 2:
					val2 = 5;
				break;
				case 1:
					val2 = 3;
				break;
				case 0: 
					val2 = 0;
				break;
				default:
					val2 = 3*((val1+1)/3);
				break;
				}
			} else val2 = 0;
			break;

		case SC_SPEARQUICKEN:		/* スピアクイッケン */
			calc_flag = 1;
			val2 = 20+val1;
			break;

		case SC_MOONLIT:
			val2 = bl->id;
			skill_setmapcell(bl,CG_MOONLIT, val1, CELL_SETMOONLIT);
			break;
		case SC_DANCING:
			//val1 : Skill which is being danced.
			//val2 : Skill Group of the Dance.
			//val4 : Partner
			val3 = 0; //Tick duration/Speed penalty.
			if (sd) { //Store walk speed change in lower part of val3
				val3 = 500-40*pc_checkskill(sd,(sd->status.sex?BA_MUSICALLESSON:DC_DANCINGLESSON));
				if (sc->data[SC_SPIRIT].timer != -1 && sc->data[SC_SPIRIT].val2 == SL_BARDDANCER)
				val3 -= 40; //TODO: Figure out real bonus rate.
			}
			val3|= ((tick/1000)<<16)&0xFFFF0000; //Store tick in upper part of val3
			tick = 1000;
			break;
		case SC_LONGING:
			val2 = 50-10*val1; //Aspd/Speed penalty.
			break;
		case SC_EXPLOSIONSPIRITS:
			val2 = 75 + 25*val1; //Cri bonus
			break;
		case SC_ASPDPOTION0:
		case SC_ASPDPOTION1:
		case SC_ASPDPOTION2:
		case SC_ASPDPOTION3:
			val2 = 5*(2+type-SC_ASPDPOTION0);
			break;

		case SC_WEDDING:
		case SC_XMAS:
		{
			struct view_data *vd = status_get_viewdata(bl);
			if (!vd) return 0;
			//Store previous values as they could be removed.
			val1 = vd->class_;
			val2 = vd->weapon;
			val3 = vd->shield;
			val4 = vd->cloth_color;
			unit_stop_attack(bl);
			clif_changelook(bl,LOOK_BASE,type==SC_WEDDING?JOB_WEDDING:JOB_XMAS);
			clif_changelook(bl,LOOK_WEAPON,0);
			clif_changelook(bl,LOOK_SHIELD,0);
			clif_changelook(bl,LOOK_CLOTHES_COLOR,vd->cloth_color);
		}
			break;
		case SC_NOCHAT:
			if(!battle_config.muting_players) { 
				sd->status.manner = 0; //Zido
				return 0;
			}
			tick = 60000;
			if (sd) clif_updatestatus(sd,SP_MANNER);
			break;

		case SC_STONE:
			val2 = status->max_hp/100; //Petrified damage per second: 1%
			if (!val2) val2 = 1;
			val3 = tick/1000; //Petrified HP-damage iterations.
			if(val3 < 1) val3 = 1; 
			tick = 5000; //Petrifying time.
			break;

		case SC_DPOISON:
		//Lose 10/15% of your life as long as it doesn't brings life below 25%
		if (status->hp > status->max_hp>>2)
		{
			int diff = status->max_hp*(bl->type==BL_PC?10:15)/100;
			if (status->hp - diff < status->max_hp>>2)
				diff = status->hp - (status->max_hp>>2);
			status_zap(bl, diff, 0);
		}
		// fall through
		case SC_POISON:				/* 毒 */
		val3 = tick/1000; //Damage iterations
		if(val3 < 1) val3 = 1;
		tick = 1000;
		//val4: HP damage
		if (bl->type == BL_PC)
			val4 = (type == SC_DPOISON) ? 3 + status->max_hp/50 : 3 + status->max_hp*3/200;
		else
			val4 = (type == SC_DPOISON) ? 3 + status->max_hp/100 : 3 + status->max_hp/200;
		
		break;
		case SC_CONFUSION:
			clif_emotion(bl,1);
			break;
		case SC_BLEEDING:
			val4 = tick;
			tick = 10000;
			break;

		case SC_HIDING:
			val2 = tick/1000;
			tick = 1000;
 			//Store speed penalty on val3.
			if(sd && (val3 = pc_checkskill(sd,RG_TUNNELDRIVE))>0)
				val3 = 100 - 16*val3;
			val4 = val1+3; //Seconds before SP substraction happen.
			break;
		case SC_CHASEWALK:
			val2 = tick>0?tick:10000; //Interval at which SP is drained.
			val3 = 65+val1*5; //Speed adjustment.
			val4 = 10+val1*2; //SP cost.
			if (map_flag_gvg(bl->m)) val4 *= 5;
			break;
		case SC_CLOAKING:
			val2 = tick>0?tick:60000; //SP consumption rate.
			val3 = 0;
			if (sd && (sd->class_&MAPID_UPPERMASK) == MAPID_ASSASSIN &&
				(val3=pc_checkskill(sd,TF_MISS))>0)
				val3 *= -1; //Substract the Dodge speed bonus.
			val3+= 70+val1*3; //Speed adjustment without a wall.
			//With a wall, it is val3 +25.
			//val4&2 signals the presence of a wall.
			if (!val4)
			{ //val4&1 signals eternal cloaking (not cancelled on attack) [Skotlex]
				if (bl->type == BL_PC)	//Standard cloaking.
					val4 = battle_config.pc_cloak_check_type&2?1:0;
				else
					val4 = battle_config.monster_cloak_check_type&2?1:0;
			}
			break;
		case SC_SIGHT:			/* サイト/ルアフ */
		case SC_RUWACH:
		case SC_SIGHTBLASTER:
			val2 = tick/250;
			tick = 10;
			break;

		//Permanent effects.
		case SC_MODECHANGE:
		case SC_WEIGHT50:
		case SC_WEIGHT90:
		case SC_BROKENWEAPON:
		case SC_BROKENARMOR:
		case SC_READYSTORM: // Taekwon stances SCs [Dralnu]
		case SC_READYDOWN:
		case SC_READYCOUNTER:
		case SC_READYTURN:
		case SC_DODGE:
			tick = 600*1000;
			break;

		case SC_AUTOGUARD:
			if (!flag)
			{
				struct map_session_data *tsd;
				int i,t;
				for(i=val2=0;i<val1;i++) {
					t = 5-(i>>1);
					val2 += (t < 0)? 1:t;
				}
				if (sd)
				for (i = 0; i < 5; i++)
				{	//Pass the status to the other affected chars. [Skotlex]
					if (sd->devotion[i] && (tsd = map_id2sd(sd->devotion[i])))
						status_change_start(&tsd->bl,SC_AUTOGUARD,10000,val1,val2,0,0,tick,1);
				}
			}
			break;

		case SC_DEFENDER:
			if (!flag)
			{	
				struct map_session_data *tsd;
				int i;
				val2 = 5 + val1*15;
				if (sd)
				for (i = 0; i < 5; i++)
				{	//See if there are devoted characters, and pass the status to them. [Skotlex]
					if (sd->devotion[i] && (tsd = map_id2sd(sd->devotion[i])))
						status_change_start(&tsd->bl,SC_DEFENDER,10000,val1,5+val1*5,0,0,tick,1);
				}
			}
			break;

		case SC_TENSIONRELAX:
			if (sd) {
				pc_setsit(sd);
				clif_sitting(sd);
			}
			val2 = 12; //SP cost
			val4 = 10000; //Decrease at 10secs intervals.
			val3 = tick/val4;
			tick = val4;
			break;
		case SC_PARRYING:
		    val2 = 20 + val1*3; //Block Chance
			break;

		case SC_WINDWALK:
			val2 = (val1+1)/2; // Flee bonus is 1/1/2/2/3/3/4/4/5/5
			val3 = 4*val2;	//movement speed % increase is 4 times that
			break;

		case SC_JOINTBEAT: // Random break [DracoRPG]
			val2 = rand()%6; //Type of break
			if (val2 == 5) sc_start(bl,SC_BLEEDING,100,val1,skill_get_time2(StatusSkillChangeTable[type],val1));
			break;

		case SC_BERSERK:
			if (sc->data[SC_ENDURE].timer == -1 || !sc->data[SC_ENDURE].val4)
				sc_start4(bl, SC_ENDURE, 100,10,0,0,1, tick);
			//HP healing is performing after the calc_status call.
			if (sd) sd->canregen_tick = gettick() + 300000;
			//Val2 holds HP penalty
			if (!val4) val4 = 10000; //Val4 holds damage interval
			val3 = tick/val4; //val3 holds skill duration
			tick = val4;
			break;

		case SC_GOSPEL:
			if(val4 == BCT_SELF) {	// self effect
				val2 = tick/10000;
				tick = 10000;
				status_change_clear_buffs(bl,3); //Remove buffs/debuffs
			}
			break;

		case SC_MARIONETTE:
			if (sd) {
				val3 = 0;
				val2 = sd->status.str>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val3|=val2<<16;

				val2 = sd->status.agi>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val3|=val2<<8;

				val2 = sd->status.vit>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val3|=val2;

				val4 = 0;
				val2 = sd->status.int_>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val4|=val2<<16;

				val2 = sd->status.dex>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val4|=val2<<8;

				val2 = sd->status.luk>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val4|=val2;
			} else {
				struct status_data *b_status = status_get_base_status(bl);
				if (!b_status)
					return 0;

				val3 = 0;
				val2 = b_status->str>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val3|=val2<<16;

				val2 = b_status->agi>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val3|=val2<<8;

				val2 = b_status->vit>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val3|=val2;

				val4 = 0;
				val2 = b_status->int_>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val4|=val2<<16;

				val2 = b_status->dex>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val4|=val2<<8;

				val2 = b_status->luk>>1;
				if (val2 > 0xFF) val2 = 0xFF;
				val4|=val2;
			}
			val2 = tick/1000;
			tick = 1000;
			break;
		case SC_MARIONETTE2:
		{
			struct block_list *pbl = map_id2bl(val1);
			struct status_change *psc = pbl?status_get_sc(pbl):NULL;
			int stat,max;
			if (!psc || psc->data[SC_MARIONETTE].timer == -1)
				return 0;
			val2 = tick /1000;
			val3 = val4 = 0;
			if (sd) {
				max = pc_maxparameter(sd); //Cap to max parameter. [Skotlex]
				//Str
				stat = (psc->data[SC_MARIONETTE].val3>>16)&0xFF;
				if (sd->status.str+stat > max)
					stat =max-sd->status.str;
				val3 |= stat<<16;
				//Agi
				stat = (psc->data[SC_MARIONETTE].val3>>8)&0xFF;
				if (sd->status.agi+stat > max)
					stat =max-sd->status.agi;
				val3 |= stat<<8;
				//Vit
				stat = psc->data[SC_MARIONETTE].val3&0xFF;
				if (sd->status.vit+stat > max)
					stat =max-sd->status.vit;
				val3 |= stat;
				//Int
				stat = (psc->data[SC_MARIONETTE].val4>>16)&0xFF;
				if (sd->status.int_+stat > max)
					stat =max-sd->status.int_;
				val4 |= stat<<16;
				//Dex
				stat = (psc->data[SC_MARIONETTE].val4>>8)&0xFF;
				if (sd->status.dex+stat > max)
					stat =max-sd->status.dex;
				val4 |= stat<<8;
				//Luk
				stat = psc->data[SC_MARIONETTE].val4&0xFF;
				if (sd->status.luk+stat > max)
					stat =max-sd->status.luk;
				val4 |= stat;
			} else {
				struct status_data *status = status_get_base_status(bl);
				if (!status) return 0;
				max = 0xFF; //Assume a 256 max parameter
				//Str
				stat = (psc->data[SC_MARIONETTE].val3>>16)&0xFF;
				if (status->str+stat > max)
					stat = max - status->str;
				val3 |= stat<<16;
				//Agi
				stat = (psc->data[SC_MARIONETTE].val3>>8)&0xFF;
				if (status->agi+stat > max)
					stat = max - status->agi;
				val3 |= stat<<8;
				//Vit
				stat = psc->data[SC_MARIONETTE].val3&0xFF;
				if (status->vit+stat > max)
					stat = max - status->vit;
				val3 |= stat;
				//Int
				stat = (psc->data[SC_MARIONETTE].val4>>16)&0xFF;
				if (status->int_+stat > max)
					stat = max - status->int_;
				val4 |= stat<<16;
				//Dex
				stat = (psc->data[SC_MARIONETTE].val4>>8)&0xFF;
				if (status->dex+stat > max)
					stat = max - status->dex;
				val4 |= stat<<8;
				//Luk
				stat = psc->data[SC_MARIONETTE].val4&0xFF;
				if (status->luk+stat > max)
					stat = max - status->luk;
				val4 |= stat;
			}
			tick = 1000;
			break;
		}
		case SC_REJECTSWORD:
			val2 = 15*val1; //Reflect chance
			val3 = 3; //Reflections
			break;

		case SC_MEMORIZE:
			val2 = 5; //Memorized casts.
			break;

		case SC_GRAVITATION:
			if (val3 == BCT_SELF) {
				struct unit_data *ud = unit_bl2ud(bl);
				if (ud) {
					ud->canmove_tick += tick;
					ud->canact_tick += tick;
				}
			} 
			break;

		case SC_HERMODE:
			status_change_clear_buffs(bl,1);
			break;

		case SC_REGENERATION:
			if (val1 == 1)
				val2 = 2;
			else
				val2 = val1; //HP Regerenation rate: 200% 200% 300%
			val3 = val1; //SP Regeneration Rate: 100% 200% 300%
			break;

		case SC_DEVOTION:
		{
			struct map_session_data *src;
			if ((src = map_id2sd(val1)) && src->sc.count)
			{	//Try to inherit the status from the Crusader [Skotlex]
			//Ideally, we should calculate the remaining time and use that, but we'll trust that
			//once the Crusader's status changes, it will reflect on the others. 
				int type2 = SC_AUTOGUARD;
				if (src->sc.data[type2].timer != -1)
					sc_start(bl,type2,100,src->sc.data[type2].val1,skill_get_time(StatusSkillChangeTable[type2],src->sc.data[type2].val1));
				type2 = SC_DEFENDER;
				if (src->sc.data[type2].timer != -1)
					sc_start(bl,type2,100,src->sc.data[type2].val1,skill_get_time(StatusSkillChangeTable[type2],src->sc.data[type2].val1));
				type2 = SC_REFLECTSHIELD;
				if (src->sc.data[type2].timer != -1)
					sc_start(bl,type2,100,src->sc.data[type2].val1,skill_get_time(StatusSkillChangeTable[type2],src->sc.data[type2].val1));

			}
			break;
		}

		case SC_COMA: //Coma. Sends a char to 1HP
			status_zap(bl, status_get_hp(bl)-1, 0);
			return 1;

		case SC_CLOSECONFINE2:
		{
			struct block_list *src = val2?map_id2bl(val2):NULL;
			struct status_change *sc2 = src?status_get_sc(src):NULL;
			if (src && sc2) {
				if (sc2->data[SC_CLOSECONFINE].timer == -1) //Start lock on caster.
					sc_start4(src,SC_CLOSECONFINE,100,sc->data[type].val1,1,0,0,tick+1000);
				else { //Increase count of locked enemies and refresh time.
					sc2->data[SC_CLOSECONFINE].val2++;
					delete_timer(sc2->data[SC_CLOSECONFINE].timer, status_change_timer);
					sc2->data[SC_CLOSECONFINE].timer = add_timer(gettick()+tick+1000, status_change_timer, src->id, SC_CLOSECONFINE);
				}
			} else //Status failed.
				return 0;
		}
			break;
		case SC_KAITE:
			val2 = 1+val1/5; //Number of bounces: 1 + skilllv/5
			break;
		case SC_KAUPE:
			switch (val1) {
				case 3: //33*3 + 1 -> 100%
					val2++;
				case 1:
				case 2: //33, 66%
					val2 += 33*val1;
					val3 = 1; //Dodge 1 attack total.
					break;
				default: //Custom. For high level mob usage, higher level means more blocks. [Skotlex]
					val2 = 100;
					val3 = val1-2;
					break;
			}
			break;

		case SC_COMBO:
		{
			struct unit_data *ud = unit_bl2ud(bl);
			switch (val1) { //Val1 contains the skill id
				case TK_STORMKICK:
					clif_skill_nodamage(bl,bl,TK_READYSTORM,1,1);
					break;
				case TK_DOWNKICK:
					clif_skill_nodamage(bl,bl,TK_READYDOWN,1,1);
					break;
				case TK_TURNKICK:
					clif_skill_nodamage(bl,bl,TK_READYTURN,1,1);
					break;
				case TK_COUNTER:
					clif_skill_nodamage(bl,bl,TK_READYCOUNTER,1,1);
					break;
			}
			if (ud) {
				ud->attackabletime = gettick()+tick;
				unit_set_walkdelay(bl, gettick(), tick, 1);
			}
		}
			break;
		case SC_TKREST:
			val2 = 11-val1; //Chance to consume: 11-skilllv%
			break;
		case SC_RUN:
			val4 = gettick(); //Store time at which you started running.
			break;
		case SC_KAAHI:
			if(flag&4) {
				val4 = -1;
				break;
			}
			val2 = 200*val1; //HP heal
			val3 = 5*val1; //SP cost 
			val4 = -1;	//Kaahi Timer.
			break;
		case SC_BLESSING:
			if ((!undead_flag && status->race!=RC_DEMON) || bl->type == BL_PC)
				val2 = val1;
			else
				val2 = 0; //0 -> Half stat.
			break;
		case SC_TRICKDEAD:			/* 死んだふり */
		{
			struct view_data *vd = status_get_viewdata(bl);
			if (vd) vd->dead_sit = 1;
			break;
		}

		case SC_CONCENTRATE:
			val2 = 2 + val1;
			if (sd) { //Store the card-bonus data that should not count in the %
				val3 = sd->param_bonus[1]; //Agi
				val4 = sd->param_bonus[4]; //Dex
			} else {
				val3 = val4 = 0;
			}
			break;
		case SC_ADRENALINE2:
		case SC_ADRENALINE:
			if (val2 || !battle_config.party_skill_penalty)
				val2 = 30;
			else
				val2 = 20;
			break;
		case SC_CONCENTRATION:
			val2 = 5*val1; //Batk/Watk Increase
			val3 = 10*val1; //Hit Increase
			val4 = 5*val1; //Def reduction
			break;
		case SC_ANGELUS:
			val2 = 5*val1; //def increase
			break;
		case SC_IMPOSITIO:
			val2 = 5*val1; //watk increase
			break;
		case SC_MELTDOWN:
			val2 = 100*val1; //Chance to break weapon
			val3 = 70*val1; //Change to break armor
			break;
		case SC_TRUESIGHT:
			val2 = 10*val1; //Critical increase
			val3 = 3*val1; //Hit increase
			break;
		case SC_SUN_COMFORT:
			val2 = (status_get_lv(bl) + status->dex + status->luk)/2; //def increase
			break;
		case SC_MOON_COMFORT:
			val2 = (status_get_lv(bl) + status->dex + status->luk)/10; //luk increase
			break;
		case SC_STAR_COMFORT:
			val2 = (status_get_lv(bl) + status->dex + status->luk)/10; //Aspd increase
			break;
		case SC_QUAGMIRE:
			val2 = (sd?5:10)*val1; //Agi/Dex decrease.
			break;

		// gs_something1 [Vicious]
		case SC_GATLINGFEVER:
			val2 = 2*val1; //Aspd increase
			val3 = 5*val1; //Flee decrease
			break;

		case SC_FLING:
			val2 = 3*val1; //Def reduction
			val3 = 3*val1; //Def2 reduction
			break;
		case SC_PROVOKE:
			//val2 signals autoprovoke.
			val3 = 2+3*val1; //Atk increase
			val4 = 5+5*val1; //Def reduction.
			break;
		case SC_AVOID:
			val2 = 10*val1; //Speed change rate.
			break;
		case SC_BLOODLUST:
			val2 = 20+10*val1; //Atk rate change.
			break;
		case SC_FLEET:
			val2 = 3*val1; //Aspd change
			val3 = 5+5*val1; //Atk rate change
			break;
		default:
			if (calc_flag == SCB_NONE && StatusSkillChangeTable[type]==0)
			{	//Status change with no calc, and no skill associated...? unknown?
				if(battle_config.error_log)
					ShowError("UnknownStatusChange [%d]\n", type);
				return 0;
			}
	}
	else //Special considerations when loading SC data.
	switch (type) {
		case SC_WEDDING:
		case SC_XMAS:
			clif_changelook(bl,LOOK_BASE,type==SC_WEDDING?JOB_WEDDING:JOB_XMAS);
			clif_changelook(bl,LOOK_WEAPON,0);
			clif_changelook(bl,LOOK_SHIELD,0);
			clif_changelook(bl,LOOK_CLOTHES_COLOR,val4);
			break;	
		case SC_KAAHI:
			val4 = -1;
			break;
	}
	//Those that make you stop attacking/walking....
	switch (type) {
		case SC_FREEZE:
		case SC_STUN:
		case SC_SLEEP:
		case SC_STONE:
			if (sd && pc_issit(sd)) //Avoid sprite sync problems.
				pc_setstand(sd);
		case SC_TRICKDEAD:
			unit_stop_attack(bl);
			skill_stop_dancing(bl);
			// Cancel cast when get status [LuzZza]
			if (battle_config.sc_castcancel)
				unit_skillcastcancel(bl, 0);
		case SC_STOP:
		case SC_CONFUSION:
		case SC_CLOSECONFINE:
		case SC_CLOSECONFINE2:
		case SC_ANKLE:
		case SC_SPIDERWEB:
		case SC_MADNESSCANCEL:
			unit_stop_walking(bl,1);
		break;
		case SC_HIDING:
		case SC_CLOAKING:
		case SC_CHASEWALK:
			unit_stop_attack(bl);
		break;
	}

	if (sd)
	{ //Why must it be ONLY for players? [Skotlex]
	if (bl->prev)
		clif_status_change(bl,StatusIconChangeTable[type],1);
	else
		clif_status_load(bl,StatusIconChangeTable[type],1);
	}

	// Set option as needed.
	opt_flag = 1;
	switch(type){
		//OPT1
		case SC_STONE:
		case SC_FREEZE:
		case SC_STUN:
		case SC_SLEEP:
			if(type == SC_STONE)
				sc->opt1 = OPT1_STONEWAIT;
			else
				sc->opt1 = OPT1_STONE + (type - SC_STONE);
			break;
		//OPT2
		case SC_POISON:
		case SC_CURSE:
		case SC_SILENCE:
		case SC_BLIND:
			sc->opt2 |= 1<<(type-SC_POISON);
			break;
		case SC_DPOISON:
			sc->opt2 |= OPT2_DPOISON;
			break;
		case SC_SIGNUMCRUCIS:
			sc->opt2 |= OPT2_SIGNUMCRUCIS;
			break;
		//OPT3
		case SC_TWOHANDQUICKEN:
		case SC_SPEARQUICKEN:
		case SC_CONCENTRATION:
			sc->opt3 |= 1;
			opt_flag = 0;
			break;
		case SC_MAXOVERTHRUST:
		case SC_OVERTHRUST:
		case SC_SWOO:	//Why does it shares the same opt as Overthrust? Perhaps we'll never know...
			sc->opt3 |= 2;
			opt_flag = 0;
			break;
		case SC_ENERGYCOAT:
			sc->opt3 |= 4;
			opt_flag = 0;
			break;
		case SC_INCATKRATE:
			//Simulate Explosion Spirits effect for NPC_POWERUP [Skotlex]
			if (bl->type != BL_MOB) {
				opt_flag = 0;
				break;
			}
		case SC_EXPLOSIONSPIRITS:
			sc->opt3 |= 8;
			opt_flag = 0;
			break;
		case SC_STEELBODY:
		case SC_SKA:
			sc->opt3 |= 16;
			opt_flag = 0;
			break;
		case SC_BLADESTOP:
			sc->opt3 |= 32;
			opt_flag = 0;
			break;
		case SC_BERSERK:
			sc->opt3 |= 128;
			opt_flag = 0;
			break;
		case SC_MARIONETTE:
		case SC_MARIONETTE2:
			sc->opt3 |= 1024;
			opt_flag = 0;
			break;
		case SC_ASSUMPTIO:
			sc->opt3 |= 2048;
			opt_flag = 0;
			break;
		case SC_WARM: //SG skills [Komurka]
			sc->opt3 |= 4096;
			opt_flag = 0;
			break;
			
		//OPTION
		case SC_HIDING:
			sc->option |= OPTION_HIDE;
			break;
		case SC_CLOAKING:
			sc->option |= OPTION_CLOAK;
			break;
		case SC_CHASEWALK:
			sc->option |= OPTION_CHASEWALK|OPTION_CLOAK;
			break;
		case SC_SIGHT:
			sc->option |= OPTION_SIGHT;
			break;
		case SC_RUWACH:
			sc->option |= OPTION_RUWACH;
			break;
		case SC_WEDDING:
			sc->option |= OPTION_WEDDING;
			break;
		case SC_ORCISH:
			sc->option |= OPTION_ORCISH;
			break;
		case SC_SIGHTTRASHER:
			sc->option |= OPTION_SIGHTTRASHER;
			break;
		case SC_FUSION:
			sc->option |= OPTION_FLYING;
			break;
		default:
			opt_flag = 0;
	}

	if(opt_flag)
		clif_changeoption(bl);

	(sc->count)++;

	sc->data[type].val1 = val1;
	sc->data[type].val2 = val2;
	sc->data[type].val3 = val3;
	sc->data[type].val4 = val4;

	sc->data[type].timer = add_timer(
		gettick() + tick, status_change_timer, bl->id, type);

	if (calc_flag)
		status_calc_bl(bl,calc_flag);
	
	if(sd && sd->pd)
		pet_sc_check(sd, type); //Skotlex: Pet Status Effect Healing

	if (type==SC_BERSERK) {
		sc->data[type].val2 = 5*status->max_hp/100;
		status_heal(bl, status->max_hp, 0, 1); //Do not use percent_heal as this healing must override BERSERK's block.
		status_percent_damage(NULL, bl, 0, 100); //Damage all SP
	}

	if (type==SC_RUN) {
		struct unit_data *ud = unit_bl2ud(bl);
		if (ud)
			ud->state.running = unit_run(bl);
	}
	return 1;
}
/*==========================================
 * ステータス異常全解除
 *------------------------------------------
 */
int status_change_clear(struct block_list *bl,int type)
{
	struct status_change* sc;
	int i;

	sc = status_get_sc(bl);

	if (!sc || sc->count == 0)
		return 0;

	if(sc->data[SC_DANCING].timer != -1)
		skill_stop_dancing(bl);
	for(i = 0; i < SC_MAX; i++)
	{
		//Type 0: PC killed -> Place here stats that do not dispel on death.
		if(sc->data[i].timer == -1 ||
			(type == 0 && (
				i == SC_EDP || i == SC_MELTDOWN || i == SC_XMAS || i == SC_NOCHAT ||
				i == SC_FUSION || i == SC_TKREST || i == SC_READYSTORM ||
			  	i == SC_READYDOWN || i == SC_READYCOUNTER || i == SC_READYTURN ||
				i == SC_DODGE
			)))
			continue;

		status_change_end(bl, i, -1);

		if (type == 1 && sc->data[i].timer != -1)
		{	//If for some reason status_change_end decides to still keep the status when quitting. [Skotlex]
			(sc->count)--;
			delete_timer(sc->data[i].timer, status_change_timer);
			sc->data[i].timer = -1;
		}
	}
	sc->opt1 = 0;
	sc->opt2 = 0;
	sc->opt3 = 0;
	sc->option &= OPTION_MASK;

	if(!type || type&2)
		clif_changeoption(bl);

	return 1;
}

/*==========================================
 * ステータス異常終了
 *------------------------------------------
 */
int status_change_end( struct block_list* bl , int type,int tid )
{
	struct map_session_data *sd;
	struct status_change *sc;
	struct status_data *status;
	int opt_flag=0, calc_flag = 0;

	nullpo_retr(0, bl);
	
	sc = status_get_sc(bl);
	status = status_get_status_data(bl);
	nullpo_retr(0,sc);
	nullpo_retr(0,status);
	
	if(type < 0 || type >= SC_MAX)
		return 0;

	BL_CAST(BL_PC,bl,sd);

	if (sc->data[type].timer == -1 ||
		(sc->data[type].timer != tid && tid != -1))
		return 0;
		
	if (tid == -1)
		delete_timer(sc->data[type].timer,status_change_timer);

	sc->data[type].timer=-1;
	(sc->count)--;

	calc_flag = StatusChangeFlagTable[type];
	switch(type){
		case SC_WEDDING:
		case SC_XMAS:
		{
			struct view_data *vd = status_get_viewdata(bl);
			if (!vd) return 0;
			if (sd) //Load data from sd->status.* as the stored values could have changed.
				status_set_viewdata(bl, sd->status.class_);
			else {
				vd->class_ = sc->data[type].val1;
				vd->weapon = sc->data[type].val2;
				vd->shield = sc->data[type].val3;
				vd->cloth_color = sc->data[type].val4;
			}
			clif_changelook(bl,LOOK_BASE,vd->class_);
			clif_changelook(bl,LOOK_WEAPON,vd->weapon);
			clif_changelook(bl,LOOK_SHIELD,vd->shield);
			clif_changelook(bl,LOOK_CLOTHES_COLOR,vd->cloth_color);
		}
		break;
		case SC_RUN:
		{
			struct unit_data *ud = unit_bl2ud(bl);
			if (ud) {
				ud->state.running = 0;
				if (ud->walktimer != -1)
					unit_stop_walking(bl,1);
			}
			if (sc->data[type].val1 >= 7 &&
				DIFF_TICK(gettick(), sc->data[type].val4) <= 1000 &&
				(!sd || (sd->weapontype1 == 0 && sd->weapontype2 == 0 &&
				(sd->class_&MAPID_UPPERMASK) != MAPID_SOUL_LINKER))
			)
				sc_start(bl,SC_SPURT,100,sc->data[type].val1,skill_get_time2(StatusSkillChangeTable[type], sc->data[type].val1));
		}
		break;
		case SC_AUTOBERSERK:
			if (sc->data[SC_PROVOKE].timer != -1 && sc->data[SC_PROVOKE].val2 == 1)
				status_change_end(bl,SC_PROVOKE,-1);
			break;

		case SC_DEFENDER:
		case SC_REFLECTSHIELD:
		case SC_AUTOGUARD:
		if (sd) {
			struct map_session_data *tsd;
			int i;
			for (i = 0; i < 5; i++)
			{	//Clear the status from the others too [Skotlex]
				if (sd->devotion[i] && (tsd = map_id2sd(sd->devotion[i])) && tsd->sc.data[type].timer != -1)
					status_change_end(&tsd->bl,type,-1);
			}
		}
		break;
		case SC_DEVOTION:	
		{
			struct map_session_data *md = map_id2sd(sc->data[type].val1);
			//The status could have changed because the Crusader left the game. [Skotlex]
			if (md)
			{
				md->devotion[sc->data[type].val2] = 0;
				clif_devotion(md);
			}
			//Remove AutoGuard and Defender [Skotlex]
			if (sc->data[SC_AUTOGUARD].timer != -1)
				status_change_end(bl,SC_AUTOGUARD,-1);
			if (sc->data[SC_DEFENDER].timer != -1)
				status_change_end(bl,SC_DEFENDER,-1);
			if (sc->data[SC_REFLECTSHIELD].timer != -1)
				status_change_end(bl,SC_REFLECTSHIELD,-1);
			break;
		}
		case SC_BLADESTOP:
			if(sc->data[type].val4)
			{
				struct block_list *tbl = (struct block_list *)sc->data[type].val4;
				struct status_change *tsc = status_get_sc(tbl);
				sc->data[type].val4 = 0;
				if(tsc && tsc->data[SC_BLADESTOP].timer!=-1)
				{
					tsc->data[SC_BLADESTOP].val4 = 0;
					status_change_end(tbl,SC_BLADESTOP,-1);
				}
				clif_bladestop(bl,tbl,0);
			}
			break;
		case SC_DANCING:
			{
				struct map_session_data *dsd;
				struct status_change *dsc;
				struct skill_unit_group *group;
				if(sc->data[type].val2)
				{
					group = (struct skill_unit_group *)sc->data[type].val2;
					sc->data[type].val2 = 0;
					skill_delunitgroup(bl, group);
				}
				if(sc->data[type].val4 && sc->data[type].val4 != BCT_SELF && (dsd=map_id2sd(sc->data[type].val4))){
					dsc = &dsd->sc;
					if(dsc && dsc->data[type].timer!=-1)
					{	//This will prevent recursive loops. 
						dsc->data[type].val2 = dsc->data[type].val4 = 0;
						status_change_end(&dsd->bl, type, -1);
					}
				}
			}
			//Only dance that doesn't has ground tiles... [Skotlex]
			if(sc->data[type].val1 == CG_MOONLIT)
				status_change_end(bl, SC_MOONLIT, -1);

			if (sc->data[SC_LONGING].timer!=-1)
				status_change_end(bl,SC_LONGING,-1);				
			break;
		case SC_NOCHAT:
			if (sd && battle_config.manner_system)
			{
				//Why set it to 0? Can't we use good manners for something? [Skotlex]
//					if (sd->status.manner >= 0) // weeee ^^ [celest]
//						sd->status.manner = 0;
				clif_updatestatus(sd,SP_MANNER);
			}
			break;
		case SC_SPLASHER:	
			{
				struct block_list *src=map_id2bl(sc->data[type].val3);
				if(src && tid!=-1)
					skill_castend_damage_id(src, bl,sc->data[type].val2,sc->data[type].val1,gettick(),0 );
			}
			break;
		case SC_CLOSECONFINE2:
			{
				struct block_list *src = sc->data[type].val2?map_id2bl(sc->data[type].val2):NULL;
				struct status_change *sc2 = src?status_get_sc(src):NULL;
				if (src && sc2 && sc2->count) {
					//If status was already ended, do nothing.
					if (sc2->data[SC_CLOSECONFINE].timer != -1)
					{ //Decrease count
						if (--sc2->data[SC_CLOSECONFINE].val1 <= 0) //No more holds, free him up.
							status_change_end(src, SC_CLOSECONFINE, -1);
					}
				}
			}
		case SC_CLOSECONFINE:
			if (sc->data[type].val2 > 0) {
				//Caster has been unlocked... nearby chars need to be unlocked.
				int range = 1
					+skill_get_range2(bl, StatusSkillChangeTable[type], sc->data[type].val1)
					+skill_get_range2(bl, TF_BACKSLIDING, 1); //Since most people use this to escape the hold....
				map_foreachinarea(status_change_timer_sub, 
					bl->m, bl->x-range, bl->y-range, bl->x+range,bl->y+range,BL_CHAR,bl,sc,type,gettick());
			}
			break;
		case SC_COMBO: //Clear last used skill when it is part of a combo.
			if (sd && sd->skillid_old == sc->data[type].val1)
				sd->skillid_old = sd->skilllv_old = 0;
			break;

		case SC_FREEZE:
			sc->data[type].val3 = 0; //Clear Storm Gust hit count
			break;

		case SC_MARIONETTE:
		case SC_MARIONETTE2:	/// Marionette target
			if (sc->data[type].val1)
			{	// check for partner and end their marionette status as well
				int type2 = (type == SC_MARIONETTE) ? SC_MARIONETTE2 : SC_MARIONETTE;
				struct block_list *pbl = map_id2bl(sc->data[type].val1);
				struct status_change* sc2 = pbl?status_get_sc(pbl):NULL;
				
				if (sc2 && sc2->count && sc2->data[type2].timer != -1)
				{
					sc2->data[type2].val1 = 0;
					status_change_end(pbl, type2, -1);
				}
			}
			if (type == SC_MARIONETTE)
				clif_marionette(bl, 0); //Clear effect.
			break;

		case SC_BERSERK:
			//val4 indicates if the skill was dispelled. [Skotlex]
			if(status->hp > 100 && !sc->data[type].val4)
				status_zap(bl, status->hp-100, 0); 
			if(sc->data[SC_ENDURE].timer != -1)
				status_change_end(bl, SC_ENDURE, -1);
			break;
		case SC_GRAVITATION:
			if (sc->data[type].val3 == BCT_SELF) {
				struct unit_data *ud = unit_bl2ud(bl);
				if (ud)
					ud->canmove_tick = ud->canact_tick = gettick();
			}
			break;
		case SC_GOSPEL: //Clear the buffs from other chars.
			if (sc->data[type].val3) { //Clear the group.
				struct skill_unit_group *group = (struct skill_unit_group *)sc->data[type].val3;
				sc->data[type].val3 = 0;
				skill_delunitgroup(bl, group);
			}
			break;
		case SC_HERMODE: 
		case SC_BASILICA: //Clear the skill area. [Skotlex]
			if(sc->data[type].val3 == BCT_SELF)
				skill_clear_unitgroup(bl);
			break;
		case SC_MOONLIT: //Clear the unit effect. [Skotlex]
			skill_setmapcell(bl,CG_MOONLIT, sc->data[SC_MOONLIT].val1, CELL_CLRMOONLIT);
			break;
		case SC_TRICKDEAD:			/* 死んだふり */
		{
			struct view_data *vd = status_get_viewdata(bl);
			if (vd) vd->dead_sit = 0;
			break;
		}
		case SC_WARM:
			if (sc->data[type].val4) { //Clear the group.
				struct skill_unit_group *group = (struct skill_unit_group *)sc->data[type].val4;
				sc->data[type].val4 = 0;
				skill_delunitgroup(bl, group);
			}
			break;
		case SC_KAAHI:
			//Delete timer if it exists.
			if (sc->data[type].val4 != -1) {
				delete_timer(sc->data[type].val4,kaahi_heal_timer);
				sc->data[type].val4=-1;
			}
			break;
		}

	if (sd) 
	{	//Why must it be ONLY for players? [Skotlex]
	if (bl->prev)
		clif_status_change(bl,StatusIconChangeTable[type],0);
	else
		clif_status_load(bl,StatusIconChangeTable[type],0);
	}

	opt_flag = 1;
	switch(type){
	case SC_STONE:
	case SC_FREEZE:
	case SC_STUN:
	case SC_SLEEP:
		sc->opt1 = 0;
		break;

	case SC_POISON:
	case SC_CURSE:
	case SC_SILENCE:
	case SC_BLIND:
		sc->opt2 &= ~(1<<(type-SC_POISON));
		break;
	case SC_DPOISON:
		sc->opt2 &= ~OPT2_DPOISON;
		break;
	case SC_SIGNUMCRUCIS:
		sc->opt2 &= ~OPT2_SIGNUMCRUCIS;
		break;

	case SC_HIDING:
		sc->option &= ~OPTION_HIDE;
		break;
	case SC_CLOAKING:
		sc->option &= ~OPTION_CLOAK;
		break;
	case SC_CHASEWALK:
		sc->option &= ~(OPTION_CHASEWALK|OPTION_CLOAK);
		break;
	case SC_SIGHT:
		sc->option &= ~OPTION_SIGHT;
		break;
	case SC_WEDDING:	
		sc->option &= ~OPTION_WEDDING;
		break;
	case SC_ORCISH:
		sc->option &= ~OPTION_ORCISH;
		break;
	case SC_RUWACH:
		sc->option &= ~OPTION_RUWACH;
		break;
	case SC_SIGHTTRASHER:
		sc->option &= ~OPTION_SIGHTTRASHER;
		break;
	case SC_FUSION:
		sc->option &= ~OPTION_FLYING;
		break;
	//opt3
	case SC_TWOHANDQUICKEN:
	case SC_ONEHAND:
	case SC_SPEARQUICKEN:
	case SC_CONCENTRATION:
		sc->opt3 &= ~1;
		opt_flag = 0;
		break;
	case SC_OVERTHRUST:
	case SC_MAXOVERTHRUST:
	case SC_SWOO:
		sc->opt3 &= ~2;
		opt_flag = 0;
		break;
	case SC_ENERGYCOAT:
		sc->opt3 &= ~4;
		opt_flag = 0;
		break;
	case SC_INCATKRATE: //Simulated Explosion spirits effect.
		if (bl->type != BL_MOB)
			break;
	case SC_EXPLOSIONSPIRITS:
		sc->opt3 &= ~8;
		opt_flag = 0;
		break;
	case SC_STEELBODY:
	case SC_SKA:
		sc->opt3 &= ~16;
		opt_flag = 0;
		break;
	case SC_BLADESTOP:
		sc->opt3 &= ~32;
		opt_flag = 0;
		break;
	case SC_BERSERK:
		sc->opt3 &= ~128;
		opt_flag = 0;
		break;
	case SC_MARIONETTE:
	case SC_MARIONETTE2:
		sc->opt3 &= ~1024;
		opt_flag = 0;
		break;
	case SC_ASSUMPTIO:
		sc->opt3 &= ~2048;
		opt_flag = 0;
		break;
	case SC_WARM: //SG skills [Komurka]
		sc->opt3 &= ~4096;
		opt_flag = 0;
		break;
	default:
		opt_flag = 0;
	}

	if(opt_flag)
		clif_changeoption(bl);

	if (calc_flag)
		status_calc_bl(bl,calc_flag);

	return 1;
}

int kaahi_heal_timer(int tid, unsigned int tick, int id, int data)
{
	struct block_list *bl;
	struct status_change *sc;
	struct status_data *status;
	int hp;

	bl=map_id2bl(id);
	sc=status_get_sc(bl);
	status=status_get_status_data(bl);
	
	if (!sc || !status || data != SC_KAAHI || sc->data[data].timer==-1)
		return 0;
	if(sc->data[data].val4 != tid) {
		if (battle_config.error_log)
			ShowError("kaahi_heal_timer: Timer mismatch: %d != %d\n", tid, sc->data[data].val4);
		sc->data[data].val4=-1;
		return 0;
	}
		
	if(!status_charge(bl, 0, sc->data[data].val3)) {
		sc->data[data].val4=-1;
		return 0;
	}

	hp = status->max_hp - status->hp;
	if (hp > sc->data[data].val2)
		hp = sc->data[data].val2;
	if (hp) {
		status_heal(bl, hp, 0, 0);
		clif_skill_nodamage(NULL,bl,AL_HEAL,hp,1);
	}
	sc->data[data].val4=-1;
	return 1;
}

/*==========================================
 * ステータス異常終了タイマー
 *------------------------------------------
 */
int status_change_timer(int tid, unsigned int tick, int id, int data)
{
	int type = data;
	struct block_list *bl;
	struct map_session_data *sd=NULL;
	struct status_data *status;
	struct status_change *sc;

// security system to prevent forgetting timer removal
	int temp_timerid;

	bl=map_id2bl(id);
#ifndef _WIN32
	nullpo_retr_f(0, bl, "id=%d data=%d",id,data);
#endif
	sc=status_get_sc(bl);
	status = status_get_status_data(bl);
	
	if (!sc || !status)
	{	//Temporal debug until case is resolved. [Skotlex]
		ShowDebug("status_change_timer: Null pointer id: %d data: %d bl-type: %d\n", id, data, bl?bl->type:-1);
		return 0;
	}

	if(bl->type==BL_PC)
		sd=(struct map_session_data *)bl;

	if(sc->data[type].timer != tid) {
		if(battle_config.error_log)
			ShowError("status_change_timer: Mismatch for type %d: %d != %d (bl id %d)\n",type,tid,sc->data[type].timer, bl->id);
		return 0;
	}

	// security system to prevent forgetting timer removal
	// you shouldn't be that careless inside the switch here
	temp_timerid = sc->data[type].timer;
	sc->data[type].timer = -1;

	switch(type){	/* 特殊な?理になる場合 */
	case SC_MAXIMIZEPOWER:	/* マキシマイズパワ? */
	case SC_CLOAKING:
		if(!status_charge(bl, 0, 1))
			break; //Not enough SP to continue.
		sc->data[type].timer=add_timer(
			sc->data[type].val2+tick, status_change_timer, bl->id, data);
		return 0;

	case SC_CHASEWALK:
		if(!status_charge(bl, 0, sc->data[type].val4))
			break; //Not enough SP to continue.
			
		if (sc->data[SC_INCSTR].timer == -1) {
			sc_start(bl, SC_INCSTR,100,1<<(sc->data[type].val1-1),
				(sc->data[SC_SPIRIT].timer != -1 && sc->data[SC_SPIRIT].val2 == SL_ROGUE?10:1) //SL bonus -> x10 duration
				*skill_get_time2(StatusSkillChangeTable[type],sc->data[type].val1));
		}
		sc->data[type].timer = add_timer(
			sc->data[type].val2+tick, status_change_timer, bl->id, data);
		return 0;
	break;

	case SC_HIDING:
		if((--sc->data[type].val2)>0){
			
			if(sc->data[type].val2 % sc->data[type].val4 == 0 &&!status_charge(bl, 0, 1))
				break; //Fail if it's time to substract SP and there isn't.
		
			sc->data[type].timer=add_timer(
				1000+tick, status_change_timer,
				bl->id, data);
			return 0;
		}
	break;

	case SC_SIGHT:
	case SC_RUWACH:
	case SC_SIGHTBLASTER:
		{
			map_foreachinrange( status_change_timer_sub, bl, 
				skill_get_splash(StatusSkillChangeTable[type], sc->data[type].val1),
				BL_CHAR, bl,sc,type,tick);

			if( (--sc->data[type].val2)>0 ){
				sc->data[type].timer=add_timer(	/* タイマ?再設定 */
					250+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
		
	case SC_PROVOKE:
		if(sc->data[type].val2) { //Auto-provoke (it is ended in status_heal)
			sc->data[type].timer=add_timer(1000*60+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_ENDURE:
		if(sc->data[type].val4) { //Infinite Endure.
			sc->data[type].timer=add_timer(1000*60+tick,status_change_timer, bl->id, data);
			return 0;
		}
		break;

	case SC_STONE:
		if(sc->opt1 == OPT1_STONEWAIT) {
			sc->data[type].val4 = 0;
			unit_stop_walking(bl,1);
			sc->opt1 = OPT1_STONE;
			clif_changeoption(bl);
			sc->data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			status_calc_bl(bl, SCB_DEF_ELE);
			return 0;
		}
		if((--sc->data[type].val3) > 0) {
			if((++sc->data[type].val4)%5 == 0 && status->hp > status->max_hp>>2)
				status_zap(bl, sc->data[type].val2, 0);
			sc->data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_POISON:
		if(status->hp <= status->max_hp>>2) //Stop damaging after 25% HP left.
			break;
	case SC_DPOISON:
		if ((--sc->data[type].val3) > 0) {
			if (sc->data[SC_SLOWPOISON].timer == -1) {
				status_zap(bl, sc->data[type].val4, 0);
				if (status_isdead(bl))
					break;
			}
			sc->data[type].timer = add_timer (1000 + tick, status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_TENSIONRELAX:
		if(status->max_hp > status->hp && (--sc->data[type].val3) > 0){
			sc->data[type].timer=add_timer(
				sc->data[type].val4+tick, status_change_timer,
				bl->id, data);
			return 0;
		}
		break;
	case SC_BLEEDING:	// [celest]
		// i hope i haven't interpreted it wrong.. which i might ^^;
		// Source:
		// - 10�ｩｪｴｪﾈｪﾋHPｪｬﾊ�盒
		// - ����ｪﾎｪﾞｪﾞｫｵ?ｫﾐ�ｹﾔﾑｪ茘�ｫ�ｫｰｪｷｪﾆｪ�?ﾍ�ｪﾏ眈ｪｨｪﾊｪ､
		// To-do: bleeding effect increases damage taken?
		if ((sc->data[type].val4 -= 10000) >= 0) {
			status_fix_damage(NULL, bl, rand()%600 + 200, 0);
			if (status_isdead(bl))
				break;
			sc->data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data ); 
			return 0;
		}
		break;

	case SC_KNOWLEDGE:
	if (sd) {
		if(bl->m != sd->feel_map[0].m
			&& bl->m != sd->feel_map[1].m
			&& bl->m != sd->feel_map[2].m)
			break; //End it
	} //Otherwise continue.
	// Status changes that don't have a time limit
	case SC_AETERNA:
	case SC_TRICKDEAD:
	case SC_MODECHANGE:
	case SC_WEIGHT50:
	case SC_WEIGHT90:
	case SC_MAGICPOWER:
	case SC_REJECTSWORD:
	case SC_MEMORIZE:
	case SC_BROKENWEAPON:
	case SC_BROKENARMOR:
	case SC_SACRIFICE:
	case SC_READYSTORM:
	case SC_READYDOWN:
	case SC_READYTURN:
	case SC_READYCOUNTER:
	case SC_RUN:
	case SC_DODGE:
	case SC_AUTOBERSERK: //continues until triggered off manually. [Skotlex]
	case SC_NEN:
	case SC_SIGNUMCRUCIS:		/* シグナムクルシス */
		sc->data[type].timer=add_timer( 1000*600+tick,status_change_timer, bl->id, data );
		return 0;

	case SC_DANCING: //ダンススキルの時間SP消費
		{
			int s = 0;
			int sp = 1;
			int counter = sc->data[type].val3>>16;
			if (--counter <= 0)
				break;
			sc->data[type].val3&= 0xFFFF; //Remove counter
			sc->data[type].val3|=(counter<<16);//Reset it.
			switch(sc->data[type].val1){
				case BD_RICHMANKIM:
				case BD_DRUMBATTLEFIELD:
				case BD_RINGNIBELUNGEN:
				case BD_SIEGFRIED:
				case BA_DISSONANCE:
				case BA_ASSASSINCROSS:
				case DC_UGLYDANCE:
					s=3;
					break;
				case BD_LULLABY:
				case BD_ETERNALCHAOS:
				case BD_ROKISWEIL:
				case DC_FORTUNEKISS:
					s=4;
					break;
				case CG_HERMODE:
				case BD_INTOABYSS:
				case BA_WHISTLE:
				case DC_HUMMING:
				case BA_POEMBRAGI:
				case DC_SERVICEFORYOU:
					s=5;
					break;
				case BA_APPLEIDUN:
					s=6;
					break;
				case CG_MOONLIT:
					sp= 4*sc->data[SC_MOONLIT].val1; //Moonlit's cost is 4sp*skill_lv [Skotlex]
					//Upkeep is also every 10 secs.
				case DC_DONTFORGETME:
					s=10;
					break;
			}
			if (s && ((sc->data[type].val3 % s) == 0)) {
				if (sc->data[SC_LONGING].timer != -1)
					sp = s;
				if (!status_charge(bl, 0, sp))
					break;
			}
			sc->data[type].timer=add_timer(
				1000+tick, status_change_timer,
				bl->id, data);
			return 0;
		}
		break;

	case SC_DEVOTION:
		{	//Check range and timeleft to preserve status [Skotlex]
			//This implementation won't work for mobs because of map_id2sd, but it's a small cost in exchange of the speed of map_id2sd over map_id2sd
			struct map_session_data *md = map_id2sd(sc->data[type].val1);
			if (md && battle_check_range(bl, &md->bl, sc->data[type].val3) && (sc->data[type].val4-=1000)>0)
			{
				sc->data[type].timer = add_timer(1000+tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
		
	case SC_BERSERK:
		// 5% every 10 seconds [DracoRPG]
		if((--sc->data[type].val3)>0 && status_charge(bl, sc->data[type].val2, 0))
		{
			sc->data[type].timer = add_timer(
				sc->data[type].val4+tick, status_change_timer,
				bl->id, data);
			return 0;
		}
		else if (sd)
			sd->canregen_tick = gettick() + 300000;
		break;
	case SC_NOCHAT:
		if(sd && battle_config.manner_system){
			sd->status.manner++;
			clif_updatestatus(sd,SP_MANNER);
			if (sd->status.manner < 0)
			{	//Every 60 seconds your manner goes up by 1 until it gets back to 0.
				sc->data[type].timer=add_timer(60000+tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;

	case SC_SPLASHER:
		if (sc->data[type].val4 % 1000 == 0) {
			char timer[10];
			snprintf (timer, 10, "%d", sc->data[type].val4/1000);
			clif_message(bl, timer);
		}
		if((sc->data[type].val4 -= 500) > 0) {
			sc->data[type].timer = add_timer(
				500 + tick, status_change_timer,
				bl->id, data);
				return 0;
		}
		break;

	case SC_MARIONETTE:
	case SC_MARIONETTE2:
		{
			struct block_list *pbl = map_id2bl(sc->data[type].val1);
			if (pbl && battle_check_range(bl, pbl, 7) && (sc->data[type].val2--)>0)
			{
				sc->data[type].timer = add_timer(
					1000 + tick, status_change_timer,
					bl->id, data);
					return 0;
			}
		}
		break;

	case SC_GOSPEL:
		if(sc->data[type].val4 == BCT_SELF && (--sc->data[type].val2) > 0)
		{
			int hp, sp;
			hp = (sc->data[type].val1 > 5) ? 45 : 30;
			sp = (sc->data[type].val1 > 5) ? 35 : 20;
			if(!status_charge(bl, hp, sp))
				break;
			sc->data[type].timer = add_timer(
				10000+tick, status_change_timer,
					bl->id, data);
			return 0;
		}
		break;
		
	case SC_GUILDAURA:
		{
			struct block_list *tbl = map_id2bl(sc->data[type].val2);
			
			if (tbl && battle_check_range(bl, tbl, 2)){
				sc->data[type].timer = add_timer(
					1000 + tick, status_change_timer,
					bl->id, data);
					return 0;
			}
		}
		break;
	}

	// default for all non-handled control paths
	// security system to prevent forgetting timer removal

	// if we reach this point we need the timer for the next call, 
	// so restore it to have status_change_end handle a valid timer
	sc->data[type].timer = temp_timerid; 

	return status_change_end( bl,type,tid );
}

/*==========================================
 * ステータス異常タイマー範囲処理
 *------------------------------------------
 */
int status_change_timer_sub(struct block_list *bl, va_list ap )
{
	struct block_list *src;
	struct status_change *sc, *tsc;
	struct map_session_data* sd=NULL;
	struct map_session_data* tsd=NULL;

	int type;
	unsigned int tick;

	src=va_arg(ap,struct block_list*);
	sc=va_arg(ap,struct status_change*);
	type=va_arg(ap,int);
	tick=va_arg(ap,unsigned int);
	tsc=status_get_sc(bl);
	
	if (status_isdead(bl))
		return 0;
	if (src->type==BL_PC) sd= (struct map_session_data*)src;
	if (bl->type==BL_PC) tsd= (struct map_session_data*)bl;

	switch( type ){
	case SC_SIGHT:	/* サイト */
	case SC_CONCENTRATE:
		if (tsc && tsc->count) {
			if (tsc->data[SC_HIDING].timer != -1)
				status_change_end( bl, SC_HIDING, -1);
			if (tsc->data[SC_CLOAKING].timer != -1)
				status_change_end( bl, SC_CLOAKING, -1);
		}
		break;
	case SC_RUWACH:	/* ルアフ */
		if (tsc && tsc->count && (tsc->data[SC_HIDING].timer != -1 ||	// if the target is using a special hiding, i.e not using normal hiding/cloaking, don't bother
			tsc->data[SC_CLOAKING].timer != -1)) {
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);
			if(battle_check_target( src, bl, BCT_ENEMY ) > 0)
				skill_attack(BF_MAGIC,src,src,bl,AL_RUWACH,1,tick,0);
		}
		break;
	case SC_SIGHTBLASTER:
		{
			if (sc && sc->count && sc->data[type].val2 > 0 && battle_check_target( src, bl, BCT_ENEMY ) > 0)
			{	//sc_ check prevents a single round of Sight Blaster hitting multiple opponents. [Skotlex]
				skill_attack(BF_MAGIC,src,src,bl,WZ_SIGHTBLASTER,1,tick,0);
				sc->data[type].val2 = 0; //This signals it to end.
			}
		}
		break;
	case SC_CLOSECONFINE:
		//Lock char has released the hold on everyone...
		if (tsc && tsc->count && tsc->data[SC_CLOSECONFINE2].timer != -1 && tsc->data[SC_CLOSECONFINE2].val2 == src->id) {
			tsc->data[SC_CLOSECONFINE2].val2 = 0;
			status_change_end(bl, SC_CLOSECONFINE2, -1);
		}
		break;
	}
	return 0;
}

/*==========================================
 * Clears buffs/debuffs of a character.
 * type&1 -> buffs, type&2 -> debuffs
 *------------------------------------------
 */
int status_change_clear_buffs (struct block_list *bl, int type)
{
	int i;
	struct status_change *sc= status_get_sc(bl);

	if (!sc || !sc->count)
		return 0;

	if (type&2) //Debuffs
	for (i = SC_COMMON_MIN; i <= SC_COMMON_MAX; i++) {
		if(sc->data[i].timer != -1)
			status_change_end(bl,i,-1);
	}

	for (i = SC_COMMON_MAX+1; i < SC_MAX; i++) {

		if(sc->data[i].timer == -1)
			continue;
		
		switch (i) {
			//Stuff that cannot be removed
			case SC_WEIGHT50:
			case SC_WEIGHT90:
			case SC_COMBO:
			case SC_SMA:
			case SC_DANCING:
			case SC_GUILDAURA:
			case SC_SAFETYWALL:
			case SC_NOCHAT:
			case SC_ANKLE:
			case SC_BLADESTOP:
			case SC_CP_WEAPON:
			case SC_CP_SHIELD:
			case SC_CP_ARMOR:
			case SC_CP_HELM:
				continue;
				
			//Debuffs that can be removed.
			case SC_HALLUCINATION:
			case SC_QUAGMIRE:
			case SC_SIGNUMCRUCIS:
			case SC_DECREASEAGI:
			case SC_SLOWDOWN:
			case SC_MINDBREAKER:
			case SC_WINKCHARM:
			case SC_STOP:
			case SC_ORCISH:
			case SC_STRIPWEAPON:
			case SC_STRIPSHIELD:
			case SC_STRIPARMOR:
			case SC_STRIPHELM:
				if (!(type&2))
					continue;
				break;
			//The rest are buffs that can be removed.
			case SC_BERSERK:
				if (!(type&1))
					continue;
			  	sc->data[i].val4 = 1;
				break;
			default:
				if (!(type&1))
					continue;
				break;
		}
		status_change_end(bl,i,-1);
	}
	return 0;
}

static int status_calc_sigma(void)
{
	int i,j;
	unsigned int k;

	for(i=0;i<MAX_PC_CLASS;i++) {
		memset(hp_sigma_val[i],0,sizeof(hp_sigma_val[i]));
		for(k=0,j=2;j<=MAX_LEVEL;j++) {
			k += hp_coefficient[i]*j + 50;
			k -= k%100;
			hp_sigma_val[i][j-1] = k;
			if (k >= INT_MAX)
				break; //Overflow protection. [Skotlex]
		}
		for(;j<=MAX_LEVEL;j++)
			hp_sigma_val[i][j-1] = INT_MAX;
	}
	return 0;
}

int status_readdb(void) {
	int i,j;
	FILE *fp;
	char line[1024], path[1024],*p;

	sprintf(path, "%s/job_db1.txt", db_path);
	fp=fopen(path,"r"); // Job-specific values (weight, HP, SP, ASPD)
	if(fp==NULL){
		ShowError("can't read %s\n", path);
		return 1;
	}
	i = 0;
	while(fgets(line, sizeof(line)-1, fp)){
		char *split[MAX_WEAPON_TYPE + 5];
		i++;
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<(MAX_WEAPON_TYPE + 5) && p;j++){	//not 22 anymore [blackhole89]
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j < MAX_WEAPON_TYPE + 5)
		{	//Weapon #.MAX_WEAPON_TYPE is constantly not load. Fix to that: replace < with <= [blackhole89]
			ShowDebug("%s: Not enough columns at line %d\n", path, i);
			continue;
		}
		if(atoi(split[0])>=MAX_PC_CLASS)
			continue;
		
		max_weight_base[atoi(split[0])]=atoi(split[1]);
		hp_coefficient[atoi(split[0])]=atoi(split[2]);
		hp_coefficient2[atoi(split[0])]=atoi(split[3]);
		sp_coefficient[atoi(split[0])]=atoi(split[4]);
		for(j=0;j<MAX_WEAPON_TYPE;j++)
			aspd_base[atoi(split[0])][j]=atoi(split[j+5]);
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n",path);

	memset(job_bonus,0,sizeof(job_bonus)); // Job-specific stats bonus
	sprintf(path, "%s/job_db2.txt", db_path);
	fp=fopen(path,"r");
	if(fp==NULL){
		ShowError("can't read %s\n", path);
		return 1;
	}
	while(fgets(line, sizeof(line)-1, fp)){
       	char *split[MAX_LEVEL+1]; //Job Level is limited to MAX_LEVEL, so the bonuses should likewise be limited to it. [Skotlex]
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<MAX_LEVEL+1 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(atoi(split[0])>=MAX_PC_CLASS)
		    continue;
		for(i=1;i<j && split[i];i++)
			job_bonus[atoi(split[0])][i-1]=atoi(split[i]);
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n",path);

	// サイズ補正テ?ブル
	for(i=0;i<3;i++)
		for(j=0;j<MAX_WEAPON_TYPE;j++)
			atkmods[i][j]=100;
	sprintf(path, "%s/size_fix.txt", db_path);
	fp=fopen(path,"r");
	if(fp==NULL){
		ShowError("can't read %s\n", path);
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line)-1, fp)){
		char *split[MAX_WEAPON_TYPE];
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<MAX_WEAPON_TYPE && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
			atkmods[i][j]=atoi(split[j]);
		}
		i++;
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n",path);

	// 精?デ?タテ?ブル
	for(i=0;i<5;i++){
		for(j=0;j<MAX_REFINE; j++)
			percentrefinery[i][j]=100;
		percentrefinery[i][j]=0; //Slot MAX+1 always has 0% success chance [Skotlex]
		refinebonus[i][0]=0;
		refinebonus[i][1]=0;
		refinebonus[i][2]=10;
	}

	sprintf(path, "%s/refine_db.txt", db_path);
	fp=fopen(path,"r");
	if(fp==NULL){
		ShowError("can't read %s\n", path);
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line)-1, fp)){
		char *split[16];
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<16 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		refinebonus[i][0]=atoi(split[0]);	// 精?ボ?ナス
		refinebonus[i][1]=atoi(split[1]);	// 過?精?ボ?ナス
		refinebonus[i][2]=atoi(split[2]);	// 安全精?限界
		for(j=0;j<MAX_REFINE && split[j];j++)
			percentrefinery[i][j]=atoi(split[j+3]);
		i++;
	}
	fclose(fp); //Lupus. close this file!!!
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n",path);

	return 0;
}

/*==========================================
 * スキル関係初期化処理
 *------------------------------------------
 */
int do_init_status(void)
{
	if (SC_MAX > MAX_STATUSCHANGE)
	{
		ShowDebug("status.h defines %d status changes, but the MAX_STATUSCHANGE is %d! Fix it.\n", SC_MAX, MAX_STATUSCHANGE);
		exit(1);
	}
	add_timer_func_list(status_change_timer,"status_change_timer");
	add_timer_func_list(kaahi_heal_timer,"kaahi_heal_timer");
	initChangeTables();
	initDummyData();
	status_readdb();
	status_calc_sigma();
	return 0;
}
