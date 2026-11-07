/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 The Blueberry Pie OS Authors
 */

#ifndef lib_fsck_hfs_h
#define lib_fsck_hfs_h

#import <os/log.h>

#include <stdio.h>
#include <string.h>

#include "dfalib/CheckHFS.h"
#include "fsck_hfs_msgnums.h"

typedef enum {
	LOG_TYPE_ERROR,
	LOG_TYPE_WARNING,
	LOG_TYPE_INFO,
	LOG_TYPE_FATAL,
	LOG_TYPE_TERMINATE,
	LOG_TYPE_FILE_STDERR,
	LOG_TYPE_STDERR,
	LOG_TYPE_WARN
} LogMessageType;

typedef void *fsck_ctx_t;
typedef void *fsck_client_ctx_t;

typedef void (*fsck_hfs_check_start_func_t)(fsck_ctx_t, const char *, const char *);
typedef void (*fsck_hfs_check_update_func_t)(fsck_ctx_t, int);
typedef void (*fsck_hfs_check_done_func_t)(fsck_ctx_t, int);
typedef void (*fsck_hfs_print_msg_type_funct_t)(fsck_client_ctx_t, LogMessageType, const char *, va_list);
typedef int (*fsck_exfat_print_msg_num_func_t)(fsck_client_ctx_t, int, va_list);
typedef void (*fsck_exfat_print_debug_func_t)(fsck_client_ctx_t, unsigned long, const char *, va_list);

#define GET(name, type, field) type name(void);
#define SET(name, type, field) void name(type);
#include "accessors.def"
#undef GET
#undef SET

void AddBlockToList(long long);
void DestroyCache();
int checkfilesys(char *);
void fsck_init_state(void);
void fsck_set_check_update_routines(fsck_hfs_check_start_func_t, fsck_hfs_check_update_func_t,
    fsck_hfs_check_done_func_t);
void fsck_set_context_properties(fsck_hfs_print_msg_type_funct_t, fsck_exfat_print_msg_num_func_t,
    fsck_exfat_print_debug_func_t, fsck_client_ctx_t, fsck_ctx_t);

#endif
