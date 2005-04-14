// original : core.h 2003/03/14 11:55:25 Rev 1.4

#ifndef	_CORE_H_
#define	_CORE_H_

#include "base.h"

extern int runflag;

extern int do_init(int,char**);
extern void do_final();


extern inline char* get_svn_revision()	{ return "Shinomori's Modified Version"; }

#endif	// _CORE_H_
