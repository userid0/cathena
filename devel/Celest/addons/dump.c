// $Id: dump.c 1 2005-3-10 3:17:17 PM Celestia $
// Addon for Freya

#define __ADDON
#include <config.h>

#ifdef __CYGWIN
	// cygwin doesn't have backtrace support!
	// is there anyway to test if execinfo.h exists?
#else

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <execinfo.h>

#include "../common/addons.h"
#include "../common/addons_exports.h"

DLLFUNC ModuleHeader MOD_HEADER(dump) = {
	"StackDump",
	"$Id: dump.c 0.1a",
	"Creates a stack dump upon crash",
	MOD_VERSION,
	ADDONS_MAP
};

static void sig_dump(int);

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
	#ifdef __CYGWIN
		return MOD_FAILED;
	#else
		return MOD_SUCCESS;
	#endif
}

DLLFUNC int MOD_INIT(dump)(void *ct) {
	printf ("init dump\n");
	call_table=ct;
	/* load local symbols table */
	MFNC_LOCAL_TABLE(local_table);
	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(dump)() {
	printf ("load signals\n");
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
	printf ("unload\n");
	return MOD_SUCCESS;
}

void sig_dump(int sn)
{
	FILE *fp;
	void* array[20];
	char **stack;
	size_t size;		
	int no = 0;
	char tmp[256];

	printf ("dumping...!");

	// search for a usable filename
	do {
		sprintf(tmp,"log/stackdump_%04d.txt", ++no);
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
		FREE(stack);
	}
	// Pass the signal to the system's default handler
	compat_signal(sn, SIG_DFL);
	raise(sn);
}

#endif
