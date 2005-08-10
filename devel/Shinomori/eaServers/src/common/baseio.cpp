#include "base.h"
#include "baseio.h"
#include "lock.h"



//////////////////////////////////////////////////////////////////////////
#ifdef TXT_ONLY
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
#else// SQL
//////////////////////////////////////////////////////////////////////////


#include <mysql.h>
///////////////////////////////////////////////////////////////////////////////
// mysql access function
///////////////////////////////////////////////////////////////////////////////

bool mysql_SendQuery(MYSQL &mysql, MYSQL_RES*& sql_res, const char* q, size_t sz=0)
{
#ifdef DEBUG_SQL
	ShowSQL("%s\n", q);
#endif
	if( 0==mysql_real_query(&mysql, q, (sz)?sz:strlen(q)) )
	{
		if(sql_res) mysql_free_result(sql_res);
		sql_res = mysql_store_result(&mysql);
		if(sql_res)
		{
			return true;
		}
		else
		{
			ShowError("DB result error\nQuery:    %s\n", q);
		}
	}
	else
	{
		ShowError("Database Error %s\nQuery:    %s\n", mysql_error(&mysql), q);
	}
	return false;
}
bool mysql_SendQuery(MYSQL &mysql, const char* q, size_t sz=0)
{
#ifdef DEBUG_SQL
	ShowSQL("%s\n", q);
#endif
	if( 0==mysql_real_query(&mysql, q, (sz)?sz:strlen(q)) )
	{
		return true;
	}
	else
	{
		ShowError("Database Error %s\nQuery:    %s\n", mysql_error(&mysql), q);
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
#endif// SQL
//////////////////////////////////////////////////////////////////////////








//////////////////////////////////////////////////////////////////////////
// Account Database
// for storing accounts stuff in login
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
#ifdef TXT_ONLY
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// global data for txt
unsigned long next_account_id=200000;//START_ACCOUNT_NUM;

static char account_filename[1024] = "save/account.txt";
static char GM_account_filename[1024] = "conf/GM_account.txt";
static time_t creation_time_GM_account_file=0;

//////////////////////////////////////////////////////////////////////////
// helper class for gm_level reading
class CMapGM
{
public:
	unsigned long account_id;
	unsigned char gm_level;

	CMapGM(unsigned long accid=0) : account_id(accid), gm_level(0)	{}
	CMapGM(unsigned long accid, unsigned char lv) : account_id(accid), gm_level(lv)	{}

	bool operator==(const CMapGM& c) const { return this->account_id==c.account_id; }
	bool operator!=(const CMapGM& c) const { return this->account_id!=c.account_id; }
	bool operator> (const CMapGM& c) const { return this->account_id> c.account_id; }
	bool operator>=(const CMapGM& c) const { return this->account_id>=c.account_id; }
	bool operator< (const CMapGM& c) const { return this->account_id< c.account_id; }
	bool operator<=(const CMapGM& c) const { return this->account_id<=c.account_id; }
};
//////////////////////////////////////////////////////////////////////////
// read gm levels to a list
bool read_gmaccount(TslistDST<CMapGM> &gmlist)
{
	struct stat file_stat;
	FILE *fp;

	// clear all gm_levels
	gmlist.resize(0);

	// get last modify time/date
	creation_time_GM_account_file = (0==stat(GM_account_filename, &file_stat))? 0 : file_stat.st_mtime;

	fp = safefopen(GM_account_filename, "r");
	if( fp )
	{
		char line[1024];
		int level;
		unsigned long account_id=0;
		size_t line_counter=0;
		unsigned long start_range = 0, end_range = 0, is_range = 0, current_id = 0;

		while( fgets(line, sizeof(line), fp) )
		{
			line_counter++;
			if( !skip_empty_line(line) )
				continue;
			is_range = (sscanf(line, "%ld%*[-~]%ld %d",&start_range,&end_range,&level)==3); // ID Range [MC Cameri]
			if (!is_range && sscanf(line, "%ld %d", &account_id, &level) != 2 && sscanf(line, "%ld: %d", &account_id, &level) != 2)
				ShowError("read_gm_account: file [%s], invalid 'acount_id|range level' format (line #%d).\n", GM_account_filename, line_counter);
			else if (level <= 0)
				ShowError("read_gm_account: file [%s] %dth account (line #%d) (invalid level [0 or negative]: %d).\n", GM_account_filename, gmlist.size()+1, line_counter, level);
			else
			{
				if (level > 99)
				{
					ShowWarning("read_gm_account: file [%s] %dth account (invalid level, but corrected: %d->99).\n", GM_account_filename, gmlist.size()+1, level);
					level = 99;
				}
				if (is_range)
				{
					if (start_range==end_range)
						ShowError("read_gm_account: file [%s] invalid range, beginning of range is equal to end of range (line #%d).\n", GM_account_filename, line_counter);
					else if (start_range>end_range)
						ShowError("read_gm_account: file [%s] invalid range, beginning of range must be lower than end of range (line #%d).\n", GM_account_filename, line_counter);
					else
						for (current_id = start_range;current_id<=end_range;current_id++)
						{
							gmlist.insert( CMapGM(current_id, level) );
						}
				}
				else
				{
					gmlist.insert( CMapGM(account_id, level) );
				}
			}
		}
		fclose(fp);
		ShowStatus("File '%s' read (%d GM accounts found).\n", GM_account_filename, gmlist.size());
		return true;
	}
	else
	{
		ShowError("read_gm_account: GM accounts file [%s] not found.\n", GM_account_filename);
		return false;
	}

}
//////////////////////////////////////////////////////////////////////////
// read accounts from file
bool read_accounts(CAccountDB &accdb)
{
	FILE *fp;
	if ((fp = safefopen(account_filename, "r")) == NULL)
	{
		// no account file -> no account -> no login, including char-server (ERROR)
		ShowError(CL_BT_RED"mmo_auth_init: Accounts file [%s] not found.\n"CL_RESET, account_filename);
		return false;
	}
	else
	{
		size_t pos;
		TslistDST<unsigned long> acclist;
		CLoginAccount data;
		unsigned long account_id;
		int logincount, state, n, i, j, v;
		char line[2048], *p, userid[2048], pass[2048], lastlogin[2048], sex, email[2048], error_message[2048], last_ip[2048], memo[2048];
		unsigned long ban_until_time;
		unsigned long connect_until_time;
		char str[2048];
		int GM_count = 0;
		int server_count = 0;

		///////////////////////////////////
		TslistDST<CMapGM> gmlist;
		read_gmaccount(gmlist);


		while( fgets(line, sizeof(line), fp) )
		{
			if( !skip_empty_line(line) )
				continue;

			// database version reading (v2)
			if( 13 == (i=sscanf(line, "%ld\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t%[^\t]\t%[^\t]\t%ld\t%[^\t]\t%[^\t]\t%ld%n",
							&account_id, userid, pass, lastlogin, &sex, &logincount, &state, email, error_message, &connect_until_time, last_ip, memo, &ban_until_time, &n)) && (line[n] == '\t') )
			{	// with ban_time
				;
			}
			else if( 12 == (i=sscanf(line, "%ld\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t%[^\t]\t%[^\t]\t%ld\t%[^\t]\t%[^\t]%n",
							&account_id, userid, pass, lastlogin, &sex, &logincount, &state, email, error_message, &connect_until_time, last_ip, memo, &n)) && (line[n] == '\t') )
			{	// without ban_time
				ban_until_time=0;
			}
			// Old athena database version reading (v1)
			else if( 5 <= (i=sscanf(line, "%ld\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t%n",
							&account_id, userid, pass, lastlogin, &sex, &logincount, &state, &n)) )
			{
				*email=0;
				*last_ip=0;
				ban_until_time=0;
				connect_until_time=0;
				if (i < 6)
					logincount=0;
			}
			else if(sscanf(line, "%ld\t%%newid%%\n%n", &account_id, &i) == 1 && i > 0 && account_id > next_account_id)
			{
				next_account_id = account_id;
				continue;
			}
			
			/////////////////////////////////////
			// Some checks
			if (account_id > 10000000) //!!END_ACCOUNT_NUM
			{
				ShowError(CL_BT_RED"mmo_auth_init: ******Error: an account has an id higher than %d\n", 10000000);//!!
				ShowMessage("               account id #%d -> account not read (saved in log file).\n"CL_RESET, account_id);
				continue;
			}
			userid[23] = '\0';
			remove_control_chars(userid);

			if( acclist.find(account_id,0,pos) )
			{
				ShowWarning(CL_BT_RED"mmo_auth_init: ******Error: an account has an identical id to another.\n"CL_RESET);
				ShowMessage("               account id #%d -> not read (saved in log file).\nCL_RESET", account_id);

				//!! log account line to file as promised
			}
			else if( accdb.searchAccount(userid) )
			{
				ShowError(CL_BT_RED"mmo_auth_init: ******Error: account name already exists.\n"CL_RESET);
				ShowMessage("               account name '%s' -> new account not read.\n", userid); // 2 lines, account name can be long.
				ShowMessage("               Account saved in log file.\n");

				//!! log account line to file as promised
			}
			else
			{
				CLoginAccount temp;
				
				temp.account_id = account_id;
				safestrcpy(temp.userid, userid, sizeof(temp.userid));

				pass[23] = '\0';
				remove_control_chars(pass);
				safestrcpy(temp.passwd, pass, sizeof(temp.passwd));
				temp.sex = (sex == 'S' || sex == 's') ? 2 : (sex == 'M' || sex == 'm');
				if (temp.sex == 2)
					server_count++;

				remove_control_chars(email);
				if( !email_check(email) )
				{
					ShowWarning("Account %s (%d): invalid e-mail (replaced with a@a.com).\n", userid, account_id);
					safestrcpy(temp.email, "a@a.com", sizeof(temp.email));
				}
				else
					safestrcpy(temp.email, email, sizeof(temp.email));

				if( gmlist.find( CMapGM(account_id),0,pos) )
				{
					temp.gm_level = gmlist[pos].gm_level;
					GM_count++;
				}
				else
					temp.gm_level=0;

				temp.login_count = (logincount>0)?logincount:0;

				lastlogin[23] = '\0';
				remove_control_chars(lastlogin);
				safestrcpy(temp.last_login, lastlogin, sizeof(temp.last_login));

				temp.ban_until = (i==13) ? ban_until_time : 0;
				temp.valid_until = connect_until_time;

				last_ip[15] = '\0';
				remove_control_chars(last_ip);
				temp.client_ip = ipaddress(last_ip);


				p = line;
				for(j = 0; j < ACCOUNT_REG2_NUM; j++)
				{
					p += n;
					if (sscanf(p, "%[^\t,],%d %n", str, &v, &n) != 2) {
						// We must check if a str is void. If it's, we can continue to read other REG2.
						// Account line will have something like: str2,9 ,9 str3,1 (here, ,9 is not good)
						if (p[0] == ',' && sscanf(p, ",%d %n", &v, &n) == 1) {
							j--;
							continue;
						} else
							break;
					}
					str[31] = '\0';
					remove_control_chars(str);
					safestrcpy(temp.account_reg2[j].str, str, sizeof(temp.account_reg2[0].str));
					temp.account_reg2[j].value = v;
				}
				temp.account_reg2_num = j;
				
				if (next_account_id <= account_id)
					next_account_id = account_id + 1;
			
				accdb.insert(temp);
			}
		}
		fclose(fp);

		if( accdb.size() == 0 )
		{
			ShowError("mmo_auth_init: No account found in %s.\n", account_filename);
			sprintf(line, "No account found in %s.", account_filename);
		}
		else
		{
			if( accdb.size() == 1)
			{
				ShowStatus("mmo_auth_init: 1 account read in %s,\n", account_filename);
				sprintf(line, "1 account read in %s,", account_filename);
			}
			else
			{
				ShowStatus("mmo_auth_init: %d accounts read in %s,\n", accdb.size(), account_filename);
				sprintf(line, "%d accounts read in %s,", accdb.size(), account_filename);
			}
			if (GM_count == 0)
			{
				ShowMessage("               of which is no GM account, and ");
				sprintf(str, "%s of which is no GM account and", line);
			}
			else if (GM_count == 1)
			{
				ShowMessage("               of which is 1 GM account, and ");
				sprintf(str, "%s of which is 1 GM account and", line);
			}
			else
			{
				ShowMessage("               of which is %d GM accounts, and ", GM_count);
				sprintf(str, "%s of which is %d GM accounts and", line, GM_count);
			}
			if (server_count == 0)
			{
				ShowMessage("no server account ('S').\n");
				sprintf(line, "%s no server account ('S').", str);
			}
			else if (server_count == 1)
			{
				ShowMessage("1 server account ('S').\n");
				sprintf(line, "%s 1 server account ('S').", str);
			}
			else
			{
				ShowMessage("%d server accounts ('S').\n", server_count);
				sprintf(line, "%s %d server accounts ('S').", str, server_count);
			}
		}
//		login_log("%s" RETCODE, line);
		return true;
	}
}
//////////////////////////////////////////////////////////////////////////
// write accounts
bool write_accounts(CAccountDB &accdb)
{
	FILE *fp;
	size_t i, k;
	int lock;

	// Data save
	if ((fp = lock_fopen(account_filename, &lock)) == NULL) {
		return false;
	}
	fprintf(fp, "// Accounts file: here are saved all information about the accounts.\n");
	fprintf(fp, "// Structure: ID, account name, password, last login time, sex, # of logins, state, email, error message for state 7, validity time, last (accepted) login ip, memo field, ban timestamp, repeated(register text, register value)\n");
	fprintf(fp, "// Some explanations:\n");
	fprintf(fp, "//   account name    : between 4 to 23 char for a normal account (standard client can't send less than 4 char).\n");
	fprintf(fp, "//   account password: between 4 to 23 char\n");
	fprintf(fp, "//   sex             : M or F for normal accounts, S for server accounts\n");
	fprintf(fp, "//   state           : 0: account is ok, 1 to 256: error code of packet 0x006a + 1\n");
	fprintf(fp, "//   email           : between 3 to 39 char (a@a.com is like no email)\n");
	fprintf(fp, "//   error message   : text for the state 7: 'Your are Prohibited to login until <text>'. Max 19 char\n");
	fprintf(fp, "//   valitidy time   : 0: unlimited account, <other value>: date calculated by addition of 1/1/1970 + value (number of seconds since the 1/1/1970)\n");
	fprintf(fp, "//   memo field      : max 254 char\n");
	fprintf(fp, "//   ban time        : 0: no ban, <other value>: banned until the date: date calculated by addition of 1/1/1970 + value (number of seconds since the 1/1/1970)\n");
	for(i = 0; i < accdb.size(); i++)
	{
		fprintf(fp, "%ld\t%s\t%s\t%s\t%c\t%ld\t%ld\t"
					"%s\t%s\t%ld\t%s\t%s\t%ld\t",
					accdb[i].account_id, accdb[i].userid, accdb[i].passwd, accdb[i].last_login,
					(accdb[i].sex == 2) ? 'S' : (accdb[i].sex ? 'M' : 'F'),
					accdb[i].login_count, 0 /*accdb[i].state*/,
					accdb[i].email, ""/*accdb[i].error_message*/, (unsigned long)accdb[i].valid_until,
					accdb[i].last_ip, ""/*accdb[i].memo*/, (unsigned long)accdb[i].ban_until);
		for(k = 0; k< accdb[i].account_reg2_num; k++)
			if(accdb[i].account_reg2[k].str[0])
				fprintf(fp, "%s,%ld ", accdb[i].account_reg2[k].str, accdb[i].account_reg2[k].value);
		fprintf(fp, RETCODE);
	}
	fprintf(fp, "%ld\t%%newid%%"RETCODE, next_account_id);
	lock_fclose(fp, account_filename, &lock);
	return true;
}
//////////////////////////////////////////////////////////////////////////
// class implementation
bool CAccountDB::ProcessConfig(const char*w1, const char*w2)
{
	if(w1 && w2)
	{
		if( 0==strcasecmp(w1, "account_filename") )
			safestrcpy(account_filename, w2, sizeof(account_filename));
		else if( 0==strcasecmp(w1, "GM_account_filename") )
			safestrcpy(GM_account_filename, w2, sizeof(GM_account_filename));
	}
	return true;
}

bool CAccountDB::existAccount(const char* userid)
{	// check if account with userid already exist
	size_t pos;
	return cList.find( CLoginAccount(userid), pos, 1);
}
CLoginAccount* CAccountDB::searchAccount(const char* userid)
{	// get account by userid
	size_t pos;
	if( cList.find( CLoginAccount(userid), pos, 1) )
		return &(cList(pos,1));
	return NULL;
}
CLoginAccount* CAccountDB::searchAccount(unsigned long accid)
{	// get account by account_id
	size_t pos;
	if( cList.find( CLoginAccount(accid), pos, 0) )
		return &(cList(pos,0));
	return NULL;
}

CLoginAccount* CAccountDB::insertAccount(const char* userid, const char* passwd, unsigned char sex, const char* email)
{	// insert a new account
	CLoginAccount temp;
	size_t pos;
	unsigned long accid = next_account_id++;
	if( cList.find( CLoginAccount(userid), pos, 1) )
	{	// remove an existing account
		cList.removeindex(pos, 1);
	}

	cList.insert( CLoginAccount(accid, userid, passwd, sex, email) );

	if( cList.find( CLoginAccount(userid), pos, 1) )
		return &(cList(pos,1));
	return NULL;
}
bool CAccountDB::deleteAccount(CLoginAccount* account)
{
	if(account)
	{
		size_t pos;
		if( cList.find(*account,pos, 0) )
		{
			return cList.removeindex(pos, 0);
		}
	}
	return false;
}

bool CAccountDB::init(const char* configfile)
{	// init db
	CConfig::LoadConfig(configfile);
	//return 
		read_accounts(*this);

	size_t i;

	printf("--------------\n");
	for(i=0; i<cList.size(); i++)
		printf("%i %s\n", cList(i,0).account_id, cList(i,0).userid);
	printf("--------------\n");
	for(i=0; i<cList.size(); i++)
		printf("%i %s\n", cList(i,1).account_id, cList(i,1).userid);
	printf("--------------\n");

	return true;
}
bool CAccountDB::close()
{
	return saveAccount();
}
bool CAccountDB::saveAccount(CLoginAccount* account)
{	// update the account in db
	// nothing to be done here
	// accounts are saved by externally called maintainance loop
	return true;
}
bool CAccountDB::saveAccount()
{	// saving all accounts
	return write_accounts(*this);
}

//////////////////////////////////////////////////////////////////////////
#else// SQL
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// global data for sql
static MYSQL mysqldb_handle;

// db access
static char mysqldb_ip[32]="127.0.0.1";
static unsigned short mysqldb_port=3306;
static char mysqldb_id[32] = "ragnarok";
static char mysqldb_pw[32] = "ragnarok";

// table names
static char login_db[128] = "login";
static char log_db[128]   = "loginlog";

// field names
static char login_db_userid[128]     = "userid";
static char login_db_account_id[128] = "account_id";
static char login_db_user_pass[128]  = "user_pass";
static char login_db_level[128]      = "level";

// options
static bool case_sensitive=true;
static bool log_login=false;


bool CAccountDB::ProcessConfig(const char*w1, const char*w2)
{
	if(w1 && w2)
	{
		if (strcasecmp(w1, "login_db") == 0) {
			safestrcpy(login_db, w2,sizeof(login_db));
		}
		//add for DB connection
		else if(strcasecmp(w1,"login_server_ip")==0){
			safestrcpy(mysqldb_ip, w2, sizeof(mysqldb_ip));
			ShowMessage ("set login_server_ip : %s\n",w2);
		}
		else if(strcasecmp(w1,"login_server_port")==0){
			mysqldb_port=atoi(w2);
			ShowMessage ("set login_server_port : %s\n",w2);
		}
		else if(strcasecmp(w1,"login_server_id")==0){
			safestrcpy(mysqldb_id, w2,sizeof(mysqldb_id));
			ShowMessage ("set login_server_id : %s\n",w2);
		}
		else if(strcasecmp(w1,"login_server_pw")==0){
			safestrcpy(mysqldb_pw, w2,sizeof(mysqldb_pw));
			ShowMessage ("set login_server_pw : %s\n",w2);
		}
		else if(strcasecmp(w1,"login_server_db")==0){
			safestrcpy(login_db, w2,sizeof(login_db));
			ShowMessage ("set login_server_db : %s\n",w2);
		}
		//added for custom column names for custom login table
		else if(strcasecmp(w1,"login_db_account_id")==0){
			safestrcpy(login_db_account_id, w2,sizeof(login_db_account_id));
		}
		else if(strcasecmp(w1,"login_db_userid")==0){
			safestrcpy(login_db_userid, w2,sizeof(login_db_userid));
		}
		else if(strcasecmp(w1,"login_db_user_pass")==0){
			safestrcpy(login_db_user_pass, w2,sizeof(login_db_user_pass));
		}
		else if(strcasecmp(w1,"login_db_level")==0){
			safestrcpy(login_db_level, w2, sizeof(login_db_level));
		}
		//end of custom table config
		else if (strcasecmp(w1, "loginlog_db") == 0) {
			safestrcpy(log_db, w2,sizeof(log_db));
		}
		else if (strcasecmp(w1, "log_login") == 0) {
			log_login = Switch(w2);
		}
	}
	return true;
}

bool CAccountDB::existAccount(const char* userid)
{	// check if account with userid already exist
	bool ret = false;
	char uid[64];
	char query[1024];
	MYSQL_RES* sql_res=NULL;

	jstrescapecpy(uid, userid);
	size_t sz=sprintf(query, "SELECT `%s` FROM `%s` WHERE `userid` = '%s'", login_db_userid, login_db, uid);
	if( mysql_SendQuery(mysqldb_handle, sql_res, query, sz) )
	{
		ret = (mysql_num_rows(sql_res) == 1);
		mysql_free_result(sql_res);
	}
	return ret;
}
CLoginAccount* CAccountDB::searchAccount(const char* userid)
{	// get account by user/pass
	size_t sz;
	char query[4096];
	char uid[64];
	MYSQL_RES *sql_res1=NULL, *sql_res2=NULL;

	jstrescapecpy(uid, userid);
	sz=sprintf(query, "SELECT `%s`,`%s`,`%s`,`lastlogin`,`logincount`,`sex`,`connect_until`,`last_ip`,`ban_until`,`state`,`%s`,`email`"
	                " FROM `%s` WHERE %s `%s`='%s'", login_db_account_id, login_db_userid, login_db_user_pass, login_db_level, login_db, case_sensitive ? "BINARY" : "", login_db_userid, uid);
	//login {0-account_id/1-userid/2-user_pass/3-lastlogin/4-logincount/5-sex/6-connect_untl/7-last_ip/8-ban_until/9-state/10-gmlevel/11-email}
	if( mysql_SendQuery(mysqldb_handle, sql_res1, query, sz) )
	{
		MYSQL_ROW sql_row = mysql_fetch_row(sql_res1);	//row fetching
		if(sql_row)
		{	// use cList[0] as general storage, the list contains only this element

			cList[0].account_id = sql_row[0]?atol(sql_row[0]):0;
			safestrcpy(cList[0].userid, userid, 24);
			safestrcpy(cList[0].passwd, sql_row[2]?sql_row[2]:"", 32);
			safestrcpy(cList[0].last_login, sql_row[3]?sql_row[3]:"" , 24);
			cList[0].login_count = sql_row[4]?atol( sql_row[4]):0;
			cList[0].sex = sql_row[5][0] == 'S' ? 2 : sql_row[5][0]=='M';
			cList[0].valid_until = (time_t)(sql_row[6]?atol(sql_row[6]):0);
			safestrcpy(cList[0].last_ip, sql_row[7], 16);
			cList[0].ban_until = (time_t)(sql_row[8]?atol(sql_row[8]):0);;
//			cList[0].state = sql_row[9]?atol(sql_row[9]):0;;
			cList[0].gm_level = sql_row[10]?atoi( sql_row[10]):0;
			safestrcpy(cList[0].email, sql_row[11]?sql_row[11]:"" , 40);


			sz = sprintf(query, "SELECT `str`,`value` FROM `global_reg_value` WHERE `type`='1' AND `account_id`='%ld'", cList[0].account_id);
			if( mysql_SendQuery(mysqldb_handle, sql_res2, query, sz) )
			{
				size_t i=0;
				while( i<ACCOUNT_REG2_NUM && (sql_row = mysql_fetch_row(sql_res2)) )
				{
					safestrcpy(cList[0].account_reg2[i].str, sql_row[0], sizeof(cList[0].account_reg2[0].str));
					cList[0].account_reg2[i].value = (sql_row[1]) ? atoi(sql_row[1]):0;
				}
				cList[0].account_reg2_num = i;

				mysql_free_result(sql_res2);
			}
		}
		mysql_free_result(sql_res1);
	}
	return NULL;
}
CLoginAccount* CAccountDB::searchAccount(unsigned long accid)
{	// get account by account_id
	return NULL;
}

CLoginAccount* CAccountDB::insertAccount(const char* userid, const char* passwd, unsigned char sex, const char* email)
{	// insert a new account to db
	size_t sz;
	char uid[64], pwd[64];
	char query[1024];

	mysql_real_escape_string(&mysqldb_handle, uid, userid, strlen(userid));
	mysql_real_escape_string(&mysqldb_handle, pwd, passwd, strlen(passwd));
	sz = sprintf(query, "INSERT INTO `%s` (`%s`, `%s`, `sex`, `email`) VALUES ('%s', '%s', '%c', '%s')", login_db, login_db_userid, login_db_user_pass, uid, pwd, sex, email);
	if( mysql_SendQuery(mysqldb_handle, query, sz) )
	{
		// read the complete account back from db
		return searchAccount(userid);
	}
	return NULL;
}
bool CAccountDB::deleteAccount(CLoginAccount* account)
{
	if(account)
	{
		size_t sz;
		char query[1024];

		sz=sprintf(query,"DELETE FROM `%s` WHERE `account_id`='%d';",login_db, account->account_id);
		mysql_SendQuery(mysqldb_handle, query, sz);

		sz=sprintf(query,"DELETE FROM `global_reg_value` WHERE `account_id`='%d';",account->account_id);
		mysql_SendQuery(mysqldb_handle, query, sz);
	}
	return true;
}

bool CAccountDB::init(const char* configfile)
{	// init db
	CConfig::LoadConfig(configfile);
	// create a default element for the access
	//cList.resize(1);
	cList.insert( CLoginAccount() );

	mysql_init(&mysqldb_handle);
	// DB connection start
	ShowMessage("Connect Login Database Server....\n");
	if( mysql_real_connect(&mysqldb_handle, mysqldb_ip, mysqldb_id, mysqldb_pw, login_db, mysqldb_port, (char *)NULL, 0) )
	{
		ShowMessage("connect success!\n");
		if (log_login)
		{	
			char query[512];
			size_t sz = sprintf(query, "INSERT DELAYED INTO `%s`(`time`,`ip`,`user`,`rcode`,`log`) VALUES (NOW(), '', 'lserver', '100','login server started')", log_db);
			//query
			mysql_SendQuery(mysqldb_handle, query, sz);
		}
		return true;
	}
	else
	{	// pointer check
		ShowMessage("%s\n", mysql_error(&mysqldb_handle));
	}



	return false;
}
bool CAccountDB::close()
{
	size_t sz;
	char query[512];
	//set log.
	if (log_login)
	{
		sz = sprintf(query,"INSERT DELAYED INTO `%s`(`time`,`ip`,`user`,`rcode`,`log`) VALUES (NOW(), '', 'lserver','100', 'login server shutdown')", log_db);
		//query
		mysql_SendQuery(mysqldb_handle, query, sz);
	}

	//delete all server status
	sz = sprintf(query,"DELETE FROM `sstatus`");
	//query
	mysql_SendQuery(mysqldb_handle, query, sz);

	mysql_close(&mysqldb_handle);

	return true;
}

bool CAccountDB::saveAccount(CLoginAccount* account)
{	//!! update the account in db
	bool ret = false;
	if(account)
	{	
		size_t sz;
		char query[1024], tempstr[64];

		sz = sprintf(query, "UPDATE `%s` SET "
			"`%s` = '%d', "
			"`logincount` = '%ld', "
			"`sex` = '%c', "
			"`last_ip` = '%s', "
			"`connect_until` = '%ld', "
			"`ban_until` = '%ld', "
			"`email` = '%s', "
			"WHERE `%s` = '%ld'", 
			login_db, 
			login_db_level, account->gm_level,
			account->login_count,
			(account->sex==1)? 'M':'F',
			account->last_ip,
			(unsigned long)account->valid_until,
			(unsigned long)account->ban_until,
			account->email,
			login_db_account_id, account->account_id);
		ret = mysql_SendQuery(mysqldb_handle, query, sz);

		sz=sprintf(query,"DELETE FROM `global_reg_value` WHERE `type`='1' AND `account_id`='%d';",account->account_id);
		if( mysql_SendQuery(mysqldb_handle, query, sz) )
		{	
			size_t i;
			for(i=0; i<account->account_reg2_num; i++)
			{
				jstrescapecpy(tempstr,account->account_reg2[i].str);
				sz=sprintf(query,"INSERT INTO `global_reg_value` (`type`, `account_id`, `str`, `value`) VALUES ( 1 , '%d' , '%s' , '%d');",  account->account_id, tempstr, account->account_reg2[i].value);
				ret &= mysql_SendQuery(mysqldb_handle, query, sz);
			}
		}
	}
	return ret;
}
bool CAccountDB::saveAccount()
{	// saving all accounts
	// nothing to be done here
	// accounts are saved individually
	return true;
}

///////////////////////////////////////////////////////////////////////////////
#endif// SQL
///////////////////////////////////////////////////////////////////////////////






































/////////////////////////////////////////////////////////////////////
// a text based database with extra index
// not much tested though and quite basic
// but should be as fast as file access allows 
// faster than sql anyway + threadsafe (not shareable though)
/////////////////////////////////////////////////////////////////////
#define DB_OPCOUNTMAX 100	// # of db ops before forced cache flush

class txt_database : private Mutex
{
	/////////////////////////////////////////////////////////////////
	// index structure
	class _key
	{
	public:
		ulong cKey;	// key value
		ulong cPos;	// position in file
		ulong cLen;	// data length

		_key(ulong k=0, ulong p=0, ulong l=0):cKey(k), cPos(p), cLen(l)	{}
		bool operator==(const _key& k) const	{ return cKey==k.cKey; }
		bool operator> (const _key& k) const	{ return cKey> k.cKey; }
		bool operator< (const _key& k) const	{ return cKey< k.cKey; }
	};
	/////////////////////////////////////////////////////////////////
	// class data
	char *cName;			// path/name of db file
	FILE *cDB;				// file handle to db
	FILE *cIX;				// file handle to index

	size_t cOpCount;		// count of operations

	TslistDST<_key>	cIndex;	// the index

	/////////////////////////////////////////////////////////////////
public:
	/////////////////////////////////////////////////////////////////
	// construction/destruction
	txt_database() : cName(NULL),cDB(NULL), cIX(NULL), cOpCount(0)
	{}
	~txt_database()
	{
		close();
	}

	/////////////////////////////////////////////////////////////////
	// open the database, create when not exist
	// extension is always ".txt" for db and ".inx" for index
	bool open(const char *name)
	{
		ScopeLock sl(*this);
		char *ip;
		
		if(NULL==name)
			return false;

		close();

		// with some extra space for file extension
		cName = new char[5+strlen(name)]; 
		// copy and correct the path seperators
		checkpath(cName, name);

		ip = strrchr(cName, '.');		// find a point in the name
		if(!ip || strchr(ip, PATHSEP) )	// if name had no point					
			ip = cName+strlen(cName);	// take all as name

		strcpy(ip, ".txt");
		cDB = fopen(cName, "rb+");
		if(!cDB)
		{	// either not exist or locked
			cDB = fopen(cName, "wb+");
			if(!cDB)	// locked
				return false;
		}

		strcpy(ip, ".inx");
		cIX = fopen(cName, "rb+");
		if(!cIX)
		{	// either not exist or locked
			cIX = fopen(cName, "wb+");
			if(!cIX)	// locked
			{
				fclose(cIX);
				return false;
			}
		}
		// cut the file name back to default
		*ip=0;

		// read index
		// structure is:
		// <# of entries> \n <i>(0), <p>(0), <l>(0) \n ...
		fseek(cIX, 0, SEEK_SET);
		unsigned long sz, p, l;
		if( 1==fscanf(cIX,"%li\n", &sz) )
		{
			cIndex.realloc(sz);
			while( 3==fscanf(cIX,"%li,%li,%li\n",  &sz, &p, &l) )
			{
				cIndex.insert( _key(sz,p,l) );
			}
		}

		// rebuild the database at startup might be not that expensive
		rebuild();

		return true;
	}
	/////////////////////////////////////////////////////////////////
	// closes the database
	bool close()
	{
		ScopeLock sl(*this);
		flush();
		// rebuild at close either, but might need to test
		rebuild();

		if(cDB) 
		{
			fclose(cDB);
			cDB = NULL;
		}
		if(cIX) 
		{
			fclose(cIX);
			cIX = NULL;
		}
		if(cName)
		{
			delete[] cName;
			cName = NULL;
		}
		return true;
	}
	/////////////////////////////////////////////////////////////////
	// flushes cache to files
	bool flush(bool force=false)
	{	
		ScopeLock sl(*this);
		bool ret = true;
		// check if necessary
		cOpCount++;
		if( force || cOpCount > DB_OPCOUNTMAX )
		{	// need to flush
			if(cDB) 
			{
				// nothing to flush here right now
			}
			if(cIX) 
			{
				// write index
				fseek(cIX, 0, SEEK_SET);
				fprintf(cIX,"%li\n", cIndex.size());
				for(size_t i=0; i<cIndex.size(); i++)
				{
					fprintf(cIX,"%li,%li,%li\n", 
						cIndex[i].cKey, cIndex[i].cPos, cIndex[i].cLen);
				}
				fflush(cIX);
			}
			// reset op counter
			cOpCount=0;
		}
		return ret;
	}
	/////////////////////////////////////////////////////////////////
	// insert/udate
	bool insert(const ulong key, char* data)
	{
		if(!data)
			return false;

		ScopeLock sl(*this);
		ulong len = strlen(data);
		size_t i;
		if( cIndex.find( _key(key), 0, i) )
		{	// update
			if( cIndex[i].cLen >= len )
			{	// rewrite the old position
				fseek(cDB, cIndex[i].cPos, SEEK_SET);
			}
			else
			{	// insert new at the end
				fseek(cDB, 0, SEEK_END);
			}
			cIndex[i].cLen = len;
			cIndex[i].cPos = ftell(cDB);
		}
		else
		{	// insert new at the end
			fseek(cDB, 0, SEEK_END);
			cIndex.insert( _key(key, ftell(cDB), len) );
		}
		fwrite(data, 1, len, cDB);

		this->flush();
		return true;
	}
	/////////////////////////////////////////////////////////////////
	// delete
	bool remove(const ulong key)
	{
		ScopeLock sl(*this);
		size_t pos;
		if( cIndex.find( _key(key), 0, pos) )
		{
			cIndex.removeindex(pos);
			this->flush();
			return true;
		}
		return false;
	}

	/////////////////////////////////////////////////////////////////
	// search
	bool find(const ulong key, char* data) const
	{
		if(!data)
			return false;

		ScopeLock sl(*this);
		size_t i;
		if( cIndex.find( _key(key), 0, i) )
		{
			fseek(cDB, cIndex[i].cPos, SEEK_SET);
			fread(data, cIndex[i].cLen, 1, cDB);
			data[cIndex[i].cLen]=0;
			return true;
		}
		return false;
	}

	/////////////////////////////////////////////////////////////////
	// rebuild database and index
	bool rebuild()
	{
		if(!cName || !cDB || !cIX)
			return false;

		ScopeLock sl(*this);
		char buffer[1024];		// the fixed size here might be a problem
		TslistDST<_key>	inx;	// new index
		ulong k, p, l;

		char *ip = cName+strlen(cName);
		strcpy(ip, ".tmp");
		
		FILE* tmp=fopen(cName, "wb");
		if(!tmp)
			return false;

		for(size_t i=0; i<cIndex.size(); i++)
		{	
			k=cIndex[i].cKey;
			p=ftell(tmp);
			l=cIndex[i].cLen;

			fseek(cDB,cIndex[i].cPos, SEEK_SET);
			fread (buffer, l,1,cDB);

			fwrite(buffer, l,1,tmp);

			inx.insert( _key(k,p,l) );
		}
		fclose(tmp);
		fclose(cDB);

		// swap databases
		// need a new buffer for renaming
		char *name = new char[1+strlen(cName)];
		memcpy(name, cName,1+strlen(cName));
		strcpy(ip, ".txt");
		::remove( cName );
		::rename( name, cName );
		delete[] name;

		// reopen the database file
		cDB = fopen(cName, "rb+");
		if(!cDB)
		{
			cDB = fopen(cName, "wb+");
			if(!cDB)
			{
				close();
				return false;
			}
		}
		// replace the index
		cIndex = inx;

		// cut the file name back to default
		*ip=0;
		return true;
	}
};




















//////////////////////////////////////////////////////////////////////////
#ifdef TXT_ONLY
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
#else// SQL
//////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
// defaults for a sql database
//////////////////////////////////////////////////////////////////////////
class DBDefaults : public CConfig
{
protected:
	const char *sql_conf_name;
public:
	// server
	int	 server_port;
	char server_ip[32];
	char server_id[32];
	char server_pw[32];
	char server_db[32];

	// tables
	char table_login[256];
	char table_loginlog[256];

	// columns
	char column_account_id[256];
	char column_userid[256];
	char column_user_pass[256];
	char column_level[256];


	// others
	int lowest_gm_level;

	DBDefaults() : sql_conf_name("conf/inter_athena.conf")
	{	// setting defaults
		// db server defaults
		server_port = 3306;
		strcpy(server_ip, "127.0.0.1");
		strcpy(server_id, "ragnarok");
		strcpy(server_pw, "ragnarok");
		strcpy(server_db, "ragnarok");

		// db table defaults
		strcpy(table_login, "login");
		strcpy(table_loginlog, "loginlog");

		// db column defaults
		strcpy(column_account_id, "account_id");
		strcpy(column_userid, "userid");
		strcpy(column_user_pass, "user_pass");
		strcpy(column_level, "level");

		//others
		lowest_gm_level = 1;

		// loading from default file
		LoadConfig(sql_conf_name);
	}

	virtual bool ProcessConfig(const char*w1, const char*w2)
	{
		/////////////////////////////////////////////////////////////
		//add for DB connection
		/////////////////////////////////////////////////////////////
		if(strcasecmp(w1,"login_server_ip")==0){
			strcpy(server_ip, w2);
			ShowMessage ("set Database Server IP: %s\n",w2);
		}
		else if(strcasecmp(w1,"login_server_port")==0){
			server_port=atoi(w2);
			ShowMessage ("set Database Server Port: %s\n",w2);
		}
		else if(strcasecmp(w1,"login_server_id")==0){
			strcpy(server_id, w2);
			ShowMessage ("set Database Server ID: %s\n",w2);
		}
		else if(strcasecmp(w1,"login_server_pw")==0){
			strcpy(server_pw, w2);
			ShowMessage ("set Database Server PW: %s\n",w2);
		}

		/////////////////////////////////////////////////////////////
		// tables
		/////////////////////////////////////////////////////////////
		else if(strcasecmp(w1,"login_server_db")==0){
			strcpy(table_login, w2);
			ShowMessage ("set Database Server DB: %s\n",w2);
		}
		else if (strcasecmp(w1, "loginlog_db") == 0) {
			strcpy(table_loginlog, w2);
		}

		/////////////////////////////////////////////////////////////
		//added for custom column names for custom login table
		/////////////////////////////////////////////////////////////
		else if(strcasecmp(w1,"login_db_account_id")==0){
			strcpy(column_account_id, w2);
		}
		else if(strcasecmp(w1,"login_db_userid")==0){
			strcpy(column_userid, w2);
		}
		else if(strcasecmp(w1,"login_db_user_pass")==0){
			strcpy(column_user_pass, w2);
		}
		else if(strcasecmp(w1,"login_db_level")==0){
			strcpy(column_level, w2);
		}

		/////////////////////////////////////////////////////////////
		//support the import command, just like any other config
		/////////////////////////////////////////////////////////////
		else if(strcasecmp(w1,"import")==0){
			LoadConfig(w2);
		}
	}
};


//////////////////////////////////////////////////////////////////////////
// basic interface to a sql database
//////////////////////////////////////////////////////////////////////////

class SQLDatabase
{
protected:
	DBDefaults& def;

	char	tmpsql[65536];
	MYSQL	handle;

	SQLDatabase(DBDefaults &d) : def(d)	
	{
		open(NULL);
	}
	~SQLDatabase()
	{
		close();
	}

	virtual bool open(const char* str=NULL)
	{	
		bool ret = true;
		mysql_init(&handle);
		//DB connection start
		ShowInfo("Connect DB Server (%s)....\n", def.server_db);
		if(!mysql_real_connect(&handle, def.server_ip, 
										def.server_id, 
										def.server_pw, 
										def.server_db, 
										def.server_port, NULL, 0)) 
		{
			//pointer check
			ShowError("%s\n",mysql_error(&handle));
			ret = false;
		}
		else 
		{
			ShowStatus("connect success!\n");
		}

		sprintf(tmpsql, "INSERT DELAYED INTO `%s`(`time`,`ip`,`user`,`rcode`,`log`) VALUES (NOW(), '', 'lserver', '100','login server started')", def.table_loginlog);
		//query
		if (mysql_SendQuery(handle, tmpsql)) {
			ShowMessage("DB server Error - %s\n", mysql_error(&handle));
			ret = false;
		}
		return ret;
	}

	virtual bool close()
	{
		bool ret = true;
		//set log.
		sprintf(tmpsql,"INSERT DELAYED INTO `%s`(`time`,`ip`,`user`,`rcode`,`log`) VALUES (NOW(), '', 'lserver','100', 'login server shutdown')", def.table_loginlog);

		//query
		if (mysql_SendQuery(handle, tmpsql)) {
			ShowMessage("DB server Error - %s\n", mysql_error(&handle));
			ret = false;
		}

		//delete all server status
		sprintf(tmpsql,"DELETE FROM `sstatus`");

		//query
		if (mysql_SendQuery(handle, tmpsql)) {
			ShowMessage("DB server Error - %s\n", mysql_error(&handle));
			ret = false;
		}

		mysql_close(&handle);
		ShowMessage("Close DB Connection (%s)....\n", def.server_db);

		return ret;
	}

};

//////////////////////////////////////////////////////////////////////////
#endif
//////////////////////////////////////////////////////////////////////////







//////////////////////////////////////////////////////////////////////////
#ifdef TXT_ONLY
//////////////////////////////////////////////////////////////////////////






//////////////////////////////////////////////////////////////////////////
#else// SQL
//////////////////////////////////////////////////////////////////////////







class Login_DB : public SQLDatabase
{
public:
	Login_DB(DBDefaults& d) : SQLDatabase(d)
	{
	}
	~Login_DB()									
	{
	}

	int isGM(unsigned long account_id) {
		int level;

		MYSQL_RES* 	sql_res=NULL;
		MYSQL_ROW	sql_row;
		level = 0;
		sprintf(tmpsql,"SELECT `%s` FROM `%s` WHERE `%s`='%d'", 
			def.column_level, def.table_login, def.column_account_id, account_id);

		if (mysql_SendQuery(handle, sql_res, tmpsql))
		{
			sql_row = mysql_fetch_row(sql_res);
			level = atoi(sql_row[0]);
			if (level > 99)
				level = 99;
			mysql_free_result(sql_res);
		}
		return level;
	}


	bool NewAccount(const char*userid, const char* passwd, const char *sex)
	{
		MYSQL_RES* 	sql_res=NULL;
		bool ret = false;
		char t_uid[256], t_pass[256];

		jstrescapecpy(t_uid,  userid);
		jstrescapecpy(t_pass, passwd);

		sprintf(tmpsql, "SELECT `%s` FROM `%s` WHERE `userid` = '%s'", 
			def.column_userid, def.table_login, t_uid);

		if(mysql_SendQuery(handle, sql_res, tmpsql))
		{
			if(mysql_num_rows(sql_res) == 0)
			{	// ok no existing acc,

				ShowMessage("Adding a new account user: %s with passwd: %s sex: %c\n", 
					userid, passwd, sex);

				sprintf(tmpsql, "INSERT INTO `%s` (`%s`, `%s`, `sex`, `email`) VALUES ('%s', '%s', '%c', '%s')", 
					def.table_login, def.column_userid, def.column_user_pass, 
					t_uid, t_pass, sex, "a@a.com");

				if(mysql_SendQuery(handle, tmpsql))
				{
					//Failed to insert new acc :/
					ShowMessage("SQL error (NewAccount): %s", mysql_error(&handle));
				}//sql query check to insert
				else
					ret =false;
			}//rownum check (0!)
			mysql_free_result(sql_res);
		}//sqlquery
		//all values for NEWaccount ok ?
		return ret;
	}


};



//////////////////////////////////////////////////////////////////////////
#endif
//////////////////////////////////////////////////////////////////////////




















//////////////////////////////////////////////////////////////////////////
// Database Definitions
//////////////////////////////////////////////////////////////////////////


/*
///////////////////////////////////////////////////////////////////////////////
// GM Database
///////////////////////////////////////////////////////////////////////////////

class GM_Database : public Database
{
	//////////////////////////////////////////////////////////////////////////
	// database entry structure
	class gm_account 
	{
	public:
		unsigned long account_id;
		unsigned char level;

		gm_account(int a=0, int l=0):account_id(a), level(l)	{}
		bool operator==(const gm_account&g) const	{ return account_id==g.account_id; }
		bool operator> (const gm_account&g) const	{ return account_id> g.account_id; }
		bool operator< (const gm_account&g) const	{ return account_id< g.account_id; }
	};
	//////////////////////////////////////////////////////////////////////////
	// database itself is just a sorted list
	TslistDST<gm_account> gm_list;
	//////////////////////////////////////////////////////////////////////////


public:

	//////////////////////////////////////////////////////////////////////////
	#ifdef TXT_ONLY
	//////////////////////////////////////////////////////////////////////////
	GM_Database()
	{	//defaults

		read_gm_account();
	}
	~GM_Database()	
	{}

	int read_gm_account() {
		char line[512];
		FILE *fp;
		int account_id, level;
		int line_counter;
		struct stat file_stat;
		int start_range = 0, end_range = 0, is_range = 0, current_id = 0;

		gm_list.clear();

		// get last modify time/date
		if (stat(def.GM_account_filename, &file_stat))
			def.GM_account_creation = 0; // error
		else
			def.GM_account_creation = file_stat.st_mtime;

		if ((fp = safefopen(def.GM_account_filename, "r")) == NULL) {
			ShowError("read_gm_account: GM accounts file [%s] not found.\n", def.GM_account_filename);
			ShowMessage("                 Actually, there is no GM accounts on the server.\n");
			return false;
		}

		line_counter = 0;
		// limited to 4000, because we send information to char-servers (more than 4000 GM accounts???)
		// int (id) + int (level) = 8 bytes * 4000 = 32k (limit of packets in windows)
		while(fgets(line, sizeof(line), fp) && line_counter < 4000) {
			line_counter++;
			if( !skip_empty_line(line) )
				continue;
			is_range = (sscanf(line, "%d%*[-~]%d %d",&start_range,&end_range,&level)==3); // ID Range [MC Cameri]

			if (!is_range && sscanf(line, "%d %d", &account_id, &level) != 2 && sscanf(line, "%d: %d", &account_id, &level) != 2)
				ShowMessage("read_gm_account: file [%s], invalid 'acount_id|range level' format (line #%d).\n", def.GM_account_filename, line_counter);
			else if (level <= 0)
				ShowMessage("read_gm_account: file [%s] %dth account (line #%d) (invalid level [0 or negative]: %d).\n", def.GM_account_filename, gm_list.size()+1, line_counter, level);
			else {
				if (level > 99) {
					ShowMessage("read_gm_account: file [%s] %dth account (invalid level, but corrected: %d->99).\n", def.GM_account_filename, gm_list.size()+1, level);
					level = 99;
				}
				if (is_range) {
					if (start_range==end_range)
						ShowMessage("read_gm_account: file [%s] invalid range, beginning of range is equal to end of range (line #%d).\n", def.GM_account_filename, line_counter);
					else if (start_range>end_range)
						ShowMessage("read_gm_account: file [%s] invalid range, beginning of range must be lower than end of range (line #%d).\n", def.GM_account_filename, line_counter);
					else
						for (current_id = start_range;current_id<=end_range;current_id++)
							gm_list.push( gm_account(current_id,level) );
				} else {
					gm_list.push( gm_account(account_id,level) );
				}
			}
		}
		fclose(fp);
		ShowMessage("read_gm_account: file '%s' read (%d GM accounts found).\n", def.GM_account_filename, gm_list.size());
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	#else// SQL
	//////////////////////////////////////////////////////////////////////////

	GM_Database(DBDefaults& d)
	{
		read_gm_account();
	}
	~GM_Database()	
	{}


	void read_gm_account(void) 
	{
		gm_list.clear();

		snprintf(tmpsql, sizeof(tmpsql), "SELECT `%s`,`%s` FROM `%s` WHERE `%s`>='%d'",
			def.column_account_id, def.column_level, def.table_login, def.column_level, def.lowest_gm_level);

		if( mysql_SendQuery(&handle, tmpsql) ) 
		{
			ShowMessage("DB server Error (select %s to Memory)- %s\n",def.table_login,mysql_error(&handle));
		}
		else
		{
			MYSQL_RES* 	lsql_res = mysql_store_result(&handle);
			MYSQL_ROW	lsql_row;
			size_t line_counter = 0;
			if (lsql_res) {

				gm_list.realloc( (size_t)mysql_num_rows(lsql_res) );

				while( (lsql_row = mysql_fetch_row(lsql_res)) && line_counter < 4000) 
				{
					line_counter++;
					gm_list.push( gm_account(atoi(lsql_row[0]), atoi(lsql_row[1])) );
				}
			}
			mysql_free_result(lsql_res);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	#endif// SQL
	///////////////////////////////////////////////////////////////////////////////


	bool _tobuffer(unsigned char*buffer, size_t &btsz)
	{	// sizeof(gm_account) might get wrong size
		// we stuff 4byte account_id and 1byte level = 5
		btsz = 5 * gm_list.size();
		buffer_iterator bi(buffer, btsz);
		gm_list.clear();
		for(size_t i=0; i<gm_list.size() && !bi.eof(); i++ )
		{
			bi = gm_list[i].account_id;
			bi = gm_list[i].level;
		}
		return true;
	}
	bool _frombuffer(const unsigned char*buffer, size_t btsz)
	{	
		unsigned long ac;
		unsigned char lv;
		buffer_iterator bi(buffer, btsz);
		gm_list.clear();
		while( !bi.eof() )
		{
			ac = bi;
			lv = bi;
			gm_list.push( gm_account(ac, lv) );
		}
		return true;
	}

	unsigned char is_GM(unsigned long account_id)
	{
		size_t pos;
		if( gm_list.find(account_id, 0, pos) )
			return gm_list[pos].level;
		return 0;
	}
};

*/





void database_log_interal(ulong ip, const char*user_id, int code, char*msg)
{	// log the message to database or txt

}



void database_log(ulong ip, const char*user_id, int code, char*msg,...)
{
	// initially using a static fixed buffer size 
	static char		tempbuf[4096]; 
	// and make it multithread safe
	static Mutex	mtx;
	ScopeLock		sl(mtx);

	size_t sz  = 4096; // initial buffer size
	char *ibuf = tempbuf;
	va_list argptr;
	va_start(argptr, msg);

	do{
		// print
		if( vsnprintf(ibuf, sz, msg, argptr) >=0 ) // returns -1 in case of error
			break; // print ok, can break
		// otherwise
		// aFree the memory if it was dynamically alloced
		if(ibuf!=tempbuf) delete[] ibuf;
		// double the size of the buffer
		sz *= 2;
		ibuf = new char[sz];
		// and loop in again
	}while(1); 

	database_log_interal(ip,user_id,code,ibuf);

	va_end(argptr);
	if(ibuf!=tempbuf) delete[] ibuf;
	//mutex will be released on scope exit
}

















//////////////////////////////////////////////////////////////////////////
// Interface for objects that might be transfered via bytestream
//////////////////////////////////////////////////////////////////////////

class CGlobalReg : public streamable
{
public:
	char str[32];
	long value;
	
	
	//////////////////////////////////////////////////////////////////////
	// return necessary size of buffer
	virtual size_t BufferSize() const
	{
		return 32+sizeof(long);
	}
	//////////////////////////////////////////////////////////////////////
	// writes content to buffer
	virtual bool toBuffer(buffer_iterator& bi) const
	{
		bi.str2buffer(str,32);
		bi = value;
		bi = (long)10;
	}
	//////////////////////////////////////////////////////////////////////
	// reads content from buffer
	virtual bool fromBuffer(const buffer_iterator& bi)
	{
		bi.buffer2str(str,32);
		value = bi;
	}

};


class CAuthData
{
public:
	long account_id;
	char sex;
	char userid[24];
	char pass[33]; // 33 for 32 + NULL terminated
	char lastlogin[24]; 
	long logincount;
	long state; // packet 0x006a value + 1 (0: compte OK)
	char email[40]; // e-mail (by default: a@a.com)
	char error_message[20]; // Message of error code #6 = Your are Prohibited to log in until %s (packet 0x006a)
	time_t ban_until_time; // # of seconds 1/1/1970 (timestamp): ban time limit of the account (0 = no ban)
	time_t connect_until_time; // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
	char last_ip[16]; // save of last IP of connection
	char memo[255]; // a memo field
	long account_reg2_num;

	CGlobalReg globalreg[ACCOUNT_REG2_NUM];


	void test()
	{
		unsigned char buffer[10];
		buffer_iterator bi(buffer, 10);

		bi = globalreg[1];
	
	}
};


