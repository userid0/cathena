#ifndef __BASEMEM_H__
#define __BASEMEM_H__

#include "baseio.h"
#include "basecharset.h"
#include "basetxtdb.h"

///////////////////////////////////////////////////////////////////////////////
// abstract implementation of the databases 
// using in-memory storage
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// account database
///////////////////////////////////////////////////////////////////////////////
class CAccountDB_mem : public CAccountDBInterface
{
protected:
	///////////////////////////////////////////////////////////////////////////
	/// helper class for gm_level reading.
	/// always reads from file
	class CMapGM
	{
	public:
		uint32 account_id;
		unsigned char gm_level;

		CMapGM(uint32 accid=0) : account_id(accid), gm_level(0)	{}
		CMapGM(uint32 accid, unsigned char lv) : account_id(accid), gm_level(lv)	{}

		bool operator==(const CMapGM& c) const { return this->account_id==c.account_id; }
		bool operator!=(const CMapGM& c) const { return this->account_id!=c.account_id; }
		bool operator> (const CMapGM& c) const { return this->account_id> c.account_id; }
		bool operator>=(const CMapGM& c) const { return this->account_id>=c.account_id; }
		bool operator< (const CMapGM& c) const { return this->account_id< c.account_id; }
		bool operator<=(const CMapGM& c) const { return this->account_id<=c.account_id; }
	};
	///////////////////////////////////////////////////////////////////////////
	/// read gm levels to a list.
	bool readGMAccount(basics::slist<CMapGM> &gmlist);

	///////////////////////////////////////////////////////////////////////////
	/// reads all accounts from source to memory.
	virtual bool do_readAccounts()=0;
	///////////////////////////////////////////////////////////////////////////
	/// writes all accounts to source.
	virtual bool do_saveAccounts()=0;
	///////////////////////////////////////////////////////////////////////////
	/// save a single account.
	/// does nothing by default, needs overloading
	virtual void do_saveAccount(const CLoginAccount& account)	{}
	///////////////////////////////////////////////////////////////////////////
	/// create a single account.
	/// does nothing by default, needs overloading
	virtual void do_createAccount(const CLoginAccount& account)	{}
	///////////////////////////////////////////////////////////////////////////
	/// delete a single account.
	/// does nothing by default, needs overloading
	virtual void do_removeAccount(const CLoginAccount& account)	{}


	///////////////////////////////////////////////////////////////////////////
	time_t creation_time_GM_account_file;	///< timestamp of gm file
	basics::TMultiListP<CLoginAccount, 2> cList;///< double-indexed array of accounts
	///////////////////////////////////////////////////////////////////////////
	size_t			cPos;					///< position marker for alternative interface 
	basics::Mutex	cMx;					///< syncronisation for alternative interface 

public:
	///////////////////////////////////////////////////////////////////////////
	/// constructor.
	CAccountDB_mem(const char* configfile=NULL);
	///////////////////////////////////////////////////////////////////////////
	/// destructor.
	virtual ~CAccountDB_mem();

protected:
	///////////////////////////////////////////////////////////////////////////
	/// clean init.
	virtual bool init(const char* configfile)=0;
	///////////////////////////////////////////////////////////////////////////
	/// close the database.
	virtual bool close()=0;

public:
	///////////////////////////////////////////////////////////////////////////
	/// number of entries.
	virtual size_t size() const;
	///////////////////////////////////////////////////////////////////////////
	/// random access operator. internally sorted by account_id
	virtual CLoginAccount& operator[](size_t i);
	///////////////////////////////////////////////////////////////////////////
	/// query existance of a userid.
	virtual bool existAccount(const char* userid);
	///////////////////////////////////////////////////////////////////////////
	/// search and return an account by userid.
	virtual bool searchAccount(const char* userid, CLoginAccount&account);
	///////////////////////////////////////////////////////////////////////////
	/// search and return an account by account_id.
	virtual bool searchAccount(uint32 accid, CLoginAccount&account);

	///////////////////////////////////////////////////////////////////////////
	/// create an new account.
	virtual bool insertAccount(const char* userid, const char* passwd, unsigned char sex, const char* email, CLoginAccount&account);
	///////////////////////////////////////////////////////////////////////////
	/// remove account by account_id.
	virtual bool removeAccount(uint32 accid);
	///////////////////////////////////////////////////////////////////////////
	/// remove account by account_id.
	virtual bool saveAccount(const CLoginAccount& account);

	///////////////////////////////////////////////////////////////////////////
	/// aquire the iterator interface.
	virtual bool aquire();
	///////////////////////////////////////////////////////////////////////////
	/// release the iterator interface.
	virtual bool release();
	///////////////////////////////////////////////////////////////////////////
	/// set iterator to first entry.
	virtual bool first();
	///////////////////////////////////////////////////////////////////////////
	/// check if iterator is valid.
	virtual operator bool();
	///////////////////////////////////////////////////////////////////////////
	/// select the next entry.
	virtual bool operator++(int);
	///////////////////////////////////////////////////////////////////////////
	/// save the current entry.
	virtual bool save();
	///////////////////////////////////////////////////////////////////////////
	/// set iterator to specific by using userid.
	virtual bool find(const char* userid);
	///////////////////////////////////////////////////////////////////////////
	/// set iterator to specific by using account_id.
	virtual bool find(uint32 accid);
	///////////////////////////////////////////////////////////////////////////
	/// return the currently selected entry
	virtual CLoginAccount& operator()();
};


///////////////////////////////////////////////////////////////////////////////
/// character database.
///////////////////////////////////////////////////////////////////////////////
class CCharDB_mem : public CCharDBInterface
{
protected:
	///////////////////////////////////////////////////////////////////////////
	/// create a new character
	bool make_new_char(CCharCharAccount& account, const char *n, unsigned char str, unsigned char agi, unsigned char vit, unsigned char int_, unsigned char dex, unsigned char luk, unsigned char slot, unsigned char hair_style, unsigned char hair_color, CCharCharacter& target);

	///////////////////////////////////////////////////////////////////////////
	/// reads all chars from source to memory.
	virtual bool do_readChars()=0;
	///////////////////////////////////////////////////////////////////////////
	/// writes all chars to source.
	virtual bool do_saveChars()=0;
	///////////////////////////////////////////////////////////////////////////
	/// save a single char.
	/// does nothing by default, needs overloading
	virtual void do_saveChar(const CCharCharacter& data)	{}
	///////////////////////////////////////////////////////////////////////////
	/// create a single char.
	/// does nothing by default, needs overloading
	virtual void do_createChar(const CCharCharacter& data)	{}
	///////////////////////////////////////////////////////////////////////////
	/// delete a single char.
	/// does nothing by default, needs overloading
	virtual void do_removeChar(const CCharCharacter& data)	{}

	///////////////////////////////////////////////////////////////////////////
	basics::TMultiListP<CCharCharacter, 2>	cCharList;
	basics::TslistDCT<CCharCharAccount>		cAccountList;
	///////////////////////////////////////////////////////////////////////////
	// data for alternative interface
	basics::Mutex	cMx;
	size_t			cPos;

	CCharDB_mem(const char* configfile=NULL);
public:
	virtual ~CCharDB_mem();

protected:
	///////////////////////////////////////////////////////////////////////////
	/// clean init.
	virtual bool init(const char* configfile)=0;
	///////////////////////////////////////////////////////////////////////////
	/// close the database.
	virtual bool close()=0;

public:
	///////////////////////////////////////////////////////////////////////////
	// number of entries.
	virtual size_t size() const;
	///////////////////////////////////////////////////////////////////////////
	// random access operator.
	virtual CCharCharacter& operator[](size_t i);
	///////////////////////////////////////////////////////////////////////////
	// existance by name.
	virtual bool existChar(const char* name);
	///////////////////////////////////////////////////////////////////////////
	// search and return by name.
	virtual bool searchChar(const char* name, CCharCharacter&data);
	///////////////////////////////////////////////////////////////////////////
	// search and return by id.
	virtual bool searchChar(uint32 charid, CCharCharacter&data);
	///////////////////////////////////////////////////////////////////////////
	// creates a new character.
	virtual bool insertChar(CCharAccount &account, const char *name, unsigned char str, unsigned char agi, unsigned char vit, unsigned char int_, unsigned char dex, unsigned char luk, unsigned char slot, unsigned char hair_style, unsigned char hair_color, CCharCharacter&data);
	///////////////////////////////////////////////////////////////////////////
	// remove a character.
	virtual bool removeChar(uint32 charid);
	///////////////////////////////////////////////////////////////////////////
	// save a character.
	virtual bool saveChar(const CCharCharacter& data);

	///////////////////////////////////////////////////////////////////////////
	// search an account.
	virtual bool searchAccount(uint32 accid, CCharCharAccount& account);
	///////////////////////////////////////////////////////////////////////////
	// save an account.
	virtual bool saveAccount(CCharAccount& account);
	///////////////////////////////////////////////////////////////////////////
	// remove an account.
	virtual bool removeAccount(uint32 accid);

	///////////////////////////////////////////////////////////////////////////
	// aquire the alternative iterator interface.
	virtual bool aquire();
	///////////////////////////////////////////////////////////////////////////
	// release the alternative iterator interface.
	virtual bool release();
	///////////////////////////////////////////////////////////////////////////
	// set the iterator to the first entry.
	virtual bool first();
	///////////////////////////////////////////////////////////////////////////
	// checks validity of the iterator.
	virtual operator bool();
	///////////////////////////////////////////////////////////////////////////
	// select the next entry.
	virtual bool operator++(int);
	///////////////////////////////////////////////////////////////////////////
	// save the currently selected entry.
	virtual bool save();
	///////////////////////////////////////////////////////////////////////////
	// set the iterator by name.
	virtual bool find(const char* name);
	///////////////////////////////////////////////////////////////////////////
	// set the iterator by id.
	virtual bool find(uint32 charid);
	///////////////////////////////////////////////////////////////////////////
	// return the currently selected entry.
	virtual CCharCharacter& operator()();
	///////////////////////////////////////////////////////////////////////////
	// return the fame list.
	virtual void loadfamelist();
};




///////////////////////////////////////////////////////////////////////////////
/// guild/castle database.
///////////////////////////////////////////////////////////////////////////////
class CGuildDB_mem : public CGuildDBInterface
{
protected:
	virtual bool do_readGuildsCastles()=0;
	virtual bool do_saveGuildsCastles()=0;
	virtual void do_createGuild(const CGuild& guild)	{}
	virtual void do_saveGuild(const CGuild& guild)		{}
	virtual void do_removeGuild(const CGuild& guild)	{}

	virtual void do_saveCastle(const CCastle& castle)	{}


	///////////////////////////////////////////////////////////////////////////
	/// constructor.
	CGuildDB_mem(const char *configfile=NULL);
public:
	///////////////////////////////////////////////////////////////////////////
	/// destructor.
	virtual ~CGuildDB_mem();
protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::TMultiListP<CGuild, 2>	cGuilds;
	basics::slist<CCastle>			cCastles;

	///////////////////////////////////////////////////////////////////////////
	// data for alternative interface
	basics::Mutex cMxGuild;
	basics::Mutex cMxCastle;
	size_t cPosGuild;
	size_t cPosCastle;
protected:
	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile)=0;
	virtual bool close()=0;

public:
	virtual size_t size() const;
	virtual CGuild& operator[](size_t i);

	virtual size_t castlesize() const;
	virtual CCastle& castle(size_t i);


	virtual bool searchGuild(const char* name, CGuild& guild);
	virtual bool searchGuild(uint32 guildid, CGuild& guild);
	virtual bool insertGuild(const struct guild_member &member, const char *name, CGuild &guild);
	virtual bool removeGuild(uint32 guildid);
	virtual bool saveGuild(const CGuild& guild);

	virtual bool searchCastle(ushort cid, CCastle& castle);
	virtual bool saveCastle(const CCastle& castle);

	virtual bool getCastles(basics::vector<CCastle>& castlevector)	{ castlevector = cCastles; return true; }
	virtual uint32 has_conflict(uint32 guild_id, uint32 account_id, uint32 char_id)
	{
		size_t i,k;
		for(i=0; i<cGuilds.size(); ++i)
		{
			if( cGuilds[i].guild_id != guild_id )
			{
				for(k=0; k<MAX_GUILD; ++k)
				{
					if( cGuilds[i].member[k].account_id == account_id && 
						cGuilds[i].member[k].char_id == char_id)
					{	
						return cGuilds[i].guild_id;
					}
				}
			}
		}
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////
	// alternative interface
	virtual bool aquireGuild();
	virtual bool aquireCastle();
	virtual bool releaseGuild();
	virtual bool releaseCastle();
	virtual bool firstGuild();
	virtual bool firstCastle();
	virtual bool isGuildOk();
	virtual bool isCastleOk();
	virtual bool nextGuild();
	virtual bool nextCastle();
	virtual bool saveGuild();
	virtual bool saveCastle();

	virtual bool findGuild(const char* name);
	virtual bool findGuild(uint32 guildid);
	virtual bool findCastle(ushort cid);
	virtual CGuild& getGuild();
	virtual CCastle& getCastle();
};




///////////////////////////////////////////////////////////////////////////////
// Party Database
///////////////////////////////////////////////////////////////////////////////
class CPartyDB_mem : public CPartyDBInterface
{
protected:
	virtual bool do_readParties()=0;
	virtual bool do_saveParties()=0;
	virtual void do_createParty(const CParty& party)	{}
	virtual void do_saveParty(const CParty& party)		{}
	virtual void do_removeParty(const CParty& party)	{}


	CPartyDB_mem(const char *configfile=NULL);
public:
	virtual ~CPartyDB_mem();
protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::TMultiListP<CParty, 2>	cParties;

	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile)=0;
	virtual bool close()=0;

public:
	///////////////////////////////////////////////////////////////////////////
	// access interface
	virtual size_t size() const;
	virtual CParty& operator[](size_t i);

	virtual bool searchParty(const char* name, CParty& party);
	virtual bool searchParty(uint32 pid, CParty& party);
	virtual bool insertParty(uint32 accid, const char *nick, const char *mapname, ushort lv, const char *name, CParty &party);
	virtual bool removeParty(uint32 pid);
	virtual bool saveParty(const CParty& party);
};




///////////////////////////////////////////////////////////////////////////////
// Storage Database Interface
///////////////////////////////////////////////////////////////////////////////
class CPCStorageDB_mem : public CPCStorageDBInterface
{
protected:
	virtual bool do_readPCStorage()=0;
	virtual bool do_savePCStorage()=0;
	virtual void do_cratePCStorage(const CPCStorage& stor)	{}
	virtual void do_savePCStorage(const CPCStorage& stor)	{}
	virtual void do_removePCStorage(const CPCStorage& stor)	{}


	CPCStorageDB_mem(const char *dbcfgfile=NULL);
public:
	virtual ~CPCStorageDB_mem();

protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::TslistDST<CPCStorage>	cPCStorList;

	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile)=0;
	virtual bool close()=0;

	///////////////////////////////////////////////////////////////////////////
	// access interface
	virtual size_t size() const;
	virtual CPCStorage& operator[](size_t i);

	virtual bool searchStorage(uint32 accid, CPCStorage& stor);
	virtual bool removeStorage(uint32 accid);
	virtual bool saveStorage(const CPCStorage& stor);
};



///////////////////////////////////////////////////////////////////////////////
// Guild Storage Database Interface
///////////////////////////////////////////////////////////////////////////////
class CGuildStorageDB_mem : public CGuildStorageDBInterface
{
protected:
	virtual bool do_readGuildStorage()=0;
	virtual bool do_saveGuildStorage()=0;
	virtual void do_crateGuildStorage(const CGuildStorage& stor)	{}
	virtual void do_saveGuildStorage(const CGuildStorage& stor)		{}
	virtual void do_removeGuildStorage(const CGuildStorage& stor)	{}


	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CGuildStorageDB_mem(const char* configfile=NULL);
public:
	virtual ~CGuildStorageDB_mem();


protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::TslistDST<CGuildStorage> cGuildStorList;

	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile)=0;
	virtual bool close()=0;


	///////////////////////////////////////////////////////////////////////////
	// access interface
	virtual size_t size() const;
	virtual CGuildStorage& operator[](size_t i);

	virtual bool searchStorage(uint32 gid, CGuildStorage& stor);
	virtual bool removeStorage(uint32 gid);
	virtual bool saveStorage(const CGuildStorage& stor);
};


///////////////////////////////////////////////////////////////////////////////
// Pat Database Interface
///////////////////////////////////////////////////////////////////////////////
class CPetDB_mem : public CPetDBInterface
{
protected:
	virtual bool do_readPets()=0;
	virtual bool do_savePets()=0;
	virtual void do_createPet(const CPet& pet)	{}
	virtual void do_savePet(const CPet& pet)	{}
	virtual void do_removePet(const CPet& pet)	{}

	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CPetDB_mem(const char* configfile=NULL);
public:
	virtual ~CPetDB_mem();

protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::TslistDST<CPet> cPetList;

	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile)=0;
	virtual bool close()=0;

public:
	///////////////////////////////////////////////////////////////////////////
	// access interface
	virtual size_t size() const;
	virtual CPet& operator[](size_t i);

	virtual bool searchPet(uint32 pid, CPet& pet);
	virtual bool insertPet(uint32 accid, uint32 cid, short pet_class, short pet_lv, short pet_egg_id, ushort pet_equip, short intimate, short hungry, char renameflag, char incuvat, char *pet_name, CPet& pet);
	virtual bool removePet(uint32 pid);
	virtual bool savePet(const CPet& pet);
};











///////////////////////////////////////////////////////////////////////////////
// implementation of the in-memory databases 
// using read/write to textfiles
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
#if defined(WITH_TEXT)
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Account Database
// for storing account stuff in login
///////////////////////////////////////////////////////////////////////////////
class CAccountDB_txt : public basics::CTimerBase, public CAccountDB_mem
{
protected:

	///////////////////////////////////////////////////////////////////////////
	/// reads all accounts from source to memory.
	virtual bool do_readAccounts();
	///////////////////////////////////////////////////////////////////////////
	/// writes all accounts to source.
	virtual bool do_saveAccounts();
	///////////////////////////////////////////////////////////////////////////
	/// save a single account.
	/// does nothing by default, needs overloading
	virtual void do_saveAccount(const CLoginAccount& account)	{ ++this->savecount; }
	///////////////////////////////////////////////////////////////////////////
	/// create a single account.
	/// does nothing by default, needs overloading
	virtual void do_createAccount(const CLoginAccount& account)	{ ++this->savecount; }
	///////////////////////////////////////////////////////////////////////////
	/// delete a single account.
	/// does nothing by default, needs overloading
	virtual void do_removeAccount(const CLoginAccount& account)	{ ++this->savecount; }


	size_t savecount;
public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CAccountDB_txt(const char* configfile);
	virtual ~CAccountDB_txt();

protected:
	virtual bool init(const char* configfile);
	virtual bool close();

	///////////////////////////////////////////////////////////////////////////
	// timer function
	virtual bool timeruserfunc(unsigned long tick);
};


///////////////////////////////////////////////////////////////////////////////
// Chardb
///////////////////////////////////////////////////////////////////////////////
class CCharDB_txt : public basics::CTimerBase, public CCharDB_mem
{
protected:
	///////////////////////////////////////////////////////////////////////////
	// Function to create the character line (for save)
	int char_to_str(char *str, size_t sz, const CCharCharacter &p);

	///////////////////////////////////////////////////////////////////////////
	// Function to set the character from the line (at read of characters file)
	bool char_from_str(const char *str);

	///////////////////////////////////////////////////////////////////////////
	bool save_friends();
	///////////////////////////////////////////////////////////////////////////
	bool read_friends();

	///////////////////////////////////////////////////////////////////////////
	/// reads all chars from source to memory.
	virtual bool do_readChars();
	///////////////////////////////////////////////////////////////////////////
	/// saves all chars.
	virtual bool do_saveChars();
	///////////////////////////////////////////////////////////////////////////
	/// save a single char.
	/// does nothing by default, needs overloading
	virtual void do_saveChar(const CCharCharacter& data)	{ ++this->savecount; }
	///////////////////////////////////////////////////////////////////////////
	/// create a single char.
	/// does nothing by default, needs overloading
	virtual void do_createChar(const CCharCharacter& data)	{ ++this->savecount; }
	///////////////////////////////////////////////////////////////////////////
	/// delete a single char.
	/// does nothing by default, needs overloading
	virtual void do_removeChar(const CCharCharacter& data)	{ ++this->savecount; }

	basics::CParam< basics::string<> > char_txt;
	basics::CParam< basics::string<> > friends_txt;
	size_t savecount;
	basics::simple_database				cMailDB;
public:
	CCharDB_txt(const char *configfile=NULL);
	virtual ~CCharDB_txt();


	///////////////////////////////////////////////////////////////////////////
	size_t getMailCount(uint32 cid, uint32 &all, uint32 &unread);
	size_t listMail(uint32 cid, unsigned char box, unsigned char *buffer);
	bool readMail(uint32 cid, uint32 mid, CMail& mail);
	bool deleteMail(uint32 cid, uint32 mid);
	bool sendMail(uint32 senderid, const char* sendername, const char* targetname, const char *head, const char *body, uint32 zeny, const struct item& item, uint32& msgid, uint32& tid);

protected:
	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile);
	virtual bool close();

	///////////////////////////////////////////////////////////////////////////
	// timer function
	virtual bool timeruserfunc(unsigned long tick);
};




///////////////////////////////////////////////////////////////////////////////
// Guilddb
///////////////////////////////////////////////////////////////////////////////
class CGuildDB_txt : public basics::CTimerBase, public CGuildDB_mem
{
protected:
	bool string2guild(const char *str, CGuild &g);
	ssize_t guild2string(char *str, size_t maxlen, const CGuild &g);
	bool string2castle(char *str, CCastle &gc);
	ssize_t castle2string(char *str, size_t maxlen, const CCastle &gc);

	virtual bool do_readGuildsCastles();
	virtual bool do_saveGuildsCastles();
	virtual void do_createGuild(const CGuild& guild)	{ ++this->savecount; }
	virtual void do_saveGuild(const CGuild& guild)		{ ++this->savecount; }
	virtual void do_removeGuild(const CGuild& guild)	{ ++this->savecount; }

	virtual void do_saveCastle(const CCastle& castle)	{ ++this->savecount; }


public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CGuildDB_txt(const char *configfile);
	virtual ~CGuildDB_txt();
protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::CParam< basics::string<> > guild_filename;
	basics::CParam< basics::string<> > castle_filename;

	uint savecount;
	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile);
	virtual bool close();

	///////////////////////////////////////////////////////////////////////////
	// timer function
	virtual bool timeruserfunc(unsigned long tick);
};




///////////////////////////////////////////////////////////////////////////////
// Party Database
///////////////////////////////////////////////////////////////////////////////
class CPartyDB_txt : public basics::CTimerBase, public CPartyDB_mem
{
protected:
	ssize_t party_to_string(char *str, size_t maxlen, const CParty &p);
	bool party_from_string(const char *str, CParty &p);

	virtual bool do_readParties();
	virtual bool do_saveParties();
	virtual void do_createParty(const CParty& party)	{ ++this->savecount; }
	virtual void do_saveParty(const CParty& party)		{ ++this->savecount; }
	virtual void do_removeParty(const CParty& party)	{ ++this->savecount; }

public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CPartyDB_txt(const char *configfile);
	virtual ~CPartyDB_txt();
protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::CParam< basics::string<> >		party_filename;
	uint					savecount;
	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile);
	virtual bool close();

	///////////////////////////////////////////////////////////////////////////
	// timer function
	virtual bool timeruserfunc(unsigned long tick);
};




///////////////////////////////////////////////////////////////////////////////
// Storage Database Interface
///////////////////////////////////////////////////////////////////////////////
class CPCStorageDB_txt : public basics::CTimerBase, public CPCStorageDB_mem
{
protected:
	ssize_t storage_to_string(char *str, size_t maxlen, const CPCStorage &stor);
	int storage_from_string(const char *str, CPCStorage &stor);

	virtual bool do_readPCStorage();
	virtual bool do_savePCStorage();
	virtual void do_cratePCStorage(const CPCStorage& stor)	{ ++this->savecount; }
	virtual void do_savePCStorage(const CPCStorage& stor)	{ ++this->savecount; }
	virtual void do_removePCStorage(const CPCStorage& stor)	{ ++this->savecount; }

public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CPCStorageDB_txt(const char *dbcfgfile);
	virtual ~CPCStorageDB_txt();

protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::CParam< basics::string<> > pcstorage_filename;
	uint savecount;
	


	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile);
	virtual bool close();

	///////////////////////////////////////////////////////////////////////////
	// timer function
	virtual bool timeruserfunc(unsigned long tick);
};



///////////////////////////////////////////////////////////////////////////////
// Guild Storage Database Interface
///////////////////////////////////////////////////////////////////////////////
class CGuildStorageDB_txt : public basics::CTimerBase, public CGuildStorageDB_mem
{
protected:
	ssize_t guild_storage_to_string(char *str, size_t maxlen, const CGuildStorage &stor);
	bool guild_storage_from_string(const char *str, CGuildStorage &stor);

	virtual bool do_readGuildStorage();
	virtual bool do_saveGuildStorage();
	virtual void do_crateGuildStorage(const CGuildStorage& stor)	{ ++this->savecount; }
	virtual void do_saveGuildStorage(const CGuildStorage& stor)		{ ++this->savecount; }
	virtual void do_removeGuildStorage(const CGuildStorage& stor)	{ ++this->savecount; }

public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CGuildStorageDB_txt(const char *dbcfgfile);
	virtual ~CGuildStorageDB_txt();


protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::CParam< basics::string<> > guildstorage_filename;
	uint savecount;
	
	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile);
	virtual bool close();

	///////////////////////////////////////////////////////////////////////////
	// timer function
	virtual bool timeruserfunc(unsigned long tick);
};


///////////////////////////////////////////////////////////////////////////////
// Pat Database Interface
///////////////////////////////////////////////////////////////////////////////
class CPetDB_txt : public basics::CTimerBase, public CPetDB_mem
{
protected:
	int pet_to_string(char *str, size_t sz, CPet &pet);
	bool pet_from_string(const char *str, CPet &pet);

	virtual bool do_readPets();
	virtual bool do_savePets();
	virtual void do_createPet(const CPet& pet)	{ ++this->savecount; }
	virtual void do_savePet(const CPet& pet)	{ ++this->savecount; }
	virtual void do_removePet(const CPet& pet)	{ ++this->savecount; }

public:
	///////////////////////////////////////////////////////////////////////////
	// construct/destruct
	CPetDB_txt(const char *dbcfgfile);
	virtual ~CPetDB_txt();

protected:
	///////////////////////////////////////////////////////////////////////////
	// data
	basics::CParam< basics::string<> > pet_filename;
	uint savecount;

	///////////////////////////////////////////////////////////////////////////
	// normal function
	virtual bool init(const char* configfile);
	virtual bool close();

	///////////////////////////////////////////////////////////////////////////
	// timer function
	virtual bool timeruserfunc(unsigned long tick);
};




///////////////////////////////////////////////////////////////////////////////
#endif//WITH_TEXT
///////////////////////////////////////////////////////////////////////////////



#endif//__BASEMEM_H__
