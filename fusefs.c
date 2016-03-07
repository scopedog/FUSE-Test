/***************************************************************************

	Author: Hiroshi Nishida, nishida@asusa.net

	fusefs.c
	Interface for FUSE

***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/queue.h>
#define	FUSE_USE_VERSION	29 // Must be defined before fuse.h
#include <fuse.h>
#include "fusetest.h"
#include "log.h"
#ifdef EXTERN
#undef EXTERN
#endif
#include "util.h"

/**************************************************************************
	Definitions
**************************************************************************/

/**************************************************************************
	Functions
**************************************************************************/

// Get attributes
static int
FuseGetattr(const char *path, struct stat *sb)
{
	char	lpath[PATH_MAX];
	int	err = 0;

	DebugMsg("FuseGetattr: path: %s\n", path);

	// Set path
	snprintf(lpath, PATH_MAX, "%s/%s", DATA_PATH, path);

	// Stat
	if (stat(lpath, sb)) {
		Log("Error: FuseGetattr: %s: %s", path, strerror(errno));
		err = errno;
	}

	return -err;
}

// Read dir
static int 
FuseReaddir(const char *path, void *fuse_buf, fuse_fill_dir_t filler,
	    off_t offset, struct fuse_file_info *fi)
{
	char		lpath[PATH_MAX], *name;
	int		err = 0;
	DIR		*dp = NULL;
	struct dirent	*de;

	(void)offset;
	(void)fi;

	DebugMsg("FuseReaddir: path: %s\n", path);

	// Set local path
	snprintf(lpath, PATH_MAX, "%s/%s", DATA_PATH, path);

	// Open dir
	if ((dp = opendir(lpath)) == NULL) {
		Log("Error: FuseReaddir: %s: opendir: %s",
			lpath, strerror(errno));
		err = errno;
		goto END;
	}

	// Scan dir
	while ((de = readdir(dp)) != NULL) {
		name = de->d_name;

		// Skip . and ..
		if (name[0] == '.') {
			continue;
		}

		// Register to fuse_buf
		filler(fuse_buf, name, NULL, 0);
	}

END:	// Finalize
	if (dp != NULL) {
		closedir(dp);
	}

	return -err;
}

// Create
static int 
FuseCreate(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int	fd;
	char	lpath[PATH_MAX];

	DebugMsg("FuseCreate: path: %s, mode: %d\n", path, mode);

	// Set local path
	snprintf(lpath, PATH_MAX, "%s/%s", DATA_PATH, path);

	// Create
	if ((fd = creat(lpath, mode)) == -1) {
		return -errno;
	}
	else {
		fi->fh = fd;
		return 0;
	}
}

// Open
static int 
FuseOpen(const char *path, struct fuse_file_info *fi)
{
	char	lpath[PATH_MAX];
	int	fd, err = 0, flags, mode;

	DebugMsg("FuseOpen: path: %s, flags: %d\n", path, fi->flags);

	// Set local path
	snprintf(lpath, PATH_MAX, "%s/%s", DATA_PATH, path);

	// Initialize
	fi->direct_io = 1;

	// Get mode
	flags = fi->flags;
	mode = flags & O_ACCMODE;

	// Open
	DebugMsg("  %s: open O_WRONLY\n", path);
	if ((fd = open(lpath, flags, 0666)) == -1) {
		Log("Error: FuseOpen: open %s: %s", lpath, strerror(errno)); 
		err = errno;
		goto END;
	}

END:	// Finalize
	if (err) {
		fi->fh = (uint64_t)0;
		errno = err;
	}
	else {
		fi->fh = fd;
	}

	return -err;
}

// Read
static int 
FuseRead(const char *path, char *buf, size_t size, off_t offset,
	 struct fuse_file_info *fi)
{
	int	err = 0, fd = (int)fi->fh;
	ssize_t	ssize = 0;

	DebugMsg("FuseRead: path: %s, fi->fh: %p, size: %d, offset: %d\n",
		 path, fi->fh, size, offset);

	// Read
	if ((ssize = pread(fd, buf, size, offset)) < 0) {
		Log("Error: FuseRead: %s: pread: %s",
			(*path == '/') ? path + 1 : path, strerror(errno));
		err = errno;
		goto END;
	}

END:	// Finalize
	return err ? -err : (int)ssize;
}

// Write
static int 
FuseWrite(const char *path, const char *buf, size_t size, off_t offset,
	  struct fuse_file_info *fi)
{
	int	err = 0, fd = (int)fi->fh;
	ssize_t	ssize = 0;

	DebugMsg("FuseWrite: path: %s, fi->fh: %p, size: %d, "
		 "offset: %d\n", path, fi->fh, size, offset);

	// Write
	if ((ssize = pwrite(fd, buf, size, offset)) < 0) {
		Log("Error: FuseWrite: %s: pwrite: %s",
		    path, strerror(errno));
		err = errno;
	}

	return err ? -err : (int)ssize;
}

/** Release an open file
         *
         * Release is called when there are no more references to an open
         * file: all file descriptors are closed and all memory mappings
         * are unmapped.
         *
         * For every open() call there will be exactly one release() call
         * with the same flags and file descriptor.      It is possible to
         * have a file opened more than once, in which case only the last
         * release will mean, that no more reads/writes will happen on the
         * file.  The return value of release is ignored.  */
static int
FuseRelease(const char *path, struct fuse_file_info *fi)
{
	int	fd = (int)fi->fh;

	DebugMsg("FuseRelease: path: %s, fi->fh: %p\n", path, fi->fh);

	// Close
	errno = 0;
	close(fd);

	return -errno;
}

// Truncate
static int
FuseTruncate(const char *path, off_t size)
{
	char	lpath[PATH_MAX];

	DebugMsg("FuseTruncate: path: %s, size: %d\n", path, size);

	snprintf(lpath, PATH_MAX, "%s/%s", DATA_PATH, path);

	// Truncate
	errno = 0;
	truncate(lpath, size); 

	return -errno;
}

// Ftruncate
static int
FuseFtruncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	int	fd = (int)fi->fh;

	DebugMsg("FuseFtruncate: fi->fh: %p, size = %d\n", fi->fh, size);

	// Ftruncate
	errno = 0;
	ftruncate(fd, size);

	return -errno;
}

// Remove
static int
FuseUnlink(const char *path)
{
	char	lpath[PATH_MAX];

	DebugMsg("FuseUnlink: path: %s\n", path);

	snprintf(lpath, PATH_MAX, "%s/%s", DATA_PATH, path);

	// Unlink
	errno = 0;
	remove(lpath);

	return -errno;
}

// Rename
static int
FuseRename(const char *from, const char *to)
{
	char	fpath[PATH_MAX], tpath[PATH_MAX];

	DebugMsg("FuseRename: from: %s, to: %s\n", from, to);

	snprintf(fpath, PATH_MAX, "%s/%s", DATA_PATH, from);
	snprintf(tpath, PATH_MAX, "%s/%s", DATA_PATH, to);

	// Rename
	errno = 0;
	rename(fpath, tpath);

	return -errno;
}

// Mkdir
static int
FuseMkdir(const char *path, mode_t mode)
{
	char			lpath[PATH_MAX];
	int			err = 0;
	struct fuse_context	*fc;

	DebugMsg("FuseMkdir: path: %s\n", path);

	// Get uid and gid
	fc = fuse_get_context(); 

	// Create at local cache
	snprintf(lpath, PATH_MAX, "%s/%s", DATA_PATH, path);
	if (mkdir(lpath, mode) < 0) {
		Log("Error: FuseMkdir: mkdir %s: %s", lpath, strerror(errno));
		err = errno;
		goto END;
	}

	// Chown cache
	if (chown(lpath, fc->uid, fc->gid) < 0) {
		Log("Error: FuseMkdir: chown %s: %s", lpath, strerror(errno));
		err = errno; // Let's just ignore...
	}

END:	// Finalize
	return -err;
}

// Chown
static int 
FuseChown(const char *path, uid_t uid, gid_t gid)
{
	char	lpath[PATH_MAX];

	DebugMsg("FuseChown: path: %s, uid: %u, gid: %u\n", path, uid, gid);

	snprintf(lpath, PATH_MAX, "%s/%s", DATA_PATH, path);

	// Chown
	errno = 0;
	chown(path, uid, gid);

	return -errno;
}

// chmod -- Change mode
static int
FuseChmod(const char *path, mode_t mode)
{
	char	lpath[PATH_MAX];

	DebugMsg("FuseChmod: path: %s, mode: %u\n", path, mode);

	snprintf(lpath, PATH_MAX, "%s/%s", DATA_PATH, path);

	// Chmod
	errno = 0;
	chmod(path, mode);

	return -errno;
}

// Flush
static int
FuseFlush(const char *path, struct fuse_file_info *fi)
{
	DebugMsg("FuseFlush: path: %s, fi: %p\n", path, fi);

	// Do nothing
	return 0;
}

// Fsync
static int
FuseFsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
	DebugMsg("FuseFsync: isdatasync: %d, path: %s, fi: %p\n",
		isdatasync, path, fi);

	// Do nothing
	return 0;
}

// Readlink
static int
FuseReadlink(const char *path, char *link, size_t lsize)
{
	char	ppath[PATH_MAX];
	int	ret;

	DebugMsg("FuseReadlink: path: %s\n", path);

	// Set path
	snprintf(ppath, PATH_MAX, "%s/%s", DATA_PATH, path);

	// Read link
	ret = readlink(ppath, link, lsize);

	return (ret == -1) ? -errno : ret;
}

/* FUSE file system loop */
int 
FuseLoop(int argc, char *argv[])
{
	static struct fuse_operations fuse_op = {
		.getattr = FuseGetattr,
		.readdir = FuseReaddir,
		.create = FuseCreate,
		.open = FuseOpen,
		.read = FuseRead,
		.write = FuseWrite,
		.release = FuseRelease,
		.truncate = FuseTruncate,
		.ftruncate = FuseFtruncate,
		.unlink = FuseUnlink,
		.rename = FuseRename,
		.mkdir = FuseMkdir,
		.chown = FuseChown,
		.chmod = FuseChmod,
		.flush = FuseFlush,
		.fsync = FuseFsync,
		.readlink = FuseReadlink,
	};

	return fuse_main(argc, argv, &fuse_op, NULL);
}
