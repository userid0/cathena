#ifndef _J_STR_LIB_H_
#define _J_STR_LIB_H_
#include "base.h"

// String function library.
// code by Jioh L. Jung (ziozzang@4wish.net)
// This code is under license "BSD"
char* jstrescape (char* pt);
char* jstrescapecpy (char* pt, char* spt);
size_t jmemescapecpy (char* pt, char* spt, int size);


#endif
