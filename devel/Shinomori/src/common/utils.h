#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "base.h"

#ifdef _WIN32
	int	strcasecmp(const char *arg1, const char *arg2);
	int	strncasecmp(const char *arg1, const char *arg2, int n);
	void str_upper(char *name);
	void str_lower(char *name);
//    char *rindex(char *str, char c);
#endif


int config_switch(const char *str);
bool e_mail_check(const char *email);
bool remove_control_chars(char *str);

void dump(unsigned char *buffer, int num);

char* checkpath(char *path, const char* src);
void findfile(const char *p, const char *pat, void (func)(const char*) );

extern inline FILE* savefopen(const char*name, const char*option)
{	// windows MAXPATH is 260, unix is longer
	char	 namebuf[2048];
	checkpath(namebuf,name);
	return fopen( namebuf, option);
}


#define CREATE(result, type, number)  do {\
   if ((number) * sizeof(type) <= 0)   \
      ShowError("SYSERR: Zero bytes or less requested at %s:%d.\n", __FILE__, __LINE__);   \
   if (!((result) = (type *)aCalloc((number), sizeof(type))))   \
      { perror("SYSERR: malloc failure"); abort(); } } while(NULL==result)

#define CREATE_A(result, type, number)  do {\
   if ((number) * sizeof(type) <= 0)   \
      ShowError("SYSERR: Zero bytes or less requested at %s:%d.\n", __FILE__, __LINE__);   \
   if (!((result) = (type *) aCallocA ((number), sizeof(type))))   \
      { perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) aRealloc ((result), sizeof(type) * (number))))\
      { ShowError("SYSERR: realloc failure"); abort(); } } while(NULL==result)

struct StringBuf {
	char *buf_;
	char *ptr_;
	unsigned int max_;
};

extern struct StringBuf * StringBuf_Malloc();
extern void StringBuf_Init(struct StringBuf *);
extern int StringBuf_Printf(struct StringBuf *,const char *,...);
extern int StringBuf_Append(struct StringBuf *,const struct StringBuf *);
extern char * StringBuf_Value(struct StringBuf *);
extern void StringBuf_Destroy(struct StringBuf *);
extern void StringBuf_Free(struct StringBuf *);

#endif//COMMON_UTILS_H
