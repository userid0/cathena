// $Id: core.c,v 1.1.1.1 2004/09/10 17:44:49 MagicalTux Exp $
// original : core.c 2003/02/26 18:03:12 Rev 1.7

#ifdef DUMPSTACK
	#ifndef CYGWIN	// HAVE_EXECINFO_H
		#include <execinfo.h>
	#endif
#endif

#include "mmo.h"
#include "malloc.h"
#include "core.h"
#include "socket.h"
#include "timer.h"
#include "version.h"
#include "utils.h"
#include "showmsg.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif






int runflag = 1;
/*======================================
 *	CORE : Set function
 *--------------------------------------
 */

static void (*term_func)(void)=NULL;

void set_termfunc(void (*termfunc)(void))
{
	term_func = termfunc;
}

// Added by Gabuzomeu
//
// This is an implementation of signal() using sigaction() for portability.
// (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
// Programming in the UNIX Environment_.
//
#ifndef SIGPIPE
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
  sact.sa_flags |= SA_INTERRUPT;	/* SunOS */
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
	static int is_called = 0;

	if(is_called++)
		return;

	switch(sn){
	case SIGINT:
	case SIGTERM:
		//////////////////////////////////
		// shut down main loop if possible
		runflag=0;
		///////////////////////////////////////////////////////////////////////////
		// user termination
		if(term_func) term_func();
		///////////////////////////////////////////////////////////////////////////
		// core module termination
		socket_final();
		//timer_final();
		///////////////////////////////////////////////////////////////////////////
		// force exit
		exit(0);
		///////////////////////////////////////////////////////////////////////////
	}
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
	if ((fp = savefopen (tmp,"w")) != NULL) {

			fprintf(fp,"Exception: %s\n", strsignal(sn));
			fprintf(fp,"Stack trace:\n");
			size = backtrace (array, 20);
			stack = backtrace_symbols (array, size);
			for (no = 0; no < size; no++) {
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

int get_svn_revision(char *svnentry) { // Warning: minor syntax checking
	char line[1024];
	int rev = 0;
	FILE *fp;

	if( svnentry && ( NULL!=(fp = savefopen(svnentry, "r")) ) ) {
		while (fgets(line,1023,fp)) {
			if (strstr(line,"revision=")) 
				break;
		}
		fclose(fp);
		if (sscanf(line," %*[^\"]\"%d%*[^\n]",&rev)==1) 
		return rev;
	}
		return 0;
	}

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
	
	if ((revision = get_svn_revision(".svn\\entries"))>0) {
		ShowInfo("SVN Revision: '"CL_WHITE"%d"CL_RESET"'.\n",revision);
	}
}

/*======================================
 *	CORE : MAINROUTINE
 *--------------------------------------
 */
char pid_file[256];

void pid_delete(void) {
	unlink(pid_file);
}

void pid_create(const char* file) {
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
		atexit(pid_delete);
	}
}

#define LOG_UPTIME 0
void log_uptime(void)
{
#if LOG_UPTIME
	time_t curtime;
	char curtime2[24];
	FILE *fp;
	long seconds = 0, day = 24*60*60, hour = 60*60,
		minute = 60, days = 0, hours = 0, minutes = 0;

	fp = savefopen("log/uptime.log","a");
	if (fp) {
		time(&curtime);
		strftime(curtime2, 24, "%m/%d/%Y %H:%M:%S", localtime(&curtime));

		seconds = (gettick()-ticks)/CLOCKS_PER_SEC;
		days = seconds/day;
		seconds -= (seconds/day>0)?(seconds/day)*day:0;
		hours = seconds/hour;
		seconds -= (seconds/hour>0)?(seconds/hour)*hour:0;
		minutes = seconds/minute;
		seconds -= (seconds/minute>0)?(seconds/minute)*minute:0;

		fprintf(fp, "%s: %s uptime - %ld days, %ld hours, %ld minutes, %ld seconds.\n",
			curtime2, server_type, days, hours, minutes, seconds);
		fclose(fp);
	}

	return;
#endif
}

int main(int argc,char **argv)
{
	int next;
	///////////////////////////////////////////////////////////////////////////
#ifdef DUMPSTACK
	///////////////////////////////////////////////////////////////////////////
	// glibc dump
	compat_signal(SIGPIPE,SIG_IGN);
	compat_signal(SIGPIPE, sig_ignore);
	compat_signal(SIGTERM,sig_proc);
	compat_signal(SIGINT,sig_proc);

	// Signal to create coredumps by system when necessary (crash)
	compat_signal(SIGSEGV, sig_dump);
	compat_signal(SIGFPE, sig_dump);
	compat_signal(SIGILL, sig_dump);
#ifndef WIN32
		compat_signal(SIGBUS, sig_dump);
		compat_signal(SIGTRAP, SIG_DFL);
	#endif
	///////////////////////////////////////////////////////////////////////////
#else
	///////////////////////////////////////////////////////////////////////////
	// normal dump
	compat_signal(SIGPIPE,SIG_IGN);
	compat_signal(SIGTERM,sig_proc);
	compat_signal(SIGINT,sig_proc);

	// Signal to create coredumps by system when necessary (crash)
	compat_signal(SIGSEGV, SIG_DFL);
#ifndef _WIN32
	compat_signal(SIGBUS, SIG_DFL);
	compat_signal(SIGTRAP, SIG_DFL); 
#endif
	compat_signal(SIGILL, SIG_DFL);
	///////////////////////////////////////////////////////////////////////////
#endif
	///////////////////////////////////////////////////////////////////////////
	//
	display_title();

	pid_create(argv[0]);
	do_init_memmgr(argv[0]); // àÍî‘ç≈èâÇ…é¿çsÇ∑ÇÈïKóvÇ™Ç†ÇÈ
	tick_ = time(0);
	///////////////////////////////////////////////////////////////////////////
	// core component initialisation
	timer_init();
	socket_init();
	///////////////////////////////////////////////////////////////////////////
	// user module initialisation
	do_init(argc,argv);
	///////////////////////////////////////////////////////////////////////////
	// run loop
	while(runflag){
		next=do_timer(gettick_nocache());
		do_sendrecv(next);
	}
	///////////////////////////////////////////////////////////////////////////
	// wait for termination
	while(1) sleep(100);
	///////////////////////////////////////////////////////////////////////////
	return 0;
}
