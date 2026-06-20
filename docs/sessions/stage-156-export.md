 ▐▛███▜▌   Claude Code v2.1.183
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99


❯ /implement-stage docs/stages/ClaudeC99-spec-stage-156-optimization-bug.md     

● Let me read the spec file and supporting files in parallel.

  Read 5 files, listed 1 directory (ctrl+o to expand)

● Good - the spec file exists. Now let me look at the optimizer code to
  understand the bug.

  STAGE_DISPLAY: Stage 156

  Let me read the optimizer and related files.

  Searched for 2 patterns, read 1 file (ctrl+o to expand)

● Let me look at how switch statements are structured in the AST and codegen to
  understand what the optimizer might be doing wrong.

  Searched for 2 patterns, read 1 file (ctrl+o to expand)

● Now I understand the bug. Let me verify by checking the AST structure for a
  switch:

  The problem is in optimize_stmt's AST_BLOCK case. When a break statement is
  encountered, the dead-code removal scans forward and removes siblings until it
  finds an AST_LABEL_STATEMENT. But AST_CASE_SECTION and AST_DEFAULT_SECTION
  are not AST_LABEL_STATEMENT — they're different node types. So the optimizer
  incorrectly removes subsequent case sections as "dead code".

  Let me look at the existing integration tests and the test directory structure
  before starting.

  Listed 3 directories (ctrl+o to expand)

● Let me look at an existing unreachable test for reference, and also check the
  test for a switch:

  Read 1 file, listed 1 directory (ctrl+o to expand)

● Good. I now have a complete picture. Let me produce the kickoff artifact in
  parallel with checking the test structure.

  Read 1 file, listed 1 directory (ctrl+o to expand)

● Now I have everything I need. Let me spawn the kickoff artifact writer while I
  implement the fix.

● haiku-stage-artifact-writer(Generate kickoff artifact for stage 156)
  ⎿  Backgrounded agent (↓ to manage · ctrl+o to expand)

● Now let me implement the fix. The bug is in src/optimize.c at line 621-625:

  Planned Changes:
  - src/optimize.c — extend dead-code scan stop condition to include
  AST_CASE_SECTION and AST_DEFAULT_SECTION
  - src/version.c — update stage number to 15600000
  - test/integration/test_switch_break_o0/ — new test (switch state machine,
  -O0, expected 23)
  - test/integration/test_switch_break_o1/ — new test (switch state machine,
  -O1, expected 23)
  - test/integration/test_switch_break_default_o1/ — new test (switch with
  default case, -O1)
  - test/integration/test_switch_state_update_o1/ — new test (loop-carried enum
  state, -O1)

● Update(src/optimize.c)
  ⎿  Added 4 lines
      19   *            scan that frees all subsequent siblings up to the
           next label.
      20   * Stage 155: peephole infrastructure added in peephole.c / pee
          phole.h;
      21   *            this file (optimize.c) is unaffected.
      22 + * Stage 156: bug fix -- dead-code removal in AST_BLOCK must st
         +op at                                                          
      23 + *            AST_CASE_SECTION and AST_DEFAULT_SECTION, not onl
         +y at                                                           
      24 + *            AST_LABEL_STATEMENT.  Without this, a break in a 
         +switch body                                                    
      25 + *            incorrectly removes subsequent case/default secti
         +ons.                                                           
      26   */
      27  
      28  #include <stddef.h>

● Now fix the actual bug — the dead-code scan in AST_BLOCK:

● Update(src/optimize.c)
  ⎿  Added 3 lines, removed 1 line
      623                     or the end of the block, then compact the c
           hildren array. */
      624                  k = i + 1;
      625                  while (k < node->child_count &&
      626 -                       node->children[k]->type != AST_LABEL_ST
          -ATEMENT) {                                                    
      626 +                       node->children[k]->type != AST_LABEL_ST
          +ATEMENT &&                                                    
      627 +                       node->children[k]->type != AST_CASE_SEC
          +TION &&                                                       
      628 +                       node->children[k]->type != AST_DEFAULT_
          +SECTION) {                                                    
      629                      ast_free(node->children[k]);
      630                      k++;
      631                  }

● Now update version.c for stage 156:

● Update(src/version.c)
  ⎿  File must be read first

● Agent "Generate kickoff artifact for stage 156" came to rest · 15s

● The kickoff artifact is done. Now let me fix the version.c update (need to
  read first):

  Read 1 file (ctrl+o to expand)

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      3  
      4  #define VERSION_MAJOR  "00"
      5  #define VERSION_MINOR  "03"
      6 -#define VERSION_STAGE  "01550000"                               
      6 +#define VERSION_STAGE  "01560000"                               
      7  
      8  #ifndef VERSION_BUILD
      9  #define VERSION_BUILD  0
● Now create the integration tests. First, the state machine test used for both
  O0 and O1:

● Bash(mkdir -p
      /home/scott/code/claude/claudec99/test/integration/test_switch_break_o0
      mkdir -p
      /home/scott/code/claude/claudec99/test/integration/test_switch_break_o…)
  ⎿  Done

● Write(test/integration/test_switch_break_o0/test_switch_break_o0.c)
  ⎿  Wrote 72 lines to
     test/integration/test_switch_break_o0/test_switch_break_o0.c
      1 /* Regression test: switch-based state machine, -O0 baseline.
      2    Expected exit code: 23 (GCC and cc99 -O0 agree). */
      3 enum State {
      4     ST_START  = 0,
      5     ST_WORD   = 1,
      6     ST_NUMBER = 2,
      7     ST_COMMENT = 3,
      8     ST_ERROR  = 4
      9 };
     … +62 lines (ctrl+o to expand)

● Write(test/integration/test_switch_break_o0/test_switch_break_o0.expected)
  ⎿  Wrote 1 lines to
     test/integration/test_switch_break_o0/test_switch_break_o0.expected
      1 23

● Write(test/integration/test_switch_break_o0/test_switch_break_o0.cflags)
  ⎿  Wrote 1 lines to
     test/integration/test_switch_break_o0/test_switch_break_o0.cflags
      1 -O0

● Write(test/integration/test_switch_break_o1/test_switch_break_o1.c)
  ⎿  Wrote 74 lines to
     test/integration/test_switch_break_o1/test_switch_break_o1.c
       2    CC99-014: -O1 dead-code removal was removing subsequent case 
         sections
       3    after a break, leaving only the first case reachable via the 
         switch.
       4    Expected exit code: 23 (must match -O0 and GCC). */
       6     ST_START  = 0,
       7     ST_WORD   = 1,
       8     ST_NUMBER = 2,
       9     ST_COMMENT = 3,
      10     ST_ERROR  = 4
     … +66 lines (ctrl+o to expand)

● Write(test/integration/test_switch_break_o1/test_switch_break_o1.expected)
  ⎿  Wrote 1 lines to
     test/integration/test_switch_break_o1/test_switch_break_o1.expected
      1 23

● Write(test/integration/test_switch_break_o1/test_switch_break_o1.cflags)
  ⎿  Wrote 1 lines to
     test/integration/test_switch_break_o1/test_switch_break_o1.cflags
      1 -O1

● Now add a test for a switch with a default case and break in each arm:

● Write(test/integration/test_switch_break_default_o1/test_switch_break_default_
       o1.c)
  ⎿  Wrote 28 lines to test/integration/test_switch_break_default_o1/test_s
     witch_break_default_o1.c
         ak.Test: switch loop with a default case; each arm ends with bre
       2    Verifies that -O1 does not remove the default arm when case a
         rms have breaks. */
       3 static int classify(int x) {
       4     switch (x) {
       6         return 10;
       7         break;
       8     case 1:
       9         return 20;
      10         break;
     … +19 lines (ctrl+o to expand)

● Write(test/integration/test_switch_break_default_o1/test_switch_break_default_
       o1.expected)
  ⎿  Wrote 1 lines to test/integration/test_switch_break_default_o1/test_sw
     itch_break_default_o1.expected
      1 159

● Write(test/integration/test_switch_break_default_o1/test_switch_break_default_
       o1.cflags)
  ⎿  Wrote 1 lines to test/integration/test_switch_break_default_o1/test_sw
     itch_break_default_o1.cflags
      1 -O1

● Now create the loop-carried state test:

● Write(test/integration/test_switch_state_update_o1/test_switch_state_update_o1
       .c)
  ⎿  Wrote 25 lines to test/integration/test_switch_state_update_o1/test_sw
     itch_state_update_o1.c
         e variable.itch loop where each case updates a loop-carried stat
       2    Verifies that -O1 preserves state transitions across switch c
         ases. */
       3 int main(void) {
       4     int state = 0;
       6     int i;tal = 0;
       7     for (i = 0; i < 4; i = i + 1) {
       8         switch (state) {
       9         case 0:
      10             total = total + 1;
     … +17 lines (ctrl+o to expand)

● Write(test/integration/test_switch_state_update_o1/test_switch_state_update_o1
       .expected)
  ⎿  Wrote 1 lines to test/integration/test_switch_state_update_o1/test_swi
     tch_state_update_o1.expected
      1 112

● Write(test/integration/test_switch_state_update_o1/test_switch_state_update_o1
       .cflags)
  ⎿  Wrote 1 lines to test/integration/test_switch_state_update_o1/test_swi
     tch_state_update_o1.cflags
      1 -O1

● Also create the cc99_testing/ directory with the spec's reference file:

● Bash(mkdir -p /home/scott/code/claude/claudec99/cc99_testing)
  ⎿  Done

● Write(cc99_testing/test_o1_state_machine_switch.c)
  ⎿  Wrote 72 lines to cc99_testing/test_o1_state_machine_switch.c
        hine.99-014 reduced repro: -O1 miscompiles switch-based state mac
      2    Expected exit code: 23 (GCC, cc99 -O0, and fixed cc99 -O1 all 
        agree). */
      3 enum State {
      4     ST_START  = 0,
      5     ST_WORD   = 1,
      6     ST_NUMBER = 2,
      7     ST_COMMENT = 3,
      8     ST_ERROR  = 4
      9 };
     … +64 lines (ctrl+o to expand)

● Now build the compiler and run the tests:

● Bash(cmake -S . -B build -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -5 && cmake
      --build build 2>&1 | tail -5)
  ⎿  -- Configuring done (0.0s)
     -- Generating done (0.0s)
     -- Build files have been written to: /home/scott/code/claude/claudec99
     … +6 lines (ctrl+o to expand)

● Good, build succeeded. Let me verify the fix manually first:
● Bash(bin/cc99 -O0 -o /tmp/test_sm_o0
      cc99_testing/test_o1_state_machine_switch.c && /tmp/test_sm_o0; echo
      "exit: $?")
  ⎿  compiled: /home/scott/code/claude/claudec99/cc99_testing/test_o1_state
     _machine_switch.c -> test_o1_state_machine_switch.asm
     exit: 23

● Bash(bin/cc99 -O1 -o /tmp/test_sm_o1
      cc99_testing/test_o1_state_machine_switch.c && /tmp/test_sm_o1; echo
      "exit: $?")
  ⎿  compiled: /home/scott/code/claude/claudec99/cc99_testing/test_o1_state
     _machine_switch.c -> test_o1_state_machine_switch.asm
     exit: 23

● Both -O0 and -O1 now return 23. The fix works. Let me verify the other new
  tests too:

● Bash(bin/cc99 -O1 -o /tmp/test_sw_default test/integration/test_switch_break_d
      efault_o1/test_switch_break_default_o1.c && /tmp/test_sw_default; echo
      "exit: $?")
  ⎿  compiled: /home/scott/code/claude/claudec99/test/integration/test_swit
     ch_break_default_o1/test_switch_break_default_o1.c -> test_switch_brea
     k_default_o1.asm
     exit: 159

● Bash(bin/cc99 -O1 -o /tmp/test_sw_state
      test/integration/test_switch_state_update_o1/test_switch_state_update_o1.c
      && /tmp/test_sw_state; echo "exit: $?")
  ⎿  compiled: /home/scott/code/claude/claudec99/test/integration/test_swit
     ch_state_update_o1/test_switch_state_update_o1.c -> test_switch_state_
     update_o1.asm
     exit: 112

● All 4 new tests produce correct results. Now run the full test suite:

● Bash(python3 test/run_tests.py 2>&1 | tail -20)
  ⎿  python3: can't open file '/home/scott/code/claude/claudec99/test/run_t
     ests.py': [Errno 2] No such file or directory

  Listed 1 directory (ctrl+o to expand)

● Bash(bash test/run_all_tests.sh 2>&1 | tail -20)
  ⎿  FAIL  test_switch_break_o0  (expected 0, got 23)
     compiled: /home/scott/code/claude/claudec99/test/integration/test_swit
     ch_break_o1/test_switch_break_o1.c -> test_switch_break_o1.asm
     … +25 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● The expected files format seems wrong. Let me check how the test runner reads
  integration test expected values:

  Read 2 files (ctrl+o to expand)

● Bash(sed -n '80,150p'
      /home/scott/code/claude/claudec99/test/integration/run_tests.sh)
  ⎿      if [ -f "$status_file" ]; then
             expected_status="$(cat "$status_file")"
         else
     … +68 lines (ctrl+o to expand)

● Bash(sed -n '150,200p'
      /home/scott/code/claude/claudec99/test/integration/run_tests.sh)
  ⎿          (cd "$test_work" && timeout "$TIMEOUT" "$test_work/$name" "${e
     xtra_args[@]}" <"$input_file" >"$stdout_file")
         else
     … +37 lines (ctrl+o to expand)

● I see the issue. The test runner checks exit code (default expected 0)
  separately from stdout. My tests return the value as exit code but the runner
  expects exit code 0. I need to add .status files or change to print via
  printf. Let me look at existing exit-code tests:

  Read 1 file, listed 1 directory (ctrl+o to expand)

● The existing tests use printf and return 0. I need to either use .status files
  or change to printf. The printf approach matches the existing style. Let me
  update the test files to use printf-based output:

● Write(test/integration/test_switch_break_o0/test_switch_break_o0.c)
  ⎿  Added 4 lines, removed 2 lines
      1 +#include <stdio.h>                                              
      2  /* Regression test: switch-based state machine, -O0 baseline.
      2 -   Expected exit code: 23 (GCC and cc99 -O0 agree). */          
      3 +   Expected output: 23 (GCC and cc99 -O0 agree). */             
      4  enum State {
      5      ST_START  = 0,
      6      ST_WORD   = 1,
     ...
      69  }
      70  
      71  int main(void) {
      71 -    return parse_stream() & 255;                               
      72 +    printf("%d\n", parse_stream() & 255);                      
      73 +    return 0;                                                  
      74  }

● Write(test/integration/test_switch_break_o1/test_switch_break_o1.c)
  ⎿  Added 7 lines, removed 5 lines
      1 -/* Regression test: switch-based state machine, -O1.            
      2 -   CC99-014: -O1 dead-code removal was removing subsequent case 
        -sections                                                        
      3 -   after a break, leaving only the first case reachable via the 
        -switch.                                                         
      4 -   Expected exit code: 23 (must match -O0 and GCC). */          
      1 +#include <stdio.h>                                              
      2 +/* Regression test CC99-014: switch-based state machine, -O1.   
      3 +   Bug: dead-code removal was removing subsequent case sections 
        +after a break,                                                  
      4 +   leaving only the first case reachable via the switch.        
      5 +   Expected output: 23 (must match -O0 and GCC). */             
      6  enum State {
      7      ST_START  = 0,
      8      ST_WORD   = 1,
     ...
      71  }
      72  
      73  int main(void) {
      73 -    return parse_stream() & 255;                               
      74 +    printf("%d\n", parse_stream() & 255);                      
      75 +    return 0;                                                  
      76  }

● Write(test/integration/test_switch_break_default_o1/test_switch_break_default_
       o1.c)
  ⎿  Added 3 lines, removed 1 line
      1 +#include <stdio.h>                                              
      2  /* Test: switch loop with a default case; each arm ends with bre
         ak.
      3     Verifies that -O1 does not remove the default arm when case a
         rms have breaks. */
      4  static int classify(int x) {
     ...
      25      int c = classify(2);
      26      int d = classify(3);
      27      /* 10 + 20 + 30 + 99 = 159 */
      27 -    return (a + b + c + d) & 255;                              
      28 +    printf("%d\n", (a + b + c + d) & 255);                     
      29 +    return 0;                                                  
      30  }

● Write(test/integration/test_switch_state_update_o1/test_switch_state_update_o1
       .c)
  ⎿  Added 3 lines, removed 1 line
      1 +#include <stdio.h>                                              
      2  /* Test: switch loop where each case updates a loop-carried stat
         e variable.
      3     Verifies that -O1 preserves state transitions across switch c
         ases. */
      4  int main(void) {
     ...
      22          }
      23      }
      24      /* state transitions: 0->1->2->0->1; totals: 1+10+100+1=112
           */
      24 -    return total & 255;                                        
      25 +    printf("%d\n", total);                                     
      26 +    return 0;                                                  
      27  }

● Now update the expected files to match printf output (just the number printed,
  not exit code):

● Write(test/integration/test_switch_break_default_o1/test_switch_break_default_
       o1.expected)
  ⎿  

● Write(test/integration/test_switch_state_update_o1/test_switch_state_update_o1
       .expected)
  ⎿  
● Now run the full test suite again:

● Bash(bash test/run_all_tests.sh 2>&1 | tail -30)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/integration/test_stri
     ng_h_memcmp/test_string_h_memcmp.c -> test_string_h_memcmp.asm
     compiled: /home/scott/code/claude/claudec99/test/integration/test_stri
     … +49 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● Bash(bash test/run_all_tests.sh 2>&1 | grep -E "Results:|FAIL")
  ⎿  Results: 106 passed, 0 failed, 106 total
     Results: 59 passed, 0 failed, 59 total
     Results: 165 passed, 0 failed, 165 total
     … +7 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● All 2049 portable tests pass (165 unit, 1286 valid, 261 invalid, 166
  integration, 50 print-AST, 100 print-tokens, 21 print-asm). Now commit:

● Bash(git status)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +18 lines (ctrl+o to expand)

● Bash(gcc --version | head -1)
  ⎿  gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0

● Bash(git add src/optimize.c src/version.c docs/kickoffs/stage-156-kickoff.md
      docs/stages/ClaudeC99-spec-stage-156-optimization-bug.md cc99_testing/
      test/integration/…)
  ⎿  (No output)

● Bash(git commit -m "$(cat <<'EOF'
      stage 156: fix -O1 miscompile of switch-based state machine (CC99-014)…)
  ⎿  [master c0ebafc] stage 156: fix -O1 miscompile of switch-based state m
     achine (CC99-014)
      17 files changed, 525 insertions(+), 2 deletions(-)
     … +18 lines (ctrl+o to expand)

● Now run the self-host test:

● Bash(./build.sh --mode=self-host 2>&1)
  ⎿  === Self-host test ===
     Archiving build/ccompiler-c0 -> build/previous/ccompiler-c0
     Archiving build/ccompiler-c1 -> build/previous/ccompiler-c1
     … +514 lines (ctrl+o to expand)
  ⎿  (timeout 10m)

  Read 1 file (ctrl+o to expand)

● Self-host C0→C1→C2 verified with all tests passing at every stage. No source
  changes needed during bootstrap.

  Now let me update the self-compilation report:

  Read 1 file (ctrl+o to expand)

● Update(docs/self-compilation-report.md)
  ⎿  Added 22 lines
       1  # Self-Compilation Diagnostic Report
       2  
       3 +## Issues found during stage 156 self-hosting test             
       4 +                                                               
       5 +None. The change is confined to `src/optimize.c`: the dead-code
         + scan loop                                                     
       6 +in the `AST_BLOCK` case of `optimize_stmt` gained two additiona
         +l stop                                                         
       7 +conditions (`AST_CASE_SECTION` and `AST_DEFAULT_SECTION`), a mi
         +nimal                                                          
       8 +one-line change with no new declarations, VLAs, or C99-only fea
         +tures.                                                         
       9 +All 2049 tests passed at C0, C1, and C2 with no source changes 
         +needed                                                         
      10 +during bootstrap.                                              
      11 +                                                               
      12 +## Result (stage 156)                                          
      13 +                                                               
      14 +**Date:** 2026-06-20                                           
      15 +**Method:** `./build.sh --mode=self-host`                      
      16 +                                                               
      17 +| Step | Binary | Version | Tests |                            
      18 +|------|--------|---------|-------|                            
      19 +| C0 | `build/ccompiler-c0` | `00.03.01560000.01157` | 2049/204
         +9 |                                                            
      20 +| C1 | `build/ccompiler-c1` | `00.03.01560000.01158` | 2049/204
         +9 |                                                            
      21 +| C2 | `build/ccompiler-c2` | `00.03.01560000.01159` | 2049/204
         +9 |                                                            
      22 +                                                               
      23 +---                                                            
      24 +                                                               
      25  ## Issues found during stage 155 self-hosting test
      26  
      27  One bootstrap issue fixed: `include/peephole.h` originally used

● Bash(git add docs/self-compilation-report.md && git commit -m "$(cat <<'EOF'
      docs: stage 156 self-host report…)
  ⎿  [master 3aff764] docs: stage 156 self-host report
      1 file changed, 22 insertions(+)

● Now delegate the milestone summary, transcript, and README/checklist updates
  to haiku-stage-artifact-writer:

● haiku-stage-artifact-writer(Generate stage 156 milestone, transcript, README,
                             checklist updates)
  ⎿  Done (12 tool uses · 65.8k tokens · 46s)
  (ctrl+o to expand)

● Let me verify the generated artifacts:

  Searched for 4 patterns, read 1 file, listed 1 directory (ctrl+o to expand)

● The checklist doesn't have a Stage 156 section. Let me add it:

● Update(docs/outlines/checklist.md)
  ⎿  Added 12 lines
      2224  TODO items completed this stage:
      2225  - [x] Infrastructure: `peephole.c` / `include/peephole.h`; sl
            iding window (2–4 lines) over the output buffer; patterns exp
            ressed as matcher + replacer functions (Stage 155)
      2226  
      2227 +## Stage 156 - -O1 Switch Dead-Code Removal Bug Fix (CC99-014
           +)                                                            
      2228 +                                                             
      2229 +- [x] Bug fix in `src/optimize.c`: dead-code scan in `AST_BLO
           +CK` case now stops at `AST_CASE_SECTION` and `AST_DEFAULT_SEC
           +TION` in addition to `AST_LABEL_STATEMENT`                   
      2230 +  - Previously: `break` in a switch body block caused all sub
           +sequent case/default sections to be removed as "dead code," s
           +ince only `AST_LABEL_STATEMENT` was recognized as a control-f
           +low boundary                                                 
      2231 +  - Fix: two additional conditions added to the while loop sc
           +anning for the end of the dead zone                          
      2232 +- [x] 4 new integration tests: `test_switch_break_o0`, `test_
           +switch_break_o1` (CC99-014 repro), `test_switch_break_default
           +_o1`, `test_switch_state_update_o1`                          
      2233 +- [x] `cc99_testing/test_o1_state_machine_switch.c`: manual r
           +epro file matching spec                                      
      2234 +- [x] Test results: 2049/2049 portable tests pass (165 unit, 
           +1286 valid, 261 invalid, 166 integration, 50 print-AST, 100 p
           +rint-tokens, 21 print-asm)                                   
      2235 +- [x] Self-host C0→C1→C2 verified; no source changes during b
           +ootstrap (Stage 156)                                         
      2236 +                                                             
      2237 +TODO items completed this stage: none (pure bug fix)         
      2238 +                                                             
      2239  ---
      2240  
      2241  ## TODO

● Now commit the documentation artifacts:

● Bash(git add docs/milestones/stage-156-milestone.md
      docs/sessions/stage-156-transcript.md docs/outlines/checklist.md README.md
      && git status)
  ⎿  On branch master
     Your branch is ahead of 'origin/master' by 4 commits.
       (use "git push" to publish your local commits)
     … +7 lines (ctrl+o to expand)
● Bash(git commit -m "$(cat <<'EOF'
      docs: stage 156 milestone, transcript, README, checklist…)
  ⎿  [master f284f90] docs: stage 156 milestone, transcript, README, checkl
     ist
      4 files changed, 89 insertions(+)
     … +2 lines (ctrl+o to expand)

● Stage 156 is complete. Here's a summary:

  Root cause (CC99-014): The dead-code removal pass in optimize_stmt's AST_BLOCK
  case only stopped scanning at AST_LABEL_STATEMENT. In a switch body block,
  case/default sections are stored as siblings of the break statements between
  them — not as AST_LABEL_STATEMENT nodes. So when a break at position i was
  reached, the optimizer would scan forward removing AST_CASE_SECTION and
  AST_DEFAULT_SECTION nodes as "dead code," leaving only the first case
  reachable.

  Fix: Added AST_CASE_SECTION and AST_DEFAULT_SECTION as stop conditions in the
  dead-code scan loop (2-line change in src/optimize.c).

  Results:
  - cc99 -O1 now returns 23 (matching cc99 -O0 and GCC) for the state machine
  repro
  - 4 new integration tests added
  - All 2049 portable tests pass at every stage
  - Self-host C0→C1→C2 verified with no bootstrap changes needed

✻ Baked for 14m 50s