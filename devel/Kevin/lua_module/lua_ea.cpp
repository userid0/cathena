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

/*Open lua and lua libraries*/
void lua_ea::init(void)
{
	L = lua_open(); /* Open Lua*/
	luaopen_base(L); /* Open the basic library*/
	luaopen_table(L); /* Open the table library*/
	luaopen_io(L); /* Open the I/O library*/
	luaopen_string(L); /* Open the string library*/
	luaopen_math(L); /* Open the math */	
	
	lua_ea.building_funcs(default_buildins); /*Default buildin functions*/
}

/*Open lua and lua libraries and register default commands*/
void lua_ea::init(lua_ea_commands *commands)
{
	L = lua_open(); /* Open Lua*/
	luaopen_base(L); /* Open the basic library*/
	luaopen_table(L); /* Open the table library*/
	luaopen_io(L); /* Open the I/O library*/
	luaopen_string(L); /* Open the string library*/
	luaopen_math(L); /* Open the math */	
	
	lua_ea.building_funcs(default_buildins); /*Default buildin functions*/
	lua_ea.buildin_funcs(commands); /*user define defaults*/
}

/*Building functions to lua*/
void lua_ea::buildin_funcs(lua_ea_commands *commands)
{
	int i=0;

	while(commands[i].f) {
		lua_pushstring(L, commands[i].command);
        lua_pushcfunction(L, commands[i].f);
        lua_settable(L, LUA_GLOBALSINDEX);
        i++;
    }
	ShowStatus("Done registering '"CL_WHITE"%d"CL_RESET"' script build-in commands.\n",i);
}
