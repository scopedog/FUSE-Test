/******************************************************************************
*
*	PROGRAM  : util.c
*	LANGUAGE : C
*	REMARKS  : Miscellaneous convenient functions 
*	AUTHOR   : Hiroshi Nishida
*
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <fcntl.h>
#include <libgen.h>
#include <fts.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/event.h>
#include <netinet/in.h>

#define	_UTIL_MAIN_
#include "util.h"
#undef	_UTIL_MAIN_
#include "log.h"


/*****************************************************************************
	Utility functions
*****************************************************************************/

/* Output error message and exit */
void
ErrorExit(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}

/* Put perror's message and exit */
void
PerrorExit(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

/* Output debug message */
void
DebugMsg(const char *fmt, ...)
{
	va_list	ap;

	if (!Debug)
		return;

	fputs("DEBUG: ", stdout);
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);

	fflush(stdout);
}

/* Output verbose message */
void
VerboseMsg(const char *fmt, ...)
{
	va_list	ap;

	if (!Verbose && !Debug)
		return;

	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);

	fflush(stdout);
}

