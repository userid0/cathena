// original : core.c 2003/02/26 18:03:12 Rev 1.7


#include "socket.h"
#include "mmo.h"	// [Valaris] thanks to fov
#include "utils.h"
#include "showmsg.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif


//#define SOCKET_DEBUG_PRINT

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
#warning "INFO: Since I generally changed IP byte order this build will not work at all, build C++ instead"
#else
// message convention for visual c compilers
#pragma message ( "INFO: Building standard C" )
#pragma message ( "INFO: This build will not work on big endian machines even if it compiles successfully, build C++ instead" )
#pragma message ( "INFO: Since I generally changed IP byte order this build will not work at all, build C++ instead" )
#endif

#endif//__cplusplus


///////////////////////////////////////////////////////////////////////////////
#ifndef WIN32
// defines for direct access of fd_set data on unix
//
// it seams on linux you need to compile with __BSD_VISIBLE 
// defined to get the internal structures visible.
// anyway, since I can only test on solaris, 
// i cannot tell what other machines would need
// so I explicitely set the necessary structures here.
// and you would not need to redefine anything
//
// anyway, if compiler warning/error occure here, try to comment out 
// the quested thing and please give a report
//
// it should be checked if the fd_mask is still an ulong on 64bit machines 
// if lucky the compiler will throw a redefinition warning if different
// further, 64 bit unix defines FD_SETSIZE with 65535 so maybe reconsider 
// the current structures

//typedef	unsigned long	fd_mask;
#ifndef NBBY
#define	NBBY 8
#endif
#ifndef howmany
#define	howmany(x, y)	(((x) + ((y) - 1)) / (y))
#endif  
#ifndef NFDBITS
#define	NFDBITS	(sizeof (unsigned long) * NBBY)	/* bits per mask */
#endif
#ifndef fds_bits
  #if defined (__fds_bits)
    #define	fds_bits	__fds_bits
  #elif defined (_fds_bits)
    #define	fds_bits	_fds_bits
  #endif
#endif

#endif
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
unsigned long ticks;
time_t tick_;
time_t stall_time_ = 60;
///////////////////////////////////////////////////////////////////////////////

int rfifo_size = 65536;
int wfifo_size = 65536;

///////////////////////////////////////////////////////////////////////////////
#ifndef TCP_FRAME_LEN
#define TCP_FRAME_LEN 1053
#endif



///////////////////////////////////////////////////////////////////////////////
//
// Socket <-> Session Handling
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// array of session data
struct socket_data *session[FD_SETSIZE];
// number of used array elements

fd_set readfds;		// in WIN32 this contains a sorted list of the sockets

int fd_max=0;		// greatest used session fd in the field
					// it is also used for the select call on Berkeley sockets
					// but it could be replaced with FD_SETSIZE there
///////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
// extra data for windows because windows gives only system wide sockets
// so you can fast exceed the FD_SETSIZE

// list of session positions corrosponding to fd_set list
// so fist search a socket in readfds with binary search to get a position
// and then look up the corrosponding fd for session[fd]
size_t session_pos[FD_SETSIZE];	

// list of sockets corrosponding to session list
// socket_pos[fd] is the SOCKET of the session[fd]
SOCKET	socket_pos[FD_SETSIZE];	


///////////////////////////////////////////////////////////////////////////////
// binary search implementation
// might be not that efficient in this implementation
// it first checks the boundaries so calls outside 
// the list range are handles much faster 
// at the expence of some extra code
// runtime behaviour much faster if often called for outside data
///////////////////////////////////////////////////////////////////////////////
bool SessionBinarySearch(const SOCKET elem, size_t *retpos)
{	// do a binary search with smallest first
	// make some initial stuff
	bool ret = false;
	size_t a=0, b=readfds.fd_count-1, c;
	size_t pos = 0;

	// just to be sure we have to do something
	if( readfds.fd_count==0 )
	{	ret = false;
	}
	else if( elem < readfds.fd_array[a] )
	{	// less than lower
		pos = a;
		ret = false; 
	}
	else if( elem > readfds.fd_array[b] )
	{	// larger than upper
		pos = b+1;
		ret = false; 
	}
	else if( elem == readfds.fd_array[a] )
	{	// found at first position
		pos=a;
		ret = true;
	}
	else if( elem == readfds.fd_array[b] )
	{	// found at last position
		pos = b;
		ret = true;
	}
	else
	{	// binary search
		// search between first and last
		do
		{
			c=(a+b)/2;
			if( elem == readfds.fd_array[c] )
			{	// found it
				b=c;
				ret = true;
				break;
			}
			else if( elem < readfds.fd_array[c] )
				b=c;
			else
				a=c;
		}while( (a+1) < b );
		pos = b;
		// return the next larger element to the given 
		// or the found element so we can insert a new element there
	}
	// just make sure we call this with a pointer,
	// on c++ it would be a reference an you could live without NULL check...
	if(retpos) *retpos = pos;
	return ret;
}
bool SessionFindSocket(const SOCKET elem, size_t *retpos)
{
	size_t pos;
	if( SessionBinarySearch(elem, &pos) )
	{
		if(retpos) *retpos = session_pos[pos];
		return true;
	}
	return false;
}

int SessionInsertSocket(const SOCKET elem)
{
	size_t pos,fd;
	if(fd_max<FD_SETSIZE) // max number of allowed sockets
	if( !SessionBinarySearch(elem, &pos) )
	{
		if((size_t)fd_max!=pos)
		{
			memmove( session_pos     +pos+1, session_pos     +pos, (readfds.fd_count-pos)*sizeof(size_t));
			memmove( readfds.fd_array+pos+1, readfds.fd_array+pos, (readfds.fd_count-pos)*sizeof(SOCKET));
		}

		// find an empty position in session[]
		for(fd=0; fd<FD_SETSIZE; fd++)
			if( NULL==session[fd] )
				break;
		// we might not need to test validity 
		// since there must have been an empty place
		session_pos[pos]      = fd;
		readfds.fd_array[pos] = elem;
		readfds.fd_count++;

		if((size_t)fd_max<=fd)	fd_max = fd+1;
		socket_pos[fd] = elem;	// corrosponding to session[fd]

		return (int)fd;
	}
	// otherwise the socket is already in the list
	return -1;
}

bool SessionRemoveSocket(const SOCKET elem)
{
	size_t pos;
	if( SessionBinarySearch(elem, &pos) )
	{
		// be sure to clear session[]
		// we do not care for that here

		closesocket(elem);
		socket_pos[session_pos[pos]] = (SOCKET)-1;

		memmove( session_pos     +pos, session_pos     +pos+1, (readfds.fd_count-pos-1)*sizeof(size_t));
		memmove( readfds.fd_array+pos, readfds.fd_array+pos+1, (readfds.fd_count-pos-1)*sizeof(SOCKET));
		readfds.fd_count--;
		return true;
	}
	// otherwise the socket is not in the list
	return false;
}

bool SessionRemoveIndex(const size_t fd)
{
	if( fd < FD_SETSIZE )
		return SessionRemoveSocket( socket_pos[fd] );
	return false;
}
SOCKET SessionGetSocket(const size_t fd)
{
	if( fd < FD_SETSIZE )
		return socket_pos[fd];
	return (SOCKET)-1;
}
///////////////////////////////////////////////////////////////////////////////
#else//! WIN32
///////////////////////////////////////////////////////////////////////////////
bool SessionFindSocket(const SOCKET elem, size_t *retpos)
{	// socket and position are identical
	if(elem < FD_SETSIZE)
	{
		if(retpos) *retpos = (size_t)elem;
		return true;
	}
	return false;
}
int SessionInsertSocket(const SOCKET elem)
{
	if(elem > 0 && elem < FD_SETSIZE)
	{
		FD_SET(elem,&readfds);
		// need for select
		// we reduce this on building the writefds when necessary
		if(fd_max<=elem) fd_max = elem+1; 
	}
	return elem;
}
bool SessionRemoveSocket(const SOCKET elem)
{	// socket and position are identical
	if(elem > 0 && elem < FD_SETSIZE)
	{
		FD_CLR(elem,&readfds);
		closesocket(elem);
	}
	return true;
}
bool SessionRemoveIndex(const size_t pos)
{	// socket and position are identical
	if(pos > 0 && pos < FD_SETSIZE)
	{
		FD_CLR(pos,&readfds);
		closesocket(pos);
	}
	return true;
}
SOCKET SessionGetSocket(const size_t pos)
{	// socket and position are identical
	if(pos < FD_SETSIZE)
		return (SOCKET)pos;
	return (SOCKET)-1;
}
///////////////////////////////////////////////////////////////////////////////
#endif//! WIN32
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
//
// Parse Functions
//
///////////////////////////////////////////////////////////////////////////////
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


int null_parse(int fd)
{
	ShowMessage("null_parse : %d\n",fd);
	RFIFOSKIP(fd,RFIFOREST(fd));
	return 0;
}





///////////////////////////////////////////////////////////////////////////////
//
// Socket Control
//
///////////////////////////////////////////////////////////////////////////////

void socket_nonblocking(SOCKET sock, unsigned long yes) {
	// TCP_NODELAY BOOL Disables the Nagle algorithm for send coalescing. 
	//setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(char *)&yes,sizeof yes);
	
	// FIONBIO Use with a nonzero argp parameter to enable the nonblocking mode of socket s. 
	// The argp parameter is zero if nonblocking is to be disabled. 
	ioctlsocket(sock, FIONBIO, &yes); 
}

void socket_setopts(SOCKET sock)
{
	int yes = 1; // reuse fix

	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof yes);
#ifdef SO_REUSEPORT
	setsockopt(sock,SOL_SOCKET,SO_REUSEPORT,(char *)&yes,sizeof yes);
#endif

	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *) &wfifo_size , sizeof(rfifo_size ));
	setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &rfifo_size , sizeof(rfifo_size ));
}



///////////////////////////////////////////////////////////////////////////////
//
// FIFO Reading/Sending Functions
//
///////////////////////////////////////////////////////////////////////////////
#ifdef SOCKET_DEBUG_PRINT
void dumpx(unsigned char *buf, int len)
{	int i;
	if(len>0)
	{
		for(i=0; i<len; i++)
			printf("%02X ", buf[i]);
	}
	printf("\n");
}
#endif

int recv_to_fifo(int fd)
{
	int len;
	unsigned long arg = 0;
	int rv;

	//ShowMessage("recv_to_fifo : %d\n",fd);
	if( !session_isActive(fd) )
		return -1;

	// check for incoming data and the amount
	rv = ioctlsocket(SessionGetSocket(fd), FIONREAD, &arg);
	if( (rv == 0) && (arg > 0) )
	{	// we are reading 'arg' bytes of data from the socket
		if( arg > (unsigned long)RFIFOSPACE(fd) ) arg = RFIFOSPACE(fd);

		len=read(SessionGetSocket(fd),(char*)(session[fd]->rdata+session[fd]->rdata_size),arg);

//		ShowMessage (":::RECEIVE:::\n");
//		dump(session[fd]->rdata, len); ShowMessage ("\n");
#ifdef SOCKET_DEBUG_PRINT
		dumpx(session[fd]->rdata, len);
#endif
		if(len>0){
			session[fd]->rdata_size+=len;
			session[fd]->rdata_tick = tick_;
		} else if(len<=0){
			session[fd]->flag.connected = false;
		}
	} else {	
		// the socket has been terminated
		session[fd]->flag.connected = false;
	}
	return 0;
}

int send_from_fifo(int fd)
{
	int len;

	//ShowMessage("send_from_fifo : %d\n",fd);
	if(fd<0 || fd > FD_SETSIZE || !session[fd])
		return -1;

	if (session[fd]->wdata_size == 0)
		return 0;

	// clear buffer if not connected
	if( !session[fd]->flag.connected )
	{
		session[fd]->wdata_size = 0;
		return 0;
	}

	len=write(SessionGetSocket(fd),(char*)(session[fd]->wdata),session[fd]->wdata_size);

//	ShowMessage (":::SEND:::\n");
//	dump(session[fd]->wdata, len); ShowMessage ("\n");
#ifdef SOCKET_DEBUG_PRINT
	dumpx(session[fd]->wdata, len);
#endif
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
		session[fd]->flag.connected = false;	
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////////
//
// Fifo Control
//
///////////////////////////////////////////////////////////////////////////////

void flush_fifos() 
{
	// write fifos and be sure the data in on the run

	// easy method but needs a socket mode switch
	// and processes each write individually
/*	int fd;
	for(fd=0; fd<fd_max; fd++)
	{
		if( session_isActive(fd) && session[fd]->func_send == send_from_fifo && session[fd]->wdata_size > 0)
		{

			socket_nonblocking(SessionGetSocket(fd),0); // set blocking
			send_from_fifo(fd); // send data
			socket_nonblocking(SessionGetSocket(fd),1); // set non blocking
		}
	}
*/

	// more complex method which might be faster in the end
	int fd, len, c;
	fd_set wfd;
	
	while(1)
	{
		memset(&wfd,0,sizeof(fd_set));
		c=0;
		for(fd=0; fd<fd_max; fd++)
		{
			if( session_isActive(fd) && session[fd]->func_send == send_from_fifo && session[fd]->wdata_size > 0)
			{
				// try to write the data nonblocking
				len=write(SessionGetSocket(fd),(char*)(session[fd]->wdata),session[fd]->wdata_size);

				//ShowMessage (":::SEND:::\n");
				//dump(session[fd]->wdata, len); ShowMessage ("\n");
#ifdef SOCKET_DEBUG_PRINT
				dumpx(session[fd]->wdata, len);
#endif
				if(len>0)
				{
					if(len < session[fd]->wdata_size)
					{
						memmove(session[fd]->wdata,session[fd]->wdata+len,session[fd]->wdata_size-len);
						session[fd]->wdata_size-=len;

						// we have a problem, not all data has been send
						// so we mark that socket
						FD_SET(SessionGetSocket(fd), &wfd);
						c++;
					}
					else
					{	// everything is ok and on the run
						session[fd]->wdata_size=0;
					}
				} else if (errno != EAGAIN) 
				{	// the socket is gone, just disconnect it, will be cleared later
					session[fd]->flag.connected = false;	
				}
				else // EAGAIN
				{	// try it again later; might lead to infinity loop when connections is generally blocked
					FD_SET(SessionGetSocket(fd), &wfd);
					c++;
				}
			}
		}
		// finish if all has been send and no need to wait longer
		if(c==0) break; 
		// otherwise wait until marked write sockets can exept more data 
		select(fd_max,NULL,&wfd,NULL,NULL);
	}//end while
}
int realloc_fifo(int fd,int rfifo_size,int wfifo_size)
{
	struct socket_data *s;
	
	if( !session_isActive(fd) )
		return 0;

	s =session[fd];

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
	struct socket_data *s;
	
	if( !session_isActive(fd) )
		return 0;

	s =session[fd];
	if (s == NULL  || s->wdata == NULL)
		return 0;
	if( s->wdata_size+len+16384 > s->max_wdata ){
		//unsigned char *sin_addr = (unsigned char *)&s->client_addr.sin_addr;
		unsigned long ip = s->client_ip;
		realloc_fifo(fd,s->max_rdata, s->max_wdata <<1 );
		//ShowMessage("socket: %d (%d.%d.%d.%d) wdata expanded to %d bytes.\n",fd, sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3], s->max_wdata);
		ShowMessage("socket: %d (%d.%d.%d.%d) wdata expanded to %d bytes.\n",fd, (ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, (ip)&0xFF, s->max_wdata);
	}
	s->wdata_size=(s->wdata_size+(len)+2048 < s->max_wdata) ?
		 s->wdata_size+len : (ShowMessage("socket: %d wdata lost !!\n",fd),s->wdata_size);
	if (s->wdata_size > (TCP_FRAME_LEN)) 
		send_from_fifo(fd);
	return 0;
}

int RFIFOSKIP(int fd, int len)
{
	struct socket_data *s=session[fd];

	if (s->rdata_size < s->rdata_pos + len) {
		fprintf(stderr,"too many skip\n");
		exit(1);
	}
	s->rdata_pos += len;
	return 0;
}



///////////////////////////////////////////////////////////////////////////////
//
// Creating Sockets and Sessions
//
///////////////////////////////////////////////////////////////////////////////
static int connect_client(int listen_fd)
{
	SOCKET sock;
	int fd;
	struct sockaddr_in client_address;
	socklen_t len = sizeof(client_address);

	// quite dangerous, app is dead in this case
	if( !session_isActive(listen_fd) )
		return -1;

	sock = accept(SessionGetSocket(listen_fd),(struct sockaddr*)&client_address,&len);
	if(sock==-1) 
	{	// same here, app might have passed away
		perror("accept");
		return -1;
	}
#ifndef WIN32
	// on unix a socket can only be in range from 1 to FD_SETSIZE
	// otherwise it would not fit into the fd_set structure and 
	// overwrite data outside that range, so we have to reject them
	// windows uses a different fd_set structure and is not affected
	if(sock > FD_SETSIZE)
	{
		closesocket(sock);
		return -1;
	}
#endif
	socket_setopts(sock);
	socket_nonblocking(sock,1);
	// insert the socket to the fields and get the position
	fd = SessionInsertSocket(sock);

	//ShowMessage("connect_client: %d <- %d\n",listen_fd, fd);

	if(fd<0) 
	{	
		closesocket(sock);
		ShowWarning("socket insert");
		return -1;
	}

	CREATE(session[fd], struct socket_data, 1);
	CREATE(session[fd]->rdata, unsigned char, rfifo_size);
	CREATE(session[fd]->wdata, unsigned char, wfifo_size);

	session[fd]->flag.connected   = true;
	session[fd]->flag.remove      = false;
	session[fd]->flag.marked      = false;

	session[fd]->max_rdata   = rfifo_size;
	session[fd]->max_wdata   = wfifo_size;
	session[fd]->func_recv   = recv_to_fifo;
	session[fd]->func_send   = send_from_fifo;
	session[fd]->func_parse  = default_func_parse;
	session[fd]->client_ip	 = ntohl(client_address.sin_addr.s_addr);
	session[fd]->rdata_tick  = tick_;

	ShowStatus("Incoming connection from %d.%d.%d.%d:%i\n",
		(session[fd]->client_ip>>24)&0xFF,(session[fd]->client_ip>>16)&0xFF,(session[fd]->client_ip>>8)&0xFF,(session[fd]->client_ip)&0xFF,
		ntohs(client_address.sin_port),fd );
	return fd;
}

int make_listen(unsigned long ip, unsigned short port)
{
	struct sockaddr_in server_address;
	SOCKET sock;
	int fd;
	int result;

	sock = socket( AF_INET, SOCK_STREAM, 0 );
#ifndef WIN32
	// on unix a socket can only be in range from 1 to FD_SETSIZE
	// otherwise it would not fit into the fd_set structure and 
	// overwrite data outside that range, so we have to reject them
	// windows uses a different fd_set structure and is not affected
	if(sock > FD_SETSIZE)
	{
		closesocket(sock);
		return -1;
	}
#endif
	socket_setopts(sock);
	socket_nonblocking(sock,1);
	server_address.sin_family      = AF_INET;
	server_address.sin_addr.s_addr = htonl( ip );
	server_address.sin_port        = htons(port);

	result = bind(sock, (struct sockaddr*)&server_address, sizeof(server_address));
	if( result == -1 ) {
		closesocket(sock);
		perror("bind");
		exit(1);
	}
	result = listen( sock, 5 );
	if( result == -1 ) {
		closesocket(sock);
		perror("listen");
		exit(1);
	}

	// insert the socket to the fields and get the position
	fd = SessionInsertSocket(sock);

	if(fd<0) 
	{	
		closesocket(sock);
		ShowWarning("Socket Insert failed");
		return -1;
	}

	CREATE(session[fd], struct socket_data, 1);
	memset(session[fd],0,sizeof(*session[fd]));

	session[fd]->flag.connected   = true;
	session[fd]->flag.remove      = false;
	session[fd]->flag.marked      = false;

	session[fd]->func_recv   = connect_client;

	ShowStatus("Open listen port on %d.%d.%d.%d:%i\n",(ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,(ip)&0xFF,port);
	return fd;
}
int make_connection(unsigned long ip, unsigned short port)
{
	struct sockaddr_in server_address;
	int fd;
	SOCKET sock;
	int result;

	sock = socket( AF_INET, SOCK_STREAM, 0 );
#ifndef WIN32
	// on unix a socket can only be in range from 1 to FD_SETSIZE
	// otherwise it would not fit into the fd_set structure and 
	// overwrite data outside that range, so we have to reject them
	// windows uses a different fd_set structure and is not affected
	if(sock > FD_SETSIZE)
	{
		closesocket(sock);
		return -1;
	}
#endif
	socket_setopts(sock);

	server_address.sin_family		= AF_INET;
	server_address.sin_addr.s_addr	= htonl( ip );
	server_address.sin_port			= htons(port);
	
	ShowStatus("Connecting to %d.%d.%d.%d:%i\n",(ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,(ip)&0xFF,port);

	result = connect(sock, (struct sockaddr *)(&server_address),sizeof(struct sockaddr_in));

	// set nonblocking after connecting so we can use the blocking timeout
	// connecting nonblocking sockets would need to wait until connection is established
	socket_nonblocking(sock,1);
	if( result < 0 )
	{
		closesocket(sock);
		ShowWarning("Connect failed\n");
		return -1;
	}
	else
		ShowStatus("Connect ok\n");

	// insert the socket to the fields and get the position
	fd = SessionInsertSocket(sock);

	if(fd<0) 
	{	
		closesocket(sock);
		ShowWarning("Socket Insert failed");
		return -1;
	}

	CREATE(session[fd], struct socket_data, 1);
	CREATE(session[fd]->rdata, unsigned char, rfifo_size);
	CREATE(session[fd]->wdata, unsigned char, wfifo_size);

	session[fd]->flag.connected   = true;
	session[fd]->flag.remove      = false;
	session[fd]->flag.marked      = false;

	session[fd]->max_rdata  = rfifo_size;
	session[fd]->max_wdata  = wfifo_size;
	session[fd]->func_recv  = recv_to_fifo;
	session[fd]->func_send  = send_from_fifo;
	session[fd]->func_parse = default_func_parse;
	session[fd]->rdata_tick = tick_;

	return fd;
}

///////////////////////////////////////////////////////////////////////////////
//
// Console Reciever [Wizputer]
//
///////////////////////////////////////////////////////////////////////////////

int console_recieve(int fd) {
	int n;
	char buf[64];
	memset(buf,0,sizeof(64));

	if( !session_isActive(fd) )
		return -1;

	n = read(fd, buf , 64);
	if( (n < 0) || !session[fd]->func_console )
		ShowMessage("Console input read error\n");
	else if(session[fd]->func_console)
		session[fd]->func_console(buf);
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
	
	SOCKET sock=0; // default zero socket, is not used regulary
	size_t fd;
    
	// insert the socket to the fields and get the position
	fd = SessionInsertSocket(sock);

	if(fd<0) 
	{	ShowWarning("Socket Insert failed");
		return -1;
	}

	CREATE(session[fd], struct socket_data, 1);
	
	session[fd]->flag.connected   = true;
	session[fd]->flag.remove      = false;
	session[fd]->flag.marked      = false;

	session[fd]->func_recv    = console_recieve;
	session[fd]->func_console = default_console_parse;
	
	return 0;
}   

///////////////////////////////////////////////////////////////////////////////
//
// Incoming Data Processing
//
///////////////////////////////////////////////////////////////////////////////
void process_read(size_t fd)
{
	if( session_isValid(fd) )
	{
		if( session[fd]->func_recv )
			session[fd]->func_recv((int)fd);

		if(session[fd]->rdata_size==0 && session[fd]->flag.connected)
			return;

		if( session[fd]->func_parse )
			session[fd]->func_parse((int)fd);

		// session could be deleted in func_parse so better check again
		if(session[fd]) 
		{	//RFIFOFLUSH(fd);
			memmove(session[fd]->rdata, RFIFOP(fd,0), RFIFOREST(fd));
			session[fd]->rdata_size = RFIFOREST(fd);
			session[fd]->rdata_pos  = 0;
		}

	}
}
///////////////////////////////////////////////////////////////////////////////
void process_write(size_t fd)
{
	if( session_isValid(fd) )
	{
		if( session[fd]->func_send )
			session[fd]->func_send((int)fd);
	}
}

///////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
///////////////////////////////////////////////////////////////////////////////
// the windows fd_set structures are simple
// typedef struct fd_set {
//        u_int fd_count;               // how many are SET?
//        SOCKET  fd_array[FD_SETSIZE];   // an array of SOCKETs
// } fd_set;
// 
// the select the sets, the correct fd_count and the array
// and access the signaled sockets one by one
size_t process_fdset(fd_set* fds, void(*func)(size_t) )
{
	size_t i;
	size_t fd;

	if(func)
	for(i=0;i<fds->fd_count;i++)
	{
		if( SessionFindSocket( fds->fd_array[i], &fd ) )
		{
			func(fd);
		}
	}
	return fds->fd_count;
}
// just a dummy to keep win/unix structure consistent
extern inline size_t process_fdset2(fd_set* fds, void(*func)(size_t) )
{	return process_fdset(fds, func);
}
///////////////////////////////////////////////////////////////////////////////
#else//!WIN32
///////////////////////////////////////////////////////////////////////////////
//
// unix uses a bit array where the socket number equals the 
// position in the array, so finding sockets inside that array 
// is not that easy exept the socket is knows before
// so this method here goes through the bit array 
// and build the socket number from the position 
// where a set bit was found. 
// since we can skip 32 sockets all together when none is set
// we can travel quite fast through the array
size_t process_fdset(fd_set* fds, void(*func)(size_t) )
{
	size_t fd,c=0;
	SOCKET sock;
	unsigned long	val;
	unsigned long	bits;
	unsigned long	nfd=0;
	// usually go up to 'howmany(FD_SETSIZE, NFDBITS)' for the whole array
	unsigned long	max = howmany(fd_max, NFDBITS);

	if(func)
	while( nfd <  max )
	{	// while something is set in the ulong at position nfd
		bits = fds->fds_bits[nfd];
		while( bits )
		{	// method 1
			// calc the highest bit with log2 and clear it from the field
			// this method is especially fast 
			// when only a few bits are set in the field
			// which usually happens on read events
			val = log2( bits );
			bits ^= (1<<val);	
			// build the socket number
			sock = nfd*NFDBITS + val;

			///////////////////////////////////////////////////
			// call the user function
			fd = sock; // should use SessionFindSocket for consitency but this is ok
			func(fd);
			c++;
		}
		// go to next field position
		nfd++;
	}
	return c;
}

size_t process_fdset2(fd_set* fds, void(*func)(size_t) )
{
	size_t fd, c=0;
	SOCKET sock;
	unsigned long	val;
	unsigned long	bits;
	unsigned long	nfd=0;
	// usually go up to 'howmany(FD_SETSIZE, NFDBITS)'
	unsigned long	max = howmany(fd_max, NFDBITS);

	if(func)
	while( nfd <  max )
	{	// while something is set in the ulong at position nfd
		bits = fds->fds_bits[nfd];
		val = 0;
		while( bits )
		{	// method 2
			// calc the next set bit with shift/add
			// therefore copy the value from fds_bits 
			// array to an unsigned type (fd_bits is an field of long)
			// otherwise it would not shift the MSB
			// the shift add method is faster if many bits are set in the field
			// which is usually valid for write operations on large fields
			while( !(bits & 1) )
			{
				bits >>= 1;
				val ++;
			}
			//calculate the socket number
			sock = nfd*NFDBITS + val;
			// shift one more for the next loop entrance
			bits >>= 1;
			val ++;

			///////////////////////////////////////////////////
			// call the user function
			fd = sock; // should use SessionFindSocket for consitency but this is ok
			func(fd);
			c++;
		}
		// go to next field position
		nfd++;
	}
	return c; // number of processed sockets
}
///////////////////////////////////////////////////////////////////////////////
#endif//!WIN32
///////////////////////////////////////////////////////////////////////////////



int do_sendrecv(int next)
{
	fd_set rfd,wfd;
	struct timeval timeout;
	int ret;
	size_t fd;
	size_t cnt=0;

	// update global tick_timer
	tick_ = time(0);

	FD_ZERO(&wfd);
	FD_ZERO(&rfd);
#ifdef SOCKET_DEBUG_PRINT
	printf("\r[%ld]",tick_);
#endif
	for(fd=0; fd<(size_t)fd_max; fd++)
	{
		if( session[fd] )
		{
#ifdef SOCKET_DEBUG_PRINT
			printf("(%i,%i%i%i)",fd,session[fd]->flag.connected,session[fd]->flag.marked,session[fd]->flag.remove);
			fflush(stdout);
#endif
			if ((session[fd]->rdata_tick != 0) && ((tick_ - session[fd]->rdata_tick) > stall_time_)) 
			{	// emulate a disconnection
				session[fd]->flag.connected = false;
				// and call the read function
				process_read(fd);
				// it should come out with a set remove or marked flag
				// and will be deleted immediately  or on the next loops
			}

			if( session[fd]->flag.remove )
			{	// delete marked sessions here
				// we have to go through the field anyway
				// and this is the safest place for deletions
				session_Delete((int)fd);
				continue;
			}
			else if( session[fd]->flag.connected )
			{
				// build a read-fd_set with connected sockets only
				FD_SET( SessionGetSocket(fd),&rfd);

				// build a write-fd_set with all sockets that have something to write
				if( session[fd]->wdata_size )
					FD_SET( SessionGetSocket(fd),&wfd);
			}
			// double usage of fd_max as max session number and to call select is valid
			// because on unix the fd field position equals the socket identifier
			// and on windows the first parameter of select is ignored
			cnt = fd; 
		}
	}
	fd_max = (int)(cnt+1);

	timeout.tv_sec  = next/1000;
	timeout.tv_usec = next%1000*1000;

	// wait until timeout or some events on the line
	ret = select(fd_max,&rfd,&wfd,NULL,&timeout);

	// check if something happend before timeout
	if(ret>0)
	{
		// process writes
		process_fdset(&wfd,process_write);

		// process readings and parse them immediately
		process_fdset(&rfd,process_read);
	}
	

/*
	// this is the do_parsepackets placed directly here
	// and removed the call from core.c
	
	// this structure was necessary because the map func_parse 
	// did not loop until all incoming data has been processed
	// but just processed the first packet and returned

	// this could leed to serious lag when clients send many packets 
	// at once but only get the first packet processed
	// in a time between each call to select and timer

	for(fd=0;fd<(size_t)fd_max;fd++)
	{
		if(!session[fd])
			continue;

		if(session[fd]->rdata_size==0 && session[fd]->flag.connected)
			continue;

		if(session[fd]->func_parse && !(session[fd]->flag.marked || session[fd]->flag.remove))
			session[fd]->func_parse(fd);

		if(session[fd]) 
		{	//RFIFOFLUSH(fd);
			memmove(session[fd]->rdata, RFIFOP(fd,0), RFIFOREST(fd));
			session[fd]->rdata_size = RFIFOREST(fd);
			session[fd]->rdata_pos  = 0;
		}
	}
*/
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// delayed session removal timer entry
///////////////////////////////////////////////////////////////////////////////
static int session_WaitClose(int tid, unsigned long tick, int id, int data) 
{
	if( session_isValid(id) && session[id]->flag.marked )
	{
		// set session to offline
		// it will be removed by do_sendrecv
		session[id]->flag.marked = false;
		session[id]->flag.remove = true;
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////////
// delayed session removal control
///////////////////////////////////////////////////////////////////////////////
bool session_SetWaitClose(int fd, unsigned long timeoffset)
{
	if( session_isValid(fd) && !session[fd]->flag.marked )
	{
		// set the session to marked state
		session[fd]->flag.marked = true;

		if(session[fd]->session_data == NULL)
			// limited timer, just to send information.
			add_timer(gettick() + 1000, session_WaitClose, fd, 0);
		else
			add_timer(gettick() + timeoffset, session_WaitClose, fd, 0);
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
//
// Session Control
//
///////////////////////////////////////////////////////////////////////////////
bool session_Delete(int fd)
{
	if( session_isValid(fd) )
	{
		// socket is marked for delayed deletion
		// but is not called from the delay handler
		// so we skip deletion here and wait for the handler
		if( session[fd]->flag.marked && session[fd]->flag.remove )
		{
			session[fd]->flag.remove = false;
			return false;
		}

		// call the termination function to save/cleanup/etc
		if( session[fd]->func_term )
			session[fd]->func_term(fd);

		// and clean up
		if(session[fd]->rdata)
			aFree(session[fd]->rdata);
		if(session[fd]->wdata)
			aFree(session[fd]->wdata);
		if(session[fd]->session_data)
			aFree(session[fd]->session_data);
		aFree(session[fd]);
		session[fd]=NULL;

		SessionRemoveIndex( fd );
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
//
// Initialisation and Cleanup
//
///////////////////////////////////////////////////////////////////////////////

unsigned long addr_[16];   // ip addresses of local host (host byte order)
unsigned int naddr_ = 0;   // # of ip addresses

void socket_init(void)
{

#ifdef WIN32
	char** a;
	unsigned int i;
	char fullhost[255];
	struct hostent* hent;
	
	/* Start up the windows networking */
	WSADATA wsaData;
	if ( WSAStartup(WINSOCK_VERSION, &wsaData) != 0 ) {
		ShowError("SYSERR: WinSock not available!\n");
		exit(1);
	}
	
	if(gethostname(fullhost, sizeof(fullhost)) == SOCKET_ERROR) {
		ShowWarning("Ugg.. no hostname defined!\n");
		return;
	} 
	
	// XXX This should look up the local IP addresses in the registry
	// instead of calling gethostbyname. However, the way IP addresses
	// are stored in the registry is annoyingly complex, so I'll leave
	// this as T.B.D.
	hent = gethostbyname(fullhost);
	if (hent == NULL) {
		ShowWarning("Cannot resolve our own hostname to a IP address");
		return;
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
		ShowWarning("SIOCGIFCONF failed!\n");
		return;
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
	
	add_timer_func_list(session_WaitClose, "session_WaitClose");
	
}

void socket_final(void)
{
	int fd;
	for(fd=0;fd<fd_max;fd++){
		// don't unse session_Delete here
		// just force deletion of the sessions
		if( session_isValid(fd) )
		{
			if(session[fd]->rdata)			aFree(session[fd]->rdata);
			if(session[fd]->wdata)			aFree(session[fd]->wdata);
			if(session[fd]->session_data)	aFree(session[fd]->session_data);
			aFree(session[fd]);
			session[fd]=NULL;
			SessionRemoveIndex( fd );
		}
	}
}



class streamable
{
public:
	streamable()			{}
	virtual ~streamable()	{}

	virtual void toBuffer(uchar *buf) const   = 0;
	virtual void fromBuffer(const uchar *buf) = 0;
};

///////////////////////////////////////////////////////////////////////////////
//
// new ip address stuff
// just setting them up
// 
///////////////////////////////////////////////////////////////////////////////
//
// class that holds the IP addresses of app
// 
// we need a lan_ip, subnetmask and lan port; further a wan ip and wan port
// if not set explicitely, the initial network addresses will be used
// wan ip and port will the same as lan values when not specified, 
// and subnetmask is BROADCAST
// also used to check if a bind address is valid or not
//
///////////////////////////////////////////////////////////////////////////////
class ipset : public streamable
{
	template <uint C> class _ipset_helper
	{	
		ulong	cAddr[C];	// ip addresses of local host (host byte order)
		uint	cCnt;		// # of ip addresses

	public:
		_ipset_helper() : cCnt(0)
		{

#ifdef WIN32
			uchar** a;
			unsigned int i;
			char fullhost[255];
			struct hostent* hent;
			
			/* Start up the windows networking */
			WSADATA wsaData;
			if ( WSAStartup(WINSOCK_VERSION, &wsaData) != 0 )
			{
				printf("SYSERR: WinSock not available!\n");
				exit(1);
			}
			
			if(gethostname(fullhost, sizeof(fullhost)) == SOCKET_ERROR)
			{
				printf("No hostname defined!\n");
				return;
			} 
			
			// XXX This should look up the local IP addresses in the registry
			// instead of calling gethostbyname. However, the way IP addresses
			// are stored in the registry is annoyingly complex, so I'll leave
			// this as T.B.D.

			hent = gethostbyname(fullhost);
			if (hent == NULL) {
				printf("Cannot resolve our own hostname to a IP address");
				return;
			}
			a = (uchar**)hent->h_addr_list;
			for(i = 0; a[i] != NULL && i < C; ++i) {
				cAddr[i] =	  (a[i][0]<<0x18)
							| (a[i][1]<<0x10)
							| (a[i][2]<<0x08)
							| (a[i][3]);
			}
			cCnt = i;
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
				printf("SIOCGIFCONF failed!\n");
				return;
			}
			for(pos = 0; pos < ic.ifc_len;   )
			{
				struct ifreq * ir = (struct ifreq *) (ic.ifc_buf + pos);
				struct sockaddr_in * a = (struct sockaddr_in *) &(ir->ifr_addr);
				
				if(a->sin_family == AF_INET) {
					u_long ad = ntohl(a->sin_addr.s_addr);
					if(ad != INADDR_LOOPBACK) {
						cAddr[cCnt++] = ad;
						if(cCnt == C)
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
		}

		uint GetSytemCount()	
		{
			return cCnt;
		}
		ipaddress GetSytemIP(uint i=0)
		{
			if( i < cCnt )
				return cAddr[i];
			else if(cCnt>0)
				return cAddr[0];
			return INADDR_LOOPBACK;
		}
	};

	static _ipset_helper<16> iphelp;


	// read a system IP, might use the first only; maybe play a bit more with that

	ipaddress	lan_ip;		// LAN ip			(aka computer ip)
	ipaddress	lan_sub;	// LAN subnetmask	(255.255.255.0 for class-C subnet)
	ushort		lan_port;	// LAN port			(aka computer port)

	ipaddress	wan_ip;		// WAN ip			(aka router ip)
	ushort		wan_port;	// WAN port			(aka router port that is forwarded to ip/port on computer)

public:
	// construction with defaults
	ipset(const ipaddress bip = INADDR_ANY,			// 0.0.0.0 (bind on all addresses)
		  const ipaddress lip = iphelp.GetSytemIP(0),// 127.0.0.1
		  const ipaddress lsu = INADDR_BROADCAST,	// 255.255.255.255
		  const ushort lpt    = 0,
		  const ipaddress wip = iphelp.GetSytemIP(0),// identify with the first System IP by default
		  const ushort wpt    = 0)
		: lan_ip(lip),lan_sub(lsu),lan_port(lpt),wan_ip(wip),wan_port(wpt)
	{}
	virtual ~ipset()	{}

	// check if an given ip is LAN
	bool isLAN(const ipaddress ip) const
	{
		return ( (lan_ip&lan_sub) == (ip&lan_sub) );
	}
	// check if an given ip is WAN
	bool isWAN(const ipaddress ip) const
	{
		return ( (lan_ip&lan_sub) != (ip&lan_sub) );
	}

	static bool isBindable(ipaddress ip)
	{	// check if an IP is part of the system IP can can be bound to
		for(uint i=0; i<iphelp.GetSytemCount(); i++)
			if( ip==iphelp.GetSytemIP(i) )
				return true;
		return false;
	}
	void check()
	{	// check for unset wan address/port
		if( wan_port == 0 ) 
			wan_port = lan_port;
	}

	ipaddress LANIP() const	{return lan_ip;}
	bool SetLANIP(const ipaddress ip)
	{	// a valid lan address should be bindable
		if( isBindable(ip) ) 
		{
			lan_ip = ip;
			return true;
		}
		return false;
	}
	ipaddress& WANIP()		{return wan_ip;}
	ipaddress& SubnetMask()	{return lan_sub;}
	ushort& LANPort()		{return lan_port;}
	ushort& WANPort()		{return wan_port;}


	virtual void toBuffer(uchar *buf) const
	{
	}
	virtual void fromBuffer(const uchar *buf)
	{
	}

};
ipset::_ipset_helper<16> ipset::iphelp;

class app_ipaddresses
{

	ipset iplogin;
	ipset ipchar;
	ipset ipmap;

	ipaddress String2IP(const char *c)
	{
		struct hostent * h;
		h = gethostbyname(c);
		if (h != NULL) {
			return ipaddress(h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
		} else
			return ntohl(inet_addr(c));
	}


public:
	
	app_ipaddresses():
	    iplogin(INADDR_ANY,INADDR_BROADCAST),
		ipchar(INADDR_ANY,INADDR_BROADCAST),
		ipmap(INADDR_ANY,INADDR_BROADCAST)
	{}


	bool load(const char *filename)
	{
		char line[1024], w1[1024], w2[1024];
		FILE *fp;

		fp = savefopen(filename, "r");

		if (fp == NULL) {
			ShowError("Configuration file not found: %s\n", filename);
			return false;
		}

		ShowStatus("Reading Configuration...\n");

		while(fgets(line, sizeof(line)-1, fp)) {

			if (line[0] == '/' && line[1] == '/')
				continue;

			line[sizeof(line)-1] = '\0';
			if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) != 2)
				continue;

			remove_control_chars(w1);
			remove_control_chars(w2);

			if (strcasecmp(w1, "login_lan_ip") == 0)
				ipmap.SetLANIP( String2IP(w2) );
			if (strcasecmp(w1, "login_lan_port") == 0)
				ipmap.LANPort() = atoi(w2);
			if (strcasecmp(w1, "login_subnetmask") == 0)
				ipmap.SubnetMask() = String2IP(w2);
			if (strcasecmp(w1, "login_wan_ip") == 0)
				ipmap.WANIP() = String2IP(w2);
			if (strcasecmp(w1, "login_wan_port") == 0)
				ipmap.WANPort() = atoi(w2);

			if (strcasecmp(w1, "char_lan_ip") == 0)
				ipmap.SetLANIP( String2IP(w2) );
			if (strcasecmp(w1, "char_lan_port") == 0)
				ipmap.LANPort() = atoi(w2);
			if (strcasecmp(w1, "char_subnetmask") == 0)
				ipmap.SubnetMask() = String2IP(w2);
			if (strcasecmp(w1, "char_wan_ip") == 0)
				ipmap.WANIP() = String2IP(w2);
			if (strcasecmp(w1, "char_wan_port") == 0)
				ipmap.WANPort() = atoi(w2);

			if (strcasecmp(w1, "map_lan_ip") == 0)
				ipmap.SetLANIP( String2IP(w2) );
			if (strcasecmp(w1, "map_lan_port") == 0)
				ipmap.LANPort() = atoi(w2);
			if (strcasecmp(w1, "map_subnetmask") == 0)
				ipmap.SubnetMask() = String2IP(w2);
			if (strcasecmp(w1, "map_wan_ip") == 0)
				ipmap.WANIP() = String2IP(w2);
			if (strcasecmp(w1, "map_wan_port") == 0)
				ipmap.WANPort() = atoi(w2);
		}
		fclose(fp);


		iplogin.check();
		ipchar.check();
		ipmap.check();

		return 0;
	}
};
