 ▐▛███▜▌   Claude Code v2.1.114
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

❯ /implement-stage ClaudeC99-spec-stage-10-04-goto.md                           

  Searched for 1 pattern, read 5 files, listed 4 directories (ctrl+o to expand)

● STAGE_DISPLAY: Stage-10-04

  Let me set up task tracking and explore the codebase.

  Read 21 files, listed 4 directories (ctrl+o to expand)

● Now I have a full picture. Let me present the Kickoff Summary.

● Kickoff Summary — Stage-10-04: goto Statement

  1. Summary of the spec

  Add C's goto statement and the identifier form of <labeled_statement>. A goto 
  <id>; transfers control to the matching <identifier>: label within the same
  function. Labels must be unique within their function; duplicates and missing
  labels are compile errors. Generated assembly labels must be unique
  per-function across the whole object. No computed goto and no label addresses.

  2. What must change from the previous stage

  Area: Grammar
  Change: Add identifier form to <labeled_statement>. Add "goto" <identifier> 
  ";"
    to <jump_statement>.
  ────────────────────────────────────────
  Area: Tokenizer
  Change: Add TOKEN_GOTO; recognize goto keyword.
  ────────────────────────────────────────
  Area: AST
  Change: Add AST_GOTO_STATEMENT (value=target label name) and
    AST_LABEL_STATEMENT (value=label name, one child: the labeled statement).
  ────────────────────────────────────────
  Area: Parser
  Change: Handle goto <id> ; in parse_statement. Handle <identifier> ":" 
    <statement> via a 1-token lookahead at the start of parse_statement.
  ────────────────────────────────────────
  Area: Code gen
  Change: Per-function unique label prefixing (use function name or per-function

    counter suffix). Collect labels on a pre-pass for a function body; detect
    duplicates; emit .L_usr_<func>_<name>: style labels; emit jmp 
    .L_usr_<func>_<target> for goto. Detect missing targets.
  ────────────────────────────────────────
  Area: Pretty printer
  Change: Add GotoStmt: name, LabelStmt: name output.
  ────────────────────────────────────────
  Area: Tests
  Change: Three valid tests already provided. Add invalid (duplicate label,
    missing label), and a print_ast test.
  ────────────────────────────────────────
  Area: Docs
  Change: Update docs/grammar.md.

  3. Ambiguity / spec issues

  - The spec's grammar fragment has a typo: "<statement is missing the closing
  >. Harmless — intent clear.
  - Labels and goto are strictly function-scoped. The spec says "label names
  must be unique within the same function" — this permits the same label name in
   different functions. Codegen must avoid collisions by per-function prefixing.
  - The spec does not say whether goto can target a case/default label by name.
  Standard C does not — only identifier labels are targets of goto. The grammar
  puts case/default in their own production forms, and the spec only adds the
  <identifier>-form branch, so goto targets only identifier labels.
  Implementation follows that.
  - A label statement applies to the following statement. That statement may be
  any kind (declaration, expression, return, block, nested label…).
  - Identifier-labeled statements can appear anywhere in the function body —
  including inside loops, ifs, blocks. Implementation uses a recursive pre-walk
  (analogous to the switch label pre-walk, but function-scoped).

  4. Implementation plan

  1. Tokenizer — add TOKEN_GOTO and keyword recognition for goto.
  2. AST — add AST_GOTO_STATEMENT and AST_LABEL_STATEMENT node types.
  3. Parser — at the top of parse_statement, detect IDENTIFIER ":" lookahead →
  label statement. Handle goto <id> ;.
  4. Pretty printer — add GotoStmt / LabelStmt cases.
  5. Code generation — add a GotoCtx to CodeGen (holds label table for the
  current function). Pre-walk body to register labels (reject duplicates). Emit
  .L_usr_<func>_<name>: on label statements; jmp .L_usr_<func>_<name> on gotos.
  Error on missing targets.
  6. Tests — run existing tests; add invalid tests for duplicate_label and
  undefined_label; add a print_ast test.
  7. Docs — update docs/grammar.md.
  8. Commit — single commit at the end.

  Planned Changes (files)

  - include/token.h — add TOKEN_GOTO.
  - src/lexer.c — keyword mapping.
  - include/ast.h — add AST_GOTO_STATEMENT, AST_LABEL_STATEMENT.
  - src/parser.c — label-stmt dispatch + goto statement.
  - src/ast_pretty_printer.c — new node printers.
  - include/codegen.h — add GotoCtx state.
  - src/codegen.c — label pre-walk, emission for label/goto, error checks; call
  the pre-walk from codegen_function.
  - docs/grammar.md — reflect grammar updates.
  - test/invalid/test_invalid_13__duplicate_label.c,
  test/invalid/test_invalid_14__undefined_label.c.
  - test/print_ast/test_goto.c + test/print_ast/test_goto.expected.

  Saving the kickoff now.

● Write(docs/kickoffs/stage-10-04-kickoff.md)
  ⎿  Wrote 59 lines to docs/kickoffs/stage-10-04-kickoff.md
       2 # Stage-10-04 Kickoff: goto Statement
       3 ## 1. Summary of the spec
       4 
       5 Stage 10-04 adds C's `goto` statement and the identifier form of
       6 `<labeled_statement>`. A `goto <identifier>;` transfers control 
       7 matching `<identifier>:` label within the same function.
       8 
       9 - Labels must be unique within their function (duplicate = compi
         le error).
      10 - `goto` to a missing label is a compile error.
     … +70 lines (ctrl+o to expand)

● Kickoff saved. Proceeding with implementation.
● Step 1: Tokenizer

● Update(include/token.h)
  ⎿  Added 1 line
      15      TOKEN_SWITCH,
      16      TOKEN_DEFAULT,
      17      TOKEN_CASE,
      18 +    TOKEN_GOTO,                                                
      19      TOKEN_IDENTIFIER,
      20      TOKEN_INT_LITERAL,
      21      TOKEN_LPAREN,

● Update(src/lexer.c)
  ⎿  Added 2 lines
      102              token.type = TOKEN_DEFAULT;
      103          } else if (strcmp(token.value, "case") == 0) {
      104              token.type = TOKEN_CASE;
      105 +        } else if (strcmp(token.value, "goto") == 0) {        
      106 +            token.type = TOKEN_GOTO;                          
      107          } else {
      108              token.type = TOKEN_IDENTIFIER;
      109          }

● Step 2: AST and Parser

● Update(include/ast.h)
  ⎿  Added 2 lines
      18      AST_CASE_SECTION,
      19      AST_BREAK_STATEMENT,
      20      AST_CONTINUE_STATEMENT,
      21 +    AST_GOTO_STATEMENT,                                        
      22 +    AST_LABEL_STATEMENT,                                       
      23      AST_BLOCK,
      24      AST_EXPRESSION_STMT,
      25      AST_DECLARATION,

● Now add parsing. At the top of parse_statement, handle
  identifier-followed-by-colon as a labeled statement, and add the goto branch.
  I'll use a lexer snapshot/restore pattern consistent with the existing
  parse_expression approach.

● Update(src/parser.c)
  ⎿  Added 15 lines
      493   *               | <expression_stmt>
      494   */
      495  static ASTNode *parse_statement(Parser *parser) {
      496 +    /* labeled_statement: <identifier> ":" <statement> */     
      497 +    if (parser->current.type == TOKEN_IDENTIFIER) {           
      498 +        int saved_pos = parser->lexer->pos;                   
      499 +        Token saved_token = parser->current;                  
      500 +        parser->current = lexer_next_token(parser->lexer);    
      501 +        if (parser->current.type == TOKEN_COLON) {            
      502 +            parser->current = lexer_next_token(parser->lexer);
      503 +            ASTNode *node = ast_new(AST_LABEL_STATEMENT, saved
          +_token.value);                                                
      504 +            ast_add_child(node, parse_statement(parser));     
      505 +            return node;                                      
      506 +        }                                                     
      507 +        /* Not a label — restore lexer state */               
      508 +        parser->lexer->pos = saved_pos;                       
      509 +        parser->current = saved_token;                        
      510 +    }                                                         
      511      /* declaration: "int" <identifier> [ "=" <expression> ] ";
           " */
      512      if (parser->current.type == TOKEN_INT) {
      513          parser->current = lexer_next_token(parser->lexer);

● Update(src/parser.c)
  ⎿  Added 6 lines
      589          parser_expect(parser, TOKEN_SEMICOLON);
      590          return ast_new(AST_CONTINUE_STATEMENT, NULL);
      591      }
      592 +    if (parser->current.type == TOKEN_GOTO) {                 
      593 +        parser->current = lexer_next_token(parser->lexer);    
      594 +        Token name = parser_expect(parser, TOKEN_IDENTIFIER); 
      595 +        parser_expect(parser, TOKEN_SEMICOLON);               
      596 +        return ast_new(AST_GOTO_STATEMENT, name.value);       
      597 +    }                                                         
      598      /* expression_stmt (includes assignments, since assignment
            is now an expression) */
      599      ASTNode *expr = parse_expression(parser);
      600      parser_expect(parser, TOKEN_SEMICOLON);

● Update the pretty printer.

● Update(src/ast_pretty_printer.c)
  ⎿  Added 6 lines
      108      case AST_CONTINUE_STATEMENT:
      109          printf("ContinueStmt\n");
      110          break;
      111 +    case AST_GOTO_STATEMENT:                                  
      112 +        printf("GotoStmt: %s\n", node->value);                
      113 +        break;                                                
      114 +    case AST_LABEL_STATEMENT:                                 
      115 +        printf("LabelStmt: %s\n", node->value);               
      116 +        break;                                                
      117      case AST_PREFIX_INC_DEC:
      118          printf("PrefixIncDec: %s\n", operator_name(node->value
           ));
      119          break;

● Step 3: Code Generation

  Adding a per-function user-label table. I'll store the current function name
  on CodeGen so label emission can prefix it, plus a table of user label names
  for duplicate detection.

● Update(include/codegen.h)
  ⎿  Added 1 line
       8  #define MAX_BREAK_DEPTH 32
       9  #define MAX_SWITCH_DEPTH 16
      10  #define MAX_SWITCH_LABELS 64
      11 +#define MAX_USER_LABELS 64                                     
      12  
      13  typedef struct {
      14      char name[256];

● Update(include/codegen.h)
  ⎿  Added 7 lines
      49      int break_depth;
      50      SwitchCtx switch_stack[MAX_SWITCH_DEPTH];
      51      int switch_depth;
      52 +    /* Per-function user label table (populated by a pre-walk b
         +efore                                                          
      53 +     * body emission; used to reject duplicates and missing got
         +o                                                              
      54 +     * targets). Assembly names are prefixed by `current_func` 
         +so                                                             
      55 +     * reused label names in different functions never collide.
         + */                                                            
      56 +    char user_labels[MAX_USER_LABELS][256];                    
      57 +    int user_label_count;                                      
      58 +    const char *current_func;                                  
      59  } CodeGen;
      60  
      61  void codegen_init(CodeGen *cg, FILE *output);

● Now update codegen.c: init, pre-walk, label-statement emission, goto emission,
   and hook into function emission.

● Update(src/codegen.c)
  ⎿  Added 39 lines
      13      cg->has_frame = 0;
      14      cg->break_depth = 0;
      15      cg->switch_depth = 0;
      16 +    cg->user_label_count = 0;                                  
      17 +    cg->current_func = NULL;                                   
      18  }
      19  
      20  /*
      21 + * Walk a function body collecting every AST_LABEL_STATEMENT no
         +de's                                                           
      22 + * name into the per-function user-label table. Duplicate label
         +s                                                              
      23 + * within the same function are rejected here; `goto` resolutio
         +n                                                              
      24 + * against the completed table happens during emission.        
      25 + */                                                            
      26 +static void collect_user_labels(CodeGen *cg, ASTNode *node) {  
      27 +    if (!node) return;                                         
      28 +    if (node->type == AST_LABEL_STATEMENT) {                   
      29 +        for (int i = 0; i < cg->user_label_count; i++) {       
      30 +            if (strcmp(cg->user_labels[i], node->value) == 0) {
      31 +                fprintf(stderr, "error: duplicate label '%s' in
         + function '%s'\n",                                             
      32 +                        node->value, cg->current_func);        
      33 +                exit(1);                                       
      34 +            }                                                  
      35 +        }                                                      
      36 +        if (cg->user_label_count >= MAX_USER_LABELS) {         
      37 +            fprintf(stderr, "error: too many labels in function
         + '%s' (max %d)\n",                                             
      38 +                    cg->current_func, MAX_USER_LABELS);        
      39 +            exit(1);                                           
      40 +        }                                                      
      41 +        strncpy(cg->user_labels[cg->user_label_count], node->va
         +lue, 255);                                                     
      42 +        cg->user_labels[cg->user_label_count][255] = '\0';     
      43 +        cg->user_label_count++;                                
      44 +    }                                                          
      45 +    for (int i = 0; i < node->child_count; i++) {              
      46 +        collect_user_labels(cg, node->children[i]);            
      47 +    }                                                          
      48 +}                                                              
      49 +                                                               
      50 +static int user_label_defined(CodeGen *cg, const char *name) { 
      51 +    for (int i = 0; i < cg->user_label_count; i++) {           
      52 +        if (strcmp(cg->user_labels[i], name) == 0) return 1;   
      53 +    }                                                          
      54 +    return 0;                                                  
      55 +}                                                              
      56 +                                                               
      57 +/*                                                             
      58   * Walk the subtree of a switch body, collecting every AST_CASE
          _SECTION
      59   * and AST_DEFAULT_SECTION node pointer into `ctx` and assignin
          g each a
      60   * fresh label. Descent stops at nested AST_SWITCH_STATEMENT no
          des

● Now add emission for label/goto and hook into function prologue.

● Update(src/codegen.c)
  ⎿  Added 13 lines
      483          if (node->child_count > 0) {
      484              codegen_statement(cg, node->children[0], is_main);
      485          }
      486 +    } else if (node->type == AST_LABEL_STATEMENT) {           
      487 +        fprintf(cg->output, ".L_usr_%s_%s:\n", cg->current_fun
          +c, node->value);                                              
      488 +        if (node->child_count > 0) {                          
      489 +            codegen_statement(cg, node->children[0], is_main);
      490 +        }                                                     
      491 +    } else if (node->type == AST_GOTO_STATEMENT) {            
      492 +        if (!user_label_defined(cg, node->value)) {           
      493 +            fprintf(stderr, "error: undefined label '%s' in fu
          +nction '%s'\n",                                               
      494 +                    node->value, cg->current_func);           
      495 +            exit(1);                                          
      496 +        }                                                     
      497 +        fprintf(cg->output, "    jmp .L_usr_%s_%s\n",         
      498 +                cg->current_func, node->value);               
      499      } else if (node->type == AST_BREAK_STATEMENT) {
      500          int id = cg->break_stack[cg->break_depth - 1].break_la
           bel;
      501          fprintf(cg->output, "    jmp .L_break_%d\n", id);

● Now wire into codegen_function: set current_func, run collect_user_labels on
  the body before emission.

● Update(src/codegen.c)
  ⎿  Added 5 lines
      554          cg->stack_offset = 0;
      555          cg->scope_start = 0;
      556          cg->push_depth = 0;
      557 +        cg->user_label_count = 0;                             
      558 +        cg->current_func = node->value;                       
      559  
      560 +        /* Pre-walk the body to collect user labels; rejects d
          +uplicates. */                                                 
      561 +        collect_user_labels(cg, body);                        
      562 +                                                              
      563          /* Compute stack space: one slot per parameter plus ea
           ch declaration in the body. */
      564          int num_vars = count_declarations(body);
      565          int total_slots = num_params + num_vars;

● Now build and run existing tests.
● Bash(cd /home/scott/code/claude/claudec99/build && cmake --build . 2>&1 | tail
       -30)
  ⎿  Consolidate compiler generated dependencies of target ccompiler
     [ 12%] Building C object CMakeFiles/ccompiler.dir/src/compiler.c.o
     [ 25%] Building C object CMakeFiles/ccompiler.dir/src/ast.c.o
     … +7 lines (ctrl+o to expand)

● Build clean. Run all test suites.

● Bash(cd /home/scott/code/claude/claudec99 && ./test/valid/run_tests.sh 2>&1 |
      tail -10)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_while_fals
     e_initially__42.c -> test_while_false_initially__42.asm
     PASS  test_while_false_initially__42  (exit code: 42)
     … +11 lines (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99 && ./test/invalid/run_tests.sh 2>&1
      | tail -5)
  ⎿  PASS  test_invalid_7__call_to_undefined_function  (error contains: 'ca
     ll to undefined function')
     PASS  test_invalid_8__expects_2_arguments  (error contains: 'expects 2
     … +5 lines (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99 && ./test/print_ast/run_tests.sh
      2>&1 | tail -5)
  ⎿  >                 IntLiteral: 42
     PASS  test_variable_declaration
     PASS  test_while_loop
     … +2 lines (ctrl+o to expand)

● Bash(./test/print_ast/run_tests.sh 2>&1 | grep -E 'FAIL|---' | head -50)
  ⎿  FAIL  test_switch  (output mismatch)
       --- expected ---
       --- actual ---
     … +2 lines (ctrl+o to expand)

● Bash(./build/ccompiler --print-ast test/print_ast/test_switch.c)
  ⎿  TranslationUnit:
       FunctionDecl: main
         Block
     … +12 lines (ctrl+o to expand)

● Bash(git status; git stash; ./test/print_ast/run_tests.sh 2>&1 | grep FAIL |
      head -5; git stash pop)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +23 lines (ctrl+o to expand)

● Pre-existing failure on master — unrelated to this stage. Rebuilding since
  stash re-triggered it.

● Bash(cd /home/scott/code/claude/claudec99/build && cmake --build . 2>&1 | tail
       -3)
  ⎿  [ 75%] Building C object CMakeFiles/ccompiler.dir/src/parser.c.o
     [ 87%] Linking C executable ccompiler
     [100%] Built target ccompiler

● Now run the three provided valid goto tests (they're part of the 121 passing
  already, but let me confirm explicitly):

● Bash(cd /home/scott/code/claude/claudec99 && ./test/valid/run_tests.sh 2>&1 |
      grep -i goto)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_goto_backw
     ard_jump__3.c -> test_goto_backward_jump__3.asm
     PASS  test_goto_backward_jump__3  (exit code: 3)
     … +6 lines (ctrl+o to expand)

● All three goto valid tests pass. Now add invalid + print_ast tests.

● Step 4: Add new tests

● Write(test/invalid/test_invalid_13__duplicate_label.c)
  ⎿  Wrote 7 lines to test/invalid/test_invalid_13__duplicate_label.c
      1 int main() {
      2     int a = 0;
      3 done:
      4     a = 1;
      5 done:
      6     return a;
      7 }

● Write(test/invalid/test_invalid_14__undefined_label.c)
  ⎿  Wrote 4 lines to test/invalid/test_invalid_14__undefined_label.c
      1 int main() {
      2     goto nowhere;
      3     return 0;
      4 }

● Write(test/print_ast/test_goto.c)
  ⎿  Wrote 9 lines to test/print_ast/test_goto.c
      1 int main() {
      2     int a = 0;
      3 start:
      4     a = a + 1;
      5     if (a == 3) goto done;
      6     goto start;
      7 done:
      8     return a;
      9 }

● Generate the expected AST file from the compiler so it matches exactly, then
  verify it's correct.

● Bash(./build/ccompiler --print-ast test/print_ast/test_goto.c)
  ⎿  TranslationUnit:
       FunctionDecl: main
         Block
     … +17 lines (ctrl+o to expand)

● Output matches the spec's intent. Save as expected.

● Write(test/print_ast/test_goto.expected)
  ⎿  Wrote 20 lines to test/print_ast/test_goto.expected
       1 TranslationUnit:
       2   FunctionDecl: main
       3     Block
       4       VariableDeclaration: a
       5         IntLiteral: 0
       6       LabelStmt: start
       7         ExpressionStatement
       8           Assignment: a
       9             Binary: ADD
      10               VariableExpression: a
     … +10 lines (ctrl+o to expand)
● Bash(./test/invalid/run_tests.sh 2>&1 | tail -6; echo '===';
      ./test/print_ast/run_tests.sh 2>&1 | tail -4)
  ⎿  PASS  test_invalid_6__duplicate_declaration  (error contains: 'duplica
     te declaration')
     PASS  test_invalid_7__call_to_undefined_function  (error contains: 'ca
     … +12 lines (ctrl+o to expand)

● New goto tests all pass. Only the pre-existing test_switch print_ast failure
  remains, which is unrelated. Full counts: 121 valid (+3 new-for-stage-10-04
  included), 14 invalid (+2 new), 10 print_ast (+1 new). Only pre-existing
  regression.

● Step 5: Update grammar.md

● Update(docs/grammar.md)
  ⎿  Added 3 lines, removed 1 line
      39  
      40  <switch_statement> ::= "switch" "(" <expression> ")" <statement
          >
      41  
      42 -<labeled_statement> ::= "case" <constant_expression> ":" <state
         -ment>                                                          
      42 +<labeled_statement> ::= <identifier> ":" <statement>           
      43 +                      | "case" <constant_expression> ":" <state
         +ment>                                                          
      44                        | "default" ":" <statement>
      45  
      46  <constant_expression> ::= <integer_literal>
      47  
      48  <jump_statement> ::= "continue" ";"
      49                      | "break" ";"
      50 +                    | "goto" <identifier> ";"                  
      51  
      52  <expression_statement> ::= <expression> ";"
      53  

● Step 6: Write milestone + transcript

● Write(docs/milestones/stage-10-04-summary.md)
  ⎿  Wrote 26 lines to docs/milestones/stage-10-04-summary.md
       2 # Stage-10-04 Milestone: goto Statement
       3 Stage 10-04 adds support for C's `goto` statement and the identi
         fier
         control `<labeled_statement>`. A `goto <identifier>;` transfers 
       5 to the matching `<identifier>:` label within the same function.
       6 
       8 ## Completed
       9 - Tokenizer recognizes the `goto` keyword (`TOKEN_GOTO`).
      10 - Parser accepts labeled statements of the form `<identifier> ":
         " <statement>`
     … +19 lines (ctrl+o to expand)

● Write(docs/sessions/stage-10-04-transcript.md)
  ⎿  Wrote 106 lines to docs/sessions/stage-10-04-transcript.md
       2 # stage-10-04 Transcript: goto Statement
       3 ## Summary
       5 Stage 10-04 adds the C `goto` statement and the identifier form 
         of
         ifier>;`d_statement>` to the Claude C99 compiler. A `goto <ident
       7 transfers control to the matching `<identifier>:` label within t
       8 enclosing function. Label names must be unique within a function
         ;
          atlicate labels and references to undefined labels are rejected
      10 compile time.
     … +104 lines (ctrl+o to expand)

● Milestone Summary — Stage-10-04

  Stage 10-04 adds the goto statement and identifier-form labeled statements to
  Claude C99.

  Completed:
  - Tokenizer: TOKEN_GOTO and keyword recognition.
  - AST: AST_GOTO_STATEMENT, AST_LABEL_STATEMENT.
  - Parser: label-statement lookahead; goto <id> ;.
  - Pretty printer: LabelStmt, GotoStmt cases.
  - Code gen: per-function label collection pre-walk, duplicate + undefined
  detection, unique assembly labels via .L_usr_<func>_<name> prefixing.
  - Docs: docs/grammar.md updated.
  - Tests: 3 provided valid tests pass; added 2 invalid tests
  (duplicate/undefined label) and 1 print_ast test.

  Test results: 121/121 valid, 14/14 invalid, 10/11 print_ast (the one failure
  is a pre-existing test_switch regression on master — unrelated). No
  regressions introduced by this stage.

  Artifacts saved:
  - docs/kickoffs/stage-10-04-kickoff.md
  - docs/milestones/stage-10-04-summary.md
  - docs/sessions/stage-10-04-transcript.md

  The implementation is complete. Ready to commit when you want — want me to
  proceed with a commit, or leave it for your review?

✻ Sautéed for 5m 17s

❯ /export docs/sessions-test-03-export.md                                       
  ⎿  Conversation exported to:
     /home/scott/code/claude/claudec99/docs/sessions-test-03-export.md

❯ explain the failing print_ast test?                                           

● The failing test is test/print_ast/test_switch — a pre-existing issue on
  master unrelated to stage 10-04.

  Source (test_switch.c):
  int main() {
      int x = 0;
      switch (1) {
          default:
              x = 42;
              break;
      }
      return x;
  }

  Expected (test_switch.expected):
  SwitchStmt:
    IntLiteral: 1
    DefaultSection:
      ExpressionStatement ...

  Actual:
  SwitchStmt:
    IntLiteral: 1
    Block
      DefaultSection:
        ExpressionStatement ...

  Root cause: stage 10-03-03 changed the switch grammar so the body is any
  <statement> — here it's a block { default: ... }. The parser now correctly
  wraps the body in an AST_BLOCK, and the pretty printer emits a Block line
  accordingly. The .expected file was never regenerated for that grammar change,
   so it still matches the pre-10-03-03 shape where DefaultSection was a direct
  child of SwitchStmt.

  Fix (outside stage 10-04 scope): regenerate the .expected to include the Block
   layer — the actual output is the correct one under the current grammar.

  Want me to patch the .expected file? It's a one-line fix but it's a
  stage-10-03-03 cleanup, not a 10-04 change.

❯ please fix the .expected file so the test passes properly                     

  Read 1 file (ctrl+o to expand)

● Bash(./build/ccompiler --print-ast test/print_ast/test_switch.c)
  ⎿  TranslationUnit:
       FunctionDecl: main
         Block
     … +12 lines (ctrl+o to expand)

● Write(test/print_ast/test_switch.expected)
  ⎿  Added 5 lines, removed 4 lines
       5          IntLiteral: 0
       6        SwitchStmt:
       7          IntLiteral: 1
       8 -        DefaultSection:                                        
       9 -          ExpressionStatement                                  
      10 -            Assignment: x                                      
      11 -              IntLiteral: 42                                   
       8 +        Block                                                  
       9 +          DefaultSection:                                      
      10 +            ExpressionStatement                                
      11 +              Assignment: x                                    
      12 +                IntLiteral: 42                                 
      13            BreakStmt
      14        ReturnStmt:
      15          VariableExpression: x

● Bash(./test/print_ast/run_tests.sh 2>&1 | tail -4)
  ⎿  PASS  test_variable_declaration
     PASS  test_while_loop

     Results: 11 passed, 0 failed, 11 total

● All 11 print_ast tests pass now. Full suite status: 121/121 valid, 14/14 
  invalid, 11/11 print_ast — clean.