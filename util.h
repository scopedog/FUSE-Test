#ifndef _UTIL_H_
#define _UTIL_H_

#include <unistd.h>
#include <sys/stat.h>


/*******************************************************************************
 	Variables 
*******************************************************************************/

#ifdef _UTIL_MAIN_
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN int	Debug; // Debug flag
EXTERN int	Daemon; // Daemon flag
EXTERN int	Verbose; // Verbos flag

#undef	EXTERN

/*******************************************************************************
 	Utility functions 
*******************************************************************************/

/* Output error message and exit */
void	ErrorExit(const char *, ...);

/* Put perror's message and exit */
void	PerrorExit(char *);

/* Output debug message */
void	DebugMsg(const char *, ...);

/* Output verbose message */
void	VerboseMsg(const char *, ...);


#endif /* _UTIL_H_ */
