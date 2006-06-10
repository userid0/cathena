// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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
#include "skill.h"
#include "status.h"

#define PARTY_SEND_XY_INVERVAL	1000	// ���W��g�o���M�̊Ԋu

static struct dbt* party_db;
static struct party_data* party_cache = NULL; //party in cache for skipping consecutive lookups. [Skotlex]
int party_share_level = 10;
int party_send_xy_timer(int tid,unsigned int tick,int id,int data);
/*==========================================
 * �I��
 *------------------------------------------
 */
void do_final_party(void)
{
	party_db->destroy(party_db,NULL);
}
// ������
void do_init_party(void)
{
	party_db=db_alloc(__FILE__,__LINE__,DB_INT,DB_OPT_RELEASE_DATA,sizeof(int));
	add_timer_func_list(party_send_xy_timer,"party_send_xy_timer");
	add_timer_interval(gettick()+PARTY_SEND_XY_INVERVAL,party_send_xy_timer,0,0,PARTY_SEND_XY_INVERVAL);
}

// ����
struct party_data *party_search(int party_id)
{
	if(!party_id) return NULL;
	if (party_cache && party_cache->party.party_id == party_id)
		return party_cache;

	party_cache = idb_get(party_db,party_id);
	return party_cache;
}
int party_searchname_sub(DBKey key,void *data,va_list ap)
{
	struct party_data *p=(struct party_data *)data,**dst;
	char *str;
	str=va_arg(ap,char *);
	dst=va_arg(ap,struct party_data **);
	if(strncmpi(p->party.name,str,NAME_LENGTH)==0)
		*dst=p;
	return 0;
}

struct party_data* party_searchname(char *str)
{
	struct party_data *p=NULL;
	party_db->foreach(party_db,party_searchname_sub,str,&p);
	return p;
}

int party_create(struct map_session_data *sd,char *name,int item,int item2)
{
	nullpo_retr(0, sd);

	if(sd->status.party_id==0)
		intif_create_party(sd,name,item,item2);
	else
		clif_party_created(sd,2);
	return 0;
}


int party_created(int account_id,int char_id,int fail,int party_id,char *name)
{
	struct map_session_data *sd;
	struct party_data *p;
	sd=map_id2sd(account_id);

	nullpo_retr(0, sd);
	if (sd->status.char_id != char_id)
		return 0; //unlikely failure...
	
	if(fail){
		clif_party_created(sd,1);
		return 0;
	}
	sd->status.party_id=party_id;
	if(idb_get(party_db,party_id)!=NULL){
		ShowFatalError("party: id already exists!\n");
		exit(1);
	}
	p=(struct party_data *)aCalloc(1,sizeof(struct party_data));
	p->party.party_id=party_id;
	memcpy(p->party.name, name, NAME_LENGTH);
	idb_put(party_db,party_id,p);
	clif_party_created(sd,0); //Success message
	clif_charnameupdate(sd); //Update other people's display. [Skotlex]
	return 1;
}

int party_request_info(int party_id)
{
	return intif_request_partyinfo(party_id);
}

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
			for(j=0;j<MAX_PARTY;j++){
				if(p->member[j].account_id==sd->status.account_id && 
					p->member[j].char_id==sd->status.char_id)
				{
					f=0;
					break;
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

static void* create_party(DBKey key, va_list args) {
	struct party_data *p;
	p=(struct party_data *)aCalloc(1,sizeof(struct party_data));
	return p;
}

static void party_check_state(struct party_data *p)
{
	int i;
	memset(&p->state, 0, sizeof(p->state));
	for (i = 0; i < MAX_PARTY; i ++)
	{
		if (!p->data[i].sd) continue;
		if ((p->data[i].sd->class_&MAPID_UPPERMASK) == MAPID_MONK)
			p->state.monk = 1;
		else
		if ((p->data[i].sd->class_&MAPID_UPPERMASK) == MAPID_STAR_GLADIATOR)
			p->state.sg = 1;
		else
		if ((p->data[i].sd->class_&MAPID_UPPERMASK) == MAPID_SUPER_NOVICE)
			p->state.snovice = 1;
		else
		if ((p->data[i].sd->class_&MAPID_UPPERMASK) == MAPID_TAEKWON)
			p->state.tk = 1;
	}
	//TODO: Family state check.
}
int party_recv_info(struct party *sp)
{
	struct map_session_data *sd;
	struct party_data *p;
	int i;
	
	nullpo_retr(0, sp);

	p= idb_ensure(party_db, sp->party_id, create_party);
	if (!p->party.party_id) //party just received.
		party_check_member(sp);
	memcpy(&p->party,sp,sizeof(struct party));
	p->count = 0;
	memset(&p->state, 0, sizeof(p->state));
	memset(&p->data, 0, sizeof(p->data));
	for(i=0;i<MAX_PARTY;i++){
		if (!p->party.member[i].account_id)
			continue;
		sd = map_id2sd(p->party.member[i].account_id);
		if (sd && sd->status.party_id==p->party.party_id
			&& sd->status.char_id == p->party.member[i].char_id
			&& !sd->state.waitingdisconnect)
			p->data[i].sd = sd;
		p->count++;
	}
	party_check_state(p);
	for(i=0;i<MAX_PARTY;i++){
		sd = p->data[i].sd;
		if(!sd || sd->state.party_sent)
			continue;
		clif_party_main_info(p,-1);
		clif_party_option(p,sd,0x100);
		clif_party_info(p,-1);
		sd->state.party_sent=1;
	}
	
	return 0;
}

int party_invite(struct map_session_data *sd,int account_id)
{
	struct map_session_data *tsd= map_id2sd(account_id);
	struct party_data *p=party_search(sd->status.party_id);
	int i,flag=0;
	
	nullpo_retr(0, sd);

	if(tsd==NULL || p==NULL)
		return 0;
	if(!battle_config.invite_request_check) {
		if (tsd->guild_invite>0 || tsd->trade_partner) {
			clif_party_inviteack(sd,tsd->status.name,0);
			return 0;
		}
	}
	if( tsd->status.party_id>0 || tsd->party_invite>0 ){
		clif_party_inviteack(sd,tsd->status.name,0);
		return 0;
	}
	for(i=0;i<MAX_PARTY;i++){
		if(p->party.member[i].account_id == 0) //Room for a new member.
			flag = 1;
		if(p->party.member[i].account_id==account_id &&
			p->party.member[i].char_id==tsd->status.char_id){
			clif_party_inviteack(sd,tsd->status.name,0);
			return 0;
		}
	}
	if (!flag) { //Full party.
		clif_party_inviteack(sd,tsd->status.name,2);
		return 0;
	}
		
	tsd->party_invite=sd->status.party_id;
	tsd->party_invite_account=sd->status.account_id;

	clif_party_invite(sd,tsd);
	return 1;
}

int party_reply_invite(struct map_session_data *sd,int account_id,int flag)
{
	struct map_session_data *tsd= map_id2sd(account_id);

	nullpo_retr(0, sd);

	if(flag==1){
		intif_party_addmember( sd->party_invite, sd);
		return 0;
	}
	sd->party_invite=0;
	sd->party_invite_account=0;
	if(tsd==NULL)
		return 0;
	clif_party_inviteack(tsd,sd->status.name,1);
	return 1;
}

int party_member_added(int party_id,int account_id,int char_id, int flag)
{
	struct map_session_data *sd = map_id2sd(account_id),*sd2;
	struct party_data *p = party_search(party_id);
	if(sd == NULL || sd->status.char_id != char_id){
		if (flag == 0) {
			if(battle_config.error_log)
				ShowError("party: member added error %d is not online\n",account_id);
			intif_party_leave(party_id,account_id,char_id);
		}
		return 0;
	}
	sd->party_invite=0;
	sd->party_invite_account=0;

	if (!p) {
		if(battle_config.error_log)
			ShowError("party_member_added: party %d not found.\n",party_id);
		intif_party_leave(party_id,account_id,char_id);
		return 0;
	}

	sd2=map_id2sd(sd->party_invite_account);
	if(!flag) {
		sd->state.party_sent=0;
		sd->status.party_id=party_id;
		party_check_conflict(sd);
		clif_party_join_info(&p->party,sd);
		clif_charnameupdate(sd); //Update char name's display [Skotlex]
	}
	if (sd2)
		clif_party_inviteack(sd2,sd->status.name,flag?2:0);
	return 0;
}

int party_removemember(struct map_session_data *sd,int account_id,char *name)
{
	struct party_data *p;
	int i;

	nullpo_retr(0, sd);
	
	if( (p = party_search(sd->status.party_id)) == NULL )
		return 0;

	for(i=0;i<MAX_PARTY;i++){
		if(p->party.member[i].account_id==sd->status.account_id &&
			p->party.member[i].char_id==sd->status.char_id) {
			if(p->party.member[i].leader)
				break;
			return 0;
		}
	}
	if (i >= MAX_PARTY) //Request from someone not in party? o.O
		return 0;
	
	for(i=0;i<MAX_PARTY;i++){
		if(p->party.member[i].account_id==account_id &&
			strncmp(p->party.member[i].name,name,NAME_LENGTH)==0)
		{
			intif_party_leave(p->party.party_id,account_id,p->party.member[i].char_id);
			return 1;
		}
	}
	return 0;
}

int party_leave(struct map_session_data *sd)
{
	struct party_data *p;
	int i;

	nullpo_retr(0, sd);
	
	if( (p = party_search(sd->status.party_id)) == NULL )
		return 0;
	
	for(i=0;i<MAX_PARTY;i++){
		if(p->party.member[i].account_id==sd->status.account_id &&
			p->party.member[i].char_id==sd->status.char_id){
			intif_party_leave(p->party.party_id,sd->status.account_id,sd->status.char_id);
			return 0;
		}
	}
	return 0;
}

int party_member_leaved(int party_id,int account_id,int char_id)
{
	struct map_session_data *sd=map_id2sd(account_id);
	struct party_data *p=party_search(party_id);
	int i;
	if (sd && sd->status.char_id != char_id) //Wrong target
		sd = NULL;
	if(p!=NULL){
		for(i=0;i<MAX_PARTY;i++)
			if(p->party.member[i].account_id==account_id &&
				p->party.member[i].char_id==char_id){
				clif_party_leaved(p,sd,account_id,p->party.member[i].name,0x00);
				memset(&p->party.member[i], 0, sizeof(p->party.member[0]));
				memset(&p->data[i], 0, sizeof(p->data[0]));
				p->count--;
				break;
			}
	}
	if(sd!=NULL && sd->status.party_id==party_id){
		sd->status.party_id=0;
		sd->state.party_sent=0;
		clif_charnameupdate(sd); //Update name display [Skotlex]
		party_check_state(p);
	}
	return 0;
}

int party_broken(int party_id)
{
	struct party_data *p;
	int i;
	if( (p=party_search(party_id))==NULL )
		return 0;
	
	for(i=0;i<MAX_PARTY;i++){
		if(p->data[i].sd!=NULL){
			clif_party_leaved(p,p->data[i].sd,
				p->party.member[i].account_id,p->party.member[i].name,0x10);
			p->data[i].sd->status.party_id=0;
			p->data[i].sd->state.party_sent=0;
		}
	}
	if (party_cache && party_cache->party.party_id == party_id)
		party_cache = NULL;
	idb_remove(party_db,party_id);
	return 0;
}

int party_changeoption(struct map_session_data *sd,int exp,int flag)
{
	nullpo_retr(0, sd);

	if( sd->status.party_id==0)
		return 0;
	intif_party_changeoption(sd->status.party_id,sd->status.account_id,exp,flag);
	return 0;
}

int party_optionchanged(int party_id,int account_id,int exp,int item,int flag)
{
	struct party_data *p;
	struct map_session_data *sd=map_id2sd(account_id);
	if( (p=party_search(party_id))==NULL)
		return 0;

	if(!(flag&0x01)) p->party.exp=exp;
	if(!(flag&0x10)) p->party.item=item;
	clif_party_option(p,sd,flag);
	return 0;
}

int party_recv_movemap(int party_id,int account_id,int char_id, unsigned short map,int online,int lv)
{
	struct party_data *p;
	int i;
	if( (p=party_search(party_id))==NULL)
		return 0;
	for(i=0;i<MAX_PARTY;i++){
		struct map_session_data *sd;
		struct party_member *m=&p->party.member[i];
		if(m->account_id==account_id && m->char_id==char_id){
			m->map = map;
			m->online=online;
			m->lv=lv;
			//Check if they still exist on this map server
			sd = map_id2sd(m->account_id);
			p->data[i].sd = (sd!=NULL && sd->status.party_id==p->party.party_id && sd->status.char_id == m->char_id && !sd->state.waitingdisconnect)?sd:NULL;
			break;
		}
	}
	if(i==MAX_PARTY){
		if(battle_config.error_log)
			ShowError("party: not found member %d/%d on %d[%s]",account_id,char_id,party_id,p->party.name);
		return 0;
	}
	
	clif_party_info(p,-1);
	return 0;
}

int party_send_movemap(struct map_session_data *sd)
{
	int i;
	struct party_data *p;

	nullpo_retr(0, sd);

	if( sd->status.party_id==0 )
		return 0;
	intif_party_changemap(sd,1);

	p=party_search(sd->status.party_id);
	if (p && sd->fd) {
		//Send dots of other party members to this char. [Skotlex]
		for(i=0; i < MAX_PARTY; i++) {
			if (!p->data[i].sd	|| p->data[i].sd == sd ||
				p->data[i].sd->bl.m != sd->bl.m)
				continue;
			clif_party_xy_single(sd->fd, p->data[i].sd);
		}
		
	}
	
	if( sd->state.party_sent )
		return 0;

	party_check_conflict(sd);
	
	if(p){
		party_check_member(&p->party);
		if(sd->status.party_id==p->party.party_id){
			clif_party_main_info(p,sd->fd);
			clif_party_option(p,sd,0x100);
			clif_party_info(p,sd->fd);
			sd->state.party_sent=1;
		}
	}
	
	return 0;
}

int party_send_logout(struct map_session_data *sd)
{
	struct party_data *p;
	int i;

	if(!sd->status.party_id)
		return 0;
	
	intif_party_changemap(sd,0);
	p=party_search(sd->status.party_id);
	if(!p) return 0;

	for(i=0;i<MAX_PARTY && p->data[i].sd != sd;i++);
	if (i < MAX_PARTY)
		memset(&p->data[i], 0, sizeof(p->data[0]));
	
	return 1;
}

int party_send_message(struct map_session_data *sd,char *mes,int len)
{
	if(sd->status.party_id==0)
		return 0;
	intif_party_message(sd->status.party_id,sd->status.account_id,mes,len);
	party_recv_message(sd->status.party_id,sd->status.account_id,mes,len);
	//Chat Logging support Type 'P'
	if(log_config.chat&1 //we log everything then
		|| (log_config.chat&4 //if Party bit is on
		&& ( !agit_flag || !(log_config.chat&16) ))) //if WOE ONLY flag is off or AGIT is OFF
		log_chat("P", sd->status.party_id, sd->status.char_id, sd->status.account_id, (char*)mapindex_id2name(sd->mapindex), sd->bl.x, sd->bl.y, NULL, mes);

	return 0;
}

int party_recv_message(int party_id,int account_id,char *mes,int len)
{
	struct party_data *p;
	if( (p=party_search(party_id))==NULL)
		return 0;
	clif_party_message(p,account_id,mes,len);
	return 0;
}

int party_check_conflict(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	intif_party_checkconflict(sd->status.party_id,sd->status.account_id,sd->status.char_id);
	return 0;
}

int party_skill_check(struct map_session_data *sd, int party_id, int skillid, int skilllv)
{
	struct party_data *p;
	struct map_session_data *p_sd;
	int i;

	if(!party_id || (p=party_search(party_id))==NULL)
		return 0;
	switch(skillid) {
		case TK_COUNTER: //Increase Triple Attack rate of Monks.
			if (!p->state.monk) return 0;
			break;
		case MO_TRIPLEATTACK: //Increase Counter rate of Star Gladiators
			if (!p->state.sg) return 0;
			break;
		case AM_TWILIGHT2: //Twilight Pharmacy, requires Super Novice
			return p->state.snovice;
		case AM_TWILIGHT3: //Twilight Pharmacy, Requires Taekwon
			return p->state.tk;
		default:
			return 0; //Unknown case?
	}
	
	for(i=0;i<MAX_PARTY;i++){
		if ((p_sd = p->data[i].sd) == NULL)
			continue;
		switch(skillid) {
			case TK_COUNTER: //Increase Triple Attack rate of Monks.
				if((p_sd->class_&MAPID_UPPERMASK) == MAPID_MONK
					&& sd->bl.m == p_sd->bl.m
					&& pc_checkskill(p_sd,MO_TRIPLEATTACK)) {
					int rate = 50 +50*skilllv; //+100/150/200% success rate
					sc_start4(&p_sd->bl,SC_SKILLRATE_UP,100,MO_TRIPLEATTACK,rate,0,0,skill_get_time(SG_FRIEND, 1));
				}
				break;
			case MO_TRIPLEATTACK: //Increase Counter rate of Star Gladiators
				if((p_sd->class_&MAPID_UPPERMASK) == MAPID_STAR_GLADIATOR
					&& sd->bl.m == p_sd->bl.m
					&& pc_checkskill(p_sd,TK_COUNTER)) {
					int rate = 50 +50*pc_checkskill(p_sd,TK_COUNTER); //+100/150/200% success rate
					sc_start4(&p_sd->bl,SC_SKILLRATE_UP,100,TK_COUNTER,rate,0,0,skill_get_time(SG_FRIEND, 1));
				}
				break;
		}
	}
	return 0;
}

int party_send_xy_timer_sub(DBKey key,void *data,va_list ap)
{
	struct party_data *p=(struct party_data *)data;
	struct map_session_data *sd;
	int i;

	nullpo_retr(0, p);

	for(i=0;i<MAX_PARTY;i++){
		if(!p->data[i].sd) continue;
		sd = p->data[i].sd;
		if (p->data[i].x != sd->bl.x || p->data[i].y != sd->bl.y)
		{
			clif_party_xy(sd);
			p->data[i].x = sd->bl.x;
			p->data[i].y = sd->bl.y;
		}
		if (p->data[i].hp != sd->battle_status.hp)
		{
			clif_party_hp(sd);
			p->data[i].hp = sd->battle_status.hp;
		}
	}
	return 0;
}

int party_send_xy_timer(int tid,unsigned int tick,int id,int data)
{
	party_db->foreach(party_db,party_send_xy_timer_sub,tick);
	return 0;
}

int party_send_xy_clear(struct party_data *p)
{
	int i;

	nullpo_retr(0, p);

	for(i=0;i<MAX_PARTY;i++){
		if(!p->data[i].sd) continue;
		p->data[i].hp = 0;
		p->data[i].x = 0;
		p->data[i].y = 0;
	}
	return 0;
}

// exp share and added zeny share [Valaris]
int party_exp_share(struct party_data *p,struct block_list *src,unsigned int base_exp,unsigned int job_exp,int zeny)
{
	struct map_session_data* sd[MAX_PARTY];
	int i;
	short c, bonus =100; // modified [Valaris]

	nullpo_retr(0, p);

	for (i = c = 0; i < MAX_PARTY; i++)
		if ((sd[c] = p->data[i].sd)!=NULL && sd[c]->bl.m == src->m && !pc_isdead(sd[c])) {
			if (battle_config.idle_no_share && (sd[c]->chatID || sd[c]->vender_id || (sd[c]->idletime < (last_tick - battle_config.idle_no_share))))
				continue;
			c++;
		}
	if (c < 1)
		return 0;
	if (battle_config.party_even_share_bonus) //Valaris's even share exp bonus equation.
		bonus += (battle_config.party_even_share_bonus*c*(c-1)/10);	//Changed Valaris's bonus switch to an equation [Skotlex]
	else	//Official kRO/iRO sites state that the even share bonus is 10% per additional party member.
		bonus += (c-1)*10;

	base_exp/=c;	
	job_exp/=c;
	if (base_exp/100 > UINT_MAX/bonus)
		base_exp= UINT_MAX; //Exp overflow
	else
		base_exp = base_exp*bonus/100;

	if (job_exp/100 > UINT_MAX/bonus)
		job_exp = UINT_MAX;
	else
		job_exp = job_exp*bonus/100;

	for (i = 0; i < c; i++)
	{
		pc_gainexp(sd[i], src, base_exp, job_exp);
		if (battle_config.zeny_from_mobs) // zeny from mobs [Valaris]
			pc_getzeny(sd[i],bonus*zeny/(c*100));
	}
	return 0;
}

int party_share_loot(struct party_data *p, TBL_PC *sd, struct item *item_data)
{
	TBL_PC *target=NULL;
	int i;
	if (p && p->party.item&2) {
		//item distribution to party members.
		if (battle_config.party_share_type) { //Round Robin
			TBL_PC *psd;
			i = p->itemc;
			do {
				i++;
				if (i >= MAX_PARTY)
					i = 0;	// reset counter to 1st person in party so it'll stop when it reaches "itemc"
				if ((psd=p->data[i].sd)==NULL || sd->bl.m != psd->bl.m ||
					pc_isdead(psd) || (battle_config.idle_no_share && (
					psd->chatID || psd->vender_id || (psd->idletime < (last_tick - battle_config.idle_no_share)))
				))
					continue;
				
				if (pc_additem(psd,item_data,item_data->amount))
					continue; //Chosen char can't pick up loot.
				//Successful pick.
				p->itemc = i;
				target = psd;
				break;
			} while (i != p->itemc);
		} else { //Random pick
			TBL_PC *psd[MAX_PARTY];
			int count=0;
			//Collect pick candidates
			for (i = 0; i < MAX_PARTY; i++) {
				if ((psd[count]=p->data[i].sd) && psd[count]->bl.m == sd->bl.m &&
					!pc_isdead(psd[count]) && (!battle_config.idle_no_share || (
						!psd[count]->chatID && !psd[count]->vender_id &&
					  	(psd[count]->idletime >= (last_tick - battle_config.idle_no_share)))
				))
					count++;
			}
			while (count > 0) { //Pick a random member.
				i = rand()%count;
				if (pc_additem(psd[i],item_data,item_data->amount))
				{	//Discard this receiver.
					psd[i] = psd[count-1];
					count--;
				} else { //Successful pick.
					target = psd[i];
					break;
				}
			}
		}
	}
	if (!target) { //Give it to the owner.
		target = sd;
		if ((i=pc_additem(sd,item_data,item_data->amount)))
			return i;
	}

	if(log_config.pick) //Logs items, taken by (P)layers [Lupus]
		log_pick(target, "P", 0, item_data->nameid, item_data->amount, item_data);
	//Logs
	if(battle_config.party_show_share_picker && target != sd){
		char output[80];
		sprintf(output, "%s acquired the item.",target->status.name);
		clif_disp_onlyself(sd,output,strlen(output));
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
	if(!(((TBL_PC *)bl)->state.autotrade) && (((TBL_PC *)bl)->idletime < (last_tick - battle_config.idle_no_share)))
		return 1;
	else
		return 0;
}

int party_foreachsamemap(int (*func)(struct block_list*,va_list),struct map_session_data *sd,int range,...)
{
	struct party_data *p;
	va_list ap;
	int i;
	int x0,y0,x1,y1;
	struct block_list *list[MAX_PARTY];
	int blockcount=0;
	int total = 0; //Return value.
	
	nullpo_retr(0,sd);
	
	if((p=party_search(sd->status.party_id))==NULL)
		return 0;

	x0=sd->bl.x-range;
	y0=sd->bl.y-range;
	x1=sd->bl.x+range;
	y1=sd->bl.y+range;

	va_start(ap,range);
	
	for(i=0;i<MAX_PARTY;i++){
		struct map_session_data *psd=p->data[i].sd;
		if(!psd) continue;
		if(psd->bl.m!=sd->bl.m || !psd->bl.prev)
			continue;
		if(range &&
			(psd->bl.x<x0 || psd->bl.y<y0 ||
			 psd->bl.x>x1 || psd->bl.y>y1 ) )
			continue;
		list[blockcount++]=&sd->bl; 
	}

	map_freeblock_lock();
	
	for(i=0;i<blockcount;i++)
		total += func(list[i],ap);

	map_freeblock_unlock();

	va_end(ap);
	return total;
}
