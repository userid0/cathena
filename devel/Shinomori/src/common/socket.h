// original : core.h 2003/03/14 11:55:25 Rev 1.4

#ifndef	_SOCKET_H_
#define _SOCKET_H_

#include "base.h"
#include "malloc.h"
#include "timer.h"

extern time_t tick_;
extern time_t stall_time_;


///////////////////////////////////////////////////////////////////////////////
// IP number stuff
///////////////////////////////////////////////////////////////////////////////
class ipaddress
{
public:
    union
    {
        uchar   data[4];
        ulong   ldata;
    };
    ipaddress():ldata(INADDR_LOOPBACK)          {}
    ipaddress(ulong a):ldata(a)                 {}
    ipaddress(const ipaddress& a):ldata(a.ldata){}
    ipaddress(int a, int b, int c, int d)
	{
		data[0] = uchar(a);
		data[1] = uchar(b);
		data[2] = uchar(c);
		data[3] = uchar(d);
	}
    ipaddress& operator= (ulong a)              { ldata = a; return *this; }
    ipaddress& operator= (const ipaddress& a)   { ldata = a.ldata; return *this; }
    uchar& operator [] (int i)                  
	{ 
#ifdef CHECK_BOUNDS
		if( (i>=4) || (i<0) )
		{
#ifdef CHECK_EXCEPTIONS
			throw exception_bound("ipaddress: out of bound access");
#else//!CHECK_EXCEPTIONS
			static uchar dummy;
			return dummy=0;
#endif//!CHECK_EXCEPTIONS
		}
#endif//CHECK_BOUNDS
		return data[i]; 
	}
    operator ulong() const                      { return ldata; }
};






#ifdef __cplusplus

// Class for assigning/reading words from a buffer
class objW
{
	unsigned char *ip;
public:
	objW():ip(NULL)										{}
	objW(unsigned char* x):ip(x)						{}
	objW(unsigned char* x,int pos):ip(x+pos)			{}
	objW(char* x):ip((unsigned char*)x)					{}
	objW(char* x,int pos):ip((unsigned char*)(x+pos))	{}

	objW& init(unsigned char* x)			{ip=x;return *this;}
	objW& init(unsigned char* x,int pos)	{ip=x+pos;return *this;}
	objW& init(char* x)						{ip=(unsigned char*)x;return *this;}
	objW& init(char* x,int pos)				{ip=(unsigned char*)(x+pos);return *this;}

	operator unsigned short() const
	{return this->operator()();
	}
	unsigned short operator()()	const
	{
		if(ip)
		{	
			return	 ( ((unsigned short)(ip[0]))        )
					|( ((unsigned short)(ip[1])) << 0x08);
			
		}
		return 0;
	}
	objW& operator=(const objW& objw)
	{
		if(ip && objw.ip)
		{	
			memcpy(ip, objw.ip, 2);
		}
		return *this;
	}
	unsigned short operator=(unsigned short valin)
	{	
		if(ip)
		{
			ip[0] = ((valin & 0x00FF)          );
			ip[1] = ((valin & 0xFF00)  >> 0x08 );
		}
		return valin;
	}
};

// Class for assigning/reading dwords from a buffer
class objL
{
	unsigned char *ip;
public:
	objL():ip(NULL)										{}
	objL(unsigned char* x):ip(x)						{}
	objL(unsigned char* x,int pos):ip(x+pos)			{}
	objL(char* x):ip((unsigned char*)x)					{}
	objL(char* x,int pos):ip((unsigned char*)(x+pos))	{}

	objL& init(unsigned char* x)			{ip=x;return *this;}
	objL& init(unsigned char* x,int pos)	{ip=x+pos;return *this;}
	objL& init(char* x)						{ip=(unsigned char*)x;return *this;}
	objL& init(char* x,int pos)				{ip=(unsigned char*)(x+pos);return *this;}


	operator unsigned long() const
	{return this->operator()();
	}
	unsigned long operator()()	const
	{
		if(ip)
		{	
			return	 ( ((unsigned long)(ip[0]))        )
					|( ((unsigned long)(ip[1])) << 0x08)
					|( ((unsigned long)(ip[2])) << 0x10)
					|( ((unsigned long)(ip[3])) << 0x18);
			
		}
		return 0;
	}

	objL& operator=(const objL& objl)
	{
		if(ip && objl.ip)
		{	
			memcpy(ip, objl.ip, 4);
		}
		return *this;
	}
	unsigned long operator=(unsigned long valin)
	{	
		if(ip)
		{
			ip[0] = (unsigned char)((valin & 0x000000FF)          );
			ip[1] = (unsigned char)((valin & 0x0000FF00)  >> 0x08 );
			ip[2] = (unsigned char)((valin & 0x00FF0000)  >> 0x10 );
			ip[3] = (unsigned char)((valin & 0xFF000000)  >> 0x18 );
		}
		return valin;
	}
};

// Class for assigning/reading words from a buffer 
// without changing byte order
// necessary for IP numbers, which are always in network byte order
// it is implemented this way and I currently won't interfere with that
// even if it is quite questionable

// changes: 
// transfered all IP addresses in the programms to host byte order
// since IPs are transfered in network byte order
// we cannot use objL but its complementary here
class objLIP	
{
	unsigned char *ip;
public:
	objLIP():ip(NULL)									{}
	objLIP(unsigned char* x):ip(x)						{}
	objLIP(unsigned char* x,int pos):ip(x+pos)			{}
	objLIP(char* x):ip((unsigned char*)x)				{}
	objLIP(char* x,int pos):ip((unsigned char*)(x+pos))	{}

	objLIP& init(unsigned char* x)			{ip=x;return *this;}
	objLIP& init(unsigned char* x,int pos)	{ip=x+pos;return *this;}
	objLIP& init(char* x)					{ip=(unsigned char*)x;return *this;}
	objLIP& init(char* x,int pos)			{ip=(unsigned char*)(x+pos);return *this;}

	operator unsigned long() const
	{return this->operator()();
	}
	unsigned long operator()()	const
	{
		if(ip)
		{	
//			register unsigned long tmp;
//			memcpy(&tmp,ip,4);
//			return tmp;
			return	 ( ((unsigned long)(ip[3]))        )
					|( ((unsigned long)(ip[2])) << 0x08)
					|( ((unsigned long)(ip[1])) << 0x10)
					|( ((unsigned long)(ip[0])) << 0x18);


		}
		return 0;
	}
	objLIP& operator=(const objLIP& objl)
	{
		if(ip && objl.ip)
		{	
			memcpy(ip, objl.ip, 4);
		}
		return *this;
	}
	unsigned long operator=(unsigned long valin)
	{	
		if(ip)
		{
//			memcpy(ip, &valin, 4);

			ip[3] = (unsigned char)((valin & 0x000000FF)          );
			ip[2] = (unsigned char)((valin & 0x0000FF00)  >> 0x08 );
			ip[1] = (unsigned char)((valin & 0x00FF0000)  >> 0x10 );
			ip[0] = (unsigned char)((valin & 0xFF000000)  >> 0x18 );
		}
		return valin;
	}
};


// define declaration

#define RFIFOP(fd,pos) (session[fd]->rdata+session[fd]->rdata_pos+(pos))
#define RFIFOB(fd,pos) (*((unsigned char*)(session[fd]->rdata+session[fd]->rdata_pos+(pos))))
#define RFIFOW(fd,pos) (objW(session[fd]->rdata+session[fd]->rdata_pos+(pos)))
#define RFIFOL(fd,pos) (objL(session[fd]->rdata+session[fd]->rdata_pos+(pos)))
#define RFIFOLIP(fd,pos) (objLIP(session[fd]->rdata+session[fd]->rdata_pos+(pos)))
#define RFIFOREST(fd) (session[fd]->rdata_size-session[fd]->rdata_pos)
#define RFIFOSPACE(fd) (session[fd]->max_rdata-session[fd]->rdata_size)
#define RBUFP(p,pos) (((unsigned char*)(p))+(pos))
#define RBUFB(p,pos) (*((unsigned char*)RBUFP((p),(pos))))
#define RBUFW(p,pos) (objW(p,pos))
#define RBUFL(p,pos) (objL(p,pos))
#define RBUFLIP(p,pos) (objLIP(p,pos))

#define WFIFOSPACE(fd) (session[fd]->max_wdata-session[fd]->wdata_size)
#define WFIFOP(fd,pos) (session[fd]->wdata+session[fd]->wdata_size+(pos))
#define WFIFOB(fd,pos) (*((unsigned char*)(session[fd]->wdata+session[fd]->wdata_size+(pos))))
#define WFIFOW(fd,pos) (objW(session[fd]->wdata+session[fd]->wdata_size+(pos)))
#define WFIFOL(fd,pos) (objL(session[fd]->wdata+session[fd]->wdata_size+(pos)))
#define WFIFOLIP(fd,pos) (objLIP(session[fd]->wdata+session[fd]->wdata_size+(pos)))
#define WBUFP(p,pos) (((unsigned char*)(p))+(pos))
#define WBUFB(p,pos) (*((unsigned char*)WBUFP((p),(pos))))
#define WBUFW(p,pos) (objW((p),(pos)))
#define WBUFL(p,pos) (objL((p),(pos)))
#define WBUFLIP(p,pos) (objLIP((p),(pos)))

#else// !__cplusplus

// added ..IP defines for compatibility
#define RFIFOP(fd,pos) (session[fd]->rdata+session[fd]->rdata_pos+(pos))

#define RFIFOB(fd,pos) (*((unsigned char*)(session[fd]->rdata+session[fd]->rdata_pos+(pos))))
#define RFIFOW(fd,pos) (*((unsigned short*)(session[fd]->rdata+session[fd]->rdata_pos+(pos))))
#define RFIFOL(fd,pos) (*((unsigned int*)(session[fd]->rdata+session[fd]->rdata_pos+(pos))))
	
#define RFIFOLIP(fd,pos) (*((unsigned int*)(session[fd]->rdata+session[fd]->rdata_pos+(pos))))

//#define RFIFOSKIP(fd,len) ((session[fd]->rdata_size-session[fd]->rdata_pos-(len)<0) ? (fprintf(stderr,"too many skip\n"),exit(1)) : (session[fd]->rdata_pos+=(len)))
#define RFIFOREST(fd) (session[fd]->rdata_size-session[fd]->rdata_pos)
#define RFIFOSPACE(fd) (session[fd]->max_rdata-session[fd]->rdata_size)
#define RBUFP(p,pos) (((unsigned char*)(p))+(pos))
#define RBUFB(p,pos) (*((unsigned char*)RBUFP((p),(pos))))
#define RBUFW(p,pos) (*((unsigned short*)RBUFP((p),(pos))))
#define RBUFL(p,pos) (*((unsigned int*)RBUFP((p),(pos))))

#define RBUFLIP(p,pos) (*((unsigned int*)RBUFP((p),(pos))))

#define WFIFOSPACE(fd) (session[fd]->max_wdata-session[fd]->wdata_size)
#define WFIFOP(fd,pos) (session[fd]->wdata+session[fd]->wdata_size+(pos))
#define WFIFOB(fd,pos) (*((unsigned char*)(session[fd]->wdata+session[fd]->wdata_size+(pos))))
#define WFIFOW(fd,pos) (*((unsigned short*)(session[fd]->wdata+session[fd]->wdata_size+(pos))))
#define WFIFOL(fd,pos) (*((unsigned int*)(session[fd]->wdata+session[fd]->wdata_size+(pos))))

#define WFIFOLIP(fd,pos) (*((unsigned int*)(session[fd]->wdata+session[fd]->wdata_size+(pos))))

// use function instead of macro.
//#define WFIFOSET(fd,len) (session[fd]->wdata_size = (session[fd]->wdata_size+(len)+2048 < session[fd]->max_wdata) ? session[fd]->wdata_size+len : session[fd]->wdata_size)
#define WBUFP(p,pos) (((unsigned char*)(p))+(pos))
#define WBUFB(p,pos) (*((unsigned char*)WBUFP((p),(pos))))
#define WBUFW(p,pos) (*((unsigned short*)WBUFP((p),(pos))))

#define WBUFL(p,pos) (*((unsigned int*)WBUFP((p),(pos))))

#define WBUFLIP(p,pos) (*((unsigned int*)WBUFP((p),(pos))))

#endif//__cplusplus




#ifdef __INTERIX
#define FD_SETSIZE 4096
#endif	// __INTERIX


/* Removed Cygwin FD_SETSIZE declarations, now are directly passed on to the compiler through Makefile [Valaris] */

// Struct declaration
struct socket_data{
	struct {
		bool connected : 1;	// true when connected
		bool remove : 1;	// true when to be removed
		bool marked : 1;	// true when deleayed removal is initiated (optional)
	}flag;
	unsigned char *rdata;
	int max_rdata;
	int rdata_size;
	int rdata_pos;
	time_t rdata_tick;

	unsigned char *wdata;
	int max_wdata;
	int wdata_size;

	unsigned long client_ip;	// just an ip in host byte order is enough (4byte instead of 16)

	int (*func_recv)(int);
	int (*func_send)(int);
	int (*func_parse)(int);
	int (*func_term)(int);
	int (*func_console)(char*);
	void* session_data;
};

// Data prototype declaration

#ifdef WIN32
		#undef FD_SETSIZE
		#define FD_SETSIZE 4096
#endif

extern struct socket_data *session[FD_SETSIZE];
extern int fd_max;

extern int rfifo_size,wfifo_size;


extern inline bool session_isValid(int fd)
{
	return ( (fd>=0) && (fd<FD_SETSIZE) && (NULL!=session[fd]) );
}
extern inline bool session_isActive(int fd)
{
	return ( session_isValid(fd) && session[fd]->flag.connected );
}
extern inline bool session_isRemoved(int fd)
{
	return ( session_isValid(fd) && session[fd]->flag.remove );
}
extern inline bool session_Remove(int fd)
{
	if( session_isValid(fd) && !session[fd]->flag.marked )
		session[fd]->flag.remove = true;
	return session[fd]->flag.remove;
}
extern inline bool session_SetTermFunction(int fd, int (*term)(int))
{
	if( session_isValid(fd) )
		session[fd]->func_term = term;
	return true;
}

bool session_SetWaitClose(int fd, unsigned long timeoffset);
bool session_Delete(int fd);

// Function prototype declaration

int make_listen    (unsigned long, unsigned short);
int make_connection(unsigned long, unsigned short);

int realloc_fifo(int fd,int rfifo_size,int wfifo_size);
int WFIFOSET(int fd,int len);
int RFIFOSKIP(int fd,int len);

int do_sendrecv(int next);

void socket_init(void);
void socket_final(void);

extern void flush_fifos();
extern void set_nonblocking(int fd, int yes);

int start_console(void);

void set_defaultparse(int (*defaultparse)(int));
void set_defaultconsoleparse(int (*defaultparse)(char*));

extern unsigned long addr_[16];	// ip addresses of local host (host byte order)
extern unsigned int naddr_;		// # of ip addresses


#endif	// _SOCKET_H_
