// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/nullpo.h"
#include "../common/malloc.h"
#include "../common/showmsg.h"
#include "../common/strlib.h"
#include "itemdb.h"
#include "map.h"
#include "battle.h" // struct battle_config
#include "script.h" // item script processing
#include "pc.h"     // W_MUSICAL, W_WHIP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct dbt* item_db;

static struct item_group itemgroup_db[MAX_ITEMGROUP];

struct item_data dummy_item; //This is the default dummy item used for non-existant items. [Skotlex]

/*==========================================
 * 名前で検索用
 *------------------------------------------*/
// name = item alias, so we should find items aliases first. if not found then look for "jname" (full name)
int itemdb_searchname_sub(DBKey key,void *data,va_list ap)
{
	struct item_data *item=(struct item_data *)data,**dst,**dst2;
	char *str;
	str=va_arg(ap,char *);
	dst=va_arg(ap,struct item_data **);
	dst2=va_arg(ap,struct item_data **);
	if(item == &dummy_item) return 0;

	//Absolute priority to Aegis code name.
	if (*dst != NULL) return 0;
	if( strcmpi(item->name,str)==0 )
		*dst=item;

	//Second priority to Client displayed name.
	if (*dst2 != NULL) return 0;
	if( strcmpi(item->jname,str)==0 )
		*dst2=item;
	return 0;
}

/*==========================================
 * 名前で検索
 *------------------------------------------*/
struct item_data* itemdb_searchname(const char *str)
{
	struct item_data *item=NULL, *item2=NULL;

	item_db->foreach(item_db,itemdb_searchname_sub,str,&item,&item2);
	return item?item:item2;
}

static int itemdb_searchname_array_sub(DBKey key,void * data,va_list ap)
{
	struct item_data *item=(struct item_data *)data;
	char *str;
	str=va_arg(ap,char *);
	if (item == &dummy_item)
		return 1; //Invalid item.
	if(stristr(item->jname,str))
		return 0;
	if(stristr(item->name,str))
		return 0;
	return strcmpi(item->jname,str);
}

/*==========================================
 * Founds up to N matches. Returns number of matches [Skotlex]
 *------------------------------------------*/
int itemdb_searchname_array(struct item_data** data, int size, const char *str)
{
	return item_db->getall(item_db,(void**)data,size,itemdb_searchname_array_sub,str);
}


/*==========================================
 * 箱系アイテム検索
 *------------------------------------------*/
int itemdb_searchrandomid(int group)
{
	if(group<1 || group>=MAX_ITEMGROUP) {
		if (battle_config.error_log)
			ShowError("itemdb_searchrandomid: Invalid group id %d\n", group);
		return UNKNOWN_ITEM_ID;
	}
	if (itemgroup_db[group].qty)
		return itemgroup_db[group].nameid[rand()%itemgroup_db[group].qty];
	
	if (battle_config.error_log)
		ShowError("itemdb_searchrandomid: No item entries for group id %d\n", group);
	return UNKNOWN_ITEM_ID;
}

/*==========================================
 * Calculates total item-group related bonuses for the given item
 *------------------------------------------*/
int itemdb_group_bonus(struct map_session_data* sd, int itemid)
{
	int bonus = 0, i, j;
	for (i=0; i < MAX_ITEMGROUP; i++) {
		if (!sd->itemgrouphealrate[i])
			continue;
		for (j=0; j < itemgroup_db[i].qty; j++) {
			if (itemgroup_db[i].nameid[j] == itemid)
			{
				bonus += sd->itemgrouphealrate[i];
				break;
			}
		}
	}
	return bonus;
}

/*==========================================
 * DBの存在確認
 *------------------------------------------*/
struct item_data* itemdb_exists(int nameid)
{
	struct item_data* id;
	if (!nameid) return NULL;
	id = idb_get(item_db,nameid);
	//Adjust nameid in case it's used outside. [Skotlex]
	if (id == &dummy_item)
		dummy_item.nameid = nameid;
	return id;
}

/*==========================================
 * Converts the jobid from the format in itemdb 
 * to the format used by the map server. [Skotlex]
 *------------------------------------------*/
static void itemdb_jobid2mapid(unsigned int *bclass, unsigned int jobmask)
{
	int i;
	bclass[0]= bclass[1]= bclass[2]= 0;
	//Base classes
	if (jobmask & 1<<JOB_NOVICE)
	{	//Both Novice/Super-Novice are counted with the same ID
		bclass[0] |= 1<<MAPID_NOVICE;
		bclass[1] |= 1<<MAPID_NOVICE;
	}
	for (i = JOB_NOVICE+1; i <= JOB_THIEF; i++)
	{
		if (jobmask & 1<<i)
			bclass[0] |= 1<<(MAPID_NOVICE+i);
	}
	//2-1 classes
	if (jobmask & 1<<JOB_KNIGHT)
		bclass[1] |= 1<<MAPID_SWORDMAN;
	if (jobmask & 1<<JOB_PRIEST)
		bclass[1] |= 1<<MAPID_ACOLYTE;
	if (jobmask & 1<<JOB_WIZARD)
		bclass[1] |= 1<<MAPID_MAGE;
	if (jobmask & 1<<JOB_BLACKSMITH)
		bclass[1] |= 1<<MAPID_MERCHANT;
	if (jobmask & 1<<JOB_HUNTER)
		bclass[1] |= 1<<MAPID_ARCHER;
	if (jobmask & 1<<JOB_ASSASSIN)
		bclass[1] |= 1<<MAPID_THIEF;
	//2-2 classes
	if (jobmask & 1<<JOB_CRUSADER)
		bclass[2] |= 1<<MAPID_SWORDMAN;
	if (jobmask & 1<<JOB_MONK)
		bclass[2] |= 1<<MAPID_ACOLYTE;
	if (jobmask & 1<<JOB_SAGE)
		bclass[2] |= 1<<MAPID_MAGE;
	if (jobmask & 1<<JOB_ALCHEMIST)
		bclass[2] |= 1<<MAPID_MERCHANT;
	if (jobmask & 1<<JOB_BARD)
		bclass[2] |= 1<<MAPID_ARCHER;
//	Bard/Dancer share the same slot now.
//	if (jobmask & 1<<JOB_DANCER)
//		bclass[2] |= 1<<MAPID_ARCHER;
	if (jobmask & 1<<JOB_ROGUE)
		bclass[2] |= 1<<MAPID_THIEF;
	//Special classes that don't fit above.
	if (jobmask & 1<<21) //Taekwon boy
		bclass[0] |= 1<<MAPID_TAEKWON;
	if (jobmask & 1<<22) //Star Gladiator
		bclass[1] |= 1<<MAPID_TAEKWON;
	if (jobmask & 1<<23) //Soul Linker
		bclass[2] |= 1<<MAPID_TAEKWON;
	if (jobmask & 1<<JOB_GUNSLINGER)
		bclass[0] |= 1<<MAPID_GUNSLINGER;
	if (jobmask & 1<<JOB_NINJA)
		bclass[0] |= 1<<MAPID_NINJA;
}

static void create_dummy_data(void)
{
	memset(&dummy_item, 0, sizeof(struct item_data));
	dummy_item.nameid=500;
	dummy_item.weight=1;
	dummy_item.value_sell = 1;
	dummy_item.type=3; //Etc item
	strncpy(dummy_item.name,"UNKNOWN_ITEM",ITEM_NAME_LENGTH-1);
	strncpy(dummy_item.jname,"UNKNOWN_ITEM",ITEM_NAME_LENGTH-1);
	dummy_item.view_id = UNKNOWN_ITEM_ID;
}

static void* create_item_data(DBKey key, va_list args)
{
	struct item_data *id;
	id=(struct item_data *)aCalloc(1,sizeof(struct item_data));
	id->nameid = key.i;
	id->weight=1;
	id->type=IT_ETC;
	return id;
}

/*==========================================
 * Loads (and creates if not found) an item from the db.
 *------------------------------------------*/
struct item_data* itemdb_load(int nameid)
{
	struct item_data *id = idb_ensure(item_db,nameid,create_item_data);
	if (id == &dummy_item)
  	{	//Remove dummy_item, replace by real data.
		DBKey key;
		key.i = nameid;
		idb_remove(item_db,nameid);
		id = create_item_data(key, NULL);
		idb_put(item_db,nameid,id);
	}
	return id;
}

static void* return_dummy_data(DBKey key, va_list args)
{
	if (battle_config.error_log)
		ShowWarning("itemdb_search: Item ID %d does not exists in the item_db. Using dummy data.\n", key.i);
	dummy_item.nameid = key.i;
	return &dummy_item;
}

/*==========================================
 * Loads an item from the db. If not found, it will return the dummy item.
 *------------------------------------------*/
struct item_data* itemdb_search(int nameid)
{
	return idb_ensure(item_db,nameid,return_dummy_data);
}

/*==========================================
 * Returns if given item is a player-equippable piece.
 *------------------------------------------*/
int itemdb_isequip(int nameid)
{
	int type=itemdb_type(nameid);
	switch (type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_AMMO:
			return 1;
		default:
			return 0;
	}
}

/*==========================================
 * Alternate version of itemdb_isequip
 *------------------------------------------*/
int itemdb_isequip2(struct item_data *data)
{ 
	nullpo_retr(0, data);
	switch(data->type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_AMMO:
			return 1;
		default:
			return 0;
	}
}

/*==========================================
 * Returns if given item's type is stackable.
 *------------------------------------------*/
int itemdb_isstackable(int nameid)
{
  int type=itemdb_type(nameid);
  switch(type) {
	  case IT_WEAPON:
	  case IT_ARMOR:
	  case IT_PETEGG:
	  case IT_PETARMOR:
		  return 0;
	  default:
		  return 1;
  }
}

/*==========================================
 * Alternate version of itemdb_isstackable
 *------------------------------------------*/
int itemdb_isstackable2(struct item_data *data)
{
  nullpo_retr(0, data);
  switch(data->type) {
	  case IT_WEAPON:
	  case IT_ARMOR:
	  case IT_PETEGG:
	  case IT_PETARMOR:
		  return 0;
	  default:
		  return 1;
  }
}


/*==========================================
 * Trade Restriction functions [Skotlex]
 *------------------------------------------*/
int itemdb_isdropable_sub(struct item_data *item, int gmlv, int unused)
{
	return (item && (!(item->flag.trade_restriction&1) || gmlv >= item->gm_lv_trade_override));
}

int itemdb_cantrade_sub(struct item_data* item, int gmlv, int gmlv2)
{
	return (item && (!(item->flag.trade_restriction&2) || gmlv >= item->gm_lv_trade_override || gmlv2 >= item->gm_lv_trade_override));
}

int itemdb_canpartnertrade_sub(struct item_data* item, int gmlv, int gmlv2)
{
	return (item && (item->flag.trade_restriction&4 || gmlv >= item->gm_lv_trade_override || gmlv2 >= item->gm_lv_trade_override));
}

int itemdb_cansell_sub(struct item_data* item, int gmlv, int unused)
{
	return (item && (!(item->flag.trade_restriction&8) || gmlv >= item->gm_lv_trade_override));
}

int itemdb_cancartstore_sub(struct item_data* item, int gmlv, int unused)
{	
	return (item && (!(item->flag.trade_restriction&16) || gmlv >= item->gm_lv_trade_override));
}

int itemdb_canstore_sub(struct item_data* item, int gmlv, int unused)
{	
	return (item && (!(item->flag.trade_restriction&32) || gmlv >= item->gm_lv_trade_override));
}

int itemdb_canguildstore_sub(struct item_data* item, int gmlv, int unused)
{	
	return (item && (!(item->flag.trade_restriction&64) || gmlv >= item->gm_lv_trade_override));
}

int itemdb_isrestricted(struct item* item, int gmlv, int gmlv2, int (*func)(struct item_data*, int, int))
{
	struct item_data* item_data = itemdb_search(item->nameid);
	int i;

	if (!func(item_data, gmlv, gmlv2))
		return 0;
	
	if(item_data->slot == 0 || itemdb_isspecial(item->card[0]))
		return 1;
	
	for(i = 0; i < item_data->slot; i++) {
		if (!item->card[i]) continue;
		if (!func(itemdb_search(item->card[i]), gmlv, gmlv2))
			return 0;
	}
	return 1;
}

/*==========================================
 *	Specifies if item-type should drop unidentified.
 *------------------------------------------*/
int itemdb_isidentified(int nameid)
{
	int type=itemdb_type(nameid);
	switch (type) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_PETARMOR:
			return 0;
		default:
			return 1;
	}
}

/*==========================================
 * アイテム使用可能フラグのオーバーライド
 *------------------------------------------*/
static int itemdb_read_itemavail (void)
{
	FILE *fp;
	int nameid, j, k, ln = 0;
	char line[1024], *str[10], *p;
	struct item_data *id;

	sprintf(line, "%s/item_avail.txt", db_path);
	if ((fp = fopen(line,"r")) == NULL) {
		ShowError("can't read %s\n", line);
		return -1;
	}

	while(fgets(line, sizeof(line), fp))
	{
		if (line[0] == '/' && line[1] == '/')
			continue;
		memset(str, 0, sizeof(str));
		for (j = 0, p = line; j < 2 && p; j++) {
			str[j] = p;
			p = strchr(p, ',');
			if(p) *p++ = 0;
		}

		if (j < 2 || str[0] == NULL ||
			(nameid = atoi(str[0])) < 0 || !(id = itemdb_exists(nameid)))
			continue;

		k = atoi(str[1]);
		if (k > 0) {
			id->flag.available = 1;
			id->view_id = k;
		} else
			id->flag.available = 0;
		ln++;
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", ln, "item_avail.txt");

	return 0;
}

/*==========================================
 * read item group data
 *------------------------------------------*/
static void itemdb_read_itemgroup_sub(const char* filename)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int groupid,j,k,nameid;
	char *str[3],*p;
	char w1[1024], w2[1024];
	
	if( (fp=fopen(filename,"r"))==NULL ){
		ShowError("can't read %s\n", line);
		return;
	}

	while(fgets(line, sizeof(line), fp))
	{
		ln++;
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(strstr(line,"import")) {
			if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) == 2 &&
				strcmpi(w1, "import") == 0) {
				itemdb_read_itemgroup_sub(w2);
				continue;
			}
		}
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<3 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;
		if (j<3) {
			if (j>1) //Or else it barks on blank lines...
				ShowWarning("itemdb_read_itemgroup: Insufficient fields for entry at %s:%d\n", filename, ln);
			continue;
		}
		groupid = atoi(str[0]);
		if (groupid < 0 || groupid >= MAX_ITEMGROUP) {
			ShowWarning("itemdb_read_itemgroup: Invalid group %d in %s:%d\n", groupid, filename, ln);
			continue;
		}
		nameid = atoi(str[1]);
		if (!itemdb_exists(nameid)) {
			ShowWarning("itemdb_read_itemgroup: Non-existant item %d in %s:%d\n", nameid, filename, ln);
			continue;
		}
		k = atoi(str[2]);
		if (itemgroup_db[groupid].qty+k >= MAX_RANDITEM) {
			ShowWarning("itemdb_read_itemgroup: Group %d is full (%d entries) in %s:%d\n", groupid, MAX_RANDITEM, filename, ln);
			continue;
		}
		for(j=0;j<k;j++)
			itemgroup_db[groupid].nameid[itemgroup_db[groupid].qty++] = nameid;
	}
	fclose(fp);
	return;
}

static void itemdb_read_itemgroup(void)
{
	char path[256];
	snprintf(path, 255, "%s/item_group_db.txt", db_path);

	memset(&itemgroup_db, 0, sizeof(itemgroup_db));
	itemdb_read_itemgroup_sub(path);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n", "item_group_db.txt");
	return;
}

/*==========================================
 * 装備制限ファイル読み出し
 *------------------------------------------*/
static int itemdb_read_noequip(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j;
	char *str[32],*p;
	struct item_data *id;

	sprintf(line, "%s/item_noequip.txt", db_path);
	if( (fp=fopen(line,"r"))==NULL ){
		ShowError("can't read %s\n", line);
		return -1;
	}
	while(fgets(line, sizeof(line), fp))
	{
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<2 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<=0 || !(id=itemdb_exists(nameid)))
			continue;

		id->flag.no_equip=atoi(str[1]);

		ln++;

	}
	fclose(fp);
	if (ln > 0) {
		ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n",ln,"item_noequip.txt");
	}	
	return 0;
}

/*==========================================
 * Reads item trade restrictions [Skotlex]
 *------------------------------------------*/
static int itemdb_read_itemtrade(void)
{
	FILE *fp;
	int nameid, j, flag, gmlv, ln = 0;
	char line[1024], *str[10], *p;
	struct item_data *id;

	sprintf(line, "%s/item_trade.txt", db_path);
	if ((fp = fopen(line,"r")) == NULL) {
		ShowError("can't read %s\n", line);
		return -1;
	}

	while(fgets(line, sizeof(line), fp))
	{
		if (line[0] == '/' && line[1] == '/')
			continue;
		memset(str, 0, sizeof(str));
		for (j = 0, p = line; j < 3 && p; j++) {
			str[j] = p;
			p = strchr(p, ',');
			if(p) *p++ = 0;
		}

		if (j < 3 || str[0] == NULL ||
			(nameid = atoi(str[0])) < 0 || !(id = itemdb_exists(nameid)))
			continue;

		flag = atoi(str[1]);
		gmlv = atoi(str[2]);
		
		if (flag > 0 && flag < 128 && gmlv > 0) { //Check range
			id->flag.trade_restriction = flag;
			id->gm_lv_trade_override = gmlv;
			ln++;
		}
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%d"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", ln, "item_trade.txt");

	return 0;
}

/*======================================
 * Applies gender restrictions according to settings. [Skotlex]
 *======================================*/
static int itemdb_gendercheck(struct item_data *id)
{
	if (id->nameid == WEDDING_RING_M) //Grom Ring
		return 1;
	if (id->nameid == WEDDING_RING_F) //Bride Ring
		return 0;
	if (id->look == W_MUSICAL && id->type == IT_WEAPON) //Musical instruments are always male-only
		return 1;
	if (id->look == W_WHIP && id->type == IT_WEAPON) //Whips are always female-only
		return 0;

	return (battle_config.ignore_items_gender) ? 2 : id->sex;
}

/*==========================================
 * processes one itemdb entry
 *------------------------------------------*/
static bool itemdb_parse_dbrow(char** str, char* source, int line)
{
	/*
		+----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+-------------+---------------+-----------------+--------------+-------------+------------+------+--------+--------------+----------------+
		| 00 |      01      |       02      |  03  |     04    |     05     |   06   |   07   |    08   |   09  |   10  |     11     |      12     |       13      |        14       |      15      |      16     |     17     |  18  |   19   |      20      |        21      |
		+----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+-------------+---------------+-----------------+--------------+-------------+------------+------+--------+--------------+----------------+
		| id | name_english | name_japanese | type | price_buy | price_sell | weight | attack | defence | range | slots | equip_jobs | equip_upper | equip_genders | equip_locations | weapon_level | equip_level | refineable | view | script | equip_script | unequip_script |
		+----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+-------------+---------------+-----------------+--------------+-------------+------------+------+--------+--------------+----------------+
	*/
	unsigned short nameid;
	struct item_data* id;
	
	nameid = atoi(str[0]);
	if(nameid <= 0)
		return false;
	
	//ID,Name,Jname,Type,Price,Sell,Weight,ATK,DEF,Range,Slot,Job,Job Upper,Gender,Loc,wLV,eLV,refineable,View
	id = itemdb_load(nameid);
	safestrncpy(id->name, str[1], ITEM_NAME_LENGTH-1);
	safestrncpy(id->jname, str[2], ITEM_NAME_LENGTH-1);
	
	id->type = atoi(str[3]);
	if (id->type == IT_DELAYCONSUME)
	{	//Items that are consumed only after target confirmation
		id->type = IT_USABLE;
		id->flag.delay_consume = 1;
	} else //In case of an itemdb reload and the item type changed.
		id->flag.delay_consume = 0;

	id->value_buy = atoi(str[4]);
	id->value_sell = atoi(str[5]);
	if (id->value_buy < id->value_sell * 2) id->value_buy = id->value_sell * 2; // prevent exploit
	if (id->value_buy == 0 && id->value_sell > 0) id->value_buy = id->value_sell * 2;
	if (id->value_sell == 0 && id->value_buy > 0) id->value_sell = id->value_buy / 2;
	
	id->weight = atoi(str[6]);
	id->atk = atoi(str[7]);
	id->def = atoi(str[8]);
	id->range = atoi(str[9]);
	id->slot = atoi(str[10]);
	
	if (id->slot > MAX_SLOTS)
	{
		ShowWarning("itemdb_parse_dbrow: Item %d (%s) specifies %d slots, but the server only supports up to %d\n", nameid, id->jname, id->slot, MAX_SLOTS);
		id->slot = MAX_SLOTS;
	}
	
	itemdb_jobid2mapid(id->class_base, (unsigned int)strtoul(str[11],NULL,0));
	id->class_upper = atoi(str[12]);
	id->sex	= atoi(str[13]);
	id->equip = atoi(str[14]);
	
	if (!id->equip && itemdb_isequip2(id))
	{
		ShowWarning("Item %d (%s) is an equipment with no equip-field! Making it an etc item.\n", nameid, id->jname);
		id->type = IT_ETC;
	}
	
	id->wlv = atoi(str[15]);
	id->elv = atoi(str[16]);
	id->flag.no_refine = atoi(str[17]) ? 0 : 1; //FIXME: verify this
	id->look = atoi(str[18]);
	
	id->flag.available = 1;
	id->flag.value_notdc = 0;
	id->flag.value_notoc = 0;
	id->view_id = 0;
	id->sex = itemdb_gendercheck(id); //Apply gender filtering.

	if (id->script)
		script_free_code(id->script);
	if (id->equip_script)
		script_free_code(id->equip_script);
	if (id->unequip_script)
		script_free_code(id->unequip_script);

	if (*str[19])
		id->script = parse_script(str[19], source, line, 0);
	if (*str[20])
		id->equip_script = parse_script(str[20], source, line, 0);
	if (*str[21])
		id->unequip_script = parse_script(str[21], source, line, 0);

	return true;
}

/*==========================================
 * アイテムデータベースの読み込み
 *------------------------------------------*/
static int itemdb_readdb(void)
{
	char* filename[] = { "item_db.txt", "item_db2.txt" };
	int fi;

	for(fi = 0; fi < 2; fi++)
	{
		uint32 lines = 0, count = 0;
		char line[1024];

		char path[256];
		FILE* fp;

		sprintf(path, "%s/%s", db_path, filename[fi]);
		fp = fopen(path, "r");
		if(fp == NULL) {
			if(fi > 0)
				continue;
			return -1;
		}

		// process rows one by one
		while(fgets(line, sizeof(line), fp))
		{
			char *str[32], *p, *np;
			int i;

			lines++;
			if(line[0] == '/' && line[1] == '/')
				continue;
			memset(str, 0, sizeof(str));
			for(i = 0, np = p = line; i < 19 && p; i++)
			{
				str[i] = p;
				if ((p = strchr(p,',')) != NULL) {
					*p++ = '\0'; np = p;
				}
			}

			if (i < 19) {
				ShowWarning("itemdb_readdb: Insufficient columns for item with id %d, skipping.\n", atoi(str[0]));
				continue;
			}

			if((p=strchr(np,'{')) == NULL)
				continue;
			str[19] = p; //Script
			np = strchr(p,'}');
			while (np && np[1] && np[1] != ',')
				np = strchr(np+1,'}'); //Jump close brackets until the next field is found.
			if (!np || !np[1]) {
				continue; //Couldn't find the end of the script field.
			}
			np[1] = '\0'; //Set end of script
			np += 2; //Skip to next field

			if(!np || (p=strchr(np,'{'))==NULL)
				continue;
			str[20] = p; //Equip Script
			np = strchr(p,'}');
			while (np && np[1] && np[1] != ',')
				np = strchr(np+1,'}'); //Jump close brackets until the next field is found.
			if (!np || !np[1]) {
				continue; //Couldn't find the end of the script field.
			}
			np[1] = '\0'; //Set end of script
			np += 2; //Skip comma, to next field

			if(!np || (p=strchr(np,'{'))==NULL)
				continue;
			str[20] = p; //Unequip script, last column.

			if (!itemdb_parse_dbrow(str, filename[fi], lines))
				continue;
			
			count++;
		}

		fclose(fp);

		ShowStatus("Done reading '"CL_WHITE"%lu"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, filename[fi]);
	}

	return 0;
}

#ifndef TXT_ONLY
/*======================================
 * item_db table reading
 *======================================*/
static int itemdb_read_sqldb(void)
{
	char* item_db_name[] = { item_db_db, item_db2_db };
	int fi;
	
	for (fi = 0; fi < 2; fi++)
	{
		uint32 lines = 0, count = 0;
		
		// retrieve all rows from the item database
		if( SQL_ERROR == Sql_Query(mmysql_handle, "SELECT * FROM `%s`", item_db_name[fi]) )
		{
			Sql_ShowDebug(mmysql_handle);
			continue;
		}
		
		// process rows one by one
		while( SQL_SUCCESS == Sql_NextRow(mmysql_handle) )
		{
			// wrap the result into a TXT-compatible format
			char line[1024];
			char* str[22];
			char* p;
			int i;
			
			lines++;
			for (i = 0, p = line; i < 22; i++)
			{
				char* data;
				size_t len;
				Sql_GetData(mmysql_handle, i, &data, &len);
				
				if (data == NULL)
					p[0] = '\0';
				else if (i >= 19 && data[0] != '{') {
					sprintf(p, "{ %s }", data); len+= 4;
				} else
					strcpy(p, data);
				str[i] = p;
				p+= len + 1;
			}
			
			if (!itemdb_parse_dbrow(str, item_db_name[fi], lines))
				continue;
			
			count++;
		}
		
		// free the query result
		Sql_FreeResult(mmysql_handle);
		
		ShowStatus("Done reading '"CL_WHITE"%lu"CL_RESET"' entries in '"CL_WHITE"%s"CL_RESET"'.\n", count, item_db_name[fi]);
	}

	return 0;
}
#endif /* not TXT_ONLY */

/*====================================
 * read all item-related databases
 *------------------------------------*/
static void itemdb_read(void)
{
#ifndef TXT_ONLY
	if (db_use_sqldbs)
		itemdb_read_sqldb();
	else
#endif
		itemdb_readdb();

	itemdb_read_itemgroup();
	itemdb_read_itemavail();
	itemdb_read_noequip();
	itemdb_read_itemtrade();
}

/*==========================================
 * Initialize / Finalize
 *------------------------------------------*/
static int itemdb_final_sub (DBKey key,void *data,va_list ap)
{
	int flag;
	struct item_data *id = (struct item_data *)data;

	flag = va_arg(ap, int);
	if (id->script)
	{
		script_free_code(id->script);
		id->script = NULL;
	}
	if (id->equip_script)
	{
		script_free_code(id->equip_script);
		id->equip_script = NULL;
	}
	if (id->unequip_script)
	{
		script_free_code(id->unequip_script);
		id->unequip_script = NULL;
	}
	// Whether to clear the item data (exception: do not clear the dummy item data
	if (flag && id != &dummy_item) 
		aFree(id);

	return 0;
}

void itemdb_reload(void)
{
	//Just read, the function takes care of freeing scripts.
	itemdb_read();
}

void do_final_itemdb(void)
{
	item_db->destroy(item_db, itemdb_final_sub, 1);
	if (dummy_item.script) {
		script_free_code(dummy_item.script);
		dummy_item.script = NULL;
	}
	if (dummy_item.equip_script) {
		script_free_code(dummy_item.equip_script);
		dummy_item.equip_script = NULL;
	}
	if (dummy_item.unequip_script) {
		script_free_code(dummy_item.unequip_script);
		dummy_item.unequip_script = NULL;
	}
}

int do_init_itemdb(void)
{
	item_db = db_alloc(__FILE__,__LINE__,DB_INT,DB_OPT_BASE,sizeof(int)); 
	create_dummy_data(); //Dummy data item.
	itemdb_read();

	return 0;
}
