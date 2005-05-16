// $Id: core.c,v 1.1.1.1 2004/09/10 17:44:49 MagicalTux Exp $
// original : core.c 2003/02/26 18:03:12 Rev 1.7

#define LOG_UPTIME 0


#ifdef DUMPSTACK
	#if !defined(CYGWIN) && !defined(WIN32) && !defined(__NETBSD__)	// HAVE_EXECINFO_H
		#include <execinfo.h>
	#endif
#endif



#include "mmo.h"
#include "malloc.h"
#include "core.h"
#include "db.h"
#include "dll.h"
#include "socket.h"
#include "timer.h"
#include "version.h"
#include "utils.h"
#include "showmsg.h"



///////////////////////////////////////////////////////////////////////////////
// uptime class

unsigned long uptime::starttick = gettick();

void uptime::getvalues(unsigned long& days,unsigned long& hours,unsigned long& minutes,unsigned long& seconds)
{
	seconds = (gettick()-starttick)/CLOCKS_PER_SEC;
	days    = seconds/(24*60*60);
	seconds-= days*(24*60*60);
	hours   = seconds/(60*60);
	seconds-= hours*(60*60);
	minutes = seconds/(60);
	seconds-= minutes*(60);
}
const char *uptime::getstring(char *buffer)
{	// usage of the static buffer is not threadsafe
	static char tmp[128];
	char *buf = (buffer) ? buffer : tmp;

	unsigned long day, minute, hour, seconds;
	getvalues(day, minute, hour, seconds);

	sprintf(buf, "%ld days, %ld hours, %ld minutes, %ld seconds",
		day, minute, hour, seconds);
	return buf;
}


///////////////////////////////////////////////////////////////////////////////
// global data
int runflag = 1;
const char *argv0;

///////////////////////////////////////////////////////////////////////////////
// run control
void core_stoprunning()
{
	runflag = 0;
}

///////////////////////////////////////////////////////////////////////////////
// pid stuff
char pid_file[256];

void pid_delete(void)
{
	unlink(pid_file);
}

void pid_create(const char* file)
{
	FILE *fp;
	int len = strlen(file);
	memcpy(pid_file,file,len+1);
	if(len > 4 && pid_file[len - 4] == '.') {
		pid_file[len - 4] = 0;
	}
	strcat(pid_file,".pid");
	fp = savefopen(pid_file,"w");
	if(fp) {
#ifdef WIN32
		fprintf(fp,"%ld",GetCurrentProcessId());
#else
		fprintf(fp,"%ld",getpid());
#endif
		fclose(fp);
	}
}

///////////////////////////////////////////////////////////////////////////////
// uptime logging
void log_uptime(void)
{
#if LOG_UPTIME
	time_t curtime;
	char curtime2[24];
	FILE *fp;
	long seconds = 0, day = 24*60*60, hour = 60*60,
		minute = 60, days = 0, hours = 0, minutes = 0;

	fp = savefopen("log/uptime.log","a");
	if (fp)
	{
		time(&curtime);
		strftime(curtime2, 24, "%m/%d/%Y %H:%M:%S", localtime(&curtime));


		fprintf(fp, "%s: %s uptime - %s.\n",
			curtime2, argv0, uptime.getstring());
		fclose(fp);
	}
	return;
#endif
}

///////////////////////////////////////////////////////////////////////////////
// signal and dump

// Added by Gabuzomeu
// This is an implementation of signal() using sigaction() for portability.
// (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
// Programming in the UNIX Environment_.
//

#ifdef WIN32	// windows don't have SIGPIPE others don't define them
#define SIGPIPE SIGINT
#endif

#ifndef POSIX
#define compat_signal(signo, func) signal(signo, func)
#else
sigfunc *compat_signal(int signo, sigfunc *func)
{
  struct sigaction sact, oact;

  sact.sa_handler = func;
  sigemptyset(&sact.sa_mask);
  sact.sa_flags = 0;
#ifdef SA_INTERRUPT
  sact.sa_flags |= SA_INTERRUPT;	// SunOS
#endif

  if (sigaction(signo, &sact, &oact) < 0)
    return (SIG_ERR);

  return (oact.sa_handler);
}
#endif

/*======================================
 *	CORE : Signal Sub Function
 *--------------------------------------
 */

static void sig_proc(int sn)
{
	//////////////////////////////////
	// force shut down
	static int is_called = 0;
	if(++is_called > 3)
		exit(0);

	switch(sn)
	{
	case SIGINT:
	case SIGTERM:
		//////////////////////////////////
		// shut down main loop
                runflag = 0;
	}
}
static void sig_pipe(int sn) {
	ShowMessage ("Broken pipe found... closing socket\n");	// set to eof in socket.c
	return;	// does nothing here
}
/*=========================================
 *	Dumps the stack using glibc's backtrace
 *-----------------------------------------
 */
#ifdef DUMPSTACK
void sig_dump(int sn)
{	
	FILE *fp;
		void* array[20];
		char **stack;
		size_t size;
	int no = 0;
	char tmp[256];

	// search for a usable filename
	do {
		sprintf(tmp,"log/stackdump_%04d.txt", ++no);
	} while((fp = savefopen(tmp,"r")) && (fclose(fp), no < 9999));

	// dump the trace into the file
	if ((fp = savefopen (tmp,"w")) != NULL)
	{
		fprintf(fp, "Exception: %s \n", strsignal(sn));
		fprintf(fp, "Stack trace:\n");
		size = backtrace (array, 20);
		stack = backtrace_symbols (array, size);
		for (no = 0; no < size; no++)
		{
			fprintf(fp, "%s\n", stack[no]);
		}
		fprintf(fp,"End of stack trace\n");
		fclose(fp);
		aFree(stack);
	}
	//cygwin_stackdump ();
	// When pass the signal to the system's default handler
	compat_signal(sn, SIG_DFL);
	raise(sn);
}
#endif

void init_signal()
{
	///////////////////////////////////////////////////////////////////////////
#ifdef DUMPSTACK
	///////////////////////////////////////////////////////////////////////////
	// glibc dump
	compat_signal(SIGTERM,sig_proc);
	compat_signal(SIGINT,sig_proc);

	// Signal to create coredumps by system when necessary (crash)
	compat_signal(SIGSEGV, sig_dump);
	compat_signal(SIGFPE, sig_dump);
	compat_signal(SIGILL, sig_dump);
 #ifndef WIN32
	compat_signal(SIGPIPE, sig_pipe);
	compat_signal(SIGBUS, sig_dump);
	compat_signal(SIGTRAP, SIG_DFL);
 #endif
	///////////////////////////////////////////////////////////////////////////
#else
	///////////////////////////////////////////////////////////////////////////
	// normal dump
	compat_signal(SIGTERM,sig_proc);
	compat_signal(SIGINT,sig_proc);

	// Signal to create coredumps by system when necessary (crash)
	compat_signal(SIGSEGV, SIG_DFL);
	compat_signal(SIGFPE, SIG_DFL);
	compat_signal(SIGILL, SIG_DFL);
 #ifndef WIN32
	compat_signal(SIGPIPE,sig_pipe);
	compat_signal(SIGBUS, SIG_DFL);
	compat_signal(SIGTRAP, SIG_DFL); 
 #endif

	///////////////////////////////////////////////////////////////////////////
#endif
}

///////////////////////////////////////////////////////////////////////////////
// revision
int get_svn_revision(char *svnentry)
{	// Warning: minor syntax checking
	char line[1024];
	int rev = 0;
	FILE *fp;
	if( svnentry && ( NULL!=(fp = savefopen(svnentry, "r")) ) )
	{
		while (fgets(line,1023,fp))
		{
			if (strstr(line,"revision="))
				break;
		}
		fclose(fp);
		if (sscanf(line," %*[^\"]\"%d%*[^\n]",&rev)==1)
			return rev;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/*======================================
 *	CORE : Display title
 *--------------------------------------
 */
static void display_title(void)
{
	int revision;
	ShowMessage(CL_CLS); // clear screen and go up/left (0, 0 position in text)
	ShowMessage(CL_WTBL"          (=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=)"CL_CLL""CL_NORM"\n"); // white writing (37) on blue background (44), "CL_CLL" clean until end of file
	ShowMessage(CL_XXBL"          ("CL_BT_YELLOW"        (c)2004 eAthena Development Team presents        "CL_XXBL")"CL_CLL""CL_NORM"\n"); // yellow writing (33)
	ShowMessage(CL_XXBL"          ("CL_BOLD"       ______  __    __                                  "CL_XXBL")"CL_CLL""CL_NORM"\n"); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD"      /\\  _  \\/\\ \\__/\\ \\                     v%2d.%02d.%02d   "CL_XXBL")"CL_CLL""CL_NORM"\n", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD"    __\\ \\ \\_\\ \\ \\ ,_\\ \\ \\___      __    ___      __      "CL_XXBL")"CL_CLL""CL_NORM"\n"); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD"  /'__`\\ \\  __ \\ \\ \\/\\ \\  _ `\\  /'__`\\/' _ `\\  /'__`\\    "CL_XXBL")"CL_CLL""CL_NORM"\n"); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD" /\\  __/\\ \\ \\/\\ \\ \\ \\_\\ \\ \\ \\ \\/\\  __//\\ \\/\\ \\/\\ \\_\\.\\_  "CL_XXBL")"CL_CLL""CL_NORM"\n"); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD" \\ \\____\\\\ \\_\\ \\_\\ \\__\\\\ \\_\\ \\_\\ \\____\\ \\_\\ \\_\\ \\__/.\\_\\ "CL_XXBL")"CL_CLL""CL_NORM"\n"); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD"  \\/____/ \\/_/\\/_/\\/__/ \\/_/\\/_/\\/____/\\/_/\\/_/\\/__/\\/_/ "CL_XXBL")"CL_CLL""CL_NORM"\n"); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD"   _   _   _   _   _   _   _     _   _   _   _   _   _   "CL_XXBL")"CL_CLL""CL_NORM"\n"); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD"  / \\ / \\ / \\ / \\ / \\ / \\ / \\   / \\ / \\ / \\ / \\ / \\ / \\  "CL_XXBL")"CL_CLL""CL_NORM"\n"); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD" ( e | n | g | l | i | s | h ) ( A | t | h | e | n | a ) "CL_XXBL")"CL_CLL""CL_NORM"\n"); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD"  \\_/ \\_/ \\_/ \\_/ \\_/ \\_/ \\_/   \\_/ \\_/ \\_/ \\_/ \\_/ \\_/  "CL_XXBL")"CL_CLL""CL_NORM"\n"); // 1: bold char, 0: normal char
	ShowMessage(CL_XXBL"          ("CL_BOLD"                                                         "CL_XXBL")"CL_CLL""CL_NORM"\n"); // yellow writing (33)
	ShowMessage(CL_XXBL"          ("CL_BT_YELLOW"  Advanced Fusion Maps (c) 2003-2004 The Fusion Project  "CL_XXBL")"CL_CLL""CL_NORM"\n"); // yellow writing (33)
	ShowMessage(CL_WTBL"          (=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=)"CL_CLL""CL_NORM"\n\n"); // reset color
	
	if ((revision = get_svn_revision(".svn\\entries"))>0)
	{
		ShowInfo("SVN Revision: '"CL_WHITE"%d"CL_RESET"'.\n",revision);
	}
}

///////////////////////////////////////////////////////////////////////////////
/*======================================
 *	CORE : MAINROUTINE
 *--------------------------------------
 */
int main(int argc,char **argv)
{
	int next;

	///////////////////////////////////////////////////////////////////////////
	// startup
	init_signal();
	display_title();


	argv0 = strrchr(argv[0],PATHSEP);
	if(NULL==argv0)
		argv0 = argv[0];

	///////////////////////////////////////////////////////////////////////////
	// stuff

	pid_create(argv0);

	///////////////////////////////////////////////////////////////////////////
	// core module initialisation
	memmgr_init(argv0); // àÍî‘ç≈èâÇ…é¿çsÇ∑ÇÈïKóvÇ™Ç†ÇÈ
	timer_init();
	socket_init();
	dll_init();

	///////////////////////////////////////////////////////////////////////////
	// user module initialisation
	do_init(argc,argv);

	///////////////////////////////////////////////////////////////////////////
	// addon
	addon_event_trigger("Athena_Init");

	///////////////////////////////////////////////////////////////////////////
	// run loop
	while(runflag)
	{
		next=do_timer(gettick_nocache());
		do_sendrecv(next);
	}
	///////////////////////////////////////////////////////////////////////////
	// termination

	///////////////////////////////////////////////////////////////////////////
	// addon
	addon_event_trigger("Athena_Final");

	///////////////////////////////////////////////////////////////////////////
	// user module termination
	do_final();

	///////////////////////////////////////////////////////////////////////////
	// core module termination
	exit_dbn();
	dll_final();
	socket_final();
	timer_final();
	memmgr_final();
	
	///////////////////////////////////////////////////////////////////////////
	// stuff
	log_uptime();
	pid_delete();
	return 0;
}
