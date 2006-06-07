#include "basetypes.h"
#include "baseobjects.h"
#include "baseministring.h"
#include "basesync.h"
#include "basefile.h"

#include "baseparam.h"

NAMESPACE_BEGIN(basics)

//////////////////////////////////////////////////////////////////////////
// basic interface for reading configs from file
//////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// Loading a file, stripping lines,
// splitting it to "part1 : part2" or "part1 = part2"
// calling the derived function for processing
/////////////////////////////////////////////////////////////////
bool CConfig::LoadConfig(const char* cfgName)
{
	char line[1024], w1[1024], w2[1024], *ip;
	FILE *fp;
	// called with empty filename
	// just return silently 
	if( !cfgName || 0==*cfgName )
		return false;

	if( (fp = safefopen(cfgName, "r")) == NULL) {
		printf("Configuration file (%s) not found.\n", cfgName);
		return false;
	}

	printf("Reading configuration file '%s'\n", cfgName);
	while(fgets(line, sizeof(line), fp))
	{
		// terminate buffer
		line[sizeof(line)-1] = '\0';

		// skip leading spaces
		ip = line;
		while( stringcheck::isspace(*ip) ) ip++;

		// skipping comment lines
		if( !ip[0] || (ip[0] == '/' && ip[1] == '/'))
			continue;

		memset(w2, 0, sizeof(w2));
		// format: "name:value"
		if( sscanf(ip, "%[^=]= %[^\r\n]", w1, w2) == 2 ||
			sscanf(ip, "%[^:]: %[^\r\n]", w1, w2) == 2 )
		{
			CleanControlChars(w1);
			CleanControlChars(w2);
			itrim(w1);
			itrim(w2);
			if( strcasecmp(w1, "import") == 0 ||
				strcasecmp(w1, "include") == 0 )
			{	// call recursive, prevent infinity loop (first order only)
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
	printf("Reading configuration file '%s' finished\n", cfgName);
	return true;
}

// Return 0/1 for no/yes
int CConfig::SwitchValue(const char *str, int defaultmin, int defaultmax)
{
	if( str )
	{
		if( strcasecmp(str, "true")==0 || strcasecmp(str, "on") == 0 || strcasecmp(str, "yes") == 0 || strcasecmp(str, "oui") == 0 || strcasecmp(str, "ja") == 0 || strcasecmp(str, "si") == 0)
			return 1;
		else if( strcasecmp(str, "false")==0 || strcasecmp(str, "off") == 0 || strcasecmp(str, "no" ) == 0 || strcasecmp(str, "non") == 0 || strcasecmp(str, "nein") == 0)
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

// Return true/false for yes/no, if unknown return defaultval
bool CConfig::Switch(const char *str, bool defaultval)
{
	if( str )
	{
		if( strcasecmp(str, "true")==0 || strcasecmp(str, "on") == 0 || strcasecmp(str, "yes") == 0 || strcasecmp(str, "oui") == 0 || strcasecmp(str, "ja") == 0 || strcasecmp(str, "si") == 0)
			return 1;
		else if( strcasecmp(str, "false")==0 || strcasecmp(str, "off") == 0 || strcasecmp(str, "no" ) == 0 || strcasecmp(str, "non") == 0 || strcasecmp(str, "nein") == 0)
			return 0;
		else
		{
			const int ret = atoi(str);
			if(ret==0 || ret==1) return ret;
		}
	}
	return defaultval;
}

// Replace control chars with '_' and return if changed
bool CConfig::CleanControlChars(char *str)
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






///////////////////////////////////////////////////////////////////////////////
// basic class for using the old way timers
///////////////////////////////////////////////////////////////////////////////
/*
bool CTimerBase::init(unsigned long interval)
{
	if(interval<1000)
		interval = 1000;
	//cTimer = add_timer_interval(gettick()+interval, interval, timercallback, 0, basics::numptr(this), false);
	cTimer = -1;
	return (cTimer>=0);
}

// external calling from external timer implementation
int CTimerBase::timercallback(int timer, unsigned long tick, int id, numptr data)
{
	if(data.isptr)
	{
		CTimerBase* base = (CTimerBase*)data.ptr;
		if(timer==base->cTimer)
		{
			if( !base->timeruserfunc(tick) )
			{
//				delete_timer(base->cTimer, timercallback);
				base->cTimer = -1;
			}
		}
	}
	return 0;
}
void CTimerBase::timerfinalize()
{
	if(cTimer>0)
	{
//		delete_timer(cTimer, timercallback);
		cTimer = -1;
	}
}
*/

///////////////////////////////////////////////////////////////////////////////
// parse the commandline for parameters
// format
///////////////////////////////////////////////////////////////////////////////
void parseCommandline(int argc, char **argv)
{
	char buffer[1024];
	staticstring<char> str(buffer, sizeof(buffer));
	char w1[1024], w2[1024];
	int i;

	// reserved parameters
	{
		char*path = argv[0];
		char*file = strrchr(path, PATHSEP);
		if(file)
		{	// had a pathname with a filename
			*file++=0;
		}
		else
		{	// no path
			path = "";
			file = argv[0];
		}

		createParam("application", file);
		createParam("application_path", path);
	}

	for(i=1; i<argc; ++i)
	{
		if( is_file(argv[i]) )
		{
			// we have a valid filename, so we load it directly
			CParamBase::loadFile(argv[i]);

			// and clear the string, start new search
			str.empty();
		}
		else
		{	// not a file, check if it is a direct parameter
			str << argv[i] << ' ';
			memset(w1, 0, sizeof(w1));
			memset(w2, 0, sizeof(w2));
			// if not matching in the first place 
			// just try with the concatinated commandline
			// until something matches or the line runs out
			//## check effort with regex
			if( sscanf(argv[i], "%[^=]= %[^\r\n]", w1, w2) == 2 ||
				sscanf(argv[i], "%[^:]: %[^\r\n]", w1, w2) == 2 ||
				sscanf(str,     "%[^=]= %[^\r\n]", w1, w2) == 2 ||
				sscanf(str,     "%[^:]: %[^\r\n]", w1, w2) == 2 )
			{
				CConfig::CleanControlChars(w1);
				CConfig::CleanControlChars(w2);
				itrim(w1);
				itrim(w2);

				if( strcasecmp(w1, "import") == 0 ||
					strcasecmp(w1, "include") == 0 ||
					strcasecmp(w1, "load") == 0 )
				{	// load the file
					CParamBase::loadFile(w2);
				}
				else
				{	// create the parameter
					createParam(w1, w2);
				}
				// clear the string, start new search
				str.empty();
			}
		}
	}
}




#ifdef PARAM_RTTI
///////////////////////////////////////////////////////////////////////////////
//
// Parameter Class
//
///////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////
/// timer callback
bool CParamBase::CParamLoader::timeruserfunc(unsigned long tick)
{	/// check all listed files for modification
	size_t i=cFileList.size();
	while(i)
	{
		--i;
		if( cFileList[i].isModified() )
		{
			bool ok = ( cFileList[i].proc )?(*(cFileList[i].proc))(cFileList[i]):CConfig::LoadConfig( cFileList[i] );
			if(!ok)
				cFileList.removeindex(i);
		}
	}
	return true;
}
///////////////////////////////////////////////////////////////////////////////
/// external access
void CParamBase::CParamLoader::loadFile(const string<>& filename, paramproc p)
{
	const CFileData tmp(filename, p);
	size_t pos;
	const bool find=this->cFileList.find(tmp,0,pos);
	// update the processing function
	if( find && p!=this->cFileList[pos].proc )
		this->cFileList[pos].proc = p;
	if( !find || this->cFileList[pos].isModified() )
	{	// call the processing function
		const bool ok = ( p ) ? (*p)( filename ) : (this->CConfig::LoadConfig( filename ));
		// and insert
		if(ok && !find)
			this->cFileList.insert( tmp );
	}
}
///////////////////////////////////////////////////////////////////////////////
// static data
CParamBase::CSingletonData &CParamBase::getSingletonData()
{
	static CParamBase::CSingletonData sd;
	return sd;
}

///////////////////////////////////////////////////////////////////////////
// static access functions
CParamBase& CParamBase::getParam(const string<>& name)
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t pos;
	CParamBase tmp(name);
	if( !sd.cParams.find(tmp, 0, pos) )
	{
		tmp.cTime = GetTickCount();
		sd.cParams.insert(tmp);
		if( !sd.cParams.find(tmp, 0, pos) )
			throw exception("Params: insert failed");
	}
	return sd.cParams[pos];
}
// check existence of a parameter
bool CParamBase::existParam(const string<>& name)
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t pos;
	CParamBase tmp(name);
	return sd.cParams.find(tmp, 0, pos);
}
// clean unreferenced parameters
void CParamBase::clean()
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t i=sd.cParams.size();
	while(i>0)
	{
		i--;
		if( !sd.cParams[i].cParam->isReferenced() )
			sd.cParams.removeindex(i);
	}
}
void CParamBase::listall()
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t i;
	printf("\nList of Parameters (existing %li):", (unsigned long)sd.cParams.size());
	for(i=0; i<sd.cParams.size(); ++i)
		sd.cParams[i].print();
	printf("\n---------------\n");

}
string<> CParamBase::getlist()
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t i;
	string<> str;
	for(i=0; i<sd.cParams.size(); ++i)
	{
		str << sd.cParams[i].name() << " = " << sd.cParams[i].value() << "\n";
	}
	return str;
}
void CParamBase::loadFile(const string<>& name, paramproc p)
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	sd.cLoader.loadFile(name,p);
}

#else
///////////////////////////////////////////////////////////////////////////////
//
// Parameter Class
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// assign a value to the parameter object and call the childs
void CParamObj::assignvalue(const string<>& value)
{
	this->cTime = GetTickCount();
	if( this->cValue != value )
	{	// take over new value but store the old one
		string<> oldvalue = this->cValue;
		this->cValue = value;
		// check global callback
		if( !this->cCallback || this->cCallback(*this, this->cValue, oldvalue) )
		{	// and go through the childs on success
			CParamBase* ptr = this->cParamRoot;
			while( ptr )
			{
				ptr->call(value);
				ptr = ptr->cNext;
			}
		}
		else
		{	// otherwise do a rollback
			this->cValue = oldvalue;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
// print the content to stdout
void CParamObj::print()
{
	CParamBase* ptr=this->cParamRoot;
	uint i=0;
	while(ptr)
		i++, ptr=ptr->cNext;
	printf("\nname: %-16s value='%s' (linked variable type)\nreferenced: %i, count: %i, timestamp %lu", 
		(const char*)(*this), (const char*)this->cValue, this->cReferenced, i, this->cTime);
}



///////////////////////////////////////////////////////////////////
// construction/destruction
CParamBase::CParamLoader::CFileData::CFileData(const string<>& filename, paramproc p)
	: string<>(filename), proc(p)
{
	struct stat s;
	if( 0==stat((const char*)filename, &s) )
		modtime = s.st_mtime;
}
///////////////////////////////////////////////////////////////////
// checking file state and updating the locals at the same time
bool CParamBase::CParamLoader::CFileData::isModified()
{
	struct stat s;
	return ( 0==stat((const char*)(*this), &s) && s.st_mtime!=this->modtime && (this->modtime=s.st_mtime)==this->modtime );
}

/////////////////////////////////////////////////////////////
// timer callback
bool CParamBase::CParamLoader::timeruserfunc(unsigned long tick)
{	// check all listed files for modification
	size_t i=this->cFileList.size();
	while(i)
	{
		--i;
		if( this->cFileList[i].isModified() )
		{	// call the processing funcion
			bool ok = (this->cFileList[i].proc)?(*(this->cFileList[i].proc))(cFileList[i]):this->CConfig::LoadConfig( cFileList[i] );
			if(!ok)
			{	// remove the file from watchlist if processing asked for it
				this->cFileList.removeindex(i);
			}
		}
	}
	return true;
}
/////////////////////////////////////////////////////////////
// config processing callback
bool CParamBase::CParamLoader::ProcessConfig(const char*w1,const char*w2)
{	// create/update parameter
	CParamBase::createParam(w1, w2);
	return true;
}
/////////////////////////////////////////////////////////////
// external access
void CParamBase::CParamLoader::loadFile(const string<>& filename, paramproc p)
{
	const CFileData tmp(filename, p);
	size_t pos;
	const bool find=this->cFileList.find(tmp,0,pos);
	// update the processing function
	if( find && p!=this->cFileList[pos].proc )
		this->cFileList[pos].proc = p;
	if( !find || this->cFileList[pos].isModified() )
	{	// call the processing function
		const bool ok = ( p ) ? (*p)( filename ) : (this->CConfig::LoadConfig( filename ));
		// and insert it when ok
		if(ok && !find)
			this->cFileList.insert( tmp );
	}
}


///////////////////////////////////////////////////////////////////////////
// static data
// need a singleton here for allowing global parameters
CParamBase::CSingletonData &CParamBase::getSingletonData()
{
	static CParamBase::CSingletonData sd;
	return sd;
}

///////////////////////////////////////////////////////////////////////////
// get a parameter, create it with default value when not existing
TObjPtrCount<CParamObj> CParamBase::getParam(const string<>& name, const string<>& value)
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t pos;

	CParamObj tmp(name);
	if( !sd.cParams.find(tmp, 0, pos) )
	{
		sd.cParams.insert(tmp);
		if( !sd.cParams.find(tmp, 0, pos) )
			throw exception("Params: insert failed");

		sd.cParams[pos]->cTime = GetTickCount();
		sd.cParams[pos]->cValue = value;
	}
	if( !sd.cParams[pos]->cReferenced ) sd.cParams[pos]->cReferenced=true;
	return sd.cParams[pos];
}
///////////////////////////////////////////////////////////////////////////////
// create a new parameter or update the values
TObjPtr<CParamObj> CParamBase::createParam(const string<>& name, const string<>& value)
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t pos;

	CParamObj tmp(name);
	if( !sd.cParams.find(tmp, 0, pos) )
	{
		sd.cParams.insert(tmp);
		if( !sd.cParams.find(tmp, 0, pos) )
			throw exception("Params: insert failed");
	}
	sd.cParams[pos]->assignvalue(value);
	return sd.cParams[pos];
}
///////////////////////////////////////////////////////////////////////////////
// check if a parameter exists
bool CParamBase::existParam(const string<>& name)
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t pos;
	CParamObj tmp(name);
	return sd.cParams.find(tmp, 0, pos);
}
///////////////////////////////////////////////////////////////////////////////
// remove all unreferenced parameters
void CParamBase::clean()
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t i=sd.cParams.size();
	while(i>0)
	{
		i--;
		if( !sd.cParams[i]->cReferenced )
			sd.cParams.removeindex(i);
	}
}
///////////////////////////////////////////////////////////////////////////////
// prints all parameters as list to stdout
void CParamBase::listall()
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t i;
	printf("\nList of Parameters (existing %li):", (unsigned long)sd.cParams.size());
	for(i=0; i<sd.cParams.size(); ++i)
		sd.cParams[i]->print();
	printf("\n---------------\n");
}
///////////////////////////////////////////////////////////////////////////////
// returns a string with all parameters used (as file format & <nl> delimited)
string<> CParamBase::getlist()
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	size_t i;
	string<> str;
	for(i=0; i<sd.cParams.size(); ++i)
	{
		str << sd.cParams[i]->name() << " = " << sd.cParams[i]->value() << "\n";
	}
	return str;
}
///////////////////////////////////////////////////////////////////////////////
// load parameters from file
void CParamBase::loadFile(const string<>& filename, paramproc p)
{
	CSingletonData &sd = getSingletonData();
	ScopeLock sl(sd);
	sd.cLoader.loadFile(filename,p);
}

///////////////////////////////////////////////////////////////////////////////
// link a parameter access object with the parameter storage object
void CParamBase::link()
{
	this->unlink();
	if( this->cParamObj.exists() )
	{
		// put this node in front
		this->cNext = this->cParamObj->cParamRoot;
		this->cParamObj->cParamRoot = this;
	}
}
///////////////////////////////////////////////////////////////////////////////
// unlink parameter access object
void CParamBase::unlink()
{
	if( this->cParamObj.exists() )
	{
		// remove node from list
		if( this->cParamObj->cParamRoot == this )
		{	// remove root node
			this->cParamObj->cParamRoot = this->cNext;
			this->cNext = NULL;
		}
		else
		{	// look in list
			CParamBase* ptr = this->cParamObj->cParamRoot;
			while( ptr && ptr->cNext!= this )
				ptr = ptr->cNext;
			if( ptr )
			{
				ptr->cNext=this->cNext;
				this->cNext=NULL;
			}
		}
	}
}



#endif













#if defined(DEBUG)
bool doublecallback(const string<>& name, double&newval, const double&oldval)
{
	printf( "%s is changing from %lf to %lf\n", (const char*)name, oldval, newval);
	return true;
}
#endif//DEBUG


void test_parameter()
{
#if defined(DEBUG)
	try {
		double a, xx(141.30);
		MiniString b="";
		// create new param entry
		createParam("double param", "0.2");
		createParam("double param2", "0.2111");
		createParam("double param3", "0.999");

		try
		{
			CParam< MiniString > parameter2("double param", "0.0");
		}
		catch(...)
		{
			printf("'double param' not creatable with type MiniString");
		}

		// create scope persistant object
		CParam<double> parameter("double param", 0.0, doublecallback);

		a=parameter;
		printf("%lf %s\n", a, (const char*)b);

		// modify param entry
		createParam("double param", "0.4");

		a=parameter;
		printf("%lf %s\n", a, (const char*)b);

		CParam<double> testvar("double param", 0.0, doublecallback);

		a=parameter;
		b=testvar;
		printf("%lf %s\n", a, (const char*)b);

		testvar = 5;
		parameter = xx;

		a=parameter;
		b=testvar;
		printf("%lf %s\n", a, (const char*)b);


		printf("%s\n", typeid(testvar).name() );


		CParam<int> testint("int param", 8);
		CParam<MiniString> teststr("str param", "...test test...");

		CParamBase::loadFile("conf/login_athena.conf");

		CParamBase::listall();

		CParamBase::clean();
		
	}
	catch ( exception e )
	{
		printf( "exception %s", e.what() );
	}

	CParamBase::listall();

	printf("\n");
#endif//DEBUG
}


///////////////////////////////////////////////////////////////////////////////

NAMESPACE_END(basics)
