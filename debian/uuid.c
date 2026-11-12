/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 The Blueberry Pie OS Authors
 */

/*
 * int OpenDeviceByUUID(void *uuidp, char **namep);
 *
 * uuidp points at a uuid_string_t. OpenDeviceByUUID opens, read-only, the
 * raw device of the IOMedia whose UUID is that one, and returns the file
 * descriptor. If namep is not NULL, *namep is set to the device path, to be
 * freed with free(). It returns -1 if there is no such device, or if it
 * cannot be opened.
 */

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMedia.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <uuid/uuid.h>

int OpenDeviceByUUID(void *uuidp, char **namep);

static CFStringRef
copy_string(io_service_t media, CFStringRef key)
{
	CFTypeRef value;

	value = IORegistryEntryCreateCFProperty(media, key, kCFAllocatorDefault, 0);
	if (value != NULL && CFGetTypeID(value) != CFStringGetTypeID()) {
		CFRelease(value);
		value = NULL;
	}
	return (CFStringRef)value;
}

static int
has_uuid(io_service_t media, const uuid_t want)
{
	CFStringRef value;
	uuid_string_t str;
	uuid_t have;
	int match = 0;

	value = copy_string(media, CFSTR(kIOMediaUUIDKey));
	if (value == NULL)
		return 0;
	if (CFStringGetCString(value, str, sizeof(str), kCFStringEncodingASCII) &&
	    uuid_parse(str, have) == 0)
		match = uuid_compare(have, want) == 0;
	CFRelease(value);
	return match;
}

static char *
raw_path(io_service_t media)
{
	CFStringRef value;
	char bsdname[MAXPATHLEN];
	char *path = NULL;

	value = copy_string(media, CFSTR(kIOBSDNameKey));
	if (value == NULL)
		return NULL;
	if (CFStringGetCString(value, bsdname, sizeof(bsdname), kCFStringEncodingUTF8) &&
	    bsdname[0] != '\0' && strchr(bsdname, '/') == NULL &&
	    asprintf(&path, "/dev/r%s", bsdname) == -1)
		path = NULL;
	CFRelease(value);
	return path;
}

int
OpenDeviceByUUID(void *uuidp, char **namep)
{
	uuid_string_t str;
	uuid_t want;
	CFMutableDictionaryRef match;
	io_iterator_t iter;
	io_service_t media;
	char *path = NULL;
	int fd;

	if (uuidp == NULL)
		return -1;
	memcpy(str, uuidp, sizeof(str));
	str[sizeof(str) - 1] = '\0';
	if (uuid_parse(str, want) != 0)
		return -1;

	match = IOServiceMatching(kIOMediaClass);
	if (match == NULL)
		return -1;
	if (IOServiceGetMatchingServices(kIOMainPortDefault, match, &iter) != KERN_SUCCESS)
		return -1;
	while (path == NULL && (media = IOIteratorNext(iter)) != IO_OBJECT_NULL) {
		if (has_uuid(media, want))
			path = raw_path(media);
		IOObjectRelease(media);
	}
	IOObjectRelease(iter);
	if (path == NULL)
		return -1;

	fd = open(path, O_RDONLY);
	if (fd == -1 || namep == NULL)
		free(path);
	else
		*namep = path;
	return fd;
}
