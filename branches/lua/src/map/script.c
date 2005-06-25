// $Id: script.c 148 2004-09-30 14:05:37Z MouseJstr $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifndef _WIN32
#include <sys/time.h>
#endif

#include <time.h>

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

//static struct dbt *mapreg_db=NULL;
//static struct dbt *mapregstr_db=NULL;
//static int mapreg_dirty=-1;
char mapreg_txt[256]="save/mapreg.txt";
//#define MAPREG_AUTOSAVE_INTERVAL	(10*1000)

struct Script_Config script_config;
static char pos[11][100] = {"Head","Body","Left hand","Right hand","Robe","Shoes","Accessory 1","Accessory 2","Head 2","Head 3","Not Equipped"};

lua_State *L; // [DracoRPG]


/*===================================================================
 *
 *                  LUA BUILDIN COMMANDS GO HERE [Kevin]
 *
 *===================================================================
 */

// end()
// Exits definitively the script
static int buildin_end(lua_State *NL)
{
	struct map_session_data *sd = NULL;
	int charid;

	lua_pushliteral(NL, "char_id");
	lua_rawget(NL, LUA_GLOBALSINDEX);
	charid=lua_tonumber(NL, -1);
	lua_pop(NL, 1);
	if((sd = map_charid2sd(charid))==NULL) {
		ShowError("Character not found in script\n");
		return -1;
	}

	return lua_yield(NL, 0);
}

// addnpc("NPC name","name postfix","map.gat",x,y,dir,sprite,"function")
// Add an standard NPC that triggers a function when clicked
static int buildin_addnpc(lua_State *NL)
{
	char name[24], exname[24], map[16], function[512];
	short m,x,y,dir,class_;
	
	sprintf(name, "%s", lua_tostring(NL,1));
	sprintf(exname, "%s", lua_tostring(NL,2));
	sprintf(map, "%s", lua_tostring(NL,3));
	m=map_mapname2mapid(map);
	x=lua_tonumber(NL,4);
	y=lua_tonumber(NL,5);
	dir=lua_tonumber(NL,6);
	class_=lua_tonumber(NL,7);
	sprintf(function, "%s", lua_tostring(NL,8));
	
	npc_add(name,exname,m,x,y,dir,class_,function);

	return 0;
}

// addareascript("Area script name","map.gat",x1,y1,x2,y2,"function")
// Add an invisible area that triggers a function when entered
static int buildin_addareascript(lua_State *NL)
{
	char name[24], map[16], function[512];
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


	return 0;
}

// addwarp("Warp name","map.gat",x,y,"destmap.gat",destx,desty,xradius,yradius)
// Add a warp that moves players to somewhere else when entered
static int buildin_addwarp(lua_State *NL)
{
	char name[24], map[16], destmap[100];
	short m,x,y;
	short destx,desty,xs,ys;
	
	sprintf(name,"%s",lua_tostring(NL,1));
	sprintf(map,"%s",lua_tostring(NL,2));
	m=map_mapname2mapid(map);
	x=lua_tonumber(NL,3);
	y=lua_tonumber(NL,4);
	sprintf(destmap,"%s",lua_tostring(NL,5));
	destx=lua_tonumber(NL,6);
	desty=lua_tonumber(NL,7);
	xs=lua_tonumber(NL,8);
	ys=lua_tonumber(NL,9);

	warp_add(name,m,x,y,destmap,destx,desty,xs,ys);

	return 0;
}

// addmapflag("Type","map.gat")
// Sets a mapflag for map
static int buildin_addmapflag(lua_State *NL)
{
	char name[24], map[16];
	short m,x,y;
	short destx,desty,xs,ys;

	sprintf(name,"%s",lua_tostring(NL,1));
	sprintf(map,"%s",lua_tostring(NL,2));
	m=map_mapname2mapid(map);



	return 0;
}

// npcmes("A bunch of silly text")
// Print the silly text to the NPC dialog window of the player
static int buildin_npcmes(lua_State *NL)
{
	struct map_session_data *sd = NULL;
	char mes[512];
	int charid, npcid;

	lua_pushliteral(NL, "char_id");
	lua_rawget(NL, LUA_GLOBALSINDEX);
	charid=lua_tonumber(NL, -1);
	lua_pop(NL, 1);
	if((sd = map_charid2sd(charid))==NULL) {
		ShowError("Character not found in script\n");
		return -1;
	}
	npcid = sd->npc_id;
	sprintf(mes,"%s",lua_tostring(NL, 1)); 

	clif_scriptmes(sd, npcid, mes);

	return 0;
}

// npcclose()
// Display a [Close] button in the NPC dialog window of the player and pause the script until the button is clicked
static int buildin_npcclose(lua_State *NL)
{
	struct map_session_data *sd = NULL;
	int charid;
	
	lua_pushliteral(NL, "char_id");
	lua_rawget(NL, LUA_GLOBALSINDEX);
	charid=lua_tonumber(NL, -1);
	lua_pop(NL, 1);
	if((sd = map_charid2sd(charid))==NULL) {
		ShowError("Character not found in script\n");
		return -1;
	}

	clif_scriptclose(sd,sd->npc_id);

	sd->script_state = CLOSE;
	return lua_yield(NL, 0);
}

// npcnext()
// Display a [Next] button in the NPC dialog window of the player and pause the script until the button is clicked
static int buildin_npcnext(lua_State *NL)
{
	struct map_session_data *sd = NULL;
	int charid;

	lua_pushliteral(NL, "char_id");
	lua_rawget(NL, LUA_GLOBALSINDEX);
	charid=lua_tonumber(NL, -1);
	lua_pop(NL, 1);
	if((sd = map_charid2sd(charid))==NULL) {
		ShowError("Character not found in script\n");
		return -1;
	}

	clif_scriptnext(sd,sd->npc_id);

	sd->script_state = NEXT;
	return lua_yield(NL, 0);
}

// npcinput(type)
// Display an NPC input window asking the player for a value
static int buildin_npcinput(lua_State *NL)
{
	struct map_session_data *sd = NULL;
	int charid;

	lua_pushliteral(NL, "char_id");
	lua_rawget(NL, LUA_GLOBALSINDEX);
	charid=lua_tonumber(NL, -1);
	lua_pop(NL, 1);
	if((sd = map_charid2sd(charid))==NULL) {
		ShowError("Character not found in script\n");
		return -1;
	}

	switch((int)lua_tonumber(NL, 1)){
		case 0:
			clif_scriptinput(sd,sd->npc_id);
			break;
		case 1:
			clif_scriptinputstr(sd,sd->npc_id);
			break;
	}
	
	sd->script_state = INPUT;
	return lua_yield(NL, 1);
}

// npcmenu("menu_name1", return_value1, ...)
// Display an NPC input window asking the player for a value
static int buildin_npcmenu(lua_State *NL)
{
	struct map_session_data *sd = NULL;
	char *buf;
	char *menu;
	int len=0, n, i;
	int charid;

	lua_pushliteral(NL, "char_id");
	lua_rawget(NL, LUA_GLOBALSINDEX);
	charid=lua_tonumber(NL, -1);
	lua_pop(NL, 1);
	if((sd = map_charid2sd(charid))==NULL) {
		ShowError("Character not found in script\n");
		return -1;
	}

	lua_pushliteral(NL, "n");
	lua_rawget(NL, 1);
	n = lua_tonumber(NL, -1);
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

	buf=(char *)aCallocA(len+1, sizeof(char));
	buf[0]=0;

	for(i=0; i<n; i+=2) {
		lua_pushnumber(NL, i+1);
		lua_rawget(NL, 1);
		menu = (char *)lua_tostring(NL, -1);
		lua_pop(NL, 1);

		lua_pushnumber(NL, i+2);
		lua_rawget(NL, 1);
		sd->npc_menu_data.value[sd->npc_menu_data.current] = lua_tonumber(NL, -1);
		lua_pop(NL, 1);

		sd->npc_menu_data.id[sd->npc_menu_data.current] = sd->npc_menu_data.current;
		sd->npc_menu_data.current+=1;
		
		strcat(buf,menu);
		strcat(buf,":");
	}

	clif_scriptmenu(sd,sd->npc_id,buf);
	aFree(buf);

	sd->script_state = MENU;
	return lua_yield(NL, 1);
}

// npcshop()
// Start a shop with buylist of item_id selling for item_price
static int buildin_npcshop(lua_State *NL)
{
	struct map_session_data *sd = NULL;
	int n, i, j;
	int charid;

	lua_pushliteral(NL, "char_id");
	lua_rawget(NL, LUA_GLOBALSINDEX);
	charid=lua_tonumber(NL, -1);
	lua_pop(NL, 1);
	if((sd = map_charid2sd(charid))==NULL) {
		ShowError("Character not found in script\n");
		return -1;
	}

	lua_pushliteral(NL, "n");
	lua_rawget(NL, 1);
	n = lua_tonumber(NL, -1);
	lua_pop(NL, 1);

	if(n%2 == 1) {
		lua_pushstring(NL, "Incorrect number of arguments for function 'npcshop_co'\n");
		lua_error(NL);
		return -1;
	}

	sd->shop_data.n = n/2;

	for(i=1, j=0; i<=n; i+=2, j++) {
		lua_pushnumber(NL, i);
		lua_rawget(NL, 1);
		sd->shop_data.nameid[j] = lua_tonumber(NL, -1);
		lua_pop(NL, 1);

		lua_pushnumber(NL, i+1);
		lua_rawget(NL, 1);
		sd->shop_data.value[j] = lua_tonumber(NL, -1);
		lua_pop(NL, 1);
	}

	clif_npcbuysell(sd, sd->npc_id);

	sd->script_state = SHOP;
	return lua_yield(NL, 1);
}

// heal(hp,sp)
// Heal the character by a set amount of HP and SP
static int buildin_heal(lua_State *NL)
{
	struct map_session_data *sd = NULL;
	int charid, hp, sp;

	lua_pushliteral(NL, "char_id");
	lua_rawget(NL, LUA_GLOBALSINDEX);
	charid=lua_tonumber(NL, -1);
	lua_pop(NL, 1);
	if((sd = map_charid2sd(charid))==NULL) {
		ShowError("Character not found in script\n");
		return -1;
	}
	hp = lua_tonumber(NL, 1);
	sp = lua_tonumber(NL, 2);
	lua_pop(NL, 2);

	pc_heal(sd, hp, sp);

	return 0;
}

// percentheal(hp,sp)
// Heal the character by a percentage of MaxHP and MaxSP
static int buildin_percentheal(lua_State *NL)
{
	struct map_session_data *sd = NULL;
	int charid, hp, sp;

	lua_pushliteral(NL, "char_id");
	lua_rawget(NL, LUA_GLOBALSINDEX);
	charid=lua_tonumber(NL, -1);
	lua_pop(NL, 1);
	if((sd = map_charid2sd(charid))==NULL) {
		ShowError("Character not found in script\n");
		return -1;
	}
	hp = lua_tonumber(NL, 1);
	sp = lua_tonumber(NL, 2);

	pc_percentheal(sd, hp, sp);

	return 0;
}

// List of commands to build into Lua
static struct LuaCommandInfo commands[] = {
	/* Basic functions */
	{"end", buildin_end},
	/* Object creation functions */
	{"addnpc", buildin_addnpc},
	{"addareascript", buildin_addareascript},
	{"addwarp", buildin_addwarp},
//  {"addspawn", buildin_addspawn},
//	{"addgmcommand", buildin_addgmcommand},
//	{"addtimer", buildin_addtimer},
//	{"addevent", buildin_addevent},
//	{"addmapflag", buildin_addmapflag},
	/* NPC dialog window functions */
	{"npcmes", buildin_npcmes},
	{"npcclose", buildin_npcclose},
	{"npcnext", buildin_npcnext},
	{"npcinput", buildin_npcinput},
	{"npcmenu_", buildin_npcmenu},
	{"npcshop_", buildin_npcshop},
	/* Player related functions */
	{"heal", buildin_heal},
	{"percentheal", buildin_percentheal},
	/* End of build-in functions list */
	{"-End of list-", NULL},
};

/*===================================================================
 *
 *                   LUA FUNCTIONS BEGIN HERE [DracoRPG]
 *
 *===================================================================
 */

// Register build-in commands specified above
void script_buildin_commands()
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
		if(sd->script_state!=NRUN) { // Check that the player is not currently running a script
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

	if(sd && sd->script_state==NRUN) { // If the script has finished (not waiting answer from client)
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
		if(sd->script_state!=NRUN) { // Check that the player is not currently running a script
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

	if(sd && sd->script_state==NRUN) { // If the script has finished (not waiting answer from client)
	    sd->NL=NULL; // Close the player's personal thread
		sd->npc_id=0; // Set the player's current NPC to 'none'
	}
}

// Resume an already paused script
void script_resume(struct map_session_data *sd,const char *format,...) {
	va_list arg;
	int n=0;

	nullpo_retv(sd);

	if(sd->script_state==NRUN) { // Check that the player is currently running a script
		ShowError("Cannot resume script for player %d : player is not running a script\n",sd->char_id);
		return;
	}
	sd->script_state=NRUN; // Set the script flag as 'not waiting for anything'

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

	if (lua_resume(sd->NL,n)!=0) { // Tell Lua to run the function
		ShowError("Cannot resume script for player %d : %s\n",sd->char_id,lua_tostring(sd->NL,-1));
		return;
	}

	if(sd->script_state==NRUN) { // If the script has finished (not waiting answer from client)
	    sd->NL=NULL; // Close the player's personal thread
		sd->npc_id=0; // Set the player's current NPC to 'none'
	}
}

/*===================================================================
 *
 *                   LUA FUNCTIONS END HERE [DracoRPG]
 *
 *===================================================================
 */


/*==========================================
 * マップ変数の変更
 *------------------------------------------
 */
/*int mapreg_setreg(int num,int val)
{
	if(val!=0)
		numdb_insert(mapreg_db,num,val);
	else
		numdb_erase(mapreg_db,num);

	mapreg_dirty=1;
	return 0;
}*/
/*==========================================
 * 文字列型マップ変数の変更
 *------------------------------------------
 */
/*int mapreg_setregstr(int num,const char *str)
{
	char *p;

	if( (p=(char *) numdb_search(mapregstr_db,num))!=NULL )
		aFree(p);

	if( str==NULL || *str==0 ){
		numdb_erase(mapregstr_db,num);
		mapreg_dirty=1;
		return 0;
	}
	p=(char *)aCallocA(strlen(str)+1, sizeof(char));
	strcpy(p,str);
	numdb_insert(mapregstr_db,num,p);
	mapreg_dirty=1;
	return 0;
}*/

/*==========================================
 * 永続的マップ変数の読み込み
 *------------------------------------------
 */
/*static int script_load_mapreg()
{
	FILE *fp;
	char line[1024];

	if( (fp=fopen(mapreg_txt,"rt"))==NULL )
		return -1;

	while(fgets(line,sizeof(line),fp)){
		char buf1[256],buf2[1024],*p;
		int n,v,s,i;
		if( sscanf(line,"%255[^,],%d\t%n",buf1,&i,&n)!=2 &&
			(i=0,sscanf(line,"%[^\t]\t%n",buf1,&n)!=1) )
			continue;
		if( buf1[strlen(buf1)-1]=='$' ){
			if( sscanf(line+n,"%[^\n\r]",buf2)!=1 ){
				printf("%s: %s broken data !\n",mapreg_txt,buf1);
				continue;
			}
			p=(char *)aCallocA(strlen(buf2) + 1,sizeof(char));
			strcpy(p,buf2);
			s= add_str((unsigned char *) buf1);
			numdb_insert(mapregstr_db,(i<<24)|s,p);
		}else{
			if( sscanf(line+n,"%d",&v)!=1 ){
				printf("%s: %s broken data !\n",mapreg_txt,buf1);
				continue;
			}
			s= add_str((unsigned char *) buf1);
			numdb_insert(mapreg_db,(i<<24)|s,v);
		}
	}
	fclose(fp);
	mapreg_dirty=0;
	return 0;
}*/
/*==========================================
 * 永続的マップ変数の書き込み
 *------------------------------------------
 */
/*static int script_save_mapreg_intsub(void *key,void *data,va_list ap)
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
static int script_save_mapreg_strsub(void *key,void *data,va_list ap)
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
static int script_save_mapreg()
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
static int script_autosave_mapreg(int tid,unsigned int tick,int id,int data)
{
	if(mapreg_dirty)
		script_save_mapreg();
	return 0;
}*/

/*==========================================
 *
 *------------------------------------------
 */

static int set_posword(char *p)
{
	char* np,* str[15];
	int i=0;
	for(i=0;i<11;i++) {
		if((np=strchr(p,','))!=NULL) {
			str[i]=p;
			*np=0;
			p=np+1;
		} else {
			str[i]=p;
			p+=strlen(p);
		}
		if(str[i])
			strcpy(pos[i],str[i]);
	}
	return 0;
}

int script_config_read(char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;

	script_config.verbose_mode = 0;
	script_config.warn_func_no_comma = 1;
	script_config.warn_cmd_no_comma = 1;
	script_config.warn_func_mismatch_paramnum = 1;
	script_config.warn_cmd_mismatch_paramnum = 1;
	script_config.check_cmdcount = 65535;
	script_config.check_gotocount = 2048;

	script_config.die_event_name = (char *) aCallocA (24, sizeof(char));
	script_config.kill_event_name = (char *) aCallocA (24, sizeof(char));
	script_config.login_event_name = (char *) aCallocA (24, sizeof(char));
	script_config.logout_event_name = (char *) aCallocA (24, sizeof(char));
	script_config.mapload_event_name = (char *) aCallocA (24, sizeof(char));

	fp = fopen(cfgName, "r");
	if (fp == NULL) {
		printf("file not found: %s\n", cfgName);
		return 1;
	}
	while (fgets(line, sizeof(line) - 1, fp)) {
		if (line[0] == '/' && line[1] == '/')
			continue;
		i = sscanf(line,"%[^:]: %[^\r\n]",w1,w2);
		if (i != 2)
			continue;
		if(strcmpi(w1,"refine_posword")==0) {
			set_posword(w2);
		}
		else if(strcmpi(w1,"verbose_mode")==0) {
			script_config.verbose_mode = battle_config_switch(w2);
		}
		else if(strcmpi(w1,"warn_func_no_comma")==0) {
			script_config.warn_func_no_comma = battle_config_switch(w2);
		}
		else if(strcmpi(w1,"warn_cmd_no_comma")==0) {
			script_config.warn_cmd_no_comma = battle_config_switch(w2);
		}
		else if(strcmpi(w1,"warn_func_mismatch_paramnum")==0) {
			script_config.warn_func_mismatch_paramnum = battle_config_switch(w2);
		}
		else if(strcmpi(w1,"warn_cmd_mismatch_paramnum")==0) {
			script_config.warn_cmd_mismatch_paramnum = battle_config_switch(w2);
		}
		else if(strcmpi(w1,"check_cmdcount")==0) {
			script_config.check_cmdcount = battle_config_switch(w2);
		}
		else if(strcmpi(w1,"check_gotocount")==0) {
			script_config.check_gotocount = battle_config_switch(w2);
		}
		else if(strcmpi(w1,"die_event_name")==0) {			
			strcpy(script_config.die_event_name, w2);			
		}
		else if(strcmpi(w1,"kill_event_name")==0) {
			strcpy(script_config.kill_event_name, w2);
		}
		else if(strcmpi(w1,"login_event_name")==0) {
			strcpy(script_config.login_event_name, w2);
		}
		else if(strcmpi(w1,"logout_event_name")==0) {
			strcpy(script_config.logout_event_name, w2);
		}
		else if(strcmpi(w1,"mapload_event_name")==0) {
			strcpy(script_config.mapload_event_name, w2);
		}
		else if(strcmpi(w1,"import")==0){
			script_config_read(w2);
		}
	}
	fclose(fp);

	return 0;
}

/*==========================================
 * 終了
 *------------------------------------------
 */
/*static int mapreg_db_final(void *key,void *data,va_list ap)
{
	return 0;
}
static int mapregstr_db_final(void *key,void *data,va_list ap)
{
	aFree(data);
	return 0;
}*/

int do_final_script()
{
/*	if(mapreg_dirty>=0)
		script_save_mapreg();

	if(mapreg_db)
		numdb_final(mapreg_db,mapreg_db_final);
	if(mapregstr_db)
		strdb_final(mapregstr_db,mapregstr_db_final);*/

	if (script_config.die_event_name)
		aFree(script_config.die_event_name);
	if (script_config.kill_event_name)
		aFree(script_config.kill_event_name);
	if (script_config.login_event_name)
		aFree(script_config.login_event_name);
	if (script_config.logout_event_name)
		aFree(script_config.logout_event_name);
	if (script_config.mapload_event_name)
		aFree(script_config.mapload_event_name);

	lua_close(L); // Close Lua

	return 0;
}
/*==========================================
 * 初期化
 *------------------------------------------
 */
int do_init_script()
{
/*	mapreg_db=numdb_init();
	mapregstr_db=numdb_init();
	script_load_mapreg();

	add_timer_func_list(script_autosave_mapreg,"script_autosave_mapreg");
	add_timer_interval(gettick()+MAPREG_AUTOSAVE_INTERVAL,
		script_autosave_mapreg,0,0,MAPREG_AUTOSAVE_INTERVAL);*/

	L = lua_open(); // Open Lua
	luaopen_base(L); // Open the basic library
	luaopen_table(L); // Open the table library
	luaopen_io(L); // Open the I/O library
	luaopen_string(L); // Open the string library
	luaopen_math(L); // Open the math library

	lua_pushliteral(L,"char_id"); // Push global key for char_id
	lua_pushnumber(L,0); // Push value 0 for char_id (the global thread is not linked to any player !)
	lua_rawset(L,LUA_GLOBALSINDEX); // Tell Lua to set char_id as a global var

	script_buildin_commands(); // Build in the Lua commands

	return 0;
}
