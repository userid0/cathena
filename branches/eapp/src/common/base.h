#ifndef	_BASE_H_
#define _BASE_H_

//////////////////////////////////////////////////////////////////////////
// temporarily include all necessary base files here here
//////////////////////////////////////////////////////////////////////////
#include "basetypes.h"
#include "basememory.h"
#include "basecharset.h"
#include "basestring.h"
#include "basearray.h"
#include "basesync.h"
#include "basetime.h"
#include "basefile.h"
#include "basepool.h"
#include "baseinet.h"
#include "baseministring.h"




//////////////////////////////////////////////////////////////////////////
// leftovers from base files currently not included in this package
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// #include "baseinet.h"
//////////////////////////////////////////////////////////////////////////





//////////////////////////////
#ifdef WIN32
//////////////////////////////

// defines

#define RETCODE	"\r\n"
#define RET RETCODE



//////////////////////////////
#else/////////////////////////
//////////////////////////////

// defines
#define RETCODE "\n"
#define RET RETCODE



//////////////////////////////
#endif////////////////////////
//////////////////////////////








#endif//_BASE_H_

