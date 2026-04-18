#ifndef CCOMPILER_UTIL_H
#define CCOMPILER_UTIL_H

#include <stddef.h>

char *read_file(const char *path);
void make_output_path(char *out, size_t out_size, const char *input_path);

#endif
