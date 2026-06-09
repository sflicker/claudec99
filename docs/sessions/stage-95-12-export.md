 ▐▛███▜▌   Claude Code v2.1.169
▝▜█████▛▘  Opus 4.8 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

  Feature of the week: /loop — run a prompt or slash command on a recurring interval


❯ /implement-stage ./docs/stages/ClaudeC99-spec-stage-95-12-fix-cond-unary-over 
flow-and-dynamic-switch-labels.md                                               

● I'll start by reading the spec file and supporting materials.

  Read 4 files (ctrl+o to expand)

● STAGE_DISPLAY: stage-95-12

  Let me examine the source files I'll be modifying.

  Searched for 2 patterns, read 3 files (ctrl+o to expand)

● Now let me look at the codegen.c call sites for switch labels.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Now let me review the Vec and StrBuf APIs and check how the C0-compatible
  vec_get pattern is used elsewhere.

  Searched for 1 pattern, read 3 files (ctrl+o to expand)

● I now have a complete picture. Here's my analysis.

  Spec Summary — stage-95-12

  STAGE_DISPLAY: stage-95-12

  Two fixed-capacity cleanups (continuing the 95-xx theme), done one at a time
  with a commit between:

  Task 1 — eval_cond_unary ops[32] overflow (src/preprocessor.c)
  An unchecked stack write (ops[nops++]) overflows on a #if with >32 chained
  unary ops (! - + ~). Replace the fixed array with a dynamic StrBuf: append
  each op as consumed, iterate in reverse to apply. Free on every return path.
  Must preserve right-to-left folding (!-1 ≠ -!1). Not allowed to just add a
  bounds check.

  Task 2 — dynamic switch label tables (include/codegen.h, src/codegen.c)
  Replace SwitchCtx's parallel nodes[256]/labels[256] with a Vec entries of
  SwitchLabel {ASTNode *node; int label;}; count → entries.len. Remove
  MAX_SWITCH_LABELS from include/constants.h. Critical lifecycle: init entries
  before vec_push (move semantics), use the live element via vec_get, and
  vec_free(&ctx->entries) before vec_pop.

  No ambiguities or grammar issues. The spec is concrete, names exact call
  sites, and flags the two known bootstrap pitfalls (*(T**)vec_get cast-of-deref
  must be split; signed-division guards). The suggested shapes match the
  existing 95-xx conventions.

  Planned Changes

  ┌───────┬─────────────────────┬───────────────────────────────────────────┐
  │ Order │        File         │                  Change                   │
  ├───────┼─────────────────────┼───────────────────────────────────────────┤
  │ 1.    │                     │ Add #include "strbuf.h"; rewrite          │
  │ Task  │ src/preprocessor.c  │ eval_cond_unary to use StrBuf, free       │
  │ 1     │                     │ before return                             │
  ├───────┼─────────────────────┼───────────────────────────────────────────┤
  │ 1.    │ test/valid/         │ Long unary chain #if case asserting       │
  │ test  │                     │ folded result via exit code               │
  ├───────┼─────────────────────┼───────────────────────────────────────────┤
  │ 2.    │                     │ Add SwitchLabel; convert SwitchCtx to Vec │
  │ Task  │ include/codegen.h   │  entries + default_label                  │
  │ 2     │                     │                                           │
  ├───────┼─────────────────────┼───────────────────────────────────────────┤
  │ 2.    │                     │ Update collect_switch_labels,             │
  │ Task  │ src/codegen.c       │ switch_lookup_label, AST_SWITCH_STATEMENT │
  │ 2     │                     │  handler + lifecycle                      │
  ├───────┼─────────────────────┼───────────────────────────────────────────┤
  │ 2.    │                     │                                           │
  │ Task  │ include/constants.h │ Remove MAX_SWITCH_LABELS                  │
  │ 2     │                     │                                           │
  ├───────┼─────────────────────┼───────────────────────────────────────────┤
  │ 2.    │ test/valid/         │ switch with >256 cases, verify correct    │
  │ test  │                     │ arm at runtime                            │
  ├───────┼─────────────────────┼───────────────────────────────────────────┤
  │ close │ src/version.c       │ VERSION_STAGE "00951200"                  │
  ├───────┼─────────────────────┼───────────────────────────────────────────┤
  │       │                     │ fixed-capacity-inventory.md, README,      │
  │ close │ docs                │ supplemental docs, grammar (no grammar    │
  │       │                     │ change)                                   │
  └───────┴─────────────────────┴───────────────────────────────────────────┘

  Implementation order: Task 1 (preprocessor) → test → commit → Task 2
  (codegen.h/constants.h/codegen.c) → test → commit → self-host → docs.

  Let me generate the kickoff artifact, then start Task 1.

● haiku-stage-artifact-writer(Write stage 95-12 kickoff artifact)
  ⎿  Done (3 tool uses · 10.7k tokens · 20s)
  (ctrl+o to expand)

● Kickoff written. Now Task 1 — rewriting eval_cond_unary to use a StrBuf.

● Update(src/preprocessor.c)
  ⎿  Added 1 line
       5  #include <time.h>
       6  #include "constants.h"
       7  #include "preprocessor.h"
       8 +#include "strbuf.h"                                            
       9  #include "util.h"
      10  
      11  typedef struct {

● Update(src/preprocessor.c)
  ⎿  Added 12 lines, removed 7 lines
      816  /* Unary expression: optional leading !, -, +, ~ (chained) the
           n primary. */
      817  static long eval_cond_unary(const char *s, size_t *in, MacroTa
           ble *macros,
      818                               char *out_data, char *spliced_buf
           ) {
      819 -    char ops[32];                                             
      820 -    int  nops = 0;                                            
      819 +    /* Stage 95-12: collect the leading unary-operator run int
          +o a dynamic                                                   
      820 +     * StrBuf instead of a fixed char[32], so an arbitrarily l
          +ong chain                                                     
      821 +     * (e.g. `#if !!!...1`) folds with no cap and no overflow.
          + */                                                           
      822 +    StrBuf ops;                                               
      823 +    strbuf_init(&ops);                                        
      824  
      825      while (s[*in] == '!' || s[*in] == '-' || s[*in] == '+' || 
           s[*in] == '~') {
      823 -        ops[nops++] = s[(*in)++];                             
      826 +        strbuf_append_char(&ops, s[(*in)++]);                 
      827          while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
      828      }
      829  
      830      long value = eval_cond_primary(s, in, macros, out_data, sp
           liced_buf);
      831  
      829 -    for (int i = nops - 1; i >= 0; i--) {                     
      830 -        if      (ops[i] == '!') value = (value == 0) ? 1L : 0L
          -;                                                             
      831 -        else if (ops[i] == '-') value = -value;               
      832 -        else if (ops[i] == '~') value = ~value;               
      832 +    /* Apply right-to-left (operators do not commute: `!-1` !=
          + `-!1`). */                                                   
      833 +    for (int i = (int)ops.len - 1; i >= 0; i--) {             
      834 +        if      (ops.data[i] == '!') value = (value == 0) ? 1L
          + : 0L;                                                        
      835 +        else if (ops.data[i] == '-') value = -value;          
      836 +        else if (ops.data[i] == '~') value = ~value;          
      837      }
      838  
      839 +    strbuf_free(&ops);                                        
      840      return value;
      841  }
      842  

● Now let me build and create the Task 1 test. Let me check the test/valid
  format first.

● Bash(ls test/valid/ | grep -i "if\|cond\|preproc" | head; echo "---"; cmake
      --build build 2>&1 | tail -5)
  ⎿  test_basic_if__42.c
     test_call_in_if_cond__42.c
     test_comma_in_if__10.c
     … +11 lines (ctrl+o to expand)

  Searched for 1 pattern, read 1 file, listed 1 directory (ctrl+o to expand)

● I'll create the Task 1 test. I'll use a chain of 50 -~ pairs (100 operators)
  applied to 0: each -~x folds right-to-left to x+1, so the chain yields exactly
  50 — this is long (>32, proves no overflow) and order-sensitive (pins
  right-to-left folding, since left-to-right would differ).

● Bash(cd /home/scott/code/claude/claudec99
      chain=$(python3 -c "print('-~'*50)")…)
  ⎿  wrote:
     int main(void) { return 42; }
     #else

● Let me verify this test passes with the fixed build, and confirm the pre-fix
  code would have crashed.

● Bash(cd /home/scott/code/claude/claudec99
      ./build/ccompiler -I test/include
      test/valid/test_pp_if_long_unary_chain__42.c -o /tmp/t.asm 2>&1 && nasm -f
      elf64 /tmp/t.a…)
  ⎿  usage: ccompiler [--version] [--print-ast | --print-tokens] [-Werror]
     [--max-errors=N] [--sysroot=<dir>] [-DNAME[=VAL]] [-I<dir>] <source.c>
     exit=1
● Bash(cd /home/scott/code/claude/claudec99
      cp test/valid/test_pp_if_long_unary_chain__42.c /tmp/t.c…)
  ⎿  compiled: /tmp/t.c -> t.asm
     nasm: fatal: unable to open input file `/tmp/t.asm' No such file or di
     rectory
     exit=1

● Bash(cd /tmp && /home/scott/code/claude/claudec99/build/ccompiler -I
      /home/scott/code/claude/claudec99/test/include /tmp/t.c && nasm -f elf64
      /tmp/t.asm -o /tmp/t.o…)
  ⎿  compiled: /tmp/t.c -> t.asm
     exit=42
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Exit 42 — correct. Now run the full valid suite to confirm no regression, and
  verify the old code crashed (via git stash of just the fix is overkill; the
  suite is the real check).

● Bash(./test/valid/run_tests.sh 2>&1 | tail -5)
  ⎿  PASS  test_write_through_pointer__9  (exit code: 9)
     compiled: /home/scott/code/claude/claudec99/test/valid/test_xor_assign
     __5.c -> test_xor_assign__5.asm
     … +3 lines (ctrl+o to expand)

● Task 1 done (832 valid, +1 new). Let me run the full suite before committing
  Task 1.

● Bash(./test/run_all_tests.sh 2>&1 | tail -12)
  ⎿  PASS  test_stage_16_04_bitand_int
     PASS  test_stage_16_04_bitand_long
     PASS  test_stage_16_04_bitor_int
     … +9 lines (ctrl+o to expand)

● All 1480 pass. Committing Task 1.

● Bash(cd /home/scott/code/claude/claudec99
      git add src/preprocessor.c test/valid/test_pp_if_long_unary_chain__42.c
      docs/kickoffs/stage-95-12-kickoff.md…)
  ⎿  committed

● Now Task 2 — dynamic switch label tables. First include/codegen.h:

● Update(include/codegen.h)
  ⎿  Added 10 lines, removed 3 lines
       91   * switches) and assigns each a unique label id. During body e
           mission
       92   * a case/default node is matched against this table to emit i
           ts
       93   * pre-assigned label. */
       94 +/* Stage 95-12: one case/default label entry. Pairing the node
          + pointer with                                                 
       95 + * its label id keeps the two aligned by construction. */     
       96  typedef struct {
       95 -    ASTNode *nodes[MAX_SWITCH_LABELS];                        
       96 -    int labels[MAX_SWITCH_LABELS];                            
       97 -    int count;                                                
       97 +    ASTNode *node;                                            
       98 +    int label;                                                
       99 +} SwitchLabel;                                                
      100 +                                                              
      101 +typedef struct {                                              
      102 +    /* Stage 95-12: label table is now dynamic (was parallel f
          +ixed arrays                                                   
      103 +     * capped at MAX_SWITCH_LABELS). `count` is now `entries.l
          +en`. */                                                       
      104 +    Vec entries;  /* SwitchLabel */                           
      105      int default_label;
      106  } SwitchCtx;
      107  

● Now collect_switch_labels and switch_lookup_label in codegen.c. Note the C0
  cast-of-deref caveat — I'll split (SwitchLabel *)vec_get(...) into two
  statements where it's a deref pattern, but vec_get returns a pointer I
  dereference via ->, which is fine.

● Update(src/codegen.c)
  ⎿  Added 8 lines, removed 14 lines
      439  static void collect_switch_labels(CodeGen *cg, ASTNode *node, 
           SwitchCtx *ctx) {
      440      if (!node) return;
      441      if (node->type == AST_CASE_SECTION) {
      442 -        if (ctx->count >= MAX_SWITCH_LABELS) {                
      443 -            compile_error( "error: too many case/default label
          -s in switch (max %d)\n",                                      
      444 -                    MAX_SWITCH_LABELS);                       
      445 -        }                                                     
      446 -        ctx->nodes[ctx->count] = node;                        
      447 -        ctx->labels[ctx->count] = cg->label_count++;          
      448 -        ctx->count++;                                         
      442 +        SwitchLabel entry;                                    
      443 +        entry.node = node;                                    
      444 +        entry.label = cg->label_count++;                      
      445 +        vec_push(&ctx->entries, &entry);                      
      446          if (node->child_count > 1) {
      447              collect_switch_labels(cg, node->children[1], ctx);
      448          }
      449          return;
      450      }
      451      if (node->type == AST_DEFAULT_SECTION) {
      455 -        if (ctx->count >= MAX_SWITCH_LABELS) {                
      456 -            compile_error( "error: too many case/default label
          -s in switch (max %d)\n",                                      
      457 -                    MAX_SWITCH_LABELS);                       
      458 -        }                                                     
      452          int lbl = cg->label_count++;
      460 -        ctx->nodes[ctx->count] = node;                        
      461 -        ctx->labels[ctx->count] = lbl;                        
      462 -        ctx->count++;                                         
      453 +        SwitchLabel entry;                                    
      454 +        entry.node = node;                                    
      455 +        entry.label = lbl;                                    
      456 +        vec_push(&ctx->entries, &entry);                      
      457          ctx->default_label = lbl;
      458          if (node->child_count > 0) {
      459              collect_switch_labels(cg, node->children[0], ctx);

● Update(src/codegen.c)
  ⎿  Added 3 lines, removed 2 lines
      469  }
      470  
      471  static int switch_lookup_label(SwitchCtx *ctx, ASTNode *node) 
           {
      472 -    for (int i = 0; i < ctx->count; i++) {                    
      473 -        if (ctx->nodes[i] == node) return ctx->labels[i];     
      472 +    for (int i = 0; i < (int)ctx->entries.len; i++) {         
      473 +        SwitchLabel *e = (SwitchLabel *)vec_get(&ctx->entries,
          + i);                                                          
      474 +        if (e->node == node) return e->label;                 
      475      }
      476      return -1;
      477  }

● Now the AST_SWITCH_STATEMENT handler — lifecycle (init before push, free
  before pop) and dispatch loop:

● Update(src/codegen.c)
  ⎿  Added 4 lines, removed 1 line
      4462           * targets the switch-end label. */
      4463          int label_id = cg->label_count++;
      4464          {
      4465 +            /* Stage 95-12: init the embedded entries Vec BEF
           +ORE the push.                                                
      4466 +             * vec_push bit-copies the struct (a move of the 
           +Vec's data                                                   
      4467 +             * pointer) into the stack slot; the local must n
           +ot also free it. */                                          
      4468              SwitchCtx new_ctx;
      4466 -            new_ctx.count = 0;                               
      4469 +            vec_init(&new_ctx.entries, sizeof(SwitchLabel)); 
      4470              new_ctx.default_label = -1;
      4471              vec_push(&cg->switch_stack, &new_ctx);
      4472          }

● Update(src/codegen.c)
  ⎿  Added 4 lines, removed 3 lines
      4479          fprintf(cg->output, "    push rax\n");
      4480          cg->push_depth++;
      4481  
      4482 -        for (int i = 0; i < ctx->count; i++) {               
      4483 -            ASTNode *label_node = ctx->nodes[i];             
      4482 +        for (int i = 0; i < (int)ctx->entries.len; i++) {    
      4483 +            SwitchLabel *entry = (SwitchLabel *)vec_get(&ctx-
           +>entries, i);                                                
      4484 +            ASTNode *label_node = entry->node;               
      4485              if (label_node->type == AST_CASE_SECTION) {
      4486                  fprintf(cg->output, "    mov eax, [rsp]\n");
      4487                  fprintf(cg->output, "    cmp eax, %s\n",
      4488                          label_node->children[0]->value);
      4489                  fprintf(cg->output, "    je .L_switch_sec_%d\
            n",
      4489 -                        ctx->labels[i]);                     
      4490 +                        entry->label);                       
      4491              }
      4492          }
      4493          if (ctx->default_label != -1) {

● Now free the inner Vec before vec_pop:

● Update(src/codegen.c)
  ⎿  Added 4 lines
      4507          codegen_statement(cg, body, is_main);
      4508  
      4509          cg->switch_depth--;
      4510 +        /* Stage 95-12: release the inner label table before 
           +popping the                                                  
      4511 +         * SwitchCtx (vec_pop only decrements len; it does no
           +t free the                                                   
      4512 +         * embedded Vec), otherwise each switch leaks its lab
           +el table. */                                                 
      4513 +        vec_free(&ctx->entries);                             
      4514          vec_pop(&cg->switch_stack);
      4515          vec_pop(&cg->break_stack);
      4516          cg->break_depth--;

● Now remove MAX_SWITCH_LABELS from constants.h. Let me confirm no other
  references remain.

● Bash(cd /home/scott/code/claude/claudec99; grep -rn "MAX_SWITCH_LABELS" src
      include test docs 2>/dev/null)
  ⎿  include/constants.h:44:#ifndef MAX_SWITCH_LABELS
     include/constants.h:47:#define MAX_SWITCH_LABELS   256
     include/codegen.h:103:     * capped at MAX_SWITCH_LABELS). `count` is
     … +144 lines (ctrl+o to expand)

● Only code references are in include/constants.h (the comment in codegen.h is
  intentional). Removing the constant:

● Update(include/constants.h)
  ⎿  Removed 8 lines
      39  #define MAX_CALL_LAYOUT_ITEMS 24
      40  #endif
      41  
      42 -/* ---- Codegen limits ---- */                                 
      43 -                                                               
      44 -#ifndef MAX_SWITCH_LABELS                                      
      45 -/* Stage 92: raised from 64 so the compiler can self-compile. t
         -oken_type_name()                                               
      46 - * in compiler.c switches over ~83 token kinds in a single swit
         -ch. */                                                         
      47 -#define MAX_SWITCH_LABELS   256                                
      48 -#endif                                                         
      49 -                                                               
      42  /* ---- Preprocessor limits ---- */
      43  
      44  #ifndef MAX_INCLUDE_DEPTH

● Bash(cd /home/scott/code/claude/claudec99; cmake --build build 2>&1 | tail -5)  ⎿  /home/scott/code/claude/claudec99/include/type.h:60:9: note: ‘param_co
     unt’ declared here
        60 |     int param_count;
     … +3 lines (ctrl+o to expand)

● Build succeeds (the note is a pre-existing unrelated warning). Now the Task 2
  test — a switch with >256 cases:

● Bash(cd /home/scott/code/claude/claudec99
      python3 - <<'PY'…)
  ⎿  277 % 200 = 77
     4: * (0..299). The selector 277 (> 256) hits `case 277: return 77;`. *
     /
     284:        case 277: return 77;

● Bash(cd /tmp && cp /home/scott/code/claude/claudec99/test/valid/test_switch_ov
      er_256_labels__77.c /tmp/s.c
      /home/scott/code/claude/claudec99/build/ccompiler -I /home…)
  ⎿  compiled: /tmp/s.c -> s.asm
     exit=77
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Exit 77 — correct. Now the full suite:

● Bash(cd /home/scott/code/claude/claudec99; ./test/run_all_tests.sh 2>&1 | tail
      -6)
  ⎿  Results: 21 passed, 0 failed, 21 total

     ====================================================
     … +2 lines (ctrl+o to expand)

● One failure. Let me find it.

● Bash(cd /home/scott/code/claude/claudec99; ./test/run_all_tests.sh 2>&1 | grep
      -i "fail")
  ⎿  Results: 106 passed, 0 failed, 106 total
     Results: 59 passed, 0 failed, 59 total
     Results: 165 passed, 0 failed, 165 total
     … +8 lines (ctrl+o to expand)

● Bash(cd /tmp && cp /home/scott/code/claude/claudec99/test/valid/test_switch_ne
      sted_20_deep__20.c /tmp/n.c
      /home/scott/code/claude/claudec99/build/ccompiler -I /home/…)
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Bash(cd /tmp && /home/scott/code/claude/claudec99/build/ccompiler -I
      /home/scott/code/claude/claudec99/test/include /tmp/n.c; echo "cc
      exit=$?"; ls -la /tmp/n.asm 2>…)
  ⎿  /bin/bash: line 1: 369583 Segmentation fault      (core dumped) /home/
     scott/code/claude/claudec99/build/ccompiler -I /home/scott/code/claude
     /claudec99/test/include /tmp/n.c
     … +2 lines (ctrl+o to expand)
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Found it — a dangling-pointer bug from my Task 2 lifecycle. When nested
  switches push onto switch_stack, the Vec can reallocate, invalidating the
  outer handler's ctx pointer. The original code never dereferenced ctx after
  emitting the body, but my vec_free(&ctx->entries) does. I need to re-fetch the
  live element after the body before freeing.

● Update(src/codegen.c)
  ⎿  Added 7 lines, removed 2 lines
      4509          cg->switch_depth--;
      4510          /* Stage 95-12: release the inner label table before 
            popping the
      4511           * SwitchCtx (vec_pop only decrements len; it does no
            t free the
      4512 -         * embedded Vec), otherwise each switch leaks its lab
           -el table. */                                                 
      4513 -        vec_free(&ctx->entries);                             
      4512 +         * embedded Vec), otherwise each switch leaks its lab
           +el table.                                                    
      4513 +         * Re-fetch the live top element: emitting the body m
           +ay have pushed                                               
      4514 +         * (and popped) nested switches, which can reallocate
           + switch_stack                                                
      4515 +         * and leave the earlier `ctx` pointer dangling. */  
      4516 +        SwitchCtx *top = (SwitchCtx *)vec_get(&cg->switch_sta
           +ck,                                                          
      4517 +                                              cg->switch_stac
           +k.len - 1);                                                  
      4518 +        vec_free(&top->entries);                             
      4519          vec_pop(&cg->switch_stack);
      4520          vec_pop(&cg->break_stack);
      4521          cg->break_depth--;

● Bash(cd /home/scott/code/claude/claudec99; cmake --build build 2>&1 | grep -i
      "error\|Built target" | tail -2
      cd /tmp && /home/scott/code/claude/claudec99/build/ccom…)
  ⎿  [100%] Built target ccompiler
     compiled: /tmp/n.c -> n.asm
     nested cc exit=0
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Fixed. Full suite again:

● Bash(cd /home/scott/code/claude/claudec99; ./test/run_all_tests.sh 2>&1 | tail
      -4)
  ⎿  ====================================================
     Aggregate: 1481 passed, 0 failed, 1481 total
     ====================================================

● All 1481 pass (+2 new tests). Now update version.c to stage 00951200:

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      3  
      4  #define VERSION_MAJOR  "00"
      5  #define VERSION_MINOR  "02"
      6 -#define VERSION_STAGE  "00951100"                               
      6 +#define VERSION_STAGE  "00951200"                               
      7  
      8  #ifndef VERSION_BUILD
      9  #define VERSION_BUILD  0

● Bash(cd /home/scott/code/claude/claudec99; cmake --build build 2>&1 | grep -i
      "built target"; git add -A && git status --short)
  ⎿  [100%] Built target ccompiler
     M  include/codegen.h
     M  include/constants.h
     … +3 lines (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99
      git commit -q -m "stage 95-12 task 2: dynamic switch label table…)
  ⎿  done

● Both tasks committed. Now the Self-Host Phase. My new code uses (SwitchLabel 
  *)vec_get(...) (cast-of-pointer, then ->) — not the *(T**)vec_get
  cast-of-deref pattern the spec warns about — so it should bootstrap cleanly.
  Running the full C0→C1→C2 cycle:

● Bash(cd /home/scott/code/claude/claudec99; ./build.sh --mode=self-host 2>&1 |
      tail -40)
  ⎿  PASS  test_token_volatile
     PASS  test_token_while
     … +37 lines (ctrl+o to expand)
  ⎿  (timeout 10m)
● Self-host succeeded — C0/C1/C2 all built, all 1481 tests pass at each stage.
  Now update the self-compilation report. Let me read its current top section.

  Read 1 file (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99; grep -n "^## \|^###
      \|Latest\|95-1\|Bootstrap run\|Recent\|History\|result table\|Result"
      docs/self-compilation-report.md…)
  ⎿  14:## Status
     31:## Why the very first report was wrong
     48:## Pass 1 — blockers found and fixed during validation
     … +25 lines (ctrl+o to expand)

  Read 1 file (ctrl+o to expand)

● I'll insert the stage 95-12 section right after Pass 2 (before the 95-11
  section at line 139), matching the established format. Let me get the exact
  BuiltBy strings from the binaries.

● Bash(cd /home/scott/code/claude/claudec99
      for b in c0 c1 c2; do echo "== $b =="; ./build/ccompiler-$b --version;
      done)
  ⎿  == c0 ==
     ClaudeC99 version 00.02.00951200.00778
     BuiltBy: GNU_13_3_0
     … +6 lines (ctrl+o to expand)

● Update(docs/self-compilation-report.md)
  ⎿  Added 23 lines
      136  
      137  After both fixes all 1478 tests passed at C0, C1, and C2.
      138  
      139 +## Issues found during stage 95-12 self-hosting test          
      140 +                                                              
      141 +None. The preprocessor `eval_cond_unary` StrBuf conversion and
          + the `SwitchCtx`                                              
      142 +dynamic label-table conversion produced no bootstrap failures.
          + The new                                                      
      143 +`(SwitchLabel *)vec_get(...)` accesses use the cast-of-pointer
          + form (then `->`),                                            
      144 +not the `*(T **)expr` cast-of-deref pattern C0 cannot parse, s
          +o no split was                                                
      145 +needed. A dangling-pointer bug in the switch lifecycle (the po
          +st-body                                                       
      146 +`vec_free(&ctx->entries)` reused a `ctx` pointer invalidated w
          +hen nested                                                    
      147 +switches reallocated `switch_stack`) was caught by the existin
          +g                                                             
      148 +`test_switch_nested_20_deep` test during the GCC test run and 
          +fixed before the                                              
      149 +bootstrap by re-fetching the live top element via `vec_get`. A
          +ll 1481 tests                                                 
      150 +passed at C0, C1, and C2.                                     
      151 +                                                              
      152 +## Result (stage 95-12)                                       
      153 +                                                              
      154 +**Date:** 2026-06-08                                          
      155 +                                                              
      156 +| Step | Binary | Version | BuiltBy | Tests |                 
      157 +|------|--------|---------|---------|-------|                 
      158 +| C0 | `build/ccompiler-c0` | `00.02.00951200.00778` | `GNU_13
          +_3_0` | 1481/1481 |                                           
      159 +| C1 | `build/ccompiler-c1` | `00.02.00951200.00779` | `Claude
          +C99_v00_02_00951200_00778` | 1481/1481 |                      
      160 +| C2 | `build/ccompiler-c2` | `00.02.00951200.00780` | `Claude
          +C99_v00_02_00951200_00779` | 1481/1481 |                      
      161 +                                                              
      162  ## Issues found during stage 95-11 self-hosting test
      163  
      164  None. The codegen struct `const char *` pointer migrations and
            `user_labels` Vec

● Now update the report header to the current stage, then commit.

● Update(docs/self-compilation-report.md)
  ⎿  Added 2 lines, removed 2 lines
      1  # Self-Compilation Diagnostic Report
      2  
      3 -**Date:** 2026-06-07                                            
      4 -**Stage:** stage-95-08 (replace Token.value fixed buffer with po
        -inter+length fields; lexer string arena)                        
      3 +**Date:** 2026-06-08                                            
      4 +**Stage:** stage-95-12 (fix #if unary-operator overflow; dynamic
        + switch label tables)                                           
      5  **Compiler:** `build/ccompiler` (C0, gcc-built → C1 → C2 via boo
         tstrap)
      6  **Method:** `./build.sh --mode=self-host` (added in stage 94):
      7  archives previous named binaries, saves GCC-built binary as `cco
         mpiler-c0`,

● Bash(cd /home/scott/code/claude/claudec99
      git add docs/self-compilation-report.md…)
  ⎿  done

● Now the Documentation Phase. First, docs/fixed-capacity-inventory.md — I'll
  handle this myself for accuracy. Let me look at the relevant sections.

● Bash(cd /home/scott/code/claude/claudec99; grep -n
      "ops\[\|MAX_SWITCH_LABELS\|Open issue\|## Summary\|Open
      Issue\|unchecked\|eval_cond_unary\|PENDING\|DONE" docs/fix…)
  ⎿  13:- **Status** — PENDING until a future stage replaces with dynamic s
     torage
     21:| ~~`MAX_NAME_LEN`~~ | ~~256 bytes~~ | ~~`include/constants.h`~~ —
     … +151 lines (ctrl+o to expand)

● Let me read the ops section and the Summary/Open-issue callout precisely.

  Read 1 file (ctrl+o to expand)

● Now I'll update the inventory. First the MAX_SWITCH_LABELS main table row
  (line 66):

● Update(docs/fixed-capacity-inventory.md)
  ⎿  Added 1 line, removed 1 line
      63  | `MAX_GLOBALS` | 256 | ~~`include/constants.h`; `include/codeg
          en.h` (`CodeGen.globals[]`); `src/codegen.c`~~ | ~~`compile_err
          or` — "too many global variables (max %d)"~~ | ~~`GlobalVar *gv
           = codegen_find_global(...)` — returned and used locally; never
           stored~~ | ~~YES~~ | ~~MEDIUM~~ | ✓ DONE (stage 95-05) |
      64  | `MAX_BREAK_DEPTH` | 32 | ~~`include/constants.h`; `include/co
          degen.h` (`CodeGen.break_stack[]`); `src/codegen.c`~~ | ~~**No 
          check** — `break_stack` is written at `cg->break_depth` without
           a bounds test before any of the four write sites (while, do-wh
          ile, for, switch). Exceeding 32 nesting levels silently corrupt
          s adjacent `CodeGen` fields.~~ | ~~No — accessed only by index 
          `cg->break_depth`; no pointers into slots~~ | ~~YES~~ | ~~HIGH~
          ~ | ✓ DONE (stage 95-06) |
      65  | `MAX_SWITCH_DEPTH` | 16 | ~~`include/constants.h`; `include/c
          odegen.h` (`CodeGen.switch_stack[]`); `src/codegen.c`~~ | ~~`co
          mpile_error` — "switch nesting exceeds max depth %d" (checked b
          efore writing)~~ | ~~`SwitchCtx *ctx = &cg->switch_stack[...]` 
          — local, used immediately~~ | ~~YES~~ | ~~LOW~~ | ✓ DONE (stage
           95-07) |
      66 -| `MAX_SWITCH_LABELS` | 256 | `include/constants.h`; `include/c
         -odegen.h` (`SwitchCtx.nodes[]` and `SwitchCtx.labels[]` embedde
         -d in `SwitchCtx`); `src/codegen.c` | `compile_error` — "too man
         -y case/default labels in switch (max %d)" | `SwitchCtx.nodes[]`
         - stores `ASTNode *` from the AST (not aliases into the array it
         -self) | NO (arrays are embedded in `SwitchCtx` which is embedde
         -d in `CodeGen.switch_stack[]`; making them dynamic requires hea
         -p allocation inside `SwitchCtx`) | LOW | PENDING |             
      66 +| `MAX_SWITCH_LABELS` | 256 | ~~`include/constants.h`; `include
         +/codegen.h` (`SwitchCtx.nodes[]` and `SwitchCtx.labels[]` embed
         +ded in `SwitchCtx`); `src/codegen.c`~~ | ~~`compile_error` — "t
         +oo many case/default labels in switch (max %d)"~~ | ~~`SwitchCt
         +x.nodes[]` stores `ASTNode *` from the AST (not aliases into th
         +e array itself)~~ | ~~NO (arrays are embedded in `SwitchCtx`…)~
         +~ — done by giving `SwitchCtx` an embedded `Vec entries` of a `
         +SwitchLabel{node,label}` pair, init'd before push and freed bef
         +ore pop | LOW | ✓ DONE (stage 95-12) |                         
      67  | `MAX_USER_LABELS` | 64 | ~~`include/constants.h`; `include/co
          degen.h` (`CodeGen.user_labels[][MAX_NAME_LEN]`); `src/codegen.
          c`~~ | ~~`compile_error` — "too many labels in function (max %d
          )"~~ | ~~No — 2D `char` array; accessed by index only~~ | ~~NO 
          (2D `char` array; dynamic form requires `char **` and separate 
          allocations)~~ | ~~LOW~~ | ✓ DONE (stage 95-11) |
      68  | `MAX_STRING_LITERALS` | 2048 | ~~`include/constants.h`; `incl
          ude/codegen.h` (`CodeGen.string_pool[]`); `src/codegen.c`~~ | ~
          ~`compile_error` — "too many string literals (max %d)". **Note:
          ** raised from 256 → 2048 in stage 92 because the compiler itse
          lf uses ~750 string-literal occurrences.~~ | ~~`CodeGen.string_
          pool[]` stores `ASTNode *` pointers from the AST; no pointers i
          nto the pool array itself escape~~ | ~~YES~~ | ~~MEDIUM~~ | ✓ D
          ONE (stage 95-05) |
      69  | `MAX_LOCAL_STATICS` | 128 | ~~`include/constants.h`; `include
          /codegen.h` (`CodeGen.local_statics[]`); `src/codegen.c`~~ | ~~
          `compile_error` — "too many local static variables (max %d)"~~ 
          | ~~No — accessed by index only; no escaping pointers~~ | ~~YES
          ~~ | ~~LOW~~ | ✓ DONE (stage 95-04) |

● Now the scratch-buffer intro (the ops[] exception) and the ops row:

● Update(docs/fixed-capacity-inventory.md)
  ⎿  Added 5 lines, removed 4 lines
      86  Stage 95-01 table inventory — they are fixed-size local `char` 
          buffers used for
      87  one-shot formatting (label generation, number→string, path/mark
          er assembly,
      88  diagnostics), not growable collections, so none is a `Vec`/`Str
          Buf` conversion
      89 -candidate. With one exception every site is bounded by `snprint
         -f`, an explicit                                                
      90 -index guard, or a `memcpy` length clamp — so it cannot overflow
         -, though several                                               
      91 -silently **truncate** on pathological input. The exception (`op
         -s[]`) has no                                                   
      92 -bound and is a real overflow._                                 
      89 +candidate. Every site is bounded by `snprintf`, an explicit ind
         +ex guard, or a                                                 
      90 +`memcpy` length clamp — so it cannot overflow, though several s
         +ilently                                                        
      91 +**truncate** on pathological input. (The former `ops[]` excepti
         +on — the one                                                   
      92 +real overflow — was converted to a dynamic `StrBuf` in stage 95
         +-12; no                                                        
      93 +unbounded fixed-capacity write remains.)_                      
      94  
      95  | Buffer | Size | Location | Formats | Bound | Note |
      96  |--------|------|----------|---------|-------|------|

● Update(docs/fixed-capacity-inventory.md)
  ⎿  Added 1 line, removed 1 line
       94  
       95  | Buffer | Size | Location | Formats | Bound | Note |
       96  |--------|------|----------|---------|-------|------|
       97 -| `ops` | 32 | `src/preprocessor.c` `eval_cond_unary` | chaine
          -d `#if` unary ops `! - + ~` | **NONE** — `ops[nops++]` with no
          - `nops` check | **⚠ Stack buffer overflow.** A `#if` with >32 
          -leading unary operators overruns the buffer; ~200k operators S
          -IGSEGVs the compiler. Only unchecked fixed-capacity write left
          - in the tree. **PENDING — needs a bounds check (or fold operat
          -ors without storing them).** |                                
       97 +| ~~`ops`~~ | ~~32~~ | `src/preprocessor.c` `eval_cond_unary` 
          +| chained `#if` unary ops `! - + ~` | now a dynamic `StrBuf` (
          +no bound) | ✓ DONE (stage 95-12) — replaced the fixed `char op
          +s[32]` with a `StrBuf`; operators are appended as consumed and
          + applied right-to-left, freed before return. The former unchec
          +ked overflow is gone. |                                       
       98  | `label` | 256 | `src/codegen.c` (block-static emit) | `Lstat
           ic_<func>_<n>` | `snprintf` | Truncates a function name > ~240
            bytes (names are unbounded since 95-08); a truncated label co
           uld collide. Low risk. |
       99  | `tmp` | 64 | `src/codegen.c` (global ptr init) | `Lstr<idx>`
            | `snprintf` | Safe — `idx` is an `int`. |
      100  | `num_buf` | 64 | `src/lexer.c` | integer-literal digits | in
           dex guard `i < 62` | Truncates an integer literal > 62 chars. 
           |

● Now the Open-issue callout and the deferred-LOW row:

● Update(docs/fixed-capacity-inventory.md)
  ⎿  Added 9 lines, removed 6 lines
      179  
      180  ## Summary by Priority
      181  
      182 -> **Open issue (found 2026-06-08):** the `ops[32]` buffer in  
      183 -> `eval_cond_unary` (`src/preprocessor.c`) is an **unchecked**
          - fixed-capacity                                               
      184 -> write — a `#if` with >32 chained unary operators (`!`/`-`/`+
          -`/`~`) overflows                                              
      185 -> the stack and crashes the compiler. It is the last unbounded
          - fixed-capacity                                               
      186 -> write in the tree; see the *Fixed-Size Scratch / Formatting 
          -Buffers* section.                                             
      187 -> All other constant-backed table items below are resolved.   
      182 +> **Resolved (stage 95-12):** the `ops[32]` buffer in `eval_co
          +nd_unary`                                                     
      183 +> (`src/preprocessor.c`) — formerly the last unbounded fixed-c
          +apacity write in                                              
      184 +> the tree — was replaced with a dynamic `StrBuf`, and `MAX_SW
          +ITCH_LABELS`                                                  
      185 +> (the last arbitrary per-switch cap) became a dynamic `Vec` i
          +nside `SwitchCtx`.                                            
      186 +> **There are now no unchecked fixed-capacity writes anywhere 
          +and no hard cap on                                            
      187 +> case/default labels per switch.** The only remaining fixed l
          +imits are the                                                 
      188 +> intentionally-static set (`FUNC_MAX_PARAMS`, `FUNC_TYPE_MAX_
          +PARAMS`,                                                      
      189 +> `MAX_ARRAY_DIMS`, `MAX_INCLUDE_DEPTH`, `MAX_COND_DEPTH`, `MA
          +X_CALL_LAYOUT_ITEMS`),                                        
      190 +> each of which overflows cleanly with a diagnostic.          
      191  
      192  ### HIGH — fix before compiling larger programs
      193  

● Update(docs/fixed-capacity-inventory.md)
  ⎿  Added 1 line, removed 1 line
      223  |------|-------------|
      224  | `FUNC_MAX_PARAMS` | Embedded `TypeKind param_types[16]` in `
           FuncSig`; converting requires changing to a heap-allocated `Ty
           peKind *` and updating all callers. NO Safe Realloc. |
      225  | `FUNC_TYPE_MAX_PARAMS` | Embedded `Type *params[16]` in `Typ
           e`; converting requires changing to a heap-allocated `Type **`
            and updating all type construction. NO Safe Realloc. |
      226 -| `MAX_SWITCH_LABELS` | Embedded `ASTNode *nodes[256]` and `in
          -t labels[256]` in `SwitchCtx`; converting requires heap-alloca
          -ting arrays inside SwitchCtx and updating collect_switch_label
          -s. NO Safe Realloc. |                                         
      226 +| ~~`MAX_SWITCH_LABELS`~~ | ~~Embedded `ASTNode *nodes[256]` a
          +nd `int labels[256]` in `SwitchCtx`; converting requires heap-
          +allocating arrays inside SwitchCtx and updating collect_switch
          +_labels. NO Safe Realloc.~~ ✓ DONE stage 95-12 — replaced with
          + an embedded `Vec entries` of `SwitchLabel{node,label}`, init'
          +d before push / freed before pop. |                           
      227  | ~~`MAX_USER_LABELS`~~ | ~~2D `char user_labels[64][256]` in 
           CodeGen; converting requires `char **` with per-label allocati
           ons. NO Safe Realloc.~~ ✓ DONE stage 95-11 |
      228  | ~~`MAX_NAME_LEN`~~ | ~~Remaining application: `StructField.n
           ame` in `type.h`.~~ ✓ DONE — `StructField.name` converted to `
           const char *` (commit `ebf178b`); all earlier fields migrated 
           in 95-08…95-11; the dead macro was removed from `include/const
           ants.h` in commit `9fda93c`. |
      229  | `MAX_ARRAY_DIMS` | Local `#define` and stack variable in par
           ser.c; only affects nested array dimensions. N/A (stack). |
● Let me check the top status block and the footer line for any remaining "last
  unbounded" claims.

  Read 1 file (ctrl+o to expand)

● Top is generic. Now the footer line:

● Update(docs/fixed-capacity-inventory.md)
  ⎿  Added 1 line, removed 1 line
      230  | `MAX_INCLUDE_DEPTH` | Recursion depth counter in preprocesso
           r.c; not an array. N/A. |
      231  | `MAX_COND_DEPTH` | Local stack variable `CondFrame cond_stac
           k[64]` in preprocessor.c. N/A. |
      232  
      233 -Previously completed LOW items: ~~`PARSER_MAX_ENUM_TAGS`~~ ✓ D
          -ONE stage 95-04, ~~`PARSER_MAX_UNION_TAGS`~~ ✓ DONE stage 95-0
          -4, ~~`MAX_LOCAL_STATICS`~~ ✓ DONE stage 95-04, ~~`MAX_SWITCH_D
          -EPTH`~~ ✓ DONE stage 95-07, ~~`MAX_CALL_LAYOUT_ITEMS`~~ ✓ DONE
          - stage 95-07 (bounds check added), ~~`MAX_USER_LABELS`~~ ✓ DON
          -E stage 95-11 (Vec of const char*), codegen `MAX_NAME_LEN` fie
          -lds ✓ DONE stage 95-11.                                       
      233 +Previously completed LOW items: ~~`PARSER_MAX_ENUM_TAGS`~~ ✓ D
          +ONE stage 95-04, ~~`PARSER_MAX_UNION_TAGS`~~ ✓ DONE stage 95-0
          +4, ~~`MAX_LOCAL_STATICS`~~ ✓ DONE stage 95-04, ~~`MAX_SWITCH_D
          +EPTH`~~ ✓ DONE stage 95-07, ~~`MAX_CALL_LAYOUT_ITEMS`~~ ✓ DONE
          + stage 95-07 (bounds check added), ~~`MAX_USER_LABELS`~~ ✓ DON
          +E stage 95-11 (Vec of const char*), codegen `MAX_NAME_LEN` fie
          +lds ✓ DONE stage 95-11, ~~`MAX_SWITCH_LABELS`~~ ✓ DONE stage 9
          +5-12 (Vec of `SwitchLabel` in `SwitchCtx`), ~~`ops[32]`~~ ✓ DO
          +NE stage 95-12 (StrBuf in `eval_cond_unary`).                 

● Inventory updated. Now delegate README, milestone, transcript, and grammar to
  the haiku artifact writer. Grammar has no change (no syntax change). Let me
  commit the inventory first, then delegate.

● Bash(cd /home/scott/code/claude/claudec99
      git add docs/fixed-capacity-inventory.md…)
  ⎿  done

● haiku-stage-artifact-writer(Write 95-12 milestone, transcript, README)
  ⎿  Done (10 tool uses · 31.1k tokens · 51s)
  (ctrl+o to expand)

● The agent's final message was truncated. Let me verify what it actually wrote.

● Bash(cd /home/scott/code/claude/claudec99
      ls -la docs/milestones/stage-95-12-milestone.md
      docs/sessions/stage-95-12-transcript.md 2>&1…)
  ⎿  -rw-rw-r-- 1 scott scott 1812 Jun  8 20:00 docs/milestones/stage-95-12
     -milestone.md
     -rw-rw-r-- 1 scott scott 6039 Jun  8 20:00 docs/sessions/stage-95-12-t
     … +28 lines (ctrl+o to expand)

● The Code-generator subsection was removed and the stage entry added. But line
  601 (aggregate totals) still says 95-11/1479. Let me check the limits section
  and fix the totals line.

● Bash(cd /home/scott/code/claude/claudec99; grep -n "### Code generator\|###
      Preprocessor\|### Type system\|### Parser\|### AST" README.md)
  ⎿  95:### AST
     101:### Type system
     107:### Parser
     115:### Preprocessor

● The ### Code generator subsection is gone — good. Now fix the aggregate totals
  line:

● Update(README.md)
  ⎿  Error editing file

  Read 1 file (ctrl+o to expand)

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      598  ```
      599  
      600  The runner aggregates per-suite results and prints a final
      601 -`Aggregate: P passed, F failed, T total` line. As of stage 95-
          -11 all tests pass (165 unit, 831 valid, 237 invalid, 82 integr
          -ation, 43 print-AST, 100 print-tokens, 21 print-asm; 1479 tota
          -l).                                                           
      601 +`Aggregate: P passed, F failed, T total` line. As of stage 95-
          +12 all tests pass (165 unit, 833 valid, 237 invalid, 82 integr
          +ation, 43 print-AST, 100 print-tokens, 21 print-asm; 1481 tota
          +l).                                                           
      602  
      603  Individual suites can be run directly, e.g. `./test/valid/run_
           tests.sh`.
      604  

● Let me check the limits intro prose that may now be stale (it claimed
  remaining limits are conversion candidates):

  Read 1 file (ctrl+o to expand)

● Update(README.md)
  ⎿  Added 5 lines, removed 2 lines
      89  cleanly (a diagnostic, never silent corruption). These permanen
          t limits are
      90  `FUNC_MAX_PARAMS` (16), `FUNC_TYPE_MAX_PARAMS` (16), `MAX_ARRAY
          _DIMS` (8),
      91  `MAX_INCLUDE_DEPTH` (64), and `MAX_COND_DEPTH` (64); they are f
          lagged
      92 -_(permanent)_ in the tables below. The remaining tabled limits 
         -are still                                                      
      93 -candidates for conversion.                                     
      92 +_(permanent)_ in the tables below. As of stage 95-12 the dynami
         +c-capacity                                                     
      93 +work is complete: no candidates remain for conversion, there ar
         +e no unchecked                                                 
      94 +fixed-capacity writes anywhere, and the only other tabled entri
         +es are                                                         
      95 +`AST_MAX_CHILDREN` (a doubling initial capacity, not a cap) and
      96 +`MAX_CALL_LAYOUT_ITEMS` (a guarded local-stack bound).         
      97  
      98  ### AST
      99  

● Now let me verify the milestone and transcript look right, then run
  update-supplemental-docs.

  Read 1 file (ctrl+o to expand)

● Milestone looks accurate. Now run the update-supplemental-docs skill.

● Skill(update-supplemental-docs)
  ⎿  Successfully loaded skill

● Let me determine the stage range and gather current state.

● Bash(cd /home/scott/code/claude/claudec99
      echo "=== latest snapshots ==="…)
  ⎿  === latest snapshots ===
     status/project-status-through-stage-93.md
     status/project-status-through-stage-95-11.md
     … +13 lines (ctrl+o to expand)

● Range is just stage 95-12. Let me gather metrics and read the templates.
● Bash(cd /home/scott/code/claude/claudec99
      echo "src=$(ls src/*.c | wc -l) hdr=$(ls include/*.h | wc -l) loc=$(cat
      src/*.c include/*.h | wc -l)"…)
  ⎿  src=12 hdr=13 loc=13728
     commits=782 head=3c41047
     valid=833 invalid=237 integ=83
     … +2 lines (ctrl+o to expand)

● Note: integration raw count 83 includes a non-test dir; milestone says 82. Let
  me read the checklist tail and the 95-11 templates.

● Bash(cd /home/scott/code/claude/claudec99
      echo "=== checklist around 95-11 section and --- marker ==="…)
  ⎿  === checklist around 95-11 section and --- marker ===
     1278:## Stage 95-10 - Remove static char arrays from parser.h
     1283:## Stage 95-11 - Remove static char arrays from codegen.h
     … +2 lines (ctrl+o to expand)

  Read 1 file (ctrl+o to expand)

● Update(docs/outlines/checklist.md)
  ⎿  Added 7 lines
      1286  - [x] `CodeGen.user_labels[MAX_USER_LABELS][MAX_NAME_LEN]` re
            placed with a `Vec` of pointers (64-label-per-function cap el
            iminated); `MAX_USER_LABELS` removed from `include/constants.
            h`
      1287  - [x] At stage close `MAX_NAME_LEN` applied only to `StructFi
            eld.name`; that field was later converted to `const char *` a
            nd the now-dead constant removed in commit `9fda93c`
      1288  
      1289 +## Stage 95-12 - Fix #if unary overflow and dynamic switch la
           +bels                                                         
      1290 +                                                             
      1291 +- [x] `eval_cond_unary` (`src/preprocessor.c`): replaced the 
           +unchecked fixed `char ops[32]` with a dynamic `StrBuf`; leadi
           +ng `#if`/`#elif` unary operators (`! - + ~`) are appended as 
           +consumed and applied right-to-left, freed before return — rem
           +oves the last unbounded fixed-capacity write in the tree (a >
           +32-operator chain previously SIGSEGV'd)                      
      1292 +- [x] `SwitchCtx` (`include/codegen.h`): parallel fixed array
           +s `nodes[MAX_SWITCH_LABELS]`/`labels[MAX_SWITCH_LABELS]` + `c
           +ount` replaced with an embedded `Vec entries` of a new `Switc
           +hLabel{node,label}` pair struct; `count` becomes `entries.len
           +`; the per-switch case/default cap (256) is removed and `MAX_
           +SWITCH_LABELS` deleted from `include/constants.h`            
      1293 +  - [x] Lifecycle: `entries` is `vec_init`'d before the `Swit
           +chCtx` is `vec_push`'d (move semantics) and `vec_free`'d befo
           +re `vec_pop`; the live top element is re-fetched via `vec_get
           +` after the body to avoid a dangling pointer when nested swit
           +ches reallocate `switch_stack`                               
      1294 +- [x] No grammar, parser, or AST changes; switch and `#if` sy
           +ntax/semantics unchanged                                     
      1295 +                                                             
      1296  ---
      1297  
      1298  ## TODO

● No TODO items are satisfied by this internal-only stage. Now the parse-tree
  doc. Let me read the 95-11 parse-tree's header and the MAX_SWITCH_LABELS note.

● Bash(cd /home/scott/code/claude/claudec99
      head -20 docs/other/stage-95-11-parse-tree.md…)
  ⎿  # Full Grammar Parse Tree — Stage 95-11

     Complete hierarchy of parser functions, grouped into four sections. Ea
     … +32 lines (ctrl+o to expand)

  Read 1 file (ctrl+o to expand)

● I'll copy the 95-11 parse-tree to the new file and apply targeted edits.

● Bash(cd /home/scott/code/claude/claudec99
      cp docs/other/stage-95-11-parse-tree.md
      docs/other/stage-95-12-parse-tree.md…)
  ⎿  ├─► "switch"  →  parse_switch_statement
     │      AST_SWITCH_STATEMENT, children: [expr, body]
     │   ├─► parse_expression  (discriminant)
     … +21 lines (ctrl+o to expand)

● Update(docs/other/stage-95-12-parse-tree.md)
  ⎿  File must be read first

  Read 1 file (ctrl+o to expand)

● Now I have full context. Let me apply the targeted edits for stage 95-12.

● Update(docs/other/stage-95-12-parse-tree.md)
  ⎿  Added 1 line, removed 1 line
      1  
      2 -# Full Grammar Parse Tree — Stage 95-11                         
      2 +# Full Grammar Parse Tree — Stage 95-12                         
      3  
      4  Complete hierarchy of parser functions, grouped into four sectio
         ns. Each level
      5  calls the level(s) below it for sub-constructs. Indentation trac
         ks call depth;

● Update(docs/other/stage-95-12-parse-tree.md)
  ⎿  Added 1 line, removed 1 line
      24  (gcc-built) compiles itself to C1, and C1 compiles itself to C2
          , with all 1479
      25  tests passing at each step (see `docs/self-compilation-report.m
          d`).
      26  
      27 -**Stages 91-01 through 95-11 carry no expression-grammar or AST
         --shape changes.**                                              
      27 +**Stages 91-01 through 95-12 carry no expression-grammar or AST
         +-shape changes.**                                              
      28  Stage 91-01 (struct-by-value) and stage 93/94 (build workflow /
           self-host cycle)
      29  are codegen/ABI/tooling work, and the entire 95-xx series is an
           internal
      30  storage refactor: stages 95-02/95-03 add the reusable `Vec` and
           `StrBuf`

● Update(docs/other/stage-95-12-parse-tree.md)
  ⎿  Added 5 lines, removed 1 line
      35  their hard caps; and stages 95-08 through 95-11 move all token,
           AST, parser,
      36  and codegen name/tag/label text from `char[MAX_NAME_LEN]` buffe
          rs into
      37  `const char *` pointers backed by a lexer-owned string pool — w
          hich removes the
      38 -old 255-byte string-literal cap entirely. None of this changes 
         -which programs                                                 
      38 +old 255-byte string-literal cap entirely. Stage 95-12 closes th
         +e last two                                                     
      39 +fixed-capacity items: the preprocessor `#if` unary-operator buf
         +fer becomes a                                                  
      40 +`StrBuf` (removing the final unbounded fixed-capacity write) an
         +d the per-switch                                               
      41 +case/default label table inside `SwitchCtx` becomes a `Vec` (re
         +moving the                                                     
      42 +`MAX_SWITCH_LABELS` cap). None of this changes which programs  
      43  parse; the parser functions below are unchanged in dispatch and
           only differ in
      44  how their symbol tables and value strings are stored.
      45  

● Update(docs/other/stage-95-12-parse-tree.md)
  ⎿  Added 3 lines, removed 1 line
      267  │        enum constants, and unary/binary `+`/`-` over those. 
           Non-constant or
      268  │        non-integer expressions → "case label expression is n
           ot an integer
      269  │        constant expression". The folded value is stored as a
           n AST_INT_LITERAL.
      270 -│        (MAX_SWITCH_LABELS raised 64 → 256 in stage 92 for to
          -ken_type_name().)                                             
      270 +│        (Codegen collects the labels into a per-switch `Vec` 
          +of                                                            
      271 +│        `SwitchLabel{node,label}`; the case/default count per
          + switch is                                                    
      272 +│        unbounded since stage 95-12, when `MAX_SWITCH_LABELS`
          + was removed.)                                                
      273  │   └─► parse_statement  (the statement following the label)
      274  │
      275  ├─► "default"  →  default label (only when switch_depth > 0)

● Now update the two design-points entries referencing 95-07/95-11 and
  self-hosting through 95-11:

● Update(docs/other/stage-95-12-parse-tree.md)
  ⎿  Added 6 lines
      675    field-parsing scratch buffer. Lookups/registrations use `vec
           _get`/`vec_push`.
      676    Three latent overflow bugs were fixed along the way (struct 
           fields beyond 64
      677    silently dropped; unchecked `break_stack`; unguarded call-ar
           gument layout).
      678 +  Stage 95-12 finishes the series: the per-switch case/default
          + label table in                                               
      679 +  `SwitchCtx` becomes an embedded `Vec` of `SwitchLabel{node,l
          +abel}` (init'd                                                
      680 +  before the `SwitchCtx` is pushed, freed before it is popped,
          + with the live                                                
      681 +  top element re-fetched after the body so nested-switch reall
          +ocations cannot                                               
      682 +  dangle it), and the preprocessor `#if` unary-operator run in
          + `eval_cond_unary`                                            
      683 +  becomes a `StrBuf` — the last unbounded fixed-capacity write
          + in the tree.                                                 
      684  - **Pointer-based name storage (stages 95-08/09/10/11):** iden
           tifier, tag, and
      685    label text is no longer copied into `char[MAX_NAME_LEN]` buf
           fers. `Token`
      686    carries `const char *lexeme`/`value` with `size_t` lengths i
           nto a lexer-owned

● Update(docs/other/stage-95-12-parse-tree.md)
  ⎿  Added 2 lines, removed 2 lines
      693    removes typedefs registered at or below that depth.
      694  - `parse_struct_specifier`/`parse_union_specifier` complete a 
           forward-declaration
      695    placeholder in-place when a body is later provided.
      696 -- **Self-hosting (stages 92–95-11):** the compiler compiles it
          -self. C0                                                      
      696 +- **Self-hosting (stages 92–95-12):** the compiler compiles it
          +self. C0                                                      
      697    (gcc-built) produces C1, and C1 produces C2; all three pass 
           the full
      698 -  1479-test suite, confirming bootstrap stability. The origina
          -l bootstrap                                                   
      698 +  1481-test suite, confirming bootstrap stability. The origina
          +l bootstrap                                                   
      699    (stages 91-01/92) surfaced and fixed thirteen real defects (
           struct-by-value
      700    ABI, the silent AST-truncation bug, six struct-by-value/`siz
           eof` codegen bugs,
      701    duplicate-`extern` emission, literal/switch-label capacity l
           imits, missing

● Parse-tree done. Now the status and features snapshots. Let me read the 95-11
  versions.

● Bash(cd /home/scott/code/claude/claudec99; wc -l
      status/project-status-through-stage-95-11.md
      status/project-features-through-stage-95-11.md)
  ⎿    487 status/project-status-through-stage-95-11.md
         3 status/project-features-through-stage-95-11.md
       490 total

  Read 1 file (ctrl+o to expand)
● I'll update the features snapshot — fold 95-12 into the refactor narrative and
  bump the test count.

● Bash(cd /home/scott/code/claude/claudec99; cp
      status/project-features-through-stage-95-11.md
      status/project-features-through-stage-95-12.md; echo copied)
  ⎿  copied

  Read 1 file (ctrl+o to expand)

● Update(status/project-features-through-stage-95-12.md)
  ⎿  Added 1 line, removed 1 line
      1 -What works today: a full preprocessing pipeline (line splicing, 
        -comment removal, `#include "file.h"` and `#include <file.h>`, ob
        -ject-like and function-like `#define` with parameter substitutio
        -n and recursive expansion, variadic macros with `__VA_ARGS__`, m
        -acro stringification `#param`, token pasting `##`, `#undef`, con
        -ditional compilation `#ifdef`/`#ifndef`/`#if`/`#elif`/`#else`/`#
        -endif` with full recursive-descent expression evaluation includi
        -ng `defined()`, macro expansion, arithmetic, bitwise, shift, log
        -ical, and comparison operators, `#error`, predefined macros `__F
        -ILE__`/`__LINE__`/`__DATE__`/`__TIME__`/`__STDC__`/`__STDC_VERSI
        -ON__`/`__STDC_HOSTED__`/`__CLAUDEC99__`, ABI-reflecting macros `
        -__x86_64__`/`__linux__`/`__unix__`/`__LP64__`/`_LP64`/`__CHAR_BI
        -T__`/the full `__SIZEOF_*__` family, command-line `-D`/`-I`/`--s
        -ysroot` flags); an expanded set of stub system headers in `test/
        -include/` covering `stdio.h` (opaque `FILE`, `EOF`, `fopen`, `fc
        -lose`, `fgetc`, `fgets`, `fprintf`, `snprintf`, `puts`, `printf`
        -, `putchar`, the `vfprintf`/`vprintf`/`vsnprintf` variadic-forwa
        -rding family, the standard streams `stdin`/`stdout`/`stderr`, an
        -d the file-position/read stubs `fseek`/`ftell`/`fread` with `SEE
        -K_*`), `stddef.h`, `stdlib.h` (`malloc`, `calloc`, `free`, `real
        -loc`, `exit`, `strtol`, `strtoul`), `string.h` (`strlen`, `strcm
        -p`, `strcpy`, `memcpy`, `memset`, `memcmp`, `strchr`, `strncat`,
        - `strncmp`, `strncpy`, `strrchr`), `limits.h` (with `LLONG_MIN`/
        -`LLONG_MAX`/`ULLONG_MAX`), `stdint.h` (exact-width/pointer-size/
        -fast/least integer typedefs), `stdbool.h` (`bool`, `true`, `fals
        -e`), `ctype.h` (character classification), `errno.h` (`errno`, `
        -ERANGE`, `EINVAL`, …), `time.h` (`time_t`, time functions), `set
        -jmp.h` (non-local jumps), and `stdarg.h` (`va_list` plus the `va
        -_start`/`va_end`/`va_copy`/`va_arg` macros); statements (if/else
        -, while/do-while/for **including C99 `for`-init declarations** `
        -for (int i = 0; …)`, switch/case/default with **constant-express
        -ion case labels** — enum constants and `+`/`-` folding, break/co
        -ntinue/goto, blocks with shadowing, expression statements); the 
        -full signed integer hierarchy (`char`/`short`/`int`/`long`) plus
        - all four unsigned variants plus `long long` and `unsigned long 
        -long`, the `signed` keyword forms and trailing-`int` forms; `_Bo
        -ol` with value-normalization and integer promotion; unsigned-awa
        -re arithmetic, division, shifts, and comparisons; integer promot
        -ions and Usual Arithmetic Conversions for mixed signed/unsigned 
        -operands including LP64-specific rules; decimal and **`0x`/`0X` 
        -hexadecimal** integer literals with `U`/`L`/`UL`/`LL`/`ULL` suff
        -ixes and overflow-aware typing; void type and `void*` generic po
        -inter; `const` qualifier on base-type scalars, **pointer-level `
        -const` enforcement**, and **`const` in struct/union members and 
        -type-name contexts** (writes through `const*`, const-pointer rea
        -ssignment, assignment to a const member directly/via subscript/t
        -hrough a pointer to a const object, and const-discard conversion
        -s all diagnosed — the last warned or errored under `-Werror`); *
        -*minimal `volatile` handling** (tokenized, parsed everywhere `co
        -nst` is, tracked on types, no codegen effect yet); casts; multi-
        -function programs with forward declarations; **variadic function
        - definitions** with `<stdarg.h>` support — `va_list`, `va_start`
        -, `va_end`, and `va_arg` for GP-class types (`int`/`unsigned`/`l
        -ong`/`unsigned long`/`long long`/`unsigned long long`/pointer) v
        -ia a SysV AMD64 register save area with both register and overfl
        -ow-stack paths; **struct and union parameters and return values 
        -passed by value** per the System V AMD64 ABI (register-class agg
        -regates ≤16 bytes in registers, memory-class aggregates >16 byte
        -s through a hidden pointer; shared call-layout helper for call s
        -ites and prologues; `rep movsb` block copies; whole-struct assig
        -nment and decl-init from struct rvalues, including subscript/dot
        -/arrow/deref targets); **function calls with an arbitrary number
        - of arguments** (the 6-argument limit is gone — full System V AM
        -D64 stack-passing on both caller and callee sides, with 16-byte 
        -alignment); pointers (declarations, `&`/`*` as rvalue and lvalue
        -, assign-through-pointer, pointer parameters and return types, f
        -ull pointer arithmetic including difference yielding `long`, `NU
        -LL`, null-constant `0` accepted as a pointer argument, and logic
        -al `!` on pointer operands); arrays (declarations, **multidimens
        -ional arrays** up to 8 dimensions in declarations/indexing/membe
        -r access/`sizeof`, indexing, multi-level subscript, decay-to-poi
        -nter including **struct/union member-array decay**, file-scope b
        -race/string initializers with size inference, array-to-pointer p
        -arameter decay); string and character literals (tokenization, st
        -atic-data emission, decoded simple/**hex `\xNN`/octal `\NNN`** e
        -scapes, and **adjacent string-literal concatenation**; `sizeof` 
        -of a string literal is its length+1); all bitwise operators, rem
        -ainder, and all eleven compound assignment operators **on any mo
        -difiable lvalue**; prefix/postfix `++`/`--` **on any modifiable 
        -lvalue**; unary `&` (address-of) **on any addressable lvalue** i
        -ncluding `&s.member`, `&p->member`, `&arr[i].member`; `sizeof` (
        -including multidimensional array types and arrow/subscripted mem
        -ber arrays); conditional (ternary) and comma operators; comma-se
        -parated init-declarator lists; file-scope global variable declar
        -ations, `extern` object declarations (NASM `extern` directives),
        - and `global` directives for non-static file-scope definitions; 
        -storage-class specifiers `extern` and `static` at file scope **a
        -nd block-scope `static` local variables** (persist across calls;
        - `.bss`/`.data` with RIP-relative addressing); parenthesized and
        - abstract declarators; function pointer declarations/assignment/
        -indirect calls; `typedef` aliases for scalar/pointer/array/funct
        -ion-pointer and complete struct/union types; `enum` declarations
        - with compile-time constant folding; structs (definition, natura
        -l-alignment layout, `.`/`->` access with **general postfix chain
        -ing** like `p->tokens[i].kind`, brace initializers including cha
        -r-member string init, whole-struct assignment/copy, pointer-to-s
        -truct mutation, nested structs, arrays of structs, typedef alias
        -es, incomplete forward declarations); **named unions** (layout =
        - max member size, member access, nested types, whole-union assig
        -nment, globals); and **anonymous struct/union type declarations*
        -* (each definition is a unique type identified by pointer). Diag
        -nostics carry `<file>:<line>:<col>:` position prefixes on parser
        - errors, support `--max-errors=N` multi-error collection with se
        -tjmp/longjmp recovery, expose a two-line `--version` build ident
        -ifier with a `BuiltBy:` attribution field, and honor `-Werror`. 
        -A **multi-mode build workflow** (`build.sh`: `--mode=normal`/`--
        -mode=bootstrap`/`--mode=fallback`/`--mode=self-host`, with boots
        -trap timeout guards) drives builds; the compiler's own source is
        - strict ISO C99, and as of stage 92 the compiler is **fully self
        --hosting** — C0 (gcc-built) compiles itself to C1 and C1 compile
        -s itself to C2, with all 1479 tests passing at each step (see `d
        -ocs/self-compilation-report.md`). Stages 94 and 95-01 through 95
        --11 added no language features; they harden the self-host cycle 
        -(stage 94's `--mode=self-host`) and complete an internal **dynam
        -ic-storage refactor** — new `Vec` and `StrBuf` containers replac
        -e every fixed-capacity parser/codegen table (locals, globals, st
        -ring pool, enum constants/tags, struct/union tags, typedefs, fun
        -ctions, break/switch stacks, user labels, struct-field scratch),
        - and all token, AST, parser, and codegen name/tag/label text mov
        -es from `char[MAX_NAME_LEN]` buffers into `const char *` pointer
        -s backed by a lexer-owned string pool, removing the old per-tabl
        -e caps and the 255-byte string-literal limit (and fixing three l
        -atent silent-overflow bugs along the way).                      
      1 +What works today: a full preprocessing pipeline (line splicing, 
        +comment removal, `#include "file.h"` and `#include <file.h>`, ob
        +ject-like and function-like `#define` with parameter substitutio
        +n and recursive expansion, variadic macros with `__VA_ARGS__`, m
        +acro stringification `#param`, token pasting `##`, `#undef`, con
        +ditional compilation `#ifdef`/`#ifndef`/`#if`/`#elif`/`#else`/`#
        +endif` with full recursive-descent expression evaluation includi
        +ng `defined()`, macro expansion, arithmetic, bitwise, shift, log
        +ical, and comparison operators, `#error`, predefined macros `__F
        +ILE__`/`__LINE__`/`__DATE__`/`__TIME__`/`__STDC__`/`__STDC_VERSI
        +ON__`/`__STDC_HOSTED__`/`__CLAUDEC99__`, ABI-reflecting macros `
        +__x86_64__`/`__linux__`/`__unix__`/`__LP64__`/`_LP64`/`__CHAR_BI
        +T__`/the full `__SIZEOF_*__` family, command-line `-D`/`-I`/`--s
        +ysroot` flags); an expanded set of stub system headers in `test/
        +include/` covering `stdio.h` (opaque `FILE`, `EOF`, `fopen`, `fc
        +lose`, `fgetc`, `fgets`, `fprintf`, `snprintf`, `puts`, `printf`
        +, `putchar`, the `vfprintf`/`vprintf`/`vsnprintf` variadic-forwa
        +rding family, the standard streams `stdin`/`stdout`/`stderr`, an
        +d the file-position/read stubs `fseek`/`ftell`/`fread` with `SEE
        +K_*`), `stddef.h`, `stdlib.h` (`malloc`, `calloc`, `free`, `real
        +loc`, `exit`, `strtol`, `strtoul`), `string.h` (`strlen`, `strcm
        +p`, `strcpy`, `memcpy`, `memset`, `memcmp`, `strchr`, `strncat`,
        + `strncmp`, `strncpy`, `strrchr`), `limits.h` (with `LLONG_MIN`/
        +`LLONG_MAX`/`ULLONG_MAX`), `stdint.h` (exact-width/pointer-size/
        +fast/least integer typedefs), `stdbool.h` (`bool`, `true`, `fals
        +e`), `ctype.h` (character classification), `errno.h` (`errno`, `
        +ERANGE`, `EINVAL`, …), `time.h` (`time_t`, time functions), `set
        +jmp.h` (non-local jumps), and `stdarg.h` (`va_list` plus the `va
        +_start`/`va_end`/`va_copy`/`va_arg` macros); statements (if/else
        +, while/do-while/for **including C99 `for`-init declarations** `
        +for (int i = 0; …)`, switch/case/default with **constant-express
        +ion case labels** — enum constants and `+`/`-` folding, break/co
        +ntinue/goto, blocks with shadowing, expression statements); the 
        +full signed integer hierarchy (`char`/`short`/`int`/`long`) plus
        + all four unsigned variants plus `long long` and `unsigned long 
        +long`, the `signed` keyword forms and trailing-`int` forms; `_Bo
        +ol` with value-normalization and integer promotion; unsigned-awa
        +re arithmetic, division, shifts, and comparisons; integer promot
        +ions and Usual Arithmetic Conversions for mixed signed/unsigned 
        +operands including LP64-specific rules; decimal and **`0x`/`0X` 
        +hexadecimal** integer literals with `U`/`L`/`UL`/`LL`/`ULL` suff
        +ixes and overflow-aware typing; void type and `void*` generic po
        +inter; `const` qualifier on base-type scalars, **pointer-level `
        +const` enforcement**, and **`const` in struct/union members and 
        +type-name contexts** (writes through `const*`, const-pointer rea
        +ssignment, assignment to a const member directly/via subscript/t
        +hrough a pointer to a const object, and const-discard conversion
        +s all diagnosed — the last warned or errored under `-Werror`); *
        +*minimal `volatile` handling** (tokenized, parsed everywhere `co
        +nst` is, tracked on types, no codegen effect yet); casts; multi-
        +function programs with forward declarations; **variadic function
        + definitions** with `<stdarg.h>` support — `va_list`, `va_start`
        +, `va_end`, and `va_arg` for GP-class types (`int`/`unsigned`/`l
        +ong`/`unsigned long`/`long long`/`unsigned long long`/pointer) v
        +ia a SysV AMD64 register save area with both register and overfl
        +ow-stack paths; **struct and union parameters and return values 
        +passed by value** per the System V AMD64 ABI (register-class agg
        +regates ≤16 bytes in registers, memory-class aggregates >16 byte
        +s through a hidden pointer; shared call-layout helper for call s
        +ites and prologues; `rep movsb` block copies; whole-struct assig
        +nment and decl-init from struct rvalues, including subscript/dot
        +/arrow/deref targets); **function calls with an arbitrary number
        + of arguments** (the 6-argument limit is gone — full System V AM
        +D64 stack-passing on both caller and callee sides, with 16-byte 
        +alignment); pointers (declarations, `&`/`*` as rvalue and lvalue
        +, assign-through-pointer, pointer parameters and return types, f
        +ull pointer arithmetic including difference yielding `long`, `NU
        +LL`, null-constant `0` accepted as a pointer argument, and logic
        +al `!` on pointer operands); arrays (declarations, **multidimens
        +ional arrays** up to 8 dimensions in declarations/indexing/membe
        +r access/`sizeof`, indexing, multi-level subscript, decay-to-poi
        +nter including **struct/union member-array decay**, file-scope b
        +race/string initializers with size inference, array-to-pointer p
        +arameter decay); string and character literals (tokenization, st
        +atic-data emission, decoded simple/**hex `\xNN`/octal `\NNN`** e
        +scapes, and **adjacent string-literal concatenation**; `sizeof` 
        +of a string literal is its length+1); all bitwise operators, rem
        +ainder, and all eleven compound assignment operators **on any mo
        +difiable lvalue**; prefix/postfix `++`/`--` **on any modifiable 
        +lvalue**; unary `&` (address-of) **on any addressable lvalue** i
        +ncluding `&s.member`, `&p->member`, `&arr[i].member`; `sizeof` (
        +including multidimensional array types and arrow/subscripted mem
        +ber arrays); conditional (ternary) and comma operators; comma-se
        +parated init-declarator lists; file-scope global variable declar
        +ations, `extern` object declarations (NASM `extern` directives),
        + and `global` directives for non-static file-scope definitions; 
        +storage-class specifiers `extern` and `static` at file scope **a
        +nd block-scope `static` local variables** (persist across calls;
        + `.bss`/`.data` with RIP-relative addressing); parenthesized and
        + abstract declarators; function pointer declarations/assignment/
        +indirect calls; `typedef` aliases for scalar/pointer/array/funct
        +ion-pointer and complete struct/union types; `enum` declarations
        + with compile-time constant folding; structs (definition, natura
        +l-alignment layout, `.`/`->` access with **general postfix chain
        +ing** like `p->tokens[i].kind`, brace initializers including cha
        +r-member string init, whole-struct assignment/copy, pointer-to-s
        +truct mutation, nested structs, arrays of structs, typedef alias
        +es, incomplete forward declarations); **named unions** (layout =
        + max member size, member access, nested types, whole-union assig
        +nment, globals); and **anonymous struct/union type declarations*
        +* (each definition is a unique type identified by pointer). Diag
        +nostics carry `<file>:<line>:<col>:` position prefixes on parser
        + errors, support `--max-errors=N` multi-error collection with se
        +tjmp/longjmp recovery, expose a two-line `--version` build ident
        +ifier with a `BuiltBy:` attribution field, and honor `-Werror`. 
        +A **multi-mode build workflow** (`build.sh`: `--mode=normal`/`--
        +mode=bootstrap`/`--mode=fallback`/`--mode=self-host`, with boots
        +trap timeout guards) drives builds; the compiler's own source is
        + strict ISO C99, and as of stage 92 the compiler is **fully self
        +-hosting** — C0 (gcc-built) compiles itself to C1 and C1 compile
        +s itself to C2, with all 1479 tests passing at each step (see `d
        +ocs/self-compilation-report.md`). Stages 94 and 95-01 through 95
        +-12 added no language features; they harden the self-host cycle 
        +(stage 94's `--mode=self-host`) and complete an internal **dynam
        +ic-storage refactor** — new `Vec` and `StrBuf` containers replac
        +e every fixed-capacity parser/codegen table (locals, globals, st
        +ring pool, enum constants/tags, struct/union tags, typedefs, fun
        +ctions, break/switch stacks, user labels, struct-field scratch, 
        +the per-switch case/default label table, and the preprocessor `#
        +if` unary-operator buffer), and all token, AST, parser, and code
        +gen name/tag/label text moves from `char[MAX_NAME_LEN]` buffers 
        +into `const char *` pointers backed by a lexer-owned string pool
        +, removing the old per-table caps and the 255-byte string-litera
        +l limit (and fixing three latent silent-overflow bugs along the 
        +way). As of stage 95-12 the refactor is complete: there are no u
        +nchecked fixed-capacity writes anywhere and no hard cap on case/
        +default labels per switch; the only remaining fixed limits are a
        +n intentionally-static set (`FUNC_MAX_PARAMS`, `FUNC_TYPE_MAX_PA
        +RAMS`, `MAX_ARRAY_DIMS`, `MAX_INCLUDE_DEPTH`, `MAX_COND_DEPTH`, 
        +`MAX_CALL_LAYOUT_ITEMS`) that overflow cleanly with a diagnostic
        +. All 1481 tests pass at C0, C1, and C2.                        
      2  
      3  What's still out of scope: floating-point types (`float`, `doubl
         e`) and all floating-point arithmetic/literals/conversions; `va_
         arg` for floating-point and struct-by-value types, and `va_copy`
          codegen (still a no-op stub); multi-character and wide-characte
         r literals; inline struct/union literals (`(T){ … }`) — struct v
         alues must originate from a variable, a function return, or a co
         py; block-scope `static` arrays; bit-fields, flexible array memb
         ers, compound literals; `volatile` code-generation semantics (cu
         rrently parsed and tracked only) and the `restrict` qualifier; `
         typedef enum`; enum constant expressions beyond integer/characte
         r literals and `+`/`-` folding; designated initializers (`[idx] 
         = …`, `.member = …`); functions returning function pointers; poi
         nter-to-array declarators (`(*p)[10]`); old-style (K&R) function
          definitions; `__attribute__` specifiers; `#pragma` (including `
         #pragma once`); `#elifdef`/`#elifndef`; GNU variadic macro exten
         sions (`__VA_OPT__`, named variadic args, comma deletion); objec
         t-file emission, linking, and multi-file `.o` compilation.

● Update(status/project-features-through-stage-95-12.md)
  ⎿  Added 1 line, removed 1 line
      1 -What works today: a full preprocessing pipeline (line splicing, 
        -comment removal, `#include "file.h"` and `#include <file.h>`, ob
        -ject-like and function-like `#define` with parameter substitutio
        -n and recursive expansion, variadic macros with `__VA_ARGS__`, m
        -acro stringification `#param`, token pasting `##`, `#undef`, con
        -ditional compilation `#ifdef`/`#ifndef`/`#if`/`#elif`/`#else`/`#
        -endif` with full recursive-descent expression evaluation includi
        -ng `defined()`, macro expansion, arithmetic, bitwise, shift, log
        -ical, and comparison operators, `#error`, predefined macros `__F
        -ILE__`/`__LINE__`/`__DATE__`/`__TIME__`/`__STDC__`/`__STDC_VERSI
        -ON__`/`__STDC_HOSTED__`/`__CLAUDEC99__`, ABI-reflecting macros `
        -__x86_64__`/`__linux__`/`__unix__`/`__LP64__`/`_LP64`/`__CHAR_BI
        -T__`/the full `__SIZEOF_*__` family, command-line `-D`/`-I`/`--s
        -ysroot` flags); an expanded set of stub system headers in `test/
        -include/` covering `stdio.h` (opaque `FILE`, `EOF`, `fopen`, `fc
        -lose`, `fgetc`, `fgets`, `fprintf`, `snprintf`, `puts`, `printf`
        -, `putchar`, the `vfprintf`/`vprintf`/`vsnprintf` variadic-forwa
        -rding family, the standard streams `stdin`/`stdout`/`stderr`, an
        -d the file-position/read stubs `fseek`/`ftell`/`fread` with `SEE
        -K_*`), `stddef.h`, `stdlib.h` (`malloc`, `calloc`, `free`, `real
        -loc`, `exit`, `strtol`, `strtoul`), `string.h` (`strlen`, `strcm
        -p`, `strcpy`, `memcpy`, `memset`, `memcmp`, `strchr`, `strncat`,
        - `strncmp`, `strncpy`, `strrchr`), `limits.h` (with `LLONG_MIN`/
        -`LLONG_MAX`/`ULLONG_MAX`), `stdint.h` (exact-width/pointer-size/
        -fast/least integer typedefs), `stdbool.h` (`bool`, `true`, `fals
        -e`), `ctype.h` (character classification), `errno.h` (`errno`, `
        -ERANGE`, `EINVAL`, …), `time.h` (`time_t`, time functions), `set
        -jmp.h` (non-local jumps), and `stdarg.h` (`va_list` plus the `va
        -_start`/`va_end`/`va_copy`/`va_arg` macros); statements (if/else
        -, while/do-while/for **including C99 `for`-init declarations** `
        -for (int i = 0; …)`, switch/case/default with **constant-express
        -ion case labels** — enum constants and `+`/`-` folding, break/co
        -ntinue/goto, blocks with shadowing, expression statements); the 
        -full signed integer hierarchy (`char`/`short`/`int`/`long`) plus
        - all four unsigned variants plus `long long` and `unsigned long 
        -long`, the `signed` keyword forms and trailing-`int` forms; `_Bo
        -ol` with value-normalization and integer promotion; unsigned-awa
        -re arithmetic, division, shifts, and comparisons; integer promot
        -ions and Usual Arithmetic Conversions for mixed signed/unsigned 
        -operands including LP64-specific rules; decimal and **`0x`/`0X` 
        -hexadecimal** integer literals with `U`/`L`/`UL`/`LL`/`ULL` suff
        -ixes and overflow-aware typing; void type and `void*` generic po
        -inter; `const` qualifier on base-type scalars, **pointer-level `
        -const` enforcement**, and **`const` in struct/union members and 
        -type-name contexts** (writes through `const*`, const-pointer rea
        -ssignment, assignment to a const member directly/via subscript/t
        -hrough a pointer to a const object, and const-discard conversion
        -s all diagnosed — the last warned or errored under `-Werror`); *
        -*minimal `volatile` handling** (tokenized, parsed everywhere `co
        -nst` is, tracked on types, no codegen effect yet); casts; multi-
        -function programs with forward declarations; **variadic function
        - definitions** with `<stdarg.h>` support — `va_list`, `va_start`
        -, `va_end`, and `va_arg` for GP-class types (`int`/`unsigned`/`l
        -ong`/`unsigned long`/`long long`/`unsigned long long`/pointer) v
        -ia a SysV AMD64 register save area with both register and overfl
        -ow-stack paths; **struct and union parameters and return values 
        -passed by value** per the System V AMD64 ABI (register-class agg
        -regates ≤16 bytes in registers, memory-class aggregates >16 byte
        -s through a hidden pointer; shared call-layout helper for call s
        -ites and prologues; `rep movsb` block copies; whole-struct assig
        -nment and decl-init from struct rvalues, including subscript/dot
        -/arrow/deref targets); **function calls with an arbitrary number
        - of arguments** (the 6-argument limit is gone — full System V AM
        -D64 stack-passing on both caller and callee sides, with 16-byte 
        -alignment); pointers (declarations, `&`/`*` as rvalue and lvalue
        -, assign-through-pointer, pointer parameters and return types, f
        -ull pointer arithmetic including difference yielding `long`, `NU
        -LL`, null-constant `0` accepted as a pointer argument, and logic
        -al `!` on pointer operands); arrays (declarations, **multidimens
        -ional arrays** up to 8 dimensions in declarations/indexing/membe
        -r access/`sizeof`, indexing, multi-level subscript, decay-to-poi
        -nter including **struct/union member-array decay**, file-scope b
        -race/string initializers with size inference, array-to-pointer p
        -arameter decay); string and character literals (tokenization, st
        -atic-data emission, decoded simple/**hex `\xNN`/octal `\NNN`** e
        -scapes, and **adjacent string-literal concatenation**; `sizeof` 
        -of a string literal is its length+1); all bitwise operators, rem
        -ainder, and all eleven compound assignment operators **on any mo
        -difiable lvalue**; prefix/postfix `++`/`--` **on any modifiable 
        -lvalue**; unary `&` (address-of) **on any addressable lvalue** i
        -ncluding `&s.member`, `&p->member`, `&arr[i].member`; `sizeof` (
        -including multidimensional array types and arrow/subscripted mem
        -ber arrays); conditional (ternary) and comma operators; comma-se
        -parated init-declarator lists; file-scope global variable declar
        -ations, `extern` object declarations (NASM `extern` directives),
        - and `global` directives for non-static file-scope definitions; 
        -storage-class specifiers `extern` and `static` at file scope **a
        -nd block-scope `static` local variables** (persist across calls;
        - `.bss`/`.data` with RIP-relative addressing); parenthesized and
        - abstract declarators; function pointer declarations/assignment/
        -indirect calls; `typedef` aliases for scalar/pointer/array/funct
        -ion-pointer and complete struct/union types; `enum` declarations
        - with compile-time constant folding; structs (definition, natura
        -l-alignment layout, `.`/`->` access with **general postfix chain
        -ing** like `p->tokens[i].kind`, brace initializers including cha
        -r-member string init, whole-struct assignment/copy, pointer-to-s
        -truct mutation, nested structs, arrays of structs, typedef alias
        -es, incomplete forward declarations); **named unions** (layout =
        - max member size, member access, nested types, whole-union assig
        -nment, globals); and **anonymous struct/union type declarations*
        -* (each definition is a unique type identified by pointer). Diag
        -nostics carry `<file>:<line>:<col>:` position prefixes on parser
        - errors, support `--max-errors=N` multi-error collection with se
        -tjmp/longjmp recovery, expose a two-line `--version` build ident
        -ifier with a `BuiltBy:` attribution field, and honor `-Werror`. 
        -A **multi-mode build workflow** (`build.sh`: `--mode=normal`/`--
        -mode=bootstrap`/`--mode=fallback`/`--mode=self-host`, with boots
        -trap timeout guards) drives builds; the compiler's own source is
        - strict ISO C99, and as of stage 92 the compiler is **fully self
        --hosting** — C0 (gcc-built) compiles itself to C1 and C1 compile
        -s itself to C2, with all 1479 tests passing at each step (see `d
        -ocs/self-compilation-report.md`). Stages 94 and 95-01 through 95
        --12 added no language features; they harden the self-host cycle 
        -(stage 94's `--mode=self-host`) and complete an internal **dynam
        -ic-storage refactor** — new `Vec` and `StrBuf` containers replac
        -e every fixed-capacity parser/codegen table (locals, globals, st
        -ring pool, enum constants/tags, struct/union tags, typedefs, fun
        -ctions, break/switch stacks, user labels, struct-field scratch, 
        -the per-switch case/default label table, and the preprocessor `#
        -if` unary-operator buffer), and all token, AST, parser, and code
        -gen name/tag/label text moves from `char[MAX_NAME_LEN]` buffers 
        -into `const char *` pointers backed by a lexer-owned string pool
        -, removing the old per-table caps and the 255-byte string-litera
        -l limit (and fixing three latent silent-overflow bugs along the 
        -way). As of stage 95-12 the refactor is complete: there are no u
        -nchecked fixed-capacity writes anywhere and no hard cap on case/
        -default labels per switch; the only remaining fixed limits are a
        -n intentionally-static set (`FUNC_MAX_PARAMS`, `FUNC_TYPE_MAX_PA
        -RAMS`, `MAX_ARRAY_DIMS`, `MAX_INCLUDE_DEPTH`, `MAX_COND_DEPTH`, 
        -`MAX_CALL_LAYOUT_ITEMS`) that overflow cleanly with a diagnostic
        -. All 1481 tests pass at C0, C1, and C2.                        
      1 +What works today: a full preprocessing pipeline (line splicing, 
        +comment removal, `#include "file.h"` and `#include <file.h>`, ob
        +ject-like and function-like `#define` with parameter substitutio
        +n and recursive expansion, variadic macros with `__VA_ARGS__`, m
        +acro stringification `#param`, token pasting `##`, `#undef`, con
        +ditional compilation `#ifdef`/`#ifndef`/`#if`/`#elif`/`#else`/`#
        +endif` with full recursive-descent expression evaluation includi
        +ng `defined()`, macro expansion, arithmetic, bitwise, shift, log
        +ical, and comparison operators, `#error`, predefined macros `__F
        +ILE__`/`__LINE__`/`__DATE__`/`__TIME__`/`__STDC__`/`__STDC_VERSI
        +ON__`/`__STDC_HOSTED__`/`__CLAUDEC99__`, ABI-reflecting macros `
        +__x86_64__`/`__linux__`/`__unix__`/`__LP64__`/`_LP64`/`__CHAR_BI
        +T__`/the full `__SIZEOF_*__` family, command-line `-D`/`-I`/`--s
        +ysroot` flags); an expanded set of stub system headers in `test/
        +include/` covering `stdio.h` (opaque `FILE`, `EOF`, `fopen`, `fc
        +lose`, `fgetc`, `fgets`, `fprintf`, `snprintf`, `puts`, `printf`
        +, `putchar`, the `vfprintf`/`vprintf`/`vsnprintf` variadic-forwa
        +rding family, the standard streams `stdin`/`stdout`/`stderr`, an
        +d the file-position/read stubs `fseek`/`ftell`/`fread` with `SEE
        +K_*`), `stddef.h`, `stdlib.h` (`malloc`, `calloc`, `free`, `real
        +loc`, `exit`, `strtol`, `strtoul`), `string.h` (`strlen`, `strcm
        +p`, `strcpy`, `memcpy`, `memset`, `memcmp`, `strchr`, `strncat`,
        + `strncmp`, `strncpy`, `strrchr`), `limits.h` (with `LLONG_MIN`/
        +`LLONG_MAX`/`ULLONG_MAX`), `stdint.h` (exact-width/pointer-size/
        +fast/least integer typedefs), `stdbool.h` (`bool`, `true`, `fals
        +e`), `ctype.h` (character classification), `errno.h` (`errno`, `
        +ERANGE`, `EINVAL`, …), `time.h` (`time_t`, time functions), `set
        +jmp.h` (non-local jumps), and `stdarg.h` (`va_list` plus the `va
        +_start`/`va_end`/`va_copy`/`va_arg` macros); statements (if/else
        +, while/do-while/for **including C99 `for`-init declarations** `
        +for (int i = 0; …)`, switch/case/default with **constant-express
        +ion case labels** — enum constants and `+`/`-` folding, break/co
        +ntinue/goto, blocks with shadowing, expression statements); the 
        +full signed integer hierarchy (`char`/`short`/`int`/`long`) plus
        + all four unsigned variants plus `long long` and `unsigned long 
        +long`, the `signed` keyword forms and trailing-`int` forms; `_Bo
        +ol` with value-normalization and integer promotion; unsigned-awa
        +re arithmetic, division, shifts, and comparisons; integer promot
        +ions and Usual Arithmetic Conversions for mixed signed/unsigned 
        +operands including LP64-specific rules; decimal and **`0x`/`0X` 
        +hexadecimal** integer literals with `U`/`L`/`UL`/`LL`/`ULL` suff
        +ixes and overflow-aware typing; void type and `void*` generic po
        +inter; `const` qualifier on base-type scalars, **pointer-level `
        +const` enforcement**, and **`const` in struct/union members and 
        +type-name contexts** (writes through `const*`, const-pointer rea
        +ssignment, assignment to a const member directly/via subscript/t
        +hrough a pointer to a const object, and const-discard conversion
        +s all diagnosed — the last warned or errored under `-Werror`); *
        +*minimal `volatile` handling** (tokenized, parsed everywhere `co
        +nst` is, tracked on types, no codegen effect yet); casts; multi-
        +function programs with forward declarations; **variadic function
        + definitions** with `<stdarg.h>` support — `va_list`, `va_start`
        +, `va_end`, and `va_arg` for GP-class types (`int`/`unsigned`/`l
        +ong`/`unsigned long`/`long long`/`unsigned long long`/pointer) v
        +ia a SysV AMD64 register save area with both register and overfl
        +ow-stack paths; **struct and union parameters and return values 
        +passed by value** per the System V AMD64 ABI (register-class agg
        +regates ≤16 bytes in registers, memory-class aggregates >16 byte
        +s through a hidden pointer; shared call-layout helper for call s
        +ites and prologues; `rep movsb` block copies; whole-struct assig
        +nment and decl-init from struct rvalues, including subscript/dot
        +/arrow/deref targets); **function calls with an arbitrary number
        + of arguments** (the 6-argument limit is gone — full System V AM
        +D64 stack-passing on both caller and callee sides, with 16-byte 
        +alignment); pointers (declarations, `&`/`*` as rvalue and lvalue
        +, assign-through-pointer, pointer parameters and return types, f
        +ull pointer arithmetic including difference yielding `long`, `NU
        +LL`, null-constant `0` accepted as a pointer argument, and logic
        +al `!` on pointer operands); arrays (declarations, **multidimens
        +ional arrays** up to 8 dimensions in declarations/indexing/membe
        +r access/`sizeof`, indexing, multi-level subscript, decay-to-poi
        +nter including **struct/union member-array decay**, file-scope b
        +race/string initializers with size inference, array-to-pointer p
        +arameter decay); string and character literals (tokenization, st
        +atic-data emission, decoded simple/**hex `\xNN`/octal `\NNN`** e
        +scapes, and **adjacent string-literal concatenation**; `sizeof` 
        +of a string literal is its length+1); all bitwise operators, rem
        +ainder, and all eleven compound assignment operators **on any mo
        +difiable lvalue**; prefix/postfix `++`/`--` **on any modifiable 
        +lvalue**; unary `&` (address-of) **on any addressable lvalue** i
        +ncluding `&s.member`, `&p->member`, `&arr[i].member`; `sizeof` (
        +including multidimensional array types and arrow/subscripted mem
        +ber arrays); conditional (ternary) and comma operators; comma-se
        +parated init-declarator lists; file-scope global variable declar
        +ations, `extern` object declarations (NASM `extern` directives),
        + and `global` directives for non-static file-scope definitions; 
        +storage-class specifiers `extern` and `static` at file scope **a
        +nd block-scope `static` local variables** (persist across calls;
        + `.bss`/`.data` with RIP-relative addressing); parenthesized and
        + abstract declarators; function pointer declarations/assignment/
        +indirect calls; `typedef` aliases for scalar/pointer/array/funct
        +ion-pointer and complete struct/union types; `enum` declarations
        + with compile-time constant folding; structs (definition, natura
        +l-alignment layout, `.`/`->` access with **general postfix chain
        +ing** like `p->tokens[i].kind`, brace initializers including cha
        +r-member string init, whole-struct assignment/copy, pointer-to-s
        +truct mutation, nested structs, arrays of structs, typedef alias
        +es, incomplete forward declarations); **named unions** (layout =
        + max member size, member access, nested types, whole-union assig
        +nment, globals); and **anonymous struct/union type declarations*
        +* (each definition is a unique type identified by pointer). Diag
        +nostics carry `<file>:<line>:<col>:` position prefixes on parser
        + errors, support `--max-errors=N` multi-error collection with se
        +tjmp/longjmp recovery, expose a two-line `--version` build ident
        +ifier with a `BuiltBy:` attribution field, and honor `-Werror`. 
        +A **multi-mode build workflow** (`build.sh`: `--mode=normal`/`--
        +mode=bootstrap`/`--mode=fallback`/`--mode=self-host`, with boots
        +trap timeout guards) drives builds; the compiler's own source is
        + strict ISO C99, and as of stage 92 the compiler is **fully self
        +-hosting** — C0 (gcc-built) compiles itself to C1 and C1 compile
        +s itself to C2, with all 1481 tests passing at each step (see `d
        +ocs/self-compilation-report.md`). Stages 94 and 95-01 through 95
        +-12 added no language features; they harden the self-host cycle 
        +(stage 94's `--mode=self-host`) and complete an internal **dynam
        +ic-storage refactor** — new `Vec` and `StrBuf` containers replac
        +e every fixed-capacity parser/codegen table (locals, globals, st
        +ring pool, enum constants/tags, struct/union tags, typedefs, fun
        +ctions, break/switch stacks, user labels, struct-field scratch, 
        +the per-switch case/default label table, and the preprocessor `#
        +if` unary-operator buffer), and all token, AST, parser, and code
        +gen name/tag/label text moves from `char[MAX_NAME_LEN]` buffers 
        +into `const char *` pointers backed by a lexer-owned string pool
        +, removing the old per-table caps and the 255-byte string-litera
        +l limit (and fixing three latent silent-overflow bugs along the 
        +way). As of stage 95-12 the refactor is complete: there are no u
        +nchecked fixed-capacity writes anywhere and no hard cap on case/
        +default labels per switch; the only remaining fixed limits are a
        +n intentionally-static set (`FUNC_MAX_PARAMS`, `FUNC_TYPE_MAX_PA
        +RAMS`, `MAX_ARRAY_DIMS`, `MAX_INCLUDE_DEPTH`, `MAX_COND_DEPTH`, 
        +`MAX_CALL_LAYOUT_ITEMS`) that overflow cleanly with a diagnostic
        +. All 1481 tests pass at C0, C1, and C2.                        
      2  
      3  What's still out of scope: floating-point types (`float`, `doubl
         e`) and all floating-point arithmetic/literals/conversions; `va_
         arg` for floating-point and struct-by-value types, and `va_copy`
          codegen (still a no-op stub); multi-character and wide-characte
         r literals; inline struct/union literals (`(T){ … }`) — struct v
         alues must originate from a variable, a function return, or a co
         py; block-scope `static` arrays; bit-fields, flexible array memb
         ers, compound literals; `volatile` code-generation semantics (cu
         rrently parsed and tracked only) and the `restrict` qualifier; `
         typedef enum`; enum constant expressions beyond integer/characte
         r literals and `+`/`-` folding; designated initializers (`[idx] 
         = …`, `.member = …`); functions returning function pointers; poi
         nter-to-array declarators (`(*p)[10]`); old-style (K&R) function
          definitions; `__attribute__` specifiers; `#pragma` (including `
         #pragma once`); `#elifdef`/`#elifndef`; GNU variadic macro exten
         sions (`__VA_OPT__`, named variadic args, comma deletion); objec
         t-file emission, linking, and multi-file `.o` compilation.

● Now the status snapshot. Let me read its structure to make targeted edits.

● Bash(cd /home/scott/code/claude/claudec99
      grep -n "^# \|^##
      \|95-11\|1479\|1481\|through-stage\|HEAD\|Commits\|commits\|Source
      files\|LOC\|stage 95\|(current)\|Date"…)
  ⎿  1:# ClaudeC99 Project Status — Through Stage 95-11
     5:## Overview
     22:cycle. Stages 95-02 through 95-11 are an internal **dynamic-storage
     … +20 lines (ctrl+o to expand)

  Read 1 file (ctrl+o to expand)

● I'll create the status snapshot with targeted edits. Let me copy and edit the
  header, overview, metrics, timeline, recently-shipped, out-of-scope, and
  parse-tree reference.

● Bash(cd /home/scott/code/claude/claudec99; cp
      status/project-status-through-stage-95-11.md
      status/project-status-through-stage-95-12.md; echo copied)
  ⎿  copied

  Read 1 file (ctrl+o to expand)

● Update(status/project-status-through-stage-95-12.md)
  ⎿  Added 2 lines, removed 2 lines
      1 -# ClaudeC99 Project Status — Through Stage 95-11                
      1 +# ClaudeC99 Project Status — Through Stage 95-12                
      2  
      3 -_As of 2026-06-08 (commit `be5cbca`)_                           
      3 +_As of 2026-06-08 (commit `3c41047`)_                           
      4  
      5  ## Overview
      6  

● Update(status/project-status-through-stage-95-12.md)
  ⎿  Added 7 lines, removed 3 lines
      19  entire test suite, confirming bootstrap stability (see
      20  `docs/self-compilation-report.md`). Stage 93 adds a multi-mode 
          build
      21  workflow (`build.sh`); stage 94 adds a repeatable `--mode=self-
          host` C0→C1→C2
      22 -cycle. Stages 95-02 through 95-11 are an internal **dynamic-sto
         -rage refactor**:                                               
      22 +cycle. Stages 95-02 through 95-12 are an internal **dynamic-sto
         +rage refactor**:                                               
      23  new `Vec` and `StrBuf` containers replace the parser/codegen fi
          xed-capacity
      24  tables, and all token/AST/parser/codegen name and label text mo
          ves from
      25  `char[MAX_NAME_LEN]` buffers into `const char *` pointers backe
          d by a
      26 -lexer-owned string pool. No language features changed in this r
         -ange.                                                          
      26 +lexer-owned string pool. Stage 95-12 closes the refactor — the 
         +preprocessor                                                   
      27 +`#if` unary-operator buffer becomes a `StrBuf` and the per-swit
         +ch case/default                                                
      28 +label table becomes a `Vec` — so there are now no unchecked fix
         +ed-capacity                                                    
      29 +writes anywhere and no hard cap on case/default labels per swit
         +ch. No language                                                
      30 +features changed in this range.                                
      31  
      28 -**Stages completed: 201** (stage-01 through stage-95-11, includ
         -ing substages).                                                
      32 +**Stages completed: 202** (stage-01 through stage-95-12, includ
         +ing substages).                                                
      33  
      34  ## Build & Test
      35  

● Update(status/project-status-through-stage-95-12.md)
  ⎿  Added 5 lines, removed 5 lines
      37  |-------------------------------|----------------|
      38  | Source files (`src/*.c`)      | 12             |
      39  | Header files (`include/`)     | 13             |
      40 -| Total LOC (src + include)     | 13,715         |             
      40 +| Total LOC (src + include)     | 13,728         |             
      41  | Unit-test assertions          | 165            |
      42 -| Valid runtime tests           | 831            |             
      42 +| Valid runtime tests           | 833            |             
      43  | Invalid (compile-error) tests | 237            |
      44  | Integration tests             | 82             |
      45  | Print-AST golden tests        | 43             |
      46  | Print-tokens golden tests     | 100            |
      47  | Print-asm golden tests        | 21             |
      48 -| **Total tests**               | **1,479**      |             
      49 -| Git commits                   | 774            |             
      48 +| **Total tests**               | **1,481**      |             
      49 +| Git commits                   | 782            |             
      50  
      51 -All 1,479 tests pass with no regressions — under the gcc-built 
         -compiler and                                                   
      51 +All 1,481 tests pass with no regressions — under the gcc-built 
         +compiler and                                                   
      52  under the self-compiled C1/C2 bootstrap binaries alike.
      53  
      54  ## Language Features Implemented

● Now the timeline, recently-shipped, out-of-scope, and parse-tree reference.
  Let me read those sections.

  Read 1 file (ctrl+o to expand)

● Update(status/project-status-through-stage-95-12.md)
  ⎿  Added 1 line, removed 1 line
      332    portable `CC_NORETURN` / `CC_PRINTF` macros, and CMake locks
            the build
      333    to `CMAKE_C_STANDARD 99` with `-Wall -Wextra -pedantic`.
      334  
      335 -## Stage-by-Stage Timeline (76–95-11)                         
      335 +## Stage-by-Stage Timeline (76–95-12)                         
      336  
      337  Stages 01–65 are catalogued in `project-status-through-stage-6
           5.md`, and
      338  66–75 in `project-status-through-stage-75-06.md`; new stages s
           ince:

● Update(status/project-status-through-stage-95-12.md)
  ⎿  Added 3 lines, removed 2 lines
      375  | 95-08      | Token text → pointer + length (255-byte string 
           cap removed)   |
      376  | 95-09      | `ASTNode.value` → `const char *`               
                          |
      377  | 95-10      | parser.h name/tag fields → `const char *`      
                          |
      378 -| **95-11**  | **codegen.h name/label fields → `const char *` 
          -(current)**    |                                              
      378 +| 95-11      | codegen.h name/label fields → `const char *`   
          +               |                                              
      379 +| **95-12**  | **`#if` unary buffer → `StrBuf`; switch labels 
          +→ `Vec` (current)** |                                         
      380  
      380 -## Recently Shipped (Stages 94–95-11)                         
      381 +## Recently Shipped (Stages 94–95-12)                         
      382  
      383  This range adds **no language features** — it hardens the boot
           strap workflow
      384  and replaces every remaining fixed-capacity table and fixed-si
           ze text buffer

  Read 1 file (ctrl+o to expand)

● Update(status/project-status-through-stage-95-12.md)
  ⎿  Added 17 lines
      428  `StructField.name` until that field too became a pointer; the 
           now-dead constant
      429  was removed in commit `9fda93c`.)
      430  
      431 +**Stage 95-12 — the last two fixed-capacity items.** The prepr
          +ocessor's                                                     
      432 +`eval_cond_unary` replaced its unchecked `char ops[32]` (a `#i
          +f` with >32                                                   
      433 +chained unary operators overran the stack and crashed) with a 
          +dynamic `StrBuf`;                                             
      434 +operators are appended as consumed and applied right-to-left. 
          +`SwitchCtx`                                                   
      435 +replaced its parallel fixed arrays `nodes[256]`/`labels[256]` 
          +with an embedded                                              
      436 +`Vec` of a new `SwitchLabel{node,label}` pair struct, removing
          + the                                                          
      437 +`MAX_SWITCH_LABELS` per-switch cap (constant deleted). The `Ve
          +c` is init'd                                                  
      438 +before the `SwitchCtx` is pushed onto `switch_stack` and freed
          + before it is                                                 
      439 +popped, re-fetching the live top element after the body so nes
          +ted switches that                                             
      440 +reallocate the stack cannot dangle it (a bug caught by the exi
          +sting                                                         
      441 +nested-20-deep switch test). With these two conversions there 
          +are no unchecked                                              
      442 +fixed-capacity writes anywhere and no hard cap on case/default
          + labels; the only                                             
      443 +remaining fixed limits are an intentionally-static set (`FUNC_
          +MAX_PARAMS`,                                                  
      444 +`FUNC_TYPE_MAX_PARAMS`, `MAX_ARRAY_DIMS`, `MAX_INCLUDE_DEPTH`,
          + `MAX_COND_DEPTH`,                                            
      445 +`MAX_CALL_LAYOUT_ITEMS`) that overflow cleanly with a diagnost
          +ic. Two tests                                                 
      446 +added (100-operator `#if` chain; 300-case switch).            
      447 +                                                              
      448  ## Out of Scope (Not Yet Implemented)
      449  
      450  - Floating-point types (`float`, `double`) and all FP

● Now the parse-tree reference near the end:

  Read 1 file (ctrl+o to expand)

● Update(status/project-status-through-stage-95-12.md)
  ⎿  Added 1 line, removed 1 line
      488  ```
      489  
      490  The grammar is documented in `docs/grammar.md`, the parser cal
           l hierarchy
      491 -in `docs/other/stage-95-11-parse-tree.md`, and the feature che
          -cklist in                                                     
      491 +in `docs/other/stage-95-12-parse-tree.md`, and the feature che
          +cklist in                                                     
      492  `docs/outlines/checklist.md` — each updated alongside any stag
           e that
      493  touches it. The bootstrap workflow is driven by `build.sh`; th
           e
      494  self-compilation findings are recorded in `docs/self-compilati
           on-report.md`.

● Now let me check the Out-of-Scope section — no language features changed, so
  likely nothing to remove, but let me verify it doesn't mention capacity limits
  as a limitation.

● Bash(cd /home/scott/code/claude/claudec99; sed -n '/## Out of Scope/,/##
      Architecture/p' status/project-status-through-stage-95-12.md)
  ⎿  ## Out of Scope (Not Yet Implemented)

     - Floating-point types (`float`, `double`) and all FP
     … +22 lines (ctrl+o to expand)
● Out-of-scope is purely language features — no capacity items, so no change
  needed. Test totals: 165+833+237+82+43+100+21 = 1481 ✓. Supplemental docs
  complete. Let me verify and commit everything.

● Bash(cd /home/scott/code/claude/claudec99; git status --short)
  ⎿   M README.md
      M docs/outlines/checklist.md
     ?? docs/milestones/stage-95-12-milestone.md
     … +4 lines (ctrl+o to expand)

● Let me quickly sanity-check the transcript artifact before committing.

  Read 1 file (ctrl+o to expand)

● The transcript has two small factual errors (the var is ops not ops_buf, and
  strbuf_init takes one argument). Let me correct them.

● Update(docs/sessions/stage-95-12-transcript.md)
  ⎿  Added 5 lines, removed 5 lines
       9  ### Step 1: Preprocessor Unary-Operator Buffer
      10  
      11  - Included `strbuf.h` in src/preprocessor.c.
      12 -- Replaced the local `char ops[32]` array in `eval_cond_unary` 
         -with a dynamic `StrBuf`.                                       
      13 -- Initialization: `strbuf_init(&ops_buf, 32)` at function entry
         -.                                                              
      14 -- Each consumed unary operator (when peeking `!`, `-`, `+`, `~`
         -) is now appended via `strbuf_append_char(&ops_buf, op)`.      
      15 -- Loop index changed from iterating a fixed-size array to check
         -ing `ops_buf.len`, applying operators right-to-left.           
      16 -- Cleanup: `strbuf_free(&ops_buf)` before return (including ear
         -ly returns).                                                   
      12 +- Replaced the local `char ops[32]` array in `eval_cond_unary` 
         +with a dynamic `StrBuf ops`.                                   
      13 +- Initialization: `strbuf_init(&ops)` at function entry (the bu
         +ffer allocates lazily; no initial-capacity argument).          
      14 +- Each consumed unary operator (when peeking `!`, `-`, `+`, `~`
         +) is now appended via `strbuf_append_char(&ops, c)`.           
      15 +- Loop index changed from iterating a fixed-size array to walki
         +ng `ops.data[i]` in reverse over `ops.len`, applying operators 
         +right-to-left.                                                 
      16 +- Cleanup: `strbuf_free(&ops)` before returning the folded valu
         +e.                                                             
      17  - Unary chains of arbitrary length now supported; no overflow.
      18  
      19  ### Step 2: Code Generator SwitchCtx Dynamic Vec

● Now commit all documentation.

● Bash(cd /home/scott/code/claude/claudec99
      git add README.md docs/outlines/checklist.md
      docs/milestones/stage-95-12-milestone.md
      docs/other/stage-95-12-parse-tree.md…)
  ⎿  done
     2a2dbb7 docs: stage 95-12 milestone, transcript, README, supplemental
     snapshots
     … +6 lines (ctrl+o to expand)

● All committed. Let me do a final verification — clean tree and the working
  compiler builds.

● Bash(cd /home/scott/code/claude/claudec99; git status --short; echo "--- tree
      clean ---"; ./build/ccompiler --version | head -1)
  ⎿  --- tree clean ---
     ClaudeC99 version 00.02.00951200.00780

● Stage 95-12 is complete. Working tree is clean; build/ccompiler is the
  self-hosted C2 at stage 00951200.

  Summary

  STAGE_DISPLAY: stage-95-12 — the two remaining non-static fixed-capacity
  issues are closed. After this stage there are no unchecked fixed-capacity 
  writes anywhere and no hard cap on case/default labels per switch.

  Task 1 — #if unary-operator overflow (src/preprocessor.c)
  Replaced the unchecked char ops[32] in eval_cond_unary with a dynamic StrBuf.
  Operators are appended as consumed and applied right-to-left (preserving !-1 ≠
  -!1), freed before return. This removed the last unbounded fixed-capacity
  write — a #if with >32 chained unary ops previously SIGSEGV'd.

  Task 2 — dynamic switch labels (include/codegen.h, src/codegen.c,
  include/constants.h)
  Replaced SwitchCtx's parallel nodes[256]/labels[256] arrays with an embedded
  Vec entries of a new SwitchLabel{node,label} pair struct; removed
  MAX_SWITCH_LABELS. Lifecycle: entries init'd before the push, freed before the
  pop. One bug caught and fixed during testing — the post-body vec_free reused
  a ctx pointer that dangles when nested switches reallocate switch_stack; fixed
  by re-fetching the live top element via vec_get. The existing
  test_switch_nested_20_deep test surfaced it.

  Tests (+2)
  - test_pp_if_long_unary_chain__42.c — 50 -~ pairs (100 operators) fold to
  exactly 50; proves no overflow and pins right-to-left folding.
  - test_switch_over_256_labels__77.c — 300-case switch, selector 277
  returns 77.

  Result: all 1481 tests pass (165 unit, 833 valid, 237 invalid, 82 integration,
  43 print_ast, 100 print_tokens, 21 print_asm) under GCC and at C0/C1/C2 in
  the full self-host cycle — no bootstrap issues.

  Commits (feature work committed one task at a time per the spec cadence): Task
  1 → Task 2 (+ version.c) → self-compilation report → inventory →
  milestone/transcript/README/supplemental snapshots. docs/grammar.md was not
  touched (no grammar change this stage).

  No /schedule follow-up applies — the stage left no future-dated obligation.

✻ Cogitated for 17m 9s