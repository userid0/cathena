// $Id: clif.c 2200 2004-11-07 11:49:58Z Yor $

#define DUMP_UNKNOWN_PACKET	1
#define	DUMP_ALL_PACKETS	0
#include "base.h"
#include "socket.h"
#include "timer.h"
#include "malloc.h"
#include "version.h"
#include "nullpo.h"
#include "showmsg.h"
#include "utils.h"

#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "pc.h"
#include "status.h"
#include "npc.h"
#include "itemdb.h"
#include "chat.h"
#include "trade.h"
#include "storage.h"
#include "script.h"
#include "skill.h"
#include "atcommand.h"
#include "charcommand.h"
#include "intif.h"
#include "battle.h"
#include "mob.h"
#include "party.h"
#include "guild.h"
#include "vending.h"
#include "pet.h"
#include "log.h"

#define STATE_BLIND 0x10

struct Clif_Config clif_config;
struct packet_db packet_db[MAX_PACKET_VER + 1][MAX_PACKET_DB];

#define USE_PACKET_DB(sd) clif_config.enable_packet_db && sd->packet_ver == clif_config.packet_db_ver
#define IS_PACKET_DB_VER(cmd) cmd == clif_config.connect_cmd


static const int packet_len_table[MAX_PACKET_DB] = {
   10,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
//#0x0040
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0, 55, 17,  3, 37,  46, -1, 23, -1,  3,108,  3,  2,
#if PACKETVER < 2
    3, 28, 19, 11,  3, -1,  9,  5,  52, 51, 56, 58, 41,  2,  6,  6,
#else	// 78-7b �T���ȍ~ lv99�G�t�F�N�g�p
    3, 28, 19, 11,  3, -1,  9,  5,  54, 53, 58, 60, 41,  2,  6,  6,
#endif
//#0x0080
    7,  3,  2,  2,  2,  5, 16, 12,  10,  7, 29,  2, -1, -1, -1,  0, // 0x8b changed to 2 (was 23)
    7, 22, 28,  2,  6, 30, -1, -1,   3, -1, -1,  5,  9, 17, 17,  6,
   23,  6,  6, -1, -1, -1, -1,  8,   7,  6,  7,  4,  7,  0, -1,  6,
    8,  8,  3,  3, -1,  6,  6, -1,   7,  6,  2,  5,  6, 44,  5,  3,
//#0x00C0
    7,  2,  6,  8,  6,  7, -1, -1,  -1, -1,  3,  3,  6,  3,  2, 27, // 0xcd change to 3 (was 6)
    3,  4,  4,  2, -1, -1,  3, -1,   6, 14,  3, -1, 28, 29, -1, -1,
   30, 30, 26,  2,  6, 26,  3,  3,   8, 19,  5,  2,  3,  2,  2,  2,
    3,  2,  6,  8, 21,  8,  8,  2,   2, 26,  3, -1,  6, 27, 30, 10,

//#0x0100
    2,  6,  6, 30, 79, 31, 10, 10,  -1, -1,  4,  6,  6,  2, 11, -1,
   10, 39,  4, 10, 31, 35, 10, 18,   2, 13, 15, 20, 68,  2,  3, 16,
    6, 14, -1, -1, 21,  8,  8,  8,   8,  8,  2,  2,  3,  4,  2, -1,
    6, 86,  6, -1, -1,  7, -1,  6,   3, 16,  4,  4,  4,  6, 24, 26,
//#0x0140
   22, 14,  6, 10, 23, 19,  6, 39,   8,  9,  6, 27, -1,  2,  6,  6,
  110,  6, -1, -1, -1, -1, -1,  6,  -1, 54, 66, 54, 90, 42,  6, 42,
   -1, -1, -1, -1, -1, 30, -1,  3,  14,  3, 30, 10, 43, 14,186,182,
   14, 30, 10,  3, -1,  6,106, -1,   4,  5,  4, -1,  6,  7, -1, -1,
//#0x0180
    6,  3,106, 10, 10, 34,  0,  6,   8,  4,  4,  4, 29, -1, 10,  6,
#if PACKETVER < 1
   90, 86, 24,  6, 30,102,  8,  4,   8,  4, 14, 10, -1,  6,  2,  6,
#else	// 196 comodo�ȍ~ ��ԕ\���A�C�R���p
   90, 86, 24,  6, 30,102,  9,  4,   8,  4, 14, 10, -1,  6,  2,  6,
#endif
    3,  3, 35,  5, 11, 26, -1,  4,   4,  6, 10, 12,  6, -1,  4,  4,
   11,  7, -1, 67, 12, 18,114,  6,   3,  6, 26, 26, 26, 26,  2,  3,
//#0x01C0,   Set 0x1d5=-1
    2, 14, 10, -1, 22, 22,  4,  2,  13, 97,  3,  9,  9, 30,  6, 28,
    8, 14, 10, 35,  6, -1,  4, 11,  54, 53, 60,  2, -1, 47, 33,  6,
   30,  8, 34, 14,  2,  6, 26,  2,  28, 81,  6, 10, 26,  2, -1, -1,
   -1, -1, 20, 10, 32,  9, 34, 14,   2,  6, 48, 56, -1,  4,  5, 10,
//#0x200
   26, -1,  26, 10, 18, 26, 11, 34,  14, 36, 10, 0,  0, -1, 24, 10, // 0x20c change to 0 (was 19)
   22,  0,  26, 26, 42, -1, -1,  2,   0,282,282,10, 10, -1, -1, 66,
   10, -1,  -1,  8, 10,  2,282, 18,  18, 15, 58, 57, 64, 5,  0,  0,
    0,  0,   9, 11, -1
};


// local define
enum {
	ALL_CLIENT,
	ALL_SAMEMAP,
	AREA,
	AREA_WOS,
	AREA_WOC,
	AREA_WOSC,
	AREA_CHAT_WOC,
	CHAT,
	CHAT_WOS,
	PARTY,
	PARTY_WOS,
	PARTY_SAMEMAP,
	PARTY_SAMEMAP_WOS,
	PARTY_AREA,
	PARTY_AREA_WOS,
	GUILD,
	GUILD_WOS,
	GUILD_SAMEMAP,	// [Valaris]
	GUILD_SAMEMAP_WOS,
	GUILD_AREA,
	GUILD_AREA_WOS,	// end additions [Valaris]
	SELF
};
#define WBUFPOS(p,pos,x,y) { unsigned char *__p = (p); __p+=(pos); __p[0] = (unsigned char)((x)>>2); __p[1] = (unsigned char)(((x)<<6) | (((y)>>4)&0x3f)); __p[2] = (unsigned char)((y)<<4); }
#define WBUFPOS2(p,pos,x0,y0,x1,y1) { unsigned char *__p = (p); __p+=(pos); __p[0] = (unsigned char)((x0)>>2); __p[1] = (unsigned char)(((x0)<<6) | (((y0)>>4)&0x3f)); __p[2] = (unsigned char)(((y0)<<4) | (((x1)>>6)&0x0f)); __p[3]=(unsigned char)(((x1)<<2) | (((y1)>>8)&0x03)); __p[4]=(unsigned char)((y1)); }

#define WFIFOPOS(fd,pos,x,y) { WBUFPOS (WFIFOP(fd,pos),0,x,y); }
#define WFIFOPOS2(fd,pos,x0,y0,x1,y1) { WBUFPOS2(WFIFOP(fd,pos),0,x0,y0,x1,y1); }


static unsigned long	map_ip	= INADDR_ANY;
static unsigned short	map_port= 5121;
int map_fd;
char talkie_mes[80];


/*==========================================
 * map�I��ip�ݒ�
 *------------------------------------------
 */
void clif_setip(unsigned long ip)
{
	map_ip = ip;
}

/*==========================================
 * map�I��port�ݒ�
 *------------------------------------------
 */
void clif_setport(unsigned short port)
{
	map_port = port;
}

/*==========================================
 * map�I��ip�ǂݏo��
 *------------------------------------------
 */
unsigned long clif_getip(void)
{
	return map_ip;
}

/*==========================================
 * map�I��port�ǂݏo��
 *------------------------------------------
 */
unsigned short clif_getport(void)
{
	return map_port;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_countusers(void)
{
	size_t users = 0, i;
	struct map_session_data *sd;

	for(i = 0; i < fd_max; i++)
	{
		if( session[i] && 
			(sd = (struct map_session_data*)session[i]->session_data) && 
			sd->state.auth &&
		    !(battle_config.hide_GM_session && pc_isGM(*sd)) )
			users++;
	}
	return users;
}

/*==========================================
 * �S�Ă�client�ɑ΂���func()���s
 *------------------------------------------
 */
 
int clif_foreachclient(int (*func)(struct map_session_data&, va_list),...) //recoded by sasuke, bug when player count gets higher [Kevin]
{
	size_t i;
	va_list ap;
	struct map_session_data *sd;
	va_start(ap,func);
	for(i = 0; i < fd_max; i++)
	{
		if ( session[i] && (sd = (struct map_session_data*)session[i]->session_data) && sd->state.auth )
			func(*sd, ap);
	}
	va_end(ap);
	return 0;
}

/*==========================================
 * clif_send��AREA*�w�莞�p
 *------------------------------------------
 */
int clif_send_sub(struct block_list *bl, va_list ap)
{
	struct block_list *src_bl;
	struct map_session_data *sd;
	unsigned char *buf;
	int len, type;
	
	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	buf = va_arg(ap,unsigned char*);
	len = va_arg(ap,int);
	nullpo_retr(0, src_bl = va_arg(ap,struct block_list*));
	type = va_arg(ap,int);

	switch(type) {
	case AREA_WOS:
		if (bl == src_bl)
			return 0;
		break;
	case AREA_WOC:
		if (sd->chatID || bl == src_bl)
			return 0;
		break;
	case AREA_WOSC:
		{
			struct map_session_data *ssd = (struct map_session_data *)src_bl;
			if (ssd && sd->chatID && sd->chatID == ssd->chatID)
				return 0;
		}
		break;
	}

	if (session[sd->fd] != NULL) {
		if (WFIFOP(sd->fd,0) == buf) {
			ShowMessage("WARNING: Invalid use of clif_send function\n");
			ShowMessage("         Packet x%4x use a WFIFO of a player instead of to use a buffer.\n", (unsigned short)WBUFW(buf,0));
			ShowMessage("         Please correct your code.\n");
			// don't send to not move the pointer of the packet for next sessions in the loop
		} else {
			if (packet_db[sd->packet_ver][RBUFW(buf,0)].len) { // packet must exist for the client version
				memcpy(WFIFOP(sd->fd,0), buf, len);
				WFIFOSET(sd->fd,len);
			}
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_send (unsigned char *buf, int len, struct block_list *bl, int type) {
	size_t i;
	struct map_session_data *sd = NULL;
	struct party *p = NULL;
	struct guild *g = NULL;
	int x0 = 0, x1 = 0, y0 = 0, y1 = 0;

	if (type != ALL_CLIENT) {
		nullpo_retr(0, bl);
		if (bl->type == BL_PC) {
			nullpo_retr (0, sd = (struct map_session_data *)bl);
		}
	}

	switch(type) {
	case ALL_CLIENT: // �S�N���C�A���g�ɑ��M
		for (i = 0; i < fd_max; i++) {
			if (session[i] && (sd = (struct map_session_data *)session[i]->session_data) != NULL && sd->state.auth) {
				if (packet_db[sd->packet_ver][RBUFW(buf,0)].len) { // packet must exist for the client version
					memcpy(WFIFOP(i,0), buf, len);
					WFIFOSET(i,len);
				}
			}
		}
		break;
	case ALL_SAMEMAP: // �����}�b�v�̑S�N���C�A���g�ɑ��M
		for(i = 0; i < fd_max; i++) {
			if (session[i] && (sd = (struct map_session_data*)session[i]->session_data) != NULL &&
				sd->state.auth && sd->bl.m == bl->m) {
				if (packet_db[sd->packet_ver][RBUFW(buf,0)].len) { // packet must exist for the client version
					memcpy(WFIFOP(i,0), buf, len);
					WFIFOSET(i,len);
				}
			}
		}
		break;
	case AREA:
	case AREA_WOS:
	case AREA_WOC:
	case AREA_WOSC:
		map_foreachinarea(clif_send_sub, bl->m, bl->x-AREA_SIZE, bl->y-AREA_SIZE, bl->x+AREA_SIZE, bl->y+AREA_SIZE,
			BL_PC, buf, len, bl, type);
		break;
	case AREA_CHAT_WOC:
		map_foreachinarea(clif_send_sub, bl->m, bl->x-(AREA_SIZE-5), bl->y-(AREA_SIZE-5),
			bl->x+(AREA_SIZE-5), bl->y+(AREA_SIZE-5), BL_PC, buf, len, bl, AREA_WOC);
		break;
	case CHAT:
	case CHAT_WOS:
		{
			struct chat_data *cd;
			if (sd) {
				cd = (struct chat_data*)map_id2bl(sd->chatID);
			} else if (bl->type == BL_CHAT) {
				cd = (struct chat_data*)bl;
			} else break;
			if (cd == NULL)
				break;
			for(i = 0; i < cd->users; i++) {
				if (type == CHAT_WOS && cd->usersd[i] == sd)
					continue;
				if (packet_db[cd->usersd[i]->packet_ver][RBUFW(buf,0)].len) { // packet must exist for the client version
					if (cd->usersd[i]->fd >=0 && session[cd->usersd[i]->fd]) // Added check to see if session exists [PoW]
						memcpy(WFIFOP(cd->usersd[i]->fd,0), buf, len);
					WFIFOSET(cd->usersd[i]->fd,len);
				}
			}
		}
		break;

	case PARTY_AREA:		// ������ʓ��̑S�p�[�e�B�[�����o�ɑ��M
	case PARTY_AREA_WOS:	// �����ȊO�̓�����ʓ��̑S�p�[�e�B�[�����o�ɑ��M
		x0 = bl->x - AREA_SIZE;
		y0 = bl->y - AREA_SIZE;
		x1 = bl->x + AREA_SIZE;
		y1 = bl->y + AREA_SIZE;
	case PARTY:				// �S�p�[�e�B�[�����o�ɑ��M
	case PARTY_WOS:			// �����ȊO�̑S�p�[�e�B�[�����o�ɑ��M
	case PARTY_SAMEMAP:		// �����}�b�v�̑S�p�[�e�B�[�����o�ɑ��M
	case PARTY_SAMEMAP_WOS:	// �����ȊO�̓����}�b�v�̑S�p�[�e�B�[�����o�ɑ��M
		if (sd) {
			if (sd->partyspy > 0) {
				p = party_search(sd->partyspy);
			} else if (sd->status.party_id > 0) {
				p = party_search(sd->status.party_id);
			}
		}
		if (p) {
			for(i=0;i<MAX_PARTY;i++){
				if ((sd = p->member[i].sd) != NULL) {
					if ((session[sd->fd] == NULL) || (session[sd->fd]->session_data == NULL))
						continue;
					if (sd->bl.id == bl->id && (type == PARTY_WOS ||
					    type == PARTY_SAMEMAP_WOS || type == PARTY_AREA_WOS))
						continue;
					if (type != PARTY && type != PARTY_WOS && bl->m != sd->bl.m) // �}�b�v�`�F�b�N
						continue;
					if ((type == PARTY_AREA || type == PARTY_AREA_WOS) &&
					    (sd->bl.x < x0 || sd->bl.y < y0 ||
					     sd->bl.x > x1 || sd->bl.y > y1))
						continue;
					if (packet_db[sd->packet_ver][RBUFW(buf,0)].len) { // packet must exist for the client version
						memcpy(WFIFOP(sd->fd,0), buf, len);
						WFIFOSET(sd->fd,len);
					}
//					if(battle_config.etc_log)
//						ShowMessage("send party %d %d %d\n",p->party_id,i,flag)

				}
			}
			for (i = 0; i < fd_max; i++){
				if (session[i] && (sd = (struct map_session_data*)session[i]->session_data) != NULL && sd->state.auth) {
					if (sd->partyspy == p->party_id) {
						if (packet_db[sd->packet_ver][RBUFW(buf,0)].len) { // packet must exist for the client version
							memcpy(WFIFOP(sd->fd,0), buf, len);
							WFIFOSET(sd->fd,len);
						}
					}
				}
			}
		}
		break;
	case SELF:
		if (sd && packet_db[sd->packet_ver][RBUFW(buf,0)].len) { // packet must exist for the client version
			memcpy(WFIFOP(sd->fd,0), buf, len);
			WFIFOSET(sd->fd,len);
		}
		break;

/* New definitions for guilds [Valaris]	*/

	case GUILD_AREA:
	case GUILD_AREA_WOS:
		x0 = bl->x - AREA_SIZE;
		y0 = bl->y - AREA_SIZE;
		x1 = bl->x + AREA_SIZE;
		y1 = bl->y + AREA_SIZE;
	case GUILD:
	case GUILD_WOS:
		if (sd) { // guildspy [Syrus22]
			if (sd->guildspy > 0) {
				g = guild_search(sd->guildspy);
			} else if (sd->status.guild_id > 0) {
				g = guild_search(sd->status.guild_id);
			}
		}
		if (g) {
			for(i = 0; i < g->max_member; i++) {
				if ((sd = g->member[i].sd) != NULL) {
                                        if (session[sd->fd] == NULL || sd->state.auth == 0 || session[sd->fd]->session_data == NULL)
                                                continue;
					if (type == GUILD_WOS && sd->bl.id == bl->id)
						continue;
					if (sd->packet_ver > MAX_PACKET_VER)
						continue;
					if (packet_db[sd->packet_ver][RBUFW(buf,0)].len) { // packet must exist for the client version
						memcpy(WFIFOP(sd->fd,0), buf, len);
						WFIFOSET(sd->fd,len);
					}
				}
			}
			for (i = 0; i < fd_max; i++){
				if (session[i] && (sd = (struct map_session_data*)session[i]->session_data) != NULL && sd->state.auth) {
					if (sd->guildspy == g->guild_id) {
						if (packet_db[sd->packet_ver][RBUFW(buf,0)].len) { // packet must exist for the client version
							memcpy(WFIFOP(sd->fd,0), buf, len);
							WFIFOSET(sd->fd,len);
						}
					}
				}
			}
		}
		break;
	case GUILD_SAMEMAP:
	case GUILD_SAMEMAP_WOS:
		if (sd && sd->status.guild_id > 0) {
			g = guild_search(sd->status.guild_id);
		}
		if (g) {
			for(i = 0; i < g->max_member; i++) {
				if ((sd = g->member[i].sd) != NULL) {
					if (sd->bl.id == bl->id && (type == GUILD_WOS ||
					    type == GUILD_SAMEMAP_WOS || type == GUILD_AREA_WOS))
						continue;
					if (type != GUILD && type != GUILD_WOS && bl->m != sd->bl.m) // �}�b�v�`�F�b�N
						continue;
					if ((type == GUILD_AREA || type == GUILD_AREA_WOS) &&
					    (sd->bl.x < x0 || sd->bl.y < y0 ||
					     sd->bl.x > x1 || sd->bl.y > y1))
						continue;
					if (packet_db[sd->packet_ver][RBUFW(buf,0)].len) { // packet must exist for the client version
						memcpy(WFIFOP(sd->fd,0), buf, len);
						WFIFOSET(sd->fd,len);
					}
				}
			}
		}
		break;
/*				End [Valaris]			*/

	default:
		if (battle_config.error_log)
			ShowMessage("clif_send �܂�����ĂȂ���[\n");
		return -1;
	}

	return 0;
}

//
// �p�P�b�g����đ��M
//
/*==========================================
 *
 *------------------------------------------
 */
int clif_authok(struct map_session_data *sd) {
	int fd;

	nullpo_retr(0, sd);

	fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd, 0) = 0x73;
	WFIFOL(fd, 2) = gettick();
	WFIFOPOS(fd, 6, sd->bl.x, sd->bl.y);
	WFIFOB(fd, 9) = 5;
	WFIFOB(fd,10) = 5;
	WFIFOSET(fd,packet_len_table[0x73]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_authfail_fd(int fd, unsigned long type)
{
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x81;
	WFIFOL(fd,2) = type;
	WFIFOSET(fd,packet_len_table[0x81]);

	session_SetWaitClose(fd, 5000);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_charselectok(unsigned long id)
{
	struct map_session_data *sd;
	int fd;

	if ((sd = map_id2sd(id)) == NULL)
		return 1;
	fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0xb3;
	WFIFOB(fd,2) = 1;
	WFIFOSET(fd,packet_len_table[0xb3]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dropflooritem(struct flooritem_data *fitem) {
	unsigned char buf[64];
	int view;

	nullpo_retr(0, fitem);

	if(fitem->item_data.nameid <= 0)
		return 0;

	WBUFW(buf, 0) = 0x9e;
	WBUFL(buf, 2) = fitem->bl.id;
	if ((view = itemdb_viewid(fitem->item_data.nameid)) > 0)
		WBUFW(buf, 6) = view;
	else
		WBUFW(buf, 6) = fitem->item_data.nameid;
	WBUFB(buf, 8) = fitem->item_data.identify;
	WBUFW(buf, 9) = fitem->bl.x;
	WBUFW(buf,11) = fitem->bl.y;
	WBUFB(buf,13) = (unsigned char)fitem->subx;
	WBUFB(buf,14) = (unsigned char)fitem->suby;
	WBUFW(buf,15) = fitem->item_data.amount;

	return clif_send(buf, packet_len_table[0x9e], &fitem->bl, AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearflooritem(struct flooritem_data *fitem, int fd)
{
	unsigned char buf[16];
	nullpo_retr(0, fitem);

	WBUFW(buf,0) = 0xa1;
	WBUFL(buf,2) = fitem->bl.id;

	if (fd == 0) {
		clif_send(buf, packet_len_table[0xa1], &fitem->bl, AREA);
	} else if( session_isActive(fd) ) {
		memcpy(WFIFOP(fd,0), buf, 6);
		WFIFOSET(fd,packet_len_table[0xa1]);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar(struct block_list *bl, unsigned char type)
{
	unsigned char buf[16];
	nullpo_retr(0, bl);

	WBUFW(buf,0) = 0x80;
	WBUFL(buf,2) = bl->id;
	if (type == 9) {
		WBUFB(buf,6) = 0;
		clif_send(buf, packet_len_table[0x80], bl, AREA);
	} else {
		WBUFB(buf,6) = type;
		clif_send(buf, packet_len_table[0x80], bl, type == 1 ? AREA : AREA_WOS);
	}

	return 0;
}


static int clif_clearchar_delay_sub(int tid, unsigned long tick, int id, int data) 
{
	struct block_list *bl = (struct block_list *)id;

	clif_clearchar(bl,data);
	map_freeblock(bl);

	return 0;
}

int clif_clearchar_delay(unsigned long tick, struct block_list *bl, int type) 
{
	struct block_list *tmpbl;

	tmpbl = (struct block_list*)aMalloc( sizeof(struct block_list) );
	memcpy(tmpbl, bl, sizeof(struct block_list));
	add_timer(tick, clif_clearchar_delay_sub, (int)tmpbl, type); // !!todo!!

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar_id(int fd, unsigned long id, unsigned char type)
{
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x80;
	WFIFOL(fd,2) = id;
	WFIFOB(fd,6) = type;
	WFIFOSET(fd, packet_len_table[0x80]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set0078(struct map_session_data *sd, unsigned char *buf) {
	unsigned int level;

	nullpo_retr(0, sd);

	if(sd->disguise_id > 23 && sd->disguise_id < 4001) { // mob disguises [Valaris]

		WBUFW(buf,0) = 0x78;
		WBUFL(buf,2) = sd->bl.id;
		WBUFW(buf,6) = status_get_speed(&sd->bl);
		WBUFW(buf,8) = sd->opt1;
		WBUFW(buf,10) = sd->opt2;
		WBUFW(buf,12) = sd->status.option;
		WBUFW(buf,14) = sd->disguise_id;
		WBUFW(buf,42) = 0;
		WBUFB(buf,44) = 0;
		WBUFPOS(buf, 46, sd->bl.x, sd->bl.y);
		WBUFB(buf,48) |= sd->dir & 0x0f;
		WBUFB(buf,49) = 5;
		WBUFB(buf,50) = 5;
		WBUFB(buf,51) = 0;
		WBUFW(buf,52) = ((level = status_get_lv(&sd->bl)) > battle_config.max_lv) ? battle_config.max_lv : level;

		return packet_len_table[0x78];
	}

#if PACKETVER < 4
	WBUFW(buf,0)= 0x78;
	WBUFL(buf,2)= sd->bl.id;
	WBUFW(buf,6)= sd->speed;
	WBUFW(buf,8)= sd->opt1;
	WBUFW(buf,10)= sd->opt2;
	WBUFW(buf,12)= sd->status.option;
	WBUFW(buf,14)= sd->view_class;
	WBUFW(buf,16)= sd->status.hair;
	if (sd->view_class != 22)
		WBUFW(buf,18) = sd->status.weapon;
	else
		WBUFW(buf,18)=0;
	WBUFW(buf,20)=sd->status.head_bottom;
	WBUFW(buf,22)=sd->status.shield;
	WBUFW(buf,24)=sd->status.head_top;
	WBUFW(buf,26)=sd->status.head_mid;
	WBUFW(buf,28)=sd->status.hair_color;
	WBUFW(buf,30)=sd->status.clothes_color;
	WBUFW(buf,32)=sd->head_dir;
	WBUFL(buf,34)=sd->status.guild_id;
	WBUFL(buf,38)=sd->guild_emblem_id;
	WBUFW(buf,42)=sd->status.manner;
	WBUFB(buf,44)=sd->status.karma;
	WBUFB(buf,45)=sd->status.sex;
	WBUFPOS(buf,46,sd->bl.x,sd->bl.y);
	WBUFB(buf,48)|=sd->dir&0x0f;
	WBUFB(buf,49)=5;
	WBUFB(buf,50)=5;
	WBUFB(buf,51)= sd->state.dead_sit;
	WBUFW(buf,52)=(sd->status.base_level>battle_config.max_lv)?battle_config.max_lv:sd->status.base_level;

	return packet_len_table[0x78];
#else
	WBUFW(buf,0) = 0x1d8;
	WBUFL(buf,2) = sd->bl.id;
	WBUFW(buf,6) = sd->speed;
	WBUFW(buf,8) = sd->opt1;
	WBUFW(buf,10) = sd->opt2;
	WBUFW(buf,12) = sd->status.option;
	WBUFW(buf,14) = sd->view_class;
	WBUFW(buf,16) = sd->status.hair;
	if (sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]] && sd->view_class != 22) {
		if (sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,18) = sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,18) = sd->status.inventory[sd->equip_index[9]].nameid;
	} else
		WBUFW(buf,18) = 0;
	if (sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9] && sd->inventory_data[sd->equip_index[8]] && sd->view_class != 22) {
		if (sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,20) = sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,20) = sd->status.inventory[sd->equip_index[8]].nameid;
	} else
		WBUFW(buf,20) = 0;
	WBUFW(buf,22) = sd->status.head_bottom;
	WBUFW(buf,24) = sd->status.head_top;
	WBUFW(buf,26) = sd->status.head_mid;
	WBUFW(buf,28) = sd->status.hair_color;
	WBUFW(buf,30) = sd->status.clothes_color;
	WBUFW(buf,32) = sd->head_dir;
	WBUFL(buf,34) = sd->status.guild_id;
	WBUFW(buf,38) = sd->guild_emblem_id;
	WBUFW(buf,40) = sd->status.manner;
	WBUFW(buf,42)=sd->opt3;
	WBUFB(buf,44) = sd->status.karma;
	WBUFB(buf,45) = sd->status.sex;
	WBUFPOS(buf, 46, sd->bl.x, sd->bl.y);
	WBUFB(buf,48) |= sd->dir & 0x0f;
	WBUFB(buf,49) = 5;
	WBUFB(buf,50) = 5;
	WBUFB(buf,51) = sd->state.dead_sit;
	WBUFW(buf,52) = ((level = status_get_lv(&sd->bl)) > battle_config.max_lv) ? battle_config.max_lv : level;

	return packet_len_table[0x1d8];
#endif
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set007b(struct map_session_data *sd,unsigned char *buf) {
	unsigned int level;

	nullpo_retr(0, sd);

	if(sd->disguise_id > 23 && sd->disguise_id < 4001) { // mob disguises [Valaris]

		WBUFW(buf,0)=0x7b;
		WBUFL(buf,2)=sd->bl.id;
		WBUFW(buf,6)=status_get_speed(&sd->bl);
		WBUFW(buf,8)=sd->opt1;
		WBUFW(buf,10)=sd->opt2;
		WBUFW(buf,12)=sd->status.option;
		WBUFW(buf,14)=sd->disguise_id;
		WBUFL(buf,22)=gettick();
		WBUFW(buf,46)=0;
		WBUFB(buf,48)=0;
		WBUFPOS2(buf,50,sd->bl.x,sd->bl.y,sd->to_x,sd->to_y);
		WBUFB(buf,55)=0;
		WBUFB(buf,56)=5;
		WBUFB(buf,57)=5;
		WBUFW(buf,58)=((level = status_get_lv(&sd->bl))>battle_config.max_lv)? battle_config.max_lv:level;

		return packet_len_table[0x7b];
	}

#if PACKETVER < 4
	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->opt1;
	WBUFW(buf,10)=sd->opt2;
	WBUFW(buf,12)=sd->status.option;
	WBUFW(buf,14)=sd->view_class;
	WBUFW(buf,16)=sd->status.hair;
	if(sd->view_class != 22)
		WBUFW(buf,18)=sd->status.weapon;
	else
		WBUFW(buf,18)=0;
	WBUFW(buf,20)=sd->status.head_bottom;
	WBUFL(buf,22)=gettick();
	WBUFW(buf,26)=sd->status.shield;
	WBUFW(buf,28)=sd->status.head_top;
	WBUFW(buf,30)=sd->status.head_mid;
	WBUFW(buf,32)=sd->status.hair_color;
	WBUFW(buf,34)=sd->status.clothes_color;
	WBUFW(buf,36)=sd->head_dir;
	WBUFL(buf,38)=sd->status.guild_id;
	WBUFL(buf,42)=sd->guild_emblem_id;
	WBUFW(buf,46)=sd->status.manner;
	WBUFB(buf,48)=sd->status.karma;
	WBUFB(buf,49)=sd->status.sex;
	WBUFPOS2(buf,50,sd->bl.x,sd->bl.y,sd->to_x,sd->to_y);
	WBUFB(buf,55)=0;
	WBUFB(buf,56)=5;
	WBUFB(buf,57)=5;
	WBUFW(buf,58)=(sd->status.base_level>battle_config.max_lv)?battle_config.max_lv:sd->status.base_level;

	return packet_len_table[0x7b];
#else
	WBUFW(buf,0)=0x1da;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->opt1;
	WBUFW(buf,10)=sd->opt2;
	WBUFW(buf,12)=sd->status.option;
	WBUFW(buf,14)=sd->view_class;
	WBUFW(buf,16)=sd->status.hair;
	if(sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]] && sd->view_class != 22) {
		if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,18)=sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,18)=sd->status.inventory[sd->equip_index[9]].nameid;
	}
	else
		WBUFW(buf,18)=0;
	if(sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9] && sd->inventory_data[sd->equip_index[8]] && sd->view_class != 22) {
		if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,20)=sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,20)=sd->status.inventory[sd->equip_index[8]].nameid;
	}
	else
		WBUFW(buf,20)=0;
	WBUFW(buf,22)=sd->status.head_bottom;
	WBUFL(buf,24)=gettick();
	WBUFW(buf,28)=sd->status.head_top;
	WBUFW(buf,30)=sd->status.head_mid;
	WBUFW(buf,32)=sd->status.hair_color;
	WBUFW(buf,34)=sd->status.clothes_color;
	WBUFW(buf,36)=sd->head_dir;
	WBUFL(buf,38)=sd->status.guild_id;
	WBUFW(buf,42)=sd->guild_emblem_id;
	WBUFW(buf,44)=sd->status.manner;
	WBUFW(buf,46)=sd->opt3;
	WBUFB(buf,48)=sd->status.karma;
	WBUFB(buf,49)=sd->status.sex;
	WBUFPOS2(buf,50,sd->bl.x,sd->bl.y,sd->to_x,sd->to_y);
	WBUFB(buf,55)=0;
	WBUFB(buf,56)=5;
	WBUFB(buf,57)=5;
	WBUFW(buf,58)=(sd->status.base_level>battle_config.max_lv)?battle_config.max_lv:sd->status.base_level;

	return packet_len_table[0x1da];
#endif
}

/*==========================================
 * �N���X�`�F���W type��Mob�̏ꍇ��1�ő���0�H
 *------------------------------------------
 */
int clif_class_change(struct block_list *bl,unsigned short class_,unsigned char type)
{
	unsigned char buf[16];

	nullpo_retr(0, bl);

	if(class_ >= MAX_PC_CLASS) {
		WBUFW(buf,0)=0x1b0;
		WBUFL(buf,2)=bl->id;
		WBUFB(buf,6)=type;
		WBUFL(buf,7)=class_;

		clif_send(buf,packet_len_table[0x1b0],bl,AREA);
	}
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int clif_mob_class_change(struct mob_data *md, unsigned short class_) {
	unsigned char buf[16];
	int view = mob_get_viewclass(class_);

	nullpo_retr(0, md);

	if(view >= MAX_PC_CLASS) {
		WBUFW(buf,0)=0x1b0;
		WBUFL(buf,2)=md->bl.id;
		WBUFB(buf,6)=1;
		WBUFL(buf,7)=view;

		clif_send(buf,packet_len_table[0x1b0],&md->bl,AREA);
	}
	return 0;
}
// mob equipment [Valaris]

int clif_mob_equip(struct mob_data *md, unsigned short nameid) {
	unsigned char buf[16];

	nullpo_retr(0, md);

	WBUFW(buf,0)=0x1a4;
	WBUFB(buf,2)=3;
	WBUFL(buf,3)=md->bl.id;
	WBUFL(buf,7)=nameid;

	clif_send(buf,packet_len_table[0x1a4],&md->bl,AREA);

	return 0;
}

/*==========================================
 * MOB�\��1
 *------------------------------------------
 */
int clif_mob0078(struct mob_data *md, unsigned char *buf)
{
	unsigned int level, i;

	memset(buf,0,packet_len_table[0x78]);

	nullpo_retr(0, md);

	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=md->bl.id;
	WBUFW(buf,6)=status_get_speed(&md->bl);
	WBUFW(buf,8)=md->opt1;
	WBUFW(buf,10)=md->opt2;
	WBUFW(buf,12)=md->option;
	WBUFW(buf,14)=mob_get_viewclass(md->class_);
	if((i=mob_get_viewclass(md->class_)) <= 23 || i == 812 || i >= 4001) {
		WBUFW(buf,12)= WBUFW(buf,12) | mob_db[md->class_].option;
		WBUFW(buf,16)=mob_get_hair(md->class_);
		WBUFW(buf,18)=mob_get_weapon(md->class_);
		WBUFW(buf,20)=mob_get_head_buttom(md->class_);
		WBUFW(buf,22)=mob_get_shield(md->class_);
		WBUFW(buf,24)=mob_get_head_top(md->class_);
		WBUFW(buf,26)=mob_get_head_mid(md->class_);
		WBUFW(buf,28)=mob_get_hair_color(md->class_);
		WBUFW(buf,30)=mob_get_clothes_color(md->class_);	//Add for player monster dye - Valaris
		WBUFB(buf,45)=mob_get_sex(md->class_);
	}

	if (md->class_ >= 1285 && md->class_ <= 1287 && md->guild_id) {	// Added guardian emblems [Valaris]
		struct guild *g;
		struct guild_castle *gc=guild_mapname2gc(map[md->bl.m].mapname);
		if (gc && gc->guild_id > 0) {
			g=guild_search(gc->guild_id);
			if (g) {
				WBUFL(buf,22)=g->emblem_id;
				WBUFL(buf,26)=gc->guild_id;
			}
		}
	}	// End addition

	WBUFPOS(buf,46,md->bl.x,md->bl.y);
	WBUFB(buf,48)|=md->dir&0x0f;
	WBUFB(buf,49)=5;
	WBUFB(buf,50)=5;
	WBUFW(buf,52)=((level = status_get_lv(&md->bl))>battle_config.max_lv)? battle_config.max_lv:level;

	return packet_len_table[0x78];
}

/*==========================================
 * MOB�\��2
 *------------------------------------------
 */
int clif_mob007b(struct mob_data *md, unsigned char *buf) {
	unsigned int level;

	memset(buf,0,packet_len_table[0x7b]);

	nullpo_retr(0, md);

	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=md->bl.id;
	WBUFW(buf,6)=status_get_speed(&md->bl);
	WBUFW(buf,8)=md->opt1;
	WBUFW(buf,10)=md->opt2;
	WBUFW(buf,12)=md->option;
	WBUFW(buf,14)=mob_get_viewclass(md->class_);
	if ((mob_get_viewclass(md->class_) < 24) || (mob_get_viewclass(md->class_) > 4000)) {
		WBUFW(buf,12)= WBUFW(buf,12) | mob_db[md->class_].option;
		WBUFW(buf,16)=mob_get_hair(md->class_);
		WBUFW(buf,18)=mob_get_weapon(md->class_);
		WBUFW(buf,20)=mob_get_head_buttom(md->class_);
		WBUFL(buf,22)=gettick();
		WBUFW(buf,26)=mob_get_shield(md->class_);
		WBUFW(buf,28)=mob_get_head_top(md->class_);
		WBUFW(buf,30)=mob_get_head_mid(md->class_);
		WBUFW(buf,32)=mob_get_hair_color(md->class_);
		WBUFW(buf,34)=mob_get_clothes_color(md->class_);	//Add for player monster dye - Valaris
		WBUFB(buf,49)=mob_get_sex(md->class_);
	} else
		WBUFL(buf,22)=gettick();

		if(md->class_ >= 1285 && md->class_ <= 1287 && md->guild_id)	{	// Added guardian emblems [Valaris]
			struct guild *g;
			struct guild_castle *gc=guild_mapname2gc(map[md->bl.m].mapname);
			if(gc && gc->guild_id > 0){
				g=guild_search(gc->guild_id);
					if(g) {
					WBUFL(buf,28)=gc->guild_id;
					WBUFL(buf,24)=g->emblem_id;
					}
			}
		}									// End addition

	WBUFPOS2(buf,50,md->bl.x,md->bl.y,md->to_x,md->to_y);
	WBUFB(buf,56)=5;
	WBUFB(buf,57)=5;
	WBUFW(buf,58)=((level = status_get_lv(&md->bl))>battle_config.max_lv)? battle_config.max_lv:level;

	return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_npc0078(struct npc_data *nd, unsigned char *buf) {
	struct guild *g;

	nullpo_retr(0, nd);

	memset(buf,0,packet_len_table[0x78]);

	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=nd->bl.id;
	WBUFW(buf,6)=nd->speed;
	WBUFW(buf,14)=nd->class_;
	if ((nd->class_ == 722) && (nd->u.scr.guild_id > 0) && ((g=guild_search(nd->u.scr.guild_id)) != NULL)) {
		WBUFL(buf,22)=g->emblem_id;
		WBUFL(buf,26)=g->guild_id;
	}
	WBUFPOS(buf,46,nd->bl.x,nd->bl.y);
	WBUFB(buf,48)|=nd->dir&0x0f;
	WBUFB(buf,49)=5;
	WBUFB(buf,50)=5;

	return packet_len_table[0x78];
}

// NPC Walking [Valaris]
int clif_npc007b(struct npc_data *nd, unsigned char *buf) {
	struct guild *g;

	nullpo_retr(0, nd);

	memset(buf,0,packet_len_table[0x7b]);

	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=nd->bl.id;
	WBUFW(buf,6)=nd->speed;
	WBUFW(buf,14)=nd->class_;
	if ((nd->class_ == 722) && (nd->u.scr.guild_id > 0) && ((g=guild_search(nd->u.scr.guild_id)) != NULL)) {
		WBUFL(buf,22)=g->emblem_id;
		WBUFL(buf,26)=g->guild_id;
	}

	WBUFL(buf,22)=gettick();
	WBUFPOS2(buf,50,nd->bl.x,nd->bl.y,nd->to_x,nd->to_y);
	WBUFB(buf,56)=5;
	WBUFB(buf,57)=5;

	return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pet0078(struct pet_data *pd, unsigned char *buf) {
	unsigned int view,level;

	nullpo_retr(0, pd);

	memset(buf,0,packet_len_table[0x78]);

	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=pd->bl.id;
	WBUFW(buf,6)=pd->speed;
	WBUFW(buf,14)=mob_get_viewclass(pd->class_);
	if((mob_get_viewclass(pd->class_) < 24) || (mob_get_viewclass(pd->class_) > 4000)) {
		WBUFW(buf,12)=mob_db[pd->class_].option;
		WBUFW(buf,16)=mob_get_hair(pd->class_);
		WBUFW(buf,18)=mob_get_weapon(pd->class_);
		WBUFW(buf,20)=mob_get_head_buttom(pd->class_);
		WBUFW(buf,22)=mob_get_shield(pd->class_);
		WBUFW(buf,24)=mob_get_head_top(pd->class_);
		WBUFW(buf,26)=mob_get_head_mid(pd->class_);
		WBUFW(buf,28)=mob_get_hair_color(pd->class_);
		WBUFW(buf,30)=mob_get_clothes_color(pd->class_);	//Add for player pet dye - Valaris
		WBUFB(buf,45)=mob_get_sex(pd->class_);
	} else {
		WBUFW(buf,16)=0x14;
		if((view = itemdb_viewid(pd->equip_id)) > 0)
			WBUFW(buf,20)=view;
		else
			WBUFW(buf,20)=pd->equip_id;
	}
	WBUFPOS(buf,46,pd->bl.x,pd->bl.y);
	WBUFB(buf,48)|=pd->dir&0x0f;
	WBUFB(buf,49)=0;
	WBUFB(buf,50)=0;
	WBUFW(buf,52)=((level = status_get_lv(&pd->bl))>battle_config.max_lv)? battle_config.max_lv:level;

	return packet_len_table[0x78];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pet007b(struct pet_data *pd, unsigned char *buf) {
	unsigned int view,level;

	nullpo_retr(0, pd);

	memset(buf,0,packet_len_table[0x7b]);

	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=pd->bl.id;
	WBUFW(buf,6)=pd->speed;
	WBUFW(buf,14)=mob_get_viewclass(pd->class_);
	if((mob_get_viewclass(pd->class_) < 24) || (mob_get_viewclass(pd->class_) > 4000)) {
		WBUFW(buf,12)=mob_db[pd->class_].option;
		WBUFW(buf,16)=mob_get_hair(pd->class_);
		WBUFW(buf,18)=mob_get_weapon(pd->class_);
		WBUFW(buf,20)=mob_get_head_buttom(pd->class_);
		WBUFL(buf,22)=gettick();
		WBUFW(buf,26)=mob_get_shield(pd->class_);
		WBUFW(buf,28)=mob_get_head_top(pd->class_);
		WBUFW(buf,30)=mob_get_head_mid(pd->class_);
		WBUFW(buf,32)=mob_get_hair_color(pd->class_);
		WBUFW(buf,34)=mob_get_clothes_color(pd->class_);	//Add for player pet dye - Valaris
		WBUFB(buf,49)=mob_get_sex(pd->class_);
	} else {
		WBUFW(buf,16)=0x14;
		if((view = itemdb_viewid(pd->equip_id)) > 0)
			WBUFW(buf,20)=view;
		else
			WBUFW(buf,20)=pd->equip_id;
		WBUFL(buf,22)=gettick();
	}
	WBUFPOS2(buf,50,pd->bl.x,pd->bl.y,pd->to_x,pd->to_y);
	WBUFB(buf,56)=0;
	WBUFB(buf,57)=0;
	WBUFW(buf,58)=((level = status_get_lv(&pd->bl))>battle_config.max_lv)? battle_config.max_lv:level;

	return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_set01e1(struct map_session_data *sd, unsigned char *buf) {
	nullpo_retr(0, sd);

	WBUFW(buf,0)=0x1e1;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->spiritball;

	return packet_len_table[0x1e1];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_set0192(int fd, int m, int x, int y, int type) {

	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x192;
	WFIFOW(fd,2) = x;
	WFIFOW(fd,4) = y;
	WFIFOW(fd,6) = type;
	memcpy(WFIFOP(fd,8),map[m].mapname,16);
	WFIFOSET(fd,packet_len_table[0x192]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnpc(struct map_session_data *sd) {
	unsigned char buf[128];

	nullpo_retr(0, sd);

	if(sd->disguise_id > 23 && sd->disguise_id < 4001) { // mob disguises [Valaris]
		clif_clearchar(&sd->bl, 9);

		memset(buf, 0, packet_len_table[0x119]);

		WBUFW(buf, 0) = 0x119;
		WBUFL(buf, 2) = sd->bl.id;
		WBUFW(buf, 6) = 0;
		WBUFW(buf, 8) = 0;
		WBUFW(buf,10) = 0x40;
		WBUFB(buf,12) = 0;

		clif_send(buf, packet_len_table[0x119], &sd->bl, SELF);

		memset(buf, 0, packet_len_table[0x7c]);

		WBUFW(buf, 0) = 0x7c;
		WBUFL(buf, 2) = sd->bl.id;
		WBUFW(buf, 6) = sd->speed;
		WBUFW(buf, 8) = sd->opt1;
		WBUFW(buf,10) = sd->opt2;
		WBUFW(buf,12) = sd->status.option;
		WBUFW(buf,20) = sd->disguise_id;
		WBUFPOS(buf, 36, sd->bl.x, sd->bl.y);
		clif_send(buf, packet_len_table[0x7c], &sd->bl, AREA);
	}

	clif_set0078(sd, buf);

#if PACKETVER < 4
	WBUFW(buf, 0) = 0x79;
	WBUFW(buf,51) = (sd->status.base_level > battle_config.max_lv) ? battle_config.max_lv : sd->status.base_level;
	clif_send(buf, packet_len_table[0x79], &sd->bl, AREA_WOS);
#else
	WBUFW(buf, 0) = 0x1d9;
	WBUFW(buf,51) = (sd->status.base_level > battle_config.max_lv) ? battle_config.max_lv : sd->status.base_level;
	clif_send(buf, packet_len_table[0x1d9], &sd->bl, AREA_WOS);
#endif


	if (sd->spiritball > 0)
		clif_spiritball(sd);

	if (sd->status.guild_id > 0) { // force display of guild emblem [Valaris]
		struct guild *g = guild_search(sd->status.guild_id);
		if (g)
			clif_guild_emblem(sd,g);
	}	// end addition [Valaris]

	if (sd->status.class_==13 || sd->status.class_==21 || sd->status.class_==4014 || sd->status.class_==4022)
		pc_setoption(sd,sd->status.option|0x0020); // [Valaris]

	if ((pc_isriding(*sd) && pc_checkskill(*sd,KN_RIDING)>0) && (sd->status.class_==7 ||
	    sd->status.class_==14 || sd->status.class_==4008 || sd->status.class_==4015))
		pc_setriding(sd); // update peco riders for people upgrading athena [Valaris]


	if (map[sd->bl.m].flag.snow)
		clif_specialeffect(&sd->bl, 162, 1);
	if (map[sd->bl.m].flag.clouds)
		clif_specialeffect(&sd->bl, 233, 1);
	if (map[sd->bl.m].flag.fog)
		clif_specialeffect(&sd->bl, 515, 1);
	if (map[sd->bl.m].flag.fireworks) {
		clif_specialeffect(&sd->bl, 297, 1);
		clif_specialeffect(&sd->bl, 299, 1);
		clif_specialeffect(&sd->bl, 301, 1);
	}
	if (map[sd->bl.m].flag.sakura)
		clif_specialeffect(&sd->bl, 163, 1);
	if (map[sd->bl.m].flag.leaves)
		clif_specialeffect(&sd->bl, 333, 1);
	if (map[sd->bl.m].flag.rain)
		clif_specialeffect(&sd->bl, 161, 1);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnnpc(struct npc_data *nd)
{
	unsigned char buf[64];
	int len;

	nullpo_retr(0, nd);

	if(nd->class_ < 0 || nd->flag&1 || nd->class_ == INVISIBLE_CLASS)
		return 0;

	memset(buf,0,packet_len_table[0x7c]);

	WBUFW(buf,0)=0x7c;
	WBUFL(buf,2)=nd->bl.id;
	WBUFW(buf,6)=nd->speed;
	WBUFW(buf,20)=nd->class_;
	WBUFPOS(buf,36,nd->bl.x,nd->bl.y);

	clif_send(buf,packet_len_table[0x7c],&nd->bl,AREA);

	len = clif_npc0078(nd,buf);
	clif_send(buf,len,&nd->bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnmob(struct mob_data *md)
{
	unsigned char buf[64];
	int len;

	nullpo_retr(0, md);

	if (mob_get_viewclass(md->class_) > 23) {
		memset(buf,0,packet_len_table[0x7c]);

		WBUFW(buf,0)=0x7c;
		WBUFL(buf,2)=md->bl.id;
		WBUFW(buf,6)=md->speed;
		WBUFW(buf,8)=md->opt1;
		WBUFW(buf,10)=md->opt2;
		WBUFW(buf,12)=md->option;
		WBUFW(buf,20)=mob_get_viewclass(md->class_);
		WBUFPOS(buf,36,md->bl.x,md->bl.y);
		clif_send(buf,packet_len_table[0x7c],&md->bl,AREA);
	}

	len = clif_mob0078(md,buf);
	clif_send(buf,len,&md->bl,AREA);

	if (mob_get_equip(md->class_) > 0) // mob equipment [Valaris]
		clif_mob_equip(md,mob_get_equip(md->class_));

	if(md->size==2) // tiny/big mobs [Valaris]
		clif_specialeffect(&md->bl,423,0);
	else if(md->size==1)
		clif_specialeffect(&md->bl,421,0);

	return 0;
}

// pet

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnpet(struct pet_data *pd)
{
	unsigned char buf[64];
	int len;

	nullpo_retr(0, pd);

	if (mob_get_viewclass(pd->class_) >= MAX_PC_CLASS) {
		memset(buf,0,packet_len_table[0x7c]);

		WBUFW(buf,0)=0x7c;
		WBUFL(buf,2)=pd->bl.id;
		WBUFW(buf,6)=pd->speed;
		WBUFW(buf,20)=mob_get_viewclass(pd->class_);
		WBUFPOS(buf,36,pd->bl.x,pd->bl.y);

		clif_send(buf,packet_len_table[0x7c],&pd->bl,AREA);
	}

	len = clif_pet0078(pd,buf);
	clif_send(buf,len,&pd->bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movepet(struct pet_data *pd) {
	unsigned char buf[256];
	int len;

	nullpo_retr(0, pd);

	len = clif_pet007b(pd,buf);
	clif_send(buf,len,&pd->bl,AREA);

	return 0;
}

/*==========================================
 * npc walking [Valaris]
 *------------------------------------------
 */
int clif_movenpc(struct npc_data *nd) {
	unsigned char buf[256];
	int len;

	nullpo_retr(0, nd);

	len = clif_npc007b(nd,buf);
	clif_send(buf,len,&nd->bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_servertick(struct map_session_data *sd)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x7f;
	WFIFOL(fd,2)=sd->server_tick;
	WFIFOSET(fd,packet_len_table[0x7f]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_walkok(struct map_session_data *sd)
{
	int fd;

	nullpo_retr(0, sd);
	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x87;
	WFIFOL(fd,2)=gettick();;
	WFIFOPOS2(fd,6,sd->bl.x,sd->bl.y,sd->to_x,sd->to_y);
	WFIFOB(fd,11)=0;
	WFIFOSET(fd,packet_len_table[0x87]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movechar(struct map_session_data *sd) {
	int len;
	unsigned char buf[256];

	nullpo_retr(0, sd);

	len = clif_set007b(sd, buf);

	if(sd->disguise_id > 23 && sd->disguise_id < 4001) {
		clif_send(buf, len, &sd->bl, AREA);
		return 0;
	} else
		clif_send(buf, len, &sd->bl, AREA_WOS);

	if(battle_config.save_clothcolor &&
		sd->status.clothes_color > 0 &&
		(sd->view_class != 22 || !battle_config.wedding_ignorepalette)
		)
		clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemap(struct map_session_data *sd, char *mapname, unsigned short x, unsigned short y)
{
	int fd;
	nullpo_retr(0, sd);
	fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x91;
	memcpy(WFIFOP(fd,2), mapname, 16);
	WFIFOW(fd,18) = x;
	WFIFOW(fd,20) = y;
	WFIFOSET(fd, packet_len_table[0x91]);

	if(sd->disguise_id > 23 && sd->disguise_id < 4001) // mob disguises [Valaris]
		clif_spawnpc(sd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapserver(struct map_session_data *sd, char *mapname, unsigned short x, unsigned short y, unsigned long ip, unsigned short port)
{
	int fd;
	nullpo_retr(0, sd);

	fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x92;
	memcpy(WFIFOP(fd,2), mapname, 16);
	WFIFOW(fd,18) = x;
	WFIFOW(fd,20) = y;
	WFIFOLIP(fd,22) = ip;
	WFIFOW(fd,26) = port;
	WFIFOSET(fd, packet_len_table[0x92]);

	session_SetWaitClose(fd, 1000);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_fixpos(struct block_list *bl) {
	unsigned char buf[16];

	nullpo_retr(0, bl);

	WBUFW(buf,0)=0x88;
	WBUFL(buf,2)=bl->id;
	WBUFW(buf,6)=bl->x;
	WBUFW(buf,8)=bl->y;

	return clif_send(buf, packet_len_table[0x88], bl, AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_npcbuysell(struct map_session_data* sd, unsigned long id)
{
	int fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xc4;
	WFIFOL(fd,2)=id;
	WFIFOSET(fd,packet_len_table[0xc4]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_buylist(struct map_session_data *sd, struct npc_data *nd) {
	struct item_data *id;
	int fd,i,val;

	nullpo_retr(0, sd);
	nullpo_retr(0, nd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0xc6;
	for(i=0;nd->u.shop_item[i].nameid > 0;i++){
		id = itemdb_search(nd->u.shop_item[i].nameid);
		val=nd->u.shop_item[i].value;
		WFIFOL(fd,4+i*11)=val;
		if (!id->flag.value_notdc)
			val=pc_modifybuyvalue(sd,val);
		WFIFOL(fd,8+i*11)=val;
		WFIFOB(fd,12+i*11)=id->type;
		if (id->view_id > 0)
			WFIFOW(fd,13+i*11)=id->view_id;
		else
			WFIFOW(fd,13+i*11)=nd->u.shop_item[i].nameid;
	}
	WFIFOW(fd,2)=i*11+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_selllist(struct map_session_data *sd) {
	int fd,i,c=0,val;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0xc7;
	for(i=0;i<MAX_INVENTORY;i++) {
		if(sd->status.inventory[i].nameid > 0 && sd->inventory_data[i]) {
			val=sd->inventory_data[i]->value_sell;
			if (val < 0)
				continue;
			WFIFOW(fd,4+c*10)=i+2;
			WFIFOL(fd,6+c*10)=val;
			if (!sd->inventory_data[i]->flag.value_notoc)
				val=pc_modifysellvalue(sd,val);
			WFIFOL(fd,10+c*10)=val;
			c++;
		}
	}
	WFIFOW(fd,2)=c*10+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmes(struct map_session_data *sd, unsigned long npcid, const char *mes)
{
	int fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) || !mes)
		return 0;

	size_t len = (mes) ? 1+strlen(mes) : 0;
	WFIFOW(fd,0)=0xb4;
	WFIFOW(fd,2)=len+8;
	WFIFOL(fd,4)=npcid;
	memcpy(WFIFOP(fd,8),mes,len);
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptnext(struct map_session_data *sd,unsigned long npcid)
{
	int fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xb5;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0xb5]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptclose(struct map_session_data *sd, unsigned long npcid)
{
	int fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xb6;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0xb6]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmenu(struct map_session_data *sd, unsigned long npcid, const char *mes)
{
	int fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	size_t len = (mes) ? 1+strlen(mes) : 0;

	WFIFOW(fd,0)=0xb7;
	WFIFOW(fd,2)=len+8;
	WFIFOL(fd,4)=npcid;
	memcpy(WFIFOP(fd,8),mes,len);
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptinput(struct map_session_data *sd, unsigned long npcid)
{
	int fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0x142;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0x142]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptinputstr(struct map_session_data *sd, unsigned long npcid)
{
	int fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0x1d4;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0x1d4]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_viewpoint(struct map_session_data *sd, unsigned long npc_id, unsigned long id, unsigned long x, unsigned long y, unsigned char type, unsigned long color)
{
	int fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0x144;
	WFIFOL(fd,2)=npc_id;
	WFIFOL(fd,6)=id;
	WFIFOL(fd,10)=x;
	WFIFOL(fd,14)=y;
	WFIFOB(fd,18)=type;
	WFIFOL(fd,19)=color;
	WFIFOSET(fd,packet_len_table[0x144]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_cutin(struct map_session_data *sd, const char *image, unsigned char type)
{
	int fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0x1b3;
	memcpy(WFIFOP(fd,2),image,64);
	WFIFOB(fd,66)=type;
	WFIFOSET(fd,packet_len_table[0x1b3]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_additem(struct map_session_data *sd, unsigned short n, unsigned short amount, unsigned char fail)
{
	int fd, j;
	unsigned char *buf;
	nullpo_retr(0, sd);

	fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;

	buf = WFIFOP(fd,0);
	if(fail) {
		WBUFW(buf,0)=0xa0;
		WBUFW(buf,2)=n+2;
		WBUFW(buf,4)=amount;
		WBUFW(buf,6)=0;
		WBUFB(buf,8)=0;
		WBUFB(buf,9)=0;
		WBUFB(buf,10)=0;
		WBUFW(buf,11)=0;
		WBUFW(buf,13)=0;
		WBUFW(buf,15)=0;
		WBUFW(buf,17)=0;
		WBUFW(buf,19)=0;
		WBUFB(buf,21)=0;
		WBUFB(buf,22)=fail;
	} else {
		if( n>=MAX_INVENTORY || sd->status.inventory[n].nameid <=0 || sd->inventory_data[n] == NULL)
			return 1;

		WBUFW(buf,0)=0xa0;
		WBUFW(buf,2)=n+2;
		WBUFW(buf,4)=amount;
		if (sd->inventory_data[n]->view_id > 0)
			WBUFW(buf,6)=sd->inventory_data[n]->view_id;
		else
			WBUFW(buf,6)=sd->status.inventory[n].nameid;
		WBUFB(buf,8)=sd->status.inventory[n].identify;
		WBUFB(buf,9)=sd->status.inventory[n].attribute;
		WBUFB(buf,10)=sd->status.inventory[n].refine;
		if(sd->status.inventory[n].card[0]==0x00ff || sd->status.inventory[n].card[0]==0x00fe || sd->status.inventory[n].card[0]==0xff00) {
			WBUFW(buf,11)=sd->status.inventory[n].card[0];
			WBUFW(buf,13)=sd->status.inventory[n].card[1];
			WBUFW(buf,15)=sd->status.inventory[n].card[2];
			WBUFW(buf,17)=sd->status.inventory[n].card[3];
		} else {
			if (sd->status.inventory[n].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[0])) > 0)
				WBUFW(buf,11)=j;
			else
				WBUFW(buf,11)=sd->status.inventory[n].card[0];
			if (sd->status.inventory[n].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[1])) > 0)
				WBUFW(buf,13)=j;
			else
				WBUFW(buf,13)=sd->status.inventory[n].card[1];
			if (sd->status.inventory[n].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[2])) > 0)
				WBUFW(buf,15)=j;
			else
				WBUFW(buf,15)=sd->status.inventory[n].card[2];
			if (sd->status.inventory[n].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[3])) > 0)
				WBUFW(buf,17)=j;
			else
				WBUFW(buf,17)=sd->status.inventory[n].card[3];
		}
		WBUFW(buf,19)=pc_equippoint(*sd,n);
		WBUFB(buf,21)=(sd->inventory_data[n]->type == 7)? 4:sd->inventory_data[n]->type;
		WBUFB(buf,22)=fail;
	}

	WFIFOSET(fd,packet_len_table[0xa0]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_delitem(struct map_session_data *sd,unsigned short n,unsigned short amount)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xaf;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=amount;

	WFIFOSET(fd,packet_len_table[0xaf]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_itemlist(struct map_session_data *sd)
{
	int i,n,fd,arrow=-1;
	unsigned char *buf;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf = WFIFOP(fd,0);
#if PACKETVER < 5
	WBUFW(buf,0)=0xa3;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if (sd->status.inventory[i].nameid <=0 || sd->inventory_data[i] == NULL || itemdb_isequip2(*sd->inventory_data[i]))
			continue;
		WBUFW(buf,n*10+4)=i+2;
		if (sd->inventory_data[i]->view_id > 0)
			WBUFW(buf,n*10+6)=sd->inventory_data[i]->view_id;
		else
			WBUFW(buf,n*10+6)=sd->status.inventory[i].nameid;
		WBUFB(buf,n*10+8)=sd->inventory_data[i]->type;
		WBUFB(buf,n*10+9)=sd->status.inventory[i].identify;
		WBUFW(buf,n*10+10)=sd->status.inventory[i].amount;
		if (sd->inventory_data[i]->equip == 0x8000) {
			WBUFW(buf,n*10+12)=0x8000;
			if (sd->status.inventory[i].equip)
				arrow=i;	// ���łɖ���`�F�b�N
		} else
			WBUFW(buf,n*10+12)=0;
		n++;
	}
	if (n) {
		WBUFW(buf,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#else
	WBUFW(buf,0)=0x1ee;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid <=0 || sd->inventory_data[i] == NULL || itemdb_isequip2(*sd->inventory_data[i]))
			continue;
		WBUFW(buf,n*18+4)=i+2;
		if(sd->inventory_data[i]->view_id > 0)
			WBUFW(buf,n*18+6)=sd->inventory_data[i]->view_id;
		else
			WBUFW(buf,n*18+6)=sd->status.inventory[i].nameid;
		WBUFB(buf,n*18+8)=sd->inventory_data[i]->type;
		WBUFB(buf,n*18+9)=sd->status.inventory[i].identify;
		WBUFW(buf,n*18+10)=sd->status.inventory[i].amount;
		if (sd->inventory_data[i]->equip == 0x8000) {
			WBUFW(buf,n*18+12)=0x8000;
			if(sd->status.inventory[i].equip)
				arrow=i;	// ���łɖ���`�F�b�N
		} else
			WBUFW(buf,n*18+12)=0;
		WBUFW(buf,n*18+14)=sd->status.inventory[i].card[0];
		WBUFW(buf,n*18+16)=sd->status.inventory[i].card[1];
		WBUFW(buf,n*18+18)=sd->status.inventory[i].card[2];
		WBUFW(buf,n*18+20)=sd->status.inventory[i].card[3];
		n++;
	}
	if (n) {
		WBUFW(buf,2)=4+n*18;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#endif
	if(arrow >= 0)
		clif_arrowequip(sd,arrow);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equiplist(struct map_session_data *sd)
{
	int i,j,n,fd;
	unsigned char *buf;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf = WFIFOP(fd,0);
	WBUFW(buf,0)=0xa4;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL || !itemdb_isequip2(*sd->inventory_data[i]))
			continue;
		WBUFW(buf,n*20+4)=i+2;
		if(sd->inventory_data[i]->view_id > 0)
			WBUFW(buf,n*20+6)=sd->inventory_data[i]->view_id;
		else
			WBUFW(buf,n*20+6)=sd->status.inventory[i].nameid;
		WBUFB(buf,n*20+8)=(sd->inventory_data[i]->type == 7)? 4:sd->inventory_data[i]->type;
		WBUFB(buf,n*20+9)=sd->status.inventory[i].identify;
		WBUFW(buf,n*20+10)=pc_equippoint(*sd,i);
		WBUFW(buf,n*20+12)=sd->status.inventory[i].equip;
		WBUFB(buf,n*20+14)=sd->status.inventory[i].attribute;
		WBUFB(buf,n*20+15)=sd->status.inventory[i].refine;
		if(sd->status.inventory[i].card[0]==0x00ff || sd->status.inventory[i].card[0]==0x00fe || sd->status.inventory[i].card[0]==0xff00) {
			WBUFW(buf,n*20+16)=sd->status.inventory[i].card[0];
			WBUFW(buf,n*20+18)=sd->status.inventory[i].card[1];
			WBUFW(buf,n*20+20)=sd->status.inventory[i].card[2];
			WBUFW(buf,n*20+22)=sd->status.inventory[i].card[3];
		} else {
			if(sd->status.inventory[i].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[0])) > 0)
				WBUFW(buf,n*20+16)=j;
			else
				WBUFW(buf,n*20+16)=sd->status.inventory[i].card[0];
			if(sd->status.inventory[i].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[1])) > 0)
				WBUFW(buf,n*20+18)=j;
			else
				WBUFW(buf,n*20+18)=sd->status.inventory[i].card[1];
			if(sd->status.inventory[i].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[2])) > 0)
				WBUFW(buf,n*20+20)=j;
			else
				WBUFW(buf,n*20+20)=sd->status.inventory[i].card[2];
			if(sd->status.inventory[i].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[3])) > 0)
				WBUFW(buf,n*20+22)=j;
			else
				WBUFW(buf,n*20+22)=sd->status.inventory[i].card[3];
		}
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * �J�v������ɗa���Ă�����Օi&���W�i���X�g
 *------------------------------------------
 */
int clif_storageitemlist(struct map_session_data *sd,struct pc_storage *stor)
{
	struct item_data *id;
	int i,n,fd;
	unsigned char *buf;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf = WFIFOP(fd,0);
#if PACKETVER < 5
	WBUFW(buf,0)=0xa5;
	for(i=0,n=0;i<MAX_STORAGE;i++){
		if(stor->storage[i].nameid<=0)
			continue;
		nullpo_retr(0, id = itemdb_search(stor->storage[i].nameid));
		if(itemdb_isequip2(*id))
			continue;

		WBUFW(buf,n*10+4)=i+1;
		if(id->view_id > 0)
			WBUFW(buf,n*10+6)=id->view_id;
		else
			WBUFW(buf,n*10+6)=stor->storage[i].nameid;
		WBUFB(buf,n*10+8)=id->type;;
		WBUFB(buf,n*10+9)=stor->storage[i].identify;
		WBUFW(buf,n*10+10)=stor->storage[i].amount;
		WBUFW(buf,n*10+12)=0;
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#else
	WBUFW(buf,0)=0x1f0;
	for(i=0,n=0;i<MAX_STORAGE;i++){
		if(stor->storage[i].nameid<=0)
			continue;
		nullpo_retr(0, id = itemdb_search(stor->storage[i].nameid));
		if(itemdb_isequip2(*id))
			continue;

		WBUFW(buf,n*18+4)=i+1;
		if(id->view_id > 0)
			WBUFW(buf,n*18+6)=id->view_id;
		else
			WBUFW(buf,n*18+6)=stor->storage[i].nameid;
		WBUFB(buf,n*18+8)=id->type;;
		WBUFB(buf,n*18+9)=stor->storage[i].identify;
		WBUFW(buf,n*18+10)=stor->storage[i].amount;
		WBUFW(buf,n*18+12)=0;
		WBUFW(buf,n*18+14)=stor->storage[i].card[0];
		WBUFW(buf,n*18+16)=stor->storage[i].card[1];
		WBUFW(buf,n*18+18)=stor->storage[i].card[2];
		WBUFW(buf,n*18+20)=stor->storage[i].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*18;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#endif
	return 0;
}

/*==========================================
 * �J�v������ɗa���Ă��鑕�����X�g
 *------------------------------------------
 */
int clif_storageequiplist(struct map_session_data *sd,struct pc_storage *stor)
{
	struct item_data *id;
	int i,j,n,fd;
	unsigned char *buf;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf = WFIFOP(fd,0);
	WBUFW(buf,0)=0xa6;
	for(i=0,n=0;i<MAX_STORAGE;i++){
		if(stor->storage[i].nameid<=0)
			continue;
		nullpo_retr(0, id = itemdb_search(stor->storage[i].nameid));
		if(!itemdb_isequip2(*id))
			continue;
		WBUFW(buf,n*20+4)=i+1;
		if(id->view_id > 0)
			WBUFW(buf,n*20+6)=id->view_id;
		else
			WBUFW(buf,n*20+6)=stor->storage[i].nameid;
		WBUFB(buf,n*20+8)=id->type;
		WBUFB(buf,n*20+9)=stor->storage[i].identify;
		WBUFW(buf,n*20+10)=id->equip;
		WBUFW(buf,n*20+12)=stor->storage[i].equip;
		WBUFB(buf,n*20+14)=stor->storage[i].attribute;
		WBUFB(buf,n*20+15)=stor->storage[i].refine;
		if(stor->storage[i].card[0]==0x00ff || stor->storage[i].card[0]==0x00fe || stor->storage[i].card[0]==0xff00) {
			WBUFW(buf,n*20+16)=stor->storage[i].card[0];
			WBUFW(buf,n*20+18)=stor->storage[i].card[1];
			WBUFW(buf,n*20+20)=stor->storage[i].card[2];
			WBUFW(buf,n*20+22)=stor->storage[i].card[3];
		} else {
			if(stor->storage[i].card[0] > 0 && (j=itemdb_viewid(stor->storage[i].card[0])) > 0)
				WBUFW(buf,n*20+16)=j;
			else
				WBUFW(buf,n*20+16)=stor->storage[i].card[0];
			if(stor->storage[i].card[1] > 0 && (j=itemdb_viewid(stor->storage[i].card[1])) > 0)
				WBUFW(buf,n*20+18)=j;
			else
				WBUFW(buf,n*20+18)=stor->storage[i].card[1];
			if(stor->storage[i].card[2] > 0 && (j=itemdb_viewid(stor->storage[i].card[2])) > 0)
				WBUFW(buf,n*20+20)=j;
			else
				WBUFW(buf,n*20+20)=stor->storage[i].card[2];
			if(stor->storage[i].card[3] > 0 && (j=itemdb_viewid(stor->storage[i].card[3])) > 0)
				WBUFW(buf,n*20+22)=j;
			else
				WBUFW(buf,n*20+22)=stor->storage[i].card[3];
		}
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_guildstorageitemlist(struct map_session_data *sd,struct guild_storage *stor)
{
	struct item_data *id;
	int i,n,fd;
	unsigned char *buf;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf=WFIFOP(fd,0);

#if PACKETVER < 5
	WBUFW(buf,0)=0xa5;
	for(i=0,n=0;i<MAX_GUILD_STORAGE;i++){
		if(stor->storage[i].nameid<=0)
			continue;
		nullpo_retr(0, id = itemdb_search(stor->storage[i].nameid));
		if(itemdb_isequip2(*id))
			continue;

		WBUFW(buf,n*10+4)=i+1;
		if(id->view_id > 0)
			WBUFW(buf,n*10+6)=id->view_id;
		else
			WBUFW(buf,n*10+6)=stor->storage[i].nameid;
		WBUFB(buf,n*10+8)=id->type;;
		WBUFB(buf,n*10+9)=stor->storage[i].identify;
		WBUFW(buf,n*10+10)=stor->storage[i].amount;
		WBUFW(buf,n*10+12)=0;
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#else
	WBUFW(buf,0)=0x1f0;
	for(i=0,n=0;i<MAX_GUILD_STORAGE;i++){
		if(stor->storage[i].nameid<=0)
			continue;
		nullpo_retr(0, id = itemdb_search(stor->storage[i].nameid));
		if(itemdb_isequip2(*id))
			continue;

		WBUFW(buf,n*18+4)=i+1;
		if(id->view_id > 0)
			WBUFW(buf,n*18+6)=id->view_id;
		else
			WBUFW(buf,n*18+6)=stor->storage[i].nameid;
		WBUFB(buf,n*18+8)=id->type;;
		WBUFB(buf,n*18+9)=stor->storage[i].identify;
		WBUFW(buf,n*18+10)=stor->storage[i].amount;
		WBUFW(buf,n*18+12)=0;
		WBUFW(buf,n*18+14)=stor->storage[i].card[0];
		WBUFW(buf,n*18+16)=stor->storage[i].card[1];
		WBUFW(buf,n*18+18)=stor->storage[i].card[2];
		WBUFW(buf,n*18+20)=stor->storage[i].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*18;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#endif
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_guildstorageequiplist(struct map_session_data *sd,struct guild_storage *stor)
{
	struct item_data *id;
	int i,j,n,fd;
	unsigned char *buf;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf=WFIFOP(fd,0);

	WBUFW(buf,0)=0xa6;
	for(i=0,n=0;i<MAX_GUILD_STORAGE;i++){
		if(stor->storage[i].nameid<=0)
			continue;
		nullpo_retr(0, id = itemdb_search(stor->storage[i].nameid));
		if(!itemdb_isequip2(*id))
			continue;
		WBUFW(buf,n*20+4)=i+1;
		if(id->view_id > 0)
			WBUFW(buf,n*20+6)=id->view_id;
		else
			WBUFW(buf,n*20+6)=stor->storage[i].nameid;
		WBUFB(buf,n*20+8)=id->type;
		WBUFB(buf,n*20+9)=stor->storage[i].identify;
		WBUFW(buf,n*20+10)=id->equip;
		WBUFW(buf,n*20+12)=stor->storage[i].equip;
		WBUFB(buf,n*20+14)=stor->storage[i].attribute;
		WBUFB(buf,n*20+15)=stor->storage[i].refine;
		if(stor->storage[i].card[0]==0x00ff || stor->storage[i].card[0]==0x00fe || stor->storage[i].card[0]==0xff00) {
			WBUFW(buf,n*20+16)=stor->storage[i].card[0];
			WBUFW(buf,n*20+18)=stor->storage[i].card[1];
			WBUFW(buf,n*20+20)=stor->storage[i].card[2];
			WBUFW(buf,n*20+22)=stor->storage[i].card[3];
		} else {
			if(stor->storage[i].card[0] > 0 && (j=itemdb_viewid(stor->storage[i].card[0])) > 0)
				WBUFW(buf,n*20+16)=j;
			else
				WBUFW(buf,n*20+16)=stor->storage[i].card[0];
			if(stor->storage[i].card[1] > 0 && (j=itemdb_viewid(stor->storage[i].card[1])) > 0)
				WBUFW(buf,n*20+18)=j;
			else
				WBUFW(buf,n*20+18)=stor->storage[i].card[1];
			if(stor->storage[i].card[2] > 0 && (j=itemdb_viewid(stor->storage[i].card[2])) > 0)
				WBUFW(buf,n*20+20)=j;
			else
				WBUFW(buf,n*20+20)=stor->storage[i].card[2];
			if(stor->storage[i].card[3] > 0 && (j=itemdb_viewid(stor->storage[i].card[3])) > 0)
				WBUFW(buf,n*20+22)=j;
			else
				WBUFW(buf,n*20+22)=stor->storage[i].card[3];
		}
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * �X�e�[�^�X�𑗂����
 * �\����p�����͂��̒��Ōv�Z���đ���
 *------------------------------------------
 */
int clif_updatestatus(struct map_session_data *sd,unsigned short type)
{
	int fd,len=8;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	if ( !session_isActive(fd) ) // Invalid pointer fix, by sasuke [Kevin]
		return 0;
 
	WFIFOW(fd,0)=0xb0;
	WFIFOW(fd,2)=type;
	switch(type){
		// 00b0
	case SP_WEIGHT:
		pc_checkweighticon(*sd);
		WFIFOW(fd,0)=0xb0;
		WFIFOW(fd,2)=type;
		WFIFOL(fd,4)=sd->weight;
		break;
	case SP_MAXWEIGHT:
		WFIFOL(fd,4)=sd->max_weight;
		break;
	case SP_SPEED:
		WFIFOL(fd,4)=sd->speed;
		break;
	case SP_BASELEVEL:
		WFIFOL(fd,4)=sd->status.base_level;
		break;
	case SP_JOBLEVEL:
		WFIFOL(fd,4)=sd->status.job_level;
		break;
	case SP_MANNER:
		WFIFOL(fd,4)=sd->status.manner;
		clif_changestatus(&sd->bl,SP_MANNER,sd->status.manner);
		break;
	case SP_STATUSPOINT:
		WFIFOL(fd,4)=sd->status.status_point;
		break;
	case SP_SKILLPOINT:
		WFIFOL(fd,4)=sd->status.skill_point;
		break;
	case SP_HIT:
		WFIFOL(fd,4)=sd->hit;
		break;
	case SP_FLEE1:
		WFIFOL(fd,4)=sd->flee;
		break;
	case SP_FLEE2:
		WFIFOL(fd,4)=sd->flee2/10;
		break;
	case SP_MAXHP:
		WFIFOL(fd,4)=sd->status.max_hp;
		break;
	case SP_MAXSP:
		WFIFOL(fd,4)=sd->status.max_sp;
		break;
	case SP_HP:
		WFIFOL(fd,4)=sd->status.hp;
		if(battle_config.disp_hpmeter)
			clif_hpmeter(sd);
		break;
	case SP_SP:
		WFIFOL(fd,4)=sd->status.sp;
		break;
	case SP_ASPD:
		WFIFOL(fd,4)=sd->aspd;
		break;
	case SP_ATK1:
		WFIFOL(fd,4)=sd->base_atk+sd->right_weapon.watk;
		break;
	case SP_DEF1:
		WFIFOL(fd,4)=sd->def;
		break;
	case SP_MDEF1:
		WFIFOL(fd,4)=sd->mdef;
		break;
	case SP_ATK2:
		WFIFOL(fd,4)=sd->right_weapon.watk2;
		break;
	case SP_DEF2:
		WFIFOL(fd,4)=sd->def2;
		break;
	case SP_MDEF2:
		WFIFOL(fd,4)=sd->mdef2;
		break;
	case SP_CRITICAL:
		WFIFOL(fd,4)=sd->critical/10;
		break;
	case SP_MATK1:
		WFIFOL(fd,4)=sd->matk1;
		break;
	case SP_MATK2:
		WFIFOL(fd,4)=sd->matk2;
		break;


	case SP_ZENY:
		WFIFOW(fd,0)=0xb1;
		if(sd->status.zeny < 0)
			sd->status.zeny = 0;
		WFIFOL(fd,4)=sd->status.zeny;
		break;
	case SP_BASEEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=sd->status.base_exp;
		break;
	case SP_JOBEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=sd->status.job_exp;
		break;
	case SP_NEXTBASEEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=pc_nextbaseexp(sd);
		break;
	case SP_NEXTJOBEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=pc_nextjobexp(sd);
		break;

		// 00be �I��
	case SP_USTR:
	case SP_UAGI:
	case SP_UVIT:
	case SP_UINT:
	case SP_UDEX:
	case SP_ULUK:
		WFIFOW(fd,0)=0xbe;
		WFIFOB(fd,4)=pc_need_status_point(sd,type-SP_USTR+SP_STR);
		len=5;
		break;

		// 013a �I��
	case SP_ATTACKRANGE:
		WFIFOW(fd,0)=0x13a;
		WFIFOW(fd,2)=sd->attackrange;
		len=4;
		break;

		// 0141 �I��
	case SP_STR:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.str;
		WFIFOL(fd,10)=sd->paramb[0] + sd->parame[0];
		len=14;
		break;
	case SP_AGI:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.agi;
		WFIFOL(fd,10)=sd->paramb[1] + sd->parame[1];
		len=14;
		break;
	case SP_VIT:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.vit;
		WFIFOL(fd,10)=sd->paramb[2] + sd->parame[2];
		len=14;
		break;
	case SP_INT:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.int_;
		WFIFOL(fd,10)=sd->paramb[3] + sd->parame[3];
		len=14;
		break;
	case SP_DEX:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.dex;
		WFIFOL(fd,10)=sd->paramb[4] + sd->parame[4];
		len=14;
		break;
	case SP_LUK:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.luk;
		WFIFOL(fd,10)=sd->paramb[5] + sd->parame[5];
		len=14;
		break;

	case SP_CARTINFO:
		WFIFOW(fd,0)=0x121;
		WFIFOW(fd,2)=sd->cart_num;
		WFIFOW(fd,4)=sd->cart_max_num;
		WFIFOL(fd,6)=sd->cart_weight;
		WFIFOL(fd,10)=sd->cart_max_weight;
		len=14;
		break;

	default:
		if(battle_config.error_log)
			ShowMessage("clif_updatestatus : make %d routine\n",type);
		return 1;
	}
	WFIFOSET(fd,len);

	return 0;
}
int clif_changestatus(struct block_list *bl,unsigned short type,unsigned long val)
{
	unsigned char buf[12];
	nullpo_retr(0, bl);

	if(bl->type == BL_PC)
	{
		WBUFW(buf,0)=0x1ab;
		WBUFL(buf,2)=bl->id;
		WBUFW(buf,6)=type;
		switch(type){
		case SP_MANNER:
			WBUFL(buf,8)=val;
			break;
		default:
			if(battle_config.error_log)
				ShowMessage("clif_changestatus : make %d routine\n",type);
			return 1;
		}
		clif_send(buf,packet_len_table[0x1ab],bl,AREA_WOS);
	}
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int clif_changelook(struct block_list *bl,unsigned char type, unsigned short val)
{

	unsigned char buf[32];
	struct map_session_data *sd = NULL;

	nullpo_retr(0, bl);

	if(bl->type == BL_PC)
		sd = (struct map_session_data *)bl;

	if(sd && sd->disguise_id > 23 && sd->disguise_id < 4001) // mob disguises [Valaris]
		return 0;

#if PACKETVER < 4
	if(sd && (type == LOOK_WEAPON || type == LOOK_SHIELD) && sd->view_class == 22)
		val =0;
	WBUFW(buf,0)=0xc3;
	WBUFL(buf,2)=bl->id;
	WBUFB(buf,6)=type;
	WBUFB(buf,7)=val;
	clif_send(buf,packet_len_table[0xc3],bl,AREA);
#else
	if(sd && (type == LOOK_WEAPON || type == LOOK_SHIELD || type == LOOK_SHOES)) {
		WBUFW(buf,0)=0x1d7;
		WBUFL(buf,2)=bl->id;
		if(type == LOOK_SHOES) {
			WBUFB(buf,6)=9;
			if(sd->equip_index[2] >= 0 && sd->inventory_data[sd->equip_index[2]]) {
				if(sd->inventory_data[sd->equip_index[2]]->view_id > 0)
					WBUFW(buf,7)=sd->inventory_data[sd->equip_index[2]]->view_id;
				else
					WBUFW(buf,7)=sd->status.inventory[sd->equip_index[2]].nameid;
			} else
				WBUFW(buf,7)=0;
			WBUFW(buf,9)=0;
		}
		else {
			WBUFB(buf,6)=2;
			if(sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]] && sd->view_class != 22) {
				if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
					WBUFW(buf,7)=sd->inventory_data[sd->equip_index[9]]->view_id;
				else
					WBUFW(buf,7)=sd->status.inventory[sd->equip_index[9]].nameid;
			} else
				WBUFW(buf,7)=0;
			if(sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9] && sd->inventory_data[sd->equip_index[8]] &&
				sd->view_class != 22) {
				if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
					WBUFW(buf,9)=sd->inventory_data[sd->equip_index[8]]->view_id;
				else
					WBUFW(buf,9)=sd->status.inventory[sd->equip_index[8]].nameid;
			} else
				WBUFW(buf,9)=0;
		}
		clif_send(buf,packet_len_table[0x1d7],bl,AREA);
	}
	else if(sd && (type == LOOK_BASE) && (val > 255))
		{
		WBUFW(buf,0)=0x1d7;
		WBUFL(buf,2)=bl->id;
		WBUFB(buf,6)=type;
		WBUFW(buf,7)=val;
		WBUFW(buf,9)=0;
		clif_send(buf,packet_len_table[0x1d7],bl,AREA);
	} else {
		WBUFW(buf,0)=0xc3;
		WBUFL(buf,2)=bl->id;
		WBUFB(buf,6)=type;
		WBUFB(buf,7)=val;
		clif_send(buf,packet_len_table[0xc3],bl,AREA);
	}
#endif
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_initialstatus(struct map_session_data *sd)
{
	int fd;
	unsigned char *buf;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf=WFIFOP(fd,0);

	WBUFW(buf,0)=0xbd;
	WBUFW(buf,2)=sd->status.status_point;
	WBUFB(buf,4)=(sd->status.str > 255)? 255:sd->status.str;
	WBUFB(buf,5)=pc_need_status_point(sd,SP_STR);
	WBUFB(buf,6)=(sd->status.agi > 255)? 255:sd->status.agi;
	WBUFB(buf,7)=pc_need_status_point(sd,SP_AGI);
	WBUFB(buf,8)=(sd->status.vit > 255)? 255:sd->status.vit;
	WBUFB(buf,9)=pc_need_status_point(sd,SP_VIT);
	WBUFB(buf,10)=(sd->status.int_ > 255)? 255:sd->status.int_;
	WBUFB(buf,11)=pc_need_status_point(sd,SP_INT);
	WBUFB(buf,12)=(sd->status.dex > 255)? 255:sd->status.dex;
	WBUFB(buf,13)=pc_need_status_point(sd,SP_DEX);
	WBUFB(buf,14)=(sd->status.luk > 255)? 255:sd->status.luk;
	WBUFB(buf,15)=pc_need_status_point(sd,SP_LUK);

	WBUFW(buf,16) = sd->base_atk + sd->right_weapon.watk;
	WBUFW(buf,18) = sd->right_weapon.watk2; //atk bonus
	WBUFW(buf,20) = sd->matk1;
	WBUFW(buf,22) = sd->matk2;
	WBUFW(buf,24) = sd->def; // def
	WBUFW(buf,26) = sd->def2;
	WBUFW(buf,28) = sd->mdef; // mdef
	WBUFW(buf,30) = sd->mdef2;
	WBUFW(buf,32) = sd->hit;
	WBUFW(buf,34) = sd->flee;
	WBUFW(buf,36) = sd->flee2/10;
	WBUFW(buf,38) = sd->critical/10;
	WBUFW(buf,40) = sd->status.karma;
	WBUFW(buf,42) = sd->status.manner;

	WFIFOSET(fd,packet_len_table[0xbd]);

	clif_updatestatus(sd,SP_STR);
	clif_updatestatus(sd,SP_AGI);
	clif_updatestatus(sd,SP_VIT);
	clif_updatestatus(sd,SP_INT);
	clif_updatestatus(sd,SP_DEX);
	clif_updatestatus(sd,SP_LUK);

	clif_updatestatus(sd,SP_ATTACKRANGE);
	clif_updatestatus(sd,SP_ASPD);

	return 0;
}

/*==========================================
 *���
 *------------------------------------------
 */
int clif_arrowequip(struct map_session_data *sd,unsigned short val)
{
	int fd;

	nullpo_retr(0, sd);

	if(sd->attacktarget && sd->attacktarget > 0) // [Valaris]
		sd->attacktarget = 0;

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x013c;
	WFIFOW(fd,2)=val+2;//��̃A�C�e��ID

	WFIFOSET(fd,packet_len_table[0x013c]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_arrow_fail(struct map_session_data *sd,unsigned short type)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0x013b;
	WFIFOW(fd,2)=type;

	WFIFOSET(fd,packet_len_table[0x013b]);

	return 0;
}

/*==========================================
 * �쐬�\ ��X�g���M
 *------------------------------------------
 */
int clif_arrow_create_list(struct map_session_data *sd)
{
	int i, c, j;
	int fd;

	nullpo_retr(0, sd);

	fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0) = 0x1ad;

	for (i = 0, c = 0; i < MAX_SKILL_ARROW_DB; i++) {
		if (skill_arrow_db[i].nameid > 0 &&
			(j = pc_search_inventory(*sd, skill_arrow_db[i].nameid)) >= 0 &&
			!sd->status.inventory[j].equip)
		{
			if ((j = itemdb_viewid(skill_arrow_db[i].nameid)) > 0)
				WFIFOW(fd,c*2+4) = j;
			else
				WFIFOW(fd,c*2+4) = skill_arrow_db[i].nameid;
			c++;
		}
	}
	WFIFOW(fd,2) = c*2+4;
	WFIFOSET(fd, WFIFOW(fd,2));
	if (c > 0) sd->state.produce_flag = 1;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_statusupack(struct map_session_data *sd,unsigned short type,unsigned char ok,unsigned char val)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xbc;
	WFIFOW(fd,2)=type;
	WFIFOB(fd,4)=ok;
	WFIFOB(fd,5)=val;
	WFIFOSET(fd,packet_len_table[0xbc]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equipitemack(struct map_session_data *sd,unsigned short n,unsigned short pos,unsigned char ok)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xaa;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=pos;
	WFIFOB(fd,6)=ok;
	WFIFOSET(fd,packet_len_table[0xaa]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_unequipitemack(struct map_session_data *sd,unsigned short n,unsigned short pos,unsigned char ok)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xac;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=pos;
	WFIFOB(fd,6)=ok;
	WFIFOSET(fd,packet_len_table[0xac]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_misceffect(struct block_list* bl, unsigned long type)
{
	unsigned char buf[32];

	nullpo_retr(0, bl);

	WBUFW(buf,0) = 0x19b;
	WBUFL(buf,2) = bl->id;
	WBUFL(buf,6) = type;

	return clif_send(buf,packet_len_table[0x19b],bl,AREA);
}

int clif_misceffect2(struct block_list *bl, unsigned long type)
{
	unsigned char buf[24];
	nullpo_retr(0, bl);

	memset(buf, 0, packet_len_table[0x1f3]);

	WBUFW(buf,0) = 0x1f3;
	WBUFL(buf,2) = bl->id;
	WBUFL(buf,6) = type;

	return clif_send(buf, packet_len_table[0x1f3], bl, AREA);
}
/*==========================================
 * �\���I�v�V�����ύX
 *------------------------------------------
 */
int clif_changeoption(struct block_list* bl)
{
	unsigned char buf[32];
	short option;
	struct status_change *sc_data;
	static const int omask[]={ 0x10,0x20 };
	static const int scnum[]={ SC_FALCON, SC_RIDING };
	size_t i;

	nullpo_retr(0, bl);

	option = *status_get_option(bl);
	sc_data = status_get_sc_data(bl);

	WBUFW(buf,0) = 0x119;
	WBUFL(buf,2) = bl->id;
	WBUFW(buf,6) = *status_get_opt1(bl);
	WBUFW(buf,8) = *status_get_opt2(bl);
	WBUFW(buf,10) = option;
	WBUFB(buf,12) = 0;	// ??

	if(bl->type==BL_PC) { // disguises [Valaris]
		struct map_session_data *sd=(struct map_session_data *)bl;
		if(sd && sd->disguise_id > 23 && sd->disguise_id < 4001) {
			clif_send(buf,packet_len_table[0x119],bl,AREA_WOS);
			clif_spawnpc(sd);
		} else
			clif_send(buf,packet_len_table[0x119],bl,AREA);
	} else
		clif_send(buf,packet_len_table[0x119],bl,AREA);

	// �A�C�R���̕\��
	for(i=0;i<sizeof(omask)/sizeof(omask[0]);i++){
		if( option&omask[i] ){
			if( sc_data[scnum[i]].timer==-1)
				status_change_start(bl,scnum[i],0,0,0,0,0,0);
		} else {
			status_change_end(bl,scnum[i],-1);
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_useitemack(struct map_session_data *sd,unsigned short index,unsigned short amount,unsigned char ok)
{
	nullpo_retr(0, sd);


	if(!ok) {
		int fd=sd->fd;
		if( !session_isActive(fd) )
			return 0;
		
		WFIFOW(fd,0)=0xa8;
		WFIFOW(fd,2)=index+2;
		WFIFOW(fd,4)=amount;
		WFIFOB(fd,6)=ok;
		WFIFOSET(fd,packet_len_table[0xa8]);
	}
	else {
#if PACKETVER < 3
		int fd=sd->fd;
		if( !session_isActive(fd) )
			return 0;
		
		WFIFOW(fd,0)=0xa8;
		WFIFOW(fd,2)=index+2;
		WFIFOW(fd,4)=amount;
		WFIFOB(fd,6)=ok;
		WFIFOSET(fd,packet_len_table[0xa8]);
#else
		unsigned char buf[32];

		WBUFW(buf,0)=0x1c8;
		WBUFW(buf,2)=index+2;
		if(sd->inventory_data[index] && sd->inventory_data[index]->view_id > 0)
			WBUFW(buf,4)=sd->inventory_data[index]->view_id;
		else
			WBUFW(buf,4)=sd->status.inventory[index].nameid;
		WBUFL(buf,6)=sd->bl.id;
		WBUFW(buf,10)=amount;
		WBUFB(buf,12)=ok;
		clif_send(buf,packet_len_table[0x1c8],&sd->bl,AREA);
#endif
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_createchat(struct map_session_data *sd,unsigned char fail)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xd6;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xd6]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dispchat(struct chat_data *cd,int fd)
{
	unsigned char buf[128];	// �ő�title(60�o�C�g)+17

	if(cd==NULL || cd->owner==NULL)
		return 1;

	WBUFW(buf,0)=0xd7;
	WBUFW(buf,2)=strlen((char*)(cd->title))+17;
	WBUFL(buf,4)=cd->owner->id;
	WBUFL(buf,8)=cd->bl.id;
	WBUFW(buf,12)=cd->limit;
	WBUFW(buf,14)=cd->users;
	WBUFB(buf,16)=cd->pub;
	strcpy((char*)WBUFP(buf,17),(char*)(cd->title));

	if(fd && session_isActive(fd) ) {
		memcpy(WFIFOP(fd,0),buf,WBUFW(buf,2));
		WFIFOSET(fd,WBUFW(buf,2));
	} else {
		clif_send(buf,WBUFW(buf,2),cd->owner,AREA_WOSC);
	}

	return 0;
}

/*==========================================
 * chat�̏�ԕύX����
 * �O���̐l�p�Ɩ��߃R�[�h(d7->df)���Ⴄ����
 *------------------------------------------
 */
int clif_changechatstatus(struct chat_data *cd)
{
	unsigned char buf[128];	// �ő�title(60�o�C�g)+17

	if(cd==NULL || cd->usersd[0]==NULL)
		return 1;

	WBUFW(buf,0)=0xdf;
	WBUFW(buf,2)=strlen((char*)(cd->title))+17;
	WBUFL(buf,4)=cd->usersd[0]->bl.id;
	WBUFL(buf,8)=cd->bl.id;
	WBUFW(buf,12)=cd->limit;
	WBUFW(buf,14)=cd->users;
	WBUFB(buf,16)=cd->pub;
	strcpy((char*)WBUFP(buf,17),(char*)(cd->title));
	clif_send(buf,WBUFW(buf,2),&cd->usersd[0]->bl,CHAT);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchat(struct chat_data *cd,int fd)
{
	unsigned char buf[32];

	nullpo_retr(0, cd);

	WBUFW(buf,0)=0xd8;
	WBUFL(buf,2)=cd->bl.id;

	if(fd && session_isActive(fd) ) {
		memcpy(WFIFOP(fd,0),buf,packet_len_table[0xd8]);
		WFIFOSET(fd,packet_len_table[0xd8]);
	} else {
		clif_send(buf,packet_len_table[0xd8],cd->owner,AREA_WOSC);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_joinchatfail(struct map_session_data *sd,unsigned char fail)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0xda;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xda]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_joinchatok(struct map_session_data *sd,struct chat_data* cd)
{
	int fd;
	int i;

	nullpo_retr(0, sd);
	nullpo_retr(0, cd);

	fd = sd->fd;
	if (!session_isActive(fd))
		return 0;

	WFIFOW(fd, 0) = 0xdb;
	WFIFOW(fd, 2) = 8 + (28*cd->users);
	WFIFOL(fd, 4) = cd->bl.id;
	for (i = 0; i < cd->users; i++)
	{
		WFIFOL(fd, 8+i*28) = (i!=0) || (cd->owner->type == BL_NPC);
		memcpy(WFIFOP(fd, 8+i*28+4), cd->usersd[i]->status.name, 24);
	}
	WFIFOSET(fd, WFIFOW(fd, 2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_addchat(struct chat_data* cd,struct map_session_data *sd)
{
	unsigned char buf[32];

	nullpo_retr(0, sd);
	nullpo_retr(0, cd);

	WBUFW(buf, 0) = 0x0dc;
	WBUFW(buf, 2) = cd->users;
	memcpy(WBUFP(buf, 4),sd->status.name,24);
	return clif_send(buf,packet_len_table[0xdc],&sd->bl,CHAT_WOS);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changechatowner(struct chat_data* cd,struct map_session_data *sd)
{
	unsigned char buf[64];

	nullpo_retr(0, sd);
	nullpo_retr(0, cd);

	WBUFW(buf, 0) = 0xe1;
	WBUFL(buf, 2) = 1;
	memcpy(WBUFP(buf,6),cd->usersd[0]->status.name,24);
	WBUFW(buf,30) = 0xe1;
	WBUFL(buf,32) = 0;
	memcpy(WBUFP(buf,36),sd->status.name,24);

	return clif_send(buf,packet_len_table[0xe1]*2,&sd->bl,CHAT);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_leavechat(struct chat_data* cd,struct map_session_data *sd)
{
	unsigned char buf[32];

	nullpo_retr(0, sd);
	nullpo_retr(0, cd);

	WBUFW(buf, 0) = 0xdd;
	WBUFW(buf, 2) = cd->users-1;
	memcpy(WBUFP(buf,4),sd->status.name,24);
	WBUFB(buf,28) = 0;

	return clif_send(buf,packet_len_table[0xdd],&sd->bl,CHAT);
}

/*==========================================
 * �������v����
 *------------------------------------------
 */
int clif_traderequest(struct map_session_data *sd,char *name)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xe5;
	strcpy((char*)WFIFOP(fd,2),name);
	WFIFOSET(fd,packet_len_table[0xe5]);

	return 0;
}

/*==========================================
 * �������v������
 *------------------------------------------
 */
int clif_tradestart(struct map_session_data *sd,unsigned char type)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xe7;
	WFIFOB(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0xe7]);

	return 0;
}

/*==========================================
 * ���������̃A�C�e���ǉ�
 *------------------------------------------
 */
int clif_tradeadditem(struct map_session_data *sd,struct map_session_data *tsd,unsigned short index,unsigned long amount)
{
	int fd;
	unsigned short j;

	nullpo_retr(0, sd);
	nullpo_retr(0, tsd);

	fd=tsd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0xe9;
	WFIFOL(fd,2)=amount;
	index -= 2;
	// will cover all invalid states of variable 'index '
	if(index >= MAX_INVENTORY)
	{
		WFIFOW(fd,6) = 0; // type id
		WFIFOB(fd,8) = 0; //identify flag
		WFIFOB(fd,9) = 0; // attribute
		WFIFOB(fd,10)= 0; //refine
		WFIFOW(fd,11)= 0; //card (4w)
		WFIFOW(fd,13)= 0; //card (4w)
		WFIFOW(fd,15)= 0; //card (4w)
		WFIFOW(fd,17)= 0; //card (4w)
	}
	else
	{
		if(sd->inventory_data[index] && sd->inventory_data[index]->view_id > 0)
			WFIFOW(fd,6) = sd->inventory_data[index]->view_id;
		else
			WFIFOW(fd,6) = sd->status.inventory[index].nameid; // type id
		WFIFOB(fd,8) = sd->status.inventory[index].identify; //identify flag
		WFIFOB(fd,9) = sd->status.inventory[index].attribute; // attribute
		WFIFOB(fd,10)= sd->status.inventory[index].refine; //refine
		if(sd->status.inventory[index].card[0]==0x00ff || sd->status.inventory[index].card[0]==0x00fe || sd->status.inventory[index].card[0]==0xff00) {
			WFIFOW(fd,11)= sd->status.inventory[index].card[0]; //card (4w)
			WFIFOW(fd,13)= sd->status.inventory[index].card[1]; //card (4w)
			WFIFOW(fd,15)= sd->status.inventory[index].card[2]; //card (4w)
			WFIFOW(fd,17)= sd->status.inventory[index].card[3]; //card (4w)
		} else {
			if(sd->status.inventory[index].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[index].card[0])) > 0)
				WFIFOW(fd,11)= j;
			else
				WFIFOW(fd,11)= sd->status.inventory[index].card[0];
			if(sd->status.inventory[index].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[index].card[1])) > 0)
				WFIFOW(fd,13)= j;
			else
				WFIFOW(fd,13)= sd->status.inventory[index].card[1];
			if(sd->status.inventory[index].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[index].card[2])) > 0)
				WFIFOW(fd,15)= j;
			else
				WFIFOW(fd,15)= sd->status.inventory[index].card[2];
			if(sd->status.inventory[index].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[index].card[3])) > 0)
				WFIFOW(fd,17)= j;
			else
				WFIFOW(fd,17)= sd->status.inventory[index].card[3];
		}
	}
	WFIFOSET(fd,packet_len_table[0xe9]);

	return 0;
}

/*==========================================
 * �A�C�e���ǉ�����/���s
 *------------------------------------------
 */
int clif_tradeitemok(struct map_session_data *sd,unsigned short index,unsigned char fail)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xea;
	WFIFOW(fd,2)=index;
	WFIFOB(fd,4)=fail;
	WFIFOSET(fd,packet_len_table[0xea]);

	return 0;
}

/*==========================================
 * ������ok����
 *------------------------------------------
 */
int clif_tradedeal_lock(struct map_session_data *sd,unsigned char fail)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xec;
	WFIFOB(fd,2)=fail; // 0=you 1=the other person
	WFIFOSET(fd,packet_len_table[0xec]);

	return 0;
}

/*==========================================
 * ���������L�����Z������܂���
 *------------------------------------------
 */
int clif_tradecancelled(struct map_session_data *sd)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xee;
	WFIFOSET(fd,packet_len_table[0xee]);

	return 0;
}

/*==========================================
 * ����������
 *------------------------------------------
 */
int clif_tradecompleted(struct map_session_data *sd,unsigned char fail)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xf0;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xf0]);

	return 0;
}

/*==========================================
 * �J�v���q�ɂ̃A�C�e�������X�V
 *------------------------------------------
 */
int clif_updatestorageamount(struct map_session_data *sd,struct pc_storage *stor)
{
	int fd;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0) = 0xf2;  // update storage amount
	WFIFOW(fd,2) = stor->storage_amount;  //items
	WFIFOW(fd,4) = MAX_STORAGE; //items max
	WFIFOSET(fd,packet_len_table[0xf2]);

	return 0;
}

/*==========================================
 * �J�v���q�ɂɃA�C�e����ǉ�����
 *------------------------------------------
 */
int clif_storageitemadded(struct map_session_data *sd,struct pc_storage *stor,unsigned short index,unsigned long amount)
{
	int view,fd,j;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0) =0xf4; // Storage item added
	WFIFOW(fd,2) =index+1; // index
	WFIFOL(fd,4) =amount; // amount
	if((view = itemdb_viewid(stor->storage[index].nameid)) > 0)
		WFIFOW(fd,8) =view;
	else
		WFIFOW(fd,8) =stor->storage[index].nameid; // id
	WFIFOB(fd,10)=stor->storage[index].identify; //identify flag
	WFIFOB(fd,11)=stor->storage[index].attribute; // attribute
	WFIFOB(fd,12)=stor->storage[index].refine; //refine
	if(stor->storage[index].card[0]==0x00ff || stor->storage[index].card[0]==0x00fe || stor->storage[index].card[0]==0xff00) {
		WFIFOW(fd,13)=stor->storage[index].card[0]; //card (4w)
		WFIFOW(fd,15)=stor->storage[index].card[1]; //card (4w)
		WFIFOW(fd,17)=stor->storage[index].card[2]; //card (4w)
		WFIFOW(fd,19)=stor->storage[index].card[3]; //card (4w)
	} else {
		if(stor->storage[index].card[0] > 0 && (j=itemdb_viewid(stor->storage[index].card[0])) > 0)
			WFIFOW(fd,13)= j;
		else
			WFIFOW(fd,13)= stor->storage[index].card[0];
		if(stor->storage[index].card[1] > 0 && (j=itemdb_viewid(stor->storage[index].card[1])) > 0)
			WFIFOW(fd,15)= j;
		else
			WFIFOW(fd,15)= stor->storage[index].card[1];
		if(stor->storage[index].card[2] > 0 && (j=itemdb_viewid(stor->storage[index].card[2])) > 0)
			WFIFOW(fd,17)= j;
		else
			WFIFOW(fd,17)= stor->storage[index].card[2];
		if(stor->storage[index].card[3] > 0 && (j=itemdb_viewid(stor->storage[index].card[3])) > 0)
			WFIFOW(fd,19)= j;
		else
			WFIFOW(fd,19)= stor->storage[index].card[3];
	}
	WFIFOSET(fd,packet_len_table[0xf4]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_updateguildstorageamount(struct map_session_data *sd,struct guild_storage *stor)
{
	int fd;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0) = 0xf2;  // update storage amount
	WFIFOW(fd,2) = stor->storage_amount;  //items
	WFIFOW(fd,4) = MAX_GUILD_STORAGE; //items max
	WFIFOSET(fd,packet_len_table[0xf2]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_guildstorageitemadded(struct map_session_data *sd,struct guild_storage *stor,unsigned short index,unsigned long amount)
{
	int view,fd,j;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0) =0xf4; // Storage item added
	WFIFOW(fd,2) =index+1; // index
	WFIFOL(fd,4) =amount; // amount
	if((view = itemdb_viewid(stor->storage[index].nameid)) > 0)
		WFIFOW(fd,8) =view;
	else
		WFIFOW(fd,8) =stor->storage[index].nameid; // id
	WFIFOB(fd,10)=stor->storage[index].identify; //identify flag
	WFIFOB(fd,11)=stor->storage[index].attribute; // attribute
	WFIFOB(fd,12)=stor->storage[index].refine; //refine
	if(stor->storage[index].card[0]==0x00ff || stor->storage[index].card[0]==0x00fe || stor->storage[index].card[0]==0xff00) {
		WFIFOW(fd,13)=stor->storage[index].card[0]; //card (4w)
		WFIFOW(fd,15)=stor->storage[index].card[1]; //card (4w)
		WFIFOW(fd,17)=stor->storage[index].card[2]; //card (4w)
		WFIFOW(fd,19)=stor->storage[index].card[3]; //card (4w)
	} else {
		if(stor->storage[index].card[0] > 0 && (j=itemdb_viewid(stor->storage[index].card[0])) > 0)
			WFIFOW(fd,13)= j;
		else
			WFIFOW(fd,13)= stor->storage[index].card[0];
		if(stor->storage[index].card[1] > 0 && (j=itemdb_viewid(stor->storage[index].card[1])) > 0)
			WFIFOW(fd,15)= j;
		else
			WFIFOW(fd,15)= stor->storage[index].card[1];
		if(stor->storage[index].card[2] > 0 && (j=itemdb_viewid(stor->storage[index].card[2])) > 0)
			WFIFOW(fd,17)= j;
		else
			WFIFOW(fd,17)= stor->storage[index].card[2];
		if(stor->storage[index].card[3] > 0 && (j=itemdb_viewid(stor->storage[index].card[3])) > 0)
			WFIFOW(fd,19)= j;
		else
			WFIFOW(fd,19)= stor->storage[index].card[3];
	}
	WFIFOSET(fd,packet_len_table[0xf4]);

	return 0;
}

/*==========================================
 * �J�v���q�ɂ���A�C�e������苎��
 *------------------------------------------
 */
int clif_storageitemremoved(struct map_session_data *sd,unsigned short index,unsigned long amount)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xf6; // Storage item removed
	WFIFOW(fd,2)=index+1;
	WFIFOL(fd,4)=amount;
	WFIFOSET(fd,packet_len_table[0xf6]);

	return 0;
}

/*==========================================
 * �J�v���q�ɂ����
 *------------------------------------------
 */
int clif_storageclose(struct map_session_data *sd)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xf8; // Storage Closed
	WFIFOSET(fd,packet_len_table[0xf8]);

	return 0;
}

//
// callback�n ?
//
/*==========================================
 * PC�\��
 *------------------------------------------
 */
int clif_getareachar_pc(struct map_session_data* sd,struct map_session_data* dstsd)
{
	int len;

	nullpo_retr(0,sd);
	nullpo_retr(0,dstsd);

	if(dstsd->walktimer != -1){
		len = clif_set007b(dstsd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	} else {
		len = clif_set0078(dstsd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	}

	if(dstsd->chatID){
		struct chat_data *cd;
		cd=(struct chat_data*)map_id2bl(dstsd->chatID);
		if(cd->usersd[0]==dstsd)
			clif_dispchat(cd,sd->fd);
	}
	if(dstsd->vender_id){
		clif_showvendingboard(&dstsd->bl,dstsd->message,sd->fd);
	}
	if(dstsd->spiritball > 0) {
		clif_set01e1(dstsd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,packet_len_table[0x1e1]);
	}
	if(battle_config.save_clothcolor &&
		dstsd->status.clothes_color > 0 &&
		(dstsd->view_class != 22 || !battle_config.wedding_ignorepalette)
		)
		clif_changelook(&dstsd->bl,LOOK_CLOTHES_COLOR,dstsd->status.clothes_color);
	if(sd->status.manner < 0)
		clif_changestatus(&sd->bl,SP_MANNER,sd->status.manner);

	return 0;
}

/*==========================================
 * NPC�\��
 *------------------------------------------
 */
//fixed by Valaris
int clif_getareachar_npc(struct map_session_data* sd,struct npc_data* nd)
{
	int len;
	nullpo_retr(0,sd);
	nullpo_retr(0,nd);
	if(nd->class_ < 0 || nd->flag&1 || nd->class_ == INVISIBLE_CLASS)
		return 0;
	if(nd->state.state == MS_WALK){
		len = clif_npc007b(nd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	} else {
	len = clif_npc0078(nd,WFIFOP(sd->fd,0));
	WFIFOSET(sd->fd,len);
	}
	if(nd->chat_id){
		clif_dispchat((struct chat_data*)map_id2bl(nd->chat_id),sd->fd);
	}
	return 0;
}

/*==========================================
 * �ړ���~
 *------------------------------------------
 */
int clif_movemob(struct mob_data *md)
{
	unsigned char buf[256];
	int len;

	nullpo_retr(0, md);

	len = clif_mob007b(md,buf);
	clif_send(buf,len,&md->bl,AREA);

	if(mob_get_equip(md->class_) > 0) // mob equipment [Valaris]
		clif_mob_equip(md,mob_get_equip(md->class_));

	if(md->size==2) // tiny/big mobs [Valaris]
		clif_specialeffect(&md->bl,423,0);
	else if(md->size==1)
		clif_specialeffect(&md->bl,421,0);

	return 0;
}

/*==========================================
 * �����X�^�[�̈ʒu�C��
 *------------------------------------------
 */
int clif_fixmobpos(struct mob_data *md)
{
	unsigned char buf[256];
	int len;

	nullpo_retr(0, md);

	if(md->state.state == MS_WALK){
		len = clif_mob007b(md,buf);
	} else {
		len = clif_mob0078(md,buf);
	}
	return clif_send(buf,len,&md->bl,AREA);
}

/*==========================================
 * PC�̈ʒu�C��
 *------------------------------------------
 */
int clif_fixpcpos(struct map_session_data *sd)
{
	unsigned char buf[256];
	int len;

	nullpo_retr(0, sd);

	if(sd->walktimer != -1){
		len = clif_set007b(sd,buf);
	} else {
		len = clif_set0078(sd,buf);
	}
	return clif_send(buf,len,&sd->bl,AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_fixpetpos(struct pet_data *pd)
{
	unsigned char buf[256];
	int len;

	nullpo_retr(0, pd);

	if(pd->state.state == MS_WALK){
		len = clif_pet007b(pd,buf);
	} else {
		len = clif_pet0078(pd,buf);
	}
	return clif_send(buf,len,&pd->bl,AREA);
}

// npc walking [Valaris]
int clif_fixnpcpos(struct npc_data *nd)
{
	unsigned char buf[256];
	int len;

	nullpo_retr(0, nd);

	if(nd->state.state == MS_WALK){
		len = clif_npc007b(nd,buf);
	} else {
		len = clif_npc0078(nd,buf);
	}
	return clif_send(buf,len,&nd->bl,AREA);
}


/*==========================================
 * �ʏ�U���G�t�F�N�g���_���[�W
 *------------------------------------------
 */
int clif_damage(struct block_list *src,struct block_list *dst,unsigned long tick,unsigned long sdelay,unsigned long ddelay,unsigned short damage,unsigned short div,unsigned char type,unsigned short damage2)
{
	unsigned char buf[256];
	struct status_change *sc_data;

	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	sc_data = status_get_sc_data(dst);

	if(type != 4 && dst->type == BL_PC && ((struct map_session_data *)dst)->state.infinite_endure)
		type = 9;
	if(sc_data) {
		if(type != 4 && sc_data[SC_ENDURE].timer != -1 &&
			(dst->type == BL_PC && !map[dst->m].flag.gvg))
			type = 9;
		if(sc_data[SC_HALLUCINATION].timer != -1) {
			if(damage > 0)
				damage = damage*(5+sc_data[SC_HALLUCINATION].val1) + rand()%100;
			if(damage2 > 0)
				damage2 = damage2*(5+sc_data[SC_HALLUCINATION].val1) + rand()%100;
		}
	}

	WBUFW(buf,0)=0x8a;
	WBUFL(buf,2)=src->id;
	WBUFL(buf,6)=dst->id;
	WBUFL(buf,10)=tick;
	WBUFL(buf,14)=sdelay;
	WBUFL(buf,18)=ddelay;
	WBUFW(buf,22)=(damage > 0x7fff)? 0x7fff:damage;
	WBUFW(buf,24)=div;
	WBUFB(buf,26)=type;
	WBUFW(buf,27)=damage2;
	return clif_send(buf,packet_len_table[0x8a],src,AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_getareachar_mob(struct map_session_data* sd,struct mob_data* md)
{
	int len;
	nullpo_retr(0,sd);
	nullpo_retr(0,md);

	if (session[sd->fd] == NULL)
		return 0;

	if(md->state.state == MS_WALK){
		len = clif_mob007b(md,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	} else {
		len = clif_mob0078(md,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	}

	if(mob_get_equip(md->class_) > 0) // mob equipment [Valaris]
		clif_mob_equip(md,mob_get_equip(md->class_));

	if(md->size==2) // tiny/big mobs [Valaris]
		clif_specialeffect(&md->bl,423,0);
	else if(md->size==1)
		clif_specialeffect(&md->bl,421,0);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_getareachar_pet(struct map_session_data* sd,struct pet_data* pd)
{
	int len;
	int fd;

	nullpo_retr(0,sd);
	nullpo_retr(0,pd);

	fd = sd->fd;
	if( !session_isActive(fd) )
			return 0;

	if(pd->state.state == MS_WALK){
		len = clif_pet007b(pd,WFIFOP(sd->fd,0));
	} else {
		len = clif_pet0078(pd,WFIFOP(sd->fd,0));
	}
		WFIFOSET(sd->fd,len);
	return 0;
	}

/*==========================================
 *
 *------------------------------------------
 */
int clif_getareachar_item(struct map_session_data* sd,struct flooritem_data* fitem)
{
	int view,fd;

	nullpo_retr(0,sd);
	nullpo_retr(0,fitem);

	fd=sd->fd;
	if( !session_isActive(fd) )
			return 0;
	
	//009d <ID>.l <item ID>.w <identify flag>.B <X>.w <Y>.w <amount>.w <subX>.B <subY>.B
	WFIFOW(fd,0)=0x9d;
	WFIFOL(fd,2)=fitem->bl.id;
	if((view = itemdb_viewid(fitem->item_data.nameid)) > 0)
		WFIFOW(fd,6)=view;
	else
		WFIFOW(fd,6)=fitem->item_data.nameid;
	WFIFOB(fd,8)=fitem->item_data.identify;
	WFIFOW(fd,9)=fitem->bl.x;
	WFIFOW(fd,11)=fitem->bl.y;
	WFIFOW(fd,13)=fitem->item_data.amount;
	WFIFOB(fd,15)=(unsigned char)fitem->subx;
	WFIFOB(fd,16)=(unsigned char)fitem->suby;

	WFIFOSET(fd,packet_len_table[0x9d]);
	return 0;
}
/*==========================================
 * �ꏊ�X�L���G�t�F�N�g�����E�ɓ���
 *------------------------------------------
 */
int clif_getareachar_skillunit(struct map_session_data *sd,struct skill_unit *unit)
{
	int fd;
	struct block_list *bl;

	nullpo_retr(0, unit);

	fd=sd->fd;
	if( !session_isActive(fd) )
			return 0;
	
	bl=map_id2bl(unit->group->src_id);
#if PACKETVER < 3
	memset(WFIFOP(fd,0),0,packet_len_table[0x11f]);
	WFIFOW(fd, 0)=0x11f;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOL(fd, 6)=unit->group->src_id;
	WFIFOW(fd,10)=unit->bl.x;
	WFIFOW(fd,12)=unit->bl.y;
	WFIFOB(fd,14)=unit->group->unit_id;
	WFIFOB(fd,15)=0;
	WFIFOSET(fd,packet_len_table[0x11f]);
#else
	memset(WFIFOP(fd,0),0,packet_len_table[0x1c9]);
	WFIFOW(fd, 0)=0x1c9;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOL(fd, 6)=unit->group->src_id;
	WFIFOW(fd,10)=unit->bl.x;
	WFIFOW(fd,12)=unit->bl.y;
	WFIFOB(fd,14)=unit->group->unit_id;
	WFIFOB(fd,15)=1;
	WFIFOL(fd,15+1)=0;						//1-4���ׂ�����Œ�
	WFIFOL(fd,15+5)=0;						//5-8���ׂ�����Œ�
											//9-12�}�b�v���Ƃň���77-80�Ƃ͂܂��Ⴄ4�o�C�g�̂��Ȃ�傫�Ȑ���
	WFIFOL(fd,15+13)=unit->bl.y - 0x12;		//13-16���j�b�g��Y���W-18���ۂ�(Y:17��FF FF FF FF)
	WFIFOL(fd,15+17)=0x004f37dd;			//17-20���ׂ�����Œ�
	WFIFOL(fd,15+21)=0x0012f674;			//21-24���ׂ�����Œ�
	WFIFOL(fd,15+25)=0x0012f664;			//25-28���ׂ�����Œ�
	WFIFOL(fd,15+29)=0x0012f654;			//29-32���ׂ�����Œ�
	WFIFOL(fd,15+33)=0x77527bbc;			//33-36���ׂ�����Œ�
											//37-39
	WFIFOB(fd,15+40)=0x2d;					//40���ׂ�����Œ�
	WFIFOL(fd,15+41)=0;						//41-44���ׂ�����0�Œ�
	WFIFOL(fd,15+45)=0;						//45-48���ׂ�����0�Œ�
	WFIFOL(fd,15+49)=0;						//49-52���ׂ�����0�Œ�
	WFIFOL(fd,15+53)=0x0048d919;			//53-56���ׂ�����Œ�
	WFIFOL(fd,15+57)=0x0000003e;			//57-60���ׂ�����Œ�
	WFIFOL(fd,15+61)=0x0012f66c;			//61-64���ׂ�����Œ�
											//65-68
											//69-72
	if(bl) WFIFOL(fd,15+73)=bl->y;			//73-76�p�҂�Y���W
		WFIFOL(fd,15+77)=unit->bl.m;			//77-80�}�b�vID���Ȃ��H���Ȃ�2�o�C�g�ő��肻���Ȑ���
	WFIFOB(fd,15+81)=0xaa;					//81�I�[����0xaa

	/*	Graffiti [Valaris]	*/
	if(unit->group->unit_id==0xb0)	{
		WFIFOL(fd,15)=1;
		WFIFOL(fd,16)=1;
		memcpy(WFIFOP(fd,17),unit->group->valstr,80);
	}

	WFIFOSET(fd,packet_len_table[0x1c9]);
#endif
	if(unit->group && unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(fd,unit->bl.m,unit->bl.x,unit->bl.y,5);

	return 0;
}
/*==========================================
 * �ꏊ�X�L���G�t�F�N�g�����E���������
 *------------------------------------------
 */
int clif_clearchar_skillunit(struct skill_unit *unit,int fd)
{
	nullpo_retr(0, unit);
	if( !session_isActive(fd) )
			return 0;

	WFIFOW(fd, 0)=0x120;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOSET(fd,packet_len_table[0x120]);
	if(unit->group && unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(fd,unit->bl.m,unit->bl.x,unit->bl.y,unit->val2);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_01ac(struct block_list *bl)
{
	unsigned char buf[32];

	nullpo_retr(0, bl);

	WBUFW(buf, 0) = 0x1ac;
	WBUFL(buf, 2) = bl->id;

	return clif_send(buf,packet_len_table[0x1ac],bl,AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
 int clif_getareachar(struct block_list* bl,va_list ap)
{
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	sd=va_arg(ap,struct map_session_data*);

	if (sd == NULL || session[sd->fd] == NULL)
		return 0;

	switch(bl->type){
	case BL_PC:
		if(sd==(struct map_session_data*)bl)
			break;
		clif_getareachar_pc(sd,(struct map_session_data*) bl);
		break;
	case BL_NPC:
		clif_getareachar_npc(sd,(struct npc_data*) bl);
		break;
	case BL_MOB:
		clif_getareachar_mob(sd,(struct mob_data*) bl);
		break;
	case BL_PET:
		clif_getareachar_pet(sd,(struct pet_data*) bl);
		break;
	case BL_ITEM:
		clif_getareachar_item(sd,(struct flooritem_data*) bl);
		break;
	case BL_SKILL:
		clif_getareachar_skillunit(sd,(struct skill_unit *)bl);
		break;
	default:
		if(battle_config.error_log)
			ShowMessage("get area char ??? %d\n",bl->type);
		break;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd,*dstsd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=va_arg(ap,struct map_session_data*));

	switch(bl->type){
	case BL_PC:
		dstsd = (struct map_session_data*) bl;
		if(sd != dstsd) {
			clif_clearchar_id(sd->fd, dstsd->bl.id,0);
			clif_clearchar_id(dstsd->fd, sd->bl.id,0);
			if(dstsd->chatID){
				struct chat_data *cd;
				cd=(struct chat_data*)map_id2bl(dstsd->chatID);
				if(cd->usersd[0]==dstsd)
					clif_dispchat(cd,sd->fd);
			}
			if(dstsd->vender_id){
				clif_closevendingboard(&dstsd->bl,sd->fd);
			}
		}
		break;
	case BL_NPC:
		if( ((struct npc_data *)bl)->class_ != INVISIBLE_CLASS )
			clif_clearchar_id(sd->fd,bl->id,0);
		break;
	case BL_MOB:
	case BL_PET:
		clif_clearchar_id(sd->fd,bl->id,0);
		break;
	case BL_ITEM:
		clif_clearflooritem((struct flooritem_data*)bl,sd->fd);
		break;
	case BL_SKILL:
		clif_clearchar_skillunit((struct skill_unit *)bl,sd->fd);
		break;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd,*dstsd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=va_arg(ap,struct map_session_data*));

	switch(bl->type){
	case BL_PC:
		dstsd = (struct map_session_data *)bl;
		if(sd != dstsd) {
			clif_getareachar_pc(sd,dstsd);
			clif_getareachar_pc(dstsd,sd);
		}
		break;
	case BL_NPC:
		clif_getareachar_npc(sd,(struct npc_data*)bl);
		break;
	case BL_MOB:
		clif_getareachar_mob(sd,(struct mob_data*)bl);
		break;
	case BL_PET:
		clif_getareachar_pet(sd,(struct pet_data*)bl);
		break;
	case BL_ITEM:
		clif_getareachar_item(sd,(struct flooritem_data*)bl);
		break;
	case BL_SKILL:
		clif_getareachar_skillunit(sd,(struct skill_unit *)bl);
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_moboutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct mob_data *md;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md=va_arg(ap,struct mob_data*));

	if(bl->type==BL_PC
	  && ((sd = (struct map_session_data*) bl) != NULL)
	  && session[sd->fd] != NULL) {
		clif_clearchar_id(sd->fd,md->bl.id,0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mobinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct mob_data *md;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	md=va_arg(ap,struct mob_data*);
	if(bl->type==BL_PC
	  && ((sd = (struct map_session_data*) bl) != NULL)
	  && session[sd->fd] != NULL) {
		clif_getareachar_mob(sd,md);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_petoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct pet_data *pd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, pd=va_arg(ap,struct pet_data*));

	if(bl->type==BL_PC
	  && ((sd = (struct map_session_data*) bl) != NULL)
	  && session[sd->fd] != NULL) {
		clif_clearchar_id(sd->fd,pd->bl.id,0);
	}

	return 0;
}

// npc walking [Valaris]
int clif_npcoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct npc_data *nd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, nd=va_arg(ap,struct npc_data*));

	if(bl->type==BL_PC
	  && ((sd = (struct map_session_data*) bl) != NULL)
	  && session[sd->fd] != NULL) {
		clif_clearchar_id(sd->fd,nd->bl.id,0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_petinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct pet_data *pd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	pd=va_arg(ap,struct pet_data*);
	if(bl->type==BL_PC
	  && ((sd = (struct map_session_data*) bl) != NULL)
	  && session[sd->fd] != NULL) {
		clif_getareachar_pet(sd,pd);
	}

	return 0;
}

// npc walking [Valaris]
int clif_npcinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct npc_data *nd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	nd=va_arg(ap,struct npc_data*);
	if(bl->type==BL_PC
	  && ((sd = (struct map_session_data*) bl) != NULL)
	  && session[sd->fd] != NULL) {
		clif_getareachar_npc(sd,nd);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillinfo(struct map_session_data *sd,unsigned short skillid, short type,short range)
{
	int fd;
	unsigned short id;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	id=sd->status.skill[skillid].id;

	WFIFOW(fd,0)=0x147;
	WFIFOW(fd,2) = id;
	if(type < 0)
		WFIFOW(fd,4) = skill_get_inf(id);
	else
		WFIFOW(fd,4) = type;
	WFIFOW(fd,6) = 0;
	WFIFOW(fd,8) = sd->status.skill[skillid].lv;
	WFIFOW(fd,10) = skill_get_sp(id,sd->status.skill[skillid].lv);
	if(range < 0) {
		range = skill_get_range(id,sd->status.skill[skillid].lv);
		if(range < 0)
			range = status_get_range(&sd->bl) - (range + 1);
		WFIFOW(fd,12)= range;
	} else
		WFIFOW(fd,12)= range;
	memset(WFIFOP(fd,14),0,24);
	if(!(skill_get_inf2(id)&0x01) || battle_config.quest_skill_learn == 1 || (battle_config.gm_allskill > 0 && pc_isGM(*sd) >= battle_config.gm_allskill) )
		//WFIFOB(fd,38)= (sd->status.skill[skillid].lv < skill_get_max(id) && sd->status.skill[skillid].flag ==0 )? 1:0;
		WFIFOB(fd,38)= (sd->status.skill[skillid].lv < skill_tree_get_max(id, sd->status.class_) && sd->status.skill[skillid].flag ==0 )? 1:0;
	else
		WFIFOB(fd,38) = 0;
	WFIFOSET(fd,packet_len_table[0x147]);

	return 0;
}

/*==========================================
 * �X�L�����X�g�𑗐M����
 *------------------------------------------
 */
int clif_skillinfoblock(struct map_session_data *sd)
{
	int fd;
	int i,c,len=4,id,range;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
			return 0;
	
	WFIFOW(fd,0)=0x10f;
	for ( i = c = 0; i < MAX_SKILL; i++){
		if( (id=sd->status.skill[i].id)!=0 ){
			WFIFOW(fd,len  ) = id;
			WFIFOW(fd,len+2) = skill_get_inf(id);
			WFIFOW(fd,len+4) = 0;
			WFIFOW(fd,len+6) = sd->status.skill[i].lv;
			WFIFOW(fd,len+8) = skill_get_sp(id,sd->status.skill[i].lv);
			range = skill_get_range(id,sd->status.skill[i].lv);
			if(range < 0)
				range = status_get_range(&sd->bl) - (range + 1);
			WFIFOW(fd,len+10)= range;
			memset(WFIFOP(fd,len+12),0,24);
			if(!(skill_get_inf2(id)&0x01) || battle_config.quest_skill_learn == 1 || (battle_config.gm_allskill > 0 && pc_isGM(*sd) >= battle_config.gm_allskill) )
				//WFIFOB(fd,len+36)= (sd->status.skill[i].lv < skill_get_max(id) && sd->status.skill[i].flag ==0 )? 1:0;
				WFIFOB(fd,len+36)= (sd->status.skill[i].lv < skill_tree_get_max(id, sd->status.class_) && sd->status.skill[i].flag ==0 )? 1:0;
			else
				WFIFOB(fd,len+36) = 0;
			len+=37;
			c++;
		}
	}
	WFIFOW(fd,2)=len;
	WFIFOSET(fd,len);

	return 0;
}

/*==========================================
 * �X�L������U��ʒm
 *------------------------------------------
 */
int clif_skillup(struct map_session_data *sd, unsigned short skill_num)
{
	int range,fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
			return 0;
	
	WFIFOW(fd,0) = 0x10e;
	WFIFOW(fd,2) = skill_num;
	WFIFOW(fd,4) = sd->status.skill[skill_num].lv;
	WFIFOW(fd,6) = skill_get_sp(skill_num,sd->status.skill[skill_num].lv);
	range = skill_get_range(skill_num,sd->status.skill[skill_num].lv);
	if(range < 0)
		range = status_get_range(&sd->bl) - (range + 1);
	WFIFOW(fd,8) = range;
	//WFIFOB(fd,10) = (sd->status.skill[skill_num].lv < skill_get_max(sd->status.skill[skill_num].id)) ? 1 : 0;
	WFIFOB(fd,10) = (sd->status.skill[skill_num].lv < skill_tree_get_max(sd->status.skill[skill_num].id, sd->status.class_)) ? 1 : 0;
	WFIFOSET(fd,packet_len_table[0x10e]);

	return 0;
}

/*==========================================
 * �X�L���r���G�t�F�N�g�𑗐M����
 *------------------------------------------
 */
int clif_skillcasting(struct block_list* bl,unsigned long src_id,unsigned long dst_id,unsigned short dst_x,unsigned short dst_y,unsigned short skill_id,unsigned long casttime)
{
	unsigned char buf[32];
	WBUFW(buf,0) = 0x13e;
	WBUFL(buf,2) = src_id;
	WBUFL(buf,6) = dst_id;
	WBUFW(buf,10) = dst_x;
	WBUFW(buf,12) = dst_y;
	WBUFW(buf,14) = skill_id;//���@�r���X�L��
	WBUFL(buf,16) = skill_get_pl(skill_id);//����
	WBUFL(buf,20) = casttime;//skill�r������
	return clif_send(buf,packet_len_table[0x13e], bl, AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillcastcancel(struct block_list* bl)
{
	unsigned char buf[16];

	nullpo_retr(0, bl);

	WBUFW(buf,0) = 0x1b9;
	WBUFL(buf,2) = bl->id;
	return clif_send(buf,packet_len_table[0x1b9], bl, AREA);
}

/*==========================================
 * �X�L���r�����s
 *------------------------------------------
 */
int clif_skill_fail(struct map_session_data *sd,unsigned short skill_id,unsigned char type,unsigned short btype)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
			return 0;

	// reset all variables [celest]
	sd->skillx    = sd->skilly      = 0xFFFF;
	sd->skillid   = sd->skilllv     = 0xFFFF;
	sd->skillitem = sd->skillitemlv = 0xFFFF;

	if(type==0x4 && (battle_config.display_delay_skill_fail==0 || sd->state.nodelay)){
		return 0;
	}

	WFIFOW(fd,0) = 0x110;
	WFIFOW(fd,2) = skill_id;
	WFIFOW(fd,4) = btype;
	WFIFOW(fd,6) = 0;
	WFIFOB(fd,8) = 0;
	WFIFOB(fd,9) = type;
	WFIFOSET(fd,packet_len_table[0x110]);

	return 0;
}

/*==========================================
 * �X�L���U���G�t�F�N�g���_���[�W
 *------------------------------------------
 */
int clif_skill_damage(struct block_list *src,struct block_list *dst,unsigned long tick,unsigned long sdelay,unsigned long ddelay,unsigned long damage,unsigned short div,unsigned short skill_id,unsigned short skill_lv,int type)
{
	unsigned char buf[64];
	struct status_change *sc_data;

	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	sc_data = status_get_sc_data(dst);

	if(type != 5 && dst->type == BL_PC && ((struct map_session_data *)dst)->state.infinite_endure)
		type = 9;
	if(sc_data) {
		if(type != 5 && sc_data[SC_ENDURE].timer != -1)
			type = 9;
		if(sc_data[SC_HALLUCINATION].timer != -1 && damage > 0)
			damage = damage*(5+sc_data[SC_HALLUCINATION].val1) + rand()%100;
	}

#if PACKETVER < 3
	WBUFW(buf,0)=0x114;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFW(buf,24)=damage;
	WBUFW(buf,26)=skill_lv;
	WBUFW(buf,28)=div;
	WBUFB(buf,30)=(type>0)?type:skill_get_hit(skill_id);
	return clif_send(buf,packet_len_table[0x114],src,AREA);
#else
	WBUFW(buf,0)=0x1de;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFL(buf,24)=damage;
	WBUFW(buf,28)=skill_lv;
	WBUFW(buf,30)=div;
	WBUFB(buf,32)=(type>0)?type:skill_get_hit(skill_id);
	return clif_send(buf,packet_len_table[0x1de],src,AREA);
#endif
}

/*==========================================
 * ������΂��X�L���U���G�t�F�N�g���_���[�W
 *------------------------------------------
 */
int clif_skill_damage2(struct block_list *src,struct block_list *dst,unsigned long tick,unsigned long sdelay,unsigned long ddelay,unsigned long damage,unsigned short div,unsigned short skill_id,unsigned short skill_lv,int type)
{
	unsigned char buf[64];
	struct status_change *sc_data;

	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	sc_data = status_get_sc_data(dst);

	if(type != 5 && dst->type == BL_PC && ((struct map_session_data *)dst)->state.infinite_endure)
		type = 9;
	if(sc_data) {
		if(type != 5 && sc_data[SC_ENDURE].timer != -1)
			type = 9;
		if(sc_data[SC_HALLUCINATION].timer != -1 && damage > 0)
			damage = damage*(5+sc_data[SC_HALLUCINATION].val1) + rand()%100;
	}

	WBUFW(buf,0)=0x115;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFW(buf,24)=dst->x;
	WBUFW(buf,26)=dst->y;
	WBUFW(buf,28)=damage;
	WBUFW(buf,30)=skill_lv;
	WBUFW(buf,32)=div;
	WBUFB(buf,34)=(type>0)?type:skill_get_hit(skill_id);
	return clif_send(buf,packet_len_table[0x115],src,AREA);
}

/*==========================================
 * �x��/�񕜃X�L���G�t�F�N�g
 *------------------------------------------
 */
int clif_skill_nodamage(struct block_list *src,struct block_list *dst,unsigned short skill_id,unsigned short heal,unsigned char fail)
{
	unsigned char buf[32];

	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	WBUFW(buf,0)=0x11a;
	WBUFW(buf,2)=skill_id;
	WBUFW(buf,4)=(heal > 0x7fff)? 0x7fff:heal;
	WBUFL(buf,6)=dst->id;
	WBUFL(buf,10)=src->id;
	WBUFB(buf,14)=fail;
	return clif_send(buf,packet_len_table[0x11a],src,AREA);
}

/*==========================================
 * �ꏊ�X�L���G�t�F�N�g
 *------------------------------------------
 */
int clif_skill_poseffect(struct block_list *src,unsigned short skill_id,unsigned short val,unsigned short x,unsigned short y,unsigned long tick)
{
	unsigned char buf[32];

	nullpo_retr(0, src);

	WBUFW(buf,0)=0x117;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFW(buf,8)=val;
	WBUFW(buf,10)=x;
	WBUFW(buf,12)=y;
	WBUFL(buf,14)=tick;
	return clif_send(buf,packet_len_table[0x117],src,AREA);
}

/*==========================================
 * �ꏊ�X�L���G�t�F�N�g�\��
 *------------------------------------------
 */
int clif_skill_setunit(struct skill_unit *unit)
{
	unsigned char buf[128];
	struct block_list *bl;

	nullpo_retr(0, unit);

	bl=map_id2bl(unit->group->src_id);

#if PACKETVER < 3
	memset(WBUFP(buf, 0),0,packet_len_table[0x11f]);
	WBUFW(buf, 0)=0x11f;
	WBUFL(buf, 2)=unit->bl.id;
	WBUFL(buf, 6)=unit->group->src_id;
	WBUFW(buf,10)=unit->bl.x;
	WBUFW(buf,12)=unit->bl.y;
	WBUFB(buf,14)=unit->group->unit_id;
	WBUFB(buf,15)=0;
	return clif_send(buf,packet_len_table[0x11f],&unit->bl,AREA);
#else
		memset(WBUFP(buf, 0),0,packet_len_table[0x1c9]);
		WBUFW(buf, 0)=0x1c9;
		WBUFL(buf, 2)=unit->bl.id;
		WBUFL(buf, 6)=unit->group->src_id;
		WBUFW(buf,10)=unit->bl.x;
		WBUFW(buf,12)=unit->bl.y;
		WBUFB(buf,14)=unit->group->unit_id;
		WBUFB(buf,15)=1;
		WBUFL(buf,15+1)=0;						//1-4���ׂ�����Œ�
		WBUFL(buf,15+5)=0;						//5-8���ׂ�����Œ�
											//9-12�}�b�v���Ƃň���77-80�Ƃ͂܂��Ⴄ4�o�C�g�̂��Ȃ�傫�Ȑ���
		WBUFL(buf,15+13)=unit->bl.y - 0x12;		//13-16���j�b�g��Y���W-18���ۂ�(Y:17��FF FF FF FF)
		WBUFL(buf,15+17)=0x004f37dd;			//17-20���ׂ�����Œ�(0x1b2��0x004fdbdd������)
		WBUFL(buf,15+21)=0x0012f674;			//21-24���ׂ�����Œ�
		WBUFL(buf,15+25)=0x0012f664;			//25-28���ׂ�����Œ�
		WBUFL(buf,15+29)=0x0012f654;			//29-32���ׂ�����Œ�
		WBUFL(buf,15+33)=0x77527bbc;			//33-36���ׂ�����Œ�
											//37-39
		WBUFB(buf,15+40)=0x2d;					//40���ׂ�����Œ�
		WBUFL(buf,15+41)=0;						//41-44���ׂ�����0�Œ�
		WBUFL(buf,15+45)=0;						//45-48���ׂ�����0�Œ�
		WBUFL(buf,15+49)=0;						//49-52���ׂ�����0�Œ�
		WBUFL(buf,15+53)=0x0048d919;			//53-56���ׂ�����Œ�(0x01b2��0x00495119������)
		WBUFL(buf,15+57)=0x0000003e;			//57-60���ׂ�����Œ�
		WBUFL(buf,15+61)=0x0012f66c;			//61-64���ׂ�����Œ�
											//65-68
											//69-72
		if(bl) WBUFL(buf,15+73)=bl->y;			//73-76�p�҂�Y���W
			WBUFL(buf,15+77)=unit->bl.m;			//77-80�}�b�vID���Ȃ��H���Ȃ�2�o�C�g�ő��肻���Ȑ���
		WBUFB(buf,15+81)=0xaa;					//81�I�[����0xaa

		/*		Graffiti [Valaris]		*/
		if(unit->group->unit_id==0xb0)	{
			WBUFL(buf,15)=1;
			WBUFL(buf,16)=1;
			memcpy(WBUFP(buf,17),unit->group->valstr,80);
		}

	return clif_send(buf,packet_len_table[0x1c9],&unit->bl,AREA);
#endif
}
/*==========================================
 * �ꏊ�X�L���G�t�F�N�g�폜
 *------------------------------------------
 */
int clif_skill_delunit(struct skill_unit *unit)
{
	unsigned char buf[16];

	nullpo_retr(0, unit);

	WBUFW(buf, 0)=0x120;
	WBUFL(buf, 2)=unit->bl.id;
	return clif_send(buf,packet_len_table[0x120],&unit->bl,AREA);
}
/*==========================================
 * ���[�v�ꏊ�I��
 *------------------------------------------
 */
int clif_skill_warppoint(struct map_session_data *sd,unsigned short skill_id,
	const char *map1,const char *map2,const char *map3,const char *map4)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
			return 0;
	
	WFIFOW(fd,0)=0x11c;
	WFIFOW(fd,2)=skill_id;
	memcpy(WFIFOP(fd, 4),map1,16);
	memcpy(WFIFOP(fd,20),map2,16);
	memcpy(WFIFOP(fd,36),map3,16);
	memcpy(WFIFOP(fd,52),map4,16);
	WFIFOSET(fd,packet_len_table[0x11c]);
	return 0;
}
/*==========================================
 * ��������
 *------------------------------------------
 */
int clif_skill_memo(struct map_session_data *sd,unsigned char flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
			return 0;

	WFIFOW(fd,0)=0x11e;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x11e]);
	return 0;
}
int clif_skill_teleportmessage(struct map_session_data *sd,unsigned short flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
			return 0;
	
	WFIFOW(fd,0)=0x189;
	WFIFOW(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x189]);
	return 0;
}

/*==========================================
 * �����X�^�[���
 *------------------------------------------
 */
int clif_skill_estimation(struct map_session_data *sd,struct block_list *dst)
{
	struct mob_data *md;
	unsigned char buf[64];
	int i;

	nullpo_retr(0, sd);
	nullpo_retr(0, dst);

	if(dst->type!=BL_MOB )
		return 0;
	if((md=(struct mob_data *)dst) == NULL)
		return 0;

	WBUFW(buf, 0)=0x18c;
	WBUFW(buf, 2)=mob_get_viewclass(md->class_);
	WBUFW(buf, 4)=md->level;
	WBUFW(buf, 6)=mob_db[md->class_].size;
	WBUFL(buf, 8)=md->hp;
	WBUFW(buf,12)=status_get_def2(&md->bl);
	WBUFW(buf,14)=mob_db[md->class_].race;
	WBUFW(buf,16)=status_get_mdef2(&md->bl) - (mob_db[md->class_].vit>>1);
	WBUFW(buf,18)=status_get_elem_type(&md->bl);
	for(i=0;i<9;i++)
		WBUFB(buf,20+i)= battle_attr_fix(100,i+1,md->def_ele);

	if(sd->status.party_id>0)

		return clif_send(buf,packet_len_table[0x18c],&sd->bl,PARTY_AREA);

	else if( session_isActive(sd->fd) ) {
		memcpy(WFIFOP(sd->fd,0),buf,packet_len_table[0x18c]);
		WFIFOSET(sd->fd,packet_len_table[0x18c]);
	}
	return 0;
}
/*==========================================
 * �A�C�e�������\���X�g
 *------------------------------------------
 */
int clif_skill_produce_mix_list(struct map_session_data *sd,int trigger)
{
	int i,c,view,fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
			return 0;
	
	WFIFOW(fd, 0)=0x18d;

	for(i=0,c=0;i<MAX_SKILL_PRODUCE_DB;i++){
		if( skill_can_produce_mix(sd,skill_produce_db[i].nameid,trigger) ){
			if((view = itemdb_viewid(skill_produce_db[i].nameid)) > 0)
				WFIFOW(fd,c*8+ 4)= view;
			else
				WFIFOW(fd,c*8+ 4)= skill_produce_db[i].nameid;
			WFIFOW(fd,c*8+ 6)= 0x0012;
			WFIFOL(fd,c*8+ 8)= sd->status.char_id;
			c++;
		}
	}
	WFIFOW(fd, 2)=c*8+8;
	WFIFOSET(fd,WFIFOW(fd,2));
	if(c > 0) sd->state.produce_flag = 1;
	return 0;
}

/*==========================================
 * ��Ԉُ�A�C�R��/���b�Z�[�W�\��
 *------------------------------------------
 */
int clif_status_change(struct block_list *bl,unsigned short type,unsigned char flag)
{
	unsigned char buf[16];

	nullpo_retr(0, bl);

	WBUFW(buf,0)=0x0196;
	WBUFW(buf,2)=type;
	WBUFL(buf,4)=bl->id;
	WBUFB(buf,8)=flag;
	return clif_send(buf,packet_len_table[0x196],bl,AREA);
}

/*==========================================
 * Send message (modified by [Yor])
 *------------------------------------------
 */
int clif_displaymessage(int fd, const char* mes)
{
	// invalid pointer?
	nullpo_retr(-1, mes);
	
	//Console [Wizputer]
	if (fd == 0)
		ShowConsole(CL_BOLD"%s\n"CL_NORM, mes);

	else if(mes && session_isActive(fd) ) {

		size_t len=strlen(mes);
		if( len > 0 ) { // don't send a void message (it's not displaying on the client chat). @help can send void line.
			WFIFOW(fd,0) = 0x8e;
			WFIFOW(fd,2) = 4 + len+1; // 4 + len + NULL terminate
			memcpy(WFIFOP(fd,4), mes, len+1);
			WFIFOSET(fd, 4 + len+1);
		}
	}
	return 0;
}

/*==========================================
 * �V�̐��𑗐M����
 *------------------------------------------
 */
int clif_GMmessage(struct block_list *bl, const char* mes, int flag)
{
	if(mes)
	{
		unsigned char buf[512];
	int lp;
		int len = strlen(mes)+1;

	lp = (flag & 0x10) ? 8 : 4;
		if( len > 512-lp ) len = 512-lp;

	WBUFW(buf,0) = 0x9a;
	WBUFW(buf,2) = len + lp;
	WBUFL(buf,4) = 0x65756c62;
	memcpy(WBUFP(buf,lp), mes, len);
		buf[511] = 0; //force EOS
	flag &= 0x07;
		clif_send(buf, len + lp, bl,
	          (flag == 1) ? ALL_SAMEMAP :
	          (flag == 2) ? AREA :
	          (flag == 3) ? SELF :
	          ALL_CLIENT);
	}
	return 0;
}

/*==========================================
 * �O���[�o�����b�Z�[�W
 *------------------------------------------
 */
int clif_GlobalMessage(struct block_list *bl,char *message)
{
	unsigned char buf[108];

	if(!bl || !message)
		return 0;

	size_t len = (message)? strlen(message)+1 : 0;
	if(len>100) len=100;

	WBUFW(buf,0)=0x8d;
	WBUFW(buf,2)=len+8;
	WBUFL(buf,4)=bl->id;
	memcpy(WBUFP(buf,8),message,len);
	buf[107]=0;// just paranoid
	return clif_send(buf,len+8,bl,AREA_CHAT_WOC);
}

/*==========================================
 * HPSP�񕜃G�t�F�N�g�𑗐M����
 *------------------------------------------
 */
int clif_heal(int fd,unsigned short type,unsigned short val)
{
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x13d;
	WFIFOW(fd,2)=type;
	WFIFOW(fd,4)=val;
	WFIFOSET(fd,packet_len_table[0x13d]);

	return 0;
}

/*==========================================
 * ��������
 *------------------------------------------
 */
int clif_resurrection(struct block_list *bl,unsigned short type)
{
	unsigned char buf[16];

	nullpo_retr(0, bl);

	if(bl->type==BL_PC) { // disguises [Valaris]
		struct map_session_data *sd=((struct map_session_data *)bl);
		if(sd && sd->disguise_id > 23 && sd->disguise_id < 4001)
			clif_spawnpc(sd);
	}

	WBUFW(buf,0)=0x148;
	WBUFL(buf,2)=bl->id;
	WBUFW(buf,6)=type;

	return clif_send(buf,packet_len_table[0x148],bl,type==1 ? AREA : AREA_WOS);
}

/*==========================================
 * PVP�����H�i���j
 *------------------------------------------
 */
int clif_set0199(int fd,unsigned short type)
{
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x199;
	WFIFOW(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0x199]);

	return 0;
}

/*==========================================
 * PVP�����H(��)
 *------------------------------------------
 */
int clif_pvpset(struct map_session_data *sd,unsigned long pvprank,unsigned long pvpnum,int type)
{
	nullpo_retr(0, sd);

	if(map[sd->bl.m].flag.nopvp)
		return 0;

	if(type == 2) {
		WFIFOW(sd->fd,0) = 0x19a;
		WFIFOL(sd->fd,2) = sd->bl.id;
		if(pvprank<=0)
			pc_calc_pvprank(sd);
		WFIFOL(sd->fd,6) = pvprank;
		WFIFOL(sd->fd,10) = pvpnum;
		WFIFOSET(sd->fd,packet_len_table[0x19a]);
	} else {
		unsigned char buf[32];

		WBUFW(buf,0) = 0x19a;
		WBUFL(buf,2) = sd->bl.id;
		if(sd->status.option&0x46)
			WBUFL(buf,6) = 0xFFFFFFFF;
		else
			if(pvprank<=0)
				pc_calc_pvprank(sd);
			WBUFL(buf,6) = pvprank;
		WBUFL(buf,10) = pvpnum;
		if(!type)
			clif_send(buf,packet_len_table[0x19a],&sd->bl,AREA);
		else
			clif_send(buf,packet_len_table[0x19a],&sd->bl,ALL_SAMEMAP);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_send0199(unsigned short map,unsigned short type)
{
	struct block_list bl;
	unsigned char buf[16];

	bl.m = map;
	WBUFW(buf,0)=0x199;
	WBUFW(buf,2)=type;
	return clif_send(buf,packet_len_table[0x199],&bl,ALL_SAMEMAP);
}

/*==========================================
 * ���B�G�t�F�N�g�𑗐M����
 *------------------------------------------
 */
int clif_refine(int fd,struct map_session_data *sd,unsigned short fail,unsigned short index,unsigned short val)
{
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x188;
	WFIFOW(fd,2)=fail;
	WFIFOW(fd,4)=index+2;
	WFIFOW(fd,6)=val;
	WFIFOSET(fd,packet_len_table[0x188]);

	return 0;
}

/*==========================================
 * Wisp/page is transmitted to the destination player
 *------------------------------------------
 */
int clif_wis_message(int fd, char *nick, char *mes, size_t mes_len)
{	 // R 0097 <len>.w <nick>.24B <message>.?B
//	ShowMessage("clif_wis_message(%d, %s, %s)\n", fd, nick, mes);
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x97;
	WFIFOW(fd,2) = mes_len + 24 + 4;
	memcpy(WFIFOP(fd,4), nick, 24);
	memcpy(WFIFOP(fd,28), mes, mes_len);
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * The transmission result of Wisp/page is transmitted to the source player
 *------------------------------------------
 */
int clif_wis_end(int fd, unsigned short flag)
{	 // R 0098 <type>.B: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x98;
	WFIFOW(fd,2) = flag;
	WFIFOSET(fd,packet_len_table[0x98]);
	return 0;
}

/*==========================================
 * �L����ID���O�������ʂ𑗐M����
 *------------------------------------------
 */
int clif_solved_charname(struct map_session_data *sd, unsigned long char_id)
{
	char *p= map_charid2nick(char_id);
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	if(p!=NULL){
		WFIFOW(fd,0)=0x194;
		WFIFOL(fd,2)=char_id;
		memcpy(WFIFOP(fd,6), p,24 );
		WFIFOSET(fd,packet_len_table[0x194]);
	}else{
		map_reqchariddb(*sd,char_id);
		chrif_searchcharid(char_id);
	}
	return 0;
}

/*==========================================
 * �J�[�h�̑}���\���X�g��Ԃ�
 *------------------------------------------
 */
int clif_use_card(struct map_session_data *sd,size_t idx)
{
	nullpo_retr(0, sd);

	if( idx<MAX_INVENTORY && sd->inventory_data[idx])
	{
		size_t i,j;
		unsigned short c;
		unsigned short ep=sd->inventory_data[idx]->equip;
		int fd=sd->fd;
		if( !session_isActive(fd) )
			return 0;
		
		WFIFOW(fd,0)=0x017b;

		for(i=c=0;i<MAX_INVENTORY;i++)
		{
			if(sd->inventory_data[i] == NULL)
				continue;
			if(sd->inventory_data[i]->type!=4 && sd->inventory_data[i]->type!=5)	// ����h���Ȃ�
				continue;
			if(sd->status.inventory[i].card[0]==0x00ff)	// ��������
				continue;
			if(sd->status.inventory[i].card[0]==0xff00 || sd->status.inventory[i].card[0]==0x00fe)
				continue;
			if(sd->status.inventory[i].identify==0 )	// ���Ӓ�
				continue;

			if((sd->inventory_data[i]->equip&ep)==0)	// ���������Ⴄ
				continue;
			if(sd->inventory_data[i]->type==4 && ep==32)	// ���J�[�h�Ɨ��蕐��
				continue;

			for(j=0;j<sd->inventory_data[i]->flag.slot;j++){
				if( sd->status.inventory[i].card[j]==0 )
					break;
			}
			if(j==sd->inventory_data[i]->flag.slot)	// ���łɃJ�[�h����t
				continue;

			WFIFOW(fd,4+c*2)=i+2;
			c++;
		}
		WFIFOW(fd,2)=4+c*2;
		WFIFOSET(fd,WFIFOW(fd,2));
	}

	return 0;
}
/*==========================================
 * �J�[�h�̑}���I��
 *------------------------------------------
 */
int clif_insert_card(struct map_session_data *sd,unsigned short idx_equip,unsigned short idx_card,unsigned char flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0x17d;
	WFIFOW(fd,2)=idx_equip+2;
	WFIFOW(fd,4)=idx_card+2;
	WFIFOB(fd,6)=flag;
	WFIFOSET(fd,packet_len_table[0x17d]);
	return 0;
}

/*==========================================
 * �Ӓ�\�A�C�e�����X�g���M
 *------------------------------------------
 */
int clif_item_identify_list(struct map_session_data *sd)
{
	int i,c;
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x177;
	for(i=c=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].identify!=1){
			WFIFOW(fd,c*2+4)=i+2;
			c++;
		}
	}
	if(c > 0) {
		WFIFOW(fd,2)=c*2+4;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * �Ӓ茋��
 *------------------------------------------
 */
int clif_item_identified(struct map_session_data *sd,unsigned short idx,unsigned char flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd, 0)=0x179;
	WFIFOW(fd, 2)=idx+2;
	WFIFOB(fd, 4)=flag;
	WFIFOSET(fd,packet_len_table[0x179]);
	return 0;
}

/*==========================================
 * �C���\�A�C�e�����X�g���M
 * �����ۂ̃p�P�b�g���킩��Ȃ��̂œ��삵�܂���
 *------------------------------------------
 */
int clif_item_repair_list(struct map_session_data *sd)
{
	int i,c;
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x177; // temporarily use same packet as clif_item_identify
	for(i=c=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].attribute==1){
			WFIFOW(fd,c*2+4)=i+2;
			c++;
		}
	}
	if(c > 0) {
		WFIFOW(fd,2)=c*2+4;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * Weapon Refining [Celest]
 *------------------------------------------
 */
int clif_item_refine_list(struct map_session_data *sd)
{
	int i,c;
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x177; // temporarily use same packet as clif_item_identify
	for(i=c=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid > 0 && itemdb_type(sd->status.inventory[i].nameid)==4){
			WFIFOW(fd,c*2+4)=i+2;
			c++;
		}
	}
	if(c > 0) {
		WFIFOW(fd,2)=c*2+4;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * �A�C�e���ɂ��ꎞ�I�ȃX�L������
 *------------------------------------------
 */
int clif_item_skill(struct map_session_data *sd,unsigned short skillid,unsigned short skilllv,const char *name)
{
	int range,fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd, 0)=0x147;
	WFIFOW(fd, 2)=skillid;
	WFIFOW(fd, 4)=skill_get_inf(skillid);
	WFIFOW(fd, 6)=0;
	WFIFOW(fd, 8)=skilllv;
	WFIFOW(fd,10)=skill_get_sp(skillid,skilllv);
	range = skill_get_range(skillid,skilllv);
	if(range < 0)
		range = status_get_range(&sd->bl) - (range + 1);
	WFIFOW(fd,12)=range;
	memcpy(WFIFOP(fd,14),name,24);
	WFIFOB(fd,38)=0;
	WFIFOSET(fd,packet_len_table[0x147]);
	return 0;
}

/*==========================================
 * �J�[�g�ɃA�C�e���ǉ�
 *------------------------------------------
 */
int clif_cart_additem(struct map_session_data *sd,unsigned short n,unsigned long amount,unsigned char fail)
{
	int view,j,fd;
	unsigned char *buf;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf=WFIFOP(fd,0);
	if(n>=MAX_CART || sd->status.cart[n].nameid>=MAX_ITEMS)
		return 1;

	WBUFW(buf,0)=0x124;
	WBUFW(buf,2)=n+2;
	WBUFL(buf,4)=amount;
	if((view = itemdb_viewid(sd->status.cart[n].nameid)) > 0)
		WBUFW(buf,8)=view;
	else
		WBUFW(buf,8)=sd->status.cart[n].nameid;
	WBUFB(buf,10)=sd->status.cart[n].identify;
	WBUFB(buf,11)=sd->status.cart[n].attribute;
	WBUFB(buf,12)=sd->status.cart[n].refine;
	if(sd->status.cart[n].card[0]==0x00ff || sd->status.cart[n].card[0]==0x00fe || sd->status.cart[n].card[0]==0xff00) {
		WBUFW(buf,13)=sd->status.cart[n].card[0];
		WBUFW(buf,15)=sd->status.cart[n].card[1];
		WBUFW(buf,17)=sd->status.cart[n].card[2];
		WBUFW(buf,19)=sd->status.cart[n].card[3];
	} else {
		if(sd->status.cart[n].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[0])) > 0)
			WBUFW(buf,13)= j;
		else
			WBUFW(buf,13)= sd->status.cart[n].card[0];
		if(sd->status.cart[n].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[1])) > 0)
			WBUFW(buf,15)= j;
		else
			WBUFW(buf,15)= sd->status.cart[n].card[1];
		if(sd->status.cart[n].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[2])) > 0)
			WBUFW(buf,17)= j;
		else
			WBUFW(buf,17)= sd->status.cart[n].card[2];
		if(sd->status.cart[n].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[3])) > 0)
			WBUFW(buf,19)= j;
		else
			WBUFW(buf,19)= sd->status.cart[n].card[3];
	}
	WFIFOSET(fd,packet_len_table[0x124]);
	return 0;
}

/*==========================================
 * �J�[�g����A�C�e���폜
 *------------------------------------------
 */
int clif_cart_delitem(struct map_session_data *sd,unsigned short n,unsigned long amount)
{
	int fd;
	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x125;
	WFIFOW(fd,2)=n+2;
	WFIFOL(fd,4)=amount;

	WFIFOSET(fd,packet_len_table[0x125]);

	return 0;
}

/*==========================================
 * �J�[�g�̃A�C�e�����X�g
 *------------------------------------------
 */
int clif_cart_itemlist(struct map_session_data *sd)
{
	struct item_data *id;
	int i,n,fd;
	unsigned char *buf;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf = WFIFOP(fd,0);
#if PACKETVER < 5
	WBUFW(buf,0)=0x123;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(itemdb_isequip2(*id))
			continue;
		WBUFW(buf,n*10+4)=i+2;
		if(id->view_id > 0)
			WBUFW(buf,n*10+6)=id->view_id;
		else
			WBUFW(buf,n*10+6)=sd->status.cart[i].nameid;
		WBUFB(buf,n*10+8)=id->type;
		WBUFB(buf,n*10+9)=sd->status.cart[i].identify;
		WBUFW(buf,n*10+10)=sd->status.cart[i].amount;
		WBUFW(buf,n*10+12)=0;
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#else
	WBUFW(buf,0)=0x1ef;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(itemdb_isequip2(*id))
			continue;
		WBUFW(buf,n*18+4)=i+2;
		if(id->view_id > 0)
			WBUFW(buf,n*18+6)=id->view_id;
		else
			WBUFW(buf,n*18+6)=sd->status.cart[i].nameid;
		WBUFB(buf,n*18+8)=id->type;
		WBUFB(buf,n*18+9)=sd->status.cart[i].identify;
		WBUFW(buf,n*18+10)=sd->status.cart[i].amount;
		WBUFW(buf,n*18+12)=0;
		WBUFW(buf,n*18+14)=sd->status.cart[i].card[0];
		WBUFW(buf,n*18+16)=sd->status.cart[i].card[1];
		WBUFW(buf,n*18+18)=sd->status.cart[i].card[2];
		WBUFW(buf,n*18+20)=sd->status.cart[i].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*18;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#endif
	return 0;
}

/*==========================================
 * �J�[�g�̑����i���X�g
 *------------------------------------------
 */
int clif_cart_equiplist(struct map_session_data *sd)
{
	struct item_data *id;
	int i,j,n,fd;
	unsigned char *buf;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf = WFIFOP(fd,0);
	WBUFW(buf,0)=0x122;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(!itemdb_isequip2(*id))
			continue;
		WBUFW(buf,n*20+4)=i+2;
		if(id->view_id > 0)
			WBUFW(buf,n*20+6)=id->view_id;
		else
			WBUFW(buf,n*20+6)=sd->status.cart[i].nameid;
		WBUFB(buf,n*20+8)=id->type;
		WBUFB(buf,n*20+9)=sd->status.cart[i].identify;
		WBUFW(buf,n*20+10)=id->equip;
		WBUFW(buf,n*20+12)=sd->status.cart[i].equip;
		WBUFB(buf,n*20+14)=sd->status.cart[i].attribute;
		WBUFB(buf,n*20+15)=sd->status.cart[i].refine;
		if(sd->status.cart[i].card[0]==0x00ff || sd->status.cart[i].card[0]==0x00fe || sd->status.cart[i].card[0]==0xff00) {
			WBUFW(buf,n*20+16)=sd->status.cart[i].card[0];
			WBUFW(buf,n*20+18)=sd->status.cart[i].card[1];
			WBUFW(buf,n*20+20)=sd->status.cart[i].card[2];
			WBUFW(buf,n*20+22)=sd->status.cart[i].card[3];
		} else {
			if(sd->status.cart[i].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[0])) > 0)
				WBUFW(buf,n*20+16)= j;
			else
				WBUFW(buf,n*20+16)= sd->status.cart[i].card[0];
			if(sd->status.cart[i].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[1])) > 0)
				WBUFW(buf,n*20+18)= j;
			else
				WBUFW(buf,n*20+18)= sd->status.cart[i].card[1];
			if(sd->status.cart[i].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[2])) > 0)
				WBUFW(buf,n*20+20)= j;
			else
				WBUFW(buf,n*20+20)= sd->status.cart[i].card[2];
			if(sd->status.cart[i].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[3])) > 0)
				WBUFW(buf,n*20+22)= j;
			else
				WBUFW(buf,n*20+22)= sd->status.cart[i].card[3];
		}
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * �I�X�J��
 *------------------------------------------
 */
int clif_openvendingreq(struct map_session_data *sd, unsigned short num)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0x12d;
	WFIFOW(fd,2)=num;
	WFIFOSET(fd,packet_len_table[0x12d]);

	return 0;
}

/*==========================================
 * �I�X�Ŕ\��
 *------------------------------------------
 */
int clif_showvendingboard(struct block_list* bl,const char *message,int fd)
{
	unsigned char buf[128];

	nullpo_retr(0, bl);

	WBUFW(buf,0)=0x131;
	WBUFL(buf,2)=bl->id;
	memcpy((char*)WBUFP(buf,6),message,80);
	if(fd && session_isActive(fd) ) {
		memcpy(WFIFOP(fd,0),buf,packet_len_table[0x131]);
		WFIFOSET(fd,packet_len_table[0x131]);
	}else{
		clif_send(buf,packet_len_table[0x131],bl,AREA_WOS);
	}
	return 0;
}

/*==========================================
 * �I�X�Ŕ���
 *------------------------------------------
 */
int clif_closevendingboard(struct block_list* bl,int fd)
{
	unsigned char buf[16];

	nullpo_retr(0, bl);

	WBUFW(buf,0)=0x132;
	WBUFL(buf,2)=bl->id;
	if(fd && session_isActive(fd) ) {
		memcpy(WFIFOP(fd,0),buf,packet_len_table[0x132]);
		WFIFOSET(fd,packet_len_table[0x132]);
	}else{
		clif_send(buf,packet_len_table[0x132],bl,AREA_WOS);
	}

	return 0;
}
/*==========================================
 * �I�X�A�C�e�����X�g
 *------------------------------------------
 */
int clif_vendinglist(struct map_session_data *sd,unsigned long id,struct vending *vending)
{
	struct item_data *data;
	int i,j,n,index,fd;
	struct map_session_data *vsd;
	unsigned char *buf;

	nullpo_retr(0, sd);
	nullpo_retr(0, vending);
	nullpo_retr(0, vsd=map_id2sd(id));

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf = WFIFOP(fd,0);
	WBUFW(buf,0)=0x133;
	WBUFL(buf,4)=id;
	for(i=0,n=0;i<vsd->vend_num;i++){
		if(vending[i].amount<=0)
			continue;
		WBUFL(buf,8+n*22)=vending[i].value;
		WBUFW(buf,12+n*22)=vending[i].amount;
		WBUFW(buf,14+n*22)=(index=vending[i].index)+2;
		if(vsd->status.cart[index].nameid <= 0 || vsd->status.cart[index].amount <= 0)
			continue;
		data = itemdb_search(vsd->status.cart[index].nameid);
		WBUFB(buf,16+n*22)=data->type;
		if(data->view_id > 0)
			WBUFW(buf,17+n*22)=data->view_id;
		else
			WBUFW(buf,17+n*22)=vsd->status.cart[index].nameid;
		WBUFB(buf,19+n*22)=vsd->status.cart[index].identify;
		WBUFB(buf,20+n*22)=vsd->status.cart[index].attribute;
		WBUFB(buf,21+n*22)=vsd->status.cart[index].refine;
		if(vsd->status.cart[index].card[0]==0x00ff || vsd->status.cart[index].card[0]==0x00fe || vsd->status.cart[index].card[0]==0xff00) {
			WBUFW(buf,22+n*22)=vsd->status.cart[index].card[0];
			WBUFW(buf,24+n*22)=vsd->status.cart[index].card[1];
			WBUFW(buf,26+n*22)=vsd->status.cart[index].card[2];
			WBUFW(buf,28+n*22)=vsd->status.cart[index].card[3];
		} else {
			if(vsd->status.cart[index].card[0] > 0 && (j=itemdb_viewid(vsd->status.cart[index].card[0])) > 0)
				WBUFW(buf,22+n*22)= j;
			else
				WBUFW(buf,22+n*22)= vsd->status.cart[index].card[0];
			if(vsd->status.cart[index].card[1] > 0 && (j=itemdb_viewid(vsd->status.cart[index].card[1])) > 0)
				WBUFW(buf,24+n*22)= j;
			else
				WBUFW(buf,24+n*22)= vsd->status.cart[index].card[1];
			if(vsd->status.cart[index].card[2] > 0 && (j=itemdb_viewid(vsd->status.cart[index].card[2])) > 0)
				WBUFW(buf,26+n*22)= j;
			else
				WBUFW(buf,26+n*22)= vsd->status.cart[index].card[2];
			if(vsd->status.cart[index].card[3] > 0 && (j=itemdb_viewid(vsd->status.cart[index].card[3])) > 0)
				WBUFW(buf,28+n*22)= j;
			else
				WBUFW(buf,28+n*22)= vsd->status.cart[index].card[3];
		}
		n++;
	}
	if(n > 0){
		WBUFW(buf,2)=8+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}

	return 0;
}

/*==========================================
 * �I�X�A�C�e���w�����s
 *------------------------------------------
*/
int clif_buyvending(struct map_session_data *sd,unsigned short index,unsigned short amount,unsigned char fail)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0x135;
	WFIFOW(fd,2)=index+2;
	WFIFOW(fd,4)=amount;
	WFIFOB(fd,6)=fail;
	WFIFOSET(fd,packet_len_table[0x135]);

	return 0;
}

/*==========================================
 * �I�X�J�ݐ���
 *------------------------------------------
*/
int clif_openvending(struct map_session_data *sd,unsigned long id,struct vending *vending)
{
	struct item_data *data;
	int i,j,n,index,fd;
	unsigned char *buf;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0x136;
	WBUFL(buf,4)=id;
	for(i=0,n=0;i<sd->vend_num;i++){
		if (sd->vend_num > 2+pc_checkskill(*sd,MC_VENDING)) return 0;
		WBUFL(buf,8+n*22)=vending[i].value;
		WBUFW(buf,12+n*22)=(index=vending[i].index)+2;
		WBUFW(buf,14+n*22)=vending[i].amount;
		if(sd->status.cart[index].nameid <= 0 || sd->status.cart[index].amount <= 0 || sd->status.cart[index].identify==0 ||
			sd->status.cart[index].attribute==1) // Prevent unidentified and broken items from being sold [Valaris]
			continue;
		data = itemdb_search(sd->status.cart[index].nameid);
		WBUFB(buf,16+n*22)=data->type;
		if(data->view_id > 0)
			WBUFW(buf,17+n*22)=data->view_id;
		else
			WBUFW(buf,17+n*22)=sd->status.cart[index].nameid;
		WBUFB(buf,19+n*22)=sd->status.cart[index].identify;
		WBUFB(buf,20+n*22)=sd->status.cart[index].attribute;
		WBUFB(buf,21+n*22)=sd->status.cart[index].refine;
		if(sd->status.cart[index].card[0]==0x00ff || sd->status.cart[index].card[0]==0x00fe || sd->status.cart[index].card[0]==0xff00) {
			WBUFW(buf,22+n*22)=sd->status.cart[index].card[0];
			WBUFW(buf,24+n*22)=sd->status.cart[index].card[1];
			WBUFW(buf,26+n*22)=sd->status.cart[index].card[2];
			WBUFW(buf,28+n*22)=sd->status.cart[index].card[3];
		} else {
			if(sd->status.cart[index].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[index].card[0])) > 0)
				WBUFW(buf,22+n*22)= j;
			else
				WBUFW(buf,22+n*22)= sd->status.cart[index].card[0];
			if(sd->status.cart[index].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[index].card[1])) > 0)
				WBUFW(buf,24+n*22)= j;
			else
				WBUFW(buf,24+n*22)= sd->status.cart[index].card[1];
			if(sd->status.cart[index].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[index].card[2])) > 0)
				WBUFW(buf,26+n*22)= j;
			else
				WBUFW(buf,26+n*22)= sd->status.cart[index].card[2];
			if(sd->status.cart[index].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[index].card[3])) > 0)
				WBUFW(buf,28+n*22)= j;
			else
				WBUFW(buf,28+n*22)= sd->status.cart[index].card[3];
		}
		n++;
	}
	if(n > 0){
		WBUFW(buf,2)=8+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}

	return n;
}

/*==========================================
 * �I�X�A�C�e���̔���
 *------------------------------------------
*/
int clif_vendingreport(struct map_session_data *sd,unsigned short index,unsigned short amount)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0x137;
	WFIFOW(fd,2)=index+2;
	WFIFOW(fd,4)=amount;
	WFIFOSET(fd,packet_len_table[0x137]);

	return 0;
}

/*==========================================
 * �p�[�e�B�쐬����
 *------------------------------------------
 */
int clif_party_created(struct map_session_data *sd,unsigned char flag)
{
	int fd;

	// ShowMessage("clif_party_message(%s, %d, %s)\n", p->name, account_id, mes);

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xfa;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0xfa]);
	return 0;
}
/*==========================================
 * �p�[�e�B��񑗐M
 *------------------------------------------
 */
int clif_party_info(struct party *p,int fd)
{
	unsigned char buf[1024];
	int i,c;
	struct map_session_data *sd=NULL;

	nullpo_retr(0, p);

	WBUFW(buf,0)=0xfb;
	memcpy(WBUFP(buf,4),p->name,24);
	for(i=c=0;i<MAX_PARTY;i++){
		struct party_member *m=&p->member[i];
		if(m->account_id>0){
			if(sd==NULL) sd=m->sd;
			WBUFL(buf,28+c*46)=m->account_id;
			memcpy(WBUFP(buf,28+c*46+ 4),m->name,24);
			memcpy(WBUFP(buf,28+c*46+28),m->map,16);
			WBUFB(buf,28+c*46+44)=(m->leader)?0:1;
			WBUFB(buf,28+c*46+45)=(m->online)?0:1;
			c++;
		}
	}
	WBUFW(buf,2)=28+c*46;
	if(fd>=0 && session_isActive(fd) ) {	// fd���ݒ肳��Ă�Ȃ炻��ɑ���

		memcpy(WFIFOP(fd,0),buf,WBUFW(buf,2));
		WFIFOSET(fd,WFIFOW(fd,2));
		return 0;
	}
	if(sd!=NULL)
		clif_send(buf,WBUFW(buf,2),&sd->bl,PARTY);
	return 0;
}
/*==========================================
 * �p�[�e�B���U
 *------------------------------------------
 */
int clif_party_invite(struct map_session_data *sd,struct map_session_data *tsd)
{
	int fd;
	struct party *p;

	nullpo_retr(0, sd);
	nullpo_retr(0, tsd);

	fd=tsd->fd;
	if( !session_isActive(fd) )
		return 0;

	if( (p=party_search(sd->status.party_id))==NULL )
		return 0;

	WFIFOW(fd,0)=0xfe;
	WFIFOL(fd,2)=sd->status.account_id;
	memcpy(WFIFOP(fd,6),p->name,24);
	WFIFOSET(fd,packet_len_table[0xfe]);
	return 0;
}

/*==========================================
 * �p�[�e�B���U����
 *------------------------------------------
 */
int clif_party_inviteack(struct map_session_data *sd,char *nick,unsigned char flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;
	
	WFIFOW(fd,0)=0xfd;
	memcpy(WFIFOP(fd,2),nick,24);
	WFIFOB(fd,26)=flag;
	WFIFOSET(fd,packet_len_table[0xfd]);
	return 0;
}

/*==========================================
 * �p�[�e�B�ݒ著�M
 * flag & 0x001=exp�ύX�~�X
 *        0x010=item�ύX�~�X
 *        0x100=��l�ɂ̂ݑ��M
 *------------------------------------------
 */
int clif_party_option(struct party *p,struct map_session_data *sd,int flag)
{
	unsigned char buf[16];

	nullpo_retr(0, p);

//	if(battle_config.etc_log)
//		ShowMessage("clif_party_option: %d %d %d\n",p->exp,p->item,flag);
	if(sd==NULL && flag==0){
		int i;
		for(i=0;i<MAX_PARTY;i++)
			if((sd=map_id2sd(p->member[i].account_id))!=NULL)
				break;
	}

	WBUFW(buf,0)=0x101;
	WBUFW(buf,2)=((flag&0x01)?2:p->exp);
	WBUFW(buf,4)=((flag&0x10)?2:p->item);
	if(flag==0)
		clif_send(buf,packet_len_table[0x101],&sd->bl,PARTY);
	else if( sd && session_isActive(sd->fd) ) {
		memcpy(WFIFOP(sd->fd,0),buf,packet_len_table[0x101]);
		WFIFOSET(sd->fd,packet_len_table[0x101]);
	}
	return 0;
}
/*==========================================
 * �p�[�e�B�E�ށi�E�ޑO�ɌĂԂ��Ɓj
 *------------------------------------------
 */
int clif_party_leaved(struct party *p,struct map_session_data *sd,unsigned long account_id,char *name,unsigned char flag)
{
	unsigned char buf[64];
	int i;

	nullpo_retr(0, p);

	WBUFW(buf,0)=0x105;
	WBUFL(buf,2)=account_id;
	memcpy(WBUFP(buf,6),name,24);
	WBUFB(buf,30)=flag&0x0f;

	if(sd==NULL && (flag&0xf0)==0){
			for(i=0;i<MAX_PARTY;i++)
				if((sd=p->member[i].sd)!=NULL)
					break;

	if(flag==0)
			clif_send(buf,packet_len_table[0x105],&sd->bl,PARTY);
	} else if( sd && session_isActive(sd->fd) ) {
		memcpy(WFIFOP(sd->fd,0),buf,packet_len_table[0x105]);
		WFIFOSET(sd->fd,packet_len_table[0x105]);
	}
	return 0;
}
/*==========================================
 * �p�[�e�B���b�Z�[�W���M
 *------------------------------------------
 */
int clif_party_message(struct party *p,unsigned long account_id, const char *mes,size_t len)
{
	struct map_session_data *sd=NULL;
	int i;

	nullpo_retr(0, p);
	// test for someone in the party
	for(i=0;i<MAX_PARTY;i++)
		if((sd=p->member[i].sd)!=NULL)
			break;

	if(sd!=NULL)
	{
		unsigned char buf[1024];
		WBUFW(buf,0)=0x109;
		WBUFW(buf,2)=len+8;
		WBUFL(buf,4)=account_id;
		memcpy(WBUFP(buf,8),mes,len);
		clif_send(buf,len+8,&sd->bl,PARTY);
	}
	return 0;
}
/*==========================================
 * �p�[�e�B���W�ʒm
 *------------------------------------------
 */
int clif_party_xy(struct party *p,struct map_session_data *sd)
{
	unsigned char buf[16];

	nullpo_retr(0, sd);

	WBUFW(buf,0)=0x107;
	WBUFL(buf,2)=sd->status.account_id;
	WBUFW(buf,6)=sd->bl.x;
	WBUFW(buf,8)=sd->bl.y;
	return clif_send(buf,packet_len_table[0x107],&sd->bl,PARTY_SAMEMAP_WOS);
}
/*==========================================
 * �p�[�e�BHP�ʒm
 *------------------------------------------
 */
int clif_party_hp(struct party *p,struct map_session_data *sd)
{
	unsigned char buf[16];

	nullpo_retr(0, sd);

	WBUFW(buf,0)=0x106;
	WBUFL(buf,2)=sd->status.account_id;
	WBUFW(buf,6)=(unsigned short)((sd->status.hp > 0x7fff)? 0x7fff:sd->status.hp);
	WBUFW(buf,8)=(unsigned short)((sd->status.max_hp > 0x7fff)? 0x7fff:sd->status.max_hp);
	return clif_send(buf,packet_len_table[0x106],&sd->bl,PARTY_AREA_WOS);
}
/*==========================================
 * GM�֏ꏊ��HP�ʒm
 *------------------------------------------
 */
int clif_hpmeter(struct map_session_data *sd)
{
	struct map_session_data *sd2;
	unsigned char buf1[16];
	unsigned char buf2[16];
	size_t i;
	int x0, y0, x1, y1;

	nullpo_retr(0, sd);

	WBUFW(buf1,0) = 0x107;
	WBUFL(buf1,2) = sd->bl.id;
	WBUFW(buf1,6) = sd->bl.x;
	WBUFW(buf1,8) = sd->bl.y;

	WBUFW(buf2,0) = 0x106;
	WBUFL(buf2,2) = sd->status.account_id;
	WBUFW(buf2,6) = (unsigned short)((sd->status.hp > 0x7fff) ? 0x7fff : sd->status.hp);
	WBUFW(buf2,8) = (unsigned short)((sd->status.max_hp > 0x7fff) ? 0x7fff : sd->status.max_hp);

	x0 = sd->bl.x - AREA_SIZE;
	y0 = sd->bl.y - AREA_SIZE;
	x1 = sd->bl.x + AREA_SIZE;
	y1 = sd->bl.y + AREA_SIZE;

	for (i = 0; i < fd_max; i++)
	{
		if( session[i] && 
			(sd2 = (struct map_session_data*)session[i]->session_data) &&  
			sd2->bl.m == sd->bl.m &&
			sd2->bl.x > x0 && sd2->bl.x < x1 &&
			sd2->bl.y > y0 && sd2->bl.y < y1 &&
			pc_isGM(*sd2) >= pc_isGM(*sd) &&
			sd != sd2 && 
			sd2->state.auth)
		{
			memcpy(WFIFOP(i,0), buf1, packet_len_table[0x107]);
			WFIFOSET (i, packet_len_table[0x107]);

			memcpy (WFIFOP(i,0), buf2, packet_len_table[0x106]);
			WFIFOSET (i, packet_len_table[0x106]);
		}
	}
	return 0;
}
/*==================================================
 * Update monster hp view if it has changed [Celest]
 *--------------------------------------------------
 */
int clif_update_mobhp(struct mob_data *md)
{
	unsigned char buf[102];
	char mobhp[50];

	nullpo_retr(0, md);

	memset(buf,0,102);
	WBUFW(buf,0) = 0x95;
	WBUFL(buf,2) = md->bl.id;

	memcpy(WBUFP(buf,6), md->name, 24);
	sprintf(mobhp, "hp: %d/%d", md->hp, mob_db[md->class_].max_hp);

	WBUFW(buf, 0) = 0x195;
	memcpy(WBUFP(buf,30), mobhp, 24);
	WBUFL(buf,54) = 0;
	WBUFL(buf,78) = 0;
	return clif_send(buf,packet_len_table[0x195],&md->bl,AREA);
}
/*==========================================
 * �p�[�e�B�ꏊ�ړ��i���g�p�j
 *------------------------------------------
 */
int clif_party_move(struct party *p,struct map_session_data *sd,unsigned char online)
{
	unsigned char buf[128];

	nullpo_retr(0, sd);
	nullpo_retr(0, p);

	WBUFW(buf, 0)=0x104;
	WBUFL(buf, 2)=sd->status.account_id;
	WBUFL(buf, 6)=0;
	WBUFW(buf,10)=sd->bl.x;
	WBUFW(buf,12)=sd->bl.y;
	WBUFB(buf,14)=!online;
	memcpy(WBUFP(buf,15),p->name,24);
	memcpy(WBUFP(buf,39),sd->status.name,24);
	memcpy(WBUFP(buf,63),map[sd->bl.m].mapname,16);
	return clif_send(buf,packet_len_table[0x104],&sd->bl,PARTY);
}
/*==========================================
 * �U�����邽�߂Ɉړ����K�v
 *------------------------------------------
 */
int clif_movetoattack(struct map_session_data *sd,struct block_list *bl)
{
	int fd;

	nullpo_retr(0, sd);
	nullpo_retr(0, bl);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd, 0)=0x139;
	WFIFOL(fd, 2)=bl->id;
	WFIFOW(fd, 6)=bl->x;
	WFIFOW(fd, 8)=bl->y;
	WFIFOW(fd,10)=sd->bl.x;
	WFIFOW(fd,12)=sd->bl.y;
	WFIFOW(fd,14)=sd->attackrange;
	WFIFOSET(fd,packet_len_table[0x139]);
	return 0;
}
/*==========================================
 * �����G�t�F�N�g
 *------------------------------------------
 */
int clif_produceeffect(struct map_session_data *sd,unsigned short flag,unsigned short nameid)
{
	unsigned short view;
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	// ���O�̓o�^�Ƒ��M���ɂ��Ă���
	if( map_charid2nick(sd->status.char_id)==NULL )
		map_addchariddb(sd->status.char_id,sd->status.name);
	clif_solved_charname(sd,sd->status.char_id);

	WFIFOW(fd, 0)=0x18f;
	WFIFOW(fd, 2)=flag;
	if((view = itemdb_viewid(nameid)) > 0)
		WFIFOW(fd, 4)=view;
	else
		WFIFOW(fd, 4)=nameid;
	WFIFOSET(fd,packet_len_table[0x18f]);
	return 0;
}

// pet
int clif_catch_process(struct map_session_data *sd)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x19e;
	WFIFOSET(fd,packet_len_table[0x19e]);

	return 0;
}

int clif_pet_rulet(struct map_session_data *sd,unsigned char data)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x1a0;
	WFIFOB(fd,2)=data;
	WFIFOSET(fd,packet_len_table[0x1a0]);

	return 0;
}

/*==========================================
 * pet�����X�g�쐬
 *------------------------------------------
 */
int clif_sendegg(struct map_session_data *sd)
{
	//R 01a6 <len>.w <index>.w*
	int i,n=0,fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x1a6;
	if(sd->status.pet_id <= 0) {
		for(i=0,n=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL ||
			   sd->inventory_data[i]->type!=7 ||
			   sd->status.inventory[i].amount<=0)
				continue;
			WFIFOW(fd,n*2+4)=i+2;
			n++;
		}
	}
	WFIFOW(fd,2)=4+n*2;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

int clif_send_petdata(struct map_session_data *sd,unsigned char type,unsigned long param)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x1a4;
	WFIFOB(fd,2)=type;
	WFIFOL(fd,3)=sd->pd->bl.id;
	WFIFOL(fd,7)=param;
	WFIFOSET(fd,packet_len_table[0x1a4]);

	return 0;
}

int clif_send_petstatus(struct map_session_data *sd)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x1a2;
	memcpy(WFIFOP(fd,2),sd->pet.name,24);
	WFIFOB(fd,26)=(battle_config.pet_rename == 1)? 0:sd->pet.rename_flag;
	WFIFOW(fd,27)=sd->pet.level;
	WFIFOW(fd,29)=sd->pet.hungry;
	WFIFOW(fd,31)=sd->pet.intimate;
	WFIFOW(fd,33)=sd->pet.equip_id;
	WFIFOSET(fd,packet_len_table[0x1a2]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pet_emotion(struct pet_data *pd,unsigned long param)
{
	unsigned char buf[16];
	struct map_session_data *sd;

	nullpo_retr(0, pd);
	nullpo_retr(0, sd = pd->msd);

	memset(buf,0,packet_len_table[0x1aa]);

	WBUFW(buf,0)=0x1aa;
	WBUFL(buf,2)=pd->bl.id;
	if(param >= 100 && sd->petDB->talk_convert_class) {
		if(sd->petDB->talk_convert_class < 0)
			return 0;
		else if(sd->petDB->talk_convert_class > 0) {
			param -= (pd->class_ - 100)*100;
			param += (sd->petDB->talk_convert_class - 100)*100;
		}
	}
	WBUFL(buf,6)=param;

	return clif_send(buf,packet_len_table[0x1aa],&pd->bl,AREA);
}

int clif_pet_performance(struct block_list *bl,unsigned long param)
{
	unsigned char buf[16];

	nullpo_retr(0, bl);

	memset(buf,0,packet_len_table[0x1a4]);

	WBUFW(buf,0)=0x1a4;
	WBUFB(buf,2)=4;
	WBUFL(buf,3)=bl->id;
	WBUFL(buf,7)=param;

	return clif_send(buf,packet_len_table[0x1a4],bl,AREA);
}

int clif_pet_equip(struct pet_data *pd,unsigned short nameid)
{
	unsigned char buf[16];
	unsigned short view;

	nullpo_retr(0, pd);

	memset(buf,0,packet_len_table[0x1a4]);

	WBUFW(buf,0)=0x1a4;
	WBUFB(buf,2)=3;
	WBUFL(buf,3)=pd->bl.id;
	if((view = itemdb_viewid(nameid)) > 0)
		WBUFL(buf,7)=view;
	else
		WBUFL(buf,7)=nameid;

	return clif_send(buf,packet_len_table[0x1a4],&pd->bl,AREA);
}

int clif_pet_food(struct map_session_data *sd,unsigned short foodid,unsigned char fail)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x1a3;
	WFIFOB(fd,2)=fail;
	WFIFOW(fd,3)=foodid;
	WFIFOSET(fd,packet_len_table[0x1a3]);

	return 0;
}

/*==========================================
 * �I�[�g�X�y�� ���X�g���M
 *------------------------------------------
 */
int clif_autospell(struct map_session_data *sd,unsigned short skilllv)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd, 0)=0x1cd;

	if(skilllv>0 && pc_checkskill(*sd,MG_NAPALMBEAT)>0)
		WFIFOL(fd,2)= MG_NAPALMBEAT;
	else
		WFIFOL(fd,2)= 0x00000000;
	if(skilllv>1 && pc_checkskill(*sd,MG_COLDBOLT)>0)
		WFIFOL(fd,6)= MG_COLDBOLT;
	else
		WFIFOL(fd,6)= 0x00000000;
	if(skilllv>1 && pc_checkskill(*sd,MG_FIREBOLT)>0)
		WFIFOL(fd,10)= MG_FIREBOLT;
	else
		WFIFOL(fd,10)= 0x00000000;
	if(skilllv>1 && pc_checkskill(*sd,MG_LIGHTNINGBOLT)>0)
		WFIFOL(fd,14)= MG_LIGHTNINGBOLT;
	else
		WFIFOL(fd,14)= 0x00000000;
	if(skilllv>4 && pc_checkskill(*sd,MG_SOULSTRIKE)>0)
		WFIFOL(fd,18)= MG_SOULSTRIKE;
	else
		WFIFOL(fd,18)= 0x00000000;
	if(skilllv>7 && pc_checkskill(*sd,MG_FIREBALL)>0)
		WFIFOL(fd,22)= MG_FIREBALL;
	else
		WFIFOL(fd,22)= 0x00000000;
	if(skilllv>9 && pc_checkskill(*sd,MG_FROSTDIVER)>0)
		WFIFOL(fd,26)= MG_FROSTDIVER;
	else
		WFIFOL(fd,26)= 0x00000000;

	WFIFOSET(fd,packet_len_table[0x1cd]);
	return 0;
}

/*==========================================
 * �f�B�{�[�V�����̐���
 *------------------------------------------
 */
int clif_devotion(struct map_session_data *sd,unsigned long target_id)
{
	unsigned char buf[56];
	int n;

	nullpo_retr(0, sd);

	WBUFW(buf,0)=0x1cf;
	WBUFL(buf,2)=sd->bl.id;
//	WBUFL(buf,6)=target;
	for(n=0;n<5;n++)
		WBUFL(buf,6+4*n)=sd->dev.val2[n];
//		WBUFL(buf,10+4*n)=0;
	WBUFB(buf,26)=8;
	WBUFB(buf,27)=0;

	return clif_send(buf,packet_len_table[0x1cf],&sd->bl,AREA);
}

/*==========================================
 * ����
 *------------------------------------------
 */
int clif_spiritball(struct map_session_data *sd)
{
	unsigned char buf[16];

	nullpo_retr(0, sd);

	WBUFW(buf,0)=0x1d0;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->spiritball;
	return clif_send(buf,packet_len_table[0x1d0],&sd->bl,AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_combo_delay(struct block_list *bl,unsigned long wait)
{
	unsigned char buf[32];

	nullpo_retr(0, bl);

	WBUFW(buf,0)=0x1d2;
	WBUFL(buf,2)=bl->id;
	WBUFL(buf,6)=wait;
	return clif_send(buf,packet_len_table[0x1d2],bl,AREA);
}
/*==========================================
 *���n���
 *------------------------------------------
 */
int clif_bladestop(struct block_list *src,struct block_list *dst, unsigned long _bool)
{
	unsigned char buf[32];

	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	WBUFW(buf,0)=0x1d1;
	WBUFL(buf,2)=src->id;
	WBUFL(buf,6)=dst->id;
	WBUFL(buf,10)=_bool;

	return clif_send(buf,packet_len_table[0x1d1],src,AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapcell(unsigned short m,unsigned short x,unsigned short y,unsigned short cell_type,int type)
{
	struct block_list bl;
	unsigned char buf[32];

	bl.m = m;
	bl.x = x;
	bl.y = y;
	WBUFW(buf,0) = 0x192;
	WBUFW(buf,2) = x;
	WBUFW(buf,4) = y;
	WBUFW(buf,6) = cell_type;
	memcpy(WBUFP(buf,8),map[m].mapname,16);
	return clif_send(buf,packet_len_table[0x192],&bl, (!type)?AREA:ALL_SAMEMAP);
}

/*==========================================
 * MVP�G�t�F�N�g
 *------------------------------------------
 */
int clif_mvp_effect(struct map_session_data *sd)
{
	unsigned char buf[16];

	nullpo_retr(0, sd);

	WBUFW(buf,0)=0x10c;
	WBUFL(buf,2)=sd->bl.id;
	return clif_send(buf,packet_len_table[0x10c],&sd->bl,AREA);
}
/*==========================================
 * MVP�A�C�e������
 *------------------------------------------
 */
int clif_mvp_item(struct map_session_data *sd,unsigned short nameid)
{
	unsigned short view;
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x10a;
	if((view = itemdb_viewid(nameid)) > 0)
		WFIFOW(fd,2)=view;
	else
		WFIFOW(fd,2)=nameid;
	WFIFOSET(fd,packet_len_table[0x10a]);
	return 0;
}
/*==========================================
 * MVP�o���l����
 *------------------------------------------
 */
int clif_mvp_exp(struct map_session_data *sd, unsigned long exp)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x10b;
	WFIFOL(fd,2)=exp;
	WFIFOSET(fd,packet_len_table[0x10b]);
	return 0;
}

/*==========================================
 * �M���h�쐬�ےʒm
 *------------------------------------------
 */
int clif_guild_created(struct map_session_data *sd,unsigned char flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x167;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x167]);
	return 0;
}
/*==========================================
 * �M���h�����ʒm
 *------------------------------------------
 */
int clif_guild_belonginfo(struct map_session_data *sd,struct guild *g)
{
	int ps,fd;

	nullpo_retr(0, sd);
	nullpo_retr(0, g);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	ps=guild_getposition(*sd,*g);

	memset(WFIFOP(fd,0),0,packet_len_table[0x16c]);
	WFIFOW(fd,0)=0x16c;
	WFIFOL(fd,2)=g->guild_id;
	WFIFOL(fd,6)=g->emblem_id;
	WFIFOL(fd,10)=g->position[ps].mode;
	memcpy(WFIFOP(fd,19),g->name,24);
	WFIFOSET(fd,packet_len_table[0x16c]);
	return 0;
}
/*==========================================
 * �M���h�����o���O�C���ʒm
 *------------------------------------------
 */
int clif_guild_memberlogin_notice(struct guild *g,unsigned long idx,unsigned long flag)
{
	unsigned char buf[64];

	nullpo_retr(0, g);

	if( idx>=MAX_GUILD )
		return 0;

	WBUFW(buf, 0)=0x16d;
	WBUFL(buf, 2)=g->member[idx].account_id;
	WBUFL(buf, 6)=g->member[idx].char_id;
	WBUFL(buf,10)=flag;
	if(g->member[idx].sd==NULL){
		struct map_session_data *sd=guild_getavailablesd(*g);
		if(sd!=NULL)
			clif_send(buf,packet_len_table[0x16d],&sd->bl,GUILD);
	}else
		clif_send(buf,packet_len_table[0x16d],&g->member[idx].sd->bl,GUILD_WOS);
	return 0;
}
/*==========================================
 * �M���h�}�X�^�[�ʒm(14d�ւ̉���)
 *------------------------------------------
 */
int clif_guild_masterormember(struct map_session_data *sd)
{
	int type=0x57,fd;
	struct guild *g;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	g=guild_search(sd->status.guild_id);
	if(g!=NULL && strcmp(g->master,sd->status.name)==0)
		type=0xd7;
	WFIFOW(fd,0)=0x14e;
	WFIFOL(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0x14e]);
	return 0;
}
/*==========================================
 * Basic Info (Territories [Valaris])
 *------------------------------------------
 */
int clif_guild_basicinfo(struct map_session_data *sd)
{
	int fd,i,t=0;
	struct guild *g;
	struct guild_castle *gc=NULL;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;

	WFIFOW(fd, 0)=0x1b6;//0x150;
	WFIFOL(fd, 2)=g->guild_id;
	WFIFOL(fd, 6)=g->guild_lv;
	WFIFOL(fd,10)=g->connect_member;
	WFIFOL(fd,14)=g->max_member;
	WFIFOL(fd,18)=g->average_lv;
	WFIFOL(fd,22)=g->exp;
	WFIFOL(fd,26)=g->next_exp;
	WFIFOL(fd,30)=0;	// ��[
	WFIFOL(fd,34)=0;	// VW�i���i�̈����H�F�����O���t���E�j
	WFIFOL(fd,38)=0;	// RF�i���`�̓x�����H�F�����O���t�㉺�j
	WFIFOL(fd,42)=0;	// �l���H
	memcpy(WFIFOP(fd,46),g->name,24);
	memcpy(WFIFOP(fd,70),g->master,24);

	for(i=0;i<MAX_GUILDCASTLE;i++){
		gc=guild_castle_search(i);
		if(!gc) continue;
			if(g->guild_id == gc->guild_id)	t++;
	}

	if     (t==1)  memcpy(WFIFOP(fd,94),"One Castle",20);
	else if(t==2)  memcpy(WFIFOP(fd,94),"Two Castles",20);
	else if(t==3)  memcpy(WFIFOP(fd,94),"Three Castles",20);
	else if(t==4)  memcpy(WFIFOP(fd,94),"Four Castles",20);
	else if(t==5)  memcpy(WFIFOP(fd,94),"Five Castles",20);
	else if(t==6)  memcpy(WFIFOP(fd,94),"Six Castles",20);
	else if(t==7)  memcpy(WFIFOP(fd,94),"Seven Castles",20);
	else if(t==8)  memcpy(WFIFOP(fd,94),"Eight Castles",20);
	else if(t==9)  memcpy(WFIFOP(fd,94),"Nine Castles",20);
	else if(t==10) memcpy(WFIFOP(fd,94),"Ten Castles",20);
	else if(t==11) memcpy(WFIFOP(fd,94),"Eleven Castles",20);
	else if(t==12) memcpy(WFIFOP(fd,94),"Twelve Castles",20);
	else if(t==13) memcpy(WFIFOP(fd,94),"Thirteen Castles",20);
	else if(t==14) memcpy(WFIFOP(fd,94),"Fourteen Castles",20);
	else if(t==15) memcpy(WFIFOP(fd,94),"Fifteen Castles",20);
	else if(t==16) memcpy(WFIFOP(fd,94),"Sixteen Castles",20);
	else if(t==17) memcpy(WFIFOP(fd,94),"Seventeen Castles",20);
	else if(t==18) memcpy(WFIFOP(fd,94),"Eighteen Castles",20);
	else if(t==19) memcpy(WFIFOP(fd,94),"Nineteen Castles",20);
	else if(t==20) memcpy(WFIFOP(fd,94),"Twenty Castles",20);
	else if(t==21) memcpy(WFIFOP(fd,94),"Twenty One Castles",20);
	else if(t==22) memcpy(WFIFOP(fd,94),"Twenty Two Castles",20);
	else if(t==23) memcpy(WFIFOP(fd,94),"Twenty Three Castles",20);
	else if(t==24) memcpy(WFIFOP(fd,94),"Twenty Four Castles",20);
	else if(t==MAX_GUILDCASTLE) memcpy(WFIFOP(fd,94),"Total Domination",20);
	else memcpy(WFIFOP(fd,94),"None Taken",20);

	WFIFOSET(fd,packet_len_table[WFIFOW(fd,0)]);
	clif_guild_emblem(sd,g);	// Guild emblem vanish fix [Valaris]
	return 0;
}

/*==========================================
 * �M���h����/�G�Ώ��
 *------------------------------------------
 */
int clif_guild_allianceinfo(struct map_session_data *sd)
{
	int fd,i,c;
	struct guild *g;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd, 0)=0x14c;
	for(i=c=0;i<MAX_GUILDALLIANCE;i++){
		struct guild_alliance *a=&g->alliance[i];
		if(a->guild_id>0){
			WFIFOL(fd,c*32+4)=a->opposition;
			WFIFOL(fd,c*32+8)=a->guild_id;
			memcpy(WFIFOP(fd,c*32+12),a->name,24);
			c++;
		}
	}
	WFIFOW(fd, 2)=c*32+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * �M���h�����o�[���X�g
 *------------------------------------------
 */
int clif_guild_memberlist(struct map_session_data *sd)
{
	int fd;
	int i,c;
	struct guild *g;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;

	WFIFOW(fd, 0)=0x154;
	for(i=0,c=0;i<g->max_member;i++){
		struct guild_member *m=&g->member[i];
		if(m->account_id==0)
			continue;
		WFIFOL(fd,c*104+ 4)=m->account_id;
		WFIFOL(fd,c*104+ 8)=m->char_id;
		WFIFOW(fd,c*104+12)=m->hair;
		WFIFOW(fd,c*104+14)=m->hair_color;
		WFIFOW(fd,c*104+16)=m->gender;
		WFIFOW(fd,c*104+18)=m->class_;
		WFIFOW(fd,c*104+20)=m->lv;
		WFIFOL(fd,c*104+22)=m->exp;
		WFIFOL(fd,c*104+26)=m->online;
		WFIFOL(fd,c*104+30)=m->position;
		memset(WFIFOP(fd,c*104+34),0,50);	// �����H
		memcpy(WFIFOP(fd,c*104+84),m->name,24);
		c++;
	}
	WFIFOW(fd, 2)=c*104+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * �M���h��E�����X�g
 *------------------------------------------
 */
int clif_guild_positionnamelist(struct map_session_data *sd)
{
	int i,fd;
	struct guild *g;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd, 0)=0x166;
	for(i=0;i<MAX_GUILDPOSITION;i++){
		WFIFOL(fd,i*28+4)=i;
		memcpy(WFIFOP(fd,i*28+8),g->position[i].name,24);
	}
	WFIFOW(fd,2)=i*28+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * �M���h��E��񃊃X�g
 *------------------------------------------
 */
int clif_guild_positioninfolist(struct map_session_data *sd)
{
	int i,fd;
	struct guild *g;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd, 0)=0x160;
	for(i=0;i<MAX_GUILDPOSITION;i++){
		struct guild_position *p=&g->position[i];
		WFIFOL(fd,i*16+ 4)=i;
		WFIFOL(fd,i*16+ 8)=p->mode;
		WFIFOL(fd,i*16+12)=i;
		WFIFOL(fd,i*16+16)=p->exp_mode;
	}
	WFIFOW(fd, 2)=i*16+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * �M���h��E�ύX�ʒm
 *------------------------------------------
 */
int clif_guild_positionchanged(struct guild *g,unsigned long idx)
{
	struct map_session_data *sd;
	unsigned char buf[128];

	nullpo_retr(0, g);
	if( idx>=MAX_GUILD )
		return 0;

	WBUFW(buf, 0)=0x174;
	WBUFW(buf, 2)=44;
	WBUFL(buf, 4)=idx;
	WBUFL(buf, 8)=g->position[idx].mode;
	WBUFL(buf,12)=idx;
	WBUFL(buf,16)=g->position[idx].exp_mode;
	memcpy(WBUFP(buf,20),g->position[idx].name,24);
	if( (sd=guild_getavailablesd(*g))!=NULL )
		clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * �M���h�����o�ύX�ʒm
 *------------------------------------------
 */
int clif_guild_memberpositionchanged(struct guild *g,unsigned long idx)
{
	struct map_session_data *sd;
	unsigned char buf[64];

	nullpo_retr(0, g);
	if( idx>=MAX_GUILD )
		return 0;

	WBUFW(buf, 0)=0x156;
	WBUFW(buf, 2)=16;
	WBUFL(buf, 4)=g->member[idx].account_id;
	WBUFL(buf, 8)=g->member[idx].char_id;
	WBUFL(buf,12)=g->member[idx].position;
	if( (sd=guild_getavailablesd(*g))!=NULL )
		clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * �M���h�G���u�������M
 *------------------------------------------
 */
int clif_guild_emblem(struct map_session_data *sd,struct guild *g)
{
	int fd;

	nullpo_retr(0, sd);
	nullpo_retr(0, g);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	if(g->emblem_len<=0)
		return 0;
	WFIFOW(fd,0)=0x152;
	WFIFOW(fd,2)=g->emblem_len+12;
	WFIFOL(fd,4)=g->guild_id;
	WFIFOL(fd,8)=g->emblem_id;
	memcpy(WFIFOP(fd,12),g->emblem_data,g->emblem_len);
	WFIFOSET(fd,g->emblem_len+12);
	return 0;
}
/*==========================================
 * �M���h�X�L�����M
 *------------------------------------------
 */
int clif_guild_skillinfo(struct map_session_data *sd)
{
	int fd;
	int i,id,c,up=1;
	struct guild *g;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd,0)=0x0162;
	WFIFOW(fd,4)=g->skill_point;
	for(i=c=0;i<MAX_GUILDSKILL;i++){
		if(g->skill[i].id>0){
			WFIFOW(fd,c*37+ 6) = id = g->skill[i].id;
			WFIFOW(fd,c*37+ 8) = guild_skill_get_inf(id);
			WFIFOW(fd,c*37+10) = 0;
			WFIFOW(fd,c*37+12) = g->skill[i].lv;
			WFIFOW(fd,c*37+14) = guild_skill_get_sp(id,g->skill[i].lv);
			WFIFOW(fd,c*37+16) = guild_skill_get_range(id);
			memset(WFIFOP(fd,c*37+18),0,24);
			if(g->skill[i].lv < guild_skill_get_max(id)) {
				//Kafra and Guardian changed to require Approval [Sara]
				switch (g->skill[i].id)
				{
					case GD_KAFRACONTRACT:
					case GD_GUARDIANRESEARCH:
					case GD_GUARDUP:
					case GD_DEVELOPMENT:
						up = guild_checkskill(*g,GD_APPROVAL) > 0;
						break;
					case GD_LEADERSHIP:
						//Glory skill requirements -- Pretty sure correct [Sara]
						up = (battle_config.require_glory_guild) ?
							guild_checkskill(*g,GD_GLORYGUILD) > 0 : 1;
						// what skill does it need now that glory guild was removed? [celest]
						break;
					case GD_GLORYWOUNDS:
						up = (battle_config.require_glory_guild) ?
							guild_checkskill(*g,GD_GLORYGUILD) > 0 : 1;
						break;
					case GD_SOULCOLD:
						up = guild_checkskill(*g,GD_GLORYWOUNDS) > 0;
						break;
					case GD_HAWKEYES:
						up = guild_checkskill(*g,GD_LEADERSHIP) > 0;
						break;
					case GD_BATTLEORDER:
						up = guild_checkskill(*g,GD_APPROVAL) > 0 &&
							guild_checkskill(*g,GD_EXTENSION) >= 2;
						break;
					case GD_REGENERATION:
						up = guild_checkskill(*g,GD_EXTENSION) >= 5 &&
							guild_checkskill(*g,GD_BATTLEORDER) > 0;
						break;
					case GD_RESTORE:
						up = guild_checkskill(*g,GD_REGENERATION) >= 2;
						break;
					case GD_EMERGENCYCALL:
						up = guild_checkskill(*g,GD_GUARDIANRESEARCH) > 0 &&
							guild_checkskill(*g,GD_REGENERATION) > 0;
						break;
					case GD_GLORYGUILD:
						up = (battle_config.require_glory_guild) ? 1 : 0;
						break;
					default:
						up = 1;
				}
			}
			else {
				up = 0;
			}
			WFIFOB(fd,c*37+42)= up;
			c++;
		}
	}
	WFIFOW(fd,2)=c*37+6;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * �M���h���m���M
 *------------------------------------------
 */
int clif_guild_notice(struct map_session_data *sd,struct guild *g)
{
	int fd;

	nullpo_retr(0, sd);
	nullpo_retr(0, g);

	fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;
 
	if(*g->mes1==0 && *g->mes2==0)
		return 0;
	WFIFOW(fd,0)=0x16f;
	memcpy(WFIFOP(fd,2),g->mes1,60);
	memcpy(WFIFOP(fd,62),g->mes2,120);
	WFIFOSET(fd,packet_len_table[0x16f]);
	return 0;
}

/*==========================================
 * �M���h�����o���U
 *------------------------------------------
 */
int clif_guild_invite(struct map_session_data *sd,struct guild *g)
{
	int fd;

	nullpo_retr(0, sd);
	nullpo_retr(0, g);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x16a;
	WFIFOL(fd,2)=g->guild_id;
	memcpy(WFIFOP(fd,6),g->name,24);
	WFIFOSET(fd,packet_len_table[0x16a]);
	return 0;
}
/*==========================================
 * �M���h�����o���U����
 *------------------------------------------
 */
int clif_guild_inviteack(struct map_session_data *sd,unsigned char flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x169;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x169]);
	return 0;
}
/*==========================================
 * �M���h�����o�E�ޒʒm
 *------------------------------------------
 */
int clif_guild_leave(struct map_session_data *sd,const char *name,const char *mes)
{
	unsigned char buf[128];

	nullpo_retr(0, sd);

	WBUFW(buf, 0)=0x15a;
	memcpy(WBUFP(buf, 2),name,24);
	memcpy(WBUFP(buf,26),mes,40);
	return clif_send(buf,packet_len_table[0x15a],&sd->bl,GUILD);
}
/*==========================================
 * �M���h�����o�Ǖ��ʒm
 *------------------------------------------
 */
int clif_guild_explusion(struct map_session_data *sd,const char *name,const char *mes,unsigned long account_id)
{
	unsigned char buf[128];

	nullpo_retr(0, sd);

	WBUFW(buf, 0)=0x15c;
	memcpy(WBUFP(buf, 2),name,24);
	memcpy(WBUFP(buf,26),mes,40);
	memcpy(WBUFP(buf,66),"dummy",24);
	return clif_send(buf,packet_len_table[0x15c],&sd->bl,GUILD);
}
/*==========================================
 * �M���h�Ǖ������o���X�g
 *------------------------------------------
 */
int clif_guild_explusionlist(struct map_session_data *sd)
{
	int fd;
	int i,c;
	struct guild *g;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd,0)=0x163;
	for(i=c=0;i<MAX_GUILDEXPLUSION;i++){
		struct guild_explusion *e=&g->explusion[i];
		if(e->account_id>0){
			memcpy(WFIFOP(fd,c*88+ 4),e->name,24);
			memcpy(WFIFOP(fd,c*88+28),e->acc,24);
			memcpy(WFIFOP(fd,c*88+52),e->mes,44);
			c++;
		}
	}
	WFIFOW(fd,2)=c*88+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * �M���h��b
 *------------------------------------------
 */
int clif_guild_message(struct guild *g,unsigned long account_id,const char *mes,size_t len)
{
	struct map_session_data *sd;
	unsigned char buf[512];

	if( mes && g && ((sd = guild_getavailablesd(*g)) != NULL) )
	{
		if(len>512-4) len=512-4; // max buffer limit

	WBUFW(buf, 0) = 0x17f;
	WBUFW(buf, 2) = len + 4;
	memcpy(WBUFP(buf,4), mes, len);
		return clif_send(buf, len + 4, &sd->bl, GUILD);
	}
	return 0;
}
/*==========================================
 * �M���h�X�L������U��ʒm
 *------------------------------------------
 */
int clif_guild_skillup(struct map_session_data *sd,unsigned short skillid,unsigned short skilllv)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x10e;
	WFIFOW(fd,2) = skillid;
	WFIFOW(fd,4) = skilllv;
	WFIFOW(fd,6) = guild_skill_get_sp(skillid,skilllv);
	WFIFOW(fd,8) = guild_skill_get_range(skillid);
	WFIFOB(fd,10) = 1;
	WFIFOSET(fd,11);
	return 0;
}
/*==========================================
 * �M���h�����v��
 *------------------------------------------
 */
int clif_guild_reqalliance(struct map_session_data *sd,unsigned long account_id,const char *name)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x171;
	WFIFOL(fd,2)=account_id;
	memcpy(WFIFOP(fd,6),name,24);
	WFIFOSET(fd,packet_len_table[0x171]);
	return 0;
}
/*==========================================
 * �M���h��������
 *------------------------------------------
 */
int clif_guild_allianceack(struct map_session_data *sd,unsigned long flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x173;
	WFIFOL(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x173]);
	return 0;
}
/*==========================================
 * �M���h�֌W�����ʒm
 *------------------------------------------
 */
int clif_guild_delalliance(struct map_session_data *sd,unsigned long guild_id,unsigned long flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x184;
	WFIFOL(fd,2)=guild_id;
	WFIFOL(fd,6)=flag;
	WFIFOSET(fd,packet_len_table[0x184]);
	return 0;
}
/*==========================================
 * �M���h�G�Ό���
 *------------------------------------------
 */
int clif_guild_oppositionack(struct map_session_data *sd,unsigned char flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x181;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x181]);
	return 0;
}
/*==========================================
 * �M���h�֌W�ǉ�
 *------------------------------------------
 */
/*int clif_guild_allianceadded(struct guild *g,int idx)
{
	unsigned char buf[64];
	WBUFW(fd,0)=0x185;
	WBUFL(fd,2)=g->alliance[idx].opposition;
	WBUFL(fd,6)=g->alliance[idx].guild_id;
	memcpy(WBUFP(fd,10),g->alliance[idx].name,24);
	clif_send(buf,packet_len_table[0x185],guild_getavailablesd(*g),GUILD);
	return 0;
}*/

/*==========================================
 * �M���h���U�ʒm
 *------------------------------------------
 */
int clif_guild_broken(struct map_session_data *sd,unsigned long flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x15e;
	WFIFOL(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x15e]);
	return 0;
}

/*==========================================
 * �G���[�V����
 *------------------------------------------
 */
int clif_emotion(struct block_list *bl,unsigned char type)
{
	unsigned char buf[8];

	nullpo_retr(0,bl);

	WBUFW(buf,0)=0xc0;
	WBUFL(buf,2)=bl->id;
	WBUFB(buf,6)=type;
	return clif_send(buf,packet_len_table[0xc0],bl,AREA);
}

/*==========================================
 * �g�[�L�[�{�b�N�X
 *------------------------------------------
 */
int clif_talkiebox(struct block_list *bl,char* talkie)
{
	unsigned char buf[86];

	nullpo_retr(0,bl);

	WBUFW(buf,0)=0x191;
	WBUFL(buf,2)=bl->id;
	memcpy(WBUFP(buf,6),talkie,80);
	return clif_send(buf,packet_len_table[0x191],bl,AREA);
}

/*==========================================
 * �����G�t�F�N�g
 *------------------------------------------
 */
int clif_wedding_effect(struct block_list *bl) {
	unsigned char buf[6];

	nullpo_retr(0,bl);

	WBUFW(buf,0) = 0x1ea;
	WBUFL(buf,2) = bl->id;
	return clif_send(buf, packet_len_table[0x1ea], bl, AREA);
}
/*==========================================
 * ���Ȃ��Ɉ��������g�p�����O����
 *------------------------------------------

int clif_callpartner(struct map_session_data *sd)
{
	unsigned char buf[26];
	char *p;

	nullpo_retr(0,sd);

	if(sd->status.partner_id){
		WBUFW(buf,0)=0x1e6;
		p = map_charid2nick(sd->status.partner_id);
		if(p){
			memcpy(WBUFP(buf,2),p,24);
		}else{
			map_reqchariddb(*sd,sd->status.partner_id);
			chrif_searchcharid(sd->status.partner_id);
			WBUFB(buf,2) = 0;
		}
		clif_send(buf,packet_len_table[0x1e6]&sd->bl,AREA);
	}
	return 0;
}
*/
/*==========================================
 * Adopt baby [Celest]
 *------------------------------------------
 */
int clif_adopt_process(struct map_session_data *sd)
{
	int fd;
	nullpo_retr(0,sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x1f8;
	WFIFOSET(fd,packet_len_table[0x1f8]);
	return 0;
}
/*==========================================
 * Marry [DracoRPG]
 *------------------------------------------
 */
void clif_marriage_process(struct map_session_data *sd)
{
	int fd;
	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x1e4;
	WFIFOSET(fd,packet_len_table[0x1e4]);
}


/*==========================================
 * Notice of divorce
 *------------------------------------------
 */
int clif_divorced(struct map_session_data *sd, char *name)
{
	int fd;
	nullpo_ret(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x205;
	memcpy(WFIFOP(fd,2), name, 24);
	WFIFOSET(fd, packet_len_table[0x205]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_ReqAdopt(int fd, struct map_session_data *sd)
{
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x1f6;
	WFIFOSET(fd, packet_len_table[0x1f6]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_ReqMarriage(int fd, struct map_session_data &sd)
{
	if( !session_isActive(fd) )
		return 0;
	WFIFOW(fd,0)=0x1e2;
	WFIFOSET(fd, packet_len_table[0x1e2]);
	return 0;
}

/*==========================================
 * ����
 *------------------------------------------
 */
int clif_sitting(struct map_session_data &sd)
{
	unsigned char buf[64];

	WBUFW(buf, 0) = 0x8a;
	WBUFL(buf, 2) = sd.bl.id;
	WBUFB(buf,26) = 2;
	return clif_send(buf, packet_len_table[0x8a], &sd.bl, AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_disp_onlyself(struct map_session_data &sd, const char *mes)
{
	if(mes)
	{
		unsigned char buf[512];
		int len = strlen(mes)+1;
		if(len>512-4) len=512-4; // max buffer limit

		WBUFW(buf, 0) = 0x17f;
		WBUFW(buf, 2) = len+4;
		memcpy(WBUFP(buf,4), mes, len);

		clif_send(buf, len+4, &sd.bl, SELF);
	}
	return 0;
}


/*==========================================
 *
 *------------------------------------------
 */

int clif_GM_kickack(struct map_session_data &sd, unsigned long id)
{
	int fd = sd.fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0xcd;
	WFIFOL(fd,2) = id;
	WFIFOSET(fd, packet_len_table[0xcd]);
	return 0;
}

int clif_GM_kick(struct map_session_data &sd,struct map_session_data &tsd,int type)
{
	if(type)
		clif_GM_kickack(sd,tsd.status.account_id);
	tsd.opt1 = tsd.opt2 = 0;

	if( !session_isActive(tsd.fd) )
		return 0;

	WFIFOW(tsd.fd,0) = 0x18b;
	WFIFOW(tsd.fd,2) = 0;
	WFIFOSET(tsd.fd,packet_len_table[0x18b]);
	session_SetWaitClose(tsd.fd, 5000);

	return 0;
}

int clif_GM_silence(struct map_session_data &sd, struct map_session_data &tsd, int type)
{
	int fd;
	fd = tsd.fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x14b;
	WFIFOB(fd,2) = 0;
	memcpy(WFIFOP(fd,3), sd.status.name, 24);
	WFIFOSET(fd, packet_len_table[0x14b]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */

int clif_timedout(struct map_session_data &sd)
{
	ShowInfo("%sCharacter with Account ID '"CL_WHITE"%d"CL_RESET"' timed out.\n", (pc_isGM(sd))?"GM ":"", sd.bl.id);
	map_quit(sd);
	clif_authfail_fd(sd.fd,3); // Even if player is not on we still send anyway
	session_Remove(sd.fd); // Set session to EOF
	return 0;
}

/*==========================================
 * Wis���ۋ�����
 *------------------------------------------
 */
int clif_wisexin(struct map_session_data *sd,int type,int flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0xd1;
	WFIFOB(fd,2)=type;
	WFIFOB(fd,3)=flag;
	WFIFOSET(fd,packet_len_table[0xd1]);

	return 0;
}
/*==========================================
 * Wis�S���ۋ�����
 *------------------------------------------
 */
int clif_wisall(struct map_session_data *sd,int type,int flag)
{
	int fd;

	nullpo_retr(0, sd);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0xd2;
	WFIFOB(fd,2)=type;
	WFIFOB(fd,3)=flag;
	WFIFOSET(fd,packet_len_table[0xd2]);

	return 0;
}
/*==========================================
 * �T�E���h�G�t�F�N�g
 *------------------------------------------
 */
int clif_soundeffect(struct map_session_data *sd,struct block_list *bl,const char *name,unsigned char type)
{
	int fd;

	nullpo_retr(0,sd);
	nullpo_retr(0,bl);

	fd=sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0)=0x1d3;
	memcpy(WFIFOP(fd,2),name,24);
	WFIFOB(fd,26)=type;
	WFIFOL(fd,27)=0;
	WFIFOL(fd,31)=bl->id;
	WFIFOSET(fd,packet_len_table[0x1d3]);

	return 0;
}

int clif_soundeffectall(struct block_list *bl, const char *name, unsigned char type)
{
	unsigned char buf[31];
	memset(buf, 0, packet_len_table[0x1d3]);

	nullpo_retr(0, bl);

	WBUFW(buf,0)=0x1d3;
	memcpy(WBUFP(buf,2), name, 24);
	WBUFB(buf,26)=type;
	WBUFL(buf,27)=0;
	WBUFL(buf,31)=bl->id;
	return clif_send(buf, packet_len_table[0x1d3], bl, AREA);
}

// displaying special effects (npcs, weather, etc) [Valaris]
int clif_specialeffect(struct block_list *bl, unsigned long type, int flag)
{
	unsigned char buf[24];

	nullpo_retr(0, bl);

	memset(buf, 0, packet_len_table[0x1f3]);

	WBUFW(buf,0) = 0x1f3;
	WBUFL(buf,2) = bl->id;
	WBUFL(buf,6) = type;

	switch (flag) {
	case 3:
		clif_send(buf, packet_len_table[0x1f3], bl, ALL_CLIENT);
		break;
	case 2:
		clif_send(buf, packet_len_table[0x1f3], bl, ALL_SAMEMAP);
		break;
	case 1:
		clif_send(buf, packet_len_table[0x1f3], bl, SELF);
		break;
	default:
		clif_send(buf, packet_len_table[0x1f3], bl, AREA);
	}

	return 0;
}

// refresh the client's screen, getting rid of any effects
int clif_refresh(struct map_session_data *sd) {
	nullpo_retr(-1, sd);
	clif_changemap(sd,sd->mapname,sd->bl.x,sd->bl.y);
	return 0;
}

// ------------
// clif_parse_*
// ------------
// �p�P�b�g�ǂݎ���ĐF�X����
/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_WantToConnection(int fd, struct map_session_data *sd)
{
	struct map_session_data *old_sd;
	unsigned short cmd;
	unsigned long account_id;
	unsigned long char_id;
	unsigned long login_id1;
	unsigned long client_tick;
	unsigned char sex;
	size_t packet_ver;	// 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])

	if(sd)
	{
		if (battle_config.error_log)
			ShowMessage("clif_parse_WantToConnection : invalid request?\n");
		return 0;
	}

	if( !session_isActive(fd) )
		return 0;

	cmd = RFIFOW(fd,0);
	// printf("Received bytes %d with packet 0x%d.\n", RFIFOREST(fd), cmd);

	if (IS_PACKET_DB_VER(cmd) &&
		RFIFOREST(fd) == packet_db[clif_config.packet_db_ver][clif_config.connect_cmd].len &&
		(RFIFOB(fd, packet_db[clif_config.packet_db_ver][clif_config.connect_cmd].pos[4]) == 0 ||	// 00 = Female
		RFIFOB(fd, packet_db[clif_config.packet_db_ver][clif_config.connect_cmd].pos[4]) == 1))		// 01 = Male
	{
		packet_ver	= clif_config.packet_db_ver;
		account_id	= RFIFOL(fd, packet_db[packet_ver][cmd].pos[0]);
		char_id		= RFIFOL(fd, packet_db[packet_ver][cmd].pos[1]);
		login_id1	= RFIFOL(fd, packet_db[packet_ver][cmd].pos[2]);
		client_tick	= RFIFOL(fd, packet_db[packet_ver][cmd].pos[3]);
		sex			= RFIFOB(fd, packet_db[packet_ver][cmd].pos[4]);
	}
	else if( cmd == 0x72 )
	{
		if( RFIFOREST(fd) >= 39 && (RFIFOB(fd,38) == 0 || RFIFOB(fd,38) == 1) )
		{
			packet_ver = 7;
			account_id	= RFIFOL(fd, 12);
			char_id		= RFIFOL(fd, 22);
			login_id1	= RFIFOL(fd, 30);
			client_tick	= RFIFOL(fd, 34);
			sex			= RFIFOB(fd, 38);
		}
		else if( RFIFOREST(fd) >= 22 && (RFIFOB(fd,21) == 0 || RFIFOB(fd,21) == 1) )
		{
			packet_ver = 6;
			account_id	= RFIFOL(fd, 5);
			char_id		= RFIFOL(fd, 9);
			login_id1	= RFIFOL(fd, 13);
			client_tick	= RFIFOL(fd, 17);
			sex			= RFIFOB(fd, 21);
		}
		else
		{
			packet_ver = 5;
			account_id	= RFIFOL(fd, 2);
			char_id		= RFIFOL(fd, 6);
			login_id1	= RFIFOL(fd, 10);
			client_tick	= RFIFOL(fd, 14);
			sex			= RFIFOB(fd, 18);
		}
	}
	else if( cmd == 0x7E )
	{
		if( RFIFOREST(fd) >= 37 && (RFIFOB(fd,36) == 0 || RFIFOB(fd,36) == 1) )
		{
			packet_ver = 9;
			account_id	= RFIFOL(fd, 9);
			char_id		= RFIFOL(fd, 21);
			login_id1	= RFIFOL(fd, 28);
			client_tick	= RFIFOL(fd, 32);
			sex			= RFIFOB(fd, 36);
		}
		else
		{
			packet_ver = 8;
			account_id	= RFIFOL(fd, 12);
			char_id		= RFIFOL(fd, 18);
			login_id1	= RFIFOL(fd, 24);
			client_tick	= RFIFOL(fd, 28);
			sex			= RFIFOB(fd, 32);
		}
	}
	else if( cmd == 0xF5 )
	{
		if( RFIFOREST(fd) >= 34 && (RFIFOB(fd,33) == 0 || RFIFOB(fd,33) == 1) )
		{
			packet_ver = 10;
			account_id	= RFIFOL(fd, 7);
			char_id		= RFIFOL(fd, 15);
			login_id1	= RFIFOL(fd, 25);
			client_tick	= RFIFOL(fd, 29);
			sex			= RFIFOB(fd, 33);
		}
		else if( RFIFOREST(fd) >= 33 && (RFIFOB(fd,32) == 0 || RFIFOB(fd,32) == 1) )
		{
			packet_ver = 12;
			account_id	= RFIFOL(fd, 12);
			char_id		= RFIFOL(fd, 18);
			login_id1	= RFIFOL(fd, 24);
			client_tick	= RFIFOL(fd, 28);
			sex			= RFIFOB(fd, 32);
		}
		else if( RFIFOREST(fd) >= 32 && (RFIFOB(fd,31) == 0 || RFIFOB(fd,31) == 1) )
		{
			packet_ver = 11;
			account_id	= RFIFOL(fd, 10);
			char_id		= RFIFOL(fd, 17);
			login_id1	= RFIFOL(fd, 23);
			client_tick	= RFIFOL(fd, 27);
			sex			= RFIFOB(fd, 31);
		}
		else
		{	// 29
			// if account id and char id of version 14
			if(RFIFOL(fd, 3) > 700000 && RFIFOL(fd, 10) >= 150000 && RFIFOL(fd, 10) < 5000000)
			{	// account id / char id (more than 5.000.000 characters?)
				packet_ver = 15;
				account_id	= RFIFOL(fd, 3);
				char_id		= RFIFOL(fd, 10);
				login_id1	= RFIFOL(fd, 20);
				client_tick	= RFIFOL(fd, 24);
				sex			= RFIFOB(fd, 28);
			}
			else
			{
				packet_ver = 13;
				account_id	= RFIFOL(fd, 5);
				char_id		= RFIFOL(fd, 14);
				login_id1	= RFIFOL(fd, 20);
				client_tick	= RFIFOL(fd, 24);
				sex			= RFIFOB(fd, 28);
			}
		}
	}
	else
	{
		if(RFIFOREST(fd) >= 32 && (RFIFOB(fd,31) == 0 || RFIFOB(fd,31) == 1))
		{
			packet_ver = 16;
			account_id	= RFIFOL(fd, 3);
			char_id		= RFIFOL(fd, 12);
			login_id1	= RFIFOL(fd, 23);
			client_tick	= RFIFOL(fd, 27);
			sex			= RFIFOB(fd, 31);
		}
		else
		{
			packet_ver = 17;
			account_id	= RFIFOL(fd, 4);
			char_id		= RFIFOL(fd, 9);
			login_id1	= RFIFOL(fd, 17);
			client_tick	= RFIFOL(fd, 18);
			sex			= RFIFOB(fd, 25);
		}
	}

	// if same account already connected, we disconnect the 2 sessions
	if((old_sd = map_id2sd(account_id)) != NULL)
	{
		clif_authfail_fd(fd, 8); // still recognizes last connection
		clif_authfail_fd(old_sd->fd, 2); // same id
		if (sd != 0)
			session_Remove(fd); // Set session to EOF
	}
	else
	{
		sd = (struct map_session_data *) (session[fd]->session_data = aCalloc(1,sizeof(struct map_session_data)));
		sd->fd = fd;
		sd->packet_ver = packet_ver;

		pc_setnewpc(fd, *sd, account_id, char_id, login_id1, client_tick, sex);
		WFIFOL(fd,0) = sd->bl.id;
		WFIFOSET(fd,4);

		map_addiddb(sd->bl);
		chrif_authreq(*sd);
	}

	return 0;
}

/*==========================================
 * 007d �N���C�A���g���}�b�v�ǂݍ��݊���
 * map�N�����ɕK�v�ȃf�[�^��S�đ������
 *------------------------------------------
 */
int clif_parse_LoadEndAck(int fd,struct map_session_data *sd)
{
//	struct item_data* item;
	int i;

	if( !session_isActive(fd) )
		return 0;

	nullpo_retr(0,sd);

	if(sd->bl.prev != NULL)
		return 0;

	// �ڑ�ok��
	//clif_authok();
	if(sd->npc_id) npc_event_dequeue(*sd);
	clif_skillinfoblock(sd);
	pc_checkitem(*sd);
	//guild_info();

	// loadendack��
	// next exp
	clif_updatestatus(sd,SP_NEXTBASEEXP);
	clif_updatestatus(sd,SP_NEXTJOBEXP);
	// skill point
	clif_updatestatus(sd,SP_SKILLPOINT);
	// item
	clif_itemlist(sd);
	clif_equiplist(sd);
	// cart
	if(pc_iscarton(*sd)){
		clif_cart_itemlist(sd);
		clif_cart_equiplist(sd);
		clif_updatestatus(sd,SP_CARTINFO);
	}
	// param all
	clif_initialstatus(sd);
	// party
	party_send_movemap(*sd);
	// guild
	guild_send_memberinfoshort(*sd,1);
	// 119
	// 78

	if(battle_config.pc_invincible_time > 0) {
		if(map[sd->bl.m].flag.gvg)
			pc_setinvincibletimer(sd,battle_config.pc_invincible_time<<1);
		else
			pc_setinvincibletimer(sd,battle_config.pc_invincible_time);
	}

	map_addblock(sd->bl);	// �u���b�N�o�^
	clif_spawnpc(sd);	// spawn

	// weight max , now
	clif_updatestatus(sd,SP_MAXWEIGHT);
	clif_updatestatus(sd,SP_WEIGHT);

	// pvp
	if(sd->pvp_timer!=-1 && !battle_config.pk_mode)
		delete_timer(sd->pvp_timer,pc_calc_pvprank_timer);
	if(map[sd->bl.m].flag.pvp){
		if(!battle_config.pk_mode) { // remove pvp stuff for pk_mode [Valaris]
			sd->pvp_timer=add_timer(gettick()+200,pc_calc_pvprank_timer,sd->bl.id,0);
			sd->pvp_rank=0;
			sd->pvp_lastusers=0;
			sd->pvp_point=5;
			sd->pvp_won=0;
			sd->pvp_lost=0;
		}
		clif_set0199(sd->fd,1);
	} else {
		sd->pvp_timer=-1;
	}
	if(map[sd->bl.m].flag.gvg) {
		clif_set0199(sd->fd,3);
	}

	// pet
	if(sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 0) {
		map_addblock(sd->pd->bl);
		clif_spawnpet(sd->pd);
		clif_send_petdata(sd,0,0);
		clif_send_petdata(sd,5,0x14);
		clif_send_petstatus(sd);
	}

	if(sd->state.connect_new) {
		sd->state.connect_new = 0;
		if(sd->status.class_ != sd->view_class)
			clif_changelook(&sd->bl,LOOK_BASE,sd->view_class);
		if(sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 900)
			clif_pet_emotion(sd->pd,(sd->pd->class_ - 100)*100 + 50 + pet_hungry_val(*sd));


		// Stop players from spawning inside castles [Valaris]
                        {
			struct guild_castle *gc=guild_mapname2gc(map[sd->bl.m].mapname);
			if (gc)
				pc_setpos(*sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,2);
                        }
		// End Addition [Valaris]
		}
	// view equipment item
#if PACKETVER < 4
	clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
	clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
#else
	clif_changelook(&sd->bl,LOOK_WEAPON,0);
#endif
	if(battle_config.save_clothcolor &&
		sd->status.clothes_color > 0 &&
		(sd->view_class != 22 || !battle_config.wedding_ignorepalette)
		)
		clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->status.clothes_color);

	//if(sd->status.hp<sd->status.max_hp>>2 && pc_checkskill(*sd,SM_AUTOBERSERK)>0 &&
	if(sd->status.hp<sd->status.max_hp>>2 && sd->sc_data[SC_AUTOBERSERK].timer != -1 &&
		(sd->sc_data[SC_PROVOKE].timer==-1 || sd->sc_data[SC_PROVOKE].val2==0 ))
		// �I�[�g�o�[�T�[�N����
		status_change_start(&sd->bl,SC_PROVOKE,10,1,0,0,0,0);

//	if(time(&timer) < ((weddingtime=pc_readglobalreg(sd,"PC_WEDDING_TIME")) + 3600))
//		status_change_start(&sd->bl,SC_WEDDING,0,weddingtime,0,0,36000,0);

	if(battle_config.muting_players && sd->status.manner < 0)
		status_change_start(&sd->bl,SC_NOCHAT,0,0,0,0,0,0);

	if (night_flag) {
		if (battle_config.night_darkness_level > 0 && !map[sd->bl.m].flag.indoors)
			clif_specialeffect(&sd->bl, 474 + battle_config.night_darkness_level, 0);
		else {
			//clif_specialeffect(&sd->bl, 483, 0); // default darkness level
			// night - when changing from indoors to outdoors - celest
			if (!map[sd->bl.m].flag.indoors && sd->opt2 != STATE_BLIND)
				sd->opt2 |= STATE_BLIND;
			// changing from outdoors to indoors
			else if (map[sd->bl.m].flag.indoors && sd->opt2 == STATE_BLIND)
				sd->opt2 &= ~STATE_BLIND;
		}
	}

	// option
	clif_changeoption(&sd->bl);
	if(sd->sc_data[SC_TRICKDEAD].timer != -1)
		status_change_end(&sd->bl,SC_TRICKDEAD,-1);
	if(sd->sc_data[SC_SIGNUMCRUCIS].timer != -1 && !battle_check_undead(7,sd->def_ele))
		status_change_end(&sd->bl,SC_SIGNUMCRUCIS,-1);
	if(sd->state.infinite_endure && sd->sc_data[SC_ENDURE].timer == -1)
		status_change_start(&sd->bl,SC_ENDURE,10,1,0,0,0,0);
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].equip && sd->status.inventory[i].equip & 0x0002 && sd->status.inventory[i].attribute==1)
			status_change_start(&sd->bl,SC_BROKNWEAPON,0,0,0,0,0,0);
		if(sd->status.inventory[i].equip && sd->status.inventory[i].equip & 0x0010 && sd->status.inventory[i].attribute==1)
			status_change_start(&sd->bl,SC_BROKNARMOR,0,0,0,0,0,0);
	}

	map_foreachinarea(clif_getareachar,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,0,sd);

	// ============================================
	// ADDITION Qamera death/disconnect/connect event mod
	if(!sd->state.event_onconnect){
		npc_event_doall_attached("OnConnect",*sd);
		sd->state.event_onconnect=1;
}
	// END ADDITION
	// ============================================ 
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_TickSend(int fd, struct map_session_data *sd) {
	nullpo_retr(0,sd);

	if( !session_isActive(fd) )
		return 0;

	if( USE_PACKET_DB(sd) )
	{
		sd->client_tick=RFIFOL(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0]);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 8:
			sd->client_tick = RFIFOL(fd,6);
			break;
		case 9:
			sd->client_tick = RFIFOL(fd,9);
			break;
		case 10:
			sd->client_tick = RFIFOL(fd,7);
			break;
		case 11:
			sd->client_tick = RFIFOL(fd,10);
			break;
		case 12:
			sd->client_tick = RFIFOL(fd,6);
			break;
		case 13:
		case 14:
			sd->client_tick = RFIFOL(fd,5);
			break;
		case 15:
			sd->client_tick = RFIFOL(fd,3);
			break;
		case 16:
			sd->client_tick = RFIFOL(fd,5);
			break;
		case 17:
			sd->client_tick = RFIFOL(fd,4);
			break;

		default: // old version by default (and version 6 + 7)
			sd->client_tick = RFIFOL(fd,2);
			break;
		}
	}

	sd->server_tick = gettick();
	return clif_servertick(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_WalkToXY(int fd, struct map_session_data *sd) {
	int x, y;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (pc_isdead(*sd)) {
		return clif_clearchar_area(&sd->bl, 1);
	}

	if (sd->npc_id != 0 || sd->vender_id != 0)
		return 0;

	if (sd->skilltimer != -1 && pc_checkskill(*sd, SA_FREECAST) <= 0) // �t���[�L���X�g
		return 0;

	if (sd->chatID)
		return 0;

	if (sd->canmove_tick > gettick())
		return 0;

	// �X�e�[�^�X�ُ��n�C�f�B���O��(�g���l���h���C�u��)�œ����Ȃ�
	if ((sd->opt1 > 0 && sd->opt1 != 6) ||
	     sd->sc_data[SC_ANKLE].timer !=-1 || //�A���N���X�l�A
	     sd->sc_data[SC_AUTOCOUNTER].timer !=-1 || //�I�[�g�J�E���^�[
	     sd->sc_data[SC_TRICKDEAD].timer !=-1 || //���񂾂ӂ�
	     sd->sc_data[SC_BLADESTOP].timer !=-1 || //���n���
	     sd->sc_data[SC_SPIDERWEB].timer !=-1 || //�X�p�C�_�[�E�F�b�u
	     (sd->sc_data[SC_DANCING].timer !=-1 && sd->sc_data[SC_DANCING].val4) || //���t�X�L�����t���͓����Ȃ�
		 (sd->sc_data[SC_GOSPEL].timer !=-1 && sd->sc_data[SC_GOSPEL].val4 == BCT_SELF) ||	// cannot move while gospel is in effect
		 sd->sc_data[SC_CONFUSION].timer !=-1)
		return 0;
	if ((sd->status.option & 2) && pc_checkskill(*sd, RG_TUNNELDRIVE) <= 0)
		return 0;

	if (sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);

	pc_stopattack(sd);

	if (USE_PACKET_DB(sd)) {
		int cmd = RFIFOW(fd,0);
		x = RFIFOB(fd,packet_db[clif_config.packet_db_ver][cmd].pos[0]) * 4 +
			(RFIFOB(fd,packet_db[clif_config.packet_db_ver][cmd].pos[0] + 1) >> 6);
		y = ((RFIFOB(fd,packet_db[clif_config.packet_db_ver][cmd].pos[0]+1) & 0x3f) << 4) +
			(RFIFOB(fd,packet_db[clif_config.packet_db_ver][cmd].pos[0] + 2) >> 4);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 6:
			x = RFIFOB(fd,5) * 4 + (RFIFOB(fd,6) >> 6);
			y = ((RFIFOB(fd,6) & 0x3f) << 4) + (RFIFOB(fd,7) >> 4);
			break;
		case 7:
			x = RFIFOB(fd,6) * 4 + (RFIFOB(fd,7) >> 6);
			y = ((RFIFOB(fd,7) & 0x3f) << 4) + (RFIFOB(fd,8) >> 4);
			break;
		case 8:
			x = RFIFOB(fd,3) * 4 + (RFIFOB(fd,4) >> 6);
			y = ((RFIFOB(fd,4) & 0x3f) << 4) + (RFIFOB(fd,5) >> 4);
			break;
		case 9:
			x = RFIFOB(fd,12) * 4 + (RFIFOB(fd,13) >> 6);
			y = ((RFIFOB(fd,13) & 0x3f) << 4) + (RFIFOB(fd,14) >> 4);
			break;
		case 10:
			x = RFIFOB(fd,6) * 4 + (RFIFOB(fd,7) >> 6);
			y = ((RFIFOB(fd,7) & 0x3f) << 4) + (RFIFOB(fd,8) >> 4);
			break;
		case 11:
			x = RFIFOB(fd,11) * 4 + (RFIFOB(fd,12) >> 6);
			y = ((RFIFOB(fd,12) & 0x3f) << 4) + (RFIFOB(fd,13) >> 4);
			break;
		case 12:
			x = RFIFOB(fd,3) * 4 + (RFIFOB(fd,4) >> 6);
			y = ((RFIFOB(fd,4) & 0x3f) << 4) + (RFIFOB(fd,5) >> 4);
			break;
		case 13:
		case 14:
			x = RFIFOB(fd,3) * 4 + (RFIFOB(fd,4) >> 6);
			y = ((RFIFOB(fd,4) & 0x3f) << 4) + (RFIFOB(fd,5) >> 4);
			break;
		case 15:
			x = RFIFOB(fd,4) * 4 + (RFIFOB(fd,5) >> 6);
			y = ((RFIFOB(fd,5) & 0x3f) << 4) + (RFIFOB(fd,6) >> 4);
			break;
		case 16:
			x = RFIFOB(fd,10) * 4 + (RFIFOB(fd,11) >> 6);
			y = ((RFIFOB(fd,11) & 0x3f) << 4) + (RFIFOB(fd,12) >> 4);
			break;
		case 17:
			x = RFIFOB(fd,5) * 4 + (RFIFOB(fd,6) >> 6);
			y = ((RFIFOB(fd,6) & 0x3f) << 4) + (RFIFOB(fd,7) >> 4);
			break;
		default: // old version by default
			x = RFIFOB(fd,2) * 4 + (RFIFOB(fd,3) >> 6);
			y = ((RFIFOB(fd,3) & 0x3f) << 4) + (RFIFOB(fd,4) >> 4);
			break;
		}
	}
	pc_walktoxy(*sd, x, y);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_QuitGame(int fd, struct map_session_data *sd) {
	unsigned long tick=gettick();
	struct skill_unit_group* sg;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x18b;
	if ((!pc_isdead(*sd) && (sd->opt1 || (sd->opt2 && !(night_flag == 1 && sd->opt2 == STATE_BLIND)))) ||
	    sd->skilltimer != -1 ||
	    (DIFF_TICK(tick, sd->canact_tick) < 0) ||
	    (sd->sc_data && sd->sc_data[SC_DANCING].timer!=-1 && sd->sc_data[SC_DANCING].val4 && (sg=(struct skill_unit_group *)sd->sc_data[SC_DANCING].val2) && sg->src_id == sd->bl.id)) {
		WFIFOW(fd,2)=1;
		WFIFOSET(fd,packet_len_table[0x18b]);
		return 0;
	}

	/*	Rovert's prevent logout option fixed [Valaris]	*/
	if( ((gettick() - sd->canlog_tick) >= 10000) || (!battle_config.prevent_logout)) {
		session_SetWaitClose(fd, 1000);
		WFIFOW(fd,2)=0;
	} else {
		WFIFOW(fd,2)=1;
	}
	WFIFOSET(fd,packet_len_table[0x18b]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_GetCharNameRequest(int fd, struct map_session_data *sd) {
	struct block_list *bl;
	unsigned long account_id;
	if( !session_isActive(fd) )
		return 0;

	if (USE_PACKET_DB(sd)) {
		account_id = RFIFOL(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0]);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 8:
			account_id = RFIFOL(fd,11);
			break;
		case 9:
			account_id = RFIFOL(fd,8);
			break;
		case 10:
			account_id = RFIFOL(fd,10);
			break;
		case 11:
			account_id = RFIFOL(fd,6);
			break;
		case 12:
			account_id = RFIFOL(fd,11);
			break;
		case 13:
		case 14:
			account_id = RFIFOL(fd,6);
			break;
		case 15:
			account_id = RFIFOL(fd,9);
			break;
		case 16:
			account_id = RFIFOL(fd,4);
			break;
		case 17:
			account_id = RFIFOL(fd,7);
			break;
		default: // old version by default (+ packet version 6 and 7)
			account_id = RFIFOL(fd,2);
			break;
		}
	}
	bl = map_id2bl(account_id);
	if (bl == NULL)
		return 0;

	WFIFOW(fd,0) = 0x95;
	WFIFOL(fd,2) = account_id;

	switch(bl->type) {
	case BL_PC:
	  {
		struct map_session_data *ssd = (struct map_session_data *)bl;
		struct party *p = NULL;
		struct guild *g = NULL;

		nullpo_retr(0,ssd);

		memcpy(WFIFOP(fd,6), ssd->status.name, 24);
		if (ssd->status.guild_id > 0 && (g = guild_search(ssd->status.guild_id)) != NULL &&
		    (ssd->status.party_id == 0 || (p = party_search(ssd->status.party_id)) != NULL)) {
			// �M���h�����Ȃ�p�P�b�g0195��Ԃ�
			int i, ps = -1;
			for(i = 0; i < g->max_member; i++) {
				if (g->member[i].account_id == ssd->status.account_id &&
				    g->member[i].char_id == ssd->status.char_id )
					ps = g->member[i].position;
			}
			if (ps >= 0 && ps < MAX_GUILDPOSITION) {
				WFIFOW(fd, 0) = 0x195;
				if (p)
					memcpy(WFIFOP(fd,30), p->name, 24);
				else
					WFIFOB(fd,30) = 0;
				memcpy(WFIFOP(fd,54), g->name,24);
				memcpy(WFIFOP(fd,78), g->position[ps].name, 24);
				WFIFOSET(fd,packet_len_table[0x195]);
				break;
			}
		}
		WFIFOSET(fd,packet_len_table[0x95]);
	  }
		break;
	case BL_PET:
		//memcpy(WFIFOP(fd,6), ((struct pet_data*)bl)->name, 24);
		memcpy(WFIFOP(fd,6), ((struct pet_data*)bl)->namep, 24);
		WFIFOSET(fd,packet_len_table[0x95]);
		break;
	case BL_NPC:
		memcpy(WFIFOP(fd,6), ((struct npc_data*)bl)->name, 24);
		WFIFOSET(fd,packet_len_table[0x95]);
		break;
	case BL_MOB:
		{
			struct mob_data *md = (struct mob_data *)bl;

			nullpo_retr(0,md);

			memcpy(WFIFOP(fd,6), md->name, 24);
			if (md->class_ >= 1285 && md->class_ <= 1288 && md->guild_id) {
				struct guild *g;
				struct guild_castle *gc = guild_mapname2gc(map[md->bl.m].mapname);
				if (gc && gc->guild_id > 0 && (g = guild_search(gc->guild_id)) != NULL) {
					WFIFOW(fd, 0) = 0x195;
					WFIFOB(fd,30) = 0;
					memcpy(WFIFOP(fd,54), g->name, 24);
					memcpy(WFIFOP(fd,78), gc->castle_name, 24);
					WFIFOSET(fd,packet_len_table[0x195]);
				} else {
					WFIFOSET(fd,packet_len_table[0x95]);
				}
			} else if (battle_config.show_mob_hp == 1) {
				char mobhp[50];
				sprintf(mobhp, "hp: %d/%d", md->hp, md->max_hp);
				WFIFOW(fd, 0) = 0x195;
				memcpy(WFIFOP(fd,30), mobhp, 24);
				WFIFOB(fd,54) = 0;
				WFIFOB(fd,78) = 0;
				WFIFOSET(fd,packet_len_table[0x195]);
			} else {
				WFIFOSET(fd,packet_len_table[0x95]);
			}
		}
		break;
	default:
		if (battle_config.error_log)
			ShowMessage("clif_parse_GetCharNameRequest : bad type %d(%d)\n", bl->type, account_id);
		break;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_GlobalMessage(int fd, struct map_session_data *sd) { // S 008c <len>.w <str>.?B
	char *message;
	unsigned char *buf;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if( (is_atcommand(fd, *sd, (char*)RFIFOP(fd,4), 0) != AtCommand_None) ||
        (is_charcommand(fd, *sd, (char*)RFIFOP(fd,4),0)!= CharCommand_None) ||
	    (sd->sc_data &&
	    (sd->sc_data[SC_BERSERK].timer != -1 || //�o�[�T�[�N���͉�b���s��
	     sd->sc_data[SC_NOCHAT].timer != -1 ))) //�`���b�g�֎~
		return 0;

	message = (char*)aMalloc( (RFIFOW(fd,2) + 128) * sizeof(char));
	buf = (unsigned char*)aMalloc( (RFIFOW(fd,2) + 4) * sizeof(unsigned char));

	//ShowMessage("clif_parse_GlobalMessage: message: '%s'.\n", RFIFOP(fd,4));
	if (strncmp((char*)RFIFOP(fd,4), sd->status.name, strlen(sd->status.name)) != 0) {
		ShowMessage("Hack on global message: character '%s' (account: %d), use an other name to send a (normal) message.\n", sd->status.name, sd->status.account_id);

		// information is sended to all online GM
		sprintf(message, "Hack on global message (normal message): character '%s' (account: %ld) uses another name.", sd->status.name, sd->status.account_id);
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message);
		if (strlen((char*)RFIFOP(fd,4)) == 0)
			strcpy(message, " This player sends a void name and a void message.");
		else
			sprintf(message, " This player sends (name:message): '%s'.", RFIFOP(fd,4));
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message);
		// message about the ban
		if (battle_config.ban_spoof_namer > 0)
			sprintf(message, " This player has been banned for %d minute(s).", battle_config.ban_spoof_namer);
		else
			sprintf(message, " This player hasn't been banned (Ban option is disabled).");
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message);

		// if we ban people
		if (battle_config.ban_spoof_namer > 0) {
			chrif_char_ask_name(-1, sd->status.name, 2, 0, 0, 0, 0, battle_config.ban_spoof_namer, 0); // type: 2 - ban (year, month, day, hour, minute, second)
			session_SetWaitClose(fd, 1000); // forced to disconnect because of the hack
			if(message) aFree(message);
			if(buf) aFree(buf);
			return 0;
		}
		// but for the hacker, we display on his screen (he see/look no difference).
	} else {
		// send message to others
		WBUFW(buf,0) = 0x8d;
		WBUFW(buf,2) = RFIFOW(fd,2) + 4; // len of message - 4 + 8
		WBUFL(buf,4) = sd->bl.id;
		memcpy(WBUFP(buf,8), RFIFOP(fd,4), RFIFOW(fd,2) - 4);
		clif_send(buf, WBUFW(buf,2), &sd->bl, sd->chatID ? CHAT_WOS : AREA_CHAT_WOC);
	}

	// send back message to the speaker
	memcpy(WFIFOP(fd,0), RFIFOP(fd,0), RFIFOW(fd,2));
	WFIFOW(fd,0) = 0x8e;
	WFIFOSET(fd, WFIFOW(fd,2));

	map_foreachinarea(npc_chat_sub, sd->bl.m, sd->bl.x-AREA_SIZE, sd->bl.y-AREA_SIZE, sd->bl.x+AREA_SIZE, sd->bl.y+AREA_SIZE, BL_NPC, RFIFOP(fd,4), strlen((char*)RFIFOP(fd,4)), &sd->bl);

	// Celest
	if (pc_calc_base_job2 (sd->status.class_) == 23 ) {
		int next = pc_nextbaseexp(sd) > 0 ? pc_nextbaseexp(sd) : sd->status.base_exp;
		char *rfifo = (char*)RFIFOP(fd,4);
		if (next > 0 && (sd->status.base_exp * 100 / next) % 10 == 0) {
			if (sd->state.snovice_flag == 0 && strstr(rfifo, msg_txt(504)))
				sd->state.snovice_flag = 1;
			else if (sd->state.snovice_flag == 1) {
				sprintf(message, msg_txt(505), sd->status.name);
				if (strstr(rfifo, message))
					sd->state.snovice_flag = 2;
			}
			else if (sd->state.snovice_flag == 2 && strstr(rfifo, msg_txt(506)))
				sd->state.snovice_flag = 3;
			else if (sd->state.snovice_flag == 3) {
				clif_skill_nodamage(&sd->bl,&sd->bl,MO_EXPLOSIONSPIRITS,0xFFFF,1);
				status_change_start(&sd->bl,SkillStatusChangeTable[MO_EXPLOSIONSPIRITS],
						1,0,0,0,skill_get_time(MO_EXPLOSIONSPIRITS,1),0 );
				sd->state.snovice_flag = 0;
			}
		}
	}

	if(message) aFree(message);
	if(buf) aFree(buf);

	return 0;
}

int clif_message(struct block_list *bl, char* msg)
{
	if(msg)
	{
	unsigned short msg_len = strlen(msg) + 1;
	unsigned char buf[256];

		if(msg_len>256-8)	msg_len=256-8;

	nullpo_retr(0, bl);

	WBUFW(buf, 0) = 0x8d;
	WBUFW(buf, 2) = msg_len + 8;
	WBUFL(buf, 4) = bl->id;
	memcpy(WBUFP(buf, 8), msg, msg_len);
		buf[255] = 0;// force EOS

	clif_send(buf, WBUFW(buf,2), bl, AREA_CHAT_WOC);	// by Gengar
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_MapMove(int fd, struct map_session_data *sd) {
// /m /mapmove (as @rura GM command)
	char output[30]; // 17+4+4=26, 30 max.
	char map_name[17];

	nullpo_retr(0,sd);

	if( battle_config.atc_gmonly == 0 || 
	    pc_isGM(*sd) >= get_atcommand_level(AtCommand_MapMove) )
	{
		memcpy(map_name, RFIFOP(fd,2), 16);
		map_name[16]='\0';
		sprintf(output, "%s %d %d", map_name, (unsigned char)RFIFOW(fd,18), (unsigned char)RFIFOW(fd,20));
		atcommand_rura(fd, *sd, "@rura", output);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changed_dir(struct block_list *bl) {
	unsigned char buf[64];
	struct map_session_data *sd = NULL;

	if(!bl)	return 0;

	if (bl->type == BL_PC)
		nullpo_retr (0,sd=(struct map_session_data *)bl);

	WBUFW(buf,0) = 0x9c;
	WBUFL(buf,2) = bl->id;
		WBUFW(buf,6) = sd->head_dir;
	WBUFB(buf,8) = status_get_dir(bl);
	if(sd && sd->disguise_id > 23 && sd->disguise_id < 4001) // mob disguises [Valaris]
		return clif_send(buf, packet_len_table[0x9c], &sd->bl, AREA);
	else
		return clif_send(buf, packet_len_table[0x9c], bl, AREA_WOS);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_ChangeDir(int fd, struct map_session_data *sd)
{
	unsigned char bodydir, headdir;
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (USE_PACKET_DB(sd))
	{
		headdir = RFIFOB(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0]);
		bodydir = RFIFOB(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[1]);
	}
	else
	{
		switch (sd->packet_ver)
		{ // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 7:
			headdir = RFIFOB(fd,5);
			bodydir = RFIFOB(fd,12);
			break;
		case 8:
			headdir = RFIFOB(fd,5);
			bodydir = RFIFOB(fd,12);
			break;
		case 9:
			headdir = RFIFOB(fd,7);
			bodydir = RFIFOB(fd,11);
			break;
		case 10:
			headdir = RFIFOB(fd,4);
			bodydir = RFIFOB(fd,9);
			break;
		case 11:
			headdir = RFIFOB(fd,8);
			bodydir = RFIFOB(fd,17);
			break;
		case 12:
			headdir = RFIFOB(fd,5);
			bodydir = RFIFOB(fd,12);
			break;
		case 13:
		case 14:
			headdir = RFIFOB(fd,6);
			bodydir = RFIFOB(fd,14);
			break;
		case 15:
			headdir = RFIFOB(fd,3);
			bodydir = RFIFOB(fd,7);
			break;
		case 16:
			headdir = RFIFOB(fd,12);
			bodydir = RFIFOB(fd,22);
			break;
		case 17:
			headdir = RFIFOB(fd,7);
			bodydir = RFIFOB(fd,10);
			break;

		default: // old version by default (and packet version 6)
			headdir = RFIFOB(fd,2);
			bodydir = RFIFOB(fd,4);
			break;
		}
	}
	pc_setdir(*sd, bodydir, headdir);
	clif_changed_dir(&sd->bl);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_Emotion(int fd, struct map_session_data *sd) {
	unsigned char buf[64];
	if( !session_isActive(fd) )
		return 0;

	nullpo_retr(0,sd);

	if(battle_config.basic_skill_check == 0 || pc_checkskill(*sd, NV_BASIC) >= 2)
	{
		if(RFIFOB(fd,2) == 34)
		{	// prevent use of the mute emote [Valaris]
			return clif_skill_fail(sd, 1, 0, 1);
		}
		// fix flood of emotion icon (ro-proxy): flood only the hacker player
		if(sd->emotionlasttime >= time(NULL))
		{
			sd->emotionlasttime = time(NULL) + 2; // not more than 1 every 2 seconds (normal client is every 3-4 seconds)
			return clif_skill_fail(sd, 1, 0, 1);
		}
		sd->emotionlasttime = time(NULL) + 2; // not more than 1 every 2 seconds (normal client is every 3-4 seconds)

		WBUFW(buf,0) = 0xc0;
		WBUFL(buf,2) = sd->bl.id;
		WBUFB(buf,6) = RFIFOB(fd,2);
		return clif_send(buf, packet_len_table[0xc0], &sd->bl, AREA);
	} else
		return clif_skill_fail(sd, 1, 0, 1);
}

/*==========================================
 *
 *------------------------------------------
 */
int  clif_parse_HowManyConnections(int fd, struct map_session_data *sd) {

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0xc2;
	WFIFOL(fd,2) = map_getusers();
	WFIFOSET(fd,packet_len_table[0xc2]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_ActionRequest(int fd, struct map_session_data *sd) {
	unsigned long tick;
	unsigned char buf[64];
	int action_type;
	unsigned long target_id;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (pc_isdead(*sd)) {
		return clif_clearchar_area(&sd->bl, 1);
	}
	if (sd->npc_id != 0 || sd->opt1 > 0 || sd->status.option & 2 ||
	    (sd->sc_data &&
	     (sd->sc_data[SC_TRICKDEAD].timer != -1 ||
		  sd->sc_data[SC_AUTOCOUNTER].timer != -1 || //�I�[�g�J�E���^�[
	      sd->sc_data[SC_BLADESTOP].timer != -1 || //���n���
	      sd->sc_data[SC_DANCING].timer != -1))) //�_���X��
		return 0;

	tick = gettick();

	pc_stop_walking(*sd, 0);
	pc_stopattack(sd);

	if (USE_PACKET_DB(sd)) {
		target_id = RFIFOL(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0]);
		action_type = RFIFOB(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[1]);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 8:
			target_id = RFIFOL(fd,3);
			action_type = RFIFOB(fd,8);
			break;
		case 9:
			target_id = RFIFOL(fd,7);
			action_type = RFIFOB(fd,17);
			break;
		case 10:
			target_id = RFIFOL(fd,9);
			action_type = RFIFOB(fd,22);
			break;
		case 11:
			target_id = RFIFOL(fd,3);
			action_type = RFIFOB(fd,8);
			break;
		case 12:
			target_id = RFIFOL(fd,3);
			action_type = RFIFOB(fd,8);
			break;
		case 13:
		case 14:
			target_id = RFIFOL(fd,4);
			action_type = RFIFOB(fd,14);
			break;
		case 15:
			target_id = RFIFOL(fd,6);
			action_type = RFIFOB(fd,17);
			break;
		case 16:
			target_id = RFIFOL(fd,9);
			action_type = RFIFOB(fd,19);
			break;
		case 17:
			target_id = RFIFOL(fd,5);
			action_type = RFIFOB(fd,18);
			break;

		default: // old version by default (and packet version 6 and 7)
			target_id = RFIFOL(fd,2);
			action_type = RFIFOB(fd,6);
			break;
		}
	}

	switch(action_type) {
	case 0x00: // once attack
	case 0x07: // continuous attack


		if(sd->sc_data[SC_WEDDING].timer != -1 || sd->view_class==22)
			return 0;
		if (sd->vender_id != 0)
			return 0;
		if (!battle_config.sdelay_attack_enable && pc_checkskill(*sd, SA_FREECAST) <= 0) {
			if (DIFF_TICK(tick, sd->canact_tick) < 0) {
				return clif_skill_fail(sd, 1, 4, 0);
			}
		}
		if (sd->invincible_timer != -1)
			pc_delinvincibletimer(sd);
		if (sd->attacktarget > 0) // [Valaris]
			sd->attacktarget = 0;
		pc_attack(sd, target_id, action_type != 0);
		break;
	case 0x02: // sitdown
		if (battle_config.basic_skill_check == 0 || pc_checkskill(*sd, NV_BASIC) >= 3) {
			pc_stop_walking(*sd, 1);
			pc_setsit(*sd);
			skill_gangsterparadise(sd, 1); // �M�����O�X�^�[�p���_�C�X�ݒ� fixed Valaris
			clif_sitting(*sd);
		} else
			clif_skill_fail(sd, 1, 0, 2);
		break;
	case 0x03: // standup
		pc_setstand(*sd);
		skill_gangsterparadise(sd, 0); // �M�����O�X�^�[�p���_�C�X���� fixed Valaris
		WBUFW(buf, 0) = 0x8a;
		WBUFL(buf, 2) = sd->bl.id;
		WBUFB(buf,26) = 3;
		clif_send(buf, packet_len_table[0x8a], &sd->bl, AREA);
		break;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_Restart(int fd, struct map_session_data *sd) {
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	switch(RFIFOB(fd,2)) {
	case 0x00:
		if (pc_isdead(*sd)) {
			pc_setstand(*sd);
			pc_setrestartvalue(*sd, 3);
			pc_setpos(*sd, sd->status.save_point.map, sd->status.save_point.x, sd->status.save_point.y, 2);
		}
		// in case the player's status somehow wasn't updated yet [Celest]
		else if (sd->status.hp <= 0)
			pc_setdead(*sd);
		break;
	case 0x01:
		if(!pc_isdead(*sd) && (sd->opt1 || (sd->opt2 && !(night_flag == 1 && sd->opt2 == STATE_BLIND))))
			return 0;

		/*	Rovert's Prevent logout option - Fixed [Valaris]	*/
		if( ((gettick() - sd->canlog_tick) >= 10000) || (!battle_config.prevent_logout)) {
			chrif_charselectreq(*sd);
			session_SetWaitClose(fd, 1000);
		} else {
			WFIFOW(fd,0)=0x18b;
			WFIFOW(fd,2)=1;
			WFIFOSET(fd,packet_len_table[0x018b]);
		}
		break;
	}
	return 0;
}

/*==========================================
 * Transmission of a wisp (S 0096 <len>.w <nick>.24B <message>.?B)
 *------------------------------------------
 */
int clif_parse_Wis(int fd, struct map_session_data *sd)
{	// S 0096 <len>.w <nick>.24B <message>.?B // rewritten by [Yor]
	char *gm_command;
	struct map_session_data *dstsd;
	int i;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	//ShowMessage("clif_parse_Wis: message: '%s'.\n", RFIFOP(fd,28));

	gm_command = (char*)aMalloc( (strlen((char*)RFIFOP(fd,28)) + 28) * sizeof(char)); // 24+3+(RFIFOW(fd,2)-28)+1 or 24+3+(strlen(RFIFOP(fd,28))+1 (size can be wrong with hacker)

	sprintf(gm_command, "%s : %s", sd->status.name, RFIFOP(fd,28));
	if ((is_charcommand(fd, *sd, gm_command, 0) != CharCommand_None) ||
		(is_atcommand(fd, *sd, gm_command, 0) != AtCommand_None) ||
	    (sd && sd->sc_data &&
	     (sd->sc_data[SC_BERSERK].timer!=-1 || //�o�[�T�[�N���͉�b���s��
	      sd->sc_data[SC_NOCHAT].timer != -1))) //�`���b�g�֎~
	{
		if(gm_command) aFree(gm_command);
		return 0;
	}

	if(gm_command) aFree(gm_command);


	//Chat Logging type 'W' / Whisper
	log_chat("W", 0, sd->status.char_id, sd->status.account_id, (char*)sd->mapname, sd->bl.x, sd->bl.y, (char*)RFIFOP(fd, 4), (char*)RFIFOP(fd, 28));

	// searching destination character
	dstsd = map_nick2sd((char*)RFIFOP(fd,4));
	// player is not on this map-server
	if (dstsd == NULL ||
	// At this point, don't send wisp/page if it's not exactly the same name, because (example)
	// if there are 'Test' player on an other map-server and 'test' player on this map-server,
	// and if we ask for 'Test', we must not contact 'test' player
	// so, we send information to inter-server, which is the only one which decide (and copy correct name).
	    strcmp(dstsd->status.name, (char*)RFIFOP(fd,4)) != 0) // not exactly same name
		// send message to inter-server
		intif_wis_message(*sd, (char*)RFIFOP(fd,4), (char*)RFIFOP(fd,28), RFIFOW(fd,2)-28);
	// player is on this map-server
	else {
		// if you send to your self, don't send anything to others
		if (dstsd->fd == fd) // but, normaly, it's impossible!
			clif_wis_message(fd, wisp_server_name, "You can not page yourself. Sorry.", strlen("You can not page yourself. Sorry.") + 1);
		// otherwise, send message and answer immediatly
		else {
			if(dstsd->state.ignoreAll == 1) {
				if (dstsd->status.option & OPTION_HIDE && pc_isGM(*sd) < pc_isGM(*dstsd))
					clif_wis_end(fd, 1); // 1: target character is not loged in
				else
					clif_wis_end(fd, 3); // 3: everyone ignored by target
			} else {
				// if player ignore the source character
				for(i = 0; i < MAX_IGNORE_LIST; i++)
					if (strcmp(dstsd->ignore[i].name, sd->status.name) == 0) {
						clif_wis_end(fd, 2);	// 2: ignored by target
						break;
					}
				// if source player not found in ignore list
				if (i == MAX_IGNORE_LIST) {
					clif_wis_message(dstsd->fd, sd->status.name, (char*)RFIFOP(fd,28), RFIFOW(fd,2) - 28);
					clif_wis_end(fd, 0); // 0: success to send wisper
				}
			}
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_GMmessage(int fd, struct map_session_data *sd) {
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if( battle_config.atc_gmonly == 0 || 
	    pc_isGM(*sd) >= get_atcommand_level(AtCommand_Broadcast) )
		intif_GMmessage((char*)RFIFOP(fd,4), 0);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_TakeItem(int fd, struct map_session_data *sd) {
	struct flooritem_data *fitem;
	int map_object_id;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (USE_PACKET_DB(sd)) {
		map_object_id = RFIFOL(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0]);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 7:
			map_object_id = RFIFOL(fd,6);
			break;
		case 8:
			map_object_id = RFIFOL(fd,6);
			break;
		case 9:
			map_object_id = RFIFOL(fd,9);
			break;
		case 10:
			map_object_id = RFIFOL(fd,7);
			break;
		case 11:
			map_object_id = RFIFOL(fd,10);
			break;
		case 12:
			map_object_id = RFIFOL(fd,6);
			break;
		case 13:
		case 14:
			map_object_id = RFIFOL(fd,5);
			break;
		case 15:
			map_object_id = RFIFOL(fd,3);
			break;
		case 16:
			map_object_id = RFIFOL(fd,5);
			break;
		case 17:
			map_object_id = RFIFOL(fd,4);
			break;
		default: // old version by default (and packet version 6)
			map_object_id = RFIFOL(fd,2);
			break;
		}
	}
	fitem = (struct flooritem_data*)map_id2bl(map_object_id);

	if (pc_isdead(*sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return 0;
	}

	if( sd->npc_id!=0 || sd->vender_id != 0 || sd->opt1 > 0 ||
		(sd->sc_data && (sd->sc_data[SC_TRICKDEAD].timer != -1 || //���񂾂ӂ�
		sd->sc_data[SC_BLADESTOP].timer != -1 || //���n���
		sd->sc_data[SC_BERSERK].timer!=-1 ||	//�o�[�T�[�N
		sd->sc_data[SC_NOCHAT].timer!=-1 )) )	//��b�֎~
		return 0;

	if (fitem == NULL || fitem->bl.m != sd->bl.m)
		return 0;

	pc_takeitem(*sd, *fitem);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_DropItem(int fd, struct map_session_data *sd) {
	int item_index, item_amount;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (pc_isdead(*sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return 0;
	}
	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->opt1 > 0 ||
		(sd->sc_data && (sd->sc_data[SC_AUTOCOUNTER].timer != -1 || //�I�[�g�J�E���^�[
		sd->sc_data[SC_BLADESTOP].timer != -1 || //���n���
		sd->sc_data[SC_BERSERK].timer != -1)) ) //�o�[�T�[�N
		return 0;

	if (USE_PACKET_DB(sd)) {
		item_index = RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0])-2;
		item_amount = RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[1]);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 8:
			item_index = RFIFOW(fd,5) - 2;
			item_amount = RFIFOW(fd,12);
			break;
		case 9:
			item_index = RFIFOW(fd,8) - 2;
			item_amount = RFIFOW(fd,15);
			break;
		case 10:
			item_index = RFIFOW(fd,6) - 2;
			item_amount = RFIFOW(fd,15);
			break;
		case 11:
			item_index = RFIFOW(fd,12) - 2;
			item_amount = RFIFOW(fd,17);
			break;
		case 12:
			item_index = RFIFOW(fd,5) - 2;
			item_amount = RFIFOW(fd,12);
			break;
		case 13:
		case 14:
			item_index = RFIFOW(fd,6) - 2;
			item_amount = RFIFOW(fd,10);
			break;
		case 15:
			item_index = RFIFOW(fd,4) - 2;
			item_amount = RFIFOW(fd,10);
			break;
		case 16:
			item_index = RFIFOW(fd,15) - 2;
			item_amount = RFIFOW(fd,18);
			break;
		case 17:
			item_index = RFIFOW(fd,5) - 2;
			item_amount = RFIFOW(fd,8);
			break;
		default: // old version by default (+ packet version 6 and 7)
			item_index = RFIFOW(fd,2) - 2;
			item_amount = RFIFOW(fd,4);
			break;
		}
	}

	pc_dropitem(*sd, item_index, item_amount);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_UseItem(int fd, struct map_session_data *sd) {
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (pc_isdead(*sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return 0;
	}
	if (sd->npc_id!=0 || sd->vender_id != 0 || (sd->opt1 > 0 && sd->opt1 != 6) ||
	    (sd->sc_data && (sd->sc_data[SC_TRICKDEAD].timer != -1 || //���񂾂ӂ�
	     sd->sc_data[SC_BLADESTOP].timer != -1 || //���n���
		sd->sc_data[SC_BERSERK].timer!=-1 ||	//�o�[�T�[�N
		sd->sc_data[SC_NOCHAT].timer!=-1 ||
		sd->sc_data[SC_GRAVITATION].timer!=-1)) )	//��b�֎~
		return 0;

	if (sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);

	if (USE_PACKET_DB(sd)) {
		pc_useitem(sd,RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0])-2);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 6:
			pc_useitem(sd,RFIFOW(fd,5)-2);
			break;
		case 7:
			pc_useitem(sd,RFIFOW(fd,6)-2);
			break;
		case 8:
			pc_useitem(sd,RFIFOW(fd,6)-2);
			break;
		case 9:
			pc_useitem(sd,RFIFOW(fd,9)-2);
			break;
		case 10:
			pc_useitem(sd,RFIFOW(fd,7)-2);
			break;
		case 11:
			pc_useitem(sd,RFIFOW(fd,10)-2);
			break;
		case 12:
			pc_useitem(sd,RFIFOW(fd,6)-2);
			break;
		case 13:
		case 14:
			pc_useitem(sd,RFIFOW(fd,5)-2);
			break;
		case 15:
			pc_useitem(sd,RFIFOW(fd,3)-2);
			break;
		case 16:
			pc_useitem(sd,RFIFOW(fd,5)-2);
			break;
		case 17:
			pc_useitem(sd,RFIFOW(fd,4)-2);
			break;
		default: // old version by default
			pc_useitem(sd,RFIFOW(fd,2)-2);
			break;
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_EquipItem(int fd,struct map_session_data *sd)
{
	int index;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(pc_isdead(*sd)) {
		clif_clearchar_area(&sd->bl,1);
		return 0;
	}
	index = RFIFOW(fd,2)-2;
	if(sd->npc_id!=0 || sd->vender_id != 0)
		return 0;
	if(sd->sc_data && ( sd->sc_data[SC_BLADESTOP].timer!=-1 || sd->sc_data[SC_BERSERK].timer!=-1 )) 
		return 0;

	if(sd->status.inventory[index].identify != 1) {		// ���Ӓ�
		return clif_equipitemack(sd,index,0,0);	// fail
	}
	//�y�b�g�p�����ł��邩�Ȃ���
	if(sd->inventory_data[index]) {
		if(sd->inventory_data[index]->type != 8){
			if(sd->inventory_data[index]->type == 10)
				RFIFOW(fd,4)=0x8000;	// ��𖳗���葕���ł���悤�Ɂi�|�|�G
			pc_equipitem(sd,index,RFIFOW(fd,4));
		} else
			pet_equipitem(*sd,index);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_UnequipItem(int fd,struct map_session_data *sd)
{
	int index;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(pc_isdead(*sd)) {
		clif_clearchar_area(&sd->bl,1);
		return 0;
	}
	if(sd->npc_id!=0 || sd->vender_id != 0 || sd->opt1 > 0)
		return 0;
	index = RFIFOW(fd,2)-2;

	/*if(sd->status.inventory[index].attribute == 1 && sd->sc_data && sd->sc_data[SC_BROKNWEAPON].timer!=-1)
		status_change_end(&sd->bl,SC_BROKNWEAPON,-1);
	if(sd->status.inventory[index].attribute == 1 && sd->sc_data && sd->sc_data[SC_BROKNARMOR].timer!=-1)
		status_change_end(&sd->bl,SC_BROKNARMOR,-1);
	if(sd->sc_count && ( sd->sc_data[SC_BLADESTOP].timer!=-1 || sd->sc_data[SC_BERSERK].timer!=-1 ))
		return 0;*/

	pc_unequipitem(sd,index,1);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_NpcClicked(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(pc_isdead(*sd)) {
		clif_clearchar_area(&sd->bl,1);
		return 0;
	}
	if(sd->npc_id!=0 || sd->vender_id != 0)
		return 0;
	npc_click(*sd,RFIFOL(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_NpcBuySellSelected(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	npc_buysellsel(*sd,RFIFOL(fd,2),RFIFOB(fd,6));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_NpcBuyListSend(int fd,struct map_session_data *sd)
{
	int fail=3,n;
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	n = (RFIFOW(fd,2)-4) /4;
	fail = npc_buylist(*sd,n,RFIFOP(fd,4));
	WFIFOW(fd,0)=0xca;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xca]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_NpcSellListSend(int fd,struct map_session_data *sd)
{
	int fail=1,n;
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	n = (RFIFOW(fd,2)-4) /4;
	fail = npc_selllist(*sd,n,RFIFOP(fd,4));

	WFIFOW(fd,0)=0xcb;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xcb]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_CreateChatRoom(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(battle_config.basic_skill_check == 0 || pc_checkskill(*sd,NV_BASIC) >= 4){
		chat_createchat(*sd,RFIFOW(fd,4),RFIFOB(fd,6),(char*)RFIFOP(fd,7),(char*)RFIFOP(fd,15),RFIFOW(fd,2)-15);
	} else
		clif_skill_fail(sd,1,0,3);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_ChatAddMember(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	chat_joinchat(*sd,RFIFOL(fd,2),(char*)RFIFOP(fd,6));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_ChatRoomStatusChange(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	chat_changechatstatus(*sd,RFIFOW(fd,4),RFIFOB(fd,6),(char*)RFIFOP(fd,7),(char*)RFIFOP(fd,15),RFIFOW(fd,2)-15);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_ChangeChatOwner(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	chat_changechatowner(*sd,(char*)RFIFOP(fd,6));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_KickFromChat(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	chat_kickchat(*sd,(char*)RFIFOP(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_ChatLeave(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	chat_leavechat(*sd);
	return 0;
}

/*==========================================
 * ����v���𑊎�ɑ���
 *------------------------------------------
 */
int clif_parse_TradeRequest(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(battle_config.basic_skill_check == 0 || pc_checkskill(*sd,NV_BASIC) >= 1){
		trade_traderequest(*sd,RFIFOL(sd->fd,2));
	} else
		clif_skill_fail(sd,1,0,0);
	return 0;
}

/*==========================================
 * ����v��
 *------------------------------------------
 */
int clif_parse_TradeAck(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	trade_tradeack(*sd,RFIFOB(sd->fd,2));
	return 0;
}

/*==========================================
 * �A�C�e���ǉ�
 *------------------------------------------
 */
int clif_parse_TradeAddItem(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	trade_tradeadditem(*sd,RFIFOW(sd->fd,2),RFIFOL(sd->fd,4));
	return 0;
}

/*==========================================
 * �A�C�e���ǉ�����(ok����)
 *------------------------------------------
 */
int clif_parse_TradeOk(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	trade_tradeok(*sd);
	return 0;
}

/*==========================================
 * ����L�����Z��
 *------------------------------------------
 */
int clif_parse_TradeCancel(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	trade_tradecancel(*sd);
	return 0;
}

/*==========================================
 * �������(trade����)
 *------------------------------------------
 */
int clif_parse_TradeCommit(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	trade_tradecommit(*sd);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_StopAttack(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	pc_stopattack(sd);
	return 0;
}

/*==========================================
 * �J�[�g�փA�C�e�����ڂ�
 *------------------------------------------
 */
int clif_parse_PutItemToCart(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(sd->npc_id!=0 || sd->vender_id != 0)
		return 0;
	pc_putitemtocart(*sd,RFIFOW(fd,2)-2,RFIFOL(fd,4));
	return 0;
}
/*==========================================
 * �J�[�g����A�C�e�����o��
 *------------------------------------------
 */
int clif_parse_GetItemFromCart(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(sd->npc_id!=0 || sd->vender_id != 0) return 0;
	pc_getitemfromcart(*sd,RFIFOW(fd,2)-2,RFIFOL(fd,4));
	return 0;
}

/*==========================================
 * �t���i(��,�y�R,�J�[�g)���͂���
 *------------------------------------------
 */
int clif_parse_RemoveOption(int fd,struct map_session_data *sd)
{
	if(pc_isriding(*sd)) {	// jobchange when removing peco [Valaris]
		if(sd->status.class_==13)
			sd->status.class_=sd->view_class=7;

		if(sd->status.class_==21)
			sd->status.class_=sd->view_class=14;

		if(sd->status.class_==4014)
			sd->status.class_=sd->view_class=4008;

		if(sd->status.class_==4022)
			sd->status.class_=sd->view_class=4015;
	}

	pc_setoption(sd,0);
	return 0;
}

/*==========================================
 * �`�F���W�J�[�g
 *------------------------------------------
 */
int clif_parse_ChangeCart(int fd,struct map_session_data *sd)
{
	if( !session_isActive(fd) )
		return 0;

	pc_setcart(sd,RFIFOW(fd,2));
	return 0;
}

/*==========================================
 * �X�e�[�^�X�A�b�v
 *------------------------------------------
 */
int clif_parse_StatusUp(int fd,struct map_session_data *sd)
{
		return 0;

		pc_statusup(sd,RFIFOW(fd,2));
	return 0;
}

/*==========================================
 * �X�L�����x���A�b�v
 *------------------------------------------
 */
int clif_parse_SkillUp(int fd,struct map_session_data *sd)
{
	if( !session_isActive(fd) )
		return 0;

	pc_skillup(sd,RFIFOW(fd,2));
	return 0;
}

/*==========================================
 * �X�L���g�p�iID�w��j
 *------------------------------------------
 */
int clif_parse_UseSkillToId(int fd, struct map_session_data *sd) {
	unsigned short skillnum, skilllv, lv;
	unsigned long target_id;
	unsigned long tick = gettick();

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (sd->chatID || sd->npc_id != 0 || sd->vender_id != 0)
		return 0;

	if (USE_PACKET_DB(sd)) {
		skilllv = RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0]);
		skillnum = RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[1]);
		target_id = RFIFOL(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[2]);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 6:
			skilllv = RFIFOW(fd,4);
			skillnum = RFIFOW(fd,9);
			target_id = RFIFOL(fd,11);
			break;
		case 7:
			skilllv = RFIFOW(fd,7);
			skillnum = RFIFOW(fd,9);
			target_id = RFIFOL(fd,15);
			break;
		case 8:
			skilllv = RFIFOW(fd,7);
			skillnum = RFIFOW(fd,12);
			target_id = RFIFOL(fd,16);
			break;
		case 9:
			skilllv = RFIFOW(fd,11);
			skillnum = RFIFOW(fd,18);
			target_id = RFIFOL(fd,22);
			break;
		case 10:
			skilllv = RFIFOW(fd,9);
			skillnum = RFIFOW(fd,15);
			target_id = RFIFOL(fd,18);
			break;
		case 11:
			skilllv = RFIFOW(fd,4);
			skillnum = RFIFOW(fd,7);
			target_id = RFIFOL(fd,10);
			break;
		case 12:
			skilllv = RFIFOW(fd,7);
			skillnum = RFIFOW(fd,12);
			target_id = RFIFOL(fd,16);
			break;
		case 13:
		case 14:
			skilllv = RFIFOW(fd,4);
			skillnum = RFIFOW(fd,10);
			target_id = RFIFOL(fd,22);
			break;
		case 15:
			skilllv = RFIFOW(fd,8);
			skillnum = RFIFOW(fd,12);
			target_id = RFIFOL(fd,18);
			break;
		case 16:
			skilllv = RFIFOW(fd,8);
			skillnum = RFIFOW(fd,16);
			target_id = RFIFOL(fd,22);
			break;
		case 17:
			skilllv = RFIFOW(fd,6);
			skillnum = RFIFOW(fd,10);
			target_id = RFIFOL(fd,21);
			break;
		default: // old version by default
			skilllv = RFIFOW(fd,2);
			skillnum = RFIFOW(fd,4);
			target_id = RFIFOL(fd,6);
			break;
		}
	}

	if (skillnotok(skillnum, *sd))
		return 0;

	if (sd->skilltimer != -1) {
		if (skillnum != SA_CASTCANCEL)
			return 0;
	} else if (DIFF_TICK(tick, sd->canact_tick) < 0 &&
		// allow monk combos to ignore this delay [celest]
		!(//!!sd->sc_count && 
		sd->sc_data[SC_COMBO].timer!=-1 &&
		(skillnum == MO_EXTREMITYFIST ||
		skillnum == MO_CHAINCOMBO ||
		skillnum == MO_COMBOFINISH ||
		skillnum == CH_PALMSTRIKE ||
		skillnum == CH_TIGERFIST ||
		skillnum == CH_CHAINCRUSH))) {
		clif_skill_fail(sd, skillnum, 4, 0);
		return 0;
	}

	if ((sd->sc_data[SC_TRICKDEAD].timer != -1 && skillnum != NV_TRICKDEAD) ||
	    sd->sc_data[SC_BERSERK].timer != -1 || sd->sc_data[SC_NOCHAT].timer != -1 ||
	    sd->sc_data[SC_WEDDING].timer != -1 || sd->view_class == 22)
		return 0;
	if (sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);
	if(sd->skillitem == skillnum) {
		if (skilllv != sd->skillitemlv)
			skilllv = sd->skillitemlv;
		skill_use_id(sd, target_id, skillnum, skilllv);
	} else {
		sd->skillitem = sd->skillitemlv = 0xFFFF;
		if (skillnum == MO_EXTREMITYFIST) {
			if ((sd->sc_data[SC_COMBO].timer == -1 || (sd->sc_data[SC_COMBO].val1 != MO_COMBOFINISH && sd->sc_data[SC_COMBO].val1 != CH_CHAINCRUSH))) {
				if (!sd->state.skill_flag ) {
					sd->state.skill_flag = 1;
					clif_skillinfo(sd, MO_EXTREMITYFIST, 1, -1);
					return 0;
				} else if (sd->bl.id == target_id) {
					clif_skillinfo(sd, MO_EXTREMITYFIST, 1, -1);
					return 0;
				}
			}
		} else if (skillnum == CH_TIGERFIST) {
			if (sd->sc_data[SC_COMBO].timer == -1 || sd->sc_data[SC_COMBO].val1 != MO_COMBOFINISH) {
				if (!sd->state.skill_flag ) {
					sd->state.skill_flag = 1;
					if (!sd->attacktarget) {
						clif_skillinfo(sd, CH_TIGERFIST, 1, -2);
						return 0;
					} else
						target_id = sd->attacktarget;
				} else if (sd->bl.id == target_id) {
					clif_skillinfo(sd, CH_TIGERFIST, 1, -2);
					return 0;
				}
			}
		}
		if ((lv = pc_checkskill(*sd, skillnum)) > 0) {
			if (skilllv > lv)
				skilllv = lv;
			skill_use_id(sd, target_id, skillnum, skilllv);
			if (sd->state.skill_flag)
				sd->state.skill_flag = 0;
		}
	}
	return 0;
}

/*==========================================
 * �X�L���g�p�i�ꏊ�w��j
 *------------------------------------------
 */
int clif_parse_UseSkillToPos(int fd, struct map_session_data *sd) {
	unsigned short skillnum, skilllv, lv, x, y;
	unsigned long tick = gettick();
	int skillmoreinfo;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(sd->npc_id != 0 || sd->vender_id != 0) return 0;
	if(sd->chatID) return 0;

	skillmoreinfo = -1;
	if (USE_PACKET_DB(sd)) {
		skilllv = RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0]);
		skillnum = RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[1]);
		x = RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[2]);
		y = RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[3]);
		if (packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[4] > 0)
			skillmoreinfo = packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[4];
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 6:
			skilllv = RFIFOW(fd,4);
			skillnum = RFIFOW(fd,9);
			x = RFIFOW(fd,11);
			y = RFIFOW(fd,13);
			if (RFIFOW(fd,0) == 0x190)
				skillmoreinfo = 15;
			break;
		case 7:
			skilllv = RFIFOW(fd,7);
			skillnum = RFIFOW(fd,9);
			x = RFIFOW(fd,15);
			y = RFIFOW(fd,17);
			if (RFIFOW(fd,0) == 0x190)
				skillmoreinfo = 19;
			break;
		case 8:
			skilllv = RFIFOW(fd,3);
			skillnum = RFIFOW(fd,6);
			x = RFIFOW(fd,17);
			y = RFIFOW(fd,21);
			if (RFIFOW(fd,0) == 0x0a2)
				skillmoreinfo = 23;
			break;
		case 9:
			skilllv = RFIFOW(fd,5);
			skillnum = RFIFOW(fd,15);
			x = RFIFOW(fd,29);
			y = RFIFOW(fd,38);
			if (RFIFOW(fd,0) == 0x0a2)
				skillmoreinfo = 40;
			break;
		case 10:
			skilllv = RFIFOW(fd,10);
			skillnum = RFIFOW(fd,14);
			x = RFIFOW(fd,18);
			y = RFIFOW(fd,23);
			if (RFIFOW(fd,0) == 0x08c)
				skillmoreinfo = 25;
			break;
		case 11:
			skilllv = RFIFOW(fd,6); // 16? to check.
			skillnum = RFIFOW(fd,20);
			x = RFIFOW(fd,23);
			y = RFIFOW(fd,27);
			if (RFIFOW(fd,0) == 0x08c)
				skillmoreinfo = 29;
			break;
		case 12:
			skilllv = RFIFOW(fd,3); // 2? to check.
			skillnum = RFIFOW(fd,6);
			x = RFIFOW(fd,17);
			y = RFIFOW(fd,21);
			if (RFIFOW(fd,0) == 0x08c)
				skillmoreinfo = 23;
			break;
		case 13:
		case 14:
			skilllv = RFIFOW(fd,6);
			skillnum = RFIFOW(fd,9);
			x = RFIFOW(fd,23);
			y = RFIFOW(fd,26);
			if (RFIFOW(fd,0) == 0x08c)
				skillmoreinfo = 28;
			break;
		case 15:
			skilllv = RFIFOW(fd,4);
			skillnum = RFIFOW(fd,9);
			x = RFIFOW(fd,22);
			y = RFIFOW(fd,28);
			if (RFIFOW(fd,0) == 0x113)
				skillmoreinfo = 30;
			break;
		case 16:
			skilllv = RFIFOW(fd,10);
			skillnum = RFIFOW(fd,18);
			x = RFIFOW(fd,22);
			y = RFIFOW(fd,32);
			if (RFIFOW(fd,0) == 0x07e)
				skillmoreinfo = 34;
			break;
		case 17:
			skilllv = RFIFOW(fd,5);
			skillnum = RFIFOW(fd,9);
			x = RFIFOW(fd,12);
			y = RFIFOW(fd,20);
			if (RFIFOW(fd,0) == 0x07e)
				skillmoreinfo = 22;
			break;
		default: // old version by default
			skilllv = RFIFOW(fd,2);
			skillnum = RFIFOW(fd,4);
			x = RFIFOW(fd,6);
			y = RFIFOW(fd,8);
			if (RFIFOW(fd,0) == 0x190)
				skillmoreinfo = 10;
			break;
		}
	}

	if (skillnotok(skillnum, *sd))
		return 0;

	if (skillmoreinfo != -1) {
		if (pc_issit(*sd)) {
			clif_skill_fail(sd, skillnum, 0, 0);
			return 0;
		}
		memcpy(talkie_mes, RFIFOP(fd,skillmoreinfo), 80);
	}

	if (sd->skilltimer != -1)
		return 0;
	else if (DIFF_TICK(tick, sd->canact_tick) < 0 &&
		// allow monk combos to ignore this delay [celest]
		!(//!!sd->sc_count && 
		sd->sc_data[SC_COMBO].timer!=-1 &&
		(skillnum == MO_EXTREMITYFIST ||
		skillnum == MO_CHAINCOMBO ||
		skillnum == MO_COMBOFINISH ||
		skillnum == CH_PALMSTRIKE ||
		skillnum == CH_TIGERFIST ||
		skillnum == CH_CHAINCRUSH))) {
		clif_skill_fail(sd, skillnum, 4, 0);
		return 0;
	}

	if ((sd->sc_data[SC_TRICKDEAD].timer != -1 && skillnum != NV_TRICKDEAD) ||
	    sd->sc_data[SC_BERSERK].timer != -1 || sd->sc_data[SC_NOCHAT].timer != -1 ||
	    sd->sc_data[SC_WEDDING].timer != -1 || sd->view_class == 22)
		return 0;
	if (sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);
	if(sd->skillitem == skillnum) {
		if (skilllv != sd->skillitemlv)
			skilllv = sd->skillitemlv;
		skill_use_pos(sd, x, y, skillnum, skilllv);
	} else {
		sd->skillitem = sd->skillitemlv = 0xFFFF;
		if ((lv = pc_checkskill(*sd, skillnum)) > 0) {
			if (skilllv > lv)
				skilllv = lv;
			skill_use_pos(sd, x, y, skillnum,skilllv);
		}
	}
	return 0;
}

/*==========================================
 * �X�L���g�p�imap�w��j
 *------------------------------------------
 */
int clif_parse_UseSkillMap(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(sd->chatID) return 0;

	if (sd->npc_id!=0 || sd->vender_id != 0 || (sd->sc_data &&
		(sd->sc_data[SC_TRICKDEAD].timer != -1 ||
		sd->sc_data[SC_BERSERK].timer!=-1 ||
		sd->sc_data[SC_NOCHAT].timer!=-1 ||
		sd->sc_data[SC_WEDDING].timer!=-1 ||
		sd->view_class==22)))
		return 0;

	if(sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);

	skill_castend_map(sd,RFIFOW(fd,2),(char*)RFIFOP(fd,4));
	return 0;
}
/*==========================================
 * �����v��
 *------------------------------------------
 */
int clif_parse_RequestMemo(int fd,struct map_session_data *sd)
{
	if(sd && !pc_isdead(*sd))
		pc_memo(*sd,-1);
	return 0;
}
/*==========================================
 * �A�C�e������
 *------------------------------------------
 */
int clif_parse_ProduceMix(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	sd->state.produce_flag = 0;
	skill_produce_mix(sd,RFIFOW(fd,2),RFIFOW(fd,4),RFIFOW(fd,6),RFIFOW(fd,8));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_NpcSelectMenu(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	sd->npc_menu=RFIFOB(fd,6);
	npc_scriptcont(*sd,RFIFOL(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_NpcNextClicked(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	npc_scriptcont(*sd,RFIFOL(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_NpcAmountInput(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	sd->npc_amount=RFIFOL(fd,6);
	npc_scriptcont(*sd,RFIFOL(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_NpcStringInput(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if((size_t)RFIFOW(fd,2) >= 7+sizeof(sd->npc_str)){
		ShowMessage("clif: input string too long !\n");
		memcpy(sd->npc_str,RFIFOP(fd,8),sizeof(sd->npc_str));
		sd->npc_str[sizeof(sd->npc_str)-1]=0;
	} else
		strcpy(sd->npc_str,(char*)RFIFOP(fd,8));
	npc_scriptcont(*sd,RFIFOL(fd,4));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_parse_NpcCloseClicked(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	npc_scriptcont(*sd,RFIFOL(fd,2));
	return 0;
}

/*==========================================
 * �A�C�e���Ӓ�
 *------------------------------------------
 */
int clif_parse_ItemIdentify(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	pc_item_identify(*sd,RFIFOW(fd,2)-2);
	return 0;
}
/*==========================================
 * ��쐬
 *------------------------------------------
 */
int clif_parse_SelectArrow(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	sd->state.produce_flag = 0;
	skill_arrow_create(sd,RFIFOW(fd,2));
	return 0;
}
/*==========================================
 * �I�[�g�X�y����M
 *------------------------------------------
 */
int clif_parse_AutoSpell(int fd,struct map_session_data *sd)
{
	if( !session_isActive(fd) )
		return 0;

	skill_autospell(sd,RFIFOW(fd,2));
	return 0;
}
/*==========================================
 * �J�[�h�g�p
 *------------------------------------------
 */
int clif_parse_UseCard(int fd,struct map_session_data *sd)
{
	if( !session_isActive(fd) )
		return 0;

	clif_use_card(sd,RFIFOW(fd,2)-2);
	return 0;
}
/*==========================================
 * �J�[�h�}�������I��
 *------------------------------------------
 */
int clif_parse_InsertCard(int fd,struct map_session_data *sd)
{
	nullpo_retr(0, sd);
	if( !session_isActive(fd) )
		return 0;

	pc_insert_card(*sd,RFIFOW(fd,2)-2,RFIFOW(fd,4)-2);
	return 0;
}

/*==========================================
 * 0193 �L����ID���O����
 *------------------------------------------
 */
int clif_parse_SolveCharName(int fd, struct map_session_data *sd) {
	int char_id;
	if( !session_isActive(fd) )
		return 0;

	if (USE_PACKET_DB(sd)) {
		char_id = RFIFOL(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0]);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 8:
			char_id = RFIFOL(fd,8);
			break;
		case 9:
			char_id = RFIFOL(fd,7);
			break;
		case 10:
			char_id = RFIFOL(fd,10);
			break;
		case 11:
			char_id = RFIFOL(fd,6);
			break;
		case 12:
			char_id = RFIFOL(fd,8);
			break;
		case 13:
		case 14:
			char_id = RFIFOL(fd,12);
			break;
		case 15:
			char_id = RFIFOL(fd,10);
			break;
		case 16:
			char_id = RFIFOL(fd,7);
			break;
		case 17:
			char_id = RFIFOL(fd,11);
			break;
		default: // old version by default (+ packet version 6 and 7)
			char_id = RFIFOL(fd,2);
			break;
		}
	}
	return clif_solved_charname(sd, char_id);
}

/*==========================================
 * 0197 /resetskill /resetstate
 *------------------------------------------
 */
int clif_parse_ResetChar(int fd, struct map_session_data *sd) {
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if( battle_config.atc_gmonly == 0 || 
		pc_isGM(*sd) >= get_atcommand_level(AtCommand_ResetState) )
	{
		switch(RFIFOW(fd,2)){
		case 0:
			pc_resetstate(sd);
			break;
		case 1:
			pc_resetskill(sd);
			break;
		}
	}
	return 0;
}

/*==========================================
 * 019c /lb��
 *------------------------------------------
 */
int clif_parse_LGMmessage(int fd, struct map_session_data *sd) {
	unsigned char buf[512];

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if( battle_config.atc_gmonly == 0 || 
	    pc_isGM(*sd) >= get_atcommand_level(AtCommand_LocalBroadcast) )
	{
		WBUFW(buf,0) = 0x9a;
		WBUFW(buf,2) = RFIFOW(fd,2);
		memcpy(WBUFP(buf,4), RFIFOP(fd,4), RFIFOW(fd,2) - 4);
		clif_send(buf, RFIFOW(fd,2), &sd->bl, ALL_SAMEMAP);
	}
	return 0;
}

/*==========================================
 * �J�v���q�ɂ֓����
 *------------------------------------------
 */
int clif_parse_MoveToKafra(int fd, struct map_session_data *sd) {
	int item_index, item_amount;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (sd->npc_id != 0 || sd->vender_id != 0)
		return 0;

	if (USE_PACKET_DB(sd)) {
		item_index = RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0])-2;
		item_amount = RFIFOL(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[1]);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 8:
			item_index = RFIFOW(fd,5) - 2;
			item_amount = RFIFOL(fd,12);
			break;
		case 9:
			item_index = RFIFOW(fd,5) - 2;
			item_amount = RFIFOL(fd,19);
			break;
		case 10:
			item_index = RFIFOW(fd,3) - 2;
			item_amount = RFIFOL(fd,15);
			break;
		case 11:
			item_index = RFIFOW(fd,6) - 2;
			item_amount = RFIFOL(fd,21);
			break;
		case 12:
			item_index = RFIFOW(fd,5) - 2;
			item_amount = RFIFOL(fd,12);
			break;
		case 13:
		case 14:
			item_index = RFIFOW(fd,6) - 2;
			item_amount = RFIFOL(fd,9);
			break;
		case 15:
			item_index = RFIFOW(fd,4) - 2;
			item_amount = RFIFOL(fd,10);
			break;
		case 16:
			item_index = RFIFOW(fd,10) - 2;
			item_amount = RFIFOL(fd,16);
			break;
		case 17:
			item_index = RFIFOW(fd,7) - 2;
			item_amount = RFIFOL(fd,10);
			break;
		default: // old version by default (+ packet version 6 and 7)
			item_index = RFIFOW(fd,2) - 2;
			item_amount = RFIFOL(fd,4);
			break;
		}
	}

	if (item_index < 0 || item_index >= MAX_INVENTORY)
		return 0;

	if( !itemdb_isdropable(sd->status.inventory[item_index].nameid) )
		return 0;

	if (sd->state.storage_flag)
		storage_guild_storageadd(*sd, item_index, item_amount);
	else
		storage_storageadd(*sd, item_index, item_amount);
	return 0;
}

/*==========================================
 * �J�v���q�ɂ���o��
 *------------------------------------------
 */
int clif_parse_MoveFromKafra(int fd,struct map_session_data *sd) {
	int item_index, item_amount;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (USE_PACKET_DB(sd)) {
		item_index = RFIFOW(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[0])-1;
		item_amount = RFIFOL(fd,packet_db[clif_config.packet_db_ver][RFIFOW(fd,0)].pos[1]);
	} else {
		switch (sd->packet_ver) { // 5: old, 6: 7july04, 7: 13july04, 8: 26july04, 9: 9aug04/16aug04/17aug04, 10: 6sept04, 11: 21sept04, 12: 18oct04, 13: 25oct04 (by [Yor])
		case 8:
			item_index = RFIFOW(fd,10) - 1;
			item_amount = RFIFOL(fd,22);
			break;
		case 9:
			item_index = RFIFOW(fd,11) - 1;
			item_amount = RFIFOL(fd,22);
			break;
		case 10:
			item_index = RFIFOW(fd,3) - 1;
			item_amount = RFIFOL(fd,13);
			break;
		case 11:
			item_index = RFIFOW(fd,4) - 1;
			item_amount = RFIFOL(fd,8);
			break;
		case 12:
			item_index = RFIFOW(fd,10) - 1;
			item_amount = RFIFOL(fd,22);
			break;
		case 13:
		case 14:
			item_index = RFIFOW(fd,12) - 1;
			item_amount = RFIFOL(fd,18);
			break;
		case 15:
			item_index = RFIFOW(fd,4) - 1;
			item_amount = RFIFOL(fd,17);
			break;
		case 16:
			item_index = RFIFOW(fd,11) - 1;
			item_amount = RFIFOL(fd,17);
			break;
		case 17:
			item_index = RFIFOW(fd,14) - 1;
			item_amount = RFIFOL(fd,18);
			break;
		default: // old version by default (+ packet version 6 and 7)
			item_index = RFIFOW(fd,2) - 1;
			item_amount = RFIFOL(fd,4);
			break;
		}
	}

	if (sd->npc_id != 0 || sd->vender_id != 0)
		return 0;

	if (sd->state.storage_flag)
		storage_guild_storageget(*sd, item_index, item_amount);
	else
		storage_storageget(*sd, item_index, item_amount);
	return 0;
}

/*==========================================
 * �J�v���q�ɂփJ�[�g��������
 *------------------------------------------
 */
int clif_parse_MoveToKafraFromCart(int fd, struct map_session_data *sd) {
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->trade_partner != 0)
		return 0;
	if (sd->state.storage_flag)
		storage_guild_storageaddfromcart(*sd, RFIFOW(fd,2) - 2, RFIFOL(fd,4));
	else
		storage_storageaddfromcart(*sd, RFIFOW(fd,2) - 2, RFIFOL(fd,4));
	return 0;
}

/*==========================================
 * �J�v���q�ɂ���o��
 *------------------------------------------
 */
int clif_parse_MoveFromKafraToCart(int fd, struct map_session_data *sd) {
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (sd->npc_id != 0 || sd->vender_id != 0)
		return 0;
	if (sd->state.storage_flag)
		storage_guild_storagegettocart(*sd, RFIFOW(fd,2)-1, RFIFOL(fd,4));
	else
		storage_storagegettocart(*sd, RFIFOW(fd,2)-1, RFIFOL(fd,4));
	return 0;
}

/*==========================================
 * �J�v���q�ɂ����
 *------------------------------------------
 */
int clif_parse_CloseKafra(int fd, struct map_session_data *sd) {
	nullpo_retr(0,sd);

	if (sd->state.storage_flag)
		storage_guild_storageclose(*sd);
	else
		storage_storageclose(*sd);
	return 0;
}

/*==========================================
 * �p�[�e�B�����
 *------------------------------------------
 */
int clif_parse_CreateParty(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (battle_config.basic_skill_check == 0 || pc_checkskill(*sd,NV_BASIC) >= 7) {
		party_create(*sd,(const char*)RFIFOP(fd,2),0,0);
	} else
		clif_skill_fail(sd,1,0,4);
	return 0;
}

/*==========================================
 * �p�[�e�B�����
 *------------------------------------------
 */
int clif_parse_CreateParty2(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (battle_config.basic_skill_check == 0 || pc_checkskill(*sd,NV_BASIC) >= 7){
		party_create(*sd,(const char*)RFIFOP(fd,2),RFIFOB(fd,26),RFIFOB(fd,27));
	} else
		clif_skill_fail(sd,1,0,4);
	return 0;
}

/*==========================================
 * �p�[�e�B�Ɋ��U
 *------------------------------------------
 */
int clif_parse_PartyInvite(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	party_invite(*sd, RFIFOL(fd,2));
	return 0;
}

/*==========================================
 * �p�[�e�B���U�ԓ�
 *------------------------------------------
 */
int clif_parse_ReplyPartyInvite(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(battle_config.basic_skill_check == 0 || pc_checkskill(*sd,NV_BASIC) >= 5){
		party_reply_invite(*sd,RFIFOL(fd,2),RFIFOL(fd,6));
	} else {
		party_reply_invite(*sd,RFIFOL(fd,2),-1);
		clif_skill_fail(sd,1,0,4);
	}
	return 0;
}

/*==========================================
 * �p�[�e�B�E�ޗv��
 *------------------------------------------
 */
int clif_parse_LeaveParty(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	party_leave(*sd);
	return 0;
}

/*==========================================
 * �p�[�e�B�����v��
 *------------------------------------------
 */
int clif_parse_RemovePartyMember(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	party_removemember(*sd,RFIFOL(fd,2),(char*)RFIFOP(fd,6));
	return 0;
}

/*==========================================
 * �p�[�e�B�ݒ�ύX�v��
 *------------------------------------------
 */
int clif_parse_PartyChangeOption(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	party_changeoption(*sd, RFIFOW(fd,2), RFIFOW(fd,4));
	return 0;
}

/*==========================================
 * �p�[�e�B���b�Z�[�W���M�v��
 *------------------------------------------
 */
int clif_parse_PartyMessage(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;


	if (is_charcommand(fd, *sd, (char*)RFIFOP(fd,4), 0) != CharCommand_None ||
		is_atcommand(fd, *sd, (char*)RFIFOP(fd,4), 0) != AtCommand_None ||
		(sd->sc_data && 
		(sd->sc_data[SC_BERSERK].timer!=-1 ||	//�o�[�T�[�N���͉�b���s��
		sd->sc_data[SC_NOCHAT].timer!=-1)))		//�`���b�g�֎~
		return 0;

	party_send_message(*sd, (const char*)RFIFOP(fd,4), RFIFOW(fd,2)-4);
	return 0;
}

/*==========================================
 * �I�X��
 *------------------------------------------
 */
int clif_parse_CloseVending(int fd, struct map_session_data *sd)
{
	if(sd) vending_closevending(*sd);
	return 0;
}

/*==========================================
 * �I�X�A�C�e�����X�g�v��
 *------------------------------------------
 */
int clif_parse_VendingListReq(int fd, struct map_session_data *sd) {
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	vending_vendinglistreq(*sd,RFIFOL(fd,2));
	if(sd->npc_id)
		npc_event_dequeue(*sd);
	return 0;
}

/*==========================================
 * �I�X�A�C�e���w��
 *------------------------------------------
 */
int clif_parse_PurchaseReq(int fd, struct map_session_data *sd)
 {
	if( !session_isActive(fd) )
		return 0;

	if(sd) vending_purchasereq(*sd, RFIFOW(fd,2), RFIFOL(fd,4), RFIFOP(fd,8));
	return 0;
}

/*==========================================
 * �I�X�J��
 *------------------------------------------
 */
int clif_parse_OpenVending(int fd,struct map_session_data *sd) {
	if( !session_isActive(fd) )
		return 0;

	if(sd) vending_openvending(*sd, RFIFOW(fd,2), (char*)RFIFOP(fd,4), RFIFOB(fd,84), RFIFOP(fd,85));
	return 0;
}

/*==========================================
 * /monster /item rewriten by [Yor]
 *------------------------------------------
 */
int clif_parse_GM_Monster_Item(int fd, struct map_session_data *sd) {
	char monster_item_name[25];

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	memset(monster_item_name, '\0', sizeof(monster_item_name));

	if(battle_config.atc_gmonly == 0 || pc_isGM(*sd))
	{
		memcpy(monster_item_name, RFIFOP(fd,2), 24);
		if(mobdb_searchname(monster_item_name) != 0) {
			if(pc_isGM(*sd) >= get_atcommand_level(AtCommand_Monster))
				atcommand_spawn(fd, *sd, "@spawn", monster_item_name); // as @spawn
		} else if(itemdb_searchname(monster_item_name) != NULL) {
			if(pc_isGM(*sd) >= get_atcommand_level(AtCommand_Item))
				atcommand_item(fd, *sd, "@item", monster_item_name); // as @item
		}

	}
	return 0;
}

/*==========================================
 * �M���h�����
 *------------------------------------------
 */
int clif_parse_CreateGuild(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_create(*sd, (char*)RFIFOP(fd,6));
	return 0;
}

/*==========================================
 * �M���h�}�X�^�[���ǂ����m�F
 *------------------------------------------
 */
int clif_parse_GuildCheckMaster(int fd, struct map_session_data *sd) {
	clif_guild_masterormember(sd);
	return 0;
}

/*==========================================
 * �M���h���v��
 *------------------------------------------
 */
int clif_parse_GuildRequestInfo(int fd, struct map_session_data *sd) {
	if( !session_isActive(fd) )
		return 0;

	switch(RFIFOL(fd,2)){
	case 0:	// �M���h��{���A�����G�Ώ��
		clif_guild_basicinfo(sd);
		clif_guild_allianceinfo(sd);
		break;
	case 1:	// �����o�[���X�g�A��E�����X�g
		clif_guild_positionnamelist(sd);
		clif_guild_memberlist(sd);
		break;
	case 2:	// ��E�����X�g�A��E��񃊃X�g
		clif_guild_positionnamelist(sd);
		clif_guild_positioninfolist(sd);
		break;
	case 3:	// �X�L�����X�g
		clif_guild_skillinfo(sd);
		break;
	case 4:	// �Ǖ����X�g
		clif_guild_explusionlist(sd);
		break;
	default:
		if (battle_config.error_log)
			ShowMessage("clif: guild request info: unknown type %ld\n", (unsigned long)RFIFOL(fd,2));
		break;
	}
	return 0;
}

/*==========================================
 * �M���h��E�ύX
 *------------------------------------------
 */
int clif_parse_GuildChangePositionInfo(int fd, struct map_session_data *sd) {
	int i;
	if( !session_isActive(fd) )
		return 0;

	for(i = 4; i < RFIFOW(fd,2); i += 40 ){
		guild_change_position(*sd, RFIFOL(fd,i), RFIFOL(fd,i+4), RFIFOL(fd,i+12), (char*)RFIFOP(fd,i+16));
	}
	return 0;
}

/*==========================================
 * �M���h�����o��E�ύX
 *------------------------------------------
 */
int clif_parse_GuildChangeMemberPosition(int fd, struct map_session_data *sd) {
	int i;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	for(i=4;i<RFIFOW(fd,2);i+=12){
		guild_change_memberposition(sd->status.guild_id,
			RFIFOL(fd,i),RFIFOL(fd,i+4),RFIFOL(fd,i+8));
	}
	return 0;
}

/*==========================================
 * �M���h�G���u�����v��
 *------------------------------------------
 */
int clif_parse_GuildRequestEmblem(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	struct guild *g=guild_search(RFIFOL(fd,2));
	if(g!=NULL)
		clif_guild_emblem(sd,g);
	return 0;
}

/*==========================================
 * �M���h�G���u�����ύX
 *------------------------------------------
 */
int clif_parse_GuildChangeEmblem(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_change_emblem(*sd,RFIFOW(fd,2)-4,RFIFOP(fd,4));
	return 0;
}

/*==========================================
 * �M���h���m�ύX
 *------------------------------------------
 */
int clif_parse_GuildChangeNotice(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_change_notice(*sd,RFIFOL(fd,2),(char*)RFIFOP(fd,6),(char*)RFIFOP(fd,66));
	return 0;
}

/*==========================================
 * �M���h���U
 *------------------------------------------
 */
int clif_parse_GuildInvite(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_invite(*sd,RFIFOL(fd,2));
	return 0;
}

/*==========================================
 * �M���h���U�ԐM
 *------------------------------------------
 */
int clif_parse_GuildReplyInvite(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_reply_invite(*sd,RFIFOL(fd,2),RFIFOB(fd,6));
	return 0;
}

/*==========================================
 * �M���h�E��
 *------------------------------------------
 */
int clif_parse_GuildLeave(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_leave(*sd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),(char*)RFIFOP(fd,14));
	return 0;
}

/*==========================================
 * �M���h�Ǖ�
 *------------------------------------------
 */
int clif_parse_GuildExplusion(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_explusion(*sd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),(char*)RFIFOP(fd,14));
	return 0;
}

/*==========================================
 * �M���h��b
 *------------------------------------------
 */
int clif_parse_GuildMessage(int fd,struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;


	if (is_charcommand(fd, *sd, (char*)RFIFOP(fd, 4), 0) != CharCommand_None ||
		is_atcommand(fd, *sd, (char*)RFIFOP(fd, 4), 0) != AtCommand_None ||
		(sd->sc_data &&
		(sd->sc_data[SC_BERSERK].timer!=-1 ||	//�o�[�T�[�N���͉�b���s��
		sd->sc_data[SC_NOCHAT].timer!=-1)))		//�`���b�g�֎~
		return 0;

	guild_send_message(*sd, (char*)RFIFOP(fd,4), RFIFOW(fd,2)-4);
	return 0;
}

/*==========================================
 * �M���h�����v��
 *------------------------------------------
 */
int clif_parse_GuildRequestAlliance(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_reqalliance(*sd,RFIFOL(fd,2));
	return 0;
}

/*==========================================
 * �M���h�����v���ԐM
 *------------------------------------------
 */
int clif_parse_GuildReplyAlliance(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_reply_reqalliance(*sd,RFIFOL(fd,2),RFIFOL(fd,6));
	return 0;
}

/*==========================================
 * �M���h�֌W����
 *------------------------------------------
 */
int clif_parse_GuildDelAlliance(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_delalliance(sd,RFIFOL(fd,2),RFIFOL(fd,6));
	return 0;
}

/*==========================================
 * �M���h�G��
 *------------------------------------------
 */
int clif_parse_GuildOpposition(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_opposition(sd,RFIFOL(fd,2));
	return 0;
}

/*==========================================
 * �M���h���U
 *------------------------------------------
 */
int clif_parse_GuildBreak(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	guild_break(*sd,(char*)RFIFOP(fd,2));
	return 0;
}

// pet
int clif_parse_PetMenu(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(sd) pet_menu(*sd,RFIFOB(fd,2));
	return 0;
}

int clif_parse_CatchPet(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	pet_catch_process2(*sd,RFIFOL(fd,2));
	return 0;
}

int clif_parse_SelectEgg(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	pet_select_egg(*sd,RFIFOW(fd,2)-2);
	return 0;
}

int clif_parse_SendEmotion(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(sd->pd)
		clif_pet_emotion(sd->pd,RFIFOL(fd,2));
	return 0;
}

int clif_parse_ChangePetName(int fd, struct map_session_data *sd)
{
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	pet_change_name(*sd,(const char*)RFIFOP(fd,2));
	return 0;
}

// Kick (right click menu for GM "(name) force to quit")
int clif_parse_GMKick(int fd, struct map_session_data *sd) {
	struct block_list *target;

	if( !session_isActive(fd) )
		return 0;

	int tid = RFIFOL(fd,2);

	nullpo_retr(0,sd);

	if ((battle_config.atc_gmonly == 0 || pc_isGM(*sd)) &&
	    (pc_isGM(*sd) >= get_atcommand_level(AtCommand_Kick))) {
		target = map_id2bl(tid);
		if (target) {
			if (target->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data *)target;
				if (pc_isGM(*sd) > pc_isGM(*tsd))
					clif_GM_kick(*sd, *tsd, 1);
				else
					clif_GM_kickack(*sd, 0);
			} else if (target->type == BL_MOB) {
				struct mob_data *md = (struct mob_data *)target;
				sd->state.attack_type = 0;
				mob_damage(&sd->bl, *md, md->hp, 2);
			} else
				clif_GM_kickack(*sd, 0);
		} else
			clif_GM_kickack(*sd, 0);
	}
	return 0;
}

/*==========================================
 * /shift
 *------------------------------------------
 */
int clif_parse_Shift(int fd, struct map_session_data *sd) {	// Rewriten by [Yor]
	char player_name[25];

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	memset(player_name, '\0', sizeof(player_name));

	if ((battle_config.atc_gmonly == 0 || pc_isGM(*sd)) &&
	    (pc_isGM(*sd) >= get_atcommand_level(AtCommand_JumpTo))) {
		memcpy(player_name, RFIFOP(fd,2), 24);
		atcommand_jumpto(fd, *sd, "@jumpto", player_name); // as @jumpto
	}

	return 0;
}

/*==========================================
 * /recall
 *------------------------------------------
 */
int clif_parse_Recall(int fd, struct map_session_data *sd) {	// Added by RoVeRT
	char player_name[25];

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	memset(player_name, '\0', sizeof(player_name));

	if ((battle_config.atc_gmonly == 0 || pc_isGM(*sd)) &&
	    (pc_isGM(*sd) >= get_atcommand_level(AtCommand_Recall))) {
		memcpy(player_name, RFIFOP(fd,2), 24);
		atcommand_recall(fd, *sd, "@recall", player_name); // as @recall
	}

	return 0;
}

int clif_parse_GMHide(int fd, struct map_session_data *sd) {	// Modified by [Yor]
	nullpo_retr(0,sd);

	//ShowMessage("%2x %2x %2x\n", (unsigned short)RFIFOW(fd,0), (unsigned short)RFIFOW(fd,2), (unsigned short)RFIFOW(fd,4)); // R 019d <Option_value>.2B <flag>.2B
	if ((battle_config.atc_gmonly == 0 || pc_isGM(*sd)) &&
	    (pc_isGM(*sd) >= get_atcommand_level(AtCommand_Hide))) {
		if (sd->status.option & OPTION_HIDE) { // OPTION_HIDE = 0x40
			sd->status.option &= ~OPTION_HIDE; // OPTION_HIDE = 0x40
			clif_displaymessage(fd, "Invisible: Off.");
		} else {
			sd->status.option |= OPTION_HIDE; // OPTION_HIDE = 0x40
			clif_displaymessage(fd, "Invisible: On.");
		}
		clif_changeoption(&sd->bl);
	}
	return 0;
}

/*==========================================
 * GM�ɂ��`���b�g�֎~���ԕt�^
 *------------------------------------------
 */
int clif_parse_GMReqNoChat(int fd,struct map_session_data *sd)
{
	if( !session_isActive(fd) )
		return 0;

	unsigned long tid = RFIFOL(fd,2);
	int type = RFIFOB(fd,6);
	int limit = RFIFOW(fd,7);
	struct block_list *bl = map_id2bl(tid);
	struct map_session_data *dstsd;
	int dstfd;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if(!battle_config.muting_players) {
		clif_displaymessage(fd, "Muting is disabled.");
		return 0;
	}

	if (!bl || bl->type != BL_PC)
		return 0;
	dstsd =(struct map_session_data *)bl;

	if (type == 0)
		limit = 0 - limit;
	
	if(bl->type == BL_PC && (dstsd =(struct map_session_data *)bl))
	{
		if((tid == bl->id && type == 2 && !pc_isGM(*sd)) || (pc_isGM(*sd) > pc_isGM(*dstsd)) )
		{
			dstfd = dstsd->fd;

			if( session_isActive(dstfd) )
			{
				WFIFOW(dstfd,0)=0x14b;
				WFIFOB(dstfd,2)=(type==2)?1:type;
				memcpy(WFIFOP(dstfd,3),sd->status.name,24);
				WFIFOSET(dstfd,packet_len_table[0x14b]);
			}
		dstsd->status.manner -= limit;
		if(dstsd->status.manner < 0)
			status_change_start(bl,SC_NOCHAT,0,0,0,0,0,0);
			else
			{
			dstsd->status.manner = 0;
			status_change_end(bl,SC_NOCHAT,-1);
		}
			ShowMessage("name:%s type:%d limit:%d manner:%d\n",dstsd->status.name,type,limit,dstsd->status.manner);
	}
}
	return 0;
}
/*==========================================
 * GM�ɂ��`���b�g�֎~���ԎQ�Ɓi�H�j
 *------------------------------------------
 */
int clif_parse_GMReqNoChatCount(int fd, struct map_session_data *sd)
{
	if( !session_isActive(fd) )
		return 0;

	int tid = RFIFOL(fd,2);

	WFIFOW(fd,0) = 0x1e0;
	WFIFOL(fd,2) = tid;
	sprintf((char*)WFIFOP(fd,6),"%d",tid);
//	memcpy(WFIFOP(fd,6), "TESTNAME", 24);
	WFIFOSET(fd, packet_len_table[0x1e0]);

	return 0;
}

int clif_parse_PMIgnore(int fd, struct map_session_data *sd) {	// Rewritten by [Yor]
	char output[512];
	char *nick; // S 00cf <nick>.24B <type>.B: 00 (/ex nick) deny speech from nick, 01 (/in nick) allow speech from nick
	int i, pos;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	memset(output, '\0', sizeof(output));

	nick = (char*)RFIFOP(fd,2); // speed up
	RFIFOB(fd,25) = '\0'; // to be sure that the player name have at maximum 23 characters
	//ShowMessage("Ignore: char '%s' state: %d\n", nick, RFIFOB(fd,26));

	WFIFOW(fd,0) = 0x0d1; // R 00d1 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
	WFIFOB(fd,2) = RFIFOB(fd,26);
	// do nothing only if nick can not exist
	if (strlen(nick) < 4) {
		WFIFOB(fd,3) = 1; // fail
		WFIFOSET(fd, packet_len_table[0x0d1]);
		if (RFIFOB(fd,26) == 0) // type
			clif_wis_message(fd, wisp_server_name, "It's impossible to block this player.", strlen("It's impossible to block this player.") + 1);
		else
			clif_wis_message(fd, wisp_server_name, "It's impossible to unblock this player.", strlen("It's impossible to unblock this player.") + 1);
		return 0;
	// name can exist
	} else {
		// deny action (we add nick only if it's not already exist
		if (RFIFOB(fd,26) == 0) { // type
			pos = -1;
			for(i = 0; i < MAX_IGNORE_LIST; i++) {
				if (strcmp(sd->ignore[i].name, nick) == 0) {
					WFIFOB(fd,3) = 1; // fail
					WFIFOSET(fd, packet_len_table[0x0d1]);
					clif_wis_message(fd, wisp_server_name, "This player is already blocked.", strlen("This player is already blocked.") + 1);
					if (strcmp(wisp_server_name, nick) == 0) { // to found possible bot users who automaticaly ignore people.
						sprintf(output, "Character '%s' (account: %ld) has tried AGAIN to block wisps from '%s' (wisp name of the server). Bot user?", sd->status.name, sd->status.account_id, wisp_server_name);
						intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, output);
					}
					return 0;
				} else if (pos == -1 && sd->ignore[i].name[0] == '\0')
					pos = i;
			}
			// if a position is found and name not found, we add it in the list
			if (pos != -1) {
				memcpy(sd->ignore[pos].name, nick, 24);
				WFIFOB(fd,3) = 0; // success
				WFIFOSET(fd, packet_len_table[0x0d1]);
				if (strcmp(wisp_server_name, nick) == 0) { // to found possible bot users who automaticaly ignore people.
					sprintf(output, "Character '%s' (account: %ld) has tried to block wisps from '%s' (wisp name of the server). Bot user?", sd->status.name, sd->status.account_id, wisp_server_name);
					intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, output);
					// send something to be inform and force bot to ignore twice... ifGM receiving block + block again, it's a bot :)
					clif_wis_message(fd, wisp_server_name, "Add me in your ignore list, doesn't block my wisps.", strlen("Add me in your ignore list, doesn't block my wisps.") + 1);
				}
			} else {
				WFIFOB(fd,3) = 1; // fail
				WFIFOSET(fd, packet_len_table[0x0d1]);
				clif_wis_message(fd, wisp_server_name, "You can not block more people.", strlen("You can not block more people.") + 1);
				if (strcmp(wisp_server_name, nick) == 0) { // to found possible bot users who automaticaly ignore people.
					sprintf(output, "Character '%s' (account: %ld) has tried to block wisps from '%s' (wisp name of the server). Bot user?", sd->status.name, sd->status.account_id, wisp_server_name);
					intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, output);
				}
			}
		// allow action (we remove all same nicks if they exist)
		} else {
			pos = -1;
			for(i = 0; i < MAX_IGNORE_LIST; i++)
				if (strcmp(sd->ignore[i].name, nick) == 0) {
					memset(sd->ignore[i].name, 0, sizeof(sd->ignore[i].name));
					if (pos == -1) {
						WFIFOB(fd,3) = 0; // success
						WFIFOSET(fd, packet_len_table[0x0d1]);
						pos = i; // don't break, to remove ALL same nick
					}
				}
			if (pos == -1) {
				WFIFOB(fd,3) = 1; // fail
				WFIFOSET(fd, packet_len_table[0x0d1]);
				clif_wis_message(fd, wisp_server_name, "This player is not blocked by you.", strlen("This player is not blocked by you.") + 1);
			}
		}
	}

//	for(i = 0; i < MAX_IGNORE_LIST; i++) // for debug only
//		if (sd->ignore[i].name[0] != '\0')
//			ShowMessage("Ignored player: '%s'\n", sd->ignore[i].name);

	return 0;
}

int clif_parse_PMIgnoreAll(int fd, struct map_session_data *sd) { // Rewritten by [Yor]
	//ShowMessage("Ignore all: state: %d\n", RFIFOB(fd,2));
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	if (RFIFOB(fd,2) == 0) {// S 00d0 <type>len.B: 00 (/exall) deny all speech, 01 (/inall) allow all speech
		WFIFOW(fd,0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
		WFIFOB(fd,2) = 0;
		if(sd->state.ignoreAll == 0) {
			sd->state.ignoreAll = 1;
			WFIFOB(fd,3) = 0; // success
			WFIFOSET(fd, packet_len_table[0x0d2]);
		} else {
			WFIFOB(fd,3) = 1; // fail
			WFIFOSET(fd, packet_len_table[0x0d2]);
			clif_wis_message(fd, wisp_server_name, "You already block everyone.", strlen("You already block everyone.") + 1);
		}
	} else {
		WFIFOW(fd,0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
		WFIFOB(fd,2) = 1;
		if(sd->state.ignoreAll == 1) {
			sd->state.ignoreAll = 0;
			WFIFOB(fd,3) = 0; // success
			WFIFOSET(fd, packet_len_table[0x0d2]);
		} else {
			WFIFOB(fd,3) = 1; // fail
			WFIFOSET(fd, packet_len_table[0x0d2]);
			clif_wis_message(fd, wisp_server_name, "You already allow everyone.", strlen("You already allow everyone.") + 1);
		}
	}
	return 0;
}

/*==========================================
 * Wis���ۃ��X�g
 *------------------------------------------
 */
 int pstrcmp(const void *a, const void *b)
{
	return strcmp((char *)a, (char *)b);
}
int clif_parse_PMIgnoreList(int fd,struct map_session_data *sd)
{
	int i,j=0,count=0;
	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	qsort (sd->ignore[0].name, MAX_IGNORE_LIST, sizeof(sd->ignore[0].name), pstrcmp);

	for(i = 0; i < MAX_IGNORE_LIST; i++){	//���g������̂𐔂���
		if(sd->ignore[i].name[0] != 0)
			count++;
	}
	WFIFOW(fd,0) = 0xd4;
	WFIFOW(fd,2) = 4 + (24 * count);
	for(i = 0; i < MAX_IGNORE_LIST; i++){
		if(sd->ignore[i].name[0] != 0){
			memcpy(WFIFOP(fd, 4 + j * 24),sd->ignore[i].name, 24);
			j++;
		}
	}
	WFIFOSET(fd, WFIFOW(fd,2));
	if(count >= MAX_IGNORE_LIST)	//���^���Ȃ�Ō��1������
		sd->ignore[MAX_IGNORE_LIST - 1].name[0] = 0;

	return 0;
}

/*==========================================
 * �X�p�m�r��/doridori�ɂ��SPR2�{
 *------------------------------------------
 */
int clif_parse_NoviceDoriDori(int fd, struct map_session_data *sd) {
	if (sd)
		sd->doridori_counter = 1;

	return 0;
}
/*==========================================
 * �X�p�m�r�̔����g��
 *------------------------------------------
 */
int clif_parse_NoviceExplosionSpirits(int fd, struct map_session_data *sd)
{
	if(sd){
		int nextbaseexp=pc_nextbaseexp(sd);
		struct pc_base_job s_class = pc_calc_base_job(sd->status.class_);
		if (battle_config.etc_log){
			if(nextbaseexp != 0)
				ShowMessage("SuperNovice explosionspirits!! %d %d %d %d\n",sd->bl.id,s_class.job,sd->status.base_exp,1000*sd->status.base_exp/nextbaseexp);
			else
				ShowMessage("SuperNovice explosionspirits!! %d %d %d 000\n",sd->bl.id,s_class.job,sd->status.base_exp);
		}
		if(s_class.job == 23 && sd->status.base_exp > 0 && nextbaseexp > 0 && (1000*sd->status.base_exp/nextbaseexp)%100==0){
			clif_skill_nodamage(&sd->bl,&sd->bl,MO_EXPLOSIONSPIRITS,5,1);
			status_change_start(&sd->bl,SkillStatusChangeTable[MO_EXPLOSIONSPIRITS],5,0,0,0,skill_get_time(MO_EXPLOSIONSPIRITS,5),0 );
		}
	}
	return 0;
}

// random notes:
// 0x214: forging chance?

/*==========================================
 * Friends List
 *------------------------------------------
 */
int clif_friendslist_send(struct map_session_data *sd)
{
	size_t i, n;

	nullpo_retr(0,sd);
	if( !session_isActive(sd->fd) )
		return 0;

	// Send friends list
	WFIFOW(sd->fd, 0) = 0x201;
	for(i=0,n=4; i<MAX_FRIENDLIST; i++)
	{
		if(sd->status.friend_id[i])
		{
			WFIFOL(sd->fd, n+0) = (map_charid2sd(sd->status.friend_id[i]) != NULL);
			WFIFOL(sd->fd, n+4) = sd->status.friend_id[i];
			memcpy(WFIFOP(sd->fd, n+8), &sd->status.friend_name[i], 24);
			n += 32;
		}
	}
	WFIFOW(sd->fd,2) = n;
	WFIFOSET(sd->fd, WFIFOW(sd->fd,2));
	return 0;
}

// Status for adding friend - 0: successfull 1: not exist/rejected 2: over limit
int clif_friendslist_reqack(struct map_session_data *sd, char *name, unsigned short type)
{
	int fd;
	nullpo_ret(sd);

	fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x209;
	WFIFOW(fd,2) = type;
	if (type != 2)
		memcpy(WFIFOP(fd, 12), name, 24);
	WFIFOSET(fd, packet_len_table[0x209]);
	return 0;
}

int clif_parse_FriendsListAdd(int fd, struct map_session_data *sd)
{
	struct map_session_data *f_sd;
	size_t i, count;
	int f_fd;

	nullpo_retr(0,sd);
	if( !session_isActive(fd) )
		return 0;

	f_sd = map_nick2sd((char*)RFIFOP(fd,2));

	if(f_sd == NULL)
	{	// Friend doesn't exist (no player with this name)
		clif_displaymessage(fd, msg_txt(3));
	}
	else
	{
		for(i=0, count=0; i < MAX_FRIENDLIST; i++)
		{
		if (sd->status.friend_id[i] != 0)
			count++;
			if( sd->status.friend_id[i] == f_sd->status.char_id )
			{	// Friend already exists
				return clif_displaymessage(fd, "Friend already exists.");
		}
	}
	f_fd = f_sd->fd;
	WFIFOW(f_fd,0) = 0x207;
	WFIFOL(f_fd,2) = sd->status.char_id;
	WFIFOL(f_fd,6) = sd->bl.id;
	memcpy(WFIFOP(f_fd,10), sd->status.name, 24);
	WFIFOSET(f_fd, packet_len_table[0x207]);
}
	return 0;
}

int clif_parse_FriendsListReply(int fd, struct map_session_data *sd)
{	//<W: id> <L: Player 1 chara ID> <L: Player 1 AID> <B: Response>
	struct map_session_data *f_sd;
	unsigned long char_id, id;
	char reply;

	if( !session_isActive(fd) )
		return 0;

	char_id = RFIFOL(fd,2);
	id = RFIFOL(fd,6);
	reply = RFIFOB(fd,10);
//	ShowMessage ("reply: %d %d %d\n", char_id, id, reply);

	f_sd = map_id2sd(id);
	if (f_sd == NULL)
		return 0;

	if (reply == 0)
		clif_friendslist_reqack(f_sd, sd->status.name, 1);
	else
	{
		size_t i;
		// Find an empty slot
		for (i = 0; i < MAX_FRIENDLIST; i++)
			if (f_sd->status.friend_id[i] == 0)
				break;
		if(i == MAX_FRIENDLIST)
		{
			clif_friendslist_reqack(f_sd, sd->status.name, 2);
		}
		else
		{
		f_sd->status.friend_id[i] = sd->status.char_id;
		memset(f_sd->status.friend_name[i], 0, sizeof(f_sd->status.friend_name[i]));
		memcpy(f_sd->status.friend_name[i], f_sd->status.name, 23);
		clif_friendslist_reqack(f_sd, sd->status.name, 0);
		clif_friendslist_send(sd);
	}
}
	return 0;
}

int clif_parse_FriendsListRemove(int fd, struct map_session_data *sd)
{	// 0x203 </o> <ID to be removed W 4B>
	int i, j;

	nullpo_retr(0, sd);

	if( !session_isActive(fd) )
		return 0;

	unsigned long id = RFIFOL(fd,6);
	struct map_session_data *f_sd = map_charid2sd(id);

	// Search friend
	for (i = 0; i < MAX_FRIENDLIST; i ++)
	{
		if(sd->status.friend_id[i] == id)
		{
			// move all chars down
			for(j = i + 1; j < MAX_FRIENDLIST; j++)
			{
				sd->status.friend_id[j-1] = sd->status.friend_id[j];
				memcpy(sd->status.friend_name[j-1], sd->status.friend_name[j], sizeof(sd->status.friend_name[j]));
			}
			// clear the last one
			sd->status.friend_id[MAX_FRIENDLIST-1] = 0;
			memset(sd->status.friend_name[MAX_FRIENDLIST-1], 0, sizeof(sd->status.friend_name[MAX_FRIENDLIST-1]));
			clif_displaymessage(fd, "Friend removed");
			WFIFOW(fd,0) = 0x20a;
			WFIFOL(fd,2) = (f_sd) ? f_sd->bl.id : 0;	//account id;
			WFIFOL(fd,6) = id;
			WFIFOSET(fd, packet_len_table[0x20a]);
			clif_friendslist_send(sd);
			break;
		}
	}

	if (i == 20)
		return clif_displaymessage(fd, "Name not found in list.");

	return 0;
}

/*==========================================
 * /killall
 *------------------------------------------
 */
int clif_parse_GMKillAll(int fd,struct map_session_data *sd)
{
	char message[50];

	nullpo_retr(0, sd);

	memcpy(message,sd->status.name, 24);
	is_atcommand(fd, *sd, strcat(message," : @kickall"),0);

	return 0;
}

/*==========================================
 * /pvpinfo
 *------------------------------------------
 */
int clif_parse_PVPInfo(int fd,struct map_session_data *sd)
{
	nullpo_ret(sd);
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x210;
	//WFIFOL(fd,2) = 0;	// not sure what for yet
	//WFIFOL(fd,6) = 0;
	WFIFOL(fd,10) = sd->pvp_won;	// times won
	WFIFOL(fd,14) = sd->pvp_lost;	// times lost
	WFIFOL(fd,18) = sd->pvp_point;
	WFIFOSET(fd, packet_len_table[0x210]);

	return 0;
}

/*==========================================
 * /blacksmith
 *------------------------------------------
 */
int clif_parse_Blacksmith(int fd,struct map_session_data *sd)
{
	int i;
	char *name;
		
	nullpo_ret(sd);
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x219;
	for (i = 0; i < MAX_FAMELIST; i++)
	{
		// To-do: save blacksmith names in fame list...
		if(smith_fame_list[i].id > 0)
		{
			if (strcmp(smith_fame_list[i].name, "-") == 0 &&
				(name = map_charid2nick(smith_fame_list[i].id)) != NULL)
			{
				memcpy(WFIFOP(fd, 2 + 24 * i), name, 24);
			}
			else
				memcpy(WFIFOP(fd, 2 + 24 * i), smith_fame_list[i].name, 24);
		}
		else
			memcpy(WFIFOP(fd, 2 + 24 * i), "None", 24);
		WFIFOL(fd, 242 + i * 4) = smith_fame_list[i].fame;
	}
	WFIFOSET(fd, packet_len_table[0x219]);
	return 0;
}

int clif_fame_blacksmith(struct map_session_data *sd, unsigned long points)
{
	nullpo_ret(sd);
	int fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x21b;
	WFIFOL(fd,2) = points;
	WFIFOL(fd,6) = sd->status.fame_points;
	WFIFOSET(fd, packet_len_table[0x21b]);

	return 0;
}

/*==========================================
 * /alchemist
 *------------------------------------------
 */
int clif_parse_Alchemist(int fd,struct map_session_data *sd)
{
	int i;
	char *name;

	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x21a;
	for (i = 0; i < MAX_FAMELIST; i++)
	{	// To-do: save alchemist names in fame list...
		if(chemist_fame_list[i].id > 0)
		{
			if (strcmp(chemist_fame_list[i].name, "-") == 0 &&
				(name = map_charid2nick(chemist_fame_list[i].id)) != NULL)
			{
				memcpy(WFIFOP(fd, 2 + 24 * i), name, 24);
			}
			else
				memcpy(WFIFOP(fd, 2 + 24 * i), chemist_fame_list[i].name, 24);
		}
		else
			memcpy(WFIFOP(fd, 2 + 24 * i), "None", 24);
		WFIFOL(fd, 242 + i * 4) = chemist_fame_list[i].fame;
	}
	WFIFOSET(fd, packet_len_table[0x21a]);
	return 0;
}

int clif_fame_alchemist(struct map_session_data *sd, unsigned long points)
{
	nullpo_ret(sd);
	int fd = sd->fd;
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x21c;
	WFIFOL(fd,2) = points;
	WFIFOL(fd,6) = sd->status.fame_points;
	WFIFOSET(fd, packet_len_table[0x21c]);
	
	return 0;
}

/*==========================================
 * /taekwon?
 *------------------------------------------
 */
void clif_parse_Taekwon(int fd,struct map_session_data *sd)
{
	int i;

	nullpo_retv(sd);

	WFIFOW(fd,0) = 0x226;
	for (i = 0; i < 10; i++) {
		memcpy(WFIFOP(fd, 2 + 24 * i), "Unknown", 24);
		WFIFOL(fd, 242 + i * 4) = 0;
	}
	WFIFOSET(fd, packet_db[clif_config.packet_db_ver][0x226].len);
}

/*==========================================
 * �p�P�b�g�f�o�b�O
 *------------------------------------------
 */
int clif_parse_debug(int fd,struct map_session_data *sd)
{
	int i, cmd;

	if( !session_isActive(fd) )
		return 0;

	cmd = RFIFOW(fd,0);

	ShowMessage("packet debug 0x%4X\n",cmd);
//	ShowMessage("---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
	for(i=0;i<packet_db[sd->packet_ver][cmd].len;i++){
		if((i&15)==0)
			ShowMessage("\n%04X ",i);
		ShowMessage("%02X ",RFIFOB(fd,i));
	}
	ShowMessage("\n");
	return 0;
}

// functions list
static int (*clif_parse_func_table[MAX_PACKET_DB])(int, struct map_session_data *) = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

	// 40
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

	// 70
	NULL, NULL, clif_parse_WantToConnection, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, clif_parse_LoadEndAck, clif_parse_TickSend, NULL,

	// 80
	NULL, NULL, NULL, NULL, NULL, clif_parse_WalkToXY, NULL, NULL,
	NULL, clif_parse_ActionRequest, NULL, NULL, clif_parse_GlobalMessage, NULL, NULL, NULL,
	// 90
	clif_parse_NpcClicked, NULL, NULL, NULL, clif_parse_GetCharNameRequest, NULL, clif_parse_Wis, NULL,
	NULL, clif_parse_GMmessage, NULL, clif_parse_ChangeDir, NULL, NULL, NULL, clif_parse_TakeItem,
	// a0
	NULL, NULL, clif_parse_DropItem, NULL, NULL, NULL, NULL, clif_parse_UseItem,
	NULL, clif_parse_EquipItem, NULL, clif_parse_UnequipItem, NULL, NULL, NULL, NULL,
	// b0
	NULL, NULL, clif_parse_Restart, NULL, NULL, NULL, NULL, NULL,
	clif_parse_NpcSelectMenu, clif_parse_NpcNextClicked, NULL, clif_parse_StatusUp, NULL, NULL, NULL, clif_parse_Emotion,

	// c0
	NULL, clif_parse_HowManyConnections, NULL, NULL, NULL, clif_parse_NpcBuySellSelected, NULL, NULL,
	clif_parse_NpcBuyListSend, clif_parse_NpcSellListSend, NULL, NULL, clif_parse_GMKick, NULL, clif_parse_GMKillAll, clif_parse_PMIgnore,
	// d0
	clif_parse_PMIgnoreAll, NULL, NULL, clif_parse_PMIgnoreList, NULL, clif_parse_CreateChatRoom, NULL, NULL,
	NULL, clif_parse_ChatAddMember, NULL, NULL, NULL, NULL, clif_parse_ChatRoomStatusChange, NULL,
	// e0
	clif_parse_ChangeChatOwner, NULL, clif_parse_KickFromChat, clif_parse_ChatLeave, clif_parse_TradeRequest, NULL, clif_parse_TradeAck, NULL,
	clif_parse_TradeAddItem, NULL, NULL, clif_parse_TradeOk, NULL, clif_parse_TradeCancel, NULL, clif_parse_TradeCommit,
	// f0
	NULL, NULL, NULL, clif_parse_MoveToKafra, NULL, clif_parse_MoveFromKafra, NULL, clif_parse_CloseKafra,
	NULL, clif_parse_CreateParty, NULL, NULL, clif_parse_PartyInvite, NULL, NULL, clif_parse_ReplyPartyInvite,

	// 100
	clif_parse_LeaveParty, NULL, clif_parse_PartyChangeOption, clif_parse_RemovePartyMember, NULL, NULL, NULL, NULL,
	clif_parse_PartyMessage, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 110
	NULL, NULL, clif_parse_SkillUp, clif_parse_UseSkillToId, NULL, NULL, clif_parse_UseSkillToPos, NULL,
	clif_parse_StopAttack, NULL, NULL, clif_parse_UseSkillMap, NULL, clif_parse_RequestMemo, NULL, NULL,
	// 120
	NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_PutItemToCart, clif_parse_GetItemFromCart,
	clif_parse_MoveFromKafraToCart, clif_parse_MoveToKafraFromCart, clif_parse_RemoveOption, NULL, NULL, NULL, clif_parse_CloseVending, NULL,
	// 130
	clif_parse_VendingListReq, NULL, NULL, NULL, clif_parse_PurchaseReq, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_GM_Monster_Item,

	// 140
	clif_parse_MapMove, NULL, NULL, clif_parse_NpcAmountInput, NULL, NULL, clif_parse_NpcCloseClicked, NULL,
	NULL, clif_parse_GMReqNoChat, NULL, NULL, NULL, clif_parse_GuildCheckMaster, NULL, clif_parse_GuildRequestInfo,
	// 150
	NULL, clif_parse_GuildRequestEmblem, NULL, clif_parse_GuildChangeEmblem, NULL, clif_parse_GuildChangeMemberPosition, NULL, NULL,
	NULL, clif_parse_GuildLeave, NULL, clif_parse_GuildExplusion, NULL, clif_parse_GuildBreak, NULL, NULL,
	// 160
	NULL, clif_parse_GuildChangePositionInfo, NULL, NULL, NULL, clif_parse_CreateGuild, NULL, NULL,
	clif_parse_GuildInvite, NULL, NULL, clif_parse_GuildReplyInvite, NULL, NULL, clif_parse_GuildChangeNotice, NULL,
	// 170
	clif_parse_GuildRequestAlliance, NULL, clif_parse_GuildReplyAlliance, NULL, NULL, NULL, NULL, NULL,
	clif_parse_ItemIdentify, NULL, clif_parse_UseCard, NULL, clif_parse_InsertCard, NULL, clif_parse_GuildMessage, NULL,

	// 180
	clif_parse_GuildOpposition, NULL, NULL, clif_parse_GuildDelAlliance, NULL, NULL, NULL, NULL,
	NULL, NULL, clif_parse_QuitGame, NULL, NULL, NULL, clif_parse_ProduceMix, NULL,
	// 190
	clif_parse_UseSkillToPos, NULL, NULL, clif_parse_SolveCharName, NULL, NULL, NULL, clif_parse_ResetChar,
	NULL, NULL, NULL, NULL, clif_parse_LGMmessage, clif_parse_GMHide, NULL, clif_parse_CatchPet,
	// 1a0
	NULL, clif_parse_PetMenu, NULL, NULL, NULL, clif_parse_ChangePetName, NULL, clif_parse_SelectEgg,
	NULL, clif_parse_SendEmotion, NULL, NULL, NULL, NULL, clif_parse_SelectArrow, clif_parse_ChangeCart,
	// 1b0
	NULL, NULL, clif_parse_OpenVending, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, clif_parse_Shift, clif_parse_Shift, clif_parse_Recall, clif_parse_Recall, NULL, NULL,

	// 1c0
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_AutoSpell,
	NULL,
	// 1d0
	NULL, NULL, NULL, NULL, NULL, clif_parse_NpcStringInput, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_GMReqNoChatCount,
	// 1e0
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_NoviceDoriDori,
	clif_parse_CreateParty2, NULL, NULL, NULL, NULL, clif_parse_NoviceExplosionSpirits, NULL, NULL,
	// 1f0
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, clif_parse_ReqAdopt,
	NULL, NULL, NULL, NULL, NULL, NULL,

	// 200
	NULL, NULL, clif_parse_FriendsListAdd, clif_parse_FriendsListRemove, NULL, NULL, NULL, NULL,
	clif_parse_FriendsListReply, NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_PVPInfo,
	// 210
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_Blacksmith,  clif_parse_Alchemist,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
#if 0
	case 0xd3: clif_parse_IgnoreList
#endif
};

/*==========================================
 * terminate function on logout ans session delete
 * 
 *------------------------------------------
 */
int clif_terminate(int fd)
{
	if( session_isRemoved(fd) )
	{	
		struct map_session_data *sd = (struct map_session_data*)session[fd]->session_data;
		if( sd!=NULL )
		{
			if( sd->state.event_disconnect )
				npc_event_doall_attached("OnDisconnect",*sd);   

			if(sd->state.auth) 
			{	// the function doesn't send to inter-server/char-server ifit is not connected [Yor]
				map_quit(*sd);
				if(sd->status.name != NULL)
					ShowInfo("%sCharacter '"CL_WHITE"%s"CL_RESET"' logged off.\n", (pc_isGM(*sd))?"GM ":"",sd->status.name); // Player logout display [Valaris]
				else
					ShowInfo("%sCharacter with Account ID '"CL_WHITE"%d"CL_RESET"' logged off.\n", (pc_isGM(*sd))?"GM ":"", sd->bl.id); // Player logout display [Yor]
			}
			else 
			{	// not authentified! (refused by char-server or disconnect before to be authentified)
				ShowInfo("Player not authenticated with Account ID '"CL_WHITE"%d"CL_RESET"' logged off.\n", sd->bl.id); // Player logout display [Yor]
				map_deliddb(sd->bl); // account_id has been included in the DB before auth answer [Yor]
			} 
			chrif_chardisconnect(*sd);
		} 
		else
		{
			unsigned long ip = session[fd]->client_ip;
			ShowWarning("Player not identified with IP '"CL_WHITE"%d.%d.%d.%d"CL_RESET"' logged off.\n", (ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,(ip)&0xFF);
		}
	}
	return 0;
}

/*==========================================
 * �N���C�A���g����̃p�P�b�g���
 * socket.c��do_parsepacket����Ăяo�����
 *------------------------------------------
 */
static int clif_parse(int fd) {
	int packet_len = 0, cmd, packet_ver;
	struct map_session_data *sd;

	if( !session_isValid(fd) )
		return 0;

	sd = (struct map_session_data*)session[fd]->session_data;

	// �ڑ����؂�Ă�̂Ō�n��
	// char�I�Ɍq�����ĂȂ��Ԃ͐ڑ��֎~ (!chrif_isconnect())
	if( //!chrif_isconnect() || // I try not to delete users on a char server drop
		!session_isActive(fd) )
	{ 
		// wait some time (10sec) before removing the pc after a forced logout
		// other disconnections get a different WaitClose time

		if( session_isMarked(fd) || sd->state.waitingdisconnect )
			// removing marked sessons might be no thread 
			// even if the timer is not removed and called on an empty or reused session
			session_Remove(fd); 
		else
			session_SetWaitClose(fd, 10000);
		return 0;
	}



	while(RFIFOREST(fd) >= 2)
	{
	cmd = RFIFOW(fd,0);
		//ShowMessage("clif_parse: connection #%d, packet: 0x%x (with being read: %d bytes).\n", fd, cmd, RFIFOREST(fd));
	// �Ǘ��p�p�P�b�g����
	if (cmd >= 30000) {
		switch(cmd) {
		case 0x7530: // Athena��񏊓�
			WFIFOW(fd,0) = 0x7531;
			WFIFOB(fd,2) = ATHENA_MAJOR_VERSION;
			WFIFOB(fd,3) = ATHENA_MINOR_VERSION;
			WFIFOB(fd,4) = ATHENA_REVISION;
			WFIFOB(fd,5) = ATHENA_RELEASE_FLAG;
			WFIFOB(fd,6) = ATHENA_OFFICIAL_FLAG;
			WFIFOB(fd,7) = ATHENA_SERVER_MAP;
			WFIFOW(fd,8) = ATHENA_MOD_VERSION;
			WFIFOSET(fd,10);
			RFIFOSKIP(fd,2);
			break;

		case 0x7532: // �ڑ��̐ؒf
					session_Remove(fd);
			break;
		}
		return 0;
	}

	// get packet version before to parse
	packet_ver = 0;
		if(sd)
		packet_ver = sd->packet_ver;
	// check authentification packet to know packet version
		else {
		// packet DB
		if (IS_PACKET_DB_VER (cmd) && 
			RFIFOREST(fd) >= packet_db[clif_config.packet_db_ver][cmd].len &&
			(RFIFOB(fd,packet_db[clif_config.packet_db_ver][cmd].pos[4]) == 0 ||	// 01 = Male
			RFIFOB(fd,packet_db[clif_config.packet_db_ver][cmd].pos[4]) == 1))		// 00 = Female
		{
			packet_ver = clif_config.packet_db_ver;
		// 0x72
		} else if (cmd == 0x72) {
			if (RFIFOREST(fd) >= 39 && (RFIFOB(fd,38) == 0 || RFIFOB(fd,38) == 1)) // 00 = Female, 01 = Male
				packet_ver = 7; // 7: 13july04
			else if (RFIFOREST(fd) >= 22 && (RFIFOB(fd,21) == 0 || RFIFOB(fd,21) == 1)) // 00 = Female, 01 = Male
				packet_ver = 6; // 6: 7july04
			else if (RFIFOREST(fd) >= 19 && (RFIFOB(fd,18) == 0 || RFIFOB(fd,18) == 1)) // 00 = Female, 01 = Male
				packet_ver = 5; // 5: old
			// else probably incomplete packet
			else if (RFIFOREST(fd) < 19)
				return 0;
		// 0x7E
		} else if (cmd == 0x7E) {
			if (RFIFOREST(fd) >= 37 && (RFIFOB(fd,36) == 0 || RFIFOB(fd,36) == 1)) // 00 = Female, 01 = Male
				packet_ver = 9; // 9: 9aug04/16aug04/17aug04
			else if (RFIFOREST(fd) >= 33 && (RFIFOB(fd,32) == 0 || RFIFOB(fd,32) == 1)) // 00 = Female, 01 = Male
				packet_ver = 8; // 8: 26july04
			// else probably incomplete packet
			else if (RFIFOREST(fd) < 33)
				return 0;
		// 0xF5
		} else if (cmd == 0xF5) {
			if (RFIFOREST(fd) >= 34 && (RFIFOB(fd,33) == 0 || RFIFOB(fd,33) == 1)) // 00 = Female, 01 = Male
				packet_ver = 10; // 10: 6sept04
			else if (RFIFOREST(fd) >= 33 && (RFIFOB(fd,32) == 0 || RFIFOB(fd,32) == 1)) // 00 = Female, 01 = Male
				packet_ver = 12; // 12: 18oct04
			else if (RFIFOREST(fd) >= 32 && (RFIFOB(fd,31) == 0 || RFIFOB(fd,31) == 1)) // 00 = Female, 01 = Male
				packet_ver = 11; // 11: 21sept04
			else if (RFIFOREST(fd) >= 29 && (RFIFOB(fd,28) == 0 || RFIFOB(fd,28) == 1)) {	// 00 = Female, 01 = Male
				if (RFIFOL(fd,3) > 700000 && RFIFOL(fd,10) >= 150000 && RFIFOL(fd,10) < 5000000) // account id / char id (more than 5.000.000 characters?) [Yor]
					packet_ver = 15;	// 14: 6dec04
				else
					packet_ver = 13; // 13: 25oct04 (by [Yor])
			}
			// else probably incomplete packet
			else if (RFIFOREST(fd) < 29)
				return 0;
		// 0x9B
		} else if (cmd == 0x9B) {
			if (RFIFOREST(fd) >= 32 && (RFIFOB(fd,31) == 0 || RFIFOB(fd,31) == 1))	// 00 = Female, 01 = Male
				packet_ver = 16; // 16: 10jan05
			else if (RFIFOREST(fd) >= 26 && (RFIFOB(fd,25) == 0 || RFIFOB(fd,25) == 1))	// 00 = Female, 01 = Male
				packet_ver = 17; // 17: 10may05
			else if (RFIFOREST(fd) < 32)
				return 0;
		} else {
			// unknown client? leave packet ver as 0 so it'll disconnect anyway
		}

		// check if version is accepted
		if (packet_ver < 5 ||	// reject really old client versions
			(packet_ver <= 9 && (battle_config.packet_ver_flag &	1) == 0) ||	// older than 6sept04
			(packet_ver == 10 && (battle_config.packet_ver_flag &	2) == 0) ||
			(packet_ver == 11 && (battle_config.packet_ver_flag &	4) == 0) ||
			(packet_ver == 12 && (battle_config.packet_ver_flag &	8) == 0) ||
			(packet_ver == 13 && (battle_config.packet_ver_flag &	16) == 0) ||
			(packet_ver == 14 && (battle_config.packet_ver_flag &	32) == 0) ||
			(packet_ver == 15 && (battle_config.packet_ver_flag &	64) == 0) ||
			(packet_ver == 16 && (battle_config.packet_ver_flag &	128) == 0) ||
			(packet_ver == 17 && (battle_config.packet_ver_flag &	256) == 0) ||
			packet_ver > MAX_PACKET_VER ||	// no packet version support yet
			// identified version, but unknown client?
			(!sd && packet_db[packet_ver][cmd].func != clif_parse_WantToConnection)) {
			WFIFOW(fd,0) = 0x6a;
			WFIFOB(fd,2) = 5; // 05 = Game's EXE is not the latest version
			WFIFOSET(fd,23);
					session_SetWaitClose(fd, 5000);
			return 0;
		}
	}

	// �Q�[���p�ȊO�p�P�b�g���A�F�؂��I����O��0072�ȊO��������A�ؒf����
			if(cmd >= MAX_PACKET_DB || packet_db[packet_ver][cmd].len == 0) 
			{	// ifpacket is not inside these values: session is incorrect?? or auth packet is unknown
				ShowMessage("clif_parse: session #%d, packet 0x%x (%d bytes received) -> disconnected.\n", fd, cmd, RFIFOREST(fd));
				session_Remove(fd);
			return 0;
	}

	// �p�P�b�g�����v�Z
	packet_len = packet_db[packet_ver][cmd].len;
	if (packet_len == -1) {
		if (RFIFOREST(fd) < 4)
			return 0; // �ϒ��p�P�b�g�Œ����̏��܂Ńf�[�^�����ĂȂ�
		packet_len = RFIFOW(fd,2);
		if (packet_len < 4 || packet_len > 32768) {
					session_Remove(fd);
			return 0;
		}
	}
	if (RFIFOREST(fd) < packet_len)
		return 0; // �܂�1�p�P�b�g���f�[�^�������ĂȂ�


	if (sd && sd->state.auth == 1 && sd->state.waitingdisconnect == 1) { // �ؒf�҂��̏ꍇ�p�P�b�g���������Ȃ�

	} else if (packet_db[packet_ver][cmd].func) { // packet version 5-6-7 use same functions, but size are different
		// �p�P�b�g����
		packet_db[packet_ver][cmd].func(fd, sd);
	} else {
		// �s���ȃp�P�b�g
#if DUMP_UNKNOWN_PACKET == 1
				int i;
				FILE *fp;
				char packet_txt[256] = "save/packet.txt";
				time_t now;

			if((fp = savefopen(packet_txt, "a")) == NULL) {
				ShowMessage("clif.c: cant write [%s] !!! data is lost !!!\n", packet_txt);
					return 1;
				} else {
					time(&now);
					if (sd && sd->state.auth) {
						if (sd->status.name != NULL)
							fprintf(fp, "%sPlayer with account ID %ld (character ID %ld, player name %s) sent wrong packet:\n",
							        asctime(localtime(&now)), sd->status.account_id, sd->status.char_id, sd->status.name);
						else
							fprintf(fp, "%sPlayer with account ID %ld sent wrong packet:\n", asctime(localtime(&now)), sd->bl.id);
					} else if (sd) // not authentified! (refused by char-server or disconnect before to be authentified)
						fprintf(fp, "%sPlayer with account ID %ld sent wrong packet:\n", asctime(localtime(&now)), sd->bl.id);

					fprintf(fp, "\t---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
					for(i = 0; i < packet_len; i++) {
						if ((i & 15) == 0)
							fprintf(fp, "\n\t%04X ", i);
						fprintf(fp, "%02X ", RFIFOB(fd,i));
					}
					fprintf(fp, "\n\n");
					fclose(fp);
				}
#endif
		}
#if DUMP_ALL_PACKETS == 1
		{
		int i;
		if (fd)
				ShowMessage("\nclif_parse: session #%d, packet 0x%x, lenght %d\n", fd, cmd, packet_len);
		for(i = 0; i < packet_len; i++) {
			if ((i & 15) == 0)
						ShowMessage("\n%04X ",i);
					ShowMessage("%02X ", RFIFOB(fd,i));
		}
		if (sd && sd->state.auth) {
			if (sd->status.name != NULL)
						ShowMessage("\nAccount ID %d, character ID %d, player name %s.\n",
			       sd->status.account_id, sd->status.char_id, sd->status.name);
			else
						ShowMessage("\nAccount ID %d.\n", sd->bl.id);
		} else if (sd) // not authentified! (refused by char-server or disconnect before to be authentified)
					ShowMessage("\nAccount ID %d.\n", sd->bl.id);
	}
#endif
	RFIFOSKIP(fd, packet_len);
	}// end while

	return 0;
}

/*==========================================
 * �p�P�b�g�f�[�^�x�[�X�ǂݍ���
 *------------------------------------------
 */
static int packetdb_readdb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int cmd;
	size_t j,k,packet_ver;
	char *str[64],*p,*str2[64],*p2,w1[64],w2[64];

	struct {
		int (*func)(int, struct map_session_data *);
		char *name;
	} clif_parse_func[]={
		{clif_parse_WantToConnection,"wanttoconnection"},
		{clif_parse_LoadEndAck,"loadendack"},
		{clif_parse_TickSend,"ticksend"},
		{clif_parse_WalkToXY,"walktoxy"},
		{clif_parse_QuitGame,"quitgame"},
		{clif_parse_GetCharNameRequest,"getcharnamerequest"},
		{clif_parse_GlobalMessage,"globalmessage"},
		{clif_parse_MapMove,"mapmove"},
		{clif_parse_ChangeDir,"changedir"},
		{clif_parse_Emotion,"emotion"},
		{clif_parse_HowManyConnections,"howmanyconnections"},
		{clif_parse_ActionRequest,"actionrequest"},
		{clif_parse_Restart,"restart"},
		{clif_parse_Wis,"wis"},
		{clif_parse_GMmessage,"gmmessage"},
		{clif_parse_TakeItem,"takeitem"},
		{clif_parse_DropItem,"dropitem"},
		{clif_parse_UseItem,"useitem"},
		{clif_parse_EquipItem,"equipitem"},
		{clif_parse_UnequipItem,"unequipitem"},
		{clif_parse_NpcClicked,"npcclicked"},
		{clif_parse_NpcBuySellSelected,"npcbuysellselected"},
		{clif_parse_NpcBuyListSend,"npcbuylistsend"},
		{clif_parse_NpcSellListSend,"npcselllistsend"},
		{clif_parse_CreateChatRoom,"createchatroom"},
		{clif_parse_ChatAddMember,"chataddmember"},
		{clif_parse_ChatRoomStatusChange,"chatroomstatuschange"},
		{clif_parse_ChangeChatOwner,"changechatowner"},
		{clif_parse_KickFromChat,"kickfromchat"},
		{clif_parse_ChatLeave,"chatleave"},
		{clif_parse_TradeRequest,"traderequest"},
		{clif_parse_TradeAck,"tradeack"},
		{clif_parse_TradeAddItem,"tradeadditem"},
		{clif_parse_TradeOk,"tradeok"},
		{clif_parse_TradeCancel,"tradecancel"},
		{clif_parse_TradeCommit,"tradecommit"},
		{clif_parse_StopAttack,"stopattack"},
		{clif_parse_PutItemToCart,"putitemtocart"},
		{clif_parse_GetItemFromCart,"getitemfromcart"},
		{clif_parse_RemoveOption,"removeoption"},
		{clif_parse_ChangeCart,"changecart"},
		{clif_parse_StatusUp,"statusup"},
		{clif_parse_SkillUp,"skillup"},
		{clif_parse_UseSkillToId,"useskilltoid"},
		{clif_parse_UseSkillToPos,"useskilltopos"},
		{clif_parse_UseSkillMap,"useskillmap"},
		{clif_parse_RequestMemo,"requestmemo"},
		{clif_parse_ProduceMix,"producemix"},
		{clif_parse_NpcSelectMenu,"npcselectmenu"},
		{clif_parse_NpcNextClicked,"npcnextclicked"},
		{clif_parse_NpcAmountInput,"npcamountinput"},
		{clif_parse_NpcStringInput,"npcstringinput"},
		{clif_parse_NpcCloseClicked,"npccloseclicked"},
		{clif_parse_ItemIdentify,"itemidentify"},
		{clif_parse_SelectArrow,"selectarrow"},
		{clif_parse_AutoSpell,"autospell"},
		{clif_parse_UseCard,"usecard"},
		{clif_parse_InsertCard,"insertcard"},
		{clif_parse_SolveCharName,"solvecharname"},
		{clif_parse_ResetChar,"resetchar"},
		{clif_parse_LGMmessage,"lgmmessage"},
		{clif_parse_MoveToKafra,"movetokafra"},
		{clif_parse_MoveFromKafra,"movefromkafra"},
		{clif_parse_MoveToKafraFromCart,"movetokafrafromcart"},
		{clif_parse_MoveFromKafraToCart,"movefromkafratocart"},
		{clif_parse_CloseKafra,"closekafra"},
		{clif_parse_CreateParty,"createparty"},
		{clif_parse_CreateParty2,"createparty2"},
		{clif_parse_PartyInvite,"partyinvite"},
		{clif_parse_ReplyPartyInvite,"replypartyinvite"},
		{clif_parse_LeaveParty,"leaveparty"},
		{clif_parse_RemovePartyMember,"removepartymember"},
		{clif_parse_PartyChangeOption,"partychangeoption"},
		{clif_parse_PartyMessage,"partymessage"},
		{clif_parse_CloseVending,"closevending"},
		{clif_parse_VendingListReq,"vendinglistreq"},
		{clif_parse_PurchaseReq,"purchasereq"},
		{clif_parse_OpenVending,"openvending"},
		{clif_parse_CreateGuild,"createguild"},
		{clif_parse_GuildCheckMaster,"guildcheckmaster"},
		{clif_parse_GuildRequestInfo,"guildrequestinfo"},
		{clif_parse_GuildChangePositionInfo,"guildchangepositioninfo"},
		{clif_parse_GuildChangeMemberPosition,"guildchangememberposition"},
		{clif_parse_GuildRequestEmblem,"guildrequestemblem"},
		{clif_parse_GuildChangeEmblem,"guildchangeemblem"},
		{clif_parse_GuildChangeNotice,"guildchangenotice"},
		{clif_parse_GuildInvite,"guildinvite"},
		{clif_parse_GuildReplyInvite,"guildreplyinvite"},
		{clif_parse_GuildLeave,"guildleave"},
		{clif_parse_GuildExplusion,"guildexplusion"},
		{clif_parse_GuildMessage,"guildmessage"},
		{clif_parse_GuildRequestAlliance,"guildrequestalliance"},
		{clif_parse_GuildReplyAlliance,"guildreplyalliance"},
		{clif_parse_GuildDelAlliance,"guilddelalliance"},
		{clif_parse_GuildOpposition,"guildopposition"},
		{clif_parse_GuildBreak,"guildbreak"},
		{clif_parse_PetMenu,"petmenu"},
		{clif_parse_CatchPet,"catchpet"},
		{clif_parse_SelectEgg,"selectegg"},
		{clif_parse_SendEmotion,"sendemotion"},
		{clif_parse_ChangePetName,"changepetname"},
		{clif_parse_GMKick,"gmkick"},
		{clif_parse_GMHide,"gmhide"},
		{clif_parse_GMReqNoChat,"gmreqnochat"},
		{clif_parse_GMReqNoChatCount,"gmreqnochatcount"},
		{clif_parse_NoviceDoriDori,"sndoridori"},
		{clif_parse_NoviceExplosionSpirits,"snexplosionspirits"},
		{clif_parse_PMIgnore,"wisexin"},
		{clif_parse_PMIgnoreList,"wisexlist"},
		{clif_parse_PMIgnoreAll,"wisall"},
		{clif_parse_FriendsListAdd,"friendslistadd"},
		{clif_parse_FriendsListRemove,"friendslistremove"},
		{clif_parse_FriendsListReply,"friendslistreply"},
		{clif_parse_GMKillAll,"killall"},
		{clif_parse_Recall,"summon"},
		{clif_parse_GM_Monster_Item,"itemmonster"},
		{clif_parse_Shift,"shift"},
		{clif_parse_Blacksmith,"blacksmith"},
		{clif_parse_Alchemist,"alchemist"},
		{clif_parse_debug,"debug"},

		{NULL,NULL}
	};

	if( (fp=savefopen("db/packet_db.txt","r"))==NULL ){
		ShowMessage("can't read db/packet_db.txt\n");		
		return 1;
	}

	clif_config.packet_db_ver = MAX_PACKET_VER;
	packet_ver = MAX_PACKET_VER;	// read into packet_db's version by default

	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		if (sscanf(line,"%[^:]: %[^\r\n]",w1,w2) == 2) {
			if(strcasecmp(w1,"packet_ver")==0) {
				packet_ver = atoi(w2);
				// copy from previous version into new version and continue
				// - indicating all following packets should be read into the newer version
				// -- on 2nd thought, rereading everything isn't the best thing to do...
				// memcpy(&packet_db[packet_ver], &packet_db[packet_ver - 1], sizeof(packet_db[0]));
				continue;
			} else if(strcasecmp(w1,"packet_db_ver")==0) {
				// optional: if you do not wish to read multiple versions from the packet_db,
				// remove all "packet_ver: ##" lines, and define the packet DB version with this
				if(strcasecmp(w2,"default")==0)
					clif_config.packet_db_ver = MAX_PACKET_VER;
				else {
					// to manually set the packet DB version
					clif_config.packet_db_ver = atoi(w2);
					// check for invalid version
					if (clif_config.packet_db_ver > MAX_PACKET_VER ||
						clif_config.packet_db_ver < 0)
						clif_config.packet_db_ver = MAX_PACKET_VER;
				}
				continue;
			} else if(strcasecmp(w1,"enable_packet_db")==0) {
				// whether we want to allow identifying clients via the packet DB
				clif_config.enable_packet_db = config_switch(w2);
				// if we don't want to read the packet DB, and use hardcoded values only
				if (!clif_config.enable_packet_db)
					return 0;
				continue;
			} else if(strcasecmp(w1,"prefer_packet_db")==0) {
				// whether the packet DB takes higher precedence over the hardcoded one (type 1)
				// and whether to overwrite predefined packet length and functions when reading
				// from the DB (type 2)
				clif_config.prefer_packet_db = config_switch(w2); // not used for now
				continue;
			}
		}

		memset(str,0,sizeof(str));
		for(j=0,p=line;j<4 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;
		cmd=strtol(str[0],(char **)NULL,0);
		if(cmd<=0 || cmd>=MAX_PACKET_DB)
			continue;
		if(str[1]==NULL){
			ShowError("packet_db: packet len error\n");
			continue;
		}
		k = atoi(str[1]);
		// if (packet_db[packet_ver][cmd].len != k && clif_config.prefer_packet_db)	// not used for now
			packet_db[packet_ver][cmd].len = k;

		if(str[2]==NULL){
			ln++;
			continue;
		}
		for(j=0;j<sizeof(clif_parse_func)/sizeof(clif_parse_func[0]);j++){
			if(clif_parse_func[j].name != NULL &&
				strcmp(str[2],clif_parse_func[j].name)==0){
				// if (packet_db[packet_ver][cmd].func != clif_parse_func[j].func && !clif_config.prefer_packet_db)
				//	break;	// not used for now
				packet_db[packet_ver][cmd].func = clif_parse_func[j].func;
				break;
			}
		}
		// set the identifying cmd for the packet_db version
		if (strcmp(str[2],"wanttoconnection")==0){
			clif_config.connect_cmd = cmd;
		}
		if(str[3]==NULL){
			ShowError("packet_db: packet error\n");
			exit(1);
		}
		for(j=0,p2=str[3];p2;j++){
			str2[j]=p2;
			p2=strchr(p2,':');
			if(p2) *p2++=0;
			k = atoi(str2[j]);
			// if (packet_db[packet_ver][cmd].pos[j] != k && clif_config.prefer_packet_db)	// not used for now
				packet_db[packet_ver][cmd].pos[j] = k;
		}

		ln++;
//		if(packet_db[clif_config.packet_db_ver][cmd].len > 2 /* && packet_db[cmd].pos[0] == 0 */)
//			ShowMessage("packet_db ver %d: %d 0x%x %d %s %p\n",packet_ver,ln,cmd,packet_db[packet_ver][cmd].len,str[2],packet_db[packet_ver][cmd].func);
	}

	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","db/packet_db.txt");
	return 0;

}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_clif(void) {
	size_t i;

	clif_config.enable_packet_db = 1; // whether to use the packet DB for client connection
	clif_config.packet_db_ver    = 0xFFFF;	// the main packet version of the DB
	clif_config.prefer_packet_db = 1; // whether the packet version takes precedence
	clif_config.connect_cmd = 0xF5;	// the default packet used for connecting to the server

	memset(packet_db,0,sizeof(packet_db));

	// size of packet version 5 (old)
	for(i=0;i<sizeof(packet_len_table)/sizeof(packet_len_table[0]);i++){
		packet_db[0][i].len = packet_len_table[i];
	}
	// packet functions of version 5 (old)
	for(i=0;i<sizeof(clif_parse_func_table)/sizeof(clif_parse_func_table[0]);i++){
		packet_db[0][i].func = clif_parse_func_table[i];
	}

	// using hardcoded packet values isn't necessary with packet_db now,
	// but... just in case, i'll leave it to initialise for now ^^;
	// init packet version 5 and lower
	memcpy(&packet_db[1], &packet_db[0], sizeof(packet_db[0]));
	memcpy(&packet_db[2], &packet_db[1], sizeof(packet_db[0]));
	memcpy(&packet_db[3], &packet_db[2], sizeof(packet_db[0]));
	memcpy(&packet_db[4], &packet_db[3], sizeof(packet_db[0]));
	memcpy(&packet_db[5], &packet_db[4], sizeof(packet_db[0]));

#ifndef PREFER_PACKET_DB
	// functions of packet version 5-6-7 are same, but size are different
	// size of packet version 6 (7july04)
	memcpy(&packet_db[6], &packet_db[5], sizeof(packet_db[0]));
	packet_db[6][0x072].len = 22;
	packet_db[6][0x085].len = 8;
	packet_db[6][0x0a7].len = 13;
	packet_db[6][0x113].len = 15;
	packet_db[6][0x116].len = 15;
	packet_db[6][0x190].len = 95;
	// size of packet version 7 (13july04)
	memcpy(&packet_db[7], &packet_db[6], sizeof(packet_db[0]));
	packet_db[7][0x072].len = 39;
	packet_db[7][0x085].len = 9;
	packet_db[7][0x09b].len = 13;
	packet_db[7][0x09f].len = 10;
	packet_db[7][0x0a7].len = 17;
	packet_db[7][0x113].len = 19;
	packet_db[7][0x116].len = 19;
	packet_db[7][0x190].len = 99;
	// init packet function calls for packet ver 8
	memcpy(packet_db[8], packet_db[7], sizeof(packet_db[0]));
	packet_db[8][0x072].func = clif_parse_DropItem;
	packet_db[8][0x07e].func = clif_parse_WantToConnection;
	packet_db[8][0x085].func = clif_parse_UseSkillToId;
	packet_db[8][0x089].func = clif_parse_GetCharNameRequest;
	packet_db[8][0x08c].func = clif_parse_UseSkillToPos;
	packet_db[8][0x094].func = clif_parse_TakeItem;
	packet_db[8][0x09b].func = clif_parse_WalkToXY;
	packet_db[8][0x09f].func = clif_parse_ChangeDir;
	packet_db[8][0x0a2].func = clif_parse_UseSkillToPos;
	packet_db[8][0x0a7].func = clif_parse_SolveCharName;
	packet_db[8][0x0f3].func = clif_parse_GlobalMessage;
	packet_db[8][0x0f5].func = clif_parse_UseItem;
	packet_db[8][0x0f7].func = clif_parse_TickSend;
	packet_db[8][0x113].func = clif_parse_MoveToKafra;
	packet_db[8][0x116].func = clif_parse_CloseKafra;
	packet_db[8][0x190].func = clif_parse_MoveFromKafra;
	packet_db[8][0x193].func = clif_parse_ActionRequest;
	// size of packet version 8 (26july04)
	packet_db[8][0x072].len = 14;
	packet_db[8][0x07e].len = 33;
	packet_db[8][0x085].len = 20;
	packet_db[8][0x089].len = 15;
	packet_db[8][0x08c].len = 23;
	packet_db[8][0x094].len = 10;
	packet_db[8][0x09b].len = 6;
	packet_db[8][0x09f].len = 13;
	packet_db[8][0x0a2].len = 103;
	packet_db[8][0x0a7].len = 12;
	packet_db[8][0x0f3].len = -1;
	packet_db[8][0x0f5].len = 17;
	packet_db[8][0x0f7].len = 10;
	packet_db[8][0x113].len = 16;
	packet_db[8][0x116].len = 2;
	packet_db[8][0x190].len = 26;
	packet_db[8][0x193].len = 9;
	// init packet function calls for packet ver 9 (same function of packet version 8, but size are different)
	memcpy(packet_db[9], packet_db[8], sizeof(packet_db[0]));
	// size of packet version 9 (9aug04/16aug04/17aug04)
	packet_db[9][0x072].len = 17;
	packet_db[9][0x07e].len = 37;
	packet_db[9][0x085].len = 26;
	packet_db[9][0x089].len = 12;
	packet_db[9][0x08c].len = 40;
	packet_db[9][0x094].len = 13;
	packet_db[9][0x09b].len = 15;
	packet_db[9][0x09f].len = 12;
	packet_db[9][0x0a2].len = 120;
	packet_db[9][0x0a7].len = 11;
//	packet_db[9][0x0f3].len = -1;
	packet_db[9][0x0f5].len = 24;
	packet_db[9][0x0f7].len = 13;
	packet_db[9][0x113].len = 23;
//	packet_db[9][0x116].len = 2;
	packet_db[9][0x190].len = 26;
	packet_db[9][0x193].len = 18;
	// new packet
	packet_db[9][0x20f].len = 10;
	packet_db[9][0x210].len = 22;
	packet_db[9][0x212].len = 26;
	packet_db[9][0x213].len = 26;
	packet_db[9][0x214].len = 42;
	// init packet function calls for packet ver 10
	memcpy(packet_db[10], packet_db[9], sizeof(packet_db[0]));
	packet_db[10][0x072].func = clif_parse_UseItem;
	packet_db[10][0x07e].func = clif_parse_MoveToKafra;
	packet_db[10][0x085].func = clif_parse_ActionRequest;
	packet_db[10][0x089].func = clif_parse_WalkToXY;
	packet_db[10][0x08c].func = clif_parse_UseSkillToPos;
	packet_db[10][0x094].func = clif_parse_DropItem;
	packet_db[10][0x09b].func = clif_parse_GetCharNameRequest;
	packet_db[10][0x09f].func = clif_parse_GlobalMessage;
	packet_db[10][0x0a2].func = clif_parse_SolveCharName;
	packet_db[10][0x0a7].func = clif_parse_UseSkillToPos;
	packet_db[10][0x0f3].func = clif_parse_ChangeDir;
	packet_db[10][0x0f5].func = clif_parse_WantToConnection;
	packet_db[10][0x0f7].func = clif_parse_CloseKafra;
	packet_db[10][0x113].func = clif_parse_TakeItem;
	packet_db[10][0x116].func = clif_parse_TickSend;
	packet_db[10][0x190].func = clif_parse_UseSkillToId;
	packet_db[10][0x193].func = clif_parse_MoveFromKafra;
	// size of packet version 10 (6sept04)
	packet_db[10][0x072].len = 20;
	packet_db[10][0x07e].len = 19;
	packet_db[10][0x085].len = 23;
	packet_db[10][0x089].len = 9;
	packet_db[10][0x08c].len = 105;
	packet_db[10][0x094].len = 17;
	packet_db[10][0x09b].len = 14;
	packet_db[10][0x09f].len = -1;
	packet_db[10][0x0a2].len = 14;
	packet_db[10][0x0a7].len = 25;
	packet_db[10][0x0f3].len = 10;
	packet_db[10][0x0f5].len = 34;
	packet_db[10][0x0f7].len = 2;
	packet_db[10][0x113].len = 11;
	packet_db[10][0x116].len = 11;
	packet_db[10][0x190].len = 22;
	packet_db[10][0x193].len = 17;
	// init packet function calls for packet ver 11 (same function of packet version 10, but size are different)
	memcpy(packet_db[11], packet_db[10], sizeof(packet_db[0]));
	// size of packet version 11 (21sept04)
	packet_db[11][0x072].len = 18;
	packet_db[11][0x07e].len = 25;
	packet_db[11][0x085].len = 9;
	packet_db[11][0x089].len = 14;
	packet_db[11][0x08c].len = 109;
	packet_db[11][0x094].len = 19;
	packet_db[11][0x09b].len = 10;
//	packet_db[11][0x09f].len = -1;
	packet_db[11][0x0a2].len = 10;
	packet_db[11][0x0a7].len = 29;
	packet_db[11][0x0f3].len = 18;
	packet_db[11][0x0f5].len = 32;
//	packet_db[11][0x0f7].len = 2;
	packet_db[11][0x113].len = 14;
	packet_db[11][0x116].len = 14;
	packet_db[11][0x190].len = 14;
	packet_db[11][0x193].len = 12;
	// init packet function calls for packet ver 12 (same function of packet version 11, but size are different)
	memcpy(packet_db[12], packet_db[11], sizeof(packet_db[0]));
	// size of packet version 12 (18oct04)
	packet_db[12][0x072].len = 17;
	packet_db[12][0x07e].len = 16;
//	packet_db[12][0x085].len = 9;
	packet_db[12][0x089].len = 6;
	packet_db[12][0x08c].len = 103;
	packet_db[12][0x094].len = 14;
	packet_db[12][0x09b].len = 15;
//	packet_db[12][0x09f].len = -1;
	packet_db[12][0x0a2].len = 12;
	packet_db[12][0x0a7].len = 23;
	packet_db[12][0x0f3].len = 13;
	packet_db[12][0x0f5].len = 33;
//	packet_db[12][0x0f7].len = 2;
	packet_db[12][0x113].len = 10;
	packet_db[12][0x116].len = 10;
	packet_db[12][0x190].len = 20;
	packet_db[12][0x193].len = 26;
	// init packet function calls for packet ver 13 (same function of packet version 12, but size are different)
	memcpy(packet_db[13], packet_db[12], sizeof(packet_db[0]));
	// size of packet version 13 (25oct04)
	packet_db[13][0x072].len = 13;
	packet_db[13][0x07e].len = 13;
	packet_db[13][0x085].len = 15;
//	packet_db[13][0x089].len = 6;
	packet_db[13][0x08c].len = 108;
	packet_db[13][0x094].len = 12;
	packet_db[13][0x09b].len = 10;
//	packet_db[13][0x09f].len = -1;
	packet_db[13][0x0a2].len = 16;
	packet_db[13][0x0a7].len = 28;
	packet_db[13][0x0f3].len = 15;
	packet_db[13][0x0f5].len = 29;
//	packet_db[13][0x0f7].len = 2;
	packet_db[13][0x113].len = 9;
	packet_db[13][0x116].len = 9;
	packet_db[13][0x190].len = 26;
	packet_db[13][0x193].len = 22;
	// init packet function calls for packet ver 14 (same function of packet version 13, but size are different)
	memcpy(packet_db[14], packet_db[13], sizeof(packet_db[0]));
	// size of packet version 14 - Added by nsstrunks (1nov04)
	packet_db[14][0x072].len = 13;
	packet_db[14][0x07e].len = 13;
	packet_db[14][0x085].len = 15;
	packet_db[14][0x089].len = 6;
	packet_db[14][0x08c].len = 108;
	packet_db[14][0x094].len = 12;
	packet_db[14][0x09b].len = 10;
	packet_db[14][0x09f].len = -1;
	packet_db[14][0x0a2].len = 16;
	packet_db[14][0x0a7].len = 28;
	packet_db[14][0x0f3].len = 15;
	packet_db[14][0x0f5].len = 29;
	packet_db[14][0x113].len = 9;
	packet_db[14][0x116].len = 9;
	packet_db[14][0x190].len = 26;
	packet_db[14][0x193].len = 22;
	packet_db[14][0x215].len = 6;
	packet_db[14][0x216].len = 6;
	packet_db[14][0x217].len = 2;
	packet_db[14][0x218].len = 2;
	packet_db[14][0x219].len = 282;
	packet_db[14][0x21a].len = 282;
	packet_db[14][0x21b].len = 10;
	packet_db[14][0x21c].len = 10;
//	packet_db[14][0x143].len = 23;	// is this required? uncomment if it is
	// Init packet function calls for packet ver 15
	memcpy(packet_db[15], packet_db[14], sizeof(packet_db[0]));
	packet_db[15][0x072].func = clif_parse_UseSkillToId;
	packet_db[15][0x07e].func = clif_parse_UseSkillToPos;
	packet_db[15][0x089].func = clif_parse_TickSend;
	packet_db[15][0x085].func = clif_parse_GlobalMessage;
	packet_db[15][0x08c].func = clif_parse_GetCharNameRequest;
	packet_db[15][0x094].func = clif_parse_MoveToKafra;
	packet_db[15][0x09b].func = clif_parse_CloseKafra;
	packet_db[15][0x09f].func = clif_parse_ActionRequest;
	packet_db[15][0x0a2].func = clif_parse_TakeItem;
	packet_db[15][0x0a7].func = clif_parse_WalkToXY;
	packet_db[15][0x0f3].func = clif_parse_ChangeDir;
	packet_db[15][0x0f5].func = clif_parse_WantToConnection;
	packet_db[15][0x0f7].func = clif_parse_SolveCharName;
	packet_db[15][0x113].func = clif_parse_UseSkillToPos;
	packet_db[15][0x116].func = clif_parse_DropItem;
	packet_db[15][0x190].func = clif_parse_UseItem;
	packet_db[15][0x193].func = clif_parse_MoveFromKafra;
	// Size of packet version 15 - Added by nsstrunks (6dec04)
	packet_db[15][0x072].len = 22;
	packet_db[15][0x07e].len = 30;
	packet_db[15][0x094].len = 14;
	packet_db[15][0x09f].len = 18;
	packet_db[15][0x085].len = -1;
	packet_db[15][0x08c].len = 13;
	packet_db[15][0x089].len = 7;
	packet_db[15][0x09b].len = 2;
	packet_db[15][0x0a2].len = 7;
	packet_db[15][0x0a7].len = 7;
	packet_db[15][0x0f3].len = 8;
	packet_db[15][0x0f5].len = 29;
	packet_db[15][0x0f7].len = 14;
	packet_db[15][0x113].len = 110;
	packet_db[15][0x116].len = 12;
	packet_db[15][0x190].len = 15;
	packet_db[15][0x193].len = 21;
	packet_db[15][0x21d].len = 6;
	packet_db[15][0x222].len = 6;
	packet_db[15][0x221].len = -1;
	packet_db[15][0x223].len = 8;
	// Init packet function calls for packet ver 16 (10jan05)
	memcpy(packet_db[16], packet_db[15], sizeof(packet_db[0]));
	packet_db[16][0x072].func = clif_parse_UseSkillToId;
	packet_db[16][0x07e].func = clif_parse_UseSkillToPos;
	packet_db[16][0x089].func = clif_parse_TickSend;
	packet_db[16][0x0f3].func = clif_parse_GlobalMessage;
	packet_db[16][0x08c].func = clif_parse_GetCharNameRequest;
	packet_db[16][0x094].func = clif_parse_MoveToKafra;
	packet_db[16][0x193].func = clif_parse_CloseKafra;
	packet_db[16][0x190].func = clif_parse_ActionRequest;
	packet_db[16][0x0f5].func = clif_parse_TakeItem;
	packet_db[16][0x0a7].func = clif_parse_WalkToXY;
	packet_db[16][0x085].func = clif_parse_ChangeDir;
	packet_db[16][0x09b].func = clif_parse_WantToConnection;
	packet_db[16][0x0a2].func = clif_parse_SolveCharName;
	packet_db[16][0x113].func = clif_parse_UseSkillToPos;
	packet_db[16][0x116].func = clif_parse_DropItem;
	packet_db[16][0x09f].func = clif_parse_UseItem;
	packet_db[16][0x0f7].func = clif_parse_MoveFromKafra;
	// Size of packet version 16 [10jan05]
	packet_db[16][0x072].len = 26;
	packet_db[16][0x07e].len = 114;
	packet_db[16][0x089].len = 9;
	packet_db[16][0x0f3].len = -1;
	packet_db[16][0x08c].len = 8;
	packet_db[16][0x094].len = 20;
	packet_db[16][0x193].len = 2;
	packet_db[16][0x190].len = 20;
	packet_db[16][0x0f5].len = 9;
	packet_db[16][0x0a7].len = 13;
	packet_db[16][0x085].len = 23;
	packet_db[16][0x09b].len = 32;
	packet_db[16][0x0a2].len = 11;
	packet_db[16][0x113].len = 34;
	packet_db[16][0x116].len = 20;
	packet_db[16][0x09f].len = 17;
	packet_db[16][0x0f7].len = 21;
	packet_db[16][0x143].len = 10;
	// Init packet function calls for packet ver 17 (10may05)
	memcpy(packet_db[17], packet_db[16], sizeof(packet_db[0]));
	packet_db[17][0x072].len = 25;
	packet_db[17][0x07e].len = 102;
	packet_db[17][0x089].len = 8;
	packet_db[17][0x08c].len = 11;
	packet_db[17][0x094].len = 14;
	packet_db[17][0x190].len = 19;
	packet_db[17][0x0f5].len = 8;
	packet_db[17][0x0a7].len = 8;
	packet_db[17][0x085].len = 11;
	packet_db[17][0x09b].len = 26;
	packet_db[17][0x0a2].len = 15;
	packet_db[17][0x113].len = 22;
	packet_db[17][0x116].len = 10;
	packet_db[17][0x09f].len = 14;
	packet_db[17][0x0f7].len = 22;
	// Init packet function calls for the packet_db.txt (18)
	memcpy(packet_db[MAX_PACKET_VER], packet_db[MAX_PACKET_VER - 1], sizeof(packet_db[0]));
#endif

	if (clif_config.enable_packet_db)
		packetdb_readdb();

	set_defaultparse(clif_parse);


	for(i = 0; i < 10; i++) {
		if( make_listen(map_ip, map_port) >= 0 )
			break;
		sleep(20);
	}


	add_timer_func_list(clif_clearchar_delay_sub, "clif_clearchar_delay_sub");

	return 0;
}



//minimap fix [Kevin]

/* Remove dot from minimap 
 *------------------------------------------
*/
int clif_party_xy_remove(struct map_session_data &sd)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x107;
	WBUFL(buf,2)=sd.status.account_id;
	WBUFW(buf,6)=0xFFFF;
	WBUFW(buf,8)=0xFFFF;

	clif_send(buf, packet_len_table[0x107], &sd.bl, PARTY_SAMEMAP_WOS);
	return 0;
}
