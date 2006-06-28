// $Id: chat.c,v 1.2 2004/09/22 02:59:47 Akitasha Exp $
#include "nullpo.h"
#include "showmsg.h"
#include "utils.h"

#include "chat.h"
#include "battle.h"
#include "map.h"
#include "clif.h"
#include "pc.h"
#include "npc.h"

int chat_triggerevent(chat_data &cd);

/*==========================================
 * �`���b�g���[���쐬
 *------------------------------------------
 */
int chat_createchat(struct map_session_data &sd,unsigned short limit,unsigned char pub,char* pass,char* title,size_t titlelen)
{
	if( !sd.chatID )
	{
		chat_data *cd = new chat_data;

		cd->block_list::id	= map_addobject(*cd);
		if(cd->block_list::id==0)
		{
			clif_createchat(sd,1);
			delete cd;
			return 0;
		}

		cd->block_list::m		= sd.block_list::m;
		cd->block_list::x		= sd.block_list::x;
		cd->block_list::y		= sd.block_list::y;
		cd->block_list::type	= BL_CHAT;

		cd->owner	= &sd;
		cd->usersd[0] = &sd;
		
		cd->limit = limit;
		cd->pub = pub;
		cd->users = 1;

		memcpy(cd->pass,pass,sizeof(pass)-1);
		pass[sizeof(pass)-1]=0;

		if( titlelen+1>=sizeof(cd->title)) titlelen=sizeof(cd->title)-1;
		memcpy(cd->title,title,titlelen);
		cd->title[titlelen]=0;

		pc_setchatid(sd, cd->block_list::id);
		clif_createchat(sd,0);
		clif_dispchat(*cd,0);
	}
	return 0;
}

/*==========================================
 * �����`���b�g���[���ɎQ��
 *------------------------------------------
 */
int chat_joinchat(struct map_session_data &sd,uint32 chatid,const char* pass)
{
	chat_data *cd = (chat_data*)map_id2bl(chatid);

	if( !cd || cd->block_list::m != sd.block_list::m || sd.chatID || cd->limit <= cd->users )
	{
		clif_joinchatfail(sd,0);
		return 0;
	}

	//Allows Gm access to protected room with any password they want by valaris
	if( (cd->pub == 0 && 0!=strncmp(pass, (char *)cd->pass, 8) && (sd.isGM()<battle_config.gm_join_chat || !battle_config.gm_join_chat)) 
		|| chatid == sd.chatID ) //Double Chat fix by Alex14, thx CHaNGeTe
	{
		clif_joinchatfail(sd,1);
		return 0;
	}

	cd->usersd[cd->users] = &sd;
	cd->users++;

	pc_setchatid(sd,cd->block_list::id);

	clif_joinchatok(sd,*cd);	// �V���ɎQ�������l�ɂ͑S���̃��X�g
	clif_addchat(*cd,sd);	// ���ɒ��ɋ����l�ɂ͒ǉ������l�̕�
	clif_dispchat(*cd,0);	// ���͂̐l�ɂ͐l���ω���

	chat_triggerevent(*cd); // �C�x���g

	return 0;
}

/*==========================================
 * �`���b�g���[�����甲����
 *------------------------------------------
 */
int chat_leavechat(struct map_session_data &sd)
{
	chat_data *cd;
	size_t leavechar;

	cd=(chat_data*)map_id2bl(sd.chatID);
	if(cd==NULL)
		return 1;

	for(leavechar=0; leavechar<cd->users; ++leavechar)
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
		clif_changechatowner(*cd,*cd->usersd[1]);
		clif_clearchat(*cd,0);
	}

	// ������PC�ɂ�����̂�users�����炷�O�Ɏ��s
	clif_leavechat(*cd,sd);

	cd->users--;
	pc_setchatid(sd,0);

	if(cd->users == 0 && cd->owner && cd->owner->type==BL_PC)
	{	// �S�����Ȃ��Ȃ���&PC�̃`���b�g�Ȃ̂ŏ���
		clif_clearchat(*cd,0);
		map_delobject(cd->block_list::id);	// free�܂ł��Ă����
	}
	else
	{
		size_t i;
		for(i=leavechar; i+1<cd->users; ++i)
			cd->usersd[i] = cd->usersd[i+1];
		if(leavechar==0 && cd->owner && cd->owner->type==BL_PC)
		{	// PC�̃`���b�g�Ȃ̂ŏ��L�҂��������̂ňʒu�ύX
			cd->block_list::x = cd->owner->x;
			cd->block_list::y = cd->owner->y;
		}
		clif_dispchat(*cd,0);
	}
	return 0;
}

/*==========================================
 * �`���b�g���[���̎����������
 *------------------------------------------
 */
int chat_changechatowner(struct map_session_data &sd, const char *nextownername)
{
	chat_data *cd;
	struct map_session_data *tmp_sd;
	size_t nextowner;

	cd=(chat_data*)map_id2bl(sd.chatID);
	if(cd==NULL || &sd!=cd->owner)
		return 1;

	for(nextowner=1; nextowner<cd->users; ++nextowner){
		if(strcmp(cd->usersd[nextowner]->status.name,nextownername)==0){
			break;
		}
	}
	if(nextowner>=cd->users) // ����Ȑl�͋��Ȃ�
		return -1;

	clif_changechatowner(*cd,*cd->usersd[nextowner]);
	// ��U����
	clif_clearchat(*cd,0);

	// userlist�̏��ԕύX (0�����L�҂Ȃ̂�)
	if( (tmp_sd = cd->usersd[0]) == NULL )
		return 1; //���肦��̂��ȁH
	cd->usersd[0] = cd->usersd[nextowner];
	cd->usersd[nextowner] = tmp_sd;

	// �V�������L�҂̈ʒu�֕ύX
	cd->block_list::x=cd->usersd[0]->block_list::x;
	cd->block_list::y=cd->usersd[0]->block_list::y;

	// �ēx�\��
	clif_dispchat(*cd,0);

	return 0;
}

/*==========================================
 * �`���b�g�̏��(�^�C�g����)��ύX
 *------------------------------------------
 */
int chat_changechatstatus(struct map_session_data &sd,unsigned short limit,unsigned char pub,const char* pass,const char* title, size_t titlelen)
{
	chat_data *cd;

	cd=(chat_data*)map_id2bl(sd.chatID);
	if(cd==NULL || &sd!=cd->owner)
		return 1;

	cd->limit = limit;
	cd->pub = pub;
	memcpy(cd->pass,pass,sizeof(pass)-1);
	cd->pass[sizeof(pass)-1]=0;

	if(titlelen+1>=sizeof(cd->title)) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	clif_changechatstatus(*cd);
	clif_dispchat(*cd,0);

	return 0;
}

/*==========================================
 * �`���b�g���[������R��o��
 *------------------------------------------
 */
int chat_kickchat(struct map_session_data &sd, const char *kickusername)
{
	chat_data *cd;
	size_t kickuser;

	cd = (chat_data *)map_id2bl(sd.chatID);
	if( cd == NULL )
		return 1;

	for(kickuser=1; kickuser<cd->users; kickuser++)
	{
		if( cd->usersd[kickuser] && 
			0==strcmp(cd->usersd[kickuser]->status.name, kickusername) )
		{
			if( cd->usersd[kickuser]->isGM() < battle_config.gm_kick_chat )
				chat_leavechat( *(cd->usersd[kickuser]) );
			return 0;
		}
	}

	return -1;
}

/*==========================================
 * npc�`���b�g���[���쐬
 *------------------------------------------
 */
int chat_createnpcchat(struct npc_data &nd,unsigned short limit,unsigned char pub, int trigger,const char* title, unsigned short titlelen,const char *ev)
{
	chat_data *cd;

	cd = new chat_data;

	cd->limit = cd->trigger = limit;
	if(trigger>0)
		cd->trigger = trigger;
	cd->pub = pub;
	cd->users = 0;
	memcpy(cd->pass,"",1);
	if((size_t)titlelen+1>=sizeof(cd->title)) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	cd->block_list::m = nd.block_list::m;
	cd->block_list::x = nd.block_list::x;
	cd->block_list::y = nd.block_list::y;
	cd->block_list::type = BL_CHAT;
	cd->owner = &nd;
	memcpy(cd->npc_event,ev,strlen(ev));

	cd->block_list::id = map_addobject(*cd);	
	if(cd->block_list::id==0)
	{
		delete cd;
		return 0;
	}
	nd.chat_id=cd->block_list::id;

	clif_dispchat(*cd,0);

	return 0;
}
/*==========================================
 * npc�`���b�g���[���폜
 *------------------------------------------
 */
int chat_deletenpcchat(struct npc_data &nd)
{
	chat_data *cd;

	nullpo_retr(0, cd=(chat_data*)map_id2bl(nd.chat_id));
	
	cd->kickall();
	clif_clearchat(*cd,0);
	map_delobject(cd->block_list::id);	// free�܂ł��Ă����
	nd.chat_id=0;
	
	return 0;
}

/*==========================================
 * �K��l���ȏ�ŃC�x���g����`����Ă�Ȃ���s
 *------------------------------------------
 */
int chat_triggerevent(chat_data &cd)
{
	if(cd.users>=cd.trigger && cd.npc_event[0])
		npc_event_do(cd.npc_event);
	return 0;
}

/*==========================================
 * �C�x���g�̗L����
 *------------------------------------------
 */
void chat_data::enable_event()
{
	this->trigger&=0x7f;
	chat_triggerevent(*this);
}
/*==========================================
 * �C�x���g�̖�����
 *------------------------------------------
 */
void chat_data::disable_event()
{
	this->trigger|=0x80;
}
/*==========================================
 * �`���b�g���[������S���R��o��
 *------------------------------------------
 */
void chat_data::kickall()
{
	while(this->users>0 && this->usersd[this->users-1])
		chat_leavechat( *this->usersd[this->users-1] );
}
