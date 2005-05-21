// $Id: chat.c,v 1.2 2004/09/22 02:59:47 Akitasha Exp $
#include "base.h"
#include "db.h"
#include "nullpo.h"
#include "malloc.h"
#include "map.h"
#include "clif.h"
#include "pc.h"
#include "chat.h"
#include "npc.h"
#include "showmsg.h"
#include "utils.h"

int chat_triggerevent(struct chat_data &cd);


/*==========================================
 * �`���b�g���[���쐬
 *------------------------------------------
 */
int chat_createchat(struct map_session_data &sd,unsigned short limit,unsigned char pub,char* pass,char* title,size_t titlelen)
{
	struct chat_data *cd;

	cd = (struct chat_data*)aCalloc(1,sizeof(struct chat_data));

	cd->bl.id	= map_addobject(cd->bl);
	if(cd->bl.id==0)
	{
		clif_createchat(&sd,1);
		aFree(cd);
		return 0;
	}

	cd->bl.m	= sd.bl.m;
	cd->bl.x	= sd.bl.x;
	cd->bl.y	= sd.bl.y;
	cd->bl.type	= BL_CHAT;

	cd->owner	= &sd.bl;
	cd->usersd[0] = &sd;
	
	cd->limit	= limit;
	cd->pub		= pub;
	cd->users	= 1;

	memcpy(cd->pass,pass,sizeof(pass));
	pass[sizeof(pass)-1]=0;

	if( titlelen+1>=sizeof(cd->title)) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;


	pc_setchatid(sd,cd->bl.id);
	clif_createchat(&sd,0);
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * �����`���b�g���[���ɎQ��
 *------------------------------------------
 */
int chat_joinchat(struct map_session_data &sd,unsigned long chatid,const char* pass)
{
	struct chat_data *cd;

	cd=(struct chat_data*)map_id2bl(chatid);
	if(cd==NULL)
		return 1;

	if(cd->bl.m != sd.bl.m || cd->limit <= cd->users){
		clif_joinchatfail(&sd,0);
		return 0;
	}
	if(cd->pub==0 && (!pass || 0!=strcmp(pass,cd->pass)) )
	{
		clif_joinchatfail(&sd,1);
		return 0;
	}
	if(chatid == sd.chatID) //Double Chat fix by Alex14, thx CHaNGeTe 
	{
		clif_joinchatfail(&sd,1);
		return 0;
	}

	cd->usersd[cd->users] = &sd;
	cd->users++;

	pc_setchatid(sd,cd->bl.id);

	clif_joinchatok(&sd,cd);	// �V���ɎQ�������l�ɂ͑S���̃��X�g
	clif_addchat(cd,&sd);	// ���ɒ��ɋ����l�ɂ͒ǉ������l�̕�
	clif_dispchat(cd,0);	// ���͂̐l�ɂ͐l���ω���

	chat_triggerevent(*cd); // �C�x���g
	
	return 0;
}

/*==========================================
 * �`���b�g���[�����甲����
 *------------------------------------------
 */
int chat_leavechat(struct map_session_data &sd)
{
	struct chat_data *cd;
	size_t leavechar;

	cd=(struct chat_data*)map_id2bl(sd.chatID);
	if(cd==NULL)
		return 1;

	for(leavechar=0; leavechar<cd->users; leavechar++)
	{
		if(cd->usersd[leavechar] == &sd)
		{
			break;
		}
	}
	if(leavechar>=cd->users)	// ����chat�ɏ������Ă��Ȃ��炵�� (�o�O���̂�)
		return -1;

	if(leavechar==0 && cd->users>1 && cd->owner && cd->owner->type==BL_PC)
	{	// ���L�҂�����&���ɐl������&PC�̃`���b�g
		clif_changechatowner(cd,cd->usersd[1]);
		clif_clearchat(cd,0);
	}

	// ������PC�ɂ�����̂�users�����炷�O�Ɏ��s
	clif_leavechat(cd,&sd);

	cd->users--;
	pc_setchatid(sd,0);

	if(cd->users == 0 && cd->owner && cd->owner->type==BL_PC)
	{	// �S�����Ȃ��Ȃ���&PC�̃`���b�g�Ȃ̂ŏ���
		clif_clearchat(cd,0);
		map_delobject(cd->bl.id);	// free�܂ł��Ă����
	}
	else
	{
		size_t i;
		for(i=leavechar; i+1<cd->users; i++)
			cd->usersd[i] = cd->usersd[i+1];
		if(leavechar==0 && cd->owner && cd->owner->type==BL_PC)
		{	// PC�̃`���b�g�Ȃ̂ŏ��L�҂��������̂ňʒu�ύX
			cd->bl.x = cd->owner->x;
			cd->bl.y = cd->owner->y;
		}
		clif_dispchat(cd,0);
	}
	return 0;
}

/*==========================================
 * �`���b�g���[���̎����������
 *------------------------------------------
 */
int chat_changechatowner(struct map_session_data &sd, const char *nextownername)
{
	struct chat_data *cd;
	struct map_session_data *tmp_sd;
	size_t nextowner;

	cd=(struct chat_data*)map_id2bl(sd.chatID);
	if(cd==NULL || &sd.bl!=cd->owner)
		return 1;

	for(nextowner=1; nextowner<cd->users; nextowner++){
		if(strcmp(cd->usersd[nextowner]->status.name,nextownername)==0){
			break;
		}
	}
	if(nextowner>=cd->users) // ����Ȑl�͋��Ȃ�
		return -1;

	clif_changechatowner(cd,cd->usersd[nextowner]);
	// ��U����
	clif_clearchat(cd,0);

	// userlist�̏��ԕύX (0�����L�҂Ȃ̂�)
	if( (tmp_sd = cd->usersd[0]) == NULL )
		return 1; //���肦��̂��ȁH
	cd->usersd[0] = cd->usersd[nextowner];
	cd->usersd[nextowner] = tmp_sd;

	// �V�������L�҂̈ʒu�֕ύX
	cd->bl.x=cd->usersd[0]->bl.x;
	cd->bl.y=cd->usersd[0]->bl.y;

	// �ēx�\��
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * �`���b�g�̏��(�^�C�g����)��ύX
 *------------------------------------------
 */
int chat_changechatstatus(struct map_session_data &sd,unsigned short limit,unsigned char pub,const char* pass,const char* title, size_t titlelen)
{
	struct chat_data *cd;

	cd=(struct chat_data*)map_id2bl(sd.chatID);
	if(cd==NULL || &sd.bl!=cd->owner)
		return 1;

	cd->limit = limit;
	cd->pub = pub;
	memcpy(cd->pass,pass,8);
	if(titlelen+1>=sizeof(cd->title)) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	clif_changechatstatus(cd);
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * �`���b�g���[������R��o��
 *------------------------------------------
 */
int chat_kickchat(struct map_session_data &sd,const char *kickusername)
{
	struct chat_data *cd;
	size_t kickuser;

	cd=(struct chat_data*)map_id2bl(sd.chatID);
	if(cd==NULL || &sd.bl!=cd->owner)
		return 1;

	for(kickuser=1; kickuser<cd->users; kickuser++)
	{
		if( cd->usersd[kickuser] && 0==strcmp(cd->usersd[kickuser]->status.name,kickusername) )
			break;
	}
	if(kickuser>=cd->users) // ����Ȑl�͋��Ȃ�
		return -1;

	chat_leavechat(*cd->usersd[kickuser]);

	return 0;
}

/*==========================================
 * npc�`���b�g���[���쐬
 *------------------------------------------
 */
int chat_createnpcchat(struct npc_data &nd,unsigned short limit,unsigned char pub, int trigger,const char* title, unsigned short titlelen,const char *ev)
{
	struct chat_data *cd;

	cd = (struct chat_data *)aCalloc(1,sizeof(struct chat_data));

	cd->limit = cd->trigger = limit;
	if(trigger>0)
		cd->trigger = trigger;
	cd->pub = pub;
	cd->users = 0;
	memcpy(cd->pass,"",1);
	if((size_t)titlelen+1>=sizeof(cd->title)) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	cd->bl.m = nd.bl.m;
	cd->bl.x = nd.bl.x;
	cd->bl.y = nd.bl.y;
	cd->bl.type = BL_CHAT;
	cd->owner = &nd.bl;
	memcpy(cd->npc_event,ev,strlen(ev));

	cd->bl.id = map_addobject(cd->bl);	
	if(cd->bl.id==0){
		aFree(cd);
		return 0;
	}
	nd.chat_id=cd->bl.id;

	clif_dispchat(cd,0);

	return 0;
}
/*==========================================
 * npc�`���b�g���[���폜
 *------------------------------------------
 */
int chat_deletenpcchat(struct npc_data &nd)
{
	struct chat_data *cd;

	nullpo_retr(0, cd=(struct chat_data*)map_id2bl(nd.chat_id));
	
	chat_npckickall(*cd);
	clif_clearchat(cd,0);
	map_delobject(cd->bl.id);	// free�܂ł��Ă����
	nd.chat_id=0;
	
	return 0;
}

/*==========================================
 * �K��l���ȏ�ŃC�x���g����`����Ă�Ȃ���s
 *------------------------------------------
 */
int chat_triggerevent(struct chat_data &cd)
{
	if(cd.users>=cd.trigger && cd.npc_event[0])
		npc_event_do(cd.npc_event);
	return 0;
}

/*==========================================
 * �C�x���g�̗L����
 *------------------------------------------
 */
int chat_enableevent(struct chat_data &cd)
{
	cd.trigger&=0x7f;
	chat_triggerevent(cd);
	return 0;
}
/*==========================================
 * �C�x���g�̖�����
 *------------------------------------------
 */
int chat_disableevent(struct chat_data &cd)
{
	cd.trigger|=0x80;
	return 0;
}
/*==========================================
 * �`���b�g���[������S���R��o��
 *------------------------------------------
 */
int chat_npckickall(struct chat_data &cd)
{
	while(cd.users>0 && cd.usersd[cd.users-1])
		chat_leavechat( *cd.usersd[cd.users-1] );
	return 0;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
int do_final_chat(void)
{
	return 0;
}
