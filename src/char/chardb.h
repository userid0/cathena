// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _CHARDB_H_
#define _CHARDB_H_

#include "../common/mmo.h" // struct mmo_charstatus, NAME_LENGTH

typedef struct CharDB CharDB;

// standard engines
#ifdef WITH_TXT
CharDB* char_db_txt(void);
#endif
#ifdef WITH_SQL
CharDB* char_db_sql(void);
#endif


struct CharDB
{
	bool (*init)(CharDB* self);
	void (*destroy)(CharDB* self);

	bool (*create)(CharDB* self, struct mmo_charstatus* status);

	bool (*remove)(CharDB* self, const int char_id);

	bool (*save)(CharDB* self, const struct mmo_charstatus* status);

	// retrieve data using charid
	bool (*load_num)(CharDB* self, struct mmo_charstatus* status, int char_id);

	// retrieve data using charname
	bool (*load_str)(CharDB* self, struct mmo_charstatus* status, const char* name);

	// retrieve data using accid + slot
	bool (*load_slot)(CharDB* self, struct mmo_charstatus* status, int account_id, int slot);

	// look up name using charid
	bool (*id2name)(CharDB* self, int char_id, char name[NAME_LENGTH]);

	// look up charid using name
	bool (*name2id)(CharDB* self, const char* name, int* char_id);

	// look up charid using accid + slot
	bool (*slot2id)(CharDB* self, int account_id, int slot, int* char_id);
};


extern int char_num, char_max;


struct mmo_charstatus* search_character(int aid, int cid);
struct mmo_charstatus* search_character_byname(char* character_name);
int search_character_index(char* character_name);
char* search_character_name(int index);


void char_clearparty(int party_id);
int char_married(int pl1,int pl2);
int char_child(int parent_id, int child_id);
int char_family(int cid1, int cid2, int cid3);


#endif /* _CHARDB_H_ */