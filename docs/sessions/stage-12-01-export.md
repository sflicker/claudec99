 ▐▛███▜▌   Claude Code v2.1.119
▝▜█████▛▘  Opus 4.7 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

❯ /usage 
  ⎿  Status dialog dismissed

❯ Unknown command: /pricing                                                     

❯ /implement-stage ClaudeC99-spec-stage-stage-12-01-pointer-types.md            

  Read 4 files (ctrl+o to expand)

● I'll note that the spec filename has a duplicate stage token
  (...stage-stage-12-01...). Strictly applying the rules, the first stage is
  followed by another non-numeric stage, but the second stage is followed by
  12-01. The clear intent is stage-12-01.

  STAGE_DISPLAY: Stage-12-01

  Let me explore the project structure to understand the existing code.

  Read 2 files, listed 4 directories (ctrl+o to expand)

  Read 19 files, listed 4 directories (ctrl+o to expand)

● I have enough context. Let me echo the derived stage label and produce the
  kickoff.

  STAGE_DISPLAY: Stage-12-01

  Kickoff Summary

  Spec Summary

  Stage 12-01 introduces internal representation for pointer types in the type
  system. The compiler must accept pointer variable declarations (char *, short 
  *, int *, long *, plus arbitrary nesting like int **p, int ***t), allocate 8
  bytes per pointer local on x86-64, and continue to handle every pre-existing
  integer declaration without regression. The address-of (&), dereference (*p),
  pointer arithmetic, comparisons, arrays, and pointer-typed parameters/returns
  are all explicitly out of scope. Pointer initializers are allowed only insofar
   as the existing expression/type system already supports them.

  Spec Notes / Ambiguities

  1. Filename has duplicated stage token (...stage-stage-12-01...). I derived
  STAGE_LABEL=stage-12-01 from the second occurrence, since that's where the
  numeric tokens follow. Worth flagging since the strict letter of the rule says
   no numeric tokens follow the first stage.
  2. Initializer semantics are loose. "may be initialized only if the existing
  expression/type system can already support the initializer" — taken literally,
   an initializer expression is parsed and stored using the existing init path;
  no new pointer-vs-integer type checking is added. With no & operator, the only
   practical initializer would be an integer expression.
  3. AST representation question. ASTNode currently stores decl_type as a
  TypeKind, which cannot represent the pointer base type. The spec is silent on
  the AST field, but does require a Type *base field on Type. Minimum-viable
  plan: add a Type *full_type parallel field to ASTNode (NULL for non-pointer
  decls).
  4. Pretty-printer format for pointers isn't specified. I'll use standard C
  notation: VariableDeclaration: int *p, VariableDeclaration: int **s, etc.

  What changes from Stage 11-07

  Currently Type only models four integer kinds and ASTNodes carry only a
  TypeKind for declared type. The parser hands a single integer keyword to
  AST_DECLARATION. After this stage:
  - TypeKind gains TYPE_POINTER; Type gains a Type *base field; a new
  type_pointer(Type *base) factory builds chains.
  - The declaration parser accepts a sequence of * tokens after the integer base
   type and wraps the type accordingly.
  - ASTNode gains a Type *full_type slot used by pointer declarations to carry
  the full chain.
  - Codegen treats TYPE_POINTER as 8 bytes for stack allocation; integer paths
  are unchanged.
  - The pretty printer prints pointer types like int *, int **, etc.

  Planned Changes

  1. Tokenizer — no changes. * is already TOKEN_STAR.
  2. Type system (include/type.h, src/type.c)
    - Add TYPE_POINTER to TypeKind.
    - Add Type *base to the Type struct.
    - Add Type *type_pointer(Type *base) (heap-allocated; size=8, alignment=8,
  is_signed=0).
    - Update type_kind_name to return "pointer" for TYPE_POINTER.
    - Update type_is_integer to return 0 for TYPE_POINTER.
  3. AST (include/ast.h)
    - Add struct Type *full_type to ASTNode (NULL except for pointer
  declarations).
  4. Parser (src/parser.c)
    - In the declaration branch of parse_statement: after the integer keyword,
  consume zero-or-more TOKEN_STAR tokens, wrapping a Type * chain. If at least
  one * was seen, set decl_type = TYPE_POINTER and full_type = <chain>.
    - Function parameters and return types remain integer-only (out of scope).
  5. Code generator (src/codegen.c)
    - type_kind_bytes(TYPE_POINTER) returns 8.
  6. Pretty printer (src/ast_pretty_printer.c)
    - For AST_DECLARATION with decl_type == TYPE_POINTER, walk full_type and
  print the base type followed by the right number of * characters; otherwise
  unchanged.
  7. Tests
    - test/valid/: add minimal pointer-decl tests for char *, short *, int *,
  long *, plus int ** and int ***. Each returns 0.
    - test/print_ast/: add test_stage_12_01_pointer_types.c and matching
  .expected covering the spec examples.
  8. Documentation
    - Update docs/grammar.md: <declaration> production to use a new <type>
  non-terminal with optional * repetition.
  9. Kickoff/milestone/transcript artifacts under docs/kickoffs/,
  docs/milestones/, docs/sessions/.
  10. Commit with a message describing the stage.

  I'll now save the kickoff file, then pause for your confirmation before
  beginning implementation.

● Write(docs/kickoffs/stage-12-01-kickoff.md)
  ⎿  Wrote 82 lines to docs/kickoffs/stage-12-01-kickoff.md
       2 # Stage-12-01 Kickoff: Pointer Types
       3 ## Spec Summary
       4 Stage 12-01 introduces internal representation for pointer types
       5 compiler. The type system gains a pointer kind that records its
       6 referenced base type, and the parser accepts pointer variable
       7 declarations using the existing integer base types — `char *`,
         ,short *`, `int *`, `long *` — with arbitrary nesting (`int **p`
       9 `int ***t`). Pointer locals reserve 8 bytes on x86-64. Pointer
      10 initializers are allowed only insofar as the existing expression
         /type
     … +77 lines (ctrl+o to expand)

● Kickoff saved to docs/kickoffs/stage-12-01-kickoff.md.

  Pausing here. Should I proceed with the type-system changes (TYPE_POINTER + 
  Type *base + type_pointer factory)?

✻ Churned for 3m 22s

❯ continue                                                                      

● Proceeding with the type system changes.

● Update(include/type.h)
  ⎿  Added 4 lines, removed 1 line
      14      TYPE_CHAR,
      15      TYPE_SHORT,
      16      TYPE_INT,
      17 -    TYPE_LONG                                                  
      17 +    TYPE_LONG,                                                 
      18 +    TYPE_POINTER                                               
      19  } TypeKind;
      20  
      21  typedef struct Type {
     ...
      23      int size;
      24      int alignment;
      25      int is_signed;
      26 +    struct Type *base;                                         
      27  } Type;
      28  
      29  Type *type_char(void);
      30  Type *type_short(void);
      31  Type *type_int(void);
      32  Type *type_long(void);
      33 +Type *type_pointer(Type *base);                                
      34  
      35  const char *type_kind_name(TypeKind kind);
      36  int type_size(Type *t);

● Update(src/type.c)
  ⎿  Added 32 lines, removed 8 lines
      1  #include <stddef.h>
      2 +#include <stdio.h>                                              
      3 +#include <stdlib.h>                                             
      4  #include "type.h"
      5  
      6  /*
     ...
       9   * any of them yet.
      10   */
      11  
      10 -static Type type_char_singleton  = { TYPE_CHAR,  1, 1, 1 };    
      11 -static Type type_short_singleton = { TYPE_SHORT, 2, 2, 1 };    
      12 -static Type type_int_singleton   = { TYPE_INT,   4, 4, 1 };    
      13 -static Type type_long_singleton  = { TYPE_LONG,  8, 8, 1 };    
      12 +static Type type_char_singleton  = { TYPE_CHAR,  1, 1, 1, NULL 
         +};                                                             
      13 +static Type type_short_singleton = { TYPE_SHORT, 2, 2, 1, NULL 
         +};                                                             
      14 +static Type type_int_singleton   = { TYPE_INT,   4, 4, 1, NULL 
         +};                                                             
      15 +static Type type_long_singleton  = { TYPE_LONG,  8, 8, 1, NULL 
         +};                                                             
      16  
      17  Type *type_char(void)  { return &type_char_singleton;  }
      18  Type *type_short(void) { return &type_short_singleton; }
      19  Type *type_int(void)   { return &type_int_singleton;   }
      20  Type *type_long(void)  { return &type_long_singleton;  }
      21  
      22 +/*                                                             
      23 + * Stage 12-01: heap-allocate a pointer Type that wraps `base`.
      24 + * Pointer chains can nest arbitrarily, so a static singleton  
      25 + * approach is not used here. The compiler does not free Types.
      26 + */                                                            
      27 +Type *type_pointer(Type *base) {                               
      28 +    Type *t = calloc(1, sizeof(Type));                         
      29 +    if (!t) {                                                  
      30 +        fprintf(stderr, "error: out of memory\n");             
      31 +        exit(1);                                               
      32 +    }                                                          
      33 +    t->kind = TYPE_POINTER;                                    
      34 +    t->size = 8;                                               
      35 +    t->alignment = 8;                                          
      36 +    t->is_signed = 0;                                          
      37 +    t->base = base;                                            
      38 +    return t;                                                  
      39 +}                                                              
      40 +                                                               
      41  const char *type_kind_name(TypeKind kind) {
      42      switch (kind) {
      22 -    case TYPE_CHAR:  return "char";                            
      23 -    case TYPE_SHORT: return "short";                           
      24 -    case TYPE_INT:   return "int";                             
      25 -    case TYPE_LONG:  return "long";                            
      43 +    case TYPE_CHAR:    return "char";                          
      44 +    case TYPE_SHORT:   return "short";                         
      45 +    case TYPE_INT:     return "int";                           
      46 +    case TYPE_LONG:    return "long";                          
      47 +    case TYPE_POINTER: return "pointer";                       
      48      }
      49      return "unknown";
      50  }
     ...
      65      case TYPE_INT:
      66      case TYPE_LONG:
      67          return 1;
      68 +    case TYPE_POINTER:                                         
      69 +        return 0;                                              
      70      }
      71      return 0;
      72  }

● Type system updates complete. Pausing.

  Should I proceed with the AST and parser changes?

❯ yes                                                                           

● Update(include/ast.h)
  ⎿  Added 4 lines
      42      int child_count;
      43      TypeKind decl_type;
      44      TypeKind result_type;
      45 +    /* Stage 12-01: full Type chain for pointer declarations. N
         +ULL for                                                        
      46 +     * non-pointer nodes; for AST_DECLARATION with decl_type ==
      47 +     * TYPE_POINTER, points to the head of the pointer Type cha
         +in. */                                                         
      48 +    struct Type *full_type;                                    
      49  } ASTNode;
      50  
      51  ASTNode *ast_new(ASTNodeType type, const char *value);

● Now the parser. Update the declaration branch in parse_statement to consume *
  after the integer base type and build a pointer Type chain.

● Update(src/parser.c)
  ⎿  Added 26 lines, removed 7 lines
      572          parser->lexer->pos = saved_pos;
      573          parser->current = saved_token;
      574      }
      575 -    /* declaration: <integer-type> <identifier> [ "=" <express
          -ion> ] ";" */                                                 
      575 +    /* declaration: <type> <identifier> [ "=" <expression> ] "
          +;"                                                            
      576 +     * <type>     ::= <integer_type> { "*" }                  
      577 +     *                                                        
      578 +     * Stage 12-01: zero or more '*' tokens after the integer 
          +base type                                                     
      579 +     * wrap the type in a pointer Type chain. When at least on
          +e '*' is                                                      
      580 +     * consumed, decl_type becomes TYPE_POINTER and full_type 
          +carries                                                       
      581 +     * the head of the chain. */                              
      582      if (parser->current.type == TOKEN_CHAR ||
      583          parser->current.type == TOKEN_SHORT ||
      584          parser->current.type == TOKEN_INT ||
      585          parser->current.type == TOKEN_LONG) {
      580 -        TypeKind kind;                                        
      586 +        TypeKind base_kind;                                   
      587 +        Type *base_type;                                      
      588          switch (parser->current.type) {
      582 -        case TOKEN_CHAR:  kind = TYPE_CHAR;  break;           
      583 -        case TOKEN_SHORT: kind = TYPE_SHORT; break;           
      584 -        case TOKEN_LONG:  kind = TYPE_LONG;  break;           
      585 -        default:          kind = TYPE_INT;   break;           
      589 +        case TOKEN_CHAR:  base_kind = TYPE_CHAR;  base_type = 
          +type_char();  break;                                          
      590 +        case TOKEN_SHORT: base_kind = TYPE_SHORT; base_type = 
          +type_short(); break;                                          
      591 +        case TOKEN_LONG:  base_kind = TYPE_LONG;  base_type = 
          +type_long();  break;                                          
      592 +        default:          base_kind = TYPE_INT;   base_type = 
          +type_int();   break;                                          
      593          }
      594          parser->current = lexer_next_token(parser->lexer);
      595 +        Type *full_type = base_type;                          
      596 +        int pointer_levels = 0;                               
      597 +        while (parser->current.type == TOKEN_STAR) {          
      598 +            full_type = type_pointer(full_type);              
      599 +            pointer_levels++;                                 
      600 +            parser->current = lexer_next_token(parser->lexer);
      601 +        }                                                     
      602          Token name = parser_expect(parser, TOKEN_IDENTIFIER);
      603          ASTNode *decl = ast_new(AST_DECLARATION, name.value);
      590 -        decl->decl_type = kind;                               
      604 +        if (pointer_levels > 0) {                             
      605 +            decl->decl_type = TYPE_POINTER;                   
      606 +            decl->full_type = full_type;                      
      607 +        } else {                                              
      608 +            decl->decl_type = base_kind;                      
      609 +        }                                                     
      610          if (parser->current.type == TOKEN_ASSIGN) {
      611              parser->current = lexer_next_token(parser->lexer);
      612              ASTNode *init = parse_expression(parser);

● AST and parser changes complete. Pausing.

  Should I proceed with the codegen and pretty-printer changes?

❯ yes                                                                           
● Update(src/codegen.c)
  ⎿  Added 5 lines, removed 4 lines
       6  
       7  static int type_kind_bytes(TypeKind kind) {
       8      switch (kind) {
       9 -    case TYPE_CHAR:  return 1;                                 
      10 -    case TYPE_SHORT: return 2;                                 
      11 -    case TYPE_INT:   return 4;                                 
      12 -    case TYPE_LONG:  return 8;                                 
       9 +    case TYPE_CHAR:    return 1;                               
      10 +    case TYPE_SHORT:   return 2;                               
      11 +    case TYPE_INT:     return 4;                               
      12 +    case TYPE_LONG:    return 8;                               
      13 +    case TYPE_POINTER: return 8;                               
      14      }
      15      return 4;
      16  }

● Now the pretty printer.

● Update(src/ast_pretty_printer.c)
  ⎿  Added 16 lines
      28      }
      29  }
      30  
      31 +/*                                                             
      32 + * Stage 12-01: print a Type chain in C-style notation. The bas
         +e                                                              
      33 + * integer type prints with a trailing space; each pointer leve
         +l                                                              
      34 + * appends a '*' (no separating space), so `int **` reads as   
      35 + * "int **" before the variable name.                          
      36 + */                                                            
      37 +static void ast_print_type(Type *t) {                          
      38 +    if (!t) return;                                            
      39 +    if (t->kind == TYPE_POINTER) {                             
      40 +        ast_print_type(t->base);                               
      41 +        printf("*");                                           
      42 +    } else {                                                   
      43 +        printf("%s ", type_kind_name(t->kind));                
      44 +    }                                                          
      45 +}                                                              
      46 +                                                               
      47  void ast_pretty_print(ASTNode *node, int depth) {
      48      if (!node) return;
      49  

● Update(src/ast_pretty_printer.c)
  ⎿  Added 8 lines, removed 2 lines
      65          printf("Block\n");
      66          break;
      67      case AST_DECLARATION:
      68 -        printf("VariableDeclaration: %s %s\n",                 
      69 -               type_kind_name(node->decl_type), node->value);  
      68 +        if (node->decl_type == TYPE_POINTER && node->full_type)
         + {                                                             
      69 +            printf("VariableDeclaration: ");                   
      70 +            ast_print_type(node->full_type);                   
      71 +            printf("%s\n", node->value);                       
      72 +        } else {                                               
      73 +            printf("VariableDeclaration: %s %s\n",             
      74 +                   type_kind_name(node->decl_type), node->value
         +);                                                             
      75 +        }                                                      
      76          break;
      77      case AST_RETURN_STATEMENT:
      78          printf("ReturnStmt:\n");

● Now let me build the compiler to confirm everything compiles cleanly.

● Bash(cd /home/scott/code/claude/claudec99/build && cmake --build . 2>&1 | tail
       -40)
  ⎿  Consolidate compiler generated dependencies of target ccompiler
     [ 11%] Building C object CMakeFiles/ccompiler.dir/src/compiler.c.o
     [ 22%] Building C object CMakeFiles/ccompiler.dir/src/ast.c.o
     … +8 lines (ctrl+o to expand)

● Clean build. Run the existing test suites to confirm no regressions before
  adding new tests.

● Bash(cd /home/scott/code/claude/claudec99 && bash test/valid/run_tests.sh 2>&1
       | tail -10 && echo "---PRINT_AST---" && bash test/print_ast/run_tests.sh
      2>&1 | tail -…)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_while_long
     _counter__10.c -> test_while_long_counter__10.asm
     PASS  test_while_long_counter__10  (exit code: 10)
     … +19 lines (ctrl+o to expand)

● All 210 existing tests still pass (183 valid + 13 print_ast + 14 invalid).
  Codegen and pretty-printer changes complete. Pausing.

  Should I proceed with adding the new pointer tests?

❯ yes                                                                           

  Read 1 file, listed 1 directory (ctrl+o to expand)

● Now I'll create the pointer tests.

● Write(test/valid/test_pointer_int_decl__0.c)
  ⎿  Wrote 4 lines to test/valid/test_pointer_int_decl__0.c
      1 int main() {
      2     int *p;
      3     return 0;
      4 }

● Write(test/valid/test_pointer_char_decl__0.c)
  ⎿  Wrote 4 lines to test/valid/test_pointer_char_decl__0.c
      1 int main() {
      2     char *p;
      3     return 0;
      4 }

● Write(test/valid/test_pointer_short_decl__0.c)
  ⎿  Wrote 4 lines to test/valid/test_pointer_short_decl__0.c
      1 int main() {
      2     short *p;
      3     return 0;
      4 }

● Write(test/valid/test_pointer_long_decl__0.c)
  ⎿  Wrote 4 lines to test/valid/test_pointer_long_decl__0.c
      1 int main() {
      2     long *p;
      3     return 0;
      4 }

● Write(test/valid/test_pointer_double_decl__0.c)
  ⎿  Wrote 4 lines to test/valid/test_pointer_double_decl__0.c
      1 int main() {
      2     int **s;
      3     return 0;
      4 }

● Write(test/valid/test_pointer_triple_decl__0.c)
  ⎿  Wrote 4 lines to test/valid/test_pointer_triple_decl__0.c
      1 int main() {
      2     int ***t;
      3     return 0;
      4 }

● Write(test/valid/test_pointer_with_int_local__0.c)
  ⎿  Wrote 6 lines to test/valid/test_pointer_with_int_local__0.c
      1 int main() {
      2     int x;
      3     int *p;
      4     int **q;
      5     return 0;
      6 }

● Now the print_ast test.

● Write(test/print_ast/test_stage_12_01_pointer_types.c)
  ⎿  Wrote 9 lines to test/print_ast/test_stage_12_01_pointer_types.c
      1 int main() {
      2     int *p;
      3     char *c;
      4     short *s;
      5     long *l;
      6     int **q;
      7     int ***t;
      8     return 0;
      9 }

● Write(test/print_ast/test_stage_12_01_pointer_types.expected)
  ⎿  Wrote 11 lines to
     test/print_ast/test_stage_12_01_pointer_types.expected
       1 TranslationUnit:
       2   FunctionDecl: int main
       3     Block
       4       VariableDeclaration: int *p
       5       VariableDeclaration: char *c
       6       VariableDeclaration: short *s
       7       VariableDeclaration: long *l
       8       VariableDeclaration: int **q
       9       VariableDeclaration: int ***t
      10       ReturnStmt:
     … +1 line (ctrl+o to expand)

● Bash(bash test/valid/run_tests.sh 2>&1 | grep -E "(pointer|Results:)" | head
      -20 && echo "---PRINT_AST---" && bash test/print_ast/run_tests.sh 2>&1 |
      grep -E "(stage…)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_pointer_ch
     ar_decl__0.c -> test_pointer_char_decl__0.asm
     PASS  test_pointer_char_decl__0  (exit code: 0)
     … +16 lines (ctrl+o to expand)
● Bash(bash test/invalid/run_tests.sh 2>&1 | tail -3)
  ⎿  PASS  test_invalid_9__call_to_undefined_function  (error contains: 'ca
     ll to undefined function')

     Results: 14 passed, 0 failed, 14 total

● All 218 tests pass (190 valid + 14 print_ast + 14 invalid). 7 new valid
  pointer-decl tests + 1 new print_ast test, no regressions. Pausing.

  Should I proceed with the documentation update (docs/grammar.md)?

❯ yes                                                                           

● Update(docs/grammar.md)
  ⎿  Added 3 lines, removed 1 line
      25                      | <jump_statement>
      26                      | <expression_statement>
      27  
      28 -<declaration> ::= <integer-type> <identifier> [ "=" <expression
         -> ] ";"                                                        
      28 +<declaration> ::= <type> <identifier> [ "=" <expression> ] ";" 
      29  
      30 +<type> ::= <integer_type> { "*" }                              
      31 +                                                               
      32  <integer_type> ::= "char" | "short" | "int" | "long"
      33  
      34  <return_statement> ::= "return" <expression> ";"

● Documentation updated. Pausing.

  Should I write the milestone summary and transcript, then commit the changes?

❯ yes                                                                           

● Write(docs/milestones/stage-12-01-summary.md)
  ⎿  Wrote 47 lines to docs/milestones/stage-12-01-summary.md
       2 # Stage-12-01 Milestone: Pointer Types
       3 The compiler's type system now represents pointer types and the 
       4 accepts pointer variable declarations using the existing integer
          base
       6 nesting (`int **p`, `int ***t`). Pointer locals reserve 8 bytes 
         on
         ,86-64. Address-of, dereference, pointer arithmetic, comparisons
       8 arrays, and pointer-typed parameters/returns remain out of scope
       9 .
      10 ## What was completed
     … +39 lines (ctrl+o to expand)

● Write(docs/sessions/stage-12-01-transcript.md)
  ⎿  Wrote 128 lines to docs/sessions/stage-12-01-transcript.md
       2 # stage-12-01 Transcript: Pointer Types
       3 ## Summary
       4 
         terge 12-01 extends the compiler's type system to represent poin
       6 types. A new `TYPE_POINTER` kind and a `Type *base` field on the
       7 `Type` struct allow nested pointer chains; a `type_pointer` fact
       8 constructs them on the heap so arbitrary nesting (`int ***t`) is
       9 supported. The declaration parser accepts zero or more `*` token
         s
      10 between the integer base type and the variable name, wrapping a
     … +120 lines (ctrl+o to expand)

● Now the commit. Let me check the git status and review the planned commit
  message.

● Bash(git status && echo "---" && git log -3 --oneline)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +24 lines (ctrl+o to expand)

● Bash(git diff docs/stages/ClaudeC99-spec-stage-stage-12-01-pointer-types.md |
      head -30)
  ⎿  diff --git a/docs/stages/ClaudeC99-spec-stage-stage-12-01-pointer-type
     s.md b/docs/stages/ClaudeC99-spec-stage-stage-12-01-pointer-types.md
     index e69de29..258948a 100644
     … +20 lines (ctrl+o to expand)

● Bash(git add include/type.h include/ast.h src/type.c src/parser.c
      src/codegen.c src/ast_pretty_printer.c docs/grammar.md
      docs/stages/ClaudeC99-spec-stage-stage-12-01…)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +19 lines (ctrl+o to expand)

● Bash(git commit -m "$(cat <<'EOF'
      stage-12-01: pointer types…)
  ⎿  [master 7c5e4f0] stage-12-01: pointer types
      20 files changed, 480 insertions(+), 23 deletions(-)
      create mode 100644 docs/kickoffs/stage-12-01-kickoff.md
     … +13 lines (ctrl+o to expand)

● Stage-12-01 complete. Commit 7c5e4f0.

  Milestone Summary

  Pointer types are now represented internally: TypeKind gains TYPE_POINTER, the
   Type struct gains a base pointer, and a type_pointer factory builds nested
  chains. The declaration parser accepts <integer_type> { "*" }, codegen
  reserves 8 bytes per pointer local, and the pretty printer renders pointer
  declarations in standard C notation. All 218 tests pass (190 valid + 14
  invalid + 14 print_ast) — 7 new pointer-decl tests and 1 new print_ast test,
  no regressions.

  Artifacts: docs/kickoffs/stage-12-01-kickoff.md,
  docs/milestones/stage-12-01-summary.md,
  docs/sessions/stage-12-01-transcript.md.

✻ Brewed for 1m 9s