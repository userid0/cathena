// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

//#define DEBUG_DISP
//#define DEBUG_DISASM
//#define DEBUG_RUN
//#define DEBUG_HASH
//#define DEBUG_DUMP_STACK

#include "../common/cbasetypes.h"
#include "../common/malloc.h"
#include "../common/md5calc.h"
#include "../common/lock.h"
#include "../common/nullpo.h"
#include "../common/showmsg.h"
#include "../common/strlib.h"
#include "../common/timer.h"
#include "../common/utils.h"
#include "../common/mmo.h"

#include "map.h"
#include "path.h"
#include "clif.h"
#include "chrif.h"
#include "itemdb.h"
#include "pc.h"
#include "status.h"
#include "storage.h"
#include "mob.h"
#include "npc.h"
#include "pet.h"
#include "mapreg.h"
#include "homunculus.h"
#include "instance.h"
#include "mercenary.h"
#include "intif.h"
#include "skill.h"
#include "status.h"
#include "chat.h"
#include "battle.h"
#include "battleground.h"
#include "party.h"
#include "guild.h"
#include "atcommand.h"
#include "log.h"
#include "unit.h"
#include "pet.h"
#include "mail.h"
#include "script.h"
#include "quest.h"
#include "luascript.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
	#include <sys/time.h>
#endif
#include <time.h>
#include <setjmp.h>
#include <errno.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static struct map_session_data* script_get_target(lua_State *NL,int idx);

static int buildin_killmonster_sub(struct block_list *bl,va_list ap);
static int buildin_killmonster_sub_strip(struct block_list *bl,va_list ap);
static int buildin_killmonsterall_sub(struct block_list *bl,va_list ap);
static int buildin_killmonsterall_sub_strip(struct block_list *bl,va_list ap);

extern enum { CLOSE, NRUN, NEXT, INPUT, MENU };

/*-----------------------------------------------------------------------------
 buildin functions for lua
 
 Elements in the lua stack use an index and those elements are pushed in order starting
 from 1. In other words, they're the arguments passed when a script command is used.
 
 Reminder: All lua script commands set the player that called the function as the target.
 lua_get_target is used to do this. If a value is passed in the script for a char id,
 lua_get_target will execute that command under the specified target. If no target
 is supplied, it is executed under the caller of the script.
*/
#define LUA_FUNC(x) static int buildin_ ## x (lua_State *NL)
#define lua_get_target(x) struct map_session_data *sd = script_get_target(NL,x)

//Exits the script
LUA_FUNC(scriptend)
{
	return lua_yield(NL, 0);
}

//Adds a standard NPC that triggers a function when clicked
LUA_FUNC(_addnpc)
{
	char name[NAME_LENGTH],exname[NAME_LENGTH],map[16],function[50];
	short m,x,y,dir,class_;
	sprintf(name, "%s", luaL_checkstring(NL,1));
	sprintf(exname, "%s", luaL_checkstring(NL,2));
	sprintf(map, "%s", luaL_checkstring(NL,3));
	m=map_mapname2mapid(map);
	x=luaL_checkint(NL,4);
	y=luaL_checkint(NL,5);
	dir=luaL_checkint(NL,6);
	class_=luaL_checkint(NL,7);
	sprintf(function, "%s", luaL_checkstring(NL,8));
	return npc_add_lua(name,exname,m,x,y,dir,class_,function);
}

// npcmes("Text",[id])
// Print the text into the NPC dialog window of the player
LUA_FUNC(mes)
{
	char mes[512];
	lua_get_target(2);

	sprintf(mes,"%s",luaL_checkstring(NL, 1));

	clif_scriptmes(sd, sd->npc_id, mes);

	return 0;
}

//Display a NPC menu window asking the player for a value
//npcmenu("menu_name1",return_value1,...)
LUA_FUNC(npcmenu)
{
	struct StringBuf buf;
	char *menu;
	int len=0, n, i;

	lua_get_target(2);
	
	if ( sd->lua_script_state == INPUT || sd->lua_script_state == MENU ) {
		//You shouldn't get another input/menu while already in an input/menu
		lua_pushstring(NL, "Player received npc input request while already in one in function 'npcmenu'\n");
		lua_error(NL);
		return -1;
	}

	lua_pushliteral(NL, "n");
	lua_rawget(NL, 1);
	n = luaL_checkint(NL, -1);
	lua_pop(NL, 1);

	if(n%2 == 1) {
		lua_pushstring(NL, "Incorrect number of arguments for function 'npcmenu'\n");
		lua_error(NL);
		return -1;
	}

	if(!sd->npc_menu_data.current) {
		sd->npc_menu_data.current=0;
	}

	for(i=0; i<n; i+=2) {
		lua_pushinteger(NL, i+1);
		lua_rawget(NL, 1);
		menu = (char *)luaL_checkstring(NL, -1);
		lua_pop(NL, 1);
		len += strlen(menu);
	}

	StringBuf_Init(&buf);

	for(i=0; i<n; i+=2) {
		lua_pushinteger(NL, i+1);
		lua_rawget(NL, 1);
		menu = (char *)luaL_checkstring(NL, -1);
		lua_pop(NL, 1);

		lua_pushinteger(NL, i+2);
		lua_rawget(NL, 1);
		sd->npc_menu_data.value[sd->npc_menu_data.current] = luaL_checkint(NL, -1);
		lua_pop(NL, 1);

		sd->npc_menu_data.id[sd->npc_menu_data.current] = sd->npc_menu_data.current;
		sd->npc_menu_data.current+=1;
		
		StringBuf_AppendStr(&buf, menu);
		StringBuf_AppendStr(&buf, ":");
	}

	clif_scriptmenu(sd,sd->npc_id,StringBuf_Value(&buf));
	StringBuf_Destroy(&buf);
	
	sd->lua_script_state = MENU;
	return lua_yield(NL, 1);
}

//Display a [Next] button in the NPC dialog window
//npcnext(id)
LUA_FUNC(npcnext)
{
	lua_get_target(1);

	clif_scriptnext(sd,sd->npc_id);

	sd->lua_script_state = L_NEXT;
	return lua_yield(NL, 0);
}

// npcclose([id])
// Display a [Close] button in the NPC dialog window of the player and pause the script until the button is clicked
LUA_FUNC(close)
{
	lua_get_target(1);
	
	if (sd == NULL)
		return 0;

	clif_scriptclose(sd,sd->npc_id);

	sd->lua_script_state = L_CLOSE;
	return lua_yield(NL, 0);
}

//Add an invisible area that triggers a function when entered
//addareascript("Area script name","map.gat",x1,y1,x2,y2,"function")
LUA_FUNC(addareascript)
{
	char name[24],map[16],function[50];
	short m,x1,y1,x2,y2;

	sprintf(name,"%s",luaL_checkstring(NL,1));
	sprintf(map,"%s",luaL_checkstring(NL,2));
	m=map_mapname2mapid(map);
	x1=((luaL_checkint(NL,5)>luaL_checkint(NL,3))?luaL_checkint(NL,3):luaL_checkint(NL,5));
	y1=((luaL_checkint(NL,6)>luaL_checkint(NL,4))?luaL_checkint(NL,4):luaL_checkint(NL,6));
	x2=((luaL_checkint(NL,5)>luaL_checkint(NL,3))?luaL_checkint(NL,5):luaL_checkint(NL,3));
	y2=((luaL_checkint(NL,6)>luaL_checkint(NL,4))?luaL_checkint(NL,6):luaL_checkint(NL,4));
	sprintf(function,"%s",luaL_checkstring(NL,7));

	areascript_add_lua(name,m,x1,y1,x2,y2,function);
	return 0;
}

//Add a warp that moves players somewhere else when entered
// addwarp("Warp name","map.gat",x,y,"destmap.gat",destx,desty,xradius,yradius)
LUA_FUNC(addwarp)
{
	char name[24],map[16],destmap[16];
	short m,x,y;
	short destx,desty,xs,ys;
	
	sprintf(name,"%s",luaL_checkstring(NL,1));
	sprintf(map,"%s",luaL_checkstring(NL,2));
	m=map_mapname2mapid(map);
	x=luaL_checkint(NL,3);
	y=luaL_checkint(NL,4);
	sprintf(destmap,"%s",luaL_checkstring(NL,5));
	destx=luaL_checkint(NL,6);
	desty=luaL_checkint(NL,7);
	xs=luaL_checkint(NL,8);
	ys=luaL_checkint(NL,9);

	npc_warp_add_lua(name,m,x,y,destmap,destx,desty,xs,ys);

	return 0;
}

//Add a monster spawn
LUA_FUNC(addspawn)
{
	char name[24],map[16],function[50];
	short m,x,y,xs,ys,class_,num,d1,d2;

	sprintf(name,"%s",luaL_checkstring(NL,1));
	sprintf(map,"%s",luaL_checkstring(NL,2));
	m=map_mapname2mapid(map);
	x=luaL_checkint(NL,3);
	y=luaL_checkint(NL,4);
	xs=luaL_checkint(NL,5);
	ys=luaL_checkint(NL,6);
	class_=luaL_checkint(NL,7);
	num=luaL_checkint(NL,8);
	d1=luaL_checkint(NL,9);
	d2=luaL_checkint(NL,10);
	sprintf(function,"%s",luaL_checkstring(NL,11));

	npc_add_mob_lua(name,m,x,y,xs,ys,class_,num,d1,d2,function);

	return 0;
}

//Display a NPC input window asking the player for a value
//npcinput(type,[id])
LUA_FUNC(input)
{
	lua_get_target(2);
	int type;

	type = luaL_checkint(NL,1);

	switch(type){
		case 0:
			clif_scriptinput(sd,sd->npc_id);
			break;
		case 1:
			clif_scriptinputstr(sd,sd->npc_id);
			break;
	}
	
	//TODO: there's a sd->state.menu_or_input for this
	sd->lua_script_state = L_INPUT;
	return lua_yield(NL, 1);
}

//Start a npc shop
//npcshop(item_id1,item_price1,...)
LUA_FUNC(npcshop)
{
	int n, i, j;

	lua_get_target(2);

	lua_pushliteral(NL, "n");
	lua_rawget(NL, 1);
	n = luaL_checkint(NL, -1);
	lua_pop(NL, 1);

	if(n%2 == 1) {
		lua_pushstring(NL, "Incorrect number of arguments for function 'npcshop'\n");
		lua_error(NL);
		return -1;
	}

	sd->shop_data.n = n/2;

	for(i=1, j=0; i<=n; i+=2, j++) {
		lua_pushinteger(NL, i);
		lua_rawget(NL, 1);
		sd->shop_data.nameid[j] = luaL_checkint(NL, -1);
		lua_pop(NL, 1);

		lua_pushinteger(NL, i+1);
		lua_rawget(NL, 1);
		sd->shop_data.value[j] = luaL_checkint(NL, -1);
		lua_pop(NL, 1);
	}

	/* Needs cash shop support
	if ( nd->subtype == CASHSHOP )
		clif_cashshop_show(sd, sd->npc_id);
	else*/
		clif_npcbuysell(sd, sd->npc_id);

	//sd->npc_shopid = nd->bl.id
	//May need this later
	sd->lua_script_state = SHOP;
	return lua_yield(NL, 1);
}

//Display a cutin picture on the screen
//npccutin(name,type,[id])
LUA_FUNC(cutin)
{
	lua_get_target(3);
	char name[50];
	int type;

	sprintf(name, "%s", luaL_checkstring(NL,1));
	type = luaL_checkint(NL, 2);

	clif_cutin(sd,name,type);

	return 0;
}

//Heals the character by a set amount of HP and SP
//npcheal(hp,sp,[id])
LUA_FUNC(heal)
{
	lua_get_target(3);
	int hp, sp;

	hp = luaL_checkint(NL, 1);
	sp = luaL_checkint(NL, 2);

	pc_heal(sd, hp, sp, 0);

	return 0;
}

//Heals the character by a percentage of their Max HP / SP
//npcpercentheal(hp,sp,[id])
LUA_FUNC(percentheal)
{
	lua_get_target(3);
	int hp, sp;

	hp = luaL_checkint(NL, 1);
	sp = luaL_checkint(NL, 2);

	pc_percentheal(sd, hp, sp);

	return 0;
}

//Heal the character by an amount that increases with VIT/INT, skills, etc
//npcitemheal(hp,sp,[id]);
LUA_FUNC(itemheal)
{
	lua_get_target(3);
	int hp, sp;

	hp = luaL_checkint(NL, 1);
	sp = luaL_checkint(NL, 2);
	
	if ( sd == NULL )
		return 0;

	if ( potion_flag == 1 )
	{
		potion_hp = hp;
		potion_sp = sp;
		return 0;
	}
	
	pc_itemheal(sd, sd->itemid, hp, sp);

	return 0;
}

//Change the job of the character
//npcjobchange(job_id,[id])
LUA_FUNC(jobchange)
{
	lua_get_target(2);
	int job;

	job = luaL_checkint(NL, 1);

	pc_jobchange(sd,job,0);

	return 0;
}

//Change the look of the character
//npcsetlook(type,val,[id])
LUA_FUNC(setlook)
{
	lua_get_target(3);
	int type,val;

	type = luaL_checkint(NL, 1);
	val = luaL_checkint(NL, 2);

	pc_changelook(sd,type,val);

	return 0;
}

//Warp a player to a set location
//warp("map",x,y,[id])
LUA_FUNC(warp)
{
	lua_get_target(4);
	char str[16];
	int x, y;

	sprintf(str, "%s", luaL_checkstring(NL,1));
	x = luaL_checkint(NL, 2);
	y = luaL_checkint(NL, 3);
	
	if ( sd == NULL )
		return 0;

	if(strcmp(str,"Random")==0) // Warp to random location
		pc_randomwarp(sd,3);
	else if(strcmp(str,"SavePoint")==0 || strcmp(str,"Save")==0) { // Warp to save point
		pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
	} else // Warp to given location
		pc_setpos(sd,mapindex_name2id(str),x,y,0);

	return 0;
}

//returns the name of a given job using the msg_athena entries 550-650
//jobname(job_number)
LUA_FUNC(getjobname)
{
	int class_ = luaL_checkint(NL,1); // receive job number from script
	lua_pushstring(NL,job_name(class_)); // push job name to the stack
	return 1;
}

//same as setlook but only client side
//npcchangelook(type,val,[id])
LUA_FUNC(changelook)
{
	lua_get_target(3);
	int type,val;

	type = luaL_checkint(NL, 1);
	val = luaL_checkint(NL, 2);

	clif_changelook(&sd->bl,type,val);

	return 0;
}

//returns the amount of item ID in a player's posession.
//if it fails, the function returns nil
//getitemcount(item_id,[char_id])
LUA_FUNC(getitemcount)
{
	int nameid, i;
	int count = 0;
	lua_get_target(2);
	
	if (!sd) //Return nil if no player exists
		return 0;
	
	nameid = luaL_checkint(NL,1); //attempt to get the item by id
	if ( !nameid ) //if it's null, it's not an integer. it's treated as a string now while the item is searched
	{
		const char* name =luaL_checkstring(NL,1);
		struct item_data* item_data;
		if ( (item_data = itemdb_searchname(name)) != NULL )
			nameid = item_data->nameid;
		else
			nameid = 0; //the string search failed too.
	}
	
	if ( nameid < 500 ) {
		ShowError("wrong item ID: countitem(%i)\n", nameid);
		return 0;
	}
	
	for(i = 0; i < MAX_INVENTORY; i++)
		if(sd->status.inventory[i].nameid == nameid)
				count += sd->status.inventory[i].amount;
				
	lua_pushinteger(NL,count);
	return 1;
}

LUA_FUNC(getitemcount2)
{
	//countitem2
	return 0;
}


LUA_FUNC(getweight)
{
	int nameid = 0, amount, i;
	unsigned long weight;
	lua_get_target(3);	
	if ( sd == NULL ) {
		return 0;
	}
	if ( lua_isstring(NL,1) )
	{
		const char *name = luaL_checkstring(NL,1);
		struct item_data *item_data = itemdb_searchname(name);
		if ( item_data )
			nameid = item_data->nameid;
	} else {
		nameid = luaL_checkint(NL,1);
	}
		
	amount = luaL_checkint(NL,2);
	if ( amount <= 0 || nameid < 500 )
		return 0;
	
	weight = itemdb_weight(nameid)*amount;
	if ( amount > MAX_AMOUNT || weight + sd->weight > sd->max_weight )
		lua_pushinteger(NL,0);
		
	else if ( itemdb_isstackable(nameid) )
	{
		if ( ( i = pc_search_inventory(sd,nameid)) >= 0 )
			lua_pushinteger(NL,amount + sd->status.inventory[i].amount > MAX_AMOUNT ? 0 : 1);
		else
			lua_pushinteger(NL,pc_search_inventory(sd,0) >= 0 ? 1 : 0);
	}
	else
	{
		for( i = 0; i < MAX_INVENTORY && amount; ++i )
			if ( sd->status.inventory[i].nameid == 0 )
				amount--;
		lua_pushinteger(NL,amount ? 0 : 1);
	}
	
	return 1;
}

//giveitem(item id, amount[,character id];
LUA_FUNC(giveitem)
{
	int nameid,amount,get_count,i,flag = 0;
	struct item it;
	lua_get_target(3);
	if( lua_isstring(NL,1) )
	{
		const char *name = luaL_checkstring(NL,1);
		struct item_data *item_data = itemdb_searchname(name);
		if ( item_data == NULL )
		{
			return 0;
		}
		nameid = item_data->nameid;
	} else {
		nameid = luaL_checkint(NL,1);
		if ( nameid < 0 )
		{
			nameid = itemdb_searchrandomid(-nameid);
			flag = 1;
		}
		if ( nameid <= 0 || !itemdb_exists(nameid) )
		{
			return 0;
		}
	}
	// <amount>
	if( (amount=luaL_checkint(NL,2)) <= 0) {
		return 0; //return if amount <=0, skip the useles iteration
	}
	memset(&it,0,sizeof(it));
	it.nameid=nameid;
	if(!flag) {
		it.identify=1;
	}
	else {
		it.identify=itemdb_isidentified(nameid);
	}
	//Check if it's stackable.
	if (!itemdb_isstackable(nameid)) {
		get_count = 1;
	}
	else {
		get_count = amount;
	}
	for (i = 0; i < amount; i += get_count)
	{
		// if not pet egg
		if (!pet_create_egg(sd, nameid))
		{
			if ((flag = pc_additem(sd, &it, get_count)))
			{
				clif_additem(sd, 0, 0, flag);
				if( pc_candrop(sd,&it) )
					map_addflooritem(&it,get_count,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0);
			}
                }
        }
	//Logs items, got from (N)PC scripts [Lupus]
	if(log_config.enable_logs&LOG_SCRIPT_TRANSACTIONS) {
		log_pick_pc(sd, "N", nameid, amount, NULL);
	}
	lua_pushinteger(NL,1);
	return 1;
}

LUA_FUNC(giveitem2)
{
	return 0;
}

LUA_FUNC(getparam)
{
	int type;
	lua_get_target(2);
	
	type = luaL_checkint(NL,1);
	
	if ( sd == NULL )
		return 0;
		
	lua_pushinteger(NL,pc_readparam(sd,type));
	return 1;
}

LUA_FUNC(getrentalitem)
{
	lua_get_target(3);
	struct item it;
	int seconds;
	int nameid = 0, flag;

	if( sd == NULL )
		return 0;

	if( lua_isstring(NL,1) )
	{
		const char *name = luaL_checkstring(NL,1);
		struct item_data *itd = itemdb_searchname(name);
		if( itd == NULL )
		{
			ShowError("buildin_getrentalitem: Nonexistant item %s requested.\n", name);
			return 0;
		}
		nameid = itd->nameid;

	}
	else {
		nameid = luaL_checkint(NL,1);
		if( nameid <= 0 || !itemdb_exists(nameid) )
		{
			ShowError("buildin_getrentalitem: Nonexistant item %d rquested.\n", nameid);
			return 0;
		}
	}

	seconds = luaL_checkint(NL,2);
	memset(&it, 0, sizeof(it));
	it.nameid = nameid;
	it.identify = 1;
	it.expire_time = (unsigned int)(time(NULL) + seconds);

	if( (flag = pc_additem(sd, &it, 1)) )
	{
		clif_additem(sd, 0, 0, flag);
		return 0;
	}

	clif_rental_time(sd->fd, nameid, seconds);
	pc_inventory_rental_add(sd, seconds);

	if( log_config.enable_logs&LOG_SCRIPT_TRANSACTIONS )
		log_pick_pc(sd, "N", nameid, 1, NULL);

	lua_pushinteger(NL,1);
	return 1;
}

LUA_FUNC(getcharid)
{
	int num;
	struct map_session_data *sd;
	
	num = luaL_checkint(NL,1);
	
	sd = map_nick2sd((const char*)lua_tostring(NL,2));
	
	if ( sd == NULL )
	{
		sd = script_get_target(NL,2);
	}

	switch( num )
	{
		case 0:	
			lua_pushinteger(NL, sd->status.char_id);
			return 1;
		case 1: 
			lua_pushinteger(NL, sd->status.party_id);
			return 1;
		case 2: 
			lua_pushinteger(NL, sd->status.guild_id);
			return 1;
		case 3: 
			lua_pushinteger(NL, sd->status.account_id);
			return 1;
		case 4: 
			lua_pushinteger(NL, sd->state.bg_id); 
			return 1;
		default:
			return 0;
	}
	return 1;
}

static char *buildin_getpartyname_sub(int party_id)
{
	struct party_data *p;

	p=party_search(party_id);

	if(p!=NULL){
		char *buf;
		buf=(char *)aMallocA(NAME_LENGTH*sizeof(char));
		memcpy(buf, p->party.name, NAME_LENGTH);
		buf[NAME_LENGTH-1] = '\0';
		return buf;
	}

	return 0;
}

LUA_FUNC(getpartyname)
{
	char *name;
	int party_id;
	lua_get_target(2);
	
	party_id = luaL_checkint(NL,1);
	
	name = buildin_getpartyname_sub(party_id);
	
	if ( name != NULL )
	{
		lua_pushstring(NL,name);
		return 1;
	}
	
	return 0;
}

LUA_FUNC(getpartymember)
{
	struct party_data *p;
	int i,j=0,n;

	p=party_search(luaL_checkint(NL,1));
	
	if(p!=NULL){
		for(i=0;i<MAX_PARTY;i++){
			if(p->party.member[i].account_id){
				switch (luaL_checkint(NL,2)) {
				case 2:
					lua_newtable(NL);
					for(n=0; n < MAX_PARTY; n++)
					{
						lua_pushinteger(NL,p->party.member[i].account_id);
						lua_rawseti(NL,-2,n + 1);
					}
					break;
				case 1:
					lua_newtable(NL);
					for(n=0; n < MAX_PARTY; n++)
					{
						lua_pushinteger(NL,p->party.member[i].char_id);
						lua_rawseti(NL,-2,n + 1);
					}
					break;
				default:
					lua_newtable(NL);
					for(n=0; n < MAX_PARTY; n++)
					{
						lua_pushstring(NL,p->party.member[i].name);
						lua_rawseti(NL,-2,n + 1);
					}
				}
				j++;
			}
		}
		return 1;
	}
	else
		return 0;
}

LUA_FUNC(itemuseenable)
{
	lua_get_target(1);
	
	if ( sd ) {
		sd->npc_item_flag = sd->npc_id;
		lua_pushinteger(NL,1);
		return 1;
	}
	
	return 0;
}

LUA_FUNC(itemusedisable)
{
	lua_get_target(1);
	
	if ( sd )
	{
		sd->npc_item_flag = 0;
		lua_pushinteger(NL,1);
		return 1;
	}
	
	return 0;
}

LUA_FUNC(givenameditem)
{
	int nameid;
	struct item item_tmp;
	lua_get_target(3);
	struct map_session_data *tsd;
	
	if( lua_isstring(NL,1) )
	{
		const char *name = (const char*)luaL_checkstring(NL,1);
		struct item_data *item_data = itemdb_searchname(name);
		if( item_data == NULL)
		{	//Failed
			return 0;
		}
		nameid = item_data->nameid;
	}else
		nameid = luaL_checkint(NL,1);

	if(!itemdb_exists(nameid))
	{	//Even though named stackable items "could" be risky, they are required for certain quests.
		return 0;
	}
		
	if ( lua_isstring(NL,2) )
	{
		tsd = map_nick2sd(luaL_checkstring(NL,2));
	}
	else
		tsd = map_charid2sd(luaL_checkint(NL,2));
	
	if( tsd == NULL )
	{	//Failed
		return 0;
	}

	memset(&item_tmp,0,sizeof(item_tmp));
	item_tmp.nameid=nameid;
	item_tmp.amount=1;
	item_tmp.identify=1;
	item_tmp.card[0]=CARD0_CREATE; //we don't use 255! because for example SIGNED WEAPON shouldn't get TOP10 BS Fame bonus [Lupus]
	item_tmp.card[2]=tsd->status.char_id;
	item_tmp.card[3]=tsd->status.char_id >> 16;
	if(pc_additem(sd,&item_tmp,1)) {
		return 0;	//Failed to add item, we will not drop if they don't fit
	}

	//Logs items, got from (N)PC scripts [Lupus]
	if(log_config.enable_logs&0x40)
		log_pick_pc(sd, "N", item_tmp.nameid, item_tmp.amount, &item_tmp);

	lua_pushinteger(NL,1);
	return 1;
}

LUA_FUNC(getrandgroupitem)
{
	lua_pushinteger(NL,itemdb_searchrandomid(luaL_checkint(NL,1)));
	return 1;
}

LUA_FUNC(additem)
{
	int nameid,amount,flag = 0;
	int x,y,m;
	const char *mapname;
	struct item item_tmp;

	if( lua_isstring(NL,1) )
	{
		const char *name = luaL_checkstring(NL,1);
		struct item_data *item_data = itemdb_searchname(name);
		if( item_data )
			nameid = item_data->nameid;
		else
			nameid = UNKNOWN_ITEM_ID;
	}else
		nameid = luaL_checkint(NL,1);

	amount = luaL_checkint(NL,2);
	mapname = (const char*)lua_tostring(NL,3);
	x = luaL_checkint(NL,4);
	y = luaL_checkint(NL,5);

	if(strcmp(mapname,"this")==0)
	{
		lua_get_target(6);
		if (!sd) return 0; //Failed...
		m=sd->bl.m;
	} else
		m=map_mapname2mapid(mapname);

	if(nameid<0) { // ランダム
		nameid=itemdb_searchrandomid(-nameid);
		flag = 1;
	}

	if(nameid > 0) {
		memset(&item_tmp,0,sizeof(item_tmp));
		item_tmp.nameid=nameid;
		if(!flag)
			item_tmp.identify=1;
		else
			item_tmp.identify=itemdb_isidentified(nameid);

		map_addflooritem(&item_tmp,amount,m,x,y,0,0,0,0);
	}

	lua_pushinteger(NL,1);
	return 1;
}

LUA_FUNC(deleteitem)
{
	int nameid=0,amount,i,important_item=0;
	lua_get_target(3);

	if( sd == NULL )
	{
		return 0;
	}
	
	if( lua_isstring(NL,1) )
	{
		const char* item_name = luaL_checkstring(NL,1);
		struct item_data* id = itemdb_searchname(item_name);
		if( id == NULL )
		{
			sd->lua_script_state = L_CLOSE;
			return 0;
		}
		nameid = id->nameid;
	}
	else
		nameid = luaL_checkint(NL,1);

	amount = luaL_checkint(NL,2);

	if( amount <= 0 )
		return 0;

	//1st pass
	//here we won't delete items with CARDS, named items but we count them
	for(i=0;i<MAX_INVENTORY;i++){
		//we don't delete wrong item or equipped item
		if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL ||
		   sd->status.inventory[i].amount<=0 || sd->status.inventory[i].nameid!=nameid)
			continue;
		//1 egg uses 1 cell in the inventory. so it's ok to delete 1 pet / per cycle
		if(sd->inventory_data[i]->type==IT_PETEGG &&
			sd->status.inventory[i].card[0] == CARD0_PET)
		{
			if (!intif_delete_petdata(MakeDWord(sd->status.inventory[i].card[1], sd->status.inventory[i].card[2])))
				continue; //pet couldn't be sent for deletion.
		} else
		//is this item important? does it have cards? or Player's name? or Refined/Upgraded
		if(itemdb_isspecial(sd->status.inventory[i].card[0]) ||
			sd->status.inventory[i].card[0] ||
		  	sd->status.inventory[i].refine) {
			//this is important item, count it (except for pet eggs)
			if(sd->status.inventory[i].card[0] != CARD0_PET)
				important_item++;
			continue;
		}

		if(sd->status.inventory[i].amount>=amount){

			//Logs items, got from (N)PC scripts
			if(log_config.enable_logs&0x40)
				log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -amount, &sd->status.inventory[i]);

			pc_delitem(sd,i,amount,0);
			lua_pushinteger(NL,1);
			return 1;
		} else {
			amount-=sd->status.inventory[i].amount;

			//Logs items, got from (N)PC scripts
			if(log_config.enable_logs&0x40) {
				log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -sd->status.inventory[i].amount, &sd->status.inventory[i]);
			}
			//Logs

			pc_delitem(sd,i,sd->status.inventory[i].amount,0);
		}
	}
	//2nd pass
	//now if there WERE items with CARDs/REFINED/NAMED... and if we still have to delete some items. we'll delete them finally
	if (important_item>0 && amount>0)
		for(i=0;i<MAX_INVENTORY;i++){
			//we don't delete wrong item
			if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL ||
				sd->status.inventory[i].amount<=0 || sd->status.inventory[i].nameid!=nameid )
				continue;

			if(sd->status.inventory[i].amount>=amount){

				//Logs items, got from (N)PC scripts
				if(log_config.enable_logs&0x40)
					log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -amount, &sd->status.inventory[i]);

				pc_delitem(sd,i,amount,0);
				lua_pushinteger(NL,1);
				return 1; //we deleted exact amount of items. now exit
			} else {
				amount-=sd->status.inventory[i].amount;

				//Logs items, got from (N)PC scripts
				if(log_config.enable_logs&0x40)
					log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -sd->status.inventory[i].amount, &sd->status.inventory[i]);

				pc_delitem(sd,i,sd->status.inventory[i].amount,0);
			}
		}

	sd->lua_script_state = L_CLOSE;
	return 1;
}

LUA_FUNC(deleteitem2)
{
	int nameid=0,amount,i=0;
	int iden,ref,attr,c1,c2,c3,c4;
	lua_get_target(10);

	if ( sd == NULL )
		return 0;

	if( lua_isstring(NL,1) )
	{
		const char* item_name = luaL_checkstring(NL,1);
		struct item_data* id = itemdb_searchname(item_name);
		if( id == NULL )
		{
			sd->lua_script_state = L_CLOSE;
			return 1;
		}
		nameid = id->nameid;// "<item name>"
	}
	else
		nameid = luaL_checkint(NL,1);// <item id>

	amount = luaL_checkint(NL,2);
	iden = luaL_checkint(NL,3);
	ref = luaL_checkint(NL,4);
	attr = luaL_checkint(NL,5);
	c1 = luaL_checkint(NL,6);
	c2 = luaL_checkint(NL,7);
	c3 = luaL_checkint(NL,8);
	c4 = luaL_checkint(NL,9);
	
	if( amount <= 0 )
		return 0;// nothing to do

	for(i=0;i<MAX_INVENTORY;i++){
	//we don't delete wrong item or equipped item
		if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL ||
			sd->status.inventory[i].amount<=0 || sd->status.inventory[i].nameid!=nameid ||
			sd->status.inventory[i].identify!=iden || sd->status.inventory[i].refine!=ref ||
			sd->status.inventory[i].attribute!=attr || sd->status.inventory[i].card[0]!=c1 ||
			sd->status.inventory[i].card[1]!=c2 || sd->status.inventory[i].card[2]!=c3 ||
			sd->status.inventory[i].card[3]!=c4)
			continue;
	//1 egg uses 1 cell in the inventory. so it's ok to delete 1 pet / per cycle
		if(sd->inventory_data[i]->type==IT_PETEGG && sd->status.inventory[i].card[0] == CARD0_PET)
		{
			if (!intif_delete_petdata( MakeDWord(sd->status.inventory[i].card[1], sd->status.inventory[i].card[2])))
				continue; //Failed to send delete the pet.
		}

		if(sd->status.inventory[i].amount>=amount){

			//Logs items, got from (N)PC scripts [Lupus]
			if(log_config.enable_logs&0x40)
				log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -amount, &sd->status.inventory[i]);

			pc_delitem(sd,i,amount,0);
			lua_pushinteger(NL,1);
			return 1; //we deleted exact amount of items. now exit
		} else {
			amount-=sd->status.inventory[i].amount;

			//Logs items, got from (N)PC scripts [Lupus]
			if(log_config.enable_logs&0x40)
				log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -sd->status.inventory[i].amount, &sd->status.inventory[i]);

			pc_delitem(sd,i,sd->status.inventory[i].amount,0);
		}
	}

	sd->lua_script_state = L_CLOSE;
	return 1;
}

LUA_FUNC(getpartyleader)
{
	int party_id, type = 0, i=0;
	struct party_data *p;

	party_id = luaL_checkint(NL,1);
	type = luaL_checkint(NL,2);

	p=party_search(party_id);

	if (p) //Search leader
	for(i = 0; i < MAX_PARTY && !p->party.member[i].leader; i++);

	if (!p || i == MAX_PARTY) { //leader not found
			return 0;
	}

	switch (type) {
		case 1: lua_pushinteger(NL,p->party.member[i].account_id); break;
		case 2: lua_pushinteger(NL,p->party.member[i].char_id); break;
		case 3: lua_pushinteger(NL,p->party.member[i].class_); break;
		case 4: lua_pushstring(NL,mapindex_id2name(p->party.member[i].map)); break;
		case 5: lua_pushinteger(NL,p->party.member[i].lv); break;
		default: lua_pushstring(NL,p->party.member[i].name); break;
	}
	return 1;
}

static char *buildin_getguildname_sub(int guild_id)
{
	struct guild *g=NULL;
	g=guild_search(guild_id);

	if(g!=NULL){
		char *buf;
		buf=(char *)aMallocA(NAME_LENGTH*sizeof(char));
		memcpy(buf, g->name, NAME_LENGTH);
		buf[NAME_LENGTH-1] = '\0';
		return buf;
	}
	return NULL;
}

static char *buildin_getguildmaster_sub(int guild_id)
{
	struct guild *g=NULL;
	g=guild_search(guild_id);

	if(g!=NULL){
		char *buf;
		buf=(char *)aMallocA(NAME_LENGTH*sizeof(char));
		memcpy(buf, g->master, NAME_LENGTH);
		buf[NAME_LENGTH-1] = '\0';
		return buf;
	}

	return 0;
}

LUA_FUNC(getguildname)
{
	char *name;
	int guild_id=luaL_checkint(NL,1);
	name=buildin_getguildname_sub(guild_id);
	
	name = buildin_getguildname_sub(guild_id);
	
	if(name != NULL) {
		lua_pushstring(NL,name);
		return 1;
	}
	
	return 0;
}

LUA_FUNC(getguildmaster)
{
	char *master;
	int guild_id=luaL_checkint(NL,1);
	master=buildin_getguildmaster_sub(guild_id);
	if(master!=0) {
		lua_pushstring(NL,master);
		return 1;
	}
	return 0;
}

LUA_FUNC(getguildmasterid)
{
	char *master;
	TBL_PC *sd=NULL;
	int guild_id=luaL_checkint(NL,1);
	master=buildin_getguildmaster_sub(guild_id);
	if(master!=0){
		if((sd=map_nick2sd(master)) == NULL){
			return 0;
		}
		lua_pushinteger(NL,sd->status.char_id);
		return 1;
	}
	return 0;
}

LUA_FUNC(strcharinfo)
{
	lua_get_target(2);
	int num;
	char *buf;

	if ( sd == NULL )
		return 0;
		
	num = luaL_checkint(NL,1);
	switch(num){
		case 0:
			lua_pushstring(NL,sd->status.name);
			break;
		case 1:
			buf=buildin_getpartyname_sub(sd->status.party_id);
			if(buf!=0)
				lua_pushstring(NL,buf);
			else
				return 0;
			break;
		case 2:
			buf=buildin_getguildname_sub(sd->status.guild_id);
			if(buf != NULL)
				lua_pushstring(NL,buf);
			else
				return 0;
			break;
		case 3:
			lua_pushstring(NL,map[sd->bl.m].name);
			break;
		default:
			return 0;
	}

	return 1;
}

LUA_FUNC(strnpcinfo)
{
	TBL_NPC* nd;
	int num;
	int oid = luaL_checkint(NL,2);
	char *name=NULL;

	nd = map_id2nd(oid);
	
	if (!nd)
		return 0;

	num = luaL_checkint(NL,1);
	switch(num){
		case 0: // display name
			name = aStrdup(nd->name);
			break;
		case 1: // unique name
			name = aStrdup(nd->exname);
			break;
		default:
			break;
	}

	if(name) {
		lua_pushstring(NL,name);
		return 1;
	}

	return 0;
}

// aegis->athena slot position conversion table
static unsigned int equip[] = {EQP_HEAD_TOP,EQP_ARMOR,EQP_HAND_L,EQP_HAND_R,EQP_GARMENT,EQP_SHOES,EQP_ACC_L,EQP_ACC_R,EQP_HEAD_MID,EQP_HEAD_LOW};
LUA_FUNC(getequipid)
{
	int i, num;
	lua_get_target(2);
	struct item_data* item;

	num = luaL_checkint(NL,1) - 1;
	
	if( num < 0 || num >= ARRAYLENGTH(equip) )
		return 0;

	// get inventory position of item
	i = pc_checkequip(sd,equip[num]);
	if( i < 0 )
		return 0;
		
	item = sd->inventory_data[i];
	if( item != 0 )
		lua_pushinteger(NL,item->nameid);

	return 1;
}

LUA_FUNC(getequipname)
{
	int i, num;
	lua_get_target(2);
	struct item_data* item;
	
	num = luaL_checkint(NL,1) - 1;
	if( num < 0 || num >= ARRAYLENGTH(equip) )
		return 0;

	// get inventory position of item
	i = pc_checkequip(sd,equip[num]);
	if( i < 0 )
		return 0;

	item = sd->inventory_data[i];
	
	if( item != 0 )
		lua_pushstring(NL,item->jname);
	else
		return 0;

	return 1;
}

LUA_FUNC(getbrokenid)
{
	int i,num;
	lua_get_target(2);
	int id=0;
	int brokencounter=0;

	num=luaL_checkint(NL,1);
	for(i=0; i<MAX_INVENTORY; i++) {
		if(sd->status.inventory[i].attribute){
				brokencounter++;
				if(num==brokencounter){
					id=sd->status.inventory[i].nameid;
					break;
				}
		}
	}

	lua_pushinteger(NL,id);

	return 1;
}

LUA_FUNC(repair)
{
	int i,num;
	int repaircounter=0;
	lua_get_target(2);

	num=luaL_checkint(NL,1);
	for(i=0; i<MAX_INVENTORY; i++) {
		if(sd->status.inventory[i].attribute){
				repaircounter++;
				if(num==repaircounter){
					sd->status.inventory[i].attribute=0;
					clif_equiplist(sd);
					clif_produceeffect(sd, 0, sd->status.inventory[i].nameid);
					clif_misceffect(&sd->bl, 3);
					break;
				}
		}
	}

	return 0;
}

LUA_FUNC(getequipisenableref)
{
	int i=-1,num;
	lua_get_target(2);

	num=luaL_checkint(NL,1);

	if( num > 0 && num <= ARRAYLENGTH(equip) )
		i = pc_checkequip(sd,equip[num-1]);
	if( i >= 0 && sd->inventory_data[i] && !sd->inventory_data[i]->flag.no_refine && !sd->status.inventory[i].expire_time )
		lua_pushinteger(NL,1);
	else
		return 0;

	return 1;
}

LUA_FUNC(getequiprefinerycnt)
{
	int i=-1,num;
	lua_get_target(2);

	num=luaL_checkint(NL,1);

	if (num > 0 && num <= ARRAYLENGTH(equip))
		i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0)
		lua_pushinteger(NL,sd->status.inventory[i].refine);
	else
		return 0;

	return 1;
}

LUA_FUNC(getequipweaponlv)
{
	int i=-1,num;
	lua_get_target(2);
	num=luaL_checkint(NL,1);
	if (num > 0 && num <= ARRAYLENGTH(equip)) {
		i=pc_checkequip(sd,equip[num-1]);
	}
	if(i >= 0 && sd->inventory_data[i]) {
		lua_pushinteger(NL,sd->inventory_data[i]->wlv);
		return 1;
	}
	return 0;
}

LUA_FUNC(getequippercentrefinery)
{
	int i=-1,num;
	lua_get_target(2);
	num=luaL_checkint(NL,1);
	if (num > 0 && num <= ARRAYLENGTH(equip)) {
		i=pc_checkequip(sd,equip[num-1]);
	}
	if(i >= 0 && sd->status.inventory[i].nameid && sd->status.inventory[i].refine < MAX_REFINE) {
		lua_pushinteger(NL,percentrefinery[itemdb_wlv(sd->status.inventory[i].nameid)][(int)sd->status.inventory[i].refine]);
	}
	else
		return 0;
	return 1;
}

LUA_FUNC(successrefitem)
{
	int i=-1,num,ep;
	lua_get_target(2);
	num=luaL_checkint(NL,1);
	if (num > 0 && num <= ARRAYLENGTH(equip))
		i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0) {
		ep=sd->status.inventory[i].equip;
		//Logs items, got from (N)PC scripts
		if(log_config.enable_logs&0x40) {
			log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -1, &sd->status.inventory[i]);
		}
		sd->status.inventory[i].refine++;
		pc_unequipitem(sd,i,2); // status calc will happen in pc_equipitem() below
		clif_refine(sd->fd,0,i,sd->status.inventory[i].refine);
		clif_delitem(sd,i,1);
		//Logs items, got from (N)PC scripts
		if(log_config.enable_logs&0x40) {
			log_pick_pc(sd, "N", sd->status.inventory[i].nameid, 1, &sd->status.inventory[i]);
		}
		clif_additem(sd,i,1,0);
		pc_equipitem(sd,i,ep);
		clif_misceffect(&sd->bl,3);
		if(sd->status.inventory[i].refine == MAX_REFINE &&
			sd->status.inventory[i].card[0] == CARD0_FORGE &&
		  	sd->status.char_id == (int)MakeDWord(sd->status.inventory[i].card[2],sd->status.inventory[i].card[3])
		){ // Fame point system
	 		switch (sd->inventory_data[i]->wlv){
				case 1:
					pc_addfame(sd,1); // Success to refine to +10 a lv1 weapon you forged = +1 fame point
					break;
				case 2:
					pc_addfame(sd,25); // Success to refine to +10 a lv2 weapon you forged = +25 fame point
					break;
				case 3:
					pc_addfame(sd,1000); // Success to refine to +10 a lv3 weapon you forged = +1000 fame point
					break;
	 	 	 }
		}
	}

	return 0;
}

LUA_FUNC(failedrefitem)
{
	int i=-1,num;
	lua_get_target(2);
	num=luaL_checkint(NL,1);
	if (num > 0 && num <= ARRAYLENGTH(equip))
		i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0) {
		//Logs items, got from (N)PC scripts
		if(log_config.enable_logs&0x40)
			log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -1, &sd->status.inventory[i]);

		sd->status.inventory[i].refine = 0;
		pc_unequipitem(sd,i,3);
		clif_refine(sd->fd,1,i,sd->status.inventory[i].refine);

		pc_delitem(sd,i,1,0);
		clif_misceffect(&sd->bl,2);
	}
	return 0;
}

LUA_FUNC(statusup)
{
	int type;
	lua_get_target(2);
	type=luaL_checkint(NL,1);
	pc_statusup(sd,type);
	return 0;
}

LUA_FUNC(statusup2)
{
	int type,val;
	lua_get_target(3);
	type=luaL_checkint(NL,1);
	val=luaL_checkint(NL,2);
	pc_statusup2(sd,type,val);
	return 0;
}

LUA_FUNC(bonus)
{
	int type;
	int val1;
	int val2 = 0;
	int val3 = 0;
	int val4 = 0;
	int val5 = 0;
	lua_get_target(7);
	type=luaL_checkint(NL,1);
	switch( type )
	{
	case SP_AUTOSPELL:
	case SP_AUTOSPELL_WHENHIT:
	case SP_AUTOSPELL_ONSKILL:
	case SP_SKILL_ATK:
	case SP_SKILL_HEAL:
	case SP_SKILL_HEAL2:
	case SP_ADD_SKILL_BLOW:
	case SP_CASTRATE:
	case SP_ADDEFF_ONSKILL:
		// these bonuses support skill names
		val1=( lua_isstring(NL,2) ? skill_name2id(luaL_checkstring(NL,2)) : luaL_checkint(NL,2) );
		break;
	default:
		val1=luaL_checkint(NL,2);
		break;
	}
	switch( lua_tointeger(NL,-1) )
	{
	case 1:
		pc_bonus(sd, type, val1);
		break;
	case 2:	
		val2 = luaL_checkint(NL,3);
		pc_bonus2(sd, type, val1, val2);
		break;
	case 3:
		val2 = luaL_checkint(NL,3);
		val3 = luaL_checkint(NL,4);
		pc_bonus3(sd, type, val1, val2, val3);
		break;
	case 4:
		if( type == SP_AUTOSPELL_ONSKILL && lua_isstring(NL,3) )
			val2 = skill_name2id(luaL_checkstring(NL,3)); // 2nd value can be skill name
		else
			val2 = luaL_checkint(NL,3);
		val3 = luaL_checkint(NL,4);
		val4 = luaL_checkint(NL,5);
		pc_bonus4(sd, type, val1, val2, val3, val4);
		break;
	case 5:
		if( type == SP_AUTOSPELL_ONSKILL && lua_isstring(NL,3) )
			val2 = skill_name2id(luaL_checkstring(NL,3)); // 2nd value can be skill name
		else
			val2 = luaL_checkint(NL,3);
		val3 = luaL_checkint(NL,4);
		val4 = luaL_checkint(NL,5);
		val5 = luaL_checkint(NL,6);
		pc_bonus5(sd, type, val1, val2, val3, val4, val5);
		break;
	default:
		break;
	}
	return 0;
}

LUA_FUNC(autobonus)
{
	unsigned int dur;
	short rate;
	short atk_type = 0;
	lua_get_target(6);
	const char *bonus_script, *other_script = NULL;
	if( sd->state.autobonus&sd->status.inventory[current_equip_item_index].equip )
		return 0;
	rate=luaL_checkint(NL,2);
	dur=luaL_checkint(NL,3);
	bonus_script=luaL_checkstring(NL,1);
	if( !rate || !dur || !bonus_script )
		return 0;
	if( lua_tointeger(NL,4) )
		atk_type=luaL_checkint(NL,4);
	if( lua_tostring(NL,5) )
		other_script=luaL_checkstring(NL,5);
	if( pc_addautobonus(sd->autobonus,ARRAYLENGTH(sd->autobonus),bonus_script,rate,dur,atk_type,other_script,sd->status.inventory[current_equip_item_index].equip,false) )
	{
		script_add_autobonus(bonus_script);
		if( other_script )
			script_add_autobonus(other_script);
	}
	return 0;
}

LUA_FUNC(autobonus2)
{
	unsigned int dur;
	short rate;
	short atk_type = 0;
	lua_get_target(6);
	const char *bonus_script, *other_script = NULL;
	if( sd->state.autobonus&sd->status.inventory[current_equip_item_index].equip )
		return 0;
	rate=luaL_checkint(NL,2);
	dur=luaL_checkint(NL,3);
	bonus_script=luaL_checkstring(NL,1);
	if( !rate || !dur || !bonus_script )
		return 0;
	if( lua_tointeger(NL,4) )
		atk_type=luaL_checkint(NL,4);
	if( lua_tostring(NL,5) )
		other_script=luaL_checkstring(NL,5);
	if( pc_addautobonus(sd->autobonus2,ARRAYLENGTH(sd->autobonus2),
		bonus_script,rate,dur,atk_type,other_script,sd->status.inventory[current_equip_item_index].equip,false) )
	{
		script_add_autobonus(bonus_script);
		if( other_script )
			script_add_autobonus(other_script);
	}
	return 0;
}

LUA_FUNC(autobonus3)
{
	unsigned int dur;
	short rate,atk_type;
	lua_get_target(6);
	const char *bonus_script, *other_script = NULL;
	if( sd->state.autobonus&sd->status.inventory[current_equip_item_index].equip ) {
		return 0;
	}
	rate = luaL_checkint(NL,2);
	dur = luaL_checkint(NL,3);
	atk_type = ( lua_isstring(NL,4) ? skill_name2id(luaL_checkstring(NL,4)) : luaL_checkint(NL,4) );
	bonus_script = luaL_checkstring(NL,1);
	if( !rate || !dur || !atk_type || !bonus_script ) {
		return 0;
	}
	if( lua_isstring(NL,5) ) {
		other_script = luaL_checkstring(NL,5);
	}
	if( pc_addautobonus(sd->autobonus3,ARRAYLENGTH(sd->autobonus3),bonus_script,rate,dur,atk_type,other_script,sd->status.inventory[current_equip_item_index].equip,true) )
	{
		script_add_autobonus(bonus_script);
		if( other_script )
		{
			script_add_autobonus(other_script);
		}
	}
	return 0;
}

LUA_FUNC(skill)
{
	int id;
	int level;
	int flag = 1;
	lua_get_target(4);
	id = ( lua_isstring(NL,1) ? skill_name2id(luaL_checkstring(NL,1)) : luaL_checkint(NL,1) );
	level = luaL_checkint(NL,2);
	if( lua_tointeger(NL,3) )
		flag = luaL_checkint(NL,3);
	pc_skill(sd, id, level, flag);
	return 0;
}


LUA_FUNC(guildskill)
{
	int id;
	int level;
	int i;
	lua_get_target(3);
	id = ( lua_isstring(NL,1) ? skill_name2id(luaL_checkstring(NL,1)) : luaL_checkint(NL,1) );
	level = luaL_checkint(NL,2);
	for( i=0; i < level; i++ )
	{
		guild_skillup(sd, id);
	}
	return 0;
}

LUA_FUNC(getskilllv)
{
	int id;
	lua_get_target(2);
	id = ( lua_isstring(NL,1) ? skill_name2id(luaL_checkstring(NL,1)) : luaL_checkint(NL,2) );
	lua_pushinteger(NL, pc_checkskill(sd,id));
	return 1;
}

LUA_FUNC(getgdskilllv)
{
	int guild_id;
	int skill_id;
	struct guild* g;
	guild_id = luaL_checkint(NL,1);
	skill_id = ( lua_isstring(NL,2) ? skill_name2id(luaL_checkstring(NL,2)) : luaL_checkint(NL,2) );
	g = guild_search(guild_id);
	if( g == NULL ) {
		return 0;
	}
	else {
		lua_pushinteger(NL, guild_checkskill(g,skill_id));
	}	
	return 1;
}

LUA_FUNC(basicskillcheck)
{
	lua_pushinteger(NL, battle_config.basic_skill_check);
	return 1;
}

LUA_FUNC(getgmlevel)
{
	lua_get_target(1);
	lua_pushinteger(NL, pc_isGM(sd));
	return 1;
}

LUA_FUNC(checkoption)
{
	int option;
	lua_get_target(2);
	option = luaL_checkint(NL,1);
	if( sd->sc.option&option ) {
		lua_pushinteger(NL, 1);
		return 1;
	}
	return 0;
}

LUA_FUNC(checkoption1)
{
	int opt1;
	lua_get_target(2);
	opt1 = luaL_checkint(NL,1);
	if( sd->sc.opt1 == opt1 ) {
		lua_pushinteger(NL, 1);
		return 1;
	}
	return 0;
}

LUA_FUNC(checkoption2)
{
	int opt2;
	lua_get_target(2);
	opt2 = luaL_checkint(NL,1);
	if( sd->sc.opt2&opt2 ) {
		lua_pushinteger(NL, 1);
		return 1;
	}
	return 0;
}

LUA_FUNC(setoption)
{
	int option;
	int flag = 1;
	lua_get_target(3);
	option = luaL_checkint(NL,1);
	if( lua_tonumber(NL,2) )
		flag = luaL_checkint(NL,2);
	else if( !option ){// Request to remove everything.
		flag = 0;
		option = OPTION_CART|OPTION_FALCON|OPTION_RIDING;
	}
	if( flag ){// Add option
		if( option&OPTION_WEDDING && !battle_config.wedding_modifydisplay )
			option &= ~OPTION_WEDDING;// Do not show the wedding sprites
		pc_setoption(sd, sd->sc.option|option);
	} else// Remove option
		pc_setoption(sd, sd->sc.option&~option);
	return 0;
}

LUA_FUNC(checkcart)
{
	lua_get_target(1);
	if( pc_iscarton(sd) ) {
		lua_pushinteger(NL, 1);
		return 1;
	}
	return 0;
}

LUA_FUNC(setcart)
{
	int type=luaL_checkint(NL,1);
	lua_get_target(2);
	pc_setcart(sd,type);
	return 0;
}

LUA_FUNC(checkfalcon)
{
	lua_get_target(1);
	if( pc_isfalcon(sd) ) {
		lua_pushinteger(NL,1);
		return 1;
	}
	return 0;
}

LUA_FUNC(setfalcon)
{
	int flag = luaL_checkint(NL,1);
	lua_get_target(2);
	pc_setfalcon(sd, flag);
	return 0;
}

LUA_FUNC(checkriding)
{
	lua_get_target(1);
	if( pc_isriding(sd) ) {
		lua_pushinteger(NL, 1);
		return 1;
	}
	return 0;
}

LUA_FUNC(setriding)
{
	int flag = luaL_checkint(NL,1);
	lua_get_target(2);
	pc_setriding(sd, flag);
	return 0;
}

LUA_FUNC(savepoint)
{
	int x;
	int y;
	short map;
	const char* str;
	lua_get_target(4);
	str = luaL_checkstring(NL,1);
	x   = luaL_checkint(NL,2);
	y   = luaL_checkint(NL,3);
	map = mapindex_name2id(str);
	if( map ) {
		pc_setsavepoint(sd, map, x, y);
	}
	return 0;
}

LUA_FUNC(gettimetick)
{
	int type;
	time_t timer;
	struct tm *t;
	type=luaL_checkint(NL,1);
	switch(type){
	case 2: 
		//type 2:(Get the number of seconds elapsed since 00:00 hours, Jan 1, 1970 UTC
		//        from the system clock.)
		lua_pushinteger(NL,(int)time(NULL));
		break;
	case 1:
		//type 1:(Second Ticks: 0-86399, 00:00:00-23:59:59)
		time(&timer);
		t=localtime(&timer);
		lua_pushinteger(NL,((t->tm_hour)*3600+(t->tm_min)*60+t->tm_sec));
		break;
	default:
		//type 0:(System Ticks)
		lua_pushinteger(NL,gettick());
		break;
	}
	return 1;
}

LUA_FUNC(gettime)
{
	int type=luaL_checkint(NL,1);
	time_t timer;
	struct tm *t;
	time(&timer);
	t=localtime(&timer);
	switch(type){
	case 1://Sec(0~59)
		lua_pushinteger(NL,t->tm_sec);
		break;
	case 2://Min(0~59)
		lua_pushinteger(NL,t->tm_min);
		break;
	case 3://Hour(0~23)
		lua_pushinteger(NL,t->tm_hour);
		break;
	case 4://WeekDay(0~6)
		lua_pushinteger(NL,t->tm_wday);
		break;
	case 5://MonthDay(01~31)
		lua_pushinteger(NL,t->tm_mday);
		break;
	case 6://Month(01~12)
		lua_pushinteger(NL,t->tm_mon+1);
		break;
	case 7://Year(20xx)
		lua_pushinteger(NL,t->tm_year+1900);
		break;
	case 8://Year Day(01~366)
		lua_pushinteger(NL,t->tm_yday+1);
		break;
	default://(format error)
		return 0;
	}
	return 1;
}

LUA_FUNC(gettimestr)
{
	char *tmpstr;
	const char *fmtstr=luaL_checkstring(NL,1);
	int maxlen=luaL_checkint(NL,2);
	time_t now = time(NULL);
	tmpstr=(char *)aMallocA((maxlen+1)*sizeof(char));
	strftime(tmpstr,maxlen,fmtstr,localtime(&now));
	tmpstr[maxlen]='\0';
	lua_pushstring(NL,tmpstr);
	return 1;
}

LUA_FUNC(openstorage)
{
	lua_get_target(1);
	storage_storageopen(sd);
	return 0;
}

LUA_FUNC(guildopenstorage)
{
	lua_get_target(1);
	int ret;
	ret = storage_guild_storageopen(sd);
	lua_pushinteger(NL,ret);
	return 1;
}

LUA_FUNC(itemskill)
{
	int id;
	int lv;
	lua_get_target(3);
	if( sd->ud.skilltimer != INVALID_TIMER ) {
		return 0;
	}
	id = ( lua_isstring(NL,1) ? skill_name2id(luaL_checkstring(NL,1)) : luaL_checkint(NL,1) );
	lv = luaL_checkint(NL,2);
	sd->skillitem=id;
	sd->skillitemlv=lv;
	clif_item_skill(sd,id,lv);
	return 0;
}

LUA_FUNC(produce)
{
	int trigger;
	lua_get_target(2);
	trigger=luaL_checkint(NL,1);
	clif_skill_produce_mix_list(sd, trigger);
	return 0;
}

LUA_FUNC(cooking)
{
	int trigger;
	lua_get_target(2);
	trigger=luaL_checkint(NL,1);
	clif_cooking_list(sd, trigger);
	return 0;
}

LUA_FUNC(makepet)
{
	int id,pet_id;
	lua_get_target(2);
	id=luaL_checkint(NL,1);
	pet_id = search_petDB_index(id, PET_CLASS);
	if (pet_id < 0) {
		pet_id = search_petDB_index(id, PET_EGG);
	}
	if (pet_id >= 0 && sd) {
		sd->catch_target_class = pet_db[pet_id].class_;
		intif_create_pet(
			sd->status.account_id, sd->status.char_id,
			(short)pet_db[pet_id].class_, (short)mob_db(pet_db[pet_id].class_)->lv,
			(short)pet_db[pet_id].EggID, 0, (short)pet_db[pet_id].intimate,
			100, 0, 1, pet_db[pet_id].jname);
	}
	return 0;
}

LUA_FUNC(getexp)
{
	int base=0,job=0;
	double bonus;
	lua_get_target(3);
	base=luaL_checkint(NL,1);
	job=luaL_checkint(NL,2);
	if(base<0 || job<0) {
		return 0;
	}
	// bonus for npc-given exp
	bonus = battle_config.quest_exp_rate / 100.;
	base = (int) cap_value(base * bonus, 0, INT_MAX);
	job = (int) cap_value(job * bonus, 0, INT_MAX);
	pc_gainexp(sd, NULL, base, job, true);
	return 0;
}

LUA_FUNC(guildgetexp)
{
	lua_get_target(2);
	int exp;
	exp=luaL_checkint(NL,1);
	if(exp < 0) {
		return 0;
	}
	if(sd && sd->status.guild_id > 0) {
		guild_getexp (sd, exp);
	}
	return 0;
}

LUA_FUNC(guildchangegm)
{
	lua_get_target(3);
	int guild_id;
	const char *name;
	guild_id=luaL_checkint(NL,1);
	name=luaL_checkstring(NL,2);
	sd=map_nick2sd(name);
	if (!sd) {
		return 0;
	}
	else {
		lua_pushinteger(NL,guild_gm_change(guild_id, sd));
	}
	return 1;
}

LUA_FUNC(monster)
{
	lua_get_target(8);
	int m;
	char function[50];
	const char* mapn  = luaL_checkstring(NL,1);
	int x             = luaL_checkint(NL,2);
	int y             = luaL_checkint(NL,3);
	const char* str   = luaL_checkstring(NL,4);
	int class_        = luaL_checkint(NL,5);
	int amount        = luaL_checkint(NL,6);
	sprintf(function, "%s", lua_tostring(NL,7));
	if (class_ >= 0 && !mobdb_checkid(class_)) {
		//ShowWarning("buildin_monster: Attempted to spawn non-existing monster class %d\n", class_);
		return 0;
	}
	if( sd && strcmp(mapn,"this") == 0 )
		m = sd->bl.m;
/* [TODO] Instance Support
	else
	{
		m = map_mapname2mapid(mapn);
		if( map[m].flag.src4instance && st->instance_id )
		{ // Try to redirect to the instance map, not the src map
			if( (m = instance_mapid2imapid(m, st->instance_id)) < 0 )
			{
				//ShowError("buildin_monster: Trying to spawn monster (%d) on instance map (%s) without instance attached.\n", class_, mapn);
				return 0;
			}
		}
	}
*/
	mob_once_spawn(sd,m,x,y,str,class_,amount,function,1);
	return 0;
}

LUA_FUNC(getmobdrops)
{
	int class_ = luaL_checkint(NL,1);
	int i, j = 0;
	struct item_data *i_data;
	struct mob_db *mob;
	if( !mobdb_checkid(class_) )
	{
		return 0;
	}
	mob = mob_db(class_);
	lua_newtable(NL);
	for( i = 0; i < MAX_MOB_DROP; i++ )
	{
		if( mob->dropitem[i].nameid < 1 ) {
			continue;
		}
		if( (i_data = itemdb_exists(mob->dropitem[i].nameid)) == NULL ) {
			continue;		
		}
		lua_pushinteger(NL,mob->dropitem[i].nameid);
		lua_rawseti(NL,-2,i + 1);
		j++;
	}
	lua_pushinteger(NL,j);
	lua_rawseti(NL,-1,i+1);
	return 1;
}

LUA_FUNC(areamonster)
{
	lua_get_target(10);
	int m;
	char function[50];
	const char* mapn  = luaL_checkstring(NL,1);
	int x0            = luaL_checkint(NL,2);
	int y0            = luaL_checkint(NL,3);
	int x1            = luaL_checkint(NL,4);
	int y1            = luaL_checkint(NL,5);
	const char* str   = luaL_checkstring(NL,6);
	int class_        = luaL_checkint(NL,7);
	int amount        = luaL_checkint(NL,8);
	sprintf(function, "%s", lua_tostring(NL,9));
	if( sd && strcmp(mapn,"this") == 0 )
		m = sd->bl.m;
/*[TODO]Instance Support
	else
	{
		m = map_mapname2mapid(mapn);
		if( map[m].flag.src4instance && st->instance_id )
		{ // Try to redirect to the instance map, not the src map
			if( (m = instance_mapid2imapid(m, st->instance_id)) < 0 )
			{
				ShowError("buildin_areamonster: Trying to spawn monster (%d) on instance map (%s) without instance attached.\n", class_, mapn);
				return 1;
			}
		}
	}
*/	
	mob_once_spawn_area(sd,m,x0,y0,x1,y1,str,class_,amount,function,1);
	return 0;
}

LUA_FUNC(killmonster)
{
	const char *mapname,*event;
	int m,allflag=0;
	mapname=luaL_checkstring(NL,1);
	event=luaL_checkstring(NL,2);
	if(strcmp(event,"All")==0) {
		allflag = 1;
	}
	if( (m=map_mapname2mapid(mapname))<0 )
		return 0;
	//TODO : instance support	
	//if( map[m].flag.src4instance && st->instance_id && (m = instance_mapid2imapid(m, st->instance_id)) < 0 )
	//	return 0;
	if( lua_isnumber(NL,3) ) {
		if ( lua_tointeger(NL,3) == 1 ) {
			map_foreachinmap(buildin_killmonster_sub, m, BL_MOB, event ,allflag);
			return 0;
		}
	}
	map_freeblock_lock();
	map_foreachinmap(buildin_killmonster_sub_strip, m, BL_MOB, event ,allflag);
	map_freeblock_unlock();
	return 0;
}

static int buildin_killmonster_sub(struct block_list *bl,va_list ap)
{
	TBL_MOB* md = (TBL_MOB*)bl;
	char *event=va_arg(ap,char *);
	int allflag=va_arg(ap,int);

	if(!allflag){
		if(!md->function[0])
			status_kill(bl);
	}else{
		if(!md->spawn)
			status_kill(bl);
	}
	return 0;
}

 static int buildin_killmonster_sub_strip(struct block_list *bl,va_list ap)
{ //same fix but with killmonster instead - stripping function calls from mobs.
	//depending on what param was passed in killmonster(), the mobs have to be killed without triggering their functions.
	TBL_MOB* md = (TBL_MOB*)bl;
	char *event=va_arg(ap,char *);
	int allflag=va_arg(ap,int);

	md->state.npc_killmonster = 1;
	
	if(!allflag){
		if(!md->function[0])
			status_kill(bl);
	}else{
		if(!md->spawn)
			status_kill(bl);
	}
	md->state.npc_killmonster = 0;
	return 0;
}

LUA_FUNC(killmonsterall)
{
	const char *mapname;
	int m;
	mapname=luaL_checkstring(NL,1);
	if( (m = map_mapname2mapid(mapname))<0 ) {
		return 0;
	}
	//TODO Instance Support
	//if( map[m].flag.src4instance && st->instance_id && (m = instance_mapid2imapid(m, st->instance_id)) < 0 )
	//	return 0;
	if(lua_isnumber(NL,2) && lua_isnumber(NL,2) == 1) {
		map_foreachinmap(buildin_killmonsterall_sub,m,BL_MOB);
		return 0;
	}
	map_foreachinmap(buildin_killmonsterall_sub_strip,m,BL_MOB);
	return 0;
}

static int buildin_killmonsterall_sub(struct block_list *bl,va_list ap)
{
	status_kill(bl);
	return 0;
}

static int buildin_killmonsterall_sub_strip(struct block_list *bl,va_list ap)
{ //Strips the event from the mob if it's killed the old method.
	struct mob_data *md;
	
	md = BL_CAST(BL_MOB, bl);
	if (md->npc_event[0]) {
		md->npc_event[0] = 0;
	}
	if(md->function[0]) {
		md->function[0] = 0;
	}
	status_kill(bl);
	return 0;
}

LUA_FUNC(clone)
{
	TBL_PC *sd, *msd=NULL;
	int char_id,master_id=0,x,y, mode = 0, flag = 0, m;
	unsigned int duration = 0;
	const char *map,*event="";
	map=luaL_checkstring(NL,1);
	x=luaL_checkint(NL,2);
	y=luaL_checkint(NL,3);
	event=luaL_checkstring(NL,4);
	char_id=luaL_checkint(NL,5);
	if(lua_isnumber(NL,6)) {
		master_id=luaL_checkint(NL,6);
	}
	if(lua_isnumber(NL,7)) {
		mode=luaL_checkint(NL,7);
	}
	if(lua_isnumber(NL,8)) {
		flag=luaL_checkint(NL,8);
	}
	if(lua_isnumber(NL,9)) {
		duration=luaL_checkint(NL,9);
	}	
	m = map_mapname2mapid(map);
	if (m < 0) return 0;
	sd = map_charid2sd(char_id);
	if (master_id) {
		msd = map_charid2sd(master_id);
		if (msd) {
			master_id = msd->bl.id;
		}
		else {
			master_id = 0;
		}
	}
	if (sd) {//Return ID of newly crafted clone.
		lua_pushinteger(NL,mob_clone_spawn(sd, m, x, y, event, master_id, mode, flag, 1000*duration,1));
		return 1;
	}
	return 0;
}

LUA_FUNC(addtimer)
{
	lua_get_target(3);
	int tick = luaL_checkint(NL,1);
	const char* function = luaL_checkstring(NL,2);
	add_timer_lua(pc_addeventtimer(sd,tick,function));
	lua_pushinteger(NL,tick);
	return 1;
}

LUA_FUNC(deltimer)
{
	const char *function;
	lua_get_target(2);
	function=luaL_checkstring(NL,1);
	pc_deleventtimer(sd,function);
	return 0;
}

LUA_FUNC(addtimercount)
{
	const char *function;
	int tick;
	lua_get_target(3);
	function=luaL_checkstring(NL,1);
	tick=luaL_checkint(NL,2);
	pc_addeventtimercount(sd,function,tick);
	return 0;
}

LUA_FUNC(addnpctimer)
{
	int tick = luaL_checkint(NL,1);
	const char* function = luaL_checkstring(NL,2);
	npc_add_lua_timer(tick,function);
	lua_pushinteger(NL,tick);
	return 1;
}

LUA_FUNC(delnpctimer)
{
	const char *function = luaL_checkstring(NL,1);
	npc_del_lua_eventtimer(function);
	return 0;
}

LUA_FUNC(addnpctimercount)
{
	const char *function = luaL_checkstring(NL,1);
	int tick = luaL_checkint(NL,2);
	npc_addluaeventtimercount(function,tick);
	lua_pushinteger(NL,tick);
	return 1;
}

static int buildin_announce_sub(struct block_list *bl, va_list ap)
{
	char *mes       = va_arg(ap, char *);
	int   len       = va_arg(ap, int);
	int   type      = va_arg(ap, int);
	char *fontColor = va_arg(ap, char *);
	short fontType  = (short)va_arg(ap, int);
	short fontSize  = (short)va_arg(ap, int);
	short fontAlign = (short)va_arg(ap, int);
	short fontY     = (short)va_arg(ap, int);
	if (fontColor)
		clif_broadcast2(bl, mes, len, strtol(fontColor, (char **)NULL, 0), fontType, fontSize, fontAlign, fontY, SELF);
	else
		clif_broadcast(bl, mes, len, type, SELF);
	return 0;
}

LUA_FUNC(announce)
{
	const char *mes=luaL_checkstring(NL,1);
	int flag=luaL_checkint(NL,2);
	const char *fontColor=lua_isstring(NL,3) ? luaL_checkstring(NL,4) : NULL;
	int fontType=lua_isnumber(NL,4) ? luaL_checkint(NL,4) : 0x190;
	int fontSize=lua_isnumber(NL,5) ? luaL_checkint(NL,5) : 12;
	int fontAlign = lua_isnumber(NL,6) ? luaL_checkint(NL,6) : 0;
	int fontY = lua_isnumber(NL,7) ? luaL_checkint(NL,7) : 0;
	int npcid = lua_isnumber(NL,8) ? luaL_checkint(NL,8) : 0;
	if (flag&0x0f) // Broadcast source or broadcast region defined
	{
		send_target target;
		struct block_list *bl = (flag&0x08) ? map_id2bl(npcid) : (struct block_list *)script_get_target(NL,9); // If bc_npc flag is set, use NPC as broadcast source
		if (bl == NULL) {
			return 0;
		}
		flag &= 0x07;
		target = (flag == 1) ? ALL_SAMEMAP :
		         (flag == 2) ? AREA :
		         (flag == 3) ? SELF :
		                       ALL_CLIENT;
		if (fontColor)
			clif_broadcast2(bl, mes, (int)strlen(mes)+1, strtol(fontColor, (char **)NULL, 0), fontType, fontSize, fontAlign, fontY, target);
		else
			clif_broadcast(bl, mes, (int)strlen(mes)+1, flag&0xf0, target);
	}
	else
	{
		if (fontColor)
			intif_broadcast2(mes, (int)strlen(mes)+1, strtol(fontColor, (char **)NULL, 0), fontType, fontSize, fontAlign, fontY);
		else
			intif_broadcast(mes, (int)strlen(mes)+1, flag&0xf0);
	}
	return 0;
}

LUA_FUNC(mapannounce)
{
	const char *mapname   = luaL_checkstring(NL,1);
	const char *mes       = luaL_checkstring(NL,2);
	int         flag      = luaL_checkint(NL,3);
	const char *fontColor = lua_isstring(NL,4) ? luaL_checkstring(NL,4) : NULL;
	int         fontType  = lua_isnumber(NL,5) ? luaL_checkint(NL,5) : 0x190; // default fontType (FW_NORMAL)
	int         fontSize  = lua_isnumber(NL,6) ? luaL_checkint(NL,6)	: 12;    // default fontSize
	int         fontAlign = lua_isnumber(NL,7) ? luaL_checkint(NL,7) : 0;     // default fontAlign
	int         fontY     = lua_isnumber(NL,8) ? luaL_checkint(NL,8) : 0;     // default fontY
	int m;
	if ((m = map_mapname2mapid(mapname)) < 0) {
		return 0;
	}
	map_foreachinmap(buildin_announce_sub, m, BL_PC,
			mes, strlen(mes)+1, flag&0xf0, fontColor, fontType, fontSize, fontAlign, fontY);
	return 0;
}

LUA_FUNC(areaannounce)
{
	const char *mapname   = luaL_checkstring(NL,1);
	int         x0        = luaL_checkint(NL,2);
	int         y0        = luaL_checkint(NL,3);
	int         x1        = luaL_checkint(NL,4);
	int         y1        = luaL_checkint(NL,5);
	const char *mes       = luaL_checkstring(NL,6);
	int         flag      = luaL_checkint(NL,7);
	const char *fontColor = lua_isstring(NL,8) ? luaL_checkstring(NL,8) : NULL;
	int         fontType  = lua_isnumber(NL,9) ? luaL_checkint(NL,9) : 0x190; // default fontType (FW_NORMAL)
	int         fontSize  = lua_isnumber(NL,10) ? luaL_checkint(NL,10) : 12;    // default fontSize
	int         fontAlign = lua_isnumber(NL,11) ? luaL_checkint(NL,11) : 0;     // default fontAlign
	int         fontY     = lua_isnumber(NL,12) ? luaL_checkint(NL,12) : 0;     // default fontY
	int m;
	if ((m = map_mapname2mapid(mapname)) < 0) {
		return 0;
	}
	map_foreachinarea(buildin_announce_sub, m, x0, y0, x1, y1, BL_PC,
		mes, strlen(mes)+1, flag&0xf0, fontColor, fontType, fontSize, fontAlign, fontY);
	return 0;
}

LUA_FUNC(getusers)
{
	lua_get_target(3);
	int flag=luaL_checkint(NL,1);
	int npcid=luaL_checkint(NL,2);
	struct block_list *bl=map_id2bl((flag&0x08)?npcid:sd->status.account_id);
	int val=0;
	switch(flag&0x07){
	case 0: val=map[bl->m].users; break;
	case 1: val=map_getusers(); break;
	}
	lua_pushinteger(NL,val);
	return 1;
}

LUA_FUNC(getusersname)
{
	lua_get_target(1);
	TBL_PC *pl_sd;
	int disp_num=1;
	struct s_mapiterator* iter;
	iter = mapit_getallusers();
	for( pl_sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); pl_sd = (TBL_PC*)mapit_next(iter) )
	{
		if( battle_config.hide_GM_session && pc_isGM(pl_sd) ) {
			continue; // skip hidden GMs
		}
		if((disp_num++)%10==0) {
			clif_scriptnext(sd,sd->npc_id);
		}
		clif_scriptmes(sd,sd->npc_id,pl_sd->status.name);
	}
	mapit_free(iter);
	return 0;
}

LUA_FUNC(getmapguildusers)
{
	const char *str;
	int m, gid;
	int i=0,c=0;
	struct guild *g = NULL;
	str=luaL_checkstring(NL,1);
	gid=luaL_checkint(NL,2);
	if ((m = map_mapname2mapid(str)) < 0) {
		return 0; //not on this map-server
	}
	g = guild_search(gid);
	if (g){
		for(i = 0; i < g->max_member; i++)
		{
			if (g->member[i].sd && g->member[i].sd->bl.m == m)
				c++;
		}
	}
	lua_pushinteger(NL,c);
	return 1;
}

LUA_FUNC(getmapusers)
{
	const char *str;
	int m;
	str=luaL_checkstring(NL,1);
	if( (m=map_mapname2mapid(str))< 0){
		return 0;
	}
	lua_pushinteger(NL,map[m].users);
	return 0;
}

static int buildin_getareausers_sub(struct block_list *bl,va_list ap)
{
	int *users=va_arg(ap,int *);
	(*users)++;
	return 0;
}

LUA_FUNC(getareausers)
{
	const char *str;
	int m,x0,y0,x1,y1,users=0;
	str=luaL_checkstring(NL,1);
	x0=luaL_checkint(NL,2);
	y0=luaL_checkint(NL,3);
	x1=luaL_checkint(NL,4);
	y1=luaL_checkint(NL,5);
	if( (m=map_mapname2mapid(str))< 0){
		return 0;
	}
	map_foreachinarea(buildin_getareausers_sub,
		m,x0,y0,x1,y1,BL_PC,&users);
	lua_pushinteger(NL,users);
	return 0;
}

static int buildin_getareadropitem_sub(struct block_list *bl,va_list ap)
{
	int item=va_arg(ap,int);
	int *amount=va_arg(ap,int *);
	struct flooritem_data *drop=(struct flooritem_data *)bl;

	if(drop->item_data.nameid==item)
		(*amount)+=drop->item_data.amount;

	return 0;
}

LUA_FUNC(getareadropitem)
{
	const char *str;
	int m,x0,y0,x1,y1,item,amount=0;
	str = luaL_checkstring(NL,1);
	x0 = luaL_checkint(NL,2);
	y0 = luaL_checkint(NL,3);
	x1 = luaL_checkint(NL,4);
	y1 = luaL_checkint(NL,5);
	if( lua_isstring(NL,6) ){
		const char *name=luaL_checkstring(NL,6);
		struct item_data *item_data = itemdb_searchname(name);
		item=UNKNOWN_ITEM_ID;
		if( item_data )
			item=item_data->nameid;
	}else {
		item=luaL_checkint(NL,6);
	}
	if( (m=map_mapname2mapid(str))< 0){
		return 0;
	}
	map_foreachinarea(buildin_getareadropitem_sub,
		m,x0,y0,x1,y1,BL_ITEM,item,&amount);
	lua_pushinteger(NL,amount);
	return 0;
}

LUA_FUNC(npc_enable)
{
	const char *str = luaL_checkstring(NL,1);
	int flag = luaL_checkint(NL,2);
	npc_enable(str,flag);
	return 0;
}

LUA_FUNC(sc_start)
{
	struct block_list* bl;
	enum sc_type type;
	int tick;
	int val1;
	lua_get_target(5);
	int val4 = 0;
	type = (sc_type)luaL_checkint(NL,1);
	tick = luaL_checkint(NL,2);
	val1 = luaL_checkint(NL,3);
	if( lua_isnumber(NL,4) ) {
		bl = map_id2bl(luaL_checkint(NL,4));
	}
	else {
		bl = map_id2bl(sd->status.account_id);
	}
	if( tick == 0 && val1 > 0 && type > SC_NONE && type < SC_MAX && status_sc2skill(type) != 0 )
	{// When there isn't a duration specified, try to get it from the skill_db
		tick = skill_get_time(status_sc2skill(type), val1);
	}
	if( potion_flag == 1 && potion_target )
	{	//skill.c set the flags before running the script, this must be a potion-pitched effect.
		bl = map_id2bl(potion_target);
		tick /= 2;// Thrown potions only last half.
		val4 = 1;// Mark that this was a thrown sc_effect
	}
	if( bl )
		status_change_start(bl, type, 10000, val1, 0, 0, val4, tick, 2);
	return 0;
}

LUA_FUNC(sc_start2)
{
	lua_get_target(6);
	struct block_list* bl;
	enum sc_type type;
	int tick;
	int val1;
	int val4 = 0;
	int rate;	
	type = (sc_type)luaL_checkint(NL,1);
	tick = luaL_checkint(NL,2);
	val1 = luaL_checkint(NL,3);
	rate = luaL_checkint(NL,4);
	if (sd == NULL)
		bl = map_id2bl(luaL_checkint(NL,5));
	else
		bl = &sd->bl;
	if( tick == 0 && val1 > 0 && type > SC_NONE && type < SC_MAX && status_sc2skill(type) != 0 )
	{// When there isn't a duration specified, try to get it from the skill_db
		tick = skill_get_time(status_sc2skill(type), val1);
	}

	if( potion_flag == 1 && potion_target )
	{	//skill.c set the flags before running the script, this must be a potion-pitched effect.
		bl = map_id2bl(potion_target);
		tick /= 2;// Thrown potions only last half.
		val4 = 1;// Mark that this was a thrown sc_effect
	}

	if( bl )
		status_change_start(bl, type, rate, val1, 0, 0, val4, tick, 2);

	return 0;
}

LUA_FUNC(sc_start4)
{
	lua_get_target(8);
	struct block_list* bl;
	enum sc_type type;
	int tick;
	int val1;
	int val2;
	int val3;
	int val4;
	type = (sc_type)luaL_checkint(NL,1);
	tick = luaL_checkint(NL,2);
	val1 = luaL_checkint(NL,3);
	val2 = luaL_checkint(NL,4);
	val3 = luaL_checkint(NL,5);
	val4 = luaL_checkint(NL,6);
	if (!sd)
		bl = map_id2bl(luaL_checkint(NL,7));
	else
		bl = &sd->bl;

	if( tick == 0 && val1 > 0 && type > SC_NONE && type < SC_MAX && status_sc2skill(type) != 0 )
	{// When there isn't a duration specified, try to get it from the skill_db
		tick = skill_get_time(status_sc2skill(type), val1);
	}

	if( potion_flag == 1 && potion_target )
	{	//skill.c set the flags before running the script, this must be a potion-pitched effect.
		bl = map_id2bl(potion_target);
		tick /= 2;// Thrown potions only last half.
	}

	if( bl )
		status_change_start(bl, type, 10000, val1, val2, val3, val4, tick, 2);

	return 0;
}

LUA_FUNC(sc_end)
{
	lua_get_target(3);
	struct block_list* bl;
	int type = luaL_checkint(NL,1);
	if(!sd)
		bl = map_id2bl(luaL_checkint(NL,2));
	else
		bl = &sd->bl;
	
	if( potion_flag==1 && potion_target )
	{//##TODO how does this work [FlavioJS]
		bl = map_id2bl(potion_target);
	}

	if( !bl ) return 0;

	if( type >= 0 && type < SC_MAX )
	{
		struct status_change *sc = status_get_sc(bl);
		struct status_change_entry *sce = sc?sc->data[type]:NULL;
		if (!sce) return 0;
		//This should help status_change_end force disabling the SC in case it has no limit.
		sce->val1 = sce->val2 = sce->val3 = sce->val4 = 0;
		status_change_end(bl, (sc_type)type, INVALID_TIMER);
	} else
		status_change_clear(bl, 2);// remove all effects
	return 0;
}

LUA_FUNC(getscrate)
{
	lua_get_target(4);
	struct block_list *bl;
	int type,rate;
	type=luaL_checkint(NL,1);
	rate=luaL_checkint(NL,2);
	if( lua_isnumber(NL,3) )
		bl = map_id2bl(luaL_checkint(NL,3));
	else
		bl = map_id2bl(sd->status.account_id);
	if (bl)
		rate = status_get_sc_def(bl, (sc_type)type, 10000, 10000, 0);
	lua_pushinteger(NL,rate);
	return 1;
}

LUA_FUNC(catchpet)
{
	int pet_id;
	lua_get_target(2);
	pet_id=luaL_checkint(NL,1);
	pet_catch_process1(sd,pet_id);
	return 0;
}

LUA_FUNC(homunculus_evolution)
{
	lua_get_target(1);
	if(merc_is_hom_active(sd->hd))
	{
		if (sd->hd->homunculus.intimacy > 91000)
			merc_hom_evolution(sd->hd);
		else
			clif_emotion(&sd->hd->bl, 4) ;	//swt
	}
	return 0;
}

LUA_FUNC(homunculus_shuffle)
{
	lua_get_target(1);
	if(merc_is_hom_active(sd->hd))
		merc_hom_shuffle(sd->hd);
	return 0;
}

LUA_FUNC(eaclass)
{
	lua_get_target(2);
	int class_;
	if( lua_isnumber(NL,1) )
		class_ = luaL_checkint(NL,1);
	else {
		if (!sd) 
			return 0;

		class_ = sd->status.class_;
	}
	lua_pushinteger(NL,pc_jobid2mapid(class_));
	return 1;
}

LUA_FUNC(roclass)
{
	lua_get_target(3);
	int class_ =luaL_checkint(NL,1);
	int sex;
	if( lua_isnumber(NL,2) )
		sex = luaL_checkint(NL,2);
	else {
		if (sd->status.account_id)
			sex = sd->status.sex;
		else
			sex = 1; //Just use male when not found.
	}
	lua_pushnumber(NL,pc_mapid2jobid(class_, sex));
	return 1;
}

LUA_FUNC(birthpet)
{
	lua_get_target(1);
	clif_sendegg(sd);
	return 0;
}

LUA_FUNC(resetlvl)
{
	lua_get_target(2);
	int type=luaL_checkint(NL,1);
	pc_resetlvl(sd,type);
	return 0;
}

LUA_FUNC(resetstatus)
{
	lua_get_target(1);
	pc_resetstate(sd);
	return 0;
}

LUA_FUNC(resetskill)
{
	lua_get_target(1);
	pc_resetskill(sd,1);
	return 0;
}

LUA_FUNC(skillpointcount)
{
	lua_get_target(1);
	lua_pushinteger(NL,sd->status.skill_point + pc_resetskill(sd,2));
	return 0;
}

LUA_FUNC(changebase)
{
	lua_get_target(2);
	int vclass;
	vclass = luaL_checkint(NL,1);
	if(vclass == JOB_WEDDING)
	{
		if (!battle_config.wedding_modifydisplay || //Do not show the wedding sprites
			sd->class_&JOBL_BABY //Baby classes screw up when showing wedding sprites.
			) 
		return 0;
	}
	if(!sd->disguise && vclass != sd->vd.class_) {
		status_set_viewdata(&sd->bl, vclass);
		//Updated client view. Base, Weapon and Cloth Colors.
		clif_changelook(&sd->bl,LOOK_BASE,sd->vd.class_);
		clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
		if (sd->vd.cloth_color)
			clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->vd.cloth_color);
		clif_skillinfoblock(sd);
	}
	return 0;
}

LUA_FUNC(changesex)
{
	lua_get_target(1);
	chrif_changesex(sd);
	return 0;
}

LUA_FUNC(globalmes)
{
	const char *name=NULL, *mes;
	mes=luaL_checkstring(NL,1);
	if( lua_isstring(NL,2) ) {
		name = luaL_checkstring(NL,2);
	} else {
		name = "";
	}
	npc_globalmessage(name,mes);
	return 0;
}

//##TODO waiting room events
LUA_FUNC(waitingroom)
{
	struct npc_data* nd;
	const char* title;
	const char* ev = "";
	int limit;
	int trigger = 0;
	int pub = 1;
	int npc_id = luaL_checkint(NL,3);
	title = luaL_checkstring(NL,1);
	limit = luaL_checkint(NL,2);
	if( lua_isstring(NL,4) ) {
		trigger = luaL_checkint(NL,5);
		ev = luaL_checkstring(NL,4);
	}
	nd = (struct npc_data *)map_id2bl(npc_id);
	if( nd != NULL )
		chat_createnpcchat(nd, title, limit, pub, trigger, ev);
	return 0;
}

LUA_FUNC(delwaitingroom)
{
	struct npc_data* nd;
	const char *name = luaL_checkstring(NL,1);
	nd = npc_name2id(name);
	if( nd != NULL )
		chat_deletenpcchat(nd);
	return 0;
}

LUA_FUNC(waitingroomkickall)
{
	struct chat_data* cd;
	struct npc_data* nd = npc_name2id(luaL_checkstring(NL,1));
	if( nd != NULL && (cd=(struct chat_data *)map_id2bl(nd->chat_id)) != NULL )
		chat_npckickall(cd);
	return 0;
}

LUA_FUNC(enablewaitingroomevent)
{
	struct chat_data* cd;
	struct npc_data* nd = npc_name2id(luaL_checkstring(NL,1));
	if( nd != NULL && (cd=(struct chat_data *)map_id2bl(nd->chat_id)) != NULL )
		chat_enableevent(cd);
	return 0;
}

LUA_FUNC(disablewaitingroomevent)
{
	struct chat_data *cd;
	struct npc_data *nd = npc_name2id(luaL_checkstring(NL,1));
	if( nd != NULL && (cd=(struct chat_data *)map_id2bl(nd->chat_id)) != NULL )
		chat_disableevent(cd);
	return 0;
}

LUA_FUNC(getwaitingroomstate)
{
	struct chat_data *cd;
	int type = luaL_checkint(NL,1);
	struct npc_data *nd = npc_name2id(luaL_checkstring(NL,2));
	if( nd == NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id)) == NULL )
	{
		return 0;
	}
	switch(type)
	{
	case 0:  lua_pushinteger(NL, cd->users); break;
	case 1:  lua_pushinteger(NL, cd->limit); break;
	case 2:  lua_pushinteger(NL, cd->trigger&0x7f); break;
	case 3:  lua_pushinteger(NL, ((cd->trigger&0x80)!=0)); break;
	case 4:  lua_pushstring(NL, cd->title); break;
	case 5:  lua_pushstring(NL, cd->pass); break;
	case 16: lua_pushstring(NL, cd->npc_event);break;
	case 32: lua_pushinteger(NL, (cd->users >= cd->limit)); break;
	case 33: lua_pushinteger(NL, (cd->users >= cd->trigger)); break;
	default: return 0;
	}
	return 1;
}

LUA_FUNC(warpwaitingpc)
{
	int x, y, i, n;
	const char* map_name;
	struct npc_data* nd;
	struct chat_data* cd;
	TBL_PC* sd;
	int npc_id = luaL_checkint(NL,1);
	nd = (struct npc_data *)map_id2bl(npc_id);
	if( nd == NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id)) == NULL ) {
		return 0;
	}
	map_name = luaL_checkstring(NL,2);
	x = luaL_checkint(NL,3);
	y = luaL_checkint(NL,4);
	n = cd->trigger&0x7f;
	if( lua_isnumber(NL,5) ) {
		n = luaL_checkint(NL,5);
	}
	lua_newtable(NL);
	for( i = 0; i < n && cd->users > 0; i++ )
	{
		sd = cd->usersd[0];
		if( sd == NULL )
		{
			ShowDebug("script:warpwaitingpc: no user in chat room position 0 (cd->users=%d,%d/%d)\n", cd->users, i, n);
			continue;// Broken npc chat room?
		}
		lua_pushinteger(NL,sd->bl.id);
		lua_rawseti(NL,-2,i + 1);
		if( strcmp(map_name,"Random") == 0 )
			pc_randomwarp(sd,3);
		else if( strcmp(map_name,"SavePoint") == 0 )
		{
			if( map[sd->bl.m].flag.noteleport )
				return 0;// can't teleport on this map

			pc_setpos(sd, sd->status.save_point.map, sd->status.save_point.x, sd->status.save_point.y, 3);
		}
		else
			pc_setpos(sd, mapindex_name2id(map_name), x, y, 0);
	}
	return 1;
}


LUA_FUNC(isloggedin)
{
	lua_get_target(1);
	int id = luaL_checkint(NL,1);
	TBL_PC* fsd = map_id2sd(id);
	if (!fsd) {
		return 0;
	}
	if (!sd || sd->status.char_id != id) {
		return 0;
	}
	lua_pushinteger(NL,1);
	return 1;
}

LUA_FUNC(setmapflagnosave)
{
	int m,x,y;
	unsigned short mapindex;
	const char *str,*str2;
	str=luaL_checkstring(NL,1);
	str2=luaL_checkstring(NL,2);
	x=luaL_checkint(NL,3);
	y=luaL_checkint(NL,4);
	m = map_mapname2mapid(str);
	mapindex = mapindex_name2id(str2);
	if(m >= 0 && mapindex) {
		map[m].flag.nosave=1;
		map[m].save.map=mapindex;
		map[m].save.x=x;
		map[m].save.y=y;
	}
	return 0;
}

LUA_FUNC(getmapflag)
{
	int m,i;
	const char *str;
	str=luaL_checkstring(NL,1);
	i=luaL_checkint(NL,2);
	m = map_mapname2mapid(str);
	if(m >= 0) {
		switch(i) {
			case MF_NOMEMO:			lua_pushinteger(NL,map[m].flag.nomemo); break;
			case MF_NOTELEPORT:		lua_pushinteger(NL,map[m].flag.noteleport); break;
			case MF_NOBRANCH:		lua_pushinteger(NL,map[m].flag.nobranch); break;
			case MF_NOPENALTY:		lua_pushinteger(NL,map[m].flag.noexppenalty); break;
			case MF_NOZENYPENALTY:	lua_pushinteger(NL,map[m].flag.nozenypenalty); break;
			case MF_PVP:			lua_pushinteger(NL,map[m].flag.pvp); break;
			case MF_PVP_NOPARTY:	lua_pushinteger(NL,map[m].flag.pvp_noparty); break;
			case MF_PVP_NOGUILD:	lua_pushinteger(NL,map[m].flag.pvp_noguild); break;
			case MF_GVG:			lua_pushinteger(NL,map[m].flag.gvg); break;
			case MF_GVG_NOPARTY:	lua_pushinteger(NL,map[m].flag.gvg_noparty); break;
			case MF_GVG_DUNGEON:	lua_pushinteger(NL,map[m].flag.gvg_dungeon); break;
			case MF_GVG_CASTLE:		lua_pushinteger(NL,map[m].flag.gvg_castle); break;
			case MF_NOTRADE:		lua_pushinteger(NL,map[m].flag.notrade); break;
			case MF_NODROP:			lua_pushinteger(NL,map[m].flag.nodrop); break;
			case MF_NOSKILL:		lua_pushinteger(NL,map[m].flag.noskill); break;
			case MF_NOWARP:			lua_pushinteger(NL,map[m].flag.nowarp); break;
			case MF_NOICEWALL:		lua_pushinteger(NL,map[m].flag.noicewall); break;
			case MF_SNOW:			lua_pushinteger(NL,map[m].flag.snow); break;
			case MF_CLOUDS:			lua_pushinteger(NL,map[m].flag.clouds); break;
			case MF_CLOUDS2:		lua_pushinteger(NL,map[m].flag.clouds2); break;
			case MF_FOG:			lua_pushinteger(NL,map[m].flag.fog); break;
			case MF_FIREWORKS:		lua_pushinteger(NL,map[m].flag.fireworks); break;
			case MF_SAKURA:			lua_pushinteger(NL,map[m].flag.sakura); break;
			case MF_LEAVES:			lua_pushinteger(NL,map[m].flag.leaves); break;
			case MF_RAIN:			lua_pushinteger(NL,map[m].flag.rain); break;
			case MF_INDOORS:		lua_pushinteger(NL,map[m].flag.indoors); break;
			case MF_NIGHTENABLED:	lua_pushinteger(NL,map[m].flag.nightenabled); break;
			case MF_NOGO:			lua_pushinteger(NL,map[m].flag.nogo); break;
			case MF_NOBASEEXP:		lua_pushinteger(NL,map[m].flag.nobaseexp); break;
			case MF_NOJOBEXP:		lua_pushinteger(NL,map[m].flag.nojobexp); break;
			case MF_NOMOBLOOT:		lua_pushinteger(NL,map[m].flag.nomobloot); break;
			case MF_NOMVPLOOT:		lua_pushinteger(NL,map[m].flag.nomvploot); break;
			case MF_NORETURN:		lua_pushinteger(NL,map[m].flag.noreturn); break;
			case MF_NOWARPTO:		lua_pushinteger(NL,map[m].flag.nowarpto); break;
			case MF_NIGHTMAREDROP:	lua_pushinteger(NL,map[m].flag.pvp_nightmaredrop); break;
			case MF_RESTRICTED:		lua_pushinteger(NL,map[m].flag.restricted); break;
			case MF_NOCOMMAND:		lua_pushinteger(NL,map[m].nocommand); break;
			case MF_JEXP:			lua_pushinteger(NL,map[m].jexp); break;
			case MF_BEXP:			lua_pushinteger(NL,map[m].bexp); break;
			case MF_NOVENDING:		lua_pushinteger(NL,map[m].flag.novending); break;
			case MF_LOADEVENT:		lua_pushinteger(NL,map[m].flag.loadevent); break;
			case MF_NOCHAT:			lua_pushinteger(NL,map[m].flag.nochat); break;
			case MF_PARTYLOCK:		lua_pushinteger(NL,map[m].flag.partylock); break;
			case MF_GUILDLOCK:		lua_pushinteger(NL,map[m].flag.guildlock); break;
		}
	}

	return 0;
}

LUA_FUNC(editmapflag)
{
	int m,i,flag;
	const char *str;
	const char *val=NULL;
	str=luaL_checkstring(NL,1);
	i=luaL_checkint(NL,2);
	flag=luaL_checkint(NL,3);
	if(lua_isnumber(NL,4)){
		val=luaL_checkstring(NL,4);
	}
	m = map_mapname2mapid(str);
	if(m >= 0) {
		switch(i) {
			case MF_NOMEMO:        map[m].flag.nomemo=flag; break;
			case MF_NOTELEPORT:    map[m].flag.noteleport=flag; break;
			case MF_NOBRANCH:      map[m].flag.nobranch=flag; break;
			case MF_NOPENALTY:     map[m].flag.noexppenalty=flag; map[m].flag.nozenypenalty=flag; break;
			case MF_NOZENYPENALTY: map[m].flag.nozenypenalty=flag; break;
			case MF_PVP:           map[m].flag.pvp=flag; break;
			case MF_PVP_NOPARTY:   map[m].flag.pvp_noparty=flag; break;
			case MF_PVP_NOGUILD:   map[m].flag.pvp_noguild=flag; break;
			case MF_GVG:           map[m].flag.gvg=flag; break;
			case MF_GVG_NOPARTY:   map[m].flag.gvg_noparty=flag; break;
			case MF_GVG_DUNGEON:   map[m].flag.gvg_dungeon=flag; break;
			case MF_GVG_CASTLE:    map[m].flag.gvg_castle=flag; break;
			case MF_NOTRADE:       map[m].flag.notrade=flag; break;
			case MF_NODROP:        map[m].flag.nodrop=flag; break;
			case MF_NOSKILL:       map[m].flag.noskill=flag; break;
			case MF_NOWARP:        map[m].flag.nowarp=flag; break;
			case MF_NOICEWALL:     map[m].flag.noicewall=flag; break;
			case MF_SNOW:          map[m].flag.snow=flag; break;
			case MF_CLOUDS:        map[m].flag.clouds=flag; break;
			case MF_CLOUDS2:       map[m].flag.clouds2=flag; break;
			case MF_FOG:           map[m].flag.fog=flag; break;
			case MF_FIREWORKS:     map[m].flag.fireworks=flag; break;
			case MF_SAKURA:        map[m].flag.sakura=flag; break;
			case MF_LEAVES:        map[m].flag.leaves=flag; break;
			case MF_RAIN:          map[m].flag.rain=flag; break;
			case MF_INDOORS:       map[m].flag.indoors=flag; break;
			case MF_NIGHTENABLED:  map[m].flag.nightenabled=flag; break;
			case MF_NOGO:          map[m].flag.nogo=flag; break;
			case MF_NOBASEEXP:     map[m].flag.nobaseexp=flag; break;
			case MF_NOJOBEXP:      map[m].flag.nojobexp=flag; break;
			case MF_NOMOBLOOT:     map[m].flag.nomobloot=flag; break;
			case MF_NOMVPLOOT:     map[m].flag.nomvploot=flag; break;
			case MF_NORETURN:      map[m].flag.noreturn=flag; break;
			case MF_NOWARPTO:      map[m].flag.nowarpto=flag; break;
			case MF_NIGHTMAREDROP: map[m].flag.pvp_nightmaredrop=flag; break;
			case MF_RESTRICTED:    map[m].flag.restricted=flag; break;
			case MF_NOCOMMAND:     map[m].nocommand = (!val || atoi(val) <= 0) ? 100 : atoi(val); break;
			case MF_JEXP:          map[m].jexp = (!val || atoi(val) < 0) ? 100 : atoi(val); break;
			case MF_BEXP:          map[m].bexp = (!val || atoi(val) < 0) ? 100 : atoi(val); break;
			case MF_NOVENDING:     map[m].flag.novending=flag; break;
			case MF_LOADEVENT:     map[m].flag.loadevent=flag; break;
			case MF_NOCHAT:        map[m].flag.nochat=flag; break;
			case MF_PARTYLOCK:     map[m].flag.partylock=flag; break;
			case MF_GUILDLOCK:     map[m].flag.guildlock=flag; break;
		}
	}

	return 0;
}

LUA_FUNC(pvpon)
{
	int m;
	const char *str;
	TBL_PC* sd = NULL;
	struct s_mapiterator* iter;
	str = luaL_checkstring(NL,1);
	m = map_mapname2mapid(str);
	if( m < 0 || map[m].flag.pvp )
		return 0; // nothing to do
	map[m].flag.pvp = 1;
	clif_send0199(m,1);
	if(battle_config.pk_mode) // disable ranking functions if pk_mode is on [Valaris]
		return 0;
	iter = mapit_getallusers();
	for( sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); sd = (TBL_PC*)mapit_next(iter) )
	{
		if( sd->bl.m != m || sd->pvp_timer != -1 )
			continue; // not applicable

		sd->pvp_timer = add_timer(gettick()+200,pc_calc_pvprank_timer,sd->bl.id,0);
		sd->pvp_rank = 0;
		sd->pvp_lastusers = 0;
		sd->pvp_point = 5;
		sd->pvp_won = 0;
		sd->pvp_lost = 0;
	}
	mapit_free(iter);
	return 0;
}

static int buildin_pvpoff_sub(struct block_list *bl,va_list ap)
{
	TBL_PC* sd = (TBL_PC*)bl;
	clif_pvpset(sd, 0, 0, 2);
	if (sd->pvp_timer != -1) {
		delete_timer(sd->pvp_timer, pc_calc_pvprank_timer);
		sd->pvp_timer = INVALID_TIMER;
	}
	return 0;
}

LUA_FUNC(pvpoff)
{
	int m;
	const char *str;
	str=luaL_checkstring(NL,1);
	m = map_mapname2mapid(str);
	if(m < 0 || !map[m].flag.pvp)
		return 0;
	map[m].flag.pvp = 0;
	clif_send0199(m,0);
	if(battle_config.pk_mode) // disable ranking options if pk_mode is on
		return 0;
	map_foreachinmap(buildin_pvpoff_sub, m, BL_PC);
	return 0;
}

LUA_FUNC(gvgon)
{
	int m;
	const char *str;
	str=luaL_checkstring(NL,1);
	m = map_mapname2mapid(str);
	if(m >= 0 && !map[m].flag.gvg) {
		map[m].flag.gvg = 1;
		clif_send0199(m,3);
	}
	return 0;
}
LUA_FUNC(gvgoff)
{
	int m;
	const char *str;
	str=luaL_checkstring(NL,1);
	m = map_mapname2mapid(str);
	if(m >= 0 && map[m].flag.gvg) {
		map[m].flag.gvg = 0;
		clif_send0199(m,0);
	}
	return 0;
}

LUA_FUNC(emotion)
{
	struct map_session_data *sd;
	int type;
	int obj;
	type=luaL_checkint(NL,1);
	obj=luaL_checkint(NL,2);
	if(type < 0 || type > 100)
		return 0;
	if ( (sd = map_charid2sd(obj)) != NULL) {
		clif_emotion(&sd->bl,type);
	}
	else
		clif_emotion(map_id2bl(obj),type);
	return 0;
}

static int buildin_maprespawnguildid_sub_pc(struct map_session_data* sd, va_list ap)
{
	int m=va_arg(ap,int);
	int g_id=va_arg(ap,int);
	int flag=va_arg(ap,int);

	if(!sd || sd->bl.m != m)
		return 0;
	if(
		(sd->status.guild_id == g_id && flag&1) || //Warp out owners
		(sd->status.guild_id != g_id && flag&2) || //Warp out outsiders
		(sd->status.guild_id == 0)	// Warp out players not in guild [Valaris]
	)
		pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
	return 1;
}

static int buildin_maprespawnguildid_sub_mob(struct block_list *bl,va_list ap)
{
	struct mob_data *md=(struct mob_data *)bl;

	if(!md->guardian_data && md->class_ != MOBID_EMPERIUM)
		status_kill(bl);

	return 0;
}

LUA_FUNC(maprespawnguildid)
{
	const char *mapname=luaL_checkstring(NL,1);
	int g_id=luaL_checkint(NL,2);
	int flag=luaL_checkint(NL,3);
	int m=map_mapname2mapid(mapname);
	if(m == -1)
		return 0;
	//Catch ALL players (in case some are 'between maps' on execution time)
	map_foreachpc(buildin_maprespawnguildid_sub_pc,m,g_id,flag);
	if (flag&4) //Remove script mobs.
		map_foreachinmap(buildin_maprespawnguildid_sub_mob,m,BL_MOB);
	return 0;
}

LUA_FUNC(agitstart)
{
	if(agit_flag==1) return 0;      // Agit already Start.
	agit_flag=1;
	guild_agit_start();
	return 0;
}

LUA_FUNC(agitend)
{
	if(agit_flag==0) return 0;      // Agit already End.
	agit_flag=0;
	guild_agit_end();
	return 0;
}

LUA_FUNC(agitstart2)
{
	if(agit2_flag==1) return 0;      // Agit2 already Start.
	agit2_flag=1;
	guild_agit2_start();
	return 0;
}

LUA_FUNC(agitend2)
{
	if(agit2_flag==0) return 0;      // Agit2 already End.
	agit2_flag=0;
	guild_agit2_end();
	return 0;
}

LUA_FUNC(agitcheck)
{
	lua_pushinteger(NL,agit_flag);
	return 1;
}

LUA_FUNC(agitcheck2)
{
	lua_pushinteger(NL,agit2_flag);
	return 1;
}

LUA_FUNC(flagemblem)
{
	TBL_NPC* nd;
	int g_id=luaL_checkint(NL,1);
	int obj=luaL_checkint(NL,2);
	if(g_id < 0) return 0;
	nd = (TBL_NPC*)map_id2nd(obj);
	if( nd == NULL )
	{
		ShowError("script:flagemblem: npc %d not found\n", obj);
	}
	else
	{
		nd->u.scr.guild_id = g_id;
		clif_guild_emblem_area(&nd->bl);
	}
	return 0;
}

LUA_FUNC(getcastlename)
{
	const char* mapname = mapindex_getmapname(luaL_checkstring(NL,1),NULL);
	struct guild_castle* gc = guild_mapname2gc(mapname);
	const char* name = (gc) ? gc->castle_name : "";
	lua_pushstring(NL,name);
	return 1;
}

LUA_FUNC(getcastledata)
{
	const char* mapname = mapindex_getmapname(luaL_checkstring(NL,1),NULL);
	int index = luaL_checkint(NL,2);

	struct guild_castle* gc = guild_mapname2gc(mapname);

	if ( lua_isstring(NL,3) )
	{
		const char* event = luaL_checkstring(NL,3);
		guild_addcastleinfoevent(gc->castle_id,17,event);
	}

	if(gc){
		switch(index){
			case 0: {
				int i;
				for(i=1;i<18;i++) // Initialize[AgitInit]
					guild_castledataload(gc->castle_id,i);
				} break;
			case 1:
				lua_pushinteger(NL,gc->guild_id); break;
			case 2:
				lua_pushinteger(NL,gc->economy); break;
			case 3:
				lua_pushinteger(NL,gc->defense); break;
			case 4:
				lua_pushinteger(NL,gc->triggerE); break;
			case 5:
				lua_pushinteger(NL,gc->triggerD); break;
			case 6:
				lua_pushinteger(NL,gc->nextTime); break;
			case 7:
				lua_pushinteger(NL,gc->payTime); break;
			case 8:
				lua_pushinteger(NL,gc->createTime); break;
			case 9:
				lua_pushinteger(NL,gc->visibleC); break;
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
				lua_pushinteger(NL,gc->guardian[index-10].visible); break;
			default:
				return 0;
		}
		return 1;
	}
	return 0;
}

LUA_FUNC(setcastledata)
{
	const char* mapname = mapindex_getmapname(luaL_checkstring(NL,1),NULL);
	int index = luaL_checkint(NL,2);
	int value = luaL_checkint(NL,3);

	struct guild_castle* gc = guild_mapname2gc(mapname);

	if(gc) {
		// Save Data byself First
		switch(index){
			case 1:
				gc->guild_id = value; break;
			case 2:
				gc->economy = value; break;
			case 3:
				gc->defense = value; break;
			case 4:
				gc->triggerE = value; break;
			case 5:
				gc->triggerD = value; break;
			case 6:
				gc->nextTime = value; break;
			case 7:
				gc->payTime = value; break;
			case 8:
				gc->createTime = value; break;
			case 9:
				gc->visibleC = value; break;
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
				gc->guardian[index-10].visible = value; break;
			default:
				return 0;
		}
		guild_castledatasave(gc->castle_id,index,value);
	}
	return 0;
}

LUA_FUNC(requestguildinfo)
{
	int guild_id=luaL_checkint(NL,1);
	const char *event=NULL;

	if( lua_isstring(NL,2) )
	{
		event = luaL_checkstring(NL,2);
	}

	if(guild_id>0)
		guild_npc_request_info(guild_id,event);
	return 0;
}

LUA_FUNC(getequipcardcnt)
{
	lua_get_target(2);
	int i=-1,j,num;
	int count;

	num=luaL_checkint(NL,1);
	if (num > 0 && num <= ARRAYLENGTH(equip))
		i=pc_checkequip(sd,equip[num-1]);

	if (i < 0 || !sd->inventory_data[i]) {
		return 0;
	}

	if(itemdb_isspecial(sd->status.inventory[i].card[0]))
	{
		return 0;
	}

	count = 0;
	for( j = 0; j < sd->inventory_data[i]->slot; j++ )
		if( sd->status.inventory[i].card[j] && itemdb_type(sd->status.inventory[i].card[j]) == IT_CARD )
			count++;

	lua_pushinteger(NL,count);
	return 0;
}

LUA_FUNC(successremovecards)
{
	lua_get_target(2);
	int i=-1,j,c,cardflag=0;
	int num = luaL_checkint(NL,1);

	if (num > 0 && num <= ARRAYLENGTH(equip))
		i=pc_checkequip(sd,equip[num-1]);

	if (i < 0 || !sd->inventory_data[i]) {
		return 0;
	}

	if(itemdb_isspecial(sd->status.inventory[i].card[0])) 
		return 0;

	for( c = sd->inventory_data[i]->slot - 1; c >= 0; --c )
	{
		if( sd->status.inventory[i].card[c] && itemdb_type(sd->status.inventory[i].card[c]) == IT_CARD )
		{// extract this card from the item
			int flag;
			struct item item_tmp;
			cardflag = 1;
			item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].card[c];
			item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=0;
			item_tmp.attribute=0,item_tmp.expire_time=0;
			for (j = 0; j < MAX_SLOTS; j++)
				item_tmp.card[j]=0;

			//Logs items, got from (N)PC scripts [Lupus]
			if(log_config.enable_logs&0x40)
				log_pick_pc(sd, "N", item_tmp.nameid, 1, NULL);

			if((flag=pc_additem(sd,&item_tmp,1))){
				clif_additem(sd,0,0,flag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0);
			}
		}
	}

	if(cardflag == 1)
	{	
		int flag;
		struct item item_tmp;
		item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].nameid;
		item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=sd->status.inventory[i].refine;
		item_tmp.attribute=sd->status.inventory[i].attribute,item_tmp.expire_time=sd->status.inventory[i].expire_time;
		for (j = 0; j < sd->inventory_data[i]->slot; j++)
			item_tmp.card[j]=0;
		for (j = sd->inventory_data[i]->slot; j < MAX_SLOTS; j++)
			item_tmp.card[j]=sd->status.inventory[i].card[j];

		//Logs items, got from (N)PC scripts [Lupus]
		if(log_config.enable_logs&0x40)
			log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -1, &sd->status.inventory[i]);

		pc_delitem(sd,i,1,0);

		//Logs items, got from (N)PC scripts [Lupus]
		if(log_config.enable_logs&0x40)
			log_pick_pc(sd, "N", item_tmp.nameid, 1, &item_tmp);

		if((flag=pc_additem(sd,&item_tmp,1))){	// もてないならドロップ
			clif_additem(sd,0,0,flag);
			map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0);
		}

		clif_misceffect(&sd->bl,3);
	}
	return 0;
}

LUA_FUNC(failedremovecards)
{
	lua_get_target(3);
	int i=-1,j,c,cardflag=0;
	int num = luaL_checkint(NL,1);
	int typefail = luaL_checkint(NL,2);

	if (num > 0 && num <= ARRAYLENGTH(equip))
		i=pc_checkequip(sd,equip[num-1]);

	if (i < 0 || !sd->inventory_data[i])
		return 0;

	if(itemdb_isspecial(sd->status.inventory[i].card[0]))
		return 0;

	for( c = sd->inventory_data[i]->slot - 1; c >= 0; --c )
	{
		if( sd->status.inventory[i].card[c] && itemdb_type(sd->status.inventory[i].card[c]) == IT_CARD )
		{
			cardflag = 1;

			if(typefail == 2)
			{// add cards to inventory, clear 
				int flag;
				struct item item_tmp;
				item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].card[c];
				item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=0;
				item_tmp.attribute=0,item_tmp.expire_time=0;
				for (j = 0; j < MAX_SLOTS; j++)
					item_tmp.card[j]=0;

				//Logs items, got from (N)PC scripts [Lupus]
				if(log_config.enable_logs&0x40)
					log_pick_pc(sd, "N", item_tmp.nameid, 1, NULL);

				if((flag=pc_additem(sd,&item_tmp,1))){
					clif_additem(sd,0,0,flag);
					map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0);
				}
			}
		}
	}

	if(cardflag == 1)
	{
		if(typefail == 0 || typefail == 2){	// 武具損失
			//Logs items, got from (N)PC scripts [Lupus]
			if(log_config.enable_logs&0x40)
				log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -1, &sd->status.inventory[i]);

			pc_delitem(sd,i,1,0);
		}
		if(typefail == 1){	// カードのみ損失（武具を返す）
			int flag;
			struct item item_tmp;
			item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].nameid;
			item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=sd->status.inventory[i].refine;
			item_tmp.attribute=sd->status.inventory[i].attribute,item_tmp.expire_time=sd->status.inventory[i].expire_time;

			//Logs items, got from (N)PC scripts [Lupus]
			if(log_config.enable_logs&0x40)
				log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -1, &sd->status.inventory[i]);

			for (j = 0; j < sd->inventory_data[i]->slot; j++)
				item_tmp.card[j]=0;
			for (j = sd->inventory_data[i]->slot; j < MAX_SLOTS; j++)
				item_tmp.card[j]=sd->status.inventory[i].card[j];
			pc_delitem(sd,i,1,0);

			//Logs items, got from (N)PC scripts [Lupus]
			if(log_config.enable_logs&0x40)
				log_pick_pc(sd, "N", item_tmp.nameid, 1, &item_tmp);

			if((flag=pc_additem(sd,&item_tmp,1))){
				clif_additem(sd,0,0,flag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0);
			}
		}
		clif_misceffect(&sd->bl,2);
	}

	return 0;
}

static int buildin_areawarp_sub(struct block_list *bl,va_list ap)
{
	int x,y;
	unsigned int map;
	map=va_arg(ap, unsigned int);
	x=va_arg(ap,int);
	y=va_arg(ap,int);
	if(map == 0)
		pc_randomwarp((TBL_PC *)bl,3);
	else
		pc_setpos((TBL_PC *)bl,map,x,y,0);
	return 0;
}

LUA_FUNC(mapwarp)
{
	int x,y,m,check_val=0,check_ID=0,i=0;
	struct guild *g = NULL;
	struct party_data *p = NULL;
	const char *str;
	const char *mapname;
	unsigned int index;
	mapname=luaL_checkstring(NL,1);
	str=luaL_checkstring(NL,2);
	x=luaL_checkint(NL,3);
	y=luaL_checkint(NL,4);
	
	if( lua_isnumber(NL,5) ) {
		check_val = luaL_checkint(NL,5);
		check_ID = luaL_checkint(NL,6);
	}

	if((m=map_mapname2mapid(mapname))< 0)
		return 0;

	if(!(index=mapindex_name2id(str)))
		return 0;

	switch(check_val){
		case 1:
			g = guild_search(check_ID);
			if (g){
				for( i=0; i < g->max_member; i++)
				{
					if(g->member[i].sd && g->member[i].sd->bl.m==m){
						pc_setpos(g->member[i].sd,index,x,y,3);
					}
				}
			}
			break;
		case 2:
			p = party_search(check_ID);
			if(p){
				for(i=0;i<MAX_PARTY; i++){
					if(p->data[i].sd && p->data[i].sd->bl.m == m){
						pc_setpos(p->data[i].sd,index,x,y,3);
					}
				}
			}
			break;
		default:
			map_foreachinmap(buildin_areawarp_sub,m,BL_PC,index,x,y);
			break;
	}

	return 0;
}

static int buildin_mobcount_sub(struct block_list *bl,va_list ap)
{
	char *event=va_arg(ap,char *);
	struct mob_data *md = ((struct mob_data *)bl);
	if(strcmp(event,md->function)==0 && md->status.hp > 0)
		return 1;
	return 0;
}

LUA_FUNC(mobcount)
{
	const char *mapname,*event;
	int m;
	mapname=luaL_checkstring(NL,1);
	event=luaL_checkstring(NL,2);

	if( (m = map_mapname2mapid(mapname)) < 0 ) {
		return 0;
	}
	/* TODO ## Instance Support
	if( map[m].flag.src4instance && map[m].instance_id == 0 && st->instance_id && (m = instance_mapid2imapid(m, st->instance_id)) < 0 )
	{
		return 0;
	}
	*/
	lua_pushinteger(NL,map_foreachinmap(buildin_mobcount_sub, m, BL_MOB, event));
	return 0;
}

LUA_FUNC(marriage)
{
	lua_get_target(1);
	const char *partner=luaL_checkstring(NL,2);
	TBL_PC *p_sd=map_nick2sd(partner);
	if(sd==NULL || p_sd==NULL || pc_marriage(sd,p_sd) < 0){
		return 0;
	}
	lua_pushinteger(NL,1);
	return 1;
}

LUA_FUNC(wedding_effect)
{
	lua_get_target(2);
	struct block_list *bl;
	if(sd==NULL) {
		bl=map_id2bl(luaL_checkint(NL,1));
	} else
		bl=&sd->bl;
	clif_wedding_effect(bl);
	return 0;
}

LUA_FUNC(divorce)
{
	lua_get_target(1);
	if(sd==NULL || pc_divorce(sd) < 0){
		return 0;
	}
	lua_pushinteger(NL,1);
	return 1;
}

LUA_FUNC(ispartneron)
{
	lua_get_target(1);
	TBL_PC *p_sd=NULL;
	if(sd==NULL || !pc_ismarried(sd) ||
            (p_sd=map_charid2sd(sd->status.partner_id)) == NULL) {
		return 0;
	}
	lua_pushinteger(NL,1);
	return 1;
}

LUA_FUNC(getpartnerid)
{
    lua_get_target(1);
    if (sd == NULL) {
        return 0;
    }
    lua_pushinteger(NL,sd->status.partner_id);
    return 0;
}

LUA_FUNC(getchildid)
{
    lua_get_target(1);
    if (sd == NULL) {
        return 0;
    }
    lua_pushinteger(NL,sd->status.child);
    return 0;
}

LUA_FUNC(getmotherid)
{
    lua_get_target(1);
    if (sd == NULL) {
        return 0;
    }
    lua_pushinteger(NL,sd->status.mother);
    return 0;
}

LUA_FUNC(getfatherid)
{
    lua_get_target(1);
    if (sd == NULL) {
        return 0;
    }
	lua_pushinteger(NL,sd->status.father);
    return 0;
}

LUA_FUNC(warppartner)
{
	int x,y;
	unsigned short mapindex;
	const char *str;
	lua_get_target(4);
	TBL_PC *p_sd=NULL;
	if(sd==NULL || !pc_ismarried(sd) ||
   	(p_sd=map_charid2sd(sd->status.partner_id)) == NULL) {
		return 0;
	}
	str=luaL_checkstring(NL,1);
	x=luaL_checkint(NL,2);
	y=luaL_checkint(NL,3);
	mapindex = mapindex_name2id(str);
	if (mapindex) {
		pc_setpos(p_sd,mapindex,x,y,0);
		lua_pushinteger(NL,1);
		return 1;
	}
	return 0;
}

LUA_FUNC(strmobinfo)
{
	int num=luaL_checkint(NL,1);
	int class_=luaL_checkint(NL,2);
	if(!mobdb_checkid(class_))
		return 0;
	switch (num) {
	case 1: lua_pushstring(NL,mob_db(class_)->name); break;
	case 2: lua_pushstring(NL,mob_db(class_)->jname); break;
	case 3: lua_pushinteger(NL,mob_db(class_)->lv); break;
	case 4: lua_pushinteger(NL,mob_db(class_)->status.max_hp); break;
	case 5: lua_pushinteger(NL,mob_db(class_)->status.max_sp); break;
	case 6: lua_pushinteger(NL,mob_db(class_)->base_exp); break;
	case 7: lua_pushinteger(NL,mob_db(class_)->job_exp); break;
	default:
		return 0;
	}
	return 1;
}

LUA_FUNC(guardian)
{
	int class_=0,x=0,y=0,guardian=0;
	const char *str,*map,*evt="";
	bool has_index = false;
	map	  =luaL_checkstring(NL,1);
	x	  =luaL_checkint(NL,2);
	y	  =luaL_checkint(NL,3);
	str	  =luaL_checkstring(NL,4);
	class_=luaL_checkint(NL,5);
	if( lua_isnumber(NL,6) )
	{// "<event label>",<guardian index>
		guardian=luaL_checkint(NL,6);
		has_index = true;
		if( lua_isstring(NL,7) )
		{
			evt=luaL_checkstring(NL,7);
		}
	}
	lua_pushinteger(NL, mob_spawn_guardian(map,x,y,str,class_,evt,guardian,has_index));
	return 1;
}

LUA_FUNC(setwall)
{
	const char *map, *name;
	int x, y, m, size, dir;
	bool shootable;
	
	map = luaL_checkstring(NL,1);
	x = luaL_checkint(NL,2);
	y = luaL_checkint(NL,3);
	size = luaL_checkint(NL,4);
	dir = luaL_checkint(NL,5);
	shootable = luaL_checkint(NL,6);
	name = luaL_checkstring(NL,7);

	if( (m = map_mapname2mapid(map)) < 0 )
		return 0; // Invalid Map

	map_iwall_set(m, x, y, size, dir, shootable, name);
	return 0;
}

LUA_FUNC(delwall)
{
	const char *name = luaL_checkstring(NL,1);
	map_iwall_remove(name);
	return 0;
}

LUA_FUNC(guardianinfo)
{
	const char* mapname = mapindex_getmapname(luaL_checkstring(NL,1),NULL);
	int id = luaL_checkint(NL,2);
	int type = luaL_checkint(NL,3);

	struct guild_castle* gc = guild_mapname2gc(mapname);
	struct mob_data* gd;

	if( gc == NULL || id < 0 || id >= MAX_GUARDIANS )
	{
		return 0;
	}

	if( type == 0 )
		return 0;
	else
	if( !gc->guardian[id].visible )
		return 0;
	else
	if( (gd = map_id2md(gc->guardian[id].id)) == NULL )
		return 0;
	else
	{
		if     ( type == 1 ) lua_pushinteger(NL,gd->status.max_hp);
		else if( type == 2 ) lua_pushinteger(NL,gd->status.hp);
		else
			return 0;
	}

	return 1;
}

LUA_FUNC(getitemname)
{
	struct item_data *i_data;
	char *item_name;
	int item_id = luaL_checkint(NL,1);
	i_data = itemdb_exists(item_id);
	if (i_data == NULL)
		return 0;
	item_name=(char *)aMallocA(ITEM_NAME_LENGTH*sizeof(char));
	memcpy(item_name, i_data->jname, ITEM_NAME_LENGTH);
	lua_pushstring(NL,item_name);
	return 0;
}

LUA_FUNC(getitemslots)
{
	struct item_data *i_data;
	int item_id=luaL_checkint(NL,1);
	i_data = itemdb_exists(item_id);
	if (i_data)
		lua_pushnumber(NL,i_data->slot);
	else
		return 0;
	return 1;
}

LUA_FUNC(iteminfo)
{
	int item_id,n,flag;
	int *item_arr;
	struct item_data *i_data;
	item_id	= luaL_checkint(NL,1);
	n = luaL_checkint(NL,2);
	flag = luaL_checkint(NL,3); // 0 = get item info , 1 = set item info
	i_data = itemdb_exists(item_id);
	if (i_data && n>=0 && n<=14) {
		item_arr = (int*)&i_data->value_buy;
		if (flag) {
			item_arr[n] = luaL_checkint(NL,4);
			lua_pushinteger(NL,item_arr[n]);
		}
		else
			lua_pushinteger(NL,item_arr[n]);
	} else
		lua_pushnil(NL);
	return 1;
}

LUA_FUNC(getequipcardid)
{
	int i=-1,num,slot;
	lua_get_target(3);
	num=luaL_checkint(NL,1);
	slot=luaL_checkint(NL,2);
	if (num > 0 && num <= ARRAYLENGTH(equip))
		i=pc_checkequip(sd,equip[num-1]);
		
	if(i >= 0 && slot>=0 && slot<4)
		lua_pushinteger(NL,sd->status.inventory[i].card[slot]);
	else
		lua_pushnil(NL);

	return 1;
}

LUA_FUNC(petskillbonus)
{
	struct pet_data *pd;
	
	lua_get_target(1);
	nullpo_retr(0,sd);

	pd=sd->pd;
	if (pd->bonus)
	{ //Clear previous bonus
		if (pd->bonus->timer != -1)
			delete_timer(pd->bonus->timer, pet_skill_bonus_timer);
	} else //init
		pd->bonus = (struct pet_bonus *) aMalloc(sizeof(struct pet_bonus));

	pd->bonus->type=luaL_checkint(NL,1);
	pd->bonus->val=luaL_checkint(NL,2);
	pd->bonus->duration=luaL_checkint(NL,3);
	pd->bonus->delay=luaL_checkint(NL,4);

	if (pd->state.skillbonus == 1)
		pd->state.skillbonus=0;	// waiting state

	// wait for timer to start
	if (battle_config.pet_equip_required && pd->pet.equip == 0)
		pd->bonus->timer = INVALID_TIMER;
	else
		pd->bonus->timer = add_timer(gettick()+pd->bonus->delay*1000, pet_skill_bonus_timer, sd->bl.id, 0);

	return 0;
}

LUA_FUNC(petloot)
{
	int max;
	struct pet_data *pd;
	lua_get_target(2);
	max=luaL_checkint(NL,1);

	if(max < 1)
		max = 1;	//Let'em loot at least 1 item.
	else if (max > MAX_PETLOOT_SIZE)
		max = MAX_PETLOOT_SIZE;
	
	pd = sd->pd;
	if (pd->loot != NULL)
	{	//Release whatever was there already and reallocate memory
		pet_lootitem_drop(pd, pd->msd);
		aFree(pd->loot->item);
	}
	else
		pd->loot = (struct pet_loot *)aMalloc(sizeof(struct pet_loot));

	pd->loot->item = (struct item *)aCalloc(max,sizeof(struct item));
	
	pd->loot->max=max;
	pd->loot->count = 0;
	pd->loot->weight = 0;

	return 0;
}

LUA_FUNC(_getinventorylist)
{
	lua_get_target(2);
	int i,j=0,k,val;
	nullpo_retr(0,sd);

	val=luaL_checkint(NL,1);
	lua_newtable(NL);
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].amount > 0){
			switch(val)
			{
				case 1:
					lua_pushinteger(NL,sd->status.inventory[i].nameid);
					lua_rawseti(NL,-2,i+1);
					break;
				case 2:
					lua_pushinteger(NL,sd->status.inventory[i].amount);
					lua_rawseti(NL,-2,i+1);
					break;
				case 3:
					lua_pushinteger(NL,sd->status.inventory[i].equip);
					lua_rawseti(NL,-2,i+1);
					break;
				case 4:
					lua_pushinteger(NL,sd->status.inventory[i].refine);
					lua_rawseti(NL,-2,i+1);
					break;
				case 5:
					lua_pushinteger(NL,sd->status.inventory[i].identify);
					lua_rawseti(NL,-2,i+1);
					break;
				case 6:
					lua_pushinteger(NL,sd->status.inventory[i].attribute);
					lua_rawseti(NL,-2,i+1);
					break;
				default:
					break;
			}
			for (k = 0; k < MAX_SLOTS; k++)
			{
				switch(val)
				{
					case 7:
						lua_pushinteger(NL,sd->status.inventory[i].card[k]);
						lua_rawseti(NL,-2,i+1);
					default:
						break;
				}
			}
			switch(val)
			{
				case 9:
					lua_pushinteger(NL,sd->status.inventory[i].expire_time);
					lua_rawseti(NL,-2,i+1);
				default:
					break;
			}
			j++;
		}
	}
	switch(val)
	{
		case 10:
			lua_pushinteger(NL,j);
		default:
			break;
	}
	return 1;
}

LUA_FUNC(_getskilllist)
{
	lua_get_target(2);
	int i,j=0;
	int val=luaL_checkint(NL,1);
	nullpo_retr(0,sd);
	lua_newtable(NL);
	for(i=0;i<MAX_SKILL;i++){
		if(sd->status.skill[i].id > 0 && sd->status.skill[i].lv > 0){
			switch(val)
			{
				case 1:
					lua_pushinteger(NL,sd->status.skill[i].id);
					lua_rawseti(NL,-2,i+1);
					break;
				case 2:
					lua_pushinteger(NL,sd->status.skill[i].lv);
					lua_rawseti(NL,-2,i+1);
					break;
				case 3:
					lua_pushinteger(NL,sd->status.skill[i].flag);
					lua_rawseti(NL,-2,i+1);
				default:
					break;
			}
			j++;
		}
	}
	switch(val)
	{
		case 4:
			lua_pushinteger(NL,j);
			lua_rawseti(NL,-2,i+1);
			break;
		default:
			break;
	}
	return 1;
}

LUA_FUNC(clearitem)
{
	lua_get_target(1);
	int i;
	nullpo_retr(0,sd);
	for (i=0; i<MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].amount) {

			//Logs items, got from (N)PC scripts
			if(log_config.enable_logs&0x40)
				log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -sd->status.inventory[i].amount, &sd->status.inventory[i]);

			pc_delitem(sd, i, sd->status.inventory[i].amount, 0);
		}
	}
	return 0;
}

LUA_FUNC(_disguise)
{
	int id,val;
	lua_get_target(3);
	nullpo_retr(0,sd);
	id = luaL_checkint(NL,1);
	val = luaL_checkint(NL,2);
	if (mobdb_checkid(id) || npcdb_checkid(id)) {
		if (val) {
			pc_disguise(sd, 0);
			lua_pushinteger(NL,id);
		}
		else {
			pc_disguise(sd, id);
			lua_pushinteger(NL,id);
		}
	} else
		lua_pushnil(NL);

	return 1;
}

LUA_FUNC(misceffect)
{
	int type;
	int obj;
	obj=luaL_checkint(NL,1);
	type=luaL_checkint(NL,2);
	if(obj && obj != fake_nd->bl.id) {
		struct block_list *bl = map_id2bl(obj);
		if (bl)
			clif_misceffect2(bl,type);
	} else{
		lua_get_target(3);
		if(sd)
			clif_misceffect2(&sd->bl,type);
	}
	return 0;
}

LUA_FUNC(soundeffect)
{
	const char* name = luaL_checkstring(NL,1);
	int type = luaL_checkint(NL,2);
	int obj = luaL_checkint(NL,3);

	struct map_session_data *sd = map_charid2sd(obj);
	if(sd)
	{
		if(!sd->status.account_id)
			clif_soundeffect(sd,map_id2bl(obj),name,type);
		else
			clif_soundeffect(sd,&sd->bl,name,type);
	}
	return 0;
}

static int soundeffect_sub(struct block_list* bl,va_list ap)
{
	char* name = va_arg(ap,char*);
	int type = va_arg(ap,int);

	clif_soundeffect((TBL_PC *)bl, bl, name, type);

    return 0;	
}

LUA_FUNC(petrecovery)
{
	struct pet_data *pd;
	lua_get_target(3);
	
	nullpo_retr(0,sd);
	pd=sd->pd;
	
	if (pd->recovery)
	{ //Halt previous bonus
		if (pd->recovery->timer != -1)
			delete_timer(pd->recovery->timer, pet_recovery_timer);
	} else //Init
		pd->recovery = (struct pet_recovery *)aMalloc(sizeof(struct pet_recovery));
		
	pd->recovery->type = (sc_type)luaL_checkint(NL,1);
	pd->recovery->delay = luaL_checkint(NL,2);
	pd->recovery->timer = INVALID_TIMER;

	return 0;
}

LUA_FUNC(petheal)
{
	struct pet_data *pd;
	lua_get_target(5);
	nullpo_retr(0,sd);
	pd=sd->pd;
	nullpo_retr(0,pd);
	if (pd->s_skill)
	{ //Clear previous skill
		if (pd->s_skill->timer != -1)
		{
			if (pd->s_skill->id)
				delete_timer(pd->s_skill->timer, pet_skill_support_timer);
			else
				delete_timer(pd->s_skill->timer, pet_heal_timer);
		}
	} else //init memory
		pd->s_skill = (struct pet_skill_support *) aMalloc(sizeof(struct pet_skill_support)); 
	
	pd->s_skill->id=0; //This id identifies that it IS petheal rather than pet_skillsupport
	//Use the lv as the amount to heal
	pd->s_skill->lv=luaL_checkint(NL,1);
	pd->s_skill->delay=luaL_checkint(NL,2);
	pd->s_skill->hp=luaL_checkint(NL,3);
	pd->s_skill->sp=luaL_checkint(NL,4);

	//Use delay as initial offset to avoid skill/heal exploits
	if (battle_config.pet_equip_required && pd->pet.equip == 0)
		pd->s_skill->timer = INVALID_TIMER;
	else
		pd->s_skill->timer = add_timer(gettick()+pd->s_skill->delay*1000,pet_heal_timer,sd->bl.id,0);

	return 0;
}

LUA_FUNC(petskillattack)
{
	struct pet_data *pd;
	lua_get_target(5);
	pd=sd->pd;
	nullpo_retr(0,sd);
	nullpo_retr(0,pd);
	if (pd->a_skill == NULL)
		pd->a_skill = (struct pet_skill_attack *)aMalloc(sizeof(struct pet_skill_attack));
				
	pd->a_skill->id=( lua_isstring(NL,1) ? skill_name2id(luaL_checkstring(NL,1)) : luaL_checkint(NL,1) );
	pd->a_skill->lv=luaL_checkint(NL,2);
	pd->a_skill->div_ = luaL_checkint(NL,3);
	pd->a_skill->rate=luaL_checkint(NL,4);
	pd->a_skill->bonusrate=luaL_checkint(NL,5);

	return 0;
}

LUA_FUNC(petskillsupport)
{
	struct pet_data *pd;
	lua_get_target(6);
	nullpo_retr(0,sd);
	pd=sd->pd;
	nullpo_retr(0,pd);
	if (pd->s_skill)
	{ //Clear previous skill
		if (pd->s_skill->timer != -1)
		{
			if (pd->s_skill->id)
				delete_timer(pd->s_skill->timer, pet_skill_support_timer);
			else
				delete_timer(pd->s_skill->timer, pet_heal_timer);
		}
	} else //init memory
		pd->s_skill = (struct pet_skill_support *) aMalloc(sizeof(struct pet_skill_support)); 
	
	pd->s_skill->id=( lua_isstring(NL,1) ? skill_name2id(luaL_checkstring(NL,1)) : luaL_checkint(NL,1) );
	pd->s_skill->lv=luaL_checkint(NL,2);
	pd->s_skill->delay=luaL_checkint(NL,3);
	pd->s_skill->hp=luaL_checkint(NL,4);
	pd->s_skill->sp=luaL_checkint(NL,5);

	//Use delay as initial offset to avoid skill/heal exploits
	if (battle_config.pet_equip_required && pd->pet.equip == 0)
		pd->s_skill->timer = INVALID_TIMER;
	else
		pd->s_skill->timer = add_timer(gettick()+pd->s_skill->delay*1000,pet_skill_support_timer,sd->bl.id,0);

	return 0;
}

LUA_FUNC(skilleffect)
{
	int skillid,skilllv;
	lua_get_target(3);
	skillid=( lua_isstring(NL,1) ? skill_name2id(luaL_checkstring(NL,1)) : luaL_checkint(NL,1) );
	skilllv=luaL_checkint(NL,2);
	clif_skill_nodamage(&sd->bl,&sd->bl,skillid,skilllv,1);
	return 0;
}

LUA_FUNC(npcskilleffect)
{
	int skillid,skilllv,x,y;
	struct block_list *bl= map_id2bl(luaL_checkint(NL,1));
	nullpo_retr(0,bl);
	skillid=( lua_isstring(NL,2) ? skill_name2id(luaL_checkstring(NL,2)) : luaL_checkint(NL,2) );
	skilllv=luaL_checkint(NL,3);
	x=luaL_checkint(NL,4);
	y=luaL_checkint(NL,5);

	if (bl)
		clif_skill_poseffect(bl,skillid,skilllv,x,y,gettick());

	return 0;
}

LUA_FUNC(specialeffect)
{
	struct block_list *bl=map_id2bl(luaL_checkint(NL,1));
	int type = luaL_checkint(NL,2);
	int val = luaL_checkint(NL,3);
	enum send_target target = val;
	
	if (!val)
		target = AREA;
	
	nullpo_retr(0,bl);
	clif_specialeffect(bl, type, target);
	return 0;
}

LUA_FUNC(nude)
{
	lua_get_target(1);
	int i,calcflag=0;
	nullpo_retr(0,sd);
	for(i=0;i<11;i++)
		if(sd->equip_index[i] >= 0) {
			if(!calcflag)
				calcflag=1;
			pc_unequipitem(sd,sd->equip_index[i],2);
		}
	if(calcflag)
		status_calc_pc(sd,0);
	return 0;
}

LUA_FUNC(gmcommand)
{
	TBL_PC dummy_sd;
	TBL_PC* sd;
	struct block_list* bl;
	int fd,obj;
	const char* cmd;
	cmd = luaL_checkstring(NL,2);
	obj = luaL_checkint(NL,1);
	sd = map_id2sd(obj);
	if (sd) {
		fd = sd->fd;
	} else { //Use a dummy character.
		sd = &dummy_sd;
		fd = 0;
		memset(&dummy_sd, 0, sizeof(TBL_PC));
		bl = map_id2bl(obj);
		if (bl)
		{
			memcpy(&dummy_sd.bl, bl, sizeof(struct block_list));
			if (bl->type == BL_NPC)
				safestrncpy(dummy_sd.status.name, ((TBL_NPC*)bl)->name, NAME_LENGTH);
		}
	}

	// compatibility with previous implementation (deprecated!)
	if(cmd[0] != atcommand_symbol)
	{
		cmd += strlen(sd->status.name);
		while(*cmd != atcommand_symbol && *cmd != 0)
			cmd++;
	}

	is_atcommand(fd, sd, cmd, 0);
	return 0;
}

LUA_FUNC(disp)
{
	lua_get_target(2);
	const char *message;
	message=luaL_checkstring(NL,1);
	if(sd)
		clif_disp_onlyself(sd,message,(int)strlen(message));
	return 0;
}

LUA_FUNC(recovery)
{
	lua_get_target(1);
	struct s_mapiterator* iter;
	nullpo_retr(0,sd);
	iter = mapit_getallusers();
	for( sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); sd = (TBL_PC*)mapit_next(iter) )
	{
		if(pc_isdead(sd))
			status_revive(&sd->bl, 100, 100);
		else
			status_percent_heal(&sd->bl, 100, 100);
		clif_displaymessage(sd->fd,"You have been recovered!");
	}
	mapit_free(iter);
	return 0;
}

LUA_FUNC(getpetinfo)
{
	lua_get_target(2);
	TBL_PET *pd;
	int type=luaL_checkint(NL,1);
	nullpo_retr(0,sd);
	nullpo_retr(0,sd->pd);
	pd = sd->pd;
	switch(type){
		case 0: lua_pushinteger(NL,pd->pet.pet_id); break;
		case 1: lua_pushinteger(NL,pd->pet.class_); break;
		case 2: lua_pushstring(NL,pd->pet.name); break;
		case 3: lua_pushinteger(NL,pd->pet.intimate); break;
		case 4: lua_pushinteger(NL,pd->pet.hungry); break;
		case 5: lua_pushinteger(NL,pd->pet.rename_flag); break;
		default:
			lua_pushnil(NL);
			break;
	}
	return 1;
}



// List of commands to build into Lua, format : {"function_name_in_lua", C_function_name}
#define LUA_DEF(x) {#x, buildin_ ##x}
static struct LuaCommandInfo commands[] = {
	LUA_DEF(scriptend),
	LUA_DEF(_addnpc),
	LUA_DEF(addareascript),
	LUA_DEF(addwarp),
	LUA_DEF(addspawn),
	LUA_DEF(mes),
	LUA_DEF(npcmenu),
	LUA_DEF(close),
	LUA_DEF(npcnext),
	LUA_DEF(input),
	LUA_DEF(npcshop),
	LUA_DEF(cutin),
	LUA_DEF(heal),
	LUA_DEF(percentheal),
	LUA_DEF(itemheal),
	LUA_DEF(warp),
	LUA_DEF(jobchange),
	LUA_DEF(setlook),
	LUA_DEF(changelook),
	LUA_DEF(getjobname),
	LUA_DEF(getitemcount),
	LUA_DEF(getitemcount2),
	LUA_DEF(getweight),
	LUA_DEF(giveitem),
	LUA_DEF(giveitem2),
	LUA_DEF(getparam),
	LUA_DEF(getrentalitem),
	LUA_DEF(getcharid),
	LUA_DEF(getpartyname),
	LUA_DEF(getpartymember),
	LUA_DEF(itemusedisable),
	LUA_DEF(itemuseenable),
	LUA_DEF(givenameditem),
	LUA_DEF(getrandgroupitem),
	LUA_DEF(additem),
	LUA_DEF(deleteitem),
	LUA_DEF(deleteitem2),
	LUA_DEF(getpartyleader),
	LUA_DEF(getguildname),
	LUA_DEF(getguildmasterid),
	LUA_DEF(strnpcinfo),
	LUA_DEF(getequipid),
	LUA_DEF(getequipname),
	LUA_DEF(getbrokenid),
	LUA_DEF(repair),
	LUA_DEF(getequipisenableref),
	LUA_DEF(getequiprefinerycnt),
	LUA_DEF(getequipweaponlv),
	LUA_DEF(getequippercentrefinery),
	LUA_DEF(successrefitem),
	LUA_DEF(failedrefitem),
	LUA_DEF(statusup),
	LUA_DEF(statusup2),
	LUA_DEF(bonus),
	LUA_DEF(autobonus),
	LUA_DEF(autobonus2),
	LUA_DEF(autobonus3),
	LUA_DEF(skill),
	LUA_DEF(guildskill),
	LUA_DEF(getskilllv),
	LUA_DEF(getgdskilllv),
	LUA_DEF(basicskillcheck),
	LUA_DEF(getgmlevel),
	LUA_DEF(checkoption),
	LUA_DEF(checkoption1),
	LUA_DEF(checkoption2),
	LUA_DEF(setoption),
	LUA_DEF(checkcart),
	LUA_DEF(setcart),
	LUA_DEF(checkfalcon),
	LUA_DEF(setfalcon),
	LUA_DEF(checkriding),
	LUA_DEF(setriding),
	LUA_DEF(savepoint),
	LUA_DEF(gettimetick),
	LUA_DEF(gettime),
	LUA_DEF(gettimestr),
	LUA_DEF(openstorage),
	LUA_DEF(guildopenstorage),
	LUA_DEF(itemskill),
	LUA_DEF(produce),
	LUA_DEF(cooking),
	LUA_DEF(makepet),
	LUA_DEF(getexp),
	LUA_DEF(guildgetexp),
	LUA_DEF(guildchangegm),
	LUA_DEF(monster),
	LUA_DEF(getmobdrops),
	LUA_DEF(areamonster),
	LUA_DEF(killmonster),
	LUA_DEF(killmonsterall),
	LUA_DEF(clone),
	LUA_DEF(addtimer),
	LUA_DEF(deltimer),
	LUA_DEF(addtimercount),
	LUA_DEF(addnpctimer),
	LUA_DEF(announce),
	LUA_DEF(mapannounce),
	LUA_DEF(areaannounce),
	LUA_DEF(getusers),
	LUA_DEF(getusersname),
	LUA_DEF(getmapguildusers),
	LUA_DEF(getmapusers),
	LUA_DEF(getareausers),
	LUA_DEF(getareadropitem),
	LUA_DEF(npc_enable),
	LUA_DEF(sc_start),
	LUA_DEF(sc_start2),
	LUA_DEF(sc_start4),
	LUA_DEF(sc_end),
	LUA_DEF(getscrate),
	LUA_DEF(catchpet),
	LUA_DEF(homunculus_evolution),
	LUA_DEF(homunculus_shuffle),
	LUA_DEF(eaclass),
	LUA_DEF(roclass),
	LUA_DEF(birthpet),
	LUA_DEF(resetlvl),
	LUA_DEF(resetstatus),
	LUA_DEF(resetskill),
	LUA_DEF(skillpointcount),
	LUA_DEF(changebase),
	LUA_DEF(changesex),
	LUA_DEF(waitingroom),
	LUA_DEF(delwaitingroom),
	LUA_DEF(waitingroomkickall),
	LUA_DEF(enablewaitingroomevent),
	LUA_DEF(disablewaitingroomevent),
	LUA_DEF(getwaitingroomstate),
	LUA_DEF(warpwaitingpc),
	LUA_DEF(isloggedin),
	LUA_DEF(setmapflagnosave),
	LUA_DEF(getmapflag),
	LUA_DEF(editmapflag),
	LUA_DEF(pvpon),
	LUA_DEF(pvpoff),
	LUA_DEF(gvgon),
	LUA_DEF(gvgoff),
	LUA_DEF(emotion),
	LUA_DEF(maprespawnguildid),
	LUA_DEF(agitstart),
	LUA_DEF(agitend),
	LUA_DEF(agitstart2),
	LUA_DEF(agitend2),
	LUA_DEF(agitcheck),
	LUA_DEF(flagemblem),
	LUA_DEF(getcastlename),
	LUA_DEF(getcastledata),
	LUA_DEF(setcastledata),
	LUA_DEF(requestguildinfo),
	LUA_DEF(getequipcardcnt),
	LUA_DEF(successremovecards),
	LUA_DEF(failedremovecards),
	LUA_DEF(mapwarp),
	LUA_DEF(mobcount),
	LUA_DEF(marriage),
	LUA_DEF(wedding_effect),
	LUA_DEF(divorce),
	LUA_DEF(ispartneron),
	LUA_DEF(getpartnerid),
	LUA_DEF(getchildid),
	LUA_DEF(getmotherid),
	LUA_DEF(getfatherid),
	LUA_DEF(warppartner),
	LUA_DEF(strmobinfo),
	LUA_DEF(guardian),
	LUA_DEF(setwall),
	LUA_DEF(delwall),
	LUA_DEF(guardianinfo),
	LUA_DEF(getitemname),
	LUA_DEF(getitemslots),
	LUA_DEF(iteminfo),
	LUA_DEF(getequipcardid),
	LUA_DEF(petskillbonus),
	LUA_DEF(petloot),
	LUA_DEF(_getinventorylist),
	LUA_DEF(_getskilllist),
	LUA_DEF(clearitem),
	LUA_DEF(_disguise),
	LUA_DEF(misceffect),
	LUA_DEF(soundeffect),
	//LUA_DEF(soundeffectall),
	LUA_DEF(petrecovery),
	LUA_DEF(petheal),
	LUA_DEF(petskillattack),
	//LUA_DEF(petskillattack2),
	LUA_DEF(petskillsupport),
	LUA_DEF(skilleffect),
	LUA_DEF(npcskilleffect),
	LUA_DEF(specialeffect),
	//LUA_DEF(specialeffect2),
	LUA_DEF(nude),
	LUA_DEF(gmcommand),
	LUA_DEF(disp),
	LUA_DEF(recovery),
	LUA_DEF(getpetinfo),
	// End of build-in functions list
	{"-End of list-", NULL},
};

//-----------------------------------------------------------------------------
// LUA SCRIPT FUNCTIONS
//

int do_init_luascript()
{
	L = lua_open();
	luaL_openlibs(L);	
	lua_pushliteral(L,"char_id");
	lua_pushnumber(L,0);
	lua_rawset(L,LUA_GLOBALSINDEX);	
	script_buildin_commands_lua();
	return 0;
}

void do_final_luascript()
{
	lua_close(L);
}

void script_buildin_commands_lua()
{
	int i=0;
	
	for( i = 0; commands[i].f; i++ )
	{
		lua_pushstring(L, commands[i].command);
        lua_pushcfunction(L, commands[i].f);
        lua_settable(L, LUA_GLOBALSINDEX);
	}
	
	ShowStatus("Done registering '"CL_WHITE"%d"CL_RESET"' script build-in commands.\n",i);
}

// Check whether a char ID was passed as argument, else check if there's a global one
static struct map_session_data* script_get_target(lua_State *NL,int idx)
{
	int char_id;
	struct map_session_data* sd;

	if((char_id=(int)lua_tointeger(NL, idx))==0) { // If 0 or nothing was passed as argument
		lua_pushliteral(NL, "char_id");
		lua_rawget(NL, LUA_GLOBALSINDEX);
		char_id=(int)lua_tointeger(NL, -1); // Get the thread's char ID if it's a personal one
		lua_pop(NL, 1);
	}

	if(char_id==0 || (sd=map_charid2sd(char_id))==NULL) { // If we still dont have a valid char ID here, there's a problem
		ShowError("Target character not found for script command\n");
		return NULL;
	}
	
	return sd;
}

/* Run a Lua function that was previously loaded, specifying the type of arguments with a "format" string

 A bit of an explanation on how format works. When a lua function is called using script_run_function, these values are pushed as arguments to the function.
 The args can be named whatever in lua.
 
 ex: script_run_function(nd->function,sd->status.char_id,"ii",sd->status.char_id,nd->bl.id)
 run from npc_click() will push the character's id and the npc's id as arguments in
 
 function foo(arg,arg2) (let's say nd-> function was "foo")
 end

 And if you're wondering, the stack size is 20 for these function calls. [sketchyphoenix]
*/
void script_run_function(const char *name,int char_id,const char *format,...)
{
	va_list arg;
	lua_State *NL;
	struct map_session_data *sd=NULL;
	int n=0;

	if (char_id == 0) { // If char_id points to no player
		NL = L; // Use the global thread
	} else { // Else we want to run the function for a specific player
		sd = map_charid2sd(char_id);
		nullpo_retv(sd);
		if(sd->lua_script_state!=L_NRUN) { // Check that the player is not currently running a script
			ShowError("Cannot run function %s for player %d : player is already running a script\n",name,char_id);
			return;
		}
		NL = sd->NL = lua_newthread(L); // Use the player's personal thread
		lua_pushliteral(NL,"char_id"); // Push global key for char_id
		lua_pushnumber(NL,char_id); // Push value for char_id
		lua_rawset(NL,LUA_GLOBALSINDEX); // Tell Lua to set char_id as a global var
	}

	lua_getglobal(NL,name); // Pass function name to Lua
 
	va_start(arg,format); // Initialize the argument list
	while (*format) { // Pass arguments to Lua, according to the types defined by "format"
        switch (*format++) {
          case 'd': // d = Double
            lua_pushnumber(NL,va_arg(arg,double));
            break;
          case 'i': // i = Integer
            lua_pushnumber(NL,va_arg(arg,int));
            break;
          case 's': // s = String
            lua_pushstring(NL,va_arg(arg,char*));
            break;
          default: // Unknown code
            ShowError("%c : Invalid argument type code, allowed codes are 'd'/'i'/'s'\n",*(format-1));
        }
        n++;
        luaL_checkstack(NL,1,"Too many arguments");
    }
	va_end(arg);
	
	// Tell Lua to run the function
	if ( lua_resume(NL,n)!=0 && lua_tostring(NL,-1) != NULL ) {
		//If it fails, print to console the error that lua returned.
		ShowError("Cannot run function %s : %s\n",name,lua_tostring(NL,-1));
		return;
	}

	if(sd && sd->lua_script_state==L_NRUN) { // If the script has finished (not waiting answer from client)
	    sd->NL=NULL; // Close the player's personal thread
		sd->npc_id=0; // Set the player's current NPC to 'none'
	}
}

// Run a Lua chunk
void script_run_chunk(const char *chunk,int char_id)
{
	lua_State *NL;
    struct map_session_data *sd=NULL;

	if (char_id == 0) { // If char_id points to no player
		NL = L; // Use the global thread
	} else { // Else we want to run the chunk for a specific player
		sd = map_charid2sd(char_id);
		nullpo_retv(sd);
		if(sd->lua_script_state!=L_NRUN) { // Check that the player is not currently running a script
			ShowError("Cannot run chunk %s for player %d : player is currently running a script\n",chunk,char_id);
			return;
		}
		NL = sd->NL = lua_newthread(L); // Else the player's personal thread
		lua_pushliteral(NL,"char_id"); // Push global key for char_id
		lua_pushnumber(NL,char_id); // Push value for char_id
		lua_rawset(NL,LUA_GLOBALSINDEX); // Tell Lua to set char_id as a global var
	}
    
	luaL_loadbuffer(NL,chunk,strlen(chunk),"chunk"); // Pass chunk to Lua
	if (lua_pcall(NL,0,0,0)!=0){ // Tell Lua to run the chunk
		ShowError("Cannot run chunk %s : %s\n",chunk,lua_tostring(NL,-1));
		return;
	}

	if(sd && sd->lua_script_state==L_NRUN) { // If the script has finished (not waiting answer from client)
	    sd->NL=NULL; // Close the player's personal thread
		sd->npc_id=0; // Set the player's current NPC to 'none'
	}
}

// Resume an already paused script
void script_resume(struct map_session_data *sd,const char *format,...) {
	va_list arg;
	int n=0;

	nullpo_retv(sd);
	
	if(sd->lua_script_state==L_NRUN) { // Check that the player is currently running a script
		ShowError("Cannot resume script for player %d : player is not running a script\n",sd->status.char_id);
		return;
	}
	sd->lua_script_state=L_NRUN; // Set the script flag as 'not waiting for anything'

	va_start(arg,format); // Initialize the argument list
	while (*format) { // Pass arguments to Lua, according to the types defined by "format"
        switch (*format++) {
          case 'd': // d = Double
            lua_pushnumber(sd->NL,va_arg(arg,double));
            break;
          case 'i': // i = Integer
            lua_pushnumber(sd->NL,va_arg(arg,int));
            break;
          case 's': // s = String
            lua_pushstring(sd->NL,va_arg(arg,char*));
            break;
          default: // Unknown code
            ShowError("%c : Invalid argument type code, allowed codes are 'd'/'i'/'s'\n",*(format-1));
        }
        n++;
        luaL_checkstack(sd->NL,1,"Too many arguments");
    }

    va_end(arg);

	/*Attempt to run the function otherwise print the error to the console.*/
	if ( lua_resume(sd->NL,n)!=0 && lua_tostring(sd->NL,-1) != NULL ) {
		ShowError("Cannot resume script for player %d : %s\n",sd->status.char_id,lua_tostring(sd->NL,-1));
		return;
	}

	if(sd->lua_script_state==L_NRUN) { // If the script has finished (not waiting answer from client)
	    sd->NL=NULL; // Close the player's personal thread
		sd->npc_id=0; // Set the player's current NPC to 'none'
	}
}