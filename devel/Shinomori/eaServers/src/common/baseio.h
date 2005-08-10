#ifndef __IO_H__
#define __IO_H__

#include "base.h"
#include "showmsg.h"	// ShowMessage
#include "utils.h"		// safefopen
#include "socket.h"		// buffer iterator
#include "timer.h"		// timed config reload
#include "db.h"
#include "strlib.h"
#include "mmo.h"






//////////////////////////////////////////////////////////////////////////
// basic interface for reading configs from file
//////////////////////////////////////////////////////////////////////////
class CConfig
{
public:
	CConfig()
	{}
	virtual ~CConfig()
	{}
	//////////////////////////////////////////////////////////////////////
	// Loading a file, stripping lines, splitting it to part1 : part2
	// calling the derived function for processing
	//////////////////////////////////////////////////////////////////////
	bool LoadConfig(const char* cfgName)
	{
		char line[1024], w1[1024], w2[1024], *ip;
		FILE *fp;

		if ((fp = safefopen(cfgName, "r")) == NULL) {
			ShowError("Configuration file (%s) not found.\n", cfgName);
			return false;
		}
		ShowInfo("Reading configuration file '%s'\n", cfgName);
		while(fgets(line, sizeof(line), fp)) 
		{
			// terminate buffer
			line[sizeof(line)-1] = '\0';

			// skip leading spaces
			ip = line;
			while( isspace((int)((unsigned char)*ip) ) ) ip++; 

			// skipping comment lines
			if( ip[0] == '/' && ip[1] == '/')
				continue;
			
			memset(w2, 0, sizeof(w2));
			// format: "name:value"
			if (sscanf(ip, "%[^:]: %[^\r\n]", w1, w2) == 2)
			{
				CleanControlChars(w1);
				CleanControlChars(w2);

				if( strcasecmp(w1, "import") == 0 )
				{	// call recursive, prevent infinity loop
					if( strcasecmp(cfgName,w2) !=0 )
						LoadConfig(w2);
				}
				else
				{	// calling derived function to process
					ProcessConfig(w1,w2);
				}
			}
		}
		fclose(fp);
		ShowInfo("Reading configuration file '%s' finished\n", cfgName);
		return true;
	}
	//////////////////////////////////////////////////////////////////////
	// virtual function for processing/storing tokens
	//////////////////////////////////////////////////////////////////////
	virtual bool ProcessConfig(const char*w1,const char*w2) = 0;

	//////////////////////////////////////////////////////////////////////
	// some global data processings
	//////////////////////////////////////////////////////////////////////
	static ulong String2IP(const char* str)
	{	// host byte order
		struct hostent *h = gethostbyname(str);
		if (h != NULL) 
		{	// ip's are hostbyte order
			return	  (((ulong)h->h_addr[3]) << 0x18 )
					| (((ulong)h->h_addr[2]) << 0x10 )
					| (((ulong)h->h_addr[1]) << 0x08 )
					| (((ulong)h->h_addr[0])         );
		}
		else
			return ntohl(inet_addr(str));
	}
	static const char* IP2String(ulong ip, char*buffer=NULL)
	{	// host byte order
		// usage of the static buffer here is not threadsave
		static char temp[32], *pp= (buffer) ? buffer:temp;

		sprintf(pp, "%d.%d.%d.%d", (ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,(ip)&0xFF);
		return pp;
	}
	static int SwitchValue(const char *str, int defaultmin=0x80000000, int defaultmax=0x7FFFFFFF)
	{
		if( str )
		{
			if (strcasecmp(str, "on") == 0 || strcasecmp(str, "yes") == 0 || strcasecmp(str, "oui") == 0 || strcasecmp(str, "ja") == 0 || strcasecmp(str, "si") == 0)
				return 1;
			else if (strcasecmp(str, "off") == 0 || strcasecmp(str, "no" ) == 0 || strcasecmp(str, "non") == 0 || strcasecmp(str, "nein") == 0)
				return 0;
			else
			{
				int ret = atoi(str);
				return (ret<defaultmin) ? defaultmin : (ret>defaultmax) ? defaultmax : ret;
			}
		}
		else
			return 0;
	}
	static bool Switch(const char *str, bool defaultval=false)
	{
		if( str )
		{
			if (strcasecmp(str, "on") == 0 || strcasecmp(str, "yes") == 0 || strcasecmp(str, "oui") == 0 || strcasecmp(str, "ja") == 0 || strcasecmp(str, "si") == 0)
				return true;
			else if (strcasecmp(str, "off") == 0 || strcasecmp(str, "no" ) == 0 || strcasecmp(str, "non") == 0 || strcasecmp(str, "nein") == 0)
				return false;
		}
		return defaultval;
	}
	static bool CleanControlChars(char *str)
	{
		bool change = false;
		if(str)
		while( *str )
		{	// replace control chars 
			// but skip chars >0x7F which are negative in char representations
			if ( (*str<32) && (*str>0) )
			{
				*str = '_';
				change = true;
			}
			str++;
		}
		return change;
	}
};












///////////////////////////////////////////////////////////////////////////////
// common structures
///////////////////////////////////////////////////////////////////////////////
class CAuth
{
public:
	unsigned long login_id1;
	unsigned long login_id2;
	unsigned long client_ip;

	CAuth()		{}
	~CAuth()	{}

	///////////////////////////////////////////////////////////////////////////
	// comparing
	bool operator==(const CAuth& a) const	{ return client_ip==a.client_ip && login_id1==a.login_id1 && login_id2==a.login_id2; }
	bool operator!=(const CAuth& a) const	{ return client_ip!=a.client_ip || login_id1!=a.login_id1 || login_id2!=a.login_id2; }

	///////////////////////////////////////////////////////////////////////////
	// buffer transfer
	size_t size()	{ return 3*sizeof(unsigned long); }
	void tobuffer(unsigned char* buf)
	{
		if(!buf) return;
		_L_tobuffer( login_id1, buf);
		_L_tobuffer( login_id2, buf);
		_L_tobuffer( client_ip, buf);
	}
	void frombuffer(const unsigned char* buf)
	{
		if(!buf) return;
		_L_frombuffer( login_id1, buf);
		_L_frombuffer( login_id2, buf);
		_L_frombuffer( client_ip, buf);
	}
};

class CAccoutReg
{
public:
	unsigned short account_reg2_num;
	struct global_reg account_reg2[ACCOUNT_REG2_NUM];

	CAccoutReg()	{}
	~CAccoutReg()	{}

	///////////////////////////////////////////////////////////////////////////
	// buffer transfer
	size_t size()	{ return sizeof(unsigned short)+ACCOUNT_REG2_NUM*sizeof(struct global_reg); }
	void tobuffer(unsigned char* buf)
	{
		size_t i;
		if(!buf) return;
		_W_tobuffer( (account_reg2_num),	buf);
		for(i=0; i<account_reg2_num; i++)
			_global_reg_tobuffer(account_reg2[i],buf);
	}
	void frombuffer(const unsigned char* buf)
	{
		size_t i;
		if(!buf) return;
		_W_frombuffer( (account_reg2_num),	buf);
		for(i=0; i<account_reg2_num; i++)
			_global_reg_frombuffer(account_reg2[i],buf);
	}
};
class CAccoutID : public CAccoutReg
{
public:
	unsigned long account_id;
	char userid[24];
	char passwd[34];
	unsigned char gm_level;
	unsigned char sex;

	char email[40];
	unsigned long login_count;
	char last_ip[16];
	char last_login[24];
	time_t ban_until;
	time_t valid_until;

	CAccoutID()		{}
	~CAccoutID()	{}

	///////////////////////////////////////////////////////////////////////////
	// buffer transfer
	size_t size()	
	{
		return 
		sizeof(account_id) +
		sizeof(userid) +
		sizeof(passwd) +
		sizeof(gm_level) +
		sizeof(sex) +
		sizeof(email) +
		sizeof(login_count) +
		sizeof(last_login) +
		2*sizeof(time_t) +
		CAccoutReg::size();
	}
	void tobuffer(unsigned char* buf)
	{
		unsigned long time;
		if(!buf) return;
		_L_tobuffer( account_id,	buf);
		_S_tobuffer( userid,		buf, sizeof(userid));
		_S_tobuffer( passwd,		buf, sizeof(passwd));
		_B_tobuffer( gm_level,		buf);
		_B_tobuffer( sex,			buf);
		_S_tobuffer( email,			buf, sizeof(email));
		_L_tobuffer( login_count,	buf);
		_S_tobuffer( last_login,	buf, sizeof(last_login));
		time = ban_until;	_L_tobuffer( time, buf);
		time = valid_until;	_L_tobuffer( time, buf);

		CAccoutReg::tobuffer(buf);
	}
	void frombuffer(const unsigned char* buf)
	{
		unsigned long time;
		if(!buf) return;
		_L_frombuffer( account_id,	buf);
		_S_frombuffer( userid,		buf, sizeof(userid));
		_S_frombuffer( passwd,		buf, sizeof(passwd));
		_B_frombuffer( gm_level,	buf);
		_B_frombuffer( sex,			buf);
		_S_frombuffer( email,		buf, sizeof(email));
		_L_frombuffer( login_count,	buf);
		_S_frombuffer( last_login,	buf, sizeof(last_login));
		_L_frombuffer( time, buf);	ban_until=time;
		_L_frombuffer( time, buf);	valid_until=time;

		CAccoutReg::frombuffer(buf);
	}
};

///////////////////////////////////////////////////////////////////////////////
// login structure
///////////////////////////////////////////////////////////////////////////////
class CLoginAccount : public CAccoutID, public CAuth
{
public:
	CLoginAccount()		{}
	~CLoginAccount()	{}

	///////////////////////////////////////////////////////////////////////////
	// creation of a new account
	CLoginAccount(unsigned long accid, const char* uid, const char* pwd, unsigned char s, const char* em)
	{	// init account data
		this->account_id = accid;
		safestrcpy(this->userid, uid, 24);
		safestrcpy(this->passwd, pwd, 34);
		this->sex = sex;
		if( !email_check(em) )
			safestrcpy(this->email, "a@a.com", 40);
		else
			safestrcpy(this->email, em, 40);
		this->gm_level=0;
		this->login_count=0;
		*this->last_login= 0;
		this->ban_until = 0;
		this->valid_until = 0;
		this->account_reg2_num=0;
	}
	CLoginAccount(const char* uid)		{ safestrcpy(this->userid, uid, sizeof(this->userid));  }
	CLoginAccount(unsigned long accid)	{ this->account_id=accid; }


	///////////////////////////////////////////////////////////////////////////
	// for sorting by userid
	bool operator==(const CLoginAccount& c) const { return 0==strcmp(this->userid, c.userid); }
	bool operator!=(const CLoginAccount& c) const { return 0!=strcmp(this->userid, c.userid); }
	bool operator> (const CLoginAccount& c) const { return 0> strcmp(this->userid, c.userid); }
	bool operator>=(const CLoginAccount& c) const { return 0>=strcmp(this->userid, c.userid); }
	bool operator< (const CLoginAccount& c) const { return 0< strcmp(this->userid, c.userid); }
	bool operator<=(const CLoginAccount& c) const { return 0<=strcmp(this->userid, c.userid); }

	///////////////////////////////////////////////////////////////////////////
	// compare for Multilist
	int compare(const CLoginAccount& c, size_t i=0) const	
	{
		if(i==0)
			return (account_id - c.account_id);
		else
			return strcmp(this->userid, c.userid); 
	}


};
///////////////////////////////////////////////////////////////////////////////
// char structures
///////////////////////////////////////////////////////////////////////////////
class CCharAccount : public CAccoutID, public CAuth
{
public:
	unsigned long charid[9];

	CCharAccount()		{}
	~CCharAccount()		{}

	///////////////////////////////////////////////////////////////////////////
	// creation and sorting by accountid
	CCharAccount(unsigned long aid)				{ this->account_id=aid; }
	bool operator==(const CCharAccount& c) const { return this->account_id==c.account_id; }
	bool operator!=(const CCharAccount& c) const { return this->account_id!=c.account_id; }
	bool operator> (const CCharAccount& c) const { return this->account_id> c.account_id; }
	bool operator>=(const CCharAccount& c) const { return this->account_id>=c.account_id; }
	bool operator< (const CCharAccount& c) const { return this->account_id< c.account_id; }
	bool operator<=(const CCharAccount& c) const { return this->account_id<=c.account_id; }
};

class CCharCharacter : public mmo_charstatus
{
public:
	CCharCharacter()		{}
	~CCharCharacter()		{}

	///////////////////////////////////////////////////////////////////////////
	// creation and sorting by charid
	CCharCharacter(unsigned long aid)			{ this->account_id=aid; }
	bool operator==(const CCharCharacter& c) const { return this->char_id==c.char_id; }
	bool operator!=(const CCharCharacter& c) const { return this->char_id!=c.char_id; }
	bool operator> (const CCharCharacter& c) const { return this->char_id> c.char_id; }
	bool operator>=(const CCharCharacter& c) const { return this->char_id>=c.char_id; }
	bool operator< (const CCharCharacter& c) const { return this->char_id< c.char_id; }
	bool operator<=(const CCharCharacter& c) const { return this->char_id<=c.char_id; }
};

///////////////////////////////////////////////////////////////////////////////
// map structure
///////////////////////////////////////////////////////////////////////////////
class CMapCharacter : public CCharCharacter, public CAuth
{
public:
	CMapCharacter()			{}
	~CMapCharacter()		{}

};















///////////////////////////////////////////////////////////////////////////////
// Account Database
// for storing accounts stuff in login
///////////////////////////////////////////////////////////////////////////////

class CAccountDB : private CConfig, public global, public noncopyable
{
	///////////////////////////////////////////////////////////////////////////
	// config stuff


	///////////////////////////////////////////////////////////////////////////
	// data
	TMultiListP<CLoginAccount, 2> cList; //!! Multilist is pod type


public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CAccountDB()
	{}
	virtual ~CAccountDB()
	{
		close();
	}

	///////////////////////////////////////////////////////////////////////////
	// Config processor
	virtual bool ProcessConfig(const char*w1, const char*w2);


	bool insert(const CLoginAccount& la){ return cList.insert(la); }
public:

	size_t size()						{ return cList.size(); }
	CLoginAccount& operator[](size_t i)	{ return cList[i]; };


	///////////////////////////////////////////////////////////////////////////
	// functions for db implementation dependend access
	bool init(const char* configfile);
	bool close();

	bool existAccount(const char* userid);
	CLoginAccount* searchAccount(const char* userid);
	CLoginAccount* searchAccount(unsigned long accid);
	CLoginAccount* insertAccount(const char* userid, const char* passwd, unsigned char sex, const char* email);

	bool deleteAccount(CLoginAccount* account);
	bool saveAccount(CLoginAccount* account);
	bool saveAccount();

};











#endif//__IO_H__
