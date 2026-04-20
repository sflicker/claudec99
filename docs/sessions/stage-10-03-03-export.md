 ▐▛███▜▌   Claude Code v2.1.114
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

❯ /implement-stage ClaudeC99-spec-stage-10-03-03-standard-switch-support.md     

  Searched for 1 pattern, read 5 files, listed 1 directory (ctrl+o to expand)

● STAGE_DISPLAY: Stage-10-03-03

  Let me explore the codebase structure.

  Listed 2 directories (ctrl+o to expand)

  Read 19 files, listed 7 directories (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99/build && ls)
  ⎿  ccompiler
     CMakeCache.txt
     CMakeFiles
     … +7 lines (ctrl+o to expand)

● Bash(cmake --build . 2>&1 | tail -20 && echo "---" && cd
      /home/scott/code/claude/claudec99/test/valid && bash run_tests.sh 2>&1 |
      tail -30)
  ⎿  Consolidate compiler generated dependencies of target ccompiler
     [100%] Built target ccompiler
     ---
     … +34 lines (ctrl+o to expand)

● Bash(bash run_tests.sh 2>&1 | grep -E "fallthrough|switch")
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_switch_bre
     ak__42.c -> test_switch_break__42.asm
     PASS  test_switch_break__42  (exit code: 42)
     … +34 lines (ctrl+o to expand)

● STAGE_DISPLAY: Stage-10-03-03

  All 116 existing tests pass. Stacked-case tests happen to pass already because
   the stage 10-03-02 parser permits empty case sections, and sections are
  emitted sequentially in assembly so fallthrough works implicitly when a
  section has no statements. However, true "unlabeled statements inside a switch
   body" does not currently parse — that is the real change required by this
  stage.

  Now let me create the Kickoff Summary and pause for confirmation.

  Kickoff Summary

  1. Summary of the spec

  Stage 10-03-03 moves switch from the restricted stage 10-03-02 form (switch 
  (...) { case...: {stmts}  default: {stmts} }) to a standard C-shape where the
  switch body is any statement, and case <integer_literal> : and default :
  become <labeled_statement>s — regular statements that may appear within the
  body (typically a block).

  Behavior under the new grammar:

  - The controlling expression is evaluated exactly once.
  - Control transfers to the first matching case label; if none matches, to
  default if present, otherwise past the switch.
  - After entering a labeled statement, execution continues sequentially through
   subsequent statements until break, return, or end of switch (fallthrough).
  - Stacked case labels may apply to a single statement.
  - Unlabeled statements may appear anywhere in the switch body (they are
  reached only via fallthrough, never by dispatch).
  - case expressions remain restricted to <integer_literal>; <int_literal> is
  renamed to <integer_literal> in the grammar.

  2. What must change from the previous stage

  Area: Grammar
  Change: Remove <switch_body>, <switch_section>, <case_section>,
    <default_section>. Redefine <switch_statement> ::= "switch" "(" <expression>
   
    ")" <statement>. Add <labeled_statement> ::= "case" <constant_expression> 
  ":"
     <statement> | "default" ":" <statement>. Add <constant_expression> ::= 
    <integer_literal>. Rename <int_literal> → <integer_literal>.
  ────────────────────────────────────────
  Area: Tokenizer
  Change: No changes.
  ────────────────────────────────────────
  Area: Parser
  Change: parse_switch_statement becomes switch (expr) <statement>. Top-level
    parse_statement handles case/default as labeled statements guarded by
    switch_depth > 0.
  ────────────────────────────────────────
  Area: AST
  Change: Keep names AST_CASE_SECTION/AST_DEFAULT_SECTION (per spec's allowance
    for internal representation). Semantic change: a case node has children
    [int_literal, inner_statement] and a default node has children
    [inner_statement].
  ────────────────────────────────────────
  Area: Code generation
  Change: Pre-walk switch body (not descending into nested switches) to collect
    case/default node pointers and assign labels. Emit dispatch
    (compare-and-branch) before the body. Emit the body as ordinary statements;
    when a case/default node is visited during emission, output its pre-assigned

    label. Keep the switch-end + break label.
  ────────────────────────────────────────
  Area: Pretty printer
  Change: Adjust AST_CASE_SECTION / AST_DEFAULT_SECTION to print a single inner
    statement instead of an explicit list.
  ────────────────────────────────────────
  Area: Tests
  Change: Existing fallthrough/stacked-case tests already exist and must still
    pass. Add a test exercising an unlabeled statement inside a switch body (the

    feature newly enabled by this stage).
  ────────────────────────────────────────
  Area: Docs
  Change: Update docs/grammar.md.

  3. Ambiguity / grammar / spec issues

  - Minor spec wording: "stacked case labels" is demonstrated only by example
  elsewhere; the new grammar makes them fall out naturally (labeled_statement
  can nest because its body is <statement>, which can itself be a
  <labeled_statement>). No action needed.
  - The spec renames <int_literal> to <integer_literal> — this is purely a
  grammar-document change; the token/name TOKEN_INT_LITERAL / AST_INT_LITERAL
  can stay, per the note that the compiler may use a different internal
  representation.
  - The spec lists <constant_expression> ::= <integer_literal> — we still
  restrict case labels to integer literals, so this will remain parsed as
  TOKEN_INT_LITERAL directly, no new AST node needed.
  - Nesting of case/default inside blocks, if/else, or loops within the same
  switch: the standard-C grammar implied by the spec allows this. I'll implement
   the pre-walk to descend through inner statements and stop at nested switch
  bodies. This matches standard semantics without additional scope/spec risk.

  4. Implementation plan

  1. Parser — rewrite parse_switch_statement; add case/default handling in
  parse_statement with switch_depth guard.
  2. AST — no struct changes; update the semantic shape of
  AST_CASE_SECTION/AST_DEFAULT_SECTION (one inner statement + case literal).
  3. Pretty printer — update those two cases to match the new shape.
  4. Code generation — add a SwitchCtx stack to CodeGen; implement pre-walk
  label collection; rework dispatch emission; handle
  AST_CASE_SECTION/AST_DEFAULT_SECTION during statement emission (emit label,
  recurse into inner statement).
  5. Tests — add a positive test for unlabeled statements in a switch body;
  verify all 116 existing tests continue to pass.
  6. Docs — update docs/grammar.md with the new rules.
  7. Commit — single commit for the stage.

  Planned Changes

  - src/parser.c — rewrite switch parsing; add labeled-statement branch inside
  parse_statement.
  - src/codegen.c — add switch-context stack + pre-walk label collection; rework
   AST_SWITCH_STATEMENT; add AST_CASE_SECTION/AST_DEFAULT_SECTION handling in
  codegen_statement.
  - include/codegen.h — add SwitchCtx type and stack field on CodeGen.
  - src/ast_pretty_printer.c — adjust AST_CASE_SECTION/AST_DEFAULT_SECTION
  printing.
  - docs/grammar.md — sync grammar.
  - test/valid/test_switch_unlabeled_stmt_in_body__42.c (new) — exercise
  newly-legal unlabeled statements in switch body.
  - docs/kickoffs/stage-10-03-03-kickoff.md — save kickoff.
  - docs/milestones/stage-10-03-03-summary.md — save milestone.
  - docs/sessions/stage-10-03-03-transcript.md — save transcript.

  Let me save the kickoff file and pause for your approval of the plan before
  beginning implementation.

● Write(docs/kickoffs/stage-10-03-03-kickoff.md)
  ⎿  Wrote 66 lines to docs/kickoffs/stage-10-03-03-kickoff.md
       2 # Stage-10-03-03 Kickoff: Standard Switch Support
       3 ## 1. Summary of the spec
       4 
          form 10-03-03 moves `switch` from the restricted stage 10-03-02
       6 (`switch (...) { case ...: {stmts}  default: {stmts} }`) to a
       7 standard C-shape where the switch body is any statement and
         ent>`s<integer_literal> :` / `default :` become `<labeled_statem
       9 — regular statements that may appear within the body (typically 
         a
      10 block).
     … +74 lines (ctrl+o to expand)

● Kickoff saved. Pausing here for confirmation before starting implementation.
  Shall I proceed with the parser changes first?

✻ Cogitated for 4m 0s

❯ continue                                                                      

● Proceeding with parser changes first.

  Step 1: Parser

● Update(src/parser.c)
  ⎿  Added 8 lines, removed 44 lines
      454  }
      455  
      456  /*
      457 - * <switch_statement> ::= "switch" "(" <expression> ")" <switc
          -h_body>                                                       
      458 - * <switch_body>      ::= "{" <switch_section> { <switch_secti
          -on> } "}"                                                     
      459 - * <switch_section>   ::= <case_section> | <default_section>  
      460 - * <case_section>     ::= "case" <integer_literal> ":" { <stat
          -ement> }                                                      
      461 - * <default_section>  ::= "default" ":" { <statement> }       
      457 + * <switch_statement>   ::= "switch" "(" <expression> ")" <sta
          +tement>                                                       
      458 + * <labeled_statement>  ::= "case" <integer_literal> ":" <stat
          +ement>                                                        
      459 + *                        | "default" ":" <statement>         
      460   *
      463 - * Stage 10-03-02: the switch body is a sequence of case/defau
          -lt                                                            
      464 - * sections. At most one default is allowed. Case values must 
          -be                                                            
      465 - * integer literals. Statements within a section are consumed 
          -until                                                         
      466 - * the next `case`, `default`, or closing `}`.                
      461 + * Stage 10-03-03: the switch body is any statement. `case` an
          +d                                                             
      462 + * `default` labels are parsed as labeled statements inside   
      463 + * parse_statement and are only legal while switch_depth > 0. 
      464   */
      465  static ASTNode *parse_switch_statement(Parser *parser) {
      466      parser_expect(parser, TOKEN_SWITCH);
      467      parser_expect(parser, TOKEN_LPAREN);
      468      ASTNode *expr = parse_expression(parser);
      469      parser_expect(parser, TOKEN_RPAREN);
      473 -    parser_expect(parser, TOKEN_LBRACE);                      
      470  
      471      ASTNode *switch_node = ast_new(AST_SWITCH_STATEMENT, NULL)
           ;
      472      ast_add_child(switch_node, expr);
      473  
      474      parser->switch_depth++;
      479 -    int default_seen = 0;                                     
      480 -    while (parser->current.type != TOKEN_RBRACE) {            
      481 -        if (parser->current.type == TOKEN_CASE) {             
      482 -            parser_expect(parser, TOKEN_CASE);                
      483 -            Token value = parser_expect(parser, TOKEN_INT_LITE
          -RAL);                                                         
      484 -            parser_expect(parser, TOKEN_COLON);               
      485 -            ASTNode *case_section = ast_new(AST_CASE_SECTION, 
          -NULL);                                                        
      486 -            ast_add_child(case_section, ast_new(AST_INT_LITERA
          -L, value.value));                                             
      487 -            while (parser->current.type != TOKEN_CASE &&      
      488 -                   parser->current.type != TOKEN_DEFAULT &&   
      489 -                   parser->current.type != TOKEN_RBRACE) {    
      490 -                ast_add_child(case_section, parse_statement(pa
          -rser));                                                       
      491 -            }                                                 
      492 -            ast_add_child(switch_node, case_section);         
      493 -        } else if (parser->current.type == TOKEN_DEFAULT) {   
      494 -            if (default_seen) {                               
      495 -                fprintf(stderr, "error: duplicate default sect
          -ion in switch\n");                                            
      496 -                exit(1);                                      
      497 -            }                                                 
      498 -            default_seen = 1;                                 
      499 -            parser_expect(parser, TOKEN_DEFAULT);             
      500 -            parser_expect(parser, TOKEN_COLON);               
      501 -            ASTNode *default_section = ast_new(AST_DEFAULT_SEC
          -TION, NULL);                                                  
      502 -            while (parser->current.type != TOKEN_CASE &&      
      503 -                   parser->current.type != TOKEN_DEFAULT &&   
      504 -                   parser->current.type != TOKEN_RBRACE) {    
      505 -                ast_add_child(default_section, parse_statement
          -(parser));                                                    
      506 -            }                                                 
      507 -            ast_add_child(switch_node, default_section);      
      508 -        } else {                                              
      509 -            parser_expect(parser, TOKEN_CASE); /* reports "exp
          -ected token type" */                                          
      510 -        }                                                     
      511 -    }                                                         
      475 +    ASTNode *body = parse_statement(parser);                  
      476      parser->switch_depth--;
      477  
      514 -    parser_expect(parser, TOKEN_RBRACE);                      
      478 +    ast_add_child(switch_node, body);                         
      479  
      480      return switch_node;
      481  }

● Now add case/default labeled-statement handling inside parse_statement.

● Update(src/parser.c)
  ⎿  Added 24 lines
      529      if (parser->current.type == TOKEN_SWITCH) {
      530          return parse_switch_statement(parser);
      531      }
      532 +    if (parser->current.type == TOKEN_CASE) {                 
      533 +        if (parser->switch_depth == 0) {                      
      534 +            fprintf(stderr, "error: 'case' label outside of sw
          +itch\n");                                                     
      535 +            exit(1);                                          
      536 +        }                                                     
      537 +        parser->current = lexer_next_token(parser->lexer);    
      538 +        Token value = parser_expect(parser, TOKEN_INT_LITERAL)
          +;                                                             
      539 +        parser_expect(parser, TOKEN_COLON);                   
      540 +        ASTNode *node = ast_new(AST_CASE_SECTION, NULL);      
      541 +        ast_add_child(node, ast_new(AST_INT_LITERAL, value.val
          +ue));                                                         
      542 +        ast_add_child(node, parse_statement(parser));         
      543 +        return node;                                          
      544 +    }                                                         
      545 +    if (parser->current.type == TOKEN_DEFAULT) {              
      546 +        if (parser->switch_depth == 0) {                      
      547 +            fprintf(stderr, "error: 'default' label outside of
          + switch\n");                                                  
      548 +            exit(1);                                          
      549 +        }                                                     
      550 +        parser->current = lexer_next_token(parser->lexer);    
      551 +        parser_expect(parser, TOKEN_COLON);                   
      552 +        ASTNode *node = ast_new(AST_DEFAULT_SECTION, NULL);   
      553 +        ast_add_child(node, parse_statement(parser));         
      554 +        return node;                                          
      555 +    }                                                         
      556      if (parser->current.type == TOKEN_LBRACE) {
      557          return parse_block(parser);
      558      }

● Step 2: Pretty printer — match new shape

● Update(src/ast_pretty_printer.c)
  ⎿  Added 6 lines, removed 3 lines
       92          break;
       93      case AST_DEFAULT_SECTION:
       94          printf("DefaultSection:\n");
       95 -        break;                                                
       95 +        if (node->child_count > 0) {                          
       96 +            ast_pretty_print(node->children[0], depth + 1);   
       97 +        }                                                     
       98 +        return;                                               
       99      case AST_CASE_SECTION:
      100          printf("CaseSection: %s\n", node->children[0]->value);
       98 -        for (int i = 1; i < node->child_count; i++) {         
       99 -            ast_pretty_print(node->children[i], depth + 1);   
      101 +        if (node->child_count > 1) {                          
      102 +            ast_pretty_print(node->children[1], depth + 1);   
      103          }
      104          return;
      105      case AST_BREAK_STATEMENT:

● Step 3: Code generation — add switch context and rework emission

● Update(include/codegen.h)  ⎿  Added 17 lines
       6  
       7  #define MAX_LOCALS 64
       8  #define MAX_BREAK_DEPTH 32
       9 +#define MAX_SWITCH_DEPTH 16                                    
      10 +#define MAX_SWITCH_LABELS 64                                   
      11  
      12  typedef struct {
      13      char name[256];
     ...
      22      int continue_label;
      23  } BreakFrame;
      24  
      25 +/* Per-switch context. Populated by a pre-walk of the switch bo
         +dy                                                             
      26 + * that collects every case/default node pointer (stopping at n
         +ested                                                          
      27 + * switches) and assigns each a unique label id. During body em
         +ission                                                         
      28 + * a case/default node is matched against this table to emit it
         +s                                                              
      29 + * pre-assigned label. */                                      
      30 +typedef struct ASTNode ASTNode;                                
      31  typedef struct {
      32 +    ASTNode *nodes[MAX_SWITCH_LABELS];                         
      33 +    int labels[MAX_SWITCH_LABELS];                             
      34 +    int count;                                                 
      35 +    int default_label;                                         
      36 +} SwitchCtx;                                                   
      37 +                                                               
      38 +typedef struct {                                               
      39      FILE *output;
      40      int label_count;
      41      LocalVar locals[MAX_LOCALS];
     ...
      46      int has_frame;
      47      BreakFrame break_stack[MAX_BREAK_DEPTH];
      48      int break_depth;
      49 +    SwitchCtx switch_stack[MAX_SWITCH_DEPTH];                  
      50 +    int switch_depth;                                          
      51  } CodeGen;
      52  
      53  void codegen_init(CodeGen *cg, FILE *output);

● Update(src/codegen.c)
  ⎿  Added 55 lines
      12      cg->push_depth = 0;
      13      cg->has_frame = 0;
      14      cg->break_depth = 0;
      15 +    cg->switch_depth = 0;                                      
      16  }
      17  
      18 +/*                                                             
      19 + * Walk the subtree of a switch body, collecting every AST_CASE
         +_SECTION                                                       
      20 + * and AST_DEFAULT_SECTION node pointer into `ctx` and assignin
         +g each a                                                       
      21 + * fresh label. Descent stops at nested AST_SWITCH_STATEMENT no
         +des                                                            
      22 + * because labels inside a nested switch belong to that inner s
         +witch.                                                         
      23 + * A case node's case-value child (children[0]) is not descende
         +d into.                                                        
      24 + */                                                            
      25 +static void collect_switch_labels(CodeGen *cg, ASTNode *node, S
         +witchCtx *ctx) {                                               
      26 +    if (!node) return;                                         
      27 +    if (node->type == AST_CASE_SECTION) {                      
      28 +        if (ctx->count >= MAX_SWITCH_LABELS) {                 
      29 +            fprintf(stderr, "error: too many case/default label
         +s in switch (max %d)\n",                                       
      30 +                    MAX_SWITCH_LABELS);                        
      31 +            exit(1);                                           
      32 +        }                                                      
      33 +        ctx->nodes[ctx->count] = node;                         
      34 +        ctx->labels[ctx->count] = cg->label_count++;           
      35 +        ctx->count++;                                          
      36 +        if (node->child_count > 1) {                           
      37 +            collect_switch_labels(cg, node->children[1], ctx); 
      38 +        }                                                      
      39 +        return;                                                
      40 +    }                                                          
      41 +    if (node->type == AST_DEFAULT_SECTION) {                   
      42 +        if (ctx->count >= MAX_SWITCH_LABELS) {                 
      43 +            fprintf(stderr, "error: too many case/default label
         +s in switch (max %d)\n",                                       
      44 +                    MAX_SWITCH_LABELS);                        
      45 +            exit(1);                                           
      46 +        }                                                      
      47 +        int lbl = cg->label_count++;                           
      48 +        ctx->nodes[ctx->count] = node;                         
      49 +        ctx->labels[ctx->count] = lbl;                         
      50 +        ctx->count++;                                          
      51 +        ctx->default_label = lbl;                              
      52 +        if (node->child_count > 0) {                           
      53 +            collect_switch_labels(cg, node->children[0], ctx); 
      54 +        }                                                      
      55 +        return;                                                
      56 +    }                                                          
      57 +    if (node->type == AST_SWITCH_STATEMENT) {                  
      58 +        return; /* nested switch — its labels belong to it */  
      59 +    }                                                          
      60 +    for (int i = 0; i < node->child_count; i++) {              
      61 +        collect_switch_labels(cg, node->children[i], ctx);     
      62 +    }                                                          
      63 +}                                                              
      64 +                                                               
      65 +static int switch_lookup_label(SwitchCtx *ctx, ASTNode *node) {
      66 +    for (int i = 0; i < ctx->count; i++) {                     
      67 +        if (ctx->nodes[i] == node) return ctx->labels[i];      
      68 +    }                                                          
      69 +    return -1;                                                 
      70 +}                                                              
      71 +                                                               
      72  static int codegen_find_var(CodeGen *cg, const char *name) {
      73      /* Walk backward so the innermost (most recently declared) 
          shadow wins. */
      74      for (int i = cg->local_count - 1; i >= 0; i--) {

● Now rewrite the AST_SWITCH_STATEMENT branch and add case/default label
  emission inside codegen_statement.

● Update(src/codegen.c)
  ⎿  Added 47 lines, removed 30 lines
      371          fprintf(cg->output, ".L_break_%d:\n", label_id);
      372          cg->break_depth--;
      373      } else if (node->type == AST_SWITCH_STATEMENT) {
      374 -        /* children: [0]=controlling expression, [1..]=section
          -s (case or                                                    
      375 -         * default). Evaluate the expression once, spill to th
          -e stack,                                                      
      376 -         * then linearly compare against each case value and j
          -ump to the                                                    
      377 -         * matching section label. If no case matches, jump to
          - default                                                      
      378 -         * (if present) or to the switch-end label. `break` al
          -so targets                                                    
      379 -         * the end label. */                                  
      380 -        int label_id = cg->label_count++;                     
      381 -        int num_sections = node->child_count - 1;             
      382 -        int section_labels[AST_MAX_CHILDREN];                 
      383 -        int default_label = -1;                               
      384 -        for (int i = 0; i < num_sections; i++) {              
      385 -            section_labels[i] = cg->label_count++;            
      386 -            if (node->children[i + 1]->type == AST_DEFAULT_SEC
          -TION) {                                                       
      387 -                default_label = section_labels[i];            
      388 -            }                                                 
      374 +        /* children: [0]=controlling expression, [1]=body stat
          +ement.                                                        
      375 +         * Pre-walk the body to collect every case/default lab
          +el                                                            
      376 +         * (stopping at nested switches), evaluate the express
          +ion once,                                                     
      377 +         * spill it to the stack, emit a linear compare-and-br
          +anch                                                          
      378 +         * dispatch, then emit the body as ordinary statements
          +. When a                                                      
      379 +         * case/default node is visited during emission it loo
          +ks up its                                                     
      380 +         * pre-assigned label via the innermost SwitchCtx. `br
          +eak`                                                          
      381 +         * targets the switch-end label. */                   
      382 +        if (cg->switch_depth >= MAX_SWITCH_DEPTH) {           
      383 +            fprintf(stderr, "error: switch nesting exceeds max
          + depth %d\n",                                                 
      384 +                    MAX_SWITCH_DEPTH);                        
      385 +            exit(1);                                          
      386          }
      387 +        int label_id = cg->label_count++;                     
      388 +        SwitchCtx *ctx = &cg->switch_stack[cg->switch_depth]; 
      389 +        ctx->count = 0;                                       
      390 +        ctx->default_label = -1;                              
      391 +        ASTNode *body = node->children[1];                    
      392 +        collect_switch_labels(cg, body, ctx);                 
      393  
      394          codegen_expression(cg, node->children[0]);
      395          fprintf(cg->output, "    push rax\n");
      396          cg->push_depth++;
      397  
      395 -        for (int i = 0; i < num_sections; i++) {              
      396 -            ASTNode *section = node->children[i + 1];         
      397 -            if (section->type == AST_CASE_SECTION) {          
      398 +        for (int i = 0; i < ctx->count; i++) {                
      399 +            ASTNode *label_node = ctx->nodes[i];              
      400 +            if (label_node->type == AST_CASE_SECTION) {       
      401                  fprintf(cg->output, "    mov eax, [rsp]\n");
      402                  fprintf(cg->output, "    cmp eax, %s\n",
      400 -                        section->children[0]->value);         
      403 +                        label_node->children[0]->value);      
      404                  fprintf(cg->output, "    je .L_switch_sec_%d\n
           ",
      402 -                        section_labels[i]);                   
      405 +                        ctx->labels[i]);                      
      406              }
      407          }
      405 -        if (default_label != -1) {                            
      406 -            fprintf(cg->output, "    jmp .L_switch_sec_%d\n", 
          -default_label);                                               
      408 +        if (ctx->default_label != -1) {                       
      409 +            fprintf(cg->output, "    jmp .L_switch_sec_%d\n", 
          +ctx->default_label);                                          
      410          } else {
      411              fprintf(cg->output, "    jmp .L_switch_end_%d\n", 
           label_id);
      412          }
     ...
      414          cg->break_stack[cg->break_depth].break_label = label_i
           d;
      415          cg->break_stack[cg->break_depth].continue_label = -1;
      416          cg->break_depth++;
      417 +        cg->switch_depth++;                                   
      418  
      415 -        for (int i = 0; i < num_sections; i++) {              
      416 -            ASTNode *section = node->children[i + 1];         
      417 -            fprintf(cg->output, ".L_switch_sec_%d:\n", section
          -_labels[i]);                                                  
      418 -            int start_idx = (section->type == AST_CASE_SECTION
          -) ? 1 : 0;                                                    
      419 -            for (int j = start_idx; j < section->child_count; 
          -j++) {                                                        
      420 -                codegen_statement(cg, section->children[j], is
          -_main);                                                       
      421 -            }                                                 
      422 -        }                                                     
      419 +        codegen_statement(cg, body, is_main);                 
      420  
      421 +        cg->switch_depth--;                                   
      422          cg->break_depth--;
      423  
      424          fprintf(cg->output, ".L_switch_end_%d:\n", label_id);
      425          fprintf(cg->output, ".L_break_%d:\n", label_id);
      426          fprintf(cg->output, "    add rsp, 8\n");
      427          cg->push_depth--;
      428 +    } else if (node->type == AST_CASE_SECTION) {              
      429 +        /* children: [0]=int_literal, [1]=inner statement. The
          + label                                                        
      430 +         * was pre-assigned by collect_switch_labels when the 
      431 +         * enclosing switch was emitted; look it up and emit, 
          +then                                                          
      432 +         * fall through to the inner statement. */            
      433 +        SwitchCtx *ctx = &cg->switch_stack[cg->switch_depth - 
          +1];                                                           
      434 +        int lbl = switch_lookup_label(ctx, node);             
      435 +        fprintf(cg->output, ".L_switch_sec_%d:\n", lbl);      
      436 +        if (node->child_count > 1) {                          
      437 +            codegen_statement(cg, node->children[1], is_main);
      438 +        }                                                     
      439 +    } else if (node->type == AST_DEFAULT_SECTION) {           
      440 +        /* children: [0]=inner statement. */                  
      441 +        SwitchCtx *ctx = &cg->switch_stack[cg->switch_depth - 
          +1];                                                           
      442 +        int lbl = switch_lookup_label(ctx, node);             
      443 +        fprintf(cg->output, ".L_switch_sec_%d:\n", lbl);      
      444 +        if (node->child_count > 0) {                          
      445 +            codegen_statement(cg, node->children[0], is_main);
      446 +        }                                                     
      447      } else if (node->type == AST_BREAK_STATEMENT) {
      448          int id = cg->break_stack[cg->break_depth - 1].break_la
           bel;
      449          fprintf(cg->output, "    jmp .L_break_%d\n", id);

● Step 4: Build and run tests

● Bash(cd /home/scott/code/claude/claudec99/build && cmake --build . 2>&1 | tail
       -20)
  ⎿  [ 12%] Building C object CMakeFiles/ccompiler.dir/src/compiler.c.o
     [ 25%] Building C object CMakeFiles/ccompiler.dir/src/ast_pretty_print
     er.c.o
     … +4 lines (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99/test/valid && bash run_tests.sh 2>&1
       | tail -5)
  ⎿  PASS  test_while_nested__42  (exit code: 42)
     compiled: /home/scott/code/claude/claudec99/test/valid/test_while_sing
     le_stmt__42.c -> test_while_single_stmt__42.asm
     … +3 lines (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99/test/invalid && ls && echo "---" &&
      bash run_tests.sh 2>&1 | tail -5)
  ⎿  run_tests.sh
     test_invalid_10__parameter_count_mismatch.c
     test_invalid_11__break_outside_of_loop.c
     … +19 lines (ctrl+o to expand)

● Bash(bash run_tests.sh 2>&1 | grep -E "FAIL|PASS" | head -20)
  ⎿  PASS  test_invalid_10__parameter_count_mismatch  (error contains: 'par
     ameter count mismatch')
     PASS  test_invalid_11__break_outside_of_loop  (error contains: 'break
     … +16 lines (ctrl+o to expand)

  Read 2 files (ctrl+o to expand)

● Stage 10-03-03 makes unlabeled statements in a switch body legal, so this
  invalid test is obsolete and needs to be removed.

● Bash(rm /home/scott/code/claude/claudec99/test/invalid/test_invalid_13__expect
      ed_token_type.c && bash run_tests.sh 2>&1 | tail -3)
  ⎿  PASS  test_invalid_9__call_to_undefined_function  (error contains: 'ca
     ll to undefined function')

     Results: 12 passed, 0 failed, 12 total

● Step 5: Add test for unlabeled statement in switch body

● Write(test/valid/test_switch_unlabeled_stmt_in_body__42.c)
  ⎿  Wrote 12 lines to test/valid/test_switch_unlabeled_stmt_in_body__42.c
       1 int main() {
       2     int x = 0;
       3     switch (1) {
       4         x = 99;
       5         case 1:
       6             x = 42;
       7             break;
       8         default:
       9             x = 7;
      10     }
     … +2 lines (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99/test/valid && bash run_tests.sh 2>&1
       | tail -3 && echo "---" && bash run_tests.sh 2>&1 | grep -E
      "FAIL|unlabeled")
  ⎿  PASS  test_while_single_stmt__42  (exit code: 42)

     Results: 117 passed, 0 failed, 117 total
     … +5 lines (ctrl+o to expand)

● All 117 valid + 12 invalid tests pass, including the new unlabeled-statement
  test. Let me verify the generated assembly looks right.

● Bash(cd /tmp && /home/scott/code/claude/claudec99/build/ccompiler /home/scott/
      code/claude/claudec99/test/valid/test_switch_case_fallthrough_1__10.c &&
      cat test_switc…)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_switch_cas
     e_fallthrough_1__10.c -> test_switch_case_fallthrough_1__10.asm
     section .text
     … +31 lines (ctrl+o to expand)
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Stacked case labels, fallthrough to b=10, break-to-end, default path all emit
  correctly. Let me also verify the new unlabeled-statement test:

● Bash(cd /tmp && /home/scott/code/claude/claudec99/build/ccompiler /home/scott/
      code/claude/claudec99/test/valid/test_switch_unlabeled_stmt_in_body__42.c
      && cat test_s…)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_switch_unl
     abeled_stmt_in_body__42.c -> test_switch_unlabeled_stmt_in_body__42.as
     m
     … +30 lines (ctrl+o to expand)
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Dispatch skips the unlabeled x = 99; — correct. Now update the grammar doc.

  Step 6: Update docs/grammar.md

● Update(docs/grammar.md)
  ⎿  Added 1 line
      20                      | <do_while_statement>
      21                      | <for_statement>
      22                      | <switch_statement>
      23 +                    | <labeled_statement>                      
      24                      | <block_statement>
      25                      | <jump_statement>
      26                      | <expression_statement>

● Update(docs/grammar.md)
  ⎿  Added 4 lines, removed 7 lines
      37  
      38  <for_statement> ::= "for" "(" [<expression>] ";" [<expression>]
           ";" [<expression>] ")" <statement>
      39  
      40 -<switch_statement> ::= "switch" "(" <expression> ")" <switch_bo
         -dy>                                                            
      40 +<switch_statement> ::= "switch" "(" <expression> ")" <statement
         +>                                                              
      41  
      42 -<switch_body> ::= "{" <switch_section> { <switch_section> } "}"
      42 +<labeled_statement> ::= "case" <constant_expression> ":" <state
         +ment>                                                          
      43 +                      | "default" ":" <statement>              
      44  
      44 -<switch_section> ::= <case_section> | <default_section>        
      45 +<constant_expression> ::= <integer_literal>                    
      46  
      46 -<case_section> ::= "case" <integer_literal> ":" { <statement> }
      47 -                                                               
      48 -<default_section> ::= "default" ":" { <statement> }            
      49 -                                                               
      47  <jump_statement> ::= "continue" ";"
      48                      | "break" ";"
      49  

● Update(docs/grammar.md)
  ⎿  Added 1 line, removed 1 line
      73                      
      74  <postfix_expression> ::= <primary_expression> { "++" | "--" }  
                            
      75  
      76 -<primary_expression> ::= <int_literal>                         
      76 +<primary_expression> ::= <integer_literal>                     
      77                           | <identifier>
      78                           | <function_call>
      79                           | "(" <expression> ")"
● Update(docs/grammar.md)
  ⎿  Added 1 line, removed 1 line
      84  
      85  <identifier> ::= [a-zA-Z_][a-zA-Z0-9_]*
      86  
      87 -<int_literal> ::= [0-9]+                                       
      87 +<integer_literal> ::= [0-9]+                                   
      88  
      89  ```

  Read 1 file (ctrl+o to expand)

● Grammar updated. Run all tests one final time.

● Bash(cd /home/scott/code/claude/claudec99/test/valid && bash run_tests.sh 2>&1
       | tail -2 && cd ../invalid && bash run_tests.sh 2>&1 | tail -2)
  ⎿  Results: 117 passed, 0 failed, 117 total

     Results: 12 passed, 0 failed, 12 total

● All 129 tests (117 valid + 12 invalid) pass. Now the milestone + transcript
  artifacts.

  Step 7: Milestone summary & transcript

● Write(docs/milestones/stage-10-03-03-summary.md)
  ⎿  Wrote 32 lines to docs/milestones/stage-10-03-03-summary.md
      2 # Stage-10-03-03 Milestone Summary: Standard Switch Support
      3 Stage 10-03-03 brings `switch` to a standard C shape. The switch 
        body
      4 is now any statement, and `case <integer_literal> :` / `default :
        `
        sre `<labeled_statement>`s that may appear anywhere a statement i
      6 legal inside a switch body (including nested blocks and inner
      7 control-flow constructs within the same switch).
      8 
      9 Completed in this stage:
     … +24 lines (ctrl+o to expand)

● Write(docs/sessions/stage-10-03-03-transcript.md)
  ⎿  Wrote 123 lines to docs/sessions/stage-10-03-03-transcript.md
       2 # stage-10-03-03 Transcript: Standard Switch Support
       3 ## Summary
       4 
         -02ge 10-03-03 advances `switch` from the restricted stage 10-03
       6 form (a braced list of `case`/`default` sections) to a standard 
         C
       8 `default` are now labeled statements and may appear anywhere a
       9 statement is legal inside the switch body. Unlabeled statements 
         in
      10 a switch body are legal, stacked case labels are supported, and
     … +114 lines (ctrl+o to expand)

● Step 8: Commit

● Bash(git status)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +19 lines (ctrl+o to expand)

● Bash(git diff --stat)
  ⎿   docs/grammar.md                                    |  16 ++-
      include/codegen.h                                  |  17 +++
      src/ast_pretty_printer.c                           |   9 +-
     … +5 lines (ctrl+o to expand)

● Bash(git add docs/grammar.md include/codegen.h src/ast_pretty_printer.c
      src/codegen.c src/parser.c docs/kickoffs/stage-10-03-03-kickoff.md
      docs/milestones/stage-10-0…)
  ⎿  Error: Exit code 128
     fatal: pathspec 'docs/grammar.md' did not match any files

● Bash(cd /home/scott/code/claude/claudec99 && git add docs/grammar.md
      include/codegen.h src/ast_pretty_printer.c src/codegen.c src/parser.c
      docs/kickoffs/stage-10-03-…)
  ⎿  rm 'test/invalid/test_invalid_13__expected_token_type.c'
     On branch master
     Your branch is up to date with 'origin/master'.
     … +13 lines (ctrl+o to expand)

● Bash(git commit -m "$(cat <<'EOF'
      stage-10-03-03: standard switch support…)
  ⎿  [master d7a2cd5] stage-10-03-03: standard switch support
      10 files changed, 397 insertions(+), 92 deletions(-)
      create mode 100644 docs/kickoffs/stage-10-03-03-kickoff.md
     … +6 lines (ctrl+o to expand)

● Milestone Summary

  Stage-10-03-03 implemented and committed (d7a2cd5).

  - Parser now treats switch body as any statement; case/default parsed as
  labeled statements guarded by switch_depth.
  - Codegen uses a SwitchCtx stack + pre-walk to collect labels (stopping at
  nested switches) and emits dispatch before the body; body emission places
  labels inline — giving stacked cases, fallthrough, and unlabeled switch-body
  statements for free.
  - AST shape updated (case = [int_literal, stmt], default = [stmt]); pretty
  printer matched.
  - Grammar doc synced (<labeled_statement>, <constant_expression>, int_literal
  → integer_literal).
  - 117 valid + 12 invalid tests pass (added 1 new test for unlabeled
  statements, removed 1 obsolete invalid test).
  - Kickoff, milestone, and transcript saved under docs/.

✻ Sautéed for 3m 53s