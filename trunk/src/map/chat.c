// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/malloc.h"
#include "../common/nullpo.h"
#include "../common/strlib.h"
#include "atcommand.h" // msg_txt()
#include "battle.h" // struct battle_config
#include "clif.h"
#include "map.h"
#include "npc.h" // npc_event_do()
#include "pc.h"
#include "chat.h"

#include <stdio.h>
#include <string.h>

int chat_triggerevent(struct chat_data *cd);

/*==========================================
 * chatroom creation
 *------------------------------------------*/
int chat_createchat(struct map_session_data* sd, int limit, bool pub, char* pass, char* title, int titlelen)
{
	struct chat_data *cd;

	nullpo_retr(0, sd);

	if (sd->chatID)
		return 0;	//Prevent people abusing the chat system by creating multiple chats, as pointed out by End of Exam. [Skotlex]

	if (map[sd->bl.m].flag.nochat) {
		clif_displaymessage (sd->fd, msg_txt(281));
		return 0; //Can't create chatrooms on this map.
	}
	pc_stop_walking(sd,1);
	cd = (struct chat_data *) aMalloc(sizeof(struct chat_data));

	cd->limit = limit;
	cd->pub = pub;
	cd->users = 1;
	titlelen = cap_value(titlelen, 0, sizeof(cd->title)-1); // empty string achievable by using custom client
	// the following two input strings aren't zero terminated and need to be handled carefully
	safestrncpy(cd->title, title, min(titlelen+1,CHATROOM_TITLE_SIZE));
	safestrncpy(cd->pass, pass, CHATROOM_PASS_SIZE);

	cd->owner = (struct block_list **)(&cd->usersd[0]);
	cd->usersd[0] = sd;
	cd->bl.m = sd->bl.m;
	cd->bl.x = sd->bl.x;
	cd->bl.y = sd->bl.y;
	cd->bl.type = BL_CHAT;
	cd->bl.next = cd->bl.prev = NULL;
	cd->bl.id = map_addobject(&cd->bl);	
	if(cd->bl.id==0){
		clif_createchat(sd,1);
		aFree(cd);
		return 0;
	}
	pc_setchatid(sd,cd->bl.id);

	clif_createchat(sd,0);
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * �����`���b�g���[���ɎQ��
 *------------------------------------------*/
int chat_joinchat(struct map_session_data* sd, int chatid, char* pass)
{
	struct chat_data *cd;

	nullpo_retr(0, sd);
	cd = (struct chat_data*)map_id2bl(chatid);

 //No need for a nullpo check. The chatid was sent by the client, if they lag or mess with the packet 
 //a wrong chat id can be received. [Skotlex]
	if (cd == NULL)
		return 1;
	if (cd->bl.type != BL_CHAT || cd->bl.m != sd->bl.m || sd->vender_id || sd->chatID || cd->limit <= cd->users) {
		clif_joinchatfail(sd,0);
		return 0;
	}
	//Allows Gm access to protected room with any password they want by valaris
	if ((!cd->pub && strncmp(pass, (char *)cd->pass, 8) && (pc_isGM(sd) < battle_config.gm_join_chat || !battle_config.gm_join_chat)) ||
		chatid == (int)sd->chatID) //Double Chat fix by Alex14, thx CHaNGeTe
	{
		clif_joinchatfail(sd,1);
		return 0;
	}

	pc_stop_walking(sd,1);
	cd->usersd[cd->users] = sd;
	cd->users++;

	pc_setchatid(sd,cd->bl.id);

	clif_joinchatok(sd,cd);	// �V���ɎQ�������l�ɂ͑S���̃��X�g
	clif_addchat(cd,sd);	// ���ɒ��ɋ����l�ɂ͒ǉ������l�̕�
	clif_dispchat(cd,0);	// ���͂̐l�ɂ͐l���ω���

	chat_triggerevent(cd); // �C�x���g
	
	return 0;
}

/// Removes the user from the chat room.
int chat_leavechat(struct map_session_data* sd)
{
	struct chat_data* cd;
	int i;
	int leavechar;

	nullpo_retr(1, sd);

	cd = (struct chat_data*)map_id2bl(sd->chatID);
	if( cd == NULL )
	{
		pc_setchatid(sd, 0);
		return 1;
	}

	for( i = 0, leavechar = -1; i < cd->users; i++ )
	{
		if(cd->usersd[i] == sd){
			leavechar=i;
			break;
		}
	}
	if( leavechar < 0 )
	{// Not found in the chatroom?
		pc_setchatid(sd, 0);
		return -1;
	}

	if( leavechar == 0 && cd->users > 1 && (*cd->owner)->type == BL_PC )
	{// Change ownership to the next user
		clif_changechatowner(cd, cd->usersd[1]);
		clif_clearchat(cd, 0);
	}

	// ������PC�ɂ�����̂�users�����炷�O�Ɏ��s
	clif_leavechat(cd, sd);

	cd->users--;
	pc_setchatid(sd, 0);

	if( cd->users == 0 && (*cd->owner)->type == BL_PC )
	{// Delete empty chatroom
		clif_clearchat(cd, 0);
		map_delobject(cd->bl.id);
		return 1;
	}
	for( i = leavechar; i < cd->users; i++ )
		cd->usersd[i] = cd->usersd[i + 1];

	if( leavechar == 0 && (*cd->owner)->type == BL_PC )
	{
		//Adjust Chat location after owner has been changed.
		map_delblock( &cd->bl );
		cd->bl.x=cd->usersd[0]->bl.x;
		cd->bl.y=cd->usersd[0]->bl.y;
		map_addblock( &cd->bl );
	}
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * �`���b�g���[���̎����������
 *------------------------------------------*/
int chat_changechatowner(struct map_session_data* sd, char* nextownername)
{
	struct chat_data *cd;
	struct map_session_data *tmp_sd;
	int i, nextowner;

	nullpo_retr(1, sd);

	cd = (struct chat_data*)map_id2bl(sd->chatID);
	if (cd == NULL || (struct block_list *)sd != (*cd->owner))
		return 1;

	for(i = 1,nextowner=-1;i < cd->users;i++){
		if(strcmp(cd->usersd[i]->status.name,nextownername)==0){
			nextowner=i;
			break;
		}
	}
	if(nextowner<0) // ����Ȑl�͋��Ȃ�
		return -1;

	clif_changechatowner(cd,cd->usersd[nextowner]);
	// ��U����
	clif_clearchat(cd,0);

	// userlist�̏��ԕύX (0�����L�҂Ȃ̂�)
	if( (tmp_sd = cd->usersd[0]) == NULL ) //FIXME: How is this even possible!? Invoking character should be owner, hence, it SHOULD be on sc->usersd[0]!
		return 1; //���肦��̂��ȁH
	cd->usersd[0] = cd->usersd[nextowner];
	cd->usersd[nextowner] = tmp_sd;

	map_delblock( &cd->bl );
	cd->bl.x=cd->usersd[0]->bl.x;
	cd->bl.y=cd->usersd[0]->bl.y;
	map_addblock( &cd->bl );

	// �ēx�\��
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * �`���b�g�̏��(�^�C�g����)��ύX
 *------------------------------------------*/
int chat_changechatstatus(struct map_session_data* sd, char* title, char* pass, int limit, bool pub)
{
	struct chat_data *cd;

	nullpo_retr(1, sd);

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL || (struct block_list *)sd != (*cd->owner))
		return 1;

	safestrncpy(cd->title, title, CHATROOM_TITLE_SIZE);
	safestrncpy(cd->pass, pass, CHATROOM_PASS_SIZE);
	cd->limit = limit;
	cd->pub = pub;

	clif_changechatstatus(cd);
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * �`���b�g���[������R��o��
 *------------------------------------------*/
int chat_kickchat(struct map_session_data* sd,char* kickusername)
{
	struct chat_data *cd;
	int i;

	nullpo_retr(1, sd);

	cd = (struct chat_data *)map_id2bl(sd->chatID);
	
	if (!cd) return -1;

	for(i = 0; i < cd->users; i++) {
		if (strcmp(cd->usersd[i]->status.name, kickusername) == 0) {
			if (battle_config.gm_kick_chat && pc_isGM(cd->usersd[i]) >= battle_config.gm_kick_chat)
				//gm kick protection by valaris
				return 0;

			chat_leavechat(cd->usersd[i]);
			return 0;
		}
	}

	return -1;
}

/// Creates a chat room for the npc.
int chat_createnpcchat(struct npc_data* nd, int limit, bool pub, int trigger, const char* title, const char* ev)
{
	struct chat_data *cd;

	nullpo_retr(1, nd);

	cd = (struct chat_data *) aMalloc(sizeof(struct chat_data));

	cd->limit = cd->trigger = limit;
	if( trigger > 0 )
		cd->trigger = trigger;
	cd->pub = pub;
	cd->users = 0;

	safestrncpy(cd->title, title, CHATROOM_TITLE_SIZE);
	memset(cd->pass, '\0', CHATROOM_PASS_SIZE);
	cd->bl.m    = nd->bl.m;
	cd->bl.x    = nd->bl.x;
	cd->bl.y    = nd->bl.y;
	cd->bl.type = BL_CHAT;
	cd->bl.prev = cd->bl.next = NULL;
	cd->owner   = &(struct block_list *)nd;
	safestrncpy(cd->npc_event, ev, ARRAYLENGTH(cd->npc_event));
	cd->bl.id = map_addobject(&cd->bl);	
	if( cd->bl.id == 0)
	{
		aFree(cd);
		return 0;
	}
	nd->chat_id = cd->bl.id;

	clif_dispchat(cd, 0);

	return 0;
}

/// Removes the chatroom from the npc.
int chat_deletenpcchat(struct npc_data* nd)
{
	struct chat_data *cd;

	nullpo_retr(0, nd);
	nullpo_retr(0, cd=(struct chat_data*)map_id2bl(nd->chat_id));
	
	chat_npckickall(cd);
	clif_clearchat(cd, 0);
	map_delobject(cd->bl.id);	// free�܂ł��Ă����
	nd->chat_id = 0;
	
	return 0;
}

/*==========================================
 * �K��l���ȏ�ŃC�x���g����`����Ă�Ȃ���s
 *------------------------------------------*/
int chat_triggerevent(struct chat_data *cd)
{
	nullpo_retr(0, cd);

	if( cd->users >= cd->trigger && cd->npc_event[0] )
		npc_event_do(cd->npc_event);
	return 0;
}

/// Enables the event of the chat room.
/// At most, 127 users are needed to trigger the event.
int chat_enableevent(struct chat_data* cd)
{
	nullpo_retr(0, cd);

	cd->trigger &= 0x7f;
	chat_triggerevent(cd);
	return 0;
}

/// Disables the event of the chat room
int chat_disableevent(struct chat_data* cd)
{
	nullpo_retr(0, cd);

	cd->trigger |= 0x80;
	return 0;
}

/// Kicks all the users for the chat room.
int chat_npckickall(struct chat_data* cd)
{
	nullpo_retr(0, cd);

	while( cd->users > 0 )
		chat_leavechat(cd->usersd[cd->users-1]);

	return 0;
}
