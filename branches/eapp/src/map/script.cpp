// $Id: script.c 148 2004-09-30 14:05:37Z MouseJstr $
//#define DEBUG_FUNCIN
//#define DEBUG_DISP
//#define DEBUG_RUN

#include "base.h"
#include "socket.h"
#include "timer.h"
#include "malloc.h"
#include "lock.h"
#include "nullpo.h"
#include "db.h"
#include "socket.h"
#include "showmsg.h"
#include "utils.h"
#include "log.h"


#include "map.h"
#include "clif.h"
#include "chrif.h"
#include "itemdb.h"
#include "pc.h"
#include "status.h"
#include "script.h"
#include "storage.h"
#include "mob.h"
#include "npc.h"
#include "pet.h"
#include "intif.h"
#include "skill.h"
#include "chat.h"
#include "battle.h"
#include "party.h"
#include "guild.h"
#include "atcommand.h"
#include "charcommand.h"

#define SCRIPT_BLOCK_SIZE 256

enum { LABEL_NEXTLINE=1,LABEL_START };
static char * script_buf = NULL;
static int script_pos,script_size;

char *str_buf;
size_t str_pos;
size_t str_size;

static struct s_str_data {
	int type;
	int str;
	int backpatch;
	int label;
	int (*func)(CScriptEngine &);
	int val;
	int next;
} *str_data = NULL;
int str_num=LABEL_START,str_data_size;
int str_hash[16];

static struct dbt *mapreg_db=NULL;
static struct dbt *mapregstr_db=NULL;
static int mapreg_dirty=-1;
char mapreg_txt[256]="save/mapreg.txt";
#define MAPREG_AUTOSAVE_INTERVAL	(10*1000)

static struct dbt *scriptlabel_db=NULL;
static struct dbt *userfunc_db=NULL;

struct dbt* script_get_label_db(){ return scriptlabel_db; }
struct dbt* script_get_userfunc_db(){ if(!userfunc_db) userfunc_db=strdb_init(50); return userfunc_db; }

int scriptlabel_final(void *k,void *d,va_list ap){ return 0; }
static char positions[11][64] = {"��","��","����","�E��","���[�u","�C","�A�N�Z�T���[1","�A�N�Z�T���[2","��2","��3","�������Ă��Ȃ�"};

struct Script_Config script_config;

static int parse_cmd_if=0;
static int parse_cmd;

extern int current_equip_item_index; //for New CARS Scripts. It contains Inventory Index of the EQUIP_SCRIPT caller item. [Lupus]

/*==========================================
 * ���[�J���v���g�^�C�v�錾 (�K�v�ȕ��̂�)
 *------------------------------------------
 */
char* parse_subexpr(char *,int);
int buildin_mes(CScriptEngine &st);
int buildin_goto(CScriptEngine &st);
int buildin_callsub(CScriptEngine &st);
int buildin_callfunc(CScriptEngine &st);
int buildin_return(CScriptEngine &st);
int buildin_getarg(CScriptEngine &st);
int buildin_next(CScriptEngine &st);
int buildin_close(CScriptEngine &st);
int buildin_close2(CScriptEngine &st);
int buildin_menu(CScriptEngine &st);
int buildin_rand(CScriptEngine &st);
int buildin_warp(CScriptEngine &st);
int buildin_areawarp(CScriptEngine &st);
int buildin_heal(CScriptEngine &st);
int buildin_itemheal(CScriptEngine &st);
int buildin_percentheal(CScriptEngine &st);
int buildin_jobchange(CScriptEngine &st);
int buildin_input(CScriptEngine &st);
int buildin_setlook(CScriptEngine &st);
int buildin_set(CScriptEngine &st);
int buildin_setarray(CScriptEngine &st);
int buildin_cleararray(CScriptEngine &st);
int buildin_copyarray(CScriptEngine &st);
int buildin_getarraysize(CScriptEngine &st);
int buildin_deletearray(CScriptEngine &st);
int buildin_getelementofarray(CScriptEngine &st);
int buildin_if(CScriptEngine &st);
int buildin_getitem(CScriptEngine &st);
int buildin_getitem2(CScriptEngine &st);
int buildin_makeitem(CScriptEngine &st);
int buildin_delitem(CScriptEngine &st);
int buildin_viewpoint(CScriptEngine &st);
int buildin_countitem(CScriptEngine &st);
int buildin_checkweight(CScriptEngine &st);
int buildin_readparam(CScriptEngine &st);
int buildin_getcharid(CScriptEngine &st);
int buildin_getpartyname(CScriptEngine &st);
int buildin_getpartymember(CScriptEngine &st);
int buildin_getguildname(CScriptEngine &st);
int buildin_getguildmaster(CScriptEngine &st);
int buildin_getguildmasterid(CScriptEngine &st);
int buildin_strcharinfo(CScriptEngine &st);
int buildin_getequipid(CScriptEngine &st);
int buildin_getequipname(CScriptEngine &st);
int buildin_getbrokenid(CScriptEngine &st); // [Valaris]
int buildin_repair(CScriptEngine &st); // [Valaris]
int buildin_getequipisequiped(CScriptEngine &st);
int buildin_getequipisenableref(CScriptEngine &st);
int buildin_getequipisidentify(CScriptEngine &st);
int buildin_getequiprefinerycnt(CScriptEngine &st);
int buildin_getequipweaponlv(CScriptEngine &st);
int buildin_getequippercentrefinery(CScriptEngine &st);
int buildin_successrefitem(CScriptEngine &st);
int buildin_failedrefitem(CScriptEngine &st);
int buildin_cutin(CScriptEngine &st);
int buildin_cutincard(CScriptEngine &st);
int buildin_statusup(CScriptEngine &st);
int buildin_statusup2(CScriptEngine &st);
int buildin_bonus(CScriptEngine &st);
int buildin_bonus2(CScriptEngine &st);
int buildin_bonus3(CScriptEngine &st);
int buildin_bonus4(CScriptEngine &st);
int buildin_skill(CScriptEngine &st);
int buildin_addtoskill(CScriptEngine &st); // [Valaris]
int buildin_guildskill(CScriptEngine &st);
int buildin_getskilllv(CScriptEngine &st);
int buildin_getgdskilllv(CScriptEngine &st);
int buildin_basicskillcheck(CScriptEngine &st);
int buildin_getgmlevel(CScriptEngine &st);
int buildin_end(CScriptEngine &st);
int buildin_checkoption(CScriptEngine &st);
int buildin_setoption(CScriptEngine &st);
int buildin_setcart(CScriptEngine &st);
int buildin_checkcart(CScriptEngine &st); // check cart [Valaris]
int buildin_setfalcon(CScriptEngine &st);
int buildin_checkfalcon(CScriptEngine &st); // check falcon [Valaris]
int buildin_setriding(CScriptEngine &st);
int buildin_checkriding(CScriptEngine &st); // check for pecopeco [Valaris]
int buildin_savepoint(CScriptEngine &st);
int buildin_gettimetick(CScriptEngine &st);
int buildin_gettime(CScriptEngine &st);
int buildin_gettimestr(CScriptEngine &st);
int buildin_openstorage(CScriptEngine &st);
int buildin_guildopenstorage(CScriptEngine &st);
int buildin_itemskill(CScriptEngine &st);
int buildin_produce(CScriptEngine &st);
int buildin_monster(CScriptEngine &st);
int buildin_areamonster(CScriptEngine &st);
int buildin_killmonster(CScriptEngine &st);
int buildin_killmonsterall(CScriptEngine &st);
int buildin_doevent(CScriptEngine &st);
int buildin_donpcevent(CScriptEngine &st);
int buildin_addtimer(CScriptEngine &st);
int buildin_deltimer(CScriptEngine &st);
int buildin_addtimercount(CScriptEngine &st);
int buildin_initnpctimer(CScriptEngine &st);
int buildin_stopnpctimer(CScriptEngine &st);
int buildin_startnpctimer(CScriptEngine &st);
int buildin_setnpctimer(CScriptEngine &st);
int buildin_getnpctimer(CScriptEngine &st);
int buildin_attachnpctimer(CScriptEngine &st);	// [celest]
int buildin_detachnpctimer(CScriptEngine &st);	// [celest]
int buildin_announce(CScriptEngine &st);
int buildin_mapannounce(CScriptEngine &st);
int buildin_areaannounce(CScriptEngine &st);
int buildin_getusers(CScriptEngine &st);
int buildin_getmapusers(CScriptEngine &st);
int buildin_getareausers(CScriptEngine &st);
int buildin_getareadropitem(CScriptEngine &st);
int buildin_enablenpc(CScriptEngine &st);
int buildin_disablenpc(CScriptEngine &st);
int buildin_enablearena(CScriptEngine &st);	// Added by RoVeRT
int buildin_disablearena(CScriptEngine &st);	// Added by RoVeRT
int buildin_hideoffnpc(CScriptEngine &st);
int buildin_hideonnpc(CScriptEngine &st);
int buildin_sc_start(CScriptEngine &st);
int buildin_sc_start2(CScriptEngine &st);
int buildin_sc_end(CScriptEngine &st);
int buildin_getscrate(CScriptEngine &st);
int buildin_debugmes(CScriptEngine &st);
int buildin_catchpet(CScriptEngine &st);
int buildin_birthpet(CScriptEngine &st);
int buildin_resetlvl(CScriptEngine &st);
int buildin_resetstatus(CScriptEngine &st);
int buildin_resetskill(CScriptEngine &st);
int buildin_changebase(CScriptEngine &st);
int buildin_changesex(CScriptEngine &st);
int buildin_waitingroom(CScriptEngine &st);
int buildin_delwaitingroom(CScriptEngine &st);
int buildin_enablewaitingroomevent(CScriptEngine &st);
int buildin_disablewaitingroomevent(CScriptEngine &st);
int buildin_getwaitingroomstate(CScriptEngine &st);
int buildin_warpwaitingpc(CScriptEngine &st);
int buildin_attachrid(CScriptEngine &st);
int buildin_detachrid(CScriptEngine &st);
int buildin_isloggedin(CScriptEngine &st);
int buildin_setmapflagnosave(CScriptEngine &st);
int buildin_setmapflag(CScriptEngine &st);
int buildin_removemapflag(CScriptEngine &st);
int buildin_pvpon(CScriptEngine &st);
int buildin_pvpoff(CScriptEngine &st);
int buildin_gvgon(CScriptEngine &st);
int buildin_gvgoff(CScriptEngine &st);
int buildin_emotion(CScriptEngine &st);
int buildin_maprespawnguildid(CScriptEngine &st);
int buildin_agitstart(CScriptEngine &st);		// <Agit>
int buildin_agitend(CScriptEngine &st);
int buildin_agitcheck(CScriptEngine &st);  // <Agitcheck>
int buildin_flagemblem(CScriptEngine &st);		// Flag Emblem
int buildin_getcastlename(CScriptEngine &st);
int buildin_getcastledata(CScriptEngine &st);
int buildin_setcastledata(CScriptEngine &st);
int buildin_requestguildinfo(CScriptEngine &st);
int buildin_getequipcardcnt(CScriptEngine &st);
int buildin_successremovecards(CScriptEngine &st);
int buildin_failedremovecards(CScriptEngine &st);
int buildin_marriage(CScriptEngine &st);
int buildin_wedding_effect(CScriptEngine &st);
int buildin_divorce(CScriptEngine &st);
int buildin_ispartneron(CScriptEngine &st); // MouseJstr
int buildin_getpartnerid(CScriptEngine &st); // MouseJstr
int buildin_getchildid(CScriptEngine &st); // Skotlex
int buildin_warppartner(CScriptEngine &st); // MouseJstr
int buildin_getitemname(CScriptEngine &st);
int buildin_makepet(CScriptEngine &st);
int buildin_getexp(CScriptEngine &st);
int buildin_getinventorylist(CScriptEngine &st);
int buildin_getskilllist(CScriptEngine &st);
int buildin_clearitem(CScriptEngine &st);
int buildin_classchange(CScriptEngine &st);
int buildin_misceffect(CScriptEngine &st);
int buildin_soundeffect(CScriptEngine &st);
int buildin_soundeffectall(CScriptEngine &st);
int buildin_setcastledata(CScriptEngine &st);
int buildin_mapwarp(CScriptEngine &st);
int buildin_inittimer(CScriptEngine &st);
int buildin_stoptimer(CScriptEngine &st);
int buildin_cmdothernpc(CScriptEngine &st);
int buildin_mobcount(CScriptEngine &st);
int buildin_strmobinfo(CScriptEngine &st); // Script for displaying mob info [Valaris]
int buildin_guardian(CScriptEngine &st); // Script for displaying mob info [Valaris]
int buildin_guardianinfo(CScriptEngine &st); // Script for displaying mob info [Valaris]
int buildin_petskillbonus(CScriptEngine &st); // petskillbonus [Valaris]
int buildin_petrecovery(CScriptEngine &st); // pet skill for curing status [Valaris]
int buildin_petloot(CScriptEngine &st); // pet looting [Valaris]
int buildin_petheal(CScriptEngine &st); // pet healing [Valaris]
//int buildin_petmag(CScriptEngine &st); // pet magnificat [Valaris]
int buildin_petskillattack(CScriptEngine &st); // pet skill attacks [Skotlex]
int buildin_petskillattack2(CScriptEngine &st); // pet skill attacks [Skotlex]
int buildin_petskillsupport(CScriptEngine &st); // pet support skill [Valaris]
int buildin_skilleffect(CScriptEngine &st); // skill effects [Celest]
int buildin_npcskilleffect(CScriptEngine &st); // skill effects for npcs [Valaris]
int buildin_specialeffect(CScriptEngine &st); // special effect script [Valaris]
int buildin_specialeffect2(CScriptEngine &st); // special effect script [Valaris]
int buildin_nude(CScriptEngine &st); // nude [Valaris]
int buildin_gmcommand(CScriptEngine &st); // [MouseJstr]
int buildin_atcommand(CScriptEngine &st); // [MouseJstr]
int buildin_charcommand(CScriptEngine &st); // [MouseJstr]
int buildin_movenpc(CScriptEngine &st); // [MouseJstr]
int buildin_message(CScriptEngine &st); // [MouseJstr]
int buildin_npctalk(CScriptEngine &st); // [Valaris]
int buildin_hasitems(CScriptEngine &st); // [Valaris]
int buildin_getlook(CScriptEngine &st);	//Lorky [Lupus]
int buildin_getsavepoint(CScriptEngine &st);	//Lorky [Lupus]
int buildin_npcspeed(CScriptEngine &st); // [Valaris]
int buildin_npcwalkto(CScriptEngine &st); // [Valaris]
int buildin_npcstop(CScriptEngine &st); // [Valaris]
int buildin_getmapxy(CScriptEngine &st);  //get map position for player/npc/pet/mob by Lorky [Lupus]
int buildin_checkoption1(CScriptEngine &st); // [celest]
int buildin_checkoption2(CScriptEngine &st); // [celest]
int buildin_guildgetexp(CScriptEngine &st); // [celest]
int buildin_skilluseid(CScriptEngine &st); // originally by Qamera [celest]
int buildin_skillusepos(CScriptEngine &st); // originally by Qamera [celest]
int buildin_logmes(CScriptEngine &st); // [Lupus]
int buildin_summon(CScriptEngine &st); // [celest]
int buildin_isnight(CScriptEngine &st); // [celest]
int buildin_isday(CScriptEngine &st); // [celest]
int buildin_isequipped(CScriptEngine &st); // [celest]
int buildin_isequippedcnt(CScriptEngine &st); // [celest]
int buildin_cardscnt(CScriptEngine &st); // [Lupus]
int buildin_getrefine(CScriptEngine &st); // [celest]
int buildin_adopt(CScriptEngine &st);
int buildin_night(CScriptEngine &st);
int buildin_day(CScriptEngine &st);
int buildin_getusersname(CScriptEngine &st); //jA commands added [Lupus]
int buildin_dispbottom(CScriptEngine &st);
int buildin_recovery(CScriptEngine &st);
int buildin_getpetinfo(CScriptEngine &st);
int buildin_checkequipedcard(CScriptEngine &st);
int buildin_globalmes(CScriptEngine &st);
int buildin_jump_zero(CScriptEngine &st);
int buildin_select(CScriptEngine &st);
int buildin_getmapmobs(CScriptEngine &st); //jA addition end
int buildin_getstrlen(CScriptEngine &st); //strlen [valaris]
int buildin_charisalpha(CScriptEngine &st);//isalpha [valaris]
int buildin_fakenpcname(CScriptEngine &st); // [Lance]

int buildin_defpattern(CScriptEngine &st); // MouseJstr
int buildin_activatepset(CScriptEngine &st); // MouseJstr
int buildin_deactivatepset(CScriptEngine &st); // MouseJstr
int buildin_deletepset(CScriptEngine &st); // MouseJstr

int buildin_unequip(CScriptEngine &st); // unequip [Spectre]

int buildin_pcstrcharinfo(CScriptEngine &st);
int buildin_getnameditem(CScriptEngine &st);
int buildin_compare(CScriptEngine &st);
int buildin_warpparty(CScriptEngine &st);
int buildin_warpguild(CScriptEngine &st);
int buildin_pc_emotion(CScriptEngine &st);



int mapreg_setreg(int num,int val);
int mapreg_setregstr(int num,const char *str);


struct {
	int (*func)(CScriptEngine &);
	char *name;
	char *arg;
} buildin_func[]={
	{buildin_mes,"mes","s"},
	{buildin_next,"next",""},
	{buildin_close,"close",""},
	{buildin_close2,"close2",""},
	{buildin_menu,"menu","*"},
	{buildin_goto,"goto","l"},
	{buildin_callsub,"callsub","i*"},
	{buildin_callfunc,"callfunc","s*"},
	{buildin_return,"return","*"},
	{buildin_getarg,"getarg","i"},
	{buildin_jobchange,"jobchange","i*"},
	{buildin_input,"input","*"},
	{buildin_warp,"warp","sii"},
	{buildin_areawarp,"areawarp","siiiisii"},
	{buildin_setlook,"setlook","ii"},
	{buildin_set,"set","ii"},
	{buildin_setarray,"setarray","ii*"},
	{buildin_cleararray,"cleararray","iii"},
	{buildin_copyarray,"copyarray","iii"},
	{buildin_getarraysize,"getarraysize","i"},
	{buildin_deletearray,"deletearray","ii"},
	{buildin_getelementofarray,"getelementofarray","ii"},
	{buildin_if,"if","i*"},
	{buildin_getitem,"getitem","ii**"},
	{buildin_getitem2,"getitem2","iiiiiiiii*"},
	{buildin_getnameditem,"getnameditem","is"},
	{buildin_makeitem,"makeitem","iisii"},
	{buildin_delitem,"delitem","ii"},
	{buildin_cutin,"cutin","si"},
	{buildin_cutincard,"cutincard","i"},
	{buildin_viewpoint,"viewpoint","iiiii"},
	{buildin_heal,"heal","ii"},
	{buildin_itemheal,"itemheal","ii"},
	{buildin_percentheal,"percentheal","ii"},
	{buildin_rand,"rand","i*"},
	{buildin_countitem,"countitem","i"},
	{buildin_checkweight,"checkweight","ii"},
	{buildin_readparam,"readparam","i*"},
	{buildin_getcharid,"getcharid","i*"},
	{buildin_getpartyname,"getpartyname","i"},
	{buildin_getpartymember,"getpartymember","i"},
	{buildin_getguildname,"getguildname","i"},
	{buildin_getguildmaster,"getguildmaster","i"},
	{buildin_getguildmasterid,"getguildmasterid","i"},
	{buildin_strcharinfo,"strcharinfo","i"},
	{buildin_getequipid,"getequipid","i"},
	{buildin_getequipname,"getequipname","i"},
	{buildin_getbrokenid,"getbrokenid","i"}, // [Valaris]
	{buildin_repair,"repair","i"}, // [Valaris]
	{buildin_getequipisequiped,"getequipisequiped","i"},
	{buildin_getequipisenableref,"getequipisenableref","i"},
	{buildin_getequipisidentify,"getequipisidentify","i"},
	{buildin_getequiprefinerycnt,"getequiprefinerycnt","i"},
	{buildin_getequipweaponlv,"getequipweaponlv","i"},
	{buildin_getequippercentrefinery,"getequippercentrefinery","i"},
	{buildin_successrefitem,"successrefitem","i"},
	{buildin_failedrefitem,"failedrefitem","i"},
	{buildin_statusup,"statusup","i"},
	{buildin_statusup2,"statusup2","ii"},
	{buildin_bonus,"bonus","ii"},
	{buildin_bonus2,"bonus2","iii"},
	{buildin_bonus3,"bonus3","iiii"},
	{buildin_bonus4,"bonus4","iiiii"},
	{buildin_skill,"skill","ii*"},
	{buildin_addtoskill,"addtoskill","ii*"}, // [Valaris]
	{buildin_guildskill,"guildskill","ii"},
	{buildin_getskilllv,"getskilllv","i"},
	{buildin_getgdskilllv,"getgdskilllv","ii"},
	{buildin_basicskillcheck,"basicskillcheck","*"},
	{buildin_getgmlevel,"getgmlevel","*"},
	{buildin_end,"end",""},
	{buildin_end,"break",""},
	{buildin_checkoption,"checkoption","i"},
	{buildin_setoption,"setoption","i"},
	{buildin_setcart,"setcart",""},
	{buildin_checkcart,"checkcart","*"},		//fixed by Lupus (added '*')
	{buildin_setfalcon,"setfalcon",""},
	{buildin_checkfalcon,"checkfalcon","*"},	//fixed by Lupus (fixed wrong pointer, added '*')
	{buildin_setriding,"setriding",""},
	{buildin_checkriding,"checkriding","*"},	//fixed by Lupus (fixed wrong pointer, added '*')
	{buildin_savepoint,"save","sii"},
	{buildin_savepoint,"savepoint","sii"},
	{buildin_gettimetick,"gettimetick","i"},
	{buildin_gettime,"gettime","i"},
	{buildin_gettimestr,"gettimestr","si"},
	{buildin_openstorage,"openstorage",""},
	{buildin_guildopenstorage,"guildopenstorage","*"},
	{buildin_itemskill,"itemskill","iis"},
	{buildin_produce,"produce","i"},
	{buildin_monster,"monster","siisii*"},
	{buildin_areamonster,"areamonster","siiiisii*"},
	{buildin_killmonster,"killmonster","ss"},
	{buildin_killmonsterall,"killmonsterall","s"},
	{buildin_doevent,"doevent","s"},
	{buildin_donpcevent,"donpcevent","s"},
	{buildin_addtimer,"addtimer","is"},
	{buildin_deltimer,"deltimer","s"},
	{buildin_addtimercount,"addtimercount","si"},
	{buildin_initnpctimer,"initnpctimer","*"},
	{buildin_stopnpctimer,"stopnpctimer","*"},
	{buildin_startnpctimer,"startnpctimer","*"},
	{buildin_setnpctimer,"setnpctimer","*"},
	{buildin_getnpctimer,"getnpctimer","i*"},
	{buildin_attachnpctimer,"attachnpctimer","*"}, // attached the player id to the npc timer [Celest]
	{buildin_detachnpctimer,"detachnpctimer","*"}, // detached the player id from the npc timer [Celest]
	{buildin_announce,"announce","si"},
	{buildin_mapannounce,"mapannounce","ssi"},
	{buildin_areaannounce,"areaannounce","siiiisi"},
	{buildin_getusers,"getusers","i"},
	{buildin_getmapusers,"getmapusers","s"},
	{buildin_getareausers,"getareausers","siiii"},
	{buildin_getareadropitem,"getareadropitem","siiiii"},
	{buildin_enablenpc,"enablenpc","s"},
	{buildin_disablenpc,"disablenpc","s"},
	{buildin_enablearena,"enablearena",""},		// Added by RoVeRT
	{buildin_disablearena,"disablearena",""},	// Added by RoVeRT
	{buildin_hideoffnpc,"hideoffnpc","s"},
	{buildin_hideonnpc,"hideonnpc","s"},
	{buildin_sc_start,"sc_start","iii*"},
	{buildin_sc_start2,"sc_start2","iiii*"},
	{buildin_sc_end,"sc_end","i"},
	{buildin_getscrate,"getscrate","ii*"},
	{buildin_debugmes,"debugmes","s"},
	{buildin_catchpet,"pet","i"},
	{buildin_birthpet,"bpet",""},
	{buildin_resetlvl,"resetlvl","i"},
	{buildin_resetstatus,"resetstatus",""},
	{buildin_resetskill,"resetskill",""},
	{buildin_changebase,"changebase","i"},
	{buildin_changesex,"changesex",""},
	{buildin_waitingroom,"waitingroom","si*"},
	{buildin_warpwaitingpc,"warpwaitingpc","sii"},
	{buildin_delwaitingroom,"delwaitingroom","*"},
	{buildin_enablewaitingroomevent,"enablewaitingroomevent","*"},
	{buildin_disablewaitingroomevent,"disablewaitingroomevent","*"},
	{buildin_getwaitingroomstate,"getwaitingroomstate","i*"},
	{buildin_warpwaitingpc,"warpwaitingpc","sii*"},
	{buildin_attachrid,"attachrid","i"},
	{buildin_detachrid,"detachrid",""},
	{buildin_isloggedin,"isloggedin","i"},
	{buildin_setmapflagnosave,"setmapflagnosave","ssii"},
	{buildin_setmapflag,"setmapflag","si"},
	{buildin_removemapflag,"removemapflag","si"},
	{buildin_pvpon,"pvpon","s"},
	{buildin_pvpoff,"pvpoff","s"},
	{buildin_gvgon,"gvgon","s"},
	{buildin_gvgoff,"gvgoff","s"},
	{buildin_emotion,"emotion","i"},
	{buildin_maprespawnguildid,"maprespawnguildid","sii"},
	{buildin_agitstart,"agitstart",""},	// <Agit>
	{buildin_agitend,"agitend",""},
	{buildin_agitcheck,"agitcheck","i"},   // <Agitcheck>
	{buildin_flagemblem,"flagemblem","i"},	// Flag Emblem
	{buildin_getcastlename,"getcastlename","s"},
	{buildin_getcastledata,"getcastledata","si*"},
	{buildin_setcastledata,"setcastledata","sii"},
	{buildin_requestguildinfo,"requestguildinfo","i*"},
	{buildin_getequipcardcnt,"getequipcardcnt","i"},
	{buildin_successremovecards,"successremovecards","i"},
	{buildin_failedremovecards,"failedremovecards","ii"},
	{buildin_marriage,"marriage","s"},
	{buildin_wedding_effect,"wedding",""},
	{buildin_divorce,"divorce","*"},
	{buildin_ispartneron,"ispartneron","*"},
	{buildin_getpartnerid,"getpartnerid","*"},
	{buildin_getchildid,"getchildid",""},
	{buildin_warppartner,"warppartner","sii"},
	{buildin_getitemname,"getitemname","i"},
	{buildin_makepet,"makepet","i"},
	{buildin_getexp,"getexp","ii"},
	{buildin_getinventorylist,"getinventorylist",""},
	{buildin_getskilllist,"getskilllist",""},
	{buildin_clearitem,"clearitem",""},
	{buildin_classchange,"classchange","ii"},
	{buildin_misceffect,"misceffect","i"},
	{buildin_soundeffect,"soundeffect","si"},
	{buildin_soundeffectall,"soundeffectall","si"},	// SoundEffectAll [Codemaster]
	{buildin_strmobinfo,"strmobinfo","ii"},	// display mob data [Valaris]
	{buildin_guardian,"guardian","siisii*i"},	// summon guardians
	{buildin_guardianinfo,"guardianinfo","i"},	// display guardian data [Valaris]
	{buildin_petskillbonus,"petskillbonus","iiii"}, // [Valaris]
	{buildin_petrecovery,"petrecovery","ii"}, // [Valaris]
	{buildin_petloot,"petloot","i"}, // [Valaris]
	{buildin_petheal,"petheal","iiii"}, // [Valaris]
//	{buildin_petmag,"petmag","iiii"}, // [Valaris]
	{buildin_petskillattack,"petskillattack","iiii"}, // [Skotlex]
	{buildin_petskillattack2,"petskillattack2","iiiii"}, // [Valaris]
	{buildin_petskillsupport,"petskillsupport","iiiii"}, // [Skotlex]
	{buildin_skilleffect,"skilleffect","ii"}, // skill effect [Celest]
	{buildin_npcskilleffect,"npcskilleffect","iiii"}, // npc skill effect [Valaris]
	{buildin_specialeffect,"specialeffect","i"}, // npc skill effect [Valaris]
	{buildin_specialeffect2,"specialeffect2","i"}, // skill effect on players[Valaris]
	{buildin_nude,"nude",""}, // nude command [Valaris]
	{buildin_mapwarp,"mapwarp","ssii"},		// Added by RoVeRT
	{buildin_inittimer,"inittimer",""},
	{buildin_stoptimer,"stoptimer",""},
	{buildin_cmdothernpc,"cmdothernpc","ss"},
	{buildin_atcommand,"atcommand","*"}, // [MouseJstr]
	{buildin_charcommand,"charcommand","*"}, // [MouseJstr]
	{buildin_movenpc,"movenpc","siis"}, // [MouseJstr]
	{buildin_message,"message","s*"}, // [MouseJstr]
	{buildin_npctalk,"npctalk","*"}, // [Valaris]
	{buildin_hasitems,"hasitems","*"}, // [Valaris]
	{buildin_mobcount,"mobcount","ss"},
	{buildin_getlook,"getlook","i"},
	{buildin_getsavepoint,"getsavepoint","i"},
	{buildin_npcspeed,"npcspeed","i"}, // [Valaris]
	{buildin_npcwalkto,"npcwalkto","ii"}, // [Valaris]
	{buildin_npcstop,"npcstop",""}, // [Valaris]
	{buildin_getmapxy,"getmapxy","siii*"},	//by Lorky [Lupus]
	{buildin_checkoption1,"checkoption1","i"},
	{buildin_checkoption2,"checkoption2","i"},
	{buildin_guildgetexp,"guildgetexp","i"},
	{buildin_skilluseid,"skilluseid","ii"}, // originally by Qamera [Celest]
	{buildin_skilluseid,"doskill","ii"}, // since a lot of scripts would already use 'doskill'...
	{buildin_skillusepos,"skillusepos","iiii"}, // [Celest]
	{buildin_logmes,"logmes","s"}, //this command actls as MES but prints info into LOG file either SQL/TXT [Lupus]
	{buildin_summon,"summon","si*"}, // summons a slave monster [Celest]
	{buildin_isnight,"isnight",""}, // check whether it is night time [Celest]
	{buildin_isday,"isday",""}, // check whether it is day time [Celest]
	{buildin_isequipped,"isequipped","i*"}, // check whether another item/card has been equipped [Celest]
	{buildin_isequippedcnt,"isequippedcnt","i*"}, // check how many items/cards are being equipped [Celest]
	{buildin_cardscnt,"cardscnt","i*"}, // check how many items/cards are being equipped in the same arm [Lupus]
	{buildin_getrefine,"getrefine","*"}, // returns the refined number of the current item, or an item with index specified [celest]
	{buildin_adopt,"adopt","sss"}, // allows 2 parents to adopt a child
	{buildin_night,"night",""}, // sets the server to night time
	{buildin_day,"day",""}, // sets the server to day time
	{buildin_defpattern, "defpattern", "iss"}, // Define pattern to listen for [MouseJstr]
	{buildin_activatepset, "activatepset", "i"}, // Activate a pattern set [MouseJstr]
	{buildin_deactivatepset, "deactivatepset", "i"}, // Deactive a pattern set [MouseJstr]
	{buildin_deletepset, "deletepset", "i"}, // Delete a pattern set [MouseJstr]
	{buildin_dispbottom,"dispbottom","s"}, //added from jA [Lupus]
	{buildin_getusersname,"getusersname","*"},
	{buildin_recovery,"recovery",""},
	{buildin_getpetinfo,"getpetinfo","i"},
	{buildin_checkequipedcard,"checkequipedcard","i"},
	{buildin_jump_zero,"jump_zero","ii"}, //for future jA script compatibility
	{buildin_select,"select","*"}, //for future jA script compatibility
	{buildin_globalmes,"globalmes","s*"},
	{buildin_getmapmobs,"getmapmobs","s"}, //end jA addition
	{buildin_unequip,"unequip","i"}, // unequip command [Spectre]
	{buildin_pcstrcharinfo,"pcstrcharinfo","ii"},
	{buildin_getstrlen,"getstrlen","s"}, //strlen [Valaris]
	{buildin_charisalpha,"charisalpha","si"}, //isalpha [Valaris]
	{buildin_fakenpcname,"fakenpcname","ssi"}, // [Lance]
	{buildin_compare,"compare","ss"},
	{buildin_warpparty,"warpparty","siii"},
	{buildin_warpguild,"warpguild","siii"},
	{buildin_pc_emotion,"pc_emotion","i"},

	{NULL,NULL,NULL},
};



///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Script Parser Implementation
//
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

/*==========================================
 * ������̃n�b�V�����v�Z
 *------------------------------------------
 */
unsigned char calc_hash(const char *str)
{
	size_t h=0;
	if(str)
	{	// we need it unsigned here
		const unsigned char *p = (const unsigned char *)str;
		while(*p)
		{
			h=(h<<1)+(h>>3)+(h>>5)+(h>>8);
			h+=*p++;
		}
	}
	return h&0xF;
}

/*==========================================
 * str_data�̒��ɖ��O�����邩��������
 *------------------------------------------
 */
// �����̂ł���Δԍ��A�������-1
int search_str(const char *p)
{
	int i;
	i=str_hash[calc_hash(p)];
	while(i)
	{
		if(strcasecmp(str_buf+str_data[i].str,p)==0)
			return i;
		i=str_data[i].next;
	}
	return -1;
}

/*==========================================
 * str_data�ɖ��O��o�^
 *------------------------------------------
 */
// �����̂ł���Δԍ��A������Γo�^���ĐV�K�ԍ�
int add_str(const char *p)
{
	int i;
	// maybe a fixed size buffer would be sufficient
	char *lowcase;

	if(NULL==p) return -1; // should not happen

	lowcase=(char *)aMalloc((strlen(p)+1)*sizeof(char));
	strcpytolower(lowcase, p);
	i=search_str(lowcase);
	aFree(lowcase);

	if(i >= 0)
		return i;

	i=calc_hash(p);
	if(str_hash[i]==0){
		str_hash[i]=str_num;
	} else {
		i=str_hash[i];
		for(;;){
			if(strcmp(str_buf+str_data[i].str,p)==0){
				return i;
			}
			if(str_data[i].next==0)
				break;
			i=str_data[i].next;
		}
		str_data[i].next=str_num;
	}
	if(str_num>=str_data_size){
		str_data_size+=128;
		str_data=(struct s_str_data*)aRealloc(str_data,str_data_size*sizeof(struct s_str_data));
		memset(str_data + (str_data_size - 128), 0, 128*sizeof(struct s_str_data));
	}
	while(str_pos+(int)strlen(p)+1>=str_size){
		str_size+=256;
		str_buf=(char *)aRealloc(str_buf,str_size*sizeof(char));
		memset(str_buf + (str_size - 256), 0, 256*sizeof(char));
	}

	memcpy(str_buf+str_pos,p,strlen(p)+1);
	str_data[str_num].type=CScriptEngine::C_NOP;
	str_data[str_num].str=str_pos;
	str_data[str_num].next=0;
	str_data[str_num].func=NULL;
	str_data[str_num].backpatch=-1;
	str_data[str_num].label=-1;
	str_pos += (int)strlen(p)+1;
	return str_num++;
}


/*==========================================
 * �X�N���v�g�o�b�t�@�T�C�Y�̊m�F�Ɗg��
 *------------------------------------------
 */
void check_script_buf(int size)
{
	if(script_pos+size>=script_size)
	{
		script_size+=SCRIPT_BLOCK_SIZE;
		script_buf=(char *)aRealloc(script_buf,script_size*sizeof(char));
		memset(script_buf + (script_size - SCRIPT_BLOCK_SIZE), 0, SCRIPT_BLOCK_SIZE*sizeof(char));
	}
}

/*==========================================
 * �X�N���v�g�o�b�t�@�ɂP�o�C�g��������
 *------------------------------------------
 */
void add_scriptb(int a)
{
	check_script_buf(1);
	script_buf[script_pos++]=a;
}

/*==========================================
 * �X�N���v�g�o�b�t�@�Ƀf�[�^�^�C�v����������
 *------------------------------------------
 */
void add_scriptc(int a)
{
	while(a>=0x40){
		add_scriptb((a&0x3f)|0x40);
		a=(a-0x40)>>6;
	}
	add_scriptb(a&0x3f);
}

/*==========================================
 * �X�N���v�g�o�b�t�@�ɐ�������������
 *------------------------------------------
 */
void add_scripti(int a)
{
	while(a>=0x40){
		add_scriptb(a|0xc0);
		a=(a-0x40)>>6;
	}
	add_scriptb(a|0x80);
}

/*==========================================
 * �X�N���v�g�o�b�t�@�Ƀ��x��/�ϐ�/�֐�����������
 *------------------------------------------
 */
// �ő�16M�܂�
void add_scriptl(int l)
{
	int backpatch = str_data[l].backpatch;

	switch(str_data[l].type){
	case CScriptEngine::C_POS:
		add_scriptc(CScriptEngine::C_POS);
		add_scriptb(str_data[l].label);
		add_scriptb(str_data[l].label>>8);
		add_scriptb(str_data[l].label>>16);
		break;
	case CScriptEngine::C_NOP:
		// ���x���̉\��������̂�backpatch�p�f�[�^���ߍ���
		add_scriptc(CScriptEngine::C_NAME);
		str_data[l].backpatch=script_pos;
		add_scriptb(backpatch);
		add_scriptb(backpatch>>8);
		add_scriptb(backpatch>>16);
		break;
	case CScriptEngine::C_INT:
		add_scripti(str_data[l].val);
		break;
	default:
		// �������̗p�r�Ɗm�肵�Ă�̂Ő��������̂܂�
		add_scriptc(CScriptEngine::C_NAME);
		add_scriptb(l);
		add_scriptb(l>>8);
		add_scriptb(l>>16);
		break;
	}
}

/*==========================================
 * ���x������������
 *------------------------------------------
 */
void set_label(int l,int pos)
{
	int i,next;

	str_data[l].type=CScriptEngine::C_POS;
	str_data[l].label=pos;
	for(i=str_data[l].backpatch;i>=0 && i!=0x00ffffff;){
		next = (0xFF&script_buf[i])
			 | (0xFF&script_buf[i+1])<<8
			 | (0xFF&script_buf[i+2])<<16;
		script_buf[i-1]=CScriptEngine::C_POS;
		script_buf[i]=pos;
		script_buf[i+1]=pos>>8;
		script_buf[i+2]=pos>>16;
		i=next;
	}
}

/*==========================================
 * �X�y�[�X/�R�����g�ǂݔ�΂�
 *------------------------------------------
 */
char *skip_space(const char *p)
{
	while(1){
		while( *p==0x20 || (*p>=0x09 && *p<=0x0D) )
			p++;
		if(p[0]=='/' && p[1]=='/'){
			while(*p && *p!='\n')
				p++;
		} else if(p[0]=='/' && p[1]=='*'){
			p++;
			while(*p && (p[-1]!='*' || p[0]!='/'))
				p++;
			if(*p) p++;
		} else
			break;
	}
	return (char*)p;
}

/*==========================================
 * �P�P��X�L�b�v
 *------------------------------------------
 */
char *skip_word(const char *str)
{
	unsigned char*p =(unsigned char*)str;
	if(p)
	{	// prefix
		if(*p=='$') p++;	// MAP�I�����L�ϐ��p
		if(*p=='@') p++;	// �ꎞ�I�ϐ��p(like weiss)
		if(*p=='#') p++;	// account�ϐ��p
		if(*p=='#') p++;	// ���[���haccount�ϐ��p
		if(*p=='l') p++;	// �ꎞ�I�ϐ��p(like weiss)

		// this is for skipping multibyte characters
		// I do not modify here but just set the string pointer back to unsigned
		while(isalnum((int)((unsigned char)*p))||*p=='_'|| *p>=0x81)
		{
			if(*p>=0x81 && p[1])
				p+=2;
			else
				p++;
		}

		// postfix
		if(*p=='$') p++;	// ������ϐ�
	}
	return (char*)p;
}

static char *startptr;
static int startline;

/*==========================================
 * �G���[���b�Z�[�W�o��
 *------------------------------------------
 */
void disp_error_message(const char *mes,const char *pos)
{
	int line,c=0,i;
	char *p,*linestart,*lineend;

	for(line=startline,p=startptr;p && *p;line++){
		linestart=p;
		lineend=strchr(p,'\n');
		if(lineend){
			c=*lineend;
			*lineend=0;
		}
		if(lineend==NULL || pos<lineend){
			ShowMessage("%s line "CL_WHITE"\'%d\'"CL_RESET" : ", mes, line);
			for(i=0;(linestart[i]!='\r') && (linestart[i]!='\n') && linestart[i];i++){
				if(linestart+i!=pos)
					ShowMessage("%c",linestart[i]);
				else
					ShowMessage("\'%c\'",linestart[i]);
			}
			ShowMessage("\a\n");
			if(lineend)
				*lineend=c;
			return;
		}
		*lineend=c;
		p=lineend+1;
	}
}

/*==========================================
 * ���̉��
 *------------------------------------------
 */
char* parse_simpleexpr(char *p)
{
	p=skip_space(p);

#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		ShowMessage("parse_simpleexpr %s\n",p);
#endif
	if(*p==';' || *p==','){
		disp_error_message("unexpected expr end",p);
		exit(1);
	}
	if(*p=='('){

		p=parse_subexpr(p+1,-1);
		p=skip_space(p);
		if((*p++)!=')'){
			disp_error_message("unmatch ')'",p);
			exit(1);
		}
	} else if(isdigit((int)((unsigned char)*p)) || ((*p=='-' || *p=='+') && isdigit((int)((unsigned char)p[1])))){
		char *np;
		int i=strtoul(p,&np,0);
		add_scripti(i);
		p= np;
	} else if(*p=='"'){
		add_scriptc(CScriptEngine::C_STR);
		p++;
		while(*p && *p!='"'){
			if(p[-1]<=0x7e && *p=='\\')
				p++;
			else if(*p=='\n'){
				disp_error_message("unexpected newline @ string",p);
				exit(1);
			}
			add_scriptb(*p++);
		}
		if(!*p){
			disp_error_message("unexpected eof @ string",p);
			exit(1);
		}
		add_scriptb(0);
		p++;	//'"'
	} else {
		int c,l;
		char *p2;
		// label , register , function etc
		if(skip_word(p)==p && !(*p==')' && p[-1]=='(')){
			disp_error_message("unexpected character",p);
			exit(1);
		}
		p2 = skip_word(p);
		c=*p2;	*p2=0;	// ���O��add_str����
		l=add_str(p);

		parse_cmd=l;	// warn_*_mismatch_paramnum�̂��߂ɕK�v
		if(l==search_str("if"))	// warn_cmd_no_comma�̂��߂ɕK�v
			parse_cmd_if++;
/*
		// �p�~�\���l14/l15,����уv���t�B�b�N�X���̌x��
		if(	strcmp(str_buf+str_data[l].str,"l14")==0 ||
			strcmp(str_buf+str_data[l].str,"l15")==0 ){
			disp_error_message("l14 and l15 is DEPRECATED. use @menu instead of l15.",p);
		}else if(str_buf[str_data[l].str]=='l'){
			disp_error_message("prefix 'l' is DEPRECATED. use prefix '@' instead.",p2);
		}
*/
		*p2=c;	
		p=p2;

		if(str_data[l].type!=CScriptEngine::C_FUNC && c=='['){
			// array(name[i] => getelementofarray(name,i) )
			add_scriptl(search_str("getelementofarray"));
			add_scriptc(CScriptEngine::C_ARG);
			add_scriptl(l);
			p=parse_subexpr(p+1,-1);
			p=skip_space(p);
			if((*p++)!=']'){
				disp_error_message("unmatch ']'",p);
				exit(1);
			}
			add_scriptc(CScriptEngine::C_FUNC);
		}else
			add_scriptl(l);

	}

#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		ShowMessage("parse_simpleexpr end %s\n",p);
#endif
	return p;
}

/*==========================================
 * ���̉��
 *------------------------------------------
 */
char* parse_subexpr(char *p,int limit)
{
	int op,opl,len;
	char *tmpp;

#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		ShowMessage("parse_subexpr %s\n",p);
#endif
	p=skip_space(p);

	if(*p=='-'){
		tmpp = skip_space(p+1);
		if(*tmpp==';' || *tmpp==','){
			add_scriptl(LABEL_NEXTLINE);
			p++;
			return p;
		}
	}
	tmpp = p;
	if((op=CScriptEngine::C_NEG,*p=='-') || (op=CScriptEngine::C_LNOT,*p=='!') || (op=CScriptEngine::C_NOT,*p=='~')){
		p=parse_subexpr(p+1,100);
		add_scriptc(op);
	} else
		p=parse_simpleexpr(p);
	p=skip_space(p);
	while(((op=CScriptEngine::C_ADD,opl=6,len=1,*p=='+') ||
		   (op=CScriptEngine::C_SUB,opl=6,len=1,*p=='-') ||
		   (op=CScriptEngine::C_MUL,opl=7,len=1,*p=='*') ||
		   (op=CScriptEngine::C_DIV,opl=7,len=1,*p=='/') ||
		   (op=CScriptEngine::C_MOD,opl=7,len=1,*p=='%') ||
		   (op=CScriptEngine::C_FUNC,opl=8,len=1,*p=='(') ||
		   (op=CScriptEngine::C_LAND,opl=1,len=2,*p=='&' && p[1]=='&') ||
		   (op=CScriptEngine::C_AND,opl=5,len=1,*p=='&') ||
		   (op=CScriptEngine::C_LOR,opl=0,len=2,*p=='|' && p[1]=='|') ||
		   (op=CScriptEngine::C_OR,opl=4,len=1,*p=='|') ||
		   (op=CScriptEngine::C_XOR,opl=3,len=1,*p=='^') ||
		   (op=CScriptEngine::C_EQ,opl=2,len=2,*p=='=' && p[1]=='=') ||
		   (op=CScriptEngine::C_NE,opl=2,len=2,*p=='!' && p[1]=='=') ||
		   (op=CScriptEngine::C_R_SHIFT,opl=5,len=2,*p=='>' && p[1]=='>') ||
		   (op=CScriptEngine::C_GE,opl=2,len=2,*p=='>' && p[1]=='=') ||
		   (op=CScriptEngine::C_GT,opl=2,len=1,*p=='>') ||
		   (op=CScriptEngine::C_L_SHIFT,opl=5,len=2,*p=='<' && p[1]=='<') ||
		   (op=CScriptEngine::C_LE,opl=2,len=2,*p=='<' && p[1]=='=') ||
		   (op=CScriptEngine::C_LT,opl=2,len=1,*p=='<')) && opl>limit){
		p+=len;
		if(op==CScriptEngine::C_FUNC){
			int i=0,func=parse_cmd;
			const char *plist[128];

			if( str_data[func].type!=CScriptEngine::C_FUNC ){
				disp_error_message("expect function",tmpp);
				exit(0);
			}

			add_scriptc(CScriptEngine::C_ARG);
			do {
				plist[i]=p;
				p=parse_subexpr(p,-1);
				p=skip_space(p);
				if(*p==',') p++;
				else if(*p!=')' && script_config.warn_func_no_comma){
					disp_error_message("expect ',' or ')' at func params",p);
				}
				p=skip_space(p);
				i++;
			} while(*p && *p!=')' && i<128);
			plist[i]=p;
			if(*(p++)!=')'){
				disp_error_message("func request '(' ')'",p);
				exit(1);
			}

			if (str_data[func].type == CScriptEngine::C_FUNC && script_config.warn_func_mismatch_paramnum) {
				const char *arg = buildin_func[str_data[func].val].arg;
				int j = 0;
				for (; arg[j]; j++) if (arg[j] == '*') break;
				if (!(i <= 1 && j == 0) && ((arg[j] == 0 && i != j) || (arg[j] == '*' && i < j))) {
					disp_error_message("illegal number of parameters",plist[(i<j)?i:j]);
				}
			}
		} else {
			p=parse_subexpr(p,opl);
		}
		add_scriptc(op);
		p=skip_space(p);
	}
#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		ShowMessage("parse_subexpr end %s\n",p);
#endif
	return p;  /* return first untreated operator */
}

/*==========================================
 * ���̕]��
 *------------------------------------------
 */
char* parse_expr(char *p)
{
#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		ShowMessage("parse_expr %s\n",p);
#endif
	switch(*p){
	case ')': case ';': case ':': case '[': case ']':
	case '}':
		disp_error_message("unexpected char",p);
		exit(1);
	}
	p=parse_subexpr(p,-1);
#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		ShowMessage("parse_expr end %s\n",p);
#endif
	return p;
}

/*==========================================
 * �s�̉��
 *------------------------------------------
 */
char* parse_line(char *p)
{
	int i=0,cmd;
	const char *plist[128];
	char *p2;

	p=skip_space(p);
	if(*p==';')
		return p;

	parse_cmd_if=0;	// warn_cmd_no_comma�̂��߂ɕK�v

	// �ŏ��͊֐���
	p2 = p;
	p=parse_simpleexpr(p);
	p=skip_space(p);

	cmd=parse_cmd;
	if( str_data[cmd].type!=CScriptEngine::C_FUNC ){
		disp_error_message("expect command", p2);
		return p;
	}

	add_scriptc(CScriptEngine::C_ARG);
	while(p && *p && *p!=';' && i<128){
		plist[i]=p;

		p=parse_expr(p);
		p=skip_space(p);
		// ������؂��,����
		if(*p==',') p++;
		else if(*p!=';' && script_config.warn_cmd_no_comma && parse_cmd_if*2<=i ){
			disp_error_message("expect ',' or ';' at cmd params",p);
		}
		p=skip_space(p);
		i++;
	}
	plist[i]=(char *) p;
	if(!p || *(p++)!=';'){
		disp_error_message("need ';'",p);
		exit(1);
	}
	add_scriptc(CScriptEngine::C_FUNC);

	if( str_data[cmd].type==CScriptEngine::C_FUNC && script_config.warn_cmd_mismatch_paramnum){
		const char *arg=buildin_func[str_data[cmd].val].arg;
		int j=0;
		for(j=0;arg[j];j++) if(arg[j]=='*')break;
		if( (arg[j]==0 && i!=j) || (arg[j]=='*' && i<j) ){
			disp_error_message("illegal number of parameters",plist[(i<j)?i:j]);
		}
	}


	return p;
}

/*==========================================
 * �g�ݍ��݊֐��̒ǉ�
 *------------------------------------------
 */
void add_buildin_func(void)
{
	int i,n;
	for(i=0;buildin_func[i].func;i++){
		n=add_str(buildin_func[i].name);
		str_data[n].type=CScriptEngine::C_FUNC;
		str_data[n].val=i;
		str_data[n].func=buildin_func[i].func;
	}
}

/*==========================================
 * �萔�f�[�^�x�[�X�̓ǂݍ���
 *------------------------------------------
 */
void read_constdb(void)
{
	FILE *fp;
	char line[1024];
	char name[1024];
	int val,n,type;

	fp=safefopen("db/const.txt","r");
	if(fp==NULL){
		ShowError("can't read %s\n","db/const.txt");
		return ;
	}
	while(fgets(line,sizeof(line),fp)){
		if( !skip_empty_line(line) )
			continue;
		type=0;
		if(sscanf(line,"%[A-Za-z0-9_],%d,%d",name,&val,&type)>=2 ||
		   sscanf(line,"%[A-Za-z0-9_] %d %d",name,&val,&type)>=2)
		{
			tolower(name);
			n=add_str(name);
			if(type==0)
				str_data[n].type=CScriptEngine::C_INT;
			else
				str_data[n].type=CScriptEngine::C_PARAM;
			str_data[n].val=val;
		}
	}
	fclose(fp);
}



void debug_script(const char*script, size_t i, size_t sz)
{
	sz += i&15;
	i &= ~15;
	while(sz>0)
	{
		if((i&15)==0) ShowMessage("%04x : ",i);
		ShowMessage("%02x ", 0xFF&script[i]);
		if((i&15)==15) ShowMessage("\n");

		sz--;
		i++;
	}
	ShowMessage("\n");
}

/*==========================================
 * �X�N���v�g�̉��
 *------------------------------------------
 */
char* parse_script(unsigned char *src, size_t line)
{
	char *p,*tmpp;
	int i;
	static int first = 1;
	if(first)
	{
		add_buildin_func();
		read_constdb();
	}
	first = 0;

	////////////////////////////////////////////// 
	// additional check on the input to filter empty scripts ("{}" and "{ }") 
	p = (char*)src;
	p = skip_space(p);
	if(*p!='{')
	{
		disp_error_message("not found '{'", p);
		return NULL;
	}
	p++;
	p = skip_space(p);
	if(*p=='}')
	{	// an empty function, just return
		return NULL;
	}
	//////////////////////////////////////////////


	if(script_buf) aFree(script_buf);
	script_buf=(char *)aCalloc(SCRIPT_BLOCK_SIZE,sizeof(char));

	script_pos = 0;
	script_size = SCRIPT_BLOCK_SIZE;
	str_data[LABEL_NEXTLINE].type = CScriptEngine::C_NOP;
	str_data[LABEL_NEXTLINE].backpatch = -1;
	str_data[LABEL_NEXTLINE].label = -1;
	for (i = LABEL_START; i < str_num; i++) {
		if (str_data[i].type == CScriptEngine::C_POS || str_data[i].type == CScriptEngine::C_NAME) {
			str_data[i].type = CScriptEngine::C_NOP;
			str_data[i].backpatch = -1;
			str_data[i].label = -1;
		}
	}

	// �O���plabel db�̏�����
	if (scriptlabel_db)
		strdb_final (scriptlabel_db, scriptlabel_final);
	scriptlabel_db = strdb_init(50);

	// for error message
	startptr = (char*)src;
	startline = line;

	while(p && *p && *p!='}')
	{
		p = skip_space(p);
		// label�������ꏈ��
		tmpp = skip_space(skip_word(p));
		if(*tmpp==':')
		{
			int l, c;
			c = *skip_word(p);
			*skip_word(p) = 0;
			l = add_str(p);
			if (str_data[l].label != -1) {
				*skip_word(p) = c;
				disp_error_message("dup label ", p);
				exit(1);
			}
			set_label(l, script_pos);
			strdb_insert(scriptlabel_db,p,script_pos);
			*skip_word(p) = c;
			p = tmpp + 1;
		}
		else
		{	// ���͑S���ꏏ����
			p = parse_line(p);
			p = skip_space(p);
			add_scriptc(CScriptEngine::C_EOL);
			set_label(LABEL_NEXTLINE, script_pos);
			str_data[LABEL_NEXTLINE].type = CScriptEngine::C_NOP;
			str_data[LABEL_NEXTLINE].backpatch = -1;
			str_data[LABEL_NEXTLINE].label = -1;
		}
	}

	add_scriptc(CScriptEngine::C_NOP);
	script_size = script_pos;
	script_buf=(char*)aRealloc(script_buf,(script_pos + 1)*sizeof(char));

	// �������̃��x��������
	for (i = LABEL_START; i < str_num; i++) {
		if (str_data[i].type == CScriptEngine::C_NOP) {
			int j, next;
			str_data[i].type = CScriptEngine::C_NAME;
			str_data[i].label = i;
			for (j = str_data[i].backpatch; j >= 0 && j != 0x00ffffff; ) {
				next = (0xFF&script_buf[j])
					 | (0xFF&script_buf[j+1])<<8
					 | (0xFF&script_buf[j+2])<<16;
				script_buf[j] = i;
				script_buf[j+1] = i>>8;
				script_buf[j+2] = i>>16;
				j = next;
			}
		}
	}

#ifdef DEBUG_DISP
	for (i = 0; i < script_pos; i++) {
		if((i&15)==0) ShowMessage("%04x : ",i);
		ShowMessage("%02x ", 0xFF&script_buf[i]);
		if((i&15)==15) ShowMessage("\n");
	}
	ShowMessage("\n");
#endif

	char *local = script_buf;
	script_buf = NULL;
	return local;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Script Engine Implementation
//
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

/*==========================================
 * rid����sd�ւ̉���
 *------------------------------------------
 */
struct map_session_data *script_rid2sd(CScriptEngine &st)
{
	struct map_session_data *sd=map_id2sd(st.rid);
	if(!sd)
		ShowError("script_rid2sd: fatal error! player not attached!\n");
	return sd;
}


/*==========================================
 * �ϐ��̓ǂݎ��
 *------------------------------------------
 */
void CScriptEngine::ConvertName(CScriptEngine::CValue &data)
{
	if(data.type==CScriptEngine::C_NAME)
	{
		char *name=str_buf+str_data[data.num&0x00ffffff].str;
		char prefix=*name;
		char postfix=name[strlen(name)-1];

		if(postfix=='$')
		{
			data.type=CScriptEngine::C_CONSTSTR;
			if( prefix=='@'/* || prefix=='l' */)
			{
				struct map_session_data *sd=script_rid2sd(*this);
				if(sd)
					data.str = pc_readregstr(*sd,data.num);
			}
			else if(prefix=='$')
			{
				data.str = (char *)numdb_search(mapregstr_db,data.num);
			}
			else
			{
				ShowError("script: get_val: illegal scope string variable.\n");
				data.str = "!!ERROR!!";
			}
			if( data.str == NULL )
				data.str = "";
		}
		else
		{
			data.type=CScriptEngine::C_INT;
			if(str_data[data.num&0x00ffffff].type==CScriptEngine::C_INT)
			{
				data.num = str_data[data.num&0x00ffffff].val;
			}
			else if(str_data[data.num&0x00ffffff].type==CScriptEngine::C_PARAM)
			{
				struct map_session_data *sd=script_rid2sd(*this);
				if(sd)
					data.num = pc_readparam(*sd,str_data[data.num&0x00ffffff].val);
			}
			else if(prefix=='@'/* || prefix=='l'*/)
			{
				struct map_session_data *sd=script_rid2sd(*this);
				if(sd)
					data.num = pc_readreg(*sd,data.num);
			}
			else if(prefix=='$')
			{
				data.num = (int)numdb_search(mapreg_db,data.num);
			}
			else if(prefix=='#')
			{
				struct map_session_data *sd=script_rid2sd(*this);
				if(sd)
				{
					if( name[1]=='#')
						data.num = pc_readaccountreg2(*sd,name);
					else
						data.num = pc_readaccountreg(*sd,name);
				}
			}
			else
			{
				struct map_session_data *sd=script_rid2sd(*this);
				if(sd)
					data.num = pc_readglobalreg(*sd,name);
			}
		}
	}
}
/*==========================================
 * �ϐ��̓ǂݎ��2
 *------------------------------------------
 */
void* get_val2(CScriptEngine &st, int num)
{
	CScriptEngine::CValue dat;
	dat.type=CScriptEngine::C_NAME;
	dat.num=num;
	st.ConvertName(dat);
	if( dat.type==CScriptEngine::C_INT )
		return (void*)dat.num;
	else 
		return (void*)dat.str;
}

/*==========================================
 * �ϐ��ݒ�p
 *------------------------------------------
 */

int set_var(const char *name, void *v)
{
	if(name)
	{
		int num = add_str(name);
		char prefix =name[0];
		char postfix=name[strlen(name)-1];
		if(prefix=='$')
		{
			if( postfix=='$' )
			{
				mapreg_setregstr(num,(char*)v);
			}
			else
			{
				mapreg_setreg(num,(int)v);
			}
		}
		else
		{
			ShowError("script: set_var: illegal scope string variable !");
		}
	}
    return 0;
}
int set_var(struct map_session_data &sd, const char *name, void *v)
{
	if(name)
	{
		int num = add_str(name);
		char prefix =name[0];
		char postfix=name[strlen(name)-1];
		if( prefix=='@' || prefix=='l')
		{
			if( postfix=='$' )
			{
				pc_setregstr(sd,num,(char*)v);
			}
			else
			{
				pc_setreg(sd,num,(int)v);
			}
		}
		else
		{
			ShowError("script: set_var: illegal scope string variable !");
		}
	}
    return 0;
}


int set_reg(CScriptEngine &st,int num,const char *name, void *v)
{
	if(name)
	{
		char prefix =name[0];
		char postfix=name[strlen(name)-1];
		
		if( postfix=='$' )
		{	// string variable
			char *str=(char*)v;
			if( prefix=='@' || prefix=='l')
			{
				struct map_session_data *sd = script_rid2sd(st);
				if(sd)
					pc_setregstr(*sd,num,str);
				else
					ShowError("set_reg error name?:%s\n",name);
					
			}
			else if(prefix=='$')
			{
				mapreg_setregstr(num,str);
			}
			else
			{
				ShowError("script: set_reg: illegal scope string variable !");
			}
		}
		else 
		{	// ���l
			int val = (int)v;
			if(str_data[num&0x00ffffff].type==CScriptEngine::C_PARAM)
			{
				struct map_session_data *sd=script_rid2sd(st);
				if(sd) pc_setparam(*sd,str_data[num&0x00ffffff].val,val);
			}
			else if(prefix=='@' || prefix=='l')
			{
				struct map_session_data *sd=script_rid2sd(st);
				if(sd)
					pc_setreg(*sd,num,val);
				else
					ShowError("set_reg error name?:%s\n",name);
					
			}
			else if(prefix=='$')
			{
				mapreg_setreg(num,val);
			}
			else if(prefix=='#')
			{
				struct map_session_data *sd=script_rid2sd(st);
				if(sd)
				{
					if( name[1]=='#' )
						pc_setaccountreg2(*sd,name,val);
					else
						pc_setaccountreg(*sd,name,val);
				}
				else
					ShowError("set_reg error name?:%s\n",name);
			}
			else
			{
				struct map_session_data *sd=script_rid2sd(st);
				if(sd)
					pc_setglobalreg(*sd,name,val);
				else
					ShowError("set_reg error name?:%s\n",name);
			}
		}
	}
	return 0;
}


/*==========================================
 * ������ւ̕ϊ�
 *------------------------------------------
 */
const char* CScriptEngine::GetString(CScriptEngine::CValue &data)
{
	ConvertName(data);
	if(data.type==CScriptEngine::C_INT)
	{
		char *buf;
		buf=(char *)aMalloc(24*sizeof(char));
		sprintf(buf,"%d",data.num);
		data.type=CScriptEngine::C_STR;
		data.str=buf;
	}
	else if(data.type==CScriptEngine::C_NAME)
	{	// �e���|�����B�{�������͂�
		data.type=CScriptEngine::C_CONSTSTR;
		data.str=str_buf+str_data[data.num].str;
	}
	return data.str;
}

/*==========================================
 * ���l�֕ϊ�
 *------------------------------------------
 */
int CScriptEngine::GetInt(CScriptEngine::CValue &data)
{
	ConvertName(data);
	if( data.isString() )
	{
		const char *p=data.str;
		data.num = atoi(p);
		if(data.type==CScriptEngine::C_STR)
			aFree((void*)p);
		data.type=CScriptEngine::C_INT;
	}
	return data.num;
}


/*==========================================
 * �X�^�b�N�֐��l���v�b�V��
 *------------------------------------------
 */
void CScriptEngine::push_val(int type,int val)
{
	if(stack.sp >= stack.sp_max){
		stack.sp_max += 64;
		stack.stack_data = (CScriptEngine::CValue *)aRealloc(stack.stack_data, stack.sp_max*sizeof(CScriptEngine::CValue) );
		memset(stack.stack_data + (stack.sp_max - 64), 0, 64 * sizeof(CScriptEngine::CValue));
	}
//	if(battle_config.etc_log)
//		ShowMessage("push (%d,%d)-> %d\n",type,val,stack.sp);

	// check if freeing the previous stack value needs clearing
	if(stack.stack_data[stack.sp].type==CScriptEngine::C_STR)
		aFree((void*)(stack.stack_data[stack.sp].str));
	stack.stack_data[stack.sp].type=type;
	stack.stack_data[stack.sp].num=val;
	stack.sp++;
}

/*==========================================
 * �X�^�b�N�֕�������v�b�V��
 *------------------------------------------
 */
void CScriptEngine::push_str(int type, const char *str)
{
	if((size_t)stack.sp >= stack.sp_max)
	{
		stack.sp_max += 64;
		stack.stack_data = (CScriptEngine::CValue *)aRealloc(stack.stack_data, stack.sp_max * sizeof(CScriptEngine::CValue));
		memset(stack.stack_data + (stack.sp_max - 64), 0, 64 * sizeof(CScriptEngine::CValue));
	}
//	if(battle_config.etc_log)
//		ShowMessage("push (%d,%x)-> %d\n",type,str,stack.sp);
	// check if freeing the previous stack value needs clearing
	if(stack.stack_data[stack.sp].type==CScriptEngine::C_STR)
		aFree((void*)(stack.stack_data[stack.sp].str));

	stack.stack_data[stack.sp].type=type;
	stack.stack_data[stack.sp].str=str;
	stack.sp++;
}

/*==========================================
 * �X�^�b�N�֕������v�b�V��
 *------------------------------------------
 */
void CScriptEngine::push_copy(size_t pos)
{
	switch(stack.stack_data[pos].type){
	case CScriptEngine::C_CONSTSTR:
		push_str(CScriptEngine::C_CONSTSTR,stack.stack_data[pos].str);
		break;
	case CScriptEngine::C_STR:
		push_str(CScriptEngine::C_STR, aStrdup(stack.stack_data[pos].str));
		break;
	default:
		push_val(stack.stack_data[pos].type,stack.stack_data[pos].num);
		break;
	}
}

/*==========================================
 * �X�^�b�N����|�b�v
 *------------------------------------------
 */
void CScriptEngine::pop_stack(size_t start, size_t end)
{
	size_t i;
	for(i=start;i<end;i++)
	{
		if(stack.stack_data[i].type==C_STR)
		{
			aFree( (void*)(stack.stack_data[i].str) );
			stack.stack_data[i].type=C_INT;
			stack.stack_data[i].num=0;
		}
	}
	if(stack.sp>end){
		memmove(stack.stack_data+start,stack.stack_data+end,(stack.sp-end)*sizeof(CScriptEngine::CValue));
		memset(stack.stack_data+start+(stack.sp-end),0,(end-start)*sizeof(CScriptEngine::CValue));
	}
	stack.sp -= end-start;
}
/*==========================================
 * �X�^�b�N����l�����o��
 *------------------------------------------
 */
int CScriptEngine::pop_val()
{
	if(stack.sp <= 0)
		return 0;
	stack.sp--;
	ConvertName(stack.stack_data[stack.sp]);
	if(stack.stack_data[stack.sp].type==C_INT)
		return stack.stack_data[stack.sp].num;
	else if(stack.stack_data[stack.sp].type==C_STR)
	{
		aFree( (void*)(stack.stack_data[stack.sp].str) );
		stack.stack_data[stack.sp].type=C_INT;
		stack.stack_data[stack.sp].num=0;
	}
	return 0;
}



//
// ���s��main
//
/*==========================================
 * �R�}���h�̓ǂݎ��
 *------------------------------------------
 */
static int unget_com_data=-1;
int get_com(const char *script,unsigned int &pos)
{
	const unsigned char *s = (const unsigned char *)script;
	int i,j;
	if(unget_com_data>=0){
		i=unget_com_data;
		unget_com_data=-1;
		return i;
	}
	if(s[pos]>=0x80){
		return CScriptEngine::C_INT;
	}
	i=0; j=0;
	while(s[pos]>=0x40){
		i=s[pos++]<<j;
		j+=6;
	}
	return i+(s[pos++]<<j);
}

/*==========================================
 * �R�}���h�̃v�b�V���o�b�N
 *------------------------------------------
 */
void unget_com(int c)
{
	if(unget_com_data!=-1){
		if(battle_config.error_log)
			ShowMessage("unget_com can back only 1 data\n");
	}
	unget_com_data=c;
}

/*==========================================
 * ���l�̏���
 *------------------------------------------
 */
int get_num(const char *script,unsigned int &pos)
{
	const unsigned char *s = (const unsigned char *)script;
	int i=0,j=0;

	while(s[pos]>=0xc0){
		i+=(s[pos++]&0x7f)<<j;
		j+=6;
	}
	return i+((s[pos++]&0x7f)<<j);
}


///////////////////////////////////////////////////////////////////////////////
// Binomial operators (string) �񍀉��Z�q(������)
void CScriptEngine::op_2str(int op)
{
	int a=0;
	char *buf=NULL;
	const char *s2 = GetString( stack.stack_data[stack.sp  ] );
	const char *s1 = GetString( stack.stack_data[stack.sp-1] );

	switch(op)
	{
	case C_AND:
	case C_ADD:
		buf=(char *)aMalloc((1+strlen(stack.stack_data[stack.sp-1].str)+
							   strlen(stack.stack_data[stack.sp  ].str)) * sizeof(char));
		strcpy(buf,stack.stack_data[stack.sp-1].str);
		strcat(buf,stack.stack_data[stack.sp].str);
		break;
	case C_EQ:
		a= (strcmp(s1,s2)==0);
		break;
	case C_NE:
		a= (strcmp(s1,s2)!=0);
		break;
	case C_GT:
		a= (strcmp(s1,s2)> 0);
		break;
	case C_GE:
		a= (strcmp(s1,s2)>=0);
		break;
	case C_LT:
		a= (strcmp(s1,s2)< 0);
		break;
	case C_LE:
		a= (strcmp(s1,s2)<=0);
		break;
	default:
		ShowError("Script: illegal string operater\n");
		break;
	}
	// clear the stack values
	if( stack.stack_data[stack.sp-1].type==C_STR )
	{
		aFree( (void*)(stack.stack_data[stack.sp-1].str) );
		stack.stack_data[stack.sp-1].type=C_INT;
		stack.stack_data[stack.sp-1].num=0;
	}
	if( stack.stack_data[stack.sp].type==C_STR )
	{
		aFree((void*)(stack.stack_data[stack.sp].str) );
		stack.stack_data[stack.sp].type=C_INT;
		stack.stack_data[stack.sp].num=0;
	}
	// set the return value of the operation
	if(buf)
	{
		stack.stack_data[stack.sp-1].type= C_STR;
		stack.stack_data[stack.sp-1].str = buf;
	}
	else
	{
		stack.stack_data[stack.sp-1].type= C_INT;
		stack.stack_data[stack.sp-1].num = a;
	}
}
///////////////////////////////////////////////////////////////////////////////
// Binomial operators (numbers) �񍀉��Z�q(���l)
void CScriptEngine::op_2int(int op)
{
	int i2 = GetInt( stack.stack_data[stack.sp  ] );
	int i1 = GetInt( stack.stack_data[stack.sp-1] );

	switch(op)
	{
	case C_ADD:
		i1+=i2;
		break;
	case C_SUB:
		i1-=i2;
		break;
	case C_MUL:
	{
		int64 res = (int64)i1 * (int64)i2;
		if (res >  LLCONST( 2147483647 ) )
			i1 = INT_MAX;
		else if (res <  LLCONST(-2147483648) )
			i1 = INT_MIN;
		else
			i1 = (int)res;
		break;
	}
	case C_DIV:
		if(i2!=0)
			i1/=i2;
		else
			i1 = (i1<0)? INT_MIN:INT_MAX; // but no mathematical correct left/right side limiting value
		break;
	case C_MOD:
		if(i2!=0)
			i1%=i2;
		else
			i1 = 0;	// not mathematical correct
		break;
	case C_AND:
		i1&=i2;
		break;
	case C_OR:
		i1|=i2;
		break;
	case C_XOR:
		i1^=i2;
		break;
	case C_LAND:
		i1=i1&&i2;
		break;
	case C_LOR:
		i1=i1||i2;
		break;
	case C_EQ:
		i1=i1==i2;
		break;
	case C_NE:
		i1=i1!=i2;
		break;
	case C_GT:
		i1=i1>i2;
		break;
	case C_GE:
		i1=i1>=i2;
		break;
	case C_LT:
		i1=i1<i2;
		break;
	case C_LE:
		i1=i1<=i2;
		break;
	case C_R_SHIFT:
		i1=i1>>i2;
		break;
	case C_L_SHIFT:
		i1=i1<<i2;
		break;
	}
	stack.stack_data[stack.sp-1].type= C_INT;
	stack.stack_data[stack.sp-1].num = i1;
}
///////////////////////////////////////////////////////////////////////////////
// Binomial Operator �񍀉��Z�q
void CScriptEngine::op_2(int op)
{	// 2 input values on the stack result in 1 output value
	stack.sp--;
	// convert variables
	ConvertName(stack.stack_data[stack.sp  ]);
	ConvertName(stack.stack_data[stack.sp-1]);
	// check for string or number operation
	if( stack.stack_data[stack.sp].isString() || stack.stack_data[stack.sp-1].isString() )
	{	// at least one string, so let it be string operation
		op_2str(op);
	}
	else
	{	// numbers
		op_2int(op);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Unary Operators �P�����Z�q
void CScriptEngine::op_1(int op)
{
	int i1=pop_val();
	switch(op){
	case C_NEG:
		i1 = -i1;
		break;
	case C_NOT:
		i1 = ~i1;
		break;
	case C_LNOT:
		i1 = !i1;
		break;
	}
	push_val(C_INT,i1);
}


/*==========================================
 * �֐��̎��s
 *------------------------------------------
 */
int CScriptEngine::run_func()
{
	int i,start_sp,end_sp,func;

	end_sp=stack.sp;
	for(i=end_sp-1;i>=0 && stack.stack_data[i].type!=C_ARG;i--);
	if(i==0)
	{
		if(battle_config.error_log)
			ShowError("function not found\n");
		state=END;
		return 0;
	}
	this->start=start_sp=i-1;
	this->end  =end_sp;
	
	func=stack.stack_data[this->start].num;
	if( stack.stack_data[this->start].type!=C_NAME || str_data[func].type!=C_FUNC )
	{
		ShowMessage ("run_func: '"CL_WHITE"%s"CL_RESET"' (type %d) is not function and command!\n",
			str_buf + str_data[func].str, str_data[func].type);
		state=END;
		return 0;
	}
#ifdef DEBUG_RUN
	if(battle_config.etc_log) {
		ShowMessage("run_func : %s? (%d(%d))\n",str_buf+str_data[func].str,func,str_data[func].type);
		ShowMessage("stack dump :");
		for(i=0;i<end_sp;i++){
			switch(stack.stack_data[i].type){
			case C_INT:
				ShowMessage("| int(%d)", stack.stack_data[i].num);
				break;
			case C_NAME:
				ShowMessage("| name(%s)",str_buf+str_data[stack.stack_data[i].num].str);
				break;
			case C_ARG:
				ShowMessage("| arg");
				break;
			case C_POS:
				ShowMessage("| pos(%d)",stack.stack_data[i].num);
				break;
			default:
				ShowMessage("| %d,%d",stack.stack_data[i].type,stack.stack_data[i].num);
			}
		}
		ShowMessage("\n");
	}
#endif
	if(str_data[func].func){
		str_data[func].func(*this);
	}
	else
	{
		if(battle_config.error_log)
			ShowMessage("run_func : %s? (%d(%d))\n",str_buf+str_data[func].str,func,str_data[func].type);
		push_val(C_INT,0);
	}
	pop_stack(start_sp,end_sp);

	if(state==RETFUNC)
	{	// ���[�U�[��`�֐�����̕��A
		int olddefsp=defsp;
		int i;

		pop_stack(defsp,start_sp);	// ���A�Ɏז��ȃX�^�b�N�폜
		if( defsp<4 || stack.stack_data[defsp-1].type!=C_RETINFO)
		{
			ShowMessage("script:run_func(return) return without callfunc or callsub!\n");
			state=END;
			return 0;
		}
		pos	  = GetInt   (stack.stack_data[this->defsp-1]);	// �X�N���v�g�ʒu�̕���
		script= GetString(stack.stack_data[this->defsp-2]);	// �X�N���v�g�𕜌�
		defsp = GetInt   (stack.stack_data[this->defsp-3]);	// ��X�^�b�N�|�C���^�𕜌�
		i	  = GetInt   (stack.stack_data[this->defsp-4]);	// �����̐�����

		pop_stack(olddefsp-4-i,olddefsp);		// �v��Ȃ��Ȃ����X�^�b�N(�����ƕ��A�p�f�[�^)�폜

		state=GOTO;
	}

	return 0;
}

/*==========================================
 * �X�N���v�g�̎��s���C������
 *------------------------------------------
 */
int CScriptEngine::run_main()
{
	int c=0, rerun_pos;
	int cmdcount=script_config.check_cmdcount;
	int gotocount=script_config.check_gotocount;

	if(!script)
		return END;

	defsp = stack.sp;

	rerun_pos=pos;

	for(state=0; state==0;)
	{
		c = get_com(script, pos);

		switch(c)
		{
		case C_EOL:
			if(stack.sp!=defsp)
			{
				if(battle_config.error_log)
				{
					ShowMessage("stack.sp(%d) != default(%d)\n",stack.sp,defsp);
					//!!
					printf("(%d)\n", pos);
					debug_script(script,((pos>32)?pos-32:0),((pos>32)?32:pos));
				}
				stack.sp=defsp;
			}
			rerun_pos = pos;
			break;
		case C_INT:
			push_val(C_INT,get_num(script,pos));
			break;
		case C_POS:
		case C_NAME:
		{
			unsigned long tmp;
			tmp = (0xFF&script[pos])
				| (0xFF&script[pos+1])<<8
				| (0xFF&script[pos+2])<<16;
			push_val(c,tmp);
			pos+=3;
			break;
		}
		case C_ARG:
			push_val(c,0);
			break;
		case C_STR:
			push_str( C_CONSTSTR, (script+pos));
			while(script[pos++]);
			break;
		case CScriptEngine::C_FUNC:
			run_func();
			if(state==GOTO)
			{
				rerun_pos=pos;
				state=0;
				if( gotocount>0 && (--gotocount)<=0 )
				{
					ShowMessage("run_script: infinity loop func!\n");
					state=END;
				}
			}
			break;
		case C_ADD:
		case C_SUB:
		case C_MUL:
		case C_DIV:
		case C_MOD:
		case C_EQ:
		case C_NE:
		case C_GT:
		case C_GE:
		case C_LT:
		case C_LE:
		case C_AND:
		case C_OR:
		case C_XOR:
		case C_LAND:
		case C_LOR:
		case C_R_SHIFT:
		case C_L_SHIFT:
			op_2(c);
			break;
		case C_NEG:
		case C_NOT:
		case C_LNOT:
			op_1(c);
			break;
		case C_NOP:
			pos++;
			state=END;
			break;
		default:
			if(battle_config.error_log)
				ShowMessage("unknown command : %d @ %d\n",c,pos);
			state=END;
			break;
		}
		if( cmdcount>0 && (--cmdcount)<=0 ){
			ShowMessage("run_script: infinity loop cmd!\n");
			state=END;
		}
	}
	if( state==RERUNLINE )
		pos=rerun_pos;
	return state;
}

/*==========================================
 * �X�N���v�g�̎��s
 *------------------------------------------
 */
 int CScriptEngine::run(const char *rootscript, size_t pos, unsigned long rid, unsigned long oid)
{
	if( rootscript!=NULL )
	{
		struct map_session_data *sd=map_id2sd(rid);

		if( sd )
		{	// running script within player environment
			if( sd->ScriptEngine.script && (sd->ScriptEngine.script != rootscript || sd->ScriptEngine.pos != pos) )
			{
				ShowWarning("PlayerScript already started, queueing %p %i %i %i\n", rootscript, pos, rid, oid);
				sd->ScriptEngine.enqueue(rootscript, pos, rid, oid);
			}
			else
			{
				if( !sd->ScriptEngine.script )
				{	// start a new script

					if( NULL==sd->ScriptEngine.stack.stack_data )
					{	// create a new stack
						sd->ScriptEngine.stack.sp_max	 = 64;
						sd->ScriptEngine.stack.stack_data = (CScriptEngine::CValue *)aCalloc(sd->ScriptEngine.stack.sp_max,sizeof(CScriptEngine::CValue));
					}
					sd->ScriptEngine.stack.sp	= 0;
					sd->ScriptEngine.script		= rootscript;
					sd->ScriptEngine.pos		= pos;
					sd->ScriptEngine.rid		= rid;
					sd->ScriptEngine.oid		= oid;
				}
				if( END==sd->ScriptEngine.run_main() )
				{
					sd->ScriptEngine.state	= 0;
					sd->ScriptEngine.script	= NULL;
					sd->ScriptEngine.oid	= 0;

					// dequeue the next script caller if any
					CScriptEngine::CCallQueue* elem = sd->ScriptEngine.dequeue();
					if(elem)
					{
						// run it
						CScriptEngine::run(elem->script, elem->pos, elem->rid, elem->oid);
						// delete the caller
						delete elem;
					}
					else
					{	// something I dont understand yet
						npc_event_dequeue(*sd);
					}
				}
				else
				return sd->ScriptEngine.pos;
			}
		}
		else
		{	// no player, using server environment
			CScriptEngine st;
			memset(&st,0,sizeof(st));

			// create a new stack
			st.stack.sp			= 0;
			st.stack.sp_max		= 64;
			st.stack.stack_data = (CScriptEngine::CValue *)aCalloc(st.stack.sp_max,sizeof(CScriptEngine::CValue));

			st.script	= rootscript;
			st.pos		= pos;
			st.rid		= rid;
			st.oid		= oid;

			if( END!=st.run_main() )
				ShowWarning("ServerScript not finished with state 'End'\n");

			st.stack.clear();
		}
	}
	return 0;
}

void CScriptEngine::CStack::clear()
{
	if(stack_data)
	{
		for(size_t i = 0; i < sp; i++)
		{
			if (stack_data[i].type == CScriptEngine::C_STR)
				aFree((void*)(stack_data[i].str));
		}
		aFree(stack_data);
		stack_data=NULL;
		sp=0;
		sp_max=0;
	}
}


















//
// ���ߍ��݊֐�
//
/*==========================================
 *
 *------------------------------------------
 */
int buildin_mes(CScriptEngine &st)
{
	map_session_data *sd = script_rid2sd(st);
	if(sd) clif_scriptmes(*sd,st.oid,st.GetString(st[2]) );
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_goto(CScriptEngine &st)
{

	if (st[2].type != CScriptEngine::C_POS){
		int func = st[2].num;
		ShowMessage("script: goto '"CL_WHITE"%s"CL_RESET"': not label!\n", str_buf + str_data[func].str);
		st.state = CScriptEngine::END;
		return 0;
	}
	st.pos = st.GetInt( st[2] );
	st.state = CScriptEngine::GOTO;
	return 0;
}

/*==========================================
 * ���[�U�[��`�֐��̌Ăяo��
 *------------------------------------------
 */
int buildin_callfunc(CScriptEngine &st)
{
	char *scr;
	const char *str=st.GetString((st[2]));

	if( (scr=(char *) strdb_search(script_get_userfunc_db(),str)) )
	{
		size_t i,j;
		for(i=st.start+3,j=0;i<st.end;i++,j++)
			st.push_copy(i);

		st.push_val(CScriptEngine::C_INT,j);				// �����̐����v�b�V��
		st.push_val(CScriptEngine::C_INT,st.defsp);			// ���݂̊�X�^�b�N�|�C���^���v�b�V��
		st.push_str(CScriptEngine::C_CONSTSTR,st.script);	// ���݂̃X�N���v�g���v�b�V��
		st.push_val(CScriptEngine::C_RETINFO,st.pos);		// ���݂̃X�N���v�g�ʒu���v�b�V��

		st.pos=0;
		st.script=scr;
		st.defsp=st.start+4+j;
		st.state=CScriptEngine::GOTO;
	}
	else
	{
		ShowError("script:callfunc: function not found! [%s]\n",str);
		st.state=CScriptEngine::END;
	}
	return 0;
}
/*==========================================
 * �T�u���[�e�B���̌Ăяo��
 *------------------------------------------
 */
int buildin_callsub(CScriptEngine &st)
{
	int pos=st.GetInt( (st[2]));
	size_t i,j;
	for(i=st.start+3,j=0;i<st.end;i++,j++)
		st.push_copy(i);

	st.push_val(CScriptEngine::C_INT,j);					// �����̐����v�b�V��
	st.push_val(CScriptEngine::C_INT,st.defsp);			// ���݂̊�X�^�b�N�|�C���^���v�b�V��
	st.push_str(CScriptEngine::C_CONSTSTR,st.script);	// ���݂̃X�N���v�g���v�b�V��
	st.push_val(CScriptEngine::C_RETINFO,st.pos);		// ���݂̃X�N���v�g�ʒu���v�b�V��

	st.pos=pos;
	st.defsp=st.start+4+j;
	st.state=CScriptEngine::GOTO;
	return 0;
}

/*==========================================
 * �����̏���
 *------------------------------------------
 */
int buildin_getarg(CScriptEngine &st)
{
	int num=st.GetInt( st[2]);
	int max,stsp;
	if( st.defsp<4 || st.getDirectData(st.defsp-1).type!=CScriptEngine::C_RETINFO ){
		ShowError("script:getarg without callfunc or callsub!\n");
		st.state=CScriptEngine::END;
		return 0;
	}
	max=st.GetInt( (st.getDirectData(st.defsp-4)));
	stsp=st.defsp - max -4;
	if( num >= max ){
		ShowError("script:getarg arg1(%d) out of range(%d) !\n",num,max);
		st.state=CScriptEngine::END;
		return 0;
	}
	st.push_copy(stsp+num);
	return 0;
}

/*==========================================
 * �T�u���[�`��/���[�U�[��`�֐��̏I��
 *------------------------------------------
 */
int buildin_return(CScriptEngine &st)
{
	if(st.end>st.start+2){	// �߂�l�L��
		st.push_copy(st.start+2);
	}
	st.state=CScriptEngine::RETFUNC;
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_next(CScriptEngine &st)
{
	st.state=CScriptEngine::STOP;
	map_session_data *sd = script_rid2sd(st);
	if(sd) clif_scriptnext(*sd,st.oid);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_close(CScriptEngine &st)
{
	map_session_data *sd = script_rid2sd(st);
	if(sd)
	{
		if(st.rerun_flag==0)
		{
			st.rerun_flag=1;
			st.state=CScriptEngine::RERUNLINE;
			clif_scriptclose(*sd,st.oid);
			return 0;
		}
		else
		{
			st.rerun_flag=0;
		}
	}
	st.state=CScriptEngine::END;
	return 0;
}
int buildin_close2(CScriptEngine &st)
{
	st.state=CScriptEngine::STOP;
	map_session_data *sd = script_rid2sd(st);
	if(sd) clif_scriptclose(*sd,st.oid);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_menu(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if(!sd)
		return 0;

	if(st.rerun_flag==0)
	{
		size_t len,i;
		char *buf;
		st.state=CScriptEngine::RERUNLINE;
		st.rerun_flag=1;
		for(i=st.start+2,len=16;i<st.end;i+=2){
			st.GetString((st.getDirectData(i)));
			len+=strlen(st.getDirectData(i).str)+1;
		}
		buf=(char *)aMalloc((len+1)*sizeof(char));
		buf[0]=0;
		for(i=st.start+2,len=0;i<st.end;i+=2){
			strcat(buf,st.getDirectData(i).str);
			strcat(buf,":");
		}
		clif_scriptmenu(*sd,st.oid,buf);
		aFree(buf);
	}
	else if(sd->npc_menu==0xff)
	{	// cansel
		st.rerun_flag=0;
		st.state=CScriptEngine::END;
	}
	else
	{	// goto����
		// ragemu�݊��̂���
		pc_setreg(*sd,add_str("l15"),sd->npc_menu);
		pc_setreg(*sd,add_str("@menu"),sd->npc_menu);
		st.rerun_flag=0;
		if(sd->npc_menu>0 && sd->npc_menu<(st.end-st.start)/2){
			size_t pos;
			if( st[sd->npc_menu*2+1].type!=CScriptEngine::C_POS ){
				ShowMessage("script: menu: not label !\n");
				st.state=CScriptEngine::END;
				return 0;
			}
			pos=st.GetInt( (st[sd->npc_menu*2+1]));
			st.pos=pos;
			st.state=CScriptEngine::GOTO;
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_rand(CScriptEngine &st)
{
	int range;
	int min = 0;
	if (st.end > st.start+3){
		int max;
		min = st.GetInt( (st[2]));
		max = st.GetInt( (st[3]));
		if(min>max)	swap(max, min);
		range = max - min + 1;
		if (range == 0) range = 1;
	} else {
		range = st.GetInt( (st[2]));
		if (range == 0) range = 1;
	}
	min += rand()%range;
	st.push_val(CScriptEngine::C_INT,min);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_input(CScriptEngine &st)
{
	int num=(st.end>st.start+2)?st[2].num:0;
	char *name=(st.end>st.start+2)?str_buf+str_data[num&0x00ffffff].str:(char*)"";
	char postfix=name[strlen(name)-1];
	struct map_session_data *sd=script_rid2sd(st);
	struct npc_data *nd=(struct npc_data *)map_id2bl(st.oid);

	if( sd && nd )
	if( !npc_isNear(*sd, *nd) )
	{
		if(!st.rerun_flag)
			clif_scriptmes(*sd,st.oid, "Script is using input box with invisible or nonreachable npc and is terminated");
		return buildin_close(st);
	}
	else
	{
		if(st.rerun_flag)
		{
			st.rerun_flag=0;
			if( postfix=='$' )
			{	// ������
				if(st.end>st.start+2)
				{	// ����1��
					set_reg(st,num,name,(void*)sd->npc_str);
				}
				else
				{
					ShowError("buildin_input: string discarded !!\n");
				}
			}
			else
			{	// ���l
				if(st.end>st.start+2)
				{	// ����1��
					set_reg(st,num,name,(void*)sd->npc_amount);
				}
				else
				{	// ragemu�݊��̂���
					pc_setreg(*sd,add_str( "l14"),sd->npc_amount);
				}
			}
		}
		else
		{
			st.state=CScriptEngine::RERUNLINE;
			if(postfix=='$')clif_scriptinputstr(*sd,st.oid);
			else			clif_scriptinput(*sd,st.oid);
			st.rerun_flag=1;
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_if(CScriptEngine &st)
{
	size_t sel,i;

	sel=st.GetInt( (st[2]));
	if(!sel)
		return 0;

	// �֐������R�s�[
	st.push_copy(st.start+3);
	// �ԂɈ����}�[�J������
	st.push_val(CScriptEngine::C_ARG,0);
	// �c��̈������R�s�[
	for(i=st.start+4;i<st.end;i++){
		st.push_copy(i);
	}
	st.run_func();

	return 0;
}


/*==========================================
 * �ϐ��ݒ�
 *------------------------------------------
 */
int buildin_set(CScriptEngine &st)
{
	int num=st[2].num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char postfix=name[strlen(name)-1];

	if( st[2].type!=CScriptEngine::C_NAME ){
		ShowMessage("script: buildin_set: not name\n");
		return 0;
	}

	if( postfix=='$' ){
		// ������
		const char *str = st.GetString((st[3]));
		set_reg(st,num,name,(void*)str);
	}else{
		// ���l
		int val = st.GetInt( (st[3]));
		set_reg(st,num,name,(void*)val);
	}

	return 0;
}
/*==========================================
 * �z��ϐ��ݒ�
 *------------------------------------------
 */
int buildin_setarray(CScriptEngine &st)
{
	int num=st[2].num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];
	size_t i,j;

	if( prefix!='$' && prefix!='@' ){
		ShowMessage("buildin_setarray: illegal scope !\n");
		return 0;
	}

	for(j=0,i=st.start+3; i<st.end && j<128;i++,j++){
		void *v;
		if( postfix=='$' )
			v=(void*)st.GetString((st.getDirectData(i)));
		else
			v=(void*)st.GetInt( (st.getDirectData(i)));
		set_reg(st, num+(j<<24), name, v);
	}
	return 0;
}
/*==========================================
 * �z��ϐ��N���A
 *------------------------------------------
 */
int buildin_cleararray(CScriptEngine &st)
{
	int num=st[2].num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];
	int sz=st.GetInt( (st[4]));
	int i;
	void *v;

	if( prefix!='$' && prefix!='@' ){
		ShowMessage("buildin_cleararray: illegal scope !\n");
		return 0;
	}

	if( postfix=='$' )
		v=(void*)st.GetString((st[3]));
	else
		v=(void*)st.GetInt( (st[3]));

	for(i=0;i<sz;i++)
		set_reg(st,num+(i<<24),name,v);
	return 0;
}
/*==========================================
 * �z��ϐ��R�s�[
 *------------------------------------------
 */
int buildin_copyarray(CScriptEngine &st)
{
	int num=st[2].num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];
	int num2=st[3].num;
	char *name2=str_buf+str_data[num2&0x00ffffff].str;
	char prefix2=*name2;
	char postfix2=name2[strlen(name2)-1];
	int sz=st.GetInt( (st[4]));
	int i;

	if( prefix!='$' && prefix!='@' && prefix2!='$' && prefix2!='@' ){
		ShowMessage("buildin_copyarray: illegal scope !\n");
		return 0;
	}
	if( (postfix=='$' || postfix2=='$') && postfix!=postfix2 ){
		ShowMessage("buildin_copyarray: type mismatch !\n");
		return 0;
	}

	for(i=0;i<sz;i++)
		set_reg(st,num+(i<<24),name, get_val2(st,num2+(i<<24)) );
	return 0;
}
/*==========================================
 * �z��ϐ��̃T�C�Y����
 *------------------------------------------
 */
int getarraysize(CScriptEngine &st,int num,int postfix)
{
	int i=(num>>24),c=i;
	for(;i<128;i++){
		void *v=get_val2(st,num+(i<<24));
		if(postfix=='$' && *((char*)v) ) c=i;
		if(postfix!='$' && (int)v )c=i;
	}
	return c+1;
}
int buildin_getarraysize(CScriptEngine &st)
{
	int num=st[2].num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];

	if( prefix!='$' && prefix!='@' ){
		ShowMessage("buildin_copyarray: illegal scope !\n");
		return 0;
	}

	st.push_val(CScriptEngine::C_INT,getarraysize(st,num,postfix) );
	return 0;
}
/*==========================================
 * �z��ϐ�����v�f�폜
 *------------------------------------------
 */
int buildin_deletearray(CScriptEngine &st)
{
	int num=st[2].num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];
	int count=1;
	int i,sz=getarraysize(st,num,postfix)-(num>>24)-count+1;


	if( (st.end > st.start+3) )
		count=st.GetInt( (st[3]));

	if( prefix!='$' && prefix!='@' ){
		ShowMessage("buildin_deletearray: illegal scope !\n");
		return 0;
	}

	for(i=0;i<sz;i++){
		set_reg(st,num+(i<<24),name, get_val2(st,num+((i+count)<<24) ) );
	}
	for(;i<(128-(num>>24));i++){
		if( postfix!='$' ) set_reg(st,num+(i<<24),name, 0);
		if( postfix=='$' ) set_reg(st,num+(i<<24),name, (void *) "");
	}
	return 0;
}

/*==========================================
 * �w��v�f��\���l(�L�[)����������
 *------------------------------------------
 */
int buildin_getelementofarray(CScriptEngine &st)
{
	if( st[2].type==CScriptEngine::C_NAME ){
		int i=st.GetInt( (st[3]));
		if(i>127 || i<0){
			ShowMessage("script: getelementofarray (operator[]): param2 illegal number %d\n",i);
			st.push_val(CScriptEngine::C_INT,0);
		}else{
			st.push_val(CScriptEngine::C_NAME, (i<<24) | st[2].num );
		}
	}else{
		ShowMessage("script: getelementofarray (operator[]): param1 not name !\n");
		st.push_val(CScriptEngine::C_INT,0);
	}
	return 0;
}


/*==========================================
 *
 *------------------------------------------
 */
int buildin_warp(CScriptEngine &st)
{
	int x,y;
	const char *str;
	struct map_session_data *sd=script_rid2sd(st);

	str=st.GetString((st[2]));
	x=st.GetInt( (st[3]));
	y=st.GetInt( (st[4]));

	if(sd==NULL || str==NULL)
		return 0;

	if( 0==strcmp(str,"Random") )
	{
		pc_randomwarp(*sd,3);
	}
	else if( 0==strcmp(str,"SavePoint") )
	{
		if(map[sd->bl.m].flag.noreturn)	// ���֎~
			return 0;
		pc_setpos(*sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
	}
	else if( 0==strcmp(str,"Save") )
	{
		if(map[sd->bl.m].flag.noreturn)	// ���֎~
			return 0;
		pc_setpos(*sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
	}
	else
	{
		pc_setpos(*sd,str,x,y,0);
	}
	return 0;
}
/*==========================================
 * �G���A�w�胏�[�v
 *------------------------------------------
 */
int buildin_areawarp_sub(struct block_list &bl,va_list ap)
{
	int x,y;
	char *map;
	map=va_arg(ap, char *);
	x=va_arg(ap,int);
	y=va_arg(ap,int);
	if(strcmp(map,"Random")==0)
		pc_randomwarp(((struct map_session_data &)bl),3);
	else
		pc_setpos(((struct map_session_data &)bl),map,x,y,0);
	return 0;
}
int buildin_areawarp(CScriptEngine &st)
{
	int x,y,m;
	const char *str;
	const char *mapname;
	int x0,y0,x1,y1;

	mapname=st.GetString((st[2]));
	x0=st.GetInt( (st[3]));
	y0=st.GetInt( (st[4]));
	x1=st.GetInt( (st[5]));
	y1=st.GetInt( (st[6]));
	str=st.GetString((st[7]));
	x=st.GetInt( (st[8]));
	y=st.GetInt( (st[9]));

	if( (m=map_mapname2mapid(mapname))< 0)
		return 0;
//!! broadcast command if not on this mapserver

	map_foreachinarea(buildin_areawarp_sub,
		m,x0,y0,x1,y1,BL_PC, str,x,y );
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_heal(CScriptEngine &st)
{
	int hp,sp;

	hp=st.GetInt( (st[2]));
	sp=st.GetInt( (st[3]));
	map_session_data *sd = script_rid2sd(st);
	if(sd) pc_heal(*sd,hp,sp);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_itemheal(CScriptEngine &st)
{
	int hp,sp;

	hp=st.GetInt( (st[2]));
	sp=st.GetInt( (st[3]));
	map_session_data *sd = script_rid2sd(st);
	if(sd) pc_itemheal(*sd,hp,sp);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_percentheal(CScriptEngine &st)
{
	int hp,sp;

	hp=st.GetInt( (st[2]));
	sp=st.GetInt( (st[3]));
	map_session_data *sd = script_rid2sd(st);
	if(sd) pc_percentheal(*sd,hp,sp);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_jobchange(CScriptEngine &st)
{
	int job, upper=-1;

	job=st.GetInt( (st[2]));
	if( st.end>st.start+3 )
		upper=st.GetInt( (st[3]));

	map_session_data *sd = script_rid2sd(st);
	if( sd && job>=0 && job<MAX_PC_CLASS )
		pc_jobchange(*sd,job, upper);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_setlook(CScriptEngine &st)
{
	int type,val;

	type=st.GetInt( (st[2]));
	val=st.GetInt( (st[3]));

	struct map_session_data *sd = script_rid2sd(st);
	if(sd) pc_changelook(*sd,type,val);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_cutin(CScriptEngine &st)
{
	int type;

	st.GetString((st[2]));
	type=st.GetInt( (st[3]));

	struct map_session_data *sd = script_rid2sd(st);
	if(sd) clif_cutin(*sd,st[2].str,type);

	return 0;
}
/*==========================================
 * �J�[�h�̃C���X�g��\������
 *------------------------------------------
 */
int buildin_cutincard(CScriptEngine &st)
{
	int itemid =st.GetInt( (st[2]));
	map_session_data *sd = script_rid2sd(st);
	if(sd)
	{	struct item_data* idata = itemdb_exists(itemid);
		if(idata) clif_cutin(*sd,idata->cardillustname,4);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_viewpoint(CScriptEngine &st)
{
	int type,x,y,id,color;

	type=st.GetInt( (st[2]));
	x=st.GetInt( (st[3]));
	y=st.GetInt( (st[4]));
	id=st.GetInt( (st[5]));
	color=st.GetInt( (st[6]));

	map_session_data *sd = script_rid2sd(st);
	if(sd) clif_viewpoint(*sd,st.oid,type,x,y,id,color);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_countitem(CScriptEngine &st)
{
	unsigned short nameid=0;
	size_t count=0,i;
	struct map_session_data *sd = script_rid2sd(st);
	if(sd)
	{
		CScriptEngine::CValue &data = st[2];
		st.ConvertName(data);

		if( data.isString() )
		{
			const char *name=st.GetString(data);
			struct item_data *item_data;
			if( (item_data = itemdb_searchname(name)) != NULL)
				nameid=item_data->nameid;
		}
		else
			nameid=st.GetInt(data);
		
		if(nameid>=500 && nameid<MAX_ITEMS)
		{
			for(i=0;i<MAX_INVENTORY;i++)
			{
				if(sd->status.inventory[i].nameid==nameid)
					count+=sd->status.inventory[i].amount;
			}
		}
		else if(battle_config.error_log)
			ShowMessage("wrong item ID : countitem(%i)\n",nameid);
	}
	st.push_val(CScriptEngine::C_INT,count);
	return 0;
}

/*==========================================
 * �d�ʃ`�F�b�N
 *------------------------------------------
 */
int buildin_checkweight(CScriptEngine &st)
{
	int val = 1;
	unsigned short nameid=0, amount;
	CScriptEngine::CValue &data= st[2];
	struct map_session_data *sd = script_rid2sd(st);

	if(sd)
	{
		st.ConvertName(data);
		if( data.isString() )
		{
			const char *name=st.GetString(data);
			struct item_data *item_data = itemdb_searchname(name);
			if( item_data )
				nameid=item_data->nameid;
		}
		else
			nameid=st.GetInt(data);
		amount=st.GetInt( (st[3]));
		if( amount<MAX_AMOUNT && nameid>=500 && nameid<MAX_ITEMS)
		{
			if( itemdb_weight(nameid)*amount + sd->weight <= sd->max_weight )
			{
				val = 0;
			}
		}
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_getitem(CScriptEngine &st)
{
	//!! some stupid game with signs
	short nameid,nameidsrc,amount;
	int flag = 0;
	struct item item_tmp;
	CScriptEngine::CValue &data=st[2];
	struct map_session_data *sd = script_rid2sd(st);

	st.ConvertName(data);
	if( data.isString() )
	{
		const char *name=st.GetString(data);
		struct item_data *item_data = itemdb_searchname(name);
		nameid=512; //Apple item ID
		if( item_data != NULL)
			nameid=item_data->nameid;
	}else
		nameid=st.GetInt(data);

	if ( ( amount=st.GetInt( (st[3])) ) <= 0) {
		return 0; //return if amount <=0, skip the useles iteration
	}
	//Violet Box, Blue Box, etc - random item pick
	nameidsrc = nameid;
	if( sd && nameid<0 )
	{	// Save real ID of the source Box [Lupus]
		nameid=itemdb_searchrandomid(-nameid);

		if(log_config.present > 0)
			log_present(*sd, -nameidsrc, nameid); //fixed missing ID by Lupus
		flag = 1;
	}

	if(nameid > 0)
	{
		memset(&item_tmp,0,sizeof(item_tmp));
		item_tmp.nameid=nameid;
		if(!flag)
			item_tmp.identify=1;
		else
			item_tmp.identify = !itemdb_isEquipment(nameid);
		if( st.end>st.start+5 ) //�A�C�e�����w�肵��ID�ɓn��
			sd=map_id2sd(st.GetInt( (st[5])));
		if(sd == NULL) //�A�C�e����n�����肪���Ȃ������炨�A��
			return 0;
		if((flag = pc_additem(*sd,item_tmp,amount)))
		{	// additem failed
			clif_additem(*sd,0,0,flag);
			// create it on floor if dropable, let it vanish otherwise
			if( itemdb_isdropable(nameid, pc_isGM(*sd)) )
				map_addflooritem(item_tmp,amount,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_getitem2(CScriptEngine &st)
{
	//!! some stupid game with signs
	short nameid,amount;
	int flag = 0;
	int iden,ref,attr,c1,c2,c3,c4;
	struct item_data *item_data;
	struct item item_tmp;
	CScriptEngine::CValue &data= st[2];
	struct map_session_data *sd = script_rid2sd(st);

	
	st.ConvertName(data);
	if( data.isString() ){
		const char *name=st.GetString(data);
		struct item_data *item_data = itemdb_searchname(name);
		nameid=512; //Apple item ID
		if( item_data )
			nameid=item_data->nameid;
	}else
		nameid=st.GetInt(data);

	amount=st.GetInt( (st[3]));
	iden=st.GetInt( (st[4]));
	ref=st.GetInt( (st[5]));
	attr=st.GetInt( (st[6]));
	c1=st.GetInt( (st[7]));
	c2=st.GetInt( (st[8]));
	c3=st.GetInt( (st[9]));
	c4=st.GetInt( (st[10]));
	if( st.end>st.start+11 ) //�A�C�e�����w�肵��ID�ɓn��
		sd=map_id2sd(st.GetInt( (st[11])));
	if(sd == NULL) //�A�C�e����n�����肪���Ȃ������炨�A��
		return 0;

	if(nameid<0) { // �����_��
		nameid=itemdb_searchrandomid(-nameid);
		flag = 1;
	}

	if(nameid > 0) {
		memset(&item_tmp,0,sizeof(item_tmp));
		item_data=itemdb_search(nameid);
		if(item_data->type==4 || item_data->type==5){
			if(ref > 10) ref = 10;
		}
		else if(item_data->type==7) {
			iden = 1;
			ref = 0;
		}
		else {
			iden = 1;
			ref = attr = 0;
		}

		item_tmp.nameid=nameid;
		if(!flag)
			item_tmp.identify=iden;
		else if(item_data->type==4 || item_data->type==5)
			item_tmp.identify=0;
		item_tmp.refine=ref;
		item_tmp.attribute=attr;
		item_tmp.card[0]=c1;
		item_tmp.card[1]=c2;
		item_tmp.card[2]=c3;
		item_tmp.card[3]=c4;
		if((flag = pc_additem(*sd,item_tmp,amount))) {
			clif_additem(*sd,0,0,flag);
			map_addflooritem(item_tmp,amount,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
	}

	return 0;
}

/*==========================================
 * gets an item with someone's name inscribed [Skotlex]
 * getinscribeditem item_num, character_name
 * Returned Qty is always 1, only works on equip-able
 * equipment
 *------------------------------------------
 */
int buildin_getnameditem(CScriptEngine &st)
{
	int nameid;
	struct item item_tmp;
	struct map_session_data *sd, *tsd;
	CScriptEngine::CValue &data = st[2];


	sd = script_rid2sd(st);
	if (sd == NULL)
	{	//Player not attached!
		st.push_val(CScriptEngine::C_INT,0);
		return 0; 
	}
	
	st.ConvertName(data);
	if( data.isString() ){
		const char *name=st.GetString(data);
		struct item_data *item_data = itemdb_searchname(name);
		if( item_data == NULL)
		{	//Failed
			st.push_val(CScriptEngine::C_INT,0);
			return 0;
		}
		nameid = item_data->nameid;
	}else
		nameid = st.GetInt(data);

	if(!itemdb_exists(nameid) || !itemdb_isEquipment(nameid))
	{	//We don't allow non-equipable/stackable items to be named
		//to avoid any qty exploits that could happen because of it.
		st.push_val(CScriptEngine::C_INT,0);
		return 0;
	}

	data = st[3];
	st.ConvertName(data);
	if( data.isString() )	//Char Name
		tsd=map_nick2sd(st.GetString(data));
	else	//Char Id was given
		tsd=map_charid2sd(st.GetInt(data));
	
	if( tsd == NULL )
	{	//Failed
		st.push_val(CScriptEngine::C_INT,0);
		return 0;
	}

	memset(&item_tmp,0,sizeof(item_tmp));
	item_tmp.nameid=nameid;
	item_tmp.amount=1;
	item_tmp.identify=1;
	item_tmp.card[0]=255;
	item_tmp.card[2]=tsd->status.char_id;
	item_tmp.card[3]=tsd->status.char_id >> 16;
	if(pc_additem(*sd,item_tmp,1)) {
		st.push_val(CScriptEngine::C_INT,0);
		return 0;	//Failed to add item, we will not drop if they don't fit
	}

	st.push_val(CScriptEngine::C_INT,1);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_makeitem(CScriptEngine &st)
{
	//!! some stupid game with signs
	short nameid, amount;
	int flag = 0;
	int x,y,m;
	const char *mapname;
	struct item item_tmp;
	struct map_session_data *sd;
	CScriptEngine::CValue &data=st[2];

	sd = script_rid2sd(st);

	st.ConvertName(data);
	if( data.isString() ){
		const char *name=st.GetString(data);
		struct item_data *item_data = itemdb_searchname(name);
		nameid=512; //Apple Item ID
		if( item_data )
			nameid=item_data->nameid;
	}else
		nameid=st.GetInt(data);

	amount=st.GetInt( (st[3]));
	mapname	=st.GetString((st[4]));
	x	=st.GetInt( (st[5]));
	y	=st.GetInt( (st[6]));

	if( sd && strcmp(mapname,"this")==0)
		m=sd->bl.m;
	else
		m=map_mapname2mapid(mapname);

	if(nameid<0) { // �����_��
		nameid=itemdb_searchrandomid(-nameid);
		flag = 1;
	}

	if(nameid > 0) {
		memset(&item_tmp,0,sizeof(item_tmp));
		item_tmp.nameid=nameid;
		if(!flag)
			item_tmp.identify=1;
		else
			item_tmp.identify = !itemdb_isEquipment(nameid);
//		clif_additem(sd,0,0,flag);
		map_addflooritem(item_tmp,amount,m,x,y,NULL,NULL,NULL,0);
	}

	return 0;
}
/*==========================================
 * script DELITEM command (fixed 2 bugs by Lupus, added deletion priority by Lupus)
 *------------------------------------------
 */
int buildin_delitem(CScriptEngine &st)
{
	unsigned short nameid=0,amount;
	int i,important_item=0;
	struct map_session_data *sd=script_rid2sd(st);
	CScriptEngine::CValue &data = st[2];

	st.ConvertName(data);
	if( data.isString() )
	{
		const char *name=st.GetString(data);
		struct item_data *item_data = itemdb_searchname(name);
		if( item_data )
			nameid=item_data->nameid;
	}
	else
		nameid=st.GetInt(data);

	amount=st.GetInt( (st[3]));

	if(sd && nameid>500 && nameid<20000 && amount<=MAX_AMOUNT)
	{	//by Lupus. Don't run FOR if u got wrong item ID or amount<=0
		
		//1st pass
		//here we won't delete items with CARDS, named items but we count them
		for(i=0;i<MAX_INVENTORY && amount>0; i++)
		{	//we don't delete wrong item or equipped item
			if( sd->status.inventory[i].nameid!=nameid || sd->inventory_data[i] == NULL ||
				sd->status.inventory[i].nameid>=20000  || sd->status.inventory[i].amount>=MAX_AMOUNT )
				continue;

			//1 egg uses 1 cell in the inventory. so it's ok to delete 1 pet / per cycle
			if(sd->inventory_data[i]->type==7 && sd->status.inventory[i].card[0] == 0xff00 && search_petDB_index(nameid, PET_EGG) >= 0 )
			{
				intif_delete_petdata( MakeDWord(sd->status.inventory[i].card[1], sd->status.inventory[i].card[2]) );
				//clear egg flag. so it won't be put in IMPORTANT items (eggs look like item with 2 cards ^_^)
				sd->status.inventory[i].card[1] = sd->status.inventory[i].card[0] = 0;
				//now this egg'll be deleted as a common unimportant item
			}
			//is this item important? does it have cards? or Player's name? or Refined/Upgraded
			if( sd->status.inventory[i].card[0] || sd->status.inventory[i].card[1] ||
				sd->status.inventory[i].card[2] || sd->status.inventory[i].card[3] || sd->status.inventory[i].refine)
			{
				//this is important item, count it
				important_item++;
				continue;
			}
			if(sd->status.inventory[i].amount>=amount)
			{
				pc_delitem(*sd,i,amount,0);
				return 0; //we deleted exact amount of items. now exit
			}
			else
			{
				amount-=sd->status.inventory[i].amount;
				pc_delitem(*sd,i,sd->status.inventory[i].amount,0);
			}
		}
		//2nd pass
		//now if there WERE items with CARDs/REFINED/NAMED... and if we still have to delete some items. we'll delete them finally
		if (important_item>0 && amount>0)
		{
			for(i=0;i<MAX_INVENTORY && amount>0;i++)
			{
				//we don't delete wrong item
				if( sd->status.inventory[i].nameid!=nameid || sd->inventory_data[i] == NULL ||
					sd->status.inventory[i].nameid>=20000  || sd->status.inventory[i].amount>=MAX_AMOUNT )
					continue;

				if(sd->status.inventory[i].amount>=amount)
				{
					pc_delitem(*sd,i,amount,0);
					return 0; //we deleted exact amount of items. now exit
				}
				else
				{
					amount-=sd->status.inventory[i].amount;
					pc_delitem(*sd,i,sd->status.inventory[i].amount,0);
				}
			}
		}
		if(amount>0) 
			ShowWarning("delitem (item %i) on player %s failed, missing %i pieces\n", nameid, sd->status.name, amount);
	}
	return 0;
}

/*==========================================
 *�L�����֌W�̃p�����[�^�擾
 *------------------------------------------
 */
int buildin_readparam(CScriptEngine &st)
{
	int type;
	struct map_session_data *sd;

	type=st.GetInt( (st[2]));
	if( st.end>st.start+3 )
		sd=map_nick2sd(st.GetString((st[3])));
	else
		sd=script_rid2sd(st);

	st.push_val(CScriptEngine::C_INT, (sd) ? pc_readparam(*sd,type) : -1);
	return 0;
}
/*==========================================
 *�L�����֌W��ID�擾
 *------------------------------------------
 */
int buildin_getcharid(CScriptEngine &st)
{
	int num, val=-1;
	struct map_session_data *sd;

	num=st.GetInt( (st[2]));
	if( st.end>st.start+3 )
		sd=map_nick2sd(st.GetString((st[3])));
	else
		sd=script_rid2sd(st);
	if(sd)
	{
		if(num==0) val = sd->status.char_id;
		else if(num==1) val = sd->status.party_id;
		else if(num==2) val = sd->status.guild_id;
		else if(num==3) val = sd->status.account_id;
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}
/*==========================================
 *�w��ID��PT���擾
 *------------------------------------------
 */
char *buildin_getpartyname_sub(unsigned long party_id)
{
	struct party *p;

	p=NULL;
	p=party_search(party_id);

	if(p!=NULL){
		char *buf;
		buf=(char *)aMalloc(24*sizeof(char));
		memcpy(buf, p->name, 24);
		return buf;
	}
	return NULL;
}
int buildin_getpartyname(CScriptEngine &st)
{
	char *name;
	int party_id;

	party_id=st.GetInt( (st[2]));
	name=buildin_getpartyname_sub(party_id);
	if(name!=0)
		st.push_str(CScriptEngine::C_STR, name);
	else
		st.push_str(CScriptEngine::C_CONSTSTR, "(not in party)");

	return 0;
}
/*==========================================
 *�w��ID��PT�l���ƃ����o�[ID�擾
 *------------------------------------------
 */
int buildin_getpartymember(CScriptEngine &st)
{
	struct party *p;
	int i,j=0;

	p=party_search(st.GetInt( (st[2])));
	if(p!=NULL)
	{
		for(i=0;i<MAX_PARTY;i++){
			if(p->member[i].account_id){
//				ShowMessage("name:%s %d\n",p->member[i].name,i);
				mapreg_setregstr(add_str("$@partymembername$")+(i<<24),p->member[i].name);
				j++;
			}
		}
	}
	mapreg_setreg(add_str( "$@partymembercount"),j);

	return 0;
}
/*==========================================
 *�w��ID�̃M���h���擾
 *------------------------------------------
 */
char *buildin_getguildname_sub(int guild_id)
{
	struct guild *g=NULL;
	g=guild_search(guild_id);

	if(g!=NULL){
		char *buf;
		buf=(char *)aMalloc(24*sizeof(char));
		memcpy(buf, g->name, 24);
		return buf;
	}
	return 0;
}
int buildin_getguildname(CScriptEngine &st)
{
	char *name;
	int guild_id=st.GetInt( (st[2]));
	name=buildin_getguildname_sub(guild_id);
	if(name!=0)
		st.push_str(CScriptEngine::C_STR, name);
	else
		st.push_str(CScriptEngine::C_CONSTSTR, "(not in guild)");
	return 0;
}

/*==========================================
 *�w��ID��GuildMaster���擾
 *------------------------------------------
 */
char *buildin_getguildmaster_sub(int guild_id)
{
	struct guild *g=NULL;
	g=guild_search(guild_id);
	if(g!=NULL){
		char *buf;
		buf=(char *)aMalloc(24*sizeof(char));
		memcpy(buf,g->master, 24);//EOS included
		return buf;
	}
	return NULL;
}
int buildin_getguildmaster(CScriptEngine &st)
{
	char *master;
	int guild_id=st.GetInt( (st[2]));
	master=buildin_getguildmaster_sub(guild_id);
	if(master!=0)
		st.push_str(CScriptEngine::C_STR, master);
	else
		st.push_str(CScriptEngine::C_CONSTSTR, "(not available)");
	return 0;
}

int buildin_getguildmasterid(CScriptEngine &st)
{
	char *master;
	struct map_session_data *sd=NULL;
	int val=0;
	int guild_id=st.GetInt( (st[2]));
	master=buildin_getguildmaster_sub(guild_id);
	if( master &&  (sd=map_nick2sd(master)) != NULL )
		val = sd->status.char_id;

	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}

/*==========================================
 * �L�����N�^�̖��O
 *------------------------------------------
 */
int buildin_strcharinfo(CScriptEngine &st)
{
	struct map_session_data *sd;
	int num;

	sd=script_rid2sd(st);
	num=st.GetInt( (st[2]));
	if(num==0){
		char *buf;
		buf=(char *)aMalloc(24*sizeof(char));
		memcpy(buf,sd->status.name, 24);//EOS included
		st.push_str(CScriptEngine::C_STR, buf);
	}
	if(num==1){
		char *buf;
		buf=buildin_getpartyname_sub(sd->status.party_id);
		if(buf!=0)
			st.push_str(CScriptEngine::C_STR, buf);
		else
			st.push_str(CScriptEngine::C_CONSTSTR, "");
	}
	if(num==2){
		char *buf;
		buf=buildin_getguildname_sub(sd->status.guild_id);
		if(buf!=0)
			st.push_str(CScriptEngine::C_STR, buf);
		else
			st.push_str(CScriptEngine::C_CONSTSTR, "");
	}

	return 0;
}

unsigned short equip[10]={0x0100,0x0010,0x0020,0x0002,0x0004,0x0040,0x0008,0x0080,0x0200,0x0001};

/*==========================================
 * GetEquipID(Pos);     Pos: 1-10
 *------------------------------------------
 */
int buildin_getequipid(CScriptEngine &st)
{
	unsigned short itempos,num;
	struct map_session_data *sd = script_rid2sd(st);
	struct item_data* item;
	int val = -1;

	if(sd)
	{
		num = st.GetInt( (st[2])) - 1;
		if(num<10)
		{
			itempos=pc_checkequip(*sd,equip[num]);
			if(itempos < MAX_INVENTORY)
			{
				item=sd->inventory_data[itempos];
				val = (item) ? item->nameid : 0;
			}
		}
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}

/*==========================================
 * ������������i���B���j���[�p�j
 *------------------------------------------
 */
int buildin_getequipname(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	struct item_data* item;
	unsigned short num, itempos;
	char *buf = (char *)aCalloc(128,sizeof(char)); // string is clear by default
	if(sd)
	{
		num = st.GetInt( (st[2])) - 1;
		if(num<10)
		{
			itempos=pc_checkequip(*sd,equip[num]);
			if(itempos < MAX_INVENTORY && (item=sd->inventory_data[itempos])!=NULL)
				snprintf(buf,sizeof(buf),"%s-[%s]",positions[num],item->jname);
			else
				snprintf(buf,sizeof(buf),"%s-[%s]",positions[num],positions[10]);
			buf[sizeof(buf)-1]=0;
		}
	}
	st.push_str(CScriptEngine::C_STR, buf);
	return 0;
}

/*==========================================
 * getbrokenid [Valaris]
 *------------------------------------------
 */
int buildin_getbrokenid(CScriptEngine &st)
{
	unsigned short itempos;
	size_t count, brokencounter=0;
	struct map_session_data *sd;
	int itemid=0;

	sd=script_rid2sd(st);
	if(sd)
	{
		count = st.GetInt( (st[2]));
		for(itempos=0; itempos<MAX_INVENTORY; itempos++)
		{
			if(sd->status.inventory[itempos].attribute==1)
			{
				brokencounter++;
				if(count==brokencounter)
				{
					itemid=sd->status.inventory[itempos].nameid;
					break;
				}
			}
		}
	}
	st.push_val(CScriptEngine::C_INT,itemid);
	return 0;
}

/*==========================================
 * repair [Valaris]
 *------------------------------------------
 */
int buildin_repair(CScriptEngine &st)
{
	unsigned short itempos;
	size_t count, repaircounter=0;
	struct map_session_data *sd=script_rid2sd(st);

	if(sd)
	{
		count=st.GetInt( (st[2]));
		for(itempos=0; itempos<MAX_INVENTORY; itempos++)
		{
			if(sd->status.inventory[itempos].attribute==1)
			{
				repaircounter++;
				if(count==repaircounter)
				{
					sd->status.inventory[itempos].attribute=0;
					clif_equiplist(*sd);
					clif_produceeffect(*sd, sd->status.inventory[itempos].nameid, 0);
					clif_misceffect(sd->bl, 3);
					clif_displaymessage(sd->fd,"Item has been repaired.");
					break;
				}
			}
		}
	}
	return 0;
}

/*==========================================
 * �����`�F�b�N
 *------------------------------------------
 */
int buildin_getequipisequiped(CScriptEngine &st)
{
	unsigned short itempos,num;
	struct map_session_data *sd=script_rid2sd(st);
	int val=0;

	if(sd)
	{
		num=st.GetInt( (st[2])) - 1;
		if(num<10)
		{
			itempos=pc_checkequip(*sd,equip[num]);
			if(itempos < MAX_INVENTORY)
				val = 1;
		}
	}
	st.push_val(CScriptEngine::C_INT,val);

	return 0;
}

/*==========================================
 * �����i���B�\�`�F�b�N
 *------------------------------------------
 */
int buildin_getequipisenableref(CScriptEngine &st)
{
	unsigned short itempos,num;
	struct map_session_data *sd = script_rid2sd(st);
	int val=0;

	if(sd)
	{
		num=st.GetInt( (st[2])) - 1;
		if(num<10)
		{
			itempos=pc_checkequip(*sd,equip[num]);
			if( itempos<MAX_INVENTORY && num<7 && 
				sd->inventory_data[itempos] && !sd->inventory_data[itempos]->flag.no_refine)
				val = 1;
		}
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}

/*==========================================
 * �����i�Ӓ�`�F�b�N
 *------------------------------------------
 */
int buildin_getequipisidentify(CScriptEngine &st)
{
	unsigned short itempos, num;
	struct map_session_data *sd = script_rid2sd(st);
	int val = 0;

	if(sd)
	{
		num=st.GetInt( (st[2])) - 1;
		if(num<10)
		{
			itempos=pc_checkequip(*sd,equip[num]);
			if(itempos < MAX_INVENTORY)
				val = sd->status.inventory[itempos].identify;
		}
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}

/*==========================================
 * �����i���B�x
 *------------------------------------------
 */
int buildin_getequiprefinerycnt(CScriptEngine &st)
{
	unsigned short itempos,num;
	struct map_session_data *sd=script_rid2sd(st);
	int val = 0;

	if(sd)
	{
		num=st.GetInt( (st[2])) - 1;
		if(num<10)
		{
			itempos=pc_checkequip(*sd,equip[num]);
			if(itempos < MAX_INVENTORY)
				val = sd->status.inventory[itempos].refine;
		}
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}

/*==========================================
 * �����i����LV
 *------------------------------------------
 */
int buildin_getequipweaponlv(CScriptEngine &st)
{
	unsigned short itempos,num;
	struct map_session_data *sd=script_rid2sd(st);
	int val = 0;

	if(sd)
	{
		num=st.GetInt( (st[2])) - 1;
		if(num<10)
		{
			itempos=pc_checkequip(*sd,equip[num]);
			if(itempos < MAX_INVENTORY && sd->inventory_data[itempos])
				val = sd->inventory_data[itempos]->wlv;
		}
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}

/*==========================================
 * �����i���B������
 *------------------------------------------
 */
int buildin_getequippercentrefinery(CScriptEngine &st)
{
	unsigned short itempos,num;
	struct map_session_data *sd = script_rid2sd(st);
	int val = 0;

	if(sd)
	{
		num=st.GetInt( (st[2])) - 1;
		if(num<10)
		{
			itempos=pc_checkequip(*sd,equip[num]);
			if(itempos < MAX_INVENTORY)
				val = status_percentrefinery(*sd,sd->status.inventory[itempos]);
		}
	}
	st.push_val(CScriptEngine::C_INT,val);

	return 0;
}

/*==========================================
 * ���B����
 *------------------------------------------
 */
int buildin_successrefitem(CScriptEngine &st)
{
	unsigned short itempos,num, equippos;
	struct map_session_data *sd=script_rid2sd(st);

	if(sd)
	{
		num=st.GetInt( (st[2])) - 1;
		if(num<10)
		{
			itempos=pc_checkequip(*sd,equip[num]);
			if(itempos < MAX_INVENTORY)
			{
				equippos=sd->status.inventory[itempos].equip;
				if(log_config.refine > 0)
					log_refine(*sd, itempos, 1);
				
				sd->status.inventory[itempos].refine++;
				pc_unequipitem(*sd,itempos,2);
				clif_refine(sd->fd,*sd,0,itempos,sd->status.inventory[itempos].refine);
				clif_delitem(*sd,itempos,1);
				clif_additem(*sd,itempos,1,0);
				pc_equipitem(*sd,itempos,equippos);
				clif_misceffect(sd->bl,3);
				if( sd->status.inventory[itempos].refine == 10 && 
					sd->status.inventory[itempos].card[0] == 0x00ff && 
					sd->status.char_id == MakeDWord(sd->status.inventory[itempos].card[2],sd->status.inventory[itempos].card[3])  )
				{	// Fame point system [DracoRPG]
					switch (sd->inventory_data[itempos]->wlv)
					{
					 // Success to refine to +10 a lv1 weapon you forged = +1 fame point
					case 1:
						pc_addfame(*sd,1,0);
						break;
					// Success to refine to +10 a lv2 weapon you forged = +25 fame point
					case 2:
						pc_addfame(*sd,25,0);
						break;
					// Success to refine to +10 a lv3 weapon you forged = +1000 fame point
					case 3:
						pc_addfame(*sd,1000,0);
						break;
					}
				}
			}
		}
	}
	return 0;
}

/*==========================================
 * ���B���s
 *------------------------------------------
 */
int buildin_failedrefitem(CScriptEngine &st)
{
	unsigned short itempos,num;
	struct map_session_data *sd=script_rid2sd(st);

	if(sd)
	{
		num=st.GetInt( (st[2])) - 1;
		if(num<10)
		{
			itempos=pc_checkequip(*sd,equip[num]);
			if(itempos < MAX_INVENTORY)
			{
				if(log_config.refine > 0)
					log_refine(*sd, itempos, 0);
				sd->status.inventory[itempos].refine = 0;
				pc_unequipitem(*sd,itempos,3);
				// ���B���s�G�t�F�N�g�̃p�P�b�g
				clif_refine(sd->fd,*sd,1,itempos,sd->status.inventory[itempos].refine);
				pc_delitem(*sd,itempos,1,0);
				// ���̐l�ɂ����s��ʒm
				clif_misceffect(sd->bl,2);
			}
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_statusup(CScriptEngine &st)
{
	int type;
	struct map_session_data *sd;

	type=st.GetInt( (st[2]));
	sd=script_rid2sd(st);
	if(sd) pc_statusup(*sd,type);

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_statusup2(CScriptEngine &st)
{
	int type,val;
	struct map_session_data *sd;

	type=st.GetInt( (st[2]));
	val=st.GetInt( (st[3]));
	sd=script_rid2sd(st);
	if(sd) pc_statusup2(*sd,type,val);

	return 0;
}
/*==========================================
 * �����i�ɂ��\�͒l�{�[�i�X
 *------------------------------------------
 */
int buildin_bonus(CScriptEngine &st)
{
	int type,val;
	struct map_session_data *sd;

	type=st.GetInt( (st[2]));
	val=st.GetInt( (st[3]));
	sd=script_rid2sd(st);
	if(sd) pc_bonus(*sd,type,val);

	return 0;
}
/*==========================================
 * �����i�ɂ��\�͒l�{�[�i�X
 *------------------------------------------
 */
int buildin_bonus2(CScriptEngine &st)
{
	int type,type2,val;
	struct map_session_data *sd;

	type=st.GetInt( (st[2]));
	type2=st.GetInt( (st[3]));
	val=st.GetInt( (st[4]));
	sd=script_rid2sd(st);
	if(sd) pc_bonus2(*sd,type,type2,val);

	return 0;
}
/*==========================================
 * �����i�ɂ��\�͒l�{�[�i�X
 *------------------------------------------
 */
int buildin_bonus3(CScriptEngine &st)
{
	int type,type2,type3,val;
	struct map_session_data *sd;

	type=st.GetInt( (st[2]));
	type2=st.GetInt( (st[3]));
	type3=st.GetInt( (st[4]));
	val=st.GetInt( (st[5]));
	sd=script_rid2sd(st);
	if(sd) pc_bonus3(*sd,type,type2,type3,val);

	return 0;
}

int buildin_bonus4(CScriptEngine &st)
{
	int type,type2,type3,type4,val;
	struct map_session_data *sd;

	type=st.GetInt( (st[2]));
	type2=st.GetInt( (st[3]));
	type3=st.GetInt( (st[4]));
	type4=st.GetInt( (st[5]));
	val=st.GetInt( (st[6]));
	sd=script_rid2sd(st);
	if(sd) pc_bonus4(*sd,type,type2,type3,type4,val);

	return 0;
}
/*==========================================
 * �X�L������
 *------------------------------------------
 */
int buildin_skill(CScriptEngine &st)
{
	int id,level,flag=1;
	struct map_session_data *sd;

	id=st.GetInt( (st[2]));
	level=st.GetInt( (st[3]));
	if( st.end>st.start+4 )
		flag=st.GetInt((st[4]) );
	sd=script_rid2sd(st);
	if(sd) pc_skill(*sd,id,level,flag);

	return 0;
}

// add x levels of skill (stackable) [Valaris]
int buildin_addtoskill(CScriptEngine &st)
{
	int id,level,flag=2;
	struct map_session_data *sd;

	id=st.GetInt( (st[2]));
	level=st.GetInt( (st[3]));
	if( st.end>st.start+4 )
		flag=st.GetInt((st[4]) );
	sd=script_rid2sd(st);
	if(sd) pc_skill(*sd,id,level,flag);

	return 0;
}

/*==========================================
 * �M���h�X�L���擾
 *------------------------------------------
 */
int buildin_guildskill(CScriptEngine &st)
{
	int id,level,flag=0;
	struct map_session_data *sd;
	int i=0;

	id=st.GetInt( (st[2]));
	level=st.GetInt( (st[3]));
	if( st.end>st.start+4 )
		flag=st.GetInt((st[4]) );
	sd=script_rid2sd(st);
	for(i=0;i<level;i++)
		guild_skillup(*sd,id,flag);

	return 0;
}
/*==========================================
 * �X�L�����x������
 *------------------------------------------
 */
int buildin_getskilllv(CScriptEngine &st)
{
	int id=st.GetInt( (st[2]));
	map_session_data *sd = script_rid2sd(st);
	st.push_val(CScriptEngine::C_INT, (sd)?pc_checkskill(*sd,id):0 );
	return 0;
}
/*==========================================
 * getgdskilllv(Guild_ID, Skill_ID);
 * skill_id = 10000 : GD_APPROVAL
 *            10001 : GD_KAFRACONTRACT
 *            10002 : GD_GUARDIANRESEARCH
 *            10003 : GD_GUARDUP
 *            10004 : GD_EXTENSION
 *------------------------------------------
 */
int buildin_getgdskilllv(CScriptEngine &st)
{
	unsigned long guild_id=st.GetInt( (st[2]));
	unsigned short skill_id=st.GetInt( (st[3]));    
	struct guild *g=guild_search(guild_id);
	st.push_val( CScriptEngine::C_INT, (g==NULL)?-1:guild_checkskill(*g,skill_id) );
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_basicskillcheck(CScriptEngine &st)
{
	st.push_val(CScriptEngine::C_INT, battle_config.basic_skill_check);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_getgmlevel(CScriptEngine &st)
{
	map_session_data *sd = script_rid2sd(st);
	st.push_val(CScriptEngine::C_INT, ((sd) ? pc_isGM(*sd):0) );
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_end(CScriptEngine &st)
{
	st.state = CScriptEngine::END;
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_checkoption(CScriptEngine &st)
{
	int type;
	struct map_session_data *sd;

	type=st.GetInt( (st[2]));
	sd=script_rid2sd(st);

	if(sd->status.option & type){
		st.push_val(CScriptEngine::C_INT,1);
	} else {
		st.push_val(CScriptEngine::C_INT,0);
	}

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_checkoption1(CScriptEngine &st)
{
	int type;
	struct map_session_data *sd;

	type=st.GetInt( (st[2]));
	sd=script_rid2sd(st);

	if(sd->opt1 & type){
		st.push_val(CScriptEngine::C_INT,1);
	} else {
		st.push_val(CScriptEngine::C_INT,0);
	}

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_checkoption2(CScriptEngine &st)
{
	int type;
	struct map_session_data *sd;

	type=st.GetInt( (st[2]));
	sd=script_rid2sd(st);

	if(sd->opt2 & type){
		st.push_val(CScriptEngine::C_INT,1);
	} else {
		st.push_val(CScriptEngine::C_INT,0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_setoption(CScriptEngine &st)
{
	int type;
	struct map_session_data *sd=script_rid2sd(st);
	type=st.GetInt( (st[2]));
	if(sd) pc_setoption(*sd,type);
	return 0;
}

/*==========================================
 * Checkcart [Valaris]
 *------------------------------------------
 */

int buildin_checkcart(CScriptEngine &st)
{
	struct map_session_data *sd;

	sd=script_rid2sd(st);

	if(sd && pc_iscarton(*sd)){
		st.push_val(CScriptEngine::C_INT,1);
	} else {
		st.push_val(CScriptEngine::C_INT,0);
	}
	return 0;
}

/*==========================================
 * �J�[�g��t����
 *------------------------------------------
 */
int buildin_setcart(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if(sd) pc_setcart(*sd,1);

	return 0;
}

/*==========================================
 * checkfalcon [Valaris]
 *------------------------------------------
 */

int buildin_checkfalcon(CScriptEngine &st)
{
	struct map_session_data *sd;

	sd=script_rid2sd(st);

	if(sd && pc_isfalcon(*sd)){
		st.push_val(CScriptEngine::C_INT,1);
	} else {
		st.push_val(CScriptEngine::C_INT,0);
	}

	return 0;
}


/*==========================================
 * ���t����
 *------------------------------------------
 */
int buildin_setfalcon(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if(sd) pc_setfalcon(*sd);

	return 0;
}

/*==========================================
 * Checkcart [Valaris]
 *------------------------------------------
 */

int buildin_checkriding(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if(sd && pc_isriding(*sd))
		st.push_val(CScriptEngine::C_INT,1);
	else
		st.push_val(CScriptEngine::C_INT,0);

	return 0;
}


/*==========================================
 * �y�R�y�R���
 *------------------------------------------
 */
int buildin_setriding(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if(sd) pc_setriding(*sd);
	return 0;
}

/*==========================================
 *	�Z�[�u�|�C���g�̕ۑ�
 *------------------------------------------
 */
int buildin_savepoint(CScriptEngine &st)
{
	int x,y;
	const char *str;

	str=st.GetString((st[2]));
	x=st.GetInt( (st[3]));
	y=st.GetInt( (st[4]));
	map_session_data *sd = script_rid2sd(st);
	if(sd) pc_setsavepoint(*sd,str,x,y);
	return 0;
}

/*==========================================
 * GetTimeTick(0: System Tick, 1: Time Second Tick)
 *------------------------------------------
 */
int buildin_gettimetick(CScriptEngine &st)	/* Asgard Version */
{
	int type;
	time_t timer;
	struct tm *t;

	type=st.GetInt( (st[2]));

	switch(type){
	case 1:
		//type 1:(Second Ticks: 0-86399, 00:00:00-23:59:59)
		time(&timer);
		t=localtime(&timer);
		st.push_val(CScriptEngine::C_INT,((t->tm_hour)*3600+(t->tm_min)*60+t->tm_sec));
		break;
	case 0:
	default:
		//type 0:(System Ticks)
		st.push_val(CScriptEngine::C_INT,gettick());
		break;
	}
	return 0;
}

/*==========================================
 * GetTime(Type);
 * 1: Sec     2: Min     3: Hour
 * 4: WeekDay     5: MonthDay     6: Month
 * 7: Year
 *------------------------------------------
 */
int buildin_gettime(CScriptEngine &st)	/* Asgard Version */
{
	int type;
	time_t timer;
	struct tm *t;

	type=st.GetInt( (st[2]));

	time(&timer);
	t=localtime(&timer);

	switch(type){
	case 1://Sec(0~59)
		st.push_val(CScriptEngine::C_INT,t->tm_sec);
		break;
	case 2://Min(0~59)
		st.push_val(CScriptEngine::C_INT,t->tm_min);
		break;
	case 3://Hour(0~23)
		st.push_val(CScriptEngine::C_INT,t->tm_hour);
		break;
	case 4://WeekDay(0~6)
		st.push_val(CScriptEngine::C_INT,t->tm_wday);
		break;
	case 5://MonthDay(01~31)
		st.push_val(CScriptEngine::C_INT,t->tm_mday);
		break;
	case 6://Month(01~12)
		st.push_val(CScriptEngine::C_INT,t->tm_mon+1);
		break;
	case 7://Year(20xx)
		st.push_val(CScriptEngine::C_INT,t->tm_year+1900);
		break;
	default://(format error)
		st.push_val(CScriptEngine::C_INT,-1);
		break;
	}
	return 0;
}

/*==========================================
 * GetTimeStr("TimeFMT", Length);
 *------------------------------------------
 */
int buildin_gettimestr(CScriptEngine &st)
{
	char *tmpstr;
	const char *fmtstr;
	int maxlen;
	time_t now = time(NULL);

	fmtstr=st.GetString((st[2]));
	maxlen=st.GetInt( (st[3]));

	tmpstr=(char *)aMalloc((maxlen+1)*sizeof(char));
	strftime(tmpstr,maxlen,fmtstr,localtime(&now));
	tmpstr[maxlen]='\0';

	st.push_str(CScriptEngine::C_STR, tmpstr);
	return 0;
}

/*==========================================
 * �J�v���q�ɂ��J��
 *------------------------------------------
 */
int buildin_openstorage(CScriptEngine &st)
{
	struct map_session_data *sd = script_rid2sd(st);
	int ret=0;
	if(sd) ret = storage_storageopen(*sd);
	st.push_val(CScriptEngine::C_INT,ret);
	return 0;
}

int buildin_guildopenstorage(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int ret=0;
	if(sd) ret = storage_guild_storageopen(*sd);
	st.push_val(CScriptEngine::C_INT,ret);
	return 0;
}

/*==========================================
 * �A�C�e���ɂ��X�L������
 *------------------------------------------
 */
int buildin_itemskill(CScriptEngine &st)
{
	int id,lv;
	const char *str;
	struct map_session_data *sd=script_rid2sd(st);

	id=st.GetInt( (st[2]));
	lv=st.GetInt( (st[3]));
	str=st.GetString((st[4]));

	if(sd)
	{
		// �r�����ɃX�L���A�C�e���͎g�p�ł��Ȃ�
		if(sd->skilltimer == -1)
		{
			sd->skillitem=id;
			sd->skillitemlv=lv;
			clif_item_skill(*sd,id,lv,str);
		}
	}
	return 0;
}
/*==========================================
 * �A�C�e���쐬
 *------------------------------------------
 */
int buildin_produce(CScriptEngine &st)
{
	int trigger;
	struct map_session_data *sd=script_rid2sd(st);

	if(	sd->state.produce_flag == 1) return 0;
	trigger=st.GetInt( (st[2]));
	clif_skill_produce_mix_list(*sd,trigger);
	return 0;
}
/*==========================================
 * NPC�Ńy�b�g���
 *------------------------------------------
 */
int buildin_makepet(CScriptEngine &st)
{
	struct map_session_data *sd = script_rid2sd(st);
	CScriptEngine::CValue *data;
	int id,pet_id;

	data=&(st[2]);
	st.ConvertName(*data);

	id=st.GetInt(*data);

	pet_id = search_petDB_index(id, PET_CLASS);

	if (pet_id < 0)
		pet_id = search_petDB_index(id, PET_EGG);
	if (pet_id >= 0 && sd) {
		sd->catch_target_class = pet_db[pet_id].class_;
		intif_create_pet(
			sd->status.account_id, sd->status.char_id,				//long
			pet_db[pet_id].class_, mob_db[pet_db[pet_id].class_].lv,//short
			pet_db[pet_id].EggID, 0, pet_db[pet_id].intimate, 100,	//short
			0, 1,													//char
			pet_db[pet_id].jname);									//char*
	}

	return 0;
}
/*==========================================
 * NPC�Ōo���l�グ��
 *------------------------------------------
 */
int buildin_getexp(CScriptEngine &st)
{
	struct map_session_data *sd = script_rid2sd(st);
	int base=0,job=0;

	base=st.GetInt( (st[2]));
	job =st.GetInt( (st[3]));
	if(base<0 || job<0)
		return 0;
	if(sd)
		pc_gainexp(*sd,base,job);
	return 0;
}

/*==========================================
 * Gain guild exp [Celest]
 *------------------------------------------
 */
int buildin_guildgetexp(CScriptEngine &st)
{
	struct map_session_data *sd = script_rid2sd(st);
	int exp;

	exp = st.GetInt( (st[2]));
	if(exp < 0)
		return 0;
	if(sd && sd->status.guild_id > 0)
		guild_getexp (*sd, exp);

	return 0;
}

/*==========================================
 * �����X�^�[����
 *------------------------------------------
 */
int buildin_monster(CScriptEngine &st)
{
	int class_,amount,x,y;
	const char *str,*map,*event="";

	map	=st.GetString((st[2]));
	x	=st.GetInt( (st[3]));
	y	=st.GetInt( (st[4]));
	str	=st.GetString((st[5]));
	class_=st.GetInt( (st[6]));
	amount=st.GetInt( (st[7]));
	if( st.end>st.start+8 )
		event=st.GetString((st[8]));
//!! broadcast command if not on this mapserver
	map_session_data *sd = map_id2sd(st.rid);
	mob_once_spawn(sd,map,x,y,str,class_,amount,event);
	return 0;
}
/*==========================================
 * �����X�^�[����
 *------------------------------------------
 */
int buildin_areamonster(CScriptEngine &st)
{
	int class_,amount,x0,y0,x1,y1;
	const char *str,*map,*event="";

	map	=st.GetString( (st[2]));
	x0	=st.GetInt( (st[3]));
	y0	=st.GetInt( (st[4]));
	x1	=st.GetInt( (st[5]));
	y1	=st.GetInt( (st[6]));
	str	=st.GetString( (st[7]));
	class_=st.GetInt( (st[8]));
	amount=st.GetInt( (st[9]));
	if( st.end>st.start+10 )
		event=st.GetString( (st[10]));
//!! broadcast command if not on this mapserver
	mob_once_spawn_area(map_id2sd(st.rid),map,x0,y0,x1,y1,str,class_,amount,event);
	return 0;
}
/*==========================================
 * �����X�^�[�폜
 *------------------------------------------
 */
int buildin_killmonster_sub(struct block_list &bl,va_list ap)
{
	struct mob_data &md =(struct mob_data&)bl;
	char *event=va_arg(ap,char *);
	int allflag=va_arg(ap,int);

	if(allflag)
	{	// delete all script-summoned mobs
		if( !md.cache  )
			mob_unload(md);
	}
	else
	{	// delete only mobs with same event name
		if(strcmp(event, md.npc_event)==0)
			mob_remove_map(md, 0);
	}
	return 0;
}

int buildin_killmonster(CScriptEngine &st)
{
	const char *mapname,*event;
	int m,allflag=0;
	mapname=st.GetString( (st[2]));
	event=st.GetString( (st[3]));
	if(strcmp(event,"All")==0)
		allflag = 1;

	if( (m=map_mapname2mapid(mapname))<0 )
		return 0;
//!! broadcast command if not on this mapserver
	map_foreachinarea(buildin_killmonster_sub,
		m,0,0,map[m].xs-1,map[m].ys-1,BL_MOB, event ,allflag);
	return 0;
}

int buildin_killmonsterall_sub(struct block_list &bl,va_list ap)
{
	mob_remove_map((struct mob_data &)bl, 1);
	return 0;
}
int buildin_killmonsterall(CScriptEngine &st)
{
	const char *mapname;
	int m;
	mapname=st.GetString( (st[2]));

	if( (m=map_mapname2mapid(mapname))<0 )
		return 0;
//!! broadcast command if not on this mapserver
	map_foreachinarea(buildin_killmonsterall_sub,
		m,0,0,map[m].xs-1,map[m].ys-1,BL_MOB);
	return 0;
}

/*==========================================
 * �C�x���g���s
 *------------------------------------------
 */
int buildin_doevent(CScriptEngine &st)
{
	const char *event;
	event=st.GetString( (st[2]));
//!! broadcast command if not on this mapserver
	map_session_data *sd = map_id2sd(st.rid);
	if(sd) npc_event(*sd,event,0);
	return 0;
}
/*==========================================
 * NPC��̃C�x���g���s
 *------------------------------------------
 */
int buildin_donpcevent(CScriptEngine &st)
{
	const char *event;
	event=st.GetString( (st[2]));
	npc_event_do(event);
	return 0;
}
/*==========================================
 * �C�x���g�^�C�}�[�ǉ�
 *------------------------------------------
 */
int buildin_addtimer(CScriptEngine &st)
{
	const char *event;
	unsigned long tick;
	tick=st.GetInt( (st[2]));
	event=st.GetString( (st[3]));
	map_session_data *sd = script_rid2sd(st);
	if(sd) pc_addeventtimer(*sd,tick,event);
	return 0;
}
/*==========================================
 * �C�x���g�^�C�}�[�폜
 *------------------------------------------
 */
int buildin_deltimer(CScriptEngine &st)
{
	const char *event;
	event=st.GetString( (st[2]));
	map_session_data *sd = script_rid2sd(st);
	if(sd) pc_deleventtimer(*sd,event);
	return 0;
}
/*==========================================
 * �C�x���g�^�C�}�[�̃J�E���g�l�ǉ�
 *------------------------------------------
 */
int buildin_addtimercount(CScriptEngine &st)
{
	const char *event;
	unsigned long tick;
	event=st.GetString( (st[2]));
	tick=st.GetInt( (st[3]));
	map_session_data *sd = script_rid2sd(st); 
	if(sd) pc_addeventtimercount(*sd,event,tick);
	return 0;
}

/*==========================================
 * NPC�^�C�}�[������
 *------------------------------------------
 */
int buildin_initnpctimer(CScriptEngine &st)
{
	struct npc_data *nd;
	if( st.end > st.start+2 )
		nd=npc_name2id(st.GetString( (st[2])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);

	if(nd)
	{
		npc_settimerevent_tick(*nd,0);
		npc_timerevent_start(*nd, st.rid);
	}
	return 0;
}
/*==========================================
 * NPC�^�C�}�[�J�n
 *------------------------------------------
 */
int buildin_startnpctimer(CScriptEngine &st)
{
	struct npc_data *nd;
	if( st.end > st.start+2 )
		nd=npc_name2id(st.GetString( (st[2])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);

	if(nd)
	{
		npc_timerevent_start(*nd, st.rid);
	}
	return 0;
}
/*==========================================
 * NPC�^�C�}�[��~
 *------------------------------------------
 */
int buildin_stopnpctimer(CScriptEngine &st)
{
	struct npc_data *nd;
	if( st.end > st.start+2 )
		nd=npc_name2id(st.GetString( (st[2])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);

	if(nd) npc_timerevent_stop(*nd);
	return 0;
}
/*==========================================
 * NPC�^�C�}�[��񏊓�
 *------------------------------------------
 */
int buildin_getnpctimer(CScriptEngine &st)
{
	struct npc_data *nd;
	int type=st.GetInt( (st[2]));
	int val=0;
	if( st.end > st.start+3 )
		nd=npc_name2id(st.GetString( (st[3])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);

	if(nd)
	{
	switch(type){
		case 0: val=npc_gettimerevent_tick(*nd); break;
	case 1: val= (nd->u.scr.nexttimer>=0); break;
	case 2: val= nd->u.scr.timeramount; break;
	}
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}
/*==========================================
 * NPC�^�C�}�[�l�ݒ�
 *------------------------------------------
 */
int buildin_setnpctimer(CScriptEngine &st)
{
	unsigned long tick;
	struct npc_data *nd;
	tick=st.GetInt( (st[2]));
	if( st.end > st.start+3 )
		nd=npc_name2id(st.GetString( (st[3])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);

	if(nd)
	{
		npc_settimerevent_tick(*nd,tick);
	}
	return 0;
}

/*==========================================
 * attaches the player rid to the timer [Celest]
 *------------------------------------------
 */
int buildin_attachnpctimer(CScriptEngine &st)
{
	struct map_session_data *sd;
	struct npc_data *nd;

	nd=(struct npc_data *)map_id2bl(st.oid);
	if( st.end > st.start+2 ) {
		const char *name = st.GetString( (st[2]));
		sd=map_nick2sd(name);
	} else {
		sd = script_rid2sd(st);
	}
	if(sd) nd->u.scr.rid = sd->bl.id;
		return 0;
}

/*==========================================
 * detaches a player rid from the timer [Celest]
 *------------------------------------------
 */
int buildin_detachnpctimer(CScriptEngine &st)
{
	struct npc_data *nd;
	if( st.end > st.start+2 )
		nd=npc_name2id(st.GetString( (st[2])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);

	nd->u.scr.rid = 0;
	return 0;
}

/*==========================================
 * �V�̐��A�i�E���X
 *------------------------------------------
 */
int buildin_announce(CScriptEngine &st)
{
	const char *str;
	int flag;
	str=st.GetString( (st[2]));
	flag=st.GetInt( (st[3]));

	if(flag&0x0f){
		struct block_list *bl=(flag&0x08)? map_id2bl(st.oid) :
			(struct block_list *)script_rid2sd(st);
		clif_GMmessage(bl,str,flag);
	}else
		intif_GMmessage(str,flag);
	return 0;
}
/*==========================================
 * �V�̐��A�i�E���X�i����}�b�v�j
 *------------------------------------------
 */
int buildin_mapannounce_sub(struct block_list &bl,va_list ap)
{
	char *str;
	int len,flag;
	str=va_arg(ap,char *);
	len=va_arg(ap,int);
	flag=va_arg(ap,int);
	clif_GMmessage(&bl,str,flag|3);
	return 0;
}
int buildin_mapannounce(CScriptEngine &st)
{
	const char *mapname,*str;
	int flag,m;

	mapname=st.GetString( (st[2]));
	str=st.GetString( (st[3]));
	flag=st.GetInt( (st[4]));

	if( (m=map_mapname2mapid(mapname))<0 )
		return 0;
//!! broadcast command if not on this mapserver
	map_foreachinarea(buildin_mapannounce_sub,
		m,0,0,map[m].xs-1,map[m].ys-1,BL_PC, str,strlen(str)+1,flag&0x10);
	return 0;
}
/*==========================================
 * �V�̐��A�i�E���X�i����G���A�j
 *------------------------------------------
 */
int buildin_areaannounce(CScriptEngine &st)
{
	const char *map,*str;
	int flag,m;
	int x0,y0,x1,y1;

	map=st.GetString( (st[2]));
	x0=st.GetInt( (st[3]));
	y0=st.GetInt( (st[4]));
	x1=st.GetInt( (st[5]));
	y1=st.GetInt( (st[6]));
	str=st.GetString( (st[7]));
	flag=st.GetInt( (st[8]));

	if( (m=map_mapname2mapid(map))<0 )
		return 0;
//!! broadcast command if not on this mapserver
	map_foreachinarea(buildin_mapannounce_sub,
		m,x0,y0,x1,y1,BL_PC, str,strlen(str)+1,flag&0x10 );
	return 0;
}
/*==========================================
 * ���[�U�[������
 *------------------------------------------
 */
int buildin_getusers(CScriptEngine &st)
{
	int flag=st.GetInt( (st[2]));
	struct block_list *bl=map_id2bl((flag&0x08)?st.oid:st.rid);
	int val=0;
	switch(flag&0x07){
	case 0: val=map[bl->m].users; break;
	case 1: val=map_getusers(); break;
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}
/*==========================================
 * Works like @WHO - displays all online users names in window
 *------------------------------------------
 */
int buildin_getusersname(CScriptEngine &st)
{
	struct map_session_data *pl_sd = NULL;
	size_t i=0,disp_num=1;
	map_session_data *sd = script_rid2sd(st);
	if(sd) 
	for (i=0;i<fd_max;i++)
		if(session[i] && (pl_sd=(struct map_session_data *) session[i]->session_data) && pl_sd->state.auth){
			if( !(battle_config.hide_GM_session && pc_isGM(*pl_sd)) ){
				if((disp_num++)%10==0)
					clif_scriptnext(*sd,st.oid);
				clif_scriptmes(*sd,st.oid,pl_sd->status.name);
			}
		}
	return 0;
}
/*==========================================
 * �}�b�v�w�胆�[�U�[������
 *------------------------------------------
 */
int buildin_getmapusers(CScriptEngine &st)
{
	const char *str;
	int m;
	str=st.GetString( (st[2]));
	if( (m=map_mapname2mapid(str))< 0){
		st.push_val(CScriptEngine::C_INT,-1);
		return 0;
	}
//!! broadcast command if not on this mapserver
	st.push_val(CScriptEngine::C_INT,map[m].users);
	return 0;
}
/*==========================================
 * �G���A�w�胆�[�U�[������
 *------------------------------------------
 */
int buildin_getareausers_sub(struct block_list &bl,va_list ap)
{
	int *users=va_arg(ap,int *);
	(*users)++;
	return 0;
}
int buildin_getareausers(CScriptEngine &st)
{
	const char *str;
	int m,x0,y0,x1,y1,users=0;
	str=st.GetString( (st[2]));
	x0=st.GetInt( (st[3]));
	y0=st.GetInt( (st[4]));
	x1=st.GetInt( (st[5]));
	y1=st.GetInt( (st[6]));
	if( (m=map_mapname2mapid(str))< 0){
		st.push_val(CScriptEngine::C_INT,-1);
		return 0;
	}
//!! broadcast command if not on this mapserver
	map_foreachinarea(buildin_getareausers_sub,
		m,x0,y0,x1,y1,BL_PC,&users);
	st.push_val(CScriptEngine::C_INT,users);
	return 0;
}

/*==========================================
 * �G���A�w��h���b�v�A�C�e��������
 *------------------------------------------
 */
int buildin_getareadropitem_sub(struct block_list &bl,va_list ap)
{
	int item=va_arg(ap,int);
	int *amount=va_arg(ap,int *);
	struct flooritem_data &drop=(struct flooritem_data &)bl;

	if(drop.item_data.nameid==item)
		(*amount)+=drop.item_data.amount;

	return 0;
}
int buildin_getareadropitem(CScriptEngine &st)
{
	const char *str;
	int m,x0,y0,x1,y1,item,amount=0;
	CScriptEngine::CValue &data= st[7];

	str = st.GetString( (st[2]));
	x0  = st.GetInt( (st[3]));
	y0  = st.GetInt( (st[4]));
	x1  = st.GetInt( (st[5]));
	y1  = st.GetInt( (st[6]));

	st.ConvertName(data);
	if( data.isString() ){
		const char *name=st.GetString(data);
		struct item_data *item_data = itemdb_searchname(name);
		item=512;
		if( item_data )
			item=item_data->nameid;
	}else
		item=st.GetInt(data);

	if( (m=map_mapname2mapid(str))< 0){
		st.push_val(CScriptEngine::C_INT,-1);
		return 0;
	}
//!! broadcast command if not on this mapserver
	map_foreachinarea(buildin_getareadropitem_sub,
		m,x0,y0,x1,y1,BL_ITEM,item,&amount);
	st.push_val(CScriptEngine::C_INT,amount);
	return 0;
}
/*==========================================
 * NPC�̗L����
 *------------------------------------------
 */
int buildin_enablenpc(CScriptEngine &st)
{
	const char *str;
	str=st.GetString( (st[2]));
	npc_enable(str,1);
	return 0;
}
/*==========================================
 * NPC�̖�����
 *------------------------------------------
 */
int buildin_disablenpc(CScriptEngine &st)
{
	const char *str;
	str=st.GetString( (st[2]));
	npc_enable(str,0);
	return 0;
}

int buildin_enablearena(CScriptEngine &st)	// Added by RoVeRT
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st.oid);
	struct chat_data *cd;


	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL)
		return 0;

	npc_enable(nd->name,1);
	nd->arenaflag=1;

	if(cd->users>=cd->trigger && cd->npc_event[0])
		npc_timer_event(cd->npc_event);

	return 0;
}
int buildin_disablearena(CScriptEngine &st)	// Added by RoVeRT
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st.oid);
	nd->arenaflag=0;

	return 0;
}
/*==========================================
 * �B��Ă���NPC�̕\��
 *------------------------------------------
 */
int buildin_hideoffnpc(CScriptEngine &st)
{
	const char *str;
	str=st.GetString( (st[2]));
	npc_enable(str,2);
	return 0;
}
/*==========================================
 * NPC���n�C�f�B���O
 *------------------------------------------
 */
int buildin_hideonnpc(CScriptEngine &st)
{
	const char *str;
	str=st.GetString( (st[2]));
	npc_enable(str,4);
	return 0;
}
/*==========================================
 * ��Ԉُ�ɂ�����
 *------------------------------------------
 */
int buildin_sc_start(CScriptEngine &st)
{
	struct block_list *bl;
	int type;
	unsigned long tick;
	int val1;
	type=st.GetInt( (st[2]));
	tick=st.GetInt( (st[3]));
	val1=st.GetInt( (st[4]));
	if( st.end>st.start+5 ) //�w�肵���L��������Ԉُ�ɂ���
		bl = map_id2bl(st.GetInt( (st[5])));
	else
		bl = map_id2bl(st.rid);

	if (bl != 0)
	{
		if(bl->type == BL_PC && ((struct map_session_data *)bl)->state.potion_flag==1)
			bl = map_id2bl(((struct map_session_data *)bl)->skilltarget);
		status_change_start(bl,type,val1,0,0,0,tick,0);
	}
	return 0;
}

/*==========================================
 * ��Ԉُ�ɂ�����(�m���w��)
 *------------------------------------------
 */
int buildin_sc_start2(CScriptEngine &st)
{
	struct block_list *bl;
	int type;
	unsigned long tick;
	int val1,per;
	type=st.GetInt( (st[2]));
	tick=st.GetInt( (st[3]));
	val1=st.GetInt( (st[4]));
	per=st.GetInt( (st[5]));
	if( st.end>st.start+6 ) //�w�肵���L��������Ԉُ�ɂ���
		bl = map_id2bl(st.GetInt( (st[6])));
	else
	bl = map_id2bl(st.rid);
	if(bl->type == BL_PC && ((struct map_session_data *)bl)->state.potion_flag==1)
		bl = map_id2bl(((struct map_session_data *)bl)->skilltarget);
	if(rand()%10000 < per)
	status_change_start(bl,type,val1,0,0,0,tick,0);
	return 0;
}

/*==========================================
 * ��Ԉُ킪����
 *------------------------------------------
 */
int buildin_sc_end(CScriptEngine &st)
{
	struct block_list *bl;
	int type;
	type=st.GetInt( (st[2]));
	bl = map_id2bl(st.rid);
	
	nullpo_retr(0,bl);

	if(bl->type == BL_PC && ((struct map_session_data *)bl)->state.potion_flag==1)
		bl = map_id2bl(((struct map_session_data *)bl)->skilltarget);
	status_change_end(bl,type,-1);
//	if(battle_config.etc_log)
//		ShowMessage("sc_end : %d %d\n",st.rid,type);
	return 0;
}
/*==========================================
 * ��Ԉُ�ϐ����v�Z�����m����Ԃ�
 *------------------------------------------
 */
int buildin_getscrate(CScriptEngine &st)
{
	struct block_list *bl;
	int sc_def,type,rate;

	type=st.GetInt( (st[2]));
	rate=st.GetInt( (st[3]));
	if( st.end>st.start+4 ) //�w�肵���L�����̑ϐ����v�Z����
		bl = map_id2bl(st.GetInt( (st[6])));
	else
		bl = map_id2bl(st.rid);

	sc_def = status_get_sc_def(bl,type);

	rate = rate * sc_def / 100;
	st.push_val(CScriptEngine::C_INT,rate);

	return 0;

}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_debugmes(CScriptEngine &st)
{
	st.GetString( (st[2]));
	ShowMessage("script debug : %d %d : %s\n",st.rid,st.oid,st[2].str);
	return 0;
}

/*==========================================
 *�ߊl�A�C�e���g�p
 *------------------------------------------
 */
int buildin_catchpet(CScriptEngine &st)
{
	int pet_id= st.GetInt( (st[2]));
	struct map_session_data *sd=script_rid2sd(st);
	if(sd) pet_catch_process1(*sd,pet_id);
	return 0;
}

/*==========================================
 *�g�ї��z���@�g�p
 *------------------------------------------
 */
int buildin_birthpet(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if(sd) clif_sendegg(*sd);
	return 0;
}

/*==========================================
 * Added - AppleGirl For Advanced Classes, (Updated for Cleaner Script Purposes)
 *------------------------------------------
 */
int buildin_resetlvl(CScriptEngine &st)
{
	int type=st.GetInt( (st[2]));
	struct map_session_data *sd=script_rid2sd(st);
	if(sd) pc_resetlvl(*sd,type);
	return 0;
}
/*==========================================
 * �X�e�[�^�X���Z�b�g
 *------------------------------------------
 */
int buildin_resetstatus(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if(sd) pc_resetstate(*sd);
	return 0;
}

/*==========================================
 * �X�L�����Z�b�g
 *------------------------------------------
 */
int buildin_resetskill(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if(sd) pc_resetskill(*sd);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_changebase(CScriptEngine &st)
{
	struct map_session_data *sd=NULL;
	int vclass;

	if( st.end>st.start+3 )
		sd=map_id2sd(st.GetInt( (st[3])));
	else
	sd=script_rid2sd(st);

	if(sd == NULL)
		return 0;

	vclass = st.GetInt( (st[2]));
	if( vclass == 22 && 
		(!battle_config.wedding_modifydisplay ||					// Do not show the wedding sprites
		(sd->status.class_ >= 4023 && sd->status.class_ <= 4045)) )	// Baby classes screw up when showing wedding sprites. [Skotlex]
		return 0;

//	if(vclass==22) {
//		pc_unequipitem(sd,sd->equip_index[9],0);	// �����O
//	}

	sd->view_class = vclass;

	return 0;
}

/*==========================================
 * ���ʕϊ�
 *------------------------------------------
 */
int buildin_changesex(CScriptEngine &st) {
	struct map_session_data *sd = NULL;
	sd = script_rid2sd(st);

	if (sd->status.sex == 0) {
		sd->status.sex = 1;
		if (sd->status.class_ == 20 || sd->status.class_ == 4021)
			sd->status.class_ -= 1;
	} else if (sd->status.sex == 1) {
		sd->status.sex = 0;
		if(sd->status.class_ == 19 || sd->status.class_ == 4020)
			sd->status.class_ += 1;
	}
	chrif_char_ask_name(-1, sd->status.name, 5, 0, 0, 0, 0, 0, 0); // type: 5 - changesex
	chrif_save(*sd);
	return 0;
}

/*==========================================
 * npc�`���b�g�쐬
 *------------------------------------------
 */
int buildin_waitingroom(CScriptEngine &st)
{
	const char *name,*ev="";
	int limit, trigger = 0,pub=1;
	name=st.GetString( (st[2]));
	limit= st.GetInt( (st[3]));
	if(limit==0)
		pub=3;

	if( (st.end > st.start+5) ){
		CScriptEngine::CValue* data=&(st[5]);
		st.ConvertName(*data);
		if(data->type==CScriptEngine::C_INT){
			// �VAthena�d�l(��Athena�d�l�ƌ݊�������)
			ev=st.GetString( (st[4]));
			trigger=st.GetInt( (st[5]));
		}else{
			// eathena�d�l
			trigger=st.GetInt( (st[4]));
			ev=st.GetString( (st[5]));
		}
	}else{
		// ��Athena�d�l
		if( st.end > st.start+4 )
			ev=st.GetString( (st[4]));
	}
	struct npc_data *nd = (struct npc_data *)map_id2bl(st.oid);
	if(nd) chat_createnpcchat(*nd,limit,pub,trigger,name,strlen(name)+1,ev);
	return 0;
}
/*==========================================
 * Works like 'announce' but outputs in the common chat window
 *------------------------------------------
 */
int buildin_globalmes(CScriptEngine &st)
{
	struct block_list *bl = map_id2bl(st.oid);
	struct npc_data *nd = (struct npc_data *)bl;
	const char *name=NULL,*mes;

	mes=st.GetString( (st[2]));	// ���b�Z�[�W�̎擾
	if(mes==NULL) return 0;
	
	if(st.end>st.start+3){	// NPC���̎擾(123#456)
		name=st.GetString( (st[3]));
	} else {
		name=nd->name;
	}

	npc_globalmessage(name,mes);	// �O���[�o�����b�Z�[�W���M

	return 0;
}
/*==========================================
 * npc�`���b�g�폜
 *------------------------------------------
 */
int buildin_delwaitingroom(CScriptEngine &st)
{
	struct npc_data *nd;
	if( st.end > st.start+2 )
		nd=npc_name2id(st.GetString( (st[2])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);
	if(nd) chat_deletenpcchat(*nd);
	return 0;
}
/*==========================================
 * npc�`���b�g�S���R��o��
 *------------------------------------------
 */
int buildin_waitingroomkickall(CScriptEngine &st)
{
	struct npc_data *nd;
	struct chat_data *cd;

	if( st.end > st.start+2 )
		nd=npc_name2id(st.GetString( (st[2])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL )
		return 0;
	chat_npckickall(*cd);
	return 0;
}

/*==========================================
 * npc�`���b�g�C�x���g�L����
 *------------------------------------------
 */
int buildin_enablewaitingroomevent(CScriptEngine &st)
{
	struct npc_data *nd;
	struct chat_data *cd;

	if( st.end > st.start+2 )
		nd=npc_name2id(st.GetString( (st[2])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL )
		return 0;
	chat_enableevent(*cd);
	return 0;
}

/*==========================================
 * npc�`���b�g�C�x���g������
 *------------------------------------------
 */
int buildin_disablewaitingroomevent(CScriptEngine &st)
{
	struct npc_data *nd;
	struct chat_data *cd;

	if( st.end > st.start+2 )
		nd=npc_name2id(st.GetString( (st[2])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL )
		return 0;
	chat_disableevent(*cd);
	return 0;
}
/*==========================================
 * npc�`���b�g��ԏ���
 *------------------------------------------
 */
int buildin_getwaitingroomstate(CScriptEngine &st)
{
	struct npc_data *nd;
	struct chat_data *cd;
	int val=0,type;
	type=st.GetInt( (st[2]));
	if( st.end > st.start+3 )
		nd=npc_name2id(st.GetString( (st[3])));
	else
		nd=(struct npc_data *)map_id2bl(st.oid);

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL ){
		st.push_val(CScriptEngine::C_INT,-1);
		return 0;
	}

	switch(type){
	case 0: val=cd->users; break;
	case 1: val=cd->limit; break;
	case 2: val=cd->trigger&0x7f; break;
	case 3: val=((cd->trigger&0x80)>0); break;
	case 32: val=(cd->users >= cd->limit); break;
	case 33: val=(cd->users >= cd->trigger); break;

	case 4:
		st.push_str(CScriptEngine::C_CONSTSTR, cd->title);
		return 0;
	case 5:
		st.push_str(CScriptEngine::C_CONSTSTR, cd->pass);
		return 0;
	case 16:
		st.push_str(CScriptEngine::C_CONSTSTR, cd->npc_event);
		return 0;
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}

/*==========================================
 * �`���b�g�����o�[(�K��l��)���[�v
 *------------------------------------------
 */
int buildin_warpwaitingpc(CScriptEngine &st)
{
	int x,y,i,n;
	const char *str;
	struct npc_data *nd=(struct npc_data *)map_id2bl(st.oid);
	struct chat_data *cd;

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL )
		return 0;

	n=cd->trigger&0x7f;
	str=st.GetString( (st[2]));
	x=st.GetInt( (st[3]));
	y=st.GetInt( (st[4]));

	if( st.end > st.start+5 )
		n=st.GetInt( (st[5]));

	for(i=0;i<n;i++){
		struct map_session_data *sd=cd->usersd[0];	// ���X�g�擪��PC�����X�ɁB

		mapreg_setreg(add_str( "$@warpwaitingpc")+(i<<24),sd->bl.id);

		if(strcmp(str,"Random")==0)
			pc_randomwarp(*sd,3);
		else if(strcmp(str,"SavePoint")==0){
			if(map[sd->bl.m].flag.noteleport)	// �e���|�֎~
				return 0;

			pc_setpos(*sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
		}else
			pc_setpos(*sd,str,x,y,0);
	}
	mapreg_setreg(add_str( "$@warpwaitingpcnum"),n);
	return 0;
}
/*==========================================
 * RID�̃A�^�b�`
 *------------------------------------------
 */
int buildin_attachrid(CScriptEngine &st)
{
	st.rid=st.GetInt(st[2]);
	st.push_val(CScriptEngine::C_INT, (map_id2sd(st.rid)!=NULL));
	return 0;
}
/*==========================================
 * RID�̃f�^�b�`
 *------------------------------------------
 */
int buildin_detachrid(CScriptEngine &st)
{
	st.rid=0;
	return 0;
}
/*==========================================
 * ���݃`�F�b�N
 *------------------------------------------
 */
int buildin_isloggedin(CScriptEngine &st)
{
	st.push_val(CScriptEngine::C_INT, map_id2sd(
		st.GetInt( (st[2])) )!=NULL );
	return 0;
}


/*==========================================
 *
 *------------------------------------------
 */
enum 
{
	MF_NOMEMO,			// 0
	MF_NOTELEPORT,		// 1
	MF_NOSAVE,			// 2
	MF_NOBRANCH,		// 3
	MF_NOPENALTY,		// 4
	MF_NOZENYPENALTY,	// 5
	MF_PVP,				// 6
	MF_PVP_NOPARTY,		// 7
	MF_PVP_NOGUILD,		// 8
	MF_GVG,				// 9
	MF_GVG_NOPARTY,		//10
	MF_NOTRADE,			//11
	MF_NOSKILL,			//12
	MF_NOWARP,			//13
	MF_NOPVP,			//14
	MF_NOICEWALL,		//15
	MF_SNOW,			//16
	MF_FOG,				//17
	MF_SAKURA,			//18
	MF_LEAVES,			//19
	MF_RAIN,			//20
	MF_INDOORS,			//21
	MF_NOGO,			//22
	MF_CLOUDS,			//23
	MF_FIREWORKS, 		//24
	MF_GVG_DUNGEON		//25
};



int buildin_setmapflagnosave(CScriptEngine &st)
{
	int m,x,y;
	const char *str,*str2;

	str=st.GetString( (st[2]));
	str2=st.GetString( (st[3]));
	x=st.GetInt( (st[4]));
	y=st.GetInt( (st[5]));
	m = map_mapname2mapid(str);
//!! broadcast command if not on this mapserver
	if(m >= 0) {
		map[m].flag.nosave=1;
		memcpy(map[m].save.map,str2,16);
		map[m].save.x=x;
		map[m].save.y=y;
	}

	return 0;
}

int buildin_setmapflag(CScriptEngine &st)
{
	int m,i;
	const char *str;

	str=st.GetString( (st[2]));
	i=st.GetInt( (st[3]));
	m = map_mapname2mapid(str);
//!! broadcast command if not on this mapserver
	if(m >= 0) {
		switch(i) {
			case MF_NOMEMO:
				map[m].flag.nomemo=1;
				break;
			case MF_NOTELEPORT:
				map[m].flag.noteleport=1;
				break;
			case MF_NOBRANCH:
				map[m].flag.nobranch=1;
				break;
			case MF_NOPENALTY:
				map[m].flag.nopenalty=1;
				break;
			case MF_NOZENYPENALTY:
				map[m].flag.nozenypenalty=1;
				break;
			case MF_PVP:
				map[m].flag.pvp=1;
				break;
			case MF_PVP_NOPARTY:
				map[m].flag.pvp_noparty=1;
				break;
			case MF_PVP_NOGUILD:
				map[m].flag.pvp_noguild=1;
				break;
			case MF_GVG:
				map[m].flag.gvg=1;
				break;
			case MF_GVG_NOPARTY:
				map[m].flag.gvg_noparty=1;
				break;
			case MF_GVG_DUNGEON:
				map[m].flag.gvg_dungeon=1;
				break;
			case MF_NOTRADE:
				map[m].flag.notrade=1;
				break;
			case MF_NOSKILL:
				map[m].flag.noskill=1;
				break;
			case MF_NOWARP:
				map[m].flag.nowarp=1;
				break;
			case MF_NOPVP:
				map[m].flag.nopvp=1;
				break;
			case MF_NOICEWALL: // [Valaris]
				map[m].flag.noicewall=1;
				break;
			case MF_SNOW: // [Valaris]
				map[m].flag.snow=1;
				break;
			case MF_CLOUDS:
				map[m].flag.clouds=1;
				break;
			case MF_FOG: // [Valaris]
				map[m].flag.fog=1;
				break;
			case MF_FIREWORKS:
				map[m].flag.fireworks=1;
				break;
			case MF_SAKURA: // [Valaris]
				map[m].flag.sakura=1;
				break;
			case MF_LEAVES: // [Valaris]
				map[m].flag.leaves=1;
				break;
			case MF_RAIN: // [Valaris]
				map[m].flag.rain=1;
				break;
			case MF_INDOORS: // celest
				map[m].flag.indoors=1;
				break;
			case MF_NOGO: // celest
				map[m].flag.nogo=1;
				break;
		}
	}
	return 0;
}

int buildin_removemapflag(CScriptEngine &st)
{
	int m,i;
	const char *str;

	str=st.GetString( (st[2]));
	i=st.GetInt( (st[3]));
	m = map_mapname2mapid(str);
//!! broadcast command if not on this mapserver
	if(m >= 0) {
		switch(i) {
			case MF_NOMEMO:
				map[m].flag.nomemo=0;
				break;
			case MF_NOTELEPORT:
				map[m].flag.noteleport=0;
				break;
			case MF_NOSAVE:
				map[m].flag.nosave=0;
				break;
			case MF_NOBRANCH:
				map[m].flag.nobranch=0;
				break;
			case MF_NOPENALTY:
				map[m].flag.nopenalty=0;
				break;
			case MF_PVP:
				map[m].flag.pvp=0;
				break;
			case MF_PVP_NOPARTY:
				map[m].flag.pvp_noparty=0;
				break;
			case MF_PVP_NOGUILD:
				map[m].flag.pvp_noguild=0;
				break;
			case MF_GVG:
				map[m].flag.gvg=0;
				break;
			case MF_GVG_NOPARTY:
				map[m].flag.gvg_noparty=0;
				break;
			case MF_GVG_DUNGEON:
				map[m].flag.gvg_dungeon=0;
				break;
			case MF_NOZENYPENALTY:
				map[m].flag.nozenypenalty=0;
				break;
			case MF_NOSKILL:
				map[m].flag.noskill=0;
				break;
			case MF_NOWARP:
				map[m].flag.nowarp=0;
				break;
			case MF_NOPVP:
				map[m].flag.nopvp=0;
				break;
			case MF_NOICEWALL: // [Valaris]
				map[m].flag.noicewall=0;
				break;
			case MF_SNOW: // [Valaris]
				map[m].flag.snow=0;
				break;
			case MF_CLOUDS:
				map[m].flag.clouds=0;
				break;
			case MF_FOG: // [Valaris]
				map[m].flag.fog=0;
				break;
			case MF_FIREWORKS:
				map[m].flag.fireworks=0;
				break;
			case MF_SAKURA: // [Valaris]
				map[m].flag.sakura=0;
				break;
			case MF_LEAVES: // [Valaris]
				map[m].flag.leaves=0;
				break;
			case MF_RAIN: // [Valaris]
				map[m].flag.rain=0;
				break;
			case MF_INDOORS: // celest
				map[m].flag.indoors=0;
				break;
			case MF_NOGO: // celest
				map[m].flag.nogo=0;
				break;
		}
	}

	return 0;
}

int buildin_pvpon(CScriptEngine &st)
{
	size_t i;
	short m;
	const char *str;
	struct map_session_data *pl_sd=NULL;

	str=st.GetString( (st[2]));
	m = map_mapname2mapid(str);
//!! broadcast command if not on this mapserver
	if(m >= 0 && !map[m].flag.pvp && !map[m].flag.nopvp) {
		map[m].flag.pvp = 1;
		clif_send0199(m,1);

		if(battle_config.pk_mode) // disable ranking functions if pk_mode is on [Valaris]
			return 0;

		for(i=0;i<fd_max;i++){	//�l�������[�v
			if(session[i] && (pl_sd=(struct map_session_data *) session[i]->session_data) && pl_sd->state.auth){
				if(m == pl_sd->bl.m && pl_sd->pvp_timer == -1) {
					pl_sd->pvp_timer=add_timer(gettick()+200,pc_calc_pvprank_timer,pl_sd->bl.id,0);
					pl_sd->pvp_rank=0;
					pl_sd->pvp_lastusers=0;
					pl_sd->pvp_point=5;
					pl_sd->pvp_won = 0;
					pl_sd->pvp_lost = 0;
				}
			}
		}
	}
	return 0;
}

int buildin_pvpoff(CScriptEngine &st)
{
	size_t i;
	short m;
	const char *str;
	struct map_session_data *pl_sd=NULL;

	str=st.GetString( (st[2]));
	m = map_mapname2mapid(str);
//!! broadcast command if not on this mapserver
	if(m >= 0 && map[m].flag.pvp && map[m].flag.nopvp) {
		map[m].flag.pvp = 0;
		clif_send0199(m,0);

		if(battle_config.pk_mode) // disable ranking options if pk_mode is on [Valaris]
			return 0;

		for(i=0;i<fd_max;i++){	//�l�������[�v
			if(session[i] && (pl_sd=(struct map_session_data *) session[i]->session_data) && pl_sd->state.auth){
				if(m == pl_sd->bl.m) {
					clif_pvpset(*pl_sd,0,0,2);
					if(pl_sd->pvp_timer != -1) {
						delete_timer(pl_sd->pvp_timer,pc_calc_pvprank_timer);
						pl_sd->pvp_timer = -1;
					}
				}
			}
		}
	}
	return 0;
}

int buildin_gvgon(CScriptEngine &st)
{
	int m;
	const char *str;

	str=st.GetString( (st[2]));
	m = map_mapname2mapid(str);
//!! broadcast command if not on this mapserver
	if(m >= 0 && !map[m].flag.gvg) {
		map[m].flag.gvg = 1;
		clif_send0199(m,3);
	}
	return 0;
}

int buildin_gvgoff(CScriptEngine &st)
{
	int m;
	const char *str;

	str=st.GetString( (st[2]));
	m = map_mapname2mapid(str);
//!! broadcast command if not on this mapserver
	if(m >= 0 && map[m].flag.gvg) {
		map[m].flag.gvg = 0;
		clif_send0199(m,0);
	}
	return 0;
}

/*==========================================
 *	NPC�G���[�V����
 *------------------------------------------
 */
int buildin_emotion(CScriptEngine &st)
{
	int type;
	type=st.GetInt( (st[2]));
	if(type < 0 || type > 100)
		return 0;
	block_list *bl = map_id2bl(st.oid);
	if(bl) clif_emotion(*bl,type);
	return 0;
}

int buildin_maprespawnguildid_sub(struct block_list &bl,va_list ap)
{
	unsigned long g_id=va_arg(ap,unsigned long);
	int flag=va_arg(ap,int);
	struct map_session_data *sd=NULL;
	struct mob_data *md=NULL;

	if(bl.type == BL_PC)
		sd=(struct map_session_data*)&bl;
	else if(bl.type == BL_MOB)
		md=(struct mob_data *)&bl;

	if(sd){
		if( (sd->status.guild_id == 0) ||
			((sd->status.guild_id == g_id) && (flag&1)) ||
			((sd->status.guild_id != g_id) && (flag&2)) )
		{	// move players out that not belong here
			pc_setpos(*sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
		}
	}
	else if(md && flag&4){
		if(md->class_ < 1285 || md->class_ > 1288)
			mob_remove_map(*md, 1);
	}
	return 0;
}

int buildin_maprespawnguildid(CScriptEngine &st)
{
	const char *mapname=st.GetString( (st[2]));
	int g_id=st.GetInt( (st[3]));
	int flag=st.GetInt( (st[4]));

	int m=map_mapname2mapid(mapname);

	if(m) map_foreachinarea(buildin_maprespawnguildid_sub,m,0,0,map[m].xs-1,map[m].ys-1,BL_NUL,g_id,flag);
	return 0;
}

int buildin_agitstart(CScriptEngine &st)
{
	if(agit_flag==1) return 1;      // Agit already Start.
	agit_flag=1;
	guild_agit_start();
	return 0;
}

int buildin_agitend(CScriptEngine &st)
{
	if(agit_flag==0) return 1;      // Agit already End.
	agit_flag=0;
	guild_agit_end();
	return 0;
}
/*==========================================
 * agitcheck 1;    // choice script
 * if(@agit_flag == 1) goto agit;
 * if(agitcheck(0) == 1) goto agit;
 *------------------------------------------
 */
int buildin_agitcheck(CScriptEngine &st)
{
	int cond=st.GetInt( (st[2]));
	if(cond == 0) {
		if (agit_flag==1) st.push_val(CScriptEngine::C_INT,1);
		if (agit_flag==0) st.push_val(CScriptEngine::C_INT,0);
	} else {
		struct map_session_data *sd=script_rid2sd(st);
		if (agit_flag==1) pc_setreg(*sd,add_str( "@agit_flag"),1);
		if (agit_flag==0) pc_setreg(*sd,add_str( "@agit_flag"),0);
	}
	return 0;
}

int buildin_flagemblem(CScriptEngine &st)
{
	int g_id=st.GetInt( (st[2]));

	if(g_id < 0) return 0;

//	ShowMessage("Script.c: [FlagEmblem] GuildID=%d, Emblem=%d.\n", g->guild_id, g->emblem_id);
	((struct npc_data *)map_id2bl(st.oid))->u.scr.guild_id = g_id;
	return 1;
}

int buildin_getcastlename(CScriptEngine &st)
{
	const char *mapname=st.GetString( (st[2]));
	struct guild_castle *gc;
	int i;
	char *buf=NULL;

	for(i=0;i<MAX_GUILDCASTLE;i++)
	{
		if( (gc=guild_castle_search(i)) != NULL )
		{
			if( strcasecmp(mapname,gc->map_name)==0 )
			{
				buf=(char *)aMalloc(24*sizeof(char));
				memcpy(buf,gc->castle_name,24);//EOS included
				break;
			}
		}
	}
	if(buf)
		st.push_str(CScriptEngine::C_STR, buf);
	else
		st.push_str(CScriptEngine::C_CONSTSTR, "");
	return 0;
}

int buildin_getcastledata(CScriptEngine &st)
{
	const char *mapname=st.GetString( (st[2]));
	int index=st.GetInt( (st[3]));
	struct guild_castle *gc;
	int i, val=0;

	for(i=0;i<MAX_GUILDCASTLE;i++)
	{
		if( (gc=guild_castle_search(i)) != NULL && 0==strcmp(mapname,gc->map_name) )
		{
			switch(index)
			{
			case  0:
				if(st.end>st.start+4)
				{
					const char *event = st.GetString( (st[4]));
					guild_addcastleinfoevent(i,17,event);
				}
				for(i=1;i<26;i++) guild_castledataload(gc->castle_id,i);
				val = 0;
				break;  // Initialize[AgitInit]
			case  1: val = gc->guild_id; break;
			case  2: val = gc->economy; break;
			case  3: val = gc->defense; break;
			case  4: val = gc->triggerE; break;
			case  5: val = gc->triggerD; break;
			case  6: val = gc->nextTime; break;
			case  7: val = gc->payTime; break;
			case  8: val = gc->createTime; break;
			case  9: val = gc->visibleC; break;
			case 10: val = gc->visibleG0; break;
			case 11: val = gc->visibleG1; break;
			case 12: val = gc->visibleG2; break;
			case 13: val = gc->visibleG3; break;
			case 14: val = gc->visibleG4; break;
			case 15: val = gc->visibleG5; break;
			case 16: val = gc->visibleG6; break;
			case 17: val = gc->visibleG7; break;
			case 18: val = gc->Ghp0; break;
			case 19: val = gc->Ghp1; break;
			case 20: val = gc->Ghp2; break;
			case 21: val = gc->Ghp3; break;
			case 22: val = gc->Ghp4; break;
			case 23: val = gc->Ghp5; break;
			case 24: val = gc->Ghp6; break;
			case 25: val = gc->Ghp7; break;
			}// end switch
		break; // the for loop
		}
	}
	st.push_val(CScriptEngine::C_INT,val);
	return 0;
}

int buildin_setcastledata(CScriptEngine &st)
{
	const char *mapname=st.GetString( (st[2]));
	int index=st.GetInt( (st[3]));
	int value=st.GetInt( (st[4]));
	struct guild_castle *gc;
	int i;

	for(i=0;i<MAX_GUILDCASTLE;i++){
		if( (gc=guild_castle_search(i)) != NULL ){
			if(strcmp(mapname,gc->map_name)==0){
				// Save Data byself First
				switch(index){
				case 1: gc->guild_id = value; break;
				case 2: gc->economy = value; break;
				case 3: gc->defense = value; break;
				case 4: gc->triggerE = value; break;
				case 5: gc->triggerD = value; break;
				case 6: gc->nextTime = value; break;
				case 7: gc->payTime = value; break;
				case 8: gc->createTime = value; break;
				case 9: gc->visibleC = value; break;
				case 10: gc->visibleG0 = value; break;
				case 11: gc->visibleG1 = value; break;
				case 12: gc->visibleG2 = value; break;
				case 13: gc->visibleG3 = value; break;
				case 14: gc->visibleG4 = value; break;
				case 15: gc->visibleG5 = value; break;
				case 16: gc->visibleG6 = value; break;
				case 17: gc->visibleG7 = value; break;
				case 18: gc->Ghp0 = value; break;
				case 19: gc->Ghp1 = value; break;
				case 20: gc->Ghp2 = value; break;
				case 21: gc->Ghp3 = value; break;
				case 22: gc->Ghp4 = value; break;
				case 23: gc->Ghp5 = value; break;
				case 24: gc->Ghp6 = value; break;
				case 25: gc->Ghp7 = value; break;
				default: return 0;
				}
				guild_castledatasave(gc->castle_id,index,value);
				return 0;
			}
		}
	}
	return 0;
}

/* =====================================================================
 * �M���h����v������
 * ---------------------------------------------------------------------
 */
int buildin_requestguildinfo(CScriptEngine &st)
{
	int guild_id=st.GetInt( (st[2]));
	const char *event=NULL;

	if( st.end>st.start+3 )
		event=st.GetString( (st[3]));

	if(guild_id>0)
		guild_npc_request_info(guild_id,event);
	return 0;
}

/* =====================================================================
 * �J�[�h�̐��𓾂�
 * ---------------------------------------------------------------------
 */
int buildin_getequipcardcnt(CScriptEngine &st)
{
	int i,num;
	struct map_session_data *sd;
	int c=4;

	num=st.GetInt( (st[2]));
	sd=script_rid2sd(st);
	if(!sd)
	{
		st.push_val(CScriptEngine::C_INT,0);
		return 0;
	}
	i=pc_checkequip(*sd,equip[num-1]);
	if(sd->status.inventory[i].card[0] == 0x00ff){ // ��������̓J�[�h�Ȃ�
		st.push_val(CScriptEngine::C_INT,0);
		return 0;
	}
	do{
		if( (sd->status.inventory[i].card[c-1] > 4000 &&
			sd->status.inventory[i].card[c-1] < 5000) ||
			itemdb_type(sd->status.inventory[i].card[c-1]) == 6){	// [Celest]
			st.push_val(CScriptEngine::C_INT,(c));
			return 0;
		}
	}while(c--);
	st.push_val(CScriptEngine::C_INT,0);
	return 0;
}

/* ================================================================
 * �J�[�h���O������
 * ----------------------------------------------------------------
 */
int buildin_successremovecards(CScriptEngine &st)
{
	int i,num,cardflag=0,flag;
	struct map_session_data *sd;
	struct item item_tmp;
	int c=4;

	num=st.GetInt( (st[2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(*sd,equip[num-1]);
	if(sd->status.inventory[i].card[0]==0x00ff){ // ��������͏������Ȃ�
		return 0;
	}
	do{
		if( (sd->status.inventory[i].card[c-1] > 4000 &&
			sd->status.inventory[i].card[c-1] < 5000) ||
			itemdb_type(sd->status.inventory[i].card[c-1]) == 6){	// [Celest]

			cardflag = 1;
			item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].card[c-1];
			item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=0;
			item_tmp.attribute=0;
			item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;

			if((flag=pc_additem(*sd,item_tmp,1))){	// ���ĂȂ��Ȃ�h���b�v
				clif_additem(*sd,0,0,flag);
				map_addflooritem(item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
			}
		}
	}while(c--);

	if(cardflag == 1){	// �J�[�h����菜�����A�C�e������
		flag=0;
		item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].nameid;
		item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=sd->status.inventory[i].refine;
		item_tmp.attribute=sd->status.inventory[i].attribute;
		item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;
		pc_delitem(*sd,i,1,0);
		if((flag=pc_additem(*sd,item_tmp,1))){	// ���ĂȂ��Ȃ�h���b�v
			clif_additem(*sd,0,0,flag);
			map_addflooritem(item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
		clif_misceffect(sd->bl,3);
		return 0;
	}
	return 0;
}

/* ================================================================
 * �J�[�h���O�����s slot,type
 * type=0: ���������A1:�J�[�h�����A2:������A3:��������
 * ----------------------------------------------------------------
 */
int buildin_failedremovecards(CScriptEngine &st)
{
	int i,num,cardflag=0,flag,typefail;
	struct map_session_data *sd;
	struct item item_tmp;
	int c=4;

	num=st.GetInt( (st[2]));
	typefail=st.GetInt( (st[3]));
	sd=script_rid2sd(st);
	i=pc_checkequip(*sd,equip[num-1]);
	if(sd->status.inventory[i].card[0]==0x00ff){ // ��������͏������Ȃ�
		return 0;
	}
	do{
		if( (sd->status.inventory[i].card[c-1] > 4000 &&
			sd->status.inventory[i].card[c-1] < 5000) ||
			itemdb_type(sd->status.inventory[i].card[c-1]) == 6){	// [Celest]

			cardflag = 1;

			if(typefail == 2){ // ����̂ݑ����Ȃ�A�J�[�h�͎󂯎�点��
				item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].card[c-1];
				item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=0;
				item_tmp.attribute=0;
				item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;
				if((flag=pc_additem(*sd,item_tmp,1))){
					clif_additem(*sd,0,0,flag);
					map_addflooritem(item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
				}
			}
		}
	}while(c--);

	if(cardflag == 1){

		if(typefail == 0 || typefail == 2){	// �����
			pc_delitem(*sd,i,1,0);
			clif_misceffect(sd->bl,2);
			return 0;
		}
		if(typefail == 1){	// �J�[�h�̂ݑ����i�����Ԃ��j
			flag=0;
			item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].nameid;
			item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=sd->status.inventory[i].refine;
			item_tmp.attribute=sd->status.inventory[i].attribute;
			item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;
			pc_delitem(*sd,i,1,0);
			if((flag=pc_additem(*sd,item_tmp,1))){
				clif_additem(*sd,0,0,flag);
				map_addflooritem(item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
			}
		}
		clif_misceffect(sd->bl,2);
		return 0;
	}
	return 0;
}

int buildin_mapwarp(CScriptEngine &st)	// Added by RoVeRT
{
	int x,y,m;
	const char *str;
	const char *mapname;
	int x0,y0,x1,y1;

	mapname=st.GetString( (st[2]));
	x0=0;
	y0=0;
	x1=map[map_mapname2mapid(mapname)].xs;
	y1=map[map_mapname2mapid(mapname)].ys;
	str=st.GetString( (st[3]));
	x=st.GetInt( (st[4]));
	y=st.GetInt( (st[5]));

	if( (m=map_mapname2mapid(mapname))< 0)
		return 0;
//!! broadcast command if not on this mapserver
	map_foreachinarea(buildin_areawarp_sub,
		m,x0,y0,x1,y1,BL_PC, str,x,y );
	return 0;
}

int buildin_cmdothernpc(CScriptEngine &st)	// Added by RoVeRT
{
	const char *npc,*command;

	npc=st.GetString( (st[2]));
	command=st.GetString( (st[3]));

	map_session_data *sd = map_id2sd(st.rid);
	if(sd) npc_command(*sd,npc,command);
	return 0;
}

int buildin_inittimer(CScriptEngine &st)	// Added by RoVeRT
{
//	struct npc_data *nd=(struct npc_data*)map_id2bl(st.oid);
//	nd->lastaction=nd->timer=gettick();

	map_session_data *sd = map_id2sd(st.rid);
	if(sd) npc_do_ontimer(st.oid, *sd, 1);
	return 0;
}

int buildin_stoptimer(CScriptEngine &st)	// Added by RoVeRT
{
//	struct npc_data *nd=(struct npc_data*)map_id2bl(st.oid);
//	nd->lastaction=nd->timer=-1;

	map_session_data *sd = map_id2sd(st.rid);
	if(sd) npc_do_ontimer(st.oid, *sd, 0);

	return 0;
}

int buildin_mobcount_sub(struct block_list &bl,va_list ap)	// Added by RoVeRT
{
	char *event=va_arg(ap,char *);
	int *c=va_arg(ap,int *);

	if(strcmp(event,((struct mob_data *)&bl)->npc_event)==0)
		(*c)++;
	return 0;
}

int buildin_mobcount(CScriptEngine &st)	// Added by RoVeRT
{
	const char *mapname,*event;
	int m,c=0;
	mapname=st.GetString( (st[2]));
	event=st.GetString( (st[3]));

	if( (m=map_mapname2mapid(mapname))<0 ) {
		st.push_val(CScriptEngine::C_INT,-1);
		return 0;
	}
	map_foreachinarea(buildin_mobcount_sub,
		m,0,0,map[m].xs-1,map[m].ys-1,BL_MOB, event,&c );

	st.push_val(CScriptEngine::C_INT, (c));

	return 0;
}
int buildin_marriage(CScriptEngine &st)
{
	const char *partner=st.GetString( (st[2]));
	struct map_session_data *sd=script_rid2sd(st);
	struct map_session_data *p_sd=map_nick2sd(partner);

	st.push_val(CScriptEngine::C_INT, (sd!=NULL && p_sd!=NULL && pc_marriage(*sd,*p_sd)) );

	return 0;
}

int buildin_wedding_effect(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	struct block_list *bl;

	if(sd==NULL) {
		bl=map_id2bl(st.oid);
	} else
		bl=&sd->bl;
	if(bl) clif_wedding_effect(*bl);
	return 0;
}
int buildin_divorce(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	st.push_val(CScriptEngine::C_INT, (sd && pc_divorce(*sd)) );
	return 0;
}

int buildin_ispartneron(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	st.push_val(CScriptEngine::C_INT,
		(sd && pc_ismarried(*sd) &&
		NULL!=map_nick2sd(map_charid2nick(sd->status.partner_id)))
		);
	return 0;
}

int buildin_getpartnerid(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if (sd == NULL) {
		st.push_val(CScriptEngine::C_INT,0);
		return 0;
	}
	st.push_val(CScriptEngine::C_INT,sd->status.partner_id);
	return 0;
}

int buildin_getchildid(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if (sd == NULL) {
		st.push_val(CScriptEngine::C_INT,0);
		return 0;
	}
	st.push_val(CScriptEngine::C_INT,sd->status.child_id);
	return 0;
}

int buildin_warppartner(CScriptEngine &st)
{
	int x,y;
	const char *str;
	struct map_session_data *sd=script_rid2sd(st);
	struct map_session_data *p_sd=NULL;

	if( sd==NULL || !pc_ismarried(*sd) ||
		((p_sd=map_nick2sd(map_charid2nick(sd->status.partner_id))) == NULL))
	{
		st.push_val(CScriptEngine::C_INT,0);
		return 0;
	}

	str=st.GetString( (st[2]));
	x=st.GetInt( (st[3]));
	y=st.GetInt( (st[4]));

	pc_setpos(*p_sd,str,x,y,0);

	st.push_val(CScriptEngine::C_INT,1);
	return 0;
}

/*================================================
 * Script for Displaying MOB Information [Valaris]
 *------------------------------------------------
 */
int buildin_strmobinfo(CScriptEngine &st)
{

	int num=st.GetInt( (st[2]));
	int class_=st.GetInt( (st[3]));

	if((class_>=0 && class_<=1000) || class_ >2000)
		return 0;

	switch (num) {
	case 1:
	{
		char *buf;
		buf = (char*)aMalloc(24*sizeof(char));
		strcpy(buf,mob_db[class_].name);
		st.push_str(CScriptEngine::C_STR, buf);
		break;
	}
	case 2:
	{
		char *buf;
		buf=(char*)aMalloc(24*sizeof(char));
		strcpy(buf,mob_db[class_].jname);
		st.push_str(CScriptEngine::C_STR, buf);
		break;
	}
	case 3:
		st.push_val(CScriptEngine::C_INT,mob_db[class_].lv);
		break;
	case 4:
		st.push_val(CScriptEngine::C_INT,mob_db[class_].max_hp);
		break;
	case 5:
		st.push_val(CScriptEngine::C_INT,mob_db[class_].max_sp);
		break;
	case 6:
		st.push_val(CScriptEngine::C_INT,mob_db[class_].base_exp);
		break;
	case 7:
		st.push_val(CScriptEngine::C_INT,mob_db[class_].job_exp);
		break;
	}
	return 0;
}

/*==========================================
 * Summon guardians [Valaris]
 *------------------------------------------
 */
int buildin_guardian(CScriptEngine &st)
{
	int class_=0,amount=1,x=0,y=0,guardian=0;
	const char *str,*map,*event="";

	map	=st.GetString( (st[2]));
	x	=st.GetInt( (st[3]));
	y	=st.GetInt( (st[4]));
	str	=st.GetString( (st[5]));
	class_=st.GetInt( (st[6]));
	amount=st.GetInt( (st[7]));
	event=st.GetString( (st[8]));
	if( st.end>st.start+9 )
		guardian=st.GetInt( (st[9]));

	mob_spawn_guardian(map_id2sd(st.rid),map,x,y,str,class_,amount,event,guardian);

	return 0;
}

/*================================================
 * Script for Displaying Guardian Info [Valaris]
 *------------------------------------------------
 */
int buildin_guardianinfo(CScriptEngine &st)
{
	int guardian=st.GetInt( (st[2]));
	struct map_session_data *sd=script_rid2sd(st);
	struct guild_castle *gc=guild_mapname2gc(map[sd->bl.m].mapname);

	if(guardian==0 && gc->visibleG0 == 1) st.push_val(CScriptEngine::C_INT,gc->Ghp0);
	if(guardian==1 && gc->visibleG1 == 1) st.push_val(CScriptEngine::C_INT,gc->Ghp1);
	if(guardian==2 && gc->visibleG2 == 1) st.push_val(CScriptEngine::C_INT,gc->Ghp2);
	if(guardian==3 && gc->visibleG3 == 1) st.push_val(CScriptEngine::C_INT,gc->Ghp3);
	if(guardian==4 && gc->visibleG4 == 1) st.push_val(CScriptEngine::C_INT,gc->Ghp4);
	if(guardian==5 && gc->visibleG5 == 1) st.push_val(CScriptEngine::C_INT,gc->Ghp5);
	if(guardian==6 && gc->visibleG6 == 1) st.push_val(CScriptEngine::C_INT,gc->Ghp6);
	if(guardian==7 && gc->visibleG7 == 1) st.push_val(CScriptEngine::C_INT,gc->Ghp7);
	else st.push_val(CScriptEngine::C_INT,-1);

	return 0;
}
/*==========================================
 * ID����Item��
 *------------------------------------------
 */
int buildin_getitemname(CScriptEngine &st)
{
	struct item_data *item_data=NULL;
	CScriptEngine::CValue &data = st[2];

	st.ConvertName( data);
	if( data.isString() )
	{
		const char *name=st.GetString(data);
		item_data = itemdb_searchname(name);
	}
	else
	{
		int item_id=st.GetInt(data);
		item_data = itemdb_exists(item_id);
	}
	if(item_data)
	{
		char *item_name;
		item_name=(char *)aMalloc(24*sizeof(char));
		memcpy(item_name,item_data->jname,24);//EOS included
		st.push_str(CScriptEngine::C_STR, item_name);
	}
	else
		st.push_str(CScriptEngine::C_CONSTSTR, "unknown");
	return 0;
}

/*==========================================
 * petskillbonus [Valaris] //Rewritten by [Skotlex]
 *------------------------------------------
 */

int buildin_petskillbonus(CScriptEngine &st)
{
	struct pet_data *pd;

	struct map_session_data *sd=script_rid2sd(st);

	if(sd==NULL || sd->pd==NULL)
		return 0;

	pd=sd->pd;
	if (pd->bonus)
	{ //Clear previous bonus
		if (pd->bonus->timer != -1)
			delete_timer(pd->bonus->timer, pet_skill_bonus_timer);
	} else //init
		pd->bonus = (struct pet_data::pet_bonus *) aCalloc(1, sizeof(struct pet_data::pet_bonus));

	pd->bonus->type=st.GetInt( (st[2]));
	pd->bonus->val=st.GetInt( (st[3]));
	pd->bonus->duration=st.GetInt( (st[4]));
	pd->bonus->delay=st.GetInt( (st[5]));

	if (pd->state.skillbonus == -1)
		pd->state.skillbonus=0;	// waiting state

	// wait for timer to start
	if (battle_config.pet_equip_required && pd->equip_id == 0)
		pd->bonus->timer=-1;
	else
		pd->bonus->timer=add_timer(gettick()+pd->bonus->delay*1000, pet_skill_bonus_timer, sd->bl.id, 0);

	return 0;
}

/*==========================================
 * pet looting [Valaris] //Rewritten by [Skotlex]
 *------------------------------------------
 */
int buildin_petloot(CScriptEngine &st)
{
	int max;
	struct pet_data *pd;
	struct map_session_data *sd=script_rid2sd(st);
	
	if(sd==NULL || sd->pd==NULL)
		return 0;

	max=st.GetInt( (st[2]));

	if(max < 1)
		max = 1;	//Let'em loot at least 1 item.
	else if (max > MAX_PETLOOT_SIZE)
		max = MAX_PETLOOT_SIZE;
	
	pd = sd->pd;
	if(pd && pd->loot != NULL && pd->msd)
	{	//Release whatever was there already and reallocate memory
		pet_lootitem_drop(*pd, pd->msd);
		aFree(pd->loot->item);
	}
	else
		pd->loot = (struct pet_data::pet_loot *)aCalloc(1, sizeof(struct pet_data::pet_loot));

	pd->loot->item = (struct item *)aCalloc(max,sizeof(struct item));
	pd->loot->max=max;
	pd->loot->count = 0;
	pd->loot->weight = 0;
	pd->loot->loottick = gettick();

	return 0;
}
/*==========================================
 * PC�̏����i���ǂݎ��
 *------------------------------------------
 */
int buildin_getinventorylist(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int i,j=0;
	if(!sd) return 0;
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].amount > 0){
			pc_setreg(*sd,add_str( "@inventorylist_id")+(j<<24),sd->status.inventory[i].nameid);
			pc_setreg(*sd,add_str( "@inventorylist_amount")+(j<<24),sd->status.inventory[i].amount);
			pc_setreg(*sd,add_str( "@inventorylist_equip")+(j<<24),sd->status.inventory[i].equip);
			pc_setreg(*sd,add_str( "@inventorylist_refine")+(j<<24),sd->status.inventory[i].refine);
			pc_setreg(*sd,add_str( "@inventorylist_identify")+(j<<24),sd->status.inventory[i].identify);
			pc_setreg(*sd,add_str( "@inventorylist_attribute")+(j<<24),sd->status.inventory[i].attribute);
			pc_setreg(*sd,add_str( "@inventorylist_card1")+(j<<24),sd->status.inventory[i].card[0]);
			pc_setreg(*sd,add_str( "@inventorylist_card2")+(j<<24),sd->status.inventory[i].card[1]);
			pc_setreg(*sd,add_str( "@inventorylist_card3")+(j<<24),sd->status.inventory[i].card[2]);
			pc_setreg(*sd,add_str( "@inventorylist_card4")+(j<<24),sd->status.inventory[i].card[3]);
			j++;
		}
	}
	pc_setreg(*sd,add_str( "@inventorylist_count"),j);
	return 0;
}

int buildin_getskilllist(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int i,j=0;
	if(!sd) return 0;
	for(i=0;i<MAX_SKILL;i++){
		if(sd->status.skill[i].id > 0 && sd->status.skill[i].lv > 0){
			pc_setreg(*sd,add_str("@skilllist_id")+(j<<24),sd->status.skill[i].id);
			pc_setreg(*sd,add_str("@skilllist_lv")+(j<<24),sd->status.skill[i].lv);
			pc_setreg(*sd,add_str("@skilllist_flag")+(j<<24),sd->status.skill[i].flag);
			j++;
		}
	}
	pc_setreg(*sd,add_str( "@skilllist_count"),j);
	return 0;
}

int buildin_clearitem(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int i;
	if(sd==NULL) return 0;
	for (i=0; i<MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].amount)
			pc_delitem(*sd, i, sd->status.inventory[i].amount, 0);
	}
	return 0;
}

/*==========================================
 * NPC�N���X�`�F���W
 * class�͕ς�肽��class
 * type�͒ʏ�0�Ȃ̂��ȁH
 *------------------------------------------
 */
int buildin_classchange(CScriptEngine &st)
{
	int class_,type;
	struct block_list *bl=map_id2bl(st.oid);

	if(bl)
	{
		class_=st.GetInt( (st[2]));
		type=st.GetInt( (st[3]));
		clif_class_change(*bl,class_,type);
	}
	return 0;
}

/*==========================================
 * NPC���甭������G�t�F�N�g
 *------------------------------------------
 */
int buildin_misceffect(CScriptEngine &st)
{
	int type;
	type=st.GetInt( (st[2]));
	if(st.oid)
	{	block_list *bl = map_id2bl(st.oid);
		if(bl) clif_misceffect2(*bl,type);
	}
	else
	{
		struct map_session_data *sd=script_rid2sd(st);
		if(sd) clif_misceffect2(sd->bl,type);
//!! broadcast command if not on this mapserver
	}
	return 0;
}
/*==========================================
 * �T�E���h�G�t�F�N�g
 *------------------------------------------
 */
int buildin_soundeffect(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	const char *name;
	int type=0;
	name=st.GetString( (st[2]));
	type=st.GetInt( (st[3]));
	if(sd)
	{
		if(st.oid)
		{	block_list *bl = map_id2bl(st.oid);
			if(bl) clif_soundeffect(*sd,*bl,name,type);
		}
		else
		{
			clif_soundeffect(*sd,sd->bl,name,type);
		}
	}
	return 0;
}

int buildin_soundeffectall(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	block_list *bl;
	const char *name;
	int type=0;

	name=st.GetString( (st[2]));
	type=st.GetInt( (st[3]));
	
	if(st.oid && (bl=map_id2bl(st.oid))!=NULL)
		clif_soundeffectall(*bl,name,type);
	else if(sd)
		clif_soundeffectall(sd->bl,name,type);
	return 0;
}
/*==========================================
 * pet status recovery [Valaris] / Rewritten by [Skotlex]
 *------------------------------------------
 */
int buildin_petrecovery(CScriptEngine &st)
{
	struct pet_data *pd;
	struct map_session_data *sd=script_rid2sd(st);

	if(sd==NULL || sd->pd==NULL)
		return 0;

	pd=sd->pd;
	
	if (pd->recovery)
	{ //Halt previous bonus
		if (pd->recovery->timer != -1)
			delete_timer(pd->recovery->timer, pet_recovery_timer);
	}
	else //Init
		pd->recovery = (struct pet_data::pet_recovery *)aCalloc(1, sizeof(struct pet_data::pet_recovery));
		
	pd->recovery->type=st.GetInt( (st[2]));
	pd->recovery->delay=st.GetInt( (st[3]));
	pd->recovery->timer=-1;

	return 0;
}

/*==========================================
 * pet healing [Valaris] //Rewritten by [Skotlex]
 *------------------------------------------
 */
int buildin_petheal(CScriptEngine &st)
{
	struct pet_data *pd;
	struct map_session_data *sd=script_rid2sd(st);

	if(sd==NULL || sd->pd==NULL)
		return 0;

	pd=sd->pd;
	if (pd->s_skill)
	{	//Clear previous skill
		if (pd->s_skill->timer != -1)
			delete_timer(pd->s_skill->timer, pet_skill_support_timer);
	}
	else //init memory
		pd->s_skill = (struct pet_data::pet_skill_support *) aCalloc(1, sizeof(struct pet_data::pet_skill_support)); 
	
	//This id identifies that it IS petheal rather than pet_skillsupport
	pd->s_skill->id=0;
	//Use the lv as the amount to heal
	pd->s_skill->lv=st.GetInt( (st[2]));
	pd->s_skill->delay=st.GetInt( (st[3]));
	pd->s_skill->hp=st.GetInt( (st[4]));
	pd->s_skill->sp=st.GetInt( (st[5]));

	//Use delay as initial offset to avoid skill/heal exploits
	if (battle_config.pet_equip_required && pd->equip_id == 0)
		pd->s_skill->timer=-1;
	else
		pd->s_skill->timer=add_timer(gettick()+pd->s_skill->delay*1000,pet_heal_timer,sd->bl.id,0);

	return 0;
}

/*==========================================
 * pet attack skills [Valaris] //Rewritten by [Skotlex]
 *------------------------------------------
 */
int buildin_petskillattack(CScriptEngine &st)
{
	struct pet_data *pd;
	struct map_session_data *sd=script_rid2sd(st);

	if(sd==NULL || sd->pd==NULL)
		return 0;

	pd=sd->pd;
	if (pd->a_skill == NULL)
		pd->a_skill = (struct pet_data::pet_skill_attack *)aCalloc(1, sizeof(struct pet_data::pet_skill_attack));
				
	pd->a_skill->id=st.GetInt( (st[2]));
	pd->a_skill->lv=st.GetInt( (st[3]));
	pd->a_skill->div_ = 0;
	pd->a_skill->rate=st.GetInt( (st[4]));
	pd->a_skill->bonusrate=st.GetInt( (st[5]));

	return 0;
}

/*==========================================
 * pet attack skills [Valaris]
 *------------------------------------------
 */
int buildin_petskillattack2(CScriptEngine &st)
{
	struct pet_data *pd;
	struct map_session_data *sd=script_rid2sd(st);

	if(sd==NULL || sd->pd==NULL)
		return 0;

	pd=sd->pd;
	if (pd->a_skill == NULL)
		pd->a_skill = (struct pet_data::pet_skill_attack *)aCalloc(1, sizeof(struct pet_data::pet_skill_attack));
				
	pd->a_skill->id=st.GetInt( (st[2]));
	pd->a_skill->lv=st.GetInt( (st[3]));
	pd->a_skill->div_ = st.GetInt( (st[4]));
	pd->a_skill->rate=st.GetInt( (st[5]));
	pd->a_skill->bonusrate=st.GetInt( (st[6]));

	return 0;
}

/*==========================================
 * pet support skills [Skotlex]
 *------------------------------------------
 */
int buildin_petskillsupport(CScriptEngine &st)
{
	struct pet_data *pd;
	struct map_session_data *sd=script_rid2sd(st);

	if(sd==NULL || sd->pd==NULL)
		return 0;

	pd=sd->pd;
	if (pd->s_skill)
	{ //Clear previous skill
		if (pd->s_skill->timer != -1)
			delete_timer(pd->s_skill->timer, pet_skill_support_timer);
	} else //init memory
		pd->s_skill = (struct pet_data::pet_skill_support *) aCalloc(1, sizeof(struct pet_data::pet_skill_support)); 
	
	pd->s_skill->id=st.GetInt( (st[2]));
	pd->s_skill->lv=st.GetInt( (st[3]));
	pd->s_skill->delay=st.GetInt( (st[4]));
	pd->s_skill->hp=st.GetInt( (st[5]));
	pd->s_skill->sp=st.GetInt( (st[6]));

	//Use delay as initial offset to avoid skill/heal exploits
	if (battle_config.pet_equip_required && pd->equip_id == 0)
		pd->s_skill->timer=-1;
	else
		pd->s_skill->timer=add_timer(gettick()+pd->s_skill->delay*1000,pet_skill_support_timer,sd->bl.id,0);

	return 0;
}

/*==========================================
 * Scripted skill effects [Celest]
 *------------------------------------------
 */
int buildin_skilleffect(CScriptEngine &st)
{
	struct map_session_data *sd;

	int skillid=st.GetInt( (st[2]));
	int skilllv=st.GetInt( (st[3]));
	sd=script_rid2sd(st);

	clif_skill_nodamage(sd->bl,sd->bl,skillid,skilllv,1);

	return 0;
}

/*==========================================
 * NPC skill effects [Valaris]
 *------------------------------------------
 */
int buildin_npcskilleffect(CScriptEngine &st)
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st.oid);

	int skillid=st.GetInt( (st[2]));
	int skilllv=st.GetInt( (st[3]));
	int x=st.GetInt( (st[4]));
	int y=st.GetInt( (st[5]));

	clif_skill_poseffect(nd->bl,skillid,skilllv,x,y,gettick());

	return 0;
}

/*==========================================
 * Special effects [Valaris]
 *------------------------------------------
 */
int buildin_specialeffect(CScriptEngine &st)
{
	struct block_list *bl=map_id2bl(st.oid);

	if(bl) clif_specialeffect(*bl,st.GetInt( (st[2])), 0);

	return 0;
}

int buildin_specialeffect2(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);

	if(sd) clif_specialeffect(sd->bl,st.GetInt( (st[2])), 0);
		return 0;
}

/*==========================================
 * Nude [Valaris]
 *------------------------------------------
 */

int buildin_nude(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	if(sd)
	{
		size_t i;
		register bool calcflag=false;
		for(i=0;i<MAX_EQUIP;i++)
			if(sd->equip_index[i] < MAX_INVENTORY)
			{
				pc_unequipitem(*sd,sd->equip_index[i],2);
				calcflag=true;
			}
		if(calcflag)
			status_calc_pc(*sd,1);
	}
	return 0;
}

/*==========================================
 * gmcommand [MouseJstr]
 *
 * suggested on the forums...
 * splitted into atcommand & charcommand by [Skotlex]
 *------------------------------------------
 */
int buildin_gmcommand(CScriptEngine &st)
{
	struct map_session_data *sd;
	const char *cmd;

	sd = script_rid2sd(st);
	if (!sd)
		return 0;
	cmd = st.GetString( (st[2]));
	is_atcommand(sd->fd, *sd, cmd, 99);
	return 0;
}

int buildin_atcommand(CScriptEngine &st)
{
	struct map_session_data *sd;
	const char *cmd;

	sd = script_rid2sd(st);
	if (!sd)
		return 0;
	cmd = st.GetString( (st[2]));
	is_atcommand(sd->fd, *sd, cmd, 99);
	return 0;
}

int buildin_charcommand(CScriptEngine &st)
{
	struct map_session_data *sd;
	const char *cmd;

	sd = script_rid2sd(st);
	if (!sd)
		return 0;
	cmd = st.GetString( (st[2]));
	is_charcommand(sd->fd, *sd, cmd, 99);

	return 0;
}


/*==========================================
 * Displays a message for the player only (like system messages like "you got an apple" )
 *------------------------------------------
 */
int buildin_dispbottom(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	const char *message;
	message=st.GetString( (st[2]));
	if(sd)
		clif_disp_onlyself(*sd,message);
	return 0;
}

/*==========================================
 * All The Players Full Recovery
   (HP/SP full restore and resurrect if need)
 *------------------------------------------
 */
int buildin_recovery(CScriptEngine &st)
{
	size_t i;
	for (i = 0; i < fd_max; i++) {
		if (session[i]){
			struct map_session_data *sd = (struct map_session_data *) session[i]->session_data;
			if (sd && sd->state.auth) {
				sd->status.hp = sd->status.max_hp;
				sd->status.sp = sd->status.max_sp;
				clif_updatestatus(*sd, SP_HP);
				clif_updatestatus(*sd, SP_SP);
				if(pc_isdead(*sd)){
					pc_setstand(*sd);
					clif_resurrection(sd->bl, 1);
				}
				clif_displaymessage(sd->fd,"You have been recovered!");
			}
		}
	}
	return 0;
}
/*==========================================
 * Get your pet info: getpetinfo(n)  
 * n -> 0:pet_id 1:pet_class 2:pet_name
	3:friendly 4:hungry
 *------------------------------------------
 */
int buildin_getpetinfo(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int type=st.GetInt( (st[2]));

	if(sd && sd->status.pet_id){
		switch(type){
			case 0:
				st.push_val(CScriptEngine::C_INT,sd->status.pet_id);
				break;
			case 1:
				if(sd->pet.class_)
					st.push_val(CScriptEngine::C_INT,sd->pet.class_);
				else
					st.push_val(CScriptEngine::C_INT,0);
				break;
			case 2:
				if(sd->pet.name)
				{	
					char *buf=(char *)aMalloc(24*sizeof(char));
					memcpy(buf,sd->pet.name, 24);//EOS included
					st.push_str(CScriptEngine::C_STR, buf);
				}
				else
					st.push_str(CScriptEngine::C_CONSTSTR, "");
				break;
			case 3:
				//if(sd->pet.intimate)
				st.push_val(CScriptEngine::C_INT,sd->pet.intimate);
				break;
			case 4:
				//if(sd->pet.hungry)
				st.push_val(CScriptEngine::C_INT,sd->pet.hungry);
				break;
			default:
				st.push_val(CScriptEngine::C_INT,0);
				break;
		}
	}else{
		st.push_val(CScriptEngine::C_INT,0);
	}
	return 0;
}
/*==========================================
 * Shows wether your inventory(and equips) contain
   selected card or not.
	checkequipedcard(4001);
 *------------------------------------------
 */
int buildin_checkequipedcard(CScriptEngine &st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int n,i,c=0;
	c=st.GetInt( (st[2]));

	if(sd){
		for(i=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].amount){
				for(n=0;n<4;n++){
					if(sd->status.inventory[i].card[n]==c){
						st.push_val(CScriptEngine::C_INT,1);
						return 0;
					}
				}
			}
		}
	}
	st.push_val(CScriptEngine::C_INT,0);
	return 0;
}

int buildin_jump_zero(CScriptEngine &st) {
	int sel;
	sel=st.GetInt( (st[2]));
	if(!sel) {
		int pos;
		if( st[3].type!=CScriptEngine::C_POS ){
			ShowMessage("script: jump_zero: not label !\n");
			st.state=CScriptEngine::END;
			return 0;
		}

		pos=st.GetInt( (st[3]));
		st.pos=pos;
		st.state=CScriptEngine::GOTO;
		// ShowMessage("script: jump_zero: jumpto : %d\n",pos);
	} else {
		// ShowMessage("script: jump_zero: fail\n");
	}
	return 0;
}

int buildin_select(CScriptEngine &st)
{
	char *buf;
	size_t len,i;
	struct map_session_data *sd;

	sd=script_rid2sd(st);

	if(st.rerun_flag==0){
		st.state=CScriptEngine::RERUNLINE;
		st.rerun_flag=1;
		for(i=st.start+2,len=16;i<st.end;i++){
			st.GetString( (st.getDirectData(i)));
			len+=strlen(st.getDirectData(i).str)+1;
		}
		buf=(char *)aMalloc((len+1)*sizeof(char));
		buf[0]=0;
		for(i=st.start+2,len=0;i<st.end;i++){
			strcat(buf,st.getDirectData(i).str);
			strcat(buf,":");
		}
		map_session_data *sd = script_rid2sd(st);
		if(sd) clif_scriptmenu(*sd,st.oid,buf);
		aFree(buf);
	} else if(sd->npc_menu==0xff){	// cansel
		st.rerun_flag=0;
		st.state=CScriptEngine::END;
	} else {
		pc_setreg(*sd,add_str( "l15"),sd->npc_menu);
		pc_setreg(*sd,add_str( "@menu"),sd->npc_menu);
		st.rerun_flag=0;
		st.push_val(CScriptEngine::C_INT,sd->npc_menu);
	}
	return 0;
}

/*==========================================
 * GetMapMobs
	returns mob counts on a set map:
	e.g. GetMapMobs("prontera.gat")
	use "this" - for player's map
 *------------------------------------------
 */
int buildin_getmapmobs(CScriptEngine &st)
{
	const char *str;
	int m=-1,bx,by,i;
	int count=0,c;
	struct block_list *bl;

	str=st.GetString( (st[2]));

	if(strcmp(str,"this")==0){
		struct map_session_data *sd=script_rid2sd(st);
		if(sd)
			m=sd->bl.m;
		else{
			st.push_val(CScriptEngine::C_INT,-1);
			return 0;
		}
	}else
		m=map_mapname2mapid(str);

	if(m < 0){
		st.push_val(CScriptEngine::C_INT,-1);
		return 0;
	}

	for(by=0;by<=(map[m].ys-1)/BLOCK_SIZE;by++){
		for(bx=0;bx<=(map[m].xs-1)/BLOCK_SIZE;bx++){
			bl = map[m].block_mob[bx+by*map[m].bxs];
			c = map[m].block_mob_count[bx+by*map[m].bxs];
			for(i=0;i<c && bl;i++,bl=bl->next){
				if(bl->x<map[m].xs && bl->y<map[m].ys)
					count++;
			}
		}
	}
	st.push_val(CScriptEngine::C_INT,count);
	return 0;
}

/*==========================================
 * movenpc [MouseJstr]
 *------------------------------------------
 */

int buildin_movenpc(CScriptEngine &st)
{
	struct map_session_data *sd;
	const char *map,*npc;
	int x,y;

	sd = script_rid2sd(st);

	map = st.GetString( (st[2]));
	x = st.GetInt( (st[3]));
	y = st.GetInt( (st[4]));
	npc = st.GetString( (st[5]));

	return 0;
}

/*==========================================
 * message [MouseJstr]
 *------------------------------------------
 */

int buildin_message(CScriptEngine &st)
{
	struct map_session_data *sd;
	const char *msg,*player;
	struct map_session_data *pl_sd = NULL;

	sd = script_rid2sd(st);

	player = st.GetString( (st[2]));
	msg = st.GetString( (st[3]));

	if((pl_sd=map_nick2sd((char *) player)) == NULL)
             return 1;
	clif_displaymessage(pl_sd->fd, msg);

	return 0;
}

/*==========================================
 * npctalk (sends message to surrounding
 * area) [Valaris]
 *------------------------------------------
 */

int buildin_npctalk(CScriptEngine &st)
{
	const char *str;
	char message[1024];

	struct npc_data *nd=(struct npc_data *)map_id2bl(st.oid);
	str=st.GetString( (st[2]));

	if(nd) {
		snprintf(message, sizeof(message), "%s: %s", nd->name, str);
		message[sizeof(message)-1]=0;
		clif_message(nd->bl, message);
	}

	return 0;
}

/*==========================================
 * hasitems (checks to see if player has any
 * items on them, if so will return a 1)
 * [Valaris]
 *------------------------------------------
 */

int buildin_hasitems(CScriptEngine &st)
{
	int i;
	struct map_session_data *sd;

	sd=script_rid2sd(st);

	for(i=0; i<MAX_INVENTORY; i++) {
		if(sd->status.inventory[i].amount && sd->status.inventory[i].nameid!=2364 && sd->status.inventory[i].nameid!=2365)
		{
			st.push_val(CScriptEngine::C_INT,1);
			return 0;
		}
	}
	st.push_val(CScriptEngine::C_INT,0);
	return 0;
}
// change npc walkspeed [Valaris]
int buildin_npcspeed(CScriptEngine &st)
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st.oid);
	int x=0;

	x=st.GetInt( (st[2]));

	if(nd) {
		nd->speed=x;
	}

	return 0;
}
// make an npc walk to a position [Valaris]
int buildin_npcwalkto(CScriptEngine &st)
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st.oid);
	int x=0,y=0;

	x=st.GetInt( (st[2]));
	y=st.GetInt( (st[3]));

	if(nd)
		npc_walktoxy(*nd,x,y,0);

	return 0;
}
// stop an npc's movement [Valaris]
int buildin_npcstop(CScriptEngine &st)
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st.oid);

	if( (nd) && (nd->state.npcstate==MS_WALK) )
		npc_stop_walking(*nd,1);
	return 0;
}


/*==========================================
  * getlook char info. getlook(arg)
  *------------------------------------------
  */
int buildin_getlook(CScriptEngine &st){
        int type,val;
        struct map_session_data *sd;
        sd=script_rid2sd(st);

        type=st.GetInt( (st[2]));
        val=-1;
        switch(type){
        case LOOK_HAIR:	//1
                val=sd->status.hair;
                break;
        case LOOK_WEAPON: //2
                val=sd->status.weapon;
                break;
        case LOOK_HEAD_BOTTOM: //3
                val=sd->status.head_bottom;
                break;
        case LOOK_HEAD_TOP: //4
                val=sd->status.head_top;
                break;
        case LOOK_HEAD_MID: //5
                val=sd->status.head_mid;
                break;
        case LOOK_HAIR_COLOR: //6
                val=sd->status.hair_color;
                break;
        case LOOK_CLOTHES_COLOR: //7
                val=sd->status.clothes_color;
                break;
        case LOOK_SHIELD: //8
                val=sd->status.shield;
                break;
        case LOOK_SHOES: //9
                break;
        }
        st.push_val(CScriptEngine::C_INT,val);
        return 0;
}

/*==========================================
  *     get char save point. argument: 0- map name, 1- x, 2- y
  *------------------------------------------
*/
int buildin_getsavepoint(CScriptEngine &st)
{
        int x,y,type;
        char *mapname;
        struct map_session_data *sd;

        sd=script_rid2sd(st);

        type=st.GetInt( (st[2]));
		mapname=(char*)aMalloc(24*sizeof(char));
		memcpy(mapname,sd->status.save_point.map,24);//EOS included

        x=sd->status.save_point.x;
        y=sd->status.save_point.y;
        switch(type){
            case 0:
                st.push_str(CScriptEngine::C_STR,mapname);
                break;
            case 1:
                st.push_val(CScriptEngine::C_INT,x);
                break;
            case 2:
                st.push_val(CScriptEngine::C_INT,y);
                break;
        }
        return 0;
}

/*==========================================
  * Get position for  char/npc/pet/mob objects. Added by Lorky
  *
  *     int getMapXY(MapName$,MaxX,MapY,type,[CharName$]);
  *             where type:
  *                     MapName$ - String variable for output map name
  *                     MapX     - Integer variable for output coord X
  *                     MapY     - Integer variable for output coord Y
  *                     type     - type of object
  *                                0 - Character coord
  *                                1 - NPC coord
  *                                2 - Pet coord
  *                                3 - Mob coord (not released)
  *                     CharName$ - Name object. If miss or "this" the current object
  *
  *             Return:
  *                     0        - success
  *                     -1       - some error, MapName$,MapX,MapY contains unknown value.
  *------------------------------------------
*/
int buildin_getmapxy(CScriptEngine &st)
{
	struct map_session_data *sd=NULL;
	struct npc_data *nd;
	struct pet_data *pd;
	int num;
	char *name;
	int x,y,type;
	char *mapname=NULL;

	if( st[2].type!=CScriptEngine::C_NAME )
	{
		ShowMessage("script: buildin_getmapxy: not mapname variable\n");
		st.push_val(CScriptEngine::C_INT,-1);
                return 0;
        }
	if( st[3].type!=CScriptEngine::C_NAME )
	{
		ShowMessage("script: buildin_getmapxy: not mapx variable\n");
		st.push_val(CScriptEngine::C_INT,-1);
                return 0;
        }
	if( st[4].type!=CScriptEngine::C_NAME )
	{
		ShowMessage("script: buildin_getmapxy: not mapy variable\n");
		st.push_val(CScriptEngine::C_INT,-1);
                return 0;
        }


//??????????? >>>  Possible needly check function parameters on C_STR,CScriptEngine::C_INT,CScriptEngine::C_INT <<< ???????????//
	type=st.GetInt( (st[5]));

	switch(type)
	{
            case 0:                                             //Get Character Position
		if( st.end>st.start+6 )
			sd=map_nick2sd(st.GetString( (st[6])));
                    else
                        sd=script_rid2sd(st);
		if( sd==NULL )
		{	//wrong char name or char offline
			st.push_val(CScriptEngine::C_INT,-1);
                        return 0;
                    }
                    x=sd->bl.x;
                    y=sd->bl.y;
		mapname = sd->mapname;
		ShowMessage(">>>>%s %d %d\n",mapname,x,y);
                    break;
            case 1:                                             //Get NPC Position
		if( st.end > st.start+6 )
			nd=npc_name2id(st.GetString( (st[6])));
                    else
			nd=(struct npc_data *)map_id2bl(st.oid);
		if( nd==NULL )
		{	//wrong npc name or char offline
			st.push_val(CScriptEngine::C_INT,-1);
                        return 0;
                    }
                    x=nd->bl.x;
                    y=nd->bl.y;
		mapname=map[nd->bl.m].mapname;
		ShowMessage(">>>>%s %d %d\n",mapname,x,y);
                    break;
            case 2:                                             //Get Pet Position
		if( st.end>st.start+6 )
			sd=map_nick2sd(st.GetString( (st[6])));
                    else
                        sd=script_rid2sd(st);
		if( sd==NULL )
		{	//wrong char name or char offline
			st.push_val(CScriptEngine::C_INT,-1);
                        return 0;
                    }
                    pd=sd->pd;
		if(pd==NULL)
		{	//ped data not found
			st.push_val(CScriptEngine::C_INT,-1);
                        return 0;
                    }
                    x=pd->bl.x;
                    y=pd->bl.y;
		mapname=map[pd->bl.m].mapname;
		ShowMessage(">>>>%s %d %d\n",mapname,x,y);
                    break;
            case 3:                                             //Get Mob Position
		st.push_val(CScriptEngine::C_INT,-1);
                        return 0;
            default:                                            //Wrong type parameter
		st.push_val(CScriptEngine::C_INT,-1);
                        return 0;
	}//end switch

	sd=script_rid2sd(st);
	if(sd)
	{
     //Set MapName$
		num=st[2].num;
        name=(char *)(str_buf+str_data[num&0x00ffffff].str);
		set_reg(st,num,name,mapname);

     //Set MapX
		num=st[3].num;
        name=(char *)(str_buf+str_data[num&0x00ffffff].str);
		set_reg(st,num,name,(void*)x);

     //Set MapY
		num=st[4].num;
        name=(char *)(str_buf+str_data[num&0x00ffffff].str);
		set_reg(st,num,name,(void*)y);

     //Return Success value
		st.push_val(CScriptEngine::C_INT,0);
        return 0;
	}
	st.push_val(CScriptEngine::C_INT,-1);
	return 0;
}

/*=====================================================
 * Allows players to use a skill - by Qamera
 *-----------------------------------------------------
 */
int buildin_skilluseid (CScriptEngine &st)
{
   int skid,sklv;
   struct map_session_data *sd;
   skid=st.GetInt( (st[2]));
   sklv=st.GetInt( (st[3]));
   sd=script_rid2sd(st);
   if(sd) skill_use_id(sd,sd->status.account_id,skid,sklv);
   return 0;
}

/*=====================================================
 * Allows players to use a skill on a position [Celest]
 *-----------------------------------------------------
 */
int buildin_skillusepos(CScriptEngine &st)
{
   int skid,sklv,x,y;
   struct map_session_data *sd;
   skid=st.GetInt( (st[2]));
   sklv=st.GetInt( (st[3]));
   x=st.GetInt( (st[4]));
   y=st.GetInt( (st[5]));
   sd=script_rid2sd(st);
   if(sd) skill_use_pos(sd,x,y,skid,sklv);
   return 0;
}

/*==========================================
 * Allows player to write NPC logs (i.e. Bank NPC, etc) [Lupus]
 *------------------------------------------
 */
int buildin_logmes(CScriptEngine &st)
{
	if (log_config.npc <= 0 ) return 0;
	st.GetString( (st[2]));
	map_session_data *sd = script_rid2sd(st);
	if(sd) log_npc(*sd,st[2].str);
	return 0;
}

int buildin_summon(CScriptEngine &st)
{
	int class_, id;
	const char *str,*event="";
	struct map_session_data *sd;
	struct mob_data *md;

	sd=script_rid2sd(st);
	if (sd) {
		unsigned long tick = gettick();
		str	=st.GetString( (st[2]));
		class_=st.GetInt( (st[3]));
		if( st.end>st.start+4 )
			event=st.GetString( (st[4]));

		id=mob_once_spawn(sd, "this", 0, 0, str,class_,1,event);
		if((md=(struct mob_data *)map_id2bl(id))){
			md->master_id=sd->bl.id;
			md->state.special_mob_ai=1;
			md->mode=mob_db[md->class_].mode|0x04;
			md->deletetimer=add_timer(tick+60000,mob_timer_delete,id,0);
			clif_misceffect2(md->bl,344);
		}
		clif_skill_poseffect(sd->bl,AM_CALLHOMUN,1,sd->bl.x,sd->bl.y,tick);
	}

	return 0;
}

/*==========================================
 * Checks whether it is daytime/nighttime
 *------------------------------------------
 */
int buildin_isnight(CScriptEngine &st)
{
	st.push_val(CScriptEngine::C_INT, (night_flag == 1));
	return 0;
}

int buildin_isday(CScriptEngine &st)
{
	st.push_val(CScriptEngine::C_INT, (night_flag == 0));
	return 0;
}

/*================================================
 * Check whether another item/card has been
 * equipped - used for 2/15's cards patch [celest]
 *------------------------------------------------
 */
// leave this here, just in case
#if 0
int buildin_isequipped(CScriptEngine &st)
{
	struct map_session_data *sd;
	int i, j, k, id = 1;
	int ret = -1;

	sd = script_rid2sd(st);
	if(sd)
	{
	for (i=0; id!=0; i++) {
		int flag = 0;
	
			if(st.end > st.start+i+2)
				id = st.GetInt((st[i+2]));
			else 
				id = 0;

		if (id <= 0)
			continue;
		
		for (j=0; j<10; j++) {
			int index, type;
			index = sd->equip_index[j];
			if(index >= MAX_INVENTORY) continue;
			if(j == 9 && sd->equip_index[8] == index) continue;
			if(j == 5 && sd->equip_index[4] == index) continue;
			if(j == 6 && (sd->equip_index[5] == index || sd->equip_index[4] == index)) continue;
			type = itemdb_type(id);
			
			if(sd->inventory_data[index]) {
				if (type == 4 || type == 5) {
					if (sd->inventory_data[index]->nameid == id)
						flag = 1;
				} else if (type == 6) {
					for(k=0; k<sd->inventory_data[index]->slot; k++) {
						if (sd->status.inventory[index].card[0]!=0x00ff &&
							sd->status.inventory[index].card[0]!=0x00fe &&
								sd->status.inventory[index].card[0]!=0xff00 &&
							sd->status.inventory[index].card[k] == id) {
							flag = 1;
							break;
						}
					}
				}
				if (flag) break;
			}
		}
		if (ret == -1)
			ret = flag;
		else
			ret &= flag;
		if (!ret) break;
	}
	}
	st.push_val(CScriptEngine::C_INT,ret);
	return 0;
}
#endif

/*================================================
 * Check how many items/cards in the list are
 * equipped - used for 2/15's cards patch [celest]
 *------------------------------------------------
 */
int buildin_isequippedcnt(CScriptEngine &st)
{
	struct map_session_data *sd;
	size_t i, j, k;
	unsigned long id = 1;
	int ret = 0;
	int index, type;

	sd = script_rid2sd(st);
	
	if(sd)
	{
	for (i=0; id!=0; i++) {

			if(st.end > st.start+i+2)
				id = st.GetInt((st[i+2]));
			else 
				id = 0;

		if (id <= 0)
			continue;
		
		for (j=0; j<10; j++) {
			index = sd->equip_index[j];
			if(index >= MAX_INVENTORY) continue;
			if(j == 9 && sd->equip_index[8] == index) continue;
			if(j == 5 && sd->equip_index[4] == index) continue;
			if(j == 6 && (sd->equip_index[5] == index || sd->equip_index[4] == index)) continue;
			type = itemdb_type(id);
			
			if(sd->inventory_data[index]) {
				if (type == 4 || type == 5) {
					if (sd->inventory_data[index]->nameid == id)
						ret++; //[Lupus]
				} else if (type == 6) {
						for(k=0; k<sd->inventory_data[index]->flag.slot; k++) {
						if (sd->status.inventory[index].card[0]!=0x00ff &&
							sd->status.inventory[index].card[0]!=0x00fe &&
								sd->status.inventory[index].card[0]!=0xff00 &&
							sd->status.inventory[index].card[k] == id) {
							ret++; //[Lupus]
						}
					}
				}				
			}
		}
	}
	}
	st.push_val(CScriptEngine::C_INT,ret);
	return 0;
}

/*================================================
 * Check whether another card has been
 * equipped - used for 2/15's cards patch [celest]
 * -- Items checked cannot be reused in another
 * card set to prevent exploits
 *------------------------------------------------
 */
int buildin_isequipped(CScriptEngine &st)
{
	struct map_session_data *sd;
	size_t i, j, k;
	unsigned short id = 1;
	int ret = -1;

	sd = script_rid2sd(st);
	
	if(sd)
	{
		for (i=0; id!=0; i++)
		{
		int flag = 0;
	
			if(st.end>st.start+(i+2))
				id=st.GetInt((st[(i+2)]));
			else
				id = 0;
			
		if (id <= 0)
			continue;
		
			for (j=0; j<10; j++)
			{
			int index, type;
			index = sd->equip_index[j];
			if(index >= MAX_INVENTORY) continue;
			if(j == 9 && sd->equip_index[8] == index) continue;
			if(j == 5 && sd->equip_index[4] == index) continue;
			if(j == 6 && (sd->equip_index[5] == index || sd->equip_index[4] == index)) continue;
			type = itemdb_type(id);
			
				if(sd->inventory_data[index])
				{
					if (type == 4 || type == 5)
					{
					if (sd->inventory_data[index]->nameid == id)
						flag = 1;
					}
					else if (type == 6)
					{	// Item Hash format:
					// 1111 1111 1111 1111 1111 1111 1111 1111
					// [ left  ] [ right ] [ NA ] [  armor  ]
						for (k = 0; k < sd->inventory_data[index]->flag.slot; k++)
						{	// --- Calculate hash for current card ---
						// Defense equipment
						// They *usually* have only 1 slot, so we just assign 1 bit
						int hash = 0;
							if (sd->inventory_data[index]->type == 5)
							{
							hash = sd->inventory_data[index]->equip;
						}
						// Weapons
							// right hand: slot 1 - 0x0010000 ... slot 4 - 0x0080000
						// left hand: slot 1 - 0x1000000 ... slot 4 - 0x8000000
						// We can support up to 8 slots each, just in case
							else if (sd->inventory_data[index]->type == 4)
							{
							if (sd->inventory_data[index]->equip & 2)	// right hand
									hash = 0x00010000 * (1<<k);	// pow(2,k) x slot number
							else if (sd->inventory_data[index]->equip & 32)	// left hand
									hash = 0x01000000 * (1<<k);	// pow(2,k) x slot number
							}
							else
							continue;	// slotted item not armour nor weapon? we're not going to support it

						if (sd->setitem_hash & hash)	// check if card is already used by another set
							continue;	// this item is used, move on to next card

						if (sd->status.inventory[index].card[0] != 0x00ff &&
							sd->status.inventory[index].card[0] != 0x00fe &&
								sd->status.inventory[index].card[0] != 0xff00 &&
							sd->status.inventory[index].card[k] == id)
						{
							// We have found a match
							flag = 1;
							// Set hash so this card cannot be used by another
							sd->setitem_hash |= hash;
							break;
						}
					}
				}
				if (flag) break;
			}
		}
		if (ret == -1)
			ret = flag;
		else
			ret &= flag;
		if (!ret) break;
	}
	}
	st.push_val(CScriptEngine::C_INT,ret);
	return 0;
}

/*================================================
 * Check how many given inserted cards in the CURRENT
 * weapon - used for 2/15's cards patch [Lupus]
 *------------------------------------------------
 */
int buildin_cardscnt(CScriptEngine &st)
{
	struct map_session_data *sd;
	size_t i, k;
	unsigned short id = 1;
	int ret = 0;
	int index, type;

	sd = script_rid2sd(st);
	
	if(sd)
	{
		for (i=0; id!=0; i++)
		{
			if(st.end>st.start+(i+2))
				id=st.GetInt((st[(i+2)]));
			else 
				id = 0;
			
		if (id <= 0)
			continue;
		
		index = current_equip_item_index; //we get CURRENT WEAPON inventory index from status.c [Lupus]
		if(index < 0) continue;

		type = itemdb_type(id);
			
			if(sd->inventory_data[index])
			{
				if (type == 4 || type == 5)
				{
				if (sd->inventory_data[index]->nameid == id)
					ret++;
				}
				else if (type == 6)
				{
					for(k=0; k<sd->inventory_data[index]->flag.slot; k++)
					{
					if (sd->status.inventory[index].card[0]!=0x00ff &&
						sd->status.inventory[index].card[0]!=0x00fe &&
							sd->status.inventory[index].card[0]!=0xff00 &&
							sd->status.inventory[index].card[k] == id )
						{
						ret++;
					}
				}
			}				
		}
	}
	}
	st.push_val(CScriptEngine::C_INT,ret);
	return 0;
}

/*=======================================================
 * Returns the refined number of the current item, or an
 * item with inventory index specified
 *-------------------------------------------------------
 */
int buildin_getrefine(CScriptEngine &st)
{
	struct map_session_data *sd;
	if ((sd = script_rid2sd(st))!= NULL)
		st.push_val( CScriptEngine::C_INT, sd->status.inventory[current_equip_item_index].refine);
	return 0;
}

/*=======================================================
 * Allows 2 Parents to adopt a character as a Baby
 *-------------------------------------------------------
 */
int buildin_adopt(CScriptEngine &st)
{
	int ret=0;
	
	const char *parent1 = st.GetString( (st[2]));
	const char *parent2 = st.GetString( (st[3]));
	const char *child = st.GetString( (st[4]));

	struct map_session_data *p1_sd = map_nick2sd(parent1);
	struct map_session_data *p2_sd = map_nick2sd(parent2);
	struct map_session_data *c_sd = map_nick2sd(child);

	if( p1_sd && p2_sd && c_sd &&
		p1_sd->status.base_level >= 70 &&
		p2_sd->status.base_level >= 70 )
		ret = pc_adoption(*p1_sd, *p2_sd, *c_sd);

	st.push_val( CScriptEngine::C_INT, ret);
	return 0;
}

/*=======================================================
 * Day/Night controls
 *-------------------------------------------------------
 */
int buildin_night(CScriptEngine &st)
{
	if (night_flag != 1) map_night_timer(night_timer_tid, 0, 0, 1);
	return 0;
}
int buildin_day(CScriptEngine &st)
{
	if (night_flag != 0) map_day_timer(day_timer_tid, 0, 0, 1);
	return 0;
}

//=======================================================
// Unequip [Spectre]
//-------------------------------------------------------
int buildin_unequip(CScriptEngine &st)
{
	int i;
	size_t num;
	struct map_session_data *sd;
	num = st.GetInt( (st[2])) - 1;
	sd=script_rid2sd(st);
	if(sd!=NULL && num<10)
	{
		i=pc_checkequip(*sd,equip[num]);
		pc_unequipitem(*sd,i,2);
		return 0;
	}
	return 0;
}


int buildin_pcstrcharinfo(CScriptEngine &st)
{
	int aid,num;
	struct map_session_data *sd;
	
	aid=st.GetInt( (st[2]));
	num=st.GetInt( (st[3]));
	
	sd=map_id2sd(aid);
	if(sd==NULL){
		st.push_str(CScriptEngine::C_CONSTSTR,"");
		num=-1;
	}
	if(num==0){
		char *buf;
		buf=(char*)aMalloc(24 * sizeof(char));
		safestrcpy(buf,sd->status.name, 24);
		st.push_str(CScriptEngine::C_STR,buf);
	}
	else if(num==1){
		char *buf;
		buf=buildin_getpartyname_sub(sd->status.party_id);
		if(buf!=NULL)
			st.push_str(CScriptEngine::C_STR,buf);
		else
			st.push_str(CScriptEngine::C_CONSTSTR,"");
	}
	else if(num==2){
		char *buf;
		buf=buildin_getguildname_sub(sd->status.guild_id);
		if(buf!=NULL)
			st.push_str(CScriptEngine::C_STR,buf);
		else
			st.push_str(CScriptEngine::C_CONSTSTR,"");
	}
	return 0;
}

//=======================================================
// strlen [Valaris]
//-------------------------------------------------------
int buildin_getstrlen(CScriptEngine &st)
{
	const char *str=st.GetString( (st[2]));
	int len = (str) ? (int)strlen(str) : 0;
	st.push_val(CScriptEngine::C_INT,len);
	return 0;
}

//=======================================================
// isalpha [Valaris]
//-------------------------------------------------------
int buildin_charisalpha(CScriptEngine &st)
{
	const char *str=st.GetString( (st[2]));
	size_t pos =st.GetInt( (st[3]));

	int val = ( str && pos>0 && pos<strlen(str) ) ? isalpha( (int)((unsigned char)str[pos]) ) : 0;
	st.push_val(CScriptEngine::C_INT, val);
	return 0;
}



// [Lance]
int buildin_fakenpcname(CScriptEngine &st)
{
	const char *name;
	const char *newname;
	unsigned long look;
	name = st.GetString( (st[2]));
	newname = st.GetString( (st[3]));
	look = st.GetInt( (st[4]));
	if(look > 0xFFFF) return 0; // Safety measure to prevent runtime errors
	npc_changename(name, newname, look);
	return 0;
}

int buildin_compare(CScriptEngine &st)                                 
{
	char buf1[1024],buf2[1024];
	const char *message;
	const char *cmpstring;

	message = st.GetString(st[2]);
	cmpstring = st.GetString(st[3]);
	
	strcpytolower(buf1, 1024, message);
	strcpytolower(buf2, 1024, cmpstring);
	
	st.push_val(CScriptEngine::C_INT,(strstr(buf1,buf2) != NULL));
	return 0;
}



/*==========================================
 * Warpparty - [Fredzilla]
 * Syntax: warpparty "mapname.gat",x,y,Party_ID;
 *------------------------------------------
 */
int buildin_warpparty(CScriptEngine &st)
{

	const char *str =st.GetString(st[2]);
	int x			=st.GetInt(st[3]);
	int y			=st.GetInt(st[4]);
	unsigned long p	=st.GetInt(st[5]);
	struct map_session_data *sd=script_rid2sd(st);

	if(!sd || map[sd->bl.m].flag.noreturn || map[sd->bl.m].flag.noteleport || NULL==party_search(p))
		return 0;
	
	if(p!=0)
	{
		size_t i;
		struct map_session_data *pl_sd;

		if( 0==strcasecmp(str,"Random") )
		{
			if(map[sd->bl.m].flag.noteleport)
				return 0;
			for(i=0; i<fd_max; i++)
			{
				if(session[i] && (pl_sd = (struct map_session_data *)session[i]->session_data) && pl_sd->state.auth &&
					pl_sd->status.party_id == p)
				{
					if(!map[pl_sd->bl.m].flag.noteleport)
						pc_randomwarp(*pl_sd,3);
				}
			}
		}
		else if( 0==strcasecmp(str,"SavePointAll") )
		{
			if(map[sd->bl.m].flag.noreturn)
				return 0;
			for(i=0; i<fd_max; i++)
			{
				if(session[i] && (pl_sd = (struct map_session_data *) session[i]->session_data) && pl_sd->state.auth &&
					pl_sd->status.party_id == p)
				{
					if(!map[pl_sd->bl.m].flag.noreturn)
						pc_setpos(*pl_sd,pl_sd->status.save_point.map,pl_sd->status.save_point.x,pl_sd->status.save_point.y,3);
				}
			}
		}
		else if( 0==strcasecmp(str,"SavePoint") )
		{
			if(map[sd->bl.m].flag.noreturn)
				return 0;
			str=sd->status.save_point.map;
			x=sd->status.save_point.x;
			y=sd->status.save_point.y;
			for (i = 0; i < fd_max; i++)
			{
				if(session[i] && (pl_sd = (struct map_session_data *) session[i]->session_data) && pl_sd->state.auth &&
					pl_sd->status.party_id == p)
				{
					if(!map[pl_sd->bl.m].flag.noreturn)
						pc_setpos(*pl_sd,str,x,y,3);
				}
			}
		}
		else
		{
			for (i = 0; i < fd_max; i++)
			{
				if(session[i] && (pl_sd = (struct map_session_data *) session[i]->session_data) && pl_sd->state.auth &&
					pl_sd->status.party_id == p)
				{
					if(!map[pl_sd->bl.m].flag.noreturn)
						pc_setpos(*pl_sd,str,x,y,3);
				}
			}
		}
	}
	return 0;
}
/*==========================================
 * Warpguild - [Fredzilla]
 * Syntax: warpguild "mapname.gat",x,y,Guild_ID;
 *------------------------------------------
 */
int buildin_warpguild(CScriptEngine &st)
{
	const char *str =st.GetString(st[2]);
	int x			=st.GetInt(st[3]);
	int y			=st.GetInt(st[4]);
	unsigned long g	=st.GetInt(st[5]);
	struct map_session_data *sd=script_rid2sd(st);

	if(!sd || map[sd->bl.m].flag.noreturn || map[sd->bl.m].flag.noteleport || NULL==guild_search(g) )
		return 0;
	
	if(g!=0)
	{
		size_t i;
		struct map_session_data *pl_sd;

		if( 0==strcasecmp(str,"Random") )
		{
			if(map[sd->bl.m].flag.noteleport)
				return 0;
			for(i=0; i<fd_max; i++)
			{
				if (session[i] && (pl_sd = (struct map_session_data *) session[i]->session_data) && pl_sd->state.auth &&
					pl_sd->status.guild_id == g)
				{
					if(!map[pl_sd->bl.m].flag.noteleport)
						pc_randomwarp(*pl_sd,3);
				}
			}
		}
		else if( 0==strcasecmp(str,"SavePointAll") )
		{
			if(map[sd->bl.m].flag.noreturn)
				return 0;
			for(i=0; i < fd_max; i++)
			{
				if (session[i] && (pl_sd = (struct map_session_data *) session[i]->session_data) && pl_sd->state.auth &&
					pl_sd->status.guild_id == g)
				{
					if(!map[pl_sd->bl.m].flag.noreturn)
						pc_setpos(*pl_sd,pl_sd->status.save_point.map,pl_sd->status.save_point.x,pl_sd->status.save_point.y,3);
				}
			}
		}
		else if( 0==strcasecmp(str,"SavePoint")==0 )
		{
			if(map[sd->bl.m].flag.noreturn)
				return 0;
			str=sd->status.save_point.map;
			x=sd->status.save_point.x;
			y=sd->status.save_point.y;
			for(i=0; i<fd_max; i++)
			{
				if(session[i] && (pl_sd = (struct map_session_data *) session[i]->session_data) && pl_sd->state.auth &&
					pl_sd->status.guild_id == g)
				{
					if(map[pl_sd->bl.m].flag.noreturn)
						pc_setpos(*pl_sd,str,x,y,3);
				}
			}
		}
		else
		{
			for(i=0; i<fd_max; i++)
			{
				if(session[i] && (pl_sd = (struct map_session_data *) session[i]->session_data) && pl_sd->state.auth &&
					pl_sd->status.guild_id == g)
				{
					if(map[pl_sd->bl.m].flag.noreturn)
						pc_setpos(*pl_sd,str,x,y,3);
				}
			}
		}
	}
	return 0;
}
/*==========================================
 *	Shows an emoticon on top of the player.
 *------------------------------------------
 */
// Added by request as official npcs seem to use it. [Skotlex]
int buildin_pc_emotion(CScriptEngine &st)
{
	struct map_session_data *sd = script_rid2sd(st);
	int type=st.GetInt( st[2]);
	if(sd && type >= 0 && type <= 100)
		clif_emotion(sd->bl,type);
	return 0;
}


/*==========================================
 * �}�b�v�ϐ��̕ύX
 *------------------------------------------
 */
int mapreg_setreg(int num,int val)
{
	if(val!=0)
		numdb_insert(mapreg_db,num,val);
	else
		numdb_erase(mapreg_db,num);

	mapreg_dirty=1;
	return 0;
}
/*==========================================
 * ������^�}�b�v�ϐ��̕ύX
 *------------------------------------------
 */
int mapreg_setregstr(int num,const char *str)
{
	char *p;

	if( (p=(char *) numdb_search(mapregstr_db,num))!=NULL )
		aFree(p);

	if( str==NULL || *str==0 ){
		numdb_erase(mapregstr_db,num);
		mapreg_dirty=1;
		return 0;
	}
	p=(char *)aMalloc( (strlen(str)+1)*sizeof(char));
	strcpy(p,str);
	numdb_insert(mapregstr_db,num,p);
	mapreg_dirty=1;
	return 0;
}

/*==========================================
 * �i���I�}�b�v�ϐ��̓ǂݍ���
 *------------------------------------------
 */
int script_load_mapreg()
{
	FILE *fp;
	char line[1024];

	if( (fp=safefopen(mapreg_txt,"rt"))==NULL )
		return -1;

	while(fgets(line,sizeof(line),fp)){
		char buf1[256],buf2[1024],*p;
		int n,v,s,i;
		if( sscanf(line,"%255[^,],%d\t%n",buf1,&i,&n)!=2 &&
			(i=0,sscanf(line,"%[^\t]\t%n",buf1,&n)!=1) )
			continue;
		if( buf1[strlen(buf1)-1]=='$' ){
			if( sscanf(line+n,"%[^\n\r]",buf2)!=1 ){
				ShowMessage("%s: %s broken data !\n",mapreg_txt,buf1);
				continue;
			}
			p=(char *)aMalloc( (strlen(buf2) + 1)*sizeof(char));
			strcpy(p,buf2);
			s= add_str( buf1);
			numdb_insert(mapregstr_db,(i<<24)|s,p);
		}else{
			if( sscanf(line+n,"%d",&v)!=1 ){
				ShowMessage("%s: %s broken data !\n",mapreg_txt,buf1);
				continue;
			}
			s= add_str( buf1);
			numdb_insert(mapreg_db,(i<<24)|s,v);
		}
	}
	fclose(fp);
	mapreg_dirty=0;
	return 0;
}
/*==========================================
 * �i���I�}�b�v�ϐ��̏�������
 *------------------------------------------
 */
int script_save_mapreg_intsub(void *key,void *data,va_list ap)
{
	FILE *fp=va_arg(ap,FILE*);
	int num=((int)key)&0x00ffffff, i=((int)key)>>24;
	char *name=str_buf+str_data[num].str;
	if( name[1]!='@' ){
		if(i==0)
			fprintf(fp,"%s\t%d\n", name, (int)data);
		else
			fprintf(fp,"%s,%d\t%d\n", name, i, (int)data);
	}
	return 0;
}
int script_save_mapreg_strsub(void *key,void *data,va_list ap)
{
	FILE *fp=va_arg(ap,FILE*);
	int num=((int)key)&0x00ffffff, i=((int)key)>>24;
	char *name=str_buf+str_data[num].str;
	if( name[1]!='@' ){
		if(i==0)
			fprintf(fp,"%s\t%s\n", name, (char *)data);
		else
			fprintf(fp,"%s,%d\t%s\n", name, i, (char *)data);
	}
	return 0;
}
int script_save_mapreg()
{
	FILE *fp;
	int lock;

	if( (fp=lock_fopen(mapreg_txt,&lock))==NULL )
		return -1;
	numdb_foreach(mapreg_db,script_save_mapreg_intsub,fp);
	numdb_foreach(mapregstr_db,script_save_mapreg_strsub,fp);
	lock_fclose(fp,mapreg_txt,&lock);
	mapreg_dirty=0;
	return 0;
}
int script_autosave_mapreg(int tid,unsigned long tick,int id,int data)
{
	if(mapreg_dirty)
		script_save_mapreg();
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int set_posword(const char *p)
{
	const char* np;
	int i;
	if(p)
	for(i=0;i<MAX_EQUIP;i++)
	{
		if((np=strchr(p,','))!=NULL)
		{	// copy up to the comma
			memcpy(positions[i],p,np-p);
			positions[i][np-p]=0; // set the eos explicitly
			p=np+1;
		}
		else
		{	//copy the rest including the eos
			memcpy(positions[i],p,1+strlen(p));
			p+=strlen(p);
		}
	}
	return 0;
}

int script_config_read(const char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;

	script_config.verbose_mode = 0;
	script_config.warn_func_no_comma = 1;
	script_config.warn_cmd_no_comma = 1;
	script_config.warn_func_mismatch_paramnum = 1;
	script_config.warn_cmd_mismatch_paramnum = 1;
	script_config.check_cmdcount=16384;
	script_config.check_gotocount=1024;


	script_config.event_script_type = 0;
	script_config.event_requires_trigger = 1;

	fp=safefopen(cfgName,"r");
	if (fp == NULL) {
		ShowError("file not found: %s\n",cfgName);
		return 1;
	}
	while (fgets(line, sizeof(line), fp)) {
		if( !skip_empty_line(line) )
			continue;
		i = sscanf(line,"%[^:]: %[^\r\n]",w1,w2);
		if (i != 2)
			continue;
		if(strcasecmp(w1,"refine_posword")==0) {
			set_posword(w2);
		}
		else if(strcasecmp(w1,"verbose_mode")==0) {
			script_config.verbose_mode = config_switch(w2);
		}
		else if(strcasecmp(w1,"warn_func_no_comma")==0) {
			script_config.warn_func_no_comma = config_switch(w2);
		}
		else if(strcasecmp(w1,"warn_cmd_no_comma")==0) {
			script_config.warn_cmd_no_comma = config_switch(w2);
		}
		else if(strcasecmp(w1,"warn_func_mismatch_paramnum")==0) {
			script_config.warn_func_mismatch_paramnum = config_switch(w2);
		}
		else if(strcasecmp(w1,"warn_cmd_mismatch_paramnum")==0) {
			script_config.warn_cmd_mismatch_paramnum = config_switch(w2);
		}
		else if(strcasecmp(w1,"check_cmdcount")==0) {
			script_config.check_cmdcount = config_switch(w2);
		}
		else if(strcasecmp(w1,"check_gotocount")==0) {
			script_config.check_gotocount = config_switch(w2);
		}
		else if(strcasecmp(w1,"event_script_type")==0) {
			script_config.event_script_type = config_switch(w2);
		}
		else if(strcasecmp(w1,"die_event_name")==0) {			
			safestrcpy(script_config.die_event_name, w2,sizeof(script_config.die_event_name));
		}
		else if(strcasecmp(w1,"kill_event_name")==0) {
			safestrcpy(script_config.kill_event_name, w2,sizeof(script_config.kill_event_name));
		}
		else if(strcasecmp(w1,"login_event_name")==0) {
			safestrcpy(script_config.login_event_name, w2,sizeof(script_config.login_event_name));
		}
		else if(strcasecmp(w1,"logout_event_name")==0) {
			safestrcpy(script_config.logout_event_name, w2, sizeof(script_config.logout_event_name));
		}
		else if(strcasecmp(w1,"mapload_event_name")==0) {
			safestrcpy(script_config.mapload_event_name, w2, sizeof(script_config.mapload_event_name));
		}
		else if(strcasecmp(w1,"event_requires_trigger")==0) {
			script_config.event_requires_trigger = config_switch(w2);
		}
		else if(strcasecmp(w1,"import")==0){
			script_config_read(w2);
		}
	}
	fclose(fp);

	return 0;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
int mapreg_db_final(void *key,void *data,va_list ap)
{
	return 0;
}
int mapregstr_db_final(void *key,void *data,va_list ap)
{
	aFree(data);
	return 0;
}
int scriptlabel_db_final(void *key,void *data,va_list ap)
{
	return 0;
}
int userfunc_db_final(void *key,void *data,va_list ap)
{
	aFree(key);
	aFree(data);
	return 0;
}
int do_final_script()
{
	if(mapreg_dirty>=0)
		script_save_mapreg();

	if(mapreg_db)
	{
		numdb_final(mapreg_db,mapreg_db_final);
		mapreg_db=NULL;
	}
	if(mapregstr_db)
	{
		strdb_final(mapregstr_db,mapregstr_db_final);
		mapregstr_db=NULL;
	}
	if(scriptlabel_db)
	{
		strdb_final(scriptlabel_db,scriptlabel_db_final);
		scriptlabel_db=NULL;
	}
	if(userfunc_db)
	{
		strdb_final(userfunc_db,userfunc_db_final);
		userfunc_db=NULL;
	}

	if(str_data)
	{
		aFree(str_data);
		str_data=NULL;
	}
	if(str_buf)
	{
		aFree(str_buf);
		str_buf=NULL;
	}

	return 0;
}
/*==========================================
 * ������
 *------------------------------------------
 */
int do_init_script()
{
	memset(&script_config, 0, sizeof(script_config));

	safestrcpy(script_config.die_event_name    ,"PCDieEvent",sizeof(script_config.die_event_name));
	safestrcpy(script_config.kill_event_name   ,"PCKillEvent",sizeof(script_config.kill_event_name));
	safestrcpy(script_config.login_event_name  ,"PCLoginEvent",sizeof(script_config.login_event_name));
	safestrcpy(script_config.logout_event_name ,"PCLogoutEvent",sizeof(script_config.logout_event_name));
	safestrcpy(script_config.mapload_event_name,"PCLoadMapEvent",sizeof(script_config.mapload_event_name));
	script_config.verbose_mode = 0;
	script_config.warn_func_no_comma = 1;
	script_config.warn_cmd_no_comma = 1;
	script_config.warn_func_mismatch_paramnum = 1;
	script_config.warn_cmd_mismatch_paramnum = 1;
	script_config.event_requires_trigger = 1;
	script_config.event_script_type = 1;
	script_config.check_cmdcount = 0;
	script_config.check_gotocount = 0;


	mapreg_db=numdb_init();
	mapregstr_db=numdb_init();
	script_load_mapreg();

	add_timer_func_list(script_autosave_mapreg,"script_autosave_mapreg");
	add_timer_interval(gettick()+MAPREG_AUTOSAVE_INTERVAL,MAPREG_AUTOSAVE_INTERVAL,
		script_autosave_mapreg,0,0);

	if (scriptlabel_db == NULL)
	  scriptlabel_db=strdb_init(50);
	return 0;
}
