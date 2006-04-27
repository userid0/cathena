// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "../common/timer.h"
#include "../common/db.h"
#include "../common/nullpo.h"
#include "../common/malloc.h"
#include "../common/showmsg.h"
#include "../common/ers.h"

#include "map.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "status.h"
#include "mob.h"
#include "guild.h"
#include "itemdb.h"
#include "skill.h"
#include "battle.h"
#include "party.h"
#include "npc.h"
#include "log.h"
#include "script.h"
#include "atcommand.h"
#include "date.h"
#include "irc.h"

#define MIN_MOBTHINKTIME 100
#define IDLE_SKILL_INTERVAL 10	//Active idle skills should be triggered every 1 second (1000/MIN_MOBTHINKTIME)

#define MOB_LAZYSKILLPERC 10	// Probability for mobs far from players from doing their IDLE skill. (rate of 1000 minute)
#define MOB_LAZYMOVEPERC 50	// Move probability in the negligent mode MOB (rate of 1000 minute)
#define MOB_LAZYWARPPERC 20	// Warp probability in the negligent mode MOB (rate of 1000 minute)

#define MOB_SLAVEDISTANCE 2	//Distance that slaves should keep from their master.

#define MAX_MINCHASE 30	//Max minimum chase value to use for mobs.
//Dynamic mob database, allows saving of memory when there's big gaps in the mob_db [Skotlex]
struct mob_db *mob_db_data[MAX_MOB_DB+1];
struct mob_db *mob_dummy = NULL;	//Dummy mob to be returned when a non-existant one is requested.

struct mob_db *mob_db(int index) { if (index < 0 || index > MAX_MOB_DB || mob_db_data[index] == NULL) return mob_dummy; return mob_db_data[index]; }

static struct eri *item_drop_ers; //For loot drops delay structures.
static struct eri *item_drop_list_ers;
#define CLASSCHANGE_BOSS_NUM 21

/*==========================================
 * Local prototype declaration   (only required thing)
 *------------------------------------------
 */
static int mob_makedummymobdb(int);
static int mob_spawn_guardian_sub(int,unsigned int,int,int);
int mobskill_use(struct mob_data *md,unsigned int tick,int event);
int mob_skillid2skillidx(int class_,int skillid);

/*==========================================
 * Mob is searched with a name.
 *------------------------------------------
 */
int mobdb_searchname(const char *str)
{
	int i;
	struct mob_db* mob;
	for(i=0;i<=MAX_MOB_DB;i++){
		mob = mob_db(i);
		if(mob == mob_dummy) //Skip dummy mobs.
			continue;
		if(strcmpi(mob->name,str)==0 || strcmpi(mob->jname,str)==0 || strcmpi(mob->sprite,str)==0)
			return i;
	}

	return 0;
}
static int mobdb_searchname_array_sub(struct mob_db* mob, const char *str)
{
	if (mob == mob_dummy)
		return 1; //Invalid item.
	if(strstr(mob->jname,str))
		return 0;
	if(strstr(mob->name,str))
		return 0;
	return strcmpi(mob->jname,str);
}

/*==========================================
 * Founds up to N matches. Returns number of matches [Skotlex]
 *------------------------------------------
 */
int mobdb_searchname_array(struct mob_db** data, int size, const char *str)
{
	int count = 0, i;
	struct mob_db* mob;
	for(i=0;i<=MAX_MOB_DB;i++){
		mob = mob_db(i);
		if (mob == mob_dummy)
			continue;
		if (!mobdb_searchname_array_sub(mob, str)) {
			if (count < size)
				data[count] = mob;
			count++;	
		}
	}
	return count;
}

/*==========================================
 * Id Mob is checked.
 *------------------------------------------
 */
int mobdb_checkid(const int id)
{
	if (mob_db(id) == mob_dummy)
		return 0;
	if (mob_is_clone(id)) //checkid is used mostly for random ID based code, therefore clone mobs are out of the question.
		return 0;
	return id;
}

/*==========================================
 * Returns the view data associated to this mob class.
 *------------------------------------------
 */
struct view_data * mob_get_viewdata(class_) 
{
	if (mob_db(class_) == mob_dummy)
		return 0;
	return &mob_db(class_)->vd;
}
/*==========================================
 * Cleans up mob-spawn data to make it "valid"
 *------------------------------------------
 */
int mob_parse_dataset(struct spawn_data *data) {
	int i;
	//FIXME: This implementation is not stable, npc scripts will stop working once MAX_MOB_DB changes value! [Skotlex]
	if(data->class_ > 2*MAX_MOB_DB){ // large/tiny mobs [Valaris]
		data->state.size=2;
		data->class_ -= 2*MAX_MOB_DB;
	} else if (data->class_ > MAX_MOB_DB) {
		data->state.size=1;
		data->class_ -= MAX_MOB_DB;
	}
	
	if ((!mobdb_checkid(data->class_) && !mob_is_clone(data->class_)) || !data->num)
		return 0;

	//better safe than sorry, current md->npc_event has a size of 50
	if (strlen(data->eventname) >= 50)
		return 0;

	if (data->eventname[0] && strlen(data->eventname) <= 2)
	{ //Portable monster big/small implementation. [Skotlex]
		i = atoi(data->eventname);
		if (i) {
			if (i&2)
				data->state.size=1;
			else if (i&4)
				data->state.size=2;
			if (i&8)
				data->state.ai=1;
			data->eventname[0] = '\0'; //Clear event as it is not used.
		}
	}
	if (!data->level)
		data->level = mob_db(data->class_)->lv;

	if(strcmp(data->name,"--en--")==0)
		strncpy(data->name,mob_db(data->class_)->name,NAME_LENGTH-1);
	else if(strcmp(data->name,"--ja--")==0)
		strncpy(data->name,mob_db(data->class_)->jname,NAME_LENGTH-1);

	return 1;
}
/*==========================================
 * Generates the basic mob data using the spawn_data provided.
 *------------------------------------------
 */
struct mob_data* mob_spawn_dataset(struct spawn_data *data)
{
	struct mob_data *md = aCalloc(1, sizeof(struct mob_data));
	md->bl.id= npc_get_new_npc_id();
	md->bl.type = BL_MOB;
	md->bl.subtype = MONS;
	md->bl.m = data->m;
	md->bl.x = data->x;
	md->bl.y = data->y;
	md->class_ = data->class_;
	md->db = mob_db(md->class_);
	md->speed = md->db->speed;
	memcpy(md->name, data->name, NAME_LENGTH-1);
	if (data->state.ai)
		md->special_state.ai = data->state.ai;
	if (data->state.size)
		md->special_state.size = data->state.size;
	if (data->eventname[0] && strlen(data->eventname) >= 4)
		memcpy(md->npc_event, data->eventname, 50);
	md->level = data->level;

	if(md->db->mode&MD_LOOTER)
		md->lootitem = (struct item *)aCalloc(LOOTITEM_SIZE,sizeof(struct item));
	md->spawn_n = -1;
	md->deletetimer = -1;
	md->skillidx = -1;
	status_set_viewdata(&md->bl, md->class_);
	status_change_init(&md->bl);
	unit_dataset(&md->bl);
	
	map_addiddb(&md->bl);
	return md;
}

/*==========================================
 * Fetches a random mob_id [Skotlex]
 * type: Where to fetch from:
 * 0: dead branch list
 * 1: poring list
 * 2: bloody branch list
 * flag:
 * &1: Apply the summon success chance found in the list.
 * &2: Apply a monster check level.
 * lv: Mob level to check against
 *------------------------------------------
 */

int mob_get_random_id(int type, int flag, int lv) {
	struct mob_db *mob;
	int i=0, k=0, class_;
	if(type < 0 || type >= MAX_RANDOMMONSTER) {
		if (battle_config.error_log)
			ShowError("mob_get_random_id: Invalid type (%d) of random monster.\n", type);
		return 0;
	}
	do {
		class_ = rand() % MAX_MOB_DB;
		if (flag&1)
			k = rand() % 1000000;
		mob = mob_db(class_);
	} while ((mob == mob_dummy || mob->summonper[type] <= k ||
		 (flag&2 && lv < mob->lv)) && (i++) < MAX_MOB_DB);
	if(i >= MAX_MOB_DB)
		class_ = mob_db_data[0]->summonper[type];
	return class_;
}

/*==========================================
 * The MOB appearance for one time (for scripts)
 *------------------------------------------
 */
int mob_once_spawn (struct map_session_data *sd, char *mapname,
	short x, short y, const char *mobname, int class_, int amount, const char *event)
{
	struct mob_data *md = NULL;
	struct spawn_data data;
	int m, count, lv = 255;
	
	if(sd) lv = sd->status.base_level;

	if(sd && strcmp(mapname,"this")==0)
		m = sd->bl.m;
	else
		m = map_mapname2mapid(mapname);

	memset(&data, 0, sizeof(struct spawn_data));
	if (m < 0 || amount <= 0)	// �l���ُ�Ȃ珢�����~�߂�
		return 0;
	data.m = m;
	data.num = amount;
	data.class_ = class_;
	strncpy(data.name, mobname, NAME_LENGTH-1);
	
	if (class_ < 0) {
		data.class_ = mob_get_random_id(-class_ -1, battle_config.random_monster_checklv?3:1, lv);
		if (!data.class_) 
			return 0;
	}
	strncpy(data.eventname, event, 50);
	
	if (x <= 0 || y <= 0) {
		if (sd)
			map_search_freecell(&sd->bl, 0, &x, &y, 1, 1, 0);
		else
		if (!map_search_freecell(NULL, m, &x, &y, -1, -1, 1))
			return 0;	//Not solved?
	}
	data.x = x;
	data.y = y;

	if (!mob_parse_dataset(&data))
		return 0;
	
	for (count = 0; count < amount; count++) {
		md =mob_spawn_dataset (&data);

		if (class_ < 0 && battle_config.dead_branch_active)
			//Behold Aegis's masterful decisions yet again...
			//"I understand the "Aggressive" part, but the "Can Move" and "Can Attack" is just stupid" - Poki#3
			md->mode = md->db->mode|MD_AGGRESSIVE|MD_CANATTACK|MD_CANMOVE;
		mob_spawn (md);

		if(class_ == MOBID_EMPERIUM) {	// emperium hp based on defense level [Valaris]
			struct guild_castle *gc = guild_mapname2gc(map[md->bl.m].name);
			struct guild *g = gc?guild_search(gc->guild_id):NULL;
			if(gc) {
				md->max_hp += 2000 * gc->defense;
				md->hp = md->max_hp;
				md->guardian_data = aCalloc(1, sizeof(struct guardian_data));
				md->guardian_data->castle = gc;
				md->guardian_data->number = MAX_GUARDIANS;
				md->guardian_data->guild_id = gc->guild_id;
				if (g)
				{
					md->guardian_data->emblem_id = g->emblem_id;
					memcpy(md->guardian_data->guild_name, g->name, NAME_LENGTH);
				}
				else if (gc->guild_id) //Guild not yet available, retry in 5.
					add_timer(gettick()+5000,mob_spawn_guardian_sub,md->bl.id,md->guardian_data->guild_id);
			}
		}	// end addition [Valaris]
	}
	return (md)?md->bl.id : 0;
}
/*==========================================
 * The MOB appearance for one time (& area specification for scripts)
 *------------------------------------------
 */
int mob_once_spawn_area(struct map_session_data *sd,char *mapname,
	int x0,int y0,int x1,int y1,
	const char *mobname,int class_,int amount,const char *event)
{
	int x,y,i,max,lx=-1,ly=-1,id=0;
	int m;

	if(strcmp(mapname,"this")==0)
		m=sd->bl.m;
	else
		m=map_mapname2mapid(mapname);

	max=(y1-y0+1)*(x1-x0+1)*3;
	if(max>1000)max=1000;

	if (m < 0 || amount <= 0)	// �l���ُ�Ȃ珢�����~�߂�
		return 0;

	for(i=0;i<amount;i++){
		int j=0;
		do{
			x=rand()%(x1-x0+1)+x0;
			y=rand()%(y1-y0+1)+y0;
		} while (map_getcell(m,x,y,CELL_CHKNOPASS) && (++j)<max);
		if(j>=max){
			if(lx>=0){	// Since reference went wrong, the place which boiled before is used.
				x=lx;
				y=ly;
			}else
				return 0;	// Since reference of the place which boils first went wrong, it stops.
		}
		if(x==0||y==0) ShowWarning("mob_once_spawn_area: xory=0, x=%d,y=%d,x0=%d,y0=%d\n",x,y,x0,y0);
		id=mob_once_spawn(sd,mapname,x,y,mobname,class_,1,event);
		lx=x;
		ly=y;
	}
	return id;
}
/*==========================================
 * Set a Guardian's guild data [Skotlex]
 *------------------------------------------
 */
static int mob_spawn_guardian_sub(int tid,unsigned int tick,int id,int data)
{	//Needed because the guild_data may not be available at guardian spawn time.
	struct block_list* bl = map_id2bl(id);
	struct mob_data* md; 
	struct guild* g;

	if (bl == NULL) //It is possible mob was already removed from map when the castle has no owner. [Skotlex]
		return 0;
	
	if (bl->type != BL_MOB || (md = (struct mob_data*)bl) == NULL)
	{
		ShowError("mob_spawn_guardian_sub: Block error!\n");
		return 0;
	}
	
	nullpo_retr(0, md->guardian_data);
	g = guild_search(data);

	if (g == NULL)
	{	//Liberate castle, if the guild is not found this is an error! [Skotlex]
		ShowError("mob_spawn_guardian_sub: Couldn't load guild %d!\n",data);
		if (md->class_ == MOBID_EMPERIUM)
		{	//Not sure this is the best way, but otherwise we'd be invoking this for ALL guardians spawned later on.
			md->guardian_data->guild_id = 0;
			if (md->guardian_data->castle->guild_id) //Free castle up.
			{
				ShowNotice("Clearing ownership of castle %d (%s)\n", md->guardian_data->castle->castle_id, md->guardian_data->castle->castle_name);
				guild_castledatasave(md->guardian_data->castle->castle_id, 1, 0);
			}
		} else {
			if (md->guardian_data->castle->guardian[md->guardian_data->number].visible)
			{	//Safe removal of guardian.
				md->guardian_data->castle->guardian[md->guardian_data->number].visible = 0;
				guild_castledatasave(md->guardian_data->castle->castle_id, 10+md->guardian_data->number,0);
				guild_castledatasave(md->guardian_data->castle->castle_id, 18+md->guardian_data->number,0);
			}
			unit_free(&md->bl); //Remove guardian.
		}
		return 0;
	}
	md->guardian_data->emblem_id = g->emblem_id;
	memcpy (md->guardian_data->guild_name, g->name, NAME_LENGTH);
	md->guardian_data->guardup_lv = guild_checkskill(g,GD_GUARDUP);
	return 0;
}

/*==========================================
 * Summoning Guardians [Valaris]
 *------------------------------------------
 */
int mob_spawn_guardian(struct map_session_data *sd,char *mapname,
	int x,int y,const char *mobname,int class_,int amount,const char *event,int guardian)
{
	struct mob_data *md=NULL;
	struct spawn_data data;
	struct guild *g=NULL;
	struct guild_castle *gc;
	int m, count;
	memset(&data, 0, sizeof(struct spawn_data));
	data.num = 1;

	if( sd && strcmp(mapname,"this")==0)
		m=sd->bl.m;
	else
		m=map_mapname2mapid(mapname);

	if(m<0 || amount<=0)
		return 0;
	data.m = m;
	data.num = amount;
	if(class_<0)
		return 0;
	data.class_ = class_;

	if(guardian < 0 || guardian >= MAX_GUARDIANS)
	{
		ShowError("mob_spawn_guardian: Invalid guardian index %d for guardian %d (castle map %s)\n", guardian, class_, map[m].name);
		return 0;
	}
	if (amount > 1)
		ShowWarning("mob_spawn_guardian: Spawning %d guardians in position %d (castle map %s)\n", amount, map[m].name);
	
	if(sd){
		if(x<=0) x=sd->bl.x;
		if(y<=0) y=sd->bl.y;
	}
	else if(x<=0 || y<=0)
		ShowWarning("mob_spawn_guardian: Invalid coordinates (%d,%d)\n",x,y);
	data.x = x;
	data.y = y;
	strncpy(data.name, mobname, NAME_LENGTH-1);
	strncpy(data.eventname, event, 50);
	if (!mob_parse_dataset(&data))
		return 0;
	
	gc=guild_mapname2gc(map[m].name);
	if (gc == NULL)
	{
		ShowError("mob_spawn_guardian: No castle set at map %s\n", map[m].name);
		return 0;
	}
	if (!gc->guild_id)
		ShowWarning("mob_spawn_guardian: Spawning guardian %d on a castle with no guild (castle map %s)\n", class_, map[m].name);
	else
		g = guild_search(gc->guild_id);

	if (gc->guardian[guardian].id)
		ShowWarning("mob_spawn_guardian: Spawning guardian in position %d which already has a guardian (castle map %s)\n", guardian, map[m].name);
	
	for(count=0;count<data.num;count++){
		md= mob_spawn_dataset(&data);
		mob_spawn(md);

		md->max_hp += 2000 * gc->defense;
		md->guardian_data = aCalloc(1, sizeof(struct guardian_data));
		md->guardian_data->number = guardian;
		md->guardian_data->guild_id = gc->guild_id;
		md->guardian_data->castle = gc;
		md->hp = gc->guardian[guardian].hp;
		gc->guardian[guardian].id = md->bl.id;
		if (g)
		{
			md->guardian_data->emblem_id = g->emblem_id;
			memcpy (md->guardian_data->guild_name, g->name, NAME_LENGTH);
			md->guardian_data->guardup_lv = guild_checkskill(g,GD_GUARDUP);
		} else if (md->guardian_data->guild_id)
			add_timer(gettick()+5000,mob_spawn_guardian_sub,md->bl.id,md->guardian_data->guild_id);
	}

	return (amount>0)?md->bl.id:0;
}

/*==========================================
 * Reachability to a Specification ID existence place
 * state indicates type of 'seek' mob should do:
 * - MSS_LOOT: Looking for item, path must be easy.
 * - MSS_RUSH: Chasing attacking player, path is determined by mob_ai&1
 * - MSS_FOLLOW: Initiative/support seek, path must be easy.
 *------------------------------------------
 */
int mob_can_reach(struct mob_data *md,struct block_list *bl,int range, int state)
{
	int easy = 0;

	nullpo_retr(0, md);
	nullpo_retr(0, bl);
	switch (state) {
		case MSS_RUSH:
			easy = (battle_config.mob_ai&1?0:1);
			break;
		case MSS_LOOT:
		case MSS_FOLLOW:
		default:
			easy = 1;
			break;
	}
	return unit_can_reach_bl(&md->bl, bl, range, easy, NULL, NULL);
}

/*==========================================
 * Links nearby mobs (supportive mobs)
 *------------------------------------------
 */
int mob_linksearch(struct block_list *bl,va_list ap)
{
	struct mob_data *md;
	int class_;
	struct block_list *target;
	unsigned int tick;
	
	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	md=(struct mob_data *)bl;
	class_ = va_arg(ap, int);
	target = va_arg(ap, struct block_list *);
	tick=va_arg(ap, unsigned int);

	if (md->class_ == class_ && DIFF_TICK(md->last_linktime, tick) < MIN_MOBLINKTIME
		&& !md->target_id)
	{
		md->last_linktime = tick;
		if( mob_can_reach(md,target,md->db->range2, MSS_FOLLOW) ){	// Reachability judging
			md->target_id = target->id;
			md->state.aggressive = (status_get_mode(&md->bl)&MD_ANGRY)?1:0;
			md->min_chase=md->db->range3;
			return 1;
		}
	}

	return 0;
}

/*==========================================
 * mob spawn with delay (timer function)
 *------------------------------------------
 */
static int mob_delayspawn(int tid, unsigned int tick, int m, int n)
{
	struct block_list *bl = map_id2bl(m);
	if (bl && bl->type == BL_MOB)
		mob_spawn((TBL_MOB*)bl);
	return 0;
}

/*==========================================
 * spawn timing calculation
 *------------------------------------------
 */
int mob_setdelayspawn(struct mob_data *md)
{
	unsigned int spawntime, spawntime1, spawntime2, spawntime3;


	if (!md->spawn) //Doesn't has respawn data!
		return unit_free(&md->bl);

	spawntime1 = md->last_spawntime + md->spawn->delay1;
	spawntime2 = md->last_deadtime + md->spawn->delay2;
	spawntime3 = gettick() + 5000 + rand()%5000; //Lupus
	// spawntime = max(spawntime1,spawntime2,spawntime3);
	if (DIFF_TICK(spawntime1, spawntime2) > 0)
		spawntime = spawntime1;
	else
		spawntime = spawntime2;
	if (DIFF_TICK(spawntime3, spawntime) > 0)
		spawntime = spawntime3;

	add_timer(spawntime, mob_delayspawn, md->bl.id, 0);
	return 0;
}

/*==========================================
 * Mob spawning. Initialization is also variously here.
 *------------------------------------------
 */
int mob_spawn (struct mob_data *md)
{
	int i=0;
	unsigned int c =0, tick = gettick();

	md->last_spawntime = tick;
	md->last_thinktime = tick -MIN_MOBTHINKTIME;
	if (md->bl.prev != NULL)
		unit_remove_map(&md->bl,2);
	else if (md->vd->class_ != md->class_) {
		status_set_viewdata(&md->bl, md->class_);
		md->db = mob_db(md->class_);
		md->speed=md->db->speed;
		if (md->spawn)
			memcpy(md->name,md->spawn->name,NAME_LENGTH);
		else if (battle_config.override_mob_names == 1)
			memcpy(md->name,md->db->name,NAME_LENGTH);
		else
			memcpy(md->name,md->db->jname,NAME_LENGTH);
	}

	if (md->spawn) { //Respawn data
		md->bl.m = md->spawn->m;

		if ((md->spawn->x == 0 && md->spawn->y == 0) || md->spawn->xs || md->spawn->ys)
		{	//Monster can be spawned on an area.
			short x, y, xs, ys;
			if (md->spawn->x == 0 && md->spawn->y == 0)
				xs = ys = -1;
			else {
				x = md->spawn->x;
				y = md->spawn->y;
				xs = md->spawn->xs/2;
				ys = md->spawn->ys/2;
			}
			if (!map_search_freecell(NULL, md->spawn->m, &x, &y, xs, ys, battle_config.no_spawn_on_player?5:1)) {
				// retry again later
				add_timer(tick+5000,mob_delayspawn,md->bl.id,0);
				return 1;
			}
			md->bl.x = x;
			md->bl.y = y;
		} else {
			md->bl.x = md->spawn->x;
			md->bl.y = md->spawn->y;
		}
	}
	memset(&md->state, 0, sizeof(md->state));

	md->attacked_id = 0;
	md->attacked_players = 0;
	md->attacked_count = 0;
	md->target_id = 0;
	md->mode = 0;
	md->move_fail_count = 0;

	md->def_ele = md->db->element;

	if (!md->level) // [Valaris]
		md->level=md->db->lv;

	md->master_id = 0;
	md->master_dist = 0;

	md->state.skillstate = MSS_IDLE;
	md->next_walktime = tick+rand()%5000+1000;
	md->last_linktime = tick;

	/* Guardians should be spawned using mob_spawn_guardian! [Skotlex]
	 * and the Emperium is spawned using mob_once_spawn.
	md->guild_id = 0;
	if (md->class_ >= 1285 && md->class_ <= 1288) {
		struct guild_castle *gc=guild_mapname2gc(map[md->bl.m].name);
		if(gc)
			md->guild_id = gc->guild_id;
	}
	*/

	for (i = 0, c = tick-1000*3600*10; i < MAX_MOBSKILL; i++)
		md->skilldelay[i] = c;

	memset(md->dmglog, 0, sizeof(md->dmglog));
	md->tdmg = 0;
	if (md->lootitem)
		memset(md->lootitem, 0, sizeof(md->lootitem));
	md->lootitem_count = 0;

	if(md->db->option)
		// Added for carts, falcons and pecos for cloned monsters. [Valaris]
		md->sc.option = md->db->option;

	md->max_hp = md->db->max_hp;
	if(md->special_state.size==1) // change for sized monsters [Valaris]
		md->max_hp/=2;
	else if(md->special_state.size==2)
		md->max_hp*=2;
	md->hp = md->max_hp;

	map_addblock(&md->bl);
	clif_spawn(&md->bl);
	skill_unit_move(&md->bl,tick,1);
	mobskill_use(md, tick, MSC_SPAWN);
	return 0;
}

/*==========================================
 * Determines if the mob can change target. [Skotlex]
 *------------------------------------------
 */
static int mob_can_changetarget(struct mob_data* md, struct block_list* target, int mode)
{
	// if the monster was provoked ignore the above rule [celest]
	if(md->state.provoke_flag && md->state.provoke_flag != target->id &&
		!battle_config.mob_ai&4)
		return 0;
	
	switch (md->state.skillstate) {
		case MSS_BERSERK: //Only Assist, Angry or Aggressive+CastSensor mobs can change target while attacking.
			if (mode&(MD_ASSIST|MD_ANGRY) || (mode&(MD_AGGRESSIVE|MD_CASTSENSOR)) == (MD_AGGRESSIVE|MD_CASTSENSOR))
				return (battle_config.mob_ai&4 || check_distance_bl(&md->bl, target, 3));
			else
				return 0;
		case MSS_RUSH:
			return (mode&MD_AGGRESSIVE);
		case MSS_FOLLOW:
		case MSS_ANGRY:
		case MSS_IDLE:
		case MSS_WALK:
		case MSS_LOOT:
			return 1;
		default:
			return 0;
	}
}

/*==========================================
 * Determination for an attack of a monster
 *------------------------------------------
 */
int mob_target(struct mob_data *md,struct block_list *bl,int dist)
{
	nullpo_retr(0, md);
	nullpo_retr(0, bl);

	// Nothing will be carried out if there is no mind of changing TAGE by TAGE ending.
	if(md->target_id && !mob_can_changetarget(md, bl, status_get_mode(&md->bl)))
		return 0;

	if(!status_check_skilluse(&md->bl, bl, 0, 0))
		return 0;

	md->target_id = bl->id;	// Since there was no disturbance, it locks on to target.
	if (md->state.provoke_flag && bl->id != md->state.provoke_flag)
		md->state.provoke_flag = 0;
	md->min_chase=dist+md->db->range3;
	if(md->min_chase>MAX_MINCHASE)
		md->min_chase=MAX_MINCHASE;
	return 0;
}

/*==========================================
 * The ?? routine of an active monster
 *------------------------------------------
 */
static int mob_ai_sub_hard_activesearch(struct block_list *bl,va_list ap)
{
	struct mob_data *md;
	struct block_list **target;
	int dist;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	md=va_arg(ap,struct mob_data *);
	target= va_arg(ap,struct block_list**);

	//If can't seek yet, not an enemy, or you can't attack it, skip.
	if ((*target) == bl || battle_check_target(&md->bl,bl,BCT_ENEMY)<=0 || !status_check_skilluse(&md->bl, bl, 0, 0))
		return 0;

	switch (bl->type)
	{
	case BL_PC:
		if (((struct map_session_data*)bl)->state.gangsterparadise &&
			!(status_get_mode(&md->bl)&MD_BOSS))
			return 0; //Gangster paradise protection.
	case BL_MOB:
		if((dist=distance_bl(&md->bl, bl)) < md->db->range2
			&& (md->db->range > 6 || mob_can_reach(md,bl,dist+1, MSS_FOLLOW))
			&& ((*target) == NULL || !check_distance_bl(&md->bl, *target, dist)) //New target closer than previous one.
		) {
			(*target) = bl;
			md->target_id=bl->id;
			md->state.aggressive = (status_get_mode(&md->bl)&MD_ANGRY)?1:0;
			md->min_chase= dist + md->db->range3;
			if(md->min_chase>MAX_MINCHASE)
				md->min_chase=MAX_MINCHASE;
			return 1;
		}
		break;
	}
	return 0;
}

/*==========================================
 * chase target-change routine.
 *------------------------------------------
 */
static int mob_ai_sub_hard_changechase(struct block_list *bl,va_list ap)
{
	struct mob_data *md;
	struct block_list **target;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	md=va_arg(ap,struct mob_data *);
	target= va_arg(ap,struct block_list**);

	//If can't seek yet, not an enemy, or you can't attack it, skip.
	if ((*target) == bl || battle_check_target(&md->bl,bl,BCT_ENEMY)<=0 || !status_check_skilluse(&md->bl, bl, 0, 0))
		return 0;

	switch (bl->type)
	{
	case BL_PC:
	case BL_MOB:
		if(check_distance_bl(&md->bl, bl, md->db->range) &&
			battle_check_range (&md->bl, bl, md->db->range)
		) {
			(*target) = bl;
			md->target_id=bl->id;
			md->state.aggressive = (status_get_mode(&md->bl)&MD_ANGRY)?1:0;
			md->min_chase= md->db->range3;
			return 1;
		}
		break;
	}
	return 0;
}


/*==========================================
 * loot monster item search
 *------------------------------------------
 */
static int mob_ai_sub_hard_lootsearch(struct block_list *bl,va_list ap)
{
	struct mob_data* md;
	struct block_list **target;
	int dist;

	md=va_arg(ap,struct mob_data *);
	target= va_arg(ap,struct block_list**);

	if((dist=distance_bl(&md->bl, bl)) < md->db->range2 &&
		mob_can_reach(md,bl,dist, MSS_LOOT) && 
		((*target) == NULL || !check_distance_bl(&md->bl, *target, dist)) //New target closer than previous one.
	) {
		(*target) = bl;
		md->target_id=bl->id;
		md->min_chase=md->db->range3;
		md->next_walktime = gettick() + 500; //So that the mob may go after the item inmediately.
	}
	return 0;
}

/*==========================================
 * Processing of slave monsters
 *------------------------------------------
 */
static int mob_ai_sub_hard_slavemob(struct mob_data *md,unsigned int tick)
{
	struct block_list *bl;
	int old_dist;

	nullpo_retr(0, md);

	bl=map_id2bl(md->master_id);

	if (!bl || status_isdead(bl)) {	//�傪���S���Ă��邩������Ȃ�
		if(md->special_state.ai>0)
			unit_remove_map(&md->bl, 1);
		else
			mob_damage(NULL,md,md->hp,0);
		return 0;
	}

	if(status_get_mode(&md->bl)&MD_CANMOVE)
	{	//If the mob can move, follow around. [Check by Skotlex]
		
		if(bl->m != md->bl.m || md->master_dist > 30)
		{	// Since it is not in the same map (or is way to far), just warp it
			unit_warp(&md->bl,bl->m,bl->x,bl->y,3);
			return 0;
		}

		// Distance with between slave and master is measured.
		old_dist=md->master_dist;
		md->master_dist=distance_bl(&md->bl, bl);

		// Since the master was in near immediately before, teleport is carried out and it pursues.
		if(old_dist<10 && md->master_dist>18){
			unit_warp(&md->bl,-1,bl->x,bl->y,3);
			return 0;
		}

		// Approach master if within view range, chase back to Master's area also if standing on top of the master.
		if(md->master_dist<md->db->range3 &&
			(md->master_dist>MOB_SLAVEDISTANCE || md->master_dist == 0) &&
			unit_can_move(&md->bl))
		{
			short x = bl->x, y = bl->y;
			mob_stop_attack(md);
			if (map_search_freecell(&md->bl, bl->m, &x, &y, MOB_SLAVEDISTANCE, MOB_SLAVEDISTANCE, 1))
				unit_walktoxy(&md->bl, x, y, 0);
		}	
	} else if (bl->m != md->bl.m && map_flag_gvg(md->bl.m)) {
		//Delete the summoned mob if it's in a gvg ground and the master is elsewhere. [Skotlex]
		if(md->special_state.ai>0)
			unit_remove_map(&md->bl, 1);
		else
			mob_damage(NULL,md,md->hp,0);
		return 0;
	}
	
	//Avoid attempting to lock the master's target too often to avoid unnecessary overload. [Skotlex]
	if (DIFF_TICK(md->last_linktime, tick) < MIN_MOBLINKTIME && !md->target_id) {
		struct unit_data *ud = unit_bl2ud(bl);
		md->last_linktime = tick;
		
		if (ud) {
			struct block_list *tbl=NULL;
			if (ud->target && ud->attacktimer != -1)
				tbl=map_id2bl(ud->target);
			else if (ud->skilltarget) {
				tbl = map_id2bl(ud->skilltarget);
				//Required check as skilltarget is not always an enemy. [Skotlex]
				if (tbl && battle_check_target(&md->bl, tbl, BCT_ENEMY) <= 0)
					tbl = NULL;
			}
			if (tbl && status_check_skilluse(&md->bl, tbl, 0, 0)) {
				md->target_id=tbl->id;
				md->state.aggressive = (status_get_mode(&md->bl)&MD_ANGRY)?1:0;
				md->min_chase=md->db->range3+distance_bl(&md->bl, tbl);
				if(md->min_chase>MAX_MINCHASE)
					md->min_chase=MAX_MINCHASE;
			}
		}
	}
	return 0;
}

/*==========================================
 * A lock of target is stopped and mob moves to a standby state.
 *------------------------------------------
 */
int mob_unlocktarget(struct mob_data *md,int tick)
{
	nullpo_retr(0, md);

	md->target_id=0;
	md->state.skillstate=MSS_IDLE;
	md->next_walktime=tick+rand()%3000+3000;
	mob_stop_attack(md);
	return 0;
}
/*==========================================
 * Random walk
 *------------------------------------------
 */
int mob_randomwalk(struct mob_data *md,int tick)
{
	const int retrycount=20;
	int speed;

	nullpo_retr(0, md);

	if(DIFF_TICK(md->next_walktime,tick)<0 && unit_can_move(&md->bl)){
		int i,x,y,c,d=12-md->move_fail_count;
		speed=status_get_speed(&md->bl);
		if(d<5) d=5;
		for(i=0;i<retrycount;i++){	// Search of a movable place
			int r=rand();
			x=r%(d*2+1)-d;
			y=r/(d*2+1)%(d*2+1)-d;
			x+=md->bl.x;
			y+=md->bl.y;

			if((map_getcell(md->bl.m,x,y,CELL_CHKPASS)) && unit_walktoxy(&md->bl,x,y,1)){
				md->move_fail_count=0;
				break;
			}
			if(i+1>=retrycount){
				md->move_fail_count++;
				if(md->move_fail_count>1000){
					if(battle_config.error_log)
						ShowWarning("MOB cant move. random spawn %d, class = %d\n",md->bl.id,md->class_);
					md->move_fail_count=0;
					mob_spawn(md);
				}
			}
		}
		for(i=c=0;i<md->ud.walkpath.path_len;i++){	// The next walk start time is calculated.
			if(md->ud.walkpath.path[i]&1)
				c+=speed*14/10;
			else
				c+=speed;
		}
		md->next_walktime = tick+rand()%3000+3000+c;
		md->state.skillstate=MSS_WALK;
		return 1;
	}
	return 0;
}

/*==========================================
 * AI of MOB whose is near a Player
 *------------------------------------------
 */
static int mob_ai_sub_hard(struct block_list *bl,va_list ap)
{
	struct mob_data *md;
	struct block_list *tbl = NULL, *abl = NULL;
	unsigned int tick;
	int dist;
	int mode;
	int search_size;
	int view_range, can_move, can_walk;

	md = (struct mob_data*)bl;
	tick = va_arg(ap, unsigned int);

	if(md->bl.prev == NULL || md->hp <= 0)
		return 1;
		
	if (DIFF_TICK(tick, md->last_thinktime) < MIN_MOBTHINKTIME)
		return 0;
	md->last_thinktime = tick;

	if (md->ud.skilltimer != -1)
		return 0;

	if( md->ud.walktimer != -1 && md->ud.walkpath.path_pos <= 3)
		return 0;

	// Abnormalities
	if((md->sc.opt1 > 0 && md->sc.opt1 != OPT1_STONEWAIT) || md->sc.data[SC_BLADESTOP].timer != -1)
		return 0;

	if (md->sc.count && md->sc.data[SC_BLIND].timer != -1)
		view_range = 3;
	else
		view_range = md->db->range2;
	mode = status_get_mode(&md->bl);

	can_move = (mode&MD_CANMOVE)&&unit_can_move(&md->bl);
	//Since can_move is false when you are casting or the damage-delay kicks in, some special considerations
	//must be taken to avoid unlocking the target or triggering rude-attacked skills in said cases. [Skotlex]
	can_walk = DIFF_TICK(tick, md->ud.canmove_tick) > 0;

	if (md->target_id)
	{	//Check validity of current target. [Skotlex]
		tbl = map_id2bl(md->target_id);
		if (!tbl || tbl->m != md->bl.m ||
			(md->ud.attacktimer == -1 && !status_check_skilluse(&md->bl, tbl, 0, 0)) ||
			(md->ud.walktimer != -1 && !check_distance_bl(&md->bl, tbl, md->min_chase)) ||
			(
				tbl->type == BL_PC && !(mode&MD_BOSS) &&
				((struct map_session_data*)tbl)->state.gangsterparadise
		)) {	//Unlock current target.
			if (battle_config.mob_ai&8) //Inmediately stop chasing.
				mob_stop_walking(md,1);
			mob_unlocktarget(md, tick-(battle_config.mob_ai&8?3000:0)); //Imediately do random walk.
			tbl = NULL;
		}
	}
			
	// Check for target change.
	if (md->attacked_id && mode&MD_CANATTACK)
	{
		if (md->attacked_id == md->target_id)
		{
			/* Currently being unable to move shouldn't trigger rude-attacked conditions.
			if (!can_move && !battle_check_range (&md->bl, tbl, md->db->range))
			{	//Rude-attacked.
				if (md->attacked_count++ > 3)
					mobskill_use(md, tick, MSC_RUDEATTACKED);
			}
			*/
		} else
		if ((abl= map_id2bl(md->attacked_id)) && (!tbl || mob_can_changetarget(md, abl, mode))) {
			if (md->bl.m != abl->m || abl->prev == NULL ||
				(dist = distance_bl(&md->bl, abl)) >= MAX_MINCHASE ||
				battle_check_target(bl, abl, BCT_ENEMY) <= 0 ||
				(battle_config.mob_ai&2 && !status_check_skilluse(bl, abl, 0, 0)) ||
				!mob_can_reach(md, abl, dist+2, MSS_RUSH) ||
				(	//Gangster Paradise check
					abl->type == BL_PC && !(mode&MD_BOSS) &&
					((TBL_PC*)abl)->state.gangsterparadise
				)
			)	{	//Can't attack back
				if (md->attacked_count++ > 3) {
					if (mobskill_use(md, tick, MSC_RUDEATTACKED) == 0 && can_move)
					{
						int dist = rand() % 10 + 1;//��ނ��鋗��
						int dir = map_calc_dir(abl, bl->x, bl->y);
						int mask[8][2] = {{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1},{1,0},{1,1}};
						unit_walktoxy(&md->bl, md->bl.x + dist * mask[dir][0], md->bl.y + dist * mask[dir][1], 0);
					}
				}
			} else if (!(battle_config.mob_ai&2) && !status_check_skilluse(bl, abl, 0, 0)) {
				//Can't attack back, but didn't invoke a rude attacked skill...
				md->attacked_id = 0; //Simply unlock, shouldn't attempt to run away when in dumb_ai mode.
			} else { //Attackable
				if (!tbl || dist < md->db->range || !check_distance_bl(&md->bl, tbl, dist)
					|| battle_gettarget(tbl) != md->bl.id)
				{	//Change if the new target is closer than the actual one
					//or if the previous target is not attacking the mob. [Skotlex]
					md->target_id = md->attacked_id; // set target
					md->state.aggressive = 0; //Retaliating.
					md->attacked_count = 0;
					md->min_chase = dist+md->db->range2;
					if(md->min_chase>MAX_MINCHASE)
						md->min_chase=MAX_MINCHASE;
					tbl = abl; //Set the new target
				}
			}
		}
		if (md->state.aggressive && md->attacked_id == md->target_id)
			md->state.aggressive = 0; //No longer aggressive, change to retaliate AI.
		//Clear it since it's been checked for already.
		md->attacked_players = 0;
		md->attacked_id = 0;
	}
	
	// Processing of slave monster, is it needed when there's a target to deal with?
	if (md->master_id > 0 && !tbl)
		mob_ai_sub_hard_slavemob(md, tick);

	// Scan area for targets
	if ((!tbl && mode&MD_AGGRESSIVE && battle_config.monster_active_enable) ||
		(mode&MD_ANGRY && md->state.skillstate == MSS_FOLLOW)
	) {
		map_foreachinrange (mob_ai_sub_hard_activesearch, &md->bl,
			view_range, md->special_state.ai?BL_CHAR:BL_PC, md, &tbl);
	} else if (mode&MD_CHANGECHASE && (md->state.skillstate == MSS_RUSH || md->state.skillstate == MSS_FOLLOW)) {
		search_size = view_range<md->db->range ? view_range:md->db->range;
		map_foreachinrange (mob_ai_sub_hard_changechase, &md->bl,
				search_size, (md->special_state.ai?BL_CHAR:BL_PC), md, &tbl);
	}
	if (!tbl && mode&MD_LOOTER && md->lootitem && 
		(md->lootitem_count < LOOTITEM_SIZE || battle_config.monster_loot_type != 1))
	{	// Scan area for items to loot, avoid trying to loot of the mob is full and can't consume the items.
		map_foreachinrange (mob_ai_sub_hard_lootsearch, &md->bl,
			view_range, BL_ITEM, md, &tbl);
	}

	if (tbl)
	{	//Target exists, attack or loot as applicable.
		if (tbl->type != BL_ITEM)
		{	//Attempt to attack.
			//At this point we know the target is attackable, we just gotta check if the range matches.
			if (md->ud.target == tbl->id && md->ud.attacktimer != -1)
				return 0; //Already locked.
			
			if (!battle_check_range (&md->bl, tbl, md->db->range))
			{	//Out of range...
				if (!(mode&MD_CANMOVE))
				{	//Can't chase. Attempt to use a ranged skill at least?
					mobskill_use(md, tick, MSC_LONGRANGEATTACKED);
					mob_unlocktarget(md,tick);
					return 0;
				}
				md->state.skillstate = md->state.aggressive?MSS_FOLLOW:MSS_RUSH;
				if (md->ud.walktimer != -1 && md->ud.target == tbl->id &&
					(
						!battle_config.mob_ai&1 ||
						check_distance_blxy(tbl, md->ud.to_x, md->ud.to_y, md->db->range)
				)) //Current target tile is still within attack range.
					return 0;

				//Follow up
				if (!mob_can_reach(md, tbl, md->min_chase, MSS_RUSH) ||
					!unit_walktobl(&md->bl, tbl, md->db->range, 2|(!battle_config.mob_ai&1)))
					//Give up.
					mob_unlocktarget(md,tick);
				return 0;
			}
			//Target within range, engage
			md->state.skillstate = md->state.aggressive?MSS_ANGRY:MSS_BERSERK;
			unit_attack(&md->bl,tbl->id,1);
			return 0;
		} else {	//Target is BL_ITEM, attempt loot.
			struct flooritem_data *fitem;
			int i;	
			if (md->ud.target == tbl->id && md->ud.walktimer != -1)
				return 0; //Already locked.
			if (md->lootitem == NULL)
			{	//Can't loot...
				mob_unlocktarget (md, tick);
				mob_stop_walking(md,0);
				return 0;
			}

			if (!check_distance_bl(&md->bl, tbl, 1))
			{	//Still not within loot range.
				if (!(mode&MD_CANMOVE))
				{	//A looter that can't move? Real smart.
					mob_unlocktarget(md,tick);
					return 0;
				}
				if (!can_move)	// �����Ȃ���Ԃɂ���
					return 0;
				md->state.skillstate = MSS_LOOT;	// ���[�g���X�L���g�p
				if (!unit_walktobl(&md->bl, tbl, 0, 1))
					mob_unlocktarget(md, tick); //Can't loot...
				return 0;
			}
			//Within looting range.
			if (md->ud.attacktimer != -1)
				return 0; //Busy attacking?

			fitem = (struct flooritem_data *)tbl;
			if (md->lootitem_count < LOOTITEM_SIZE) {
				memcpy (&md->lootitem[md->lootitem_count++], &fitem->item_data, sizeof(md->lootitem[0]));
				if(log_config.pick > 0)	//Logs items, taken by (L)ooter Mobs [Lupus]
					log_pick((struct map_session_data*)md, "L", md->class_, md->lootitem[md->lootitem_count-1].nameid, md->lootitem[md->lootitem_count-1].amount, &md->lootitem[md->lootitem_count-1]);
			} else {	//Destroy first looted item...
				if (md->lootitem[0].card[0] == (short)0xff00)
					intif_delete_petdata( MakeDWord(md->lootitem[0].card[1],md->lootitem[0].card[2]) );
				for (i = 0; i < LOOTITEM_SIZE - 1; i++)
					memcpy (&md->lootitem[i], &md->lootitem[i+1], sizeof(md->lootitem[0]));
				memcpy (&md->lootitem[LOOTITEM_SIZE-1], &fitem->item_data, sizeof(md->lootitem[0]));
			}
			//Clear item.
			map_clearflooritem (tbl->id);
			mob_unlocktarget (md,tick);
			return 0;
		}
	}

	if(md->ud.walktimer == -1) {
		// When there's no target, it is idling.
		// Is it terribly exploitable to reuse the walkcounter for idle state skills? [Skotlex]
		md->state.skillstate = MSS_IDLE;
		if (!(++md->ud.walk_count%IDLE_SKILL_INTERVAL) && mobskill_use(md, tick, -1))
			return 0;
	}
	// Nothing else to do... except random walking.
	// Slaves do not random walk! [Skotlex]
	if (can_move && !md->master_id)
	{
		if (DIFF_TICK(md->next_walktime, tick) > 7000 &&
			(md->ud.walkpath.path_len == 0 || md->ud.walkpath.path_pos >= md->ud.walkpath.path_len))
			md->next_walktime = tick + 3000 + rand() % 2000;
		// Random movement
		if (mob_randomwalk(md,tick))
			return 0;
	}

	return 0;
}

/*==========================================
 * Serious processing for mob in PC field of view (foreachclient)
 *------------------------------------------
 */
static int mob_ai_sub_foreachclient(struct map_session_data *sd,va_list ap)
{
	unsigned int tick;
	tick=va_arg(ap,unsigned int);
	map_foreachinrange(mob_ai_sub_hard,&sd->bl, AREA_SIZE*2, BL_MOB,tick);

	return 0;
}

/*==========================================
 * Negligent mode MOB AI (PC is not in near)
 *------------------------------------------
 */
static int mob_ai_sub_lazy(DBKey key,void * data,va_list app)
{
	struct mob_data *md = (struct mob_data *)data;
	va_list ap;
	unsigned int tick;
	int mode;

	nullpo_retr(0, md);
	nullpo_retr(0, app);

	if(md->bl.type!=BL_MOB || md->bl.prev == NULL)
		return 0;

	ap = va_arg(app, va_list);

	if (battle_config.mob_ai&32 && map[md->bl.m].users>0)
		return mob_ai_sub_hard(&md->bl, ap);

	tick=va_arg(ap,unsigned int);

	if(DIFF_TICK(tick,md->last_thinktime)<MIN_MOBTHINKTIME*10)
		return 0;

	md->last_thinktime=tick;

	if (md->bl.prev==NULL || md->hp <= 0)
		return 1;

	// ��芪�������X�^�[�̏����i�Ăі߂����ꂽ���j
	if (md->master_id) {
		mob_ai_sub_hard_slavemob (md,tick);
		return 0;
	}

	mode = status_get_mode(&md->bl);
	if(DIFF_TICK(md->next_walktime,tick)<0 &&
		(mode&MD_CANMOVE) && unit_can_move(&md->bl) ){

		if( map[md->bl.m].users>0 ){
			// Since PC is in the same map, somewhat better negligent processing is carried out.

			// It sometimes moves.
			if(rand()%1000<MOB_LAZYMOVEPERC)
				mob_randomwalk(md,tick);
			else if(rand()%1000<MOB_LAZYSKILLPERC) //Chance to do a mob's idle skill.
				mobskill_use(md, tick, -1);
			// MOB which is not not the summons MOB but BOSS, either sometimes reboils.
			// People don't want this, it seems custom, noone can prove it....
//			else if( rand()%1000<MOB_LAZYWARPPERC
//				&& (md->spawn && !md->spawn->x && !md->spawn->y)
//				&& !md->target_id && !(mode&MD_BOSS))
//				unit_warp(&md->bl,-1,-1,-1,0);
		}else{
			// Since PC is not even in the same map, suitable processing is carried out even if it takes.

			// MOB which is not BOSS which is not Summons MOB, either -- a case -- sometimes -- leaping
			if( rand()%1000<MOB_LAZYWARPPERC
				&& (md->spawn && !md->spawn->x && !md->spawn->y)
				&& !(mode&MD_BOSS))
				unit_warp(&md->bl,-1,-1,-1,0);
		}

		md->next_walktime = tick+rand()%10000+5000;
	}
	return 0;
}

/*==========================================
 * Negligent processing for mob outside PC field of view   (interval timer function)
 *------------------------------------------
 */
static int mob_ai_lazy(int tid,unsigned int tick,int id,int data)
{
	map_foreachiddb(mob_ai_sub_lazy,tick);

	return 0;
}

/*==========================================
 * Serious processing for mob in PC field of view   (interval timer function)
 *------------------------------------------
 */
static int mob_ai_hard(int tid,unsigned int tick,int id,int data)
{

	if (battle_config.mob_ai&32)
		map_foreachiddb(mob_ai_sub_lazy,tick);
	else
		clif_foreachclient(mob_ai_sub_foreachclient,tick);

	return 0;
}

/*==========================================
 * Initializes the delay drop structure for mob-dropped items.
 *------------------------------------------
 */
static struct item_drop* mob_setdropitem(int nameid, int qty)
{
	struct item_drop *drop = ers_alloc(item_drop_ers, struct item_drop);
	memset(&drop->item_data, 0, sizeof(struct item));
	drop->item_data.nameid = nameid;
	drop->item_data.amount = qty;
	drop->item_data.identify = !itemdb_isequip3(nameid);
	drop->next = NULL;
	return drop;
};

/*==========================================
 * Initializes the delay drop structure for mob-looted items.
 *------------------------------------------
 */
static struct item_drop* mob_setlootitem(struct item* item)
{
	struct item_drop *drop = ers_alloc(item_drop_ers, struct item_drop);
	memcpy(&drop->item_data, item, sizeof(struct item));
	drop->next = NULL;
	return drop;
};

/*==========================================
 * item drop with delay (timer function)
 *------------------------------------------
 */
static int mob_delay_item_drop(int tid,unsigned int tick,int id,int data)
{
	struct item_drop_list *list;
	struct item_drop *ditem, *ditem_prev;
	list=(struct item_drop_list *)id;
	ditem = list->item;
	while (ditem) {
		map_addflooritem(&ditem->item_data,ditem->item_data.amount,
			list->m,list->x,list->y,
			list->first_sd,list->second_sd,list->third_sd,0);
		ditem_prev = ditem;
		ditem = ditem->next;
		ers_free(item_drop_ers, ditem_prev);
	}
	ers_free(item_drop_list_ers, list);
	return 0;
}

/*==========================================
 * Sets the item_drop into the item_drop_list.
 * Also performs logging and autoloot if enabled.
 * rate is the drop-rate of the item, required for autoloot.
 *------------------------------------------
 * by [Skotlex]
 */
static void mob_item_drop(struct mob_data *md, struct item_drop_list *dlist, struct item_drop *ditem, int loot, int drop_rate)
{
	if(log_config.pick > 0)
	{	//Logs items, dropped by mobs [Lupus]
		if (loot)
			log_pick((struct map_session_data*)md, "L", md->class_, ditem->item_data.nameid, -ditem->item_data.amount, &ditem->item_data);
		else
			log_pick((struct map_session_data*)md, "M", md->class_, ditem->item_data.nameid, -ditem->item_data.amount, NULL);
	}

	if (dlist->first_sd && dlist->first_sd->state.autoloot &&
		(drop_rate <= dlist->first_sd->state.autoloot)
	) {	//Autoloot.
		if (party_share_loot(
			dlist->first_sd->status.party_id?
				party_search(dlist->first_sd->status.party_id):
				NULL,
			dlist->first_sd,&ditem->item_data) == 0
		) {
			ers_free(item_drop_ers, ditem);
			return;
		}
	}
	ditem->next = dlist->item;
	dlist->item = ditem;
}

int mob_timer_delete(int tid, unsigned int tick, int id, int data)
{
	struct block_list *bl=map_id2bl(id);
	nullpo_retr(0, bl);
	if (bl->type != BL_MOB)
		return 0; //??
//for Alchemist CANNIBALIZE [Lupus]
	((TBL_MOB*)bl)->deletetimer = -1;
	unit_remove_map(bl, 3);
	unit_free(bl);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave_sub(struct block_list *bl,va_list ap)
{
	struct mob_data *md;
	int id;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md = (struct mob_data *)bl);

	id=va_arg(ap,int);
	if(md->master_id > 0 && md->master_id == id )
		mob_damage(NULL,md,md->hp,1);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave(struct mob_data *md)
{
	nullpo_retr(0, md);

	map_foreachinmap(mob_deleteslave_sub, md->bl.m, BL_MOB,md->bl.id);
	return 0;
}
// Mob respawning through KAIZEL or NPC_REBIRTH [Skotlex]
int mob_respawn(int tid, unsigned int tick, int id,int data )
{
	struct mob_data *md = (struct mob_data*)map_id2bl(id);
	if (!md || md->bl.type != BL_MOB)
		return 0;
	//Mob must be dead and not in a map to respawn!
	if (md->bl.prev != NULL || md->hp)
		return 0;

	md->state.skillstate = MSS_IDLE;
	md->last_thinktime = tick;
	md->next_walktime = tick+rand()%50+5000;
	md->last_linktime = tick;
	map_addblock(&md->bl);
	mob_heal(md,data*status_get_max_hp(&md->bl)/100);
	clif_spawn(&md->bl);
	skill_unit_move(&md->bl,tick,1);
	mobskill_use(md, tick, MSC_SPAWN);
	return 1;
}

/*==========================================
 * It is the damage of sd to damage to md.
 *------------------------------------------
 */
int mob_damage(struct block_list *src,struct mob_data *md,int damage,int type)
{
	int i,count,minpos,mindmg;
	struct map_session_data *sd = NULL,*tmpsd[DAMAGELOG_SIZE];
	struct {
		struct party *p;
		int id,zeny;
		unsigned int base_exp,job_exp;
	} pt[DAMAGELOG_SIZE];
	int pnum=0;
	int mvp_damage,max_hp;
	unsigned int tick = gettick();
	struct map_session_data *mvp_sd = NULL, *second_sd = NULL,*third_sd = NULL;
	double temp;
	struct item item;
	int ret, mode;
	int drop_rate;
	int race;
	char buffer[64];
	
	nullpo_retr(0, md); //src��NULL�ŌĂ΂��ꍇ������̂ŁA���Ń`�F�b�N

	max_hp = status_get_max_hp(&md->bl);
	race = status_get_race(&md->bl);

	if(src){
		if(md->nd){
			sprintf(buffer, "$@%d_attacker", md->bl.id);
			set_var(NULL, buffer, (void *)src->id);
			sprintf(buffer, "$@%d_attacktype", md->bl.id);
			set_var(NULL, buffer, (void *)(int)src->type);
			sprintf(buffer, "%s::OnDamage", md->nd->exname);
			npc_event_do(buffer);
		}
		if(src->type == BL_PC) {
			sd = (struct map_session_data *)src;
			mvp_sd = sd;
		}
	}

	if(md->bl.prev==NULL){
		if(battle_config.error_log==1)
			ShowError("mob_damage : BlockError!!\n");
		return 0;
	}

	if(md->hp<=0) {
		if(md->bl.prev != NULL)
			unit_remove_map(&md->bl, 0);
		return 0;
	}

	if(damage > max_hp>>2)
		skill_stop_dancing(&md->bl);

	if(md->hp > max_hp)
		md->hp = max_hp;

	// The amount of overkill rounds to hp.
	if(damage>md->hp)
		damage=md->hp;
	md->hp-=damage;
	md->tdmg+=damage; //Store total damage...
	
	if(!(type&2)) {
		int id = 0;
		if (src) {
			switch (src->type) {
				case BL_PC: 
					id = sd->status.char_id;
					if(rand()%1000 < 1000/++(md->attacked_players))
						md->attacked_id = sd->bl.id;
					break;
				case BL_PET:
				{
					struct pet_data *pd = (struct pet_data*)src;
					if (battle_config.pet_attack_exp_to_master) {
						id = pd->msd->status.char_id;
						damage=(damage*battle_config.pet_attack_exp_rate)/100; //Modify logged damage accordingly.
					}
					//Let mobs retaliate against the pet's master [Skotlex]
					if(rand()%1000 < 1000/++(md->attacked_players))
						md->attacked_id = pd->msd->bl.id;
					break;
				}
				case BL_MOB:
				{
					struct mob_data* md2 = (struct mob_data*)src;
					if(md2->special_state.ai && md2->master_id) {
						struct map_session_data* msd = map_id2sd(md2->master_id);
						if (msd) id = msd->status.char_id;
					}
					if(rand()%1000 < 1000/++(md->attacked_players))
					{	//Let players decide whether to retaliate versus the master or the mob. [Skotlex]
						if (md2->master_id && battle_config.retaliate_to_master)
							md->attacked_id = md2->master_id;
						else
							md->attacked_id = md2->bl.id;
					}
					break;
				}
			}
		}
		//Log damage...
		if (id && damage > 0) {
			for(i=0,minpos=DAMAGELOG_SIZE-1,mindmg=0x7fffffff;i<DAMAGELOG_SIZE;i++){
				if(md->dmglog[i].id==id)
					break;
				if(md->dmglog[i].id==0) {	//Store data in first empty slot.
					md->dmglog[i].id = id;
					break;
				}
				if(md->dmglog[i].dmg<mindmg){
					minpos=i;
					mindmg=md->dmglog[i].dmg;
				}
			}
			if(i<DAMAGELOG_SIZE)
				md->dmglog[i].dmg+=damage;
			else {
				md->dmglog[minpos].id=id;
				md->dmglog[minpos].dmg=damage;
			}
		}
	}
	
	if(md->guardian_data && md->guardian_data->number < MAX_GUARDIANS) { // guardian hp update [Valaris] (updated by [Skotlex])
		if ((md->guardian_data->castle->guardian[md->guardian_data->number].hp = md->hp) <= 0)
		{
			guild_castledatasave(md->guardian_data->castle->castle_id, 10+md->guardian_data->number,0);
			guild_castledatasave(md->guardian_data->castle->castle_id, 18+md->guardian_data->number,0);
		}
	}	// end addition

	if(md->special_state.ai == 2 &&	//�X�t�B�A�[�}�C��
		src && md->master_id == src->id)
	{
		md->state.alchemist = 1;
		mobskill_use(md, tick, MSC_ALCHEMIST);
	}

	if (battle_config.show_mob_hp)
		clif_charnameack (0, &md->bl);
		
	if(md->hp > 0)
		return damage;

	// ----- �������玀�S���� -----

	mode = status_get_mode(&md->bl); //Mode will be used for various checks regarding exp/drops.

	//changestate will clear all status effects, so we need to know if RICHMANKIM is in effect before then. [Skotlex]
	//I just recycled ret because it isn't used until much later and I didn't want to add a new variable for it.
	ret = (md->sc.data[SC_RICHMANKIM].timer != -1)?(25 + 11*md->sc.data[SC_RICHMANKIM].val1):0;

	md->state.skillstate = MSS_DEAD;	
	mobskill_use(md,tick,-1);	//On Dead skill.

	if (md->sc.data[SC_KAIZEL].timer != -1) {
		//Revive in a bit.
		max_hp = 10*md->sc.data[SC_KAIZEL].val1; //% of life to rebirth with
		clif_clearchar_area(&md->bl,1);
		mob_unlocktarget(md,tick);
		mob_stop_walking(md, 0);
		map_delblock(&md->bl);
		add_timer(gettick()+3000, mob_respawn, md->bl.id, max_hp);
		return damage;
	}

	map_freeblock_lock();

	memset(tmpsd,0,sizeof(tmpsd));
	memset(pt,0,sizeof(pt));

	max_hp = status_get_max_hp(&md->bl);

	if(src && src->type == BL_MOB)
		mob_unlocktarget((struct mob_data *)src,tick);

	if(sd) {
		int sp = 0, hp = 0;
		if (sd->state.attack_type == BF_MAGIC && sd->ud.skilltarget == md->bl.id && (i=pc_checkskill(sd,HW_SOULDRAIN))>0)
		{	//Soul Drain should only work on targetted spells [Skotlex]
			if (pc_issit(sd)) pc_setstand(sd); //Character stuck in attacking animation while 'sitting' fix. [Skotlex]
			clif_skill_nodamage(src,&md->bl,HW_SOULDRAIN,i,1);
			sp += (status_get_lv(&md->bl))*(95+15*i)/100;
		}
		sp += sd->sp_gain_value;
		sp += sd->sp_gain_race[race];
		sp += sd->sp_gain_race[mode&MD_BOSS?10:11];
		hp += sd->hp_gain_value;
		if (sp > 0) {
			if(sd->status.sp + sp > sd->status.max_sp)
				sp = sd->status.max_sp - sd->status.sp;
			sd->status.sp += sp;
			if (sp > 0 && battle_config.show_hp_sp_gain)
				clif_heal(sd->fd,SP_SP,sp);
		}
		if (hp > 0) {
			if(sd->status.hp + hp > sd->status.max_hp)
				hp = sd->status.max_hp - sd->status.hp;
			sd->status.hp += hp;
			if (hp > 0 && battle_config.show_hp_sp_gain)
				clif_heal(sd->fd,SP_HP,hp);
		}
		if (sd->mission_mobid == md->class_) { //TK_MISSION [Skotlex]
			//Recycling hp for new random target id...
			if (++sd->mission_count >= 100 && (hp = mob_get_random_id(0, 0, sd->status.base_level)))
			{
				pc_addfame(sd, 1);
				sd->mission_mobid = hp;
				pc_setglobalreg(sd,"TK_MISSION_ID", hp);
				sd->mission_count = 0;
				clif_mission_mob(sd, hp, 0);
			}
			pc_setglobalreg(sd,"TK_MISSION_COUNT", sd->mission_count);
		}
	}

	// map�O�ɏ������l�͌v�Z���珜���̂�
	// overkill���͖�������sum��max_hp�Ƃ͈Ⴄ

	for(i=0,count=0,mvp_damage=0;i<DAMAGELOG_SIZE;i++){
		if(md->dmglog[i].id==0)
			break; //Reached end of log.
		count++; //Count an attacker even if he is dead/logged-out.
		tmpsd[i] = map_charid2sd(md->dmglog[i].id);
		if(tmpsd[i] == NULL)
			continue;
		if(tmpsd[i]->bl.m != md->bl.m || pc_isdead(tmpsd[i]))
			continue;

		if(mvp_damage<md->dmglog[i].dmg){
			third_sd = second_sd;
			second_sd = mvp_sd;
			mvp_sd=tmpsd[i];
			mvp_damage=md->dmglog[i].dmg;
		}
	}

	// [MouseJstr]
	if((map[md->bl.m].flag.pvp == 0) || (battle_config.pvp_exp == 1)) {

	// �o���l�̕��z
	for(i=0;i<DAMAGELOG_SIZE;i++){
		int pid,flag=1,zeny=0;
		unsigned int base_exp,job_exp;
		double per;
		struct party *p;
		if(tmpsd[i]==NULL || tmpsd[i]->bl.m != md->bl.m || pc_isdead(tmpsd[i]))
			continue;
		
		if (battle_config.exp_calc_type)	// eAthena's exp formula based on max hp.
			per = (double)md->dmglog[i].dmg/(double)max_hp;
		else //jAthena's exp formula based on total damage.
			per = (double)md->dmglog[i].dmg/(double)md->tdmg;
	
		if (count>1)	
			per *= (9.+(double)((count > 6)? 6:count))/10.; //attackers count bonus.

		base_exp = md->db->base_exp;
		job_exp = md->db->job_exp;

		if (ret)
			per += per*ret/100.; //SC_RICHMANKIM bonus. [Skotlex]

		if(sd) {
			if (sd->expaddrace[race])
				per += per*sd->expaddrace[race]/100.;	
				per += per*sd->expaddrace[mode&MD_BOSS?10:11]/100.;
		}
		if (battle_config.pk_mode && (md->db->lv - tmpsd[i]->status.base_level >= 20))
			per *= 1.15;	// pk_mode additional exp if monster >20 levels [Valaris]	
		
		//SG additional exp from Blessings [Komurka] - probably can be optimalized ^^;;
		//
		if(md->class_ == tmpsd[i]->hate_mob[2] && (battle_config.allow_skill_without_day || is_day_of_star() || tmpsd[i]->sc.data[SC_MIRACLE].timer!=-1))
			per += per*20*pc_checkskill(tmpsd[i],SG_STAR_BLESS)/100.;
		else if(md->class_ == tmpsd[i]->hate_mob[1] && (battle_config.allow_skill_without_day || is_day_of_moon()))
			per += per*10*pc_checkskill(tmpsd[i],SG_MOON_BLESS)/100.;
		else if(md->class_ == tmpsd[i]->hate_mob[0] && (battle_config.allow_skill_without_day || is_day_of_sun()))
			per += per*10*pc_checkskill(tmpsd[i],SG_SUN_BLESS)/100.;

		if(md->special_state.size==1)	// change experience for different sized monsters [Valaris]
			per /=2.;
		else if(md->special_state.size==2)
			per *=2.;
		if(md->master_id && md->special_state.ai) //New rule: Only player-summoned mobs do not give exp. [Skotlex]
			per = 0;
		else {
			if(battle_config.zeny_from_mobs) {
				if(md->level > 0) zeny=(int) ((md->level+rand()%md->level)*per); // zeny calculation moblv + random moblv [Valaris]
				if(md->db->mexp > 0)
					zeny*=rand()%250;
				if(md->special_state.size==1 && zeny >=2) // change zeny for different sized monsters [Valaris]
					zeny/=2;
				else if(md->special_state.size==2 && zeny >1)
					zeny*=2;
			}
			if(battle_config.mobs_level_up && md->level > md->db->lv) // [Valaris]
				per+= per*(md->level-md->db->lv)*battle_config.mobs_level_up_exp_rate/100;
		}

		if (per > 4) per = 4; //Limit gained exp to quadro the mob's exp. [3->4 Komurka]
		
		if (base_exp*per > UINT_MAX)
			base_exp = UINT_MAX;
		else
			base_exp = (unsigned int)(base_exp*per);

		if (job_exp*per > UINT_MAX)
			job_exp = UINT_MAX;
		else
			job_exp = (unsigned int)(job_exp*per);
	
/*		//mapflags: noexp check [Lorky]
		if (map[md->bl.m].flag.nobaseexp == 1)	base_exp=0; 
		else if (base_exp < 1) base_exp = 1;
		
		if (map[md->bl.m].flag.nojobexp == 1)	job_exp=0; 
		else if (job_exp < 1) job_exp = 1;
*/
		if (map[md->bl.m].flag.nobaseexp == 1)
			base_exp=0; 
		else if (base_exp < 1)
			base_exp = (map[md->bl.m].bexp<=100) ? 1 : map[md->bl.m].bexp/100;
		else if ( map[md->bl.m].bexp != 100 )
			base_exp=(int)((double)base_exp*((double)map[md->bl.m].bexp/100.0));

		if (map[md->bl.m].flag.nojobexp == 1)
			job_exp=0; 
		else if (job_exp < 1)
			job_exp = (map[md->bl.m].jexp<=100) ? 1 : map[md->bl.m].jexp/100;
		else if ( map[md->bl.m].bexp != 100 )
			job_exp=(int)((double)job_exp*((double)map[md->bl.m].jexp/100.0));
 		
	
			
		//end added Lorky 
		if((pid=tmpsd[i]->status.party_id)>0){	// �p�[�e�B�ɓ����Ă���
			int j;
			for(j=0;j<pnum;j++)	// �����p�[�e�B���X�g�ɂ��邩�ǂ���
				if(pt[j].id==pid)
					break;
			if(j==pnum){	// ���Ȃ��Ƃ��͌������ǂ����m�F
				if((p=party_search(pid))!=NULL && p->exp!=0){
					pt[pnum].id=pid;
					pt[pnum].p=p;
					pt[pnum].base_exp=base_exp;
					pt[pnum].job_exp=job_exp;
					if(battle_config.zeny_from_mobs)
						pt[pnum].zeny=zeny; // zeny share [Valaris]
					pnum++;
					flag=0;
				}
			}else{	// ����Ƃ��͌���
				if (pt[j].base_exp > UINT_MAX - base_exp)
					pt[j].base_exp=UINT_MAX;
				else
					pt[j].base_exp+=base_exp;
				
				if (pt[j].job_exp > UINT_MAX - job_exp)
					pt[j].job_exp=UINT_MAX;
				else
					pt[j].job_exp+=job_exp;
				
				if(battle_config.zeny_from_mobs)
					pt[j].zeny+=zeny;  // zeny share [Valaris]
				flag=0;
			}
		}
		if(flag) {	// added zeny from mobs [Valaris]
			if(base_exp > 0 || job_exp > 0)
				pc_gainexp(tmpsd[i],base_exp,job_exp);
			if (battle_config.zeny_from_mobs && zeny > 0) {
				pc_getzeny(tmpsd[i],zeny); // zeny from mobs [Valaris]
			}
		}

	}
	// �������z
	for(i=0;i<pnum;i++)
		party_exp_share(pt[i].p,md->bl.m,pt[i].base_exp,pt[i].job_exp,pt[i].zeny);

	// item drop
	if (!(type&1)) {
		struct item_drop_list *dlist = ers_alloc(item_drop_list_ers, struct item_drop_list);
		struct item_drop *ditem;
		int drop_ore = -1, drop_items = 0; //slot N for DROP LOG, number of dropped items
		int log_item[10]; //8 -> 10 Lupus
		memset(&log_item,0,sizeof(log_item));
		dlist->m = md->bl.m;
		dlist->x = md->bl.x;
		dlist->y = md->bl.y;
		dlist->first_sd = mvp_sd;
		dlist->second_sd = second_sd;
		dlist->third_sd = third_sd;
		dlist->item = NULL;
	
		if (map[md->bl.m].flag.nomobloot ||
			(md->master_id && md->special_state.ai && (
			battle_config.alchemist_summon_reward == 0 || //Noone gives items
			(md->class_ != 1142 && battle_config.alchemist_summon_reward == 1) //Non Marine spheres don't drop items
		)))
			;	//No normal loot.
		else
		for (i = 0; i < 10; i++) { // 8 -> 10 Lupus
			if (md->db->dropitem[i].nameid <= 0)
				continue;
			drop_rate = md->db->dropitem[i].p;
			if (drop_rate <= 0 && !battle_config.drop_rate0item)
				drop_rate = 1;
			// change drops depending on monsters size [Valaris]
			if(md->special_state.size==1 && drop_rate >= 2)
				drop_rate/=2;
			else if(md->special_state.size==2 && drop_rate > 0)
				drop_rate*=2;
			//Drops affected by luk as a fixed increase [Valaris]
			if (src && battle_config.drops_by_luk > 0)
				drop_rate += status_get_luk(src)*battle_config.drops_by_luk/100;
			//Drops affected by luk as a % increase [Skotlex] 
			if (src && battle_config.drops_by_luk2 > 0)
				drop_rate += (int)(0.5+drop_rate*status_get_luk(src)*battle_config.drops_by_luk2/10000.0);
			if (sd && battle_config.pk_mode == 1 && (md->db->lv - sd->status.base_level >= 20))
				drop_rate = (int)(drop_rate*1.25); // pk_mode increase drops if 20 level difference [Valaris]

//			if (10000 < rand()%10000+drop_rate) { //May be better if MAX_RAND is too low?
			if (drop_rate < rand() % 10000 + 1) { //fixed 0.01% impossible drops bug [Lupus]
				drop_ore = i; //we remember an empty slot to put there ORE DISCOVERY drop later.
				continue;
			}
			drop_items++; //we count if there were any drops

			ditem = mob_setdropitem(md->db->dropitem[i].nameid, 1);
			log_item[i] = ditem->item_data.nameid;

			//A Rare Drop Global Announce by Lupus
			if(drop_rate<=battle_config.rare_drop_announce) {
				struct item_data *i_data;
				char message[128];
				i_data = itemdb_search(ditem->item_data.nameid);
				sprintf (message, msg_txt(541), (mvp_sd?mvp_sd->status.name:"???"), md->name, i_data->jname, (float)drop_rate/100);
				//MSG: "'%s' won %s's %s (chance: %%%0.02f)"
				intif_GMmessage(message,strlen(message)+1,0);
			}
			// Announce first, or else ditem will be freed. [Lance]
			// By popular demand, use base drop rate for autoloot code. [Skotlex]
			mob_item_drop(md, dlist, ditem, 0, md->db->dropitem[i].p);
		}

		// Ore Discovery [Celest]
		if (sd == mvp_sd && !map[md->bl.m].flag.nomobloot && pc_checkskill(sd,BS_FINDINGORE)>0 && battle_config.finding_ore_rate/10 >= rand()%10000) {
			ditem = mob_setdropitem(itemdb_searchrandomid(IG_FINDINGORE), 1);
			if (drop_ore<0) drop_ore=8; //we have only 10 slots in LOG, there's a check to not overflow (9th item usually a card, so we use 8th slot)
			log_item[drop_ore] = ditem->item_data.nameid; //it's for logging only
			drop_items++; //we count if there were any drops
			mob_item_drop(md, dlist, ditem, 0, battle_config.finding_ore_rate/10);
		}

		//this drop log contains ALL dropped items + ORE (if there was ORE Recovery) [Lupus]
		if(sd && log_config.drop > 0 && drop_items) //we check were there any drops.. and if not - don't write the log
			log_drop(sd, md->class_, log_item); //mvp_sd

		if(sd) {
			int itemid = 0;
			for (i = 0; i < sd->add_drop_count; i++) {
				if (sd->add_drop[i].id < 0)
					continue;
				if (sd->add_drop[i].race & (1<<race) ||
					sd->add_drop[i].race & 1<<(mode&MD_BOSS?10:11))
				{
					//check if the bonus item drop rate should be multiplied with mob level/10 [Lupus]
					if(sd->add_drop[i].rate<0) {
						//it's negative, then it should be multiplied. e.g. for Mimic,Myst Case Cards, etc
						// rate = base_rate * (mob_level/10) + 1
						drop_rate = -sd->add_drop[i].rate*(md->level/10)+1;
						if (drop_rate < battle_config.item_drop_adddrop_min)
							drop_rate = battle_config.item_drop_adddrop_min;
						else if (drop_rate > battle_config.item_drop_adddrop_max)
							drop_rate = battle_config.item_drop_adddrop_max;
					}
					else
						//it's positive, then it goes as it is
						drop_rate = sd->add_drop[i].rate;
					if (drop_rate < rand()%10000 +1)
						continue;
					itemid = (sd->add_drop[i].id > 0) ? sd->add_drop[i].id :
						itemdb_searchrandomid(sd->add_drop[i].group);

					mob_item_drop(md, dlist, mob_setdropitem(itemid,1), 0, drop_rate);
				}
			}
				
			if(sd->get_zeny_num && rand()%100 < sd->get_zeny_rate) //Gets get_zeny_num per level +/-10% [Skotlex]
				pc_getzeny(sd,md->db->lv*sd->get_zeny_num*(90+rand()%21)/100);
		}
		if(md->lootitem) {
			for(i=0;i<md->lootitem_count;i++)
				mob_item_drop(md, dlist, mob_setlootitem(&md->lootitem[i]), 1, 10000);
		}
		if (dlist->item) //There are drop items.
			add_timer(tick + ((!battle_config.delay_battle_damage || (sd && sd->state.attack_type == BF_MAGIC))?500:0),
				mob_delay_item_drop, (int)dlist, 0);
		else //No drops
			ers_free(item_drop_list_ers, dlist);
	}

	// mvp����
	if(mvp_sd && md->db->mexp > 0 && !md->special_state.ai){
		int log_mvp[2] = {0};
		int j;
		int mexp;
		temp = ((double)md->db->mexp * (9.+(double)count)/10.);	//[Gengar]
		mexp = (temp > 2147483647.)? 0x7fffffff:(int)temp;

		//mapflag: noexp check [Lorky]
		if (map[md->bl.m].flag.nobaseexp == 1 || map[md->bl.m].flag.nojobexp == 1)	mexp=1; 
		//end added [Lorky] 

		if(mexp < 1) mexp = 1;

		if(use_irc && irc_announce_mvp_flag)
			irc_announce_mvp(mvp_sd,md);

		clif_mvp_effect(mvp_sd);					// �G�t�F�N�g
		clif_mvp_exp(mvp_sd,mexp);
		pc_gainexp(mvp_sd,mexp,0);
		log_mvp[1] = mexp;
		for(j=0;j<3;j++){
			i = rand() % 3;
			//mapflag: noloot check [Lorky]
			if (map[md->bl.m].flag.nomvploot == 1) break;
			//end added Lorky 			

			if(md->db->mvpitem[i].nameid <= 0)
				continue;
			drop_rate = md->db->mvpitem[i].p;
			if(drop_rate <= 0 && !battle_config.drop_rate0item)
				drop_rate = 1;
			if(drop_rate <= rand()%10000+1) //if ==0, then it doesn't drop
				continue;
			memset(&item,0,sizeof(item));
			item.nameid=md->db->mvpitem[i].nameid;
			item.identify=!itemdb_isequip3(item.nameid);
			clif_mvp_item(mvp_sd,item.nameid);
			log_mvp[0] = item.nameid;
			if(mvp_sd->weight*2 > mvp_sd->max_weight)
				map_addflooritem(&item,1,mvp_sd->bl.m,mvp_sd->bl.x,mvp_sd->bl.y,mvp_sd,second_sd,third_sd,1);
			else if((ret = pc_additem(mvp_sd,&item,1))) {
				clif_additem(sd,0,0,ret);
				map_addflooritem(&item,1,mvp_sd->bl.m,mvp_sd->bl.x,mvp_sd->bl.y,mvp_sd,second_sd,third_sd,1);
			}
			
			//A Rare MVP Drop Global Announce by Lupus
			if(drop_rate<=battle_config.rare_drop_announce) {
				struct item_data *i_data;
				char message[128];
				i_data = itemdb_exists(item.nameid);
				sprintf (message, msg_txt(541), mvp_sd?mvp_sd->status.name :"???", md->name, i_data->jname, (float)drop_rate/100);
				//MSG: "'%s' won %s's %s (chance: %%%0.02f)"
				intif_GMmessage(message,strlen(message)+1,0);
			}

			if(log_config.pick > 0)	{//Logs items, MVP prizes [Lupus]
				log_pick((struct map_session_data*)md, "M", md->class_, item.nameid, -1, NULL);
				log_pick(mvp_sd, "P", 0, item.nameid, 1, NULL);
			}

			break;
		}

		if(log_config.mvpdrop > 0)
			log_mvpdrop(mvp_sd, md->class_, log_mvp);
	}

	} // [MouseJstr]

	// <Agit> NPC Event [OnAgitBreak]
	if(md->npc_event[0] && strcmp(((md->npc_event)+strlen(md->npc_event)-13),"::OnAgitBreak") == 0) {
		ShowNotice("MOB.C: Run NPC_Event[OnAgitBreak].\n");
		if (agit_flag == 1) //Call to Run NPC_Event[OnAgitBreak]
			guild_agit_break(md);
	}

		// SCRIPT���s
	if(md->npc_event[0]){
//		if(battle_config.battle_log)
//			printf("mob_damage : run event : %s\n",md->npc_event);
		if(src && src->type == BL_PET)
			sd = ((struct pet_data *)src)->msd;
		if(sd && battle_config.mob_npc_event_type)
			npc_event(sd,md->npc_event,0);
		else if(mvp_sd)
			npc_event(mvp_sd,md->npc_event,0);

	} else if (mvp_sd) {
	//lordalfa
		pc_setglobalreg(mvp_sd,"killedrid",(md->class_));
		if(mvp_sd->state.event_kill_mob){
			if (script_config.event_script_type == 0) {
				struct npc_data *npc;
				if ((npc = npc_name2id(script_config.kill_mob_event_name))) {
					run_script(npc->u.scr.script,0,mvp_sd->bl.id,npc->bl.id); // PCKillNPC [Lance]
					ShowStatus("Event '"CL_WHITE"%s"CL_RESET"' executed.\n",script_config.kill_mob_event_name);
				}
			} else {
				ShowStatus("%d '"CL_WHITE"%s"CL_RESET"' events executed.\n",	
				npc_event_doall_id(script_config.kill_mob_event_name, mvp_sd->bl.id), script_config.kill_mob_event_name);
			}
		}
	}
	if(md->level) md->level=0;
	map_freeblock_unlock();
	unit_remove_map(&md->bl, 1);
	return damage;
}

int mob_guardian_guildchange(struct block_list *bl,va_list ap)
{
	struct mob_data *md;
	struct guild* g;

	nullpo_retr(0, bl);
	nullpo_retr(0, md = (struct mob_data *)bl);

	if (!md->guardian_data)
		return 0;

	if (md->guardian_data->castle->guild_id == 0)
	{	//Castle with no owner? Delete the guardians.
		if (md->class_ == MOBID_EMPERIUM)
		{	//But don't delete the emperium, just clear it's guild-data
			md->guardian_data->guild_id = 0;
			md->guardian_data->emblem_id = 0;
			md->guardian_data->guild_name[0] = '\0';
		} else {
			if (md->guardian_data->castle->guardian[md->guardian_data->number].visible)
			{	//Safe removal of guardian.
				md->guardian_data->castle->guardian[md->guardian_data->number].visible = 0;
				guild_castledatasave(md->guardian_data->castle->castle_id, 10+md->guardian_data->number,0);
				guild_castledatasave(md->guardian_data->castle->castle_id, 18+md->guardian_data->number,0);
			}
			unit_free(&md->bl); //Remove guardian.
		}
		return 0;
	}
	
	g = guild_search(md->guardian_data->castle->guild_id);
	if (g == NULL)
	{	//Properly remove guardian info from Castle data.
		ShowError("mob_guardian_guildchange: New Guild (id %d) does not exists!\n", md->guardian_data->guild_id);
		md->guardian_data->castle->guardian[md->guardian_data->number].visible = 0;
		guild_castledatasave(md->guardian_data->castle->castle_id, 10+md->guardian_data->number,0);
		guild_castledatasave(md->guardian_data->castle->castle_id, 18+md->guardian_data->number,0);
		unit_free(&md->bl);
		return 0;
	}

	md->guardian_data->guild_id = md->guardian_data->castle->guild_id;
	md->guardian_data->emblem_id = g->emblem_id;
	md->guardian_data->guardup_lv = guild_checkskill(g,GD_GUARDUP);
	memcpy(md->guardian_data->guild_name, g->name, NAME_LENGTH);

	return 1;	
}
	
/*==========================================
 * Pick a random class for the mob
 *------------------------------------------
 */
int mob_random_class (int *value, size_t count)
{
	nullpo_retr(0, value);

	// no count specified, look into the array manually, but take only max 5 elements
	if (count < 1) {
		count = 0;
		while(count < 5 && mobdb_checkid(value[count])) count++;
		if(count < 1)	// nothing found
			return 0;
	} else {
		// check if at least the first value is valid
		if(mobdb_checkid(value[0]) == 0)
			return 0;
	}
	//Pick a random value, hoping it exists. [Skotlex]
	return mobdb_checkid(value[rand()%count]);
}

/*==========================================
 * Change mob base class
 *------------------------------------------
 */
int mob_class_change (struct mob_data *md, int class_)
{
	unsigned int tick = gettick();
	int i, c, hp_rate;

	nullpo_retr(0, md);

	if (md->bl.prev == NULL)
		return 0;

	hp_rate = md->hp*100/status_get_max_hp(&md->bl);
	md->db = mob_db(class_);
	md->max_hp = md->db->max_hp; //Update the mob's max HP
	if (battle_config.monster_class_change_full_recover) {
		md->hp = md->max_hp;
		memset(md->dmglog, 0, sizeof(md->dmglog));
		md->tdmg = 0;
	} else
		md->hp = md->max_hp*hp_rate/100;
	if(md->hp > md->max_hp) md->hp = md->max_hp;
	else if(md->hp < 1) md->hp = 1;

	if (battle_config.override_mob_names==1)
		memcpy(md->name,md->db->name,NAME_LENGTH-1);
	else
		memcpy(md->name,md->db->jname,NAME_LENGTH-1);
	md->speed = md->db->speed;
	md->def_ele = md->db->element;

	mob_stop_attack(md);
	mob_stop_walking(md, 0);
	unit_skillcastcancel(&md->bl, 0);
	status_set_viewdata(&md->bl, class_);
	clif_mob_class_change(md,class_);
	
	for(i=0,c=tick-1000*3600*10;i<MAX_MOBSKILL;i++)
		md->skilldelay[i] = c;

	if(md->lootitem == NULL && md->db->mode&MD_LOOTER)
		md->lootitem=(struct item *)aCalloc(LOOTITEM_SIZE,sizeof(struct item));

	if (battle_config.show_mob_hp)
		clif_charnameack(0, &md->bl);

	return 0;
}

/*==========================================
 * mob��
 *------------------------------------------
 */
int mob_heal(struct mob_data *md,int heal)
{
	int max_hp;

	nullpo_retr(0, md);
	max_hp = status_get_max_hp(&md->bl);

	md->hp += heal;
	if( max_hp < md->hp )
		md->hp = max_hp;
	else if (md->hp <= 0) {
		md->hp = 1;
		return mob_damage(NULL, md, 1, 0);
	}

	if(md->guardian_data && md->guardian_data->number < MAX_GUARDIANS) { // guardian hp update [Valaris] (updated by [Skotlex])
		if ((md->guardian_data->castle->guardian[md->guardian_data->number].hp = md->hp) <= 0)
		{
			guild_castledatasave(md->guardian_data->castle->castle_id, 10+md->guardian_data->number,0);
			guild_castledatasave(md->guardian_data->castle->castle_id, 18+md->guardian_data->number,0);
		}
	}	// end addition

	if (battle_config.show_mob_hp)
		clif_charnameack(0, &md->bl);

	return 0;
}


/*==========================================
 * Added by RoVeRT
 *------------------------------------------
 */
int mob_warpslave_sub(struct block_list *bl,va_list ap)
{
	struct mob_data *md=(struct mob_data *)bl;
	struct block_list *master;
	short x,y,range=0;
	master = va_arg(ap, struct block_list*);
	range = va_arg(ap, int);
	
	if(md->master_id!=master->id)
		return 0;

	map_search_freecell(master, 0, &x, &y, range, range, 0);
	unit_warp(&md->bl, master->m, x, y,2);
	return 1;
}

/*==========================================
 * Added by RoVeRT
 * Warps slaves. Range is the area around the master that they can
 * appear in randomly.
 *------------------------------------------
 */
int mob_warpslave(struct block_list *bl, int range)
{
	if (range < 1)
		range = 1; //Min range needed to avoid crashes and stuff. [Skotlex]
	
	return map_foreachinmap(mob_warpslave_sub, bl->m, BL_MOB, bl, range);
}

/*==========================================
 * ��ʓ��̎�芪���̐��v�Z�p(foreachinarea)
 *------------------------------------------
 */
int mob_countslave_sub(struct block_list *bl,va_list ap)
{
	int id;
	struct mob_data *md;
	id=va_arg(ap,int);
	
	md = (struct mob_data *)bl;
	if( md->master_id==id )
		return 1;
	return 0;
}

/*==========================================
 * ��ʓ��̎�芪���̐��v�Z
 *------------------------------------------
 */
int mob_countslave(struct block_list *bl)
{
	return map_foreachinmap(mob_countslave_sub, bl->m, BL_MOB,bl->id);
}
/*==========================================
 * Summons amount slaves contained in the value[5] array using round-robin. [adapted by Skotlex]
 *------------------------------------------
 */
int mob_summonslave(struct mob_data *md2,int *value,int amount,int skill_id)
{
	struct mob_data *md;
	struct spawn_data data;
	int count = 0,k=0,mode;

	nullpo_retr(0, md2);
	nullpo_retr(0, value);

	memset(&data, 0, sizeof(struct spawn_data));
	data.m = md2->bl.m;
	data.x = md2->bl.x;
	data.y = md2->bl.y;
	data.num = 1;
	data.state.size = md2->special_state.size;
	data.state.ai = md2->special_state.ai;

	if(mobdb_checkid(value[0]) == 0)
		return 0;

	mode = status_get_mode(&md2->bl);

	while(count < 5 && mobdb_checkid(value[count])) count++;
	if(count < 1) return 0;
	if (amount > 0 && amount < count) { //Do not start on 0, pick some random sub subset [Skotlex]
		k = rand()%count;
		amount+=k; //Increase final value by same amount to preserve total number to summon.
	}
	for(;k<amount;k++) {
		short x,y;
		data.class_ = value[k%count]; //Summon slaves in round-robin fashion. [Skotlex]
		if (mobdb_checkid(data.class_) == 0)
			continue;

		if (map_search_freecell(&md2->bl, 0, &x, &y, 4, 4, 0)) {
			data.x = x;
			data.y = y;
		} else {
			data.x = md2->bl.x;
			data.y = md2->bl.y;
		}
		strcpy(data.name, "--ja--");	//These two need to be loaded from the db for each slave.
		data.level = 0;
		if (!mob_parse_dataset(&data))
			continue;
		
		md= mob_spawn_dataset(&data);

		if (battle_config.slaves_inherit_speed && mode&MD_CANMOVE 
			&& (skill_id != NPC_METAMORPHOSIS && skill_id != NPC_TRANSFORMATION))
			md->speed=md2->speed;

		//Inherit the aggressive mode of the master.
		if (battle_config.slaves_inherit_mode) {
			md->mode = md->db->mode;
			if (mode&MD_AGGRESSIVE)
				md->mode |= MD_AGGRESSIVE;
			else
				md->mode &=~MD_AGGRESSIVE;
			if (md->mode == md->db->mode)
				md->mode = 0; //No change.
		}
		
		md->special_state.cached= battle_config.dynamic_mobs;	//[Skotlex]

		if (!battle_config.monster_class_change_full_recover &&
			(skill_id == NPC_TRANSFORMATION || skill_id == NPC_METAMORPHOSIS))
		{	//Scale HP
			md->hp = (md->max_hp*md2->hp)/md2->max_hp;
		}
		mob_spawn(md);
		clif_skill_nodamage(&md->bl,&md->bl,skill_id,amount,1);

		if(skill_id == NPC_SUMMONSLAVE)
			md->master_id=md2->bl.id;
	}
	return 0;
}

/*==========================================
 *MOBskill����Y��skillid��skillidx��Ԃ�
 *------------------------------------------
 */
int mob_skillid2skillidx(int class_,int skillid)
{
	int i, max = mob_db(class_)->maxskill;
	struct mob_skill *ms=mob_db(class_)->skill;

	if(ms==NULL)
		return -1;

	for(i=0;i<max;i++){
		if(ms[i].skill_id == skillid)
			return i;
	}
	return -1;

}

/*==========================================
 * Friendly Mob whose HP is decreasing by a nearby MOB is looked for.
 *------------------------------------------
 */
int mob_getfriendhprate_sub(struct block_list *bl,va_list ap)
{
	int min_rate, max_rate,rate;
	struct block_list **fr;
	struct mob_data *md;

	md = va_arg(ap,struct mob_data *);
	min_rate=va_arg(ap,int);
	max_rate=va_arg(ap,int);
	fr=va_arg(ap,struct block_list **);

	if( md->bl.id == bl->id && !(battle_config.mob_ai&16))
		return 0;

	if ((*fr) != NULL) //A friend was already found.
		return 0;
	
	if (battle_check_target(&md->bl,bl,BCT_ENEMY)>0)
		return 0;
	
	rate = 100*status_get_hp(bl)/status_get_max_hp(bl);
	
	if (rate >= min_rate && rate <= max_rate)
		(*fr) = bl;
	return 1;
}
static struct block_list *mob_getfriendhprate(struct mob_data *md,int min_rate,int max_rate)
{
	struct block_list *fr=NULL;
	int type = BL_MOB;
	
	nullpo_retr(NULL, md);

	if (md->special_state.ai) //Summoned creatures. [Skotlex]
		type = BL_PC;
	
	map_foreachinrange(mob_getfriendhprate_sub, &md->bl, 8, type,md,min_rate,max_rate,&fr);
	return fr;
}
/*==========================================
 * Check hp rate of its master
 *------------------------------------------
 */
struct block_list *mob_getmasterhpltmaxrate(struct mob_data *md,int rate)
{
	if (md && md->master_id > 0) {
		struct block_list *bl = map_id2bl(md->master_id);
		if (status_get_hp(bl) < status_get_max_hp(bl) * rate / 100)
			return bl;
	}

	return NULL;
}
/*==========================================
 * What a status state suits by nearby MOB is looked for.
 *------------------------------------------
 */
int mob_getfriendstatus_sub(struct block_list *bl,va_list ap)
{
	int cond1,cond2;
	struct mob_data **fr, *md, *mmd;
	int flag=0;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md=(struct mob_data *)bl);
	nullpo_retr(0, mmd=va_arg(ap,struct mob_data *));

	if( mmd->bl.id == bl->id && !(battle_config.mob_ai&16) )
		return 0;

	if (battle_check_target(&mmd->bl,bl,BCT_ENEMY)>0)
		return 0;
	cond1=va_arg(ap,int);
	cond2=va_arg(ap,int);
	fr=va_arg(ap,struct mob_data **);
	if( cond2==-1 ){
		int j;
		for(j=SC_COMMON_MIN;j<=SC_COMMON_MAX && !flag;j++){
			if ((flag=(md->sc.data[j].timer!=-1))) //Once an effect was found, break out. [Skotlex]
				break;
		}
	}else
		flag=( md->sc.data[cond2].timer!=-1 );
	if( flag^( cond1==MSC_FRIENDSTATUSOFF ) )
		(*fr)=md;

	return 0;
}
struct mob_data *mob_getfriendstatus(struct mob_data *md,int cond1,int cond2)
{
	struct mob_data *fr=NULL;

	nullpo_retr(0, md);

	map_foreachinrange(mob_getfriendstatus_sub, &md->bl, 8,
		BL_MOB,md,cond1,cond2,&fr);
	return fr;
}

/*==========================================
 * Skill use judging
 *------------------------------------------
 */
int mobskill_use(struct mob_data *md, unsigned int tick, int event)
{
	struct mob_skill *ms;
	struct block_list *fbl = NULL; //Friend bl, which can either be a BL_PC or BL_MOB depending on the situation. [Skotlex]
	struct mob_data *fmd = NULL;
	int i;

	nullpo_retr (0, md);
	nullpo_retr (0, ms = md->db->skill);

	if (battle_config.mob_skill_rate == 0 || md->ud.skilltimer != -1)
		return 0;

	if (event < 0 && DIFF_TICK(md->ud.canact_tick, tick) > 0)
		return 0; //Skill act delay only affects non-event skills.
	
	for (i = 0; i < md->db->maxskill; i++) {
		int c2 = ms[i].cond2, flag = 0;		

		// �f�B���C��
		if (DIFF_TICK(tick, md->skilldelay[i]) < ms[i].delay)
			continue;

		if (ms[i].state != md->state.skillstate && md->state.skillstate != MSS_DEAD) {
			if (ms[i].state == MSS_ANY || (ms[i].state == MSS_ANYTARGET && md->target_id))
				; //ANYTARGET works with any state as long as there's a target. [Skotlex]
			else
				continue;
		}
		if (rand() % 10000 > ms[i].permillage) //Lupus (max value = 10000)
			continue;

		// ��������
		flag = (event == ms[i].cond1);
		//Avoid entering on defined events to avoid "hyper-active skill use" due to the overflow of calls to this function
		//in battle. The only exception is MSC_SKILLUSED which explicitly uses the event value to trigger. [Skotlex]
		if (!flag && (event == -1 || event == MSC_SKILLUSED)){
			switch (ms[i].cond1)
			{
				case MSC_ALWAYS:
					flag = 1; break;
				case MSC_MYHPLTMAXRATE:		// HP< maxhp%
					flag = 100*md->hp/status_get_max_hp(&md->bl);
					flag = (flag <= c2);
				  	break;
				case MSC_MYHPINRATE:
					flag = 100*md->hp/status_get_max_hp(&md->bl);
					flag = (flag >= c2 && flag <= ms[i].val[0]);
					break;
				case MSC_MYSTATUSON:		// status[num] on
				case MSC_MYSTATUSOFF:		// status[num] off
					if (!md->sc.count) {
						flag = 0;
					} else if (ms[i].cond2 == -1) {
						int j;
						for (j = SC_COMMON_MIN; j <= SC_COMMON_MAX; j++)
							if ((flag = (md->sc.data[j].timer != -1)) != 0)
								break;
					} else {
						flag = (md->sc.data[ms[i].cond2].timer != -1);
					}
					flag ^= (ms[i].cond1 == MSC_MYSTATUSOFF); break;
				case MSC_FRIENDHPLTMAXRATE:	// friend HP < maxhp%
					flag = ((fbl = mob_getfriendhprate(md, 0, ms[i].cond2)) != NULL); break;
				case MSC_FRIENDHPINRATE	:
					flag = ((fbl = mob_getfriendhprate(md, ms[i].cond2, ms[i].val[0])) != NULL); break;
				case MSC_FRIENDSTATUSON:	// friend status[num] on
				case MSC_FRIENDSTATUSOFF:	// friend status[num] off
					flag = ((fmd = mob_getfriendstatus(md, ms[i].cond1, ms[i].cond2)) != NULL); break;					
				case MSC_SLAVELT:		// slave < num
					flag = (mob_countslave(&md->bl) < c2 ); break;
				case MSC_ATTACKPCGT:	// attack pc > num
					flag = (unit_counttargeted(&md->bl, 0) > c2); break;
				case MSC_SLAVELE:		// slave <= num
					flag = (mob_countslave(&md->bl) <= c2 ); break;
				case MSC_ATTACKPCGE:	// attack pc >= num
					flag = (unit_counttargeted(&md->bl, 0) >= c2); break;
				case MSC_AFTERSKILL:
					flag = (md->ud.skillid == c2); break;
				case MSC_SKILLUSED:		// specificated skill used
					flag = ((event & 0xffff) == MSC_SKILLUSED && ((event >> 16) == c2 || c2 == 0)); break;
				case MSC_RUDEATTACKED:
					flag = (md->attacked_count >= 3);
					if (flag) md->attacked_count = 0;	//Rude attacked count should be reset after the skill condition is met. Thanks to Komurka [Skotlex]
					break;
				case MSC_MASTERHPLTMAXRATE:
					flag = ((fbl = mob_getmasterhpltmaxrate(md, ms[i].cond2)) != NULL); break;
				case MSC_MASTERATTACKED:
					flag = (md->master_id > 0 && unit_counttargeted(map_id2bl(md->master_id), 0) > 0); break;
				case MSC_ALCHEMIST:
					flag = (md->state.alchemist);
					break;
			}
		}
		
		if (!flag)
			continue; //Skill requisite failed to be fulfilled.

		//Execute skill	
		if (skill_get_casttype(ms[i].skill_id) == CAST_GROUND)
		{
			struct block_list *bl = NULL;
			short x = 0, y = 0;
			if (ms[i].target <= MST_AROUND) {
				switch (ms[i].target) {
					case MST_TARGET:
					case MST_AROUND5:
					case MST_AROUND6:
					case MST_AROUND7:
					case MST_AROUND8:
						bl = map_id2bl(md->target_id);
						break;
					case MST_MASTER:
						bl = &md->bl;
						if (md->master_id) 
							bl = map_id2bl(md->master_id);
						if (bl) //Otherwise, fall through.
							break;
					case MST_FRIEND:
						if (fbl)
						{
							bl = fbl;
							break;
						} else if (fmd) {
							bl= &fmd->bl;
							break;
						} // else fall through
					default:
						bl = &md->bl;
						break;
				}
				if (bl != NULL) {
					x = bl->x; y=bl->y;
				}
			}
			if (x <= 0 || y <= 0)
				continue;
			// Look for an area to cast the spell around...
			if (ms[i].target >= MST_AROUND1 || ms[i].target >= MST_AROUND5) {
				int r = ms[i].target >= MST_AROUND1?
					(ms[i].target-MST_AROUND1) +1:
					(ms[i].target-MST_AROUND5) +1;
				map_search_freecell(&md->bl, md->bl.m, &x, &y, r, r, 3);
			}
			md->skillidx = i;
			flag = unit_skilluse_pos2(&md->bl, x, y, ms[i].skill_id, ms[i].skill_lv,
				skill_castfix_sc(&md->bl, ms[i].casttime), ms[i].cancel);
			if (!flag) md->skillidx = -1; //Skill failed.
			return flag;
		} else {
			if (ms[i].target <= MST_MASTER) {
				struct block_list *bl;
				switch (ms[i].target) {
					case MST_TARGET:
						bl = map_id2bl(md->target_id);
						break;
					case MST_MASTER:
						bl = &md->bl;
						if (md->master_id) 
							bl = map_id2bl(md->master_id);
						if (bl) //Otherwise, fall through.
							break;
					case MST_FRIEND:
						if (fbl) {
							bl = fbl;
							break;
						} else if (fmd) {
							bl = &fmd->bl;
							break;
						} // else fall through
					default:
						bl = &md->bl;
						break;
				}
				md->skillidx = i;
				flag = (bl && unit_skilluse_id2(&md->bl, bl->id, ms[i].skill_id, ms[i].skill_lv,
					skill_castfix_sc(&md->bl,ms[i].casttime),	ms[i].cancel));
				if (!flag) md->skillidx = -1;
				return flag;
			} else {
				if (battle_config.error_log)
					ShowWarning("Wrong mob skill target 'around' for non-ground skill %d (%s). Mob %d - %s\n",
						ms[i].skill_id, skill_get_name(ms[i].skill_id), md->class_, md->db->sprite);
				continue;
			}
		}
		return 1;
	}

	return 0;
}
/*==========================================
 * Skill use event processing
 *------------------------------------------
 */
int mobskill_event(struct mob_data *md, struct block_list *src, unsigned int tick, int flag)
{
	int target_id, res = 0;

	target_id = md->target_id;
	if (!target_id || battle_config.mob_changetarget_byskill)
		md->target_id = src->id;
			
	if (flag == -1)
		res = mobskill_use(md, tick, MSC_CASTTARGETED);
	else if ((flag&0xffff) == MSC_SKILLUSED)
		res = mobskill_use(md,tick,flag);
	else if (flag&BF_SHORT)
		res = mobskill_use(md, tick, MSC_CLOSEDATTACKED);
	else if (flag&BF_LONG)
		res = mobskill_use(md, tick, MSC_LONGRANGEATTACKED);
	
	if (!res)
	//Restore previous target only if skill condition failed to trigger. [Skotlex]
		md->target_id = target_id;
	//Otherwise check if the target is an enemy, and unlock if needed.
	else if (battle_check_target(&md->bl, src, BCT_ENEMY) <= 0)
		md->target_id = target_id;
	
	return res;
}

// Player cloned mobs. [Valaris]
int mob_is_clone(int class_)
{
	if(class_ < MOB_CLONE_START || class_ > MOB_CLONE_END)
		return 0;
	if (mob_db(class_) == mob_dummy)
		return 0;
	return class_;
}

//Flag values:
//&1: Set special ai (fight mobs, not players)
//If mode is not passed, a default aggressive mode is used.
//If master_id is passed, clone is attached to him.
//Returns: ID of newly crafted copy.
int mob_clone_spawn(struct map_session_data *sd, char *map, int x, int y, const char *event, int master_id, int mode, int flag, unsigned int duration)
{
	int class_;
	int i,j,inf,skill_id;
	struct mob_skill *ms;
	
	nullpo_retr(0, sd);

	for(class_=MOB_CLONE_START; class_<MOB_CLONE_END; class_++){
		if(mob_db_data[class_]==NULL)
			break;
	}

	if(class_>MOB_CLONE_END)
		return 0;

	mob_db_data[class_]=(struct mob_db*)aCalloc(1, sizeof(struct mob_db));
	sprintf(mob_db_data[class_]->sprite,sd->status.name);
	sprintf(mob_db_data[class_]->name,sd->status.name);
	sprintf(mob_db_data[class_]->jname,sd->status.name);
	mob_db_data[class_]->lv=status_get_lv(&sd->bl);
	mob_db_data[class_]->max_hp=status_get_max_hp(&sd->bl);
	mob_db_data[class_]->max_sp=0;
	mob_db_data[class_]->base_exp=1;
	mob_db_data[class_]->job_exp=1;
	mob_db_data[class_]->range=status_get_range(&sd->bl);
	mob_db_data[class_]->atk1=status_get_batk(&sd->bl); //Base attack as minimum damage.
	mob_db_data[class_]->atk2=mob_db_data[class_]->atk1 + status_get_atk(&sd->bl)+status_get_atk2(&sd->bl); //batk + weapon dmg
	mob_db_data[class_]->def=status_get_def(&sd->bl);
	mob_db_data[class_]->mdef=status_get_mdef(&sd->bl);
	mob_db_data[class_]->str=status_get_str(&sd->bl);
	mob_db_data[class_]->agi=status_get_agi(&sd->bl);
	mob_db_data[class_]->vit=status_get_vit(&sd->bl);
	mob_db_data[class_]->int_=status_get_int(&sd->bl);
	mob_db_data[class_]->dex=status_get_dex(&sd->bl);
	mob_db_data[class_]->luk=status_get_luk(&sd->bl);
	mob_db_data[class_]->range2=AREA_SIZE*2/3; //Chase area of 2/3rds of a screen.
	mob_db_data[class_]->range3=AREA_SIZE; //Let them have the same view-range as players.
	mob_db_data[class_]->size=status_get_size(&sd->bl);
	mob_db_data[class_]->race=status_get_race(&sd->bl);
	mob_db_data[class_]->element=status_get_element(&sd->bl);
	mob_db_data[class_]->mode=mode?mode:(MD_AGGRESSIVE|MD_ASSIST|MD_CASTSENSOR|MD_CANATTACK|MD_CANMOVE);
	mob_db_data[class_]->speed=status_get_speed(&sd->bl);
	mob_db_data[class_]->adelay=status_get_adelay(&sd->bl);
	mob_db_data[class_]->amotion=status_get_amotion(&sd->bl);
	mob_db_data[class_]->dmotion=status_get_dmotion(&sd->bl);
	memcpy(&mob_db_data[class_]->vd, &sd->vd, sizeof(struct view_data));
	mob_db_data[class_]->option=sd->sc.option;

	//Skill copy [Skotlex]
	ms = &mob_db_data[class_]->skill[0];
	//Go Backwards to give better priority to advanced skills.
	for (i=0,j = MAX_SKILL_TREE-1;j>=0 && i< MAX_MOBSKILL ;j--) {
		skill_id = skill_tree[sd->status.class_][j].id;
		if (!skill_id || sd->status.skill[skill_id].lv < 1 || (skill_get_inf2(skill_id)&(INF2_WEDDING_SKILL|INF2_GUILD_SKILL)))
			continue;
		memset (&ms[i], 0, sizeof(struct mob_skill));
		ms[i].skill_id = skill_id;
		ms[i].skill_lv = sd->status.skill[skill_id].lv;
		ms[i].state = MSS_ANY;
		ms[i].permillage = 1000; //Default chance of all skills: 10%
		ms[i].emotion = -1;
		ms[i].cancel = 0;
		ms[i].delay = 5000+skill_delayfix(&sd->bl,skill_id, ms[i].skill_lv);
		ms[i].casttime = skill_castfix(&sd->bl,skill_id, ms[i].skill_lv);

		inf = skill_get_inf(skill_id);
		if (inf&INF_ATTACK_SKILL) {
			ms[i].target = MST_TARGET;
			ms[i].cond1 = MSC_ALWAYS;
			if (skill_get_range(skill_id, ms[i].skill_lv)  > 3)
				ms[i].state = MSS_ANYTARGET;
			else
				ms[i].state = MSS_BERSERK;
		} else if(inf&INF_GROUND_SKILL) {
			//Normal aggressive mob, disable skills that cannot help them fight
			//against players (those with flags UF_NOMOB and UF_NOPC are specific 
			//to always aid players!) [Skotlex]
			if (!(flag&1) && skill_get_unit_flag(skill_id)&(UF_NOMOB|UF_NOPC))
				continue;
			if (skill_get_inf2(skill_id)&INF2_TRAP) { //Traps!
				ms[i].state = MSS_IDLE;
				ms[i].target = MST_AROUND2;
				ms[i].delay = 60000;
			} else if (skill_get_unit_target(skill_id) == BCT_ENEMY) { //Target Enemy
				ms[i].state = MSS_ANYTARGET;
				ms[i].target = MST_TARGET;
				ms[i].cond1 = MSC_ALWAYS;
			} else { //Target allies
				ms[i].target = MST_FRIEND;
				ms[i].cond1 = MSC_FRIENDHPLTMAXRATE;
				ms[i].cond2 = 95;
			}
		} else if (inf&INF_SELF_SKILL) {
			if (skill_get_inf2(skill_id)&INF2_NO_TARGET_SELF) { //auto-select target skill.
				ms[i].target = MST_TARGET;
				ms[i].cond1 = MSC_ALWAYS;
				if (skill_get_range(skill_id, ms[i].skill_lv)  > 3) {
					ms[i].state = MSS_ANYTARGET;
				} else {
					ms[i].state = MSS_BERSERK;
				}
			} else { //Self skill
				ms[i].target = MST_SELF;
				ms[i].cond1 = MSC_MYHPLTMAXRATE;
				ms[i].cond2 = 90;
				ms[i].permillage = 2000;
				//Delay: Remove the stock 5 secs and add half of the support time.
				ms[i].delay += -5000 +(skill_get_time(skill_id, ms[i].skill_lv) + skill_get_time2(skill_id, ms[i].skill_lv))/2;
				if (ms[i].delay < 5000)
					ms[i].delay = 5000; //With a minimum of 5 secs.
			}
		} else if (inf&INF_SUPPORT_SKILL) {
			ms[i].target = MST_FRIEND;
			ms[i].cond1 = MSC_FRIENDHPLTMAXRATE;
			ms[i].cond2 = 90;
			if (skill_id == AL_HEAL)
				ms[i].permillage = 5000; //Higher skill rate usage for heal.
			else if (skill_id == ALL_RESURRECTION)
				ms[i].cond2 = 1;
			//Delay: Remove the stock 5 secs and add half of the support time.
			ms[i].delay += -5000 +(skill_get_time(skill_id, ms[i].skill_lv) + skill_get_time2(skill_id, ms[i].skill_lv))/2;
			if (ms[i].delay < 2000)
				ms[i].delay = 2000; //With a minimum of 2 secs.
			
			if (i+1 < MAX_MOBSKILL) { //duplicate this so it also triggers on self.
				memcpy(&ms[i+1], &ms[i], sizeof(struct mob_skill));
				mob_db_data[class_]->maxskill = ++i;
				ms[i].target = MST_SELF;
				ms[i].cond1 = MSC_MYHPLTMAXRATE;
			}
		} else {
			switch (skill_id) { //Certain Special skills that are passive, and thus, never triggered.
				case MO_TRIPLEATTACK:
				case TF_DOUBLE:
					ms[i].state = MSS_BERSERK;
					ms[i].target = MST_TARGET;
					ms[i].cond1 = MSC_ALWAYS;
					ms[i].permillage = skill_id==TF_DOUBLE?(ms[i].skill_lv*500):(3000-ms[i].skill_lv*100);
					ms[i].delay -= 5000; //Remove the added delay as these could trigger on "all hits".
					break;
				default: //Untreated Skill
					continue;
			}
		}
		if (battle_config.mob_skill_rate!= 100)
			ms[i].permillage = ms[i].permillage*battle_config.mob_skill_rate/100;
		if (battle_config.mob_skill_delay != 100)
			ms[i].delay = ms[i].delay*battle_config.mob_skill_delay/100;
		
		mob_db_data[class_]->maxskill = ++i;
	}
	//Finally, spawn it.
	i = mob_once_spawn(sd,(char*)map,x,y,"--en--",class_,1,event);
	if ((master_id || flag || duration) && i) { //Further manipulate crafted char.
		struct mob_data* md = (struct mob_data*)map_id2bl(i);
		if (md && md->bl.type == BL_MOB) {
			if (flag&1) //Friendly Character
				md->special_state.ai = 1;
			if (master_id) //Attach to Master
				md->master_id = master_id;
			if (duration) //Auto Delete after a while.
				md->deletetimer = add_timer (gettick() + duration, mob_timer_delete, i, 0);
		}
#if 0
		//I am playing with this for packet-research purposes, enable it if you want, but don't remove it :X [Skotlex]
		//Guardian data
		if (sd->status.guild_id) {
			struct guild* g = guild_search(sd->status.guild_id);
			md->guardian_data = aCalloc(1, sizeof(struct guardian_data));
			md->guardian_data->castle = NULL;
			md->guardian_data->number = MAX_GUARDIANS;
			md->guardian_data->guild_id = sd->status.guild_id;
			if (g)
			{
				md->guardian_data->emblem_id = g->emblem_id;
				memcpy(md->guardian_data->guild_name, g->name, NAME_LENGTH);
			}
		}
#endif
	}

	return i;
}

int mob_clone_delete(int class_)
{
	if (class_ >= MOB_CLONE_START && class_ < MOB_CLONE_END
		&& mob_db_data[class_]!=NULL) {
		aFree(mob_db_data[class_]);
		mob_db_data[class_]=NULL;
		return 1;
	}
	return 0;
}

//
// ������
//
/*==========================================
 * Since un-setting [ mob ] up was used, it is an initial provisional value setup.
 *------------------------------------------
 */
static int mob_makedummymobdb(int class_)
{
	if (mob_dummy != NULL)
	{
		if (mob_db(class_) == mob_dummy)
			return 1; //Using the mob_dummy data already. [Skotlex]
		if (class_ > 0 && class_ <= MAX_MOB_DB)
		{	//Remove the mob data so that it uses the dummy data instead.
			aFree(mob_db_data[class_]);
			mob_db_data[class_] = NULL;
		}
		return 0;
	}
	//Initialize dummy data.	
	mob_dummy = (struct mob_db*)aCalloc(1, sizeof(struct mob_db)); //Initializing the dummy mob.
	sprintf(mob_dummy->sprite,"DUMMY");
	sprintf(mob_dummy->name,"Dummy");
	sprintf(mob_dummy->jname,"Dummy");
	mob_dummy->lv=1;
	mob_dummy->max_hp=1000;
	mob_dummy->max_sp=1;
	mob_dummy->base_exp=2;
	mob_dummy->job_exp=1;
	mob_dummy->range=1;
	mob_dummy->atk1=7;
	mob_dummy->atk2=10;
	mob_dummy->def=0;
	mob_dummy->mdef=0;
	mob_dummy->str=1;
	mob_dummy->agi=1;
	mob_dummy->vit=1;
	mob_dummy->int_=1;
	mob_dummy->dex=6;
	mob_dummy->luk=2;
	mob_dummy->range2=10;
	mob_dummy->range3=10;
	mob_dummy->race=0;
	mob_dummy->element=0;
	mob_dummy->mode=0;
	mob_dummy->speed=300;
	mob_dummy->adelay=1000;
	mob_dummy->amotion=500;
	mob_dummy->dmotion=500;
	return 0;
}

//Adjusts the drop rate of item according to the criteria given. [Skotlex]
static unsigned int mob_drop_adjust(unsigned int rate, int rate_adjust, unsigned short rate_min, unsigned short rate_max)
{
	if (battle_config.logarithmic_drops && rate_adjust > 0) //Logarithmic drops equation by Ishizu-Chan
		//Equation: Droprate(x,y) = x * (5 - log(x)) ^ (ln(y) / ln(5))
		//x is the normal Droprate, y is the Modificator.
		rate = (int)(rate * pow((5.0 - log10(rate)), (log(rate_adjust/100.) / log(5.0))) + 0.5);
	else	//Classical linear rate adjustment.
		rate = rate*rate_adjust/100;
	return (rate>rate_max)?rate_max:((rate<rate_min)?rate_min:rate);
}
/*==========================================
 * mob_db.txt reading
 *------------------------------------------
 */
static int mob_readdb(void)
{
	FILE *fp;
	char line[1024];
	char *filename[]={ "mob_db.txt","mob_db2.txt" };
	int class_, i, fi, k;

	for(fi=0;fi<2;fi++){
		sprintf(line, "%s/%s", db_path, filename[fi]);
		fp=fopen(line,"r");
		if(fp==NULL){
			if(fi>0)
				continue;
			return -1;
		}
		while(fgets(line,1020,fp)){
			double exp, maxhp;
			char *str[60], *p, *np; // 55->60 Lupus

			if(line[0] == '/' && line[1] == '/')
				continue;

			for(i=0,p=line;i<60;i++){
				if((np=strchr(p,','))!=NULL){
					str[i]=p;
					*np=0;
					p=np+1;
				} else
					str[i]=p;
			}

			class_ = atoi(str[0]);
			if (class_ == 0)
				continue; //Leave blank lines alone... [Skotlex]

			if (class_ <= 1000 || class_ > MAX_MOB_DB)
			{
				ShowWarning("Mob with ID: %d not loaded. ID must be in range [%d-%d]\n", class_, 1000, MAX_MOB_DB);
				continue;
			} else if (pcdb_checkid(class_))
			{
				ShowWarning("Mob with ID: %d not loaded. That ID is reserved for player classes.\n");
				continue;
			}
			if (mob_db_data[class_] == NULL)
				mob_db_data[class_] = aCalloc(1, sizeof (struct mob_data));

			mob_db_data[class_]->vd.class_ = class_;
			memcpy(mob_db_data[class_]->sprite, str[1], NAME_LENGTH-1);
			memcpy(mob_db_data[class_]->name, str[2], NAME_LENGTH-1);
			memcpy(mob_db_data[class_]->jname, str[3], NAME_LENGTH-1);
			mob_db_data[class_]->lv = atoi(str[4]);
			mob_db_data[class_]->max_hp = atoi(str[5]);
			mob_db_data[class_]->max_sp = atoi(str[6]);

			exp = (double)atoi(str[7]) * (double)battle_config.base_exp_rate / 100.;
			if (exp < 0)
				mob_db_data[class_]->base_exp = 0;
			if (exp > UINT_MAX)
				mob_db_data[class_]->base_exp = UINT_MAX;
			else
				mob_db_data[class_]->base_exp = (unsigned int)exp;

			exp = (double)atoi(str[8]) * (double)battle_config.job_exp_rate / 100.;
			if (exp < 0)
				mob_db_data[class_]->job_exp = 0;
			else if (exp > UINT_MAX)
				mob_db_data[class_]->job_exp = UINT_MAX;
			else
			mob_db_data[class_]->job_exp = (unsigned int)exp;
			
			mob_db_data[class_]->range=atoi(str[9]);
			mob_db_data[class_]->atk1=atoi(str[10]);
			mob_db_data[class_]->atk2=atoi(str[11]);
			mob_db_data[class_]->def=atoi(str[12]);
			mob_db_data[class_]->mdef=atoi(str[13]);
			mob_db_data[class_]->str=atoi(str[14]);
			mob_db_data[class_]->agi=atoi(str[15]);
			mob_db_data[class_]->vit=atoi(str[16]);
			mob_db_data[class_]->int_=atoi(str[17]);
			mob_db_data[class_]->dex=atoi(str[18]);
			mob_db_data[class_]->luk=atoi(str[19]);
			mob_db_data[class_]->range2=atoi(str[20]);
			mob_db_data[class_]->range3=atoi(str[21]);
			mob_db_data[class_]->size=atoi(str[22]);
			mob_db_data[class_]->race=atoi(str[23]);
			mob_db_data[class_]->element=atoi(str[24]);
			mob_db_data[class_]->mode=atoi(str[25]);
			mob_db_data[class_]->speed=atoi(str[26]);
			mob_db_data[class_]->adelay=atoi(str[27]);
			mob_db_data[class_]->amotion=atoi(str[28]);
			mob_db_data[class_]->dmotion=atoi(str[29]);

			for(i=0;i<10;i++){ // 8 -> 10 Lupus
				int rate = 0,rate_adjust,type;
				unsigned short ratemin,ratemax;
				struct item_data *id;
				mob_db_data[class_]->dropitem[i].nameid=atoi(str[30+i*2]);
				if (!mob_db_data[class_]->dropitem[i].nameid) {
					//No drop.
					mob_db_data[class_]->dropitem[i].p = 0;
					continue;
				}
				type = itemdb_type(mob_db_data[class_]->dropitem[i].nameid);
				rate = atoi(str[31+i*2]);
				if (class_ >= 1324 && class_ <= 1363)
				{	//Treasure box drop rates [Skotlex]
					rate_adjust = battle_config.item_rate_treasure;
					ratemin = battle_config.item_drop_treasure_min;
					ratemax = battle_config.item_drop_treasure_max;
				}
				else switch (type)
				{
				case 0:
					rate_adjust = battle_config.item_rate_heal; 
					ratemin = battle_config.item_drop_heal_min;
					ratemax = battle_config.item_drop_heal_max;
					break;
				case 2:
					rate_adjust = battle_config.item_rate_use;
					ratemin = battle_config.item_drop_use_min;
					ratemax = battle_config.item_drop_use_max;
					break;
				case 4:
				case 5:
				case 8:		// Changed to include Pet Equip
					rate_adjust = battle_config.item_rate_equip;
					ratemin = battle_config.item_drop_equip_min;
					ratemax = battle_config.item_drop_equip_max;
					break;
				case 6:
					rate_adjust = battle_config.item_rate_card;
					ratemin = battle_config.item_drop_card_min;
					ratemax = battle_config.item_drop_card_max;
					break;
				default:
					rate_adjust = battle_config.item_rate_common;
					ratemin = battle_config.item_drop_common_min;
					ratemax = battle_config.item_drop_common_max;
					break;
				}
				mob_db_data[class_]->dropitem[i].p = mob_drop_adjust(rate, rate_adjust, ratemin, ratemax);

				//calculate and store Max available drop chance of the item
				if (mob_db_data[class_]->dropitem[i].p) {
					id = itemdb_search(mob_db_data[class_]->dropitem[i].nameid);
					if (id->maxchance==10000 || (id->maxchance < mob_db_data[class_]->dropitem[i].p) ) {
					//item has bigger drop chance or sold in shops
						id->maxchance = mob_db_data[class_]->dropitem[i].p;
					}
					for (k = 0; k< MAX_SEARCH; k++) {
						if (id->mob[k].chance < mob_db_data[class_]->dropitem[i].p && id->mob[k].id != class_)
							break;
					}
					if (k == MAX_SEARCH)
						continue;
				
					memmove(&id->mob[k+1], &id->mob[k], (MAX_SEARCH-k-1)*sizeof(id->mob[0]));
					id->mob[k].chance = mob_db_data[class_]->dropitem[i].p;
					id->mob[k].id = class_;
				}
			}
			// MVP EXP Bonus, Chance: MEXP,ExpPer
			mob_db_data[class_]->mexp=atoi(str[50])*battle_config.mvp_exp_rate/100;
			mob_db_data[class_]->mexpper=atoi(str[51]);
			//Now that we know if it is an mvp or not,
			//apply battle_config modifiers [Skotlex]
			maxhp = (double)mob_db_data[class_]->max_hp;
			if (mob_db_data[class_]->mexp > 0)
			{	//Mvp
				if (battle_config.mvp_hp_rate != 100) 
					maxhp = maxhp * (double)battle_config.mvp_hp_rate /100.;
			} else if (battle_config.monster_hp_rate != 100) //Normal mob
				maxhp = maxhp * (double)battle_config.monster_hp_rate /100.;
			if (maxhp < 1) maxhp = 1;
			else if (maxhp > INT_MAX) maxhp = INT_MAX;
			mob_db_data[class_]->max_hp = (int)maxhp;

			// MVP Drops: MVP1id,MVP1per,MVP2id,MVP2per,MVP3id,MVP3per
			for(i=0;i<3;i++){
				struct item_data *id;
				mob_db_data[class_]->mvpitem[i].nameid=atoi(str[52+i*2]);
				if (!mob_db_data[class_]->mvpitem[i].nameid) {
					//No item....
					mob_db_data[class_]->mvpitem[i].p = 0;
					continue;
				}
				mob_db_data[class_]->mvpitem[i].p= mob_drop_adjust(atoi(str[53+i*2]), battle_config.item_rate_mvp,
					battle_config.item_drop_mvp_min, battle_config.item_drop_mvp_max);

				//calculate and store Max available drop chance of the MVP item
				if (mob_db_data[class_]->mvpitem[i].p) {
					id = itemdb_search(mob_db_data[class_]->mvpitem[i].nameid);
					if (id->maxchance==10000 || (id->maxchance < mob_db_data[class_]->mvpitem[i].p/10+1) ) {
					//item has bigger drop chance or sold in shops
						id->maxchance = mob_db_data[class_]->mvpitem[i].p/10+1; //reduce MVP drop info to not spoil common drop rate
					}			
				}
			}

			if (mob_db_data[class_]->max_hp <= 0) {
				ShowWarning ("Mob %d (%s) has no HP, using poring data for it\n", class_, mob_db_data[class_]->name);
				mob_makedummymobdb(class_);
			}
		}
		fclose(fp);
		ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n",filename[fi]);
	}
	return 0;
}

/*==========================================
 * MOB display graphic change data reading
 *------------------------------------------
 */
static int mob_readdb_mobavail(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int class_,j,k;
	char *str[20],*p,*np;

	sprintf(line, "%s/mob_avail.txt", db_path);
	if( (fp=fopen(line,"r"))==NULL ){
		ShowError("can't read %s\n", line);
		return -1;
	}

	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));

		for(j=0,p=line;j<12;j++){
			if((np=strchr(p,','))!=NULL){
				str[j]=p;
				*np=0;
				p=np+1;
			} else
				str[j]=p;
		}

		if(str[0]==NULL)
			continue;

		class_=atoi(str[0]);
		if (class_ == 0)
			continue; //Leave blank lines alone... [Skotlex]
		
		if(mob_db(class_) == mob_dummy)	// �l���ُ�Ȃ珈�����Ȃ��B
			continue;

		k=atoi(str[1]);
		if(k < 0)
			continue;

		memset(&mob_db_data[class_]->vd, 0, sizeof(struct view_data));
		mob_db_data[class_]->vd.class_=k;

		//Player sprites
		if(pcdb_checkid(k) && j>=12) {
			mob_db_data[class_]->vd.sex=atoi(str[2]);
			mob_db_data[class_]->vd.hair_style=atoi(str[3]);
			mob_db_data[class_]->vd.hair_color=atoi(str[4]);
			mob_db_data[class_]->vd.weapon=atoi(str[5]);
			mob_db_data[class_]->vd.shield=atoi(str[6]);
			mob_db_data[class_]->vd.head_top=atoi(str[7]);
			mob_db_data[class_]->vd.head_mid=atoi(str[8]);
			mob_db_data[class_]->vd.head_bottom=atoi(str[9]);
			mob_db_data[class_]->option=atoi(str[10])&~0x46;
			mob_db_data[class_]->vd.cloth_color=atoi(str[11]); // Monster player dye option - Valaris
		}
		else if(str[2] && atoi(str[2]) > 0)
			mob_db_data[class_]->vd.head_bottom=atoi(str[2]); // mob equipment [Valaris]

		ln++;
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n",ln,"mob_avail.txt");
	return 0;
}

/*==========================================
 * Reading of random monster data
 *------------------------------------------
 */
static int mob_read_randommonster(void)
{
	FILE *fp;
	char line[1024];
	char *str[10],*p;
	int i,j;

	const char* mobfile[] = {
		"mob_branch.txt",
		"mob_poring.txt",
		"mob_boss.txt" };

	for(i=0;i<MAX_RANDOMMONSTER;i++){
		mob_db_data[0]->summonper[i] = 1002;	// �ݒ肵�Y�ꂽ�ꍇ�̓|�������o��悤�ɂ��Ă���
		sprintf(line, "%s/%s", db_path, mobfile[i]);
		fp=fopen(line,"r");
		if(fp==NULL){
			ShowError("can't read %s\n",line);
			return -1;
		}
		while(fgets(line,1020,fp)){
			int class_,per;
			if(line[0] == '/' && line[1] == '/')
				continue;
			memset(str,0,sizeof(str));
			for(j=0,p=line;j<3 && p;j++){
				str[j]=p;
				p=strchr(p,',');
				if(p) *p++=0;
			}

			if(str[0]==NULL || str[2]==NULL)
				continue;

			class_ = atoi(str[0]);
			per=atoi(str[2]);
			if(mob_db(class_) != mob_dummy)
				mob_db_data[class_]->summonper[i]=per;
		}
		fclose(fp);
		ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n",mobfile[i]);
	}
	return 0;
}

/*==========================================
 * mob_skill_db.txt reading
 *------------------------------------------
 */
static int mob_readskilldb(void)
{
	FILE *fp;
	char line[1024];
	int i,tmp, count;

	const struct {
		char str[32];
		int id;
	} cond1[] = {
		{	"always",			MSC_ALWAYS				},
		{	"myhpltmaxrate",	MSC_MYHPLTMAXRATE		},
		{  "myhpinrate",		MSC_MYHPINRATE 		},
		{	"friendhpltmaxrate",MSC_FRIENDHPLTMAXRATE	},
		{	"friendhpinrate",	MSC_FRIENDHPINRATE	},
		{	"mystatuson",		MSC_MYSTATUSON			},
		{	"mystatusoff",		MSC_MYSTATUSOFF			},
		{	"friendstatuson",	MSC_FRIENDSTATUSON		},
		{	"friendstatusoff",	MSC_FRIENDSTATUSOFF		},
		{	"attackpcgt",		MSC_ATTACKPCGT			},
		{	"attackpcge",		MSC_ATTACKPCGE			},
		{	"slavelt",			MSC_SLAVELT				},
		{	"slavele",			MSC_SLAVELE				},
		{	"closedattacked",	MSC_CLOSEDATTACKED		},
		{	"longrangeattacked",MSC_LONGRANGEATTACKED	},
		{	"skillused",		MSC_SKILLUSED			},
		{	"afterskill",		MSC_AFTERSKILL			},
		{	"casttargeted",		MSC_CASTTARGETED		},
		{	"rudeattacked",		MSC_RUDEATTACKED		},
		{	"masterhpltmaxrate",MSC_MASTERHPLTMAXRATE	},
		{	"masterattacked",	MSC_MASTERATTACKED		},
		{	"alchemist",		MSC_ALCHEMIST			},
		{	"onspawn",		MSC_SPAWN},
	}, cond2[] ={
		{	"anybad",		-1				},
		{	"stone",		SC_STONE		},
		{	"freeze",		SC_FREEZE		},
		{	"stan",			SC_STUN			},
		{	"sleep",		SC_SLEEP		},
		{	"poison",		SC_POISON		},
		{	"curse",		SC_CURSE		},
		{	"silence",		SC_SILENCE		},
		{	"confusion",	SC_CONFUSION	},
		{	"blind",		SC_BLIND		},
		{	"hiding",		SC_HIDING		},
		{	"sight",		SC_SIGHT		},
	}, state[] = {
		{	"any",		MSS_ANY	}, //All states except Dead
		{	"idle",		MSS_IDLE	},
		{	"walk",		MSS_WALK	},
		{	"loot",		MSS_LOOT	},
		{	"dead",		MSS_DEAD	},
		{	"attack",	MSS_BERSERK	}, //Retaliating attack
		{	"angry",		MSS_ANGRY	}, //Preemptive attack (aggressive mobs)
		{	"chase",		MSS_RUSH		}, //Chase escaping target
		{	"follow",	MSS_FOLLOW	}, //Preemptive chase (aggressive mobs)
		{	"anytarget",MSS_ANYTARGET	}, //Berserk+Angry+Rush+Follow
	}, target[] = {
		{	"target",	MST_TARGET	},
		{	"self",		MST_SELF	},
		{	"friend",	MST_FRIEND	},
		{	"master",	MST_MASTER	},
		{	"around5",	MST_AROUND5	},
		{	"around6",	MST_AROUND6	},
		{	"around7",	MST_AROUND7	},
		{	"around8",	MST_AROUND8	},
		{	"around1",	MST_AROUND1	},
		{	"around2",	MST_AROUND2	},
		{	"around3",	MST_AROUND3	},
		{	"around4",	MST_AROUND4	},
		{	"around",	MST_AROUND	},
	};

	int x;
	char *filename[]={ "mob_skill_db.txt","mob_skill_db2.txt" };

	if (!battle_config.mob_skill_rate) {
		ShowStatus("Mob skill use disabled. Not reading mob skills.\n");
		return 0;
	}
	for(x=0;x<2;x++){
		int last_mob_id = 0;
		count = 0;
		sprintf(line, "%s/%s", db_path, filename[x]); 
		fp=fopen(line,"r");
		if(fp==NULL){
			if(x==0)
				ShowError("can't read %s\n",line);
			continue;
		}
		while(fgets(line,1020,fp)){
			char *sp[20],*p;
			int mob_id;
			struct mob_skill *ms, gms;
			int j=0;

			count++;
			if(line[0] == '/' && line[1] == '/')
				continue;

			memset(sp,0,sizeof(sp));
			for(i=0,p=line;i<18 && p;i++){
				sp[i]=p;
				if((p=strchr(p,','))!=NULL)
					*p++=0;
			}
			if(i == 0 || (mob_id=atoi(sp[0]))== 0)
				continue;
			if(i < 18) {
				ShowError("mob_skill: Insufficient number of fields for skill at %s, line %d\n", filename[x], count);
				continue;
			}
			if (mob_id > 0 && mob_db(mob_id) == mob_dummy)
			{
				if (mob_id != last_mob_id) {
					ShowWarning("mob_skill: Non existant Mob id %d at %s, line %d\n", mob_id, filename[x], count);
					last_mob_id = mob_id;
				}
				continue;
			}
			if( strcmp(sp[1],"clear")==0 ){
				if (mob_id < 0)
					continue;
				memset(mob_db_data[mob_id]->skill,0,sizeof(struct mob_skill));
					mob_db_data[mob_id]->maxskill=0;
				continue;
			}

			if (mob_id < 0)
			{	//Prepare global skill. [Skotlex]
				memset(&gms, 0, sizeof (struct mob_skill));
				ms = &gms;
			} else {			
				for(i=0;i<MAX_MOBSKILL;i++)
					if( (ms=&mob_db_data[mob_id]->skill[i])->skill_id == 0)
						break;
				if(i==MAX_MOBSKILL){
					if (mob_id != last_mob_id) {
						ShowWarning("mob_skill: readdb: too many skill! Line %d in %d[%s]\n",
							count,mob_id,mob_db_data[mob_id]->sprite);
						last_mob_id = mob_id;
					}
					continue;
				}
			}

			ms->state=atoi(sp[2]);
			tmp = sizeof(state)/sizeof(state[0]);
			for(j=0;j<tmp && strcmp(sp[2],state[j].str);j++);
			if (j < tmp)
				ms->state=state[j].id;
			else
				ShowError("mob_skill: Unrecognized state %s at %s, line %d\n", sp[2], filename[x], count);

			//Skill ID
			j=atoi(sp[3]);
			if (j<=0 || j>MAX_SKILL_DB) //fixed Lupus
			{
				if (mob_id < 0)
					ShowWarning("Invalid Skill ID (%d) for all mobs\n", j);
				else
					ShowWarning("Invalid Skill ID (%d) for mob %d (%s)\n", j, mob_id, mob_db_data[mob_id]->sprite);
				continue;
			}
			ms->skill_id=j;
			//Skill lvl
			j= atoi(sp[4])<=0 ? 1 : atoi(sp[4]);
			ms->skill_lv= j>battle_config.mob_max_skilllvl ? battle_config.mob_max_skilllvl : j; //we strip max skill level

			//Apply battle_config modifiers to rate (permillage) and delay [Skotlex]
			tmp = atoi(sp[5]);
			if (battle_config.mob_skill_rate != 100)
				tmp = tmp*battle_config.mob_skill_rate/100;
			if (tmp > 10000)
				ms->permillage= 10000;
			else
				ms->permillage= tmp;
			ms->casttime=atoi(sp[6]);
			ms->delay=atoi(sp[7]);
			if (battle_config.mob_skill_delay != 100)
				ms->delay = ms->delay*battle_config.mob_skill_delay/100;
			if (ms->delay < 0) //time overflow?
				ms->delay = INT_MAX;
			ms->cancel=atoi(sp[8]);
			if( strcmp(sp[8],"yes")==0 ) ms->cancel=1;
			ms->target=atoi(sp[9]);
			for(j=0;j<sizeof(target)/sizeof(target[0]);j++){
				if( strcmp(sp[9],target[j].str)==0)
					ms->target=target[j].id;
			}
			ms->cond1=-1;
			tmp = sizeof(cond1)/sizeof(cond1[0]);
			for(j=0;j<tmp && strcmp(sp[10],cond1[j].str);j++);
			if (j < tmp)
				ms->cond1=cond1[j].id;
			else
				ShowError("mob_skill: Unrecognized condition 1 %s at %s, line %d\n", sp[10], filename[x], count);

			ms->cond2=atoi(sp[11]);
			tmp = sizeof(cond2)/sizeof(cond2[0]);
			for(j=0;j<tmp && strcmp(sp[11],cond2[j].str);j++);
			if (j < tmp)
				ms->cond2=cond2[j].id;
			
			ms->val[0]=atoi(sp[12]);
			ms->val[1]=atoi(sp[13]);
			ms->val[2]=atoi(sp[14]);
			ms->val[3]=atoi(sp[15]);
			ms->val[4]=atoi(sp[16]);
			if(sp[17] != NULL && strlen(sp[17])>2)
				ms->emotion=atoi(sp[17]);
			else
				ms->emotion=-1;
			if (mob_id < 0)
			{	//Set this skill to ALL mobs. [Skotlex]
				mob_id *= -1;
				for (i = 1; i < MAX_MOB_DB; i++)
				{
					if (mob_db_data[i] == NULL)
						continue;
					if (mob_db_data[i]->mode&MD_BOSS)
					{
						if (!(mob_id&2)) //Skill not for bosses
							continue;
					} else
						if (!(mob_id&1)) //Skill not for normal enemies.
							continue;
					
					for(j=0;j<MAX_MOBSKILL;j++)
						if( mob_db_data[i]->skill[j].skill_id == 0)
							break;
					if(j==MAX_MOBSKILL)
						continue;

					memcpy (&mob_db_data[i]->skill[j], ms, sizeof(struct mob_skill));
					mob_db_data[i]->maxskill=j+1;
				}
			} else //Skill set on a single mob.
				mob_db_data[mob_id]->maxskill=i+1;
		}
		fclose(fp);
		ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n",filename[x]);
	}
	return 0;
}
/*==========================================
 * mob_race_db.txt reading
 *------------------------------------------
 */
static int mob_readdb_race(void)
{
	FILE *fp;
	char line[1024];
	int race,j,k;
	char *str[20],*p,*np;

	sprintf(line, "%s/mob_race2_db.txt", db_path);
	if( (fp=fopen(line,"r"))==NULL ){
		ShowError("can't read %s\n", line);
		return -1;
	}
	
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));

		for(j=0,p=line;j<12;j++){
			if((np=strchr(p,','))!=NULL){
				str[j]=p;
				*np=0;
				p=np+1;
			} else
				str[j]=p;
		}
		if(str[0]==NULL)
			continue;

		race=atoi(str[0]);
		if (race < 0 || race >= MAX_MOB_RACE_DB)
			continue;

		for (j=1; j<20; j++) {
			if (!str[j])
				break;
			k=atoi(str[j]);
			if (mob_db(k) == mob_dummy)
				continue;
			mob_db_data[k]->race2 = race;
		}
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","mob_race2_db.txt");
	return 0;
}

#ifndef TXT_ONLY
/*==========================================
 * SQL reading
 *------------------------------------------
 */
static int mob_read_sqldb(void)
{
	const char unknown_str[NAME_LENGTH] ="unknown";
	int i, fi, class_, k;
	double exp, maxhp;
	long unsigned int ln = 0;
	char *mob_db_name[] = { mob_db_db, mob_db2_db };

	//For easier handling of converting. [Skotlex]
#define TO_INT(a) (sql_row[a]==NULL?0:atoi(sql_row[a]))
#define TO_STR(a) (sql_row[a]==NULL?unknown_str:sql_row[a])
	
    for (fi = 0; fi < 2; fi++) {
		sprintf (tmp_sql, "SELECT * FROM `%s`", mob_db_name[fi]);
		if (mysql_query(&mmysql_handle, tmp_sql)) {
			ShowSQL("DB error (%s) - %s\n", mob_db_name[fi], mysql_error(&mmysql_handle));
			ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
			continue;
		}
		sql_res = mysql_store_result(&mmysql_handle);
		if (sql_res) {
			while((sql_row = mysql_fetch_row(sql_res))){
				class_ = TO_INT(0);
				if (class_ <= 1000 || class_ > MAX_MOB_DB)
				{
					ShowWarning("Mob with ID: %d not loaded. ID must be in range [%d-%d]\n", class_, 1000, MAX_MOB_DB);
					continue;
				} else if (pcdb_checkid(class_))
				{
					ShowWarning("Mob with ID: %d not loaded. That ID is reserved for Upper Classes.\n");
					continue;
				}
				if (mob_db_data[class_] == NULL)
					mob_db_data[class_] = aCalloc(1, sizeof (struct mob_data));
				
				ln++;

				mob_db_data[class_]->vd.class_ = class_;
				memcpy(mob_db_data[class_]->sprite, TO_STR(1), NAME_LENGTH-1);
				memcpy(mob_db_data[class_]->name, TO_STR(2), NAME_LENGTH-1);
				memcpy(mob_db_data[class_]->jname, TO_STR(3), NAME_LENGTH-1);
				mob_db_data[class_]->lv = TO_INT(4);
				mob_db_data[class_]->max_hp = TO_INT(5);
				mob_db_data[class_]->max_sp = TO_INT(6);

				exp = (double)TO_INT(7) * (double)battle_config.base_exp_rate / 100.;
				if (exp < 0)
					mob_db_data[class_]->base_exp = 0;
				else if (exp > UINT_MAX)
					mob_db_data[class_]->base_exp = UINT_MAX;
				else
					mob_db_data[class_]->base_exp = (unsigned int)exp;

				exp = (double)TO_INT(8) * (double)battle_config.job_exp_rate / 100.;
				if (exp < 0)
					mob_db_data[class_]->job_exp = 0;
				else if (exp > UINT_MAX)
					mob_db_data[class_]->job_exp = UINT_MAX;
				else
					mob_db_data[class_]->job_exp = (unsigned int)exp;
				
				mob_db_data[class_]->range = TO_INT(9);
				mob_db_data[class_]->atk1 = TO_INT(10);
				mob_db_data[class_]->atk2 = TO_INT(11);
				mob_db_data[class_]->def = TO_INT(12);
				mob_db_data[class_]->mdef = TO_INT(13);
				mob_db_data[class_]->str = TO_INT(14);
				mob_db_data[class_]->agi = TO_INT(15);
				mob_db_data[class_]->vit = TO_INT(16);
				mob_db_data[class_]->int_ = TO_INT(17);
				mob_db_data[class_]->dex = TO_INT(18);
				mob_db_data[class_]->luk = TO_INT(19);
				mob_db_data[class_]->range2 = TO_INT(20);
				mob_db_data[class_]->range3 = TO_INT(21);
				mob_db_data[class_]->size = TO_INT(22);
				mob_db_data[class_]->race = TO_INT(23);
				mob_db_data[class_]->element = TO_INT(24);
				mob_db_data[class_]->mode = TO_INT(25);
				mob_db_data[class_]->speed = TO_INT(26);
				mob_db_data[class_]->adelay = TO_INT(27);
				mob_db_data[class_]->amotion = TO_INT(28);
				mob_db_data[class_]->dmotion = TO_INT(29);

				for (i = 0; i < 10; i++){ // 8 -> 10 Lupus
					int rate = 0, rate_adjust, type;
					unsigned short ratemin, ratemax;
					struct item_data *id;
					mob_db_data[class_]->dropitem[i].nameid=TO_INT(30+i*2);
					if (!mob_db_data[class_]->dropitem[i].nameid) {
						//No drop.
						mob_db_data[class_]->dropitem[i].p = 0;
						continue;
					}
					type = itemdb_type(mob_db_data[class_]->dropitem[i].nameid);
					rate = TO_INT(31+i*2);
					if (class_ >= 1324 && class_ <= 1363)
					{	//Treasure box drop rates [Skotlex]
						rate_adjust = battle_config.item_rate_treasure;
						ratemin = battle_config.item_drop_treasure_min;
						ratemax = battle_config.item_drop_treasure_max;
					}
					else switch(type)
					{
					case 0:							// Added by Valaris
						rate_adjust = battle_config.item_rate_heal;
						ratemin = battle_config.item_drop_heal_min;
						ratemax = battle_config.item_drop_heal_max;
						break;
					case 2:
						rate_adjust = battle_config.item_rate_use;
						ratemin = battle_config.item_drop_use_min;
						ratemax = battle_config.item_drop_use_max;	// End
						break;
					case 4:
					case 5:
					case 8:	// Changed to include Pet Equip
						rate_adjust = battle_config.item_rate_equip;
						ratemin = battle_config.item_drop_equip_min;
						ratemax = battle_config.item_drop_equip_max;
						break;
					case 6:
						rate_adjust = battle_config.item_rate_card;
						ratemin = battle_config.item_drop_card_min;
						ratemax = battle_config.item_drop_card_max;
						break;
					default:
						rate_adjust = battle_config.item_rate_common;
						ratemin = battle_config.item_drop_common_min;
						ratemax = battle_config.item_drop_common_max;
						break;
					}
					mob_db_data[class_]->dropitem[i].p = mob_drop_adjust(rate, rate_adjust, ratemin, ratemax);

					//calculate and store Max available drop chance of the item
					if (mob_db_data[class_]->dropitem[i].p) {
						id = itemdb_search(mob_db_data[class_]->dropitem[i].nameid);
						if (id->maxchance==10000 || (id->maxchance < mob_db_data[class_]->dropitem[i].p) ) {
						//item has bigger drop chance or sold in shops
							id->maxchance = mob_db_data[class_]->dropitem[i].p;
						}			
						for (k = 0; k< MAX_SEARCH; k++) {
							if (id->mob[k].chance < mob_db_data[class_]->dropitem[i].p && id->mob[k].id != class_)
								break;
						}
						if (k == MAX_SEARCH)
							continue;
					
						memmove(&id->mob[k+1], &id->mob[k], (MAX_SEARCH-k-1)*sizeof(id->mob[0]));
						id->mob[k].chance = mob_db_data[class_]->dropitem[i].p;
						id->mob[k].id = class_;
					}
				}
				// MVP EXP Bonus, Chance: MEXP,ExpPer
				mob_db_data[class_]->mexp = TO_INT(50) * battle_config.mvp_exp_rate / 100;
				mob_db_data[class_]->mexpper = TO_INT(51);
				//Now that we know if it is an mvp or not,
				//apply battle_config modifiers [Skotlex]
				maxhp = (double)mob_db_data[class_]->max_hp;
				if (mob_db_data[class_]->mexp > 0)
				{	//Mvp
					if (battle_config.mvp_hp_rate != 100) 
						maxhp = maxhp * (double)battle_config.mvp_hp_rate /100.;
				} else if (battle_config.monster_hp_rate != 100) //Normal mob
					maxhp = maxhp * (double)battle_config.monster_hp_rate /100.;
				if (maxhp < 0) maxhp = 1;
				else if (maxhp > INT_MAX) maxhp = INT_MAX;
				mob_db_data[class_]->max_hp = (int)maxhp;

				// MVP Drops: MVP1id,MVP1per,MVP2id,MVP2per,MVP3id,MVP3per
				for (i=0; i<3; i++) {
					struct item_data *id;
					mob_db_data[class_]->mvpitem[i].nameid = TO_INT(52+i*2);
					if (!mob_db_data[class_]->mvpitem[i].nameid) {
						//No item....
						mob_db_data[class_]->mvpitem[i].p = 0;
						continue;
					}
					mob_db_data[class_]->mvpitem[i].p = mob_drop_adjust(TO_INT(53+i*2),
						battle_config.item_rate_mvp, battle_config.item_drop_mvp_min, battle_config.item_drop_mvp_max);

					//calculate and store Max available drop chance of the MVP item
					id = itemdb_search(mob_db_data[class_]->mvpitem[i].nameid);
					if (mob_db_data[class_]->mvpitem[i].p) {
						if (id->maxchance==10000 || (id->maxchance < mob_db_data[class_]->mvpitem[i].p/10+1) ) {
						//item has bigger drop chance or sold in shops
							id->maxchance = mob_db_data[class_]->mvpitem[i].p/10+1; //reduce MVP drop info to not spoil common drop rate
						}			
					}
				}
				if (mob_db_data[class_]->max_hp <= 0) {
					ShowWarning ("Mob %d (%s) has no HP, using poring data for it\n", class_, mob_db_data[class_]->sprite);
					mob_makedummymobdb(class_);
				}
			}

			mysql_free_result(sql_res);
			ShowStatus("Done reading '"CL_WHITE"%lu"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", ln, mob_db_name[fi]);
			ln = 0;
		}
	}
	return 0;
}
#endif /* not TXT_ONLY */

void mob_reload(void)
{
	int i;
#ifndef TXT_ONLY
    if(db_use_sqldbs)
        mob_read_sqldb();
    else
#endif /* TXT_ONLY */
	mob_readdb();

	mob_readdb_mobavail();
	mob_read_randommonster();

	//Mob skills need to be cleared before re-reading them. [Skotlex]
	for (i = 0; i < MAX_MOB_DB; i++)
		if (mob_db_data[i])
		{
			memset(&mob_db_data[i]->skill,0,sizeof(mob_db_data[i]->skill));
			mob_db_data[i]->maxskill=0;
		}
	mob_readskilldb();
	mob_readdb_race();
}

/*==========================================
 * Circumference initialization of mob
 *------------------------------------------
 */
int do_init_mob(void)
{	//Initialize the mob database
	memset(mob_db_data,0,sizeof(mob_db_data)); //Clear the array
	mob_db_data[0] = aCalloc(1, sizeof (struct mob_data));	//This mob is used for random spawns
	mob_makedummymobdb(0); //The first time this is invoked, it creates the dummy mob
	item_drop_ers = ers_new((uint32)sizeof(struct item_drop));
	item_drop_list_ers = ers_new((uint32)sizeof(struct item_drop_list));

#ifndef TXT_ONLY
    if(db_use_sqldbs)
        mob_read_sqldb();
    else
#endif /* TXT_ONLY */
        mob_readdb();

	mob_readdb_mobavail();
	mob_read_randommonster();
	mob_readskilldb();
	mob_readdb_race();

	add_timer_func_list(mob_delayspawn,"mob_delayspawn");
	add_timer_func_list(mob_delay_item_drop,"mob_delay_item_drop");
	add_timer_func_list(mob_ai_hard,"mob_ai_hard");
	add_timer_func_list(mob_ai_lazy,"mob_ai_lazy");
	add_timer_func_list(mob_timer_delete,"mob_timer_delete");
	add_timer_func_list(mob_spawn_guardian_sub,"mob_spawn_guardian_sub");
	add_timer_func_list(mob_respawn,"mob_respawn");
	add_timer_interval(gettick()+MIN_MOBTHINKTIME,mob_ai_hard,0,0,MIN_MOBTHINKTIME);
	add_timer_interval(gettick()+MIN_MOBTHINKTIME*10,mob_ai_lazy,0,0,MIN_MOBTHINKTIME*10);

	return 0;
}

/*==========================================
 * Clean memory usage.
 *------------------------------------------
 */
int do_final_mob(void)
{
	int i;
	if (mob_dummy)
	{
		aFree(mob_dummy);
		mob_dummy = NULL;
	}
	for (i = 0; i <= MAX_MOB_DB; i++)
	{
		if (mob_db_data[i] != NULL)
		{
			aFree(mob_db_data[i]);
			mob_db_data[i] = NULL;
		}
	}
	ers_destroy(item_drop_ers);
	ers_destroy(item_drop_list_ers);
	return 0;
}
