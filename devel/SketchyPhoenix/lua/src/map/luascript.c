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

lua_State *L;
static struct map_session_data* script_get_target(lua_State *NL,int idx);

extern enum { CLOSE, NRUN, NEXT, INPUT, MENU };

/*-----------------------------------------------------------------------------
 buildin functions for lua
 
 Elements in the lua stack use an index and those elements are pushed in order starting
 from 1. In other words, they're the arguments passed when a script command is used.
 
 Reminder: All lua script commands set the player that called the function as the target.
 LUA_GET_TARGET is used to do this. If a value is passed in the script for a char id,
 LUA_GET_TARGET will execute that command under the specified target. If no target
 is supplied, it is executed under the caller of the script.
*/
#define LUA_FUNC(x) static int buildin_ ## x (lua_State *NL)
#define LUA_GET_TARGET(x) struct map_session_data *sd = script_get_target(NL,x)

//Exits the script
LUA_FUNC(scriptend)
{
	return lua_yield(NL, 0);
}

//Adds a standard NPC that triggers a function when clicked
LUA_FUNC(addnpc)
{
	char name[NAME_LENGTH],exname[NAME_LENGTH],map[16],function[50];
	short m,x,y,dir,class_;
	
	sprintf(name, "%s", lua_tostring(NL,1));
	sprintf(exname, "%s", lua_tostring(NL,2));
	sprintf(map, "%s", lua_tostring(NL,3));
	m=map_mapname2mapid(map);
	x=(short)lua_tonumber(NL,4);
	y=(short)lua_tonumber(NL,5);
	dir=(short)lua_tonumber(NL,6);
	class_=(short)lua_tonumber(NL,7);
	sprintf(function, "%s", lua_tostring(NL,8));
	
	npc_add_lua(name,exname,m,x,y,dir,class_,function);

	return 0;
}

// npcmes("Text",[id])
// Print the text into the NPC dialog window of the player
LUA_FUNC(mes)
{
	char mes[512];
	LUA_GET_TARGET(2);

	sprintf(mes,"%s",lua_tostring(NL, 1));

	clif_scriptmes(sd, sd->npc_id, mes);

	return 0;
}

//Display a [Next] button in the NPC dialog window
//npcnext(id)
LUA_FUNC(next)
{
	LUA_GET_TARGET(1);

	clif_scriptnext(sd,sd->npc_id);

	sd->lua_script_state = L_NEXT;
	return lua_yield(NL, 0);
}

// npcclose([id])
// Display a [Close] button in the NPC dialog window of the player and pause the script until the button is clicked
LUA_FUNC(close)
{
	LUA_GET_TARGET(1);
	
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
	/*
	char name[24],map[16],function[50];
	short m,x1,y1,x2,y2;

	sprintf(name,"%s",lua_tostring(NL,1));
	sprintf(map,"%s",lua_tostring(NL,2));
	m=map_mapname2mapid(map);
	x1=(lua_tonumber(NL,5)>lua_tonumber(NL,3))?lua_tonumber(NL,3):lua_tonumber(NL,5);
	y1=(lua_tonumber(NL,6)>lua_tonumber(NL,4))?lua_tonumber(NL,4):lua_tonumber(NL,6);
	x2=(lua_tonumber(NL,5)>lua_tonumber(NL,3))?lua_tonumber(NL,5):lua_tonumber(NL,3);
	y2=(lua_tonumber(NL,6)>lua_tonumber(NL,4))?lua_tonumber(NL,6):lua_tonumber(NL,4);
	sprintf(function,"%s",lua_tostring(NL,7));

	areascript_add(name,m,x1,y1,x2,y2,function);
	*/
	return 0;
}

//Add a warp that moves players somewhere else when entered
// addwarp("Warp name","map.gat",x,y,"destmap.gat",destx,desty,xradius,yradius)
LUA_FUNC(addwarp)
{
	char name[24],map[16],destmap[16];
	short m,x,y;
	short destx,desty,xs,ys;
	
	sprintf(name,"%s",(char)lua_tostring(NL,1));
	sprintf(map,"%s",(char)lua_tostring(NL,2));
	m=map_mapname2mapid(map);
	x=(short)lua_tonumber(NL,3);
	y=(short)lua_tonumber(NL,4);
	sprintf(destmap,"%s",(char)lua_tostring(NL,5));
	destx=(short)lua_tonumber(NL,6);
	desty=(short)lua_tonumber(NL,7);
	xs=(short)lua_tonumber(NL,8);
	ys=(short)lua_tonumber(NL,9);

	npc_warp_add_lua(name,m,x,y,destmap,destx,desty,xs,ys);

	return 0;
}

//Add a monster spawn
LUA_FUNC(addspawn)
{
	char name[24],map[16],function[50];
	short m,x,y,xs,ys,class_,num,d1,d2;

	sprintf(name,"%s",lua_tostring(NL,1));
	sprintf(map,"%s",lua_tostring(NL,2));
	m=map_mapname2mapid(map);
	x=(short)lua_tonumber(NL,3);
	y=(short)lua_tonumber(NL,4);
	xs=(short)lua_tonumber(NL,5);
	ys=(short)lua_tonumber(NL,6);
	class_=(short)lua_tonumber(NL,7);
	num=(short)lua_tonumber(NL,8);
	d1=(short)lua_tonumber(NL,9);
	d2=(short)lua_tonumber(NL,10);
	sprintf(function,"%s",lua_tostring(NL,11));

	npc_add_mob_lua(name,m,x,y,xs,ys,class_,num,d1,d2,function);

	return 0;
}

//Display a NPC input window asking the player for a value
//npcinput(type,[id])
LUA_FUNC(input)
{
	LUA_GET_TARGET(2);
	int type;

	type = (int)lua_tonumber(NL, 1);

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

//Display a NPC menu window asking the player for a value
//npcmenu("menu_name1",return_value1,...)
LUA_FUNC(menu)
{
	struct StringBuf buf;
	char *menu;
	int len=0, n, i;

	LUA_GET_TARGET(2);
	
	if ( sd->lua_script_state == INPUT || sd->lua_script_state == MENU ) {
		//You shouldn't get another input/menu while already in an input/menu
		lua_pushstring(NL, "Player received npc input request while already in one in function 'npcmenu'\n");
		lua_error(NL);
		return -1;
	}

	lua_pushliteral(NL, "n");
	lua_rawget(NL, 1);
	n = (int)lua_tonumber(NL, -1);
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
		lua_pushnumber(NL, i+1);
		lua_rawget(NL, 1);
		menu = (char *)lua_tostring(NL, -1);
		lua_pop(NL, 1);
		len += strlen(menu);
	}

	StringBuf_Init(&buf);

	for(i=0; i<n; i+=2) {
		lua_pushnumber(NL, i+1);
		lua_rawget(NL, 1);
		menu = (char *)lua_tostring(NL, -1);
		lua_pop(NL, 1);

		lua_pushnumber(NL, i+2);
		lua_rawget(NL, 1);
		sd->npc_menu_data.value[sd->npc_menu_data.current] = (int)lua_tonumber(NL, -1);
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

//Start a npc shop
//npcshop(item_id1,item_price1,...)
LUA_FUNC(shop)
{
	int n, i, j;

	LUA_GET_TARGET(2);

	lua_pushliteral(NL, "n");
	lua_rawget(NL, 1);
	n = (int)lua_tonumber(NL, -1);
	lua_pop(NL, 1);

	if(n%2 == 1) {
		lua_pushstring(NL, "Incorrect number of arguments for function 'npcmenu'\n");
		lua_error(NL);
		return -1;
	}

	sd->shop_data.n = n/2;

	for(i=1, j=0; i<=n; i+=2, j++) {
		lua_pushnumber(NL, i);
		lua_rawget(NL, 1);
		sd->shop_data.nameid[j] = (int)lua_tonumber(NL, -1);
		lua_pop(NL, 1);

		lua_pushnumber(NL, i+1);
		lua_rawget(NL, 1);
		sd->shop_data.value[j] = (int)lua_tonumber(NL, -1);
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
	LUA_GET_TARGET(3);
	char name[50];
	int type;

	sprintf(name, "%s", lua_tostring(NL,1));
	type = (int)lua_tonumber(NL, 2);

	clif_cutin(sd,name,type);

	return 0;
}

//Heals the character by a set amount of HP and SP
//npcheal(hp,sp,[id])
LUA_FUNC(heal)
{
	LUA_GET_TARGET(3);
	int hp, sp;

	hp = (int)lua_tonumber(NL, 1);
	sp = (int)lua_tonumber(NL, 2);

	pc_heal(sd, hp, sp, 0);

	return 0;
}

//Heals the character by a percentage of their Max HP / SP
//npcpercentheal(hp,sp,[id])
LUA_FUNC(percentheal)
{
	LUA_GET_TARGET(3);
	int hp, sp;

	hp = (int)lua_tonumber(NL, 1);
	sp = (int)lua_tonumber(NL, 2);

	pc_percentheal(sd, hp, sp);

	return 0;
}

//Heal the character by an amount that increases with VIT/INT, skills, etc
//npcitemheal(hp,sp,[id]);
LUA_FUNC(itemheal)
{
	LUA_GET_TARGET(3);
	int hp, sp;

	hp = (int)lua_tonumber(NL, 1);
	sp = (int)lua_tonumber(NL, 2);
	
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
	LUA_GET_TARGET(2);
	int job;

	job = (int)lua_tonumber(NL, 1);

	pc_jobchange(sd,job,0);

	return 0;
}

//Change the look of the character
//npcsetlook(type,val,[id])
LUA_FUNC(setlook)
{
	LUA_GET_TARGET(3);
	int type,val;

	type = (int)lua_tonumber(NL, 1);
	val = (int)lua_tonumber(NL, 2);

	pc_changelook(sd,type,val);

	return 0;
}

//Warp a player to a set location
//warp("map",x,y,[id])
LUA_FUNC(warp)
{
	LUA_GET_TARGET(4);
	char str[16];
	int x, y;

	sprintf(str, "%s", lua_tostring(NL,1));
	x = (int)lua_tonumber(NL, 2);
	y = (int)lua_tonumber(NL, 3);
	
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
	int class_ = (short)lua_tonumber(NL,1); // receive job number from script
	lua_pushstring(NL,job_name(class_)); // push job name to the stack
	return 1;
}

//same as setlook but only client side
//npcchangelook(type,val,[id])
LUA_FUNC(changelook)
{
	LUA_GET_TARGET(3);
	int type,val;

	type = (int)lua_tonumber(NL, 1);
	val = (int)lua_tonumber(NL, 2);

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
	LUA_GET_TARGET(2);
	
	if (!sd) //Return nil if no player exists
		return 0;
	
	nameid = (int)lua_tonumber(NL,1); //attempt to get the item by id
	if ( !nameid ) //if it's null, it's not an integer. it's treated as a string now while the item is searched
	{
		const char* name = (const char*)lua_tostring(NL,1);
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
				
	lua_pushnumber(NL,count);
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
	const char *name;
	LUA_GET_TARGET(3);
	
	if ( sd == NULL )
		return 0;
	
	name = (const char*)lua_tostring(NL,1);
	
	if ( atoi(name) != 0 )
		nameid=(int)lua_tonumber(NL,1);
	else
	{
		struct item_data *item_data = itemdb_searchname(name);
		if ( item_data )
			nameid=item_data->nameid;
	}
		
	amount = (int)lua_tonumber(NL,2);
	if ( amount <= 0 || nameid < 500 )
	{
		ShowError("buildin_getweight: Wrong item ID or amount %d %d %s.\n", amount, nameid, name);
		return 0;
	}
	
	weight = itemdb_weight(nameid)*amount;
	if ( amount > MAX_AMOUNT || weight + sd->weight > sd->max_weight )
		lua_pushnumber(NL,0);
		
	else if ( itemdb_isstackable(nameid) )
	{
		if ( ( i = pc_search_inventory(sd,nameid)) >= 0 )
			lua_pushnumber(NL,amount + sd->status.inventory[i].amount > MAX_AMOUNT ? 0 : 1);
		else
			lua_pushnumber(NL,pc_search_inventory(sd,0) >= 0 ? 1 : 0);
	}
	else
	{
		for( i = 0; i < MAX_INVENTORY && amount; ++i )
			if ( sd->status.inventory[i].nameid == 0 )
				amount--;
		lua_pushnumber(NL,amount ? 0 : 1);
	}
	
	return 1;
}

//giveitem(item id, amount[,character id];
LUA_FUNC(giveitem)
{
	int nameid,amount,get_count,i,flag = 0;
	const char *name;
	struct item it;
	LUA_GET_TARGET(3);

	name = lua_tostring(NL,1);
	if( atoi(name) != 0 )
	{// <item id>
		nameid=(int)lua_tonumber(NL,1);
		//Violet Box, Blue Box, etc - random item pick
		if( nameid < 0 ) {
			nameid=itemdb_searchrandomid(-nameid);
			flag = 1;
		}
		if( nameid <= 0 || !itemdb_exists(nameid) ){
			ShowError("buildin_giveitem: Nonexistant item %d requested.\n", nameid);
			return 0; //No item created.
		}
	} else {// "<item name>"
		struct item_data *item_data = itemdb_searchname(name);
		if( item_data == NULL ){
			ShowError("buildin_giveitem: Nonexistant item %s requested.\n", name);
			return 0; //No item created.
		}
		nameid=item_data->nameid;
	}

	// <amount>
	if( (amount=(int)lua_tonumber(NL,2)) <= 0)
		return 0; //return if amount <=0, skip the useles iteration

	memset(&it,0,sizeof(it));
	it.nameid=nameid;
	if(!flag)
		it.identify=1;
	else
		it.identify=itemdb_isidentified(nameid);

	//Check if it's stackable.
	if (!itemdb_isstackable(nameid))
		get_count = 1;
	else
		get_count = amount;

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
	if(log_config.enable_logs&LOG_SCRIPT_TRANSACTIONS)
		log_pick_pc(sd, "N", nameid, amount, NULL);

	lua_pushnumber(NL,1);
	return 1;
}

LUA_FUNC(giveitem2)
{
	return 0;
}

LUA_FUNC(getparam)
{
	int type;
	LUA_GET_TARGET(2);
	
	type = (int)lua_tonumber(NL,1);
	
	if ( sd == NULL )
		return 0;
		
	lua_pushnumber(NL,pc_readparam(sd,type));
	return 1;
}

LUA_FUNC(getrentalitem)
{
	LUA_GET_TARGET(3);
	struct item it;
	int seconds;
	int nameid = 0, flag;

	if( sd == NULL )
		return 0;

	if( script_getstring((const char*)lua_tostring(NL,1)) )
	{
		const char *name = lua_tostring(NL,1);
		struct item_data *itd = itemdb_searchname(name);
		if( itd == NULL )
		{
			ShowError("buildin_getrentalitem: Nonexistant item %s requested.\n", name);
			return 0;
		}
		nameid = itd->nameid;

	}
	else {
		nameid = (int)lua_tonumber(NL,1);
		if( nameid <= 0 || !itemdb_exists(nameid) )
		{
			ShowError("buildin_getrentalitem: Nonexistant item %d rquested.\n", nameid);
			return 0;
		}
	}

	seconds = (int)lua_tonumber(NL,2);
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

	lua_pushnumber(NL,1);
	return 1;
}

LUA_FUNC(getcharid)
{
	int num;
	struct map_session_data *sd;
	
	num = (int)lua_tonumber(NL,1);
	
	sd = map_nick2sd((const char*)lua_tostring(NL,2));
	
	if ( sd == NULL )
	{
		sd = script_get_target(NL,2);
	}

	switch( num )
	{
		case 0:	
			lua_pushnumber(NL, sd->status.char_id);
			return 1;
		case 1: 
			lua_pushnumber(NL, sd->status.party_id);
			return 1;
		case 2: 
			lua_pushnumber(NL, sd->status.guild_id);
			return 1;
		case 3: 
			lua_pushnumber(NL, sd->status.account_id);
			return 1;
		case 4: 
			lua_pushnumber(NL, sd->state.bg_id); 
			return 1;
		default:
			//errmsg
			return 0;
	}
	return 1;
}

char *buildin_getpartyname_sub(int party_id)
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
	LUA_GET_TARGET(2);
	
	party_id = (int)lua_tonumber(NL,1);
	
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

	p=party_search((int)lua_tonumber(NL,1));
	
	if(p!=NULL){
		for(i=0;i<MAX_PARTY;i++){
			if(p->party.member[i].account_id){
				switch ((int)lua_tonumber(NL,2)) {
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
	LUA_GET_TARGET(1);
	
	if ( sd ) {
		sd->npc_item_flag = sd->npc_id;
		lua_pushnumber(NL,1);
		return 1;
	}
	
	return 0;
}

LUA_FUNC(itemusedisable)
{
	LUA_GET_TARGET(1);
	
	if ( sd )
	{
		sd->npc_item_flag = 0;
		lua_pushnumber(NL,1);
		return 1;
	}
	
	return 0;
}

LUA_FUNC(givenameditem)
{
	int nameid;
	struct item item_tmp;
	LUA_GET_TARGET(3);
	struct map_session_data *tsd;
	
	if( script_getstring((const char*)lua_tostring(NL,1)) )
	{
		const char *name = (const char*)lua_tostring(NL,1);
		struct item_data *item_data = itemdb_searchname(name);
		if( item_data == NULL)
		{	//Failed
			return 0;
		}
		nameid = item_data->nameid;
	}else
		nameid = (int)lua_tonumber(NL,1);

	if(!itemdb_exists(nameid))
	{	//Even though named stackable items "could" be risky, they are required for certain quests.
		return 0;
	}
		
	if ( script_getstring((const char*)lua_tostring(NL,2)) )
	{
		tsd = map_nick2sd((const char*)lua_tostring(NL,2));
	}
	else
		tsd = map_charid2sd((int)lua_tonumber(NL,2));
	
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

	lua_pushnumber(NL,1);
	return 1;
}

LUA_FUNC(getrandgroupitem)
{
	lua_pushnumber(NL,itemdb_searchrandomid((int)lua_tonumber(NL,1)));
	return 1;
}

LUA_FUNC(additem)
{
	int nameid,amount,flag = 0;
	int x,y,m;
	const char *mapname;
	struct item item_tmp;

	if( script_getstring((const char*)lua_tostring(NL,1)) )
	{
		const char *name = (const char*)lua_tostring(NL,1);
		struct item_data *item_data = itemdb_searchname(name);
		if( item_data )
			nameid = item_data->nameid;
		else
			nameid = UNKNOWN_ITEM_ID;
	}else
		nameid = (int)lua_tonumber(NL,1);

	amount = (int)lua_tonumber(NL,2);
	mapname = (const char*)lua_tostring(NL,3);
	x = (int)lua_tonumber(NL,4);
	y = (int)lua_tonumber(NL,5);

	if(strcmp(mapname,"this")==0)
	{
		LUA_GET_TARGET(6);
		if (!sd) return 0; //Failed...
		m=sd->bl.m;
	} else
		m=map_mapname2mapid(mapname);

	if(nameid<0) { // ƒ‰ƒ“ƒ_ƒ€
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

	lua_pushnumber(NL,1);
	return 1;
}

LUA_FUNC(deleteitem)
{
	int nameid=0,amount,i,important_item=0;
	LUA_GET_TARGET(3);

	if( sd == NULL )
	{
		return 0;
	}
	
	if( script_getstring((const char*)lua_tostring(NL,1)) )
	{
		const char* item_name = (const char*)lua_tostring(NL,1);
		struct item_data* id = itemdb_searchname(item_name);
		if( id == NULL )
		{
			//err msg.
			sd->lua_script_state = L_CLOSE;
			return 0;
		}
		nameid = id->nameid;
	}
	else
		nameid = (int)lua_tonumber(NL,1);

	amount = (int)lua_tonumber(NL,2);

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
			lua_pushnumber(NL,1);
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
				lua_pushnumber(NL,1);
				return 1; //we deleted exact amount of items. now exit
			} else {
				amount-=sd->status.inventory[i].amount;

				//Logs items, got from (N)PC scripts
				if(log_config.enable_logs&0x40)
					log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -sd->status.inventory[i].amount, &sd->status.inventory[i]);

				pc_delitem(sd,i,sd->status.inventory[i].amount,0);
			}
		}

	//err msg
	sd->lua_script_state = L_CLOSE;
	return 1;
}

LUA_FUNC(deleteitem2)
{
	int nameid=0,amount,i=0;
	int iden,ref,attr,c1,c2,c3,c4;
	LUA_GET_TARGET(10);

	if ( sd == NULL )
		return 0;

	if( script_getstring((const char*)lua_tostring(NL,1)) )
	{
		const char* item_name = (const char*)lua_tostring(NL,1);
		struct item_data* id = itemdb_searchname(item_name);
		if( id == NULL )
		{
			//errmsg.
			sd->lua_script_state = L_CLOSE;
			return 1;
		}
		nameid = id->nameid;// "<item name>"
	}
	else
		nameid = (int)lua_tonumber(NL,1);// <item id>

	amount = (int)lua_tonumber(NL,2);
	iden = (int)lua_tonumber(NL,3);
	ref = (int)lua_tonumber(NL,4);
	attr = (int)lua_tonumber(NL,5);
	c1 = (short)lua_tonumber(NL,6);
	c2 = (short)lua_tonumber(NL,7);
	c3 = (short)lua_tonumber(NL,8);
	c4 = (short)lua_tonumber(NL,9);
	
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
			lua_pushnumber(NL,1);
			return 1; //we deleted exact amount of items. now exit
		} else {
			amount-=sd->status.inventory[i].amount;

			//Logs items, got from (N)PC scripts [Lupus]
			if(log_config.enable_logs&0x40)
				log_pick_pc(sd, "N", sd->status.inventory[i].nameid, -sd->status.inventory[i].amount, &sd->status.inventory[i]);

			pc_delitem(sd,i,sd->status.inventory[i].amount,0);
		}
	}

	//errmsg.
	sd->lua_script_state = L_CLOSE;
	return 1;
}

LUA_FUNC(getpartyleader)
{
	int party_id, type = 0, i=0;
	struct party_data *p;

	party_id = (int)lua_tonumber(NL,1);
	type = (int)lua_tonumber(NL,2);

	p=party_search(party_id);

	if (p) //Search leader
	for(i = 0; i < MAX_PARTY && !p->party.member[i].leader; i++);

	if (!p || i == MAX_PARTY) { //leader not found
			return 0;
	}

	switch (type) {
		case 1: lua_pushnumber(NL,p->party.member[i].account_id); break;
		case 2: lua_pushnumber(NL,p->party.member[i].char_id); break;
		case 3: lua_pushnumber(NL,p->party.member[i].class_); break;
		case 4: lua_pushstring(NL,mapindex_id2name(p->party.member[i].map)); break;
		case 5: lua_pushnumber(NL,p->party.member[i].lv); break;
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
	int guild_id=(int)lua_tonumber(NL,1);
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
	int guild_id=(int)lua_tonumber(NL,1);
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
	int guild_id=(int)lua_tonumber(NL,1);
	master=buildin_getguildmaster_sub(guild_id);
	if(master!=0){
		if((sd=map_nick2sd(master)) == NULL){
			return 0;
		}
		lua_pushnumber(NL,sd->status.char_id);
		return 1;
	}
	return 0;
}

LUA_FUNC(strcharinfo)
{
	LUA_GET_TARGET(2);
	int num;
	char *buf;

	if ( sd == NULL )
		return 0;
		
	num = (int)lua_tonumber(NL,1);
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
	LUA_GET_TARGET(2);
	TBL_NPC* nd;
	int num;
	char *name=NULL;

	nd = map_id2nd(sd->npc_id);
	
	if (!nd)
		return 0;

	num = (int)lua_tonumber(NL,1);
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

// List of commands to build into Lua, format : {"function_name_in_lua", C_function_name}
#define LUA_DEF(x) {#x, buildin_ ##x}
static struct LuaCommandInfo commands[] = {
	LUA_DEF(scriptend),
	LUA_DEF(addnpc),
	LUA_DEF(addareascript),
	LUA_DEF(addwarp),
	LUA_DEF(addspawn),
	//LUA_DEF(addgmcommand),
	//LUA_DEF(addtimer),
	//LUA_DEF(addclock),
	//LUA_DEF(addevent),
	LUA_DEF(mes),
	LUA_DEF(close),
	LUA_DEF(next),
	LUA_DEF(input),
	LUA_DEF(menu),
	LUA_DEF(shop),
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
	// End of build-in functions list
	{"-End of list-", NULL},
};

//-----------------------------------------------------------------------------
// LUA SCRIPT FUNCTIONS
//

int do_init_script_lua()
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

	if((char_id=(int)lua_tonumber(NL, idx))==0) { // If 0 or nothing was passed as argument
		lua_pushliteral(NL, "char_id");
		lua_rawget(NL, LUA_GLOBALSINDEX);
		char_id=(int)lua_tonumber(NL, -1); // Get the thread's char ID if it's a personal one
		lua_pop(NL, 1);
	}

	if(char_id==0 || (sd=map_charid2sd(char_id))==NULL) { // If we still dont have a valid char ID here, there's a problem
		ShowError("Target character not found for script command\n");
		return NULL;
	}
	
	return sd;
}

// Run a Lua function that was previously loaded, specifying the type of arguments with a "format" string
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

/*
	Used to determine whether a lua argument passed
	is a string.
*/
int script_getstring(const char *name)
{
	while(*name != '\0')
	{
		if(isalpha(*name))
			return 1;
		else
			*name++;
	}

	return 0;
}