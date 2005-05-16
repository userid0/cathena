// original : core.h 2003/03/14 11:55:25 Rev 1.4

#ifndef	_CORE_H_
#define	_CORE_H_

#include "base.h"
#include "version.h"


extern int do_init(int,char**);
extern void do_final();
extern unsigned char getServerType();

void core_stoprunning();

extern inline char* get_svn_revision()	{ return "Shinomori's Modified Version"; }


///////////////////////////////////////////////////////////////////////////////
// uptime
class uptime
{
	static unsigned long starttick;

	uptime(const uptime&);					// prevent copy
	const uptime& operator=(const uptime&);	// prevent assign
public:
	static const char *getstring(char *buffer=NULL);
	static void getvalues(unsigned long& days,unsigned long& hours,unsigned long& minutes,unsigned long& seconds);
};


#endif	// _CORE_H_
