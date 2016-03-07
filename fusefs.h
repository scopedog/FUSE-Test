#ifndef _FUSETEST_FUSEFS_H_
#define _FUSETEST_FUSEFS_H_

#define FUSE_USE_VERSION	29 // Must be defined before <fuse.h>
#include <fuse.h>

/*******************************************************************************
 	Functions 
*******************************************************************************/

int	FuseLoop(int, char **);

#endif /* _FUSETEST_FUSEFS_H_ */
