#ifndef _J_STR_LIB_H_
#define _J_STR_LIB_H_
#include "base.h"

// String function library.
// code by Jioh L. Jung (ziozzang@4wish.net)
// This code is under license "BSD"
unsigned char* jstrescape (unsigned char* pt);
unsigned char* jstrescapecpy (unsigned char* pt,unsigned char* spt);
size_t jmemescapecpy (unsigned char* pt,unsigned char* spt, int size);

#endif
