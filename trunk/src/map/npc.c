// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/timer.h"
#include "../common/nullpo.h"
#include "../common/malloc.h"
#include "../common/showmsg.h"
#include "../common/strlib.h"
#include "../common/utils.h"
#include "../common/ers.h"
#include "../common/db.h"
#include "map.h"
#include "log.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "status.h"
#include "itemdb.h"
#include "script.h"
#include "mob.h"
#include "pet.h"
#include "battle.h"
#include "skill.h"
#include "unit.h"
#include "npc.h"
#include "chat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>


// linked list of npc source files
struct npc_src_list {
	struct npc_src_list* next;
	char name[4]; // dynamic array, the structure is allocated with extra bytes (string length)
};
static struct npc_src_list* npc_src_files = NULL;

static int npc_id=START_NPC_NUM;
static int npc_warp=0;
static int npc_shop=0;
static int npc_script=0;
static int npc_mob=0;
static int npc_delay_mob=0;
static int npc_cache_mob=0;
int npc_get_new_npc_id(void){ return npc_id++; }

static struct dbt *ev_db;
static struct dbt *npcname_db;

struct event_data {
	struct npc_data *nd;
	int pos;
};
static struct tm ev_tm_b;	// 時計イベント用

static struct eri *timer_event_ers; //For the npc timer data. [Skotlex]

//For holding the view data of npc classes. [Skotlex]
static struct view_data npc_viewdb[MAX_NPC_CLASS];

static struct script_event_s
{	//Holds pointers to the commonly executed scripts for speedup. [Skotlex]
	struct npc_data *nd;
	struct event_data *event[UCHAR_MAX];
	const char *event_name[UCHAR_MAX];
	uint8 event_count;
} script_event[NPCE_MAX];

struct view_data* npc_get_viewdata(int class_)
{	//Returns the viewdata for normal npc classes.
	if (class_ == INVISIBLE_CLASS)
		return &npc_viewdb[0];
	if (npcdb_checkid(class_) || class_ == WARP_CLASS)
		return &npc_viewdb[class_];
	return NULL;
}
/*==========================================
 * NPCの無効化/有効化
 * npc_enable
 * npc_enable_sub 有効時にOnTouchイベントを実行
 *------------------------------------------*/
int npc_enable_sub(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd;
	struct npc_data *nd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, nd=va_arg(ap,struct npc_data *));
	if(bl->type == BL_PC && (sd=(struct map_session_data *)bl))
	{
		char name[NAME_LENGTH*2+3];

		if (nd->sc.option&OPTION_INVISIBLE)	// 無効化されている
			return 1;

		if(sd->areanpc_id==nd->bl.id)
			return 1;
		sd->areanpc_id=nd->bl.id;

		snprintf(name, ARRAYLENGTH(name), "%s::OnTouch", nd->exname); // exname to be specific. exname is the unique identifier for script events. [Lance]
		npc_event(sd,name,0);
	}
	//aFree(name);
	return 0;
}
int npc_enable(const char* name, int flag)
{
	struct npc_data* nd = strdb_get(npcname_db, name);
	if (nd==NULL)
		return 0;

	if (flag&1)
		nd->sc.option&=~OPTION_INVISIBLE;
	else if (flag&2)
		nd->sc.option&=~OPTION_HIDE;
	else if (flag&4)
		nd->sc.option|= OPTION_HIDE;
	else	//Can't change the view_data to invisible class because the view_data for all npcs is shared! [Skotlex]
		nd->sc.option|= OPTION_INVISIBLE;

	if (nd->class_ == WARP_CLASS || nd->class_ == FLAG_CLASS)
	{	//Client won't display option changes for these classes [Toms]
		if (nd->sc.option&(OPTION_HIDE|OPTION_INVISIBLE))
			clif_clearunit_area(&nd->bl, 0);
		else
			clif_spawn(&nd->bl);
	} else
		clif_changeoption(&nd->bl);
		
	if(flag&3 && (nd->u.scr.xs > 0 || nd->u.scr.ys >0))
		map_foreachinarea( npc_enable_sub,nd->bl.m,nd->bl.x-nd->u.scr.xs,nd->bl.y-nd->u.scr.ys,nd->bl.x+nd->u.scr.xs,nd->bl.y+nd->u.scr.ys,BL_PC,nd);

	return 0;
}

/*==========================================
 * NPCを名前で探す
 *------------------------------------------*/
struct npc_data* npc_name2id(const char* name)
{
	return (struct npc_data *) strdb_get(npcname_db, name);
}

/*==========================================
 * イベントキューのイベント処理
 *------------------------------------------*/
int npc_event_dequeue(struct map_session_data* sd)
{
	nullpo_retr(0, sd);

	if(sd->npc_id)
	{	//Current script is aborted.
		if(sd->state.using_fake_npc){
			clif_clearunit_single(sd->npc_id, 0, sd->fd);
			sd->state.using_fake_npc = 0;
		}
		if (sd->st) {
			sd->st->pos = -1;
			script_free_stack(sd->st->stack);
			aFree(sd->st);
			sd->st = NULL;
		}
		sd->npc_id = 0;
	}

	if (!sd->eventqueue[0][0])
		return 0; //Nothing to dequeue

	if (!pc_addeventtimer(sd,100,sd->eventqueue[0]))
	{	//Failed to dequeue, couldn't set a timer.
		ShowWarning("npc_event_dequeue: event timer is full !\n");
		return 0;
	}
	//Event dequeued successfully, shift other elements.
	memmove(sd->eventqueue[0], sd->eventqueue[1], (MAX_EVENTQUEUE-1)*sizeof(sd->eventqueue[0]));
	sd->eventqueue[MAX_EVENTQUEUE-1][0]=0;
	return 1;
}

/*==========================================
 * exports a npc event label
 * npc_parse_script->strdb_foreachから呼ばれる
 *------------------------------------------*/
int npc_event_export(char* lname, void* data, va_list ap)
{
	int pos = (int)data;
	struct npc_data* nd = va_arg(ap, struct npc_data *);

	if ((lname[0]=='O' || lname[0]=='o')&&(lname[1]=='N' || lname[1]=='n')) {
		struct event_data *ev;
		char buf[NAME_LENGTH*2+3];
		char* p = strchr(lname, ':');
		// エクスポートされる
		ev = (struct event_data *) aMalloc(sizeof(struct event_data));
		if (ev==NULL) {
			ShowFatalError("npc_event_export: out of memory !\n");
			exit(EXIT_FAILURE);
		}else if (p==NULL || (p-lname)>NAME_LENGTH) {
			ShowFatalError("npc_event_export: label name error !\n");
			exit(EXIT_FAILURE);
		}else{
			ev->nd = nd;
			ev->pos = pos;
			*p = '\0';
			snprintf(buf, ARRAYLENGTH(buf), "%s::%s", nd->exname, lname);
			*p = ':';
			strdb_put(ev_db, buf, ev);
		}
	}
	return 0;
}

int npc_event_sub(struct map_session_data* sd, struct event_data* ev, const char* eventname); //[Lance]
/*==========================================
 * 全てのNPCのOn*イベント実行
 *------------------------------------------*/
int npc_event_doall_sub(DBKey key, void* data, va_list ap)
{
	const char* p = key.str;
	struct event_data* ev;
	int* c;
	int rid;
	char* name;

	ev = (struct event_data *)data;
	c = va_arg(ap, int *);
	name = va_arg(ap,char *);
	rid = va_arg(ap, int);

	if( (p=strchr(p, ':')) && p && strcmpi(name, p)==0 ) {
		if(rid)
			npc_event_sub(((struct map_session_data *)map_id2bl(rid)),ev,key.str);
		else
			run_script(ev->nd->u.scr.script,ev->pos,rid,ev->nd->bl.id);
		(*c)++;
	}

	return 0;
}
int npc_event_doall(const char* name)
{
	int c = 0;
	char buf[64] = "::";

	strncpy(buf+2, name, 62);
	ev_db->foreach(ev_db,npc_event_doall_sub,&c,buf,0);
	return c;
}
int npc_event_doall_id(const char* name, int rid)
{
	int c = 0;
	char buf[64] = "::";

	strncpy(buf+2, name, 62);
	ev_db->foreach(ev_db,npc_event_doall_sub,&c,buf,rid);
	return c;
}

int npc_event_do_sub(DBKey key, void* data, va_list ap)
{
	const char* p = key.str;
	struct event_data* ev;
	int* c;
	const char* name;

	nullpo_retr(0, ev = (struct event_data *)data);
	nullpo_retr(0, ap);
	nullpo_retr(0, c = va_arg(ap, int *));

	name = va_arg(ap, const char *);

	if (p && strcmpi(name, p)==0) {
		run_script(ev->nd->u.scr.script,ev->pos,0,ev->nd->bl.id);
		(*c)++;
	}

	return 0;
}
int npc_event_do(const char* name)
{
	int c = 0;

	if (*name == ':' && name[1] == ':') {
		return npc_event_doall(name+2);
	}

	ev_db->foreach(ev_db,npc_event_do_sub,&c,name);
	return c;
}

/*==========================================
 * 時計イベント実行
 *------------------------------------------*/
int npc_event_do_clock(int tid, unsigned int tick, int id, int data)
{
	time_t timer;
	struct tm *t;
	char buf[64];
        char *day="";
	int c=0;

	time(&timer);
	t=localtime(&timer);

        switch (t->tm_wday) {
	case 0: day = "Sun"; break;
	case 1: day = "Mon"; break;
	case 2: day = "Tue"; break;
	case 3: day = "Wed"; break;
	case 4: day = "Thu"; break;
	case 5: day = "Fri"; break;
	case 6: day = "Sat"; break;
	}

	if (t->tm_min != ev_tm_b.tm_min ) {
		sprintf(buf,"OnMinute%02d",t->tm_min);
		c+=npc_event_doall(buf);
		sprintf(buf,"OnClock%02d%02d",t->tm_hour,t->tm_min);
		c+=npc_event_doall(buf);
		sprintf(buf,"On%s%02d%02d",day,t->tm_hour,t->tm_min);
		c+=npc_event_doall(buf);
	}
	if (t->tm_hour!= ev_tm_b.tm_hour) {
		sprintf(buf,"OnHour%02d",t->tm_hour);
		c+=npc_event_doall(buf);
	}
	if (t->tm_mday!= ev_tm_b.tm_mday) {
		sprintf(buf,"OnDay%02d%02d",t->tm_mon+1,t->tm_mday);
		c+=npc_event_doall(buf);
	}
	memcpy(&ev_tm_b,t,sizeof(ev_tm_b));
	return c;
}
/*==========================================
 * OnInitイベント実行(&時計イベント開始)
 *------------------------------------------*/
int npc_event_do_oninit(void)
{
//	int c = npc_event_doall("OnInit");
	ShowStatus("Event '"CL_WHITE"OnInit"CL_RESET"' executed with '"
	CL_WHITE"%d"CL_RESET"' NPCs.\n",npc_event_doall("OnInit"));

	add_timer_interval(gettick()+100,
		npc_event_do_clock,0,0,1000);

	return 0;
}

/*==========================================
 * タイマーイベント用ラベルの取り込み
 * npc_parse_script->strdb_foreachから呼ばれる
 *------------------------------------------*/
int npc_timerevent_import(char* lname, void* data, va_list ap)
{
	int pos=(int)data;
	struct npc_data *nd=va_arg(ap,struct npc_data *);
	int t=0,i=0;

	if(sscanf(lname,"OnTimer%d%n",&t,&i)==1 && lname[i]==':') {
		// タイマーイベント
		struct npc_timerevent_list *te=nd->u.scr.timer_event;
		int j,i=nd->u.scr.timeramount;
		if(te==NULL) te=(struct npc_timerevent_list*)aMallocA(sizeof(struct npc_timerevent_list));
		else te= (struct npc_timerevent_list*)aRealloc( te, sizeof(struct npc_timerevent_list) * (i+1) );
		if(te==NULL){
			ShowFatalError("npc_timerevent_import: out of memory !\n");
			exit(EXIT_FAILURE);
		}
		for(j=0;j<i;j++){
			if(te[j].timer>t){
				memmove(te+j+1,te+j,sizeof(struct npc_timerevent_list)*(i-j));
				break;
			}
		}
		te[j].timer=t;
		te[j].pos=pos;
		nd->u.scr.timer_event=te;
		nd->u.scr.timeramount++;
	}
	return 0;
}
struct timer_event_data {
	int rid; //Attached player for this timer.
	int next; //timer index (starts with 0, then goes up to nd->u.scr.timeramount
	int time; //holds total time elapsed for the script since time 0 (whenthe timers started)
	unsigned int otick; //Holds tick value at which timer sequence was started (that is, it stores the tick value for which T= 0
};

/*==========================================
 * タイマーイベント実行
 *------------------------------------------*/
int npc_timerevent(int tid, unsigned int tick, int id, int data)
{
	int next,t,old_rid,old_timer;
	unsigned int old_tick;
	struct npc_data* nd=(struct npc_data *)map_id2bl(id);
	struct npc_timerevent_list *te;
	struct timer_event_data *ted = (struct timer_event_data*)data;
	struct map_session_data *sd=NULL;

	if( nd==NULL ){
		ShowError("npc_timerevent: NPC not found??\n");
		return 0;
	}
	if (ted->rid) {
		sd = map_id2sd(ted->rid);
		if (!sd) {
			if(battle_config.error_log)
				ShowError("npc_timerevent: Attached player not found.\n");
			ers_free(timer_event_ers, ted);
			return 0;
		}
	}
	old_rid = nd->u.scr.rid; //To restore it later.
	nd->u.scr.rid = sd?sd->bl.id:0;
	
	old_tick = nd->u.scr.timertick;
	nd->u.scr.timertick=ted->otick;
	te=nd->u.scr.timer_event+ ted->next;
	
	old_timer = nd->u.scr.timer;
	t = nd->u.scr.timer=ted->time;
	ted->next++;
	
	if( nd->u.scr.timeramount> ted->next){
		next= nd->u.scr.timer_event[ ted->next ].timer
			- nd->u.scr.timer_event[ ted->next-1 ].timer;
		ted->time+=next;
		if (sd)
			sd->npc_timer_id = add_timer(tick+next,npc_timerevent,id,(int)ted);
		else
			nd->u.scr.timerid = add_timer(tick+next,npc_timerevent,id,(int)ted);
	} else {
		if (sd)
			sd->npc_timer_id = -1;
		else
			nd->u.scr.timerid = -1;
		ers_free(timer_event_ers, ted);
	}
	run_script(nd->u.scr.script,te->pos,nd->u.scr.rid,nd->bl.id);
	//Restore previous data, only if this timer is a player-attached one.
	if (sd) {
		nd->u.scr.rid = old_rid;
		nd->u.scr.timer = old_timer;
		nd->u.scr.timertick = old_tick;
	}
	return 0;
}
/*==========================================
 * タイマーイベント開始
 *------------------------------------------*/
int npc_timerevent_start(struct npc_data* nd, int rid)
{
	int j,n, next;
	struct map_session_data *sd=NULL; //Player to whom script is attached.
	struct timer_event_data *ted;
		
	nullpo_retr(0, nd);

	n=nd->u.scr.timeramount;
	if( n==0 )
		return 0;

	for(j=0;j<n;j++){
		if( nd->u.scr.timer_event[j].timer > nd->u.scr.timer )
			break;
	}
	if(j>=n) // check if there is a timer to use !!BEFORE!! you write stuff to the structures [Shinomori]
		return 0;
	if (nd->u.scr.rid > 0) {
		//Try to attach timer to this player.
		sd = map_id2sd(nd->u.scr.rid);
		if (!sd) {
			if(battle_config.error_log)
				ShowError("npc_timerevent_start: Attached player not found!\n");
			return 1;
		}
	}
	//Check if timer is already started.
	if (sd) {
		if (sd->npc_timer_id != -1)
			return 0;
	} else if (nd->u.scr.timerid != -1)
		return 0;
		
	ted = ers_alloc(timer_event_ers, struct timer_event_data);
	ted->next = j;
	nd->u.scr.timertick=ted->otick=gettick();

	//Attach only the player if attachplayerrid was used.
	ted->rid = sd?sd->bl.id:0;

// Do not store it to make way to two types of timers: globals and personals.	
//	if (rid >= 0) nd->u.scr.rid=rid;	// changed to: attaching to given rid by default [Shinomori]
	// if rid is less than 0 leave it unchanged [celest]

	next = nd->u.scr.timer_event[j].timer - nd->u.scr.timer;
	ted->time = nd->u.scr.timer_event[j].timer;
	if (sd)
		sd->npc_timer_id = add_timer(gettick()+next,npc_timerevent,nd->bl.id,(int)ted);
	else
		nd->u.scr.timerid = add_timer(gettick()+next,npc_timerevent,nd->bl.id,(int)ted);
	return 0;
}
/*==========================================
 * タイマーイベント終了
 *------------------------------------------*/
int npc_timerevent_stop(struct npc_data* nd)
{
	struct map_session_data *sd =NULL;
	struct TimerData *td = NULL;
	int *tid;
	nullpo_retr(0, nd);
	if (nd->u.scr.rid) {
		sd = map_id2sd(nd->u.scr.rid);
		if (!sd) {
			if(battle_config.error_log)
				ShowError("npc_timerevent_stop: Attached player not found!\n");
			return 1;
		}
	}
	
	tid = sd?&sd->npc_timer_id:&nd->u.scr.timerid;
	
	if (*tid == -1) //Nothing to stop
		return 0;
	td = get_timer(*tid);
	if (td && td->data) 
		ers_free(timer_event_ers, (struct event_timer_data*)td->data);
	delete_timer(*tid,npc_timerevent);
	*tid = -1;
	//Set the timer tick to the time that has passed since the beginning of the timers and now.
	nd->u.scr.timer = DIFF_TICK(gettick(),nd->u.scr.timertick);
//	nd->u.scr.rid = 0; //Eh? why detach?
	return 0;
}
/*==========================================
 * Aborts a running npc timer that is attached to a player.
 *------------------------------------------*/
void npc_timerevent_quit(struct map_session_data* sd)
{
	struct TimerData *td;
	struct npc_data* nd;
	struct timer_event_data *ted;
	if (sd->npc_timer_id == -1)
		return;
	td = get_timer(sd->npc_timer_id);
	if (!td) {
		sd->npc_timer_id = -1;
		return; //??
	}
	nd = (struct npc_data *)map_id2bl(td->id);
	ted = (struct timer_event_data*)td->data;
	delete_timer(sd->npc_timer_id, npc_timerevent);
	sd->npc_timer_id = -1;
	if (nd && nd->bl.type == BL_NPC)
	{	//Execute OnTimerQuit
		char buf[NAME_LENGTH*2+3];
		struct event_data *ev;
		snprintf(buf, ARRAYLENGTH(buf), "%s::OnTimerQuit", nd->exname);
		ev = strdb_get(ev_db, buf);
		if(ev && ev->nd != nd) {
			ShowWarning("npc_timerevent_quit: Unable to execute \"OnTimerQuit\", two NPCs have the same event name [%s]!\n",buf);
			ev = NULL;
		}
		if (ev) {
			int old_rid,old_timer;
			unsigned int old_tick;
			//Set timer related info.
			old_rid = nd->u.scr.rid;
			nd->u.scr.rid = sd->bl.id;

			old_tick = nd->u.scr.timertick;
			nd->u.scr.timertick=ted->otick;

			old_timer = nd->u.scr.timer;
			nd->u.scr.timer=ted->time;
		
			//Execute label
			run_script(nd->u.scr.script,ev->pos,sd->bl.id,nd->bl.id);

			//Restore previous data.
			nd->u.scr.rid = old_rid;
			nd->u.scr.timer = old_timer;
			nd->u.scr.timertick = old_tick;
		}
	}
	ers_free(timer_event_ers, ted);
}

/*==========================================
 * タイマー値の所得
 *------------------------------------------*/
int npc_gettimerevent_tick(struct npc_data* nd)
{
	int tick;
	nullpo_retr(0, nd);

	tick=nd->u.scr.timer;
	if (nd->u.scr.timertick)
		tick+=DIFF_TICK(gettick(), nd->u.scr.timertick);
	return tick;
}
/*==========================================
 * タイマー値の設定
 *------------------------------------------*/
int npc_settimerevent_tick(struct npc_data* nd, int newtimer)
{
	int flag;
	struct map_session_data *sd=NULL;

	nullpo_retr(0, nd);

	if (nd->u.scr.rid) {
		sd = map_id2sd(nd->u.scr.rid);
		if (!sd) {
			if(battle_config.error_log)
				ShowError("npc_settimerevent_tick: Attached player not found!\n");
			return 1;
		}
		flag= sd->npc_timer_id != -1 ;
	} else
		flag= nd->u.scr.timerid != -1 ;

	if(flag)
		npc_timerevent_stop(nd);
	nd->u.scr.timer=newtimer;
	if(flag)
		npc_timerevent_start(nd, -1);
	return 0;
}

int npc_event_sub(struct map_session_data* sd, struct event_data* ev, const char* eventname)
{
	if ( sd->npc_id != 0 )
	{
		//Enqueue the event trigger.
		int i;
		ARR_FIND( 0, MAX_EVENTQUEUE, i, sd->eventqueue[i][0] == '\0' );
		if( i < MAX_EVENTQUEUE )
			safestrncpy(sd->eventqueue[i],eventname,50); //Event enqueued.
		else
			if( battle_config.error_log )
				ShowWarning("npc_event: event queue is full !\n");
		
		return 1;
	}
	if( ev->nd->sc.option&OPTION_INVISIBLE )
	{
		//Disabled npc, shouldn't trigger event.
		npc_event_dequeue(sd);
		return 2;
	}
	run_script(ev->nd->u.scr.script,ev->pos,sd->bl.id,ev->nd->bl.id);
	return 0;
}

/*==========================================
 * イベント型のNPC処理
 *------------------------------------------*/
int npc_event(struct map_session_data* sd, const char* eventname, int mob_kill)
{
	struct event_data* ev = strdb_get(ev_db, eventname);
	struct npc_data *nd;
	int xs,ys;
	char mobevent[100];

	if (sd == NULL) {
		nullpo_info(NLP_MARK);
		return 0;
	}

	if (ev == NULL && eventname && strcmp(((eventname)+strlen(eventname)-9),"::OnTouch") == 0)
		return 1;

	if (ev == NULL || (nd = ev->nd) == NULL) {
		if (mob_kill) {
			strcpy( mobevent, eventname);
			strcat( mobevent, "::OnMyMobDead");
			ev = strdb_get(ev_db, mobevent);
			if (ev == NULL || (nd = ev->nd) == NULL) {
				if (strnicmp(eventname, "GM_MONSTER",10) != 0)
					ShowError("npc_event: (mob_kill) event not found [%s]\n", mobevent);
				return 0;
			}
		} else {
			if (battle_config.error_log)
				ShowError("npc_event: event not found [%s]\n", eventname);
			return 0;
		}
	}

	xs=nd->u.scr.xs;
	ys=nd->u.scr.ys;
	if (xs>=0 && ys>=0 && (strcmp(((eventname)+strlen(eventname)-6),"Global") != 0) )
	{
		if (nd->bl.m >= 0) { //Non-invisible npc
		  	if (nd->bl.m != sd->bl.m )
				return 1;
			if ( xs>0 && (sd->bl.x<nd->bl.x-xs/2 || nd->bl.x+xs/2<sd->bl.x) )
				return 1;
			if ( ys>0 && (sd->bl.y<nd->bl.y-ys/2 || nd->bl.y+ys/2<sd->bl.y) )
				return 1;
		}
	}
	
	return npc_event_sub(sd,ev,eventname);
}

/*==========================================
 * 接触型のNPC処理
 *------------------------------------------*/
int npc_touch_areanpc(struct map_session_data* sd, int m, int x, int y)
{
	int i,f=1;
	int xs,ys;

	nullpo_retr(1, sd);

	if(sd->npc_id)
		return 1;

	for(i=0;i<map[m].npc_num;i++) {
		if (map[m].npc[i]->sc.option&OPTION_INVISIBLE) {	// 無効化されている
			f=0;
			continue;
		}

		switch(map[m].npc[i]->bl.subtype) {
		case WARP:
			xs=map[m].npc[i]->u.warp.xs;
			ys=map[m].npc[i]->u.warp.ys;
			break;
		case SCRIPT:
			xs=map[m].npc[i]->u.scr.xs;
			ys=map[m].npc[i]->u.scr.ys;
			break;
		default:
			continue;
		}
		if (x >= map[m].npc[i]->bl.x-xs/2 && x < map[m].npc[i]->bl.x-xs/2+xs &&
		   y >= map[m].npc[i]->bl.y-ys/2 && y < map[m].npc[i]->bl.y-ys/2+ys)
			break;
	}
	if (i==map[m].npc_num) {
		if (f) {
			if (battle_config.error_log)
				ShowError("npc_touch_areanpc : some bug \n");
		}
		return 1;
	}
	switch(map[m].npc[i]->bl.subtype) {
		case WARP:
			// hidden chars cannot use warps -- is it the same for scripts too?
			if (sd->sc.option&(OPTION_HIDE|OPTION_CLOAK|OPTION_CHASEWALK) ||
				(!battle_config.duel_allow_teleport && sd->duel_group)) // duel rstrct [LuzZza]
				break;
			pc_setpos(sd,map[m].npc[i]->u.warp.mapindex,map[m].npc[i]->u.warp.x,map[m].npc[i]->u.warp.y,0);
			break;
		case SCRIPT:
		{
			char name[NAME_LENGTH*2+3];

			if(sd->areanpc_id == map[m].npc[i]->bl.id)
				return 1;
			sd->areanpc_id = map[m].npc[i]->bl.id;

			snprintf(name, ARRAYLENGTH(name), "%s::OnTouch", map[m].npc[i]->exname); // It goes here too. exname being the unique identifier. [Lance]

			if( npc_event(sd,name,0)>0 ) {
				pc_stop_walking(sd,1); //Make it stop walking!
				npc_click(sd,map[m].npc[i]);
			}
			//aFree(name);
			break;
		}
	}
	return 0;
}

int npc_touch_areanpc2(struct block_list* bl)
{
	int i,m=bl->m;
	int xs,ys;

	for(i=0;i<map[m].npc_num;i++) {
		if (map[m].npc[i]->sc.option&OPTION_INVISIBLE)
			continue;

		if (map[m].npc[i]->bl.subtype!=WARP)
			continue;
	
		xs=map[m].npc[i]->u.warp.xs;
		ys=map[m].npc[i]->u.warp.ys;

		if (bl->x >= map[m].npc[i]->bl.x-xs/2 && bl->x < map[m].npc[i]->bl.x-xs/2+xs &&
		   bl->y >= map[m].npc[i]->bl.y-ys/2 && bl->y < map[m].npc[i]->bl.y-ys/2+ys)
			break;
	}
	if (i==map[m].npc_num)
		return 0;
	
	xs = map_mapindex2mapid(map[m].npc[i]->u.warp.mapindex);
	if (xs < 0) // Can't warp object between map servers...
		return 0;

	if (unit_warp(bl, xs, map[m].npc[i]->u.warp.x,map[m].npc[i]->u.warp.y,0))
		return 0; //Failed to warp.

	return 1;
}

//Checks if there are any NPC on-touch objects on the given range.
//Flag determines the type of object to check for:
//&1: NPC Warps
//&2: NPCs with on-touch events.
int npc_check_areanpc(int flag, int m, int x, int y, int range)
{
	int i;
	int x0,y0,x1,y1;
	int xs,ys;

	if (range < 0) return 0;
	x0 = max(x-range, 0);
	y0 = max(y-range, 0);
	x1 = min(x+range, map[m].xs-1);
	y1 = min(y+range, map[m].ys-1);
	
	//First check for npc_cells on the range given
	i = 0;
	for (ys = y0; ys <= y1 && !i; ys++) {
		for(xs = x0; xs <= x1 && !i; xs++){
			if (map_getcell(m,xs,ys,CELL_CHKNPC))
				i = 1;
		}
	}
	if (!i) return 0; //No NPC_CELLs.

	//Now check for the actual NPC on said range.
	for(i=0;i<map[m].npc_num;i++)
	{
		if (map[m].npc[i]->sc.option&OPTION_INVISIBLE)
			continue;

		switch(map[m].npc[i]->bl.subtype)
		{
		case WARP:
			if (!(flag&1))
				continue;
			xs=map[m].npc[i]->u.warp.xs;
			ys=map[m].npc[i]->u.warp.ys;
			break;
		case SCRIPT:
			if (!(flag&2))
				continue;
			xs=map[m].npc[i]->u.scr.xs;
			ys=map[m].npc[i]->u.scr.ys;
			break;
		default:
			continue;
		}

		if (x1 >= map[m].npc[i]->bl.x-xs/2 && x0 < map[m].npc[i]->bl.x-xs/2+xs &&
			y1 >= map[m].npc[i]->bl.y-ys/2 && y0 < map[m].npc[i]->bl.y-ys/2+ys)
			break; // found a npc
	}
	if (i==map[m].npc_num)
		return 0;

	return (map[m].npc[i]->bl.id);
}

/*==========================================
 * 近くかどうかの判定
 *------------------------------------------*/
int npc_checknear2(struct map_session_data* sd, struct block_list* bl)
{
	nullpo_retr(1, sd);
	if(bl == NULL) return 1;
	
	if(sd->state.using_fake_npc && sd->npc_id == bl->id)
		return 0;

	if (status_get_class(bl)<0) //Class-less npc, enable click from anywhere.
		return 0;

	if (bl->m!=sd->bl.m ||
	   bl->x<sd->bl.x-AREA_SIZE-1 || bl->x>sd->bl.x+AREA_SIZE+1 ||
	   bl->y<sd->bl.y-AREA_SIZE-1 || bl->y>sd->bl.y+AREA_SIZE+1)
		return 1;

	return 0;
}

struct npc_data* npc_checknear(struct map_session_data* sd, struct block_list* bl)
{
	struct npc_data *nd;

	nullpo_retr(NULL, sd);
	if(bl == NULL) return NULL;
	if(bl->type != BL_NPC) return NULL;
	nd = (TBL_NPC*)bl;

	if(sd->state.using_fake_npc && sd->npc_id == bl->id)
		return nd;

	if (nd->class_<0) //Class-less npc, enable click from anywhere.
		return nd;

	if (bl->m!=sd->bl.m ||
	   bl->x<sd->bl.x-AREA_SIZE-1 || bl->x>sd->bl.x+AREA_SIZE+1 ||
	   bl->y<sd->bl.y-AREA_SIZE-1 || bl->y>sd->bl.y+AREA_SIZE+1)
		return NULL;

	return nd;
}

/*==========================================
 * NPCのオープンチャット発言
 *------------------------------------------*/
int npc_globalmessage(const char* name, const char* mes)
{
	struct npc_data* nd = (struct npc_data *) strdb_get(npcname_db, name);
	char temp[100];

	if (!nd)
		return 0;

	snprintf(temp, sizeof(temp), "%s : %s", name, mes);
	clif_GlobalMessage(&nd->bl,temp);

	return 0;
}

/*==========================================
 * クリック時のNPC処理
 *------------------------------------------*/
int npc_click(struct map_session_data* sd, struct npc_data* nd)
{
	nullpo_retr(1, sd);

	if (sd->npc_id != 0) {
		if (battle_config.error_log)
			ShowError("npc_click: npc_id != 0\n");
		return 1;
	}

	if(!nd) return 1;
	if ((nd = npc_checknear(sd,&nd->bl)) == NULL)
		return 1;
	//Hidden/Disabled npc.
	if (nd->class_ < 0 || nd->sc.option&(OPTION_INVISIBLE|OPTION_HIDE))
		return 1;

	switch(nd->bl.subtype) {
	case SHOP:
		clif_npcbuysell(sd,nd->bl.id);
		break;
	case SCRIPT:
		run_script(nd->u.scr.script,0,sd->bl.id,nd->bl.id);
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------*/
int npc_scriptcont(struct map_session_data* sd, int id)
{
	nullpo_retr(1, sd);

	if( id != sd->npc_id ){
		TBL_NPC* nd_sd=(TBL_NPC*)map_id2bl(sd->npc_id);
		TBL_NPC* nd=(TBL_NPC*)map_id2bl(id);
		ShowDebug("npc_scriptcont: %s (sd->npc_id=%d) is not %s (id=%d).\n",
			nd_sd?(char*)nd_sd->name:"'Unknown NPC'", (int)sd->npc_id,
		  	nd?(char*)nd->name:"'Unknown NPC'", (int)id);
		return 1;
	}
	
	if(id != fake_nd->bl.id) { // Not item script
		if ((npc_checknear(sd,map_id2bl(id))) == NULL){
			ShowWarning("npc_scriptcont: failed npc_checknear test.\n");
			return 1;
		}
	}
	run_script_main(sd->st);

	return 0;
}

/*==========================================
 *
 *------------------------------------------*/
int npc_buysellsel(struct map_session_data* sd, int id, int type)
{
	struct npc_data *nd;

	nullpo_retr(1, sd);

	if ((nd = npc_checknear(sd,map_id2bl(id))) == NULL)
		return 1;
	
	if (nd->bl.subtype!=SHOP) {
		if (battle_config.error_log)
			ShowError("no such shop npc : %d\n",id);
		if (sd->npc_id == id)
			sd->npc_id=0;
		return 1;
	}
	if (nd->sc.option&OPTION_INVISIBLE)	// 無効化されている
		return 1;

	sd->npc_shopid=id;
	if (type==0) {
		clif_buylist(sd,nd);
	} else {
		clif_selllist(sd);
	}
	return 0;
}

//npc_buylist for script-controlled shops.
static int npc_buylist_sub(struct map_session_data* sd, int n, unsigned short* item_list, struct npc_data* nd)
{
	char npc_ev[NAME_LENGTH*2+3];
	int i;
	int regkey = add_str("@bought_nameid");
	int regkey2 = add_str("@bought_quantity");
	snprintf(npc_ev, ARRAYLENGTH(npc_ev), "%s::OnBuyItem", nd->exname);
	for(i=0;i<n;i++){
		pc_setreg(sd,regkey+(i<<24),(int)item_list[i*2+1]);
		pc_setreg(sd,regkey2+(i<<24),(int)item_list[i*2]);
	}
	npc_event(sd, npc_ev, 0);
	return 0;
}

/*==========================================
 *
 *------------------------------------------*/
int npc_buylist(struct map_session_data* sd, int n, unsigned short* item_list)
{
	struct npc_data *nd;
	double z;
	int i,j,w,skill,itemamount=0,new_=0;

	nullpo_retr(3, sd);
	nullpo_retr(3, item_list);

	if ((nd = npc_checknear(sd,map_id2bl(sd->npc_shopid))) == NULL)
		return 3;

	if (nd->master_nd) //Script-based shops.
		return npc_buylist_sub(sd,n,item_list,nd->master_nd);

	if (nd->bl.subtype!=SHOP)
		return 3;

	for(i=0,w=0,z=0;i<n;i++) {
		for(j=0;nd->u.shop_item[j].nameid;j++) {
			if (nd->u.shop_item[j].nameid==item_list[i*2+1] || //Normal items
				itemdb_viewid(nd->u.shop_item[j].nameid)==item_list[i*2+1]) //item_avail replacement
				break;
		}
		if (nd->u.shop_item[j].nameid==0)
			return 3;
		
		if (!itemdb_isstackable(nd->u.shop_item[j].nameid) && item_list[i*2] > 1)
		{	//Exploit? You can't buy more than 1 of equipment types o.O
			ShowWarning("Player %s (%d:%d) sent a hexed packet trying to buy %d of nonstackable item %d!\n",
				sd->status.name, sd->status.account_id, sd->status.char_id, item_list[i*2], nd->u.shop_item[j].nameid);
			item_list[i*2] = 1;
		}
		if (itemdb_value_notdc(nd->u.shop_item[j].nameid))
			z+=(double)nd->u.shop_item[j].value * item_list[i*2];
		else
			z+=(double)pc_modifybuyvalue(sd,nd->u.shop_item[j].value) * item_list[i*2];
		itemamount+=item_list[i*2];

		switch(pc_checkadditem(sd,item_list[i*2+1],item_list[i*2])) {
		case ADDITEM_EXIST:
			break;
		case ADDITEM_NEW:
			new_++;
			break;
		case ADDITEM_OVERAMOUNT:
			return 2;
		}

		w+=itemdb_weight(item_list[i*2+1]) * item_list[i*2];
	}
	if (z > (double)sd->status.zeny)
		return 1;	// zeny不足
	if (w+sd->weight > sd->max_weight)
		return 2;	// 重量超過
	if (pc_inventoryblank(sd)<new_)
		return 3;	// 種類数超過

	//Logs (S)hopping Zeny [Lupus]
	if(log_config.zeny > 0 )
		log_zeny(sd, "S", sd, -(int)z);
	//Logs

	pc_payzeny(sd,(int)z);
	for(i=0;i<n;i++) {
		struct item item_tmp;

		memset(&item_tmp,0,sizeof(item_tmp));
		item_tmp.nameid = item_list[i*2+1];
		item_tmp.identify = 1;	// npc販売アイテムは鑑定済み

		pc_additem(sd,&item_tmp,item_list[i*2]);

		//Logs items, Bought in NPC (S)hop [Lupus]
		if(log_config.enable_logs&0x20)
			log_pick_pc(sd, "S", item_tmp.nameid, item_list[i*2], NULL);
		//Logs
	}

	//商人経験値
	if (battle_config.shop_exp > 0 && z > 0 && (skill = pc_checkskill(sd,MC_DISCOUNT)) > 0) {
		if (sd->status.skill[MC_DISCOUNT].flag != 0)
			skill = sd->status.skill[MC_DISCOUNT].flag - 2;
		if (skill > 0) {
			z = z * (double)skill * (double)battle_config.shop_exp/10000.;
			if (z < 1)
				z = 1;
			pc_gainexp(sd,NULL,0,(int)z);
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------*/
int npc_selllist(struct map_session_data* sd, int n, unsigned short* item_list)
{
	double z;
	int i,skill,itemamount=0;
	struct npc_data *nd;
	
	nullpo_retr(1, sd);
	nullpo_retr(1, item_list);

	if ((nd = npc_checknear(sd,map_id2bl(sd->npc_shopid))) == NULL)
		return 1;
	nd = nd->master_nd; //For OnSell triggers.

	for(i=0,z=0;i<n;i++) {
		int nameid, idx;
		short qty;
		idx = item_list[i*2]-2;
		qty = (short)item_list[i*2+1];
		
		if (idx <0 || idx >=MAX_INVENTORY || qty < 0)
			break;
		
		nameid=sd->status.inventory[idx].nameid;
		if (nameid == 0 || !sd->inventory_data[idx] ||
		   sd->status.inventory[idx].amount < qty)
			break;
		
		if (sd->inventory_data[idx]->flag.value_notoc)
			z+=(double)qty*sd->inventory_data[idx]->value_sell;
		else
			z+=(double)qty*pc_modifysellvalue(sd,sd->inventory_data[idx]->value_sell);

		if(sd->inventory_data[idx]->type == IT_PETEGG &&
			sd->status.inventory[idx].card[0] == CARD0_PET)
		{
			if(search_petDB_index(sd->status.inventory[idx].nameid, PET_EGG) >= 0)
				intif_delete_petdata(MakeDWord(sd->status.inventory[idx].card[1],sd->status.inventory[idx].card[2]));
		}

		if(log_config.enable_logs&0x20) //Logs items, Sold to NPC (S)hop [Lupus]
			log_pick_pc(sd, "S", nameid, -qty, &sd->status.inventory[idx]);

		if(nd) {
			pc_setreg(sd,add_str("@sold_nameid")+(i<<24),(int)sd->status.inventory[idx].nameid);
			pc_setreg(sd,add_str("@sold_quantity")+(i<<24),qty);
		}
		itemamount+=qty;
		pc_delitem(sd,idx,qty,0);
	}

	if (z > MAX_ZENY) z = MAX_ZENY;

	if(log_config.zeny) //Logs (S)hopping Zeny [Lupus]
		log_zeny(sd, "S", sd, (int)z);

	pc_getzeny(sd,(int)z);
	
	if (battle_config.shop_exp > 0 && z > 0 && (skill = pc_checkskill(sd,MC_OVERCHARGE)) > 0) {
		if (sd->status.skill[MC_OVERCHARGE].flag != 0)
			skill = sd->status.skill[MC_OVERCHARGE].flag - 2;
		if (skill > 0) {
			z = z * (double)skill * (double)battle_config.shop_exp/10000.;
			if (z < 1)
				z = 1;
			pc_gainexp(sd,NULL,0,(int)z);
		}
	}
		
	if(nd) {
		char npc_ev[NAME_LENGTH*2+3];
		snprintf(npc_ev, ARRAYLENGTH(npc_ev), "%s::OnSellItem", nd->exname);
		npc_event(sd, npc_ev, 0);
	}
	
	if (i<n) {
		//Error/Exploit... of some sort. If we return 1, the client will not mark
		//any item as deleted even though a few were sold. In such a case, we
		//have no recourse but to kick them out so their inventory will refresh
		//correctly on relog. [Skotlex]
		if (i) clif_setwaitclose(sd->fd);
		return 1;
	}
	return 0;
}

int npc_remove_map(struct npc_data* nd)
{
	int m,i;
	nullpo_retr(1, nd);

	if(nd->bl.prev == NULL || nd->bl.m < 0)
		return 1; //Not assigned to a map.
  	m = nd->bl.m;
	clif_clearunit_area(&nd->bl,2);
	//Remove corresponding NPC CELLs
	if (nd->bl.subtype == WARP) {
		int j, xs, ys, x, y;
		x = nd->bl.x;
		y = nd->bl.y;
		xs = nd->u.warp.xs;
		ys = nd->u.warp.ys;

		for (i = 0; i < ys; i++) {
			for (j = 0; j < xs; j++) {
				if (map_getcell(m, x-xs/2+j, y-ys/2+i, CELL_CHKNPC))
					map_setcell(m, x-xs/2+j, y-ys/2+i, CELL_CLRNPC);
			}
		}
	}
	map_delblock(&nd->bl);
	//Remove npc from map[].npc list. [Skotlex]
	for(i=0;i<map[m].npc_num && map[m].npc[i] != nd;i++);
	if (i >= map[m].npc_num) return 2; //failed to find it?

	map[m].npc_num--;
	memmove(&map[m].npc[i], &map[m].npc[i+1], (map[m].npc_num-i)*sizeof(map[m].npc[0]));
	return 0;
}

static int npc_unload_ev(DBKey key, void* data, va_list ap)
{
	struct event_data* ev = (struct event_data *)data;
	char* npcname = va_arg(ap, char *);

	if(strcmp(ev->nd->exname,npcname)==0){
		db_remove(ev_db, key);
		return 1;
	}
	return 0;
}

static int npc_unload_dup_sub(DBKey key, void* data, va_list ap)
{
	struct npc_data *nd = (struct npc_data *)data;
	int src_id;

	if(nd->bl.type!=BL_NPC || nd->bl.subtype != SCRIPT)
		return 0;

	src_id=va_arg(ap,int);
	if (nd->u.scr.src_id == src_id)
		npc_unload(nd);
	return 0;
}

//Removes all npcs that are duplicates of the passed one. [Skotlex]
void npc_unload_duplicates(struct npc_data* nd)
{
	map_foreachiddb(npc_unload_dup_sub,nd->bl.id);
}

int npc_unload(struct npc_data* nd)
{
	nullpo_ret(nd);

	npc_remove_map(nd);
	map_deliddb(&nd->bl);
	strdb_remove(npcname_db, nd->exname);

	if (nd->chat_id) // remove npc chatroom object and kick users
		chat_deletenpcchat(nd);

#ifdef PCRE_SUPPORT
	npc_chat_finalize(nd); // deallocate npc PCRE data structures
#endif

	if (nd->bl.subtype == SCRIPT) {
		ev_db->foreach(ev_db,npc_unload_ev,nd->exname); //Clean up all events related.
		if (nd->u.scr.timerid != -1) {
			struct TimerData *td = NULL;
			td = get_timer(nd->u.scr.timerid);
			if (td && td->data) 
				ers_free(timer_event_ers, (struct event_timer_data*)td->data);
			delete_timer(nd->u.scr.timerid, npc_timerevent);
		}
		if (nd->u.scr.timer_event)
			aFree(nd->u.scr.timer_event);
		if (nd->u.scr.src_id == 0) {
			if(nd->u.scr.script) {
				script_free_code(nd->u.scr.script);
				nd->u.scr.script = NULL;
			}
			if (nd->u.scr.label_list) {
				aFree(nd->u.scr.label_list);
				nd->u.scr.label_list = NULL;
				nd->u.scr.label_list_num = 0;
			}
		}
	}
	script_stop_sleeptimers(nd->bl.id);
	aFree(nd);

	return 0;
}

//
// NPC Source Files
//

/// Clears the npc source file list
static void npc_clearsrcfile(void)
{
	struct npc_src_list* file = npc_src_files;
	struct npc_src_list* file_tofree;

	while( file != NULL )
	{
		file_tofree = file;
		file = file->next;
		aFree(file_tofree);
	}
	npc_src_files = NULL;
}

/// Adds a npc source file (or removes all)
void npc_addsrcfile(const char* name)
{
	struct npc_src_list* file;
	struct npc_src_list* file_prev = NULL;

	if( strcmpi(name, "clear") == 0 )
	{
		npc_clearsrcfile();
		return;
	}

	// prevent multiple insert of source files
	file = npc_src_files;
	while( file != NULL )
	{
		if( strcmp(name, file->name) == 0 )
			return;// found the file, no need to insert it again
		file_prev = file;
		file = file->next;
	}

	file = (struct npc_src_list*)aMalloc(sizeof(struct npc_src_list) + strlen(name));
	file->next = NULL;
	strncpy(file->name, name, strlen(name) + 1);
	if( file_prev == NULL )
		npc_src_files = file;
	else
		file_prev->next = file;
}

/// Removes a npc source file (or all)
void npc_delsrcfile(const char* name)
{
	struct npc_src_list* file = npc_src_files;
	struct npc_src_list* file_prev = NULL;

	if( strcmpi(name, "all") == 0 )
	{
		npc_clearsrcfile();
		return;
	}

	while( file != NULL )
	{
		if( strcmp(file->name, name) == 0 )
		{
			if( npc_src_files == file )
				npc_src_files = file->next;
			else
				file_prev->next = file->next;
			aFree(file);
			break;
		}
		file_prev = file;
		file = file->next;
	}
}

/// Parses and sets the name and exname of a npc.
/// Assumes that m, x and y are already set in nd.
static void npc_parsename(struct npc_data* nd, const char* name, const char* start, const char* buffer, const char* filepath)
{
	const char* p;
	struct npc_data* dnd;
	char newname[NAME_LENGTH];

	// parse name
	p = strstr(name,"::");
	if( p )
	{// <Display name>::<Unique name>
		size_t len = p-name;
		if( len > NAME_LENGTH )
		{
			ShowWarning("npc_parsename: Display name of '%s' is too long (len=%u) in file '%s', line'%d'. Truncating to %u characters.\n", name, (unsigned int)len, filepath, strline(buffer,start-buffer), NAME_LENGTH);
			safestrncpy(nd->name, name, sizeof(nd->name));
		}
		else
		{
			memcpy(nd->name, name, len);
			memset(nd->name+len, 0, sizeof(nd->name)-len);
		}
		len = strlen(p+2);
		if( len > NAME_LENGTH )
			ShowWarning("npc_parsename: Unique name of '%s' is too long (len=%u) in file '%s', line'%d'. Truncating to %u characters.\n", name, (unsigned int)len, filepath, strline(buffer,start-buffer), NAME_LENGTH);
		safestrncpy(nd->exname, p+2, sizeof(nd->exname));
	}
	else
	{// <Display name>
		size_t len = strlen(name);
		if( len > NAME_LENGTH )
			ShowWarning("npc_parsename: Name '%s' is too long (len=%u) in file '%s', line'%d'. Truncating to %u characters.\n", name, (unsigned int)len, filepath, strline(buffer,start-buffer), NAME_LENGTH);
		safestrncpy(nd->name, name, sizeof(nd->name));
		safestrncpy(nd->exname, name, sizeof(nd->exname));
	}

	if( *nd->exname == '\0' || strstr(nd->exname,"::") != NULL )
	{// invalid
		snprintf(newname, ARRAYLENGTH(newname), "0_%d_%d_%d", nd->bl.m, nd->bl.x, nd->bl.y);
		ShowWarning("npc_parsename: Invalid unique name in file '%s', line'%d'. Renaming '%s' to '%s'.\n", filepath, strline(buffer,start-buffer), nd->exname, newname);
		safestrncpy(nd->exname, newname, sizeof(nd->exname));
	}

	if( *nd->exname == '\0' || (dnd=npc_name2id(nd->exname)) != NULL )
	{// duplicate unique name, generate new one
		char this_mapname[32];
		char other_mapname[32];
		int i = 0;
		do
		{
			++i;
			snprintf(newname, ARRAYLENGTH(newname), "%d_%d_%d_%d", i, nd->bl.m, nd->bl.x, nd->bl.y);
		}
		while( npc_name2id(newname) != NULL );

		strcpy(this_mapname, (nd->bl.m==-1?"(not on a map)":mapindex_id2name(map[nd->bl.m].index)));
		strcpy(other_mapname, (dnd->bl.m==-1?"(not on a map)":mapindex_id2name(map[dnd->bl.m].index)));

		//Commented out by ME-- L0ne_W0lf, and maybe one day we'll uncomment it again
		//if and when I decide to/get all the warnings nad debug messages this horrible
		//BROKEN thing ever shows.
		//By the way, this_map and other_map are both WRONG. They are retuirning invalid results.
		//ShowWarning("npc_parsename: Duplicate unique name in file '%s', line'%d'. Renaming '%s' to '%s'.\n", filepath, strline(buffer,start-buffer), nd->exname, newname);
		//ShowDebug("this npc:\n   display name '%s'\n   unique name '%s'\n   map=%s, x=%d, y=%d\n", nd->name, nd->exname, this_mapname, nd->bl.x, nd->bl.y);
		//ShowDebug("other npc:\n   display name '%s'\n   unique name '%s'\n   map=%s, x=%d, y=%d\n", dnd->name, dnd->exname, other_mapname, dnd->bl.x, dnd->bl.y);
		safestrncpy(nd->exname, newname, sizeof(nd->exname));
	}
}

/// Parses a warp npc.
const char* npc_parse_warp(char* w1, char* w2, char* w3, char* w4, const char* start, const char* buffer, const char* filepath)
{
	int x, y, xs, ys, to_x, to_y, m;
	unsigned short i;
	char mapname[32], to_mapname[32];
	struct npc_data *nd;

	// w1=<from map name>,<fromX>,<fromY>,<facing>
	// w4=<spanx>,<spany>,<to map name>,<toX>,<toY>
	if( sscanf(w1, "%31[^,],%d,%d", mapname, &x, &y) != 3
	||	sscanf(w4, "%d,%d,%31[^,],%d,%d", &xs, &ys, to_mapname, &to_x, &to_y) != 5 )
	{
		ShowError("npc_parse_warp: Invalid warp definition in file '%s', line '%d'.\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,start-buffer), w1, w2, w3, w4);
		return strchr(start,'\n');// skip and continue
	}

	m = map_mapname2mapid(mapname);
	i = mapindex_name2id(to_mapname);
	if( i == 0 )
	{
		ShowError("npc_parse_warp: Unknown destination map in file '%s', line '%d' : %s\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,start-buffer), to_mapname, w1, w2, w3, w4);
		return strchr(start,'\n');// skip and continue
	}

	CREATE(nd, struct npc_data, 1);

	nd->bl.id = npc_get_new_npc_id();
	nd->n = map_addnpc(m, nd);
	nd->bl.prev = nd->bl.next = NULL;
	nd->bl.m = m;
	nd->bl.x = x;
	nd->bl.y = y;
	npc_parsename(nd, w3, start, buffer, filepath);

	if (!battle_config.warp_point_debug)
		nd->class_ = WARP_CLASS;
	else
		nd->class_ = WARP_DEBUG_CLASS;
	nd->speed = 200;

	nd->u.warp.mapindex = i;
	xs += 2;
	ys += 2;
	nd->u.warp.x = to_x;
	nd->u.warp.y = to_y;
	nd->u.warp.xs = xs;
	nd->u.warp.ys = ys;
	npc_warp++;
	nd->bl.type = BL_NPC;
	nd->bl.subtype = WARP;
	npc_setcells(nd);
	map_addblock(&nd->bl);
	status_set_viewdata(&nd->bl, nd->class_);
	status_change_init(&nd->bl);
	unit_dataset(&nd->bl);
	clif_spawn(&nd->bl);
	strdb_put(npcname_db, nd->exname, nd);

	return strchr(start,'\n');// continue
}

/// Parses a shop npc.
static const char* npc_parse_shop(char* w1, char* w2, char* w3, char* w4, const char* start, const char* buffer, const char* filepath)
{
	#define MAX_SHOPITEM 100
	struct npc_item_list items[MAX_SHOPITEM];
	char *p;
	int x, y, dir, m, i;
	struct npc_data *nd;

	if( strcmp(w1,"-") == 0 )
	{// 'floating' shop?
		x = 0; y = 0; dir = 0; m = -1;
	}
	else
	{// w1=<map name>,<x>,<y>,<facing>
		char mapname[32];
		if( sscanf(w1, "%31[^,],%d,%d,%d", mapname, &x, &y, &dir) != 4
		||	strchr(w4, ',') == NULL )
		{
			ShowError("npc_parse_shop: Invalid shop definition in file '%s', line '%d'.\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,start-buffer), w1, w2, w3, w4);
			return strchr(start,'\n');// skip and continue
		}
		m = map_mapname2mapid(mapname);
	}

	p = strchr(w4,',');
	for( i = 0; i < ARRAYLENGTH(items) && p; ++i )
	{
		int nameid, value;
		struct item_data* id;
		if( sscanf(p, ",%d:%d", &nameid, &value) != 2 )
		{
			ShowError("npc_parse_shop: Invalid item definition in file '%s', line '%d'. Ignoring the rest of the line...\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,start-buffer), w1, w2, w3, w4);
			break;
		}
		id = itemdb_search(nameid);
		if( value < 0 )
			value = id->value_buy;
		if( value*0.75 < id->value_sell*1.24 )
		{// Expoit possible: you can buy and sell back with profit
			ShowWarning("npc_parse_shop: Item %s [%d] discounted buying price (%d->%d) is less than overcharged selling price (%d->%d) at file '%s', line '%d'.\n",
				id->name, nameid, value, (int)(value*0.75), id->value_sell, (int)(id->value_sell*1.24), filepath, strline(buffer,start-buffer));
		}
		//for logs filters, atcommands and iteminfo script command
		if( id->maxchance <= 0 )
			id->maxchance = 10000; //10000 (100% drop chance)would show that the item's sold in NPC Shop

		items[i].nameid = nameid;
		items[i].value = value;
		p = strchr(p+1,',');
	}
	if( i == 0 )
	{
		ShowWarning("npc_parse_shop: Ignoring empty shop in file '%s', line '%d'.\n", filepath, strline(buffer,start-buffer));
		return strchr(start,'\n');// continue
	}

	nd = (struct npc_data *) aCalloc (1, sizeof(struct npc_data) + sizeof(nd->u.shop_item[0])*(i));
	memcpy(&nd->u.shop_item, items, sizeof(struct npc_item_list)*i);
	nd->u.shop_item[i].nameid = 0;
	nd->bl.prev = nd->bl.next = NULL;
	nd->bl.m = m;
	nd->bl.x = x;
	nd->bl.y = y;
	nd->bl.id = npc_get_new_npc_id();
	npc_parsename(nd, w3, start, buffer, filepath);
	nd->class_ = m==-1?-1:atoi(w4);
	nd->speed = 200;

	++npc_shop;
	nd->bl.type = BL_NPC;
	nd->bl.subtype = SHOP;
	if( m >= 0 )
	{// normal shop npc
		nd->n = map_addnpc(m,nd);
		map_addblock(&nd->bl);
		status_set_viewdata(&nd->bl, nd->class_);
		status_change_init(&nd->bl);
		unit_dataset(&nd->bl);
		nd->ud.dir = dir;
		clif_spawn(&nd->bl);
	} else
	{// 'floating' shop?
		map_addiddb(&nd->bl);
	}
	strdb_put(npcname_db, nd->exname, nd);

	return strchr(start,'\n');// continue
}

/*==========================================
 * NPCのラベルデータコンバート
 *------------------------------------------*/
int npc_convertlabel_db(DBKey key, void* data, va_list ap)
{
	const char* lname = (const char*)key.str;
	int lpos = (int)data;
	struct npc_label_list** label_list;
	int* label_list_num;
	const char* filepath;
	struct npc_label_list* label;
	const char *p;
	int len;

	nullpo_retr(0, ap);
	nullpo_retr(0, label_list = va_arg(ap,struct npc_label_list**));
	nullpo_retr(0, label_list_num = va_arg(ap,int*));
	nullpo_retr(0, filepath = va_arg(ap,const char*));

	if( *label_list == NULL )
	{
		*label_list = (struct npc_label_list *) aCallocA (1, sizeof(struct npc_label_list));
		*label_list_num = 0;
	} else
		*label_list = (struct npc_label_list *) aRealloc (*label_list, sizeof(struct npc_label_list)*(*label_list_num+1));
	label = *label_list+*label_list_num;

	// In case of labels not terminated with ':', for user defined function support
	p = lname;
	while( ISALNUM(*p) || *p == '_' )
		++p;
	len = p-lname;

	// here we check if the label fit into the buffer
	if( len > 23 )
	{
		ShowError("npc_parse_script: label name longer than 23 chars! '%s'\n (%s)", lname, filepath);
		exit(EXIT_FAILURE);
	}
	safestrncpy(label->name, lname, sizeof(label->name));
	label->pos = lpos;
	++(*label_list_num);

	return 0;
}

// Skip the contents of a script.
static const char* npc_skip_script(const char* start, const char* buffer, const char* filepath)
{
	const char* p;
	int curly_count;

	if( start == NULL )
		return NULL;// nothing to skip

	// initial bracket (assumes the previous part is ok)
	p = strchr(start,'{');
	if( p == NULL )
	{
		ShowError("npc_skip_script: Missing left curly in file '%s', line'%d'.", filepath, strline(buffer,start-buffer));
		return NULL;// can't continue
	}

	// skip everything
	for( curly_count = 1; curly_count > 0 ; )
	{
		p = skip_space(p+1) ;
		if( *p == '}' )
		{// right curly
			--curly_count;
		}
		else if( *p == '{' )
		{// left curly
			++curly_count;
		}
		else if( *p == '"' )
		{// string
			for( ++p; *p != '"' ; ++p )
			{
				if( *p == '\\' && (unsigned char)p[-1] <= 0x7e )
					++p;// escape sequence (not part of a multibyte character)
				else if( *p == '\0' )
				{
					script_error(buffer, filepath, 0, "Unexpected end of string.", p);
					return NULL;// can't continue
				}
				else if( *p == '\n' )
				{
					script_error(buffer, filepath, 0, "Unexpected newline at string.", p);
					return NULL;// can't continue
				}
			}
		}
		else if( *p == '\0' )
		{// end of buffer
			ShowError("Missing %d right curlys at file '%s', line '%d'.\n", curly_count, filepath, strline(buffer,p-buffer));
			return NULL;// can't continue
		}
	}

	return p+1;// return after the last '}'
}

static const char* npc_parse_script(char* w1, char* w2, char* w3, char* w4, const char* start, const char* buffer, const char* filepath)
{
	int x, y, dir = 0, m, xs = 0, ys = 0, class_ = 0;	// [Valaris] thanks to fov
	char mapname[32];
	struct script_code *script;
	int i;
	const char* end;

	struct npc_label_list* label_list;
	int label_list_num;
	int src_id;
	struct npc_data* nd;
	struct npc_data* dnd;

	if( strcmp(w1, "-") == 0 )
	{// floating npc
		x = 0;
		y = 0;
		m = -1;
	}
	else
	{// npc in a map
		if( sscanf(w1, "%31[^,],%d,%d,%d", mapname, &x, &y, &dir) != 4
		||	(strcasecmp(w2, "script") == 0 && strchr(w4,',') == NULL) )
		{
			ShowError("npc_parse_script: Unkown format for a script in file '%s', line '%d'. Skipping the rest of file...\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,start-buffer), w1, w2, w3, w4);
			return NULL;// unknown format, don't continue
		}
		m = map_mapname2mapid(mapname);
	}

	if( strcmp(w2, "script") == 0 )
	{// parsing script with curly
		const char* real_start;

		end = npc_skip_script(start, buffer, filepath);
		if( end == NULL )
			return NULL;// (simple) parse error, don't continue

		real_start = strchr(start,'{');
		script = parse_script(real_start, filepath, strline(buffer,real_start-buffer), SCRIPT_USE_LABEL_DB);
		label_list = NULL;
		label_list_num = 0;
		src_id = 0;
		if( script )
		{
			DB label_db = script_get_label_db();
			label_db->foreach(label_db, npc_convertlabel_db, &label_list, &label_list_num, filepath);
			label_db->clear(label_db, NULL); // not needed anymore, so clear the db
		}
	}
	else
	{// duplicate npc
		char srcname[128];

		end = strchr(start,'\n');
		if( sscanf(w2,"duplicate(%127[^)])",srcname) != 1 )
		{
			ShowError("npc_parse_script: bad duplicate name in file '%s', line '%d' : %s\n", filepath, strline(buffer,start-buffer), w2);
			return strchr(start, '\n');// next line, try to continue
		}
		dnd = npc_name2id(srcname);
		if( dnd == NULL) {
			ShowError("npc_parse_script: original npc not found for duplicate in file '%s', line '%d' : %s\n", filepath, strline(buffer,start-buffer), srcname);
			return strchr(start, '\n');// next line, continue
		}
		script = dnd->u.scr.script;
		label_list = dnd->u.scr.label_list;// TODO duplicate this?
		label_list_num = dnd->u.scr.label_list_num;
		src_id = dnd->bl.id;
	}

	CREATE(nd, struct npc_data, 1);

	if( sscanf(w4, "%d,%d,%d", &class_, &xs, &ys) == 3 )
	{// OnTouch area defined
		if (xs >= 0) xs = xs * 2 + 1;
		if (ys >= 0) ys = ys * 2 + 1;
		nd->u.scr.xs = xs;
		nd->u.scr.ys = ys;
	}
	else
	{
		class_ = atoi(w4);
		nd->u.scr.xs = 0;
		nd->u.scr.ys = 0;
	}

	nd->bl.prev = nd->bl.next = NULL;
	nd->bl.m = m;
	nd->bl.x = x;
	nd->bl.y = y;
	npc_parsename(nd, w3, start, buffer, filepath);
	nd->bl.id = npc_get_new_npc_id();
	nd->class_ = class_;
	nd->speed = 200;
	nd->u.scr.script = script;
	nd->u.scr.src_id = src_id;
	nd->u.scr.label_list = label_list;
	nd->u.scr.label_list_num = label_list_num;

	++npc_script;
	nd->bl.type = BL_NPC;
	nd->bl.subtype = SCRIPT;

	if( m >= 0 )
	{
		nd->n = map_addnpc(m, nd);
		status_change_init(&nd->bl);
		unit_dataset(&nd->bl);
		nd->ud.dir = dir;
		npc_setcells(nd);
		map_addblock(&nd->bl);
		if( class_ >= 0 )
		{
			status_set_viewdata(&nd->bl, nd->class_);
			clif_spawn(&nd->bl);
		}
	}
	else
	{
		// we skip map_addnpc, but still add it to the list of ID's
		map_addiddb(&nd->bl);
	}
	strdb_put(npcname_db, nd->exname, nd);

	//-----------------------------------------
	// イベント用ラベルデータのエクスポート
	for (i = 0; i < nd->u.scr.label_list_num; i++)
	{
		char* lname = nd->u.scr.label_list[i].name;
		int pos = nd->u.scr.label_list[i].pos;

		if ((lname[0] == 'O' || lname[0] == 'o') && (lname[1] == 'N' || lname[1] == 'n'))
		{
			struct event_data* ev;
			char buf[NAME_LENGTH*2+3]; // 24 for npc name + 24 for label + 2 for a "::" and 1 for EOS
			snprintf(buf, ARRAYLENGTH(buf), "%s::%s", nd->exname, lname);

			// generate the data and insert it
			CREATE(ev, struct event_data, 1);
			ev->nd = nd;
			ev->pos = pos;
			if( strdb_put(ev_db, buf, ev) != NULL )// There was already another event of the same name?
				ShowWarning("npc_parse_script : duplicate event %s (%s)\n", buf, filepath);
		}
	}

	//-----------------------------------------
	// ラベルデータからタイマーイベント取り込み
	for (i = 0; i < nd->u.scr.label_list_num; i++){
		int t = 0, k = 0;
		char *lname = nd->u.scr.label_list[i].name;
		int pos = nd->u.scr.label_list[i].pos;
		if (sscanf(lname, "OnTimer%d%n", &t, &k) == 1 && lname[k] == '\0') {
			// タイマーイベント
			struct npc_timerevent_list *te = nd->u.scr.timer_event;
			int j, k = nd->u.scr.timeramount;
			if (te == NULL)
				te = (struct npc_timerevent_list *)aMallocA(sizeof(struct npc_timerevent_list));
			else
				te = (struct npc_timerevent_list *)aRealloc( te, sizeof(struct npc_timerevent_list) * (k+1) );
			for (j = 0; j < k; j++){
				if (te[j].timer > t){
					memmove(te+j+1, te+j, sizeof(struct npc_timerevent_list)*(k-j));
					break;
				}
			}
			te[j].timer = t;
			te[j].pos = pos;
			nd->u.scr.timer_event = te;
			nd->u.scr.timeramount++;
		}
	}
	nd->u.scr.timerid = -1;

	return end;
}

void npc_setcells(struct npc_data* nd)
{
	int m = nd->bl.m, x = nd->bl.x, y = nd->bl.y, xs, ys;
	int i,j;

	if (nd->bl.subtype == WARP) {
		xs = nd->u.warp.xs;
		ys = nd->u.warp.ys;
	} else {
		xs = nd->u.scr.xs;
		ys = nd->u.scr.ys;
	}

	if (m < 0 || xs < 1 || ys < 1)
		return;

	for (i = 0; i < ys; i++) {
		for (j = 0; j < xs; j++) {
			if (map_getcell(m, x-xs/2+j, y-ys/2+i, CELL_CHKNOPASS))
				continue;
			map_setcell(m, x-xs/2+j, y-ys/2+i, CELL_SETNPC);
		}
	}
}

int npc_unsetcells_sub(struct block_list* bl, va_list ap)
{
	struct npc_data *nd = (struct npc_data*)bl;
	int id =  va_arg(ap,int);
	if (nd->bl.id == id) return 0;
	npc_setcells(nd);
	return 1;
}

void npc_unsetcells(struct npc_data* nd)
{
	int m = nd->bl.m, x = nd->bl.x, y = nd->bl.y, xs, ys;
	int i,j, x0, x1, y0, y1;

	if (nd->bl.subtype == WARP) {
		xs = nd->u.warp.xs;
		ys = nd->u.warp.ys;
	} else {
		xs = nd->u.scr.xs;
		ys = nd->u.scr.ys;
	}

	if (m < 0 || xs < 1 || ys < 1)
		return;

	//Locate max range on which we can localte npce cells
	for(x0 = x-xs/2; x0 > 0 && map_getcell(m, x0, y, CELL_CHKNPC); x0--);
	for(x1 = x+xs/2-1; x1 < map[m].xs && map_getcell(m, x1, y, CELL_CHKNPC); x1++);
	for(y0 = y-ys/2; y0 > 0 && map_getcell(m, x, y0, CELL_CHKNPC); y0--);
	for(y1 = y+ys/2-1; y1 < map[m].xs && map_getcell(m, x, y1, CELL_CHKNPC); y1++);

	for (i = 0; i < ys; i++) {
		for (j = 0; j < xs; j++)
			map_setcell(m, x-xs/2+j, y-ys/2+i, CELL_CLRNPC);
	}
	//Reset NPC cells for other nearby npcs.
	map_foreachinarea( npc_unsetcells_sub, m, x0, y0, x1, y1, BL_NPC, nd->bl.id);
}

void npc_movenpc(struct npc_data* nd, int x, int y)
{
	const int m = nd->bl.m;
	if (m < 0 || nd->bl.prev == NULL) return;	//Not on a map.

	x = cap_value(x, 0, map[m].xs-1);
	x = cap_value(y, 0, map[m].ys-1);

	npc_unsetcells(nd);
	map_foreachinrange(clif_outsight, &nd->bl, AREA_SIZE, BL_PC, &nd->bl);
	map_moveblock(&nd->bl, x, y, gettick());
	map_foreachinrange(clif_insight, &nd->bl, AREA_SIZE, BL_PC, &nd->bl);
	npc_setcells(nd);
}

int npc_changename(const char* name, const char* newname, short look)
{
	struct npc_data* nd = (struct npc_data *) strdb_remove(npcname_db, name);
	if (nd == NULL)
		return 0;
	npc_enable(name, 0);
	strcpy(nd->name, newname);
	nd->class_ = look;
	npc_enable(newname, 1);
	return 0;
}

/// Parses a function.
/// function%TAB%script%TAB%<function name>%TAB%{<code>}
static const char* npc_parse_function(char* w1, char* w2, char* w3, char* w4, const char* start, const char* buffer, const char* filepath)
{
	struct dbt *func_db;
	struct script_code *script;
	struct script_code *oldscript;
	const char* end;

	end = npc_skip_script(start,buffer,filepath);
	if( end == NULL )
		return NULL;// (simple) parse error, don't continue

	start = strchr(start,'{');
	script = parse_script(start, filepath, strline(buffer,start-buffer), SCRIPT_RETURN_EMPTY_SCRIPT);
	if( script == NULL )// parse error, continue
		return end;

	func_db = script_get_userfunc_db();
	oldscript = (struct script_code*)strdb_put(func_db, w3, script);
	if( oldscript != NULL )
	{
		ShowInfo("npc_parse_function: Overwriting user function [%s] (%s:%d)\n", w3, filepath, strline(buffer,start-buffer));
		script_free_vars(&oldscript->script_vars);
		aFree(oldscript->script_buf);
		aFree(oldscript);
	}

	return end;
}


/*==========================================
 * Parse Mob 1 - Parse mob list into each map
 * Parse Mob 2 - Actually Spawns Mob
 * [Wizputer]
 * If cached =1, it is a dynamic cached mob
 * index points to the index in the mob_list of the map_data cache.
 * -1 indicates that it is not stored on the map.
 *------------------------------------------*/
int npc_parse_mob2(struct spawn_data* mob, int index)
{
	int i;
	struct mob_data *md;

	for (i = mob->skip; i < mob->num; i++) {
		md = mob_spawn_dataset(mob);
		md->spawn = mob;
		md->spawn_n = index;
		md->special_state.cached = (index>=0);	//If mob is cached on map, it is dynamically removed
		mob_spawn(md);
	}
	mob->skip = 0;
	return 1;
}

static const char* npc_parse_mob(char* w1, char* w2, char* w3, char* w4, const char* start, const char* buffer, const char* filepath)
{
	int level, num, class_, mode, x,y,xs,ys;
	char mapname[32];
	char mobname[128];
	struct spawn_data mob, *data;
	struct mob_db* db;

	memset(&mob, 0, sizeof(struct spawn_data));

	// w1=<map name>,<x>,<y>,<xs>,<ys>
	// w4=<mob id>,<amount>,<delay1>,<delay2>,<event>
	if( sscanf(w1, "%31[^,],%d,%d,%d,%d", mapname, &x, &y, &xs, &ys) < 3
	||	sscanf(w4, "%d,%d,%u,%u,%127[^\t\r\n]", &class_, &num, &mob.delay1, &mob.delay2, mob.eventname) < 2 )
	{
		ShowError("npc_parse_mob: Invalid mob definition in file '%s', line '%d'.\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,start-buffer), w1, w2, w3, w4);
		return strchr(start,'\n');// skip and continue
	}
	if( mapindex_name2id(mapname) == 0 )
	{
		ShowError("npc_parse_mob: Unknown map '%s' in file '%s', line '%d'.\n", mapname, filepath, strline(buffer,start-buffer));
		return strchr(start,'\n');// skip and continue
	}
	mode =  map_mapname2mapid(mapname);
	if( mode < 0 )//Not loaded on this map-server instance.
		return strchr(start,'\n');// skip and continue
	mob.m = (unsigned short)mode;

	if( x < 0 || x >= map[mob.m].xs || y < 0 || y >= map[mob.m].ys )
	{
		ShowError("npc_parse_mob: Spawn coordinates out of range: %s (%d,%d), map size is (%d,%d) - %s %s (file '%s', line '%d').\n", map[mob.m].name, x, y, (map[mob.m].xs-1), (map[mob.m].ys-1), w1, w3, filepath, strline(buffer,start-buffer));
		return strchr(start,'\n');// skip and continue
	}

	// check monster ID if exists!
	if( mobdb_checkid(class_) == 0 )
	{
		ShowError("npc_parse_mob: Unknown mob ID : %s %s (file '%s', line '%d').\n", w3, w4, filepath, strline(buffer,start-buffer));
		return strchr(start,'\n');// skip and continue
	}

	if( num < 1 || num > 1000 )
	{
		ShowError("npc_parse_mob: Invalid number of monsters (must be inside the range [1,1000]) : %s %s (file '%s', line '%d').\n", w3, w4, filepath, strline(buffer,start-buffer));
		return strchr(start,'\n');// skip and continue
	}

	//Fix for previously wrong interpretation of the delays
	mob.delay2 = mob.delay1;
	mob.delay1 = 0;

	mob.num = (unsigned short)num;
	mob.class_ = (short) class_;
	mob.x = (unsigned short)x;
	mob.y = (unsigned short)y;
	mob.xs = (signed short)xs;
	mob.ys = (signed short)ys;

	if (mob.num > 1 && battle_config.mob_count_rate != 100) {
		if ((mob.num = mob.num * battle_config.mob_count_rate / 100) < 1)
			mob.num = 1;
	}

	if (battle_config.force_random_spawn || (mob.x == 0 && mob.y == 0))
	{	//Force a random spawn anywhere on the map.
		mob.x = mob.y = 0;
		mob.xs = mob.ys = -1;
	}

	db = mob_db(class_);
	//Apply the spawn delay fix [Skotlex]
	mode = db->status.mode;
	if (mode & MD_BOSS) {	//Bosses
		if (battle_config.boss_spawn_delay != 100)
		{	// Divide by 100 first to prevent overflows
			//(precision loss is minimal as duration is in ms already)
			mob.delay1 = mob.delay1/100*battle_config.boss_spawn_delay;
			mob.delay2 = mob.delay2/100*battle_config.boss_spawn_delay;
		}
	} else if (mode&MD_PLANT) {	//Plants
		if (battle_config.plant_spawn_delay != 100)
		{
			mob.delay1 = mob.delay1/100*battle_config.plant_spawn_delay;
			mob.delay2 = mob.delay2/100*battle_config.plant_spawn_delay;
		}
	} else if (battle_config.mob_spawn_delay != 100)
	{	//Normal mobs
		mob.delay1 = mob.delay1/100*battle_config.mob_spawn_delay;
		mob.delay2 = mob.delay2/100*battle_config.mob_spawn_delay;
	}

	// parse MOB_NAME,[MOB LEVEL]
	if (sscanf(w3, "%127[^,],%d", mobname, &level) > 1)
		mob.level = level;

	if(mob.delay1>0xfffffff || mob.delay2>0xfffffff) {
		ShowError("npc_parse_mob: wrong monsters spawn delays : %s %s (file '%s', line '%d').\n", w3, w4, filepath, strline(buffer,start-buffer));
		return strchr(start,'\n');// skip and continue
	}

	//Use db names instead of the spawn file ones.
	if(battle_config.override_mob_names==1)
		strcpy(mob.name,"--en--");
	else if (battle_config.override_mob_names==2)
		strcpy(mob.name,"--ja--");
	else
		strncpy(mob.name, mobname, NAME_LENGTH-1);

	if( !mob_parse_dataset(&mob) ) //Verify dataset.
	{
		ShowError("npc_parse_mob: Invalid dataset : %s %s (file '%s', line '%d').\n", w3, w4, filepath, strline(buffer,start-buffer));
		return strchr(start,'\n');// skip and continue
	}

	for(x=0; x < ARRAYLENGTH(db->spawn); x++)
	{
		if (map[mob.m].index == db->spawn[x].mapindex)
		{	//Update total
			db->spawn[x].qty += mob.num;
			//Re-sort list
			for (y = x; y>0 && db->spawn[y-1].qty < db->spawn[x].qty; y--);
			if (y != x)
			{
				xs = db->spawn[x].mapindex;
				ys = db->spawn[x].qty;
				memmove(&db->spawn[y+1], &db->spawn[y], (x-y)*sizeof(db->spawn[0]));
				db->spawn[y].mapindex = xs;
				db->spawn[y].qty = ys;
			}
			break;
		}
		if (mob.num > db->spawn[x].qty)
		{	//Insert into list
			memmove(&db->spawn[x+1], &db->spawn[x], sizeof(db->spawn) -(x+1)*sizeof(db->spawn[0]));
			db->spawn[x].mapindex = map[mob.m].index;
			db->spawn[x].qty = mob.num;
			break;
		}
	}

	//Now that all has been validated. We allocate the actual memory
	//that the re-spawn data will use.
	data = aMalloc(sizeof(struct spawn_data));
	memcpy(data, &mob, sizeof(struct spawn_data));
	
	if( !battle_config.dynamic_mobs || mob.delay1 || mob.delay2 ) {
		npc_parse_mob2(data,-1);
		npc_delay_mob += mob.num;
	} else {
		int index = map_addmobtolist(data->m, data);
		if( index >= 0 ) {
			// check if target map has players
			// (usually shouldn't occur when map server is just starting,
			// but not the case when we do @reloadscript
			if (map[mob.m].users > 0)
				npc_parse_mob2(data,index);
			npc_cache_mob += mob.num;
		} else {
			// mobcache is full
			// create them as delayed with one second
			mob.delay1 = 1000;
			mob.delay2 = 1000;
			npc_parse_mob2(data,-1);
			npc_delay_mob += mob.num;
		}
	}

	npc_mob++;

	return strchr(start,'\n');// continue
}

/*==========================================
 * マップフラグ行の解析
 *------------------------------------------*/
static const char* npc_parse_mapflag(char* w1, char* w2, char* w3, char* w4, const char* start, const char* buffer, const char* filepath)
{
	int m;
	char mapname[32];
	int state = 1;

	// w1=<mapname>
	if( sscanf(w1, "%31[^,]", mapname) != 1 )
	{
		ShowError("npc_parse_mapflag: Invalid mapflag definition in file '%s', line '%d'.\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,start-buffer), w1, w2, w3, w4);
		return strchr(start,'\n');// skip and continue
	}
	m = map_mapname2mapid(mapname);
	if( m < 0 )
	{
		ShowWarning("npc_parse_mapflag: Unknown map in file '%s', line '%d' : %s\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", mapname, filepath, strline(buffer,start-buffer), w1, w2, w3, w4);
		return strchr(start,'\n');// skip and continue
	}

	if (w4 && !strcmpi(w4, "off"))
		state = 0;	//Disable mapflag rather than enable it. [Skotlex]
	
	if (!strcmpi(w3, "nosave")) {
		char savemap[32];
		int savex, savey;
		if (state == 0)
			; //Map flag disabled.
		else if (!strcmpi(w4, "SavePoint")) {
			map[m].save.map = 0;
			map[m].save.x = -1;
			map[m].save.y = -1;
		} else if (sscanf(w4, "%31[^,],%d,%d", savemap, &savex, &savey) == 3) {
			map[m].save.map = mapindex_name2id(savemap);
			map[m].save.x = savex;
			map[m].save.y = savey;
			if (!map[m].save.map) {
				ShowWarning("npc_parse_mapflag: Specified save point map '%s' for mapflag 'nosave' not found (file '%s', line '%d'), using 'SavePoint'.\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", savemap, filepath, strline(buffer,start-buffer), w1, w2, w3, w4);
				map[m].save.x = -1;
				map[m].save.y = -1;
			}
		}
		map[m].flag.nosave = state;
	}
	else if (!strcmpi(w3,"nomemo"))
		map[m].flag.nomemo=state;
	else if (!strcmpi(w3,"noteleport"))
		map[m].flag.noteleport=state;
	else if (!strcmpi(w3,"nowarp"))
		map[m].flag.nowarp=state;
	else if (!strcmpi(w3,"nowarpto"))
		map[m].flag.nowarpto=state;
	else if (!strcmpi(w3,"noreturn"))
		map[m].flag.noreturn=state;
	else if (!strcmpi(w3,"monster_noteleport"))
		map[m].flag.monster_noteleport=state;
	else if (!strcmpi(w3,"nobranch"))
		map[m].flag.nobranch=state;
	else if (!strcmpi(w3,"nopenalty")) {
		map[m].flag.noexppenalty=state;
		map[m].flag.nozenypenalty=state;
	}
	else if (!strcmpi(w3,"pvp")) {
		map[m].flag.pvp=state;
		if (state) {
			if (map[m].flag.gvg || map[m].flag.gvg_dungeon || map[m].flag.gvg_castle)
				ShowWarning("npc_parse_mapflag: You can't set PvP and GvG flags for the same map! Removing GvG flags from %s (file '%s', line '%d').\n", map[m].name, filepath, strline(buffer,start-buffer));
			map[m].flag.gvg=0;
			map[m].flag.gvg_dungeon=0;
			map[m].flag.gvg_castle=0;
		}
	}
	else if (!strcmpi(w3,"pvp_noparty"))
		map[m].flag.pvp_noparty=state;
	else if (!strcmpi(w3,"pvp_noguild"))
		map[m].flag.pvp_noguild=state;
	else if (!strcmpi(w3, "pvp_nightmaredrop")) {
		char drop_arg1[16], drop_arg2[16];
		int drop_id = 0, drop_type = 0, drop_per = 0;
		if (sscanf(w4, "%[^,],%[^,],%d", drop_arg1, drop_arg2, &drop_per) == 3) {
			int i;
			if (!strcmpi(drop_arg1, "random"))
				drop_id = -1;
			else if (itemdb_exists((drop_id = atoi(drop_arg1))) == NULL)
				drop_id = 0;
			if (!strcmpi(drop_arg2, "inventory"))
				drop_type = 1;
			else if (!strcmpi(drop_arg2,"equip"))
				drop_type = 2;
			else if (!strcmpi(drop_arg2,"all"))
				drop_type = 3;

			if (drop_id != 0){
				for (i = 0; i < MAX_DROP_PER_MAP; i++) {
					if (map[m].drop_list[i].drop_id == 0){
						map[m].drop_list[i].drop_id = drop_id;
						map[m].drop_list[i].drop_type = drop_type;
						map[m].drop_list[i].drop_per = drop_per;
						break;
					}
				}
				map[m].flag.pvp_nightmaredrop = 1;
			}
		} else if (!state) //Disable
			map[m].flag.pvp_nightmaredrop = 0;
	}
	else if (!strcmpi(w3,"pvp_nocalcrank"))
		map[m].flag.pvp_nocalcrank=state;
	else if (!strcmpi(w3,"gvg")) {
		map[m].flag.gvg=state;
		if (state && map[m].flag.pvp)
		{
			map[m].flag.pvp=0;
			ShowWarning("npc_parse_mapflag: You can't set PvP and GvG flags for the same map! Removing PvP flag from %s (file '%s', line '%d').\n", map[m].name, filepath, strline(buffer,start-buffer));
		}
	}
	else if (!strcmpi(w3,"gvg_noparty"))
		map[m].flag.gvg_noparty=state;
	else if (!strcmpi(w3,"gvg_dungeon")) {
		map[m].flag.gvg_dungeon=state;
		if (state) map[m].flag.pvp=0;
	}
	else if (!strcmpi(w3,"gvg_castle")) {
		map[m].flag.gvg_castle=state;
		if (state) map[m].flag.pvp=0;
	}
	else if (!strcmpi(w3,"noexppenalty"))
		map[m].flag.noexppenalty=state;
	else if (!strcmpi(w3,"nozenypenalty"))
		map[m].flag.nozenypenalty=state;
	else if (!strcmpi(w3,"notrade"))
		map[m].flag.notrade=state;
	else if (!strcmpi(w3,"novending"))
		map[m].flag.novending=state;
	else if (!strcmpi(w3,"nodrop"))
		map[m].flag.nodrop=state;
	else if (!strcmpi(w3,"noskill"))
		map[m].flag.noskill=state;
	else if (!strcmpi(w3,"noicewall"))
		map[m].flag.noicewall=state;
	else if (!strcmpi(w3,"snow"))
		map[m].flag.snow=state;
	else if (!strcmpi(w3,"clouds"))
		map[m].flag.clouds=state;
	else if (!strcmpi(w3,"clouds2"))
		map[m].flag.clouds2=state;
	else if (!strcmpi(w3,"fog"))
		map[m].flag.fog=state;
	else if (!strcmpi(w3,"fireworks"))
		map[m].flag.fireworks=state;
	else if (!strcmpi(w3,"sakura"))
		map[m].flag.sakura=state;
	else if (!strcmpi(w3,"leaves"))
		map[m].flag.leaves=state;
	else if (!strcmpi(w3,"rain"))
		map[m].flag.rain=state;
	else if (!strcmpi(w3,"indoors"))
		map[m].flag.indoors=state;
	else if (!strcmpi(w3,"nightenabled"))
		map[m].flag.nightenabled=state;
	else if (!strcmpi(w3,"nogo"))
		map[m].flag.nogo=state;
	else if (!strcmpi(w3,"noexp")) {
		map[m].flag.nobaseexp=state;
		map[m].flag.nojobexp=state;
	}
	else if (!strcmpi(w3,"nobaseexp"))
		map[m].flag.nobaseexp=state;
	else if (!strcmpi(w3,"nojobexp"))
		map[m].flag.nojobexp=state;
	else if (!strcmpi(w3,"noloot")) {
		map[m].flag.nomobloot=state;
		map[m].flag.nomvploot=state;
	}
	else if (!strcmpi(w3,"nomobloot"))
		map[m].flag.nomobloot=state;
	else if (!strcmpi(w3,"nomvploot"))
		map[m].flag.nomvploot=state;
	else if (!strcmpi(w3,"nocommand")) {
		if (state) {
			if (sscanf(w4, "%d", &state) == 1)
				map[m].nocommand =state;
			else //No level specified, block everyone.
				map[m].nocommand =100;
		} else
			map[m].nocommand=0;
	}
	else if (!strcmpi(w3,"restricted")) {
		if (state) {
			map[m].flag.restricted=1;
			sscanf(w4, "%d", &state);
			map[m].zone |= 1<<(state+1);
		} else {
			map[m].flag.restricted=0;
			map[m].zone = 0;
		}
	}
	else if (!strcmpi(w3,"jexp")) {
		map[m].jexp = (state) ? atoi(w4) : 100;
		if( map[m].jexp < 0 ) map[m].jexp = 100;
		map[m].flag.nojobexp = (map[m].jexp==0)?1:0;
	}
	else if (!strcmpi(w3,"bexp")) {
		map[m].bexp = (state) ? atoi(w4) : 100;
		if( map[m].bexp < 0 ) map[m].bexp = 100;
		 map[m].flag.nobaseexp = (map[m].bexp==0)?1:0;
	}
	else if (!strcmpi(w3,"loadevent"))
		map[m].flag.loadevent=state;
	else if (!strcmpi(w3,"nochat"))
		map[m].flag.nochat=state;
	else if (!strcmpi(w3,"partylock"))
		map[m].flag.partylock=state;
	else if (!strcmpi(w3,"guildlock"))
		map[m].flag.guildlock=state;

	return strchr(start,'\n');// continue
}

/*==========================================
 * Setting up map cells
 *------------------------------------------*/
static const char* npc_parse_mapcell(char* w1, char* w2, char* w3, char* w4, const char* start, const char* buffer, const char* filepath)
{
	int m, cell, x, y, x0, y0, x1, y1;
	char type[32], mapname[32];

	// w1=<mapname>
	// w3=<type>,<x1>,<y1>,<x2>,<y2>
	if( sscanf(w1, "%31[^,]", mapname) != 1
	||	sscanf(w3, "%31[^,],%d,%d,%d,%d", type, &x0, &y0, &x1, &y1) < 5 )
	{
		ShowError("npc_parse_mapcell: Invalid mapcell definition in file '%s', line '%d'.\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,start-buffer), w1, w2, w3, w4);
		return strchr(start,'\n');// skip and continue
	}
	m = map_mapname2mapid(mapname);
	if( m < 0 )
	{
		ShowWarning("npc_parse_mapcell: Unknown map in file '%s', line '%d' : %s\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", mapname, filepath, strline(buffer,start-buffer), w1, w2, w3, w4);
		return strchr(start,'\n');// skip and continue
	}

	cell = strtol(type, (char **)NULL, 0);

	if( x0 > x1 )
		swap(x0, x1);
	if( y0 > y1 )
		swap(y0, y1);

	for( x = x0; x <= x1; ++x )
		for( y = y0; y <= y1; ++y )
			map_setcell(m, x, y, cell);

	return strchr(start,'\n');// continue
}

void npc_parsesrcfile(const char* filepath)
{
	int m, lines = 0;
	FILE* fp;
	size_t len;
	char* buffer;
	const char* p;

	// read whole file to buffer
	fp = fopen(filepath, "rb");
	if( fp == NULL )
	{
		ShowError("npc_parsesrcfile: File not found '%s'.\n", filepath);
		return;
	}
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	buffer = (char*)aMalloc(len+1);
	buffer[len] = '\0';
	fseek(fp, 0, SEEK_SET);
	if( fread(buffer, len, 1, fp) != 1 )
	{
		ShowError("npc_parsesrcfile: Failed to read file '%s'.\n", filepath);
		aFree(buffer);
		fclose(fp);
		return;
	}
	fclose(fp);

	// parse buffer
	for( p = skip_space(buffer); p && *p ; p = skip_space(p) )
	{
		char w1[2048], w2[2048], w3[2048], w4[2048], mapname[2048];
		int i, w4pos, count;
		lines++;

		w1[0] = w2[0] = w3[0] = w4[0] = '\0';

		// w1<TAB>w2<TAB>w3<TAB>w4
		if( (count = sscanf(p, "%[^\t\r\n]\t%[^\t\r\n]\t%[^\t\r\n]\t%n%[^\r\n]", w1, w2, w3, &w4pos, w4)) < 3 )
		{
			if( (count = sscanf(p, "%s %s %[^\t]\t %n%[^\n]", w1, w2, w3, &w4pos, w4)) == 4
			||	(count = sscanf(p, "%s %s %s %n%[^\n]\n", w1, w2, w3, &w4pos, w4)) >= 3 )
			{// Incorrect syntax, try to parse
				ShowWarning("npc_parsesrcfile: Incorrect separator syntax in file '%s', line '%d'. Use tabs instead of spaces!\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,p-buffer), w1, w2, w3, w4);
			}
			else
			{// Unknown syntax, try to continue
				ShowError("npc_parsesrcfile: Unknown syntax in file '%s', line '%d'.\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,p-buffer), w1, w2, w3, w4);
				p = strchr(p,'\n');// next line
				continue;
			}
		}

		if( strcmp(w1,"-") !=0 && strcasecmp(w1,"function") != 0 )
		{// w1 = <map name>,<x>,<y>,<facing>
			sscanf(w1,"%[^,]",mapname);
			if( !mapindex_name2id(mapname) )
			{// Incorrect map, we must skip the script info...
				ShowError("npc_parsesrcfile: Unknown map '%s' in file '%s', line '%d'.\n", mapname, filepath, strline(buffer,p-buffer));
				if( strcasecmp(w2,"script") == 0 && count > 3 )
					p = npc_skip_script(p,buffer,filepath);
				p = strchr(p,'\n');// next line
				continue;
			}
			m = map_mapname2mapid(mapname);
			if( m < 0 )
			{// "mapname" is not assigned to this server, we must skip the script info...
				if( strcasecmp(w2,"script") == 0 && count > 3 )
					p = npc_skip_script(p,buffer,filepath);
				p = strchr(p,'\n');// next line
				continue;
			}
		}

		if( strcasecmp(w2,"warp") == 0 && count > 3 )
		{
			p = npc_parse_warp(w1,w2,w3,w4, p, buffer, filepath);
		}
		else if( strcasecmp(w2,"shop") == 0 && count > 3 )
		{
			p = npc_parse_shop(w1,w2,w3,w4, p, buffer, filepath);
		}
		else if( strcasecmp(w2,"script") == 0 && count > 3 )
		{
			if( strcasecmp(w1,"function") == 0 )
				p = npc_parse_function(w1, w2, w3, w4, p, buffer, filepath);
			else
				p = npc_parse_script(w1,w2,w3,w4, p, buffer, filepath);
		}
		else if( (i=0, sscanf(w2,"duplicate%n",&i), (i > 0 && w2[i] == '(')) && count > 3 )
		{
			p = npc_parse_script(w1,w2,w3,w4, p, buffer, filepath);
		}
		else if( strcmpi(w2,"monster") == 0 && count > 3 )
		{
			p = npc_parse_mob(w1, w2, w3, w4, p, buffer, filepath);
		}
		else if( strcmpi(w2,"mapflag") == 0 && count >= 3 )
		{
			p = npc_parse_mapflag(w1, w2, w3, w4, p, buffer, filepath);
		}
		else if( strcmpi(w2,"setcell") == 0 && count >= 3 )
		{
			p = npc_parse_mapcell(w1, w2, w3, w4, p, buffer, filepath);
		}
		else
		{
			ShowError("Probably TAB is missing in file '%s', line '%d'.\n * w1=%s\n * w2=%s\n * w3=%s\n * w4=%s\n", filepath, strline(buffer,p-buffer), w1, w2, w3, w4);
			p = strchr(p,'\n');// skip and continue
		}
	}
	aFree(buffer);

	return;
}

int npc_script_event(struct map_session_data* sd, int type)
{
	int i;
	if (type < 0 || type >= NPCE_MAX)
		return 0;
	if (!sd) {
		if (battle_config.error_log)
			ShowError("npc_script_event: NULL sd. Event Type %d\n", type);
		return 0;
	}
	if (script_event[type].nd) {
		TBL_NPC *nd = script_event[type].nd;
		run_script(nd->u.scr.script,0,sd->bl.id,nd->bl.id);
		return 1;
	} else if (script_event[type].event_count) {
		for (i = 0; i<script_event[type].event_count; i++) {
			npc_event_sub(sd,script_event[type].event[i],script_event[type].event_name[i]);
		}
		return i;
	} 
	return 0;
}

static int npc_read_event_script_sub(DBKey key, void* data, va_list ap)
{
	const char* p = key.str;
	char* name = va_arg(ap, char *);
	struct event_data** event_buf = va_arg(ap, struct event_data**);
	const char** event_name = va_arg(ap,const char **);
	unsigned char *count = va_arg(ap, unsigned char *);

	if (*count >= UCHAR_MAX) return 0;
	
	if((p=strchr(p,':')) && p && strcmpi(name,p)==0 )
	{
		event_buf[*count] = (struct event_data *)data;
		event_name[*count] = key.str;
		(*count)++;
		return 1;
	}
	return 0;
}

void npc_read_event_script(void)
{
	int i;
	struct {
		char *name;
		char *event_name;
	} config[] = {
		{"Login Event",script_config.login_event_name},
		{"Logout Event",script_config.logout_event_name},
		{"Load Map Event",script_config.loadmap_event_name},
		{"Base LV Up Event",script_config.baselvup_event_name},
		{"Job LV Up Event",script_config.joblvup_event_name},
		{"Die Event",script_config.die_event_name},
		{"Kill PC Event",script_config.kill_pc_event_name},
		{"Kill NPC Event",script_config.kill_mob_event_name},
	};

	for (i = 0; i < NPCE_MAX; i++) {
		script_event[i].nd = NULL;
		script_event[i].event_count = 0;
		if (!script_config.event_script_type) {
			//Use a single NPC as event source.
			script_event[i].nd = npc_name2id(config[i].event_name);
		} else {
			//Use an array of Events
			char buf[64]="::";
			strncpy(buf+2,config[i].event_name,62);
			ev_db->foreach(ev_db,npc_read_event_script_sub,buf,
				&script_event[i].event,
				&script_event[i].event_name,
				&script_event[i].event_count);
		}
	}
	if (battle_config.etc_log) {
		//Print summary.
		for (i = 0; i < NPCE_MAX; i++) {
			if(!script_config.event_script_type) {
				if (script_event[i].nd)
					ShowInfo("%s: Using NPC named '%s'.\n", config[i].name, config[i].event_name);
				else
					ShowInfo("%s: No NPC found with name '%s'.\n", config[i].name, config[i].event_name);
			} else
				ShowInfo("%s: %d '%s' events.\n", config[i].name, script_event[i].event_count, config[i].event_name);
		}
	}
}

static int npc_cleanup_dbsub(DBKey key, void* data, va_list ap)
{
	struct block_list* bl = (struct block_list*)data;
	nullpo_retr(0, bl);

	switch(bl->type) {
	case BL_NPC:
		npc_unload((struct npc_data *)bl);
		break;
	case BL_MOB:
		unit_free(bl,0);
		break;
	}

	return 0;
}

int npc_reload(void)
{
	struct npc_src_list *nsl;
	int m, i;
	int npc_new_min = npc_id;

	//Remove all npcs/mobs. [Skotlex]
	map_foreachiddb(npc_cleanup_dbsub);
	for (m = 0; m < map_num; m++) {
		if(battle_config.dynamic_mobs) {	//dynamic check by [random]
			for (i = 0; i < MAX_MOB_LIST_PER_MAP; i++)
				if (map[m].moblist[i]) aFree(map[m].moblist[i]);
			memset (map[m].moblist, 0, sizeof(map[m].moblist));
		}
		if (map[m].npc_num > 0 && battle_config.error_log)
			ShowWarning("npc_reload: %d npcs weren't removed at map %s!\n", map[m].npc_num, map[m].name);
	}
	mob_clear_spawninfo();

	// clear npc-related data structures
	ev_db->clear(ev_db,NULL);
	npcname_db->clear(npcname_db,NULL);
	npc_warp = npc_shop = npc_script = 0;
	npc_mob = npc_cache_mob = npc_delay_mob = 0;

	//TODO: the following code is copy-pasted from do_init_npc(); clean it up
	// Reloading npcs now
	for (nsl = npc_src_files; nsl; nsl = nsl->next)
	{
		ShowStatus("Loading NPC file: %s"CL_CLL"\r", nsl->name);
		npc_parsesrcfile(nsl->name);
	}

	ShowInfo ("Done loading '"CL_WHITE"%d"CL_RESET"' NPCs:"CL_CLL"\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Warps\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Shops\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Scripts\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Mobs\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Mobs Cached\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Mobs Not Cached\n",
		npc_id - npc_new_min, npc_warp, npc_shop, npc_script, npc_mob, npc_cache_mob, npc_delay_mob);

	//Re-read the NPC Script Events cache.
	npc_read_event_script();

	//Execute the OnInit event for freshly loaded npcs. [Skotlex]
	ShowStatus("Event '"CL_WHITE"OnInit"CL_RESET"' executed with '"CL_WHITE"%d"CL_RESET"' NPCs.\n",npc_event_doall("OnInit"));
	// Execute rest of the startup events if connected to char-server. [Lance]
	if(!CheckForCharServer()){
		ShowStatus("Event '"CL_WHITE"OnCharIfInit"CL_RESET"' executed with '"CL_WHITE"%d"CL_RESET"' NPCs.\n", npc_event_doall("OnCharIfInit"));
		ShowStatus("Event '"CL_WHITE"OnInterIfInit"CL_RESET"' executed with '"CL_WHITE"%d"CL_RESET"' NPCs.\n", npc_event_doall("OnInterIfInit"));
		ShowStatus("Event '"CL_WHITE"OnInterIfInitOnce"CL_RESET"' executed with '"CL_WHITE"%d"CL_RESET"' NPCs.\n", npc_event_doall("OnInterIfInitOnce"));
	}
	return 0;
}

/*==========================================
 * 終了
 *------------------------------------------*/
int do_final_npc(void)
{
	int i;
	struct block_list *bl;

	for (i = START_NPC_NUM; i < npc_id; i++){
		if ((bl = map_id2bl(i))){
			if (bl->type == BL_NPC)
				npc_unload((struct npc_data *)bl);
			else if (bl->type&(BL_MOB|BL_PET|BL_HOM))
				unit_free(bl, 0);
		}
	}

	ev_db->destroy(ev_db, NULL);
	//There is no free function for npcname_db because at this point there shouldn't be any npcs left!
	//So if there is anything remaining, let the memory manager catch it and report it.
	npcname_db->destroy(npcname_db, NULL);
	ers_destroy(timer_event_ers);
	npc_clearsrcfile();

	return 0;
}

static void npc_debug_warps_sub(struct npc_data* nd)
{
	int m;
	if (nd->bl.type != BL_NPC || nd->bl.subtype != WARP || nd->bl.m < 0)
		return;

	m = map_mapindex2mapid(nd->u.warp.mapindex);
	if (m < 0) return; //Warps to another map, nothing to do about it.

	if (map_getcell(m, nd->u.warp.x, nd->u.warp.y, CELL_CHKNPC)) {
		ShowWarning("Warp %s at %s(%d,%d) warps directly on top of an area npc at %s(%d,%d)\n",
			nd->name,
			map[nd->bl.m].name, nd->bl.x, nd->bl.y,
			map[m].name, nd->u.warp.x, nd->u.warp.y
			);
	}
	if (map_getcell(m, nd->u.warp.x, nd->u.warp.y, CELL_CHKNOPASS)) {
		ShowWarning("Warp %s at %s(%d,%d) warps to a non-walkable tile at %s(%d,%d)\n",
			nd->name,
			map[nd->bl.m].name, nd->bl.x, nd->bl.y,
			map[m].name, nd->u.warp.x, nd->u.warp.y
			);
	}
}

static void npc_debug_warps(void)
{
	int m, i;
	for (m = 0; m < map_num; m++)
		for (i = 0; i < map[m].npc_num; i++)
			npc_debug_warps_sub(map[m].npc[i]);
}

/*==========================================
 * npc initialization
 *------------------------------------------*/
int do_init_npc(void)
{
	struct npc_src_list *file;
	int i;

	//Stock view data for normal npcs.
	memset(&npc_viewdb, 0, sizeof(npc_viewdb));
	npc_viewdb[0].class_ = INVISIBLE_CLASS; //Invisible class is stored here.
	for( i = 1; i < MAX_NPC_CLASS; i++ ) 
		npc_viewdb[i].class_ = i;

	ev_db = db_alloc(__FILE__,__LINE__,DB_STRING,DB_OPT_DUP_KEY|DB_OPT_RELEASE_DATA,2*NAME_LENGTH+2+1);
	npcname_db = db_alloc(__FILE__,__LINE__,DB_STRING,DB_OPT_BASE,NAME_LENGTH);

	memset(&ev_tm_b, -1, sizeof(ev_tm_b));
	timer_event_ers = ers_new(sizeof(struct timer_event_data));

	// process all npc files
	ShowStatus("Loading NPCs...\r");
	for( file = npc_src_files; file != NULL; file = file->next )
	{
		ShowStatus("Loading NPC file: %s"CL_CLL"\r", file->name);
		npc_parsesrcfile(file->name);
	}

	ShowInfo ("Done loading '"CL_WHITE"%d"CL_RESET"' NPCs:"CL_CLL"\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Warps\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Shops\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Scripts\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Mobs\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Mobs Cached\n"
		"\t-'"CL_WHITE"%d"CL_RESET"' Mobs Not Cached\n",
		npc_id - START_NPC_NUM, npc_warp, npc_shop, npc_script, npc_mob, npc_cache_mob, npc_delay_mob);

	// set up the events cache
	memset(script_event, 0, sizeof(script_event));
	npc_read_event_script();

	//Debug function to locate all endless loop warps.
	if (battle_config.warp_point_debug)
		npc_debug_warps();

	add_timer_func_list(npc_event_do_clock,"npc_event_do_clock");
	add_timer_func_list(npc_timerevent,"npc_timerevent");

	// Init dummy NPC
	fake_nd = (struct npc_data *)aCalloc(1,sizeof(struct npc_data));
	fake_nd->bl.m = -1;
	fake_nd->bl.id = npc_get_new_npc_id();
	fake_nd->class_ = -1;
	fake_nd->speed = 200;
	strcpy(fake_nd->name,"FAKE_NPC");
	memcpy(fake_nd->exname, fake_nd->name, 9);

	npc_script++;
	fake_nd->bl.type = BL_NPC;
	fake_nd->bl.subtype = SCRIPT;

	strdb_put(npcname_db, fake_nd->exname, fake_nd);
	fake_nd->u.scr.timerid = -1;
	map_addiddb(&fake_nd->bl);
	// End of initialization

	return 0;
}
