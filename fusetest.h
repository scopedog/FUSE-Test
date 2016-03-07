#ifndef _FUSETEST_H_
#define _FUSETEST_H_

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

/*********************************************************************
	Definitions
*********************************************************************/

#define DATA_PATH	"/data" // Data dir
#define	MountDir	"/mnt" // Mount dir

/*********************************************************************
	Structures
*********************************************************************/

/*********************************************************************
	Global varibales
*********************************************************************/

#ifdef  _FUSETEST_MAIN_
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN char	Program[PATH_MAX]; // Command name 
EXTERN int	Argc; // Arguments
EXTERN char	**Argv;
/*
EXTERN int	Daemon;
EXTERN int	Debug;
EXTERN int	Verbose;
*/


#endif // _FUSETEST_H_
