#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H


#ifndef NULL
#define NULL (void *)0
#endif

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

/* strcasecmp -> stricmp -> str_cmp */
#ifdef _WIN32
	int	strcasecmp(const char *arg1, const char *arg2);
	int	strncasecmp(const char *arg1, const char *arg2, int n);
	void str_upper(char *name);
	void str_lower(char *name);
    char *rindex(char *str, char c);
#endif

void dump(unsigned char *buffer, int num);

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

#endif
