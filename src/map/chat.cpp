// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder
#include "nullpo.h"
#include "showmsg.h"
#include "utils.h"

#include "chat.h"
#include "battle.h"
#include "map.h"
#include "clif.h"
#include "pc.h"
#include "npc.h"




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

		cd->owner	= &sd;
		cd->usersd[0] = &sd;
		
		cd->limit = limit;
		cd->pub = pub;
		cd->users = 1;

		safestrcpy(cd->pass, sizeof(cd->pass), pass);
		safestrcpy(cd->title,sizeof(cd->title),title);

		sd.chat = cd;
		clif_createchat(sd,0);
		clif_dispchat(*cd,0);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// �����`���b�g���[���ɎQ��
bool chat_data::join(map_session_data &sd, uint32 chatid, const char* pass)
{
	chat_data *cd = chat_data::from_blid(chatid);

	if( !cd || cd->block_list::m != sd.block_list::m || sd.chat || cd->limit <= cd->users )
	{
		clif_joinchatfail(sd,0);
	}
	else if( (cd->pub == 0 && 0!=strncmp(pass, (char *)cd->pass, 8) && 
		(sd.isGM()<config.gm_join_chat || !config.gm_join_chat)) )
	{	// Allows Gm access to protected room with any password they want by valaris
		clif_joinchatfail(sd,1);
	}
	else
	{
		cd->usersd[cd->users] = &sd;
		cd->users++;

		sd.chat = cd;

		clif_joinchatok(sd,*cd);	// �V���ɎQ�������l�ɂ͑S���̃��X�g
		clif_addchat(*cd,sd);	// ���ɒ��ɋ����l�ɂ͒ǉ������l�̕�
		clif_dispchat(*cd,0);	// ���͂̐l�ɂ͐l���ω���

		cd->trigger_event(); // �C�x���g

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
/// �`���b�g���[������S���R��o��
void chat_data::kickall()
{
	while( this->users>0 && this->usersd[this->users-1] )
		this->remove( *this->usersd[this->users-1] );
}

///////////////////////////////////////////////////////////////////////////////
/// removes session from chat.
/// �`���b�g���[�����甲����
bool chat_data::remove(map_session_data &sd)
{
	if( sd.chat && sd.chat==this )
	{
		size_t i;
		
		// in all cases, remove the chat pointer from the session
		sd.chat = NULL;

		for(i=0; i<this->users; ++i)
		{
			if(this->usersd[i] == &sd)
			{	// found

				if( i==0 && this->users>1 && this->owner && *this->owner==BL_PC )
				{	// ���L�҂�����&���ɐl������&PC�̃`���b�g
					clif_changechatowner(*this,*this->usersd[1]);
					clif_clearchat(*this,0);
				}

				// ������PC�ɂ�����̂�users�����炷�O�Ɏ��s
				clif_leavechat(*this, sd);

				--this->users;

				if(this->users == 0 && this->owner && *this->owner==BL_PC)
				{	// all users have left
					clif_clearchat(*this,0);
					map_delobject(this->block_list::id);	// free�܂ł��Ă����
				}
				else
				{
					if(i==0 && this->owner && *this->owner==BL_PC)
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
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
/// �`���b�g���[���̎����������
bool chat_data::change_owner(map_session_data &sd, const char *nextownername)
{
	if( sd.chat==this && &sd==this->owner )
	{
		size_t i;
		for(i=0; i<this->users; ++i)
		{
			if( this->usersd[i] && &sd!=this->usersd[i] && 0==strcmp(this->usersd[i]->status.name, nextownername) )
			{
				clif_changechatowner(*this, *this->usersd[i]);
				clif_clearchat(*this,0);

				basics::swap(this->usersd[0], this->usersd[i]);

				// �V�������L�҂̈ʒu�֕ύX
				this->block_list::x = this->usersd[0]->block_list::x;
				this->block_list::y = this->usersd[0]->block_list::y;

				clif_dispchat(*this,0);
				return true;
			}
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// �`���b�g�̏��(�^�C�g����)��ύX
bool chat_data::change_status(map_session_data &sd, unsigned short limit, unsigned char pub, const char* pass,const char* title)
{
	if( sd.chat==this && &sd==this->owner )
	{
		this->limit = limit;
		this->pub = pub;
		safestrcpy(this->pass, sizeof(this->pass), pass);
		safestrcpy(this->title, sizeof(this->title), title);

		clif_changechatstatus(*this);
		clif_dispchat(*this,0);
		return true;
	}
	return false;
}













///////////////////////////////////////////////////////////////////////////////
/// npc�`���b�g���[���쐬
bool npcchat_data::create(npcscript_data &nd, unsigned short limit, unsigned char pub, int trigger, const char* title, const char *ev)
{
	npcchat_data *cd = new npcchat_data();
	cd->block_list::id = map_addobject(*cd);
	if(cd->block_list::id==0)
	{
		delete cd;
	}
	else
	{
		cd->limit = cd->trigger = limit;

		if(trigger>0)
			cd->trigger = trigger;

		cd->pub = pub;
		cd->users = 0;

		*cd->pass = 0;

		safestrcpy(cd->title,sizeof(cd->title),title);

		cd->block_list::m = nd.block_list::m;
		cd->block_list::x = nd.block_list::x;
		cd->block_list::y = nd.block_list::y;
		cd->owner = &nd;
		safestrcpy(cd->npc_event, sizeof(cd->npc_event), ev);


		nd.chat = cd;

		clif_dispchat(*cd,0);
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// npc�`���b�g���[���폜
bool npcchat_data::erase(npcscript_data &nd)
{
	if(nd.chat)
	{
		npcchat_data* cd = nd.chat;

		cd->kickall();
		clif_clearchat(*cd,0);
		map_delobject(cd->block_list::id);	// free�܂ł��Ă����
		nd.chat = NULL;
		return true;
	}
	return false;
}



///////////////////////////////////////////////////////////////////////////////
/// �C�x���g�̗L����
void npcchat_data::enable_event()
{
	this->trigger&=0x7f;
	this->trigger_event();
}

///////////////////////////////////////////////////////////////////////////////
/// �C�x���g�̖�����
void npcchat_data::disable_event()
{
	this->trigger|=0x80;
}

///////////////////////////////////////////////////////////////////////////////
/// �K��l���ȏ�ŃC�x���g����`����Ă�Ȃ���s
void npcchat_data::trigger_event()
{
	if( this->users >= this->trigger && this->npc_event[0] )
		npc_event_do(this->npc_event);
}





