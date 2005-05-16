// $Id: itemdb.h,v 1.3 2004/09/25 05:32:18 MouseJstr Exp $
#ifndef _ITEMDB_H_
#define _ITEMDB_H_

#include "map.h"

struct item_data
{
	unsigned short nameid;
	char name[24];
	char jname[24];
	char prefix[24];
	char suffix[24];
	char cardillustname[64];
	long value_buy;
	long value_sell;
	unsigned char type;
	unsigned short class_;
	unsigned short equip;

	unsigned long weight;
	unsigned long atk;
	unsigned long def;

	unsigned short range;
	unsigned short look;
	unsigned short elv;
	unsigned short wlv;
	unsigned short view_id;

	struct {
		unsigned available : 1;			// 0
		unsigned value_notdc : 1;		// 1
		unsigned value_notoc : 1;		// 2
		unsigned no_equip : 3;			// 3,4,5
		unsigned no_use : 1;			// 6
		unsigned no_refine : 1;			// 7 [celest]
		unsigned sex : 2;				// 8,9 // male=0, female=1, all=2
		unsigned slot : 2;				// 10,11
		unsigned delay_consume : 1;		// 12 Signifies items that are not consumed inmediately upon double-click [Skotlex]
		unsigned _unused : 3;
	} flag;

	char *use_script;	// �񕜂Ƃ����S�����̒��ł�낤���Ȃ�
	char *equip_script;	// �U��,�h��̑����ݒ�����̒��ŉ\����?
};

struct random_item_data {
	unsigned short nameid;
	unsigned short per;
};

struct item_group {
	unsigned short nameid[30];	// 60 bytes
};

struct item_data* itemdb_searchname(const char *name);
struct item_data* itemdb_search(unsigned short nameid);
struct item_data* itemdb_exists(unsigned short nameid);
#define itemdb_type(n) itemdb_search(n)->type
#define itemdb_atk(n) itemdb_search(n)->atk
#define itemdb_def(n) itemdb_search(n)->def
#define itemdb_look(n) itemdb_search(n)->look
#define itemdb_weight(n) itemdb_search(n)->weight
#define itemdb_equip(n) itemdb_search(n)->equip
#define itemdb_usescript(n) itemdb_search(n)->use_script
#define itemdb_equipscript(n) itemdb_search(n)->equip_script
#define itemdb_wlv(n) itemdb_search(n)->wlv
#define itemdb_range(n) itemdb_search(n)->range
#define itemdb_slot(n) itemdb_search(n)->slot
#define	itemdb_available(n) (itemdb_exists(n) && itemdb_search(n)->flag.available)
#define	itemdb_viewid(n) (itemdb_search(n)->view_id)
int itemdb_group(unsigned short nameid);

int itemdb_searchrandomid(int flags);
int itemdb_searchrandomgroup(unsigned short groupid);

#define itemdb_value_buy(n) itemdb_search(n)->value_buy
#define itemdb_value_sell(n) itemdb_search(n)->value_sell
#define itemdb_value_notdc(n) itemdb_search(n)->flag.value_notdc
#define itemdb_value_notoc(n) itemdb_search(n)->flag.value_notoc
#define itemdb_canrefine(n) itemdb_search(n)->flag.no_refine

int itemdb_isequip(unsigned short nameid);
int itemdb_isequip2(struct item_data *);
int itemdb_isequip3(unsigned short nameid);
bool itemdb_isdropable(unsigned short nameid);

// itemdb_equip�}�N����itemdb_equippoint�Ƃ̈Ⴂ��
// �O�҂��I��db�Œ�`���ꂽ�l���̂��̂�Ԃ��̂ɑ΂�
// ��҂�sessiondata���l�������Ƒ��ł̑����\�ꏊ
// ���ׂĂ̑g�ݍ��킹��Ԃ�

void itemdb_reload(void);

void do_final_itemdb(void);
int do_init_itemdb(void);

#endif
