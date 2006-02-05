#include "basetypes.h"
#include "baseobjects.h"
#include "basefile.h"

///////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
// do not include this globally, it poisons the namespace
#include <direct.h>
#include <io.h>			// for access
						// and also the modi are not defined in windows
#define F_OK	0x0		// Existence only
#define W_OK	0x2		// Write permission
#define R_OK	0x4		// Read permission
#define RW_OK	0x6		// Read and write permission
#else
#include <unistd.h>		// unix is clean
#endif

///////////////////////////////////////////////////////////////////////////////
// inconvinient/inefficient/slow
// but working completely platform/implementation independend
ssize_t CFile::getline(char *buf, size_t maxlen)
{
	char *ip = buf;
	int c;
	if(cFile)
	{
		// skip leading returns
		do
		{
			c = getc(cFile);
		}while((c==0x0A) || (c==0x0D));
		if(c==EOF)
			return -1;
		//at this position c contains the first read character
		do
		{
			if(ip < buf+maxlen-1)		// if buffer not full
				*ip++ = (char)c;		// put character to buffer
			c = getc(cFile);			// get next character
		}while( ((c!=0x0A) && (c!=0x0D)) && (c!=EOF) );
		*ip=0;							// put EOS
		// return the number of chars in the buffer
		return ip-buf;
	}
	*buf=0;
	return -1;
}
ssize_t CFile::writeline(const char *buf, size_t maxlen)
{
	if(buf && cFile)
	{
		if(maxlen==0) maxlen= strlen(buf);
		ssize_t sz=fwrite(buf,1,maxlen,cFile);
		if( buf[maxlen-1] != '\n')
			sz+=fwrite("\n",1,1,cFile);
		return sz;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
// system dependend implementations
///////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// copies from scpath to path and exchange the path seperator
// no check for buffer overflows
char* checkPath(char *path, size_t sz, const char *srcpath)
{	// just make sure the char*path is not const
	if( path )
	{
		char *p=path, *ep=path+sz-1;
		if( srcpath )
		{
			while(*srcpath && p<ep)
			{
				if (*srcpath=='/') {
					*p++ = '\\';
					srcpath++;
				}
				else
					*p++ = *srcpath++;
			}
		}
		*p = 0; //EOS
	}
	return path;
}
char* checkPath(char *path, const char *srcpath)
{	// just make sure the char*path is not const
	if( path )
	{
		char *p=path;
		if( srcpath )
		{
			while(*srcpath) {
				if (*srcpath=='/') {
					*p++ = '\\';
					srcpath++;
				}
				else
					*p++ = *srcpath++;
			}		
		}
		*p = *srcpath; //EOS
	}
	return path;
}
// just exchange the path seperator
char* checkPath(char *path)
{	// just make sure the char*path is not const
	char *p=path;
	if(NULL!=path)
	while(*p) {
		if (*p=='/') {
			*p++ = '\\';
		}
		else
			p++;
	}
	return path;
}
///////////////////////////////////////////////////////////////////////////////
// true when given path is a directory
bool isDirectory(const char*name)
{
	struct stat dir_stat;       // used by stat().
	if( 0!=access(name, F_OK) || stat(name, &dir_stat) == -1 )
		return false;
	return ( 0 != (dir_stat.st_mode & _S_IFDIR) );
}
///////////////////////////////////////////////////////////////////////////////
// true when given path is a file
bool isFile(const char*name)
{
	struct stat dir_stat;       // used by stat().
	if ( 0!=access(name, F_OK) || stat(name, &dir_stat) == -1 )
		return false;
	return ( 0 == (dir_stat.st_mode & _S_IFDIR) );
}
///////////////////////////////////////////////////////////////////////////////
// runs recursively through the directories starting at the given path
// calling the FileProcessor with each encountered file name containing the given pattern
bool findFiles(const char *p, const char *pat, const CFileProcessor& fp)
{	
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char tmppath[MAX_PATH+1];
	bool ok=true;
	
	const char *path    = (p  ==NULL)? "." : p;
	const char *pattern = (pat==NULL)? "" : pat;
	
	checkPath(tmppath,path);
	if( PATHSEP != tmppath[strlen(tmppath)-1])
		strcat(tmppath, "\\*");
	else
		strcat(tmppath, "*");
	
	hFind = FindFirstFile(tmppath, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (strcmp(FindFileData.cFileName, ".") == 0)
				continue;
			if (strcmp(FindFileData.cFileName, "..") == 0)
				continue;
			// exceeded pathlength
			if( 0>snprintf(tmppath, sizeof(tmppath), "%s%c%s",path,PATHSEP,FindFileData.cFileName) )
				continue;

			// give name to processor
			if (FindFileData.cFileName && strstr(FindFileData.cFileName, pattern))
				ok = fp.process( tmppath );
			// and descend to subdirs
			if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				ok=findFiles(tmppath, pat, fp);

		}while( ok && FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
   }
   return ok;
}
///////////////////////////////////////////////////////////////////////////////
#else
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DIR_PATH 2048

///////////////////////////////////////////////////////////////////////////////
// copies from scpath to path and exchange the path seperator
char* checkPath(char *path, size_t sz, const char *srcpath)
{	// just make sure the char*path is not const
	if( path )
	{
		char *p=path, *ep=path+sz-1;
		if( srcpath )
		{
			while(*srcpath && p<ep)
			{
				if (*srcpath=='\\') {
					*p++ = '/';
					srcpath++;
				}
				else
					*p++ = *srcpath++;
			}
		}
		*p = 0; //EOS
	}
	return path;
}
char* checkPath(char *path, const char *srcpath)
{	// just make sure the char*path is not const
	if( path )
	{
		char *p=path;
		if( srcpath )
		{
			while(*srcpath) {
				if (*srcpath=='\\') {
					*p++ = '/';
					srcpath++;
				}
				else
					*p++ = *srcpath++;
			}		
		}
		*p = *srcpath; //EOS
	}
	return path;
}
// just exchange the path seperator
char* checkPath(char *path)
{	// just make sure the char*path is not const
	char *p=path;
	if(NULL!=path)
	while(*p) {
		if (*p=='\\') {
			*p++ = '/';
		}
		else
			p++;
	}
	return path;
}
///////////////////////////////////////////////////////////////////////////////
// true when given path is a directory
bool isDirectory(const char*name)
{
	struct stat dir_stat;       // used by stat().
	if(0!=access(name, F_OK) || stat(name, &dir_stat) == -1)
		return false;
	return (S_ISDIR(dir_stat.st_mode));
}
///////////////////////////////////////////////////////////////////////////////
// true when given path is a file
bool isFile(const char*name)
{
	struct stat dir_stat;       // used by stat().
	if(0!=access(name, F_OK) || stat(name, &dir_stat) == -1)
		return false;
	return (!S_ISDIR(dir_stat.st_mode));
}
///////////////////////////////////////////////////////////////////////////////
// runs recursively through the directories starting at the given path
// calling the FileProcessor with each encountered file name containing the given pattern
bool findFiles(const char *p, const char *pat, const CFileProcessor& fp)
{	
	bool ok=true;
	DIR* dir;					// pointer to the scanned directory.
	struct dirent* entry;		// pointer to one directory entry.
	struct stat dir_stat;       // used by stat().
	char tmppath[MAX_DIR_PATH+1];
	char path[MAX_DIR_PATH+1]= ".";
	const char *pattern = (pat==NULL)? "" : pat;
	if(p!=NULL)
	{
		strncpy(path,p,sizeof(path)); //"If count is less than or equal to the length of strSource, a null character is not appended automatically to the copied string."
		path[MAX_DIR_PATH]=0;
	}

	// open the directory for reading
	dir = opendir( checkPath(path, path) );
	if (!dir)
	{
		fprintf(stderr, "Cannot read directory '%s'\n", path);
		return false;
	}

	// scan the directory, traversing each sub-directory
	// matching the pattern for each file name.
	while( ok && (entry = readdir(dir)) )
	{	// skip the "." and ".." entries.
		if (strcmp(entry->d_name, ".") == 0)
			continue;
		if (strcmp(entry->d_name, "..") == 0)
			continue;

		// exceeded pathlength
		if( 0>snprintf(tmppath, sizeof(tmppath), "%s%c%s",path, PATHSEP, entry->d_name) )
			continue;

		// check if the pattern matchs.
		if( entry->d_name && strstr(entry->d_name, pattern) )
			ok = fp.process( tmppath );

		// check if it is a directory.
		if( stat(tmppath, &dir_stat) == -1)
		{
			fprintf(stderr, "stat error %s\n': ", tmppath);
			continue;
		}
		// is this a directory?
		if( S_ISDIR(dir_stat.st_mode) )
		{
			// decent recursivly
			ok = findFiles(tmppath, pat, fp);
		}
	}//end while
	return ok;
}
///////////////////////////////////////////////////////////////////////////////
#endif



///////////////////////////////////////////////////////////////////////////////
// using the processing class parameter
// with the FileProcessor object
///////////////////////////////////////////////////////////////////////////////

class CSimpleFileProcessor  : public CFileProcessor
{
	void (*func)(const char*);
public:
	CSimpleFileProcessor( void (*f)(const char*) ) : func(f)	{}
	virtual ~CSimpleFileProcessor()	{}
	virtual bool process(const char *name) const
	{
		func(name);
		return true;
	}
};

bool findFiles(const char *p, const char *pat, void (func)(const char*) )
{
	return findFiles(p,pat, CSimpleFileProcessor(func) );
}

///////////////////////////////////////////////////////////////////////////////
// or have it directly
///////////////////////////////////////////////////////////////////////////////
/*

#ifdef WIN32

bool findFiles(const char *p, const char *pat, void (func)(const char*) )
{	
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char tmppath[MAX_PATH+1];
	
	const char *path    = (p  ==NULL)? "." : p;
	const char *pattern = (pat==NULL)? "" : pat;
	
	checkpath(tmppath,path);
	if( PATHSEP != tmppath[strlen(tmppath)-1])
		strcat(tmppath, "\\*");
	else
		strcat(tmppath, "*");
	
	hFind = FindFirstFile(tmppath, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (strcmp(FindFileData.cFileName, ".") == 0)
				continue;
			if (strcmp(FindFileData.cFileName, "..") == 0)
				continue;

			snprintf(tmppath, sizeof(tmppath), "%s%c%s",path,PATHSEP,FindFileData.cFileName);

			if (FindFileData.cFileName && strstr(FindFileData.cFileName, pattern)) {
				func( tmppath );
			}


			if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				findfile(tmppath, pat, func);
			}
		}while (FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
   }
   return;
}
#else

bool findFiles(const char *p, const char *pat, void (func)(const char*) )
{	
	DIR* dir;					// pointer to the scanned directory.
	struct dirent* entry;		// pointer to one directory entry.
	struct stat dir_stat;       // used by stat().
	char tmppath[MAX_DIR_PATH+1];
	char path[MAX_DIR_PATH+1]= ".";
	const char *pattern = (pat==NULL)? "" : pat;
	if(p!=NULL) strcpy(path,p);

	// open the directory for reading
	dir = opendir( checkPath(path) );
	if (!dir) {
		fprintf(stderr, "Cannot read directory '%s'\n", path);
		return;
	}

	// scan the directory, traversing each sub-directory
	// matching the pattern for each file name.
	while ((entry = readdir(dir))) {
		// skip the "." and ".." entries.
		if (strcmp(entry->d_name, ".") == 0)
			continue;
		if (strcmp(entry->d_name, "..") == 0)
			continue;

		snprintf(tmppath, sizeof(tmppath), "%s%c%s",path, PATHSEP, entry->d_name);

		// check if the pattern matchs.
		if (entry->d_name && strstr(entry->d_name, pattern)) {
			func( tmppath );
		}
		// check if it is a directory.
		if (stat(tmppath, &dir_stat) == -1) {
			fprintf(stderr, "stat error %s\n': ", tmppath);
			continue;
		}
		// is this a directory?
		if (S_ISDIR(dir_stat.st_mode)) {
			// decent recursivly
			findfile(tmppath, pat, func);
		}
	}//end while
}
#endif
*/
