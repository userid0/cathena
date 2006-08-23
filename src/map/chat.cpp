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


///////////////////////////////////////////////////////////////////////////////
/// �����`���b�g���[���ɎQ��
int chat_joinchat(struct map_session_data &sd, uint32 chatid, const char* pass)
{
	chat_data *cd = chat_data::from_blid(chatid);

	if( !cd || cd->block_list::m != sd.block_list::m || sd.chat || cd->limit <= cd->users )
	{
		clif_joinchatfail(sd,0);
		return 0;
	}

	//Allows Gm access to protected room with any password they want by valaris
	if( (cd->pub == 0 && 0!=strncmp(pass, (char *)cd->pass, 8) && 
		(sd.isGM()<config.gm_join_chat || !config.gm_join_chat)) )
	{
		clif_joinchatfail(sd,1);
		return 0;
	}

	cd->usersd[cd->users] = &sd;
	cd->users++;

	sd.chat = cd;

	clif_joinchatok(sd,*cd);	// �V���ɎQ�������l�ɂ͑S���̃��X�g
	clif_addchat(*cd,sd);	// ���ɒ��ɋ����l�ɂ͒ǉ������l�̕�
	clif_dispchat(*cd,0);	// ���͂̐l�ɂ͐l���ω���

	chat_triggerevent(*cd); // �C�x���g

	return 0;
}



///////////////////////////////////////////////////////////////////////////////
/// �`���b�g���[���̎����������
int chat_changechatowner(struct map_session_data &sd, const char *nextownername)
{
	chat_data *cd=sd.chat;
	struct map_session_data *tmp_sd;
	size_t nextowner;

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

///////////////////////////////////////////////////////////////////////////////
/// �`���b�g�̏��(�^�C�g����)��ύX
int chat_changechatstatus(struct map_session_data &sd,unsigned short limit,unsigned char pub,const char* pass,const char* title, size_t titlelen)
{
	chat_data *cd = sd.chat;

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



///////////////////////////////////////////////////////////////////////////////
/// npc�`���b�g���[���쐬
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
///////////////////////////////////////////////////////////////////////////////
/// npc�`���b�g���[���폜
int chat_deletenpcchat(struct npc_data &nd)
{
	chat_data *cd;

	nullpo_retr(0, cd=chat_data::from_blid(nd.chat_id));
	
	cd->kickall();
	clif_clearchat(*cd,0);
	map_delobject(cd->block_list::id);	// free�܂ł��Ă����
	nd.chat_id=0;
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// �K��l���ȏ�ŃC�x���g����`����Ă�Ȃ���s
int chat_triggerevent(chat_data &cd)
{
	if(cd.users>=cd.trigger && cd.npc_event[0])
		npc_event_do(cd.npc_event);
	return 0;
}













///////////////////////////////////////////////////////////////////////////////
/// �C�x���g�̗L����
void chat_data::enable_event()
{
	this->trigger&=0x7f;
	chat_triggerevent(*this);
}
///////////////////////////////////////////////////////////////////////////////
/// �C�x���g�̖�����
void chat_data::disable_event()
{
	this->trigger|=0x80;
}
///////////////////////////////////////////////////////////////////////////////
/// �`���b�g���[������S���R��o��
void chat_data::kickall()
{
	while( this->users>0 && this->usersd[this->users-1] )
		this->remove( *this->usersd[this->users-1] );
}

///////////////////////////////////////////////////////////////////////////////
/// create a chat.
/// �`���b�g���[���쐬
bool chat_data::create(map_session_data& sd, unsigned short limit, unsigned char pub, const char* pass, const char* title)
{
	if( !sd.chat )
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

		safestrcpy(cd->pass, sizeof(pass), pass);
		safestrcpy(cd->title,sizeof(title),title);

		sd.chat = cd;
		clif_createchat(sd,0);
		clif_dispchat(*cd,0);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// kick user from chat
/// �`���b�g���[������R��o��
bool chat_data::kick(const char *kickusername)
{
	size_t i;
	for(i=1; i<this->users; ++i)
	{
		if( this->usersd[i] && 
			0==strcmp(this->usersd[i]->status.name, kickusername) )
		{
			if( this->usersd[i]->isGM() < config.gm_kick_chat )
				this->remove( *this->usersd[i] );
			return true;
		}
	}
	return false;
}
///////////////////////////////////////////////////////////////////////////////
/// removes session from chat.
/// �`���b�g���[�����甲����
bool chat_data::remove(map_session_data &sd)
{
	if( sd.chat && sd.chat==this )
	{
		size_t i;
		
		for(i=0; i<this->users; ++i)
		{
			if(this->usersd[i] == &sd)
			{	// found

				if( i==0 && this->users>1 && this->owner && this->owner->type==BL_PC )
				{	// ���L�҂�����&���ɐl������&PC�̃`���b�g
					clif_changechatowner(*this,*this->usersd[1]);
					clif_clearchat(*this,0);
				}

				// ������PC�ɂ�����̂�users�����炷�O�Ɏ��s
				clif_leavechat(*this, sd);

				--this->users;

				if(this->users == 0 && this->owner && this->owner->type==BL_PC)
				{	// all users have left
					clif_clearchat(*this,0);
					map_delobject(this->block_list::id);	// free�܂ł��Ă����
				}
				else
				{
					if(i==0 && this->owner && this->owner->type==BL_PC)
					{	// PC�̃`���b�g�Ȃ̂ŏ��L�҂��������̂ňʒu�ύX
						this->block_list::x = this->owner->x;
						this->block_list::y = this->owner->y;
					}
					clif_dispchat(*this,0);

					// shift the users
					for(++i; i<this->users; ++i)
						this->usersd[i-1] = this->usersd[i];
				}

				break;
			}
		}
		// in all cases, remove the chat pointer from the session
		sd.chat = NULL;

		return true;
	}
	return false;
}


