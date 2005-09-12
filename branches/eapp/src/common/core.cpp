// $Id: core.c,v 1.1.1.1 2004/09/10 17:44:49 MagicalTux Exp $
// original : core.c 2003/02/26 18:03:12 Rev 1.7

#define LOG_UPTIME 0



#include "mmo.h"
#include "malloc.h"
#include "core.h"
#include "db.h"
#include "plugins.h"
#include "socket.h"
#include "timer.h"
#include "version.h"
#include "utils.h"
#include "showmsg.h"



///////////////////////////////////////////////////////////////////////////////
// uptime class

time_t uptime::starttime = time(NULL);

void uptime::getvalues(unsigned long& days,unsigned long& hours,unsigned long& minutes,unsigned long& seconds)
{
	seconds = (unsigned long)difftime( time(NULL), starttime );
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
		(unsigned long)day, (unsigned long)minute, (unsigned long)hour, (unsigned long)seconds);
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
	if(file)
	{
		FILE *fp;
		size_t len = strlen(file);
		memcpy(pid_file,file,len+1);
		if(len > 4 && pid_file[len - 4] == '.') {
			pid_file[len - 4] = 0;
		}
		strcat(pid_file,".pid");
		fp = safefopen(pid_file,"w");
		if(fp)
		{
			fprintf(fp,"%ld", (unsigned long)GetCurrentProcessId());
			fclose(fp);
		}
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

	fp = safefopen("log/uptime.log","a");
	if (fp)
	{
		time(&curtime);
		strftime(curtime2, 24, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
		fprintf(fp, "%s: %s uptime - %s.\n", curtime2, argv0, uptime.getstring());
		fclose(fp);
	}
	return;
#endif
}

///////////////////////////////////////////////////////////////////////////////
// signal and dump
//
// Added by Gabuzomeu
// This is an implementation of signal() using sigaction() for portability.
// (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
// Programming in the UNIX Environment_.

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

void sig_proc(int sn)
{
	//////////////////////////////////
	// force shut down
	static int is_called = 0;
	switch(sn)
	{
	case SIGINT:
	case SIGTERM:
		if (++is_called > 3)
			exit(0);
		runflag = 0;
		break;
#ifndef WIN32
	case SIGXFSZ:
		// ignore and allow it to set errno to EFBIG
		ShowWarning ("Max file size reached!\n");
		//run_flag = 0;	// should we quit?
		break;
	case SIGPIPE:
		ShowMessage ("Broken pipe found... closing socket\n");	// set to eof in socket.c
		break;	// does nothing here
#endif
	}
}
void init_signal()
{
	///////////////////////////////////////////////////////////////////////////
	// normal dump
	compat_signal(SIGTERM, sig_proc);
	compat_signal(SIGINT, sig_proc);

	compat_signal(SIGSEGV, SIG_DFL);
	compat_signal(SIGFPE, SIG_DFL);
	compat_signal(SIGILL, SIG_DFL);
#ifndef WIN32
	compat_signal(SIGXFSZ, sig_proc);
	compat_signal(SIGPIPE, sig_proc);
	compat_signal(SIGBUS, SIG_DFL);
	compat_signal(SIGTRAP, SIG_DFL);
#endif	
	///////////////////////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
// revision
// would make it inline but dll wants it on a fixed position
// to get it's function pointer
const char* get_svn_revision()	{ return "Shinomori's Modified Version (2005-09-03)"; }
/*
{
	static char version[10]="";
	if( !*version )
	{
		FILE *fp;
		if ((fp = fopen(".svn/entries", "r")) != NULL)
		{
			char line[1024];
			int rev;
			while (fgets(line,sizeof(line),fp))
				if (strstr(line,"revision=")) break;
			fclose(fp);
			if (sscanf(line," %*[^\"]\"%d%*[^\n]", &rev) == 1)
			{
				sprintf(version, "%d", rev);
				return version;
			}
		}
		sprintf(version, "Unknown");
	}
	return version;
}
*/
///////////////////////////////////////////////////////////////////////////////
/*======================================
 *	CORE : Display title
 *--------------------------------------
 */
void display_title(void)
{
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
	ShowInfo("SVN Revision: '"CL_WHITE"%s"CL_RESET"'.\n", get_svn_revision() );
}

///////////////////////////////////////////////////////////////////////////////
/*======================================
 *	CORE : MAINROUTINE
 *--------------------------------------
 */
int main (int argc, char **argv)
{
	int next;

	///////////////////////////////////////////////////////////////////////////
	// startup
	init_signal();
	display_title();

	argv0 = strrchr(argv[0],PATHSEP);
	if(NULL==argv0)
		argv0 = argv[0];
	else
		argv0++;

	///////////////////////////////////////////////////////////////////////////
	// stuff
	pid_create(argv0);
	srand(time(NULL)^3141592654UL);

	///////////////////////////////////////////////////////////////////////////
	// core module initialisation
	memmgr_init(argv0); // ��ԍŏ��Ɏ��s����K�v������
	timer_init();
	socket_init();
	plugin_init();

	///////////////////////////////////////////////////////////////////////////
	// user module initialisation
	do_init(argc,argv);

	///////////////////////////////////////////////////////////////////////////
	// addon
	plugin_event_trigger("Athena_Init");

	///////////////////////////////////////////////////////////////////////////
	// run loop
	while(runflag)
	{
		next = do_timer(gettick_nocache());
		do_sendrecv(next);
	}
	///////////////////////////////////////////////////////////////////////////
	// termination

	///////////////////////////////////////////////////////////////////////////
	// addon
	plugin_event_trigger("Athena_Final");

	///////////////////////////////////////////////////////////////////////////
	// user module termination
	do_final();	

	///////////////////////////////////////////////////////////////////////////
	// core module termination
	exit_dbn();
	plugin_final();
	socket_final();
	timer_final();
	memmgr_final();
	
	///////////////////////////////////////////////////////////////////////////
	// stuff
	log_uptime();
	pid_delete();

	///////////////////////////////////////////////////////////////////////////
	// byebye
	return 0;
}
