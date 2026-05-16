 ▐▛███▜▌   Claude Code v2.1.143
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

❯ /clear                                                                        
  ⎿  (no content)

❯ /usage 
  ⎿  Settings dialog dismissed

❯ /implement-stage docs/stages/ClaudeC99-spec-stage-50-object-like-macros.md    

● I'll read the spec and supporting files to get started.

  Read 3 files (ctrl+o to expand)

● Now let me explore the current project structure and relevant source files.

  Searched for 1 pattern, listed 1 directory (ctrl+o to expand)

● There's already a preprocessor. Let me read the relevant files.

  Read 2 files (ctrl+o to expand)

● Now let me look at the existing tests and the compiler entry point.

  Read 1 file, listed 2 directories (ctrl+o to expand)

● Let me look at the integration tests and the test runner to understand the
  testing framework.

  Read 2 files, listed 2 directories (ctrl+o to expand)

● Let me look at one more integration test and the valid test runner to
  understand test naming conventions.

  Read 3 files (ctrl+o to expand)

● Let me check the kickoff template before delegating.

  Read 2 files, listed 2 directories (ctrl+o to expand)

● Let me check the grammar file and see if there's a preprocessing section.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me build the project first to confirm it compiles cleanly.

● Bash(make 2>&1 | tail -5)
  ⎿  make: *** No targets specified and no makefile found.  Stop.

  Read 1 file, listed 1 directory (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99/build && make -j4 2>&1 | tail -5)
  ⎿  Consolidate compiler generated dependencies of target ccompiler
     [100%] Built target ccompiler
● Good, the build is clean. Now I have everything I need.

  STAGE_DISPLAY: Stage 50
  STAGE_LABEL: stage-50

  Summary

  Stage 50 adds object-like #define macros to the preprocessor. The preprocessor
   already exists with line splicing, comment stripping, and #include expansion.
   This stage adds:
  - A macro table shared across all preprocessing levels (including included
  files)
  - #define NAME [replacement] directive parsing
  - Identifier-level macro expansion in ordinary source text
  - Duplicate definition checking (compatible = OK, incompatible = error)

  Spec Issue

  One minor typo: "tool pasting ##" should read "token pasting ##" — not a
  blocker.

  Planned Changes

  File: src/preprocessor.c
  Change: Add MacroTable, macro_define, macro_find; add #define parsing; add
    identifier expansion; thread table through recursive calls
  ────────────────────────────────────────
  File: include/preprocessor.h
  Change: Update doc comment to reflect #define support
  ────────────────────────────────────────
  File: test/valid/test_pp_define_value__42.c
  Change: New — basic single define
  ────────────────────────────────────────
  File: test/valid/test_pp_define_array__42.c
  Change: New — define SIZE + VALUE, array usage
  ────────────────────────────────────────
  File: test/valid/test_pp_define_sum_expr__10.c
  Change: New — define SUM 1+2+3+4
  ────────────────────────────────────────
  File: test/valid/test_pp_define_compat_redef__42.c
  Change: New — compatible redefinition
  ────────────────────────────────────────
  File: test/valid/test_pp_define_stmt_macros__2.c
  Change: New — INIT/INC/RETURN as statement macros
  ────────────────────────────────────────
  File: test/integration/test_pp_define_header/
  Change: New dir — macro defined in header, used in main
  ────────────────────────────────────────
  File: test/invalid/test_pp_define_incompat_redef__incompatible_replacement.c
  Change: New — incompatible redefinition error

  No changes to tokenizer, parser, AST, code generator, or compiler.c.

  Delegating the kickoff to haiku now, then I'll implement.

● haiku-stage-artifact-writer(Write Stage 50 kickoff artifact)
  ⎿  Backgrounded agent (↓ to manage · ctrl+o to expand)

● Kickoff is running in the background. Now implementing the changes.

  Step 1: Update src/preprocessor.c

● Write(src/preprocessor.c)
  ⎿  Added 144 lines, removed 7 lines
       56      return dir;
       57  }
       58  
       59 +/* ---- Macro table ------------------------------------------
          +----------- */                                                
       60 +                                                              
       61 +typedef struct {                                              
       62 +    char *name;                                               
       63 +    char *replacement;                                        
       64 +} MacroDef;                                                   
       65 +                                                              
       66 +typedef struct {                                              
       67 +    MacroDef *defs;                                           
       68 +    size_t    count;                                          
       69 +    size_t    cap;                                            
       70 +} MacroTable;                                                 
       71 +                                                              
       72 +static void macro_table_init(MacroTable *t) {                 
       73 +    t->defs  = NULL;                                          
       74 +    t->count = 0;                                             
       75 +    t->cap   = 0;                                             
       76 +}                                                             
       77 +                                                              
       78 +static void macro_table_free(MacroTable *t) {                 
       79 +    for (size_t i = 0; i < t->count; i++) {                   
       80 +        free(t->defs[i].name);                                
       81 +        free(t->defs[i].replacement);                         
       82 +    }                                                         
       83 +    free(t->defs);                                            
       84 +}                                                             
       85 +                                                              
       86 +static MacroDef *macro_find(MacroTable *t, const char *name, s
          +ize_t len) {                                                  
       87 +    for (size_t i = 0; i < t->count; i++) {                   
       88 +        if (strlen(t->defs[i].name) == len &&                 
       89 +            strncmp(t->defs[i].name, name, len) == 0)         
       90 +            return &t->defs[i];                               
       91 +    }                                                         
       92 +    return NULL;                                              
       93 +}                                                             
       94 +                                                              
       95 +static void macro_define(MacroTable *t, const char *name, size
          +_t nlen,                                                      
       96 +                          const char *repl, size_t rlen) {    
       97 +    MacroDef *existing = macro_find(t, name, nlen);           
       98 +    if (existing) {                                           
       99 +        if (strlen(existing->replacement) == rlen &&          
      100 +            strncmp(existing->replacement, repl, rlen) == 0)  
      101 +            return;                                           
      102 +        char tmp[256];                                        
      103 +        size_t copy = nlen < sizeof(tmp) - 1 ? nlen : sizeof(t
          +mp) - 1;                                                      
      104 +        memcpy(tmp, name, copy);                              
      105 +        tmp[copy] = '\0';                                     
      106 +        fprintf(stderr, "error: macro '%s' redefined with inco
          +mpatible replacement\n", tmp);                                
      107 +        exit(1);                                              
      108 +    }                                                         
      109 +                                                              
      110 +    if (t->count == t->cap) {                                 
      111 +        t->cap = t->cap * 2 + 8;                              
      112 +        t->defs = realloc(t->defs, t->cap * sizeof(MacroDef));
      113 +        if (!t->defs) { fprintf(stderr, "error: out of memory\
          +n"); exit(1); }                                               
      114 +    }                                                         
      115 +                                                              
      116 +    char *nname = malloc(nlen + 1);                           
      117 +    char *nrepl = malloc(rlen + 1);                           
      118 +    if (!nname || !nrepl) { fprintf(stderr, "error: out of mem
          +ory\n"); exit(1); }                                           
      119 +    memcpy(nname, name, nlen);                                
      120 +    nname[nlen] = '\0';                                       
      121 +    memcpy(nrepl, repl, rlen);                                
      122 +    nrepl[rlen] = '\0';                                       
      123 +                                                              
      124 +    t->defs[t->count].name        = nname;                    
      125 +    t->defs[t->count].replacement = nrepl;                    
      126 +    t->count++;                                               
      127 +}                                                             
      128 +                                                              
      129  /* ---- Phase 1: line splicing -------------------------------
           ----------- */
      130  
      131  static char *splice_lines(const char *source) {
     ...
      146  
      147  /* ---- Forward declaration for mutual recursion -------------
           ------------ */
      148  
       79 -static char *preprocess_internal(const char *source, const cha
          -r *source_path, int depth);                                   
      149 +static char *preprocess_internal(const char *source, const cha
          +r *source_path,                                               
      150 +                                  int depth, MacroTable *macro
          +s);                                                           
      151  
      152  /* ---- Read and recursively preprocess an included file -----
           ----------- */
      153  
       83 -static char *preprocess_file(const char *path, int depth) {   
      154 +static char *preprocess_file(const char *path, int depth, Macr
          +oTable *macros) {                                             
      155      FILE *f = fopen(path, "r");
      156      if (!f) {
      157          fprintf(stderr, "error: cannot open include file '%s'\
           n", path);
     ...
      165      fread(buf, 1, flen, f);
      166      buf[flen] = '\0';
      167      fclose(f);
       97 -    char *result = preprocess_internal(buf, path, depth);     
      168 +    char *result = preprocess_internal(buf, path, depth, macro
          +s);                                                           
      169      free(buf);
      170      return result;
      171  }
      172  
      102 -/* ---- Phase 2: strip comments and expand #include directives
          - ---------- */                                                
      173 +/* ---- Phase 2: strip comments, expand directives and macros 
          +----------- */                                                
      174  
      104 -static char *preprocess_internal(const char *source, const cha
          -r *source_path, int depth) {                                  
      175 +static char *preprocess_internal(const char *source, const cha
          +r *source_path,                                               
      176 +                                  int depth, MacroTable *macro
          +s) {                                                          
      177      if (depth > MAX_INCLUDE_DEPTH) {
      178          fprintf(stderr, "error: maximum include depth exceeded
            (possible recursive include)\n");
      179          exit(1);
     ...
      272                  include_path[dir_len + 1 + fname_len] = '\0';
      273                  free(dir);
      274  
      203 -                char *included = preprocess_file(include_path,
          - depth + 1);                                                  
      275 +                char *included = preprocess_file(include_path,
          + depth + 1, macros);                                          
      276                  free(include_path);
      277                  gbuf_append(&out, included, strlen(included));
      278                  free(included);
      279                  continue;
      280              }
      281  
      282 +            /* #define NAME [replacement] */                  
      283 +            if (strncmp(s + in, "define", 6) == 0 &&          
      284 +                !isalnum((unsigned char)s[in + 6]) && s[in + 6
          +] != '_') {                                                   
      285 +                in += 6;                                      
      286 +                while (s[in] == ' ' || s[in] == '\t') in++;   
      287 +                                                              
      288 +                if (!isalpha((unsigned char)s[in]) && s[in] !=
          + '_') {                                                       
      289 +                    fprintf(stderr, "error: expected macro nam
          +e after #define\n");                                          
      290 +                    free(out.data);                           
      291 +                    free(spliced);                            
      292 +                    exit(1);                                  
      293 +                }                                             
      294 +                size_t name_start = in;                       
      295 +                while (s[in] && (isalnum((unsigned char)s[in])
          + || s[in] == '_'))                                            
      296 +                    in++;                                     
      297 +                size_t name_len = in - name_start;            
      298 +                                                              
      299 +                /* Reject function-like macros. */            
      300 +                if (s[in] == '(') {                           
      301 +                    fprintf(stderr, "error: function-like macr
          +os are not supported\n");                                     
      302 +                    free(out.data);                           
      303 +                    free(spliced);                            
      304 +                    exit(1);                                  
      305 +                }                                             
      306 +                                                              
      307 +                /* Skip whitespace separating name from replac
          +ement. */                                                     
      308 +                while (s[in] == ' ' || s[in] == '\t') in++;   
      309 +                                                              
      310 +                /* Replacement: rest of line, trailing whitesp
          +ace trimmed. */                                               
      311 +                size_t repl_start = in;                       
      312 +                while (s[in] && s[in] != '\n') in++;          
      313 +                size_t repl_end = in;                         
      314 +                while (repl_end > repl_start &&               
      315 +                       (s[repl_end - 1] == ' ' || s[repl_end -
          + 1] == '\t'))                                                 
      316 +                    repl_end--;                               
      317 +                                                              
      318 +                macro_define(macros,                          
      319 +                             s + name_start, name_len,        
      320 +                             s + repl_start, repl_end - repl_s
          +tart);                                                        
      321 +                /* Directive line consumed; newline handled on
          + next iteration. */                                           
      322 +                continue;                                     
      323 +            }                                                 
      324 +                                                              
      325              /* All other directives are unsupported. */
      326              fprintf(stderr, "error: unsupported preprocessor d
           irective\n");
      327              free(out.data);
     ...
      357              continue;
      358          }
      359  
      360 +        /* Identifier: expand if it names a macro, otherwise p
          +ass through. */                                               
      361 +        if (isalpha((unsigned char)c) || c == '_') {          
      362 +            size_t id_start = in;                             
      363 +            while (s[in] && (isalnum((unsigned char)s[in]) || 
          +s[in] == '_'))                                                
      364 +                in++;                                         
      365 +            size_t id_len = in - id_start;                    
      366 +            MacroDef *def = macro_find(macros, s + id_start, i
          +d_len);                                                       
      367 +            if (def) {                                        
      368 +                gbuf_append(&out, def->replacement, strlen(def
          +->replacement));                                              
      369 +            } else {                                          
      370 +                gbuf_append(&out, s + id_start, id_len);      
      371 +            }                                                 
      372 +            line_has_content = 1;                             
      373 +            continue;                                         
      374 +        }                                                     
      375 +                                                              
      376          /* Regular character. */
      377          line_has_content = 1;
      378          gbuf_push(&out, s[in++]);
     ...
      384  }
      385  
      386  char *preprocess(const char *source, const char *source_path) 
           {
      256 -    return preprocess_internal(source, source_path ? source_pa
          -th : ".", 0);                                                 
      387 +    MacroTable macros;                                        
      388 +    macro_table_init(&macros);                                
      389 +    char *result = preprocess_internal(source,                
      390 +                                       source_path ? source_pa
          +th : ".",                                                     
      391 +                                       0, &macros);           
      392 +    macro_table_free(&macros);                                
      393 +    return result;                                            
      394  }

● Agent "Write Stage 50 kickoff artifact" completed

● Kickoff artifact done. Now updating the header doc comment.

● Update(include/preprocessor.h)
  ⎿  Added 3 lines, removed 2 lines
       6   * relative #include paths).  Pass NULL if there is no filesyst
          em context.
       7   * Returns a heap-allocated string the caller must free.
       8   * Performs line splicing, comment stripping, #include "file" e
          xpansion,
       9 - * and directive detection.                                    
       9 + * and object-like #define macro substitution.                 
      10   * Exits with an error message on unsupported directives, unter
          minated
      11 - * comments, missing include files, or exceeded include depth. 
         -*/                                                             
      11 + * comments, missing include files, exceeded include depth, or 
         +incompatible                                                   
      12 + * macro redefinition. */                                      
      13  char *preprocess(const char *source, const char *source_path);
      14  
      15  #endif

● Now build to verify the implementation compiles cleanly.

● Bash(make -j4 2>&1)
  ⎿  [ 20%] Building C object CMakeFiles/ccompiler.dir/src/preprocessor.c.o
     [ 20%] Building C object CMakeFiles/ccompiler.dir/src/compiler.c.o
     [ 30%] Linking C executable ccompiler
     [100%] Built target ccompiler

● Clean build. Now creating the test files.

● Write(test/valid/test_pp_define_value__42.c)
  ⎿  Wrote 5 lines to test/valid/test_pp_define_value__42.c
      1 #define VALUE 42
      2 
      3 int main() {
      4     return VALUE;
      5 }

● Write(test/valid/test_pp_define_array__42.c)
  ⎿  Wrote 8 lines to test/valid/test_pp_define_array__42.c
      1 #define SIZE 10
      2 #define VALUE 42
      3 
      4 int main() {
      5     int a[SIZE];
      6     a[0] = VALUE;
      7     return a[0];
      8 }

● Write(test/valid/test_pp_define_sum_expr__10.c)
  ⎿  Wrote 5 lines to test/valid/test_pp_define_sum_expr__10.c
      1 #define SUM 1+2+3+4
      2 
      3 int main() {
      4     return SUM;
      5 }

● Write(test/valid/test_pp_define_compat_redef__42.c)
  ⎿  Wrote 6 lines to test/valid/test_pp_define_compat_redef__42.c
      1 #define ANSWER 42
      2 #define ANSWER 42
      3 
      4 int main() {
      5     return ANSWER;
      6 }

● Write(test/valid/test_pp_define_stmt_macros__2.c)
  ⎿  Wrote 9 lines to test/valid/test_pp_define_stmt_macros__2.c
      1 #define INIT int a=1;
      2 #define INC a++;
      3 #define RETURN return a;
      4 
      5 int main() {
      6     INIT
      7     INC
      8     RETURN
      9 }

● Now create the integration test directory and files.

● Bash(mkdir -p
      /home/scott/code/claude/claudec99/test/integration/test_pp_define_header)
  ⎿  Done

● Write(test/integration/test_pp_define_header/constants.h)
  ⎿  Wrote 1 lines to test/integration/test_pp_define_header/constants.h
      1 #define ANSWER 42

● Write(test/integration/test_pp_define_header/test_pp_define_header.c)
  ⎿  Wrote 5 lines to
     test/integration/test_pp_define_header/test_pp_define_header.c
      1 #include "constants.h"
      2 
      3 int main() {
      4     return ANSWER;
      5 }

● Write(test/integration/test_pp_define_header/test_pp_define_header.status)
  ⎿  Wrote 1 lines to
     test/integration/test_pp_define_header/test_pp_define_header.status
      1 42

● Now the invalid test.

● Write(test/invalid/test_pp_define_incompat_redef__incompatible_replacement.c)
  ⎿  Wrote 6 lines to
     test/invalid/test_pp_define_incompat_redef__incompatible_replacement.c
      1 #define ANSWER 42
      2 #define ANSWER 43
      3 
      4 int main() {
      5     return 0;
      6 }
● Now run the full test suite to verify everything passes.

● Bash(make -j4 -s && cd /home/scott/code/claude/claudec99 && bash
      test/run_all_tests.sh 2>&1)
  ⎿  Error: Exit code 1
     Consolidate compiler generated dependencies of target ccompiler
     [100%] Built target ccompiler
     ====================================================
     Running suite: valid
     ====================================================
     compiled: /home/scott/code/claude/claudec99/test/valid/test_add__42.c -> 
     test_add__42.asm
     PASS  test_add__42  (exit code: 42)
     compiled: /home/scott/code/claude/claudec99/test/valid/test_add_a_b__42.c 
     -> test_add_a_b__42.asm
     PASS  test_add_a_b__42  (exit code: 42)
     … +114 lines (ctrl+o to expand)

● There's a failure. Let me check which tests failed specifically.

● Bash(cd /home/scott/code/claude/claudec99 && bash test/run_all_tests.sh 2>&1 |
       grep -E "^FAIL|Results:")
  ⎿  Results: 542 passed, 0 failed, 542 total
     FAIL  test_pp_unsupported_define__unsupported_preprocessor_directive
     (compiler should have rejected this, but succeeded)
     … +9 lines (ctrl+o to expand)
  ⎿  (timeout 2m)

● Two pre-existing invalid tests are now broken. Let me look at them.

  Read 2 files (ctrl+o to expand)

● Let me check when these tests were added and their context.

● Bash(git log --oneline -- test/invalid/test_pp_unsupported_define__unsupported
      _preprocessor_directive.c
      test/invalid/test_pp_unsupported_include__unsupported_preproc…)
  ⎿  f4edb8a stage 48: preprocessor foundation

● Both tests were added in Stage 48 when #define and #include were unsupported.
  Stage 49 added #include support (but didn't remove the #include test — already
   broken before this stage). Stage 50 adds #define support, making both tests
  obsolete. I'll remove them.

● Bash(rm /home/scott/code/claude/claudec99/test/invalid/test_pp_unsupported_def
      ine__unsupported_preprocessor_directive.c
      /home/scott/code/claude/claudec99/test/invali…)
  ⎿  Done

● Bash(bash test/run_all_tests.sh 2>&1 | grep -E "^FAIL|^Results:|Aggregate")
  ⎿  Results: 542 passed, 0 failed, 542 total
     Results: 181 passed, 0 failed, 181 total
     Results: 25 passed, 0 failed, 25 total
     … +4 lines (ctrl+o to expand)
  ⎿  (timeout 2m)

● All 907 tests pass. Let me verify the specific new tests are included.

● Bash(bash test/run_all_tests.sh 2>&1 | grep "pp_define")
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_pp_define_
     array__42.c -> test_pp_define_array__42.asm
     PASS  test_pp_define_array__42  (exit code: 42)
     … +14 lines (ctrl+o to expand)
  ⎿  (timeout 2m)

● All 7 new tests pass. Now commit the changes.

● Bash(git diff --stat HEAD)
  ⎿   .claude/skills/implement-stage/SKILL.md            |   3 +
      include/preprocessor.h                             |   5 +-
      src/preprocessor.c                                 | 151 ++++++++++++
     … +4 lines (ctrl+o to expand)

● Bash(git status)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +19 lines (ctrl+o to expand)

● Let me check the SKILL.md change before including it.

● Bash(git diff .claude/skills/implement-stage/SKILL.md)
  ⎿  diff --git a/.claude/skills/implement-stage/SKILL.md b/.claude/skills/
     implement-stage/SKILL.md
     index 5b89093..2bb020f 100644
     … +20 lines (ctrl+o to expand)

● I also need to check the README before delegating to haiku.

  Read 1 file (ctrl+o to expand)

● All tests pass. Let me commit and then delegate
  README/grammar/milestone/transcript to haiku.

● Bash(git add src/preprocessor.c include/preprocessor.h
      docs/kickoffs/stage-50-kickoff.md test/valid/test_pp_define_value__42.c
      test/valid/test_pp_define_array__42.c…)
  ⎿  (No output)
● Bash(git commit -m "$(cat <<'EOF'
      stage 50 object-like macros…)
  ⎿  [master fbace3f] stage 50 object-like macros
      15 files changed, 309 insertions(+), 19 deletions(-)
      create mode 100644 docs/kickoffs/stage-50-kickoff.md
     … +14 lines (ctrl+o to expand)

● Committed. Now delegating milestone, transcript, grammar, and README updates
  to haiku.

● haiku-stage-artifact-writer(Write Stage 50 milestone, transcript, grammar, and
                              README updates)
  ⎿  Done (12 tool uses · 22.6k tokens · 41s)
  (ctrl+o to expand)

● Let me verify the artifacts were created and check the README and grammar
  updates.

  Searched for 1 pattern, listed 1 directory (ctrl+o to expand)

● The test totals and "Not yet supported" sections need fixing. Let me check
  them.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      248  Anonymous structs, bit-fields, struct by-value parameters/retu
           rn values (pointer-to-struct parameters are now supported);
      249  unions; floating-point; array
      250  typedefs (pointer, function-pointer, and struct typedefs are n
           ow supported); block-scope storage class specifiers;
      251 -variadics; preprocessor macros and conditional compilation (`#
          -if`/`#ifdef`/`#else`/`#endif`); pointer-to-function-pointer an
          -d function-returning-function-pointer;                        
      251 +variadics; function-like macros; conditional compilation (`#if
          +`/`#ifdef`/`#else`/`#endif`); pointer-to-function-pointer and 
          +function-returning-function-pointer;                          
      252  calls with more than 6 arguments.
      253  
      254  The authoritative grammar for the supported language is in

● Update(README.md)
  ⎿  Added 3 lines, removed 3 lines
      275  ```
      276  
      277  The runner aggregates per-suite results and prints a final
      278 -`Aggregate: P passed, F failed, T total` line. As of stage 49 
          -all                                                           
      279 -tests pass (537 valid, 182 invalid, 24 integration, 39 print-A
          -ST,                                                           
      280 -99 print-tokens, 21 print-asm; 902 total).                    
      278 +`Aggregate: P passed, F failed, T total` line. As of stage 50 
          +all                                                           
      279 +tests pass (542 valid, 181 invalid, 25 integration, 39 print-A
          +ST,                                                           
      280 +99 print-tokens, 21 print-asm; 907 total).                    
      281  
      282  Individual suites can be run directly, e.g. `./test/valid/run_
           tests.sh`.
      283  

● Now let me check the grammar.md update.

  Searched for 1 pattern (ctrl+o to expand)

● Good, grammar.md has been updated. Now commit the remaining artifacts.

● Bash(git status)
  ⎿  On branch master
     Your branch is ahead of 'origin/master' by 1 commit.
       (use "git push" to publish your local commits)
     … +14 lines (ctrl+o to expand)

● Bash(git add README.md docs/grammar.md docs/milestones/stage-50-milestone.md
      docs/sessions/stage-50-transcript.md && git commit -m "$(cat <<'EOF'
      stage 50 session ex…)
  ⎿  [master 328ff7e] stage 50 session export and usage
      4 files changed, 117 insertions(+), 6 deletions(-)
      create mode 100644 docs/milestones/stage-50-milestone.md
      create mode 100644 docs/sessions/stage-50-transcript.md

● Stage 50 is complete. Here's what was done:

  Implementation Summary

  Core change — src/preprocessor.c: Added a MacroTable (dynamic array of
  MacroDef name/replacement pairs) threaded through all recursive preprocessing
  calls. When #define NAME [replacement] is encountered, the macro is stored.
  When an identifier is encountered in ordinary source text, it's looked up in
  the table and replaced if found. Replacement text is emitted verbatim (no
  re-scanning).

  Key behaviors:
  - Compatible redefinitions (#define X 42 → #define X 42) silently accepted
  - Incompatible redefinitions → fatal error with "incompatible replacement" in
  message
  - Macros defined in #included headers are visible in the including file
  (shared table)
  - Function-like macros (#define F(x)) are rejected with a clear error
  - String/character literals are never scanned for macro names

  Tests added (7 new, 2 deleted):
  - 5 valid tests covering: basic value, array sizing, expression replacement,
  compatible redefinition, statement macros
  - 1 integration test: macro in header file
  - 1 invalid test: incompatible redefinition
  - 2 obsolete Stage 48 tests deleted (both directives are now supported)

  Final result: 907 tests pass, 0 failures.

✻ Churned for 10m 27s