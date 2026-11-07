/*
 * Copyright (c) 1999-2008 Apple Inc. All rights reserved.
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
/* Modified 2026-11-07: exit codes from fsck_hfs.h of hfs-627.40.1, with new declarations for the state and logging interface of debian/lib_fsck (APSL 2.0 section 2.2(b)). */

#ifndef check_hfs_h
#define check_hfs_h

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

#include "cache.h"
#include "lib_fsck_hfs.h"

#define DIRTYEXIT  3
#define FIXEDROOTEXIT  4
#define	EEXIT	8
#define	MAJOREXIT	47

typedef struct lib_fsck_ctx_t {
	fsck_client_ctx_t client;
	fsck_hfs_print_msg_type_funct_t print_msg_type;
	fsck_exfat_print_msg_num_func_t print_msg_num;
	fsck_ctx_t messages_context;
	fsck_exfat_print_debug_func_t print_debug;
	fsck_hfs_check_start_func_t check_start;
	fsck_hfs_check_update_func_t check_update;
	fsck_hfs_check_done_func_t check_done;
} lib_fsck_ctx_t;

typedef struct fsck_state {
	int	fd;
	int	fsmodified;
	int	fsreadfd;
	int	fswritefd;

	int	canWrite;
	char	hotroot;
	char	hotmount;
	char	guiControl;
	char	xmlControl;
	char	rebuildBTree;
	int	rebuildOptions;
	char	modeSetting;
	char	errorOnExit;
	int	upgrading;
	int	lostAndFoundMode;
	uint64_t reqCacheSize;
	long	gBlockSize;
	int	detonatorRun;
	char	*mountpoint;
	int	verbosityLevel;

	const char *cdevname;
	char	*progname;
	char	lflag;
	char	nflag;
	char	yflag;
	char	preen;
	char	force;
	char	quick;
	char	debug;
	char	disable_journal;
	char	scanflag;
	char	embedded;

	char	repLev;
	char	chkLev;

	uint64_t blockCount;
	int	devBlockSize;

	unsigned long cur_debug_level;
} fsck_state_t;

fsck_state_t state;
lib_fsck_ctx_t ctx;

int fsckPrintFormat(lib_fsck_ctx_t, int, ...);
void fsck_debug_print(lib_fsck_ctx_t, int, const char *, ...);
void fsck_print(lib_fsck_ctx_t, LogMessageType, const char *, ...);

int CheckHFS(const char *, int, int, int, int, lib_fsck_ctx_t, int, int, int *, int, int);
void DumpData(const void *, size_t, char *);
void draw_progress(int);
void end_progress(int);
void start_progress(const char *, const char *);

#endif
