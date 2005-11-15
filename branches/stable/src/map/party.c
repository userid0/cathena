// $Id: party.c,v 1.2 2004/09/22 02:59:47 Akitasha Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/db.h"
#include "../common/timer.h"
#include "../common/socket.h"
#include "../common/nullpo.h"
#include "../common/malloc.h"
#include "../common/showmsg.h"

#include "party.h"
#include "pc.h"
#include "map.h"
#include "battle.h"
#include "intif.h"
#include "clif.h"
#include "log.h"

#define PARTY_SEND_XY_INVERVAL	1000	// ���W��g�o���M�̊Ԋu

static struct dbt* party_db;
int party_share_level = 10;
int party_send_xy_timer(int tid,unsigned int tick,int id,int data);
/*==========================================
 * �I��
 *------------------------------------------
 */
static int party_db_final(void *key,void *data,va_list ap)
{
	aFree(data);
	return 0;
}
void do_final_party(void)
{
	if(party_db)
		numdb_final(party_db,party_db_final);
}
// ������
void do_init_party(void)
{
	party_db=numdb_init();
	add_timer_func_list(party_send_xy_timer,"party_send_xy_timer");
	add_timer_interval(gettick()+PARTY_SEND_XY_INVERVAL,party_send_xy_timer,0,0,PARTY_SEND_XY_INVERVAL);
}

// ����
struct party *party_search(int party_id)
{
	return (struct party *) numdb_search(party_db,party_id);
}
int party_searchname_sub(void *key,void *data,va_list ap)
{
	struct party *p=(struct party *)data,**dst;
	char *str;
	str=va_arg(ap,char *);
	dst=va_arg(ap,struct party **);
	if(strcmpi(p->name,str)==0)
		*dst=p;
	return 0;
}
// �p�[�e�B������
struct party* party_searchname(char *str)
{
	struct party *p=NULL;
	numdb_foreach(party_db,party_searchname_sub,str,&p);
	return p;
}
// �쐬�v��
int party_create(struct map_session_data *sd,char *name,int item,int item2)
{
	nullpo_retr(0, sd);

	if(sd->status.party_id==0)
		intif_create_party(sd,name,item,item2);
	else
		clif_party_created(sd,2);
	return 0;
}

// �쐬��
int party_created(int account_id,int fail,int party_id,char *name)
{
	struct map_session_data *sd;
	sd=map_id2sd(account_id);

	nullpo_retr(0, sd);
	
	if(fail==0){
		struct party *p;
		sd->status.party_id=party_id;
		if((p=(struct party *) numdb_search(party_db,party_id))!=NULL){
			ShowFatalError("party: id already exists!\n");
			exit(1);
		}
		p=(struct party *)aCalloc(1,sizeof(struct party));
		p->party_id=party_id;
		memcpy(p->name, name, NAME_LENGTH-1);
		numdb_insert(party_db,party_id,p);
		clif_party_created(sd,0);
		clif_charnameupdate(sd); //Update other people's display. [Skotlex]
	}else{
		clif_party_created(sd,1);
	}
	return 0;
}

// ���v��
int party_request_info(int party_id)
{
	return intif_request_partyinfo(party_id);
}

// �����L�����̊m�F
int party_check_member(struct party *p)
{
	int i, users;
	struct map_session_data *sd, **all_sd;

	nullpo_retr(0, p);

	all_sd = map_getallusers(&users);
	
	for(i=0;i<users;i++)
	{
		if((sd = all_sd[i]) && sd->status.party_id==p->party_id)
		{
			int j,f=1;
			for(j=0;j<MAX_PARTY;j++){	// �p�[�e�B�Ƀf�[�^�����邩�m�F
				if(	p->member[j].account_id==sd->status.account_id){
					if(	strcmp(p->member[j].name,sd->status.name)==0 )
						f=0;	// �f�[�^������
					else
						p->member[j].sd=NULL;	// ���C�ʃL����������
				}
			}
		if(f){
			sd->status.party_id=0;
			if(battle_config.error_log)
				ShowWarning("party: check_member %d[%s] is not member\n",sd->status.account_id,sd->status.name);
			}
		}
	}
	return 0;
}

// ��񏊓����s�i����ID�̃L������S���������ɂ���j
int party_recv_noinfo(int party_id)
{
	int i, users;
	struct map_session_data *sd, **all_sd;

	all_sd = map_getallusers(&users);
	
	for(i=0;i<users;i++){
		if((sd = all_sd[i]) && sd->status.party_id==party_id)
			sd->status.party_id=0;
	}
	return 0;
}
// ��񏊓�
int party_recv_info(struct party *sp)
{
	struct party *p;
	int i;
	
	nullpo_retr(0, sp);

	if((p=(struct party *) numdb_search(party_db,sp->party_id))==NULL){
		p=(struct party *)aCalloc(1,sizeof(struct party));
		numdb_insert(party_db,sp->party_id,p);
		
		// �ŏ��̃��[�h�Ȃ̂Ń��[�U�[�̃`�F�b�N���s��
		party_check_member(sp);
	}
	memcpy(p,sp,sizeof(struct party));
	
	for(i=0;i<MAX_PARTY;i++){	// sd�̐ݒ�
		struct map_session_data *sd = map_id2sd(p->member[i].account_id);
		p->member[i].sd=(sd!=NULL && sd->status.party_id==p->party_id && !sd->state.waitingdisconnect)?sd:NULL;
	}

	clif_party_info(p,-1);

	for(i=0;i<MAX_PARTY;i++){	// �ݒ���̑��M
		struct map_session_data *sd = p->member[i].sd;
		if(sd!=NULL && sd->state.party_sent==0){
			clif_party_option(p,sd,0x100);
			sd->state.party_sent=1;
		}
	}
	
	return 0;
}

// �p�[�e�B�ւ̊��U
int party_invite(struct map_session_data *sd,int account_id)
{
	struct map_session_data *tsd= map_id2sd(account_id);
	struct party *p=party_search(sd->status.party_id);
	int i;
	
	nullpo_retr(0, sd);

	if(tsd==NULL || p==NULL)
		return 0;
	if(!battle_config.invite_request_check) {
		if (tsd->guild_invite>0 || tsd->trade_partner) {	// ���肪��������ǂ���
			clif_party_inviteack(sd,tsd->status.name,0);
			return 0;
		}
	}
	if( tsd->status.party_id>0 || tsd->party_invite>0 ){	// ����̏����m�F
		clif_party_inviteack(sd,tsd->status.name,0);
		return 0;
	}
	for(i=0;i<MAX_PARTY;i++){	// ���A�J�E���g�m�F
		if(p->member[i].account_id==account_id){
			clif_party_inviteack(sd,tsd->status.name,0);
			return 0;
		}
	}
	
	tsd->party_invite=sd->status.party_id;
	tsd->party_invite_account=sd->status.account_id;

	clif_party_invite(sd,tsd);
	return 0;
}
// �p�[�e�B���U�ւ̕ԓ�
int party_reply_invite(struct map_session_data *sd,int account_id,int flag)
{
	struct map_session_data *tsd= map_id2sd(account_id);

	nullpo_retr(0, sd);

	if(flag==1){	// ����
		//inter�I�֒ǉ��v��
		intif_party_addmember( sd->party_invite, sd->status.account_id );
		return 0;
	}
	else {		// ����
		sd->party_invite=0;
		sd->party_invite_account=0;
		if(tsd==NULL)
			return 0;
		clif_party_inviteack(tsd,sd->status.name,1);
	}
	return 0;
}
// �p�[�e�B���ǉ����ꂽ
int party_member_added(int party_id,int account_id,int flag)
{
	struct map_session_data *sd = map_id2sd(account_id),*sd2;
	if(sd == NULL){
		if (flag == 0) {
			if(battle_config.error_log)
				ShowError("party: member added error %d is not online\n",account_id);
			intif_party_leave(party_id,account_id); // �L�������ɓo�^�ł��Ȃ��������ߒE�ޗv�����o��
		}
		return 0;
	}
	sd2=map_id2sd(sd->party_invite_account);
	sd->party_invite=0;
	sd->party_invite_account=0;
	
	if(flag==1){	// ���s
		if( sd2!=NULL )
			clif_party_inviteack(sd2,sd->status.name,0);
		return 0;
	}
	
		// ����
	sd->state.party_sent=0;
	sd->status.party_id=party_id;
	
	if( sd2!=NULL)
		clif_party_inviteack(sd2,sd->status.name,2);

	// �������������m�F
	party_check_conflict(sd);
	clif_charnameupdate(sd); //Update char name's display [Skotlex]
	clif_party_hp(sd);
	clif_party_xy(sd);
	return 0;
}
// �p�[�e�B�����v��
int party_removemember(struct map_session_data *sd,int account_id,char *name)
{
	struct party *p;
	int i;

	nullpo_retr(0, sd);
	
	if( (p = party_search(sd->status.party_id)) == NULL )
		return 0;

	for(i=0;i<MAX_PARTY;i++){	// ���[�_�[���ǂ����`�F�b�N
		if(p->member[i].account_id==sd->status.account_id)
			if(p->member[i].leader==0)
				return 0;
	}

	for(i=0;i<MAX_PARTY;i++){	// �������Ă��邩���ׂ�
		if(p->member[i].account_id==account_id){
			intif_party_leave(p->party_id,account_id);
			return 0;
		}
	}
	return 0;
}

// �p�[�e�B�E�ޗv��
int party_leave(struct map_session_data *sd)
{
	struct party *p;
	int i;

	nullpo_retr(0, sd);
	
	if( (p = party_search(sd->status.party_id)) == NULL )
		return 0;
	
	for(i=0;i<MAX_PARTY;i++){	// �������Ă��邩
		if(p->member[i].account_id==sd->status.account_id){
			intif_party_leave(p->party_id,sd->status.account_id);
			return 0;
		}
	}
	return 0;
}
// �p�[�e�B�����o���E�ނ���
int party_member_leaved(int party_id,int account_id,char *name)
{
	struct map_session_data *sd=map_id2sd(account_id);
	struct party *p=party_search(party_id);
	if(p!=NULL){
		int i;
		for(i=0;i<MAX_PARTY;i++)
			if(p->member[i].account_id==account_id){
				clif_party_leaved(p,sd,account_id,name,0x00);
				p->member[i].account_id=0;
				p->member[i].sd=NULL;
			}
	}
	if(sd!=NULL && sd->status.party_id==party_id){
		sd->status.party_id=0;
		sd->state.party_sent=0;
		clif_charnameupdate(sd); //Update name display [Skotlex]
	}
	return 0;
}
// �p�[�e�B���U�ʒm
int party_broken(int party_id)
{
	struct party *p;
	int i;
	if( (p=party_search(party_id))==NULL )
		return 0;
	
	for(i=0;i<MAX_PARTY;i++){
		if(p->member[i].sd!=NULL){
			clif_party_leaved(p,p->member[i].sd,
				p->member[i].account_id,p->member[i].name,0x10);
			p->member[i].sd->status.party_id=0;
			p->member[i].sd->state.party_sent=0;
		}
	}
	numdb_erase(party_db,party_id);
	return 0;
}
// �p�[�e�B�̐ݒ�ύX�v��
int party_changeoption(struct map_session_data *sd,int exp,int item)
{
	struct party *p;

	nullpo_retr(0, sd);

	if( sd->status.party_id==0 || (p=party_search(sd->status.party_id))==NULL )
		return 0;
	intif_party_changeoption(sd->status.party_id,sd->status.account_id,exp,item);
	return 0;
}
// �p�[�e�B�̐ݒ�ύX�ʒm
int party_optionchanged(int party_id,int account_id,int exp,int item,int flag)
{
	struct party *p;
	struct map_session_data *sd=map_id2sd(account_id);
	if( (p=party_search(party_id))==NULL)
		return 0;

	if(!(flag&0x01)) p->exp=exp;
	if(!(flag&0x10)) p->item=item;
	clif_party_option(p,sd,flag);
	return 0;
}

// �p�[�e�B�����o�̈ړ��ʒm
int party_recv_movemap(int party_id,int account_id,char *map,int online,int lv)
{
	struct party *p;
	int i;
	if( (p=party_search(party_id))==NULL)
		return 0;
	for(i=0;i<MAX_PARTY;i++){
		struct party_member *m=&p->member[i];
		if( m == NULL ){
			ShowError("party_recv_movemap nullpo?\n");
			return 0;
		}
		if(m->account_id==account_id){
			memcpy(m->map,map,MAP_NAME_LENGTH-1);
			m->online=online;
			m->lv=lv;
			break;
		}
	}
	if(i==MAX_PARTY){
		if(battle_config.error_log)
			ShowError("party: not found member %d on %d[%s]",account_id,party_id,p->name);
		return 0;
	}
	
	for(i=0;i<MAX_PARTY;i++){	// sd�Đݒ�
		struct map_session_data *sd= map_id2sd(p->member[i].account_id);
		p->member[i].sd=(sd!=NULL && sd->status.party_id==p->party_id && !sd->state.waitingdisconnect)?sd:NULL;
	}

	party_send_xy_clear(p);	// ���W�Ēʒm�v��
	
	clif_party_info(p,-1);
	return 0;
}

// �p�[�e�B�����o�̈ړ�
int party_send_movemap(struct map_session_data *sd)
{
	struct party *p;

	nullpo_retr(0, sd);

	if( sd->status.party_id==0 )
		return 0;
	intif_party_changemap(sd,1);

	if( sd->state.party_sent!=0 )	// �����p�[�e�B�f�[�^�͑��M�ς�
		return 0;

	// �����m�F	
	party_check_conflict(sd);
	
	// ����Ȃ�p�[�e�B��񑗐M
	if( (p=party_search(sd->status.party_id))!=NULL ){
		party_check_member(p);	// �������m�F����
		if(sd->status.party_id==p->party_id){
			clif_party_info(p,sd->fd);
			clif_party_option(p,sd,0x100);
			sd->state.party_sent=1;
		}
	}
	
	return 0;
}
// �p�[�e�B�����o�̃��O�A�E�g
int party_send_logout(struct map_session_data *sd)
{
	struct party *p;

	nullpo_retr(0, sd);

	if( sd->status.party_id>0 )
		intif_party_changemap(sd,0);
	
	// sd�������ɂȂ�̂Ńp�[�e�B��񂩂�폜
	if( (p=party_search(sd->status.party_id))!=NULL ){
		int i;
		for(i=0;i<MAX_PARTY;i++)
			if(p->member[i].sd==sd)
				p->member[i].sd=NULL;
	}
	
	return 0;
}
// �p�[�e�B���b�Z�[�W���M
int party_send_message(struct map_session_data *sd,char *mes,int len)
{
	if(sd->status.party_id==0)
		return 0;
	intif_party_message(sd->status.party_id,sd->status.account_id,mes,len);
        party_recv_message(sd->status.party_id,sd->status.account_id,mes,len);
	//Chat Logging support Type 'P'
	if(log_config.chat&1 //we log everything then
		|| ( log_config.chat&4 //if Party bit is on
		&& ( !agit_flag || !(log_config.chat&16) ))) //if WOE ONLY flag is off or AGIT is OFF
		log_chat("P", sd->status.party_id, sd->status.char_id, sd->status.account_id, (char*)sd->mapname, sd->bl.x, sd->bl.y, NULL, mes);

	return 0;
}

// �p�[�e�B���b�Z�[�W��M
int party_recv_message(int party_id,int account_id,char *mes,int len)
{
	struct party *p;
	if( (p=party_search(party_id))==NULL)
		return 0;
	clif_party_message(p,account_id,mes,len);
	return 0;
}
// �p�[�e�B�����m�F
int party_check_conflict(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	intif_party_checkconflict(sd->status.party_id,sd->status.account_id,sd->status.name);
	return 0;
}


// �ʒu��g�o�ʒm�p
int party_send_xy_timer_sub(void *key,void *data,va_list ap)
{
	struct party *p=(struct party *)data;
	int i;

	nullpo_retr(0, p);

	for(i=0;i<MAX_PARTY;i++){
		struct map_session_data *sd;
		if((sd=p->member[i].sd)!=NULL){
			// ���W�ʒm
			if(sd->party_x!=sd->bl.x || sd->party_y!=sd->bl.y){
				clif_party_xy(sd);
				sd->party_x=sd->bl.x;
				sd->party_y=sd->bl.y;
			}
		}
	}
	return 0;
}
// �ʒu��g�o�ʒm
int party_send_xy_timer(int tid,unsigned int tick,int id,int data)
{
	numdb_foreach(party_db,party_send_xy_timer_sub,tick);
	return 0;
}

// �ʒu�ʒm�N���A
int party_send_xy_clear(struct party *p)
{
	int i;

	nullpo_retr(0, p);

	for(i=0;i<MAX_PARTY;i++){
		struct map_session_data *sd;
		if((sd=p->member[i].sd)!=NULL){
			sd->party_x=-1;
			sd->party_y=-1;
		}
	}
	return 0;
}

//Does a manual check for even share on this map server, and if the range is not satisfied,
//a request to change it sent to the char server. [Skotlex]
void party_exp_share_check(struct map_session_data *sd, struct party *p) 
{
	struct map_session_data *p_sd;
	int i,lv;
	int maxlv=0,minlv=0x7fffffff;

	if (p == NULL || p->exp == 0)
		return;
	for(i=0;i<MAX_PARTY;i++){
		if ((p_sd = p->member[i].sd) == NULL)
			continue;
		lv=p_sd->status.base_level;
		if( lv < minlv ) minlv=lv;
		else if( maxlv < lv ) maxlv=lv;
	}
	if (maxlv - minlv > party_share_level)
		party_changeoption(sd,0,p->item); //Manual resetting of exp share. [Skotlex]
}

// exp share and added zeny share [Valaris]
int party_exp_share(struct party *p,int map,int base_exp,int job_exp,int zeny)
{
	struct map_session_data* sd[MAX_PARTY];
	int i;
	short c, bonus =100; // modified [Valaris]
	unsigned long base, job;

	nullpo_retr(0, p);

	for (i = c = 0; i < MAX_PARTY; i++)
		if ((sd[c] = p->member[i].sd)!=NULL && sd[c]->bl.m == map && !pc_isdead(sd[c])) {
			if (battle_config.idle_no_share && (pc_issit(sd[c]) || sd[c]->chatID || (sd[c]->idletime < (last_tick - battle_config.idle_no_share))))
				continue;
			c++;
		}
	if (c < 1)
		return 0;
	bonus += (c-1)*10; //Official kRO/iRO sites state that the even share bonus is 10% per additional party member.
	// This other bonus seems to be a custom one brought up by Valaris
	// 1 + 0.05*c*(c-1)/2 [Shinomori]
	//bonus += (5*c*(c-1)/2);	//Changed Valaris's bonus switch to an equation [Skotlex]
	//Bonus at Full party (12): +3.3 (430% exp/12 ~= 35% of total Mob's exp)
	base = (unsigned long)(base_exp/c)*bonus/100;
	job = (unsigned long)(job_exp/c)*bonus/100;
	if (base > 0x7fffffff)
		base = 0x7fffffff;
	if (job > 0x7fffffff)
		job = 0x7fffffff;
	for (i = 0; i < c; i++)
	{
		pc_gainexp(sd[i], base, job);
		if (battle_config.zeny_from_mobs) // zeny from mobs [Valaris]
			pc_getzeny(sd[i],bonus*zeny/(c*100));
	}
	return 0;
}

int party_send_dot_remove(struct map_session_data *sd)
{
	if (sd->status.party_id)
		clif_party_xy_remove(sd);
	return 0;
}

// To use for Taekwon's "Fighting Chant"
// int c = 0;
// party_foreachsamemap(party_sub_count, sd, 0, &c);
int party_sub_count(struct block_list *bl, va_list ap)
{
	int *c = va_arg(ap, int*);

	(*c)++;
	return 1;
}

// �����}�b�v�̃p�[�e�B�����o�[�S�̂ɏ�����������
// type==0 �����}�b�v
//     !=0 ��ʓ�
void party_foreachsamemap(int (*func)(struct block_list*,va_list),struct map_session_data *sd,int type,...)
{
	struct party *p;
	va_list ap;
	int i;
	int x0,y0,x1,y1;
	struct block_list *list[MAX_PARTY];
	int blockcount=0;
	
	nullpo_retv(sd);
	
	if((p=party_search(sd->status.party_id))==NULL)
		return;

	x0=sd->bl.x-AREA_SIZE;
	y0=sd->bl.y-AREA_SIZE;
	x1=sd->bl.x+AREA_SIZE;
	y1=sd->bl.y+AREA_SIZE;

	va_start(ap,type);
	
	for(i=0;i<MAX_PARTY;i++){
		struct party_member *m=&p->member[i];
		if(m->sd!=NULL){
			if(sd->bl.m!=m->sd->bl.m)
				continue;
			if(type!=0 &&
				(m->sd->bl.x<x0 || m->sd->bl.y<y0 ||
				 m->sd->bl.x>x1 || m->sd->bl.y>y1 ) )
				continue;
			list[blockcount++]=&m->sd->bl; 
		}
	}

	map_freeblock_lock();	// ����������̉�����֎~����
	
	for(i=0;i<blockcount;i++)
		if(list[i]->prev)	// �L�����ǂ����`�F�b�N
			func(list[i],ap);

	map_freeblock_unlock();	// �����������

	va_end(ap);
}
