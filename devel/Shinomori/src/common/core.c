// $Id: core.c,v 1.1.1.1 2004/09/10 17:44:49 MagicalTux Exp $
// original : core.c 2003/02/26 18:03:12 Rev 1.7

#ifdef DUMPSTACK
#include <execinfo.h>
#endif

#include "../common/mmo.h"
#include "core.h"
#include "socket.h"
#include "timer.h"
#include "version.h"
#include "utils.h"
#include "showmsg.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static void (*term_func)(void)=NULL;

/*======================================
 *	CORE : Set function
 *--------------------------------------
 */
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
	int i;
	switch(sn){
	case SIGINT:
	case SIGTERM:
		if(term_func)
			term_func();
		for(i=0;i<fd_max;i++){
			if(!session[i])
				continue;
			close(i);
		}
		exit(0);
		break;
	}
}

/*=========================================
 *	Dumps the stack using glibc's backtrace
 *-----------------------------------------
 */
#ifdef DUMPSTACK
static void sig_dump(int sn)
{
	FILE *fp;
	void* array[20];

	char **stack;
	size_t size;		
	int no = 0;
	char tmp[256];

	// search for a usable filename
	do {
		sprintf(tmp,"save/stackdump_%04d.txt", ++no);
	} while((fp = fopen(tmp,"r")) && (fclose(fp), no < 9999));
	// dump the trace into the file
	if ((fp = fopen (tmp,"w")) != NULL) {

		fprintf(fp,"Exception: %s\n", strsignal(sn));
		fprintf(fp,"Stack trace:\n");
		size = backtrace (array, 20);
		stack = backtrace_symbols (array, size);

		for (no = 0; no < size; no++) {

			fprintf(fp, "%s\n", stack[no]);

		}
		fprintf(fp,"End of stack trace\n");

		fclose(fp);
		free(stack);
	}
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

int runflag = 1;

int main(int argc,char **argv)
{
	int next;

	Net_Init();
	do_socket();
	
	compat_signal(SIGPIPE,SIG_IGN);
	compat_signal(SIGTERM,sig_proc);
	compat_signal(SIGINT,sig_proc);
	
#ifndef DUMPSTACK
 	// Signal to create coredumps by system when necessary (crash)
 	compat_signal(SIGSEGV, SIG_DFL);
	compat_signal(SIGFPE, SIG_DFL);
	compat_signal(SIGILL, SIG_DFL);
	#ifndef _WIN32
		compat_signal(SIGBUS, SIG_DFL);
		compat_signal(SIGTRAP, SIG_DFL); 
	#endif
#else
	compat_signal(SIGSEGV, sig_dump);
	compat_signal(SIGFPE, sig_dump);
	compat_signal(SIGILL, sig_dump);
	#ifndef _WIN32
		compat_signal(SIGBUS, sig_dump);
		compat_signal(SIGTRAP, SIG_DFL); 
	#endif
#endif

	display_title();

	tick_ = time(0);

	do_init(argc,argv);

	while(runflag){
		next=do_timer(gettick_nocache());
		do_sendrecv(next);
		do_parsepacket();
	}
	return 0;
}
