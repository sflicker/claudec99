 ▐▛███▜▌   Claude Code v2.1.126
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

❯ /implement-stage                                                              
docs/stages/ClaudeC99-spec-stage-20-01-declarator-refactor.md                  

● I'll read the spec and supporting files to get started.

  Read 3 files (ctrl+o to expand)

● Now let me read the templates and current source files in parallel.

  Read 3 files, listed 1 directory (ctrl+o to expand)

● Now let me read the key implementation files to understand the current state.

  Read 3 files (ctrl+o to expand)

● Now let me read the parser implementation to understand what needs to change.

  Read 1 file (ctrl+o to expand)

● Let me also look at the codegen to understand how it uses AST_CAST and type
  information.

  Searched for 1 pattern (ctrl+o to expand)

● Let me also check the sizeof section in codegen more carefully.

  Searched for 1 pattern (ctrl+o to expand)

● Let me check the tests to understand what needs to remain passing.

  Listed 2 directories (ctrl+o to expand)

● Bash(make 2>&1 | tail -5)
  ⎿  Error: make: *** No targets specified and no makefile found.  Stop.

● Bash(./test/valid/run_tests.sh 2>&1 | tail -10)
  ⎿  Results: 376 passed, 0 failed, 376 total

● STAGE_DISPLAY: Stage 20-01

  ---
  Stage 20-01 Summary

  This is a pure parser refactor — no AST node changes, no codegen changes.
  The goal is:

  1. Split <type> into <type_specifier> (base keyword only) and <type_name>
     (base + optional stars, for nameless contexts like casts and sizeof).
  2. Introduce <declarator> grammar productions that hold the pointer stars,
     identifier, and optional array bracket suffix in named declarations.
  3. Add parse_type_specifier, parse_type_name, and parse_declarator helpers
     in the parser.
  4. All existing 376 valid tests must continue to pass.

  ---
  Issues / Ambiguities

  None. One notable behavior change enabled by the grammar update: casts with
  pointer types (e.g., (int*)x) were silently unparseable before because
  parse_cast never consumed '*' after the type keyword. The new <type_name>
  grammar makes this work. No existing tests break — this is purely additive.

  ---
  Planned Changes

  | File           | Change                                                    |
  |----------------|-----------------------------------------------------------|
  | src/parser.c   | Add ParsedDeclarator struct; add parse_type_specifier,    |
  |                | parse_type_name, parse_declarator helpers; refactor       |
  |                | parse_cast, parse_unary (sizeof), parse_statement (decl   |
  |                | branch), parse_parameter_declaration, parse_function_decl |
  | docs/grammar.md| Update grammar to reflect new rules                       |

  No changes to: include/ast.h, include/type.h, src/codegen.c,
  src/ast_pretty_printer.c, or tests.

  Generating kickoff artifact…

● haiku-stage-artifact-writer(Generate stage 20-01 kickoff artifact)
  ⎿  Done (5 tool uses · 11.0k tokens · 22s)
  (ctrl+o to expand)

● Step 1: Add the ParsedDeclarator struct after the #include block.

● Update(src/parser.c)
  ⎿  Added 12 lines
       5  #include "parser.h"
       6  
       7 +/*
       8 + * Internal helper filled by parse_declarator. Carries the identifier
       9 + * name, pointer depth, and optional array-size information parsed out
      10 + * of one declarator before the caller assembles the semantic Type.
      11 + */
      12 +typedef struct {
      13 +    char name[256];
      14 +    int  pointer_count;
      15 +    int  is_array;
      16 +    int  has_size;
      17 +    int  array_length;
      18 +} ParsedDeclarator;
      19 +
      20  void parser_init(Parser *parser, Lexer *lexer) {

● Step 2: Add parse_type_specifier, parse_type_name, and parse_declarator helpers
  before parse_primary.

● Update(src/parser.c)
  ⎿  Added 72 lines
      108 +/*
      109 + * <type_specifier> ::= "char" | "short" | "int" | "long"
      110 + *
      111 + * Parses one integer-type keyword, advances the token, and returns the
      112 + * corresponding Type*. Writes the TypeKind to *out_kind when non-NULL.
      113 + */
      114 +static Type *parse_type_specifier(Parser *parser, TypeKind *out_kind) {
      115 +    TypeKind kind;
      116 +    Type *t;
      117 +    switch (parser->current.type) {
      118 +    case TOKEN_CHAR:  kind = TYPE_CHAR;  t = type_char();  break;
      119 +    case TOKEN_SHORT: kind = TYPE_SHORT; t = type_short(); break;
      120 +    case TOKEN_LONG:  kind = TYPE_LONG;  t = type_long();  break;
      121 +    case TOKEN_INT:   kind = TYPE_INT;   t = type_int();   break;
      122 +    default:
      123 +        fprintf(stderr, "error: expected integer type, got '%s'\n",
      124 +                parser->current.value);
      125 +        exit(1);
      126 +    }
      127 +    parser->current = lexer_next_token(parser->lexer);
      128 +    if (out_kind) *out_kind = kind;
      129 +    return t;
      130 +}
      131 +
      132 +/*
      133 + * <type_name> ::= <type_specifier> { "*" }
      134 + *
      135 + * Nameless type context: cast expressions and sizeof(type).
      136 + * Returns the fully pointer-wrapped Type*.
      137 + */
      138 +static Type *parse_type_name(Parser *parser) {
      139 +    Type *t = parse_type_specifier(parser, NULL);
      140 +    while (parser->current.type == TOKEN_STAR) {
      141 +        t = type_pointer(t);
      142 +        parser->current = lexer_next_token(parser->lexer);
      143 +    }
      144 +    return t;
      145 +}
      146 +
      147 +/*
      148 + * <declarator>        ::= { "*" } <direct_declarator>
      149 + * <direct_declarator> ::= <identifier>
      150 + *                       | <identifier> "[" [<integer_literal>] "]"
      151 + */
      152 +static ParsedDeclarator parse_declarator(Parser *parser) {
      153 +    ParsedDeclarator d;
      154 +    memset(&d, 0, sizeof(d));
      155 +    while (parser->current.type == TOKEN_STAR) {
      156 +        d.pointer_count++;
      157 +        parser->current = lexer_next_token(parser->lexer);
      158 +    }
      159 +    Token name = parser_expect(parser, TOKEN_IDENTIFIER);
      160 +    strncpy(d.name, name.value, sizeof(d.name) - 1);
      161 +    d.name[sizeof(d.name) - 1] = '\0';
      162 +    if (parser->current.type == TOKEN_LBRACKET) {
      163 +        d.is_array = 1;
      164 +        parser->current = lexer_next_token(parser->lexer);
      165 +        if (parser->current.type == TOKEN_INT_LITERAL) {
      166 +            Token size_tok = parser->current;
      167 +            parser->current = lexer_next_token(parser->lexer);
      168 +            int length = (int)size_tok.long_value;
      169 +            if (length <= 0) {
      170 +                fprintf(stderr, "error: array size must be greater than zero\n");
      171 +                exit(1);
      172 +            }
      173 +            d.array_length = length;
      174 +            d.has_size = 1;
      175 +        } else if (parser->current.type != TOKEN_RBRACKET) {
      176 +            fprintf(stderr, "error: array size must be an integer literal\n");
      177 +            exit(1);
      178 +        }
      179 +        parser_expect(parser, TOKEN_RBRACKET);
      180 +    }
      181 +    return d;
      182 +}
      183 +

● Step 3: Refactor parse_cast to use parse_type_name.

● Update(src/parser.c)
  ⎿  Added 5 lines, removed 14 lines
      422  
      423  /*
      424 - * <cast_expression> ::= <unary_expression>
      425 - *                     | "(" <integer_type> ")" <cast_expression>
      426 + * <cast_expression> ::= "(" <type_name> ")" <cast_expression>
      427 + *                     | <unary_expression>
      428   *
      429   * When the current token is '(', save lexer state and peek past it to
      430   * decide between a cast and a parenthesized expression. If the next
      431 - * token is an integer-type keyword, consume "(", <type>, ")" and
      432 + * token is an integer-type keyword, consume "(", <type_name>, ")" and
      433   * recurse to parse the operand cast expression. Otherwise restore
      434   * the saved state and fall through to parse_unary — parenthesized
      435   * expressions are then handled by parse_primary as before.
      436   */
      437  static ASTNode *parse_cast(Parser *parser) {
      ...
      446          if (parser->current.type == TOKEN_CHAR ||
      447              parser->current.type == TOKEN_SHORT ||
      448              parser->current.type == TOKEN_INT ||
      449              parser->current.type == TOKEN_LONG) {
      450 -            TypeKind target_kind;
      451 -            switch (parser->current.type) {
      452 -            case TOKEN_CHAR:  target_kind = TYPE_CHAR;  break;
      453 -            case TOKEN_SHORT: target_kind = TYPE_SHORT; break;
      454 -            case TOKEN_LONG:  target_kind = TYPE_LONG;  break;
      455 -            default:          target_kind = TYPE_INT;   break;
      456 -            }
      457 -            parser->current = lexer_next_token(parser->lexer);
      458 -            parser_expect(parser, TOKEN_RPAREN);
      450 +            Type *cast_type = parse_type_name(parser);
      451 +            parser_expect(parser, TOKEN_RPAREN);
      452              ASTNode *operand = parse_cast(parser);
      453              ASTNode *cast = ast_new(AST_CAST, NULL);
      454 -            cast->decl_type = target_kind;
      455 +            cast->decl_type = cast_type->kind;
      456 +            if (cast_type->kind == TYPE_POINTER) {
      457 +                cast->full_type = cast_type;
      458 +            }
      459              ast_add_child(cast, operand);
      460              return cast;
      461          }

● Step 4: Refactor parse_unary sizeof branch to use parse_type_name.

● Update(src/parser.c)
  ⎿  Added 3 lines, removed 14 lines
      315          if (parser->current.type == TOKEN_CHAR ||
      316              parser->current.type == TOKEN_SHORT ||
      317              parser->current.type == TOKEN_INT ||
      318              parser->current.type == TOKEN_LONG) {
      319 -            /* sizeof(<type>) — existing path */
      320 -            TypeKind base_kind;
      321 -            switch (parser->current.type) {
      322 -            case TOKEN_CHAR:  base_kind = TYPE_CHAR;  break;
      323 -            case TOKEN_SHORT: base_kind = TYPE_SHORT; break;
      324 -            case TOKEN_LONG:  base_kind = TYPE_LONG;  break;
      325 -            default:          base_kind = TYPE_INT;   break;
      326 -            }
      327 -            parser->current = lexer_next_token(parser->lexer);
      328 -            TypeKind result_kind = base_kind;
      329 -            while (parser->current.type == TOKEN_STAR) {
      330 -                result_kind = TYPE_POINTER;
      331 -                parser->current = lexer_next_token(parser->lexer);
      332 -            }
      333 -            parser_expect(parser, TOKEN_RPAREN);
      319 +            /* <sizeof_expression> ::= "sizeof" "(" <type_name> ")" */
      320 +            Type *t = parse_type_name(parser);
      321 +            parser_expect(parser, TOKEN_RPAREN);
      322              ASTNode *node = ast_new(AST_SIZEOF_TYPE, NULL);
      323 -            node->decl_type = result_kind;
      323 +            node->decl_type = t->kind;
      324              return node;
      325          }

● Step 5: Refactor parse_statement declaration branch to use parse_type_specifier
  + parse_declarator.

● Update(src/parser.c)
  ⎿  Added 20 lines, removed 30 lines
      952      /* <declaration> ::= <type_specifier> <init_declarator> ";"
      953       * ...
      954       */
      955      if (parser->current.type == TOKEN_CHAR || ...) {
      956 -        TypeKind base_kind;
      957 -        Type *base_type;
      958 -        switch (parser->current.type) { ... }
      959 -        parser->current = lexer_next_token(parser->lexer);
      960 -        Type *full_type = base_type;
      961 -        int pointer_levels = 0;
      962 -        while (parser->current.type == TOKEN_STAR) {
      963 -            full_type = type_pointer(full_type);
      964 -            pointer_levels++;
      965 -            parser->current = lexer_next_token(parser->lexer);
      966 -        }
      967 -        Token name = parser_expect(parser, TOKEN_IDENTIFIER);
      968 -        ASTNode *decl = ast_new(AST_DECLARATION, name.value);
      969 -        if (parser->current.type == TOKEN_LBRACKET) {
      970 -            parser->current = lexer_next_token(parser->lexer);
      971 -            int has_size = 0;
      972 -            int length = 0;
      973 -            if (parser->current.type == TOKEN_INT_LITERAL) { ... }
      974 -            ...
      975 -        }
      976 +        TypeKind base_kind;
      977 +        Type *base_type = parse_type_specifier(parser, &base_kind);
      978 +        ParsedDeclarator d = parse_declarator(parser);
      979 +
      980 +        /* Build the fully-wrapped element type: base + pointer levels. */
      981 +        Type *full_type = base_type;
      982 +        for (int i = 0; i < d.pointer_count; i++) {
      983 +            full_type = type_pointer(full_type);
      984 +        }
      985 +
      986 +        ASTNode *decl = ast_new(AST_DECLARATION, d.name);
      987 +
      988 +        if (d.is_array) { ... }  /* array/string-init handling unchanged */
      989 +
      990 +        if (d.pointer_count > 0) {
      991 +            decl->decl_type = TYPE_POINTER;
      992 +            decl->full_type = full_type;
      993 +        } else {
      994 +            decl->decl_type = base_kind;
      995 +        }
      996          ...
      997      }

● Step 6: Refactor parse_parameter_declaration to use parse_type_specifier.

● Update(src/parser.c)
  ⎿  Added 3 lines, removed 17 lines
      1170 +static ASTNode *parse_parameter_declaration(Parser *parser) {
      1171 +    TypeKind base_kind;
      1172 +    Type *base_type = parse_type_specifier(parser, &base_kind);
      1173      Type *full_type = base_type;
      1174 -    int pointer_levels = 0;
      1175 +    int pointer_count = 0;
      1176      while (parser->current.type == TOKEN_STAR) {
      1177          full_type = type_pointer(full_type);
      1178 -        pointer_levels++;
      1179 +        pointer_count++;
      1180          parser->current = lexer_next_token(parser->lexer);
      1181      }
      1182      Token name = parser_expect(parser, TOKEN_IDENTIFIER);
      1183      ASTNode *param = ast_new(AST_PARAM, name.value);
      1184 -    if (pointer_levels > 0) {
      1184 +    if (pointer_count > 0) {
      1185          param->decl_type = TYPE_POINTER;
      1186          param->full_type = full_type;
      1187      } else {

● Step 7: Refactor parse_function_decl to use parse_type_specifier.

● Update(src/parser.c)
  ⎿  Added 4 lines, removed 19 lines
      1232 +static ASTNode *parse_function_decl(Parser *parser) {
      1233 +    TypeKind base_kind;
      1234 +    Type *base_type = parse_type_specifier(parser, &base_kind);
      1235      Type *full_type = base_type;
      1236 -    int pointer_levels = 0;
      1237 +    int pointer_count = 0;
      1238      while (parser->current.type == TOKEN_STAR) {
      1239          full_type = type_pointer(full_type);
      1240 -        pointer_levels++;
      1240 +        pointer_count++;
      1241          parser->current = lexer_next_token(parser->lexer);
      1242      }
      1243 -    TypeKind return_kind = (pointer_levels > 0) ? TYPE_POINTER : base_kind;
      1243 +    TypeKind return_kind = (pointer_count > 0) ? TYPE_POINTER : base_kind;
      1244      Token name = parser_expect(parser, TOKEN_IDENTIFIER);
      1245      ASTNode *func = ast_new(AST_FUNCTION_DECL, name.value);
      1246      func->decl_type = return_kind;
      1247 -    if (pointer_levels > 0) {
      1247 +    if (pointer_count > 0) {
      1248          func->full_type = full_type;
      1249      }

● Bash(cmake --build cmake-build-debug 2>&1 | tail -5)
  ⎿  [1/2] Building C object CMakeFiles/ccompiler.dir/src/parser.c.o
     [2/2] Linking C executable ccompiler

● Clean build. Running tests:

● Bash(./test/valid/run_tests.sh 2>&1 | tail -5)
  ⎿  Results: 376 passed, 0 failed, 376 total

● Bash(./test/invalid/run_tests.sh 2>&1 | tail -5)
  ⎿  Results: 102 passed, 0 failed, 102 total

● 478 tests passing. Updating docs/grammar.md…

● Update(docs/grammar.md)
  ⎿  Added 22 lines, removed 6 lines
      Replaced <type> and <function>/<parameter_declaration> with new rules:
      <type_specifier>, <type_name>, <declarator>, <direct_declarator>,
      <init_declarator>, <initializer_expression>, <function_declarator>,
      <parameter_declarator>, <sizeof_expression>

● Delegating milestone, transcript, and README to Haiku…

● haiku-stage-artifact-writer(Generate stage 20-01 milestone, transcript, README)
  ⎿  Done (13 tool uses · 18.7k tokens · 31s)
  (ctrl+o to expand)

● Bash(git add src/parser.c docs/grammar.md README.md
      docs/kickoffs/stage-20-01-kickoff.md
      docs/milestones/stage-20-01-milestone.md
      docs/sessions/stage-20-01-transcript.md &&
      git commit -m "stage-20-01: declarator refactor …")
  ⎿  [master 5b0c7c7] stage-20-01: declarator refactor
     6 files changed, 334 insertions(+), 138 deletions(-)

● Stage 20-01 complete.

  Implementation — src/parser.c only:
  - Added ParsedDeclarator internal struct
  - Added parse_type_specifier: parses char|short|int|long base keyword
  - Added parse_type_name: parses <type_specifier> { "*" } for casts/sizeof
  - Added parse_declarator: parses { "*" } <direct_declarator> with optional []
  - Refactored parse_cast (now uses parse_type_name; pointer casts supported)
  - Refactored parse_unary sizeof branch (now uses parse_type_name)
  - Refactored parse_statement declaration (now uses parse_type_specifier +
    parse_declarator)
  - Refactored parse_parameter_declaration (now uses parse_type_specifier)
  - Refactored parse_function_decl (now uses parse_type_specifier)

  Grammar — docs/grammar.md: replaced old <type> rule with <type_specifier> and
  <type_name>; added full declarator grammar hierarchy.

  Tests: 478 total (376 valid + 102 invalid), 0 failures — unchanged.

✻ Completed in ~8m
