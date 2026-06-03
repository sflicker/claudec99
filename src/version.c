#include "version.h"
#include <stdio.h>

#define VERSION_MAJOR  "00"
#define VERSION_MINOR  "01"
#define VERSION_STAGE  "00880000"

#ifndef VERSION_BUILD
#define VERSION_BUILD  0
#endif

/* Extra information appended in brackets after the version number.
 * Set to a non-empty string to populate the field. */
#define VERSION_EXTRA  ""

static char version_buf[128];

const char *get_version_string(void) {
    if (sizeof(VERSION_EXTRA) > 1) {
        snprintf(version_buf, sizeof(version_buf),
                 "ClaudeC99 version %s.%s.%s.%05d [%s]",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_STAGE,
                 VERSION_BUILD, VERSION_EXTRA);
    } else {
        snprintf(version_buf, sizeof(version_buf),
                 "ClaudeC99 version %s.%s.%s.%05d",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_STAGE,
                 VERSION_BUILD);
    }
    return version_buf;
}
