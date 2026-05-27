#ifndef CLAUDEC99_STDARG_H
#define CLAUDEC99_STDARG_H

typedef struct __claudec00_va_list_tag {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} va_list[1];

#define va_start(ap, last)     __builtin_va_start(ap, last)
#define va_end(ap)             __builtin_va_end(ap)
#define va_copy(dst, src)      __builtin_va_copy(dst, src)
#define va_arg(ap, type)       __builtin_va_arg(ap, type)

#endif
