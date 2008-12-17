// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/mmo.h"
#include "../common/socket.h"
#include "../common/strlib.h"
#include "char.h" // mapif_sendallwos()
#include "int_registry.h"
#include "regdb.h"
#include <stdio.h>
#include <string.h>


// accreg and charreg database
AccRegDB* accregs = NULL;
CharRegDB* charregs = NULL;


// temporary imports
extern int login_fd;


/// Serializes regs into the provided buffer.
/// Returns the number of bytes written.
int inter_regs_tobuf(uint8* buf, size_t size, const struct regs* reg)
{
	int c = 0;
	int i;

	for( i = 0; i < reg->reg_num; i++)
	{
		c += sprintf((char*)(buf+c), "%s", reg->reg[i].str)+1; //We add 1 to consider the '\0' in place.
		c += sprintf((char*)(buf+c), "%s", reg->reg[i].value)+1;
	}

	return c;
}

/// Retrieves regs from the provided buffer.
/// Returns ??
int inter_regs_frombuf(const uint8* buf, size_t size, struct regs* reg)
{
	size_t p; // cursor
	int j;

	//FIXME: doesn't watch the buffer size properly
	//FIXME: doesn't respect the specific regs maxima
	for( j = 0, p = 0; j < GLOBAL_REG_NUM && p < size; ++j )
	{
		safestrncpy(reg->reg[j].str, (const char*)(buf+p), sizeof(reg->reg[j].str));
		p += strlen(reg->reg[j].str)+1; //+1 to skip the '\0' between strings.
		safestrncpy(reg->reg[j].value, (const char*)(buf+p), sizeof(reg->reg[j].value));
		p += strlen(reg->reg[j].value)+1;
	}

	reg->reg_num = j;

	return 0;
}


//Request account registry from login server
int request_accreg2(int account_id, int char_id)
{
	if (login_fd > 0) {
		WFIFOHEAD(login_fd,10);
		WFIFOW(login_fd,0) = 0x272e;
		WFIFOL(login_fd,2) = account_id;
		WFIFOL(login_fd,6) = char_id;
		WFIFOSET(login_fd,10);
		return 1;
	}
	return 0;
}

//Send packet forward to login-server for account saving
int save_accreg2(unsigned char* buf, int len)
{
	if (login_fd > 0) {
		WFIFOHEAD(login_fd,len+4);
		memcpy(WFIFOP(login_fd,4), buf, len);
		WFIFOW(login_fd,0) = 0x2728;
		WFIFOW(login_fd,2) = len+4;
		WFIFOSET(login_fd,len+4);
		return 1;
	}
	return 0;
}

// registry transfer to map-server
static void mapif_regs(int fd, unsigned char *src)
{
	WBUFW(src,0) = 0x3804; //NOTE: writing to RFIFO
	mapif_sendallwos(fd, src, WBUFW(src,2));
}

// Send the requested regs
static void mapif_regs_reply(int fd, int account_id, int char_id, int type, const struct regs* reg)
{
	WFIFOHEAD(fd, 13 + ACCOUNT_REG_NUM * 288);
	WFIFOW(fd,0) = 0x3804;
	WFIFOL(fd,4) = account_id;
	WFIFOL(fd,8) = char_id;
	WFIFOB(fd,12) = type;
	WFIFOW(fd,2) = 13 + inter_regs_tobuf(WFIFOP(fd,13), ACCOUNT_REG_NUM * 288, reg);
	WFIFOSET(fd,WFIFOW(fd,2));
}


// save incoming registry
// 3004 <length>.w <aid>.l <cid>.l <type>.b { <str>.s <val>.s }*
int mapif_parse_Registry(int fd)
{
	int length = RFIFOW(fd,2);
	int account_id = RFIFOL(fd,4);
	int char_id = RFIFOL(fd,8);
	int type = RFIFOB(fd,12);
	uint8* buf = RFIFOP(fd,13);

	switch( type )
	{
	case 3: //Character registry
	{
		struct regs reg;
		inter_regs_frombuf(buf, length-13, &reg);
		charregs->save(charregs, &reg, char_id);
		mapif_regs(fd,RFIFOP(fd,0)); // Send regs to other map servers.
		return 0;
	}
	break;

	case 2: //Account Registry
	{
		struct regs reg;
		inter_regs_frombuf(buf, length-13, &reg);
		accregs->save(accregs, &reg, account_id);
		mapif_regs(fd,RFIFOP(fd,0)); // Send regs to other map servers.
		return 0;
	}
	break;

	case 1: //Account2 registry
		return save_accreg2(RFIFOP(fd,4), length-4); // must be sent over to login server.
	break;

	default: //Error?
		return 1;
	}
}

// Request the value of all registries.
int mapif_parse_RegistryRequest(int fd)
{
	int account_id = RFIFOL(fd,2);
	int char_id = RFIFOL(fd,6);
	int accreg2 = RFIFOB(fd,10);
	int accreg = RFIFOB(fd,11);
	int charreg = RFIFOB(fd,12);

	if( charreg )
	{// Load Char Registry
		struct regs charreg;
		charregs->load(charregs, &charreg, char_id);
		mapif_regs_reply(fd, account_id, char_id, 3, &charreg);
	}

	if( accreg )
	{// Load Account Registry
		struct regs accreg;
		accregs->load(accregs, &accreg, account_id);
		mapif_regs_reply(fd, account_id, char_id, 2, &accreg);
	}

	if( accreg2 )
	{// Ask Login Server for Account2 values.
		request_accreg2(account_id, char_id);
	}

	return 1;
}


int inter_registry_parse_frommap(int fd)
{
	switch(RFIFOW(fd,0))
	{
	case 0x3004: mapif_parse_Registry(fd); break;
	case 0x3005: mapif_parse_RegistryRequest(fd); break;
	default:
		return 0;
	}
	return 1;
}

int inter_registry_sync(void)
{
	accregs->sync(accregs);
	charregs->sync(charregs);
	return 0;
}


bool inter_charreg_delete(int char_id)
{
	return charregs->remove(charregs, char_id);
}


int inter_registry_init(AccRegDB* accregdb, CharRegDB* charregdb)
{
	accregs = accregdb;
	charregs = charregdb;
	return 0;
}


int inter_registry_final(void)
{
	accregs = NULL;
	charregs = NULL;
	return 0;
}
