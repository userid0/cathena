// Logging functions by Azndragon & Codemaster
#include "base.h"
#include "../common/strlib.h"
#include "../common/nullpo.h"
#include "itemdb.h"
#include "map.h"
#include "showmsg.h"
#include "utils.h"
#include "log.h"

struct LogConfig log_config;

char timestring[255];
time_t curtime;

//0 = none, 1024 = any
//Bits |
//1 - Healing items (Potions)
//2 - Usable Items
//4 - Etc Items
//8 - Weapon
//16 - Shields,Armor,Headgears,Accessories,etc
//32 - Cards
//64 - Pet Accessories
//128 - Eggs (well, monsters don't drop 'em but we'll use the same system for ALL logs)
//256 - Log expensive items ( >= price_log)
//512 - Log big amount of items ( >= amount_log)
int slog_healing = 0;
int slog_usable = 0;
int slog_etc = 0;
int slog_weapon = 0;
int slog_armor = 0;
int slog_card = 0;
int slog_petacc = 0;
int slog_egg = 0;
int slog_expensive = 0;
int slog_amount = 0;

//check if this item should be logger according the settings
int should_log_item(int nameid) {
	struct item_data *item_data;

	if (nameid<512 || (item_data= itemdb_search(nameid)) == NULL) return 0;

	if (slog_expensive && item_data->value_buy >= log_config.price_items_log ) return item_data->nameid;
	if (slog_healing && item_data->type == 0 ) return item_data->nameid;
	if (slog_etc && item_data->type == 3 ) return item_data->nameid;
	if (slog_weapon && item_data->type == 4 ) return item_data->nameid;
	if (slog_armor && item_data->type == 5 ) return item_data->nameid;
	if (slog_card && item_data->type == 6 ) return item_data->nameid;
	if (slog_petacc && item_data->type == 8 ) return item_data->nameid;
	if (slog_egg && item_data->type == 7 ) return item_data->nameid;
	return 0;
}

int log_branch(struct map_session_data *sd)
{
	#ifndef TXT_ONLY
		char t_name[100];
	#endif
	FILE *logfp;

	if(log_config.enable_logs <= 0)
		return 0;
	nullpo_retr(0, sd);
	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
		sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`branch_date`, `account_id`, `char_id`, `char_name`, `map`) VALUES (NOW(), '%d', '%d', '%s', '%s')",
			log_config.log_branch_db, sd->status.account_id, sd->status.char_id, jstrescapecpy(t_name, sd->status.name), sd->mapname);
		if(mysql_query(&mmysql_handle, tmp_sql))
			ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_branch,"a+")) != NULL) {
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d:%d]\t%s%s", timestring, sd->status.name, sd->status.account_id, sd->status.char_id, sd->mapname, RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 0;
}

int log_drop(struct map_session_data *sd, int monster_id, int *log_drop)
{
	FILE *logfp;
	int i,flag = 0;

	if(log_config.enable_logs <= 0)
		return 0;
	nullpo_retr(0, sd);
	for (i = 0; i<10; i++) { //Should we log these items? [Lupus]
		flag += should_log_item(log_drop[i]);
	}
	if (flag==0) return 0; //we skip logging this items set - they doesn't met our logging conditions [Lupus]

	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
		sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`drop_date`, `kill_char_id`, `monster_id`, `item1`, `item2`, `item3`, `item4`, `item5`, `item6`, `item7`, `item8`, `item9`, `itemCard`, `map`) VALUES (NOW(), '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%s') ", log_config.log_drop_db, sd->status.char_id, monster_id, log_drop[0], log_drop[1], log_drop[2], log_drop[3], log_drop[4], log_drop[5], log_drop[6], log_drop[7], log_drop[8], log_drop[9], sd->mapname);
		if(mysql_query(&mmysql_handle, tmp_sql))
			ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_drop,"a+")) != NULL) {
			

			time_t curtime;
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d:%d]\t%d\t%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%s", timestring, sd->status.name, sd->status.account_id, sd->status.char_id, monster_id, log_drop[0], log_drop[1], log_drop[2], log_drop[3], log_drop[4], log_drop[5], log_drop[6], log_drop[7], log_drop[8], log_drop[9], RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 1; //Logged
}

int log_mvpdrop(struct map_session_data *sd, int monster_id, int *log_mvp)
{
	FILE *logfp;

	if(log_config.enable_logs <= 0)
		return 0;
	nullpo_retr(0, sd);
	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
		sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`mvp_date`, `kill_char_id`, `monster_id`, `prize`, `mvpexp`, `map`) VALUES (NOW(), '%d', '%d', '%d', '%d', '%s') ", log_config.log_mvpdrop_db, sd->status.char_id, monster_id, log_mvp[0], log_mvp[1], sd->mapname);
		if(mysql_query(&mmysql_handle, tmp_sql))
			ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_mvpdrop,"a+")) != NULL) {
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d:%d]\t%d\t%d,%d%s", timestring, sd->status.name, sd->status.account_id, sd->status.char_id, monster_id, log_mvp[0], log_mvp[1], RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 0;
}

int log_present(struct map_session_data *sd, int source_type, int nameid)
{
	FILE *logfp;
	#ifndef TXT_ONLY
		char t_name[100];
	#endif

	if(log_config.enable_logs <= 0)
		return 0;
	nullpo_retr(0, sd);
	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
		sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`present_date`, `src_id`, `account_id`, `char_id`, `char_name`, `nameid`, `map`) VALUES (NOW(), '%d', '%d', '%d', '%s', '%d', '%s') ",
			log_config.log_present_db, source_type, sd->status.account_id, sd->status.char_id, jstrescapecpy(t_name, sd->status.name), nameid, sd->mapname);
		if(mysql_query(&mmysql_handle, tmp_sql))
			ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_present,"a+")) != NULL) {
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d:%d]\t%d\t%d%s", timestring, sd->status.name, sd->status.account_id, sd->status.char_id, source_type, nameid, RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 0;
}

int log_produce(struct map_session_data *sd, int nameid, int slot1, int slot2, int slot3, int success)
{
	FILE *logfp;
	#ifndef TXT_ONLY
		char t_name[100];
	#endif

	if(log_config.enable_logs <= 0)
		return 0;
	nullpo_retr(0, sd);
	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
		sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`produce_date`, `account_id`, `char_id`, `char_name`, `nameid`, `slot1`, `slot2`, `slot3`, `map`, `success`) VALUES (NOW(), '%d', '%d', '%s', '%d', '%d', '%d', '%d', '%s', '%d') ",
			log_config.log_produce_db, sd->status.account_id, sd->status.char_id, jstrescapecpy(t_name, sd->status.name), nameid, slot1, slot2, slot3, sd->mapname, success);
		if(mysql_query(&mmysql_handle, tmp_sql))
			ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_produce,"a+")) != NULL) {
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d:%d]\t%d\t%d,%d,%d\t%d%s", timestring, sd->status.name, sd->status.account_id, sd->status.char_id, nameid, slot1, slot2, slot3, success, RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 0;
}

int log_refine(struct map_session_data *sd, int n, int success)
{
	FILE *logfp;
	int log_card[4];
	int item_level;
	int i;
	#ifndef TXT_ONLY
		char t_name[100];
	#endif

	if(log_config.enable_logs <= 0)
		return 0;

	nullpo_retr(0, sd);

	if(success == 0)
		item_level = 0;
	else
		item_level = sd->status.inventory[n].refine + 1;

	for(i=0;i<4;i++)
		log_card[i] = sd->status.inventory[n].card[i];

	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
		sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`refine_date`, `account_id`, `char_id`, `char_name`, `nameid`, `refine`, `card0`, `card1`, `card2`, `card3`, `map`, `success`, `item_level`) VALUES (NOW(), '%d', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%s', '%d', '%d')",
			log_config.log_refine_db, sd->status.account_id, sd->status.char_id, jstrescapecpy(t_name, sd->status.name), sd->status.inventory[n].nameid, sd->status.inventory[n].refine, log_card[0], log_card[1], log_card[2], log_card[3], sd->mapname, success, item_level);
		if(mysql_query(&mmysql_handle, tmp_sql))
			ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_refine,"a+")) != NULL) {
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d:%d]\t%d,%d\t%d%d%d%d\t%d,%d%s", timestring, sd->status.name, sd->status.account_id, sd->status.char_id, sd->status.inventory[n].nameid, sd->status.inventory[n].refine, log_card[0], log_card[1], log_card[2], log_card[3], success, item_level, RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 0;
}

int log_trade(struct map_session_data *sd, struct map_session_data *target_sd, int n,int amount)
{
	FILE *logfp;
	int log_nameid, log_amount, log_refine, log_card[4];
	int i;
	#ifndef TXT_ONLY
		char t_name[100],t_name2[100];
	#endif

	if(log_config.enable_logs <= 0)
		return 0;

	nullpo_retr(0, sd);

	if(sd->status.inventory[n].nameid==0 || amount <= 0 || sd->status.inventory[n].amount<amount || sd->inventory_data[n] == NULL)
		return 1;

	if(sd->status.inventory[n].amount < 0)
		return 1;

	log_nameid = sd->status.inventory[n].nameid;
	log_amount = sd->status.inventory[n].amount;
	log_refine = sd->status.inventory[n].refine;

	for(i=0;i<4;i++)
		log_card[i] = sd->status.inventory[n].card[i];

	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
		sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`trade_date`, `src_account_id`, `src_char_id`, `src_char_name`, `des_account_id`, `des_char_id`, `des_char_name`, `nameid`, `amount`, `refine`, `card0`, `card1`, `card2`, `card3`, `map`) VALUES (NOW(), '%d', '%d', '%s', '%d', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%s')",
			log_config.log_trade_db, sd->status.account_id, sd->status.char_id, jstrescapecpy(t_name, sd->status.name), target_sd->status.account_id, target_sd->status.char_id, jstrescapecpy(t_name2, target_sd->status.name), log_nameid, log_amount, log_refine, log_card[0], log_card[1], log_card[2], log_card[3], sd->mapname);
		if(mysql_query(&mmysql_handle, tmp_sql))
			ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_trade,"a+")) != NULL) {
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d:%d]\t%s[%d:%d]\t%d\t%d\t%d\t%d,%d,%d,%d%s", timestring, sd->status.name, sd->status.account_id, sd->status.char_id, target_sd->status.name, target_sd->status.account_id, target_sd->status.char_id, log_nameid, log_amount, log_refine, log_card[0], log_card[1], log_card[2], log_card[3], RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 0;
}

int log_vend(struct map_session_data *sd,struct map_session_data *vsd,int n,int amount, int zeny)
{
	FILE *logfp;
	int log_nameid, log_amount, log_refine, log_card[4];
	int i;
	#ifndef TXT_ONLY
		char t_name[100],t_name2[100];
	#endif

	if(log_config.enable_logs <= 0)
		return 0;
	nullpo_retr(0, sd);

	if(sd->status.inventory[n].nameid==0 || amount <= 0 || sd->status.inventory[n].amount<amount || sd->inventory_data[n] == NULL)
		return 1;
	if(sd->status.inventory[n].amount< 0)
		return 1;

	log_nameid = sd->status.inventory[n].nameid;
	log_amount = sd->status.inventory[n].amount;
	log_refine = sd->status.inventory[n].refine;
	for(i=0;i<4;i++)
		log_card[i] = sd->status.inventory[n].card[i];

	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
			sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`vend_date`, `vend_account_id`, `vend_char_id`, `vend_char_name`, `buy_account_id`, `buy_char_id`, `buy_char_name`, `nameid`, `amount`, `refine`, `card0`, `card1`, `card2`, `card3`, `map`, `zeny`) VALUES (NOW(), '%d', '%d', '%s', '%d', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%s', '%d')",
				log_config.log_vend_db, sd->status.account_id, sd->status.char_id, jstrescapecpy(t_name, sd->status.name), vsd->status.account_id, vsd->status.char_id, jstrescapecpy(t_name2, vsd->status.name), log_nameid, log_amount, log_refine, log_card[0], log_card[1], log_card[2], log_card[3], sd->mapname, zeny);
			if(mysql_query(&mmysql_handle, tmp_sql))
				ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_vend,"a+")) != NULL) {
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d:%d]\t%s[%d:%d]\t%d\t%d\t%d\t%d,%d,%d,%d\t%d%s", timestring, sd->status.name, sd->status.account_id, sd->status.char_id, vsd->status.name, vsd->status.account_id, vsd->status.char_id, log_nameid, log_amount, log_refine, log_card[0], log_card[1], log_card[2], log_card[3], zeny, RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 0;
}

int log_zeny(struct map_session_data *sd, struct map_session_data *target_sd,int amount)
{
	FILE *logfp;
	#ifndef TXT_ONLY
		char t_name[100],t_name2[100];
	#endif

	if(log_config.enable_logs <= 0)
		return 0;
	nullpo_retr(0, sd);
	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
		sprintf(tmp_sql,"INSERT DELAYED INTO `%s` (`trade_date`, `src_account_id`, `src_char_id`, `src_char_name`, `des_account_id`, `des_char_id`, `des_char_name`, `map`, `zeny`) VALUES (NOW(), '%d', '%d', '%s', '%d', '%d', '%s', '%s', '%d')",
			log_config.log_trade_db, sd->status.account_id, sd->status.char_id, jstrescapecpy(t_name, sd->status.name), target_sd->status.account_id, target_sd->status.char_id, jstrescapecpy(t_name2, target_sd->status.name), sd->mapname, sd->deal_zeny);
		if(mysql_query(&mmysql_handle, tmp_sql))
			ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_trade,"a+")) != NULL) {
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d]\t%s[%d]\t%d\t%s", timestring, sd->status.name, sd->status.account_id, target_sd->status.name, target_sd->status.account_id, sd->deal_zeny, RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 0;
}

int log_atcommand(struct map_session_data *sd, const char *message)
{
	FILE *logfp;
	#ifndef TXT_ONLY
		char t_name[100];
	#endif

	if(log_config.enable_logs <= 0)
		return 0;
	nullpo_retr(0, sd);
	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
		sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`atcommand_date`, `account_id`, `char_id`, `char_name`, `map`, `command`) VALUES(NOW(), '%d', '%d', '%s', '%s', '%s') ",
			log_config.log_gm_db, sd->status.account_id, sd->status.char_id, jstrescapecpy(t_name, sd->status.name), sd->mapname, message);
		if(mysql_query(&mmysql_handle, tmp_sql))
			ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_gm,"a+")) != NULL) {
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d]: %s%s",timestring,sd->status.name,sd->status.account_id,message,RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 0;
}

int log_npc(struct map_session_data *sd, const char *message)
{	//[Lupus]
	FILE *logfp;
	#ifndef TXT_ONLY
		char t_name[100];
	#endif

	if(log_config.enable_logs <= 0)
		return 0;
	nullpo_retr(0, sd);
	#ifndef TXT_ONLY
	if(log_config.sql_logs > 0)
	{
		sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`npc_date`, `account_id`, `char_id`, `char_name`, `map`, `mes`) VALUES(NOW(), '%d', '%d', '%s', '%s', '%s') ",
			log_config.log_npc_db, sd->status.account_id, sd->status.char_id, jstrescapecpy(t_name, sd->status.name), sd->mapname, message);
		if(mysql_query(&mmysql_handle, tmp_sql))
			ShowMessage("DB server Error - %s\n",mysql_error(&mmysql_handle));
	} else {
	#endif
		if((logfp=savefopen(log_config.log_npc,"a+")) != NULL) {
			time(&curtime);
			strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
			fprintf(logfp,"%s - %s[%d]: %s%s",timestring,sd->status.name,sd->status.account_id,message,RETCODE);
			fclose(logfp);
		}
	#ifndef TXT_ONLY
	}
	#endif
	return 0;
}

int log_config_read(const char *cfgName)
{
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	if((fp = savefopen(cfgName, "r")) == NULL)
	{
		ShowMessage("Log configuration file not found at: %s\n", cfgName);
		return 1;
	}

	//Default values
	log_config.what_items_log = 1023; //log any items
	log_config.price_items_log = 1000;
	log_config.amount_items_log = 100;

	while(fgets(line, sizeof(line) -1, fp))
	{
		if(line[0] == '/' && line[1] == '/')
			continue;

		if(sscanf(line, "%[^:]: %[^\r\n]", w1, w2) == 2)
		{
			if(strcasecmp(w1,"enable_logs") == 0) {
				log_config.enable_logs = (atoi(w2));
			} else if(strcasecmp(w1,"sql_logs") == 0) {
				log_config.sql_logs = (atoi(w2));
			} else if(strcasecmp(w1,"what_items_log") == 0) {
				log_config.what_items_log = (atoi(w2));

//Bits |
//1 - Healing items (Potions)
//2 - Usable Items
//4 - Etc Items
//8 - Weapon
//16 - Shields,Armor,Headgears,Accessories,etc
//32 - Cards
//64 - Pet Accessories
//128 - Eggs (well, monsters don't drop 'em but we'll use the same system for ALL logs)
//256 - Log expensive items ( >= price_log)
//512 - Log big amount of items ( >= amount_log)
				slog_healing = log_config.what_items_log&1;
				slog_usable = log_config.what_items_log&2;
				slog_etc = log_config.what_items_log&4;
				slog_weapon = log_config.what_items_log&8;
				slog_armor = log_config.what_items_log&16;
				slog_card = log_config.what_items_log&32;
				slog_petacc = log_config.what_items_log&64;
				slog_egg = log_config.what_items_log&128;
				slog_expensive = log_config.what_items_log&256;
				slog_amount = log_config.what_items_log&512;

			} else if(strcasecmp(w1,"price_items_log") == 0) {
				log_config.price_items_log = (atoi(w2));
			} else if(strcasecmp(w1,"amount_items_log") == 0) {
				log_config.amount_items_log = (atoi(w2));
			} else if(strcasecmp(w1,"log_branch") == 0) {
				log_config.branch = (atoi(w2));
			} else if(strcasecmp(w1,"log_drop") == 0) {
				log_config.drop = (atoi(w2));
			} else if(strcasecmp(w1,"log_mvpdrop") == 0) {
				log_config.mvpdrop = (atoi(w2));
			} else if(strcasecmp(w1,"log_present") == 0) {
				log_config.present = (atoi(w2));
			} else if(strcasecmp(w1,"log_produce") == 0) {
				log_config.produce = (atoi(w2));
			} else if(strcasecmp(w1,"log_refine") == 0) {
				log_config.refine = (atoi(w2));
			} else if(strcasecmp(w1,"log_trade") == 0) {
				log_config.trade = (atoi(w2));
			} else if(strcasecmp(w1,"log_vend") == 0) {
				log_config.vend = (atoi(w2));
			} else if(strcasecmp(w1,"log_zeny") == 0) {
				if(log_config.trade != 1)
					log_config.zeny = 0;
				else
					log_config.zeny = (atoi(w2));
			} else if(strcasecmp(w1,"log_gm") == 0) {				
				log_config.gm = (atoi(w2));
			} else if(strcasecmp(w1,"log_npc") == 0) {
				log_config.npc = (atoi(w2));
			}

		#ifndef TXT_ONLY
			else if(strcasecmp(w1, "log_branch_db") == 0) {
				strcpy(log_config.log_branch_db, w2);
				if(log_config.branch == 1)
					ShowMessage("Logging Dead Branch Usage to table `%s`\n", w2);
			} else if(strcasecmp(w1, "log_drop_db") == 0) {
				strcpy(log_config.log_drop_db, w2);
				if(log_config.drop == 1)
					ShowMessage("Logging Item Drops to table `%s`\n", w2);
			} else if(strcasecmp(w1, "log_mvpdrop_db") == 0) {
				strcpy(log_config.log_mvpdrop_db, w2);
				if(log_config.mvpdrop == 1)
					ShowMessage("Logging MVP Drops to table `%s`\n", w2);
			} else if(strcasecmp(w1, "log_present_db") == 0) {
				strcpy(log_config.log_present_db, w2);
				if(log_config.present == 1)
					ShowMessage("Logging Present Usage & Results to table `%s`\n", w2);
			} else if(strcasecmp(w1, "log_produce_db") == 0) {
				strcpy(log_config.log_produce_db, w2);
				if(log_config.produce == 1)
					ShowMessage("Logging Producing to table `%s`\n", w2);
			} else if(strcasecmp(w1, "log_refine_db") == 0) {
				strcpy(log_config.log_refine_db, w2);
				if(log_config.refine == 1)
					ShowMessage("Logging Refining to table `%s`\n", w2);
			} else if(strcasecmp(w1, "log_trade_db") == 0) {
				strcpy(log_config.log_trade_db, w2);
				if(log_config.trade == 1)
				{
					ShowMessage("Logging Item Trades");
					if(log_config.zeny == 1)
						ShowMessage("and Zeny Trades");
					ShowMessage(" to table `%s`\n", w2);
				}
			} else if(strcasecmp(w1, "log_vend_db") == 0) {
				strcpy(log_config.log_vend_db, w2);
				if(log_config.vend == 1)
					ShowMessage("Logging Vending to table `%s`\n", w2);
			} else if(strcasecmp(w1, "log_gm_db") == 0) {
				strcpy(log_config.log_gm_db, w2);
				if(log_config.gm > 0)
					ShowMessage("Logging GM Level %d Commands to table `%s`\n", log_config.gm, w2);
			} else if(strcasecmp(w1, "log_npc_db") == 0) {
				strcpy(log_config.log_npc_db, w2);
				if(log_config.npc > 0)
					ShowMessage("Logging NPC 'logmes' to table `%s`\n", w2);
			}
		#endif

			else if(strcasecmp(w1, "log_branch_file") == 0) {
				strcpy(log_config.log_branch, w2);
				if(log_config.branch > 0 && log_config.sql_logs < 1)
					ShowMessage("Logging Dead Branch Usage to file `%s`.txt\n", w2);
			} else if(strcasecmp(w1, "log_drop_file") == 0) {
				strcpy(log_config.log_drop, w2);
				if(log_config.drop > 0 && log_config.sql_logs < 1)
					ShowMessage("Logging Item Drops to file `%s`.txt\n", w2);
			} else if(strcasecmp(w1, "log_mvpdrop_file") == 0) {
				strcpy(log_config.log_mvpdrop, w2);
				if(log_config.mvpdrop > 0 && log_config.sql_logs < 1)
					ShowMessage("Logging MVP Drops to file `%s`.txt\n", w2);
			} else if(strcasecmp(w1, "log_present_file") == 0) {
				strcpy(log_config.log_present, w2);
				if(log_config.present > 0 && log_config.sql_logs < 1)
					ShowMessage("Logging Present Usage & Results to file `%s`.txt\n", w2);
			} else if(strcasecmp(w1, "log_produce_file") == 0) {
				strcpy(log_config.log_produce, w2);
				if(log_config.produce > 0 && log_config.sql_logs < 1)
					ShowMessage("Logging Producing to file `%s`.txt\n", w2);
			} else if(strcasecmp(w1, "log_refine_file") == 0) {
				strcpy(log_config.log_refine, w2);
				if(log_config.refine > 0 && log_config.sql_logs < 1)
					ShowMessage("Logging Refining to file `%s`.txt\n", w2);
			} else if(strcasecmp(w1, "log_trade_file") == 0) {
				strcpy(log_config.log_trade, w2);
				if(log_config.trade > 0 && log_config.sql_logs < 1)
				{
					ShowMessage("Logging Item Trades");
					if(log_config.zeny > 0)
						ShowMessage("and Zeny Trades");
					ShowMessage(" to file `%s`.txt\n", w2);
				}
			} else if(strcasecmp(w1, "log_vend_file") == 0) {
				strcpy(log_config.log_vend, w2);
				if(log_config.vend > 0  && log_config.sql_logs < 1)
					ShowMessage("Logging Vending to file `%s`.txt\n", w2);
			} else if(strcasecmp(w1, "log_gm_file") == 0) {
				strcpy(log_config.log_gm, w2);
				if(log_config.gm > 0 && log_config.sql_logs < 1)
					ShowMessage("Logging GM Level %d Commands to file `%s`.txt\n", log_config.gm, w2);
			} else if(strcasecmp(w1, "log_npc_file") == 0) {
				strcpy(log_config.log_npc, w2);
				if(log_config.npc > 0 && log_config.sql_logs < 1)
					ShowMessage("Logging NPC 'logmes' to file `%s`.txt\n", w2);
			//support the import command, just like any other config
			} else if(strcasecmp(w1,"import") == 0) {
				log_config_read(w2);
			}
		}
	}

	fclose(fp);
	return 0;
}
