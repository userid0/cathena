#ifndef _SHOWMSG_H_
#define _SHOWMSG_H_

//davidsiaw, 'lookee' here!
#define SHOW_DEBUG_MSG 1

#define	CL_RESET	"\033[0;0m"
#define CL_NORMAL	CL_RESET
#define CL_NONE		CL_RESET
#define	CL_WHITE	"\033[1;29m"
#define	CL_GRAY		"\033[1;30m"
#define	CL_RED		"\033[1;31m"
#define	CL_GREEN	"\033[1;32m"
#define	CL_YELLOW	"\033[1;33m"
#define	CL_BLUE		"\033[1;34m"
#define	CL_MAGENTA	"\033[1;35m"
#define	CL_CYAN		"\033[1;36m"

extern char tmp_output[1024];

enum msg_type {MSG_STATUS, MSG_SQL, MSG_INFORMATION,MSG_NOTICE,MSG_WARNING,MSG_DEBUG,MSG_ERROR,MSG_FATALERROR};

extern int ShowStatus(const char *, ...);
extern int ShowSQL(const char *, ...);
extern int ShowInfo(const char *, ...);
extern int ShowNotice(const char *, ...);
extern int ShowWarning(const char *, ...);
extern int ShowDebug(const char *, ...);
extern int ShowError(const char *, ...);
extern int ShowFatalError(const char *, ...);

#endif
