#!/usr/bin/python3 -I
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026 The Blueberry Pie OS Authors
#
# usage: decomp-tables.py UnicodeData-2.1.2.txt UnicodeData-3.2.0.txt > DecompData.h
# The data files are from https://www.unicode.org/Public/.
import sys

ACTIONS = {(0, 2): 0x02, (0, 3): 0x03, (1, 1): 0x12, (1, 2): 0x13, (2, 1): 0x23}


def load(path):
    classes, decomps = {}, {}
    for line in open(path, encoding="ascii"):
        fields = line.split(";")
        char = int(fields[0], 16)
        if char > 0xFFFF:
            continue
        classes[char] = int(fields[3] or 0)
        if fields[5] and not fields[5].startswith("<"):
            decomps[char] = [int(x, 16) for x in fields[5].split()]
    return classes, decomps


def decompose(decomps, char):
    if char not in decomps:
        return [char]
    return [x for part in decomps[char] for x in decompose(decomps, part)]


def canonical(classes, chars):
    chars = list(chars)
    for i in range(1, len(chars)):
        j = i
        while j > 0 and classes.get(chars[j], 0) and classes.get(chars[j - 1], 0) > classes[chars[j]]:
            chars[j - 1], chars[j] = chars[j], chars[j - 1]
            j -= 1
    return chars


def changes(old_classes, old_decomps, new_classes, new_decomps):
    out = {}
    for char in sorted(set(old_decomps) | set(new_decomps)):
        if 0x2000 <= char <= 0x2FFF or 0xF900 <= char <= 0xFAFF:
            continue
        old = canonical(old_classes, decompose(old_decomps, char)) if char in old_decomps else None
        new = canonical(new_classes, decompose(new_decomps, char)) if char in new_decomps else None
        if old == new:
            continue
        if old is None:
            out[char] = (ACTIONS[0, len(new)], new)
            continue
        if len(old) < 2 or (new and sorted(old) == sorted(new)):
            continue
        if new and (old[0] != new[0] or len(old) > 2):
            continue
        result = new or [char]
        out[old[0]] = (ACTIONS[len(old) - 1, len(result)], old[1:] + result)
    return out


def rows(values, per_line):
    for i in range(0, len(values), per_line):
        print("\t" + " ".join("%s," % v for v in values[i:i + per_line]))


def main():
    old_classes, old_decomps = load(sys.argv[1])
    new_classes, new_decomps = load(sys.argv[2])
    repl = changes(old_classes, old_decomps, new_classes, new_decomps)
    classes = {c: v for c, v in new_classes.items() if v and (c < 0x3100 or c >= 0xFB00)}
    used = sorted({c >> 4 for c in classes} | {c >> 4 for c in repl})
    index = [-1] * (0x3600 >> 4)
    class_ranges, repl_ranges = [], []
    data = [0]
    for hi in used:
        index[((hi << 4) + 0x500 & 0xFFFF) >> 4] = len(class_ranges)
        class_range, repl_range = [0] * 16, [0] * 16
        for lo in range(16):
            char = hi << 4 | lo
            class_range[lo] = classes.get(char, 0)
            if char in repl:
                repl_range[lo] = len(data)
                data.append(repl[char][0])
                data.extend(repl[char][1])
        class_ranges.append(class_range)
        repl_ranges.append(repl_range)
    print("/*\n * SPDX-License-Identifier: BSD-2-Clause AND Unicode-3.0")
    print(" * Copyright (c) 2026 The Blueberry Pie OS Authors")
    print(" * Copyright (c) 1991-2002 Unicode, Inc.\n */\n")
    print("static const int8_t classAndReplIndex[kHiFieldEntryCount] = {")
    rows(index, 16)
    print("};\n\nstatic const u_int8_t combClassRanges[][kLoFieldEntryCount] = {")
    for r in class_ranges:
        print("\t{ %s }," % ", ".join(map(str, r)))
    print("};\n\nstatic const u_int8_t replaceRanges[][kLoFieldEntryCount] = {")
    for r in repl_ranges:
        print("\t{ %s }," % ", ".join(map(str, r)))
    print("};\n\nstatic const u_int16_t replaceData[] = {")
    rows(["0x%04X" % v for v in data], 8)
    print("};")


main()
