// $Id: uptime.c 1 2005-3-10 3:17:17 PM Celestia $
// Addon for Freya

#define __ADDON
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../common/addons.h"
#include "../common/addons_exports.h"
#include "../common/timer.h"

unsigned long tick = 0;
char server_type[13];

DLLFUNC ModuleHeader MOD_HEADER(uptime) = {
	"Uptime Log",
	"$Id: uptime.c 0.1",
	"Logs server uptime",
	MOD_VERSION,
	ADDONS_MAP	// currently only applies to map
};

// Known Issues:
// - it somehow doesn't work in Linux, no idea why ^^;
// - it doesn't work if the server crashes, and using sig_proc
//   would mess up coredumping and my dump addon =/

DLLFUNC int MOD_TEST(uptime)() {
	// nothing to test
	return MOD_SUCCESS;
}

DLLFUNC int MOD_INIT(uptime)(void *ct) {
	call_table=ct;
	/* load local symbols table */
	MFNC_LOCAL_TABLE(local_table);
	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(uptime)() {
	int (*gettick)();
	MFNC_GETTICK(gettick);
	tick = (*gettick)();
	switch (MOD_HEADER(name).addon_target) {
		case ADDONS_LOGIN:
			sprintf (server_type, "login-server"); break;
		case ADDONS_CHAR:
			sprintf (server_type, "char-server"); break;
		case ADDONS_MAP:
			sprintf (server_type, "map-server"); break;
		default:
			sprintf (server_type, "core"); break;	
	}
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(uptime)() {
	int (*gettick)();
	time_t curtime;
	char curtime2[24];	
	FILE *fp;
	long seconds = 0, day = 24*60*60, hour = 60*60,
		minute = 60, days = 0, hours = 0, minutes = 0;

	fp = fopen("log/uptime.log","a");
	if (fp) {
		time(&curtime);
		strftime(curtime2, 24, "%m/%d/%Y %H:%M:%S", localtime(&curtime));

		MFNC_GETTICK(gettick);
		seconds = ((*gettick)()-tick)/CLOCKS_PER_SEC;
		days = seconds/day;
		seconds -= (seconds/day>0)?(seconds/day)*day:0;
		hours = seconds/hour;
		seconds -= (seconds/hour>0)?(seconds/hour)*hour:0;
		minutes = seconds/minute;
		seconds -= (seconds/minute>0)?(seconds/minute)*minute:0;

//		printf("%s: %s uptime - %ld days, %ld hours, %ld minutes, %ld seconds.\n",
		fprintf(fp, "%s: %s uptime - %ld days, %ld hours, %ld minutes, %ld seconds.\n",
			curtime2, server_type, days, hours, minutes, seconds);
		fclose(fp);
	} else {
		// should we consider this as failed?
//		return MOD_FAILED;
	}
	return MOD_SUCCESS;
}
