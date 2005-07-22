// $Id: char.c,v 1.16 2004/09/23 18:31:16 MouseJstr Exp $
// original : char2.c 2003/03/14 11:58:35 Rev.1.5
//
// original code from athena
// SQL conversion by Jioh L. Jung
// TXT 1.105
#include "base.h"
#include "core.h"
#include "socket.h"
#include "utils.h"
#include "strlib.h"
#include "db.h"
#include "malloc.h"
#include "showmsg.h"
#include "timer.h"
#include "mmo.h"
#include "version.h"
#include "dbaccess.h"

#include "char.h"
#include "inter.h"
#include "itemdb.h"
#include "int_pet.h"
#include "int_guild.h"
#include "int_party.h"
#include "int_storage.h"


static struct dbt *char_db_;

char char_db[256] = "char";
char cart_db[256] = "cart_inventory";
char inventory_db[256] = "inventory";
char charlog_db[256] = "charlog";
char storage_db[256] = "storage";
char interlog_db[256] = "interlog";
char reg_db[256] = "global_reg_value";
char skill_db[256] = "skill";
char memo_db[256] = "memo";
char guild_db[256] = "guild";
char guild_alliance_db[256] = "guild_alliance";
char guild_castle_db[256] = "guild_castle";
char guild_expulsion_db[256] = "guild_expulsion";
char guild_member_db[256] = "guild_member";
char guild_position_db[256] = "guild_position";
char guild_skill_db[256] = "guild_skill";
char guild_storage_db[256] = "guild_storage";
char party_db[256] = "party";
char pet_db[256] = "pet";
char login_db[256] = "login";
char friend_db[256] = "friends";
int db_use_sqldbs;

char login_db_account_id[32] = "account_id";
char login_db_level[32] = "level";


char *SQL_CONF_NAME = "conf/inter_athena.conf";

struct mmo_map_server server[MAX_MAP_SERVERS];

int login_fd =-1;
int char_fd = -1;
char userid[24];
char passwd[24];
char server_name[20];
char wisp_server_name[24] = "Server";



netaddress	loginaddress(ipaddress::GetSystemIP(0), 6900);	 // first lanip as default
ipset		charaddress(6121);								 // automatic setup as default

int char_maintenance;
int char_new;
int char_new_display;
int party_modus=0;			// party modus, 0 break on leader leave, 1 successor takes over
int name_ignoring_case = 0; // Allow or not identical name for characters but with a different case by [Yor]
int char_name_option = 0; // Option to know which letters/symbols are authorised in the name of a character (0: all, 1: only those in char_name_letters, 2: all EXCEPT those in char_name_letters) by [Yor]
char char_name_letters[1024] = ""; // list of letters/symbols used to authorise or not a name of a character. by [Yor]
int char_per_account = 0; //Maximum charas per account (default unlimited) [Sirius]


int log_char = 1;	// loggin char or not [devil]
int log_inter = 1;	// loggin inter or not [devil]

char unknown_char_name[24] = "Unknown";
char db_path[1024]="db";

struct char_session_data{
	unsigned long account_id;
	unsigned long login_id1;
	unsigned long login_id2;
	unsigned char sex;
	unsigned long found_char[9];
	char email[40]; // e-mail (default: a@a.com) by [Yor]
	time_t connect_until_time; // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
};

#define AUTH_FIFO_SIZE 256
struct {
	unsigned long account_id;
	unsigned long char_id;
	unsigned long login_id1;
	unsigned long login_id2;
	unsigned long client_ip;
	int char_pos;
	int delflag;
	unsigned char sex;
	time_t connect_until_time; // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
} auth_fifo[AUTH_FIFO_SIZE];
int auth_fifo_pos = 0;

int check_ip_flag = 1; // It's to check IP of a player between char-server and other servers (part of anti-hacking system)


struct mmo_charstatus *char_dat;
int char_num,char_max;
int max_connect_user = 0;
int gm_allow_level = 99;
int autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
int start_zeny = 500;
int start_weapon = 1201;
int start_armor = 2301;

// check for exit signal
// 0 is saving complete
// other is char_id
unsigned int save_flag = 0;

// start point (you can reset point on conf file)
struct point start_point = {"new_1-1.gat", 53, 111};

struct gm_account *gm_account = NULL;
size_t GM_num = 0;

int console = 0;


//-------------------------------------------------
// Set Character online/offline [Wizputer]
//-------------------------------------------------

void set_char_online(unsigned long char_id, unsigned long account_id)
{
    if ( !session_isActive(login_fd) )
		return;

    if ( char_id != 99 ) {
        sql_query("UPDATE `%s` SET `online`='1' WHERE `char_id`='%ld'",char_db,char_id);
    }

	WFIFOW(login_fd,0) = 0x272b;
	WFIFOL(login_fd,2) = account_id;
	WFIFOSET(login_fd,6);

}

void set_all_offline(void)
{
    if ( !session_isActive(login_fd) )
		return;

	sql_query("SELECT `account_id` FROM `%s` WHERE `online`='1'",char_db);
	if (sql_res && session_isActive(login_fd) )
	{
	    while(sql_fetch_row())
		{
	        ShowMessage("send user offline: %d\n",atoi(sql_row[0]));
	            WFIFOW(login_fd,0) = 0x272c;
	            WFIFOL(login_fd,2) = atoi(sql_row[0]);
	            WFIFOSET(login_fd,6);
            }
       }

   	sql_free();
    sql_query("UPDATE `%s` SET `online`='0' WHERE `online`='1'", char_db);
}

void set_char_offline(unsigned long char_id, unsigned long account_id)
{
    struct mmo_charstatus *cp;

    if ( char_id == 99 )
        sql_query("UPDATE `%s` SET `online`='0' WHERE `account_id`='%ld'", char_db, account_id);
	else {
		cp = (struct mmo_charstatus*)numdb_search(char_db_,char_id);
		if (cp != NULL) {
			aFree(cp);
			numdb_erase(char_db_,char_id);
		}

		sql_query("UPDATE `%s` SET `online`='0' WHERE `char_id`='%ld'", char_db, char_id);
	}

    if ( session_isActive(login_fd) )
	{
   WFIFOW(login_fd,0) = 0x272c;
   WFIFOL(login_fd,2) = account_id;
   WFIFOSET(login_fd,6);
}
}

//----------------------------------------------------------------------
// Determine if an account (id) is a GM account
// and returns its level (or 0 if it isn't a GM account or if not found)
//----------------------------------------------------------------------
// Removed since nothing GM related goes on in the char server [CLOWNISIUS]
int isGM(unsigned long account_id) {
	size_t i;

	for(i = 0; i < GM_num; i++)
		if (gm_account[i].account_id == account_id)
			return gm_account[i].level;
	return 0;
}

void read_gm_account(void) {
	if (gm_account != NULL)
		aFree(gm_account);
	GM_num = 0;

	sql_query("SELECT `%s`,`%s` FROM `%s`",login_db_account_id,login_db_level,login_db);
	if (sql_res) {
		gm_account = (struct gm_account*)aCalloc(sizeof(struct gm_account) * (int)sql_num_rows(), 1);
		while (sql_fetch_row()) {
			gm_account[GM_num].account_id = atoi(sql_row[0]);
			gm_account[GM_num].level = atoi(sql_row[1]);
			GM_num++;
		}
	}

	sql_free();
	//mapif_send_gmaccounts();
}

// Insert friends list
int insert_friends(int char_id){
	int i;
	char *tmp_p = tmp_sql;

	tmp_p += sprintf(tmp_p, "REPLACE INTO `%s` (`id`, `account_id`",friend_db);

	for (i=0;i<MAX_FRIENDLIST;i++)
		tmp_p += sprintf(tmp_p, ", `friend_id%d`, `name%d`", i, i);

	tmp_p += sprintf(tmp_p, ") VALUES (NULL, '%d'", char_id);

	for (i=0;i<MAX_FRIENDLIST;i++)
		tmp_p += sprintf(tmp_p, ", '0', ''");

	tmp_p += sprintf(tmp_p, ")");

	sql_query(tmp_p,NULL);

return 1;
}

int compare_item(struct item *a, struct item *b) {
  return (
	  (a->id == b->id) &&
	  (a->nameid == b->nameid) &&
	  (a->amount == b->amount) &&
	  (a->equip == b->equip) &&
	  (a->identify == b->identify) &&
	  (a->refine == b->refine) &&
	  (a->attribute == b->attribute) &&
	  (a->card[0] == b->card[0]) &&
	  (a->card[1] == b->card[1]) &&
	  (a->card[2] == b->card[2]) &&
	  (a->card[3] == b->card[3]));
}

//=====================================================================================================
int char_base_cmp(struct mmo_charstatus *p, struct mmo_charstatus *cp){
	// Check if the Char data has changed since the last save
	// If different save the character data to SQL server
	return (!(
		(p->base_exp == cp->base_exp) &&
		(p->class_ == cp->class_) &&
	    (p->base_level == cp->base_level) &&
	    (p->job_level == cp->job_level) &&
	    (p->job_exp == cp->job_exp) &&
	    (p->zeny == cp->zeny) &&
	    (p->last_point.x == cp->last_point.x) &&
	    (p->last_point.y == cp->last_point.y) &&
	    (p->max_hp == cp->max_hp) &&
	    (p->hp == cp->hp) &&
	    (p->max_sp == cp->max_sp) &&
	    (p->sp == cp->sp) &&
	    (p->status_point == cp->status_point) &&
	    (p->skill_point == cp->skill_point) &&
	    (p->str == cp->str) &&
	    (p->agi == cp->agi) &&
	    (p->vit == cp->vit) &&
	    (p->int_ == cp->int_) &&
	    (p->dex == cp->dex) &&
	    (p->luk == cp->luk) &&
	    (p->option == cp->option) &&
	    (p->karma == cp->karma) &&
	    (p->manner == cp->manner) &&
	    (p->party_id == cp->party_id) &&
	    (p->guild_id == cp->guild_id) &&
	    (p->pet_id == cp->pet_id) &&
	    (p->hair == cp->hair) &&
	    (p->hair_color == cp->hair_color) &&
	    (p->clothes_color == cp->clothes_color) &&
	    (p->weapon == cp->weapon) &&
	    (p->shield == cp->shield) &&
	    (p->head_top == cp->head_top) &&
	    (p->head_mid == cp->head_mid) &&
	    (p->head_bottom == cp->head_bottom) &&
	    (p->partner_id == cp->partner_id) &&
	    (p->father_id == cp->father_id) &&
	    (p->mother_id == cp->mother_id) &&
	    (p->child_id == cp->child_id) &&
	    (p->fame_points == cp->fame_points)
	    ));
}

int mmo_char_tosql(unsigned long char_id, struct mmo_charstatus *p)
{
	size_t i=0;
	int party_exist,guild_exist;
//	int eqcount=1;
//	int noteqcount=1;
	int count = 0, l=0;
	int diff = 0;
	char temp_str[64]; //2x the value of the string before jstrescapecpy [Skotlex]
	char *tmp_p = tmp_sql;
	struct mmo_charstatus *cp;
	struct itemtmp mapitem[MAX_GUILD_STORAGE];

	if (char_id!=p->char_id) return 0;

	cp = (struct mmo_charstatus*)numdb_search(char_db_,char_id);

	if (cp == NULL) {
		cp = (struct mmo_charstatus *)aCalloc(sizeof(struct mmo_charstatus),1);
		numdb_insert(char_db_, char_id,cp);
	}

	save_flag = p->char_id;
	ShowMessage("("CL_BT_GREEN"%d"CL_NORM") %s \trequest save char data - ",char_id,char_dat[0].name);

	if (char_base_cmp(p,cp)) { //TBR

		//check to see if the party exists if no results, set to 0
		sql_query(
			"SELECT count(*) FROM `%s` WHERE `party_id` = '%d'",party_db, p->party_id);
		if (sql_res && sql_fetch_row()){
			if (!atoi(sql_row[0])){
				p->party_id=0;

			}
			sql_free();
		}

		//check to see if the guild exists if no results, set to 0
		sql_query(
			"SELECT count(*) FROM `%s` WHERE `guild_id` = '%d'",guild_db, p->guild_id);
		if (sql_res && sql_fetch_row()){
			if (!atoi(sql_row[0])){ p->guild_id=0;}
			sql_free();
		}


		//sql query
		//`char`( `char_id`,`account_id`,`char_num`,`name`,`class`,`base_level`,`job_level`,`base_exp`,`job_exp`,`zeny`, //9
		//`str`,`agi`,`vit`,`int`,`dex`,`luk`, //15
		//`max_hp`,`hp`,`max_sp`,`sp`,`status_point`,`skill_point`, //21
		//`option`,`karma`,`manner`,`party_id`,`guild_id`,`pet_id`, //27
		//`hair`,`hair_color`,`clothes_color`,`weapon`,`shield`,`head_top`,`head_mid`,`head_bottom`, //35
		//`last_map`,`last_x`,`last_y`,`save_map`,`save_x`,`save_y`)
			//ShowMessage("- Save char data to SQL!\n");
		sql_query("UPDATE `%s` SET `class`='%d', `base_level`='%d', `job_level`='%d',"
			"`base_exp`='%ld', `job_exp`='%ld', `zeny`='%ld',"
			"`max_hp`='%ld',`hp`='%ld',`max_sp`='%ld',`sp`='%ld',`status_point`='%d',`skill_point`='%d',"
			"`str`='%d',`agi`='%d',`vit`='%d',`int`='%d',`dex`='%d',`luk`='%d',"
			"`option`='%d',`karma`='%d',`manner`='%d',`party_id`='%ld',`guild_id`='%ld',`pet_id`='%ld',"
			"`hair`='%d',`hair_color`='%d',`clothes_color`='%d',`weapon`='%d',`shield`='%d',`head_top`='%d',`head_mid`='%d',`head_bottom`='%d',"
			"`last_map`='%s',`last_x`='%d',`last_y`='%d',`save_map`='%s',`save_x`='%d',`save_y`='%d',"
			"`partner_id`='%ld', `father`='%ld', `mother`='%ld', `child`='%ld', `fame`='%ld'"
			"WHERE  `account_id`='%ld' AND `char_id` = '%ld'",
			char_db, p->class_, p->base_level, p->job_level,
			p->base_exp, p->job_exp, p->zeny,
			p->max_hp, p->hp, p->max_sp, p->sp, p->status_point, p->skill_point,
			p->str, p->agi, p->vit, p->int_, p->dex, p->luk,
			p->option, MakeWord(p->karma, p->chaos), p->manner, p->party_id, p->guild_id, p->pet_id,
			p->hair, p->hair_color, p->clothes_color,
			p->weapon, p->shield, p->head_top, p->head_mid, p->head_bottom,
			p->last_point.map, p->last_point.x, p->last_point.y,
			p->save_point.map, p->save_point.x, p->save_point.y, p->partner_id,
			p->father_id, p->mother_id, p->child_id,
			p->fame_points,
			p->account_id, p->char_id);
		#ifdef CHAR_DEBUG
		printf("[ char data ]");
		#endif
	}


	// Check difference in Inventory from what's in memory
	//---------------------------------------------------------------------------------------------
	// If different, save to database
	if (memcmp(p->inventory, cp->inventory, sizeof(p->inventory))){
		l=0;
		// Delete here
		sql_query("DELETE from `%s` where `char_id` = '%d'",inventory_db, char_id);

		// Insert here
		sprintf(tmp_sql,"INSERT INTO `%s` (`char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3` ) VALUES ",
			inventory_db);

		for(i = 0; i<MAX_INVENTORY; i++) {
			if (p->inventory[i].nameid){
				if(l) sprintf(tmp_sql,"%s,",tmp_sql);
				sprintf(tmp_sql,"%s( '%ld','%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d' ) ",tmp_sql, char_id,
					p->inventory[i].nameid, p->inventory[i].amount, p->inventory[i].equip, p->inventory[i].identify, p->inventory[i].refine, p->inventory[i].attribute,
					p->inventory[i].card[0], p->inventory[i].card[1], p->inventory[i].card[2], p->inventory[i].card[3]);
				l++;
			}
		}
		//Do query if there was a complete query
		if (l){
			sql_query(tmp_sql,NULL);
			#ifdef CHAR_DEBUG
			printf("[ Inventory: %d ]",l);
			#endif
		}
		diff = 1;
	}

	// Check difference in Cart from what's in memory
	//---------------------------------------------------------------------------------------------
	// If different, save to database
	if (memcmp(p->cart, cp->cart, sizeof(p->cart) )){//TBR
		l=0;
		// Delete here
		sql_query("DELETE from `%s` where `char_id` = '%ld'",cart_db, char_id);

		// Insert here
		sprintf(tmp_sql,"INSERT INTO `%s` (`char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3` ) VALUES ",
			cart_db);

		for(i = 0; i<MAX_CART; i++) {
			if(p->cart[i].nameid){
				if(l) sprintf(tmp_sql,"%s,",tmp_sql);
				sprintf(tmp_sql,"%s( '%ld','%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d' ) ",tmp_sql, char_id,
					p->cart[i].nameid, p->cart[i].amount, p->cart[i].equip, p->cart[i].identify, p->cart[i].refine, p->cart[i].attribute,
					p->cart[i].card[0], p->cart[i].card[1], p->cart[i].card[2], p->cart[i].card[3]);
				l++;
			}
		}
		//Do query if there was a complete query
		if (l){
			sql_query(tmp_sql,NULL);
			#ifdef CHAR_DEBUG
			printf("[ Cart: %d ]",l);
			#endif
		}
		diff = 1;
	}

	#ifdef TEST_STORAGE
	// Check difference in Storage from what's in memory
	//---------------------------------------------------------------------------------------------
	// If different, save to database
	if(memcmp(p->storage, cp->storage, sizeof(p->storage) )){
		l=0;
		// Delete here
		sql_query("DELETE FROM `%s` WHERE `account_id` = '%ld'",storage_db,p->account_id);

		// Insert here
		sprintf(tmp_sql,"INSERT INTO `%s` (`account_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3` ) VALUES ",
			storage_db);
		for(i = 0; i<MAX_CART; i++) {
			if(p->cart[i].nameid){
				if(l) sprintf(tmp_sql,"%s,",tmp_sql);
				sprintf(tmp_sql,"%s( '%ld','%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d' ) ",tmp_sql, p->account_id,
					p->storage[i].nameid, p->storage[i].amount, p->storage[i].equip, p->storage[i].identify, p->storage[i].refine, p->storage[i].attribute,
					p->storage[i].card[0], p->storage[i].card[1], p->storage[i].card[2], p->storage[i].card[3]);
				l++;
			}
		}
		//Do query if there was a complete query
		if (l){
			sql_query(tmp_sql,NULL);
			#ifdef CHAR_DEBUG
			printf("[ Storage: %d ]",l);
			#endif
		}
		diff = 1;
	}
	#endif // TEST_STORAGE

	// Check difference in Memo from what's in memory
	//---------------------------------------------------------------------------------------------
	// If different, save to database
	if (memcmp(p->memo_point, cp->memo_point, sizeof(p->memo_point))) {//TBR
		l=0;
		// Delete here
		sql_query(
			"DELETE FROM `%s` WHERE `char_id`='%ld'",memo_db, p->char_id);

		//Insert here.
		sprintf(tmp_sql,"INSERT INTO `%s`(`char_id`,`memo_id`,`map`,`x`,`y`) VALUES ",memo_db);
		for(i=0;i<3;i++){
			if(p->memo_point[i].map[0]){
				if (l) sprintf(tmp_sql,"%s,",tmp_sql);
				sprintf(tmp_sql,"%s('%ld', '%d', '%s', '%d', '%d') ",tmp_sql,
					char_id, i, p->memo_point[i].map, p->memo_point[i].x, p->memo_point[i].y);
				l++;
			}
		}
		//Do query if there was a complete query
		if (l){
			sql_query(tmp_sql,NULL);
			#ifdef CHAR_DEBUG
			printf("[ memo: %d ]",l);
			#endif
		}
		diff = 1;
	}

	// Check difference in Skills from what's in memory
	//---------------------------------------------------------------------------------------------
	// If different, save to database
	if (memcmp(p->skill, cp->skill, sizeof(p->skill))) {//TBR
		l=0;
		// Delete here
		sql_query(
			"DELETE FROM `%s` WHERE `char_id`='%d'",skill_db, p->char_id);

		// Insert here
		sprintf(tmp_sql,"INSERT INTO `%s`(`char_id`, `id`, `lv`) VALUES ",skill_db);
		for(i=0;i<MAX_SKILL;i++){
			if (p->skill[i].id && p->skill[i].flag!=1){
				if (l) sprintf(tmp_sql,"%s,",tmp_sql);
				sprintf(tmp_sql,"%s('%ld', '%d','%d') ",tmp_sql,
					char_id, p->skill[i].id, (p->skill[i].flag==0)?p->skill[i].lv:p->skill[i].flag-2);
				l++;
			}
		}

		//Do query if there was a complete query
		if (l){
			sql_query(tmp_sql,NULL);
			#ifdef CHAR_DEBUG
			printf("[ skill: %d ]",l);
			#endif
		}
		diff = 1;
	}

	// Check difference in Global_Reg from what's in memory
	//---------------------------------------------------------------------------------------------
	// Ifdifferent, save to database
	if (memcmp(p->global_reg,cp->global_reg,sizeof(p->global_reg))) {//TBR
		l=0;
		// Delete here
		sql_query(
			"DELETE FROM `%s` WHERE `type`=3 AND `char_id`='%d'",reg_db, p->char_id);

		// Insert here
		sprintf(tmp_sql,"INSERT INTO `%s` (`char_id`, `str`, `value`) VALUES ",reg_db);
		for(i=0;i<p->global_reg_num;i++){
			if (p->global_reg[i].str){
				if(p->global_reg[i].value !=0){
					if (l) sprintf(tmp_sql,"%s,",tmp_sql);
					sprintf(tmp_sql,"%s('%ld', '%s','%d')",tmp_sql,
							 char_id, jstrescapecpy(temp_str,p->global_reg[i].str), p->global_reg[i].value);
					l++;
				}
			}
		}
		if (l){
			sql_query(tmp_sql,NULL);
			#ifdef CHAR_DEBUG
			printf("[ reg:%d ]",l);
			#endif
		}
		diff = 1;
	}
	// Check difference in friends from what's in memory
	//---------------------------------------------------------------------------------------------
	// If different, save to database
	if(memcmp(p->friends, cp->friends, sizeof(p->friends))){//TBR
		l=0;
		// Delete here
		sql_query(
			"DELETE FROM `%s` WHERE `char_id`= '%d'",friend_db,p->char_id);

		// Insert here
		sprintf(tmp_sql,"REPLACE INTO %s (`char_id`,`position`,`friend_id`) VALUES ",friend_db);
		for (i=0;i<20;i++){
			if (p->friends[i].char_id != 0){
				if (l) sprintf(tmp_sql,"%s,",tmp_sql);
				sprintf(tmp_sql,"%s('%ld','%d','%ld')",tmp_sql,
					p->char_id,i,p->friends[i].char_id);
				l++;
			}
		}
		if (l){
			sql_query(tmp_sql,NULL);
			#ifdef CHAR_DEBUG
			printf("[ friends:%d ]",l);
			#endif
		}
		diff = 1;
	}

	#ifdef CHAR_DEBUG
	printf("\n");
	#endif
	save_flag = 0;
	if (diff){
		// This makes sure memory is clean, then update CP with what P is.
		// Please fix up this area, as i know i need to clean memory before setting it
		// but i dont know if i should be inserting into the numdb
		// done at least =D
	   	memcpy(cp, p, sizeof(struct mmo_charstatus));
	}

	return 0;
}

// [Ilpalazzo-sama]
int memitemdata_to_sql(struct itemtmp mapitem[], int count, int char_id, int tableswitch)
{
	int i, flag, id, l;
	char *tablename;
	char selectoption[16];

	switch (tableswitch) {
	case TABLE_INVENTORY:
		tablename = inventory_db; // no need for sprintf here as *_db are char*.
		sprintf(selectoption,"char_id");
		break;
	case TABLE_CART:
		tablename = cart_db;
		sprintf(selectoption,"char_id");
		break;
	case TABLE_STORAGE:
		tablename = storage_db;
		sprintf(selectoption,"account_id");
		break;
	case TABLE_GUILD_STORAGE:
		tablename = guild_storage_db;
		sprintf(selectoption,"guild_id");
		break;
	default:
		ShowMessage("Invalid table name!\n");
		return 1;
	}
	//ShowMessage("Working Table : %s \n",tablename);



	l=0;
	// Delete here
	sql_query("DELETE from `%s` where `char_id` = '%d'",cart_db, char_id);

	// Prepare INSERT query here
	sprintf(tmp_sql,"INSERT INTO `%s` (`%s`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3` ) VALUES ",
			tablename, selectoption);

	for(i = 0; i<count; i++) {
		if(mapitem[i].nameid){
			if(l) sprintf(tmp_sql,"%s,",tmp_sql);
			sprintf(tmp_sql,"%s( '%ld','%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d' ) ",tmp_sql, char_id,
				mapitem[i].nameid,
				mapitem[i].amount,
				mapitem[i].equip,
				mapitem[i].identify,
				mapitem[i].refine,
				mapitem[i].attribute,
				mapitem[i].card[0],
				mapitem[i].card[1],
				mapitem[i].card[2],
				mapitem[i].card[3]);
			l++;
		}
	}
	//Do query if there was a complete query
	if (l){
		sql_query(tmp_sql,NULL);
		#ifdef CHAR_DEBUG
		printf("[ Cart: %d ]",l);
		#endif
	}

	return 0;
}
//=====================================================================================================
int mmo_char_fromsql(int char_id, struct mmo_charstatus *p, int online)
{
	size_t i, n, friends=0;
	memset(p, 0, sizeof(struct mmo_charstatus));
	char show_status[200];

	p->char_id = char_id;

	//`char`( `char_id`,`account_id`,`char_num`,`name`,`class`,`base_level`,`job_level`,`base_exp`,`job_exp`,`zeny`, //9
	//`str`,`agi`,`vit`,`int`,`dex`,`luk`, //15
	//`max_hp`,`hp`,`max_sp`,`sp`,`status_point`,`skill_point`, //21
	//`option`,`karma`,`manner`,`party_id`,`guild_id`,`pet_id`, //27
	//`hair`,`hair_color`,`clothes_color`,`weapon`,`shield`,`head_top`,`head_mid`,`head_bottom`, //35
	//`last_map`,`last_x`,`last_y`,`save_map`,`save_x`,`save_y`)
	//splite 2 parts. cause veeeery long SQL syntax

	p->char_id = char_id;
	//`char`( `char_id`,`account_id`,`char_num`,`name`,`class`,`base_level`,`job_level`,`base_exp`,`job_exp`,`zeny`, //9
	//`str`,`agi`,`vit`,`int`,`dex`,`luk`, //15
	//`max_hp`,`hp`,`max_sp`,`sp`,`status_point`,`skill_point`, //21
	//`option`,`karma`,`manner`,`party_id`,`guild_id`,`pet_id`, //27
	//`hair`,`hair_color`,`clothes_color`,`weapon`,`shield`,`head_top`,`head_mid`,`head_bottom`, //35
	//`last_map`,`last_x`,`last_y`,`save_map`,`save_x`,`save_y`)

	sql_query(
		"SELECT `char_id`,`account_id`,`char_num`,`name`,`class`,`base_level`,`job_level`,`base_exp`,`job_exp`,`zeny`,"
		"`str`,`agi`,`vit`,`int`,`dex`,`luk`, `max_hp`,`hp`,`max_sp`,`sp`,`status_point`,`skill_point`,"
		"`option`,`karma`,`manner`,`party_id`,`guild_id`,`pet_id`,`hair`,`hair_color`,"
		"`clothes_color`,`weapon`,`shield`,`head_top`,`head_mid`,`head_bottom`,"
		"`last_map`,`last_x`,`last_y`,`save_map`,`save_x`,`save_y`, `partner_id`,"
		"`father`,`mother`,`child`,`fame` "
		"FROM `%s` WHERE `char_id` = '%d'",char_db, char_id);

	if (sql_res && sql_fetch_row()) {
		p->char_id = atoi(sql_row[0]);
		p->account_id = atoi(sql_row[1]);
		p->char_num = atoi(sql_row[2]);
		strcpy(p->name, sql_row[3]);
		p->class_ = atoi(sql_row[4]);
		p->base_level = atoi(sql_row[5]);
		p->job_level = atoi(sql_row[6]);
		p->base_exp = atoi(sql_row[7]);
		p->job_exp = atoi(sql_row[8]);
		p->zeny = atoi(sql_row[9]);
		p->str = atoi(sql_row[10]);
		p->agi = atoi(sql_row[11]);
		p->vit = atoi(sql_row[12]);
		p->int_ = atoi(sql_row[13]);
		p->dex = atoi(sql_row[14]);
		p->luk = atoi(sql_row[15]);
		p->max_hp = atoi(sql_row[16]);
		p->hp = atoi(sql_row[17]);
		p->max_sp = atoi(sql_row[18]);
		p->sp = atoi(sql_row[19]);
		p->status_point = atoi(sql_row[20]);
		p->skill_point = atoi(sql_row[21]);
		p->option = atoi(sql_row[22]);
        p->karma = atoi(sql_row[23]);
        p->manner = atoi(sql_row[24]);

        p->party_id = atoi(sql_row[25]);
        p->guild_id = atoi(sql_row[26]);

        p->pet_id = atoi(sql_row[27]);
		p->hair = atoi(sql_row[28]);
        p->hair_color = atoi(sql_row[29]);
        p->clothes_color = atoi(sql_row[30]);
        p->weapon = atoi(sql_row[31]);
        p->shield = atoi(sql_row[32]);
		p->head_top = atoi(sql_row[33]);
        p->head_mid = atoi(sql_row[34]);
        p->head_bottom = atoi(sql_row[35]);
		// Check if the last_point is bugged or not.
		if (sql_row[36][0] == '\0' || atoi(sql_row[37]) == 0 || atoi(sql_row[38]) == '\0'){
			#ifdef CHAR_DEBUG
			printf("[ %s has no last point ]",p->name);
			#endif
			memcpy(&p->last_point, &start_point, sizeof(start_point));
		} else {
			strcpy(p->last_point.map,sql_row[36]);
        	p->last_point.x = atoi(sql_row[37]);
        	p->last_point.y = atoi(sql_row[38]);
		}
		// Check if the save_point is bugged or not.
		if (sql_row[39][0] == '\0' || atoi(sql_row[40]) == 0 || atoi(sql_row[41]) == 0 ){
			#ifdef CHAR_DEBUG
			printf("[ %s has no save point ]",p->name);
			#endif
			memcpy(&p->save_point, &start_point, sizeof(start_point));
		} else {
        	strcpy(p->save_point.map,sql_row[39]);
        	p->save_point.x = atoi(sql_row[40]);
        	p->save_point.y = atoi(sql_row[41]);
		}
		p->partner_id = atoi(sql_row[42]);
		p->father_id = atoi(sql_row[43]);
		p->mother_id = atoi(sql_row[44]);
		p->child_id = atoi(sql_row[45]);
		p->fame_points = atoi(sql_row[46]);


		//free sql result.
		sql_free();

		#ifdef CHAR_DEBUG
		printf("[ char data ]");
		#endif

	}


	//`inventory` (`id`,`char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3`)
	//read inventory
	sql_query("SELECT `id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3`"
		"FROM `%s` WHERE `char_id`='%d'",inventory_db, char_id); // TBR
	if (sql_res) {
		for(i=0;sql_fetch_row();i++){
			p->inventory[i].id = atoi(sql_row[0]);
			p->inventory[i].nameid = atoi(sql_row[1]);
			p->inventory[i].amount = atoi(sql_row[2]);
			p->inventory[i].equip = atoi(sql_row[3]);
			p->inventory[i].identify = atoi(sql_row[4]);
			p->inventory[i].refine = atoi(sql_row[5]);
			p->inventory[i].attribute = atoi(sql_row[6]);
			p->inventory[i].card[0] = atoi(sql_row[7]);
			p->inventory[i].card[1] = atoi(sql_row[8]);
			p->inventory[i].card[2] = atoi(sql_row[9]);
			p->inventory[i].card[3] = atoi(sql_row[10]);
		}

		//free sql result.
		sql_free();

		#ifdef CHAR_DEBUG
		printf("[ Inventory ]");
		#endif
	}

	//`cart_inventory` (`id`,`char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3`)
	//read cart.
	sql_query(
		"SELECT `id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3` "
		"FROM `%s` WHERE `char_id`='%d'",cart_db, char_id);
	if (sql_res) {
		for(i=0;sql_fetch_row();i++){
			p->cart[i].id = atoi(sql_row[0]);
			p->cart[i].nameid = atoi(sql_row[1]);
			p->cart[i].amount = atoi(sql_row[2]);
			p->cart[i].equip = atoi(sql_row[3]);
			p->cart[i].identify = atoi(sql_row[4]);
			p->cart[i].refine = atoi(sql_row[5]);
			p->cart[i].attribute = atoi(sql_row[6]);
			p->cart[i].card[0] = atoi(sql_row[7]);
			p->cart[i].card[1] = atoi(sql_row[8]);
			p->cart[i].card[2] = atoi(sql_row[9]);
			p->cart[i].card[3] = atoi(sql_row[10]);
		}

		//free sql result.
		sql_free();

		#ifdef CHAR_DEBUG
		printf("[ Cart ]");
		#endif
	}

	#ifdef TEST_STORAGE
	// storage {`account_id`/`id`/`nameid`/`amount`/`equip`/`identify`/`refine`/`attribute`/`card0`/`card1`/`card2`/`card3`}
	sql_query(
		"SELECT `id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3` "
		"FROM `%s` WHERE `account_id`='%d'",storage_db, p->account_id);
	p->storage_amount = 0;

	if (sql_res) {
		while(sql_fetch_row()) {
			p->storage_[i].id= atoi(sql_row[0]);
			p->storage_[i].nameid= atoi(sql_row[1]);
			p->storage_[i].amount= atoi(sql_row[2]);
			p->storage_[i].equip= atoi(sql_row[3]);
			p->storage_[i].identify= atoi(sql_row[4]);
			p->storage_[i].refine= atoi(sql_row[5]);
			p->storage_[i].attribute= atoi(sql_row[6]);
			p->storage_[i].card[0]= atoi(sql_row[7]);
			p->storage_[i].card[1]= atoi(sql_row[8]);
			p->storage_[i].card[2]= atoi(sql_row[9]);
			p->storage_[i].card[3]= atoi(sql_row[10]);
			p->storage_amount = ++i;
		}
		sql_free();
		#ifdef CHAR_DEBUG
		printf("[ Storage ]");
		#endif

	}
	#endif // TEST_STORAGE

	//`memo` (`memo_id`,`char_id`,`map`,`x`,`y`)
	//read memo data
	sql_query("SELECT `map`,`x`,`y` FROM `%s` WHERE `char_id`='%d'",memo_db, char_id);
	if (sql_res) {
		for(i=0;sql_fetch_row() && i < 3;i++){
			strcpy (p->memo_point[i].map,sql_row[0]);
			p->memo_point[i].x=atoi(sql_row[1]);
			p->memo_point[i].y=atoi(sql_row[2]);
		}

		//free sql result.
		sql_free();

		#ifdef CHAR_DEBUG
		printf("[ memo ]");
		#endif
	}


	//read skill
	//`skill` (`char_id`, `id`, `lv`)
	sql_query("SELECT `id`, `lv` FROM `%s` WHERE `char_id`='%d'",skill_db, char_id);
	if (sql_res) {
		for(i=0;sql_fetch_row();i++){
			n = atoi(sql_row[0]);
			p->skill[n].id = n; //memory!? shit!.
			p->skill[n].lv = atoi(sql_row[1]);
		}
		sql_free();

		#ifdef CHAR_DEBUG
		printf("[ skill ]");
		#endif
	}

	//global_reg
	//`global_reg_value` (`char_id`, `str`, `value`)
	sql_query("SELECT `str`, `value` FROM `%s` WHERE `type`=3 AND `char_id`='%d'",reg_db, char_id);

	if (sql_res) {
		for(i=0;sql_fetch_row();i++){
			strcpy (p->global_reg[i].str, sql_row[0]);
			p->global_reg[i].value = atoi(sql_row[1]);
		}
		sql_free();

		#ifdef CHAR_DEBUG
		printf("[ Global Reg ]");
		#endif
	}
	p->global_reg_num=i;

	//Read Friends
	// `friends` (`friend_id`,`friend_name`)
	sql_query("SELECT f.friend_id, c.name FROM `%s` f, `%s` c WHERE f.char_id='%d' AND f.friend_id=c.char_id",friend_db,char_db, char_id);
	if (sql_res) {
		for (i=0;sql_fetch_row();i++){
			p->friends[i].char_id = atoi(sql_row[0]);
			strcpy(p->friends[i].name,sql_row[1]);
		}
		sql_free();

		#ifdef CHAR_DEBUG
		printf("[ Friends ]");
		#endif
	}


	if (online)
	{
		set_char_online(char_id,p->account_id);
	}
	ShowMessage(show_status);	//ok. all data load successfuly!


	struct mmo_charstatus *cp;
	cp = (struct mmo_charstatus*)numdb_search(char_db_,char_id);
	if(!cp)
	{
	cp = (struct mmo_charstatus *) aMalloc(sizeof(struct mmo_charstatus));
		numdb_insert(char_db_, char_id, cp);
	}
   	memcpy(cp, p, sizeof(struct mmo_charstatus));

	return 1;
}
//==========================================================================================================
int mmo_char_sql_init(void) {
	int charcount;

        char_db_=numdb_init();

	ShowMessage("init start.......\n");
	// memory initialize
	// no need to set twice size in this routine. but some cause segmentation error. :P
	ShowMessage("initializing char memory...(%d byte)\n",sizeof(struct mmo_charstatus)*2);
	CREATE(char_dat, struct mmo_charstatus, 2);
	memset(char_dat, 0, sizeof(struct mmo_charstatus)*2);

	sql_query("SELECT `char_id` FROM `%s`", char_db);
	if(sql_res){
		charcount = (int)mysql_num_rows(sql_res);
		ShowMessage("total char data -> '%d'.......\n", charcount);
		mysql_free_result(sql_res);
	}else{
		ShowMessage("total char data -> '0'.......\n");
	}

	if(char_per_account == 0){
		ShowMessage("Chars per Account: 'Unlimited'.......\n");
	}else{
		ShowMessage("Chars per Account: '%d'.......\n", char_per_account);
	}

	sql_query("UPDATE `%s` SET `online` = '0'", char_db);//fixed the on start 0 entrys!
	sql_query("UPDATE `%s` SET `online` = '0'", guild_member_db);//fixed the 0 entrys in start ..
	sql_query("UPDATE `%s` SET `connect_member` = '0'", guild_db);//fixed the 0 entrys in start.....



	//the 'set offline' part is now in check_login_conn ...
	//if the server connects to loginserver
	//it will dc all off players
	//and send the loginserver the new state....


	// WHY DC ALL PLAYERS>?!?!?!??! as long as there is a char server there is no data lost!
	// why in heavens.. or in hell would someone DC all players due to a login DC? [CLOWNISIUS]

	ShowMessage("init end.......\n");

	return 0;
}

//==========================================================================================================

int make_new_char_sql(int fd, unsigned char *dat)
{
	struct char_session_data *sd;
	char t_name[64];
	size_t i;
	int char_id, temp;

	jstrescapecpy(t_name, (char*)dat);
	// disabled until fixed >.>
	// Note: escape characters should be added to jstrescape()!
	t_name[24-1] = '\0';

	if (!session_isValid(fd) || !(sd = (struct char_session_data*)session[fd]->session_data))
		return -2;
	ShowMessage("[CHAR] Add - ");

	//check for charcount (maxchars) :)
	if(char_per_account != 0)
	{
		sql_query("SELECT `account_id` FROM `%s` WHERE `account_id` = '%ld'", char_db, sd->account_id);
		if(sql_res){	//ok
			if(sql_num_rows() >= char_per_account){	//hehe .. limit exceeded :P
				ShowMessage("fail (aid: %d), charlimit exceeded.\n", sd->account_id);
				sql_free();
				return -2;
			}
			sql_free();
		}
	}

	// Check Authorised letters/symbols in the name of the character
	if (char_name_option == 1)
	{	// only letters/symbols in char_name_letters are authorised
		for (i = 0; i < strlen((const char*)dat); i++)
			if (strchr(char_name_letters, dat[i]) == NULL)
				return -2;
	} else if (char_name_option == 2)
	{	// letters/symbols in char_name_letters are forbidden
		for (i = 0; i < strlen((const char*)dat); i++)
			if (strchr(char_name_letters, dat[i]) != NULL)
				return -2;
	}
	// else, all letters/symbols are authorised (except control char removed before)

	//check stat error
	if ((dat[24]+dat[25]+dat[26]+dat[27]+dat[28]+dat[29]!=6*5 ) || // stats
		(dat[30] >= 9) || // slots (dat[30] can not be negativ)
		(dat[33] <= 0) || (dat[33] >= 24) || // hair style
		(dat[31] >= 9)) { // hair color (dat[31] can not be negativ)
		if (log_char) {
			// char.log to charlog
			sql_query("INSERT INTO `%s` (`time`, `char_msg`,`account_id`,`char_num`,`name`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,`hair`,`hair_color`)"
				"VALUES (NOW(), '%s', '%ld', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
				charlog_db,"make new char error", sd->account_id, dat[30], dat, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[33], dat[31]);
		}
		ShowError("fail (aid: %d), stats error(bot cheat?!)\n", sd->account_id);
		return -2;
	} // for now we have checked: stat points used <31, char slot is less then 9, hair style/color values are acceptable

	// check individual stat value
	for(i = 24; i <= 29; i++) {
		if (dat[i] < 1 || dat[i] > 9) {
			if (log_char) {
				// char.log to charlog
				sql_query("INSERT INTO `%s` (`time`, `char_msg`,`account_id`,`char_num`,`name`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,`hair`,`hair_color`)"
					"VALUES (NOW(), '%s', '%ld', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
					charlog_db,"make new char error", sd->account_id, dat[30], dat, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[33], dat[31]);
			}
			ShowError("fail (aid: %d), stats error(bot cheat?!)\n", sd->account_id);
				return -2;
		}
	} // now we know that every stat has proper value but we have to check if str/int agi/luk vit/dex pairs are correct

	if( ((dat[24]+dat[27]) > 10) || ((dat[25]+dat[29]) > 10) || ((dat[26]+dat[28]) > 10) ) {
		if (log_char) {
			// char.log to charlog
			sql_query("INSERT INTO `%s` (`time`, `char_msg`,`account_id`,`char_num`,`name`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,`hair`,`hair_color`)"
					"VALUES (NOW(), '%s', '%ld', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
					charlog_db,"make new char error", sd->account_id, dat[30], dat, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[33], dat[31]);
		}
		ShowError("fail (aid: %d), stats error(bot cheat?!)\n", sd->account_id);
		return -2;
	} // now when we have passed all stat checks

	if (log_char) {
		// char.log to charlog
		sql_query("INSERT INTO `%s`(`time`, `char_msg`,`account_id`,`char_num`,`name`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,`hair`,`hair_color`)"
			"VALUES (NOW(), '%s', '%ld', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
			charlog_db,"make new char", sd->account_id, dat[30], dat, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[33], dat[31]);
	}
	//ShowMessage("make new char %d-%d %s %d, %d, %d, %d, %d, %d - %d, %d" RETCODE,
	//	fd, dat[30], dat, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[33], dat[31]);

	//Check Name (already in use?)
	sprintf(tmp_sql, "SELECT `name` FROM `%s` WHERE `name` = '%s'",char_db, t_name);
	if(sql_res)
	{

		if (sql_num_rows() > 0)
		{
			sql_free();
			ShowMessage("fail, charname already in use\n");
			return -1;
		}
		sql_free();
	}

	// check char slot.
	sql_query("SELECT `account_id`, `char_num` FROM `%s` WHERE `account_id` = '%ld' AND `char_num` = '%d'",char_db, sd->account_id, dat[30]);
	if(sql_res)
	{
		if (sql_num_rows() > 0)
		{
			sql_free();
			ShowMessage("fail (aid: %ld, slot: %d), slot already in use\n", sd->account_id, dat[30]);
			return -2;
		}
		sql_free();
	}

	//New Querys [Sirius]
	//Insert the char to the 'chardb' ^^
	sql_query("INSERT INTO `%s` (`account_id`, `char_num`, `name`, `zeny`, `str`, `agi`, `vit`, `int`, `dex`, `luk`, `max_hp`, `hp`, `max_sp`, `sp`, `hair`, `hair_color`, `last_map`, `last_x`, `last_y`, `save_map`, `save_x`, `save_y`) VALUES ('%ld', '%d', '%s', '%d',  '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d','%d', '%d','%d', '%d', '%s', '%d', '%d', '%s', '%d', '%d')", char_db, sd->account_id , dat[30] , t_name, start_zeny, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], (40 * (100 + dat[26])/100) , (40 * (100 + dat[26])/100 ),  (11 * (100 + dat[27])/100), (11 * (100 + dat[27])/100), dat[33], dat[31], start_point.map, start_point.x, start_point.y, start_point.map, start_point.x, start_point.y);

	//Now we need the charid from sql!
	sql_query("SELECT `char_id` FROM `%s` WHERE `account_id` = '%ld' AND `char_num` = '%d' AND `name` = '%s'", char_db, sd->account_id , dat[30] , t_name);
	if(sql_res && sql_fetch_row()){
		char_id = atoi(sql_row[0]); //char id :)
		sql_free();
	}


	//Give the char the default items
	//`inventory` (`id`,`char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3`)
	if (start_weapon > 0) //add Start Weapon (Knife?)
		sql_query("INSERT INTO `%s` (`char_id`,`nameid`, `amount`, `equip`, `identify`) VALUES ('%d', '%d', '%d', '%d', '%d')", inventory_db, char_id, start_weapon,1,0x02,1);

	if (start_armor > 0) //Add default armor (cotton shirt?)
		sql_query("INSERT INTO `%s` (`char_id`,`nameid`, `amount`, `equip`, `identify`) VALUES ('%d', '%d', '%d', '%d', '%d')", inventory_db, char_id, start_armor,1,0x10,1);

	//ShowMessage("making new char success - id:(\033[1;32m%d\033[0m\tname:\033[1;32%s\033[0m\n", char_id, t_name);
	ShowMessage("success, aid: %d, cid: %d, slot: %d, name: %s\n", sd->account_id, char_id, dat[30], t_name);
	return char_id;
}

//==========================================================================================================

int count_users(void) {
	int i, users;

	if( session_isActive(login_fd) )
	{
		users = 0;
		for(i = 0; i < MAX_MAP_SERVERS; i++)
		{
			if (server[i].fd >= 0)
			{
				users += server[i].users;
			}
		}
		return users;
	}
	return 0;
}

int mmo_char_send006b(int fd, struct char_session_data *sd)
{
	int i, j, found_num = 0;
	struct mmo_charstatus *p = NULL;
	const int offset = 24;

    if ( !session_isActive(fd) )
		return 0;


	ShowMessage("mmo_char_send006b start.. (account:%d)\n",sd->account_id);

    set_char_online(99,sd->account_id);

	//search char.
	sql_query("SELECT `char_id` FROM `%s` WHERE `account_id` = '%ld'",char_db, sd->account_id);
	if (sql_res) {
		found_num = sql_num_rows();
		ShowMessage("number of chars: %d\n", found_num);
		i = 0;
		while(sql_fetch_row()) {
			sd->found_char[i] = atoi(sql_row[0]);
			i++;
		}
		sql_free();
	}


	for(i = found_num; i < 9; i++)
		sd->found_char[i] = 0xFFFFFFFF;

	memset(WFIFOP(fd, 0), 0, offset + found_num * 106);
	WFIFOW(fd, 0) = 0x6b;
	WFIFOW(fd, 2) = offset + found_num * 106;

	ShowMessage("("CL_BT_RED"%d"CL_NORM") Request Char Data:\n",sd->account_id);

	for(i = 0; i < found_num; i++) {
		mmo_char_fromsql(sd->found_char[i], char_dat, 0);

		p = &char_dat[0];

		j = offset + (i * 106); // increase speed of code

		WFIFOL(fd,j) = p->char_id;
		WFIFOL(fd,j+4) = p->base_exp;
		WFIFOL(fd,j+8) = p->zeny;
		WFIFOL(fd,j+12) = p->job_exp;
		WFIFOL(fd,j+16) = p->job_level;

		WFIFOL(fd,j+20) = 0;
		WFIFOL(fd,j+24) = 0;
		WFIFOL(fd,j+28) = p->option;

		WFIFOL(fd,j+32) = p->karma;
		WFIFOL(fd,j+36) = p->manner;

		WFIFOW(fd,j+40) = p->status_point;
		WFIFOW(fd,j+42) = (unsigned short)((p->hp > 0x7fff) ? 0x7fff : p->hp);
		WFIFOW(fd,j+44) = (unsigned short)((p->max_hp > 0x7fff) ? 0x7fff : p->max_hp);
		WFIFOW(fd,j+46) = (unsigned short)((p->sp > 0x7fff) ? 0x7fff : p->sp);
		WFIFOW(fd,j+48) = (unsigned short)((p->max_sp > 0x7fff) ? 0x7fff : p->max_sp);
		WFIFOW(fd,j+50) = DEFAULT_WALK_SPEED; // p->speed;
		WFIFOW(fd,j+52) = p->class_;
		WFIFOW(fd,j+54) = p->hair;

		// pecopeco knights/crusaders crash fix
		if (p->class_ == 13 || p->class_ == 21 ||
			p->class_ == 4014 || p->class_ == 4022)
			WFIFOW(fd,j+56) = 0;
		else WFIFOW(fd,j+56) = p->weapon;

		WFIFOW(fd,j+58) = p->base_level;
		WFIFOW(fd,j+60) = p->skill_point;
		WFIFOW(fd,j+62) = p->head_bottom;
		WFIFOW(fd,j+64) = p->shield;
		WFIFOW(fd,j+66) = p->head_top;
		WFIFOW(fd,j+68) = p->head_mid;
		WFIFOW(fd,j+70) = p->hair_color;
		WFIFOW(fd,j+72) = p->clothes_color;

		memcpy(WFIFOP(fd,j+74), p->name, 24);

		WFIFOB(fd,j+98) = (p->str > 255) ? 255 : p->str;
		WFIFOB(fd,j+99) = (p->agi > 255) ? 255 : p->agi;
		WFIFOB(fd,j+100) = (p->vit > 255) ? 255 : p->vit;
		WFIFOB(fd,j+101) = (p->int_ > 255) ? 255 : p->int_;
		WFIFOB(fd,j+102) = (p->dex > 255) ? 255 : p->dex;
		WFIFOB(fd,j+103) = (p->luk > 255) ? 255 : p->luk;
		WFIFOB(fd,j+104) = p->char_num;
	}
	WFIFOSET(fd,WFIFOW(fd,2));
//	ShowMessage("mmo_char_send006b end..\n");
	return 0;
}

int parse_tologin(int fd)
{
	size_t i;
	struct char_session_data *sd;

	// only login-server can have an access to here.
	// so, if it isn't the login-server, we disconnect the session.
	if(fd != login_fd)
	{
		session_Remove(fd);
		return 0;
	}
	// else it is the login
	if( !session_isActive(fd) )
	{	// login is disconnecting
		ShowMessage("Connection to login-server dropped (connection #%d).\n", fd);
			login_fd = -1;
		session_Remove(fd);
		return 0;
	}

	sd = (struct char_session_data*)session[fd]->session_data;

	// hehe. no need to set user limit on SQL version. :P
	// but char limitation is good way to maintain server. :D
	while(RFIFOREST(fd) >= 2) {
//		ShowMessage("parse_tologin : %d %d %x\n", fd, RFIFOREST(fd), (unsigned short)RFIFOW(fd, 0));

		switch(RFIFOW(fd, 0)){
		case 0x2711:
			if (RFIFOREST(fd) < 3)
				return 0;
			if (RFIFOB(fd, 2)) {
				//ShowMessage("connect login server error : %d\n", (unsigned char)RFIFOB(fd, 2));
				ShowMessage("Can not connect to login-server.\n");
				ShowMessage("The server communication passwords (default s1/p1) is probably invalid.\n");
				ShowMessage("Also, please make sure your login db has the username/password present and the sex of the account is S.\n");
				ShowMessage("If you changed the communication passwords, change them back at map_athena.conf and char_athena.conf\n");
			}
			else
			{
				ShowStatus("Connected to login-server (connection #%d).\n", fd);
                set_all_offline();
				// if no map-server already connected, display a message...
				for(i = 0; i < MAX_MAP_SERVERS; i++)
					if(server[i].fd >= 0 && server[i].map[0][0]) // if map-server online and at least 1 map
						break;
				if (i == MAX_MAP_SERVERS)
					ShowStatus("Awaiting maps from map-server.\n");
			}
			RFIFOSKIP(fd, 3);
			break;

		case 0x2713:
			if(RFIFOREST(fd)<51)
				return 0;
			for(i = 0; i < fd_max; i++) {
				if (session[i] && (sd = (struct char_session_data*)session[i]->session_data) && sd->account_id == RFIFOL(fd,2)) {
					if (RFIFOB(fd,6) != 0) {
						WFIFOW(i,0) = 0x6c;
						WFIFOB(i,2) = 0x42;
						WFIFOSET(i,3);
					} else if (max_connect_user == 0 || count_users() < max_connect_user) {
//						if (max_connect_user == 0)
//							ShowMessage("max_connect_user (unlimited) -> accepted.\n");
//						else
//							ShowMessage("count_users(): %d < max_connect_user (%d) -> accepted.\n", count_users(), max_connect_user);
						sd->connect_until_time = (time_t)RFIFOL(fd,47);
						memcpy(sd->email, RFIFOP(fd, 7), 40);
						// send characters to player
						mmo_char_send006b(i, sd);
					} else if(isGM(sd->account_id) >= gm_allow_level) {
						sd->connect_until_time = (time_t)RFIFOL(fd,47);
						memcpy(sd->email, RFIFOP(fd, 7), 40);
						// send characters to player
						mmo_char_send006b(i, sd);
					} else {
						// refuse connection: too much online players
//						ShowMessage("count_users(): %d < max_connect_use (%d) -> fail...\n", count_users(), max_connect_user);
						WFIFOW(i,0) = 0x6c;
						WFIFOW(i,2) = 0;
						WFIFOSET(i,3);
					}
				}
			}
			RFIFOSKIP(fd,51);
			break;

		case 0x2717:
			if (RFIFOREST(fd) < 50)
				return 0;
			for(i = 0; i < fd_max; i++)
			{
				if (session[i] && (sd = (struct char_session_data*)session[i]->session_data))
				{
					if (sd->account_id == RFIFOL(fd,2))
					{
					sd->connect_until_time = (time_t)RFIFOL(fd,46);
					break;
					}
				}
			}
			RFIFOSKIP(fd,50);
			break;

		// login-server alive packet
		case 0x2718:
			if (RFIFOREST(fd) < 2)
				return 0;

			RFIFOSKIP(fd,2);
			break;

		// Receiving authentification from Freya-type login server (to avoid char->login->char)
		case 0x2719:
			if (RFIFOREST(fd) < 18)
				return 0;
			// to conserv a maximum of authentification, search if account is already authentified and replace it
			// that will reduce multiple connection too
			for(i = 0; i < AUTH_FIFO_SIZE; i++)
				if (auth_fifo[i].account_id == RFIFOL(fd,2))
					break;
			// if not found, use next value
			if (i == AUTH_FIFO_SIZE) {
				if (auth_fifo_pos >= AUTH_FIFO_SIZE)
					auth_fifo_pos = 0;
				i = auth_fifo_pos;
				auth_fifo_pos++;
			}
			//ShowMessage("auth_fifo set (auth #%d) - account: %d, secure: %08x-%08x\n", i, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10));
			auth_fifo[i].account_id = RFIFOL(fd,2);
			auth_fifo[i].char_id = 0;
			auth_fifo[i].login_id1 = RFIFOL(fd,6);
			auth_fifo[i].login_id2 = RFIFOL(fd,10);
			auth_fifo[i].delflag = 2; // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
			auth_fifo[i].char_pos = 0;
			auth_fifo[i].connect_until_time = 0; // unlimited/unknown time by default (not display in map-server)
			auth_fifo[i].client_ip = RFIFOLIP(fd,14);
			//auth_fifo[i].map_auth = 0;
			RFIFOSKIP(fd,18);
			break;

		case 0x2723:	// changesex reply (modified by [Yor])
			if (RFIFOREST(fd) < 7)
				return 0;
		  {
			unsigned long acc;
			unsigned char sex;
			unsigned char buf[16];

			acc = RFIFOL(fd,2);
			sex = RFIFOB(fd,6);
			RFIFOSKIP(fd, 7);
			#ifndef CLOWNPHOBIA // Clownphobia doesnt support this.
			if (acc > 0) {
				sql_query("SELECT `char_id`,`class`,`skill_point` FROM `%s` WHERE `account_id` = '%ld'",char_db, acc);
				if (sql_res) {
					int char_id, jobclass, skill_point, class_;
					sql_fetch_row();
					char_id = atoi(sql_row[0]);
					jobclass = atoi(sql_row[1]);
					skill_point = atoi(sql_row[2]);
					class_ = jobclass;
					sql_free();
					if (jobclass == 19 || jobclass == 20 ||
						jobclass == 4020 || jobclass == 4021 ||
						jobclass == 4042 || jobclass == 4043) {
						// job modification
						if (jobclass == 19 || jobclass == 20) {
							class_ = (sex) ? 19 : 20;
						} else if (jobclass == 4020 || jobclass == 4021) {
							class_ = (sex) ? 4020 : 4021;
						} else if (jobclass == 4042 || jobclass == 4043) {
							class_ = (sex) ? 4042 : 4043;
						}
						// remove specifical skills of classes 19,20 4020,4021 and 4042,4043
						sql_query("SELECT `lv` FROM `%s` WHERE `char_id` = '%d' AND `id` >= '315' AND `id` <= '330'",skill_db, char_id);
						if (sql_res) {
							while(sql_fetch_row()) {
								skill_point += atoi(sql_row[0]);
							}
							sql_free();
						}
						sql_query("DELETE FROM `%s` WHERE `char_id` = '%d' AND `id` >= '315' AND `id` <= '330'",skill_db, char_id);
					}
					// to avoid any problem with equipment and invalid sex, equipment is unequiped.
					sql_query("UPDATE `%s` SET `equip` = '0' WHERE `char_id` = '%d'",inventory_db, char_id);
					sql_query("UPDATE `%s` SET `class`='%d' , `skill_point`='%d' , `weapon`='0' , `shield='0' , `head_top`='0' , `head_mid`='0' , `head_bottom`='0' WHERE `char_id` = '%d'",char_db, class_, skill_point, char_id);
				}
			}
			for(i = 0; i < fd_max; i++) {
				if (session[i] && (sd = (struct char_session_data*)session[i]->session_data)) {
					if (sd->account_id == acc) {
						session_Remove(i);
						break;
					}
				}
			}

			WBUFW(buf,0) = 0x2b0d;
			WBUFL(buf,2) = acc;
			WBUFB(buf,6) = sex;

			mapif_sendall(buf, 7);
		  }
		  	#endif // CLOWNPHOBIA
			break;

		// account_reg2変更通知
		case 0x2729:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		  {
			struct global_reg reg[ACCOUNT_REG2_NUM];
			unsigned char buf[4096];
			int j, p, acc;
			acc = RFIFOL(fd,4);
			for(p = 8, j = 0; p < RFIFOW(fd,2) && j < ACCOUNT_REG2_NUM; p += 36, j++) {
				memcpy(reg[j].str, RFIFOP(fd,p), 32);
				reg[j].value = RFIFOL(fd,p+32);
			}
			// set_account_reg2(acc,j,reg);
			memcpy(buf,RFIFOP(fd,0), RFIFOW(fd,2));
			WBUFW(buf,0) = 0x2b11;
			mapif_sendall(buf, WBUFW(buf,2));
			RFIFOSKIP(fd, RFIFOW(fd,2));
//			ShowMessage("char: save_account_reg_reply\n");
		  }
			break;

		// State change of account/ban notification (from login-server) by [Yor]
		case 0x2731:
			if (RFIFOREST(fd) < 11)
				return 0;
			#ifndef CLOWNPHOBIA // Clownphobia doesnt support this.
			// send to all map-servers to disconnect the player
		  {
			unsigned char buf[16];
			WBUFW(buf,0) = 0x2b14;
			WBUFL(buf,2) = RFIFOL(fd,2);
			WBUFB(buf,6) = RFIFOB(fd,6); // 0: change of statut, 1: ban
			WBUFL(buf,7) = RFIFOL(fd,7); // status or final date of a banishment
			mapif_sendall(buf, 11);
		  }
			// disconnect player if online on char-server
			for(i = 0; i < fd_max; i++) {
				if (session[i] && (sd = (struct char_session_data*)session[i]->session_data)) {
					if (sd->account_id == RFIFOL(fd,2)) {
						session_Remove(i);
						break;
					}
				}
			}
			#endif // CLOWNPHOBIA

			RFIFOSKIP(fd,11);
			break;

		// Receive GM accounts [Freya login server packet by Yor]
		case 0x2733:
		// add test here to remember that the login-server is Freya-type
		// sprintf (login_server_type, "Freya");
			if (RFIFOREST(fd) < 7)
				return 0;
			#ifndef CLOWNPHOBIA // Clownphobia doesnt support this.
			{
				int new_level = 0;
				for(i = 0; i < GM_num; i++)
					if (gm_account[i].account_id == RFIFOL(fd,2)) {
						if (gm_account[i].level != RFIFOB(fd,6)) {
							gm_account[i].level = RFIFOB(fd,6);
							new_level = 1;
						}
						break;
					}
				// if not found, add it
				if (i == GM_num) {
					// limited to 4000, because we send information to char-servers (more than 4000 GM accounts???)
					// int (id) + int (level) = 8 bytes * 4000 = 32k (limit of packets in windows)
					if (((int)RFIFOB(fd,6)) > 0 && GM_num < 4000) {
						if (GM_num == 0) {
							gm_account = (struct gm_account*)aMalloc(sizeof(struct gm_account));
						} else {
							gm_account = (struct gm_account*)aRealloc(gm_account, sizeof(struct gm_account) * (GM_num + 1));
						}
						gm_account[GM_num].account_id = RFIFOL(fd,2);
						gm_account[GM_num].level = (int)RFIFOB(fd,6);
						new_level = 1;
						GM_num++;
						if (GM_num >= 4000)
							ShowMessage("***WARNING: 4000 GM accounts found. Next GM accounts are not readed.\n");
					}
				}
				if (new_level == 1) {
					ShowInfo("From login-server: receiving a GM account information (%ld: level %d).\n", (unsigned long)RFIFOL(fd,2), (unsigned char)RFIFOB(fd,6));
					mapif_send_gmaccounts();

					//create_online_files(); // not change online file for only 1 player (in next timer, that will be done
					// send gm acccounts level to map-servers
				}
			}
			#endif // CLOWNPHOBIA
			RFIFOSKIP(fd,7);
			break;

		default:
			ShowMessage("set eof.\n");
			session_Remove(fd);
			return 0;
		}
	}
	return 0;
}



int parse_frommap(int fd)
{
	size_t i, j;
	int id;

	for(id = 0; id < MAX_MAP_SERVERS; id++)
		if (server[id].fd == fd)
			break;
	if(id == MAX_MAP_SERVERS){
		session_Remove(fd);
		return 0;
	}

	// else it is the map server id
	if( !session_isActive(fd) ){
		ShowWarning("Map-server %d (session #%d) has disconnected.\n", id, fd);
		sql_query("DELETE FROM `ragsrvinfo` WHERE `index`='%d'", server[id].fd);
		server[id].fd = -1;
		session_Remove(fd);

		// inform the other map servers of the loss
		unsigned char buf[16384];
		WBUFW(buf,0) = 0x2b20;
		WBUFW(buf,2) = server[id].maps * 16 + 20;

		WBUFLIP(buf,4) = server[id].address.LANIP();
		WBUFLIP(buf,8) = server[id].address.LANMask();
		WBUFW(buf,12) = server[id].address.LANPort();
		WBUFLIP(buf,14) = server[id].address.WANIP();
		WBUFW(buf,18) = server[id].address.WANPort();

		for(i=20, j=0; j<server[id].maps; i+=16, j++)
			memcpy(RBUFP(buf,i), server[id].map[j], 16);
		mapif_sendallwos(fd, buf, server[id].maps * 16 + 20);

		return 0;
	}

	while(RFIFOREST(fd) >= 2) {
//		ShowMessage("parse_frommap : %d %d %x\n", fd, RFIFOREST(fd), (unsigned short)RFIFOW(fd,0));

		switch(RFIFOW(fd, 0)) {

		// map-server alive packet
		case 0x2718:
			if (RFIFOREST(fd) < 2)
				return 0;
			RFIFOSKIP(fd,2);
			break;

		case 0x2af7:
			RFIFOSKIP(fd,2);
			read_gm_account();
			break;

		case 0x2af8: // login as map-server; update ip addresses
		{
			if (RFIFOREST(fd) < 70)
				return 0;

			server[id].address = ipset(RFIFOLIP(fd,54), RFIFOLIP(fd,58), RFIFOW(fd,62),RFIFOLIP(fd,64), RFIFOW(fd,68));

			// send new ipset to mapservers for update
			unsigned char buf[16384];
			WBUFLIP(buf,4) = server[id].address.LANIP();
			WBUFLIP(buf,8) = server[id].address.LANMask();
			WBUFW(buf,12) = server[id].address.LANPort();
			WBUFLIP(buf,14) = server[id].address.WANIP();
			WBUFW(buf,18) = server[id].address.WANPort();

			for(i=0, j=0; i<MAX_MAP_PER_SERVER; i++)
				if(server[id].map[i][0])
					memcpy(WBUFP(buf,20+(j++)*16), server[id].map[i], 16);
			if (j > 0) {
				WBUFW(buf,0) = 0x2b04;
				WBUFW(buf,2) = j*16+20;
				mapif_sendallwos(fd, buf, j*16+20);
			}

			RFIFOSKIP(fd,70);
			break;
		}
		// mapserver -> map names recv.
		case 0x2afa:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			memset(server[id].map, 0, sizeof(server[id].map));
			j = 0;
			for(i = 4; i < RFIFOW(fd,2); i += 16) {
				memcpy(server[id].map[j], RFIFOP(fd,i), 16);
//				ShowMessage("set map %d.%d : %s\n", id, j, server[id].map[j]);
				j++;
			}
			server[id].maps = j;

			ShowStatus("Map-Server %d connected from %s:%d (%d maps).\n",
			       id, server[id].address.LANIP().getstring(), server[id].address.LANPort(), j);
			set_all_offline();

			WFIFOW(fd,0) = 0x2afb;
			WFIFOB(fd,2) = 0;
			memcpy(WFIFOP(fd,3), wisp_server_name, 24); // name for wisp to player
			WFIFOSET(fd,3+24);
			//WFIFOSET(fd,27);
			{
				unsigned char buf[16384];
				int x;
				if (j == 0) {
					ShowMessage("WARNING: Map-Server %d have NO maps.\n", id);
				// Transmitting maps information to the other map-servers
				} else {
					WBUFW(buf,0) = 0x2b04;
					WBUFW(buf,2) = j*16+20;

					WBUFLIP(buf,4) = server[id].address.LANIP();
					WBUFLIP(buf,8) = server[id].address.LANMask();
					WBUFW(buf,12) = server[id].address.LANPort();
					WBUFLIP(buf,14) = server[id].address.WANIP();
					WBUFW(buf,18) = server[id].address.WANPort();

					memcpy(WBUFP(buf,20), RFIFOP(fd,4), j*16);
					mapif_sendallwos(fd, buf, j*16+20);
				}
				// Transmitting the maps of the other map-servers to the new map-server
				for(x = 0; x < MAX_MAP_SERVERS; x++) {
					if(server[x].fd >= 0 && x != id) {
						WFIFOW(fd,0) = 0x2b04;

						WFIFOLIP(fd,4) = server[x].address.LANIP();
						WFIFOLIP(fd,8) = server[x].address.LANMask();
						WFIFOW(fd,12) = server[x].address.LANPort();
						WFIFOLIP(fd,14) = server[x].address.WANIP();
						WFIFOW(fd,18) = server[x].address.WANPort();

						for(i=0, j=0; i<MAX_MAP_PER_SERVER; i++)
							if (server[x].map[i][0])
								memcpy(WFIFOP(fd,20+(j++)*16), server[x].map[i], 16);
						if (j > 0) {
							WFIFOW(fd,2) = j*16+20;
							WFIFOSET(fd,j*16+20);
						}
					}
				}
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// auth request
		case 0x2afc:
			if (RFIFOREST(fd) < 22)
				return 0;
//			ShowMessage("(AUTH request) auth_fifo search %ld %ld %ld\n", (unsigned long)RFIFOL(fd, 2), (unsigned long)RFIFOL(fd, 6), (unsigned long)RFIFOL(fd, 10));
			for(i = 0; i < AUTH_FIFO_SIZE; i++) {
				if (auth_fifo[i].account_id == RFIFOL(fd,2) &&
				    auth_fifo[i].char_id == RFIFOL(fd,6) &&
				    auth_fifo[i].login_id1 == RFIFOL(fd,10) &&
#if CMP_AUTHFIFO_LOGIN2 != 0
				    // here, it's the only area where it's possible that we doesn't know login_id2 (map-server asks just after 0x72 packet, that doesn't given the value)
				    (auth_fifo[i].login_id2 == RFIFOL(fd,14) || RFIFOL(fd,14) == 0) && // relate to the versions higher than 18
#endif
				    (!check_ip_flag || auth_fifo[i].client_ip == RFIFOLIP(fd,18)) &&
				    !auth_fifo[i].delflag)
				{
					auth_fifo[i].delflag = 1;
					WFIFOW(fd,0) = 0x2afd;
					WFIFOW(fd,2) = 16 + sizeof(struct mmo_charstatus);
					WFIFOL(fd,4) = RFIFOL(fd,2);
					WFIFOL(fd,8) = auth_fifo[i].login_id2;
					WFIFOL(fd,12) = (unsigned long)auth_fifo[i].connect_until_time;
					mmo_char_fromsql(auth_fifo[i].char_id, char_dat, 1);
					char_dat[0].sex = auth_fifo[i].sex;
					//memcpy(WFIFOP(fd,16), &char_dat[0], sizeof(struct mmo_charstatus));
					if(char_dat) mmo_charstatus_tobuffer(*char_dat,WFIFOP(fd,16));
					WFIFOSET(fd, WFIFOW(fd,2));
					//ShowMessage("auth_fifo search success (auth #%d, account %ld, character: %ld).\n", i, (unsigned long)RFIFOL(fd,2), (unsigned long)RFIFOL(fd,6));
					break;
				}
			}
			if (i == AUTH_FIFO_SIZE) {
				WFIFOW(fd,0) = 0x2afe;
				WFIFOL(fd,2) = RFIFOL(fd,2);
				WFIFOSET(fd,6);
				ShowMessage("auth_fifo search error! account %ld not authentified.\n", (unsigned long)RFIFOL(fd,2));
			}
			RFIFOSKIP(fd,22);
			break;

		// set MAP user
		case 0x2aff:
			if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			if ( server[id].users != (unsigned short)RFIFOW(fd,4) )
				ShowMessage("[UserCount]: %d (Server: %d)\n", (unsigned short)RFIFOW(fd,4), id);
			server[id].users = RFIFOW(fd,4);

			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// char saving
		case 0x2b01:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			//check account
			sprintf(tmp_sql, "SELECT count(*) FROM `%s` WHERE `account_id` = '%ld' AND `char_id`='%ld'",char_db, (unsigned long)RFIFOL(fd,4),(unsigned long)RFIFOL(fd,8)); // TBR
			if(mysql_SendQuery(&mysql_handle, tmp_sql)) {
				ShowMessage("DB server Error - %s\n", mysql_error(&mysql_handle));
			}
			sql_res = mysql_store_result(&mysql_handle);
			i = 0;
			if (sql_res) {
				sql_row = mysql_fetch_row(sql_res);
				if (sql_row)
					i = atoi(sql_row[0]);
			}
			mysql_free_result(sql_res);

			if (i == 1 && char_dat) {
				mmo_charstatus_frombuffer(*char_dat,RFIFOP(fd,12));
				mmo_char_tosql(RFIFOL(fd,8), char_dat);
				//save to DB
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// req char selection
		case 0x2b02:
			if (RFIFOREST(fd) < 18)
				return 0;

			if (auth_fifo_pos >= AUTH_FIFO_SIZE)
				auth_fifo_pos = 0;

//			ShowMessage("(charselect) auth_fifo set %d - account_id:%08x login_id1:%08x\n", auth_fifo_pos, (unsigned long)RFIFOL(fd, 2), (unsigned long)RFIFOL(fd, 6));
			auth_fifo[auth_fifo_pos].account_id = RFIFOL(fd, 2);
			auth_fifo[auth_fifo_pos].char_id = 0;
			auth_fifo[auth_fifo_pos].login_id1 = RFIFOL(fd, 6);
			auth_fifo[auth_fifo_pos].login_id2 = RFIFOL(fd,10);
			auth_fifo[auth_fifo_pos].delflag = 2;
			auth_fifo[auth_fifo_pos].char_pos = 0;
			auth_fifo[auth_fifo_pos].connect_until_time = 0; // unlimited/unknown time by default (not display in map-server)
			auth_fifo[auth_fifo_pos].client_ip = RFIFOLIP(fd,14);
			auth_fifo_pos++;

			WFIFOW(fd, 0) = 0x2b03;
			WFIFOL(fd, 2) = RFIFOL(fd, 2);
			WFIFOB(fd, 6) = 0;
			WFIFOSET(fd, 7);

			RFIFOSKIP(fd, 18);
			break;

		// request "change map server"
		case 0x2b05:
			if (RFIFOREST(fd) < 49)
				return 0;

			if (auth_fifo_pos >= AUTH_FIFO_SIZE)
				auth_fifo_pos = 0;

			WFIFOW(fd, 0) = 0x2b06;
			memcpy(WFIFOP(fd,2), RFIFOP(fd,2), 42);
//			ShowMessage("(map change) auth_fifo set %d - account_id:%08x login_id1:%08x\n", auth_fifo_pos, (unsigned long)RFIFOL(fd, 2), (unsigned long)RFIFOL(fd, 6));
			ShowMessage("[MapChange] ");
			auth_fifo[auth_fifo_pos].account_id = RFIFOL(fd, 2);
			auth_fifo[auth_fifo_pos].login_id1 = RFIFOL(fd, 6);
			auth_fifo[auth_fifo_pos].login_id2 = RFIFOL(fd,10);
			auth_fifo[auth_fifo_pos].char_id = RFIFOL(fd,14);
			auth_fifo[auth_fifo_pos].delflag = 0;
			auth_fifo[auth_fifo_pos].sex = RFIFOB(fd,44);
			auth_fifo[auth_fifo_pos].client_ip = RFIFOLIP(fd,45);

			sql_query("SELECT `char_id`, `name` FROM `%s` WHERE `account_id` = '%ld' AND `char_id`='%ld'", char_db, (unsigned long)RFIFOL(fd,2), (unsigned long)RFIFOL(fd,14));
			i = 0;
			if(sql_res){
				sql_fetch_row();
				i = atoi(sql_row[0]);
				ShowMessage("aid: %d, cid: %d, name: %s", (unsigned long)RFIFOL(fd,2), atoi(sql_row[0]), sql_row[1]);
				sql_free();
				auth_fifo[auth_fifo_pos].char_pos = auth_fifo[auth_fifo_pos].char_id;
				auth_fifo_pos++;
				WFIFOL(fd,6) = 0;
			}else{
				ShowMessage("Error, aborted\n");
				return 0;
			}
			if(i == 0){
				WFIFOW(fd, 6) = 0;
			}

			WFIFOSET(fd, 44);
			RFIFOSKIP(fd, 49);
			ShowMessage(" done.\n");
		break;

		// char name check
		case 0x2b08:
			if (RFIFOREST(fd) < 6)
				return 0;

			sql_query("SELECT `name` FROM `%s` WHERE `char_id`='%ld'", char_db, (unsigned long)RFIFOL(fd,2));
			if (sql_res){
				sql_fetch_row();

				WFIFOW(fd,0) = 0x2b09;
				WFIFOL(fd,2) = RFIFOL(fd,2);

				if (sql_row)
					memcpy(WFIFOP(fd,6), sql_row[0], 24);
				else
					memcpy(WFIFOP(fd,6), unknown_char_name, 24);
				WFIFOSET(fd,30);

				sql_free();
			}

			RFIFOSKIP(fd,6);
			break;


		case 0x2b0a:
			if(RFIFOREST(fd)<4)
				return 0;
			if(RFIFOREST(fd)<RFIFOW(fd,2))
				return 0;
			memcpy(WFIFOP(login_fd,2),RFIFOP(fd,2),RFIFOW(fd,2)-2);
			WFIFOW(login_fd,0)=0x2720;
			WFIFOSET(login_fd,RFIFOW(fd,2));
//			ShowMessage("char : change gm -> login %d %s %d\n", (unsigned long)RFIFOL(fd, 4), RFIFOP(fd, 8), (unsigned short)RFIFOW(fd, 2));
			RFIFOSKIP(fd, RFIFOW(fd, 2));
			break;

		// account_reg保存要求
		case 0x2b10:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		  {
			struct global_reg reg[ACCOUNT_REG2_NUM];
			int j,p,acc;
			acc=RFIFOL(fd,4);
			for(p=8,j=0;p<RFIFOW(fd,2) && j<ACCOUNT_REG2_NUM;p+=36,j++){
				memcpy(reg[j].str,RFIFOP(fd,p),32);
				reg[j].value=RFIFOL(fd,p+32);
			}
			// set_account_reg2(acc,j,reg);
			// loginサーバーへ送る
			if( session_isActive(login_fd) )
			{	// don't send request if no login-server
				WFIFOW(login_fd, 0) = 0x2728;
				memcpy(WFIFOP(login_fd,0), RFIFOP(fd,0), RFIFOW(fd,2));
				WFIFOSET(login_fd, WFIFOW(login_fd,2));
			}
			// ワールドへの同垢ログインがなければmapサーバーに送る必要はない
			//memcpy(buf,RFIFOP(fd,0),RFIFOW(fd,2));
			//WBUFW(buf,0)=0x2b11;
			//mapif_sendall(buf,WBUFW(buf,2));
			RFIFOSKIP(fd,RFIFOW(fd,2));
//			ShowMessage("char: save_account_reg (from map)\n");
		  }
			break;

		// Map server send information to change an email of an account -> login-server
		case 0x2b0c:
			if (RFIFOREST(fd) < 86)
				return 0;
			if( session_isActive(login_fd) )
			{	// don't send request if no login-server
				memcpy(WFIFOP(login_fd,0), RFIFOP(fd,0), 86); // 0x2722 <account_id>.L <actual_e-mail>.40B <new_e-mail>.40B
				WFIFOW(login_fd,0) = 0x2722;
				WFIFOSET(login_fd, 86);
			}
			RFIFOSKIP(fd, 86);
			break;

		// Receiving from map-server a status change resquest. Transmission to login-server (by Yor)
		case 0x2b0e:
			if (RFIFOREST(fd) < 44)
				return 0;
			#ifndef CLOWNPHOBIA // Clownphobia doesnt support this.
		  {
			char character_name[24];
			int acc = RFIFOL(fd,2); // account_id of who ask (-1 if nobody)
			memcpy(character_name, RFIFOP(fd,6), 24);
			character_name[23] = '\0';
			// prepare answer
			WFIFOW(fd,0) = 0x2b0f; // answer
			WFIFOL(fd,2) = acc; // who want do operation
			WFIFOW(fd,30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban
			sql_query("SELECT `account_id`,`name` FROM `%s` WHERE `name` = '%s'",char_db, character_name);
			if (sql_res) {
				if (sql_fetch_row()) {
					memcpy(WFIFOP(fd,6), sql_row[1], 24); // put correct name if found
					WFIFOW(fd,32) = 0; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					switch(RFIFOW(fd, 30)) {
					case 1: // block
						if (acc == -1 || isGM(acc) >= isGM(atoi(sql_row[0]))) {
							if( session_isActive(login_fd) )
							{	// don't send request if no login-server
								WFIFOW(login_fd,0) = 0x2724;
								WFIFOL(login_fd,2) = atoi(sql_row[0]); // account value
								WFIFOL(login_fd,6) = 5; // status of the account
								WFIFOSET(login_fd, 10);
//								ShowMessage("char : status -> login: account %d, status: %d \n", char_dat[i].account_id, 5);
							} else
								WFIFOW(fd,32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
						} else
							WFIFOW(fd,32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
						break;
					case 2: // ban
						if (acc == -1 || isGM(acc) >= isGM(atoi(sql_row[0]))) {
							if( session_isActive(login_fd) )
							{	// don't send request if no login-server
								WFIFOW(login_fd, 0) = 0x2725;
								WFIFOL(login_fd, 2) = atoi(sql_row[0]); // account value
								WFIFOW(login_fd, 6) = RFIFOW(fd,32); // year
								WFIFOW(login_fd, 8) = RFIFOW(fd,34); // month
								WFIFOW(login_fd,10) = RFIFOW(fd,36); // day
								WFIFOW(login_fd,12) = RFIFOW(fd,38); // hour
								WFIFOW(login_fd,14) = RFIFOW(fd,40); // minute
								WFIFOW(login_fd,16) = RFIFOW(fd,42); // second
								WFIFOSET(login_fd,18);
//								ShowMessage("char : status -> login: account %d, ban: %dy %dm %dd %dh %dmn %ds\n",
//								       char_dat[i].account_id, (short)RFIFOW(fd,32), (short)RFIFOW(fd,34), (short)RFIFOW(fd,36), (short)RFIFOW(fd,38), (short)RFIFOW(fd,40), (short)RFIFOW(fd,42));
							} else
								WFIFOW(fd,32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
						} else
							WFIFOW(fd,32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
						break;
					case 3: // unblock
						if (acc == -1 || isGM(acc) >= isGM(atoi(sql_row[0]))) {
							if( session_isActive(login_fd) )
							{	// don't send request if no login-server
								WFIFOW(login_fd,0) = 0x2724;
								WFIFOL(login_fd,2) = atoi(sql_row[0]); // account value
								WFIFOL(login_fd,6) = 0; // status of the account
								WFIFOSET(login_fd, 10);
//								ShowMessage("char : status -> login: account %d, status: %d \n", char_dat[i].account_id, 0);
							} else
								WFIFOW(fd,32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
						} else
							WFIFOW(fd,32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
						break;
					case 4: // unban
						if (acc == -1 || isGM(acc) >= isGM(atoi(sql_row[0]))) {
							if( session_isActive(login_fd) )
							{	// don't send request if no login-server
								WFIFOW(login_fd, 0) = 0x272a;
								WFIFOL(login_fd, 2) = atoi(sql_row[0]); // account value
								WFIFOSET(login_fd, 6);
//								ShowMessage("char : status -> login: account %d, unban request\n", char_dat[i].account_id);
							} else
								WFIFOW(fd,32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
						} else
							WFIFOW(fd,32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
						break;
					case 5: // changesex
						if (acc == -1 || isGM(acc) >= isGM(atoi(sql_row[0]))) {
							if( session_isActive(login_fd) )
							{	// don't send request if no login-server
								WFIFOW(login_fd, 0) = 0x2727;
								WFIFOL(login_fd, 2) = atoi(sql_row[0]); // account value
								WFIFOSET(login_fd, 6);
//								ShowMessage("char : status -> login: account %d, change sex request\n", char_dat[i].account_id);
							} else
								WFIFOW(fd,32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
						} else
							WFIFOW(fd,32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
						break;
					}
				} else {
					// character name not found
					memcpy(WFIFOP(fd,6), character_name, 24);
					WFIFOW(fd,32) = 1; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
				}
				// send answer if a player ask, not if the server ask
				if (acc != -1) {
					WFIFOSET(fd, 34);
				}
			}
		  }
		  	#endif // CLOWNPHOBIA
			RFIFOSKIP(fd, 44);
			break;

		// Recieve rates [Wizputer]
		case 0x2b16:
			if (RFIFOREST(fd) < 10 || RFIFOREST(fd) < RFIFOW(fd,8))
				return 0;
			sql_query("INSERT INTO `ragsrvinfo` SET `index`='%d',`name`='%s',`exp`='%d',`jexp`='%d',`drop`='%d',`motd`='%s'",
			        fd, server_name, (unsigned short)RFIFOW(fd,2), (unsigned short)RFIFOW(fd,4), (unsigned short)RFIFOW(fd,6), (char*)RFIFOP(fd,10));
			RFIFOSKIP(fd,RFIFOW(fd,8));
			break;


		// Character disconnected set online 0 [Wizputer]
		case 0x2b17:
			if (RFIFOREST(fd) < 6 )
				return 0;
			//ShowMessage("Setting %d char offline\n",(unsigned long)RFIFOL(fd,2));
			set_char_offline(RFIFOL(fd,2),RFIFOL(fd,6));
			RFIFOSKIP(fd,10);
			break;
		// Reset all chars to offline [Wizputer]
		case 0x2b18:
		    set_all_offline();
			RFIFOSKIP(fd,2);
			break;
		// Character set online [Wizputer]
		case 0x2b19:
			if (RFIFOREST(fd) < 6 )
				return 0;
			//ShowMessage("Setting %d char online\n",(unsigned long)RFIFOL(fd,2));
			set_char_online(RFIFOL(fd,2),RFIFOL(fd,6));
			RFIFOSKIP(fd,10);
			break;

		// Request sending of fame list
		case 0x2b1a:
			if (RFIFOREST(fd) < 2)
				return 0;
		{
			int len = 6, num = 0;
			unsigned char buf[32000];

			WBUFW(buf,0) = 0x2b1b;
			sql_query("SELECT `char_id`,`fame`,`name` FROM `%s` WHERE `class`='10' OR `class`='4011'"
				"OR `class`='4033' ORDER BY `fame` DESC", char_db);
			if (sql_res) {
				while(sql_fetch_row()) {
					WBUFL(buf, len) = atoi(sql_row[0]);
					WBUFL(buf, len+4) = atoi(sql_row[1]);
					safestrcpy((char*)WBUFP(buf, len+8), sql_row[2], 24);
					len += 32;
					if (++num == MAX_FAMELIST)
						break;
				}
				sql_free();
			}
   			WBUFW(buf, 4) = len;

			num = 0;
			sql_query("SELECT `char_id`,`fame`,`name` FROM `%s` WHERE `class`='18' OR `class`='4019'"
				"OR `class`='4041' ORDER BY `fame` DESC", char_db);
			if (sql_res) {
				while(sql_fetch_row()) {
					WBUFL(buf, len) = atoi(sql_row[0]);
					WBUFL(buf, len+4) = atoi(sql_row[1]);
					safestrcpy((char*)WBUFP(buf, len+8), sql_row[2], 24);
					len += 32;
					if (++num == 10)
						break;
				}
				sql_free();
			}
			WBUFW(buf, 2) = len;

			mapif_sendall(buf, len);
			RFIFOSKIP(fd,2);
			break;
		}

		default:
			// inter server - packet
			{
				int r = inter_parse_frommap(fd);
				if (r == 1) break;		// processed
				if (r == 2) return 0;	// need more packet
			}

			// no inter server packet. no char server packet -> disconnect
			ShowMessage("parse_frommap: unknown packet %x! \n", (unsigned short)RFIFOW(fd,0));
			session_Remove(fd);
			return 0;
		}
	}
	return 0;
}

int search_mapserver(char *map) {
	int i, j;
	char temp_map[64];
	int temp_map_len;

//	ShowMessage("Searching the map-server for map '%s'... ", map);
	memcpy(temp_map, map, 24);
	temp_map[23] = '\0';
	if (strchr(temp_map, '.') != NULL)
		temp_map[strchr(temp_map, '.') - temp_map + 1] = '\0'; // suppress the '.gat', but conserve the '.' to be sure of the name of the map

	temp_map_len = strlen(temp_map);
	for(i = 0; i < MAX_MAP_SERVERS; i++)
		if (server[i].fd >= 0)
			for (j = 0; server[i].map[j][0]; j++)
				//ShowMessage("%s : %s = %d\n", server[i].map[j], map, strncmp(server[i].map[j], temp_map, temp_map_len));
				if (strncmp(server[i].map[j], temp_map, temp_map_len) == 0) {
//					ShowMessage("found -> server #%d.\n", i);
					return i;
				}

//	ShowMessage("not found.\n");
	return -1;
}

int char_mapif_init(int fd) {
	return inter_mapif_init(fd);
}

int parse_char(int fd)
{
	unsigned short cmd;

	if( !session_isValid(fd) )
		return 0;

	unsigned long client_ip = session[fd]->client_ip;
	struct char_session_data *sd = (struct char_session_data*)session[fd]->session_data;

	if( !session_isActive(login_fd) )
	{	// no login server available, reject connection
		session_Remove(fd);
		return 0;
	}

	if( !session_isActive(fd) ) {
		if (fd == login_fd)
			login_fd = -1;
		if (sd != NULL)
			set_char_offline(99,sd->account_id);
		session_Remove(fd);
		return 0;
	}

	while(RFIFOREST(fd) >= 2) {
		cmd = RFIFOW(fd,0);
		// crc32のスキップ用
		if(	sd==NULL			&&	// 未ログインor管理パケット
			RFIFOREST(fd)>=4	&&	// 最低バイト数制限 ＆ 0x7530,0x7532管理パケ除去
			RFIFOREST(fd)<=21	&&	// 最大バイト数制限 ＆ サーバーログイン除去
			cmd!=0x20b	&&	// md5通知パケット除去
			(RFIFOREST(fd)<6 || RFIFOW(fd,4)==0x65)	){	// 次に何かパケットが来てるなら、接続でないとだめ
			RFIFOSKIP(fd,4);
			cmd = RFIFOW(fd,0);
			ShowMessage("parse_char : %d crc32 skipped\n",fd);
			if(RFIFOREST(fd)==0)
				return 0;
		}

//		if(cmd<30000 && cmd!=0x187)
//			ShowMessage("parse_char : %d %d %d\n",fd,RFIFOREST(fd),cmd);

		// 不正パケットの処理
//		if (sd == NULL && cmd != 0x65 && cmd != 0x20b && cmd != 0x187 &&
//					 cmd != 0x2af8 && cmd != 0x7530 && cmd != 0x7532)
//			cmd = 0xffff;	// パケットダンプを表示させる

		switch(cmd){
		case 0x20b: //20040622 encryption ragexe correspondence
			if (RFIFOREST(fd) < 19)
				return 0;
			RFIFOSKIP(fd,19);
			break;

		case 0x65: // request to connect
			ShowMessage("request connect - account_id:%ld/login_id1:%ld/login_id2:%ld\n", (unsigned long)RFIFOL(fd, 2), (unsigned long)RFIFOL(fd, 6), (unsigned long)RFIFOL(fd, 10));
			if (RFIFOREST(fd) < 17)
				return 0;
		{
			size_t i;
			if (sd == NULL) {
				CREATE(session[fd]->session_data, struct char_session_data, 1);
				sd = (struct char_session_data*)session[fd]->session_data;
				sd->connect_until_time = 0; // unknow or illimited (not displaying on map-server)
			}
			sd->account_id = RFIFOL(fd, 2);
			sd->login_id1 = RFIFOL(fd, 6);
			sd->login_id2 = RFIFOL(fd, 10);
			sd->sex = RFIFOB(fd, 16);

			WFIFOL(fd, 0) = RFIFOL(fd, 2);
			WFIFOSET(fd, 4);

			for(i = 0; i < AUTH_FIFO_SIZE; i++) {
				if (auth_fifo[i].account_id == sd->account_id &&
				    auth_fifo[i].login_id1 == sd->login_id1 &&
#if CMP_AUTHFIFO_LOGIN2 != 0
				    auth_fifo[i].login_id2 == sd->login_id2 && // relate to the versions higher than 18
#endif
				    (!check_ip_flag || (auth_fifo[i].client_ip == client_ip) ) &&
				    auth_fifo[i].delflag == 2)
				{
					auth_fifo[i].delflag = 1;

					if (max_connect_user == 0 || count_users() < max_connect_user) {
						if( session_isActive(login_fd) )
						{	// don't send request if no login-server
							// request to login-server to obtain e-mail/time limit
							WFIFOW(login_fd,0) = 0x2716;
							WFIFOL(login_fd,2) = sd->account_id;
							WFIFOSET(login_fd,6);
						}
						// send characters to player
						mmo_char_send006b(fd, sd);
					} else {
						// refuse connection (over populated)
						WFIFOW(fd,0) = 0x6c;
						WFIFOW(fd,2) = 0;
						WFIFOSET(fd,3);
					}
					//ShowMessage("connection request> set delflag 1(o:2)- account_id:%d/login_id1:%d(fifo_id:%d)\n", sd->account_id, sd->login_id1, i);
				break;
				}
			}
			if (i == AUTH_FIFO_SIZE) {
				if( session_isActive(login_fd) )
				{	// don't send request if no login-server
					WFIFOW(login_fd,0) = 0x2712; // ask login-server to authentify an account
					WFIFOL(login_fd,2) = sd->account_id;
					WFIFOL(login_fd,6) = sd->login_id1;
					WFIFOL(login_fd,10) = sd->login_id2;
					WFIFOB(login_fd,14) = sd->sex;
					WFIFOLIP(login_fd,15) = client_ip;
					WFIFOSET(login_fd,19);
				} else { // if no login-server, we must refuse connection
					WFIFOW(fd,0) = 0x6c;
					WFIFOW(fd,2) = 0;
					WFIFOSET(fd,3);
				}
			}
			RFIFOSKIP(fd, 17);
			break;
		}
		case 0x66: // char select
//			ShowMessage("0x66> request connect - account_id:%d/char_num:%d\n",sd->account_id,(unsigned char)RFIFOB(fd, 2));
			if (RFIFOREST(fd) < 3)
				return 0;
		{
			int j;

			if(!sd)
			{
				RFIFOSKIP(fd, 3);
				break;
			}

			sql_query("SELECT `char_id` FROM `%s` WHERE `account_id`='%ld' AND `char_num`='%d'",char_db, sd->account_id, (unsigned char)RFIFOB(fd, 2));
			if (sql_res){
				sql_fetch_row();
				if (sql_row)
					mmo_char_fromsql(atoi(sql_row[0]), char_dat, 1);
				else
				{
					mysql_free_result(sql_res);
					RFIFOSKIP(fd, 3);
					break;
				}
			}

			if (log_char) {
				sql_query("INSERT INTO `%s`(`time`, `account_id`,`char_num`,`name`) VALUES (NOW(), '%ld', '%d', '%s')",
					charlog_db, sd->account_id, (unsigned char)RFIFOB(fd, 2), char_dat[0].name);
			}
			ShowMessage("("CL_BT_BLUE"%d"CL_NORM") char selected ("CL_BT_GREEN"%d"CL_NORM") "CL_BT_GREEN"%s"CL_NORM RETCODE, sd->account_id, (unsigned char)RFIFOB(fd, 2), char_dat[0].name);

			j = search_mapserver(char_dat[0].last_point.map);

			// if map is not found, we check major cities
			if (j < 0) {
				if ((j = search_mapserver("prontera.gat")) >= 0) { // check is done without 'gat'.
					memcpy(char_dat[0].last_point.map, "prontera.gat", 24);
					char_dat[0].last_point.x = 273; // savepoint coordonates
					char_dat[0].last_point.y = 354;
				} else if ((j = search_mapserver("geffen.gat")) >= 0) { // check is done without 'gat'.
					memcpy(char_dat[0].last_point.map, "geffen.gat", 24);
					char_dat[0].last_point.x = 120; // savepoint coordonates
					char_dat[0].last_point.y = 100;
				} else if ((j = search_mapserver("morocc.gat")) >= 0) { // check is done without 'gat'.
					memcpy(char_dat[0].last_point.map, "morocc.gat", 24);
					char_dat[0].last_point.x = 160; // savepoint coordonates
					char_dat[0].last_point.y = 94;
				} else if ((j = search_mapserver("alberta.gat")) >= 0) { // check is done without 'gat'.
					memcpy(char_dat[0].last_point.map, "alberta.gat", 24);
					char_dat[0].last_point.x = 116; // savepoint coordonates
					char_dat[0].last_point.y = 57;
				} else if ((j = search_mapserver("payon.gat")) >= 0) { // check is done without 'gat'.
					memcpy(char_dat[0].last_point.map, "payon.gat", 24);
					char_dat[0].last_point.x = 87; // savepoint coordonates
					char_dat[0].last_point.y = 117;
				} else if ((j = search_mapserver("izlude.gat")) >= 0) { // check is done without 'gat'.
					memcpy(char_dat[0].last_point.map, "izlude.gat", 24);
					char_dat[0].last_point.x = 94; // savepoint coordonates
					char_dat[0].last_point.y = 103;
				} else {
					// get first online server
					j = 0;
					for(j = 0; j < MAX_MAP_SERVERS; j++)
						if( server[j].fd >= 0 && server[j].map[0][0] )
						{
							ShowMessage("Map-server #%d found with a map: '%s'.\n", j, server[j].map[0]);
							break;
						}
					// if no map-servers are connected, we send: server closed
					if (j >= MAX_MAP_SERVERS)
					{
						WFIFOW(fd,0) = 0x81;
						WFIFOL(fd,2) = 1; // 01 = Server closed
						WFIFOSET(fd,3);
						RFIFOSKIP(fd,3);
						break;
					}
				}
			}
			WFIFOW(fd, 0) =0x71;
			WFIFOL(fd, 2) =char_dat[0].char_id;
			memcpy(WFIFOP(fd, 6), char_dat[0].last_point.map, 24);
//			if( server[j].address.isLAN(client_ip) )
//			{
				ShowMessage("--Send IP of map-server: %s:%d (%s)\n", server[j].address.LANIP().getstring(), server[j].address.LANPort(), CL_LT_GREEN"LAN"CL_NORM);
				WFIFOLIP(fd, 22) = server[j].address.LANIP();
				WFIFOW(fd, 26)   = server[j].address.WANPort();
//			}
//			else
//			{
//				ShowMessage("--Send IP of map-server: %s:%d (%s)\n", server[j].address.WANIP().getstring(), server[j].address.WANPort(), CL_LT_CYAN"WAN"CL_NORM);
//				WFIFOLIP(fd, 22) = server[j].address.WANIP();
//				WFIFOW(fd, 26)   = server[j].address.WANPort();
//			}

			WFIFOSET(fd, 28);

			if (auth_fifo_pos >= AUTH_FIFO_SIZE) {
				auth_fifo_pos = 0;
			}
//			ShowMessage("auth_fifo set (auth_fifo_pos:%d) - account_id:%d char_id:%d login_id1:%d\n", auth_fifo_pos, sd->account_id, char_dat[0].char_id, sd->login_id1);
			auth_fifo[auth_fifo_pos].account_id = sd->account_id;
			auth_fifo[auth_fifo_pos].char_id = char_dat[0].char_id;
			auth_fifo[auth_fifo_pos].login_id1 = sd->login_id1;
			auth_fifo[auth_fifo_pos].login_id2 = sd->login_id2;
			auth_fifo[auth_fifo_pos].delflag = 0;
			//auth_fifo[auth_fifo_pos].char_pos = sd->found_char[ch];
			auth_fifo[auth_fifo_pos].char_pos = 0;
			auth_fifo[auth_fifo_pos].sex = sd->sex;
			auth_fifo[auth_fifo_pos].connect_until_time = sd->connect_until_time;
			auth_fifo[auth_fifo_pos].client_ip = client_ip;
			auth_fifo_pos++;
//			ShowMessage("0x66> end\n");
			RFIFOSKIP(fd, 3);
			break;
		}
		case 0x67:	// make new
//			ShowMessage("0x67>request make new char\n");
			if (RFIFOREST(fd) < 37)
				return 0;
		{
			size_t ch;
			int i;
			if(char_new == 0)
				i = -2;
			else
				i = make_new_char_sql(fd, RFIFOP(fd, 2));

			//if (i < 0) {
			//	WFIFOW(fd, 0) = 0x6e;
			//	WFIFOB(fd, 2) = 0x00;
			//	WFIFOSET(fd, 3);
			//	RFIFOSKIP(fd, 37);
			//	break;
			//}
			//Changed that we can support 'Charname already exists' (-1) amd 'Char creation denied' (-2)
			//And 'You are underaged' (-3) (XD) [Sirius]
			if(i == -1){
                //already exists
                WFIFOW(fd, 0) = 0x6e;
                WFIFOB(fd, 2) = 0x00;
                WFIFOSET(fd, 3);
                RFIFOSKIP(fd, 37);
                break;
            }else if(i == -2){
                //denied
                WFIFOW(fd, 0) = 0x6e;
                WFIFOB(fd, 2) = 0x02;
                WFIFOSET(fd, 3);
                RFIFOSKIP(fd, 37);
                break;
            }else if(i == -3){
                //underaged XD
                WFIFOW(fd, 0) = 0x6e;
                WFIFOB(fd, 2) = 0x01;
                WFIFOSET(fd, 3);
                RFIFOSKIP(fd, 37);
                break;
            }

			WFIFOW(fd, 0) = 0x6d;
			memset(WFIFOP(fd, 2), 0x00, 106);

			mmo_char_fromsql(i, char_dat, 0);
			i = 0;
			WFIFOL(fd, 2) = char_dat[i].char_id;
			WFIFOL(fd,2+4) = char_dat[i].base_exp;
			WFIFOL(fd,2+8) = char_dat[i].zeny;
			WFIFOL(fd,2+12) = char_dat[i].job_exp;
			WFIFOL(fd,2+16) = char_dat[i].job_level;

			WFIFOL(fd,2+28) = char_dat[i].karma;
			WFIFOL(fd,2+32) = char_dat[i].manner;

			WFIFOW(fd,2+40) = 0x30;
			WFIFOW(fd,2+42) = (unsigned short)((char_dat[i].hp > 0x7fff) ? 0x7fff : char_dat[i].hp);
			WFIFOW(fd,2+44) = (unsigned short)((char_dat[i].max_hp > 0x7fff) ? 0x7fff : char_dat[i].max_hp);
			WFIFOW(fd,2+46) = (unsigned short)((char_dat[i].sp > 0x7fff) ? 0x7fff : char_dat[i].sp);
			WFIFOW(fd,2+48) = (unsigned short)((char_dat[i].max_sp > 0x7fff) ? 0x7fff : char_dat[i].max_sp);
			WFIFOW(fd,2+50) = DEFAULT_WALK_SPEED; // char_dat[i].speed;
			WFIFOW(fd,2+52) = char_dat[i].class_;
			WFIFOW(fd,2+54) = char_dat[i].hair;

			WFIFOW(fd,2+58) = char_dat[i].base_level;
			WFIFOW(fd,2+60) = char_dat[i].skill_point;

			WFIFOW(fd,2+64) = char_dat[i].shield;
			WFIFOW(fd,2+66) = char_dat[i].head_top;
			WFIFOW(fd,2+68) = char_dat[i].head_mid;
			WFIFOW(fd,2+70) = char_dat[i].hair_color;

			memcpy(WFIFOP(fd,2+74), char_dat[i].name, 24);

			WFIFOB(fd,2+98)  = (unsigned char)char_dat[i].str;
			WFIFOB(fd,2+99)  = (unsigned char)char_dat[i].agi;
			WFIFOB(fd,2+100) = (unsigned char)char_dat[i].vit;
			WFIFOB(fd,2+101) = (unsigned char)char_dat[i].int_;
			WFIFOB(fd,2+102) = (unsigned char)char_dat[i].dex;
			WFIFOB(fd,2+103) = (unsigned char)char_dat[i].luk;
			WFIFOB(fd,2+104) = (unsigned char)char_dat[i].char_num;

			WFIFOSET(fd, 108);
			RFIFOSKIP(fd, 37);
			//to do
			for(ch = 0; ch < 9; ch++) {
				if (sd->found_char[ch] == 0xFFFFFFFF) {
					sd->found_char[ch] = char_dat[i].char_id;
					break;
				}
			}
		}
		case 0x68: // delete
			if (RFIFOREST(fd) < 46)
				return 0;
		{
			int i, ch;
			char email[40];

			ShowMessage(CL_BT_RED" Request Char Del:"CL_NORM" "CL_BT_GREEN"%d"CL_NORM"("CL_BT_GREEN"%d"CL_NORM")\n", sd->account_id, (unsigned long)RFIFOL(fd, 2));
			memcpy(email, RFIFOP(fd,6), 40);
/* ---------- REMOVED ----------
			sprintf(tmp_sql, "SELECT `email` FROM `%s` WHERE `%s`='%d'",login_db, login_db_account_id, sd->account_id);
			if (mysql_SendQuery(&lmysql_handle, tmp_sql)) {
				ShowMessage(CL_BT_RED" DB server Error Delete Char data - %s "CL_NORM"  \n", mysql_error(&lmysql_handle));
			}
			sql_res = mysql_store_result(&lmysql_handle);
			if (sql_res) {
				sql_row = mysql_fetch_row(sql_res);

				if ( (strcmp(email,sql_row[0]) == 0) || // client_value == db_value?
					 ((strcmp(email,"") == 0) &&
					  (strcmp(sql_row[0],"a@a.com") == 0)) ) { // client_value == "" && db_value == "a@a.com" ?
					mysql_free_result(sql_res);
				} else {
					WFIFOW(fd, 0) = 0x70;
					WFIFOB(fd, 2) = 0;
					WFIFOSET(fd, 3);
					RFIFOSKIP(fd, 46);
					mysql_free_result(sql_res);
					break;
				}
			} else {
				WFIFOW(fd, 0) = 0x70;
				WFIFOB(fd, 2) = 0;
				WFIFOSET(fd, 3);
				RFIFOSKIP(fd, 46);
				mysql_free_result(sql_res);
				break;
			}

------------- END REMOVED -------- */
			//Remove recoded by Sirius
			if(strcmp(email, sd->email) != 0){
				if(strcmp("a@a.com", sd->email) == 0){
					if(strcmp("a@a.com", email) == 0 || strcmp("", email) == 0){
						//ignore
					}else{
						//del fail
						WFIFOW(fd, 0) = 0x70;
						WFIFOB(fd, 2) = 0;
						WFIFOSET(fd, 3);
						RFIFOSKIP(fd, 46);
						break;
					}
				}else{
					//del fail
					WFIFOW(fd, 0) = 0x70;
					WFIFOB(fd, 2) = 0;
					WFIFOSET(fd, 3);
					RFIFOSKIP(fd, 46);
					break;
				}
			}

			sql_query("SELECT `name`,`partner_id` FROM `%s` WHERE `char_id`='%ld'",char_db, (unsigned long)RFIFOL(fd,2));
			if (sql_res && sql_fetch_row()) {
				//Divorce [Wizputer]
				if (atoi(sql_row[1])) {
					unsigned char buf[64];
					sql_query("UPDATE `%s` SET `partner_id`='0' WHERE `char_id`='%d'",char_db,atoi(sql_row[1]));
					sql_query("DELETE FROM `%s` WHERE (`nameid`='%d' OR `nameid`='%d') AND `char_id`='%d'",inventory_db,WEDDING_RING_M,WEDDING_RING_F,atoi(sql_row[1]));
					WBUFW(buf,0) = 0x2b12;
					WBUFL(buf,2) = atoi(sql_row[0]);
					WBUFL(buf,6) = atoi(sql_row[1]);
					mapif_sendall(buf,10);
				}

				if (atoi(sql_row[2])) {
					inter_guild_leave(atoi(sql_row[0]),sd->account_id,RFIFOL(fd,2));
				}
				sql_free();
			}
			//delete char from SQL
			sql_query("DELETE FROM `%s` WHERE `char_id`='%ld'",char_db, (unsigned long)RFIFOL(fd, 2));


			// TBR
			// Unless someone can document what the hell this is here for,
			// it will be removed in a future release
			for(i = 0; i < 9; i++) {
				ShowMessage("char comp: %d - %ld (%d)\n", sd->found_char[i], (unsigned long)RFIFOL(fd, 2), sd->account_id);
				if (sd->found_char[i] == RFIFOL(fd, 2)) {
					for(ch = i; ch < 9-1; ch++)
						sd->found_char[ch] = sd->found_char[ch+1];
					sd->found_char[8] = 0xFFFFFFFF;
					break;
				}
			}
			if (i == 9) { // reject
				WFIFOW(fd, 0) = 0x70;
				WFIFOB(fd, 2) = 0;
				WFIFOSET(fd, 3);
			} else { // deleted!
				WFIFOW(fd, 0) = 0x6f;
				WFIFOSET(fd, 2);
			}
			RFIFOSKIP(fd, 46);
			break;
		}
		case 0x2af8: // login as map-server
			if (RFIFOREST(fd) < 70)
				return 0;
		{
			int i;
			WFIFOW(fd, 0) = 0x2af9;
			for(i = 0; i < MAX_MAP_SERVERS; i++) {
				if(server[i].fd < 0)
					break;
			}
			if (i == MAX_MAP_SERVERS || strcmp((const char*)RFIFOP(fd,2), userid) || strcmp((const char*)RFIFOP(fd,26), passwd)) {
				WFIFOB(fd,2) = 3;
				WFIFOSET(fd, 3);
			} else {
//				int len;
				WFIFOB(fd,2) = 0;
				WFIFOSET(fd, 3);
				session[fd]->func_parse = parse_frommap;
				server[i].fd = fd;

//				server[i].lanip = RFIFOLIP(fd, 54);
//				server[i].lanport = RFIFOW(fd, 58);
				server[i].address = ipset(RFIFOLIP(fd,54), RFIFOLIP(fd,58), RFIFOW(fd,62),RFIFOLIP(fd,64), RFIFOW(fd,68));

				server[i].users = 0;
				memset(server[i].map, 0, sizeof(server[i].map));
				realloc_fifo(fd, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);
				char_mapif_init(fd);
			}
			RFIFOSKIP(fd,70);
			break;
		}
		case 0x187:	// Alive?
			if (RFIFOREST(fd) < 6) {
				return 0;
			}
			RFIFOSKIP(fd, 6);
			break;

		case 0x7530:	// Athena info get
			WFIFOW(fd, 0) = 0x7531;
			WFIFOB(fd, 2) = ATHENA_MAJOR_VERSION;
			WFIFOB(fd, 3) = ATHENA_MINOR_VERSION;
			WFIFOB(fd, 4) = ATHENA_REVISION;
			WFIFOB(fd, 5) = ATHENA_RELEASE_FLAG;
			WFIFOB(fd, 6) = ATHENA_OFFICIAL_FLAG;
			WFIFOB(fd, 7) = ATHENA_SERVER_INTER | ATHENA_SERVER_CHAR;
			WFIFOW(fd, 8) = ATHENA_MOD_VERSION;
			WFIFOSET(fd, 10);
			RFIFOSKIP(fd, 2);
			return 0;

		case 0x7532:	// disconnect(default also disconnect)
		default:
			session_Remove(fd);
			return 0;
		}
	}
	return 0;
}

// Console Command Parser [Wizputer]
int parse_console(char *buf) {
    char *type,*command;

    type = (char *)aMalloc(64);
    command = (char *)aMalloc(64);

    memset(type,0,64);
    memset(command,0,64);

    ShowMessage("Console: %s\n",buf);

    if ( sscanf(buf, "%[^:]:%[^\n]", type , command ) < 2 )
        sscanf(buf,"%[^\n]",type);

    ShowMessage("Type of command: %s || Command: %s \n",type,command);

    if(buf) aFree(buf);
    if(type) aFree(type);
    if(command) aFree(command);

    return 0;
}

// MAP send all
int mapif_sendall(unsigned char *buf, unsigned int len) {
	int i, c=0;
	int fd;

	if(buf)
	for(i = 0; i < MAX_MAP_SERVERS; i++)
	{
		fd = server[i].fd;
		if( session_isActive(fd) )
		{
			memcpy(WFIFOP(fd,0), buf, len);
			WFIFOSET(fd,len);
			c++;
		}
	}

	return c;
}

int mapif_sendallwos(int sfd, unsigned char *buf, unsigned int len) {
	int i, c=0;
	int fd;

	if(buf)
	for(i=0, c=0;i<MAX_MAP_SERVERS;i++)
	{
		fd = server[i].fd;
		if( session_isActive(fd) && fd != sfd )
		{
			memcpy(WFIFOP(fd,0), buf, len);
			WFIFOSET(fd, len);
			c++;
		}
	}

	return c;
}

int mapif_send(int fd, unsigned char *buf, unsigned int len) {
	int i;

	if( buf && session_isActive(fd) ) {
		for(i = 0; i < MAX_MAP_SERVERS; i++)
		{
			if( fd == server[i].fd )
			{
				memcpy(WFIFOP(fd,0), buf, len);
				WFIFOSET(fd,len);
				return 1;
			}
		}
	}
	return 0;
}

int send_users_tologin(int tid, unsigned long tick, int id, int data) {
	int users = count_users();
	unsigned char buf[16];

	if( session_isActive(login_fd) )
	{
		// send number of user to login server
		WFIFOW(login_fd,0) = 0x2714;
		WFIFOL(login_fd,2) = users;
		WFIFOSET(login_fd,6);
	}
	// send number of players to all map-servers
	WBUFW(buf,0) = 0x2b00;
	WBUFL(buf,2) = users;
	mapif_sendall(buf, 6);

	return 0;
}

int check_connect_login_server(int tid, unsigned long tick, int id, int data) {

	if ( !session_isActive(login_fd) )
	{
		ShowStatus("Attempt to connect to login-server...\n");
		login_fd = make_connection(loginaddress.addr(), loginaddress.port());
		if ( session_isActive(login_fd) )
		{
			session[login_fd]->func_parse = parse_tologin;
			realloc_fifo(login_fd, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);

			WFIFOW(login_fd,0) = 0x2710;
			memcpy(WFIFOP(login_fd,2), userid, strlen(userid) < 24 ? strlen(userid) : 24);
			WFIFOB(login_fd,25) = 0;
			memcpy(WFIFOP(login_fd,26), passwd, strlen(passwd) < 24 ? strlen(passwd) : 24);
			WFIFOB(login_fd,49) = 0;
			memcpy(WFIFOP(login_fd,50), server_name, strlen(server_name) < 20 ? strlen(server_name) : 20);
			WFIFOB(login_fd,69) = 0;
			WFIFOB(login_fd,70) = char_maintenance;
			WFIFOB(login_fd,71) = char_new_display;
			WFIFOB(login_fd,72) = 0;
			WFIFOB(login_fd,73) = 0;
			// ip's
			WFIFOLIP(login_fd,74) = charaddress.LANIP();
			WFIFOLIP(login_fd,78) = charaddress.LANMask();
			WFIFOW(login_fd,82)   = charaddress.LANPort();
			WFIFOLIP(login_fd,84) = charaddress.WANIP();
			WFIFOW(login_fd,88)   = charaddress.WANPort();

			WFIFOSET(login_fd,90);

			// (re)connected to login-server,
			// now wi'll look in sql which player's are ON and set them OFF
			// AND send to all mapservers (if we have one / ..) to kick the players
			// so the bug is fixed, if'ure using more than one charservers (worlds)
			// that the player'S got reejected from server after a 'world' crash^^
			// 2b1f AID.L B1
			struct char_session_data *sd;
			int fd, cc;
			unsigned char buf[16];

			sql_query("SELECT `account_id`, `online` FROM `%s` WHERE `online` = '1'", char_db);
			if(sql_res){
				cc = sql_num_rows();
				ShowError("Setting %d Players offline\n", cc);
				while(sql_fetch_row())
				{
					//sql_row[0] == AID
					//tell the loginserver
					WFIFOW(login_fd, 0) = 0x272c; //set off
					WFIFOL(login_fd, 2) = atoi(sql_row[0]); //AID
					WFIFOSET(login_fd, 6);

					//tell map to 'kick' the player (incase of 'on' ..)
					WBUFW(buf, 0) = 0x2b1f;
					WBUFL(buf, 2) = atoi(sql_row[0]);
					WBUFB(buf, 6) = 1;
					mapif_sendall(buf, 7);

					//kick the player if he's on charselect
					for(fd=0; (size_t)fd<fd_max; fd++)
					{
						if(session[fd] && (sd = (struct char_session_data*)session[fd]->session_data))
						{
							if(sd->account_id == (unsigned long)atoi(sql_row[0]))
							{
								session_Remove(fd);
							}
						}
					}
				}
				sql_free();
			}

			//Now Update all players to 'OFFLINE'
			sql_query("UPDATE `%s` SET `online` = '0'", char_db);
			sql_query("UPDATE `%s` SET `online` = '0'", guild_member_db);
			sql_query("UPDATE `%s` SET `connect_member` = '0'", guild_db);
		}
	}
	return 0;
}

int char_db_final(void *key,void *data,va_list ap){
	struct mmo_charstatus *p = (struct mmo_charstatus *) data;
	if (p) aFree(p);
	return 0;
}

void do_final(void)
{
	size_t i;
	ShowMessage("Doing final stage...\n");
	//inter_save();
	do_final_itemdb();
	//check SQL save progress.
	//wait until save char complete

	set_all_offline();
	flush_fifos();

	inter_final();

	sql_query("DELETE FROM `ragsrvinfo",NULL);

	if(gm_account)  {
		aFree(gm_account);
		gm_account = 0;
	}

	if(char_dat)  {
		aFree(char_dat);
		char_dat = 0;
	}

	///////////////////////////////////////////////////////////////////////////
	// delete sessions
	for(i = 0; i < fd_max; i++)
		if(session[i] != NULL)
			session_Delete(i);
	// clear externaly stored fd's
	login_fd=-1;
	char_fd=-1;
	///////////////////////////////////////////////////////////////////////////
	numdb_final(char_db_, char_db_final);

	sql_close();

	ShowMessage("ok! all done...\n");
}

void sql_config_read(const char *cfgName){ /* Kalaspuff, to get login_db */
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	ShowMessage("reading configure: %s\n", cfgName);

	if ((fp = safefopen(cfgName, "r")) == NULL) {
		ShowError("sql config file not found: %s\n", cfgName);
		return;
	}

	while(fgets(line, sizeof(line)-1, fp)){
		if( !skip_empty_line(line) )
			continue;

		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) != 2)
			continue;

		if(strcasecmp(w1, "login_db") == 0) {
			strcpy(login_db, w2);
		}else if(strcasecmp(w1,"char_db")==0){
			strcpy(char_db,w2);
		}else if(strcasecmp(w1,"cart_db")==0){
			strcpy(cart_db,w2);
		}else if(strcasecmp(w1,"inventory_db")==0){
			strcpy(inventory_db,w2);
		}else if(strcasecmp(w1,"charlog_db")==0){
			strcpy(charlog_db,w2);
		}else if(strcasecmp(w1,"storage_db")==0){
			strcpy(storage_db,w2);
		}else if(strcasecmp(w1,"reg_db")==0){
			strcpy(reg_db,w2);
		}else if(strcasecmp(w1,"skill_db")==0){
			strcpy(skill_db,w2);
		}else if(strcasecmp(w1,"interlog_db")==0){
			strcpy(interlog_db,w2);
		}else if(strcasecmp(w1,"memo_db")==0){
			strcpy(memo_db,w2);
		}else if(strcasecmp(w1,"guild_db")==0){
			strcpy(guild_db,w2);
		}else if(strcasecmp(w1,"guild_alliance_db")==0){
			strcpy(guild_alliance_db,w2);
		}else if(strcasecmp(w1,"guild_castle_db")==0){
			strcpy(guild_castle_db,w2);
		}else if(strcasecmp(w1,"guild_expulsion_db")==0){
			strcpy(guild_expulsion_db,w2);
		}else if(strcasecmp(w1,"guild_member_db")==0){
			strcpy(guild_member_db,w2);
		}else if(strcasecmp(w1,"guild_skill_db")==0){
			strcpy(guild_skill_db,w2);
		}else if(strcasecmp(w1,"guild_position_db")==0){
			strcpy(guild_position_db,w2);
		}else if(strcasecmp(w1,"guild_storage_db")==0){
			strcpy(guild_storage_db,w2);
		}else if(strcasecmp(w1,"party_db")==0){
			strcpy(party_db,w2);
		}else if(strcasecmp(w1,"pet_db")==0){
			strcpy(pet_db,w2);
		}else if(strcasecmp(w1,"db_path")==0){
			strcpy(db_path,w2);
		//Map server option to use SQL db or not
		}else if(strcasecmp(w1,"use_sql_db")==0){ // added for sql item_db read for char server [Valaris]
			db_use_sqldbs = config_switch(w2);
			ShowMessage("Using SQL dbs: %s\n",w2);
		//custom columns for login database
		}else if(strcasecmp(w1,"login_db_level")==0){
			strcpy(login_db_level,w2);
		}else if(strcasecmp(w1,"login_db_account_id")==0){
			strcpy(login_db_account_id,w2);
		//support the import command, just like any other config
		}else if(strcasecmp(w1,"import")==0){
			sql_config_read(w2);
		}

	}
	fclose(fp);
	ShowMessage("reading configure done.....\n");
}

int char_config_read(const char *cfgName) {
//	struct hostent *h = NULL;
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	if ((fp = safefopen(cfgName, "r")) == NULL) {
		ShowError("Configuration file not found: %s.\n", cfgName);
		return 0;
	}

	while(fgets(line, sizeof(line)-1, fp)) {
		if( !skip_empty_line(line) )
			continue;

		line[sizeof(line)-1] = '\0';
		if (sscanf(line,"%[^:]: %[^\r\n]", w1, w2) != 2)
			continue;

		remove_control_chars(w1);
		remove_control_chars(w2);
		if (strcasecmp(w1, "userid") == 0) {
			memcpy(userid, w2, 24);
		} else if (strcasecmp(w1, "passwd") == 0) {
			memcpy(passwd, w2, 24);
		} else if (strcasecmp(w1, "server_name") == 0) {
			memcpy(server_name, w2, 20);
			ShowStatus("%s server has been initialized\n", w2);
		} else if (strcasecmp(w1, "wisp_server_name") == 0) {
			if (strlen(w2) >= 4) {
				memcpy(wisp_server_name, w2, sizeof(wisp_server_name));
				wisp_server_name[sizeof(wisp_server_name) - 1] = '\0';
			}
		}

		else if (strcasecmp(w1, "login_ip") == 0) {
			loginaddress = w2;
			ShowInfo("Expecting login server at %s\n", loginaddress.getstring());
		}
else if(strcasecmp(w1, "login_port") == 0) {
	loginaddress.port() = atoi(w2);
}
		else if(strcasecmp(w1, "char_ip") == 0) {
			charaddress = w2;
			ShowInfo("Using char server with %s\n", loginaddress.getstring());
		}
else if(strcasecmp(w1, "char_port") == 0) {
	charaddress.LANPort() = atoi(w2);
}

		else if (strcasecmp(w1, "char_maintenance") == 0) {
			char_maintenance = atoi(w2);
		} else if (strcasecmp(w1, "char_new")==0){
			char_new = atoi(w2);
		} else if (strcasecmp(w1, "char_new_display")==0){
			char_new_display = atoi(w2);
		} else if (strcasecmp(w1, "max_connect_user") == 0) {
			max_connect_user = atoi(w2);
			if (max_connect_user < 0)
				max_connect_user = 0; // unlimited online players
		} else if(strcasecmp(w1, "gm_allow_level") == 0) {
			gm_allow_level = atoi(w2);
			if(gm_allow_level < 0)
				gm_allow_level = 99;
		} else if (strcasecmp(w1, "check_ip_flag") == 0) {
			check_ip_flag = config_switch(w2);
		} else if (strcasecmp(w1, "autosave_time") == 0) {
			autosave_interval = atoi(w2)*1000;
			if (autosave_interval <= 0)
				autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
		} else if (strcasecmp(w1, "start_point") == 0) {
			char map[64];
			int x, y;
			if (sscanf(w2,"%15[^,],%d,%d", map, &x, &y) < 3)
				continue;
			if (strstr(map, ".gat") != NULL) { // Verify at least if '.gat' is in the map name
				memcpy(start_point.map, map, 24);
				start_point.x = x;
				start_point.y = y;
			}
		} else if (strcasecmp(w1, "start_zeny") == 0) {
			start_zeny = atoi(w2);
			if (start_zeny < 0)
				start_zeny = 0;
		} else if (strcasecmp(w1, "start_weapon") == 0) {
			start_weapon = atoi(w2);
			if (start_weapon < 0)
				start_weapon = 0;
		} else if (strcasecmp(w1, "start_armor") == 0) {
			start_armor = atoi(w2);
			if (start_armor < 0)
				start_armor = 0;
		} else if(strcasecmp(w1,"log_char")==0){		//log char or not [devil]
			log_char = atoi(w2);
		} else if (strcasecmp(w1, "unknown_char_name") == 0) {
			strcpy(unknown_char_name, w2);
			unknown_char_name[23] = 0;
		} else if (strcasecmp(w1, "name_ignoring_case") == 0) {
			name_ignoring_case = config_switch(w2);
		} else if (strcasecmp(w1, "char_name_option") == 0) {
			char_name_option = atoi(w2);
		} else if (strcasecmp(w1, "char_name_letters") == 0) {
			strcpy(char_name_letters, w2);
		} else if (strcasecmp(w1, "party_modus") == 0) {
			party_modus = atoi(w2);
		} else if (strcasecmp(w1, "check_ip_flag") == 0) {
			check_ip_flag = config_switch(w2);
		} else if (strcasecmp(w1, "chars_per_account") == 0) { //maxchars per account [Sirius]
                        char_per_account = atoi(w2);
		} else if (strcasecmp(w1, "import") == 0) {
			char_config_read(w2);
		} else if (strcasecmp(w1, "console") == 0) {
			console = config_switch(w2);
        }
	}
	fclose(fp);

	return 0;
}

unsigned char getServerType(){
	return ATHENA_SERVER_CHAR | ATHENA_SERVER_INTER | ATHENA_SERVER_CORE;
}

int do_init(int argc, char **argv){
	int i;

	for(i = 0; i < MAX_MAP_SERVERS; i++) {
		memset(&server[i], 0, sizeof(struct mmo_map_server));
		server[i].fd = -1;
	}

	char_config_read((argc < 2) ? CHAR_CONF_NAME : argv[1]);
	sql_config_read(SQL_CONF_NAME);

	ShowMessage("charserver configuration reading done.....\n");

	//Read ItemDB
	do_init_itemdb();

	inter_init((argc > 2) ? argv[2] : inter_cfgName); // inter server ﾃﾊｱ篳ｭ
	ShowMessage("interserver configuration reading done.....\n");

	ShowMessage("start char server initializing.....\n");
	mmo_char_sql_init();
	ShowMessage("char server initializing done.....\n");

	ShowMessage("set parser -> parse_char().....\n");
	set_defaultparse(parse_char);

	char_fd = make_listen(charaddress.LANIP(),charaddress.LANPort());

	// send ALIVE PING to login server.
	ShowMessage("add interval tic (check_connect_login_server)....\n");
	add_timer_func_list(check_connect_login_server, "check_connect_login_server");
	i = add_timer_interval(gettick() + 10, 10*1000, check_connect_login_server, 0, 0);

	// send USER COUNT PING to login server.
	ShowMessage("add interval tic (send_users_tologin)....\n");
	add_timer_func_list(send_users_tologin, "send_users_tologin");
	i = add_timer_interval(gettick() + 10, 5*1000, send_users_tologin, 0, 0);



	read_gm_account();

	if ( console ) {
	    set_defaultconsoleparse(parse_console);
	   	start_console();
	}

	//Cleaning the tables for NULL entrys @ startup [Sirius]
	//Chardb clean
	ShowMessage("Cleaning the '%s' table...", char_db);
	sql_query("DELETE FROM `%s` WHERE `account_id` = '0'", char_db);
	ShowMessage(" done.\n");

	//guilddb clean
	ShowMessage("Cleaning the '%s' table...", guild_db);
	sql_query("DELETE FROM `%s` WHERE `guild_lv` = '0' AND `max_member` = '0' AND `exp` = '0' AND `next_exp` = '0' AND `average_lv` = '0'", guild_db);
	ShowMessage(" done.\n");

	//guildmemberdb clean
	ShowMessage("Cleaning the '%s' table...", guild_member_db);
	sql_query("DELETE FROM `%s` WHERE `guild_id` = '0' AND `account_id` = '0' AND `char_id` = '0'", guild_member_db);
	ShowMessage(" done.\n");

	ShowStatus("The char-server is "CL_LT_GREEN"ready"CL_NORM" (listening on %s:%d).\n\n", charaddress.LANIP().getstring(),charaddress.LANPort());
	return 0;
}




int char_child(int parent_id, int child_id) {
	int tmp_id = 0;
	sql_query("SELECT `child` FROM `%s` WHERE `char_id` = '%d'", char_db, parent_id);
	if (sql_res && sql_fetch_row ()) {
		tmp_id = atoi (sql_row[0]);
		sql_free();
	}
	else
		ShowMessage("CHAR: child Failed!\n");

	if ( tmp_id == child_id )
		return 1;
	else
		return 0;
}

int char_married(int pl1,int pl2) {
	int tmp_id = 0;
	sql_query("SELECT `partner_id` FROM `%s` WHERE `char_id` = '%d'", char_db, pl1);
	if (sql_res && sql_fetch_row ()) {
		tmp_id = atoi (sql_row[0]);
		sql_free();
	}
	else
		ShowMessage("CHAR: married Failed!\n");
	if ( tmp_id == pl2 )
		return 1;
	else
		return 0;
}

int char_nick2id (char *name) {
	int char_id = 0;
	sql_query( "SELECT `char_id` FROM `%s` WHERE `name` = '%s'", char_db, name);
	if (sql_res && sql_fetch_row ()) {
		char_id = atoi (sql_row[0]);
		sql_free();
	}
	else
		ShowMessage ("CHAR: nick2id Failed!\n");
	return char_id;
}

