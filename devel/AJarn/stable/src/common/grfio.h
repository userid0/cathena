// $Id: grfio.h,v 1.1.1.1 2004/09/10 17:44:49 MagicalTux Exp $
#ifndef	_GRFIO_H_
#define	_GRFIO_H_

#include "base.h"

void grfio_init(const char*);			// GRFIO Initialize
int grfio_add(const char*);				// GRFIO Resource file add
void* grfio_read(const char*);			// GRFIO data file read
void* grfio_reads(const char*,int*);	// GRFIO data file read & size get
int grfio_size(const char*);			// GRFIO data file size get

int decode_zip(unsigned char *dest, unsigned long* destLen, const unsigned char* source, unsigned long sourceLen);
int encode_zip(unsigned char *dest, unsigned long* destLen, const unsigned char* source, unsigned long sourceLen);

// Accessor to GRF filenames
char *grfio_setdatafile(const char *str);
char *grfio_setadatafile(const char *str);
char *grfio_setsdatafile(const char *str);

#endif	// _GRFIO_H_
