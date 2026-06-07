 ▐▛███▜▌   Claude Code v2.1.168
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

  Feature of the week: /loop — run a prompt or slash command on a recurring interval


❯ /usage 
  ⎿  Settings dialog dismissed

❯ /implement-stage docs/stages/ClaudeC99-spec-stage-95-09-remove-static-array-f 
rom-ast-and-parser.md                                                           

● I'll start by reading the spec file and supporting files in parallel.

  Read 4 files (ctrl+o to expand)

● Now let me read the relevant source files to understand the current state.

  Searched for 2 patterns, read 3 files (ctrl+o to expand)

● Let me read the ast.c and relevant parser sections to understand the full
  scope.

  Read 2 files (ctrl+o to expand)

● Let me look at more critical sections of the parser.

  Read 1 file (ctrl+o to expand)

● Let me check the lexer header and parser_node function, and codegen usage.

  Searched for 4 patterns, read 2 files (ctrl+o to expand)

● Let me look at the codegen string literal section and the AST pretty printer.

  Searched for 2 patterns, read 1 file (ctrl+o to expand)

● Let me check how string pool is emitted in codegen, and the
  strlen(node->value) call.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me look at the string pool emitter and codegen_emit_string_pool.

  Read 1 file (ctrl+o to expand)

● Let me look at the lexer implementation for string storage.

  Searched for 1 pattern, read 2 files (ctrl+o to expand)

● Let me check specific codegen value accesses, particularly value[0] patterns.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me look at how unnamed params are created.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)
● Let me check all AST_ASSIGNMENT creation sites and the pretty printer NULL
  patterns.

  Searched for 1 pattern, read 2 files (ctrl+o to expand)

● Let me look at codegen lines 4730 and 4758.

  Read 1 file (ctrl+o to expand)

● Let me look at ParsedDeclarator definition and how d.name is used.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me look at line 2722 and more parser context around declarations.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me check parser.c includes and the strbuf/include setup.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me check the remaining value[0] patterns in codegen that I haven't read
  yet.

  Read 1 file (ctrl+o to expand)

● Let me check the remaining char literal value[0] accesses in codegen.

  Read 1 file (ctrl+o to expand)

● Now let me read line 5265+ in codegen.

  Read 1 file (ctrl+o to expand)

● Let me check all files that reference ASTNode or node->value or ->value
  besides the ones I've already reviewed.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Good - that's all token-based accesses in compiler.c, not AST node. Now let me
  get the template files for the kickoff artifact.

  Read 2 files, listed 1 directory (ctrl+o to expand)

● Now I have a thorough understanding of all changes needed. Let me confirm the
  stage label and produce the kickoff document.

  STAGE_DISPLAY: Stage 95-09
  STAGE_LABEL: stage-95-09

  Spec Summary

  Stage 95-09 changes ASTNode.value from char value[MAX_NAME_LEN] (a fixed
  256-byte embedded buffer) to const char *value (a pointer into lexer-owned
  storage). All parser sites that memcpy token bytes into the AST node value are
  replaced with simple pointer assignments. String literal concatenation needs
  a StrBuf scratch + lexer_store_bytes to combine multiple lexer buffers into
  one allocation.

  Spec issue noted: The title says "remove statis char array" — "statis" is a
  typo for "static".

  Planned Changes

  ┌──────────────────────────────────┬───────────────────────────────────────┐
  │               File               │             What changes              │
  ├──────────────────────────────────┼───────────────────────────────────────┤
  │ include/ast.h                    │ char value[MAX_NAME_LEN] → const char │
  │                                  │  *value                               │
  ├──────────────────────────────────┼───────────────────────────────────────┤
  │ include/lexer.h                  │ Expose lexer_store_bytes (remove      │
  │                                  │ static)                               │
  ├──────────────────────────────────┼───────────────────────────────────────┤
  │ src/lexer.c                      │ Remove static from lexer_store_bytes  │
  ├──────────────────────────────────┼───────────────────────────────────────┤
  │ src/ast.c                        │ strncpy in ast_new → pointer          │
  │                                  │ assignment                            │
  ├──────────────────────────────────┼───────────────────────────────────────┤
  │                                  │ ~12 write sites +                     │
  │ src/parser.c                     │ ParsedDeclarator.name + NULL guards   │
  │                                  │ for value[0] reads                    │
  ├──────────────────────────────────┼───────────────────────────────────────┤
  │ src/codegen.c                    │ NULL guards on 5 value[0] access      │
  │                                  │ sites for nullable nodes              │
  ├──────────────────────────────────┼───────────────────────────────────────┤
  │ src/ast_pretty_printer.c         │ NULL guard on 1 value[0] access site  │
  ├──────────────────────────────────┼───────────────────────────────────────┤
  │ src/version.c                    │ VERSION_STAGE → 00950900              │
  ├──────────────────────────────────┼───────────────────────────────────────┤
  │ docs/fixed-capacity-inventory.md │ Mark ASTNode.value done               │
  └──────────────────────────────────┴───────────────────────────────────────┘

  Now let me generate the kickoff artifact and then begin implementation.

● haiku-stage-artifact-writer(Generate stage-95-09 kickoff artifact)
  ⎿  Done (10 tool uses · 14.0k tokens · 31s)
  (ctrl+o to expand)

● Good. Now let me begin implementation. I'll start with the lexer exposure,
  then AST, then parser.

  Step 1: Expose lexer_store_bytes in include/lexer.h and remove static in
  src/lexer.c.

● Update(include/lexer.h)
  ⎿  Added 5 lines
      25  void lexer_init(Lexer *lexer, const char *source, const char *f
          ilename);
      26  void lexer_free(Lexer *lexer);
      27  Token lexer_next_token(Lexer *lexer);
      28 +/* Stage 95-09: allocate a null-terminated copy of `data[0..len
         +-1]` in the                                                    
      29 + * lexer's string pool and return a stable pointer. ASTNode.val
         +ue uses this                                                   
      30 + * to store content that cannot be pointed to directly in an ex
         +isting token                                                   
      31 + * (e.g. concatenated string literals, formatted integer values
         +). */                                                          
      32 +const char *lexer_store_bytes(Lexer *lexer, const char *data, s
         +ize_t len);                                                    
      33  
      34  #endif

● Update(src/lexer.c)
  ⎿  Added 1 line, removed 1 line
      37   * callers can safely pass the result to string.h functions.  T
          he returned
      38   * pointer is stable for the lifetime of the lexer because each
           string is
      39   * an independent malloc, not part of a relocatable buffer. */
      40 -static const char *lexer_store_bytes(Lexer *lexer, const char *
         -data, size_t len) {                                            
      40 +const char *lexer_store_bytes(Lexer *lexer, const char *data, s
         +ize_t len) {                                                   
      41      char *copy = (char *)malloc(len + 1);
      42      if (!copy) { fprintf(stderr, "error: out of memory in lexer
          \n"); exit(1); }
      43      memcpy(copy, data, len);

● Step 2: Change ASTNode.value in include/ast.h.
● Update(include/ast.h)
  ⎿  Added 1 line, removed 1 line
      61  
      62  typedef struct ASTNode {
      63      ASTNodeType type;
      64 -    char value[MAX_NAME_LEN];                                  
      64 +    const char *value;                                         
      65      /* Stage 92: children is a dynamically grown array (lazily 
          allocated on
      66       * first ast_add_child, initial capacity AST_MAX_CHILDREN, 
          doubled on
      67       * overflow) rather than a fixed-size array. The previous f
          ixed cap

● Step 3: Update src/ast.c - ast_new to use pointer assignment instead of
  strncpy.

● Update(src/ast.c)
  ⎿  Added 1 line, removed 3 lines
      10          exit(1);
      11      }
      12      node->type = type;
      13 -    if (value) {                                               
      14 -        strncpy(node->value, value, sizeof(node->value) - 1);  
      15 -    }                                                          
      13 +    node->value = value;                                       
      14      return node;
      15  }
      16  

● Step 4: Update src/parser.c. First, change the ParsedDeclarator.name field and
  add strbuf.h include.

● Update(src/parser.c)
  ⎿  Added 1 line
      2  #include <stdlib.h>
      3  #include <string.h>
      4  #include "parser.h"
      5 +#include "strbuf.h"                                             
      6  #include "util.h"
      7  
      8  /* Stage 70-03: report a parser error at the current token's sou
         rce position. */

● Update(src/parser.c)
  ⎿  Added 1 line, removed 1 line
      52  #define MAX_ARRAY_DIMS 8
      53  
      54  typedef struct {
      55 -    char name[256];                                            
      55 +    const char *name;                                          
      56      int  pointer_count;
      57      int  is_array;
      58      int  has_size;

● Now update the two places in parse_declarator where d.name is assigned via
  strncpy:

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 1 line, removed 2 lines
      1186              parser->current = lexer_next_token(parser->lexer)
            ;
      1187          }
      1188          Token name = parser_expect(parser, TOKEN_IDENTIFIER);
      1189 -        strncpy(d.name, name.value, sizeof(d.name) - 1);     
      1190 -        d.name[sizeof(d.name) - 1] = '\0';                   
      1189 +        d.name = name.value;                                 
      1190          /* A function suffix on the inner identifier means "f
            unction returning
      1191           * function pointer" (e.g. int (*fp())(int)) — not ye
            t supported. */
      1192          if (parser->current.type == TOKEN_LPAREN) {

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 1 line, removed 2 lines
      1271      /* Non-parenthesized declarator: stars before the identif
            ier. */
      1272      d.pointer_count = outer_stars;
      1273      Token name = parser_expect(parser, TOKEN_IDENTIFIER);
      1274 -    strncpy(d.name, name.value, sizeof(d.name) - 1);         
      1275 -    d.name[sizeof(d.name) - 1] = '\0';                       
      1274 +    d.name = name.value;                                     
      1275      if (parser->current.type == TOKEN_LBRACKET) {
      1276          d.is_array = 1;
      1277          /* Stage 86: parse first dimension (may be empty for 
            initializer inference). */

● Now update the string literal parsing in parse_primary. Let me read that
  section carefully first:

  Read 1 file (ctrl+o to expand)

● Now I'll replace the string literal and char literal sections:

● Update(src/parser.c)
  ⎿  Added 30 lines, removed 29 lines
      1353      /* Stage 14-02: a string literal is a primary expression 
            whose
      1354       * logical type is char[N+1], where N is the literal's by
            te
      1355       * length and the trailing slot holds the implicit NUL. T
            he
      1356 -     * payload bytes ride on node->value (copied below) and t
           -he                                                           
      1357 -     * length is preserved on full_type->length.             
      1356 +     * payload bytes are referenced via node->value (a pointe
           +r into                                                       
      1357 +     * lexer-owned storage) and the length is preserved on fu
           +ll_type->length.                                             
      1358       *
      1359       * Stage 14-05: the decoded payload may contain embedded 
            NUL
      1360 -     * bytes (from `\0`), so the value is copied with memcpy 
           -bounded                                                      
      1361 -     * by token.length rather than via ast_new's strncpy, and
           - the                                                         
      1362 -     * count is also stashed on byte_length for downstream co
           -nsumers                                                      
      1363 -     * (notably codegen, where full_type is rewritten to `cha
           -r *`                                                         
      1364 -     * during the array-to-pointer decay and the length on fu
           -ll_type                                                      
      1365 -     * is no longer reachable). */                           
      1360 +     * bytes (from `\0`), so the count is stashed on byte_len
           +gth for                                                      
      1361 +     * downstream consumers (notably codegen, where full_type
           + is                                                          
      1362 +     * rewritten to `char *` during the array-to-pointer deca
           +y and                                                        
      1363 +     * the length on full_type is no longer reachable).      
      1364 +     *                                                       
      1365 +     * Stage 95-09: node->value is now a const char * into le
           +xer                                                          
      1366 +     * storage. For a single literal, point directly to token
           +.value.                                                      
      1367 +     * For adjacent (concatenated) literals, collect bytes in
           + a StrBuf                                                    
      1368 +     * and store the result via lexer_store_bytes. */        
      1369      if (parser->current.type == TOKEN_STRING_LITERAL) {
      1370          Token token = parser->current;
      1371          parser->current = lexer_next_token(parser->lexer);
      1372          ASTNode *node = parser_node(parser, AST_STRING_LITERA
            L, NULL);
      1370 -        if ((int)token.value_len >= MAX_NAME_LEN) {          
      1371 -            PARSER_ERROR(parser, "error: string literal too l
           -ong (max %d bytes)\n",                                       
      1372 -                         MAX_NAME_LEN - 1);                  
      1373 -        }                                                    
      1374 -        memcpy(node->value, token.value, token.value_len);   
      1373          int total_len = (int)token.value_len;
      1374          /* Stage 89: consume any adjacent string literal toke
            ns and
      1375           * concatenate their decoded bytes into the same node
            . */
      1378 -        while (parser->current.type == TOKEN_STRING_LITERAL) 
           -{                                                            
      1379 -            Token next = parser->current;                    
      1380 -            if (total_len + (int)next.value_len >= MAX_NAME_L
           -EN) {                                                        
      1381 -                PARSER_ERROR(parser, "error: concatenated str
           -ing literal too long (max %d bytes)\n",                      
      1382 -                             MAX_NAME_LEN - 1);              
      1383 -                break;                                       
      1376 +        if (parser->current.type == TOKEN_STRING_LITERAL) {  
      1377 +            StrBuf sb;                                       
      1378 +            strbuf_init(&sb);                                
      1379 +            strbuf_append_n(&sb, token.value, token.value_len
           +);                                                           
      1380 +            while (parser->current.type == TOKEN_STRING_LITER
           +AL) {                                                        
      1381 +                Token next = parser->current;                
      1382 +                parser->current = lexer_next_token(parser->le
           +xer);                                                        
      1383 +                strbuf_append_n(&sb, next.value, next.value_l
           +en);                                                         
      1384 +                total_len += (int)next.value_len;            
      1385              }
      1385 -            parser->current = lexer_next_token(parser->lexer)
           -;                                                            
      1386 -            memcpy(node->value + total_len, next.value, next.
           -value_len);                                                  
      1387 -            total_len += (int)next.value_len;                
      1386 +            node->value = lexer_store_bytes(parser->lexer, sb
           +.data, (size_t)total_len);                                   
      1387 +            strbuf_free(&sb);                                
      1388 +        } else {                                             
      1389 +            node->value = token.value;                       
      1390          }
      1389 -        node->value[total_len] = '\0';                       
      1391          node->byte_length = total_len;
      1392          node->decl_type = TYPE_ARRAY;
      1393          node->full_type = type_array(type_char(), total_len +
             1);
     ...
      1395      }
      1396      /* Stage 15-02: a character literal is a primary expressi
            on of type
      1397       * int. The token already carries the decoded byte at val
            ue[0] and
      1397 -     * the evaluated integer at long_value; mirror the string
           --literal                                                     
      1398 -     * convention by storing the decoded byte at node->value[
           -0]. The                                                      
      1399 -     * integer value used by codegen is recovered as         
      1400 -     * (unsigned char)node->value[0]. */                     
      1398 +     * the evaluated integer at long_value. The integer value
           + used by                                                     
      1399 +     * codegen is recovered as (unsigned char)node->value[0].
      1400 +     *                                                       
      1401 +     * Stage 95-09: node->value points directly to lexer-owne
           +d storage                                                    
      1402 +     * (token.value is a 1-byte null-terminated string in the
           + lexer pool). */                                             
      1403      if (parser->current.type == TOKEN_CHAR_LITERAL) {
      1404          Token token = parser->current;
      1405          parser->current = lexer_next_token(parser->lexer);
      1406          ASTNode *node = parser_node(parser, AST_CHAR_LITERAL,
             NULL);
      1405 -        node->value[0] = token.value[0];                     
      1406 -        node->value[1] = '\0';                               
      1407 +        node->value = token.value;                           
      1408          node->byte_length = 1;
      1409          node->decl_type = TYPE_INT;
      1410          return node;

● Now update the va_arg type name assignment:

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 1 line, removed 2 lines
      1458              Type *arg_type = parse_type_name(parser);
      1459              node->decl_type = arg_type->kind;
      1460              node->full_type = arg_type;
      1461 -            snprintf(node->value, sizeof(node->value), "%s", 
      1462 -                     type_kind_name(arg_type->kind));        
      1461 +            node->value = type_kind_name(arg_type->kind);    
      1462              parser_expect(parser, TOKEN_RPAREN);
      1463              return node;
      1464          }

● Now update the enum constant int literal case:

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 1 line
      1470                  char buf[32];
      1471                  snprintf(buf, sizeof(buf), "%ld", ec->value);
      1472                  parser->current = lexer_next_token(parser->le
            xer);
      1473 -                ASTNode *node = parser_node(parser, AST_INT_L
           -ITERAL, buf);                                                
      1473 +                ASTNode *node = parser_node(parser, AST_INT_L
           +ITERAL,                                                      
      1474 +                    lexer_store_bytes(parser->lexer, buf, str
           +len(buf)));                                                  
      1475                  node->decl_type = TYPE_INT;
      1476                  return node;
      1477              }

● Now update the two memcpy string initializer sites (local char array init and
  file-scope char array init):

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 3 lines
      2636                      Token str_tok = parser->current;
      2637                      parser->current = lexer_next_token(parser
            ->lexer);
      2638                      ASTNode *str_init = parser_node(parser, A
            ST_STRING_LITERAL, NULL);
      2639 -                    memcpy(str_init->value, str_tok.value, st
           -r_tok.value_len);                                            
      2640 -                    str_init->value[str_tok.value_len] = '\0'
           -;                                                            
      2639 +                    str_init->value = str_tok.value;         
      2640                      str_init->byte_length = (int)str_tok.valu
            e_len;
      2641                      str_init->decl_type = TYPE_ARRAY;
      2642                      str_init->full_type = type_array(type_cha
            r(), (int)str_tok.value_len + 1);
      2643                      int needed = str_init->byte_length + 1;
      2644                      if (has_size) {
      2645                          if (length < needed) {
      2647 -                            PARSER_ERROR(parser,             
      2646 +                            PARSER_ERROR(parser,             
      2647                                      "error: array too small f
            or string literal initializer\n");
      2648                          }
      2649                      } else {

● Now the file-scope char array init case:

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 3 lines
      3251                      Token str_tok = parser->current;
      3252                      parser->current = lexer_next_token(parser
            ->lexer);
      3253                      ASTNode *str_init = parser_node(parser, A
            ST_STRING_LITERAL, NULL);
      3254 -                    memcpy(str_init->value, str_tok.value, st
           -r_tok.value_len);                                            
      3255 -                    str_init->value[str_tok.value_len] = '\0'
           -;                                                            
      3254 +                    str_init->value = str_tok.value;         
      3255                      str_init->byte_length = (int)str_tok.valu
            e_len;
      3256                      int needed = str_init->byte_length + 1;
      3257                      if (has_size) {
      3258                          if (length < needed) {
      3260 -                            PARSER_ERROR(parser,             
      3259 +                            PARSER_ERROR(parser,             
      3260                                      "error: array too small f
            or string literal initializer\n");
      3261                          }
      3262                      } else {

● Now update the parameter name null checks in parse_parameter_list:

  Read 1 file (ctrl+o to expand)
● The param checks are fine since AST_PARAM always gets "" or a real name. Now I
  need to fix the unnamed param check in function definition validation and the
  AST_ASSIGNMENT NULL checks.

  Read 1 file (ctrl+o to expand)

● Good, that's fine. Now update the two NULL-safety guards needed in codegen.c
  and ast_pretty_printer.c for AST_ASSIGNMENT nodes that are created with NULL
  value:

● Update(src/codegen.c)
  ⎿  Added 1 line, removed 1 line
      1495      case AST_POSTFIX_INC_DEC:
      1496          return sizeof_type_of_expr(cg, node->children[0]);
      1497      case AST_ASSIGNMENT: {
      1498 -        if (node->value[0] != '\0') {                        
      1498 +        if (node->value && node->value[0] != '\0') {         
      1499              LocalVar *lv = codegen_find_var(cg, node->value);
      1500              if (lv) return lv->kind;
      1501              GlobalVar *gv = codegen_find_global(cg, node->val
            ue);

● Update(src/ast_pretty_printer.c)
  ⎿  Added 1 line, removed 1 line
      171          printf("Unary: %s\n", operator_name(node->value));
      172          break;
      173      case AST_ASSIGNMENT:
      174 -        if (node->value[0] == '\0') {                         
      174 +        if (!node->value || node->value[0] == '\0') {         
      175              /* Stage 12-03: deref-LHS assignment has no variab
           le
      176               * name — children carry [LHS, RHS]. */
      177              printf("Assignment:\n");

● Now update src/version.c for this stage:

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      3  
      4  #define VERSION_MAJOR  "00"
      5  #define VERSION_MINOR  "02"
      6 -#define VERSION_STAGE  "00950800"                               
      6 +#define VERSION_STAGE  "00950900"                               
      7  
      8  #ifndef VERSION_BUILD
      9  #define VERSION_BUILD  0

● Now let me try to build with GCC to see if there are any compile errors:

● Bash(cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -5 && cmake
      --build build 2>&1 | head -60)
  ⎿  -- Configuring done (0.0s)
     -- Generating done (0.0s)
     -- Build files have been written to: /home/scott/code/claude/claudec99
     … +60 lines (ctrl+o to expand)

● Bash(cmake --build build 2>&1 | grep -E
      "error:|warning:.*discards|warning:.*const" | head -30)
  ⎿  (No output)

● Bash(cmake --build build 2>&1 | tail -5)
  ⎿  [100%] Built target ccompiler

● Build succeeds. Now run the full test suite:

● Bash(./test/run_all_tests.sh 2>&1 | tail -20)
  ⎿  PASS  test_stage_14_07_libc_puts
     PASS  test_stage_16_01_remainder_int
     PASS  test_stage_16_01_remainder_long
     … +17 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● There are 18 failures. Let me examine them:

● Bash(./test/run_all_tests.sh 2>&1 | grep "^FAIL")
  ⎿  FAIL  test_switch_case_binary_add__42  (nasm error)
     FAIL  test_switch_case_fallthrough_1__10  (nasm error)
     FAIL  test_switch_case_fallthrough_2__10  (nasm error)
     … +15 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● All failures are switch-related. Let me investigate the switch code path:

● Bash(build/ccompiler test/valid/test_switch_with_case_1__42.c 2>&1 | head -20
      && echo "---" && cat test_switch_with_case_1__42.asm 2>/dev/null | head
      -30 || echo "No…)
  ⎿  compiled: test/valid/test_switch_with_case_1__42.c -> test_switch_with
     _case_1__42.asm
     ---
     … +30 lines (ctrl+o to expand)

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me look at case section parsing in the parser:

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Found the bug! case_buf is a local stack variable passed to parser_node - it's
  dangling after return. Fix it with lexer_store_bytes:

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 1 line
      2798          char case_buf[32];
      2799          snprintf(case_buf, sizeof(case_buf), "%ld", case_val)
            ;
      2800          ASTNode *node = parser_node(parser, AST_CASE_SECTION,
             NULL);
      2801 -        ast_add_child(node, parser_node(parser, AST_INT_LITER
           -AL, case_buf));                                              
      2801 +        ast_add_child(node, parser_node(parser, AST_INT_LITER
           +AL,                                                          
      2802 +            lexer_store_bytes(parser->lexer, case_buf, strlen
           +(case_buf))));                                               
      2803          ast_add_child(node, parse_statement(parser));
      2804          return node;
      2805      }

● Now I need to audit all other places where local char buf[] or snprintf
  results are passed to parser_node or used as node->value. Let me find them
  systematically:

  Searched for 1 pattern (ctrl+o to expand)
● Let me also search for any other places in parser.c where a local buffer might
  be assigned to node->value:

  Searched for 3 patterns (ctrl+o to expand)

● Let me check the other uses of local string buffers in the struct/enum
  parsing:

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me rebuild and test:

● Bash(cmake --build build 2>&1 | grep -E "error:|warning.*discards" | head -10)
  ⎿  (No output)

● Bash(cmake --build build 2>&1 | tail -3)
  ⎿  [100%] Built target ccompiler

● Bash(./test/run_all_tests.sh 2>&1 | tail -5)
  ⎿  Results: 21 passed, 0 failed, 21 total

     ====================================================
     … +2 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● All 1478 tests pass. Now let me add a new test to verify that string literals
  longer than 255 bytes work (the old MAX_NAME_LEN limit):

  Listed 1 directory (ctrl+o to expand)

● Write(test/valid/test_stage_95_09_long_string_literal__42.c)
  ⎿  Wrote 12 lines to
     test/valid/test_stage_95_09_long_string_literal__42.c
          MAX_NAME_LEN9: verify that a string literal longer than the old
         pointer rather is handled correctly now that ASTNode.value is a 
       4 int puts(char *s);ar[256] buffer. */
       5 int strlen(char *s);
       7 int main(void) {
          limit */0-character string — exceeds old 255-byte ASTNode.value
       9     char *s = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
         aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
         aaaaaaaaaaaaaaaaaaaaa";aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
      10     if (strlen(s) == 260) return 42;
     … +9 lines (ctrl+o to expand)

● Bash(build/ccompiler test/valid/test_stage_95_09_long_string_literal__42.c &&
      nasm -f elf64 test_stage_95_09_long_string_literal__42.asm -o
      /tmp/long_str.o && gcc -n…)
  ⎿  compiled: test/valid/test_stage_95_09_long_string_literal__42.c -> tes
     t_stage_95_09_long_string_literal__42.asm
     exit: 1

● Let me fix the test - I need to verify the string has exactly 260 'a' chars:

● Bash(python3 -c "s='a'*260; print(len(s)); print(repr(s[:10]))")
  ⎿  260
     'aaaaaaaaaa'

● Bash(python3 -c "s = '\"' + 'a'*260 + '\"'; print(s)" | wc -c)
  ⎿  263

  Searched for 3 patterns (ctrl+o to expand)

● Bash(python3 -c "
      with open('test/valid/test_stage_95_09_long_string_literal__42.c') as f:…)
  ⎿  Error: Exit code 1
     Traceback (most recent call last):
       File "<string>", line 5, in <module>
     ValueError: substring not found

● Bash(python3 -c "
      content =
      open('test/valid/test_stage_95_09_long_string_literal__42.c').read()…)
  ⎿  String length: 262

● The string is 262 characters. Let me fix the test:

● Write(test/valid/test_stage_95_09_long_string_literal__42.c)
  ⎿  Added 1 line, removed 2 lines
       1  /* Stage 95-09: verify that a string literal longer than the ol
          d MAX_NAME_LEN
       2   * (255 bytes) is handled correctly now that ASTNode.value is a
           pointer rather
       3   * than a fixed char[256] buffer. */
       4 -int puts(char *s);                                             
       4  int strlen(char *s);
       5  
       6  int main(void) {
       7      /* 260-character string — exceeds old 255-byte ASTNode.valu
          e limit */
       9 -    char *s = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
         -aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
         -aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
         -aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
         -aaaaaaaaaaaaaaaaaaaaaaaaa";                                    
       8 +    char *s = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
         +aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
         +aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
         +aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
         +aaaaaaaaaaaaaaaaaaaaaaa";                                      
       9      if (strlen(s) == 260) return 42;
      10      return 1;
      11  }

● Bash(python3 -c "
      content =
      open('test/valid/test_stage_95_09_long_string_literal__42.c').read()…)
  ⎿  String length: 260
     compiled: test/valid/test_stage_95_09_long_string_literal__42.c -> tes
     t_stage_95_09_long_string_literal__42.asm
     exit: 42
● The test works. Now let's run the full test suite again to confirm all 1479
  tests pass:

● Bash(./test/run_all_tests.sh 2>&1 | tail -5)
  ⎿  Results: 21 passed, 0 failed, 21 total

     ====================================================
     … +2 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● All 1479 tests pass. Now let me commit this and then run the self-host test.

  Let me also check the README.md note about MAX_NAME_LEN - the description
  there mentions ASTNode.value is still bounded by MAX_NAME_LEN, which needs
  updating.

  Read 1 file (ctrl+o to expand)

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      77  
      78  | Constant | Default | Description |
      79  |----------|---------|-------------|
      80 -| `MAX_NAME_LEN` | 256 | Maximum byte length of identifiers, ta
         -g names, assembly labels, and value strings stored in fixed-siz
         -e `char` arrays. Applies to `ASTNode.value` and struct name/tag
         - fields. `Token.value` is no longer bounded by this limit (it i
         -s a pointer+length into lexer-owned storage since Stage 95-08).
         - |                                                             
      80 +| `MAX_NAME_LEN` | 256 | Maximum byte length of identifiers, ta
         +g names, assembly labels, and value strings stored in fixed-siz
         +e `char` arrays. Applies to struct name/tag fields (`StructFiel
         +d.name`, `FuncSig.name`, etc.). `Token.value` (since Stage 95-0
         +8) and `ASTNode.value` (since Stage 95-09) are no longer bounde
         +d by this limit — both are pointers into lexer-owned storage. |
      81  
      82  ### AST
      83  

● Now update the "Through stage..." line and test totals in README.md:

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      221  
      222  ## What the compiler currently supports
      223  
      224 -Through stage 95-08 (updates to string handling in token):    
      224 +Through stage 95-09 (remove static char array from ASTNode and
          + parser):                                                     
      225  
      226  > Stage 95-08 eliminates the fixed-size `char value[MAX_NAME_L
           EN]` buffer from the `Token` struct, replacing it with pointer
           -plus-length fields (`const char *value`, `size_t value_len`, 
           `const char *lexeme`, `size_t lexeme_len`) that reference lexe
           r-owned storage. This removes the arbitrary 255-byte limitatio
           n on token text and string literals. The lexer implements a st
           ring pool (`Vec str_pool`) with a `lexer_store_bytes()` helper
            that allocates individual string copies and tracks them for c
           leanup. String literals are decoded through a temporary `StrBu
           f`, enabling strings beyond 255 bytes and supporting embedded 
           NUL bytes via `value_len`. Two bootstrap defects were found an
           d fixed: (1) C0 parser inability to parse `*(T**)expr` cast-de
           reference patterns (split into two statements); (2) signed div
           ision bug in `strbuf_append_char` overflow checks (same as sta
           ge 95-05's `vec_push` fix, rewritten using local `size_t` vari
           ables). All 1478 tests pass at C0, C1, and C2 (165 unit, 827 v
           alid, 237 invalid, 82 integration, 43 print_ast, 100 print_tok
           ens, 21 print_asm).
      227  

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      223  
      224  Through stage 95-09 (remove static char array from ASTNode and
            parser):
      225  
      226 -> Stage 95-08 eliminates the fixed-size `char value[MAX_NAME_L
          -EN]` buffer from the `Token` struct, replacing it with pointer
          --plus-length fields (`const char *value`, `size_t value_len`, 
          -`const char *lexeme`, `size_t lexeme_len`) that reference lexe
          -r-owned storage. This removes the arbitrary 255-byte limitatio
          -n on token text and string literals. The lexer implements a st
          -ring pool (`Vec str_pool`) with a `lexer_store_bytes()` helper
          - that allocates individual string copies and tracks them for c
          -leanup. String literals are decoded through a temporary `StrBu
          -f`, enabling strings beyond 255 bytes and supporting embedded 
          -NUL bytes via `value_len`. Two bootstrap defects were found an
          -d fixed: (1) C0 parser inability to parse `*(T**)expr` cast-de
          -reference patterns (split into two statements); (2) signed div
          -ision bug in `strbuf_append_char` overflow checks (same as sta
          -ge 95-05's `vec_push` fix, rewritten using local `size_t` vari
          -ables). All 1478 tests pass at C0, C1, and C2 (165 unit, 827 v
          -alid, 237 invalid, 82 integration, 43 print_ast, 100 print_tok
          -ens, 21 print_asm).                                           
      226 +> Stage 95-09 changes `ASTNode.value` from `char value[MAX_NAM
          +E_LEN]` (a fixed 256-byte embedded buffer) to `const char *val
          +ue` (a pointer into lexer-owned storage). All parser sites tha
          +t previously memcpy'd token bytes into the node's value buffer
          + are replaced with simple pointer assignments. String literal 
          +concatenation uses a StrBuf scratch buffer plus `lexer_store_b
          +ytes`. The `ParsedDeclarator.name` field is also changed to `c
          +onst char *` to avoid dangling pointers when identifier names 
          +are stored in AST nodes. Two bug fixes: case label values and 
          +enum-constant integer literals were formatted into local `char
          +[32]` buffers and passed directly to `parser_node`; these are 
          +now stored in the lexer pool via `lexer_store_bytes` first. Th
          +e `MAX_NAME_LEN` limit on `ASTNode.value` is removed; string l
          +iterals longer than 255 bytes are now handled correctly. All 1
          +479 tests pass (165 unit, 828 valid, 237 invalid, 82 integrati
          +on, 43 print_ast, 100 print_tokens, 21 print_asm).            
      227  
      228  Through stage 95-07 (convert remaining static usages):
      229  

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      585  ```
      586  
      587  The runner aggregates per-suite results and prints a final
      588 -`Aggregate: P passed, F failed, T total` line. As of stage 95-
          -08 all tests pass (165 unit, 827 valid, 237 invalid, 82 integr
          -ation, 43 print-AST, 100 print-tokens, 21 print-asm; 1478 tota
          -l).                                                           
      588 +`Aggregate: P passed, F failed, T total` line. As of stage 95-
          +09 all tests pass (165 unit, 828 valid, 237 invalid, 82 integr
          +ation, 43 print-AST, 100 print-tokens, 21 print-asm; 1479 tota
          +l).                                                           
      589  
      590  Individual suites can be run directly, e.g. `./test/valid/run_
           tests.sh`.
      591  

● Now update docs/fixed-capacity-inventory.md to mark ASTNode.value as done:

● Update(docs/fixed-capacity-inventory.md)
  ⎿  Added 1 line, removed 1 line
      18  
      19  | Name | Max | Module | On Overflow | Ext Ptr Refs | Safe Reall
          oc | Priority | Status |
      20  |------|-----|--------|-------------|--------------|-----------
          ---|----------|--------|
      21 -| `MAX_NAME_LEN` | 256 bytes | `include/constants.h`; used in `
         -char name[]`, `char tag[]`, `char label[]`, `char value[]`, `ch
         -ar static_label[]`, `char init_label[]` fields in `ASTNode`, `L
         -ocalVar`, `GlobalVar`, `LocalStaticVar`, `FuncSig`, `GlobalObjS
         -ig`, `TypedefEntry`, `EnumConst`, `EnumTag`, `StructTag`, `Unio
         -nTag`, `StructField`, and `ParsedDeclarator`. **`Token.value` w
         -as also bounded by this limit before stage 95-08; it is now a `
         -const char *` pointer+length into lexer-owned storage and no lo
         -nger subject to this limit.** | `strncpy` silently truncates to
         - 255 bytes; the identifier stored in the struct is silently wro
         -ng | No | N/A (embedded `char[]`) | LOW | PENDING |            
      21 +| `MAX_NAME_LEN` | 256 bytes | `include/constants.h`; used in `
         +char name[]`, `char tag[]`, `char label[]`, `char static_label[
         +]`, `char init_label[]` fields in `LocalVar`, `GlobalVar`, `Loc
         +alStaticVar`, `FuncSig`, `GlobalObjSig`, `TypedefEntry`, `EnumC
         +onst`, `EnumTag`, `StructTag`, `UnionTag`, `StructField`. **`To
         +ken.value` was migrated to a pointer+length into lexer-owned st
         +orage in stage 95-08. `ASTNode.value` was migrated to a `const 
         +char *` pointer into lexer-owned storage in stage 95-09 and is 
         +no longer subject to this limit.** | `strncpy` silently truncat
         +es to 255 bytes; the identifier stored in the struct is silentl
         +y wrong | No | N/A (embedded `char[]`) | LOW | PENDING (ASTNode
         +.value ✓ DONE stage 95-09) |                                   
      22  
      23  ---
      24  

● Also update the "remaining items" section:

● Update(docs/fixed-capacity-inventory.md)
  ⎿  Added 1 line, removed 1 line
      184  | `FUNC_TYPE_MAX_PARAMS` | Embedded `Type *params[16]` in `Typ
           e`; converting requires changing to a heap-allocated `Type **`
            and updating all type construction. NO Safe Realloc. |
      185  | `MAX_SWITCH_LABELS` | Embedded `ASTNode *nodes[256]` and `in
           t labels[256]` in `SwitchCtx`; converting requires heap-alloca
           ting arrays inside SwitchCtx and updating collect_switch_label
           s. NO Safe Realloc. |
      186  | `MAX_USER_LABELS` | 2D `char user_labels[64][256]` in CodeGe
           n; converting requires `char **` with per-label allocations. N
           O Safe Realloc. |
      187 -| `MAX_NAME_LEN` | Embedded `char name[256]` in every AST node
          -/var/sig struct; widest blast radius. `Token.value` was migrat
          -ed to pointer+length (stage 95-08) and is no longer bounded. R
          -emaining uses in `ASTNode`, `LocalVar`, `GlobalVar`, `FuncSig`
          -, etc. still require per-struct migration. N/A. |             
      187 +| `MAX_NAME_LEN` | Embedded `char name[256]` in `LocalVar`, `G
          +lobalVar`, `FuncSig`, and other parser/codegen structs. `Token
          +.value` was migrated to pointer+length (stage 95-08) and `ASTN
          +ode.value` to `const char *` (stage 95-09); both are no longer
          + bounded. Remaining uses in `LocalVar`, `GlobalVar`, `FuncSig`
          +, etc. still require per-struct migration. N/A. |             
      188  | `MAX_ARRAY_DIMS` | Local `#define` and stack variable in par
           ser.c; only affects nested array dimensions. N/A (stack). |
      189  | `MAX_INCLUDE_DEPTH` | Recursion depth counter in preprocesso
           r.c; not an array. N/A. |
      190  | `MAX_COND_DEPTH` | Local stack variable `CondFrame cond_stac
           k[64]` in preprocessor.c. N/A. |

● Now let's commit the stage implementation:

● Bash(git add include/ast.h include/lexer.h src/lexer.c src/ast.c
      src/ast_pretty_printer.c src/parser.c src/codegen.c src/version.c
      docs/fixed-capacity-inventory.md R…)
  ⎿  (No output)

● Bash(git status)
  ⎿  On branch master
     Your branch is ahead of 'origin/master' by 1 commit.
       (use "git push" to publish your local commits)
     … +15 lines (ctrl+o to expand)

● Bash(git commit -m "$(cat <<'EOF'
      stage 95-09: remove static char array from ASTNode and parser…)
  ⎿  [master dc5d5d9] stage 95-09: remove static char array from ASTNode an
     d parser
      12 files changed, 182 insertions(+), 58 deletions(-)
     … +3 lines (ctrl+o to expand)

● Now run the self-host test:

● Bash(./build.sh --mode=self-host 2>&1 | tail -40)
  ⎿  PASS  test_token_volatile
     PASS  test_token_while
     … +37 lines (ctrl+o to expand)
  ⎿  (timeout 10m)

● C0→C1→C2 bootstrap succeeds with all 1479 tests passing at each stage. Let me
  now update the self-compilation report:

  Read 1 file (ctrl+o to expand)

● Update(docs/self-compilation-report.md)
  ⎿  Added 19 lines
      136  
      137  After both fixes all 1478 tests passed at C0, C1, and C2.
      138  
      139 +## Issues found during stage 95-09 self-hosting test          
      140 +                                                              
      141 +None. The `ASTNode.value` pointer migration produced no bootst
          +rap failures.                                                 
      142 +All 1479 tests passed at C0, C1, and C2.                      
      143 +                                                              
      144 +## Result (stage 95-09)                                       
      145 +                                                              
      146 +**Date:** 2026-06-07                                          
      147 +                                                              
      148 +| Step | Binary | Version | BuiltBy | Tests |                 
      149 +|------|--------|---------|---------|-------|                 
      150 +| C0 | `build/ccompiler-c0` | `00.02.00950900.00741` | `gcc_Ub
          +untu_13_3_0` | 1479/1479 |                                    
      151 +| C1 | `build/ccompiler-c1` | `00.02.00950900.00742` | `Claude
          +C99_v00_02_00950900_00741` | 1479/1479 |                      
      152 +| C2 | `build/ccompiler-c2` | `00.02.00950900.00743` | `Claude
          +C99_v00_02_00950900_00742` | 1479/1479 |                      
      153 +                                                              
      154 +C0, C1, and C2 each compile successfully with distinct version
          + strings and                                                  
      155 +full build provenance. The compiler is self-hosting and the bo
          +otstrap is                                                    
      156 +reproducible.                                                 
      157 +                                                              
      158  ## Result (stage 95-08)
      159  
      160  | Step | Binary | Version | BuiltBy | Tests |

● Now delegate the milestone summary, transcript, and grammar check to haiku:

● haiku-stage-artifact-writer(Generate stage-95-09 milestone and transcript
                             artifacts)
  ⎿  Done (6 tool uses · 11.8k tokens · 34s)
  (ctrl+o to expand)

● Let me commit the remaining artifacts:

● Bash(git add docs/self-compilation-report.md
      docs/milestones/stage-95-09-milestone.md
      docs/sessions/stage-95-09-transcript.md && git commit -m "$(cat <<'EOF'
      stage 9…)  ⎿  [master 0bb0624] stage 95-09: self-host report, milestone, and transcr
     ipt
      3 files changed, 114 insertions(+)
     … +2 lines (ctrl+o to expand)

● Stage 95-09 is complete. Here's what was done:

  Core change: ASTNode.value changed from char value[MAX_NAME_LEN] (a fixed
  256-byte embedded buffer) to const char *value (a pointer into lexer-owned
  storage).

  Key changes across the codebase:
  - include/ast.h / src/ast.c — struct field change + ast_new uses pointer
  assignment
  - include/lexer.h / src/lexer.c — lexer_store_bytes exposed (was static)
  - src/parser.c — ParsedDeclarator.name also made const char *; all
  memcpy/strncpy-to-node-value replaced with direct pointer assignments or
  lexer_store_bytes; strbuf used for concatenated string literals
  - src/codegen.c + src/ast_pretty_printer.c — NULL guards for AST_ASSIGNMENT
  nodes (which are created with NULL value for complex LHS expressions)

  Two latent bugs fixed: case label values and enum-constant integer literals
  were formatted into local char[32] buffers and passed directly to parser_node
  — causing use-after-stack-free reads in codegen. All 17 switch tests were
  failing because of this.

  Result: 1479 tests pass at C0, C1, and C2 (added 1 new test for >255-byte
  string literals). No bootstrap failures.

✻ Worked for 19m 50s

❯ what static items remain                                                      

  Read 1 file (ctrl+o to expand)

● Seven items remain, all LOW priority and requiring structural refactoring:

  Item: MAX_NAME_LEN
  Cap: 256
  What it guards: char name[256] in LocalVar, GlobalVar, FuncSig, GlobalObjSig,
    TypedefEntry, EnumConst/Tag, StructTag, UnionTag, StructField
  Why it's hard: Wide blast radius; embedded in many structs that are stored by
    value in Vec; migrating requires const char *name + lexer-pool sourcing
    across all those structs
  ────────────────────────────────────────
  Item: FUNC_MAX_PARAMS
  Cap: 16
  What it guards: TypeKind param_types[16] in FuncSig
  Why it's hard: Embedded in FuncSig (itself in a Vec); would need TypeKind *
    heap allocation
  ────────────────────────────────────────
  Item: FUNC_TYPE_MAX_PARAMS
  Cap: 16
  What it guards: Type *params[16] in Type
  Why it's hard: Embedded in heap-allocated Type; changing layout breaks all
  type
    construction
  ────────────────────────────────────────
  Item: MAX_SWITCH_LABELS
  Cap: 256
  What it guards: ASTNode *nodes[256] + int labels[256] in SwitchCtx
  Why it's hard: Both arrays embedded in SwitchCtx (in a Vec); need
  per-SwitchCtx
    heap alloc
  ────────────────────────────────────────
  Item: MAX_USER_LABELS
  Cap: 64
  What it guards: char user_labels[64][256] in CodeGen
  Why it's hard: 2D char array; dynamic form needs char ** + per-label alloc
  ────────────────────────────────────────
  Item: MAX_ARRAY_DIMS
  Cap: 8
  What it guards: Local stack array in parser.c
  Why it's hard: Stack-only; low practical risk
  ────────────────────────────────────────
  Item: MAX_COND_DEPTH
  Cap: 64
  What it guards: Local CondFrame cond_stack[64] in preprocessor.c
  Why it's hard: Stack-only; low practical risk

  The two most impactful remaining items are MAX_NAME_LEN (all the name buffers
  in codegen/parser structs) and MAX_SWITCH_LABELS (hits for switch statements
  with many cases). The preprocessor/array-dims ones are stack-local and present
  no data-structure risk.

✻ Baked for 19s

❯ what is still using MAX_NAME_LEN                                              

  Searched for 1 pattern (ctrl+o to expand)

● Grouped by category:

  Struct fields (persistent, bounded by 255)

  Struct: LocalVar
  Field: name[MAX_NAME_LEN]
  Location: codegen.h:10
  Notes: local variable name
  ────────────────────────────────────────
  Struct: LocalVar
  Field: static_label[MAX_NAME_LEN]
  Location: codegen.h:28
  Notes: .bss/.data label for block-scope statics
  ────────────────────────────────────────
  Struct: LocalStaticVar
  Field: label[MAX_NAME_LEN]
  Location: codegen.h:35
  Notes: same family
  ────────────────────────────────────────
  Struct: GlobalVar
  Field: name[MAX_NAME_LEN]
  Location: codegen.h:51
  Notes: global variable name
  ────────────────────────────────────────
  Struct: GlobalVar
  Field: init_label[MAX_NAME_LEN]
  Location: codegen.h:61
  Notes: Lstr<N> or function name for pointer inits
  ────────────────────────────────────────
  Struct: CodeGen
  Field: user_labels[MAX_USER_LABELS][MAX_NAME_LEN]
  Location: codegen.h:119
  Notes: goto label names (2D array)
  ────────────────────────────────────────
  Struct: FuncSig
  Field: name[MAX_NAME_LEN]
  Location: parser.h:12
  Notes: function name
  ────────────────────────────────────────
  Struct: EnumConst
  Field: name[MAX_NAME_LEN]
  Location: parser.h:17
  Notes: enum constant name
  ────────────────────────────────────────
  Struct: EnumTag
  Field: tag[MAX_NAME_LEN]
  Location: parser.h:23
  Notes: enum tag name
  ────────────────────────────────────────
  Struct: StructTag
  Field: tag[MAX_NAME_LEN]
  Location: parser.h:29
  Notes: struct tag name
  ────────────────────────────────────────
  Struct: UnionTag
  Field: tag[MAX_NAME_LEN]
  Location: parser.h:35
  Notes: union tag name
  ────────────────────────────────────────
  Struct: TypedefEntry
  Field: name[MAX_NAME_LEN]
  Location: parser.h:42
  Notes: typedef name
  ────────────────────────────────────────
  Struct: GlobalObjSig
  Field: name[MAX_NAME_LEN]
  Location: parser.h:60
  Notes: file-scope object name
  ────────────────────────────────────────
  Struct: StructField
  Field: name[MAX_NAME_LEN]
  Location: type.h:41
  Notes: struct/union member name

  Local stack buffers (function-scoped, harmless)

  ┌─────────────────────────────────┬───────────────────────────────────────┐
  │            Location             │                Purpose                │
  ├─────────────────────────────────┼───────────────────────────────────────┤
  │ lexer.c:528                     │ scratch buffer while scanning         │
  │ ident_buf[MAX_NAME_LEN]         │ identifier chars                      │
  ├─────────────────────────────────┼───────────────────────────────────────┤
  │ parser.c:391,596,760 tag[256]   │ struct/union/enum tag while parsing   │
  │                                 │ body                                  │
  ├─────────────────────────────────┼───────────────────────────────────────┤
  │ parser.c:803 ename[256]         │ enum constant name while parsing      │
  ├─────────────────────────────────┼───────────────────────────────────────┤
  │ codegen.c:3999 label[256]       │ goto label name while emitting        │
  ├─────────────────────────────────┼───────────────────────────────────────┤
  │ preprocessor.c:130 tmp[256]     │ macro name scratch                    │
  ├─────────────────────────────────┼───────────────────────────────────────┤
  │ preprocessor.c:1364             │ include filename scratch              │
  │ fname_buf[256]                  │                                       │
  ├─────────────────────────────────┼───────────────────────────────────────┤
  │ version.c:22 version_buf[256]   │ formatted version string (unrelated)  │
  └─────────────────────────────────┴───────────────────────────────────────┘

  The 14 struct fields are the real work. The biggest cluster is the
  codegen/parser structs that store names — each would need a const char *name
  pointing to lexer-pool storage, similar to what was done for ASTNode.value and
  ParsedDeclarator.name in stage 95-09.

✻ Brewed for 19s