/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 The Blueberry Pie OS Authors
 */

#ifndef DECOMP_DATA_ENUMS_H
#define DECOMP_DATA_ENUMS_H

enum {
	kLoFieldBitSize = 4,
	kShiftUniCharOffset = 0x0500,
	kShiftUniCharLimit = 0x3600,
	kLoFieldEntryCount = 16,
	kHiFieldEntryCount = 0x360,
	kLoFieldMask = 15
};

enum {
	kReplaceCurWithTwo = 0x02,
	kReplaceCurWithThree = 0x03,
	kIfNextOneMatchesReplaceAllWithOne = 0x12,
	kIfNextOneMatchesReplaceAllWithTwo = 0x13,
	kIfNextTwoMatchReplaceAllWithOne = 0x23
};

#endif
