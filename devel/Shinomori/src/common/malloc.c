#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "malloc.h"
#include "showmsg.h"

#if !defined(DMALLOC) && !defined(GCOLLECT) && !defined(BCHECK)

void* aMalloc_( size_t size, const char *file, int line, const char *func )
{
	void *ret;
	
//	ShowMessage("%s:%d: in func %s: malloc %d\n",file,line,func,size);
	ret=malloc(size);
	if(ret==NULL){
		ShowError("%s:%d: in func %s: malloc error out of memory!\n",file,line,func);
		exit(1);
	}
	return ret;
}
void* aCalloc_( size_t num, size_t size, const char *file, int line, const char *func )
{
	void *ret;
	
//	ShowMessage("%s:%d: in func %s: calloc %d %d\n",file,line,func,num,size);
	ret=calloc(num,size);
	if(ret==NULL){
		ShowError("%s:%d: in func %s: calloc error out of memory!\n",file,line,func);
		exit(1);
	}
	return ret;
}

void* aRealloc_( void *p, size_t size, const char *file, int line, const char *func )
{
	void *ret;
	
//	ShowMessage("%s:%d: in func %s: realloc %p %d\n",file,line,func,p,size);
	ret=realloc(p,size);
	if(ret==NULL){
		ShowError("%s:%d: in func %s: realloc error out of memory!\n",file,line,func);
		exit(1);

	}
	return ret;
}

#endif


#if defined(GCOLLECT)

void * _bcallocA(size_t size, size_t cnt) {
	void *ret = aMallocA(size * cnt *sizeof(char));
	memset(ret, 0, size * cnt *sizeof(char));
	return ret;
}

void * _bcalloc(size_t size, size_t cnt) {
	void *ret = aMalloc(size * cnt*sizeof(char));
	memset(ret, 0, size * cnt*sizeof(char));
	return ret;
}
#endif

char * _bstrdup(const char *chr) {
	if(chr)
	{
		int len = strlen(chr)+1;
		char *ret = (char*)aMalloc(len*sizeof(char));
		memcpy(ret, chr, len);
		return ret;
	}
	return NULL;
}
