/*
 *	Author: Hiroshi Nishida, nishida@asusa.net
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <openssl/evp.h>
#define FUSE_USE_VERSION	29 // Must be defined before fuse.h
#include <fuse.h>
#define _FUSETEST_MAIN_
#include "fusetest.h"
#include "fusefs.h"
#include "log.h"
#ifdef EXTERN
#undef EXTERN
#endif
#include "util.h"

/************************************************************
	Functions
************************************************************/

// Usage
static void
Usage()
{
	fprintf(stderr, "Usage: %s [-fhv]\n"
		"-d: Output debug messages.\n"
		"-f: Run in the foreground.\n"
		"-h: Show help,\n"
		"-v: Output verbose messages\n",
		Program);
	exit(1);
}

/* At exit -- usually a process with FUSE must be stopped by killing it. In
   that case, FUSE does not unmount a filesystem, so we must manually do it. */
static void
AtExit()
{
	// Unmount 
	fuse_unmount(MountDir, NULL);
}

// At signal
static void
AtSignal(int sig)
{
	exit(0);
}

// Register atexit
static void
RegAtExit()
{
	// atexit
	atexit(AtExit);

	// Signals
	signal(SIGKILL, AtSignal);
	signal(SIGINT, AtSignal);
	signal(SIGABRT, AtSignal);
	signal(SIGTERM, AtSignal);
	signal(SIGFPE, SIG_IGN);
}

// Initialize 
static void
Init(int argc, char **argv)
{
	int	ch;
	char	*p, *_p;

	// Check argv[0], and store into command
	p = argv[0];
	if ((_p = strrchr(p, '/')) != NULL) {
		p = _p + 1;
	}
	strncpy(Program, p, sizeof(Program));

	// Check argc
	if (argc < 1)
		Usage();

	// Save arguments
	Argc = argc;
	Argv = argv;

	// Get options
	while ((ch = getopt(argc, argv, "dfhv")) != -1) {
		switch (ch) {
		case 'd': // Debug
			Debug = 1;
			break;
		case 'v': // Verbose
			Verbose = 1;
			break;
		case 'f': // Foreground process flag
			Daemon = 0;
			break;
		case 'h': // Help
		default:
			Usage();
		}
	}

	// Check argc again
	if (argc > optind) {
		Usage();
	}

	// Initialize log
	InitLog(NULL, Program, Daemon);

	// Register atexit
	RegAtExit();
}

// Main process
static void
MainProc()
{
	char	*argv[16];
	int	i;

	// Set argv for Fuse main loop
	i = 0;
	argv[i] = Program;	i++;
	argv[i] = MountDir;	i++;
	if (!Daemon) {
		argv[i] = "-f";
		i++;
	}

	// Other options for fuse
	argv[i] = "-o";	i++;
	argv[i] = "kernel_cache,allow_other,max_write=4915200,max_read=4915200";
	//argv[i] = "sync_read,kernel_cache,allow_other,max_write=4915200,max_read=4915200";
	i++;
	/* Sync read, Allow other users to access the file, ... */
/* One of the following causes kernel crash.....
	argv[i] = "-okernel_cache"; // Disable kernel cache
	i++;
	argv[i] = "-oauto_cache"; // 
	i++;
	argv[i] = "-s"; // Single thread
	i++;
*/

	// Main loop for Fuse
	FuseLoop(i, argv);
}

// Main
int
main(int argc, char **argv)
{
	// Initialize
	Init(argc, argv);

	// Main process
	MainProc();

	exit(0);
}
