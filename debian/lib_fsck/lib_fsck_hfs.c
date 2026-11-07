/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 The Blueberry Pie OS Authors
 */

#include "check.h"
#include "lib_fsck_hfs.h"

#define GET(name, type, field) type name(void) { return state.field; }
#define SET(name, type, field) void name(type value) { state.field = value; }

void
fsck_init_state(void)
{
	memset(&state, 0, sizeof(state));
	state.chkLev = kAlwaysCheck;
	state.gBlockSize = 512;
	state.fd = -1;
	state.fsreadfd = -1;
	state.fswritefd = -1;
}

void
fsck_set_context_properties(fsck_hfs_print_msg_type_funct_t type_printer,
    fsck_exfat_print_msg_num_func_t num_printer, fsck_exfat_print_debug_func_t debug_printer,
    fsck_client_ctx_t owner, fsck_ctx_t messages)
{
	ctx.client = owner;
	ctx.print_msg_type = type_printer;
	ctx.print_msg_num = num_printer;
	ctx.messages_context = messages;
	ctx.print_debug = debug_printer;
}

int
fsckPrintFormat(lib_fsck_ctx_t c, int number, ...)
{
	va_list args;
	int status = -1;

	if (!c.print_msg_num)
		return status;
	va_start(args, number);
	status = c.print_msg_num(c.messages_context, number, args);
	va_end(args);
	return status;
}

void
fsck_print(lib_fsck_ctx_t c, LogMessageType kind, const char *format, ...)
{
	va_list args;

	if (!c.print_msg_type)
		return;
	va_start(args, format);
	c.print_msg_type(c.messages_context, kind, format, args);
	va_end(args);
}

void
fsck_debug_print(lib_fsck_ctx_t c, int kind, const char *format, ...)
{
	va_list args;

	if (!c.print_debug)
		return;
	va_start(args, format);
	c.print_debug(c.messages_context, kind, format, args);
	va_end(args);
}

#include "accessors.def"

#undef GET
#undef SET

void
fsck_set_check_update_routines(fsck_hfs_check_start_func_t on_start,
    fsck_hfs_check_update_func_t on_update, fsck_hfs_check_done_func_t on_done)
{
	ctx.check_start = on_start;
	ctx.check_update = on_update;
	ctx.check_done = on_done;
}
