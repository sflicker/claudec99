 ▐▛███▜▌   Claude Code v2.1.114
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

❯ /implement-stage ClaudeC99-spec-stage-10-03-02-case-and-dispatch.md           

  Searched for 1 pattern, read 5 files, listed 4 directories (ctrl+o to expand)

  Read 29 files, listed 2 directories (ctrl+o to expand)

● Now let me check the existing build to understand state.

  Listed 1 directory (ctrl+o to expand)

● Now I have a full picture. Let me echo the stage label and produce the kickoff
   summary.

  STAGE_DISPLAY: Stage-10-03-02

  Kickoff Summary

  Spec Summary

  Stage 10-03-02 extends the restricted switch statement from stage 10-03-01 to
  support case labels with integer-literal values in addition to the optional
  default section. The switch body now consists of one or more
  <switch_section>s, each of which is a <case_section> (case <int_literal>:
  followed by zero or more statements) or a <default_section> (default: followed
   by zero or more statements). The controlling expression is evaluated once,
  compared against each case value in source order, and control transfers to the
   matching section; if no case matches, control transfers to default if
  present, otherwise to the end of the switch. break exits the switch.
  Restrictions: case values must be integer literals, no stacked labels, at most
   one default, no unlabeled statements outside a section, fallthrough is not
  required to work (tests must not rely on it), duplicate-case diagnostics are
  optional.

  Changes from Stage 10-03-01

  1. Tokens: Add TOKEN_CASE and recognize keyword case in the lexer.
  2. AST: Add AST_CASE_SECTION with children [IntLiteral, Statement, Statement, 
  …] — the first child is the case value, followed by the statements (parallel
  to how other nodes store auxiliaries as first children). Switch body is now a
  flat list of sections under AST_SWITCH_STATEMENT, so its AST layout changes
  from [expr, default_section] to [expr, section_0, section_1, …].
  3. Parser: Replace the "exactly one default" body parse with a loop that reads
   a sequence of case/default sections until }. Each section consumes statements
   until the next case, default, or closing brace. Enforce at-most-one default.
  Reject stacked labels implicitly by requiring each case/default to be followed
   by at least the colon plus statements (two consecutive case lines with no
  statements will naturally fall out of the grammar rule that allows zero
  statements — spec says "Duplicate ... do not need to be diagnosed" but
  "Stacked labels are not supported"; since case N: followed by case M: is legal
   per the grammar as two back-to-back sections each with zero statements, that
  is NOT stacked labels in the C sense, just empty sections — I'll note this as
  an ambiguity below).
  4. Code generation: For each switch, evaluate the expression once, save it in
  a stack slot, emit a linear compare-and-branch dispatch sequence (jumping to
  per-section labels), then emit the section bodies. No match ⇒ jump to default
  if present, else to end.
  5. Pretty printer: Add CaseSection: rendering.
  6. Grammar doc: Update to the new <switch_body> / <switch_section> /
  <case_section> productions.
  7. Tests: Existing tests in test/valid/test_switch_with_case_*.c and
  test_switch_with_return_inside_case__20.c already cover matching cases,
  default fallback, missing default, and early return. Will run them; add tests
  only if gaps are found.

  Ambiguities / Decisions

  1. Empty case sections vs. stacked labels. The spec forbids stacked labels
  (case 1: case 2: stmt;), but the grammar <case_section> ::= "case" 
  <integer_literal> ":" { <statement> } allows zero-statement sections. Two
  consecutive case N: case M: lines parse as two legal empty sections, which in
  C semantics would be fallthrough from the empty case N: into case M:. Since
  the spec also says "Tests for this stage must not rely on fallthrough
  behavior," I will implement the grammar literally: empty sections are accepted
   syntactically, and codegen will emit each section back-to-back — if the
  selected section is empty, control flows into the next section's statements,
  mirroring C semantics for free. I will NOT add a diagnostic for empty case
  sections.
  2. Case value restriction. Spec says integer literals only for this stage, so
  the parser calls parser_expect(TOKEN_INT_LITERAL) after case — no
  general-expression parse. Negative case values (case -1:) are not mentioned in
   the spec; I will accept a bare integer literal only (no unary-minus),
  matching the spec's wording literally and consistent with other stages.
  3. Duplicate case diagnostics. Spec explicitly says optional ("unless this is
  easy to add"). I will skip it to keep changes minimal.
  4. Evaluation of switch expression. Spec is explicit: evaluated exactly once.
  Codegen will evaluate the expression, store the result in a spilled stack slot
   (a fresh local), and compare from that slot for each case — rather than
  re-evaluating per case.

  Planned Changes

  ┌──────────────────────────┬───────────────────────────────────────────────┐
  │           File           │                    Change                     │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ include/token.h          │ Add TOKEN_CASE                                │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ src/lexer.c              │ Recognize case keyword                        │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ include/ast.h            │ Add AST_CASE_SECTION                          │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ src/ast_pretty_printer.c │ Print CaseSection:                            │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │                          │ Replace single-default parse with a loop over │
  │ src/parser.c             │  { case|default … }; enforce at-most-one      │
  │                          │ default                                       │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │                          │ Emit switch dispatch: spill expr to a slot,   │
  │ src/codegen.c            │ compare-and-branch per case, default/end      │
  │                          │ fallback                                      │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ docs/grammar.md          │ Update switch productions                     │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ Tests                    │ Run existing suite; add tests only if gaps    │
  │                          │ are found                                     │
  └──────────────────────────┴───────────────────────────────────────────────┘

  Next Step

  Implement in this order: tokenizer → AST → parser → codegen → pretty printer →
   docs → tests → milestone/transcript → commit.

  Let me save the kickoff and pause for confirmation.

● Write(docs/kickoffs/stage-10-03-02-kickoff.md)
  ⎿  Wrote 144 lines to docs/kickoffs/stage-10-03-02-kickoff.md
       2 # stage-10-03-02 Kickoff: switch case and dispatch
       3 ## Spec Summary
       5 Stage 10-03-02 extends the restricted `switch` statement from st
         age
       6 10-03-01 to support `case` labels with integer-literal values, i
       7 addition to the optional `default` section. The switch body is n
         ow a
       9 `<case_section>` (`case <int_literal>:` followed by zero or more
      10 statements) or a `<default_section>` (`default:` followed by zer
         o or
     … +144 lines (ctrl+o to expand)

● Kickoff saved. Proceeding with implementation — starting with tokenizer (add
  TOKEN_CASE + lexer keyword).

  Step 1: Tokenizer

● Update(include/token.h)
  ⎿  Added 1 line
      14      TOKEN_CONTINUE,
      15      TOKEN_SWITCH,
      16      TOKEN_DEFAULT,
      17 +    TOKEN_CASE,                                                
      18      TOKEN_IDENTIFIER,
      19      TOKEN_INT_LITERAL,
      20      TOKEN_LPAREN,

● Update(src/lexer.c)
  ⎿  Added 2 lines
      100              token.type = TOKEN_SWITCH;
      101          } else if (strcmp(token.value, "default") == 0) {
      102              token.type = TOKEN_DEFAULT;
      103 +        } else if (strcmp(token.value, "case") == 0) {        
      104 +            token.type = TOKEN_CASE;                          
      105          } else {
      106              token.type = TOKEN_IDENTIFIER;
      107          }

● Step 2: AST

● Update(include/ast.h)
  ⎿  Added 1 line
      15      AST_FOR_STATEMENT,
      16      AST_SWITCH_STATEMENT,
      17      AST_DEFAULT_SECTION,
      18 +    AST_CASE_SECTION,                                          
      19      AST_BREAK_STATEMENT,
      20      AST_CONTINUE_STATEMENT,
      21      AST_BLOCK,

● Step 3: Pretty printer

● Update(src/ast_pretty_printer.c)
  ⎿  Added 6 lines
       93      case AST_DEFAULT_SECTION:
       94          printf("DefaultSection:\n");
       95          break;
       96 +    case AST_CASE_SECTION:                                    
       97 +        printf("CaseSection: %s\n", node->children[0]->value);
       98 +        for (int i = 1; i < node->child_count; i++) {         
       99 +            ast_pretty_print(node->children[i], depth + 1);   
      100 +        }                                                     
      101 +        return;                                               
      102      case AST_BREAK_STATEMENT:
      103          printf("BreakStmt\n");
      104          break;

● Step 4: Parser — replace single-default parse with a section loop.

● Update(src/parser.c)
  ⎿  Added 41 lines, removed 10 lines
      455  
      456  /*
      457   * <switch_statement> ::= "switch" "(" <expression> ")" <switc
           h_body>
      458 - * <switch_body>      ::= "{" <default_section> "}"           
      458 + * <switch_body>      ::= "{" <switch_section> { <switch_secti
          +on> } "}"                                                     
      459 + * <switch_section>   ::= <case_section> | <default_section>  
      460 + * <case_section>     ::= "case" <integer_literal> ":" { <stat
          +ement> }                                                      
      461   * <default_section>  ::= "default" ":" { <statement> }
      462   *
      461 - * Stage 10-03-01 restriction: the switch body must contain ex
          -actly one                                                     
      462 - * `default` label and no other statements outside it.        
      463 + * Stage 10-03-02: the switch body is a sequence of case/defau
          +lt                                                            
      464 + * sections. At most one default is allowed. Case values must 
          +be                                                            
      465 + * integer literals. Statements within a section are consumed 
          +until                                                         
      466 + * the next `case`, `default`, or closing `}`.                
      467   */
      468  static ASTNode *parse_switch_statement(Parser *parser) {
      469      parser_expect(parser, TOKEN_SWITCH);
     ...
      471      ASTNode *expr = parse_expression(parser);
      472      parser_expect(parser, TOKEN_RPAREN);
      473      parser_expect(parser, TOKEN_LBRACE);
      470 -    parser_expect(parser, TOKEN_DEFAULT);                     
      471 -    parser_expect(parser, TOKEN_COLON);                       
      474  
      475 +    ASTNode *switch_node = ast_new(AST_SWITCH_STATEMENT, NULL)
          +;                                                             
      476 +    ast_add_child(switch_node, expr);                         
      477 +                                                              
      478      parser->switch_depth++;
      474 -    ASTNode *default_section = ast_new(AST_DEFAULT_SECTION, NU
          -LL);                                                          
      479 +    int default_seen = 0;                                     
      480      while (parser->current.type != TOKEN_RBRACE) {
      476 -        ast_add_child(default_section, parse_statement(parser)
          -);                                                            
      481 +        if (parser->current.type == TOKEN_CASE) {             
      482 +            parser_expect(parser, TOKEN_CASE);                
      483 +            Token value = parser_expect(parser, TOKEN_INT_LITE
          +RAL);                                                         
      484 +            parser_expect(parser, TOKEN_COLON);               
      485 +            ASTNode *case_section = ast_new(AST_CASE_SECTION, 
          +NULL);                                                        
      486 +            ast_add_child(case_section, ast_new(AST_INT_LITERA
          +L, value.value));                                             
      487 +            while (parser->current.type != TOKEN_CASE &&      
      488 +                   parser->current.type != TOKEN_DEFAULT &&   
      489 +                   parser->current.type != TOKEN_RBRACE) {    
      490 +                ast_add_child(case_section, parse_statement(pa
          +rser));                                                       
      491 +            }                                                 
      492 +            ast_add_child(switch_node, case_section);         
      493 +        } else if (parser->current.type == TOKEN_DEFAULT) {   
      494 +            if (default_seen) {                               
      495 +                fprintf(stderr, "error: duplicate default sect
          +ion in switch\n");                                            
      496 +                exit(1);                                      
      497 +            }                                                 
      498 +            default_seen = 1;                                 
      499 +            parser_expect(parser, TOKEN_DEFAULT);             
      500 +            parser_expect(parser, TOKEN_COLON);               
      501 +            ASTNode *default_section = ast_new(AST_DEFAULT_SEC
          +TION, NULL);                                                  
      502 +            while (parser->current.type != TOKEN_CASE &&      
      503 +                   parser->current.type != TOKEN_DEFAULT &&   
      504 +                   parser->current.type != TOKEN_RBRACE) {    
      505 +                ast_add_child(default_section, parse_statement
          +(parser));                                                    
      506 +            }                                                 
      507 +            ast_add_child(switch_node, default_section);      
      508 +        } else {                                              
      509 +            parser_expect(parser, TOKEN_CASE); /* reports "exp
          +ected token type" */                                          
      510 +        }                                                     
      511      }
      512      parser->switch_depth--;
      513  
      514      parser_expect(parser, TOKEN_RBRACE);
      515  
      482 -    ASTNode *switch_node = ast_new(AST_SWITCH_STATEMENT, NULL)
          -;                                                             
      483 -    ast_add_child(switch_node, expr);                         
      484 -    ast_add_child(switch_node, default_section);              
      516      return switch_node;
      517  }
      518  

● Step 5: Code generation

● Update(src/codegen.c)
  ⎿  Added 48 lines, removed 8 lines
      316          fprintf(cg->output, ".L_break_%d:\n", label_id);
      317          cg->break_depth--;
      318      } else if (node->type == AST_SWITCH_STATEMENT) {
      319 -        /* children: [0]=switch expression, [1]=AST_DEFAULT_SE
          -CTION.                                                        
      320 -         * Stage 10-03-01: only a `default` section is allowed
          -, so control                                                  
      321 -         * simply evaluates the expression (side effects), the
          -n falls into                                                  
      322 -         * the default body. `break` exits to the switch-end l
          -abel. */                                                      
      319 +        /* children: [0]=controlling expression, [1..]=section
          +s (case or                                                    
      320 +         * default). Evaluate the expression once, spill to th
          +e stack,                                                      
      321 +         * then linearly compare against each case value and j
          +ump to the                                                    
      322 +         * matching section label. If no case matches, jump to
          + default                                                      
      323 +         * (if present) or to the switch-end label. `break` al
          +so targets                                                    
      324 +         * the end label. */                                  
      325          int label_id = cg->label_count++;
      326 +        int num_sections = node->child_count - 1;             
      327 +        int section_labels[AST_MAX_CHILDREN];                 
      328 +        int default_label = -1;                               
      329 +        for (int i = 0; i < num_sections; i++) {              
      330 +            section_labels[i] = cg->label_count++;            
      331 +            if (node->children[i + 1]->type == AST_DEFAULT_SEC
          +TION) {                                                       
      332 +                default_label = section_labels[i];            
      333 +            }                                                 
      334 +        }                                                     
      335 +                                                              
      336          codegen_expression(cg, node->children[0]);
      337 +        fprintf(cg->output, "    push rax\n");                
      338 +        cg->push_depth++;                                     
      339 +                                                              
      340 +        for (int i = 0; i < num_sections; i++) {              
      341 +            ASTNode *section = node->children[i + 1];         
      342 +            if (section->type == AST_CASE_SECTION) {          
      343 +                fprintf(cg->output, "    mov eax, [rsp]\n");  
      344 +                fprintf(cg->output, "    cmp eax, %s\n",      
      345 +                        section->children[0]->value);         
      346 +                fprintf(cg->output, "    je .L_switch_sec_%d\n
          +",                                                            
      347 +                        section_labels[i]);                   
      348 +            }                                                 
      349 +        }                                                     
      350 +        if (default_label != -1) {                            
      351 +            fprintf(cg->output, "    jmp .L_switch_sec_%d\n", 
          +default_label);                                               
      352 +        } else {                                              
      353 +            fprintf(cg->output, "    jmp .L_switch_end_%d\n", 
          +label_id);                                                    
      354 +        }                                                     
      355 +                                                              
      356          cg->break_stack[cg->break_depth].break_label = label_i
           d;
      357          cg->break_stack[cg->break_depth].continue_label = -1;
      358          cg->break_depth++;
      328 -        ASTNode *default_section = node->children[1];         
      329 -        for (int i = 0; i < default_section->child_count; i++)
          - {                                                            
      330 -            codegen_statement(cg, default_section->children[i]
          -, is_main);                                                   
      359 +                                                              
      360 +        for (int i = 0; i < num_sections; i++) {              
      361 +            ASTNode *section = node->children[i + 1];         
      362 +            fprintf(cg->output, ".L_switch_sec_%d:\n", section
          +_labels[i]);                                                  
      363 +            int start_idx = (section->type == AST_CASE_SECTION
          +) ? 1 : 0;                                                    
      364 +            for (int j = start_idx; j < section->child_count; 
          +j++) {                                                        
      365 +                codegen_statement(cg, section->children[j], is
          +_main);                                                       
      366 +            }                                                 
      367          }
      368 +                                                              
      369 +        cg->break_depth--;                                    
      370 +                                                              
      371          fprintf(cg->output, ".L_switch_end_%d:\n", label_id);
      372          fprintf(cg->output, ".L_break_%d:\n", label_id);
      334 -        cg->break_depth--;                                    
      373 +        fprintf(cg->output, "    add rsp, 8\n");              
      374 +        cg->push_depth--;                                     
      375      } else if (node->type == AST_BREAK_STATEMENT) {
      376          int id = cg->break_stack[cg->break_depth - 1].break_la
           bel;
      377          fprintf(cg->output, "    jmp .L_break_%d\n", id);

● Step 6: Grammar doc

● Update(docs/grammar.md)
  ⎿  Added 5 lines, removed 1 line
      38  
      39  <switch_statement> ::= "switch" "(" <expression> ")" <switch_bo
          dy>
      40  
      41 -<switch_body> ::= "{" <default_section> "}"                    
      41 +<switch_body> ::= "{" <switch_section> { <switch_section> } "}"
      42  
      43 +<switch_section> ::= <case_section> | <default_section>        
      44 +                                                               
      45 +<case_section> ::= "case" <integer_literal> ":" { <statement> }
      46 +                                                               
      47  <default_section> ::= "default" ":" { <statement> }
      48  
      49  <jump_statement> ::= "continue" ";"

● Now build and run tests.

● Bash(cd /home/scott/code/claude/claudec99/build && cmake --build . 2>&1 | tail
       -30)
  ⎿  Consolidate compiler generated dependencies of target ccompiler
     [ 12%] Building C object CMakeFiles/ccompiler.dir/src/compiler.c.o
     [ 25%] Building C object CMakeFiles/ccompiler.dir/src/ast.c.o
     … +7 lines (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99 && ./test/valid/run_tests.sh 2>&1 |
      tail -15)  ⎿  PASS  test_unary_bang_zero__1  (exit code: 1)
     compiled: /home/scott/code/claude/claudec99/test/valid/test_unary_neg_
     _214.c -> test_unary_neg__214.asm
     … +14 lines (ctrl+o to expand)

● All 113 valid tests pass. Now invalid + print_ast:

● Bash(./test/invalid/run_tests.sh 2>&1 | tail -20 && echo "====" &&
      ./test/print_ast/run_tests.sh 2>&1 | tail -15)
  ⎿  PASS  test_invalid_10__parameter_count_mismatch  (error contains: 'par
     ameter count mismatch')
     PASS  test_invalid_11__break_outside_of_loop  (error contains: 'break
     … +18 lines (ctrl+o to expand)

● All 136 tests pass (113 valid + 13 invalid + 10 print_ast). 5 new case tests
  over the 108 from stage-10-03-01; no regressions.

  Let me verify one switch-with-case compiled output looks sane:

● Bash(cd /tmp && /home/scott/code/claude/claudec99/build/ccompiler
      /home/scott/code/claude/claudec99/test/valid/test_switch_with_case_1__42.c
       && cat test_switch_with_…)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_switch_wit
     h_case_1__42.c -> test_switch_with_case_1__42.asm
     section .text
     … +37 lines (ctrl+o to expand)
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Generated code looks clean: expression spilled once, linear
  compare-and-branch, per-section labels, cleanup at end.

  Planned Milestone Summary & Transcript

● Write(docs/milestones/stage-10-03-02-summary.md)
  ⎿  Wrote 30 lines to docs/milestones/stage-10-03-02-summary.md
       2 # Milestone Summary — stage-10-03-02
         elsge 10-03-02 is complete. The compiler now supports `case` lab
         nceide a restricted `switch` statement. A switch body is a seque
       5 of one or more `case <integer_literal>:` and/or `default:` secti
       6 The controlling expression is evaluated exactly once and spilled
       7 the stack; a linear compare-and-branch dispatch jumps to the fir
         st
       9 otherwise jumps to the end of the switch. `break` exits the swit
      10 `return` exits the function. Stacked labels, constant expression
         s
     … +22 lines (ctrl+o to expand)

● Write(docs/sessions/stage-10-03-02-transcript.md)
  ⎿  Wrote 140 lines to docs/sessions/stage-10-03-02-transcript.md
       2 # stage-10-03-02 Transcript: Case Labels and Dispatch
       3 ## Summary
       5 Stage 10-03-02 extends the restricted `switch` statement from
       6 stage 10-03-01 to accept `case <integer_literal>:` sections alon
       7 an optional `default:` section. The switch body is now parsed as
          a
          oruence of sections, each terminated by the next section header
       9 the closing brace. The controlling expression is evaluated exact
      10 once and spilled on the stack; a linear compare-and-branch seque
         nce
     … +135 lines (ctrl+o to expand)

● Milestone Summary

  Stage 10-03-02 complete. Restricted switch now supports case 
  <integer_literal>: sections alongside an optional default:. Controlling
  expression is evaluated exactly once (spilled on the stack); linear
  compare-and-branch dispatches to the matching case, falls back to default, or
  jumps to switch-end. All 136 tests pass (113 valid + 13 invalid + 10
  print_ast), no regressions.

  Artifacts written

  - docs/kickoffs/stage-10-03-02-kickoff.md
  - docs/milestones/stage-10-03-02-summary.md
  - docs/sessions/stage-10-03-02-transcript.md
  - docs/grammar.md updated

  Pausing here before committing — ready to commit with the following proposed
  message when you give the go-ahead:

  stage 10-03-02 implementation: case labels and dispatch

  Add case-label support to the restricted switch statement. Controlling
  expression is evaluated exactly once and spilled; linear compare-and-
  branch dispatches to the matching case section, falls back to default
  when present, or to the switch-end label. Break exits the switch.

  - tokenizer: add TOKEN_CASE; lex "case" keyword
  - AST: add AST_CASE_SECTION (value + statements as children)
  - parser: parse a sequence of case/default sections; reject duplicate default
  - codegen: emit spill + compare-and-branch dispatch with per-section labels
  - pretty printer: render CaseSection
  - grammar.md: sync switch_body / switch_section / case_section productions

  Shall I proceed with the commit?

✻ Worked for 5m 46s

❯ yes                                                                           

● Bash(git status && echo "---DIFF---" && git diff --stat)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +24 lines (ctrl+o to expand)

● Bash(git add docs/grammar.md include/ast.h include/token.h
      src/ast_pretty_printer.c src/codegen.c src/lexer.c src/parser.c
      docs/kickoffs/stage-10-03-02-kickoff.md do…)
  ⎿  [master f96b8ad] stage 10-03-02 implementation: case labels and dispat
     ch
      10 files changed, 418 insertions(+), 19 deletions(-)
     … +8 lines (ctrl+o to expand)

● Committed as f96b8ad — 10 files changed, 418 insertions, 19 deletions. Working
   tree clean, one commit ahead of origin/master.