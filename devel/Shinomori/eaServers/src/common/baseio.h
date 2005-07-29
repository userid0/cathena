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
		while(fgets(line, sizeof(line)-1, fp)) 
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



//////////////////////////////////////////////////////////////////////////
// Basic Interface for logging
//////////////////////////////////////////////////////////////////////////
class CLogger : private Mutex
{
	bool	cToScreen;
	FILE*	cToFile;

	bool log_screen(const char* msg, va_list va)
	{
		if(cToScreen)
		{
			ScopeLock sl(*this);
			vprintf(msg,va);
			return true;
		}
		return false;
	}
		
	bool log_file(const char* msg, va_list va)
	{
		if(cToFile)
		{	ScopeLock sl(*this);
			vfprintf(cToFile,msg,va);
			fflush(cToFile);
			return true;
		}
		return false;
	}
public:

	CLogger(bool s=true) : cToScreen(s), cToFile(NULL)
	{
	}
	CLogger(bool s, const char* n) : cToScreen(s), cToFile(NULL)
	{	
		open(n);
	}
	CLogger(const char* n) : cToScreen(true), cToFile(NULL)
	{	
		open(n);
	}
	~CLogger()
	{
		close();
	}

	bool open(const char* name)
	{
		close();
		cToFile = fopen(name, "a");
		return (NULL != cToFile);
	}

	bool close()
	{
		if(cToFile)
		{
			fclose(cToFile);
			cToFile = NULL;
		}
		return true;
	}

	bool SetLog(bool s)
	{
		bool x=cToScreen;
		cToScreen = s;
		return x;
	}
	bool isLog()
	{
		return cToScreen;
	}

	bool log(const char* msg, ...)
	{
		bool ret;
		va_list va;
		va_start( va, msg );
		ret  = log_screen(msg, va);
		ret &= log_file(msg, va);
		va_end( va );
		return ret;
	}
	bool log_screen(const char* msg,...)
	{
		bool ret;
		va_list va;
		va_start( va, msg );
		ret = log_screen(msg, va);
		va_end( va );
		return ret;
	}
	bool log_file(const char* msg,...)
	{
		bool ret;
		va_list va;
		va_start( va, msg );
		ret = log_file(msg, va);
		va_end( va );
		return ret;
	}
};




//////////////////////////////////////////////////////////////////////////
// Basic Database Interfaces
//////////////////////////////////////////////////////////////////////////

class Database : public global, public noncopyable
{
protected:
	Database()					{  }
	virtual ~Database()			{  }

};






//////////////////////////////////////////////////////////////////////////
// Account Database
// for storing accounts stuff in login
//////////////////////////////////////////////////////////////////////////

class CAccountDB : public Database, public CConfig
{
	///////////////////////////////////////////////////////////////////////////
	// config stuff
	bool	newaccount_allow;
	ulong	newaccount_interval;
	ulong	newaccount_tick;

	///////////////////////////////////////////////////////////////////////////
	// data
	//!! safe the indexed caching list for later
	class CAccountData : public account_data
	{
	public:
		bool operator==(const CAccountData& a) const	{ return this->account_id==a.account_id; }
	};
	TArrayDST<CAccountData> cList;

public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CAccountDB() : newaccount_allow(false), newaccount_interval(0), newaccount_tick(gettick())
	{}
	virtual ~CAccountDB()
	{}

	///////////////////////////////////////////////////////////////////////////
	// Config processor
	virtual bool ProcessConfig(const char*w1, const char*w2)
	{
		if(w1 && w2)
		{
			if( 0==strcasecmp(w1, "newaccount_allow") )
				newaccount_allow = Switch(w2);
			else if( 0==strcasecmp(w1, "newaccount_interval") )
				newaccount_interval = SwitchValue(w2, 10, 600);
		}
		return true;
	}

private:
	///////////////////////////////////////////////////////////////////////////
	// private fnctions for db implementation dependend access
	bool existAccount(const char* userid);
	struct account_data* searchAccount(unsigned long account_id);
	struct account_data* searchAccount(const char* userid, const char* pass);
	struct account_data* insertAccount(const char* userid, const char* pass, unsigned char sex, char* email, int& error);
public:

	///////////////////////////////////////////////////////////////////////////
	// public functions for db implementation dependend access
	bool init(const char* configfile);
	bool saveAccount(account_data* account);
	bool saveAccount();


	///////////////////////////////////////////////////////////////////////////
	// db implementation independend access
	struct account_data* getAccount(unsigned long account_id)
	{
		return searchAccount(account_id);
	}
	struct account_data* getAccount(const char* userid, const char* pass, int& error, const char*& errmsg)
	{	// used error codes
		// 0 = Unregistered ID/Incorrect Password
		// 1 = Account already exists
		// 2 = This ID is expired
		// 3 = Login registration not allowed/temorarily disabled
		// 6 = Your are Prohibited to log in (aka ban)
		account_data* ret=NULL;
		// default error message
		error = 0;
		errmsg= "";
		if( userid && pass )
		{
			size_t pos = strlen(userid);
			if( pos<24 && userid[pos-2] == '_' && (userid[pos-1] == 'M' || userid[pos-1] == 'F') )
			{	// _M/_F registration
				// always cut the _M/_F suffix
				char tempid[32];
				memcpy(tempid,userid,pos-2);
				tempid[pos-2] = 0;

				if( !newaccount_allow )
				{
					error = 3;
					errmsg= "Login Registration not allowed.";
				}
				else if( newaccount_interval && DIFF_TICK(newaccount_tick, gettick())>0 )
				{
					error = 3;
					errmsg= "Login Registration temorarily disabled.";
				}
				if( strlen(pass)<4 )
				{
					error = 1;
					errmsg= "Password with at least 4 chars required.";
				}
				else if( existAccount(tempid) )
				{
					error = 1;
					errmsg= "Userid is already registered.";
				}
				else
				{	// looks ok so far; insert the new account
					ret = insertAccount(tempid, pass, (userid[pos-1]=='M'), "a@a.com", error);
					if(ret && newaccount_interval)
						newaccount_tick = gettick() + 1000*newaccount_interval;
				}
			}
			else
			{	// look for an existing account
				ret = searchAccount(userid, pass);
				if( !ret )
				{
					error = 0;
					errmsg= "Userid/Password not recognized.";
				}
				else
				{
					if( ret->ban_until_time  )
					{
						if( ret->ban_until_time > time(NULL) )
						{
							error = 6;
							errmsg= "Your are prohibited to log in";
							ret = NULL;
						}
						else
						{	// end of ban
							ret->ban_until_time = 0; // reset the ban time
							// account will be saved when login process is finished
						}
					}
					if( ret->connect_until_time && ret->connect_until_time < time(NULL) )
					{
						error = 2;
						errmsg= "Your ID has expired.";
						ret = NULL;
					}
				}
			}
		}
		return ret;
	}
};




#endif//__IO_H__
