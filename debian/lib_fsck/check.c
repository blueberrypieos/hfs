/*
 * Copyright (c) 1999-2000, 2002-2008 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* Modified 2026-11-07: made from fsck_hfs.c and utilities.c of hfs-627.40.1, with the state and logging interface of debian/lib_fsck (APSL 2.0 section 2.2(b)). */
/*
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "check.h"
#include "fsck_msgnums.h"
#include "dfalib/CheckHFS.h"

#include <hfs/hfs_mount.h>

static int setup __P((char *dev));
static int ScanDisk(int);
int	reply __P((char *question));

static int printStatus;
static void siginfo(int signo);

Cache_t fscache;

enum { BLOCK_LIST_INCREMENT = 512 };
int gBlkListEntries = 0;
u_int64_t *gBlockList = NULL;
int gFoundBlockEntries = 0;
struct found_blocks *gFoundBlocksList = NULL;

int
checkfilesys(char * filesys)
{
	int flags;
	int result = 0;
	char *mntonname = state.mountpoint;
	flags = 0;
	int err = 0;
	state.cdevname = filesys;
	state.hotmount = state.hotroot;

	fsck_print(ctx, LOG_TYPE_INFO, "");

	if (state.lflag && !state.detonatorRun) {
		struct stat fs_stat;

		if (stat(state.cdevname, &fs_stat) != -1 &&
			(((fs_stat.st_mode & S_IFMT) == S_IFCHR) ||
			 ((fs_stat.st_mode & S_IFMT) == S_IFBLK))) {
			struct stat io_stat;

			if (fstat(fileno(stdin), &io_stat) != -1 &&
				(fs_stat.st_rdev == io_stat.st_dev)) {
					fsck_print(ctx, LOG_TYPE_INFO, "ERROR: input redirected from target volume for live verify.\n");
					return EEXIT;
			}
			if (fstat(fileno(stdout), &io_stat) != -1 &&
				(fs_stat.st_rdev == io_stat.st_dev)) {
					fsck_print(ctx, LOG_TYPE_INFO, "ERROR:  output redirected to target volume for live verify.\n");
					return EEXIT;
			}
			if (fstat(fileno(stderr), &io_stat) != -1 &&
				(fs_stat.st_rdev == io_stat.st_dev)) {
					fsck_print(ctx, LOG_TYPE_INFO, "ERROR:  error output redirected to target volume for live verify.\n");
					return EEXIT;
			}

		}
	}

	if (state.debug && state.preen)
		fsck_print(ctx, LOG_TYPE_WARNING, "starting\n");

	if (setup( filesys ) == 0) {
		if (state.preen)
			fsck_print(ctx, LOG_TYPE_FATAL, "CAN'T CHECK FILE SYSTEM.");
		result = EEXIT;
		goto ExitThisRoutine;
	}

	if (state.preen == 0) {
		if (state.hotroot && !state.guiControl)
			fsck_print(ctx, LOG_TYPE_INFO, "** Root file system\n");
	}

	if (state.errorOnExit && state.nflag) {
		state.chkLev = kMajorCheck;
	}

	if (state.rebuildOptions && fsck_get_writable() == 0) {
		fsck_print(ctx, LOG_TYPE_INFO, "BTree rebuild requested but writing disabled\n");
		result = EEXIT;
		goto ExitThisRoutine;
	}

	if (gBlockList != NULL && state.scanflag != 0) {
		fsck_print(ctx, LOG_TYPE_INFO, "Cannot scan for bad blocks and ask for listed blocks to file mapping\n");
		result = EEXIT;
		goto ExitThisRoutine;
	}
	if (state.scanflag != 0) {
		fsck_print(ctx, LOG_TYPE_INFO, "Scanning entire disk for bad blocks\n");
		err = ScanDisk(state.fsreadfd);
		if (err) {
			result = err;
			goto ExitThisRoutine;
		}
	}
	result = CheckHFS( filesys, state.fsreadfd, state.fswritefd, state.chkLev, state.repLev, ctx,
			state.lostAndFoundMode, fsck_get_writable(), &state.fsmodified,
			state.lflag, state.rebuildOptions );
	if (state.debug)
		fsck_print(ctx, LOG_TYPE_INFO, "\tCheckHFS returned %d, fsmodified = %d\n", result, state.fsmodified);

	if (!state.hotmount) {
		DestroyCache();
		if (state.quick) {
			if (result == 0) {
				fsck_print(ctx, LOG_TYPE_WARNING, "QUICKCHECK ONLY; FILESYSTEM CLEAN\n");
				result = 0;
				goto ExitThisRoutine;
			} else if (result == R_Dirty) {
				fsck_print(ctx, LOG_TYPE_WARNING, "QUICKCHECK ONLY; FILESYSTEM DIRTY\n");
				result = DIRTYEXIT;
				goto ExitThisRoutine;
			} else if (result == R_BadSig) {
				fsck_print(ctx, LOG_TYPE_WARNING, "QUICKCHECK ONLY; NO HFS SIGNATURE FOUND\n");
				result = DIRTYEXIT;
				goto ExitThisRoutine;
			} else {
				result = EEXIT;
				goto ExitThisRoutine;
			}
		}
	} else {
		struct statfs stfs_buf;

		if (statfs(mntonname, &stfs_buf) == 0)
			flags = stfs_buf.f_flags;
		else
			flags = 0;
		DestroyCache();
	}

	if (state.hotmount && state.fsmodified) {
		struct hfs_mount_args args;

		if (!state.preen)
			fsckPrintFormat(ctx, fsckVolumeModified);
		if (flags & MNT_RDONLY) {
			bzero(&args, sizeof(args));
			flags |= MNT_UPDATE | MNT_RELOAD;
			if (state.debug)
				fsck_print(ctx, LOG_TYPE_STDERR, "doing update / reload mount for %s now\n", mntonname);
			if (mount("hfs", mntonname, flags, &args) == 0) {
				if (result != 0)
					result = EEXIT;
				goto ExitThisRoutine;
			} else {
				fsck_print(ctx, LOG_TYPE_STDERR, "update/reload mount for %s failed: %s\n", mntonname, strerror(errno));
			}
		}
		if (!state.preen)
			fsck_print(ctx, LOG_TYPE_INFO, "\n***** REBOOT NOW *****\n");
		sync();
		result = FIXEDROOTEXIT;
		goto ExitThisRoutine;
	}

	if (result != 0 && result != MAJOREXIT)
		result = EEXIT;

ExitThisRoutine:
	return (result);
}

void
AddBlockToList(long long block)
{
	size_t blockListCount = BLOCK_LIST_INCREMENT;

	if (gBlockList == NULL) {
		gBlockList = (u_int64_t *)malloc(blockListCount * sizeof(u_int64_t));
		if (gBlockList == NULL) {
			fsck_print(ctx, LOG_TYPE_FATAL, "Can't allocate memory for block list.\n");
		}
	}

	if ((gBlkListEntries % BLOCK_LIST_INCREMENT) == 0) {
		void *tmp;

		tmp = realloc(gBlockList, (gBlkListEntries + BLOCK_LIST_INCREMENT) * sizeof(u_int64_t));
		if (tmp == NULL) {
			fsck_print(ctx, LOG_TYPE_FATAL, "Can't allocate memory for block list (%llu entries).\n", gBlkListEntries);
		}
		gBlockList = (u_int64_t*)tmp;
	}
	gBlockList[gBlkListEntries++] = block;
	return;
}

static int
setup( char *dev )
{
	struct stat statb;
	int devBlockSize;
	uint32_t cacheBlockSize;
	uint32_t cacheTotalBlocks;
	int preTouchMem = 0;

	if (!state.detonatorRun) {
		if (stat(dev, &statb) < 0) {
			fsck_print(ctx, LOG_TYPE_INFO, "Can't stat %s: %s\n", dev, strerror(errno));
			return (0);
		}
		if ((statb.st_mode & S_IFMT) != S_IFCHR) {
			fsck_print(ctx, LOG_TYPE_FATAL, "%s is not a character device", dev);
			if (reply("CONTINUE") == 0)
				return (0);
		}
		if (!state.nflag && !state.quick) {
			if (strncmp(dev, "/dev/rdisk", 10) == 0) {
				char block_device[MAXPATHLEN+1];
				int rv;
				snprintf(block_device, sizeof(block_device), "/dev/%s", dev + 6);
				rv = journal_replay(block_device);
				if (state.debug)
					fsck_print(ctx, LOG_TYPE_INFO, "journal_replay(%s) returned %d\n", block_device, rv);
			}
		}
	} else {
		fsck_print(ctx, LOG_TYPE_INFO, "fsck_hfs: detonator_run (%s).\n", dev);

		struct stat info;
		int error = fstat(state.fswritefd, &info);
		if (error) {
			fsck_print(ctx, LOG_TYPE_TERMINATE, "fsck_hfs: fstat %s", dev);
		}

		error = (int)lseek(state.fswritefd, 0, SEEK_SET);
		if (error == -1) {
			fsck_print(ctx, LOG_TYPE_TERMINATE, "fsck_hfs: Could not seek %d for dev: %s, errorno %d", state.fswritefd, dev, errno);
		}
		fsck_set_device_writable(1);
	}

	if (state.preen == 0 && !state.guiControl) {
		if (state.nflag || state.quick || state.fswritefd == -1) {
			fsck_print(ctx, LOG_TYPE_INFO, "** %s (NO WRITE)\n", dev);
		} else {
			fsck_print(ctx, LOG_TYPE_INFO, "** %s\n", dev);
		}
	}

	devBlockSize = fsck_get_dev_block_size();
	if (devBlockSize == -1) {
		fsck_print(ctx, LOG_TYPE_INFO, "Device block size was not initialized\n");
		return (0);
	}

	if (state.reqCacheSize == 0 && state.quick == 0) {
		uint64_t memSize;
		size_t dsize = sizeof(memSize);
		int rv;

		rv = sysctlbyname("hw.memsize", &memSize, &dsize, NULL, 0);
		if (rv == -1) {
			fsck_print(ctx, LOG_TYPE_STDERR, "sysctlbyname failed, not auto-setting cache size\n");
		} else {
			int d = (state.hotroot && !state.lflag) ? 2 : 8;
			if (!state.detonatorRun) {
				int safeMode = 0;
				dsize = sizeof(safeMode);
				rv = sysctlbyname("kern.safeboot", &safeMode, &dsize, NULL, 0);
				if (rv != -1 && safeMode != 0 && state.hotroot && !state.lflag) {
#define kMaxSafeModeMem	((size_t)2 * 1024 * 1024 * 1024)
					if (state.debug) {
						fsck_print(ctx, LOG_TYPE_STDERR, "Safe mode and single-user, setting memsize to a maximum of 2gbytes\n");
					}
					memSize = (memSize < kMaxSafeModeMem) ? memSize : kMaxSafeModeMem;
				}
			}
			state.reqCacheSize = memSize / d;
		}
	}

	CalculateCacheSizes(state.reqCacheSize, &cacheBlockSize, &cacheTotalBlocks, state.debug);

	preTouchMem = (state.hotroot != 0) && (state.lflag != 0);
	if (CacheInit (&fscache, state.fsreadfd, state.fswritefd, devBlockSize,
			cacheBlockSize, cacheTotalBlocks, CacheHashSize, preTouchMem) != EOK) {
		fsck_print(ctx, LOG_TYPE_FATAL, "Can't initialize disk cache\n");

		return (0);
	}

	return (1);
}

static int
ScanDisk(int fd)
{
	uint32_t devBlockSize = 512;
	uint64_t devBlockTotal;
	off_t diskSize;
	uint8_t *buffer = NULL;
	size_t bufSize = 1024 * 1024;
	ssize_t nread;
	off_t curPos = 0;
	void (*oldhandler)(int);
	uint32_t numErrors = 0;
	uint32_t maxErrors = 40;

	oldhandler = signal(SIGINFO, &siginfo);

#define PRSTAT \
	do { \
		if (diskSize) { \
			fsck_print(ctx, LOG_TYPE_ERROR, "Scanning offset %lld of %lld (%d%%)\n", \
				curPos, diskSize, (int)((curPos * 100) / diskSize)); \
		} else { \
			fsck_print(ctx, LOG_TYPE_ERROR, "Scanning offset %lld\n", curPos); \
		} \
		printStatus = 0; \
	} while (0)

	if (state.devBlockSize == -1) {
		devBlockSize = 512;
	} else {
		devBlockSize = state.devBlockSize;
	}

	devBlockTotal = state.blockCount;
	if (devBlockTotal == 0) {
		diskSize = 0;
	} else
		diskSize = devBlockTotal * devBlockSize;

	while (buffer == NULL && bufSize >= devBlockSize) {
		buffer = malloc(bufSize);
		if (buffer == NULL) {
			bufSize /= 2;
		}
	}
	if (buffer == NULL) {
		fsck_print(ctx, LOG_TYPE_FATAL, "Cannot allocate buffer for disk scan.\n");
	}

loop:

	if (printStatus) {
		PRSTAT;
	}
	while ((nread = pread(fd, buffer, bufSize, curPos)) == bufSize) {
		curPos += bufSize;
		if (printStatus) {
			PRSTAT;
		}
	}

	if (nread == 0) {
		goto done;
	}
	if (nread == -1) {
		if (errno == EIO) {
			size_t total;
			for (total = 0; total < bufSize; total += devBlockSize) {
				nread = pread(fd, buffer, devBlockSize, curPos + total);
				if (nread == -1) {
					if (errno == EIO) {
						if (state.debug)
							fsck_print(ctx, LOG_TYPE_STDERR, "Bad block at offset %lld\n", curPos + total);
						AddBlockToList((curPos + total) / fsck_get_block_size());
						if (++numErrors > maxErrors) {
							if (state.debug)
								fsck_print(ctx, LOG_TYPE_STDERR, "Got %u errors, maxing out so stopping scan\n", numErrors);
							goto done;
						}
						continue;
					} else {
						fsck_print(ctx, LOG_TYPE_FATAL, "Got a non I/O error reading disk at offset %llu: %s\n",
							curPos + total, strerror(errno));
					}
				}
				if (nread == 0) {
					goto done;
				}
				if (nread != devBlockSize) {
					fsck_print(ctx, LOG_TYPE_WARNING, "During disk scan, did not get block size (%zd) read, got %zd instead.  Skipping rest of this block.\n", (size_t)devBlockSize, nread);
					continue;
				}
			}
			curPos += total;
			goto loop;
		} else if (errno == EINTR) {
			goto loop;
		} else {
			fsck_print(ctx, LOG_TYPE_FATAL, "Got a non I/O error reading disk at offset %llu:  %s\n", curPos, strerror(errno));
			return EEXIT;
		}
	}
	if (nread < bufSize) {
		if ((nread % devBlockSize) == 0) {
			curPos += nread;
		} else {
			curPos = curPos + (((nread % devBlockSize) + 1) * devBlockSize);
		}
		goto loop;
	}
	goto loop;
done:
	if (buffer)
		free(buffer);
	signal(SIGINFO, oldhandler);
	return 0;

}

int
reply(char *question)
{
	int persevere;
	char c;

	if (state.preen)
		fsck_print(ctx, LOG_TYPE_FATAL, "INTERNAL ERROR: GOT TO reply()");
	persevere = !strcmp(question, "CONTINUE");
	fsck_print(ctx, LOG_TYPE_INFO, "\n");
	if (!persevere && (state.nflag || state.fswritefd < 0)) {
		fsck_print(ctx, LOG_TYPE_INFO, "%s? no\n\n", question);
		return (0);
	}
	if (state.yflag || (persevere && state.nflag)) {
		fsck_print(ctx, LOG_TYPE_INFO, "%s? yes\n\n", question);
		return (1);
	}
	do	{
		fsck_print(ctx, LOG_TYPE_INFO, "%s? [yn] ", question);
		(void) fflush(stdout);
		c = getc(stdin);
		while (c != '\n' && getc(stdin) != '\n') {
			if (feof(stdin)) {
				return (0);
			}
		}
	} while (c != 'y' && c != 'Y' && c != 'n' && c != 'N');
	fsck_print(ctx, LOG_TYPE_INFO, "\n");
	if (c == 'y' || c == 'Y')
		return (1);
	return (0);
}

static void
siginfo(int signo)
{
	printStatus = 1;
}

#define kProgressToggle	"kern.progressmeterenable"
#define	kProgress	"kern.progressmeter"

void
start_progress(const char *dev_name, const char *volume_name)
{
	int rv;
	int enable = 1;

	if (ctx.check_start)
		ctx.check_start(ctx.messages_context, dev_name, volume_name);
	if (state.hotroot == 0)
		return;
	rv = sysctlbyname(kProgressToggle, NULL, NULL, &enable, sizeof(enable));
	if (state.debug && rv == -1 && errno != ENOENT) {
		fsck_print(ctx, LOG_TYPE_WARN, "sysctl(%s) failed", kProgressToggle);
	}
}

void
draw_progress(int pct)
{
	int rv;

	if (ctx.check_update)
		ctx.check_update(ctx.messages_context, pct);
	if (state.hotroot == 0)
		return;
	rv = sysctlbyname(kProgress, NULL, NULL, &pct, sizeof(pct));
	if (state.debug && rv == -1 && errno != ENOENT) {
		fsck_print(ctx, LOG_TYPE_WARN, "sysctl(%s) failed", kProgress);
	}
}

void
end_progress(int err)
{
	int rv;
	int enable = 0;

	if (ctx.check_done)
		ctx.check_done(ctx.messages_context, err);
	if (state.hotroot == 0)
		return;
	rv = sysctlbyname(kProgressToggle, NULL, NULL, &enable, sizeof(enable));
	if (state.debug && rv == -1 && errno != ENOENT) {
		fsck_print(ctx, LOG_TYPE_WARN, "sysctl(%s) failed", kProgressToggle);
	}
}

void
DestroyCache()
{
	CacheFlush(&fscache);
}
