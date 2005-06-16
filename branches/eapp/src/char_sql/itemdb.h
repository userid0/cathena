#ifndef _ITEMDB_H_
#define _ITEMDB_H_
#include "mmo.h"

struct item_data {
	unsigned short nameid;
	char name[24],jname[24];
	int value_buy,value_sell,value_notdc,value_notoc;
	int type;
	int class_;
	int sex;
	int equip;
	int weight;
	int atk;
	int def;
	int range;
	int slot;
	int look;
	int elv;
	int wlv;
	char *use_script;	// 回復とかも全部この中でやろうかなと
	char *equip_script;	// 攻撃,防御の属性設定もこの中で可能かな?
	char available;
};

struct item_data* itemdb_search(unsigned short nameid);
#define itemdb_type(n) itemdb_search(n)->type

bool itemdb_isequip(unsigned short nameid);
bool itemdb_isequip2(struct item_data &data);

void do_final_itemdb(void);
int do_init_itemdb(void);

#endif
