#include "version.h"
#include <stdio.h>

#define VERSION_MAJOR  "00"
#define VERSION_MINOR  "02"
#define VERSION_STAGE  "01120000"

#ifndef VERSION_BUILD
#define VERSION_BUILD  0
#endif

/* Which compiler built this binary.  Passed as -DVERSION_BUILTBY=Token at
 * build time; the two-level stringify turns the bare token into a string
 * literal without requiring shell-escaped quotes on the command line. */
#define CC99_STR_HELPER(x) #x
#define CC99_STR(x) CC99_STR_HELPER(x)

#ifndef VERSION_BUILTBY
#define VERSION_BUILTBY gcc_Ubuntu_13_3_0
#endif

static char version_buf[256];

const char *get_version_string(void) {
    snprintf(version_buf, sizeof(version_buf),
             "ClaudeC99 version %s.%s.%s.%05d\nBuiltBy: %s",
             VERSION_MAJOR, VERSION_MINOR, VERSION_STAGE,
             VERSION_BUILD, CC99_STR(VERSION_BUILTBY));
    return version_buf;
}
