// Homunculus saving by Albator and Orn for eAthena.
// GNU/GPL rulez !

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "char.h"
#include "../common/strlib.h"
#include "../common/showmsg.h"

struct s_homunculus *homun_pt;

#ifndef SQL_DEBUG

#define mysql_query(_x, _y) mysql_real_query(_x, _y, strlen(_y)) //supports ' in names and runs faster [Kevin]

#else

#define mysql_query(_x, _y) debug_mysql_query(__FILE__, __LINE__, _x, _y)

#endif


int inter_homunculus_sql_init(void){
	//memory alloc
	homun_pt = (struct s_homunculus*)aCalloc(sizeof(struct s_homunculus), 1);
	return 0;
}
void inter_homunculus_sql_final(void){
	if (homun_pt) aFree(homun_pt);
	return;
}

int mapif_saved_homunculus(int fd, short flag)
{
	WFIFOW(fd,0) = 0x3892;
	if(flag==1)
		WFIFOB(fd,2) = 1;
	else
		WFIFOB(fd,2) = 0;
	WFIFOSET(fd, 3);
	return 0;
}
int mapif_info_homunculus(int fd, int account_id, struct s_homunculus *hd)
{
	WFIFOW(fd,0) = 0x3891;
	WFIFOW(fd,2) = sizeof(struct s_homunculus)+9;
	WFIFOL(fd,4) = account_id;
	WFIFOB(fd,8) = 1; // account loaded with success

	memcpy(WFIFOP(fd,9), hd, sizeof(struct s_homunculus));
	WFIFOSET(fd, sizeof(struct s_homunculus)+9);
	return 0;
}

int mapif_homunculus_deleted(int fd, int flag)
{
	WFIFOW(fd, 0) = 0x3893;
	if(flag == 1)
		WFIFOB(fd,2) = 1; // Homunculus deleted
	else
		WFIFOB(fd,2) = 0; /* Fail /!\ */

    WFIFOSET(fd, 3);

	return 0;

}
int mapif_homunculus_created(int fd, int account_id, struct s_homunculus *sh, short flag)
{
	WFIFOW(fd, 0) =0x3890;
	WFIFOL(fd,2) = account_id;
	WFIFOL(fd,6) = sh->char_id;
	if(flag==1){
		WFIFOW(fd, 10)=1;
		WFIFOL(fd,12) = sh->hom_id;
    }
    else{
		WFIFOW(fd, 10)=0;
		WFIFOL(fd,12) = 0;
	}
	WFIFOSET(fd, 16);

	return 0;
}
void init_homun_skills(struct s_homunculus *hd)
{
	int i;
	for(i=0;i<MAX_HOMUNSKILL;i++)
		hd->hskill[i].id = hd->hskill[i].lv = hd->hskill[i].flag = 0;
}

// Save/Update Homunculus Skills
int mapif_save_homunculus_skills(struct s_homunculus *hd)
{
	int i;

	for(i=0; i<MAX_HOMUNSKILL; i++)
	{
		if(hd->hskill[i].id != 0 && hd->hskill[i].lv != 0 )
		{
			sprintf(tmp_sql,"REPLACE INTO `skill_homunculus` (`homun_id`, `id`, `lv`) VALUES (%d, %d, %d)",
				hd->hom_id, hd->hskill[i].id, hd->hskill[i].lv);

			if(mysql_query(&mysql_handle, tmp_sql)){
				ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
				ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
				return 0;
			}
		}
	}

	return 1;
}

int mapif_save_homunculus(int fd, int account_id, struct s_homunculus *hd)
{
	int flag =1;
	char t_name[NAME_LENGTH*2];
	jstrescapecpy(t_name, hd->name);

	if(hd->hom_id==0) // new homunculus
	{
		ShowInfo("New homunculus name : %s\n",hd->name);

		sprintf(tmp_sql, "INSERT INTO `homunculus` "
			"(`char_id`, `class`,`name`,`level`,`exp`,`intimacy`,`hunger`, `str`, `agi`, `vit`, `int`, `dex`, `luk`, `hp`,`max_hp`,`sp`,`max_sp`,`skill_point`, `rename_flag`, `vaporize`) "
			"VALUES ('%d', '%d', '%s', '%d', '%lu', '%lu', '%d', '%d', %d, '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
			hd->char_id, hd->class_,t_name,hd->level,hd->exp,hd->intimacy,hd->hunger, hd->str, hd->agi, hd->vit, hd->int_, hd->dex, hd->luk,
			hd->hp,hd->max_hp,hd->sp,hd->max_sp, hd->skillpts, hd->rename_flag, hd->vaporize);

	}
	else
	{
		sprintf(tmp_sql, "UPDATE `homunculus` SET `char_id`='%d', `class`='%d',`name`='%s',`level`='%d',`exp`='%lu',`intimacy`='%lu',`hunger`='%d', `str`='%d', `agi`='%d', `vit`='%d', `int`='%d', `dex`='%d', `luk`='%d', `hp`='%d',`max_hp`='%d',`sp`='%d',`max_sp`='%d',`skill_point`='%d', `rename_flag`='%d', `vaporize`='%d' WHERE `homun_id`='%d'",
			hd->char_id, hd->class_,t_name,hd->level,hd->exp,hd->intimacy,hd->hunger, hd->str, hd->agi, hd->vit, hd->int_, hd->dex, hd->luk,
			hd->hp,hd->max_hp,hd->sp,hd->max_sp, hd->skillpts, hd->rename_flag, hd->vaporize, hd->hom_id);
	}

	if(mysql_query(&mysql_handle, tmp_sql)){
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		flag =  0;
	}

	if(hd->hom_id==0 && flag!=0)
		hd->hom_id = (int)mysql_insert_id(&mysql_handle); // new homunculus
	else
	{
		flag = mapif_save_homunculus_skills(hd);
		mapif_saved_homunculus(fd, flag);
	}
	return flag;
}



// Load an homunculus
int mapif_load_homunculus(int fd){
	int i;
	memset(homun_pt, 0, sizeof(struct s_homunculus));

	sprintf(tmp_sql,"SELECT `homun_id`,`char_id`,`class`,`name`,`level`,`exp`,`intimacy`,`hunger`, `str`, `agi`, `vit`, `int`, `dex`, `luk`, `hp`,`max_hp`,`sp`,`max_sp`,`skill_point`,`rename_flag`, `vaporize` FROM `homunculus` WHERE `homun_id`='%lu'", RFIFOL(fd,6));
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);

		return 0;
	}
	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res!=NULL && mysql_num_rows(sql_res)>0) {
		sql_row = mysql_fetch_row(sql_res);

		homun_pt->hom_id = RFIFOL(fd,6) ; //RFIFOL(fd,2);
		homun_pt->class_ = atoi(sql_row[2]);
		memcpy(homun_pt->name, sql_row[3],NAME_LENGTH-1);
		homun_pt->char_id = atoi(sql_row[1]);
		homun_pt->level = atoi(sql_row[4]);
		homun_pt->exp = atoi(sql_row[5]);
		homun_pt->intimacy = atoi(sql_row[6]);
		homun_pt->hunger = atoi(sql_row[7]);
		homun_pt->str = atoi(sql_row[8]);
		homun_pt->agi = atoi(sql_row[9]);
		homun_pt->vit = atoi(sql_row[10]);
		homun_pt->int_ = atoi(sql_row[11]);
		homun_pt->dex = atoi(sql_row[12]);
		homun_pt->luk = atoi(sql_row[13]);
		homun_pt->hp = atoi(sql_row[14]);
		homun_pt->max_hp = atoi(sql_row[15]);
		homun_pt->sp = atoi(sql_row[16]);
		homun_pt->max_sp = atoi(sql_row[17]);
		homun_pt->skillpts = atoi(sql_row[18]);
		homun_pt->rename_flag = atoi(sql_row[19]);
		homun_pt->vaporize = atoi(sql_row[20]);
	}
	if(homun_pt->hunger < 0)
		homun_pt->hunger = 0;
	else if(homun_pt->hunger > 100)
		homun_pt->hunger = 100;
	if(homun_pt->intimacy < 0)
		homun_pt->intimacy = 0;
	else if(homun_pt->intimacy > 100000)
		homun_pt->intimacy = 100000;

	mysql_free_result(sql_res);

	// Load Homunculus Skill
	init_homun_skills(homun_pt); //bousille homun_pt !!!

	sprintf(tmp_sql,"SELECT `id`,`lv` FROM `skill_homunculus` WHERE `homun_id`=%d",homun_pt->hom_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
		ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
		return 0;
	}
	sql_res = mysql_store_result(&mysql_handle);
         if(sql_res){
         	while((sql_row = mysql_fetch_row(sql_res))){
						i = (atoi(sql_row[0])-HM_SKILLBASE-1);
						homun_pt->hskill[i].id = atoi(sql_row[0]);
                        homun_pt->hskill[i].lv = atoi(sql_row[1]);
                 }
         }

	mysql_free_result(sql_res);

	ShowInfo("Homunculus loaded (%d - %s).\n", homun_pt->hom_id, homun_pt->name);
	return mapif_info_homunculus(fd, RFIFOL(fd,2), homun_pt);
}



int mapif_delete_homunculus(int fd)
{
	sprintf(tmp_sql, "DELETE FROM `homunculus` WHERE `homun_id` = '%lu'", RFIFOL(fd,2));
   if(mysql_query(&mysql_handle, tmp_sql)){
     ShowSQL("DB error - %s\n",mysql_error(&mysql_handle));
     ShowDebug("at %s:%d - %s\n", __FILE__,__LINE__,tmp_sql);
   return mapif_homunculus_deleted(fd, 0);
   }

	 return mapif_homunculus_deleted(fd, 1);
}



int mapif_parse_CreateHomunculus(int fd)
{
	memset(homun_pt, 0, sizeof(struct s_homunculus));

	/* Data from packet */
	homun_pt->char_id = RFIFOL(fd,6);
	homun_pt->class_ = RFIFOW(fd,10);
	homun_pt->max_hp = RFIFOL(fd,12);
	homun_pt->max_sp = RFIFOL(fd,16);
	memcpy(homun_pt->name, (char*)RFIFOP(fd, 20), NAME_LENGTH-1);
	homun_pt->str = RFIFOL(fd,44);
	homun_pt->agi = RFIFOL(fd,48);
	homun_pt->vit = RFIFOL(fd,52);
	homun_pt->int_ = RFIFOL(fd,56);
	homun_pt->dex = RFIFOL(fd,60);
	homun_pt->luk = RFIFOL(fd,64);

	/* Const data for each creation*/
	homun_pt->hom_id = 0;
	homun_pt->exp =0;
	homun_pt->hp = 10 ;
	homun_pt->sp = 0 ;
	homun_pt->rename_flag = 0;
	homun_pt->skillpts =0;
	homun_pt->hunger = 32;
	homun_pt->level=1;
	homun_pt->intimacy = 21;

	// Save in sql db
	if(mapif_save_homunculus(fd,RFIFOL(fd,2), homun_pt))
		return mapif_homunculus_created(fd, RFIFOL(fd,2), homun_pt, 1); // send homun_id
	else
		return mapif_homunculus_created(fd, RFIFOL(fd,2), homun_pt, 0); // fail


}




int inter_homunculus_parse_frommap(int fd){
	switch(RFIFOW(fd, 0)){
	case 0x3090: mapif_parse_CreateHomunculus(fd); break;
	case 0x3091: mapif_load_homunculus(fd); break;
	case 0x3092: mapif_save_homunculus(fd, RFIFOL(fd,6), (struct s_homunculus*) RFIFOP(fd, 10)); break;
	case 0x3093: mapif_delete_homunculus(fd); break;  // doesn't need to be parse, very simple packet...
	// rename homunculus is just like save... Map server check the rename flag before, send the save packet
	// case 0x3094: mapif_parse_rename_homunculus(fd); break;

	default:
		return 0;
	}
	return 1;
}
