// original : core.c 2003/02/26 18:03:12 Rev 1.7

#include "socket.h"
#include "mmo.h"	// [Valaris] thanks to fov
#include "utils.h"
#include "showmsg.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif


#ifdef __cplusplus

#ifdef __GNUC__ 
// message convention for gnu compilers
#warning "INFO: Building C++"
#warning "INFO: This build will work on little and big endian machines"
#else
// message convention for visual c compilers
#pragma message ( "INFO: Building C++" )
#pragma message ( "INFO: This build will work on little and big endian machines" )
#endif

#else// !__cplusplus

#ifdef __GNUC__ 
// message convention for gnu compilers
#warning "INFO: Building standard C"
#warning "INFO: This build will not work on big endian machines even if it compiles successfully, build C++ instead"
#else
// message convention for visual c compilers
#pragma message ( "INFO: Building standard C" )
#pragma message ( "INFO: This build will not work on big endian machines even if it compiles successfully, build C++ instead" )
#endif

#endif//__cplusplus








fd_set readfds;
int fd_max=0;
struct socket_data *session[FD_SETSIZE];


time_t tick_;
time_t stall_time_ = 60;

int rfifo_size = 65536;
int wfifo_size = 65536;

#ifndef TCP_FRAME_LEN
#define TCP_FRAME_LEN 1053
#endif



static int null_parse(int fd);
static int (*default_func_parse)(int) = null_parse;

static int null_console_parse(char *buf);
static int (*default_console_parse)(char*) = null_console_parse;

/*======================================
 *	CORE : Set function
 *--------------------------------------
 */
void set_defaultparse(int (*defaultparse)(int))
{
	default_func_parse = defaultparse;
}

void set_nonblocking(int fd, int yes) {
	setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(char *)&yes,sizeof yes);
}

static void setsocketopts(int fd)
{
	int yes = 1; // reuse fix

	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof yes);
#ifdef SO_REUSEPORT
	setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,(char *)&yes,sizeof yes);
#endif
	set_nonblocking(fd, yes);

	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &wfifo_size , sizeof(rfifo_size ));
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &rfifo_size , sizeof(rfifo_size ));
}

/*======================================
 *	CORE : Socket Sub Function
 *--------------------------------------
 */

void dumpx(unsigned char *buf, int len)
{	int i;
	if(len>0)
	{
		for(i=0; i<len; i++)
			printf("%02X ", buf[i]);
		printf("\n");
	}
}
static int recv_to_fifo(int fd)
{
	int len;

	//ShowMessage("recv_to_fifo : %d %d\n",fd,session[fd]->eof);
	if(session[fd]->eof)
		return -1;


	len=read(fd,(char*)(session[fd]->rdata+session[fd]->rdata_size),RFIFOSPACE(fd));

//	ShowMessage (":::RECEIVE:::\n");
//	dump(session[fd]->rdata, len); printf("\n");
//	dumpx(session[fd]->rdata, len); 

	//{ int i; ShowMessage("recv %d : ",fd); for(i=0;i<len;i++){ ShowMessage("%02x ",RFIFOB(fd,session[fd]->rdata_size+i)); } ShowMessage("\n");}
	if(len>0){
		session[fd]->rdata_size+=len;
		session[fd]->rdata_tick = tick_;
	} else if(len<=0){
		// value of connection is not necessary the same
//		ShowMessage("set eof : connection #%d\n", fd);
		session[fd]->eof=1;
	}
	return 0;
}

static int send_from_fifo(int fd)
{
	int len;

	//ShowMessage("send_from_fifo : %d\n",fd);
	if(session[fd]->eof || session[fd]->wdata == 0)
		return -1;
	if (session[fd]->wdata_size == 0)
		return 0;

	len=write(fd,(char*)(session[fd]->wdata),session[fd]->wdata_size);

//	ShowMessage (":::SEND:::\n");
//	dump(session[fd]->wdata, len); printf("\n");
//	dumpx(session[fd]->wdata, len); 

	//{ int i; ShowMessage("send %d : ",fd);  for(i=0;i<len;i++){ ShowMessage("%02x ",session[fd]->wdata[i]); } ShowMessage("\n");}
	if(len>0){
		if(len<session[fd]->wdata_size){
			memmove(session[fd]->wdata,session[fd]->wdata+len,session[fd]->wdata_size-len);
			session[fd]->wdata_size-=len;
		} else {
			session[fd]->wdata_size=0;
		}
	} else if (errno != EAGAIN) {
//		ShowMessage("set eof :%d\n",fd);
		session[fd]->eof=1;
	}
	return 0;
}

void flush_fifos() 
{
	int i;
	for(i=0;i<fd_max;i++)
		if(session[i] != NULL && 
		   session[i]->func_send == send_from_fifo)
			send_from_fifo(i);
}

static int null_parse(int fd)
{
	ShowMessage("null_parse : %d\n",fd);
	RFIFOSKIP(fd,RFIFOREST(fd));
	return 0;
}

/*======================================
 *	CORE : Socket Function
 *--------------------------------------
 */

static int connect_client(int listen_fd)
{
	int fd;
	struct sockaddr_in client_address;
	int len;

	//ShowMessage("connect_client : %d\n",listen_fd);

	len=sizeof(client_address);

	fd=accept(listen_fd,(struct sockaddr*)&client_address,&len);
//#ifndef WIN32
	// on unix a socket can only be in range from 1 to FD_SETSIZE
	// otherwise it would not fit into the fd_set structure and 
	// overwrite data outside that range, so we have to reject them
	// windows uses a different fd_set structure and is not affected

//++// but the current array structures are indexed from 0 to FD_SETSIZE-1
	// so we have to reject all sockets exceeding this range
	// also on windows
	if(fd >= FD_SETSIZE)
	{
		close(fd);
		return -1;
	}
//#endif

	if(fd_max<=fd) fd_max=fd+1;
	setsocketopts(fd);

	if(fd==-1)
		perror("accept");
	else 
		FD_SET((unsigned int)fd,&readfds);
	socket_nonblocking(fd);

	//ShowMessage("connect_client : %d<-%d\n",listen_fd,fd);


	CREATE(session[fd], struct socket_data, 1);
	CREATE(session[fd]->rdata, unsigned char, rfifo_size);
	CREATE(session[fd]->wdata, unsigned char, wfifo_size);

	session[fd]->max_rdata   = rfifo_size;
	session[fd]->max_wdata   = wfifo_size;
	session[fd]->func_recv   = recv_to_fifo;
	session[fd]->func_send   = send_from_fifo;
	session[fd]->func_parse  = default_func_parse;
	session[fd]->client_addr = client_address;
	session[fd]->rdata_tick = tick_;

  //ShowMessage("new_session : %d %d\n",fd,session[fd]->eof);
	return fd;
}

int make_listen(in_addr_t ip, unsigned short port)
{
	struct sockaddr_in server_address;
	int fd;
	int result;

	fd = socket( AF_INET, SOCK_STREAM, 0 );
//#ifndef WIN32
	// on unix a socket can only be in range from 1 to FD_SETSIZE
	// otherwise it would not fit into the fd_set structure and 
	// overwrite data outside that range, so we have to reject them
	// windows uses a different fd_set structure and is not affected

//++// but the current array structures are indexed from 0 to FD_SETSIZE-1
	// so we have to reject all sockets exceeding this range
	// also on windows
	if(fd >= FD_SETSIZE)
	{
		close(fd);
		return -1;
	}
//#endif
	if(fd_max<=fd) fd_max=fd+1;


	socket_nonblocking(fd);
	setsocketopts(fd);

	server_address.sin_family      = AF_INET;
	server_address.sin_addr.s_addr = ip; // ip is in network byte order
	server_address.sin_port        = htons(port);

	result = bind(fd, (struct sockaddr*)&server_address, sizeof(server_address));
	if( result == -1 ) {
		perror("bind");
		exit(1);
	}
	result = listen( fd, 5 );
	if( result == -1 ) { /* error */
		perror("listen");
		exit(1);
	}

	FD_SET((unsigned int)fd, &readfds );

	CREATE(session[fd], struct socket_data, 1);

	if(session[fd]==NULL){
		ShowMessage("out of memory : make_listen_port\n");
		exit(1);
	}
	memset(session[fd],0,sizeof(*session[fd]));
	session[fd]->func_recv = connect_client;

	return fd;
}



// Console Reciever [Wizputer]
int console_recieve(int i) {
	int n;
	char *buf;
	
	CREATE(buf, char , 64);
	
	memset(buf,0,sizeof(64));

	n = read(0, buf , 64);
	if ( n < 0 )
		ShowMessage("Console input read error\n");
	else
		session[0]->func_console(buf);
	return 0;
}

void set_defaultconsoleparse(int (*defaultparse)(char*))
{
	default_console_parse = defaultparse;
}

static int null_console_parse(char *buf)
{
	ShowMessage("null_console_parse : %s\n",buf);
	return 0;
}

// Console Input [Wizputer]
int start_console(void) {
	FD_SET(0,&readfds);
    
	CREATE(session[0], struct socket_data, 1);
	if(session[0]==NULL){
		ShowMessage("out of memory : start_console\n");
		exit(1);
	}
	
	memset(session[0],0,sizeof(*session[0]));
	
	session[0]->func_recv = console_recieve;
	session[0]->func_console = default_console_parse;
	
	return 0;
}   
    
int make_connection(in_addr_t ip, unsigned short port)
{
	struct sockaddr_in server_address;
	int fd;
	int result;

	fd = socket( AF_INET, SOCK_STREAM, 0 );
//#ifndef WIN32
	// on unix a socket can only be in range from 1 to FD_SETSIZE
	// otherwise it would not fit into the fd_set structure and 
	// overwrite data outside that range, so we have to reject them
	// windows uses a different fd_set structure and is not affected

//++// but the current array structures are indexed from 0 to FD_SETSIZE-1
	// so we have to reject all sockets exceeding this range
	if(fd >= FD_SETSIZE)
	{
		close(fd);
		return -1;
	}
//#endif
	if(fd_max<=fd) fd_max=fd+1;

	setsocketopts(fd);

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = ip; // ip is in network byte order
	server_address.sin_port = htons(port);


	socket_nonblocking(fd);

	result = connect(fd, (struct sockaddr *)(&server_address),sizeof(struct sockaddr_in));

	FD_SET((unsigned int)fd,&readfds);

	CREATE(session[fd], struct socket_data, 1);
	CREATE(session[fd]->rdata, unsigned char, rfifo_size);
	CREATE(session[fd]->wdata, unsigned char, wfifo_size);

	session[fd]->max_rdata  = rfifo_size;
	session[fd]->max_wdata  = wfifo_size;
	session[fd]->func_recv  = recv_to_fifo;
	session[fd]->func_send  = send_from_fifo;
	session[fd]->func_parse = default_func_parse;
	session[fd]->rdata_tick = tick_;

	return fd;
}

int delete_session(int fd)
{
	if(fd<0 || fd>=FD_SETSIZE)
		return -1;
	FD_CLR((unsigned int)fd,&readfds);
	if(session[fd]){
		if(session[fd]->rdata)
			aFree(session[fd]->rdata);
		if(session[fd]->wdata)
			aFree(session[fd]->wdata);
		if(session[fd]->session_data)
			aFree(session[fd]->session_data);
		aFree(session[fd]);
	}
	session[fd]=NULL;
	//ShowMessage("delete_session:%d\n",fd);
	return 0;
}

int realloc_fifo(int fd,int rfifo_size,int wfifo_size)
{
	struct socket_data *s=session[fd];
	if( s->max_rdata != rfifo_size && s->rdata_size < rfifo_size){
		RECREATE(s->rdata, unsigned char, rfifo_size);
		s->max_rdata  = rfifo_size;
	}
	if( s->max_wdata != wfifo_size && s->wdata_size < wfifo_size){
		RECREATE(s->wdata, unsigned char, wfifo_size);
		s->max_wdata  = wfifo_size;
	}
	return 0;
}

int WFIFOSET(int fd,int len)
{
	struct socket_data *s=session[fd];
	if (s == NULL  || s->wdata == NULL)
		return 0;
	if( s->wdata_size+len+16384 > s->max_wdata ){
		unsigned char *sin_addr = (unsigned char *)&s->client_addr.sin_addr;
		realloc_fifo(fd,s->max_rdata, s->max_wdata <<1 );
		ShowMessage("socket: %d (%d.%d.%d.%d) wdata expanded to %d bytes.\n",fd, sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3], s->max_wdata);
	}
	s->wdata_size=(s->wdata_size+(len)+2048 < s->max_wdata) ?
		 s->wdata_size+len : (ShowMessage("socket: %d wdata lost !!\n",fd),s->wdata_size);
	if (s->wdata_size > (TCP_FRAME_LEN)) 
		send_from_fifo(fd);
	return 0;
}



// Find the log base 2 of an N-bit integer in O(lg(N)) operations
// in this case for 32bit input it would be 11 operations
inline unsigned long log2(unsigned long v)
{
//	static const unsigned long b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
//	static const unsigned long S[] = {1, 2, 4, 8, 16};
	// result of log2(v) will go here
	register unsigned long c = 0; 
//	int i;
//	for (i = 4; i >= 0; i--) 
//	{
//	  if (v & b[i])
//	  {
//		v >>= S[i];
//		c |= S[i];
//	  } 
//	}
	// unroll for speed...
//	if (v & b[4]) { v >>= S[4]; c |= S[4]; } 
//	if (v & b[3]) { v >>= S[3]; c |= S[3]; }
//	if (v & b[2]) { v >>= S[2]; c |= S[2]; }
//	if (v & b[1]) { v >>= S[1]; c |= S[1]; }
//	if (v & b[0]) { v >>= S[0]; c |= S[0]; }
	// put values in for more speed...
	if (v & 0xFFFF0000) { v >>= 0x10; c |= 0x10; } 
	if (v & 0x0000FF00) { v >>= 0x08; c |= 0x08; }
	if (v & 0x000000F0) { v >>= 0x04; c |= 0x04; }
	if (v & 0x0000000C) { v >>= 0x02; c |= 0x02; }
	if (v & 0x00000002) { v >>= 0x01; c |= 0x01; }
	return c;
}


inline void process_fdset(fd_set* rfd, fd_set* wfd)
{
#ifdef _WIN32
// the windows fd_set structures are simple
// typedef struct fd_set {
//        u_int fd_count;               /* how many are SET? */
//        SOCKET  fd_array[FD_SETSIZE];   /* an array of SOCKETs */
// } fd_set;
// 
// the select sets the correct fd_count and the array
// so just access the signaled sockets one by one

	size_t i;
	SOCKET fd;
	for(i=0;i<rfd->fd_count;i++)
	{
		fd = rfd->fd_array[i];
		if( session[fd] && (session[fd]->func_recv) )
			session[fd]->func_recv(fd);
	}
	for(i=0;i<wfd->fd_count;i++)
	{
		fd = wfd->fd_array[i];
		if( session[fd] && (session[fd]->func_send) )
			session[fd]->func_send(fd);
	}
#else // some unix, might work on darwin as well
//
// unix uses a bit array where the socket number equals the position in the array
// so finding sockets inside that array is not that easy exept the socket is knows before
// so this method here goes through the bit array and build the socket number 
// from the position where a set bit was found. 
// since we can skip 32 sockets all together when none is set
// we can travel quite fast through the array
//
// it seams on linux you need to compile with __BSD_VISIBLE defined
// to get the internal structures visible .
// anyway, since I can only test on solaris, 
// i cannot tell what other machines would need
// so I set the necessary structures here.
//
// it should be checked if the fd_mask is still an ulong on 64bit machines
// if lucky the compiler will throw a redefinition warning if different

typedef	unsigned long	fd_mask;
#ifndef NBBY
#define	NBBY 8
#endif
#ifndef howmany
#define	howmany(x, y)	(((x) + ((y) - 1)) / (y))
#endif  
#ifndef NFDBITS
#define	NFDBITS	(sizeof (fds_mask) * NBBY)	/* bits per mask */
#endif
#ifndef fds_bits
  #if defined (__fds_bits)
    #define	fds_bits	__fds_bits
  #elif defined (_fds_bits)
    #define	fds_bits	_fds_bits
  #endif
#endif

	SOCKET	fd;
	unsigned long	val;
	unsigned long	bits;
	unsigned long	nfd=0;
	// usually go up to 'howmany(FD_SETSIZE, NFDBITS)'
	unsigned long	max = howmany(fd_max, NFDBITS);
	while( nfd <  max )
	{	// while something is set in the ulong at position nfd
		bits = rfd->fds_bits[nfd];
//		val = 0;
		while( bits )
		{	// calc the highest bit with log2
			// and clear it from the field
			// this method is fast when only a few bits are set in the field
			// which usually happens on read events
			val = log2( bits );
			bits ^= (1<<val);	
			// build the socket number
			fd = nfd*NFDBITS + val;

			// calc the next set bit with shift/add
			// therefore copy the value from fds_bits array to an unsigned type
			// otherwise it would not shift the MSB
//			while( !(bits & 1) )
//			{
//				bits >>= 1;
//				val ++;
//			}
			//calculate the socket number
//			fd = nfd*FD_NFDBITS + val;
			// shift one more for the next loop entrance
//			bits >>= 1;
//			val ++;


			///////////////////////////////////////////////////
			// call the user function
			if( session[fd] && session[fd]->func_recv )
				session[fd]->func_recv(fd);
			///////////////////////////////////////////////////

// unfortunately the return value of select is useless
// and cannot be used as global count
// so we have to go up to the end of the array
//			num--;
//			if(num==0) goto FINISH;
		}
		// go to next field position
		nfd++;
	}
//	FINISH: return;

	// vars are declared above already
	nfd=0;
	while( nfd < max  )
	{	// while something is set in the ulong at position nfd
		bits = wfd->fds_bits[nfd];
		val = 0;
		while( bits )
		{	// calc the highest bit 
			// and clear it from the field
//			val = log2( bits );
//			bits ^= (1<<val);	
//			fd = nfd*NFDBITS + val;

			// calc the next set bit with shift/add
			// therefore copy the value from fds_bits array to an unsigned type
			// (fd_bits is an field of long)
			// otherwise it would not shift the MSB
			// the shift add method is faster if many bits are set in the field
			// which is usually valid for write operations
			while( !(bits & 1) )
			{
				bits >>= 1;
				val ++;
			}
			//calculate the socket number
			fd = nfd*FD_NFDBITS + val;
			// shift one more for the next loop entrance
			bits >>= 1;
			val ++;

			///////////////////////////////////////////////////
			// call the user function
			if( session[fd] && (session[fd]->func_send) )
				session[fd]->func_send(fd);
			///////////////////////////////////////////////////

// unfortunately the return value of select is useless
// and cannot be used as global counter
// so we have to go up to the end of the array
//			num--;
//			if(num==0) goto FINISH;
		}
		// go to next field position
		nfd++;
	}
//	FINISH: return;
#endif
}


int do_sendrecv(int next)
{
	fd_set rfd,wfd;
	struct timeval timeout;
	int ret,i;

	tick_ = time(0);

	memcpy(&rfd, &readfds, sizeof(rfd));

	FD_ZERO(&wfd);
	for(i=0;i<fd_max;i++){
		if(!session[i] && FD_ISSET(i,&readfds)){
			ShowMessage("force clr fds %d\n",i);
			FD_CLR((unsigned int)i,&readfds);
			continue;
		}
		if(!session[i])
			continue;
		if(session[i]->wdata_size)
			FD_SET((unsigned int)i,&wfd);
	}
	timeout.tv_sec  = next/1000;
	timeout.tv_usec = next%1000*1000;
	ret = select(fd_max,&rfd,&wfd,NULL,&timeout);
	if(ret<=0)
		return 0;

	process_fdset(&rfd,&wfd);

/*

	for(i=0;i<fd_max;i++){
		if(!session[i])
			continue;
		if(FD_ISSET(i,&wfd)){
			//ShowMessage("write:%d\n",i);
			if(session[i]->func_send)
				session[i]->func_send(i);
		}
		if(FD_ISSET(i,&rfd)){
			//ShowMessage("read:%d\n",i);
			if(session[i]->func_recv)
				session[i]->func_recv(i);
		}
	}
*/



	return 0;
}

int do_parsepacket(void)
{
	int i;
	for(i=0;i<fd_max;i++){
		if(!session[i])
			continue;
		if ((session[i]->rdata_tick != 0) && ((tick_ - session[i]->rdata_tick) > stall_time_)) 
			session[i]->eof = 1;
		if(session[i]->rdata_size==0 && session[i]->eof==0)
			continue;
		if(session[i]->func_parse){
			session[i]->func_parse(i);
			if(!session[i])
				continue;
		}
		RFIFOFLUSH(i);
	}
	return 0;
}

void do_socket(void)
{
	FD_ZERO(&readfds);
}

int RFIFOSKIP(int fd,int len)
{
	struct socket_data *s=session[fd];

	if (s->rdata_size-s->rdata_pos-len<0) {
		fprintf(stderr,"too many skip\n");
		exit(1);
	}

	s->rdata_pos = s->rdata_pos+len;

	return 0;
}


unsigned long addr_[16];   // ip addresses of local host (host byte order)
unsigned int naddr_ = 0;   // # of ip addresses

int  Net_Init(void)
{
#ifdef _WIN32
	char** a;
	unsigned int i;
	char fullhost[255];
	struct hostent* hent;
	
	/* Start up the windows networking */
	WSADATA wsaData;
	if ( WSAStartup(WINSOCK_VERSION, &wsaData) != 0 ) {
		ShowMessage("SYSERR: WinSock not available!\n");
		exit(1);
	}
	
	if(gethostname(fullhost, sizeof(fullhost)) == SOCKET_ERROR) {
		ShowMessage("Ugg.. no hostname defined!\n");
		return 0;
	} 
	
	// XXX This should look up the local IP addresses in the registry
	// instead of calling gethostbyname. However, the way IP addresses
	// are stored in the registry is annoyingly complex, so I'll leave
	// this as T.B.D.
	hent = gethostbyname(fullhost);
	if (hent == NULL) {
		ShowMessage("Cannot resolve our own hostname to a IP address");
		return 0;
	}
	a = hent->h_addr_list;
	for(i = 0; a[i] != NULL && i < 16; ++i) {
		unsigned long addr1 = ntohl(RBUFL(a[i],0));
		addr_[i] = addr1;
	}
	naddr_ = i;
#else//not W32
	int pos;
	int fdes = socket(AF_INET, SOCK_STREAM, 0);
	char buf[16 * sizeof(struct ifreq)];
	struct ifconf ic;
	
	// The ioctl call will fail with Invalid Argument if there are more
	// interfaces than will fit in the buffer
	ic.ifc_len = sizeof(buf);
	ic.ifc_buf = buf;
	if(ioctl(fdes, SIOCGIFCONF, &ic) == -1) {
		ShowMessage("SIOCGIFCONF failed!\n");
		return 0;
	}
	for(pos = 0; pos < ic.ifc_len;   )
	{
		struct ifreq * ir = (struct ifreq *) (ic.ifc_buf + pos);
		struct sockaddr_in * a = (struct sockaddr_in *) &(ir->ifr_addr);
		
		if(a->sin_family == AF_INET) {
			u_long ad = ntohl(a->sin_addr.s_addr);
			if(ad != INADDR_LOOPBACK) {
				addr_[naddr_ ++] = ad;
				if(naddr_ == 16)
					break;
			}
		}
#if defined(_AIX) || defined(__APPLE__)
		pos += ir->ifr_addr.sa_len;  // For when we port athena to run on Mac's :)
		pos += sizeof(ir->ifr_name);
#else// not AIX or APPLE
		pos += sizeof(struct ifreq);
#endif//not AIX or APPLE
	}
#endif//not W32
	return(0);
}
