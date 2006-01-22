
#include "basetypes.h"
#include "baseobjects.h"
#include "basememory.h"
#include "basealgo.h"
#include "basetime.h"
#include "basestring.h"
#include "baseexceptions.h"



//////////////////////////////////////////////////////////////////////////
// dynamic reallocation policy for c-style allocation
//////////////////////////////////////////////////////////////////////////
void* memalloc(uint a) 
{
	if (a == 0)
		return NULL;
	else
	{
		void* p = malloc(a);
		if (p == NULL)
			throw exception_memory("Not enough memory");
		return p;
	}
}
void* memrealloc(void* p, uint a) 
{
	if (a == 0)
	{
		memfree(p);
		return NULL;
	}
	else if (p == NULL)
		return memalloc(a);
	else
	{
		p = realloc(p, a);
		if (p == NULL)
			throw exception_memory("Not enough memory");
		return p;
	}
}
void memfree(void* p)
{
	if (p != NULL)
		free(p);
}

//!! TODO copy back c++ memory handler
