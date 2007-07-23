// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/malloc.h"
#include "../common/strlib.h"
#include "mapindex.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_MAPINDEX 2000

struct _indexes {
	char name[MAP_NAME_LENGTH]; //Stores map name
	bool exists; //Set to 1 if index exists
} indexes[MAX_MAPINDEX];

static unsigned short max_index = 0;

char mapindex_cfgfile[80] = "db/map_index.txt";

// Removes the extension from a map name
char* mapindex_normalize_name(char* mapname)
{
	char* ptr = strrchr(mapname, '.');
	if (ptr && stricmp(ptr, ".gat") == 0)
		*ptr = '\0'; // remove extension
	return mapname;
}

/// Adds a map to the specified index
/// Returns 1 if successful, 0 oherwise
int mapindex_addmap(int index, const char* name)
{
	char map_name[MAP_NAME_LENGTH_EXT];

	if (index < 0 || index >= MAX_MAPINDEX) {
		ShowError("(mapindex_add) Map index (%d) for \"%s\" out of range (max is %d)\n", index, name, MAX_MAPINDEX);
		return 0;
	}

	safestrncpy(map_name, name, MAP_NAME_LENGTH_EXT);
	mapindex_normalize_name(map_name);

	if (strlen(map_name) >= MAP_NAME_LENGTH) {
		ShowError("(mapindex_add) Map name %s is too long. Maps are limited to %d characters.\n", map_name, MAP_NAME_LENGTH);
		return 0;
	}

	if (indexes[index].exists)
		ShowWarning("(mapindex_add) Overriding index %d: map \"%s\" -> \"%s\"\n", index, indexes[index].name, map_name);

	safestrncpy(indexes[index].name, map_name, MAP_NAME_LENGTH);
	indexes[index].exists = true;
	if (max_index <= index)
		max_index = index+1;

	return 1;
}

unsigned short mapindex_name2id(const char* name)
{
	//TODO: Perhaps use a db to speed this up? [Skotlex]
	int i;

	char map_name[MAP_NAME_LENGTH_EXT];
	safestrncpy(map_name, name, MAP_NAME_LENGTH_EXT);
	mapindex_normalize_name(map_name);

	for (i = 1; i < max_index; i++)
	{
		if (strcmp(indexes[i].name,map_name)==0)
			return i;
	}
#ifdef MAPINDEX_AUTOADD
	if( mapindex_addmap(i,map_name) )
	{
		ShowDebug("mapindex_name2id: Auto-added map \"%s\" to position %d\n", indexes[i], i);
		return i;
	}
	ShowWarning("mapindex_name2id: Failed to auto-add map \"%s\" to position %d!\n", name, i);
	return 0;
#else
	ShowDebug("mapindex_name2id: Map \"%s\" not found in index list!\n", name);
	return 0;
#endif
}

const char* mapindex_id2name(unsigned short id)
{
	if (id > MAX_MAPINDEX || !indexes[id].exists) {
		ShowDebug("mapindex_id2name: Requested name for non-existant map index [%d] in cache.\n", id);
		return indexes[0].name; // dummy empty string so that the callee doesn't crash
	}
	return indexes[id].name;
}

void mapindex_init(void)
{
	FILE *fp;
	char line[1024];
	int last_index = -1;
	int index;
	char map_name[1024]; // only MAP_NAME_LENGTH(_EXT) under safe conditions
	
	memset (&indexes, 0, sizeof (indexes));
	fp=fopen(mapindex_cfgfile,"r");
	if(fp==NULL){
		ShowFatalError("Unable to read mapindex config file %s!\n", mapindex_cfgfile);
		exit(1); //Server can't really run without this file.
	}
	while(fgets(line, sizeof(line), fp))
	{
		if(line[0] == '/' && line[1] == '/')
			continue;

		switch (sscanf(line, "%s\t%d", map_name, &index))
		{
			case 1: //Map with no ID given, auto-assign
				index = last_index+1;
			case 2: //Map with ID given
				mapindex_addmap(index,map_name);
				break;
			default:
				continue;
		}
		last_index = index;
	}
	fclose(fp);
}

void mapindex_final(void)
{
}
