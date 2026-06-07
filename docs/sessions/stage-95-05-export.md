 ▐▛███▜▌   Claude Code v2.1.168
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

  Feature of the week: /loop — run a prompt or slash command on a recurring interval

✻ Conversation compacted (ctrl+o for history)


● Two patterns to watch for in future stages:

  1. *(T **)vec_get(...) — check before committing

  Any time a Vec stores pointer elements (like ASTNode * or Type *), retrieval
  requires *(T **)vec_get(...). C0's parser can't handle this (the outer * sends
  the (T **) into parse_primary's parse_expression, which doesn't know it's a
  type). Catch it before self-hosting:

  grep -n '\*(.*\*\*)vec_get' src/*.c

  If any match, split immediately into two statements:
  T **ptr = (T **)vec_get(&v, i);
  T *val = *ptr;

  2. (size_t)-1 / expr — always use local variables

  Whenever you write an overflow guard involving division by a size_t value or
  constant-cast expression, the is_unsigned flag won't propagate through the
  cast or struct-member access, producing idiv (signed). The safe pattern (used
  in both vec_reserve and now vec_push) is:

  size_t lim = (size_t)-1;
  size_t cap = v->cap;   /* or whatever you're dividing by */
  if (cap > lim / 2) ...

  3. For bogus error positions — binary search is the right tool, but do it by
  function, not by line:

  - Comment out the second half of the failing function body and rerun C0 on it
  - If the error disappears → the bug is in the second half → restore, comment
  out three-quarters → repeat
  - If the error message SHIFTS proportionally to lines removed but doesn't
  disappear → the bug is in what you left visible

  The position drift is proportional to how many save/restore operations
  (parse_cast, parse_assignment_expression) ran before the actual failure — so a
  large function earlier in the file increases drift. Binary search by function
  (comment out entire functions) converges faster than by line.

  4. For future capacity/signed-division failures — test the pattern with C0
  before committing:

  /* quick canary: if this fails, you have a signed-division issue */
  size_t x = (size_t)-1;
  size_t half = x / 2;
  assert(half > 1000000);  /* should be ~9.2e18, not 0 */

✻ Cogitated for 26s

❯ add those to the fixed-capacity-inventory.md as a separate section.           
  ⎿  Referenced file README.md
  ⎿  Read docs/milestones/stage-95-05-milestone.md (11 lines)
  ⎿  Read docs/sessions/stage-95-05-transcript.md (123 lines)
  ⎿  Read docs/fixed-capacity-inventory.md (120 lines)
  ⎿  Referenced file docs/self-compilation-report.md
  ⎿  Skills restored (implement-stage)

● I'll add a "Bootstrap Compatibility Notes" section at the end of the inventory
  file documenting the two C0 limitations discovered.

● Update(docs/fixed-capacity-inventory.md)
  ⎿  Added 54 lines
       90  
       91  ---
       92  
       93 +## Bootstrap Compatibility Notes                              
       94 +                                                              
       95 +These patterns were discovered during Vec-conversion self-host
          +ing cycles. Apply them whenever converting a fixed-capacity ar
          +ray to Vec to avoid bootstrap failures.                       
       96 +                                                              
       97 +### C0 cannot parse `*(T **)vec_get(...)`                     
       98 +                                                              
       99 +When a Vec stores pointer elements (e.g. `ASTNode *`, `Type *`
          +), retrieval requires a cast-of-dereference: `*(T **)vec_get(&
          +v, i)`. C0's `parse_unary` consumes the outer `*`, then `parse
          +_primary` sees `(` and calls `parse_expression`, which can't r
          +ecognise a cast expression starting with a typedef name. This 
          +produces a bogus "expected expression, got ')'" error at a mis
          +leading source position.                                      
      100 +                                                              
      101 +**Detection:**                                                
      102 +                                                              
      103 +```bash                                                       
      104 +grep -n '\*(.*\*\*)vec_get' src/*.c                           
      105 +```                                                           
      106 +                                                              
      107 +**Fix:** split into two statements:                           
      108 +                                                              
      109 +```c                                                          
      110 +T **ptr = (T **)vec_get(&v, i);                               
      111 +T *s    = *ptr;                                               
      112 +```                                                           
      113 +                                                              
      114 +### C0 uses signed division for `(size_t)-1 / expr`           
      115 +                                                              
      116 +`AST_CAST` codegen sets `result_type` but not `is_unsigned` on
          + the result node. Any expression whose value flows through a c
          +ast — including `(size_t)-1` — is treated as signed by the dow
          +nstream arithmetic, so the division `(size_t)-1 / X` emits `cq
          +o; idiv` instead of `xor edx,edx; div`. The quotient is 0 (sig
          +ned -1÷X) instead of `SIZE_MAX/X`, turning any overflow guard 
          +of the form `if (v->cap > (size_t)-1 / 2)` into `if (v->cap > 
          +0)` — a false positive for every non-empty Vec.               
      117 +                                                              
      118 +Arrow-member access (`->`) has the same defect: `is_unsigned` 
          +is not propagated to the access result, so `v->cap / 2` is als
          +o signed.                                                     
      119 +                                                              
      120 +**Fix:** introduce explicit local `size_t` variables for both 
          +operands; the parser marks explicitly-declared `size_t` locals
          + as `is_unsigned = 1`, which propagates correctly through `uac
          +_is_unsigned`:                                                
      121 +                                                              
      122 +```c                                                          
      123 +size_t lim = (size_t)-1;                                      
      124 +size_t cap = v->cap;                                          
      125 +if (cap > lim / 2)                                            
      126 +    vec_fatal("capacity overflow");                           
      127 +```                                                           
      128 +                                                              
      129 +### Error positions are always wrong during a bootstrap failur
          +e                                                             
      130 +                                                              
      131 +C0's `parse_cast` and `parse_assignment_expression` restore `l
          +exer->pos` and `parser->current` on backtrack but do **not** r
          +estore `lexer->line` / `lexer->col`. Each failed speculative p
          +arse leaves accumulated line-drift, so the reported error posi
          +tion is always earlier in the file than the actual failure.   
      132 +                                                              
      133 +**Strategy:** ignore the reported position. Binary-search by c
          +ommenting out the second half of the failing function body; if
          + the error disappears, the construct is in the commented half.
          + Bisect until the minimal failing expression is isolated.     
      134 +                                                              
      135 +### Reproduction test before full self-host                   
      136 +                                                              
      137 +Extract the suspicious pattern into a 20–50 line file and comp
          +ile directly with C0:                                         
      138 +                                                              
      139 +```bash                                                       
      140 +build/ccompiler-c0 -I include -I test/include /tmp/repro.c    
      141 +```                                                           
      142 +                                                              
      143 +This iterates in seconds instead of waiting for a full `./buil
          +d.sh --mode=self-host` cycle.                                 
      144 +                                                              
      145 +---                                                           
      146 +                                                              
      147  ## Summary by Priority
      148  
      149  ### HIGH — fix before compiling larger programs

● Done. Added a "Bootstrap Compatibility Notes" section before the Summary by
  Priority, covering:

  1. *(T **)vec_get(...) mis-parse — why it fails, detection grep, and the
  two-statement fix
  2. Signed division via cast/arrow — root cause (AST_CAST and arrow access
  don't propagate is_unsigned), what the overflow guard actually computes under
  C0, and the local-variable workaround
  3. Bogus error positions — why they're always wrong and the binary-search
  strategy
  4. Minimal reproduction test — the one-liner to iterate quickly against C0

✻ Crunched for 2m 18s

❯ commit this                                                                   

● Bash(git diff && git log --oneline -5)
  ⎿  diff --git a/docs/fixed-capacity-inventory.md b/docs/fixed-capacity-in
     ventory.md
     index b99a931..c51cca0 100644
     … +67 lines (ctrl+o to expand)

● Bash(git commit docs/fixed-capacity-inventory.md -m "$(cat <<'EOF'
      docs: add bootstrap compatibility notes to fixed-capacity inventory…)
  ⎿  [master 919d568] docs: add bootstrap compatibility notes to fixed-capa
     city inventory
      1 file changed, 54 insertions(+)

● Committed as 919d568.

✻ Brewed for 10s