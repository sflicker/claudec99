 ▐▛███▜▌   Claude Code v2.1.153
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99


❯ /usage 
  ⎿  Settings dialog dismissed

❯ /implement-stage                                                              
docs/stages/ClaudeC99-spec-stage-75-04-va_start-codegen-foundation.md           

● I'll start by reading the spec file and supporting files in parallel.

  Read 4 files (ctrl+o to expand)

● STAGE_DISPLAY: stage 75-04

  Now let me read the relevant source files to understand the current state.

  Searched for 4 patterns, read 3 files (ctrl+o to expand)

● Now let me understand the CodeGen struct and compute_decl_bytes:

  Searched for 2 patterns, read 1 file (ctrl+o to expand)

● Now let me look at existing integration tests and the integration test
  directory for va_* related tests:

  Read 1 file, listed 3 directories (ctrl+o to expand)

● Let me look at how the function definition processes parameters to understand
  num_params and is_variadic:

  Read 1 file (ctrl+o to expand)

● Let me look at how the parser creates AST_BUILTIN_VA_START nodes:

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me check how va_list is used in the parser — how a va_list ap variable
  declaration is handled:

  Searched for 3 patterns, read 1 file (ctrl+o to expand)

● Let me look at how function parameters with typedef types are processed in the
   parser:

  Searched for 4 patterns, read 1 file (ctrl+o to expand)

● I have enough context now. Let me note the spec issues and produce the
  kickoff, then begin implementation.

  Spec Issues Found:
  1. Title: "fro" → "for"; "__buildin_va_end" → "__builtin_va_end"
  2. unsigend int fp_offset → unsigned int fp_offset
  3. Layout table: gp + 8 and gp + 16 should be ap + 8 and ap + 16
  4. vfprint should be vfprintf
  5. vsprintf(char *, size_t, ...) with size_t is vsnprintf's signature, not
  vsprintf's
  6. Test: const * fmt should be const char * fmt
  7. fbp + 16 + 8 (line 151) should be rbp + 16 + 8

  These are all minor typos that don't affect the intent. I'll proceed with the
  corrected forms.

  STAGE_DISPLAY: stage 75-04

  Let me generate the Kickoff artifact while reading the templates:

  Read 1 file (ctrl+o to expand)

● haiku-stage-artifact-writer(Generate stage 75-04 kickoff artifact)  ⎿  Done (8 tool uses · 13.5k tokens · 26s)
  (ctrl+o to expand)

● Kickoff generated. Now let me implement the changes.

  Planned Changes:
  - include/codegen.h — add 3 new fields to CodeGen struct
  - src/codegen.c — init new fields; variadic prologue register saves; va_start
  field initialization
  - test/include/stdio.h — add #include <stdarg.h> and
  vfprintf/vprintf/vsnprintf
  - test/integration/test_va_start_codegen/ — new integration test
  - src/version.c — bump to 00750400

  Step 1: Update include/codegen.h

● Update(include/codegen.h)
  ⎿  Added 10 lines
      149       * function bodies and emitted to .data/.bss after functio
           n code. */
      150      LocalStaticVar local_statics[MAX_LOCAL_STATICS];
      151      int local_static_count;
      152 +    /* Stage 75-04: variadic function register save area.     
      153 +     * variadic_reg_save_offset: rbp-relative offset of the 30
          +4-byte register                                               
      154 +     * save area (0 for non-variadic functions).              
      155 +     * variadic_named_gp_params: min(named_param_count, 6) — u
          +sed by va_start                                               
      156 +     * to initialize gp_offset.                               
      157 +     * variadic_named_stack_params: max(0, named_param_count -
          + 6) — used by                                                 
      158 +     * va_start to compute overflow_arg_area. */              
      159 +    int variadic_reg_save_offset;                             
      160 +    int variadic_named_gp_params;                             
      161 +    int variadic_named_stack_params;                          
      162  } CodeGen;
      163  
      164  void codegen_init(CodeGen *cg, FILE *output);

● Step 2: Initialize the new fields in codegen_init

● Update(src/codegen.c)
  ⎿  Added 3 lines
      254      cg->string_pool_count = 0;
      255      cg->warnings_are_errors = 0;
      256      cg->local_static_count = 0;
      257 +    cg->variadic_reg_save_offset = 0;                         
      258 +    cg->variadic_named_gp_params = 0;                         
      259 +    cg->variadic_named_stack_params = 0;                      
      260  }
      261  
      262  /* Stage 66/70-03: warn with a variable name embedded.

● Step 3: Modify codegen_function — stack size, register save area reservation, 
  and GP register saves

● Update(src/codegen.c)
  ⎿  Added 4 lines
      3473          /* Compute stack space: 8 bytes per parameter (conser
            vative
      3474           * upper bound covering long plus worst-case alignmen
            t) plus a
      3475           * conservative 8-byte upper bound per body declarati
            on.
      3476 +         * Stage 75-04: variadic functions also reserve 304 b
           +ytes for the                                                 
      3477 +         * hidden GP/FP register save area.                  
      3478           * Rounded up to a 16-byte multiple. */
      3479          int stack_size = num_params * 8 + compute_decl_bytes(
            body);
      3480 +        if (node->is_variadic)                               
      3481 +            stack_size += 304;                               
      3482          if (stack_size % 16 != 0)
      3483              stack_size = (stack_size + 15) & ~15;
      3484  

● Now I need to add the variadic register save area setup after the frame is set
   up. Let me find the exact location (after sub rsp, N and before the parameter
   loops):

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 21 lines
      3502              fprintf(cg->output, "    sub rsp, %d\n", stack_si
            ze);
      3503          }
      3504  
      3505 +        /* Stage 75-04: variadic function register save area.
      3506 +         * Reserve 304 bytes at the top of the frame (before 
           +named params) and                                            
      3507 +         * save all 6 GP argument registers so va_start can r
           +eference them. */                                            
      3508 +        if (node->is_variadic) {                             
      3509 +            cg->stack_offset += 304;                         
      3510 +            cg->variadic_reg_save_offset    = cg->stack_offse
           +t;                                                           
      3511 +            cg->variadic_named_gp_params    = num_params < 6 
           +? num_params : 6;                                            
      3512 +            cg->variadic_named_stack_params = num_params > 6 
           +? num_params - 6 : 0;                                        
      3513 +            int rso = cg->variadic_reg_save_offset;          
      3514 +            fprintf(cg->output, "    mov [rbp - %d], rdi\n", 
           +rso);                                                        
      3515 +            fprintf(cg->output, "    mov [rbp - %d], rsi\n", 
           +rso - 8);                                                    
      3516 +            fprintf(cg->output, "    mov [rbp - %d], rdx\n", 
           +rso - 16);                                                   
      3517 +            fprintf(cg->output, "    mov [rbp - %d], rcx\n", 
           +rso - 24);                                                   
      3518 +            fprintf(cg->output, "    mov [rbp - %d], r8\n",  
           +rso - 32);                                                   
      3519 +            fprintf(cg->output, "    mov [rbp - %d], r9\n",  
           +rso - 40);                                                   
      3520 +        } else {                                             
      3521 +            cg->variadic_reg_save_offset    = 0;             
      3522 +            cg->variadic_named_gp_params    = 0;             
      3523 +            cg->variadic_named_stack_params = 0;             
      3524 +        }                                                    
      3525 +                                                             
      3526          /*
      3527           * Parameters share the outermost body scope (scope_s
            tart stays at 0).
      3528           * A body-level declaration that collides with a para
            meter name is

● Step 4: Implement __builtin_va_start codegen — replace the no-op

● Update(src/codegen.c)
  ⎿  Added 31 lines, removed 6 lines
      2809          node->full_type   = node->children[1]->full_type;
      2810          return;
      2811      }
      2812 -    /* Stage 75-03: __builtin_va_* are no-ops in this stage. 
           - The three                                                   
      2813 -     * side-effecting forms (va_start, va_end, va_copy) emit 
           -nothing and                                                  
      2814 -     * leave rax undefined; va_arg emits xor eax,eax so calle
           -rs that use                                                  
      2815 -     * the result get a defined (zero) value. */             
      2816 -    if (node->type == AST_BUILTIN_VA_START ||                
      2817 -        node->type == AST_BUILTIN_VA_END   ||                
      2812 +    /* Stage 75-04: __builtin_va_start initializes the va_lis
           +t fields.                                                    
      2813 +     * __builtin_va_end is a no-op per spec.  va_copy and va_
           +arg remain                                                   
      2814 +     * stubs (out of scope for this stage). */               
      2815 +    if (node->type == AST_BUILTIN_VA_START) {                
      2816 +        /* children[0] = ap (va_list local), children[1] = la
           +st named param */                                            
      2817 +        ASTNode *ap_node = node->children[0];                
      2818 +        LocalVar *lv_ap = codegen_find_var(cg, ap_node->value
           +);                                                           
      2819 +        if (!lv_ap) {                                        
      2820 +            compile_error("error: va_start: 'ap' is not a loc
           +al variable\n");                                             
      2821 +        }                                                    
      2822 +        int ap_off = lv_ap->offset;                          
      2823 +        /* gp_offset = named_gp_params * 8, already clamped t
           +o ≤ 48 */                                                    
      2824 +        int gp_off_val  = cg->variadic_named_gp_params * 8;  
      2825 +        /* fp_offset = 48 (start of reserved FP/XMM area) */ 
      2826 +        int fp_off_val  = 48;                                
      2827 +        /* overflow_arg_area = rbp + 16 + 8 * named_stack_par
           +ams */                                                       
      2828 +        int overflow_disp = 16 + cg->variadic_named_stack_par
           +ams * 8;                                                     
      2829 +        /* ap[0].gp_offset (unsigned int, 4 bytes at ap+0) */
      2830 +        fprintf(cg->output, "    mov dword [rbp - %d], %d\n",
           + ap_off, gp_off_val);                                        
      2831 +        /* ap[0].fp_offset (unsigned int, 4 bytes at ap+4) */
      2832 +        fprintf(cg->output, "    mov dword [rbp - %d], %d\n",
           + ap_off - 4, fp_off_val);                                    
      2833 +        /* ap[0].overflow_arg_area (void *, 8 bytes at ap+8) 
           +*/                                                           
      2834 +        fprintf(cg->output, "    lea rax, [rbp + %d]\n", over
           +flow_disp);                                                  
      2835 +        fprintf(cg->output, "    mov [rbp - %d], rax\n", ap_o
           +ff - 8);                                                     
      2836 +        /* ap[0].reg_save_area (void *, 8 bytes at ap+16) */ 
      2837 +        fprintf(cg->output, "    lea rax, [rbp - %d]\n", cg->
           +variadic_reg_save_offset);                                   
      2838 +        fprintf(cg->output, "    mov [rbp - %d], rax\n", ap_o
           +ff - 16);                                                    
      2839 +        node->result_type = TYPE_VOID;                       
      2840 +        return;                                              
      2841 +    }                                                        
      2842 +    if (node->type == AST_BUILTIN_VA_END ||                  
      2843          node->type == AST_BUILTIN_VA_COPY) {
      2844          node->result_type = TYPE_VOID;
      2845          return;

● Step 5: Update test/include/stdio.h

● Update(test/include/stdio.h)
  ⎿  Added 5 lines
      3  #define CLAUDEC99_STDIO_H
      4  
      5  #include <stddef.h>
      6 +#include <stdarg.h>                                             
      7  
      8  typedef struct FILE FILE;
      9  
     ...
      18  char *fgets(char *, int, FILE *);
      19  int fprintf(FILE *, const char *, ...);
      20  int snprintf(char *, size_t, const char *, ...);
      21 +                                                               
      22 +int vfprintf(FILE *, const char *, va_list);                   
      23 +int vprintf(const char *, va_list);                            
      24 +int vsnprintf(char *, size_t, const char *, va_list);          
      25  #endif

● Step 6: Update src/version.c

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      3  
      4  #define VERSION_MAJOR  "00"
      5  #define VERSION_MINOR  "01"
      6 -#define VERSION_STAGE  "00750300"                               
      6 +#define VERSION_STAGE  "00750400"                               
      7  
      8  #ifndef VERSION_BUILD
      9  #define VERSION_BUILD  0

● Step 7: Build the compiler and verify compilation succeeds

● Bash(cmake --build build 2>&1)
  ⎿  [  9%] Building C object CMakeFiles/ccompiler.dir/src/compiler.c.o
     [ 18%] Building C object CMakeFiles/ccompiler.dir/src/codegen.c.o
     [ 27%] Building C object CMakeFiles/ccompiler.dir/src/version.c.o
     … +2 lines (ctrl+o to expand)

● Build succeeds. Now let me create the integration test:

● Bash(mkdir -p test/integration/test_va_start_codegen)
  ⎿  Done

● Write(test/integration/test_va_start_codegen/test_va_start_codegen.c)
  ⎿  Wrote 18 lines to
     test/integration/test_va_start_codegen/test_va_start_codegen.c
       1 #include <stdarg.h>
       2 #include <stdio.h>
       3 
       4 void printv(const char *fmt, va_list ap) {
       5     vprintf(fmt, ap);
       6 }
       7 
       8 void print(const char *fmt, ...) {
       9     va_list ap;
      10     va_start(ap, fmt);
     … +8 lines (ctrl+o to expand)

● Write(test/integration/test_va_start_codegen/test_va_start_codegen.expected)
  ⎿  Wrote 1 lines to
     test/integration/test_va_start_codegen/test_va_start_codegen.expected
      1 40 2

● Now let me run the new integration test to verify it works:

● Bash(cd test/integration && bash run_test.sh test_va_start_codegen 2>&1)
  ⎿  Error: Exit code 1
     linking: test_va_start_codegen
     cc: fatal error: no input files
     compilation terminated.

  Read 1 file (ctrl+o to expand)

● Bash(cd test/integration && bash run_test.sh
      test_va_start_codegen/test_va_start_codegen.c 2>&1)
  ⎿  Error: Exit code 1
     compiling: /home/scott/code/claude/claudec99/test/integration/test_va_start
     _codegen/test_va_start_codegen.c
     error: function 'vprintf' parameter '' expected integer argument, got 
     pointer
  Searched for 1 pattern, read 2 files (ctrl+o to expand)

● Found the bug: unnamed array-typedef parameters skip the array-to-pointer
  adjustment (line 2478 returns early with TYPE_ARRAY). I need to fix this in
  the parser:

● Update(src/parser.c)
  ⎿  Added 7 lines, removed 1 line
      2475      if (parser->current.type == TOKEN_COMMA ||
      2476          parser->current.type == TOKEN_RPAREN) {
      2477          ASTNode *param = ast_new(AST_PARAM, "");
      2478 -        param->decl_type = base_kind;                        
      2478 +        if (base_kind == TYPE_ARRAY) {                       
      2479 +            /* C99 6.7.5.3p7: unnamed array param is adjusted
           + to pointer-to-element. */                                   
      2480 +            param->decl_type = TYPE_POINTER;                 
      2481 +            param->full_type = type_pointer(base_type->base);
      2482 +        } else {                                             
      2483 +            param->decl_type = base_kind;                    
      2484 +        }                                                    
      2485          return param;
      2486      }
      2487  

● Bash(cmake --build build 2>&1 && echo "BUILD OK")
  ⎿  [  9%] Building C object CMakeFiles/ccompiler.dir/src/parser.c.o
     [ 18%] Linking C executable ccompiler
     [100%] Built target ccompiler
     BUILD OK

● Bash(cd test/integration && bash run_test.sh
      test_va_start_codegen/test_va_start_codegen.c 2>&1)
  ⎿  Error: Exit code 1
     compiling: /home/scott/code/claude/claudec99/test/integration/test_va_start
     _codegen/test_va_start_codegen.c
     compiled: /home/scott/code/claude/claudec99/test/integration/test_va_start_
     codegen/test_va_start_codegen.c -> test_va_start_codegen.asm
     assembling: test_va_start_codegen.asm
     linking: test_va_start_codegen
     /usr/bin/ld: cannot open output file test_va_start_codegen: Is a directory
     collect2: error: ld returned 1 exit status

  Searched for 1 pattern, read 2 files (ctrl+o to expand)

● The run_test.sh manual invocation has path issues. Let me run the full
  integration suite instead:

● Bash(./test/integration/run_tests.sh 2>&1 | tail -20)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/integration/test_stdl
     ib_malloc_free_include/test_stdlib_malloc_free_include.c -> test_stdli
     b_malloc_free_include.asm
     … +22 lines (ctrl+o to expand)

● The new test passes. Now let me run the full test suite to make sure nothing
  else broke:

● Bash(./test/run_all_tests.sh 2>&1 | tail -15)
  ⎿  PASS  test_stage_16_03_lshift_int
     PASS  test_stage_16_03_lshift_long
     PASS  test_stage_16_03_rshift_int
     … +12 lines (ctrl+o to expand)

● All 1190 tests pass (1189 before + 1 new). Now let me delegate the final
  artifacts to the haiku agent:

● haiku-stage-artifact-writer(Generate stage 75-04 milestone, transcript, and
                             README/grammar updates)
  ⎿  Done (12 tool uses · 31.1k tokens · 41s)
  (ctrl+o to expand)

  Searched for 1 pattern, listed 1 directory (ctrl+o to expand)

● The README wasn't fully updated. Let me check and fix it:

  Searched for 1 pattern (ctrl+o to expand)

● Update(README.md)
  ⎿  Error editing file

  Read 1 file (ctrl+o to expand)

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      244    `static` functions have internal linkage (no `global` NASM d
           irective emitted).
      245    Command-line argument support: `int main(int argc, char **ar
           gv)` signature with
      246    argc and argv[i] access for string arguments passed at progr
           am invocation.
      247 -  Variadic function declarations and definitions (e.g., `int f
          -(int x, ...)`) with caller compatibility checking (actual args
          - >= fixed params); callee-side access to extra arguments (`va_
          -list`, `va_start`, `va_arg`, `<stdarg.h>`) recognizes the `__b
          -uiltin_va_*` forms (parsed and semantically validated; codegen
          - is a no-op in this stage). Code generation emits `xor eax, ea
          -x` before variadic calls to satisfy the SysV AMD64 ABI float-a
          -rgument-count protocol.                                       
      247 +  Variadic function declarations and definitions (e.g., `int f
          +(int x, ...)`) with caller compatibility checking (actual args
          + >= fixed params); callee-side access to extra arguments via `
          +va_list`, `va_start`, `va_end`, `<stdarg.h>`: variadic functio
          +n prologues save all 6 GP argument registers (rdi–r9) to a hid
          +den 304-byte register save area; `__builtin_va_start` initiali
          +zes all four `va_list` fields (gp_offset, fp_offset, overflow_
          +arg_area, reg_save_area) per the SysV AMD64 ABI; `__builtin_va
          +_end` is a no-op; `va_arg` and `va_copy` remain stubs (out of 
          +scope). Code generation emits `xor eax, eax` before variadic c
          +alls to satisfy the SysV AMD64 ABI float-argument-count protoc
          +ol.                                                           
      248  - **Pointers**: pointer types, `&` and `*` as rvalue and lvalu
           e,
      249    assignment through pointer, pointer parameters and return ty
           pes,
      250    `NULL` as a null pointer constant.

● Now fix the "Not yet supported" section and test totals:

  Read 1 file (ctrl+o to expand)

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      379  ## Not yet supported
      380  
      381  Anonymous struct/union members (C11 feature), bit-fields, stru
           ct by-value parameters/return values (pointer-to-struct parame
           ters are now supported); floating-point; block-scope `extern`;
            block-scope `static` arrays and structs;
      382 -callee-side va_* runtime behavior (va_start ABI initialization
          -, va_arg extraction, va_copy—parsing and semantic validation a
          -re implemented but codegen is a no-op);                       
      382 +`va_arg` extraction and `va_copy` (va_start/va_end are now imp
          +lemented; va_arg and va_copy remain stubs); floating-point var
          +iadic arguments;                                              
      383  `#elifdef`/`#elifndef`; pointer-to-function-pointer and functi
           on-returning-function-pointer.
      384  
      385  The authoritative grammar for the supported language is in

● Now update the test totals and the stdio.h stub description:

● Update(README.md)  ⎿  Added 1 line, removed 1 line
      406  ```
      407  
      408  The runner aggregates per-suite results and prints a final
      409 -`Aggregate: P passed, F failed, T total` line. As of stage 75-
          -03 all tests pass (739 valid, 222 invalid, 67 integration, 41 
          -print-AST, 99 print-tokens, 21 print-asm; 1189 total).        
      409 +`Aggregate: P passed, F failed, T total` line. As of stage 75-
          +04 all tests pass (739 valid, 222 invalid, 68 integration, 41 
          +print-AST, 99 print-tokens, 21 print-asm; 1190 total).        
      410  
      411  Individual suites can be run directly, e.g. `./test/valid/run_
           tests.sh`.
      412  

● Now update the stdio.h description in the README to mention the va_*
  declarations:

  Searched for 1 pattern (ctrl+o to expand)

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      149    - _File inclusion_: `#include "file.h"` local inclusion, sea
           rched relative
      150      to the including file's directory; nested includes support
           ed; recursive
      151      includes detected via a depth limit.
      152 -  - _Stub system headers_: controlled stubs for `stdio.h` (wit
          -h opaque `typedef struct FILE FILE` pointer type, `#define EOF
          - (-1)`, and declarations for `fopen`, `fclose`, `fgetc`, `fget
          -s`, `fprintf`, and `snprintf`), `stddef.h`,                   
      152 +  - _Stub system headers_: controlled stubs for `stdio.h` (wit
          +h opaque `typedef struct FILE FILE` pointer type, `#define EOF
          + (-1)`, and declarations for `fopen`, `fclose`, `fgetc`, `fget
          +s`, `fprintf`, `snprintf`, `vfprintf`, `vprintf`, and `vsnprin
          +tf`), `stddef.h`,                                             
      153      `stdlib.h` (with `malloc`, `realloc`, `free`), `string.h` 
           (with `strcmp`, `strlen`, `memcpy`, `memset`, `memcmp`, `strch
           r`), `limits.h` (with `UINT_MAX` and `ULONG_MAX`),
      154      `stdint.h`, `stdbool.h`, `ctype.h` (character classificati
           on and conversion),
      155      `errno.h` (error constants and `errno` macro), `time.h` (`
           time_t`, `clock_t`,

● Now let me do a final full test run to confirm everything still passes:

● Bash(./test/run_all_tests.sh 2>&1 | tail -5)
  ⎿  Results: 21 passed, 0 failed, 21 total

     ====================================================
     … +2 lines (ctrl+o to expand)

● All 1190 tests pass. Let me commit the changes:

● Bash(git status && git diff --stat)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +25 lines (ctrl+o to expand)

● Bash(git add include/codegen.h src/codegen.c src/parser.c src/version.c
      test/include/stdio.h README.md docs/kickoffs/stage-75-04-kickoff.md
      docs/milestones/stage-75-…)
  ⎿  (No output)

● Bash(git log --oneline -3)
  ⎿  36296f5 stage 75-04 spec
     e4ba769 stage 75-04 spec in progress
     279c0eb stage 75-03 session usage and export

● Bash(git commit -m "$(cat <<'EOF'
      stage 75-04 va_start codegen foundation…)
  ⎿  [master 666203b] stage 75-04 va_start codegen foundation
      11 files changed, 320 insertions(+), 13 deletions(-)
      create mode 100644 docs/kickoffs/stage-75-04-kickoff.md
     … +6 lines (ctrl+o to expand)

● Stage 75-04 is complete. Here's a summary of what was done:

  Stage 75-04 — va_start Codegen Foundation

  What changed:

  - include/codegen.h: Added three fields to CodeGen — variadic_reg_save_offset,
   variadic_named_gp_params, variadic_named_stack_params — to track the register
   save area location and named-param counts across the function body.
  - src/codegen.c:
    - Variadic function prologues now add 304 bytes to the stack frame, save all
   6 GP argument registers (rdi–r9) into that hidden register save area, and
  record the counts for va_start.
    - AST_BUILTIN_VA_START replaced its no-op with actual va_list field
  initialization: writes gp_offset, fp_offset, overflow_arg_area, and
  reg_save_area directly to the va_list struct on the stack.
  - src/parser.c: Fixed a bug where unnamed array-typedef parameters (e.g.
  va_list in int vprintf(const char *, va_list)) skipped the C99
  array-to-pointer adjustment in the early-return path.
  - test/include/stdio.h: Added #include <stdarg.h> and declarations for
  vfprintf, vprintf, vsnprintf.
  - Integration test test_va_start_codegen: exercises the full path — print()
  calls va_start, forwards va_list to printv(), which calls libc vprintf;
  expected output 40 2.

  Result: 1190/1190 tests pass (1189 → 1190 with the new integration test).

✻ Churned for 14m 11s