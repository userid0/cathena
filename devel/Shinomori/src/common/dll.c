

#include "dll.h"
#include "mmo.h"
#include "core.h"
#include "utils.h"
#include "malloc.h"
#include "version.h"
#include "showmsg.h"



#include "base.h"


//////////////////////////////////////////////////////////////////////////
// defines 
// would be suitable for a header, but does not affect any other file, 
// so keep it away from global namespace
//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
// not much necessary here
// we change the unix to equal windows style

#define DLL_EXT		".dll"

typedef HINSTANCE DLL;

const char *LibraryError(void);

#else

#  include <dlfcn.h>

// implement a windows like loading environment
#ifdef CYGWIN
	#define DLL_EXT		".dll"
#else
	#define DLL_EXT		".so"
#endif

typedef void* DLL;

extern inline DLL LoadLibrary(const char* lpLibFileName)
{
	return dlopen(lpLibFileName,RTLD_LAZY);
}
extern inline bool FreeLibrary(DLL hLibModule)
{
	return 0==dlclose(hLibModule);
}
extern inline void *GetProcAddress(DLL hModule, const char* lpProcName)
{
	return dlsym(hModule, lpProcName);
}
extern inline const char *LibraryError(void)
{
	return dlerror();
}
#endif



#ifdef WIN32
const char *LibraryError(void)
{
	static char dllbuf[80];
	DWORD dw = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, 0, dllbuf, 80, NULL);
	return dllbuf;
}
#endif


//////////////////////////////////////////////////////////////////////////
// data definitions


//////////////////////////////////////////////////////////////////////////
// basic data layout of loadable dll files
struct Addon_Info {
	char *name;
	char type;
	char *version;
	char *req_version;
	char *description;
};

struct Addon_Event_Table {
	char *func_name;
	char *event_name;
};



//////////////////////////////////////////////////////////////////////////
// storage for loading dlls
struct Addon {
	DLL dll;
	char state;
	char *filename;
	struct Addon_Info *info;
	struct Addon *next;	
};

struct Addon_Event {
	void (*func)();
	struct Addon_Event *next;
};

struct Addon_Event_List {
	char *name;
	struct Addon_Event_List *next;
	struct Addon_Event *events;
};



//////////////////////////////////////////////
// file global data

int auto_search = 1;
struct Addon_Event_List *event_head = NULL;
struct Addon *addon_head = NULL;




//////////////////////////////////////////////
////// Plugin Events Functions //////////////////

int register_addon_func (char* name)
{
	struct Addon_Event_List *evl;
	if (name) {
		evl = (struct Addon_Event_List *) aMalloc(sizeof(struct Addon_Event_List));
		evl->name = (char *) aMalloc (strlen(name) + 1);

		evl->next = event_head;
		strcpy(evl->name, name);
		evl->events = NULL;
		event_head = evl;
	}
	return 0;
}

struct Addon_Event_List *search_addon_func (char *name)
{
	struct Addon_Event_List *evl = event_head;
	while (evl) {
		if (strcasecmp(evl->name, name) == 0)
			return evl;
		evl = evl->next;
	}
	return NULL;
}

int register_addon_event (void (*func)(), char* name)
{
	struct Addon_Event_List *evl = search_addon_func(name);
	if (evl) {
		struct Addon_Event *ev;

		ev = (struct Addon_Event *) aMalloc(sizeof(struct Addon_Event));
		ev->func = func;
		ev->next = NULL;

		if (evl->events == NULL)
			evl->events = ev;
		else {
			struct Addon_Event *ev2 = evl->events;
			while (ev2) {
				if (ev2->next == NULL) {
					ev2->next = ev;
					break;
				}
				ev2 = ev2->next;
			}
		}
	}
	return 0;
}

int addon_event_trigger (char *name)
{
	int c = 0;
	struct Addon_Event_List *evl = search_addon_func(name);
	if (evl)
	{
		struct Addon_Event *ev = evl->events;
		while (ev)
		{
			ev->func();
			ev = ev->next;
			c++;
		}
	}
	return c;
}

////// Plugins Core /////////////////////////

void dll_open (const char *filename)
{
	struct Addon *addon;
	struct Addon_Info *info;
	struct Addon_Event_Table *events;

	//printf ("loading %s\n", filename);
	
	// Check if the plugin has been loaded before
	addon = addon_head;
	while (addon) {
		if (strcasecmp(addon->filename, filename) == 0)
			return;
		addon = addon->next;
	}

	addon = (struct Addon *)aMallocA(sizeof(struct Addon));
	addon->state = -1;	// not loaded

	addon->dll = LoadLibrary(filename);
	if (!addon->dll) {
		//printf ("not loaded (invalid file) : %s\n", filename);
		goto err;
	}
	
	// Retrieve plugin information
	addon->state = 0;	// initialising
	info = (Addon_Info *)GetProcAddress(addon->dll, "addon_info");
	if( !info ||
		// plugin is based on older code
		atof(info->req_version) < atof(DLL_VERSION) )
	{
		//printf ("not loaded (incompatible) : %s\n", filename);		
		goto err;
	}
	addon->info = info;

	addon->filename = (char *) aMalloc (strlen(filename) + 1);
	strcpy(addon->filename, filename);
	
	// Register plugin events
	events = (struct Addon_Event_Table *)GetProcAddress(addon->dll, "addon_event_table");
	if (events)
	{
		int i = 0;
		while (events[i].func_name) {
			void (*func)(void);
			func = (void (*)(void))GetProcAddress(addon->dll, events[i].func_name);
			if (func)
				register_addon_event (func, events[i].event_name);
			i++;
		}
	}

	// always place the new loaded dll in front of all others
	addon->next = addon_head;
	addon_head = addon;


	addon->state = 1;	// fully loaded
	ShowStatus ("Done loading plugin '"CL_WHITE"%s"CL_RESET"'\n", addon->info->name);
	return;

err:

	if(addon)
	{
		if(addon->filename) aFree(addon->filename);
		if(addon->dll) FreeLibrary(addon->dll);
		aFree(addon);
	}
	return;
}



////// Initialize/Finalize ////////////////////

int dll_config_read(const char *cfgName)
{
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	fp = fopen(cfgName, "r");
	if (fp == NULL) {
		ShowError("File not found: %s\n", cfgName);
		return 1;
	}
	while (fgets(line, 1020, fp)) {
		if(line[0] == '/' && line[1] == '/')
			continue;
		if (sscanf(line,"%[^:]: %[^\r\n]", w1, w2) != 2)
			continue;

		if (strcasecmp(w1, "auto_search") == 0) {
			if(strcasecmp(w2, "yes")==0)
				auto_search = 1;
			else if(strcasecmp(w2, "no")==0)
				auto_search = 0;
			else auto_search = atoi(w2);
		} else if (strcasecmp(w1, "addon") == 0) {
			char filename[128];
			sprintf (filename, "addons/%s%s", w2, DLL_EXT);
			dll_open(filename);
		} else if (strcasecmp(w1, "import") == 0)
			dll_config_read(w2);
	}
	fclose(fp);
	return 0;
}

void dll_init (void)
{
	char *DLL_CONF_FILENAME = "conf/addon_athena.conf";
	register_addon_func("DLL_Init");
	register_addon_func("DLL_Final");
	register_addon_func("Athena_Init");
	register_addon_func("Athena_Final");

	dll_config_read (DLL_CONF_FILENAME);

	if (auto_search)
		findfile("addons", DLL_EXT, dll_open);

	addon_event_trigger("DLL_Init");

	return;
}

void dll_final (void)
{
	struct Addon *addon = addon_head, *addon2;
	struct Addon_Event_List *evl = event_head, *evl2;
	struct Addon_Event *ev, *ev2;

	addon_event_trigger("DLL_Final");

	while (evl)
	{
		ev = evl->events;
		while (ev) {
			ev2 = ev->next;
			aFree(ev);
			ev = ev2;
		}
		evl2 = evl->next;
		aFree(evl->name);
		aFree(evl);
		evl = evl2;
	}
	while (addon)
	{
		addon2 = addon->next;
		addon->state = 0;
		FreeLibrary(addon->dll);
		aFree(addon->filename);
		aFree(addon);
		addon = addon2;
	}

	return;
}
