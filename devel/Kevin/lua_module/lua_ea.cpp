/* LUA_EA module ***Kevin*** lua_ea.cpp v1.0 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/malloc.h"
#include "../common/lock.h"
#include "../common/db.h"
#include "../common/nullpo.h"

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
#include "log.h"
#include "showmsg.h"

#include "lua_ea.h"
#include "lua_ea_buildins.cpp"

// Register build-in commands specified above
void lua_ea_buildin_commands()
{
	int i=0;

	while(commands[i].f) {
		lua_pushstring(L, commands[i].command);
        lua_pushcfunction(L, commands[i].f);
        lua_settable(L, LUA_GLOBALSINDEX);
        i++;
    }
	ShowStatus("Done registering '"CL_BLUE"%d"CL_RESET"' script build-in commands.\n",i);
}

// Check whether a char ID was passed as argument, else check if there's a global one
struct map_session_data* lua_ea_get_target(lua_State *NL,int idx)
{
	int char_id;
	struct map_session_data* sd;

	if((char_id=lua_tonumber(NL, idx))==0) { // If 0 or nothing was passed as argument
		lua_pushliteral(NL, "char_id");
		lua_rawget(NL, LUA_GLOBALSINDEX);
		char_id=lua_tonumber(NL, -1); // Get the thread's char ID if it's a personal one
		lua_pop(NL, 1);
	}

	if(char_id==0 || (sd=map_charid2sd(char_id))==NULL) { // If we still dont have a valid char ID here, there's a problem
		lua_ea_error("Character not found!", 1);
		return NULL;
	}
	
	return sd;
}

// Run a Lua function that was previously loaded, specifying the type of arguments with a "format" string
void lua_ea_run_function(const char *name,int char_id,const char *format,...)
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
		if(sd->lua_ea_player_data.lua_ea_state!=NRUN) { // Check that the player is not currently running a script
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

	if (lua_resume(NL,n)!=0) { // Tell Lua to run the function
		ShowError("Cannot run function %s : %s\n",name,lua_tostring(NL,-1));
		return;
	}

	if(sd && sd->lua_ea_player_data.lua_ea_state==NRUN) { // If the script has finished (not waiting answer from client)
	    sd->lua_ea_player_data.NL=NULL; // Close the player's personal thread
		sd->lua_ea_player_data.script_id=0; // Set the player's current NPC to 'none'
	}
}

// Run a Lua chunk
void lua_ea_run_chunk(const char *chunk,int char_id)
{
	lua_State *NL;
    struct map_session_data *sd=NULL;

	if (char_id == 0) { // If char_id points to no player
		NL = L; // Use the global thread
	} else { // Else we want to run the chunk for a specific player
		sd = map_charid2sd(char_id);
		nullpo_retv(sd);
		if(sd->lua_ea_state!=NRUN) { // Check that the player is not currently running a script
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

	if(sd && sd->lua_ea_player_data.lua_ea_state==NRUN) { // If the script has finished (not waiting answer from client)
	    sd->lua_ea_player_data.NL=NULL; // Close the player's personal thread
		sd->lua_ea_player_data.script_id=0; // Set the player's current NPC to 'none'
	}
}

// Resume an already paused script
void lua_ea_resume(struct map_session_data *sd,const char *format,...) {
	va_list arg;
	int n=0;

	nullpo_retv(sd);

	if(sd->lua_ea_player_data.lua_ea_state==NRUN) { // Check that the player is currently running a script
		ShowError("Cannot resume script for player %d : player is not running a script\n",sd->char_id);
		return;
	}
	sd->lua_ea_player_data.lua_ea_state=NRUN; // Set the script flag as 'not waiting for anything'

	va_start(arg,format); // Initialize the argument list
	while (*format) { // Pass arguments to Lua, according to the types defined by "format"
        switch (*format++) {
          case 'd': // d = Double
            lua_pushnumber(sd->lua_ea_player_data.NL,va_arg(arg,double));
            break;
          case 'i': // i = Integer
            lua_pushnumber(sd->lua_ea_player_data.NL,va_arg(arg,int));
            break;
          case 's': // s = String
            lua_pushstring(sd->lua_ea_player_data.NL,va_arg(arg,char*));
            break;
          default: // Unknown code
            ShowError("%c : Invalid argument type code, allowed codes are 'd'/'i'/'s'\n",*(format-1));
        }
        n++;
        luaL_checkstack(sd->lua_ea_player_data.NL,1,"Too many arguments");
    }

    va_end(arg);

	if (lua_resume(sd->lua_ea_player_data.NL,n)!=0) { // Tell Lua to run the function
		ShowError("Cannot resume script for player %d : %s\n",sd->char_id,lua_tostring(sd->NL,-1));
		return;
	}

	if(sd->lua_ea_player_data.lua_ea_state==NRUN) { // If the script has finished (not waiting answer from client)
	    sd->lua_ea_player_data.NL=NULL; // Close the player's personal thread
		sd->lua_ea_player_data.script_id=0; // Set the player's current NPC to 'none'
	}
}

int lua_ea_npc_add(char *name,char *exname,char *m,int x,int y,int dir,int class_,char *function)
{
	struct npc_data *nd=(struct npc_data *)aCalloc(1, sizeof(struct npc_data));

	nd->bl.prev=nd->bl.next=NULL;
	nd->bl.m=m;
	nd->bl.x=x;
	nd->bl.y=y;
	nd->bl.id=npc_get_new_npc_id();
	nd->bl.type=BL_NPC;
	strcpy(nd->name,name);
	strcpy(nd->exname,exname);
	strcpy(nd->lua_npc_data.function,function);
	nd->dir=dir;
	nd->class_=class_;
	nd->flag=0;
	nd->option=0;
	nd->opt1=0;
	nd->opt2=0;
	nd->opt3=0;
	nd->guild_id=0;
	nd->chat_id=0;
	
	nd->lua_npc_data.type=NPC;

	nd->n=map_addnpc(m,nd);
	map_addblock(&nd->bl);
	clif_spawnnpc(nd);
	strdb_insert(npcname_db,nd->exname,nd);
	npc_num++;

	return 0;
}

int lua_ea_areascript_add(char *name,char *m,int x1,int y1,int x2,int y2,int dir,int class_,char *function)
{
	struct npc_data *nd=(struct npc_data *)aCalloc(1, sizeof(struct npc_data));

	nd->bl.prev=nd->bl.next=NULL;
	nd->bl.m=m;
	nd->bl.x=x1;
	nd->bl.y=y1;
	nd->bl.id=npc_get_new_npc_id();
	nd->bl.type=BL_NPC;
	strcpy(nd->name,name);
	strcpy(nd->exname,name);
	nd->dir=dir;
	nd->class_=class_;
	nd->flag=0;
	nd->option=0;
	nd->opt1=0;
	nd->opt2=0;
	nd->opt3=0;
	nd->guild_id=0;
	nd->chat_id=0;
	
	//Specific lua data
	nd->lua_npc_data.type=AREASCRIPT;
	strcpy(nd->lua_npc_data.function,function);
	nd->lua_npc_data.ad.x1=x1;
	nd->lua_npc_data.ad.y1=y1;
	nd->lua_npc_data.ad.x2=x2;
	nd->lua_npc_data.ad.y2=y2;

	nd->n=map_addnpc(m,nd);
	map_addblock(&nd->bl);
	clif_spawnnpc(nd);
	strdb_insert(npcname_db,nd->exname,nd);
	npc_num++;

	return 0;
}

int lua_ea_run(struct map_session_data *sd, struct npc_data *nd)
{
	
	sd->lua_ea_player_data.script_id = nd->bl.id;
	lua_ea_run_function(nd->lua_npc_data.function, sd->char_id, "");
	
}

int lua_ea_continue(struct map_session_data *sd, struct npc_data *nd)
{
	
	sd->