// $Id: dump.c 1 2005-3-10 3:17:17 PM Celestia $
// Addon for Freya

#define __ADDON
#include <config.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#ifndef __CYGWIN
	#include <execinfo.h>
#endif

#include "../common/addons.h"
#include "../common/addons_exports.h"

#ifdef __CYGWIN
	#define FOPEN_ freopen
	extern void cygwin_stackdump();
#else
	#define FOPEN_(fn,m,s) fopen(fn,m)
#endif
static void sig_dump(int);
extern const char *strsignal(int);

DLLFUNC ModuleHeader MOD_HEADER(dump) = {
	"StackDump",
	"$Id: dump.c 0.2",
	"Creates a stack dump upon crash",
	MOD_VERSION,
	ADDONS_MAP
};

// by Gabuzomeu
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

DLLFUNC int MOD_TEST(dump)() {
	return MOD_SUCCESS;	
}

DLLFUNC int MOD_INIT(dump)(void *ct) {
	printf ("init dump\n");
	call_table=ct;
	/* load local symbols table */
	MFNC_LOCAL_TABLE(local_table);
	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(dump)() {
	compat_signal(SIGSEGV, sig_dump);
	compat_signal(SIGFPE, sig_dump);
	compat_signal(SIGILL, sig_dump);
	#ifndef __WIN32
		compat_signal(SIGBUS, sig_dump);
		compat_signal(SIGTRAP, SIG_DFL);
	#endif
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(dump)() {
	return MOD_SUCCESS;
}

void sig_dump(int sn)
{
	FILE *fp;
	char file[256];
	int no = 0;
	#ifndef __CYGWIN
		void* array[20];
		char **stack;
		size_t size;
	#endif

	printf ("dumping...!");

	// search for a usable filename
	do {
		sprintf (file, "log/stackdump_%04d.txt", ++no);
	} while((fp = fopen(file,"r")) && (fclose(fp), no < 9999));
	// dump the trace into the file

	if ((fp = FOPEN_(file, "w", stderr)) != NULL) {
		printf ("Dumping stack... ");
		fprintf(fp, "Exception: %s \n", strsignal(sn));
		fflush (fp);

	#ifdef __CYGWIN
		cygwin_stackdump ();
	#else
		fprintf(fp, "Stack trace:\n");
		size = backtrace (array, 20);
		stack = backtrace_symbols (array, size);
		for (no = 0; no < size; no++) {
			fprintf(fp, "%s\n", stack[no]);
		}
		fprintf(fp,"End of stack trace\n");
		FREE(stack);
	#endif

		printf ("Done.\n");
		fflush(stdout);
		fclose(fp);
	}
	// Pass the signal to the system's default handler
	compat_signal(sn, SIG_DFL);
	raise(sn);
}

