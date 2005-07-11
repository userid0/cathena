/* LUA_EA module ***Kevin*** lua_ea.h v1.0 */

#ifndef _LUA_EA_H_
#define _LUA_EA_H_

extern struct lua_ea_Config {
	unsigned event_lua_ea_type : 1;
	char* die_event_name;
	char* kill_event_name;
	char* login_event_name;
	char* logout_event_name;
	char* mapload_event_name;
	unsigned event_requires_trigger : 1;
} lua_ea_config;

typedef enum {
	NRUN, // Script is ready
	NEXT, // Waiting for the player to click [Next]
	CLOSE, // Waiting for the player to click [Close]
	MENU, // Waiting for the player to choose a menu option
	INPUT, // Waiting for the player to input a value
	SHOP, // Waiting for the player to choose [Buy] or [Sell]
	BUY, // Waiting for the player to choose items to buy
	SELL // Waiting for the player to choose items to sell
} lua_script_state;

typedef enum {
	NPC,
	AREASCRIPT
} lua_npc_type;

struct lua_ea_player_data {
	int script_id;
	lua_State *NL;
	lua_script_state state;
};

struct lua_ea_npc_data {
	lua_npc_type type;
	char function[50];
	
	//areascript data
	struct ad {
		int x1;
		int y1;
		int x2;
		int y2;
	};
};

struct LuaCommandInfo {
	const char *command;
	lua_CFunction f;
};

lua_State *L;

void lua_ea_buildin_commands();
void lua_ea_run_function(const char *name,int char_id,const char *format,...);
void lua_ea_run_chunk(const char *chunk,int char_id);

/*script related functions*/
struct map_session_data* lua_ea_get_target(lua_State *NL,int idx);
void lua_ea_resume(struct map_session_data *sd,const char *format,...);

#endif