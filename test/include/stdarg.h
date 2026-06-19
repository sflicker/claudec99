#ifndef CLAUDEC99_STDARG_H
#define CLAUDEC99_STDARG_H

/* va_list is the same struct that the compiler preamble defines as
 * __builtin_va_list; using that typedef keeps this stub compatible
 * with system stdarg.h (which does the same typedef chain). */
typedef __builtin_va_list va_list;

#define va_start(ap, last)     __builtin_va_start(ap, last)
#define va_end(ap)             __builtin_va_end(ap)
#define va_copy(dst, src)      __builtin_va_copy(dst, src)
#define va_arg(ap, type)       __builtin_va_arg(ap, type)

#endif
