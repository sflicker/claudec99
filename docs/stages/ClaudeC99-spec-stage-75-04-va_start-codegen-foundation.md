# ClaudeC99 - stage 75-04 va_start codegen foundation

## Scope
  - add variadic-function register save area
  - save incoming GP argument registers in variadic function protocol
  - implement __builtin_va_start(ap, last)
  - initialize va_list_fields
    - ga_offset
    - fp_offset
    - overflow_arg_area
    - reg_save_area
  - implement __buildin_va_end(ap) as no-op

## Out of scope
  - va_arg
  - va_copy
  - floating point variadic arguments
  - full stdarg API edge cases

