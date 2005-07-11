/* LUA_EA module ***Kevin*** lua_ea.h v1.0 */

#ifndef _LUA_EA_H_
#define _LUA_EA_H_

class lua_ea
{
	private:
		lua_State *L; /*Lua global thread*/
	public:
		lua_ea(); /*constructor*/
		~lua_ea(); /*destructor*/
		
		void init(void); /*Initializes the module*/
		void init(lua_ea_commands *); /*Initializes the module with user defined default buildin functions*/
		void buildin_funcs(lua_ea_commands *); /*Registers buildin functions*/
		
		/*Thread functions*/
		void new_thread(lua_State *L, lua_State *NL);
		void resume(struct map_session_data *, const char *, ...);
};

class lua_ea_commands
{
	private:
		lua_CFunction f;
		const char *command;
	public:
		lua_ea_commands(const char *str_cmd, lua_CFunction func): /*sets default values for f and *command*/
			command(str_cmd), f(func)
			{ };
};
		

#endif
