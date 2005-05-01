// $Id: map.c,v 1.6 2004/09/25 17:37:01 MouseJstr Exp $

#include "base.h"
#include "core.h"
#include "timer.h"
#include "socket.h"
#include "showmsg.h"
#include "utils.h"
#include "nullpo.h"
#include "db.h"
#include "grfio.h"
#include "malloc.h"
#include "version.h"

#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "npc.h"
#include "pc.h"
#include "status.h"
#include "mob.h"
#include "chat.h"
#include "itemdb.h"
#include "storage.h"
#include "skill.h"
#include "trade.h"
#include "party.h"
#include "battle.h"
#include "script.h"
#include "guild.h"
#include "pet.h"
#include "atcommand.h"
#include "charcommand.h"
#include "log.h"
#include "mail.h"


#ifdef MEMWATCH
#include "memwatch.h"
#endif


#ifndef TXT_ONLY

MYSQL mmysql_handle;
MYSQL_RES* 	sql_res ;
MYSQL_ROW	sql_row ;
char tmp_sql[65535]="";

MYSQL lmysql_handle;
MYSQL_RES* lsql_res ;
MYSQL_ROW  lsql_row ;
char tmp_lsql[65535]="";

MYSQL mail_handle; // mail system [Valaris]
MYSQL_RES* 	mail_res ;
MYSQL_ROW	mail_row ;
char tmp_msql[65535]="";

unsigned short map_server_port = 3306;
char map_server_ip[16] = "127.0.0.1";
char map_server_id[32] = "ragnarok";
char map_server_pw[32] = "ragnarok";
char map_server_db[32] = "ragnarok";
int db_use_sqldbs = 0;

unsigned short login_server_port = 3306;
char login_server_ip[16] = "127.0.0.1";
char login_server_id[32] = "ragnarok";
char login_server_pw[32] = "ragnarok";
char login_server_db[32] = "ragnarok";

char item_db_db[32] = "item_db";
char mob_db_db[32] = "mob_db";
char login_db[32] = "login";
char login_db_level[32] = "level";
char login_db_account_id[32] = "account_id";

char log_db[32] = "log";
char log_db_ip[16] = "127.0.0.1";
char log_db_id[32] = "ragnarok";
char log_db_pw[32] = "ragnarok";
int log_db_port = 3306;

char gm_db[32] = "login";
char gm_db_level[32] = "level";
char gm_db_account_id[32] = "account_id";

int lowest_gm_level = 1;
int read_gm_interval = 600000;

char char_db[32] = "char";

#endif//TXT_OMLY

char *INTER_CONF_NAME;
char *LOG_CONF_NAME;
char *MAP_CONF_NAME;
char *BATTLE_CONF_FILENAME;
char *ATCOMMAND_CONF_FILENAME;
char *CHARCOMMAND_CONF_FILENAME;
char *SCRIPT_CONF_NAME;
char *MSG_CONF_NAME;
char *GRF_PATH_FILENAME;

#define USE_AFM
#define USE_AF2

// �ɗ� static�Ń�?�J����?�߂�
static struct dbt * id_db=NULL;
static struct dbt * map_db=NULL;
static struct dbt * nick_db=NULL;
static struct dbt * charid_db=NULL;

static size_t users=0;
static struct block_list *objects[MAX_FLOORITEM];
static int first_free_object_id=0,last_object_id=0;

#define block_free_max 1048576
static void *block_free[block_free_max];
static int block_free_count = 0, block_free_lock = 0;

#define BL_LIST_MAX 1048576
static struct block_list *bl_list[BL_LIST_MAX];
static int bl_list_count = 0;

static char afm_dir[1024] = ""; // [Valaris]

struct map_data map[MAX_MAP_PER_SERVER];
size_t map_num = 0;

//int map_port=0;

int autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
int agit_flag = 0;
int night_flag = 0; // 0=day, 1=night [Yor]

//Added for Mugendai's I'm Alive mod
int imalive_on=0;
int imalive_time=60;
//Added by Mugendai for GUI
int flush_on=1;
int flush_time=100;

struct charid2nick {
	char nick[24];
	int req_id;
};

// �ޫë׫���ë������īի髰(map_athana.conf?��read_map_from_cache���E)
// 0:���Ī��ʪ� 1:ު?������ 2:?������
int  map_read_flag = READ_FROM_GAT;
char map_cache_file[256]="db/map.info"; // �ޫë׫���ë���ի�����E�

char motd_txt[256] = "conf/motd.txt";
char help_txt[256] = "conf/help.txt";

char wisp_server_name[24] = "Server"; // can be modified in char-server configuration file

int console = 0;

int CHECK_INTERVAL = 3600000; // [Valaris]

/*==========================================
 * �Smap�I?�v�ł̐�??�ݒ�
 * (char�I���瑗���Ă���)
 *------------------------------------------
 */
void map_setusers(int fd) 
{
	users = RFIFOL(fd,2);
	// send some anser
	WFIFOW(fd,0) = 0x2718;
	WFIFOSET(fd,2);
}

/*==========================================
 * �Smap�I?�v�ł̐�??�擾 (/w�ւ�?���p)
 *------------------------------------------
 */
int map_getusers(void) {
	return users;
}

//
// block�폜�̈��S���m��?��
//

/*==========================================
 * block��aFree����Ƃ�aFree��?���ɌĂ�
 * ���b�N����Ă���Ƃ��̓o�b�t�@�ɂ��߂�
 *------------------------------------------
 */
int map_freeblock( void *bl )
{
	if(block_free_lock==0){
		aFree(bl);
	}
	else{
		if( block_free_count>=block_free_max ) {
			if(battle_config.error_log)
				ShowMessage("map_freeblock: *WARNING* too many aFree block! %d %d\n",
			block_free_count,block_free_lock);
		}
		else
			block_free[block_free_count++]=bl;
	}
	return block_free_lock;
}
/*==========================================
 * block��free����sI�ɋ֎~����
 *------------------------------------------
 */
int map_freeblock_lock(void) {
	return ++block_free_lock;
}

/*==========================================
 * block��aFree�̃��b�N����������
 * ���̂Ƃ��A���b�N�����S�ɂȂ��Ȃ��
 * �o�b�t�@�ɂ��܂��Ă���block��S���폜
 *------------------------------------------
 */
int map_freeblock_unlock(void) {
	if ((--block_free_lock) == 0) {
		int i;
//		if(block_free_count>0) {
//			if(battle_config.error_log)
//				ShowMessage("map_freeblock_unlock: aFree %d object\n",block_free_count);
//		}
		for(i=0;i<block_free_count;i++){
			aFree(block_free[i]);
			block_free[i] = NULL;
		}
		block_free_count=0;
	}else if(block_free_lock<0){
		if(battle_config.error_log)
			ShowMessage("map_freeblock_unlock: lock count < 0 !\n");
		block_free_lock = 0; // ����ȍ~�̃��b�N�Ɏx�Ⴊ�o�Ă���̂Ń��Z�b�g
	}
	return block_free_lock;
}

// map_freeblock_lock() ���Ă�� map_freeblock_unlock() ���Ă΂Ȃ�
// �֐����������̂ŁA����I��block_free_lock�����Z�b�g����悤�ɂ���B
// ���̊֐��́Ado_timer() �̃g�b�v���x������Ă΂��̂ŁA
// block_free_lock �𒼐ڂ������Ă��x�ᖳ���͂��B

int map_freeblock_timer(int tid,unsigned long tick,int id,int data) {
	if(block_free_lock > 0) {
		ShowMessage("map_freeblock_timer: block_free_lock(%d) is invalid.\n",block_free_lock);
		block_free_lock = 1;
		map_freeblock_unlock();
	}
	// else {
	// 	ShowMessage("map_freeblock_timer: check ok\n");
	// }
	return 0;
}

//
// block��?��
//
/*==========================================
 * map[]��block_list����?�����Ă���ꍇ��
 * bl->prev��bl_head�̃A�h���X�����Ă���
 *------------------------------------------
 */
static struct block_list bl_head;

/*==========================================
 * map[]��block_list�ɒǉ�
 * mob��?�������̂ŕʃ��X�g
 *
 * ?��link?�݂��̊m�F�������B��?����
 *------------------------------------------
 */
int map_addblock(struct block_list *bl)
{
	size_t m,x,y;

	nullpo_retr(0, bl);

	if(bl->prev != NULL){
			if(battle_config.error_log)
				ShowMessage("map_addblock error : bl->prev!=NULL\n");
		return 0;
	}

	m=bl->m;
	x=bl->x;
	y=bl->y;
	if(m<0 || m>=map_num ||
	   x<0 || x>=map[m].xs ||
	   y<0 || y>=map[m].ys)
		return 1;

	if(bl->type==BL_MOB){
		bl->next = map[m].block_mob[x/BLOCK_SIZE+(y/BLOCK_SIZE)*map[m].bxs];
		bl->prev = &bl_head;
		if(bl->next) bl->next->prev = bl;
		map[m].block_mob[x/BLOCK_SIZE+(y/BLOCK_SIZE)*map[m].bxs] = bl;
		map[m].block_mob_count[x/BLOCK_SIZE+(y/BLOCK_SIZE)*map[m].bxs]++;
	} else {
		bl->next = map[m].block[x/BLOCK_SIZE+(y/BLOCK_SIZE)*map[m].bxs];
		bl->prev = &bl_head;
		if(bl->next) bl->next->prev = bl;
		map[m].block[x/BLOCK_SIZE+(y/BLOCK_SIZE)*map[m].bxs] = bl;
		map[m].block_count[x/BLOCK_SIZE+(y/BLOCK_SIZE)*map[m].bxs]++;
		if(bl->type==BL_PC)
			map[m].users++;
	}

	return 0;
}

/*==========================================
 * map[]��block_list����O��
 * prev��NULL�̏ꍇlist��?�����ĂȂ�
 *------------------------------------------
 */
int map_delblock(struct block_list *bl)
{
	int b;
	nullpo_retr(0, bl);

	// ?��blocklist����?���Ă���
	if(bl->prev==NULL){
		if(bl->next!=NULL){
			// prev��NULL��next��NULL�łȂ��̂͗L���Ă͂Ȃ�Ȃ�
			if(battle_config.error_log)
				ShowMessage("map_delblock error : bl->next!=NULL\n");
		}
		return 0;
	}

	b = bl->x/BLOCK_SIZE+(bl->y/BLOCK_SIZE)*map[bl->m].bxs;

	if(bl->type==BL_PC)
		map[bl->m].users--;
	if(bl->next)
		bl->next->prev = bl->prev;
	if(bl->prev==&bl_head){
		// ���X�g�̓��Ȃ̂ŁAmap[]��block_list���X�V����
		if(bl->type==BL_MOB){
			map[bl->m].block_mob[b] = bl->next;
			if((map[bl->m].block_mob_count[b]--) < 0)
				map[bl->m].block_mob_count[b] = 0;
		} else {
			map[bl->m].block[b] = bl->next;
			if((map[bl->m].block_count[b]--) < 0)
				map[bl->m].block_count[b] = 0;
		}
	} else {
		bl->prev->next = bl->next;
	}
	bl->next = NULL;
	bl->prev = NULL;

	return 0;
}

/*==========================================
 * ��?��PC�l?��?���� (���ݖ��g�p)
 *------------------------------------------
 */
int map_countnearpc(int m, int x, int y) {
	int bx,by,c=0;
	struct block_list *bl=NULL;

	if(map[m].users==0)
		return 0;
	for(by=y/BLOCK_SIZE-AREA_SIZE/BLOCK_SIZE-1;by<=y/BLOCK_SIZE+AREA_SIZE/BLOCK_SIZE+1;by++){
		if(by<0 || by>=map[m].bys)
			continue;
		for(bx=x/BLOCK_SIZE-AREA_SIZE/BLOCK_SIZE-1;bx<=x/BLOCK_SIZE+AREA_SIZE/BLOCK_SIZE+1;bx++){
			if(bx<0 || bx>=map[m].bxs)
				continue;
			bl = map[m].block[bx+by*map[m].bxs];
			for(;bl;bl=bl->next){
				if(bl->type==BL_PC)
					c++;
			}
		}
	}
	return c;
}

/*==========================================
 * �Z�����PC��MOB��?��?���� (�O�����h�N���X�p)
 *------------------------------------------
 */
int map_count_oncell(int m, int x, int y) {
	int bx,by;
	struct block_list *bl=NULL;
	int i,c;
	int count = 0;

	if (x < 0 || y < 0 || (x >= map[m].xs) || (y >= map[m].ys))
		return 1;
	bx = x/BLOCK_SIZE;
	by = y/BLOCK_SIZE;

	bl = map[m].block[bx+by*map[m].bxs];
	c = map[m].block_count[bx+by*map[m].bxs];
	for(i=0;i<c && bl;i++,bl=bl->next){
		if(bl->x == x && bl->y == y && bl->type == BL_PC) count++;
	}
	bl = map[m].block_mob[bx+by*map[m].bxs];
	c = map[m].block_mob_count[bx+by*map[m].bxs];
	for(i=0;i<c && bl;i++,bl=bl->next){
		if(bl->x == x && bl->y == y) count++;
	}
	if(!count) count = 1;
	return count;
}
/*
 * ����E���������̸�Ī���������E�˫ëȪ�����
 */
struct skill_unit *map_find_skill_unit_oncell(struct block_list *target,int x,int y,unsigned short skill_id,struct skill_unit *out_unit)
{
	int m,bx,by;
	struct block_list *bl;
	int i,c;
	struct skill_unit *unit;
	m = target->m;

	if (x < 0 || y < 0 || (x >= map[m].xs) || (y >= map[m].ys))
		return NULL;
	bx = x/BLOCK_SIZE;
	by = y/BLOCK_SIZE;

	bl = map[m].block[bx+by*map[m].bxs];
	c = map[m].block_count[bx+by*map[m].bxs];
	for(i=0;i<c && bl;i++,bl=bl->next){
		if (bl->x != x || bl->y != y || bl->type != BL_SKILL)
			continue;
		unit = (struct skill_unit *) bl;
		if (unit==out_unit || !unit->alive ||
				!unit->group || unit->group->skill_id!=skill_id)
			continue;
		if (battle_check_target(&unit->bl,target,unit->group->target_flag)>0)
			return unit;
	}
	return NULL;
}

/*==========================================
 * map m (x0,y0)-(x1,y1)?�̑Sobj��?����
 * func���Ă�
 * type!=0 �Ȃ炻�̎�ނ̂�
 *------------------------------------------
 */
void map_foreachinarea(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int type,...) {
	va_list ap;
	int bx,by;
	struct block_list *bl=NULL;
	int blockcount=bl_list_count,i,c;

	if(m < 0)
		return;
	
	if(x0>x1) swap(x0,x1);
	if(y0>y1) swap(y0,y1);

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= map[m].xs) x1 = map[m].xs-1;
	if (y1 >= map[m].ys) y1 = map[m].ys-1;
	if (type == 0 || type != BL_MOB)
		for (by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++) {
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block[bx+by*map[m].bxs];
				c = map[m].block_count[bx+by*map[m].bxs];
				for(i=0;i<c && bl;i++,bl=bl->next){
					if(bl && type && bl->type!=type)
						continue;
					if(bl && bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && bl_list_count<BL_LIST_MAX)
						bl_list[bl_list_count++]=bl;
				}
			}
		}
	if(type==0 || type==BL_MOB)
		for(by=y0/BLOCK_SIZE;by<=y1/BLOCK_SIZE;by++){
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block_mob[bx+by*map[m].bxs];
				c = map[m].block_mob_count[bx+by*map[m].bxs];
				for(i=0;i<c && bl;i++,bl=bl->next){
					if(bl && bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && bl_list_count<BL_LIST_MAX)
						bl_list[bl_list_count++]=bl;
				}
			}
		}

	if(bl_list_count>=BL_LIST_MAX) {
		if(battle_config.error_log)
			ShowMessage("map_foreachinarea: *WARNING* block count too many!\n");
	}


	va_start(ap,type);
	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=blockcount;i<bl_list_count;i++)
		if(bl_list[i]->prev)	// �L?���ǂ����`�F�b�N
			func(bl_list[i],ap);

	map_freeblock_unlock();	// �����������
	va_end(ap);

	bl_list_count = blockcount;
}

/*==========================================
 * ��`(x0,y0)-(x1,y1)��(dx,dy)�ړ������b�
 * �̈�O�ɂȂ�̈�(��`��L���`)?��obj��
 * ?����func���Ă�
 *
 * dx,dy��-1,0,1�݂̂Ƃ���i�ǂ�Ȓl�ł��������ۂ��H�j
 *------------------------------------------
 */
void map_foreachinmovearea(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int dx,int dy,int type,...) {
	int bx,by;
	struct block_list *bl=NULL;
	va_list ap;
	int blockcount=bl_list_count,i,c;

	va_start(ap,type);
	if(dx==0 || dy==0){
		// ��`�̈�̏ꍇ
		if(dx==0){
			if(dy<0){
				y0=y1+dy+1;
			} else {
				y1=y0+dy-1;
			}
		} else if(dy==0){
			if(dx<0){
				x0=x1+dx+1;
			} else {
				x1=x0+dx-1;
			}
		}
		if(x0<0) x0=0;
		if(y0<0) y0=0;
		if(x1>=map[m].xs) x1=map[m].xs-1;
		if(y1>=map[m].ys) y1=map[m].ys-1;
		for(by=y0/BLOCK_SIZE;by<=y1/BLOCK_SIZE;by++){
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block[bx+by*map[m].bxs];
				c = map[m].block_count[bx+by*map[m].bxs];
				for(i=0;i<c && bl;i++,bl=bl->next){
					if(bl && type && bl->type!=type)
						continue;
					if(bl && bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && bl_list_count<BL_LIST_MAX)
						bl_list[bl_list_count++]=bl;
				}
				bl = map[m].block_mob[bx+by*map[m].bxs];
				c = map[m].block_mob_count[bx+by*map[m].bxs];
				for(i=0;i<c && bl;i++,bl=bl->next){
					if(bl && type && bl->type!=type)
						continue;
					if(bl && bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && bl_list_count<BL_LIST_MAX)
						bl_list[bl_list_count++]=bl;
				}
			}
		}
	}else{
		// L���̈�̏ꍇ

		if(x0<0) x0=0;
		if(y0<0) y0=0;
		if(x1>=map[m].xs) x1=map[m].xs-1;
		if(y1>=map[m].ys) y1=map[m].ys-1;
		for(by=y0/BLOCK_SIZE;by<=y1/BLOCK_SIZE;by++){
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block[bx+by*map[m].bxs];
				c = map[m].block_count[bx+by*map[m].bxs];
				for(i=0;i<c && bl;i++,bl=bl->next){
					if(bl && type && bl->type!=type)
						continue;
					if((bl) && !(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1))
						continue;
					if((bl) && ((dx>0 && bl->x<x0+dx) || (dx<0 && bl->x>x1+dx) ||
						(dy>0 && bl->y<y0+dy) || (dy<0 && bl->y>y1+dy)) &&
						bl_list_count<BL_LIST_MAX)
							bl_list[bl_list_count++]=bl;
				}
				bl = map[m].block_mob[bx+by*map[m].bxs];
				c = map[m].block_mob_count[bx+by*map[m].bxs];
				for(i=0;i<c && bl;i++,bl=bl->next){
					if(bl && type && bl->type!=type)
						continue;
					if((bl) && !(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1))
						continue;
					if((bl) && ((dx>0 && bl->x<x0+dx) || (dx<0 && bl->x>x1+dx) ||
						(dy>0 && bl->y<y0+dy) || (dy<0 && bl->y>y1+dy)) &&
						bl_list_count<BL_LIST_MAX)
							bl_list[bl_list_count++]=bl;
				}
			}
		}

	}

	if(bl_list_count>=BL_LIST_MAX) {
		if(battle_config.error_log)
			ShowMessage("map_foreachinarea: *WARNING* block count too many!\n");
	}

	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=blockcount;i<bl_list_count;i++)
		if(bl_list[i]->prev) {	// �L?���ǂ����`�F�b�N
			if (bl_list[i]->type == BL_PC
			  && session[((struct map_session_data *) bl_list[i])->fd] == NULL)
				continue;
			func(bl_list[i],ap);
		}

	map_freeblock_unlock();	// �����������

	va_end(ap);
	bl_list_count = blockcount;
}

// -- moonsoul	(added map_foreachincell which is a rework of map_foreachinarea but
//			 which only checks the exact single x/y passed to it rather than an
//			 area radius - may be more useful in some instances)
//
void map_foreachincell(int (*func)(struct block_list*,va_list),int m,int x,int y,int type,...) {
	int bx,by;
	struct block_list *bl=NULL;
	va_list ap;
	int blockcount=bl_list_count,i,c;

	va_start(ap,type);

	by=y/BLOCK_SIZE;
	bx=x/BLOCK_SIZE;

	if(type==0 || type!=BL_MOB)
	{
		bl = map[m].block[bx+by*map[m].bxs];
		c = map[m].block_count[bx+by*map[m].bxs];
		for(i=0;i<c && bl;i++,bl=bl->next)
		{
			if(type && bl && bl->type!=type)
				continue;
			if(bl && bl->x==x && bl->y==y && bl_list_count<BL_LIST_MAX)
				bl_list[bl_list_count++]=bl;
		}
	}

	if(type==0 || type==BL_MOB)
	{
		bl = map[m].block_mob[bx+by*map[m].bxs];
		c = map[m].block_mob_count[bx+by*map[m].bxs];
		for(i=0;i<c && bl;i++,bl=bl->next)
		{
			if(bl && bl->x==x && bl->y==y && bl_list_count<BL_LIST_MAX)
				bl_list[bl_list_count++]=bl;
		}
	}

	if(bl_list_count>=BL_LIST_MAX) {
		if(battle_config.error_log)
			ShowMessage("map_foreachincell: *WARNING* block count too many!\n");
	}

	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=blockcount;i<bl_list_count;i++)
		if(bl_list[i]->prev)	// �L?���ǂ����`�F�b�N
			func(bl_list[i],ap);

	map_freeblock_unlock();	// �����������

	va_end(ap);
	bl_list_count = blockcount;
}

/*============================================================
* For checking a path between two points (x0, y0) and (x1, y1)
*------------------------------------------------------------
 */
void map_foreachinpath(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int range,int type,...)
{
/*	va_list ap;
	double deltax = 0.0;
	double deltay = 0.0;
	int t, bx, by;
	int *xs, *ys;
	int blockcount = bl_list_count, i, c;
	struct block_list *bl = NULL;
	
	if(m < 0)
		return;
	va_start(ap,type);
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= map[m].xs) x1 = map[m].xs-1;
	if (y1 >= map[m].ys) y1 = map[m].ys-1;

	// I'm not finished thinking on it
	// but first it might better use a parameter equation
	// x=(x1-x0)*t+x0; y=(y1-y0)*t+y0; t=[0,1]
	// would not need special case aproximating for infinity/zero slope
	// so maybe this way:

	// find maximum runindex
	int tmax = abs(y1-y0);
	if(tmax < abs(x1-x0))
		tmax = abs(x1-x0);

	xs = (int *)aCallocA(tmax + 1, sizeof(int));
	ys = (int *)aCallocA(tmax + 1, sizeof(int));

	// pre-calculate delta values for x and y destination
	// should speed up cause you don't need to divide in the loop
	if(tmax>0)
	{
		deltax = ((double)(x1-x0)) / ((double)tmax);
		deltay = ((double)(y1-y0)) / ((double)tmax);
	}
	// go along the index
	for(t=0; t<=tmax; t++)
	{
		int x = (int)floor(deltax * (double)t +0.5)+x0;
		int y = (int)floor(deltay * (double)t +0.5)+y0;
		// the xy pairs of points in line between x0y0 and x1y1 
		// including start and end point
		xs[t] = x;
		ys[t] = y;		
	}

	if (type == 0 || type != BL_MOB)


this here is  wrong, 
there is no check if x0<x1 and y0<y1 
but this is not valid in 3 of 4 cases, 
so in this case here you check only blocks when shooting to a positive direction
shooting in other directions just do nothing like the skill has failed
if you want to keep this that way then check and swap x0,y0 with x1,y1

		for (by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++) {
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block[bx+by*map[m].bxs];
				c = map[m].block_count[bx+by*map[m].bxs];
				for(i=0;i<c && bl;i++,bl=bl->next){
					if(bl) {
						if (type && bl->type!=type)
							continue;
						for(t=0; t<=tmax; t++)
							if(bl->x==xs[t] && bl->y==ys[t] && bl_list_count<BL_LIST_MAX)
								bl_list[bl_list_count++]=bl;
					}
				}
			}
		}
	if(type==0 || type==BL_MOB)
		for(by=y0/BLOCK_SIZE;by<=y1/BLOCK_SIZE;by++){
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block_mob[bx+by*map[m].bxs];
				c = map[m].block_mob_count[bx+by*map[m].bxs];
				for(i=0;i<c && bl;i++,bl=bl->next){
					if(bl) {
						for(t=0; t<=tmax; t++)
							if(bl->x==xs[t] && bl->y==ys[t] && bl_list_count<BL_LIST_MAX)
								bl_list[bl_list_count++]=bl;
					}
				}
			}
		}

	if(bl_list_count>=BL_LIST_MAX) {
		if(battle_config.error_log)
			ShowMessage("map_foreachinpath: *WARNING* block count too many!\n");
	}

	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=blockcount;i<bl_list_count;i++)
		if(bl_list[i]->prev)	// �L?���ǂ����`�F�b�N
			func(bl_list[i],ap);

	map_freeblock_unlock();	// �����������

	bl_list_count = blockcount;
	aFree (xs);
	aFree (ys);
 	va_end(ap);	

*/

/*
//////////////////////////////////////////////////////////////
//
// sharp shooting 1
//
//////////////////////////////////////////////////////////////
// problem: 
// finding targets standing on and within some range of a line
// (t1,t2 t3 and t4 get hit)
//
//     target 1
//      x t4
//     t2
// t3 x
//   x
//  S
//////////////////////////////////////////////////////////////
// solution 1 (straight forward, but a bit calculation expensive)
// calculating perpendiculars from quesionable mobs to the straight line
// if the mob is hit then depends on the distance to the line
// 
// solution 2 (complex, need to handle many cases, but maybe faster)
// make a formula to deside if a given (x,y) is within a shooting area
// the shape can be ie. rectangular or triangular
// if the mob is hit then depends on if the mob is inside or outside the area
// I'm not going to implement this, but if somebody is interested
// in vector algebra, it might be some fun 

//////////////////////////////////////////////////////////////
// possible shooting ranges (I prefer the second one)
//////////////////////////////////////////////////////////////
//
//  ----------------                     ------
//  ----------------               ------------
// Sxxxxxxxxxxxxxxxxtarget    Sxxxxxxxxxxxxxxxxtarget
//  ----------------               ------------
//  ----------------                      -----
//
// the original code implemented the left structure
// might be not that realistic, so I changed to the other one
// I take "range" as max distance from the line
//////////////////////////////////////////////////////////////

	va_list ap;
	int i, blockcount = bl_list_count;
	struct block_list *bl;
	int c1,c2;

///////////
	double deltax,deltay;
	double k,kfact,knorm;
	double v1,v2,distance;
	double xm,ym,rd;
	int bx,by,bx0,bx1,by0,by1;
//////////////
	// no map
	if(m < 0) return;

	// xy out of range
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= map[m].xs) x1 = map[m].xs-1;
	if (y1 >= map[m].ys) y1 = map[m].ys-1;

	///////////////////////////////
	// stuff for a linear equation in xy coord to calculate 
	// the perpendicular from a block xy to the straight line
	deltax = (x1-x0);
	deltay = (y1-y0);
	kfact = (deltax*deltax+deltay*deltay);	// the sqare length of the line
	knorm = -deltax*x0-deltay*y0;			// the offset vector param

//ShowMessage("(%i,%i)(%i,%i) range: %i\n",x0,y0,x1,y1,range);

	if(kfact==0) return; // shooting at the standing position should not happen
	kfact = 1/kfact; // divide here and multiply in the loop

	range *= range; // compare with range^2 so we can skip a sqrt and signs

	///////////////////////////////
	// prepare shooting area check
	xm = (x1+x0)/2.0;
	ym = (y1+y0)/2.0;// middle point on the shooting line
	// the sqared radius of a circle around the shooting range
	// plus the sqared radius of a block
	rd = (x0-xm)*(x0-xm) + (y0-ym)*(y0-ym) + (range*range)
					+BLOCK_SIZE*BLOCK_SIZE/2;
	// so whenever a block midpoint is within this circle
	// some of the block area is possibly within the shooting range

	///////////////////////////////
	// what blocks we need to test
	// blocks covered by the xy position of begin and end of the line
	bx0 = x0/BLOCK_SIZE;
	bx1 = x1/BLOCK_SIZE;
	by0 = y0/BLOCK_SIZE;
	by1 = y1/BLOCK_SIZE;
	// swap'em for a smallest-to-biggest run
	if(bx0>bx1)	swap(bx0,bx1);
	if(by0>by1)	swap(by0,by1);

	// enlarge the block area by a range value and 1
	// so we can be sure to process all blocks that might touch the shooting area
	// in this case here with BLOCK_SIZE=8 and range=2 it will be only enlarged by 1
	// but I implement it anyway just in case that ranges will be larger 
	// or BLOCK_SIZE smaller in future
	i = (range/BLOCK_SIZE+1);//temp value
	if(bx0>i)				bx0 -=i; else bx0=0;
	if(by0>i)				by0 -=i; else by0=0;
	if(bx1+i<map[m].bxs)	bx1 +=i; else bx1=map[m].bxs-1;
	if(by1+i<map[m].bys)	by1 +=i; else by1=map[m].bys-1;


//ShowMessage("run for (%i,%i)(%i,%i)\n",bx0,by0,bx1,by1);
	for(bx=bx0; bx<=bx1; bx++)
	for(by=by0; by<=by1; by++)
	{	// block xy
		c1  = map[m].block_count[bx+by*map[m].bxs];		// number of elements in the block
		c2  = map[m].block_mob_count[bx+by*map[m].bxs];	// number of mobs in the mob block
		if( (c1==0) && (c2==0) ) continue;				// skip if nothing in the blocks

//ShowMessage("block(%i,%i) %i %i\n",bx,by,c1,c2);fflush(stdout);
		// test if the mid-point of the block is too far away
		// so we could skip the whole block in this case 
		v1 = (bx*BLOCK_SIZE+BLOCK_SIZE/2-xm)*(bx*BLOCK_SIZE+BLOCK_SIZE/2-xm)
			+(by*BLOCK_SIZE+BLOCK_SIZE/2-ym)*(by*BLOCK_SIZE+BLOCK_SIZE/2-ym);
//ShowMessage("block(%i,%i) v1=%f rd=%f\n",bx,by,v1,rd);fflush(stdout);		
		// check for the worst case scenario
		if(v1 > rd)	continue;

		// it seems that the block is at least partially covered by the shooting range
		// so we go into it
		if(type==0 || type!=BL_MOB) {
  			bl = map[m].block[bx+by*map[m].bxs];		// a block with the elements
			for(i=0;i<c1 && bl;i++,bl=bl->next){		// go through all elements
				if( bl && ( !type || bl->type==type ) && bl_list_count<BL_LIST_MAX )
				{
					// calculate the perpendicular from block xy to the straight line
					k = kfact*(deltax*bl->x + deltay*bl->y + knorm);
					// check if the perpendicular is within start and end of our line
					if(k>=0 && k<=1)
					{	// calculate the distance
						v1 = deltax*k+x0 - bl->x;
						v2 = deltay*k+y0 - bl->y;
						distance = v1*v1+v2*v2;
						// triangular shooting range
						if( distance <= range*k*k )
							bl_list[bl_list_count++]=bl;
					}
				}
			}//end for elements
		}

		if(type==0 || type==BL_MOB) {
			bl = map[m].block_mob[bx+by*map[m].bxs];	// and the mob block
			for(i=0;i<c2 && bl;i++,bl=bl->next){
				if(bl && bl_list_count<BL_LIST_MAX) {
					// calculate the perpendicular from block xy to the straight line
					k = kfact*(deltax*bl->x + deltay*bl->y + knorm);
//ShowMessage("mob: (%i,%i) k=%f ",bl->x,bl->y, k);
					// check if the perpendicular is within start and end of our line
					if(k>=0 && k<=1)
					{
						 v1 = deltax*k+x0 - bl->x;
						 v2 = deltay*k+y0 - bl->y;
						 distance = v1*v1+v2*v2;
//ShowMessage("dist: %f",distance);
						 // triangular shooting range
						 if( distance <= range*k*k )
						 {
//ShowMessage("  hit");
							bl_list[bl_list_count++]=bl;
						 }
					}
//ShowMessage("\n");
				}
			}//end for mobs
		}
	}//end for(bx,by)


	if(bl_list_count>=BL_LIST_MAX) {
		if(battle_config.error_log)
			ShowWarning("map_foreachinpath: *WARNING* block count too many!\n");
	}

	va_start(ap,type);
	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=blockcount;i<bl_list_count;i++)
		if(bl_list[i]->prev)	// �L?���ǂ����`�F�b�N
			func(bl_list[i],ap);

	map_freeblock_unlock();	// �����������
	va_end(ap);
	
	bl_list_count = blockcount;

*/
/*
//////////////////////////////////////////////////////////////
//
// sharp shooting 2
//
//////////////////////////////////////////////////////////////
// problem: 
// finding targets standing exactly on a line
// (only t1 and t2 get hit)
//
//     target 1
//      x t4
//     t2
// t3 x
//   x
//  S
//////////////////////////////////////////////////////////////
	va_list ap;
	int i, blockcount = bl_list_count;
	struct block_list *bl;
	int c1,c2;

	//////////////////////////////////////////////////////////////
	// linear parametric equation
	// x=(x1-x0)*t+x0; y=(y1-y0)*t+y0; t=[0,1]
	//////////////////////////////////////////////////////////////
	// linear equation for finding a single line between (x0,y0)->(x1,y1)
	// independent of the given xy-values
	double dx = 0.0;
	double dy = 0.0;
	int bx=-1;	// initialize block coords to some impossible value
	int by=-1;
	int t, tmax;

	// no map
	if(m < 0) return;

	// xy out of range
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= map[m].xs) x1 = map[m].xs-1;
	if (y1 >= map[m].ys) y1 = map[m].ys-1;

	///////////////////////////////
	// find maximum runindex
	tmax = abs(y1-y0);
	if(tmax  < abs(x1-x0))	
		tmax = abs(x1-x0);
	// pre-calculate delta values for x and y destination
	// should speed up cause you don't need to divide in the loop
	if(tmax>0)
	{
		dx = ((double)(x1-x0)) / ((double)tmax);
		dy = ((double)(y1-y0)) / ((double)tmax);
	}
	// go along the index
	for(t=0; t<=tmax; t++)
	{	// xy-values of the line including start and end point
		int x = (int)floor(dx * (double)t +0.5)+x0;
		int y = (int)floor(dy * (double)t +0.5)+y0;

		// check the block index of the calculated xy
		if( (bx!=x/BLOCK_SIZE) || (by!=y/BLOCK_SIZE) )
		{	// we have reached a new block
			// so we store the current block coordinates
			bx = x/BLOCK_SIZE;
			by = y/BLOCK_SIZE;

			// and process the data
			c1  = map[m].block_count[bx+by*map[m].bxs];		// number of elements in the block
			c2  = map[m].block_mob_count[bx+by*map[m].bxs];	// number of mobs in the mob block
			if( (c1==0) && (c2==0) ) continue;				// skip if nothing in the block

			if(type==0 || type!=BL_MOB) {
				bl = map[m].block[bx+by*map[m].bxs];		// a block with the elements
				for(i=0;i<c1 && bl;i++,bl=bl->next){		// go through all elements
					if( bl && ( !type || bl->type==type ) && bl_list_count<BL_LIST_MAX )
					{	
						// check if block xy is on the line
						if( abs((bl->x-x0)*(y1-y0) - (bl->y-y0)*(x1-x0)) <= tmax/2 )

						// and if it is within start and end point
						if( (((x0<=x1)&&(x0<=bl->x)&&(bl->x<=x1)) || ((x0>=x1)&&(x0>=bl->x)&&(bl->x>=x1))) &&
							(((y0<=y1)&&(y0<=bl->y)&&(bl->y<=y1)) || ((y0>=y1)&&(y0>=bl->y)&&(bl->y>=y1))) )
							bl_list[bl_list_count++]=bl;
					}
				}//end for elements
			}

			if(type==0 || type==BL_MOB) {
				bl = map[m].block_mob[bx+by*map[m].bxs];	// and the mob block
				for(i=0;i<c2 && bl;i++,bl=bl->next){
					if(bl && bl_list_count<BL_LIST_MAX) {
						// check if mob xy is on the line
						if( abs((bl->x-x0)*(y1-y0) - (bl->y-y0)*(x1-x0)) <= tmax/2 )

						// and if it is within start and end point
						if( (((x0<=x1)&&(x0<=bl->x)&&(bl->x<=x1)) || ((x0>=x1)&&(x0>=bl->x)&&(bl->x>=x1))) &&
							(((y0<=y1)&&(y0<=bl->y)&&(bl->y<=y1)) || ((y0>=y1)&&(y0>=bl->y)&&(bl->y>=y1))) )
							bl_list[bl_list_count++]=bl;
					}
				}//end for mobs
			}	
		}
	}//end for index

	if(bl_list_count>=BL_LIST_MAX) {
		if(battle_config.error_log)
			ShowWarning("map_foreachinpath: *WARNING* block count too many!\n");
	}

	va_start(ap,type);
	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=blockcount;i<bl_list_count;i++)
		if(bl_list[i]->prev)	// �L?���ǂ����`�F�b�N
			func(bl_list[i],ap);

	map_freeblock_unlock();	// �����������
	va_end(ap);
	
	bl_list_count = blockcount;
*/

//////////////////////////////////////////////////////////////
//
// sharp shooting 2 version 2
// mix between line calculation and point storage
//////////////////////////////////////////////////////////////
// problem: 
// finding targets standing exactly on a line
// (only t1 and t2 get hit)
//
//     target 1
//      x t4
//     t2
// t3 x
//   x
//  S
//////////////////////////////////////////////////////////////
	va_list ap;
	int i,k, blockcount = bl_list_count;
	struct block_list *bl;
	int c1,c2;

	//////////////////////////////////////////////////////////////
	// linear parametric equation
	// x=(x1-x0)*t+x0; y=(y1-y0)*t+y0; t=[0,1]
	//////////////////////////////////////////////////////////////
	// linear equation for finding a single line between (x0,y0)->(x1,y1)
	// independent of the given xy-values
	double dx = 0.0;
	double dy = 0.0;
	int bx=-1;	// initialize block coords to some impossible value
	int by=-1;
	int t, tmax, x,y;

	int save_x[BLOCK_SIZE],save_y[BLOCK_SIZE],save_cnt=0;

	// no map
	if(m < 0) return;

	// xy out of range
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= map[m].xs) x1 = map[m].xs-1;
	if (y1 >= map[m].ys) y1 = map[m].ys-1;

	///////////////////////////////
	// find maximum runindex, 
	if( abs(y1-y0) > abs(x1-x0) )
		tmax = abs(y1-y0);
	else
		tmax = abs(x1-x0);
	// pre-calculate delta values for x and y destination
	// should speed up cause you don't need to divide in the loop
	if(tmax>0)
	{
		dx = ((double)(x1-x0)) / ((double)tmax);
		dy = ((double)(y1-y0)) / ((double)tmax);
}
	// go along the index t from 0 to tmax
	t=0;
	do {	
		x = (int)floor(dx * (double)t +0.5)+x0;
		y = (int)floor(dy * (double)t +0.5)+y0;


		// check the block index of the calculated xy, or the last block
		if( (bx!=x/BLOCK_SIZE) || (by!=y/BLOCK_SIZE) || t>tmax)
		{	// we have reached a new block

			// and process the data of the formerly stored block, if any
			if( save_cnt!=0 )
			{
				c1  = map[m].block_count[bx+by*map[m].bxs];		// number of elements in the block
				c2  = map[m].block_mob_count[bx+by*map[m].bxs];	// number of mobs in the mob block
				if( (c1!=0) || (c2!=0) )						// skip if nothing in the block
				{

					if(type==0 || type!=BL_MOB) {
						bl = map[m].block[bx+by*map[m].bxs];		// a block with the elements
						for(i=0;i<c1 && bl;i++,bl=bl->next){		// go through all elements
							if( bl && ( !type || bl->type==type ) && bl_list_count<BL_LIST_MAX )
							{	// check if block xy is on the line
								for(k=0; k<save_cnt; k++)
								{
									if( (save_x[k]==bl->x)&&(save_y[k]==bl->y) )
									{
										bl_list[bl_list_count++]=bl;
										break;
									}
								}
							}
						}//end for elements
					}

					if(type==0 || type==BL_MOB) {
						bl = map[m].block_mob[bx+by*map[m].bxs];	// and the mob block
						for(i=0;i<c2 && bl;i++,bl=bl->next){
							if(bl && bl_list_count<BL_LIST_MAX) {
								// check if mob xy is on the line
								for(k=0; k<save_cnt; k++)
								{
									if( (save_x[k]==bl->x)&&(save_y[k]==bl->y) )
									{
										bl_list[bl_list_count++]=bl;
										break;
									}
								}
							}
						}//end for mobs
					}
				}
				// reset the point storage
				save_cnt=0;
			}

			// store the current block coordinates
			bx = x/BLOCK_SIZE;
			by = y/BLOCK_SIZE;
		}
		// store the new point of the line
		save_x[save_cnt]=x;
		save_y[save_cnt]=y;
		save_cnt++;
	}while( t++ <= tmax );

	if(bl_list_count>=BL_LIST_MAX) {
		if(battle_config.error_log)
			ShowWarning("map_foreachinpath: *WARNING* block count too many!\n");
	}

	va_start(ap,type);
	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=blockcount;i<bl_list_count;i++)
		if(bl_list[i]->prev)	// �L?���ǂ����`�F�b�N
			func(bl_list[i],ap);

	map_freeblock_unlock();	// �����������
	va_end(ap);
	
	bl_list_count = blockcount;

}


/*==========================================
 * ���A�C�e����G�t�F�N�g�p�̈�Obj����?��
 * object[]�ւ̕ۑ���id_db�o?�܂�
 *
 * bl->id�����̒��Őݒ肵�Ė�薳��?
 *------------------------------------------
 */
int map_addobject(struct block_list *bl) {
	int i;
	if( bl == NULL ){
		ShowMessage("map_addobject nullpo?\n");
		return 0;
	}
	if(first_free_object_id<2 || first_free_object_id>=MAX_FLOORITEM)
		first_free_object_id=2;
	for(i=first_free_object_id;i<MAX_FLOORITEM;i++)
		if(objects[i]==NULL)
			break;
	if(i>=MAX_FLOORITEM){
		if(battle_config.error_log)
			ShowMessage("no free object id\n");
		return 0;
	}
	first_free_object_id=i;
	if(last_object_id<i)
		last_object_id=i;
	objects[i]=bl;
	numdb_insert(id_db,i,bl);
	return i;
}

/*==========================================
 * ��Object�̉��
 *	map_delobject��aFree���Ȃ��o?�W����
 *------------------------------------------
 */
int map_delobjectnofree(int id) {
	if(objects[id]==NULL)
		return 0;

	map_delblock(objects[id]);
	numdb_erase(id_db,id);
	objects[id]=NULL;

	if(first_free_object_id>id)
		first_free_object_id=id;

	while(last_object_id>2 && objects[last_object_id]==NULL)
		last_object_id--;

	return 0;
}

/*==========================================
 * ��Object�̉��
 * block_list����̍폜�Aid_db����̍폜
 * object data��aFree�Aobject[]�ւ�NULL���
 *
 * add�Ƃ�??���������̂�?�ɂȂ�
 *------------------------------------------
 */
int map_delobject(int id) {
	struct block_list *obj = objects[id];

	if(obj==NULL)
		return 0;

	map_delobjectnofree(id);
	map_freeblock(obj);

	return 0;
}

/*==========================================
 * �S��Obj�����func���Ă�
 *
 *------------------------------------------
 */
void map_foreachobject(int (*func)(struct block_list*,va_list),int type,...) {
	int i;
	int blockcount=bl_list_count;
	va_list ap;

	va_start(ap,type);

	for(i=2;i<=last_object_id;i++){
		if(objects[i]){
			if(type && objects[i]->type!=type)
				continue;
			if(bl_list_count>=BL_LIST_MAX) {
				if(battle_config.error_log)
					ShowMessage("map_foreachobject: too many block !\n");
			}
			else
				bl_list[bl_list_count++]=objects[i];
		}
	}

	map_freeblock_lock();

	for(i=blockcount;i<bl_list_count;i++)
		if( bl_list[i]->prev || bl_list[i]->next )
			func(bl_list[i],ap);

	map_freeblock_unlock();

	va_end(ap);
	bl_list_count = blockcount;
}

/*==========================================
 * ���A�C�e��������
 *
 * data==0�̎b�timer�ŏ������� * data!=0�̎b͏E�����ŏ������bƂ��ē���
 *
 * ��҂́Amap_clearflooritem(id)��
 * map.h?��#define���Ă���
 *------------------------------------------
 */
int map_clearflooritem_timer(int tid,unsigned long tick,int id,int data) {
	struct flooritem_data *fitem=NULL;

	fitem = (struct flooritem_data *)objects[id];
	if(fitem==NULL || fitem->bl.type!=BL_ITEM || (!data && fitem->cleartimer != tid)){
		if(battle_config.error_log)
			ShowMessage("map_clearflooritem_timer : error\n");
		return 1;
	}
	if(data)
		delete_timer(fitem->cleartimer,map_clearflooritem_timer);
	else if(fitem->item_data.card[0] == 0xff00)
		intif_delete_petdata( MakeDWord(fitem->item_data.card[1],fitem->item_data.card[2]) );
	clif_clearflooritem(fitem);
	map_delobject(fitem->bl.id);

	return 0;
}

/*==========================================
 * (m,x,y)�̎�?range�}�X?�̋�(=�N���\)cell��
 * ?����K?�ȃ}�X�ڂ̍��W��x+(y<<16)�ŕԂ�
 *
 * ��?range=1�ŃA�C�e���h���b�v�p�r�̂�
 *------------------------------------------
 */
int map_searchrandfreecell(int m,int x,int y,int range) {
	int free_cell,i,j;

	for(free_cell=0,i=-range;i<=range;i++){
		if(i+y<0 || i+y>=map[m].ys)
			continue;
		for(j=-range;j<=range;j++){
			if(j+x<0 || j+x>=map[m].xs)
				continue;
			if(map_getcell(m,j+x,i+y,CELL_CHKNOPASS))
				continue;
			free_cell++;
		}
	}
	if(free_cell==0)
		return -1;
	free_cell=rand()%free_cell;
	for(i=-range;i<=range;i++){
		if(i+y<0 || i+y>=map[m].ys)
			continue;
		for(j=-range;j<=range;j++){
			if(j+x<0 || j+x>=map[m].xs)
				continue;
			if(map_getcell(m,j+x,i+y,CELL_CHKNOPASS))
				continue;
			if(free_cell==0){
				x+=j;
				y+=i;
				i=range+1;
				break;
			}
			free_cell--;
		}
	}

	return x+(y<<16);
}

/*==========================================
 * (m,x,y)�𒆐S��3x3��?�ɏ��A�C�e���ݒu
 *
 * item_data��amount�ȊO��copy����
 *------------------------------------------
 */
int map_addflooritem(struct item *item_data,int amount,int m,int x,int y,struct map_session_data *first_sd,
    struct map_session_data *second_sd,struct map_session_data *third_sd,int type) {
	int xy,r;
	unsigned long tick;
	struct flooritem_data *fitem=NULL;

	nullpo_retr(0, item_data);

	if((xy=map_searchrandfreecell(m,x,y,1))<0)
		return 0;
	r=rand();

	fitem = (struct flooritem_data *)aCalloc(1,sizeof(*fitem));
	fitem->bl.type=BL_ITEM;
	fitem->bl.prev = fitem->bl.next = NULL;
	fitem->bl.m=m;
	fitem->bl.x=xy&0xffff;
	fitem->bl.y=(xy>>16)&0xffff;
	fitem->first_get_id = 0;
	fitem->first_get_tick = 0;
	fitem->second_get_id = 0;
	fitem->second_get_tick = 0;
	fitem->third_get_id = 0;
	fitem->third_get_tick = 0;

	fitem->bl.id = map_addobject(&fitem->bl);
	if(fitem->bl.id==0){
		aFree(fitem);
		return 0;
	}

	tick = gettick();
	if(first_sd) {
		fitem->first_get_id = first_sd->bl.id;
		if(type)
			fitem->first_get_tick = tick + battle_config.mvp_item_first_get_time;
		else
			fitem->first_get_tick = tick + battle_config.item_first_get_time;
	}
	if(second_sd) {
		fitem->second_get_id = second_sd->bl.id;
		if(type)
			fitem->second_get_tick = tick + battle_config.mvp_item_first_get_time + battle_config.mvp_item_second_get_time;
		else
			fitem->second_get_tick = tick + battle_config.item_first_get_time + battle_config.item_second_get_time;
	}
	if(third_sd) {
		fitem->third_get_id = third_sd->bl.id;
		if(type)
			fitem->third_get_tick = tick + battle_config.mvp_item_first_get_time + battle_config.mvp_item_second_get_time + battle_config.mvp_item_third_get_time;
		else
			fitem->third_get_tick = tick + battle_config.item_first_get_time + battle_config.item_second_get_time + battle_config.item_third_get_time;
	}

	memcpy(&fitem->item_data,item_data,sizeof(*item_data));
	fitem->item_data.amount=amount;
	fitem->subx=(r&3)*3+3;
	fitem->suby=((r>>2)&3)*3+3;
	fitem->cleartimer=add_timer(gettick()+battle_config.flooritem_lifetime,map_clearflooritem_timer,fitem->bl.id,0);

	map_addblock(&fitem->bl);
	clif_dropflooritem(fitem);

	return fitem->bl.id;
}

/*==========================================
 * charid_db�֒ǉ�(�ԐM�҂�������ΕԐM)
 *------------------------------------------
 */
void map_addchariddb(unsigned long charid, char *name) {
	struct charid2nick *p=NULL;
	int req=0;

	p = (struct charid2nick*)numdb_search(charid_db,charid);
	if(p==NULL){	// �f?�^�x?�X�ɂȂ�
		p = (struct charid2nick *)aCallocA(1,sizeof(struct charid2nick));
		p->req_id=0;
	}else
		numdb_erase(charid_db,charid);

	req=p->req_id;
	memcpy(p->nick,name,24);
	p->req_id=0;
	numdb_insert(charid_db,charid,p);
	if(req){	// �ԐM�҂�������ΕԐM
		struct map_session_data *sd = map_id2sd(req);
		if(sd!=NULL)
			clif_solved_charname(sd,charid);
	}
}

/*==========================================
 * charid_db�֒ǉ��i�ԐM�v���̂݁j
 *------------------------------------------
 */
int map_reqchariddb(struct map_session_data * sd, unsigned long charid) 
{
	struct charid2nick *p=NULL;
	nullpo_retr(0, sd);

	p = (struct charid2nick*)numdb_search(charid_db,charid);
	if(p!=NULL)	// �f?�^�x?�X�ɂ��łɂ���
		return 0;
	p = (struct charid2nick *)aCalloc(1,sizeof(struct charid2nick));
	p->req_id=sd->bl.id;
	numdb_insert(charid_db,charid,p);
	return 0;
}

/*==========================================
 * id_db��bl��ǉ�
 *------------------------------------------
 */
void map_addiddb(struct block_list *bl) 
{
	nullpo_retv(bl);
	numdb_insert(id_db,bl->id,bl);
}

/*==========================================
 * id_db����bl���폜
 *------------------------------------------
 */
void map_deliddb(struct block_list *bl) 
{
	nullpo_retv(bl);
	numdb_erase(id_db,bl->id);
}

/*==========================================
 * nick_db��sd��ǉ�
 *------------------------------------------
 */
void map_addnickdb(struct map_session_data *sd) 
{
	nullpo_retv(sd);
	strdb_insert(nick_db,sd->status.name,sd);
}

/*==========================================
 * PC��quit?�� map.c?��
 *
 * quit?���̎�?���Ⴄ�悤��?�����Ă���
 *------------------------------------------
 */
int map_quit(struct map_session_data *sd) 
{
	
	nullpo_retr(0, sd);

	if (sd->state.event_disconnect) 
	{
		if (script_config.event_script_type == 0) 
		{
			struct npc_data *npc = npc_name2id(script_config.logout_event_name);
			if(npc && npc->u.scr.ref) 
			{
				run_script(npc->u.scr.ref->script,0,sd->bl.id,npc->bl.id); // PCLogoutNPC
				ShowStatus ("Event '"CL_WHITE"%s"CL_RESET"' executed.\n", script_config.logout_event_name);
				}
		}
		else 
		{
			ShowStatus("%d '"CL_WHITE"%s"CL_RESET"' events executed.\n",
					npc_event_doall_id(script_config.logout_event_name, sd->bl.id), script_config.logout_event_name);
			}

		if(sd->chatID)	// �`���b�g����o��
			chat_leavechat(sd);

		if(sd->trade_partner)	// �����?����
			trade_tradecancel(sd);

		if(sd->party_invite>0)	// �p?�e�B?�U�����ۂ���
			party_reply_invite(sd,sd->party_invite_account,0);

		if(sd->guild_invite>0)	// �M���h?�U�����ۂ���
			guild_reply_invite(sd,sd->guild_invite,0);
		if(sd->guild_alliance>0)	// �M���h����?�U�����ۂ���
			guild_reply_reqalliance(sd,sd->guild_alliance_account,0);

		party_send_logout(sd);	// �p?�e�B�̃��O�A�E�g���b�Z?�W���M

		guild_send_memberinfoshort(sd,0);	// �M���h�̃��O�A�E�g���b�Z?�W���M

		pc_cleareventtimer(sd);	// �C�x���g�^�C�}��j������

		if(sd->state.storage_flag)
			storage_guild_storage_quit(sd,0);
		else
			storage_storage_quit(sd);	// �q�ɂ��J���Ă�Ȃ�ۑ�����

		// check if we've been authenticated [celest]
		if (sd->state.auth)
			skill_castcancel(&sd->bl,0);	// �r����?����

		skill_stop_dancing(&sd->bl,1);// �_���X/���t��?

		if(sd->sc_data && sd->sc_data[SC_BERSERK].timer!=-1) //�o?�T?�N���̏I����HP��100��
			sd->status.hp = 100;

		status_change_clear(&sd->bl,1);	// �X�e?�^�X�ُ����������
		skill_clear_unitgroup(&sd->bl);	// �X�L�����j�b�g�O��?�v�̍폜
		skill_cleartimerskill(&sd->bl);

//		// check if we've been authenticated [celest]
//		if (sd->state.auth) {
			pc_stop_walking(sd,0);
			pc_stopattack(sd);
			pc_delinvincibletimer(sd);
//		}
		pc_delspiritball(sd,sd->spiritball,1);
		skill_gangsterparadise(sd,0);
		skill_unit_move(&sd->bl,gettick(),0);

		if (sd->state.auth)
			status_calc_pc(sd,4);

		if (!(sd->status.option & OPTION_HIDE))
			clif_clearchar_area(&sd->bl,2);

		if(sd->status.pet_id && sd->pd) {
			pet_lootitem_drop(sd->pd,sd);
			pet_remove_map(sd);
			if(sd->pet.intimate <= 0) {
				intif_delete_petdata(sd->status.pet_id);
				sd->status.pet_id = 0;
				sd->pd = NULL;
				sd->petDB = NULL;
			}
			else
				intif_save_petdata(sd->status.account_id,&sd->pet);
		}
		if(pc_isdead(sd))
			pc_setrestartvalue(sd,2);

		pc_clean_skilltree(sd);
		pc_makesavestatus(sd);
		chrif_save(sd);
		storage_storage_dirty(sd);
		storage_storage_save(sd);
		map_delblock(&sd->bl);
	}


	if( sd->npc_stackbuf != NULL) {
		aFree( sd->npc_stackbuf );
		sd->npc_stackbuf = NULL;
	}

	chrif_char_offline(sd);

	{
		struct charid2nick *p = (struct charid2nick *)numdb_search(charid_db,sd->status.char_id);
		if(p) {
			numdb_erase(charid_db,sd->status.char_id);
			aFree(p);
		}
	}

	strdb_erase(nick_db,sd->status.name);
	numdb_erase(charid_db,sd->status.char_id);
	numdb_erase(id_db,sd->bl.id);
	aFree(sd->reg);
	aFree(sd->regstr);

	// numdb_erase(charid_db,sd->status.char_id);
	{
		void *p = numdb_search(charid_db,sd->status.char_id);
		if(p) {
			numdb_erase(charid_db,sd->status.char_id);
			aFree(p);
		}
	} 

	return 0;
}

/*==========================================
 * id��?��PC��T���B���Ȃ����NULL
 *------------------------------------------
 */
struct map_session_data * map_id2sd(unsigned long id) {
// remove search from db, because:
// 1 - all players, npc, items and mob are in this db (to search, it's not speed, and search in session is more sure)
// 2 - DB seems not always correct. Sometimes, when a player disconnects, its id (account value) is not removed and structure
//     point to a memory area that is not more a session_data and value are incorrect (or out of available memory) -> crash
// replaced by searching in all session.
// by searching in session, we are sure that fd, session, and account exist.
/*
	struct block_list *bl;

	bl=numdb_search(id_db,id);
	if(bl && bl->type==BL_PC)
		return (struct map_session_data*)bl;
	return NULL;
*/
	size_t i;
	struct map_session_data *sd;

	if(id) // skip if zero id's are searched
	for(i = 0; i < fd_max; i++)
		if (session[i] && (sd = (struct map_session_data*)session[i]->session_data) && sd->bl.id == id)
			return sd;
	return NULL;
}

/*==========================================
 * char_id��?�̖��O��T��
 *------------------------------------------
 */
char * map_charid2nick(int id) {
	struct charid2nick *p = (struct charid2nick*)numdb_search(charid_db,id);

	if(p==NULL)
		return NULL;
	if(p->req_id!=0)
		return NULL;
	return p->nick;
}

struct map_session_data * map_charid2sd(unsigned long id) {
	size_t i;
	struct map_session_data *sd;

	if (id <= 0) return 0;

	for(i = 0; i < fd_max; i++)
		if (session[i] && (sd = (struct map_session_data*)session[i]->session_data) && sd->status.char_id == id)
			return sd;

	return NULL;
}

/*==========================================
 * Search session data from a nick name
 * (without sensitive case if necessary)
 * return map_session_data pointer or NULL
 *------------------------------------------
 */
struct map_session_data * map_nick2sd(char *nick) {
	size_t i;
	int quantity=0, nicklen;
	struct map_session_data *sd = NULL;
	struct map_session_data *pl_sd = NULL;

	if (nick == NULL)
		return NULL;

    nicklen = strlen(nick);

	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = (struct map_session_data*)session[i]->session_data) && pl_sd->state.auth)
			// Without case sensitive check (increase the number of similar character names found)
			if (strncasecmp(pl_sd->status.name, nick, nicklen) == 0) {
				// Strict comparison (if found, we finish the function immediatly with correct value)
				if (strcmp(pl_sd->status.name, nick) == 0)
					return pl_sd;
				quantity++;
				sd = pl_sd;
			}
	}
	// Here, the exact character name is not found
	// We return the found index of a similar account ONLY if there is 1 similar character
	if (quantity == 1)
		return sd;

	// Exact character name is not found and 0 or more than 1 similar characters have been found ==> we say not found
	return NULL;
}

/*==========================================
 * id��?�̕���T��
 * ��Object�̏ꍇ�͔z��������̂�
 *------------------------------------------
 */
struct block_list * map_id2bl(unsigned long id)
{
	struct block_list *bl=NULL;
	if((size_t)id<sizeof(objects)/sizeof(objects[0]))
		bl = objects[id];
	else
		bl = (struct block_list*)numdb_search(id_db,id);

	return bl;
}

/*==========================================
 * id_db?�̑S�Ă�func��?�s
 *------------------------------------------
 */
int map_foreachiddb(int (*func)(void*,void*,va_list),...) {
	va_list ap;

	va_start(ap,func);
	numdb_foreach(id_db,func,ap);
	va_end(ap);
	return 0;
}

/*==========================================
 * map.npc�֒ǉ� (warp���̗̈掝���̂�)
 *------------------------------------------
 */
int map_addnpc(size_t m, struct npc_data *nd)
{
	size_t i;
	if(m<0 || m>=map_num)
		return -1;
	for(i=0;i<map[m].npc_num && i<MAX_NPC_PER_MAP;i++)
		if(map[m].npc[i]==NULL)
			break;
	if(i==MAX_NPC_PER_MAP){
		if(battle_config.error_log)
			ShowMessage("too many NPCs in one map %s\n",map[m].mapname);
		return -1;
	}
	if(i==map[m].npc_num){
		map[m].npc_num++;
	}

	nullpo_retr(0, nd);

	map[m].npc[i]=nd;
	nd->n = i;
	numdb_insert(id_db,nd->bl.id,nd);

	return i;
}

void map_removenpc(void)
{
	size_t i, m,n=0;

	for(m=0;m<map_num;m++) {
		for(i=0;i<map[m].npc_num && i<MAX_NPC_PER_MAP;i++) {
			if(map[m].npc[i]!=NULL) {
				clif_clearchar_area(&map[m].npc[i]->bl,2);
				map_delblock(&map[m].npc[i]->bl);
				numdb_erase(id_db,map[m].npc[i]->bl.id);
//				if(map[m].npc[i]->bl.subtype==SCRIPT) {
//					aFree(map[m].npc[i]->u.scr.script);
//					aFree(map[m].npc[i]->u.scr.label_list);
//				}
//    just unlink npc from map
//    npc will be deleted with do_final_npc
//				aFree(map[m].npc[i]);
				map[m].npc[i] = NULL;
				n++;
			}
		}
	}
	ShowStatus("Successfully removed and freed from memory '"CL_WHITE"%d"CL_RESET"' NPCs.\n",n);
}

/*==========================================
 * map������map��?��?��
 *------------------------------------------
 */
int map_mapname2mapid(char *name) {
	struct map_data *md=NULL;

	md = (struct map_data*)strdb_search(map_db,name);

#ifdef USE_AFM
	// If we can't find the .gat map try .afm instead [celest]
		if( (md==NULL) && (NULL!=strstr(name,".gat")) ) {
			char afm_name[50];
			memcpy(afm_name, name, strlen(name) - 3);	// copy without extension including the point
			memcpy(afm_name+strlen(name) - 3, "afm",4);	// add the extension including the eos
	  md = (struct map_data*)strdb_search(map_db,afm_name);
	}
#endif

	if(md==NULL || md->gat==NULL)
		return -1;
	return md->m;
}

/*==========================================
 * ���Imap������ip,port?��
 *------------------------------------------
 */
int map_mapname2ipport(char *name, unsigned long *ip, unsigned short *port) {
	struct map_data_other_server *mdos=NULL;

	mdos = (struct map_data_other_server*)strdb_search(map_db,name);
	if(mdos==NULL || mdos->gat)
		return -1;
	if(ip)	 *ip  =mdos->ip;
	if(port) *port=mdos->port;
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_check_dir(int s_dir,int t_dir) {
	if(s_dir == t_dir)
		return 0;
	switch(s_dir) {
		case 0:
			if(t_dir == 7 || t_dir == 1 || t_dir == 0)
				return 0;
			break;
		case 1:
			if(t_dir == 0 || t_dir == 2 || t_dir == 1)
				return 0;
			break;
		case 2:
			if(t_dir == 1 || t_dir == 3 || t_dir == 2)
				return 0;
			break;
		case 3:
			if(t_dir == 2 || t_dir == 4 || t_dir == 3)
				return 0;
			break;
		case 4:
			if(t_dir == 3 || t_dir == 5 || t_dir == 4)
				return 0;
			break;
		case 5:
			if(t_dir == 4 || t_dir == 6 || t_dir == 5)
				return 0;
			break;
		case 6:
			if(t_dir == 5 || t_dir == 7 || t_dir == 6)
				return 0;
			break;
		case 7:
			if(t_dir == 6 || t_dir == 0 || t_dir == 7)
				return 0;
			break;
	}
	return 1;
}

/*==========================================
 * �މ�̕������v�Z
 *------------------------------------------
 */
int map_calc_dir( struct block_list *src,int x,int y) {
	int dir=0;
	int dx,dy;

	nullpo_retr(0, src);

	dx=x-src->x;
	dy=y-src->y;
	if( dx==0 && dy==0 ){	// �މ�̏ꏊ��v
		dir=0;	// ��
	}else if( dx>=0 && dy>=0 ){	// �����I�ɉE��
		dir=7;						// �E��
		if( dx*3-1<dy ) dir=0;		// ��
		if( dx>dy*3 ) dir=6;		// �E
	}else if( dx>=0 && dy<=0 ){	// �����I�ɉE��
		dir=5;						// �E��
		if( dx*3-1<-dy ) dir=4;		// ��
		if( dx>-dy*3 ) dir=6;		// �E
	}else if( dx<=0 && dy<=0 ){ // �����I�ɍ���
		dir=3;						// ����
		if( dx*3+1>dy ) dir=4;		// ��
		if( dx<dy*3 ) dir=2;		// ��
	}else{						// �����I�ɍ���
		dir=1;						// ����
		if( -dx*3-1<dy ) dir=0;		// ��
		if( -dx>dy*3 ) dir=2;		// ��
	}
	return dir;
}

// gat�n
/*==========================================
 * (m,x,y)�̏�Ԃ𒲂ׂ�
 *------------------------------------------
 */
/////////////////////////////////////////////////////////////////////
// as far as I have seen the gat.type values can contain
// GAT_NONE		= 0,
// GAT_WALL		= 1,
// GAT_UNUSED1	= 2,
// GAT_WATER	= 3,
// GAT_UNUSED2	= 4,
// GAT_GROUND	= 5,
// GAT_HOLE		= 6,	// holes in morroc desert
// GAT_UNUSED3	= 7,
// change the gat to a bitfield with tree bits 
// instead of using an unsigned char have it merged with other usages
/////////////////////////////////////////////////////////////////////

int map_getcell(unsigned short m,unsigned short x, unsigned short y,cell_t cellchk)
{
	return (m >= MAX_MAP_PER_SERVER) ? 0 : map_getcellp(&map[m],x,y,cellchk);
}

int map_getcellp(struct map_data* m,unsigned short x, unsigned short y,cell_t cellchk)
{
	struct mapgat *mg;
	nullpo_ret(m);

	if(x>=m->xs || y>=m->ys)
	{
		if(cellchk==CELL_CHKNOPASS) return 1;
		return 0;
	}
	mg = m->gat+x+y*m->xs;

	switch(cellchk)
	{
		case CELL_CHKPASS:
			return (mg->type != GAT_WALL && mg->type != GAT_GROUND);
		case CELL_CHKNOPASS:
			return (mg->type == GAT_WALL || mg->type == GAT_GROUND);
		case CELL_CHKWALL:
			return (mg->type == GAT_WALL);
		case CELL_CHKWATER:
			return (mg->type == GAT_WATER);
		case CELL_CHKGROUND:
			return (mg->type == GAT_GROUND);
		case CELL_CHKHOLE:
			return (mg->type == GAT_HOLE);
		case CELL_GETTYPE:
			return mg->type;
		case CELL_CHKNPC:
			return mg->npc;
		case CELL_CHKBASILICA:
			return mg->basilica;
		case CELL_CHKMOONLIT:
			return mg->moonlit;
		case CELL_CHKREGEN:
			return mg->regen;
		default:
			return 0;
	}
}
/*==========================================
 * (m,x,y)�̏�Ԃ�ݒ肷��
 *------------------------------------------
 */
void map_setcell(unsigned short m,unsigned short x, unsigned short y, int cellck)
{
	struct mapgat *mg;
	if(m >= MAX_MAP_PER_SERVER || x>=map[m].xs || y>=map[m].ys)
		return;

	mg = map[m].gat+x+y*map[m].xs;

	switch(cellck)
	{
		case CELL_SETNPC:
		if(mg->npc < 15) // max for a 4bit counter
			mg->npc++;
		else
			ShowWarning("usage of more then 15 stacked npc touchup areas\n");
			break;
	case CELL_CLRNPC:
		if(mg->npc > 0) // 4bit counter
			mg->npc--;
		//else no warning, has been warned at setting up the touchups already
		break;
		case CELL_SETBASILICA:
		mg->basilica = 1;
			break;
		case CELL_CLRBASILICA:
		mg->basilica = 0;
			break;
	case CELL_SETMOONLIT:
		mg->moonlit = 1;
		break;
	case CELL_CLRMOONLIT:
		mg->moonlit = 0;
		break;
	case CELL_SETREGEN:
		mg->regen = 1;
		break;
	case CELL_CLRREGEN:
		mg->regen = 0;
		break;
		default:
		// check the numbers from the gat and warn on an unknown type
		if( (cellck != GAT_NONE) && (cellck != GAT_WALL) && (cellck != GAT_WATER) && 
			(cellck != GAT_GROUND) && (cellck != GAT_HOLE) )
			ShowWarning("Setting mapcell with improper value %i on %s (%i,%i)\n", cellck,map[m].mapname,x,y);
		else
			mg->type = cellck & CELL_MASK;
			break;
	};
	}

/*==========================================
 * ���I�Ǘ��̃}�b�v��db�ɒǉ�
 *------------------------------------------
 */
int map_setipport(char *name, unsigned long ip, unsigned short port) {
	struct map_data *md=NULL;
	struct map_data_other_server *mdos=NULL;

	md = (struct map_data*)strdb_search(map_db,name);
	if(md==NULL){ // not exist -> add new data
		mdos=(struct map_data_other_server *)aCalloc(1,sizeof(struct map_data_other_server));
		memcpy(mdos->name,name,24);
		mdos->gat  = NULL;
		mdos->ip   = ip;
		mdos->port = port;
		mdos->map  = NULL;
		strdb_insert(map_db,mdos->name,mdos);
	} else if(md->gat){
		if(ip!=clif_getip() || port!=clif_getport()){
			// �ǂݍb�ł������ǁA�S���O�ɂȂ����}�b�v
			mdos=(struct map_data_other_server *)aCalloc(1,sizeof(struct map_data_other_server));
			memcpy(mdos->name,name,24);
			mdos->gat  = NULL;
			mdos->ip   = ip;
			mdos->port = port;
			mdos->map  = md;
			strdb_insert(map_db,mdos->name,mdos);
			// ShowMessage("from char server : %s -> %08lx:%d\n",name,ip,port);
		} else {
			// �ǂݍb�ł��āA�S���ɂȂ����}�b�v�i�������Ȃ��j
			;
		}
	} else {
		mdos=(struct map_data_other_server *)md;
		if(ip == clif_getip() && port == clif_getport()) {
			// �����̒S���ɂȂ����}�b�v
			if(mdos->map == NULL) {
				// �ǂݍb�ł��Ȃ��̂ŏI������
				ShowMessage("map_setipport : %s is not loaded.\n",name);
				exit(1);
			} else {
				// �ǂݍb�ł���̂Œu��������
				md = mdos->map;
				aFree(mdos);
				strdb_insert(map_db,md->mapname,md);
			}
		} else {
			// ���̎I�̒S���}�b�v�Ȃ̂Œu�������邾��
			mdos->ip   = ip;
			mdos->port = port;
		}
	}
	return 0;
}

/*==========================================
 * ���I�Ǘ��̃}�b�v��S�č폜
 *------------------------------------------
 */
int map_eraseallipport_sub(void *key,void *data,va_list va)
{
	struct map_data_other_server *mdos = (struct map_data_other_server*)data;
	if(mdos->gat == NULL && mdos->map == NULL) {
		strdb_erase(map_db,key);
		aFree(mdos);
	}
	return 0;
}

int map_eraseallipport(void)
{
	strdb_foreach(map_db,map_eraseallipport_sub);
	return 1;
}

/*==========================================
 * ���I�Ǘ��̃}�b�v��db����폜
 *------------------------------------------
 */
int map_eraseipport(char *name,unsigned long ip,int port)
{
	struct map_data *md;
	struct map_data_other_server *mdos;

	md=(struct map_data *) strdb_search(map_db,name);
	if(md){
		if(md->gat) // local -> check data
			return 0;
		else {
			mdos=(struct map_data_other_server *)md;
			if(mdos->ip==ip && mdos->port == port) {
				if(mdos->map) {
					// ���̃}�b�v�I�ł��ǂݍb�ł���̂ňړ��ł���
					return 1; // �Ăяo������ chrif_sendmap() ������
				} else {
					strdb_erase(map_db,name);
					aFree(mdos);
				}
				if(battle_config.etc_log)
					ShowStatus("erase map %s %d.%d.%d.%d:%d\n",name,(ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,(ip)&0xFF,port);
			}
		}
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////////
// ����������
/*==========================================
 * ���ꍂ���ݒ�
 *------------------------------------------
 */
static struct s_waterlist{
	char mapname[24];
	int waterheight;
} *waterlist=NULL;

#define NO_WATER 1000000

int map_waterheight(char *mapname) {
	if(waterlist){
		int i;
		for(i=0;waterlist[i].mapname[0] && i < MAX_MAP_PER_SERVER;i++)
			if(strcmp(waterlist[i].mapname,mapname)==0)
				return waterlist[i].waterheight;
	}
	return NO_WATER;
}

void map_readwater(const char *watertxt) {
	char line[1024],w1[1024];
	FILE *fp=NULL;
	int n=0;

	fp=savefopen(watertxt,"r");
	if(fp==NULL){
		ShowMessage("file not found: %s\n",watertxt);
		return;
	}
	if(waterlist==NULL)
		waterlist=(struct s_waterlist*)aCalloc(MAX_MAP_PER_SERVER,sizeof(struct s_waterlist));
	while(fgets(line,1020,fp) && n < MAX_MAP_PER_SERVER){
		int wh,count;
		if(line[0] == '/' && line[1] == '/')
			continue;
		if((count=sscanf(line,"%s%d",w1,&wh)) < 1){
			continue;
		}
		strcpy(waterlist[n].mapname,w1);
		if(count >= 2)
			waterlist[n].waterheight = wh;
		else
			waterlist[n].waterheight = 3;
		n++;
	}
	fclose(fp);
}
/*==========================================
* �}�b�v�L���b�V���ɒǉ�����
*===========================================*/
///////////////////////////////////////////////////////////////////////////////
// �}�b�v�L���b�V���̍ő�l
#define MAX_MAP_CACHE 768

//�e�}�b�v���Ƃ̍ŏ�������������́AREAD_FROM_BITMAP�p
struct map_cache_info {
	char fn[24];			//�t�@�C����
	unsigned short xs;
	unsigned short ys;		//���ƍ���
	int water_height;
	size_t pos;				// �f�[�^������Ă���ꏊ
	int compressed;     // zilb�ʂ���悤�ɂ���ׂ̗\��
	size_t datalen;
}; // 40 byte

struct map_cache_head {
	size_t sizeof_header;
	size_t sizeof_map;
    // ��̂Q���ϕs��
	size_t nmaps;			// �}�b�v�̌�
	long filesize;
};

struct map_cache {
        struct map_cache_head head;
	struct map_cache_info *map;
	FILE *fp;
	int dirty;
};

struct map_cache map_cache;

int map_cache_open(char *fn);
void map_cache_close(void);
int map_cache_read(struct map_data *m);
int map_cache_write(struct map_data *m);

int map_cache_open(char *fn)
{
	if(map_cache.fp)
		map_cache_close();

	map_cache.fp = savefopen(fn,"r+b");
	if(map_cache.fp) {
		fread(&map_cache.head,1,sizeof(struct map_cache_head),map_cache.fp);
		fseek(map_cache.fp,0,SEEK_END);
		if(
			map_cache.head.sizeof_header == sizeof(struct map_cache_head) &&
			map_cache.head.sizeof_map    == sizeof(struct map_cache_info) &&
			map_cache.head.nmaps         == MAX_MAP_CACHE &&
			map_cache.head.filesize      == ftell(map_cache.fp) )
		{
			// �L���b�V���ǂݍbݐ���
			map_cache.map = (struct map_cache_info *)aCalloc(map_cache.head.nmaps,sizeof(struct map_cache_info));
			fseek(map_cache.fp,sizeof(struct map_cache_head),SEEK_SET);
			fread(map_cache.map,sizeof(struct map_cache_info),map_cache.head.nmaps,map_cache.fp);
			return 1;
		}
		fclose(map_cache.fp);
	}
	// �ǂݍb݂Ɏ��s�����̂ŐV�K�ɍ쐬����
	map_cache.fp = savefopen(fn,"wb");
	if(map_cache.fp) {
		memset(&map_cache.head,0,sizeof(struct map_cache_head));
		map_cache.map   = (struct map_cache_info *) aCalloc(MAX_MAP_CACHE,sizeof(struct map_cache_info));
		map_cache.head.nmaps         = MAX_MAP_CACHE;
		map_cache.head.sizeof_header = sizeof(struct map_cache_head);
		map_cache.head.sizeof_map    = sizeof(struct map_cache_info);

		map_cache.head.filesize  = sizeof(struct map_cache_head);
		map_cache.head.filesize += sizeof(struct map_cache_info) * map_cache.head.nmaps;

		map_cache.dirty = 1;
		return 1;
	}
	return 0;
}

void map_cache_close(void)
{
	if(!map_cache.fp) { return; }
	if(map_cache.dirty) {
		fseek(map_cache.fp,0,SEEK_SET);
		fwrite(&map_cache.head,1,sizeof(struct map_cache_head),map_cache.fp);
		fwrite(map_cache.map,map_cache.head.nmaps,sizeof(struct map_cache_info),map_cache.fp);
	}
	fclose(map_cache.fp);
	map_cache.fp = NULL;
	aFree(map_cache.map);
	map_cache.map=NULL;
	return;
}

int map_cache_read(struct map_data *m)
{
	size_t i;
	if(!map_cache.fp) { return 0; }
	for(i = 0;i < map_cache.head.nmaps ; i++) {
		if(0==strcmp(m->mapname,map_cache.map[i].fn)) {
			break;
		}
	}

	if( i < map_cache.head.nmaps &&
		map_cache.map[i].water_height == map_waterheight(m->mapname) )		// ����̍������Ⴄ�̂œǂݒ���
	{
		if(map_cache.map[i].compressed == 0) {
				// �񈳏k�t�@�C��
				m->xs = map_cache.map[i].xs;
				m->ys = map_cache.map[i].ys;
			m->gat = (struct mapgat *)aMalloc( map_cache.map[i].datalen );
				fseek(map_cache.fp,map_cache.map[i].pos,SEEK_SET);
			if(fread(m->gat, 1, map_cache.map[i].datalen, map_cache.fp) != map_cache.map[i].datalen) {
				// �Ȃ����t�@�C���㔼�������Ă�̂œǂݒ���
				aFree(m->gat);
				m->gat = NULL;
				m->xs = 0; 
				m->ys = 0; 
				return 0;
			} else {
					// ����
					return 1;
				}
		} else if(map_cache.map[i].compressed == 1) { //zlib compressed
				// ���k�t���O=1 : zlib
				unsigned char *buf;
			unsigned long size_compress = map_cache.map[i].datalen;
			unsigned long dest_len = map_cache.map[i].xs * map_cache.map[i].ys * sizeof(struct mapgat);
				m->xs = map_cache.map[i].xs;
				m->ys = map_cache.map[i].ys;
				buf = (unsigned char*)aMalloc(size_compress);
				fseek(map_cache.fp,map_cache.map[i].pos,SEEK_SET);
				if(fread(buf,1,size_compress,map_cache.fp) != size_compress) {
					// �Ȃ����t�@�C���㔼�������Ă�̂œǂݒ���
					aFree(buf);
				buf = NULL;
				m->gat = NULL;
				m->xs = 0; 
				m->ys = 0; 
					return 0;
				}
			m->gat = (struct mapgat *)aMalloc( dest_len );				
			decode_zip((unsigned char*)m->gat, &dest_len, buf, size_compress);
			if(dest_len != map_cache.map[i].xs * map_cache.map[i].ys * sizeof(struct mapgat)) {
					// ����ɉ𓀂��o���ĂȂ�
					aFree(buf);
				buf=NULL;
				aFree(m->gat);
				m->gat = NULL;
				m->xs = 0; 
				m->ys = 0; 
					return 0;
				}
			if(buf)// might be ok without this check
			{	aFree(buf);
				buf=NULL;
			}
				return 1;
			}
		// might add other compressions here
		}
	return 0;
}

int map_cache_write(struct map_data *m)
{
	size_t i;
	unsigned long len_new , len_old;
	unsigned char *write_buf;
	if(!map_cache.fp) { return 0; }

	for(i = 0;i < map_cache.head.nmaps ; i++) 
	{
		if( (0==strcmp(m->mapname,map_cache.map[i].fn)) || (map_cache.map[i].fn[0] == 0) )
			break;
			}

	if(i<map_cache.head.nmaps) // should always be valid but better check it
	{
		int compress = 0;

		if(map_read_flag == READ_FROM_BITMAP_COMPRESSED) compress = 1;	// zlib compress
																		// might add other compressions here

		// prepare write_buf and len_new
		if(compress == 1) // zlib compress
		{
				// ���k�ۑ�
				// �������ɂQ�{�ɖc��鎖�͂Ȃ��Ƃ�������
			len_new = 2 * m->xs * m->ys * sizeof(struct mapgat);
			write_buf = (unsigned char *)aMalloc( len_new );
			encode_zip(write_buf,&len_new,(unsigned char *)m->gat, m->xs*m->ys*sizeof(struct mapgat));
			}
		else // no compress
		{ 
			len_new = m->xs * m->ys *sizeof(struct mapgat);
			write_buf = (unsigned char*)m->gat;
		}
		
		// now insert it
		if( (map_cache.map[i].fn[0] == 0) ) // new map is inserted
		{
			// write at the end of the mapcache file
				fseek(map_cache.fp,map_cache.head.filesize,SEEK_SET);
				fwrite(write_buf,1,len_new,map_cache.fp);

			// prepare the data header
			memcpy(map_cache.map[i].fn, m->mapname, sizeof(map_cache.map[i].fn));
			map_cache.map[i].fn[sizeof(map_cache.map[i].fn)-1]=0;			

			// update file header
				map_cache.map[i].pos = map_cache.head.filesize;
				map_cache.head.filesize += len_new;
			}
		else // update an existing map
		{
			len_old = map_cache.map[i].datalen;

			if(len_new <= len_old) // size is ok
			{	// �T�C�Y���������������Ȃ����̂ŏꏊ�͕ς��Ȃ�
				fseek(map_cache.fp,map_cache.map[i].pos,SEEK_SET);
				fwrite(write_buf,1,len_new,map_cache.fp);
	
			}
			else // new len is larger then the old space ->write at file end
			{	// �V�����ꏊ�ɓo�^
			fseek(map_cache.fp,map_cache.head.filesize,SEEK_SET);
			fwrite(write_buf,1,len_new,map_cache.fp);

				// update file header
			map_cache.map[i].pos = map_cache.head.filesize;
				map_cache.head.filesize += len_new;			
			}
		}
		// just make sure that everything gets updated
		map_cache.map[i].xs  = m->xs;
		map_cache.map[i].ys  = m->ys;
		map_cache.map[i].water_height = map_waterheight(m->mapname);
		map_cache.map[i].compressed   = compress;
		map_cache.map[i].datalen      = len_new;
		map_cache.dirty = 1;

		if(compress == 1)	// zlib compress has alloced an additional buffer
		{
			aFree(write_buf);
			write_buf = NULL;
		}
		return 0;
	}
	// �����b߂Ȃ�����
	return 1;
}

#ifdef USE_AFM
int map_readafm(int m,char *fn)
{

	/*
	Advanced Fusion Maps Support
	(c) 2003-2004, The Fusion Project
	- AlexKreuz

	The following code has been provided by me for eAthena
	under the GNU GPL.  It provides Advanced Fusion
	Map, the map format desgined by me for Fusion, support
	for the eAthena emulator.

	I understand that because it is under the GPL
	that other emulators may very well use this code in their
	GNU project as well.

	The AFM map format was not originally a part of the GNU
	GPL. It originated from scratch by my own hand.  I understand
	that distributing this code to read the AFM maps with eAthena
	causes the GPL to apply to this code.  But the actual AFM
	maps are STILL copyrighted to the Fusion Project.  By choosing

	In exchange for that 'act of faith' I ask for the following.

	A) Give credit where it is due.  If you use this code, do not
	   place your name on the changelog.  Credit should be given
	   to AlexKreuz.
	B) As an act of courtesy, ask me and let me know that you are putting
	   AFM support in your project.  You will have my blessings if you do.
	C) Use the code in its entirety INCLUDING the copyright message.
	   Although the code provided may now be GPL, the AFM maps are not
	   and so I ask you to display the copyright message on the STARTUP
	   SCREEN as I have done here. (refer to core.c)
	   "Advanced Fusion Maps (c) 2003-2004 The Fusion Project"

	Without this copyright, you are NOT entitled to bundle or distribute
	the AFM maps at all.  On top of that, your "support" for AFM maps
	becomes just as shady as your "support" for Gravity GRF files.

	The bottom line is this.  I know that there are those of you who
	would like to use this code but aren't going to want to provide the
	proper credit.  I know this because I speak frome experience.  If
	you are one of those people who is going to try to get around my
	requests, then save your breath because I don't want to hear it.

	I have zero faith in GPL and I know and accept that if you choose to
	not display the copyright for the AFMs then there is absolutely nothing
	I can do about it.  I am not about to start a legal battle over something
	this silly.

	Provide the proper credit because you believe in the GPL.  If you choose
	not to and would rather argue about it, consider the GPL failed.

	October 18th, 2004
	- AlexKreuz
	- The Fusion Project
	*/


	int x,y,xs,ys;
	size_t size;

	char afm_line[65535];
	int afm_size[2];
	FILE *afm_file;
	char *str;

	afm_file = savefopen(fn, "r");
	if (afm_file != NULL) {

		ShowMessage("\rLoading Maps [%d/%d]: %-50s  ",m,map_num,fn);
		fflush(stdout);

		str=fgets(afm_line, sizeof(afm_line)-1, afm_file);
		str=fgets(afm_line, sizeof(afm_line)-1, afm_file);
		str=fgets(afm_line, sizeof(afm_line)-1, afm_file);
		sscanf(str , "%d%d", &afm_size[0], &afm_size[1]);

		map[m].m = m;
		xs = map[m].xs = afm_size[0];
		ys = map[m].ys = afm_size[1];

		map[m].npc_num=0;
		map[m].users=0;
		memset(&map[m].flag,0,sizeof(map[m].flag));

		if(battle_config.pk_mode) map[m].flag.pvp = 1; // make all maps pvp for pk_mode [Valaris]

		map[m].gat = (struct mapgat *)aCalloc( (map[m].xs*map[m].ys), sizeof(struct mapgat));
		for (y = 0; y < ys; y++) {
			str=fgets(afm_line, sizeof(afm_line)-1, afm_file);
			for (x = 0; x < xs; x++) {
				// no direct access
				map_setcell(m,x,y, str[x] & CELL_MASK );
			}
		}

		map[m].bxs=(xs+BLOCK_SIZE-1)/BLOCK_SIZE;
		map[m].bys=(ys+BLOCK_SIZE-1)/BLOCK_SIZE;
		size = map[m].bxs * map[m].bys;
		map[m].block = (struct block_list**)aCalloc(size, sizeof(struct block_list*));
		map[m].block_mob = (struct block_list**)aCalloc(size, sizeof(struct block_list*));

		size = map[m].bxs*map[m].bys;
		map[m].block_count = (int *)aCalloc(size, sizeof(int));
		map[m].block_mob_count = (int *)aCalloc(size, sizeof(int));

		strdb_insert(map_db,map[m].mapname,&map[m]);

		fclose(afm_file);

	}

	return 0;
}

bool map_readaf2(int m, const char *fn)
{
	FILE *af2_file, *dest;
	char buf[256];
	bool ret=false;

	af2_file = savefopen(fn, "r");
	if( af2_file != NULL )
	{
		memcpy(buf,              fn,     strlen(fn)-4);
		memcpy(buf+strlen(fn)-4, ".out", 5);

		dest = fopen(buf, "w");
		if (dest == NULL)
		{
			printf ("can't open\n");
			fclose(af2_file);
			return 0;
		}
		ret = 0!=decode_file(af2_file, dest);
		fclose(af2_file);
		fclose(dest);

		if(ret) map_readafm(m, buf);
		remove(buf);
	}
	return ret;
}
#endif

/*==========================================
 * �}�b�v1���ǂݍb�
 * ===================================================*/
static int map_readmap(int m,char *fn, char *alias, int *map_cache, int maxmap) {

	size_t size;

	if(map_cache_read(&map[m]))
	{	// �L���b�V������ǂݍb߂�
		(*map_cache)++;
	}
	else
	{
		int wh;
		int x,y;
		struct gat_1cell {float high[4]; long type;};
		unsigned char *gat, *p;

		// read & convert fn
		gat = (unsigned char *)grfio_read(fn);
		if(gat==NULL)
			return -1;
		map[m].xs= (short)RBUFL(gat, 6);
		map[m].ys= (short)RBUFL(gat,10);

		map[m].gat = (struct mapgat *)aCalloc( (map[m].xs * map[m].ys), sizeof(struct mapgat));

		wh=map_waterheight(map[m].mapname);
		
		ShowMessage("\rLoading Maps [%d/%d]: %s, size (%d %d)(%i)%-10s",m,map_num,fn,map[m].xs,map[m].ys,wh,"");
		
		p = gat+14;
		for(y=0;y<map[m].ys;y++)
		for(x=0;x<map[m].xs;x++){
			struct gat_1cell pp;	// make a real structure in memory
			memcpy(&pp, p, sizeof(struct gat_1cell));	// copy all stuff
			p += sizeof(struct gat_1cell);				// set pointer to next section

			if(MSB_FIRST==CheckByteOrder()) // little/big endian
			{	// need to correct the whole struct since we have no suitable buffer assigns
				// gat_1cell contains 4 floats and one long (4byte each) so swapping these is enough
				// the structure is memory alligned so it is safe to use just the pointers
				SwapFourBytes(((char*)(&pp)) + sizeof(long)*0);
				SwapFourBytes(((char*)(&pp)) + sizeof(long)*1);
				SwapFourBytes(((char*)(&pp)) + sizeof(long)*2);
				SwapFourBytes(((char*)(&pp)) + sizeof(long)*3);
				SwapFourBytes(((char*)(&pp)) + sizeof(long)*4);
			}

			if(wh!=NO_WATER && pp.type==0)
			{	// ��Ŋ���
				// no direct access
				//map[m].gat[x+y*map[m].xs].type=(pp.high[0]>wh || pp.high[1]>wh || pp.high[2]>wh || pp.high[3]>wh) ? 3 : 0;
				map_setcell(m,x,y,(pp.high[0]>wh || pp.high[1]>wh || pp.high[2]>wh || pp.high[3]>wh) ? 3 : 0);
			}
			else
			{	// no direct access
				//map[m].gat[x+y*map[m].xs].type=() & CELL_MASK;
				map_setcell(m,x,y,pp.type);
			}
		}
		map_cache_write(&map[m]);
		aFree(gat);
	}

	memset(&map[m].flag,0,sizeof(map[m].flag));
	if(battle_config.pk_mode)
		map[m].flag.pvp = 1; // make all maps pvp for pk_mode [Valaris]

	map[m].m=m;
	map[m].npc_num=0;
	map[m].users=0;
	map[m].bxs=(short)((map[m].xs+BLOCK_SIZE-1)/BLOCK_SIZE);
	map[m].bys=(short)((map[m].ys+BLOCK_SIZE-1)/BLOCK_SIZE);
	size = map[m].bxs * map[m].bys;
	map[m].block = (struct block_list **)aCalloc(size, sizeof(struct block_list*));
	map[m].block_mob = (struct block_list **)aCalloc(size, sizeof(struct block_list*));
	map[m].block_count = (int *)aCalloc(size, sizeof(int));
	map[m].block_mob_count=(int *)aCalloc(size, sizeof(int));

	if (alias)
		strdb_insert(map_db,alias,&map[m]);
	else
		strdb_insert(map_db,map[m].mapname,&map[m]);

	return 0;
}

/*==========================================
 * �S�Ă�map�f?�^��?��?��
 *------------------------------------------
 */
int map_readallmap(void)
{
	size_t i,maps_removed=0;
	char fn[256];
	int map_cache = 0;

	// �}�b�v�L���b�V�����J��
	if(map_read_flag >= READ_FROM_BITMAP)
	{
		map_cache_open(map_cache_file);
	}

	ShowStatus("Loading Maps%s...\n",
		(map_read_flag == READ_FROM_BITMAP_COMPRESSED ? " (w/ Compressed Map Cache)" :
		 map_read_flag >= READ_FROM_BITMAP            ? " (w/ Map Cache)" :
		 map_read_flag == READ_FROM_AFM               ? " (w/ AFM)" : "") );

	// ��ɑS���̃�b�v�̑��݂��m�F
	for(i=0;i<map_num;i++)
	{
#ifdef USE_AFM
		FILE *afm_file;
		char afm_name[256] = "";

		map[i].alias = NULL;

		if(!strstr(map[i].mapname,".afm")) {
		// check if it's necessary to replace the extension - speeds up loading abit
			memcpy(afm_name, map[i].mapname, strlen(map[i].mapname) - 3);
			memcpy(afm_name+strlen(map[i].mapname) - 3, "afm", 4); // copy with EOS
		}
		sprintf(fn,"%s%c%s",afm_dir,PATHSEP,afm_name);
		afm_file = savefopen(fn, "r");
		if (afm_file != NULL)
		{
			fclose(afm_file);
			// map_readafm open and closes the file anyway
			map_readafm(i,fn);
			continue;
		}

		// try with *.af2
		fn[strlen(fn)-1] = '2';
		afm_file = savefopen(fn, "r");
		if (afm_file != NULL)
		{
			fclose(afm_file);
			if (map_readaf2(i,fn) != 0)
				continue;
		}
#endif
		char *p = strchr(map[i].mapname, '<'); // [MouseJstr]
		if (p != NULL) 
		{	// swap mapname and the stuff after the '<' 
			// asuming following ('.' is EOS marker):
			// buffer: aaaaaaaa<bbbbb. change to:
			// buffer: bbbbb.aaaaaaaa.
			// use bbbbb as mapname and aaaaaaaa as alias with pointer at map[i].alias
			// so we do not need a strdup
			char alias[64];
			*p++ = '\0';
			strcpy(alias, map[i].mapname);
			strcpy(map[i].mapname, p);
			p = map[i].mapname+strlen(map[i].mapname)+1; // the first position after the EOF of the new mapname
			strcpy(p,alias);
			map[i].alias = p;
		}
		else
			map[i].alias = NULL;

		sprintf(fn,"data\\%s",map[i].mapname);
		if(map_readmap(i,fn, p, &map_cache, map_num) == -1)
		{
			map_delmap(map[i].mapname);
			maps_removed++;
			i--;
		}
	}

	aFree(waterlist);
	ShowMessage("\r");
	ShowInfo("Successfully loaded '"CL_WHITE"%d"CL_RESET"' maps.%30s\n",map_num,"");

	map_cache_close();

	if (maps_removed) {
		ShowNotice("Maps Removed: '"CL_WHITE"%d"CL_RESET"'\n",maps_removed);
	}
	return 0;
}

/*==========================================
 * ?��?��map��ǉ�����
 *------------------------------------------
 */
int map_addmap(char *mapname) {
	if (strcasecmp(mapname,"clear")==0) {
		map_num=0;
		return 0;
	}

	if (map_num >= MAX_MAP_PER_SERVER - 1) {
		ShowError("Could not add map '"CL_WHITE"%s"CL_RESET"', the limit of maps has been reached.\n",mapname);
		return 1;
	}
	memcpy(map[map_num].mapname, mapname, 24);
	map_num++;
	return 0;
}

/*==========================================
 * ?��?��map���폜����
 *------------------------------------------
 */
int map_delmap(char *mapname) {

	size_t i;

	if (strcasecmp(mapname, "all") == 0) {
		map_num = 0;
		return 0;
	}

	for(i = 0; i < map_num; i++) {
		if (strcmp(map[i].mapname, mapname) == 0) {
		    ShowMessage("Removing map [ %s ] from maplist\n",map[i].mapname);
			memmove(map+i, map+i+1, sizeof(map[0])*(map_num-i-1));
			map_num--;
		}
	}
	return 0;
}

/*==========================================
 * Console Command Parser [Wizputer]
 *------------------------------------------
 */
int parse_console(char *buf) {
    char *type,*command,*map, *buf2;
    int x = 0, y = 0;
    int m, n;
    struct map_session_data *sd;

    sd = (struct map_session_data *)aCalloc(1,sizeof(struct map_session_data));

    sd->fd = 0;
    strcpy( sd->status.name , "console");

    type	= (char *)aCalloc(64, sizeof(char));
    command	= (char *)aCalloc(64, sizeof(char));
    map		= (char *)aCalloc(64, sizeof(char));
    buf2	= (char *)aCalloc(72, sizeof(char));

    if ( ( n = sscanf(buf, "%[^:]:%[^:]:%99s %d %d[^\n]", type , command , map , &x , &y )) < 5 )
        if ( ( n = sscanf(buf, "%[^:]:%[^\n]", type , command )) < 2 )
            n = sscanf(buf,"%[^\n]",type);

    if ( n == 5 ) {
        if (x <= 0) {
            x = rand() % 399 + 1;
            sd->bl.x = x;
        } else {
            sd->bl.x = x;
        }

        if (y <= 0) {
            y = rand() % 399 + 1;
            sd->bl.y = y;
        } else {
            sd->bl.y = y;
        }

        m = map_mapname2mapid(map);
        if ( m >= 0 )
            sd->bl.m = m;
        else {
			ShowConsole(CL_BOLD"Unknown map\n"CL_NORM);
            goto end;
        }
    }

    ShowMessage("Type of command: %s || Command: %s || Map: %s Coords: %d %d\n",type,command,map,x,y);

    if ( strcasecmp("admin",type) == 0 && n == 5 ) {
        sprintf(buf2,"console: %s",command);
        if( is_atcommand(sd->fd,sd,buf2,99) == AtCommand_None )
			ShowConsole(CL_BOLD"no valid atcommand\n"CL_NORM);
    } else if ( strcasecmp("server",type) == 0 && n == 2 ) {
        if ( strcasecmp("shutdown", command) == 0 || strcasecmp("exit",command) == 0 || strcasecmp("quit",command) == 0 ) {
            runflag = 0;
        }
    } else if ( strcasecmp("help",type) == 0 ) {
        ShowMessage("To use GM commands:\n");
        ShowMessage("admin:<gm command>:<map of \"gm\"> <x> <y>\n");
        ShowMessage("You can use any GM command that doesn't require the GM.\n");
        ShowMessage("No using @item or @warp however you can use @charwarp\n");
        ShowMessage("The <map of \"gm\"> <x> <y> is for commands that need coords of the GM\n");
        ShowMessage("IE: @spawn\n");
        ShowMessage("To shutdown the server:\n");
        ShowMessage("server:shutdown\n");
    }

    end:
    aFree(buf);
    aFree(type);
    aFree(command);
    aFree(map);
    aFree(buf2);
    aFree(sd);

    return 0;
}

/*==========================================
 * �ݒ�t�@�C����?��?��
 *------------------------------------------
 */
int map_config_read(const char *cfgName) {
	char line[1024], w1[1024], w2[1024];
	FILE *fp;
	struct hostent *h = NULL;

	fp = savefopen(cfgName,"r");
	if (fp == NULL) {
		ShowMessage("Map configuration file not found at: %s\n", cfgName);
		exit(1);
	}
	
	while(fgets(line, sizeof(line) -1, fp)) {
		if (line[0] == '/' && line[1] == '/')
			continue;
		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) == 2) {
			if (strcasecmp(w1, "userid")==0){
				chrif_setuserid(w2);
			} else if (strcasecmp(w1, "passwd") == 0) {
				chrif_setpasswd(w2);
			} else if (strcasecmp(w1, "char_ip") == 0) {
				h = gethostbyname(w2);
				if(h != NULL) {
					ShowInfo("Char Server IP Address : '"CL_WHITE"%s"CL_RESET"' -> '"CL_WHITE"%d.%d.%d.%d"CL_RESET"'.\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
					sprintf(w2,"%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				}
				chrif_setip( ntohl(inet_addr(w2)) );

			} else if (strcasecmp(w1, "char_port") == 0) {
				chrif_setport(atoi(w2));
			} else if (strcasecmp(w1, "map_ip") == 0) {
				h = gethostbyname(w2);
				if (h != NULL) {
					ShowInfo("Map Server IP Address : '"CL_WHITE"%s"CL_RESET"' -> '"CL_WHITE"%d.%d.%d.%d"CL_RESET"'.\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
					sprintf(w2, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				}
				clif_setip( ntohl(inet_addr(w2)) );
			} else if (strcasecmp(w1, "map_port") == 0) {
				clif_setport(atoi(w2));
			} else if (strcasecmp(w1, "water_height") == 0) {
				map_readwater(w2);
			} else if (strcasecmp(w1, "map") == 0) {
				map_addmap(w2);
			} else if (strcasecmp(w1, "delmap") == 0) {
				map_delmap(w2);
			} else if (strcasecmp(w1, "npc") == 0) {
				npc_addsrcfile(w2);
			} else if (strcasecmp(w1, "path") == 0) {
				////////////////////////////////////////
				// add all .txt files recursive from ./npc folder to npc source tree
				findfile(w2, ".txt", npc_addsrcfile );
				////////////////////////////////////////
			} else if (strcasecmp(w1, "delnpc") == 0) {
				npc_delsrcfile(w2);
			} else if (strcasecmp(w1, "data_grf") == 0) {
				grfio_setdatafile(w2);
			} else if (strcasecmp(w1, "sdata_grf") == 0) {
				grfio_setsdatafile(w2);
			} else if (strcasecmp(w1, "adata_grf") == 0) {
				grfio_setadatafile(w2);
			} else if (strcasecmp(w1, "autosave_time") == 0) {
				autosave_interval = atoi(w2) * 1000;
				if (autosave_interval <= 0)
					autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
			} else if (strcasecmp(w1, "motd_txt") == 0) {
				strcpy(motd_txt, w2);
			} else if (strcasecmp(w1, "help_txt") == 0) {
				strcpy(help_txt, w2);
			} else if (strcasecmp(w1, "mapreg_txt") == 0) {
				strcpy(mapreg_txt, w2);
			}else if(strcasecmp(w1,"read_map_from_cache")==0){
				if (atoi(w2) == 2)
					map_read_flag = READ_FROM_BITMAP_COMPRESSED;
				else if (atoi(w2) == 1)
					map_read_flag = READ_FROM_BITMAP;
				else
					map_read_flag = READ_FROM_GAT;
			}else if(strcasecmp(w1,"map_cache_file")==0){
				strncpy(map_cache_file,w2,255);
			} else if (strcasecmp(w1, "import") == 0) {
				map_config_read(w2);
			} else if (strcasecmp(w1, "console") == 0) {
				console = config_switch(w2);
            } else if(strcasecmp(w1,"imalive_on")==0){		//Added by Mugendai for I'm Alive mod
				imalive_on = atoi(w2);					//Added by Mugendai for I'm Alive mod
			} else if(strcasecmp(w1,"imalive_time")==0){	//Added by Mugendai for I'm Alive mod
				imalive_time = atoi(w2);				//Added by Mugendai for I'm Alive mod
			} else if(strcasecmp(w1,"flush_on")==0){		//Added by Mugendai for GUI
				flush_on = atoi(w2);					//Added by Mugendai for GUI
			} else if(strcasecmp(w1,"flush_time")==0){		//Added by Mugendai for GUI
				flush_time = atoi(w2);					//Added by Mugendai for GUI
			}
		}
	}
	fclose(fp);


	return 0;
}

int inter_config_read(char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;

	fp=savefopen(cfgName,"r");
	if(fp==NULL){
		ShowError("File not found: '%s'.\n",cfgName);
		return 1;
	}
	while(fgets(line,1020,fp)){
		if(line[0] == '/' && line[1] == '/')
			continue;
		i=sscanf(line,"%[^:]: %[^\r\n]",w1,w2);
		if(i!=2)
			continue;

		if(strcasecmp(w1,"import")==0){
		//support the import command, just like any other config
			inter_config_read(w2);
		}
	#ifndef TXT_ONLY
		  else if(strcasecmp(w1,"item_db_db")==0){
			strcpy(item_db_db,w2);
		} else if(strcasecmp(w1,"mob_db_db")==0){
			strcpy(mob_db_db,w2);
		} else if(strcasecmp(w1,"login_db_level")==0){
			strcpy(login_db_level,w2);
		} else if(strcasecmp(w1,"login_db_account_id")==0){
		    strcpy(login_db_account_id,w2);
		} else if(strcasecmp(w1,"login_db")==0){
			strcpy(login_db,w2);
		} else if (strcasecmp(w1, "char_db") == 0) {
			strcpy(char_db, w2);
		} else if(strcasecmp(w1,"gm_db_level")==0){
			strcpy(gm_db_level,w2);
		} else if(strcasecmp(w1,"gm_db_account_id")==0){
		    strcpy(gm_db_account_id,w2);
		} else if(strcasecmp(w1,"gm_db")==0){
			strcpy(gm_db,w2);
		//Map Server SQL DB
		} else if(strcasecmp(w1,"map_server_ip")==0){
			strcpy(map_server_ip, w2);
		} else if(strcasecmp(w1,"map_server_port")==0){
			map_server_port=atoi(w2);
		} else if(strcasecmp(w1,"map_server_id")==0){
			strcpy(map_server_id, w2);
		} else if(strcasecmp(w1,"map_server_pw")==0){
			strcpy(map_server_pw, w2);
		} else if(strcasecmp(w1,"map_server_db")==0){
			strcpy(map_server_db, w2);
		} else if(strcasecmp(w1,"use_sql_db")==0){
			db_use_sqldbs = config_switch(w2);
			ShowMessage ("Using SQL dbs: %s\n",w2);
		//Login Server SQL DB
		} else if(strcasecmp(w1,"login_server_ip")==0){
			strcpy(login_server_ip, w2);
        } else if(strcasecmp(w1,"login_server_port")==0){
			login_server_port = atoi(w2);
		} else if(strcasecmp(w1,"login_server_id")==0){
			strcpy(login_server_id, w2);
		} else if(strcasecmp(w1,"login_server_pw")==0){
			strcpy(login_server_pw, w2);
		} else if(strcasecmp(w1,"login_server_db")==0){
			strcpy(login_server_db, w2);
		} else if(strcasecmp(w1,"lowest_gm_level")==0){
			lowest_gm_level = atoi(w2);
		} else if(strcasecmp(w1,"read_gm_interval")==0){
			read_gm_interval = ( atoi(w2) * 60 * 1000 ); // Minutes multiplied by 60 secs per min by 1000 milliseconds per second
		} else if(strcasecmp(w1,"log_db")==0) {
			strcpy(log_db, w2);
		} else if(strcasecmp(w1,"log_db_ip")==0) {
			strcpy(log_db_ip, w2);
		} else if(strcasecmp(w1,"log_db")==0) {
			strcpy(log_db, w2);
		} else if(strcasecmp(w1,"log_db_id")==0) {
			strcpy(log_db_id, w2);
		} else if(strcasecmp(w1,"log_db_pw")==0) {
			strcpy(log_db_pw, w2);
		} else if(strcasecmp(w1,"log_db_port")==0) {
			log_db_port = atoi(w2);
		}
	#endif
		}
	fclose(fp);

	return 0;
}

#ifndef TXT_ONLY
/*=======================================
 *  MySQL Init
 *---------------------------------------
 */

int map_sql_init(void){

    mysql_init(&mmysql_handle);

	//DB connection start
	ShowMessage("Connect Map DB Server....\n");
	if(!mysql_real_connect(&mmysql_handle, map_server_ip, map_server_id, map_server_pw,
		map_server_db ,map_server_port, (char *)NULL, 0)) {
			//pointer check
			ShowMessage("%s\n",mysql_error(&mmysql_handle));
			exit(1);
	}
	else {
		ShowMessage ("connect success! (Map Server Connection)\n");
	}

    mysql_init(&lmysql_handle);

    //DB connection start
    ShowMessage("Connect Login DB Server....\n");
    if(!mysql_real_connect(&lmysql_handle, login_server_ip, login_server_id, login_server_pw,
        login_server_db ,login_server_port, (char *)NULL, 0)) {
	        //pointer check
			ShowMessage("%s\n",mysql_error(&lmysql_handle));
			exit(1);
	}
	 else {
		ShowMessage ("connect success! (Login Server Connection)\n");
	 }

	if(battle_config.mail_system) { // mail system [Valaris]
		mysql_init(&mail_handle);
		if(!mysql_real_connect(&mail_handle, map_server_ip, map_server_id, map_server_pw,
			map_server_db ,map_server_port, (char *)NULL, 0)) {
				ShowMessage("%s\n",mysql_error(&mail_handle));
				exit(1);
		}
	}

	return 0;
}

int map_sql_close(void){
	mysql_close(&mmysql_handle);
	ShowMessage("Close Map DB Connection....\n");

	mysql_close(&lmysql_handle);
	ShowMessage("Close Login DB Connection....\n");
	return 0;
}

int log_sql_init(void){

    mysql_init(&mmysql_handle);

	//DB connection start
	ShowMessage(""CL_WHITE"[SQL]"CL_RESET": Connecting to Log Database "CL_WHITE"%s"CL_RESET" At "CL_WHITE"%s"CL_RESET"...\n",log_db,log_db_ip);
	if(!mysql_real_connect(&mmysql_handle, log_db_ip, log_db_id, log_db_pw,
		log_db ,log_db_port, (char *)NULL, 0)) {
			//pointer check
			ShowMessage(""CL_WHITE"[SQL Error]"CL_RESET": %s\n",mysql_error(&mmysql_handle));
			exit(1);
	} else {
		ShowMessage(""CL_WHITE"[SQL]"CL_RESET": Successfully '"CL_GREEN"connected"CL_RESET"' to Database '"CL_WHITE"%s"CL_RESET"'.\n", log_db);
	}

	return 0;
}
#endif /* not TXT_ONLY */


void char_online_check(void)
{
	size_t i;
	struct map_session_data *sd;

	chrif_char_reset_offline();

	for(i=0;i<fd_max;i++)
	{
		if( session[i] && 
			(sd = (struct map_session_data*)session[i]->session_data) && sd->state.auth &&
			!(battle_config.hide_GM_session && 
			pc_isGM(sd)) && 
			sd->status.char_id)
		{
			chrif_char_online(sd);
		}
	}
}

int online_timer(int tid,unsigned long tick,int id,int data)
{
	char_online_check();
	return 0;
}



//-----------------------------------------------------
//I'm Alive Alert
//Used to output 'I'm Alive' every few seconds
//Intended to let frontends know if the app froze
//-----------------------------------------------------
int imalive_timer(int tid, unsigned long tick, int id, int data){
	ShowMessage("I'm Alive\n");
	return 0;
}

//-----------------------------------------------------
//Flush stdout
//stdout buffer needs flushed to be seen in GUI
//-----------------------------------------------------
int flush_timer(int tid, unsigned long tick, int id, int data){
	fflush(stdout);
	return 0;
}

int id_db_final(void *k,void *d,va_list ap) 
{
	return 0; 
}
int map_db_final(void *k,void *d,va_list ap) 
{
	return 0; 
}
int nick_db_final(void *k,void *d,va_list ap)
{
	char *p = (char *) d;
	if (p) aFree(p);
	return 0;
}
int charid_db_final(void *k,void *d,va_list ap)
{
	struct charid2nick *p = (struct charid2nick *) d;
	if (p) aFree(p);
	return 0;
}
int cleanup_sub(struct block_list *bl, va_list ap) 
{
	nullpo_retr(0, bl);
        switch(bl->type) {
        case BL_PC:
            map_quit((struct map_session_data *) bl);
            break;
        case BL_NPC:
            npc_unload((struct npc_data *)bl);
            break;
        case BL_MOB:
            mob_unload((struct mob_data *)bl);
            break;
        case BL_PET:
            pet_remove_map((struct map_session_data *)bl);
            break;
        case BL_ITEM:
            map_clearflooritem(bl->id);
            break;
        case BL_SKILL:
            skill_delunit((struct skill_unit *) bl);
            break;
        }
	return 0;
}

/*==========================================
 * map�I�I�����
 *------------------------------------------
 */
void do_final(void)
{
    size_t i;
    ShowStatus("Terminating...\n");
	///////////////////////////////////////////////////////////////////////////

    grfio_final();

    for (i = 0; i < map_num; i++)
		if(map[i].m)
			map_foreachinarea(cleanup_sub, i, 0, 0, map[i].xs, map[i].ys, 0, 0);

    chrif_char_reset_offline();
    chrif_flush_fifo();

    map_removenpc();
	do_final_chrif(); // ���̓����ŃL������S�Đؒf����
	do_final_npc();
	do_final_script();
	do_final_itemdb();
	do_final_storage();
	do_final_guild();
	do_final_party();
	do_final_pc();
	do_final_pet();
	do_final_msg();

	for (i=0; i<map_num; i++) {
		if(map[i].gat) {
			aFree(map[i].gat);
			map[i].gat=NULL;
		}
		if(map[i].block)			{ aFree(map[i].block); map[i].block=NULL; }
		if(map[i].block_mob)		{ aFree(map[i].block_mob); map[i].block_mob=NULL; }
		if(map[i].block_count)		{ aFree(map[i].block_count); map[i].block_count=NULL; }
		if(map[i].block_mob_count)	{ aFree(map[i].block_mob_count); map[i].block_mob_count=NULL; }
	}
    numdb_final(id_db, id_db_final);
	strdb_final(map_db, map_db_final);
    strdb_final(nick_db, nick_db_final);
    numdb_final(charid_db, charid_db_final);

//#endif

#ifndef TXT_ONLY
    map_sql_close();
#endif /* not TXT_ONLY */

	///////////////////////////////////////////////////////////////////////////
	// delete sessions
	for(i = 0; i < fd_max; i++)
		if(session[i] != NULL) 
			session_Delete(i);
	// clear externaly stored fd's

	///////////////////////////////////////////////////////////////////////////
}

/*======================================================
 * Map-Server Version Screen [MC Cameri]
 *------------------------------------------------------
 */
void map_helpscreen(int flag) { // by MC Cameri
	puts("Usage: map-server [options]");
	puts("Options:");
	puts(CL_WHITE"  Commands\t\t\tDescription"CL_RESET);
	puts("-----------------------------------------------------------------------------");
	puts("  --help, --h, --?, /?		Displays this help screen");
	puts("  --map-config <file>		Load map-server configuration from <file>");
	puts("  --battle-config <file>	Load battle configuration from <file>");
	puts("  --atcommand-config <file>	Load atcommand configuration from <file>");
	puts("  --charcommand-config <file>	Load charcommand configuration from <file>");
	puts("  --script-config <file>	Load script configuration from <file>");
	puts("  --msg-config <file>		Load message configuration from <file>");
	puts("  --grf-path-file <file>	Load grf path file configuration from <file>");
	puts("  --sql-config <file>		Load inter-server configuration from <file>");
	puts("				(SQL Only)");
	puts("  --log-config <file>		Load logging configuration from <file>");
	puts("				(SQL Only)");
	puts("  --version, --v, -v, /v	Displays the server's version");
	puts("\n");
	if (flag) exit(1);
}

/*======================================================
 * Map-Server Version Screen [MC Cameri]
 *------------------------------------------------------
 */
void map_versionscreen(int flag) {
	ShowMessage("CL_WHITE" "eAthena version %d.%02d.%02d, Athena Mod version %d" CL_RESET"\n",
		ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION,
		ATHENA_MOD_VERSION);
	puts(CL_GREEN "Website/Forum:" CL_RESET "\thttp://eathena.deltaanime.net/");
	puts(CL_GREEN "Download URL:" CL_RESET "\thttp://eathena.systeminplace.net/");
	puts(CL_GREEN "IRC Channel:" CL_RESET "\tirc://irc.deltaanime.net/#athena");
	puts("\nOpen " CL_WHITE "readme.html" CL_RESET " for more information.");
	if (ATHENA_RELEASE_FLAG) ShowNotice("This version is not for release.\n");
	if (flag) exit(1);
}

/*======================================================
 * Map-Server Init and Command-line Arguments [Valaris]
 *------------------------------------------------------
 */
int do_init(int argc, char *argv[]) {
	int i;
	FILE *data_conf;
	char line[1024], w1[1024], w2[1024];

#ifdef GCOLLECT
	GC_enable_incremental();
#endif

	INTER_CONF_NAME="conf/inter_athena.conf";
	LOG_CONF_NAME="conf/log_athena.conf";
	MAP_CONF_NAME = "conf/map_athena.conf";
	BATTLE_CONF_FILENAME = "conf/battle_athena.conf";
	ATCOMMAND_CONF_FILENAME = "conf/atcommand_athena.conf";
	CHARCOMMAND_CONF_FILENAME = "conf/charcommand_athena.conf";
	SCRIPT_CONF_NAME = "conf/script_athena.conf";
	MSG_CONF_NAME = "conf/msg_athena.conf";
	GRF_PATH_FILENAME = "conf/grf-files.txt";

	// just clear all maps
	memset(map, 0, MAX_MAP_PER_SERVER*sizeof(struct map_data));

	srand(gettick()^3141592654UL);

	for (i = 1; i < argc ; i++) {
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "--h") == 0 || strcmp(argv[i], "--?") == 0 || strcmp(argv[i], "/?") == 0)
			map_helpscreen(1);
		if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "--v") == 0 || strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "/v") == 0)
			map_versionscreen(1);
		else if (strcmp(argv[i], "--map_config") == 0 || strcmp(argv[i], "--map-config") == 0)
			MAP_CONF_NAME=argv[i+1];
		else if (strcmp(argv[i],"--battle_config") == 0 || strcmp(argv[i],"--battle-config") == 0)
			BATTLE_CONF_FILENAME = argv[i+1];
		else if (strcmp(argv[i],"--atcommand_config") == 0 || strcmp(argv[i],"--atcommand-config") == 0)
			ATCOMMAND_CONF_FILENAME = argv[i+1];
		else if (strcmp(argv[i],"--charcommand_config") == 0 || strcmp(argv[i],"--charcommand-config") == 0)
			CHARCOMMAND_CONF_FILENAME = argv[i+1];
		else if (strcmp(argv[i],"--script_config") == 0 || strcmp(argv[i],"--script-config") == 0)
			SCRIPT_CONF_NAME = argv[i+1];
		else if (strcmp(argv[i],"--msg_config") == 0 || strcmp(argv[i],"--msg-config") == 0)
			MSG_CONF_NAME = argv[i+1];
		else if (strcmp(argv[i],"--grf_path_file") == 0 || strcmp(argv[i],"--grf-path-file") == 0)
			GRF_PATH_FILENAME = argv[i+1];
#ifndef TXT_ONLY
		else if (strcmp(argv[i],"--inter_config") == 0 || strcmp(argv[i],"--inter-config") == 0)
		    INTER_CONF_NAME = argv[i+1];
#endif /* not TXT_ONLY */
		else if (strcmp(argv[i],"--log_config") == 0 || strcmp(argv[i],"--log-config") == 0)
		    LOG_CONF_NAME = argv[i+1];
		else if (strcmp(argv[i],"--run_once") == 0)	// close the map-server as soon as its done.. for testing [Celest]
			runflag = 0;
	}

	map_config_read(MAP_CONF_NAME);

	if ( naddr_ == 0 ) {
		ShowMessage("\nUnable to automatically determine the IP address.\n");
		ShowMessage("please edit the map_athena.conf file and set it to correct values.\n");
		ShowMessage("(127.0.0.1 is valid if you have no network interface)\n");
        }
	else if (clif_getip() == INADDR_ANY || clif_getip() == INADDR_LOOPBACK || chrif_getip() == INADDR_LOOPBACK) {
          // The map server should know what IP address it is running on
          //   - MouseJstr
		unsigned long localaddr = addr_[0]; // host order network address
          if (naddr_ != 1)
			ShowMessage("Multiple interfaces detected...  using %d.%d.%d.%d as primary IP address\n",
							(localaddr>>24)&0xFF, (localaddr>>16)&0xFF, (localaddr>>8)&0xFF, (localaddr)&0xFF);
          else
			ShowMessage("Defaulting to %d.%d.%d.%d as our IP address\n",
							(localaddr>>24)&0xFF, (localaddr>>16)&0xFF, (localaddr>>8)&0xFF, (localaddr)&0xFF);

		if (clif_getip() == INADDR_ANY || clif_getip() == INADDR_LOOPBACK)
			clif_setip(localaddr);
		if (chrif_getip() == INADDR_LOOPBACK)
			chrif_setip(localaddr);
		if ((localaddr&0xFFFF0000) == 0xC0A80000)//192.168.x.x
			ShowMessage("\nPrivate Network detected.. \nedit lan_support.conf and map_athena.conf\n\n");
        }
	if (SHOW_DEBUG_MSG) ShowNotice("Server running in '"CL_WHITE"Debug Mode"CL_RESET"'.\n");
	battle_config_read(BATTLE_CONF_FILENAME);
	msg_config_read(MSG_CONF_NAME);
	atcommand_config_read(ATCOMMAND_CONF_FILENAME);
	charcommand_config_read(CHARCOMMAND_CONF_FILENAME);
	script_config_read(SCRIPT_CONF_NAME);
	inter_config_read(INTER_CONF_NAME);
	log_config_read(LOG_CONF_NAME);

	id_db = numdb_init();
	map_db = strdb_init(24);
	nick_db = strdb_init(24);
	charid_db = numdb_init();
#ifndef TXT_ONLY
	map_sql_init();
#endif /* not TXT_ONLY */

	grfio_init(GRF_PATH_FILENAME);

	data_conf = savefopen(GRF_PATH_FILENAME, "r");
	// It will read, if there is grf-files.txt.
	if (data_conf) {
		while(fgets(line, 1020, data_conf)) {
			if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) == 2) {
				if(strcmp(w1,"afm_dir") == 0)
					strcpy(afm_dir, w2);
			}
		}
		fclose(data_conf);
	} // end of reading grf-files.txt for AFMs


	map_readallmap();

	add_timer_func_list(map_freeblock_timer,"map_freeblock_timer");
	add_timer_func_list(map_clearflooritem_timer, "map_clearflooritem_timer");
	add_timer_interval(gettick()+1000,60*1000,map_freeblock_timer,0,0);

	//Added by Mugendai for GUI support
	if (flush_on)
		add_timer_interval(gettick()+10,flush_time, flush_timer,0,0);
	//Added for Mugendais I'm Alive mod
	if (imalive_on)
		add_timer_interval(gettick()+10, imalive_time*1000, imalive_timer,0,0);

	// online status timer, checks every hour [Valaris]
	add_timer_func_list(online_timer, "online_timer");
	add_timer_interval(gettick()+10, CHECK_INTERVAL, online_timer, 0, 0);	

	do_init_chrif();
	do_init_clif();
	do_init_itemdb();
	do_init_mob();	// npc�̏��������mob_spawn���āAmob_db��?�Ƃ���̂�init_npc����
	do_init_script();
	do_init_pc();
	do_init_status();
	do_init_party();
	do_init_guild();
	do_init_storage();
	do_init_skill();
	do_init_pet();
	do_init_npc();

#ifndef TXT_ONLY /* mail system [Valaris] */
	if(battle_config.mail_system)
		do_init_mail();

	if (log_config.sql_logs && (log_config.branch || log_config.drop || log_config.mvpdrop ||
		log_config.present || log_config.produce || log_config.refine || log_config.trade))
	{
		log_sql_init();
	}
#endif /* not TXT_ONLY */

	npc_event_do_oninit();	// npc��OnInit�C�x���g?�s

	if ( console ) {
	    set_defaultconsoleparse(parse_console);
	   	start_console();
	}

	if (battle_config.pk_mode == 1)
		ShowNotice("Server is running on '"CL_WHITE"PK Mode"CL_RESET"'.\n");

	ShowStatus("Server is '"CL_GREEN"ready"CL_RESET"' and listening on port '"CL_WHITE"%d"CL_RESET"'.\n\n", clif_getport());

	return 0;
}

int compare_item(struct item *a, struct item *b) {
  return (
          (a->nameid == b->nameid) &&
          (a->identify == b->identify) &&
          (a->refine == b->refine) &&
          (a->attribute == b->attribute) &&
          (a->card[0] == b->card[0]) &&
          (a->card[1] == b->card[1]) &&
          (a->card[2] == b->card[2]) &&
          (a->card[3] == b->card[3]));
}
