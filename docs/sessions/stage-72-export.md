 ▐▛███▜▌   Claude Code v2.1.150
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

❯ /implement-stage docs/stages/ClaudeC99-spec-stage-72-union-support.md         

● I'll start by reading the spec and supporting files.

  Read 4 files (ctrl+o to expand)

● Now let me look at the existing struct implementation and relevant source
  files.

  STAGE_DISPLAY: Stage 72

  Searched for 3 patterns, read 5 files (ctrl+o to expand)

● Let me look at the parser's struct handling in declarations and the codegen's
  struct code.

  Searched for 1 pattern, read 4 files (ctrl+o to expand)

● Let me look at how addr-of works for structs and how global struct variables
  are stored/emitted.

  Searched for 3 patterns, read 1 file, listed 2 directories (ctrl+o to expand)

● Bash(for f in test/valid/test_*struct*.c; do
          if grep -q "^struct.*;" "$f" | grep -v "{" 2>/dev/null; then…)
  ⎿  test/valid/test_array_of_structs_sizeof__24.c
     test/valid/test_array_of_structs__33.c
     test/valid/test_array_of_structs_nested__110.c
     … +2 lines (ctrl+o to expand)

  Read 1 file (ctrl+o to expand)

● Let me check the print_asm tests to see if they test structs or would be
  affected.

  Listed 2 directories (ctrl+o to expand)

● Let me look at the full parser's global declaration handling.

  Read 2 files (ctrl+o to expand)

● Now I have a thorough understanding of the codebase. Let me summarize and note
   a spec issue before generating the kickoff.

  Spec issue found: The "Union inside struct" test references union Value but
  the definition shows union U:
  union U { int i; char c; };   // defines union U
  struct Box { int kind; union Value value; };  // references undefined union 
  Value
  This is a bug in the spec — union Value should be union U. I'll use union U in
   the test.

  Planned Changes section:

  - include/token.h — add TOKEN_UNION
  - src/lexer.c — keyword recognition + display name
  - include/type.h — add TYPE_UNION, type_union() declaration
  - src/type.c — type_union() implementation, type_kind_name()
  - include/parser.h — UnionTag struct, union tag table in Parser
  - src/parser.c — parse_union_specifier(), TOKEN_UNION in all type-specifier
  sites, TYPE_UNION in all declaration paths
  - src/codegen.c — TYPE_UNION throughout: emit_member_addr, emit_arrow_addr,
  expr_result_type, sizeof, local/global declaration, assignment, bss emission
  - src/ast_pretty_printer.c — TYPE_UNION display
  - test/valid/ — 8 valid tests
  - test/invalid/ — 1 invalid test

  Now generating the kickoff artifact:

● haiku-stage-artifact-writer(Generate kickoff artifact for stage 72)  ⎿  Done (2 tool uses · 7.0k tokens · 13s)
  (ctrl+o to expand)

● Kickoff generated. Now starting implementation.

  Step 1: Token — add TOKEN_UNION

● Update(include/token.h)
  ⎿  Added 1 line
      31      TOKEN_TYPEDEF,
      32      TOKEN_ENUM,
      33      TOKEN_STRUCT,
      34 +    TOKEN_UNION,                                               
      35      TOKEN_VOID,
      36      TOKEN_BOOL,
      37      TOKEN_CONST,

● Now add the keyword recognition in the lexer and the display name:

● Update(src/lexer.c)
  ⎿  Added 2 lines
      485              token.type = TOKEN_ENUM;
      486          } else if (strcmp(token.value, "struct") == 0) {
      487              token.type = TOKEN_STRUCT;
      488 +        } else if (strcmp(token.value, "union") == 0) {       
      489 +            token.type = TOKEN_UNION;                         
      490          } else if (strcmp(token.value, "void") == 0) {
      491              token.type = TOKEN_VOID;
      492          } else if (strcmp(token.value, "const") == 0) {

● Update(src/lexer.c)
  ⎿  Added 1 line
      535          case TOKEN_TYPEDEF:          return "'typedef'";
      536          case TOKEN_ENUM:             return "'enum'";
      537          case TOKEN_STRUCT:           return "'struct'";
      538 +        case TOKEN_UNION:            return "'union'";        
      539          case TOKEN_VOID:             return "'void'";
      540          case TOKEN_BOOL:             return "'_Bool'";
      541          case TOKEN_CONST:            return "'const'";

● Step 2: Type system — add TYPE_UNION

● Update(include/type.h)
  ⎿  Added 4 lines, removed 1 line
      31      TYPE_FUNCTION,
      32      /* Stage 30: named struct type. size and alignment are comp
          uted from the
      33       * field layout at definition time. */
      34 -    TYPE_STRUCT                                                
      34 +    TYPE_STRUCT,                                               
      35 +    /* Stage 72: named union type. Layout: all members at offse
         +t 0,                                                           
      36 +     * size = max member size rounded up to alignment. */      
      37 +    TYPE_UNION                                                 
      38  } TypeKind;
      39  
      40  /* Stage 31: field descriptor stored inside a TYPE_STRUCT Type 
          node. */

● Update(include/type.h)
  ⎿  Added 1 line
      83  Type *type_array(Type *element_type, int length);
      84  Type *type_function(Type *return_type, int param_count, Type **
          param_types);
      85  Type *type_struct(int size, int alignment);
      86 +Type *type_union(int size, int alignment);                     
      87  /* Stage 66: allocate a heap copy of t with is_const=1. Never m
          utates t. */
      88  Type *type_const_copy(Type *t);
      89  

● Now let me look at type_struct in src/type.c to add type_union similarly:

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Update(src/type.c)
  ⎿  Added 16 lines
      127      return t;
      128  }
      129  
      130 +/* Stage 72: heap-allocate a TYPE_UNION node. Layout mirrors t
          +ype_struct                                                    
      131 + * except the kind is TYPE_UNION. */                          
      132 +Type *type_union(int size, int alignment) {                   
      133 +    Type *t = calloc(1, sizeof(Type));                        
      134 +    if (!t) {                                                 
      135 +        fprintf(stderr, "error: out of memory\n");            
      136 +        exit(1);                                              
      137 +    }                                                         
      138 +    t->kind = TYPE_UNION;                                     
      139 +    t->size = size;                                           
      140 +    t->alignment = alignment;                                 
      141 +    t->is_signed = 0;                                         
      142 +    return t;                                                 
      143 +}                                                             
      144 +                                                              
      145  const char *type_kind_name(TypeKind kind) {
      146      switch (kind) {
      147      case TYPE_VOID:     return "void";
     ...
      156      case TYPE_ARRAY:    return "array";
      157      case TYPE_FUNCTION: return "function";
      158      case TYPE_STRUCT:   return "struct";
      159 +    case TYPE_UNION:    return "union";                       
      160      }
      161      return "unknown";
      162  }

● Also update type_is_integer to handle TYPE_UNION:

● Update(src/type.c)
  ⎿  Added 1 line
      185      case TYPE_ARRAY:
      186      case TYPE_FUNCTION:
      187      case TYPE_STRUCT:
      188 +    case TYPE_UNION:                                          
      189          return 0;
      190      }
      191      return 0;

● Step 3: Parser — add union tag table and parse_union_specifier()

  First update include/parser.h:

● Update(include/parser.h)
  ⎿  Added 1 line
      13  #define PARSER_MAX_ENUM_CONSTS 256
      14  #define PARSER_MAX_ENUM_TAGS 32
      15  #define PARSER_MAX_STRUCT_TAGS 32
      16 +#define PARSER_MAX_UNION_TAGS  32                              
      17  #define FUNC_MAX_PARAMS 16
      18  
      19  typedef struct {

● Update(include/parser.h)
  ⎿  Added 6 lines
      32      struct Type *type;   /* NULL until the struct body is parse
          d */
      33  } StructTag;
      34  
      35 +/* Stage 72: one entry per named union tag in the translation u
         +nit. */                                                        
      36 +typedef struct {                                               
      37 +    char         tag[256];                                     
      38 +    struct Type *type;   /* NULL until the union body is parsed
         + */                                                            
      39 +} UnionTag;                                                    
      40 +                                                               
      41  /* Stage 28-01/28-02: entry in the parser-level typedef table. 
          */
      42  typedef struct {
      43      char         name[256];

● Update(include/parser.h)
  ⎿  Added 3 lines
       93      /* Stage 30: struct tag table (flat, global scope). */
       94      StructTag struct_tags[PARSER_MAX_STRUCT_TAGS];
       95      int struct_tag_count;
       96 +    /* Stage 72: union tag table (flat, global scope). */     
       97 +    UnionTag union_tags[PARSER_MAX_UNION_TAGS];               
       98 +    int union_tag_count;                                      
       99  } Parser;
      100  
      101  void parser_init(Parser *parser, Lexer *lexer);

● Now initialize the union tag count in parser_init:

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 1 line
      62      parser->enum_const_count = 0;
      63      parser->enum_tag_count = 0;
      64      parser->struct_tag_count = 0;
      65 +    parser->union_tag_count = 0;                               
      66  }
      67  
      68  /* Stage 28-01: typedef name table helpers. */

● Now add the union tag helpers and parse_union_specifier() after
  parse_struct_specifier(). Let me read that section:

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 126 lines
      440      return st->type;
      441  }
      442  
      443 +/* Stage 72: union tag table helpers. */                      
      444 +static UnionTag *parser_find_union_tag(Parser *parser, const c
          +har *tag) {                                                   
      445 +    for (int i = 0; i < parser->union_tag_count; i++) {       
      446 +        if (strcmp(parser->union_tags[i].tag, tag) == 0)      
      447 +            return &parser->union_tags[i];                    
      448 +    }                                                         
      449 +    return NULL;                                              
      450 +}                                                             
      451 +                                                              
      452 +static UnionTag *parser_register_union_tag(Parser *parser, con
          +st char *tag) {                                               
      453 +    UnionTag *ut = parser_find_union_tag(parser, tag);        
      454 +    if (ut) return ut;                                        
      455 +    if (parser->union_tag_count >= PARSER_MAX_UNION_TAGS) {   
      456 +        PARSER_ERROR(parser, "error: too many union tags (max 
          +%d)\n",                                                       
      457 +                PARSER_MAX_UNION_TAGS);                       
      458 +    }                                                         
      459 +    ut = &parser->union_tags[parser->union_tag_count++];      
      460 +    strncpy(ut->tag, tag, sizeof(ut->tag) - 1);               
      461 +    ut->tag[sizeof(ut->tag) - 1] = '\0';                      
      462 +    ut->type = NULL;                                          
      463 +    return ut;                                                
      464 +}                                                             
      465 +                                                              
      466  /*
      467 + * <union_specifier> ::= "union" <identifier> [ "{" <struct_de
          +claration_list> "}" ]                                         
      468 + *                                                            
      469 + * Layout: all member offsets are 0; size = max(member sizes) 
          +rounded up to                                                 
      470 + * the union's alignment (max of all member alignments).      
      471 + * Returns the TYPE_UNION Type*.                              
      472 + */                                                           
      473 +static Type *parse_union_specifier(Parser *parser) {          
      474 +    /* consume 'union' */                                     
      475 +    parser->current = lexer_next_token(parser->lexer);        
      476 +                                                              
      477 +    if (parser->current.type != TOKEN_IDENTIFIER) {           
      478 +        PARSER_ERROR(parser, "error: expected union tag name a
          +fter 'union'\n");                                             
      479 +    }                                                         
      480 +    char tag[256];                                            
      481 +    strncpy(tag, parser->current.value, sizeof(tag) - 1);     
      482 +    tag[sizeof(tag) - 1] = '\0';                              
      483 +    parser->current = lexer_next_token(parser->lexer);        
      484 +                                                              
      485 +    UnionTag *ut = parser_register_union_tag(parser, tag);    
      486 +                                                              
      487 +    if (parser->current.type == TOKEN_LBRACE) {               
      488 +        parser->current = lexer_next_token(parser->lexer);    
      489 +                                                              
      490 +        int max_size  = 0;                                    
      491 +        int max_align = 1;                                    
      492 +                                                              
      493 +        StructField tmp_fields[64];                           
      494 +        int n_fields = 0;                                     
      495 +                                                              
      496 +        while (parser->current.type != TOKEN_RBRACE) {        
      497 +            Type *field_base = parse_type_specifier(parser, NU
          +LL);                                                          
      498 +                                                              
      499 +            do {                                              
      500 +                if (parser->current.type == TOKEN_COMMA)      
      501 +                    parser->current = lexer_next_token(parser-
          +>lexer);                                                      
      502 +                                                              
      503 +                ParsedDeclarator d = parse_declarator(parser);
      504 +                Type *field_type = field_base;                
      505 +                for (int i = 0; i < d.pointer_count; i++)     
      506 +                    field_type = type_pointer(field_type);    
      507 +                                                              
      508 +                /* Reject non-pointer field of an incomplete s
          +truct or union type. */                                       
      509 +                if ((field_type->kind == TYPE_STRUCT || field_
          +type->kind == TYPE_UNION) &&                                  
      510 +                    field_type->size == 0) {                  
      511 +                    PARSER_ERROR(parser,                      
      512 +                            "error: field '%s' has incomplete 
          +type\n", d.name);                                             
      513 +                }                                             
      514 +                                                              
      515 +                int fsz   = type_size(field_type);            
      516 +                int falign = type_alignment(field_type);      
      517 +                if (falign < 1) falign = 1;                   
      518 +                if (fsz < 1)    fsz = 1;                      
      519 +                                                              
      520 +                if (falign > max_align) max_align = falign;   
      521 +                if (fsz > max_size)     max_size  = fsz;      
      522 +                                                              
      523 +                if (n_fields < 64) {                          
      524 +                    strncpy(tmp_fields[n_fields].name, d.name,
      525 +                            sizeof(tmp_fields[n_fields].name) 
          +- 1);                                                         
      526 +                    tmp_fields[n_fields].name[sizeof(tmp_field
          +s[n_fields].name) - 1] = '\0';                                
      527 +                    tmp_fields[n_fields].offset    = 0; /* all
          + union members at offset 0 */                                 
      528 +                    tmp_fields[n_fields].kind      = field_typ
          +e->kind;                                                      
      529 +                    tmp_fields[n_fields].full_type = (field_ty
          +pe->kind == TYPE_POINTER ||                                   
      530 +                                                     field_typ
          +e->kind == TYPE_ARRAY   ||                                    
      531 +                                                     field_typ
          +e->kind == TYPE_STRUCT  ||                                    
      532 +                                                     field_typ
          +e->kind == TYPE_UNION)                                        
      533 +                                                     ? field_t
          +ype : NULL;                                                   
      534 +                    n_fields++;                               
      535 +                }                                             
      536 +                                                              
      537 +            } while (parser->current.type == TOKEN_COMMA);    
      538 +                                                              
      539 +            parser_expect(parser, TOKEN_SEMICOLON);           
      540 +        }                                                     
      541 +        parser_expect(parser, TOKEN_RBRACE);                  
      542 +                                                              
      543 +        /* Round total size up to the union's alignment. */   
      544 +        int total_size = (max_size + max_align - 1) & ~(max_al
          +ign - 1);                                                     
      545 +        if (total_size == 0) total_size = 1;                  
      546 +                                                              
      547 +        if (ut->type && ut->type->size == 0) {                
      548 +            ut->type->size      = total_size;                 
      549 +            ut->type->alignment = max_align;                  
      550 +        } else {                                              
      551 +            ut->type = type_union(total_size, max_align);     
      552 +        }                                                     
      553 +                                                              
      554 +        if (n_fields > 0) {                                   
      555 +            ut->type->fields = calloc(n_fields, sizeof(StructF
          +ield));                                                       
      556 +            memcpy(ut->type->fields, tmp_fields, n_fields * si
          +zeof(StructField));                                           
      557 +            ut->type->field_count = n_fields;                 
      558 +        }                                                     
      559 +                                                              
      560 +    } else {                                                  
      561 +        /* Forward declaration / reference without body. */   
      562 +        if (!ut->type)                                        
      563 +            ut->type = type_union(0, 0);                      
      564 +    }                                                         
      565 +                                                              
      566 +    return ut->type;                                          
      567 +}                                                             
      568 +                                                              
      569 +/*                                                            
      570   * <enum_specifier> ::= "enum" <identifier> "{" <enumerator_li
           st> "}"
      571   *                    | "enum"             "{" <enumerator_lis
           t> "}"
      572   *                    | "enum" <identifier>

● Now update parse_struct_specifier() to also set full_type for TYPE_UNION
  fields:

● Update(src/parser.c)
  ⎿  Added 6 lines, removed 5 lines
      372                  for (int i = 0; i < d.pointer_count; i++)
      373                      field_type = type_pointer(field_type);
      374  
      375 -                /* Stage 37: a non-pointer field of an incompl
          -ete struct type                                               
      376 -                 * is invalid; pointer-to-incomplete is allowe
          -d. */                                                         
      377 -                if (field_type->kind == TYPE_STRUCT && field_t
          -ype->size == 0) {                                             
      378 -                    PARSER_ERROR(parser,                      
      379 -                            "error: field '%s' has incomplete 
          -struct type\n", d.name);                                      
      375 +                /* Stage 37/72: a non-pointer field of an inco
          +mplete struct or                                              
      376 +                 * union type is invalid; pointer-to-incomplet
          +e is allowed. */                                              
      377 +                if ((field_type->kind == TYPE_STRUCT || field_
          +type->kind == TYPE_UNION) &&                                  
      378 +                    field_type->size == 0) {                  
      379 +                    PARSER_ERROR(parser,                      
      380 +                            "error: field '%s' has incomplete 
          +type\n", d.name);                                             
      381                  }
      382  
      383                  int fsz   = type_size(field_type);
● Update(src/parser.c)
  ⎿  Added 2 lines, removed 1 line
      397                      tmp_fields[n_fields].kind      = field_typ
           e->kind;
      398                      tmp_fields[n_fields].full_type = (field_ty
           pe->kind == TYPE_POINTER ||
      399                                                       field_typ
           e->kind == TYPE_ARRAY   ||
      400 -                                                     field_typ
          -e->kind == TYPE_STRUCT)                                       
      400 +                                                     field_typ
          +e->kind == TYPE_STRUCT  ||                                    
      401 +                                                     field_typ
          +e->kind == TYPE_UNION)                                        
      402                                                       ? field_t
           ype : NULL;
      403                      n_fields++;
      404                  }

● Now update parse_type_specifier() to handle TOKEN_UNION:

● Update(src/parser.c)
  ⎿  Added 6 lines
      871          if (out_kind) *out_kind = TYPE_STRUCT;
      872          return st;
      873      }
      874 +    case TOKEN_UNION: {                                       
      875 +        /* parse_union_specifier advances past all union token
          +s itself. */                                                  
      876 +        Type *ut = parse_union_specifier(parser);             
      877 +        if (out_kind) *out_kind = TYPE_UNION;                 
      878 +        return ut;                                            
      879 +    }                                                         
      880      default:
      881          PARSER_ERROR(parser, "error: expected integer type, go
           t '%s'\n",
      882                  parser->current.value);

● Now update the sizeof parsing to recognize TOKEN_UNION and handle TYPE_UNION:

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 1 line
      1342              parser->current.type == TOKEN_SIGNED ||
      1343              parser->current.type == TOKEN_UNSIGNED ||
      1344              parser->current.type == TOKEN_STRUCT ||
      1345 +            parser->current.type == TOKEN_UNION ||           
      1346              (parser->current.type == TOKEN_IDENTIFIER &&
      1347               parser_find_typedef(parser, parser->current.valu
            e))) {
      1348              /* <sizeof_expression> ::= "sizeof" "(" <type_nam
            e> ")" */
     ...
      1350              parser_expect(parser, TOKEN_RPAREN);
      1351              ASTNode *node = ast_new(AST_SIZEOF_TYPE, NULL);
      1352              node->decl_type = t->kind;
      1352 -            if (t->kind == TYPE_STRUCT)                      
      1353 +            if (t->kind == TYPE_STRUCT || t->kind == TYPE_UNI
           +ON)                                                          
      1354                  node->full_type = t;
      1355              return node;
      1356          }

● Now update local statement parsing (in parse_statement()) to handle TYPE_UNION
   alongside TYPE_STRUCT. Let me find all the places:

  Searched for 1 pattern (ctrl+o to expand)

● Now I need to update all the places in the parser that need TYPE_UNION support
   alongside TYPE_STRUCT. Let me read the context around each one and make the
  changes:

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 3 lines, removed 1 line
      2033                                  ? TYPE_POINTER : base_kind;
      2034          /* Stage 40: for unsigned scalar typedefs, store the 
            base Type* (is_signed=0)
      2035           * as full_type so the signedness is preserved when t
            he typedef is resolved. */
      2036 -        Type *reg_full_type = (typedef_kind == TYPE_POINTER |
           -| typedef_kind == TYPE_STRUCT)                               
      2036 +        Type *reg_full_type = (typedef_kind == TYPE_POINTER |
           +|                                                            
      2037 +                               typedef_kind == TYPE_STRUCT  |
           +|                                                            
      2038 +                               typedef_kind == TYPE_UNION)   
      2039                                ? full_type
      2040                                : (!base_type->is_signed ? base
            _type : NULL);
      2041          parser_register_typedef(parser, d.name, typedef_kind,
             reg_full_type);

● Now line 2083 - the if that checks whether we have a declaration type:

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 1 line
      2083          parser->current.type == TOKEN_UNSIGNED ||
      2084          parser->current.type == TOKEN_ENUM ||
      2085          parser->current.type == TOKEN_STRUCT ||
      2086 +        parser->current.type == TOKEN_UNION ||               
      2087          (parser->current.type == TOKEN_IDENTIFIER &&
      2088           parser_find_typedef(parser, parser->current.value)))
             {
      2089          TypeKind base_kind;

● Now lines 2208-2210 and 2221 (local variable declaration where base_kind ==
  TYPE_STRUCT):

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 6 lines, removed 1 line
      2212              /* Stage 30: struct variable — carry the struct T
            ype* for size/alignment. */
      2213              decl->decl_type = TYPE_STRUCT;
      2214              decl->full_type = full_type;
      2215 +        } else if (base_kind == TYPE_UNION) {                
      2216 +            /* Stage 72: union variable. */                  
      2217 +            decl->decl_type = TYPE_UNION;                    
      2218 +            decl->full_type = full_type;                     
      2219          } else {
      2220              decl->decl_type = base_kind;
      2221              /* Stage 40: mark unsigned scalar. */
     ...
      2225              parser->current = lexer_next_token(parser->lexer)
            ;
      2226              ASTNode *init = parse_initializer(parser); /* sta
            ge 32: allows brace lists */
      2227              if (init->type == AST_INITIALIZER_LIST &&
      2224 -                decl->decl_type != TYPE_STRUCT && decl->decl_
           -type != TYPE_ARRAY) {                                        
      2228 +                decl->decl_type != TYPE_STRUCT && decl->decl_
           +type != TYPE_UNION &&                                        
      2229 +                decl->decl_type != TYPE_ARRAY) {             
      2230                  PARSER_ERROR(parser, "error: brace initialize
            r not valid for scalar type\n");
      2231              }
      2232              ast_add_child(decl, init);

● Now fix the multi-declarator case for local variables (around line 2257):

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 3 lines
      2265              } else if (base_kind == TYPE_STRUCT) {
      2266                  next_decl->decl_type = TYPE_STRUCT;
      2267                  next_decl->full_type = full_type2;
      2268 +            } else if (base_kind == TYPE_UNION) {            
      2269 +                next_decl->decl_type = TYPE_UNION;           
      2270 +                next_decl->full_type = full_type2;           
      2271              } else {
      2272                  next_decl->decl_type = base_kind;
      2273                  next_decl->is_unsigned = !base_type->is_signe
            d;

● Now the parse_declaration_specifiers() function - add TOKEN_UNION:

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 1 line
      2522                     parser->current.type == TOKEN_UNSIGNED ||
      2523                     parser->current.type == TOKEN_ENUM ||
      2524                     parser->current.type == TOKEN_STRUCT ||
      2525 +                   parser->current.type == TOKEN_UNION ||    
      2526                     (!has_type &&
      2527                      parser->current.type == TOKEN_IDENTIFIER 
            &&
      2528                      parser_find_typedef(parser, parser->curre
            nt.value))) {
      2529              if (has_type) {
      2529 -                PARSER_ERROR(parser,                         
      2530 +                PARSER_ERROR(parser,                         
      2531                          "error: multiple type specifiers are 
            not allowed\n");
      2532              }
      2533              has_type = 1;

● Now the external (file-scope) typedef path:

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 3 lines, removed 1 line
      2613                                  ? TYPE_POINTER : base_kind;
      2614          /* Stage 40: for unsigned scalar typedefs, store the 
            base Type* so
      2615           * signedness is preserved when the typedef name is l
            ater resolved. */
      2616 -        Type *reg_full_type = (typedef_kind == TYPE_POINTER |
           -| typedef_kind == TYPE_STRUCT)                               
      2616 +        Type *reg_full_type = (typedef_kind == TYPE_POINTER |
           +|                                                            
      2617 +                               typedef_kind == TYPE_STRUCT  |
           +|                                                            
      2618 +                               typedef_kind == TYPE_UNION)   
      2619                                ? full_type
      2620                                : (!base_type->is_signed ? base
            _type : NULL);
      2621          parser_register_typedef(parser, d.name, typedef_kind,
             reg_full_type);

● Now lines around 2696 and 2765 (external global declarations):

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 3 lines, removed 1 line
      2707          }
      2708          TypeKind obj_kind = d.is_array ? TYPE_ARRAY :
      2709                              (d.pointer_count > 0 || base_kind
             == TYPE_POINTER) ? TYPE_POINTER : base_kind;
      2710 -        Type *reg_full_type = (obj_kind == TYPE_POINTER || ob
           -j_kind == TYPE_STRUCT) ? full_type : NULL;                   
      2710 +        Type *reg_full_type = (obj_kind == TYPE_POINTER ||   
      2711 +                               obj_kind == TYPE_STRUCT  ||   
      2712 +                               obj_kind == TYPE_UNION)  ? ful
           +l_type : NULL;                                               
      2713          parser_register_global(parser, d.name, obj_kind, sc, 
            reg_full_type);
      2714  
      2715          ASTNode *decl = ast_new(AST_DECLARATION, d.name);
● Now lines 2765-2766 and 2781 (external global TYPE_STRUCT declarations):

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 9 lines, removed 4 lines
      2781          } else if (base_kind == TYPE_STRUCT) {
      2782              decl->decl_type = TYPE_STRUCT;
      2783              decl->full_type = full_type;
      2784 +        } else if (base_kind == TYPE_UNION) {                
      2785 +            /* Stage 72: union global variable. */           
      2786 +            decl->decl_type = TYPE_UNION;                    
      2787 +            decl->full_type = full_type;                     
      2788          } else {
      2789              decl->decl_type = base_kind;
      2790              /* Stage 40: mark unsigned scalar. */
     ...
      2792          }
      2793          /* Stage 22-02: optional constant initializer.
      2794           * Stage 43: also accept string literals for pointer 
            globals.
      2791 -         * Stage 44: accept brace-list initializers for struc
           -t globals. */                                                
      2795 +         * Stage 44: accept brace-list initializers for struc
           +t globals.                                                   
      2796 +         * Stage 72: accept brace-list initializers for union
           + globals (first member). */                                  
      2797          if (parser->current.type == TOKEN_ASSIGN) {
      2798              parser->current = lexer_next_token(parser->lexer)
            ;
      2799              ASTNode *init;
      2800              if (parser->current.type == TOKEN_LBRACE) {
      2796 -                /* Stage 44: struct global initialized with b
           -race list. */                                                
      2797 -                if (decl->decl_type != TYPE_STRUCT) {        
      2798 -                    PARSER_ERROR(parser,                     
      2801 +                /* Stage 44/72: struct or union global initia
           +lized with brace list. */                                    
      2802 +                if (decl->decl_type != TYPE_STRUCT && decl->d
           +ecl_type != TYPE_UNION) {                                    
      2803 +                    PARSER_ERROR(parser,                     
      2804                              "error: brace initializer not val
            id for non-struct global '%s'\n",
      2805                              d.name);
      2806                  }

● Now let me look at the cast parsing to see if it needs TOKEN_UNION:

  Read 1 file (ctrl+o to expand)

● Now let me build and see how far we've gotten, and what errors remain:

● Bash(cmake -S . -B build -q 2>&1 && cmake --build build 2>&1 | head -50)
  ⎿  Error: Exit code 1
     CMake Error: Unknown argument -q
     CMake Error: Run 'cmake --help' for all supported options.

● Bash(cmake -S . -B build 2>&1 | tail -5 && cmake --build build 2>&1 | head
      -60)
  ⎿  -- Configuring done (0.0s)
     -- Generating done (0.0s)
     -- Build files have been written to: /home/scott/code/claude/claudec99
     … +14 lines (ctrl+o to expand)

● Good, it builds. Now let me do a quick smoke test before updating codegen:

● Bash(cat > /tmp/test_union_basic.c << 'EOF'
      union U {…)
  ⎿  error: '.' applied to non-struct 'u'
     exit: 1

● Now let me update codegen. There are many places to update. Let me do them
  systematically:

  local_var_type() — add TYPE_UNION:

● Update(src/codegen.c)
  ⎿  Added 2 lines, removed 1 line
      406      case TYPE_LONG:               t = type_long(); break;
      407      case TYPE_LONG_LONG:          t = type_long_long(); break;
      408      case TYPE_UNSIGNED_LONG_LONG: t = type_unsigned_long_long(
           ); break;
      409 -    case TYPE_STRUCT:             return lv->full_type; /* alw
          -ays set for struct locals */                                  
      409 +    case TYPE_STRUCT:             return lv->full_type; /* alw
          +ays set for struct/union locals */                            
      410 +    case TYPE_UNION:              return lv->full_type;       
      411      case TYPE_INT:
      412      default:                      t = type_int(); break;
      413      }

● type_kind_bytes() — add TYPE_UNION (returns 0 since caller uses full_type):

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 1 line
      19      case TYPE_ARRAY:              return 0; /* size lives on fu
          ll_type; caller uses that */
      20      case TYPE_FUNCTION:           return 0; /* never directly a
          llocated; base of FP pointer */
      21      case TYPE_STRUCT:             return 0; /* size lives on fu
          ll_type; caller uses that */
      22 +    case TYPE_UNION:              return 0; /* size lives on fu
         +ll_type; caller uses that */                                   
      23      }
      24      return 4;
      25  }

● compute_decl_bytes() — add TYPE_UNION:

● Update(src/codegen.c)
  ⎿  Added 3 lines, removed 2 lines
      843      if (node->type == AST_DECLARATION) {
      844          if (node->decl_type == TYPE_ARRAY && node->full_type) 
           {
      845              total = node->full_type->size + 7;
      846 -        } else if (node->decl_type == TYPE_STRUCT && node->ful
          -l_type) {                                                     
      847 -            /* Struct: actual size plus alignment slack for st
          -ack alignment. */                                             
      846 +        } else if ((node->decl_type == TYPE_STRUCT || node->de
          +cl_type == TYPE_UNION) &&                                     
      847 +                   node->full_type) {                         
      848 +            /* Struct/union: actual size plus alignment slack 
          +for stack alignment. */                                       
      849              total = node->full_type->size + (node->full_type->
           alignment - 1);
      850          } else {
      851              total = 8;

● emit_member_addr() — accept TYPE_UNION alongside TYPE_STRUCT in all checks:

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 5 lines, removed 3 lines
      679              compile_error( "error: undeclared variable '%s'\n"
           , ptr_expr->value);
      680          }
      681          if (plv->kind != TYPE_POINTER || !plv->full_type ||
      682 -            !plv->full_type->base || plv->full_type->base->kin
          -d != TYPE_STRUCT) {                                           
      682 +            !plv->full_type->base || (plv->full_type->base->ki
          +nd != TYPE_STRUCT &&                                          
      683 +                                      plv->full_type->base->ki
          +nd != TYPE_UNION)) {                                          
      684              compile_error(
      684 -                    "error: '.' via dereference: '%s' is not a
          - pointer to struct\n",                                        
      685 +                    "error: '.' via dereference: '%s' is not a
          + pointer to struct/union\n",                                  
      686                      ptr_expr->value);
      687          }
      688          Type *st = plv->full_type->base;
      689          StructField *f = find_struct_field(st, field_name);
      690          if (!f) {
      690 -            compile_error( "error: struct has no member '%s'\n
          -", field_name);                                               
      691 +            compile_error( "error: '%s' has no member '%s'\n",
      692 +                    type_kind_name(st->kind), field_name);    
      693          }
      694          if (plv->is_static)
      695              fprintf(cg->output, "    mov rax, [rel %s]\n", plv
           ->static_label);

● Update(src/codegen.c)
  ⎿  Added 5 lines, removed 3 lines
      703      /* Stage 35: chained member access — base is itself a memb
           er access (e.g. r.origin.x). */
      704      if (base->type == AST_MEMBER_ACCESS) {
      705          StructField *inner_f = emit_member_addr(cg, base);
      706 -        if (inner_f->kind != TYPE_STRUCT || !inner_f->full_typ
          -e) {                                                          
      707 -            compile_error( "error: '.' applied to non-struct m
          -ember '%s'\n",                                                
      706 +        if ((inner_f->kind != TYPE_STRUCT && inner_f->kind != 
          +TYPE_UNION) ||                                                
      707 +            !inner_f->full_type) {                            
      708 +            compile_error( "error: '.' applied to non-struct/u
          +nion member '%s'\n",                                          
      709                      base->value);
      710          }
      711          StructField *f = find_struct_field(inner_f->full_type,
            field_name);
      712          if (!f) {
      712 -            compile_error( "error: struct has no member '%s'\n
          -", field_name);                                               
      713 +            compile_error( "error: '%s' has no member '%s'\n",
      714 +                    type_kind_name(inner_f->kind), field_name)
          +;                                                             
      715          }
      716          if (f->offset != 0)
      717              fprintf(cg->output, "    add rax, %d\n", f->offset
           );

● Update(src/codegen.c)
  ⎿  Added 4 lines, removed 3 lines
      721      /* Stage 35: array-element member access — base is array s
           ubscript (e.g. points[0].x). */
      722      if (base->type == AST_ARRAY_INDEX) {
      723          Type *element = emit_array_index_addr(cg, base);
      724 -        if (element->kind != TYPE_STRUCT) {                   
      725 -            compile_error( "error: '.' applied to non-struct a
          -rray element\n");                                             
      724 +        if (element->kind != TYPE_STRUCT && element->kind != T
          +YPE_UNION) {                                                  
      725 +            compile_error( "error: '.' applied to non-struct/u
          +nion array element\n");                                       
      726          }
      727          StructField *f = find_struct_field(element, field_name
           );
      728          if (!f) {
      729 -            compile_error( "error: struct has no member '%s'\n
          -", field_name);                                               
      729 +            compile_error( "error: '%s' has no member '%s'\n",
      730 +                    type_kind_name(element->kind), field_name)
          +;                                                             
      731          }
      732          if (f->offset != 0)
      733              fprintf(cg->output, "    add rax, %d\n", f->offset
           );

● Update(src/codegen.c)
  ⎿  Added 9 lines, removed 7 lines
      739      }
      740      LocalVar *lv = codegen_find_var(cg, base->value);
      741      if (lv) {
      742 -        if (lv->kind != TYPE_STRUCT || !lv->full_type) {      
      743 -            compile_error( "error: '.' applied to non-struct '
          -%s'\n", base->value);                                         
      742 +        if ((lv->kind != TYPE_STRUCT && lv->kind != TYPE_UNION
          +) || !lv->full_type) {                                        
      743 +            compile_error( "error: '.' applied to non-struct/u
          +nion '%s'\n", base->value);                                   
      744          }
      745          StructField *f = find_struct_field(lv->full_type, fiel
           d_name);
      746          if (!f) {
      747 -            compile_error( "error: struct has no member '%s'\n
          -", field_name);                                               
      747 +            compile_error( "error: '%s' has no member '%s'\n",
      748 +                    type_kind_name(lv->kind), field_name);    
      749          }
      750          fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->o
           ffset - f->offset);
      751          return f;
      752      }
      752 -    /* Stage 44: fall back to global struct variable. */      
      753 +    /* Stage 44: fall back to global struct/union variable. */
      754      GlobalVar *gv = codegen_find_global(cg, base->value);
      755      if (!gv) {
      756          compile_error( "error: undeclared variable '%s'\n", ba
           se->value);
      757      }
      757 -    if (gv->kind != TYPE_STRUCT || !gv->full_type) {          
      758 -        compile_error( "error: '.' applied to non-struct '%s'\
          -n", base->value);                                             
      758 +    if ((gv->kind != TYPE_STRUCT && gv->kind != TYPE_UNION) ||
          + !gv->full_type) {                                            
      759 +        compile_error( "error: '.' applied to non-struct/union
          + '%s'\n", base->value);                                       
      760      }
      761      StructField *f = find_struct_field(gv->full_type, field_na
           me);
      762      if (!f) {
      762 -        compile_error( "error: struct has no member '%s'\n", f
          -ield_name);                                                   
      763 +        compile_error( "error: '%s' has no member '%s'\n",    
      764 +                type_kind_name(gv->kind), field_name);        
      765      }
      766      if (f->offset != 0)
      767          fprintf(cg->output, "    lea rax, [rel %s + %d]\n", gv
           ->name, f->offset);

● Now emit_arrow_addr():

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 5 lines, removed 3 lines
      784      if (base->type == AST_MEMBER_ACCESS) {
      785          StructField *inner = emit_member_addr(cg, base);
      786          if (inner->kind != TYPE_POINTER || !inner->full_type |
           |
      787 -            !inner->full_type->base || inner->full_type->base-
          ->kind != TYPE_STRUCT) {                                       
      788 -            compile_error( "error: '->' applied to non-pointer
          --to-struct\n");                                               
      787 +            !inner->full_type->base || (inner->full_type->base
          +->kind != TYPE_STRUCT &&                                      
      788 +                                        inner->full_type->base
          +->kind != TYPE_UNION)) {                                      
      789 +            compile_error( "error: '->' applied to non-pointer
          +-to-struct/union\n");                                         
      790          }
      791          Type *st = inner->full_type->base;
      792          StructField *f = find_struct_field(st, field_name);
      793          if (!f) {
      793 -            compile_error( "error: struct has no member '%s'\n
          -", field_name);                                               
      794 +            compile_error( "error: '%s' has no member '%s'\n",
      795 +                    type_kind_name(st->kind), field_name);    
      796          }
      797          fprintf(cg->output, "    mov rax, [rax]\n");
      798          if (f->offset != 0)
● Update(src/codegen.c)
  ⎿  Added 5 lines, removed 3 lines
      805          codegen_expression(cg, base);
      806          Type *ptr_type = base->full_type;
      807          if (!ptr_type || ptr_type->kind != TYPE_POINTER ||
      808 -            !ptr_type->base || ptr_type->base->kind != TYPE_ST
          -RUCT) {                                                       
      809 -            compile_error( "error: '->' applied to non-pointer
          --to-struct\n");                                               
      808 +            !ptr_type->base || (ptr_type->base->kind != TYPE_S
          +TRUCT &&                                                      
      809 +                                ptr_type->base->kind != TYPE_U
          +NION)) {                                                      
      810 +            compile_error( "error: '->' applied to non-pointer
          +-to-struct/union\n");                                         
      811          }
      812          StructField *f = find_struct_field(ptr_type->base, fie
           ld_name);
      813          if (!f) {
      813 -            compile_error( "error: struct has no member '%s'\n
          -", field_name);                                               
      814 +            compile_error( "error: '%s' has no member '%s'\n",
      815 +                    type_kind_name(ptr_type->base->kind), fiel
          +d_name);                                                      
      816          }
      817          if (f->offset != 0)
      818              fprintf(cg->output, "    add rax, %d\n", f->offset
           );

● Update(src/codegen.c)
  ⎿  Added 5 lines, removed 3 lines
      823          compile_error( "error: undeclared variable '%s'\n", ba
           se->value);
      824      }
      825      if (lv->kind != TYPE_POINTER || !lv->full_type ||
      826 -        !lv->full_type->base || lv->full_type->base->kind != T
          -YPE_STRUCT) {                                                 
      826 +        !lv->full_type->base || (lv->full_type->base->kind != 
          +TYPE_STRUCT &&                                                
      827 +                                  lv->full_type->base->kind !=
          + TYPE_UNION)) {                                               
      828          compile_error(
      828 -                "error: '->' applied to non-pointer-to-struct 
          -'%s'\n",                                                      
      829 +                "error: '->' applied to non-pointer-to-struct/
          +union '%s'\n",                                                
      830                  base->value);
      831      }
      832      Type *st = lv->full_type->base;
      833      StructField *f = find_struct_field(st, field_name);
      834      if (!f) {
      834 -        compile_error( "error: struct has no member '%s'\n", f
          -ield_name);                                                   
      835 +        compile_error( "error: '%s' has no member '%s'\n",    
      836 +                type_kind_name(st->kind), field_name);        
      837      }
      838      if (lv->is_static)
      839          fprintf(cg->output, "    mov rax, [rel %s]\n", lv->sta
           tic_label);

● Now let me update sizeof_type_of_expr() — the MEMBER_ACCESS and ARROW_ACCESS
  cases:

● Update(src/codegen.c)
  ⎿  Added 8 lines, removed 3 lines
      1047          ASTNode *base = node->children[0];
      1048          if (base->type == AST_VAR_REF) {
      1049              LocalVar *lv = codegen_find_var(cg, base->value);
      1050 -            if (lv && lv->kind == TYPE_STRUCT && lv->full_typ
           -e) {                                                         
      1050 +            if (lv && (lv->kind == TYPE_STRUCT || lv->kind ==
           + TYPE_UNION) &&                                              
      1051 +                lv->full_type) {                             
      1052                  StructField *f = find_struct_field(lv->full_t
            ype, node->value);
      1053                  if (f) return f->kind;
      1054              }
     ...
      1058              base->children[0]->type == AST_VAR_REF) {
      1059              LocalVar *plv = codegen_find_var(cg, base->childr
            en[0]->value);
      1060              if (plv && plv->kind == TYPE_POINTER && plv->full
            _type &&
      1060 -                plv->full_type->base && plv->full_type->base-
           ->kind == TYPE_STRUCT) {                                      
      1061 +                plv->full_type->base &&                      
      1062 +                (plv->full_type->base->kind == TYPE_STRUCT ||
      1063 +                 plv->full_type->base->kind == TYPE_UNION)) {
      1064                  StructField *f = find_struct_field(plv->full_
            type->base, node->value);
      1065                  if (f) return f->kind;
      1066              }
     ...
      1072          if (base->type == AST_VAR_REF) {
      1073              LocalVar *lv = codegen_find_var(cg, base->value);
      1074              if (lv && lv->kind == TYPE_POINTER && lv->full_ty
            pe &&
      1072 -                lv->full_type->base && lv->full_type->base->k
           -ind == TYPE_STRUCT) {                                        
      1075 +                lv->full_type->base &&                       
      1076 +                (lv->full_type->base->kind == TYPE_STRUCT || 
      1077 +                 lv->full_type->base->kind == TYPE_UNION)) { 
      1078                  StructField *f = find_struct_field(lv->full_t
            ype->base, node->value);
      1079                  if (f) return f->kind;
      1080              }

● Now expr_result_type() — MEMBER_ACCESS and ARROW_ACCESS:

● Update(src/codegen.c)
  ⎿  Added 8 lines, removed 3 lines
      1217          ASTNode *base_node = node->children[0];
      1218          if (base_node->type == AST_VAR_REF) {
      1219              LocalVar *lv = codegen_find_var(cg, base_node->va
            lue);
      1220 -            if (lv && lv->kind == TYPE_STRUCT && lv->full_typ
           -e) {                                                         
      1220 +            if (lv && (lv->kind == TYPE_STRUCT || lv->kind ==
           + TYPE_UNION) &&                                              
      1221 +                lv->full_type) {                             
      1222                  StructField *f = find_struct_field(lv->full_t
            ype, node->value);
      1223                  if (f) {
      1224                      t = (f->kind == TYPE_POINTER) ? TYPE_POIN
            TER
     ...
      1232              base_node->children[0]->type == AST_VAR_REF) {
      1233              LocalVar *plv = codegen_find_var(cg, base_node->c
            hildren[0]->value);
      1234              if (plv && plv->kind == TYPE_POINTER && plv->full
            _type &&
      1234 -                plv->full_type->base && plv->full_type->base-
           ->kind == TYPE_STRUCT) {                                      
      1235 +                plv->full_type->base &&                      
      1236 +                (plv->full_type->base->kind == TYPE_STRUCT ||
      1237 +                 plv->full_type->base->kind == TYPE_UNION)) {
      1238                  StructField *f = find_struct_field(plv->full_
            type->base, node->value);
      1239                  if (f) {
      1240                      t = (f->kind == TYPE_POINTER) ? TYPE_POIN
            TER
     ...
      1250          if (base_node->type == AST_VAR_REF) {
      1251              LocalVar *lv = codegen_find_var(cg, base_node->va
            lue);
      1252              if (lv && lv->kind == TYPE_POINTER && lv->full_ty
            pe &&
      1250 -                lv->full_type->base && lv->full_type->base->k
           -ind == TYPE_STRUCT) {                                        
      1253 +                lv->full_type->base &&                       
      1254 +                (lv->full_type->base->kind == TYPE_STRUCT || 
      1255 +                 lv->full_type->base->kind == TYPE_UNION)) { 
      1256                  StructField *f = find_struct_field(lv->full_t
            ype->base, node->value);
      1257                  if (f) {
      1258                      t = (f->kind == TYPE_POINTER) ? TYPE_POIN
            TER

● Now the local variable declaration in codegen_statement() — add TYPE_UNION:

● Update(src/codegen.c)
  ⎿  Added 40 lines, removed 13 lines
      2945           * terminator and zero fill out to the array's declar
            ed size.
      2946           * The literal is not added to the .rodata pool — cod
            egen
      2947           * emits per-byte stack stores instead. */
      2948 -        if (node->decl_type == TYPE_STRUCT && node->full_type
           -) {                                                          
      2949 -            /* Stage 30/32: struct local. Stage 32 adds brace
           --initializer support. */                                     
      2948 +        if ((node->decl_type == TYPE_STRUCT || node->decl_typ
           +e == TYPE_UNION) &&                                          
      2949 +            node->full_type) {                               
      2950 +            /* Stage 30/32: struct local. Stage 32 adds brace
           +-initializer support.                                        
      2951 +             * Stage 72: union local — same machinery, first-
           +member init via brace list. */                               
      2952              if (node->full_type->size == 0) {
      2953                  compile_error(
      2952 -                        "error: variable '%s' has incomplete 
           -struct type\n",                                              
      2953 -                        node->value);                        
      2954 +                        "error: variable '%s' has incomplete 
           +%s type\n",                                                  
      2955 +                        node->value, type_kind_name(node->dec
           +l_type));                                                    
      2956              }
      2957              int size = node->full_type->size;
      2958              int align = node->full_type->alignment;
     ...
      2961              cg->locals[cg->local_count - 1].is_const = node->
            is_const;
      2962              if (node->child_count > 0 &&
      2963                  node->children[0]->type == AST_INITIALIZER_LI
            ST) {
      2962 -                /* Zero-fill the entire struct slot first, th
           -en store provided values. */                                 
      2964 +                /* Zero-fill the entire slot first, then stor
           +e provided values.                                           
      2965 +                 * For unions, only the first member is initi
           +alized. */                                                   
      2966                  for (int b = 0; b < size; b++) {
      2967                      fprintf(cg->output, "    mov byte [rbp - 
            %d], 0\n", offset - b);
      2968                  }
      2966 -                /* Stage 44: delegate to recursive helper. */
      2967 -                emit_local_struct_init(cg, node->full_type, o
           -ffset,                                                       
      2968 -                                       node->children[0]);   
      2969 +                if (node->decl_type == TYPE_UNION) {         
      2970 +                    /* First-member initialization: initializ
           +e only the first field. */                                   
      2971 +                    ASTNode *list = node->children[0];       
      2972 +                    if (list->child_count > 1) {             
      2973 +                        compile_error(                       
      2974 +                                "error: too many initializers
           + for union '%s'\n",                                          
      2975 +                                node->value);                
      2976 +                    }                                        
      2977 +                    if (list->child_count == 1 && node->full_
           +type->field_count > 0) {                                     
      2978 +                        StructField *f = &node->full_type->fi
           +elds[0];                                                     
      2979 +                        int fsize = f->full_type ? f->full_ty
           +pe->size                                                     
      2980 +                                                 : type_kind_
           +bytes(f->kind);                                              
      2981 +                        ASTNode *elem = list->children[0];   
      2982 +                        codegen_expression(cg, elem);        
      2983 +                        int src_is_long = (elem->result_type 
           +== TYPE_LONG ||                                              
      2984 +                                           elem->result_type 
           +== TYPE_POINTER);                                            
      2985 +                        emit_store_local(cg, offset, fsize, s
           +rc_is_long);                                                 
      2986 +                    }                                        
      2987 +                } else {                                     
      2988 +                    emit_local_struct_init(cg, node->full_typ
           +e, offset,                                                   
      2989 +                                           node->children[0])
           +;                                                            
      2990 +                }                                            
      2991              } else if (node->child_count > 0) {
      2970 -                /* Stage 33: struct T d = c — copy from anoth
           -er struct variable. */                                       
      2992 +                /* struct/union T d = c — copy from another v
           +ariable of the same type. */                                 
      2993                  ASTNode *init = node->children[0];
      2994                  if (init->type != AST_VAR_REF) {
      2973 -                    compile_error( "error: struct initializer
           - must be a struct variable\n");                              
      2995 +                    compile_error( "error: %s initializer mus
           +t be a %s variable\n",                                       
      2996 +                            type_kind_name(node->decl_type), 
      2997 +                            type_kind_name(node->decl_type));
      2998                  }
      2999                  LocalVar *src = codegen_find_var(cg, init->va
            lue);
      2976 -                if (!src || src->kind != TYPE_STRUCT || !src-
           ->full_type) {                                                
      2977 -                    compile_error( "error: struct initializer
           - must be a struct variable\n");                              
      3000 +                if (!src || src->kind != node->decl_type || !
           +src->full_type) {                                            
      3001 +                    compile_error( "error: %s initializer mus
           +t be a %s variable\n",                                       
      3002 +                            type_kind_name(node->decl_type), 
      3003 +                            type_kind_name(node->decl_type));
      3004                  }
      3005                  if (src->full_type != node->full_type) {
      2980 -                    compile_error( "error: incompatible struc
           -t types in initializer for '%s'\n", node->value);            
      3006 +                    compile_error( "error: incompatible %s ty
           +pes in initializer for '%s'\n",                              
      3007 +                            type_kind_name(node->decl_type), 
           +node->value);                                                
      3008                  }
      3009                  for (int b = 0; b < size; b++) {
      3010                      fprintf(cg->output, "    movzx eax, byte 
            [rbp - %d]\n", src->offset - b);

● Now the static local check — add TYPE_UNION to the not-yet-supported list:

● Update(src/codegen.c)
  ⎿  Added 3 lines, removed 2 lines
      2869          }
      2870          /* Stage 71: block-scope static variable. */
      2871          if (node->storage_class == SC_STATIC) {
      2872 -            if (node->decl_type == TYPE_ARRAY || node->decl_t
           -ype == TYPE_STRUCT) {                                        
      2872 +            if (node->decl_type == TYPE_ARRAY || node->decl_t
           +ype == TYPE_STRUCT ||                                        
      2873 +                node->decl_type == TYPE_UNION) {             
      2874                  compile_error(
      2874 -                        "error: static local arrays and struc
           -ts are not yet supported\n");                                
      2875 +                        "error: static local arrays, structs 
           +and unions are not yet supported\n");                        
      2876              }
      2877              if (cg->local_static_count >= MAX_LOCAL_STATICS) 
            {
      2878                  compile_error(

● Now the assignment — struct-to-struct byte copy needs to handle union-to-union
   too:

● Update(src/codegen.c)
  ⎿  Added 10 lines, removed 7 lines
      1539                  compile_error(
      1540                          "error: assignment to const variable 
            '%s'\n", lv->name);
      1541              }
      1542 -            /* Stage 33: struct-to-struct assignment — byte c
           -opy of sizeof(T) bytes. */                                   
      1543 -            if (lv->kind == TYPE_STRUCT && lv->full_type) {  
      1542 +            /* Stage 33/72: struct/union-to-struct/union assi
           +gnment — byte copy. */                                       
      1543 +            if ((lv->kind == TYPE_STRUCT || lv->kind == TYPE_
           +UNION) && lv->full_type) {                                   
      1544                  if (node->child_count < 1 || node->children[0
            ]->type != AST_VAR_REF) {
      1545 -                    compile_error( "error: struct assignment 
           -requires a struct variable\n");                              
      1545 +                    compile_error( "error: %s assignment requ
           +ires a %s variable\n",                                       
      1546 +                            type_kind_name(lv->kind), type_ki
           +nd_name(lv->kind));                                          
      1547                  }
      1548                  LocalVar *src = codegen_find_var(cg, node->ch
            ildren[0]->value);
      1548 -                if (!src || src->kind != TYPE_STRUCT || !src-
           ->full_type) {                                                
      1549 -                    compile_error( "error: cannot assign non-
           -struct to struct '%s'\n", lv->name);                         
      1549 +                if (!src || src->kind != lv->kind || !src->fu
           +ll_type) {                                                   
      1550 +                    compile_error( "error: cannot assign non-
           +%s to %s '%s'\n",                                            
      1551 +                            type_kind_name(lv->kind), type_ki
           +nd_name(lv->kind), lv->name);                                
      1552                  }
      1553                  if (src->full_type != lv->full_type) {
      1552 -                    compile_error( "error: incompatible struc
           -t types in assignment to '%s'\n", lv->name);                 
      1554 +                    compile_error( "error: incompatible %s ty
           +pes in assignment to '%s'\n",                                
      1555 +                            type_kind_name(lv->kind), lv->nam
           +e);                                                          
      1556                  }
      1557                  int sz = lv->full_type->size;
      1558                  for (int b = 0; b < sz; b++) {
      1559                      fprintf(cg->output, "    movzx eax, byte 
            [rbp - %d]\n", src->offset - b);
      1560                      fprintf(cg->output, "    mov [rbp - %d], 
            al\n", lv->offset - b);
      1561                  }
      1559 -                node->result_type = TYPE_STRUCT;             
      1562 +                node->result_type = lv->kind;                
      1563                  node->full_type = lv->full_type;
      1564                  return;
      1565              }

● Now AST_SIZEOF_TYPE and AST_SIZEOF_EXPR:

● Update(src/codegen.c)
  ⎿  Added 4 lines, removed 2 lines
      1814      }
      1815      if (node->type == AST_SIZEOF_TYPE) {
      1816          int sz;
      1817 -        if (node->decl_type == TYPE_STRUCT && node->full_type
           -) {                                                          
      1817 +        if ((node->decl_type == TYPE_STRUCT || node->decl_typ
           +e == TYPE_UNION) &&                                          
      1818 +            node->full_type) {                               
      1819              if (node->full_type->size == 0) {
      1819 -                compile_error( "error: sizeof applied to inco
           -mplete struct type\n");                                      
      1820 +                compile_error( "error: sizeof applied to inco
           +mplete %s type\n",                                           
      1821 +                        type_kind_name(node->decl_type));    
      1822              }
      1823              sz = node->full_type->size;
      1824          } else {

● Now AST_SIZEOF_EXPR for local and global union variables:

● Update(src/codegen.c)
  ⎿  Added 4 lines, removed 2 lines
      1846                  node->result_type = TYPE_LONG;
      1847                  return;
      1848              }
      1849 -            if (lv && lv->kind == TYPE_STRUCT && lv->full_typ
           -e) {                                                         
      1849 +            if (lv && (lv->kind == TYPE_STRUCT || lv->kind ==
           + TYPE_UNION) &&                                              
      1850 +                lv->full_type) {                             
      1851                  fprintf(cg->output, "    mov rax, %d\n", lv->
            full_type->size);
      1852                  node->result_type = TYPE_LONG;
      1853                  return;
     ...
      1859                      node->result_type = TYPE_LONG;
      1860                      return;
      1861                  }
      1861 -                if (gv && gv->kind == TYPE_STRUCT && gv->full
           -_type) {                                                     
      1862 +                if (gv && (gv->kind == TYPE_STRUCT || gv->kin
           +d == TYPE_UNION) &&                                          
      1863 +                    gv->full_type) {                         
      1864                      fprintf(cg->output, "    mov rax, %d\n", 
            gv->full_type->size);
      1865                      node->result_type = TYPE_LONG;
      1866                      return;

● Now codegen_add_global() — handle TYPE_UNION like TYPE_STRUCT:

● Update(src/codegen.c)
  ⎿  Added 3 lines, removed 2 lines
      3742              /* Stage 43: int values[] = {10,20,30} or char *n
            ames[] = {...}. */
      3743              gv->init_node = init;
      3744              gv->is_initialized = 1;
      3745 -        } else if (gv->kind == TYPE_STRUCT && init->type == A
           -ST_INITIALIZER_LIST) {                                       
      3746 -            /* Stage 44: struct Point p = {3, 4}. */         
      3745 +        } else if ((gv->kind == TYPE_STRUCT || gv->kind == TY
           +PE_UNION) &&                                                 
      3746 +                   init->type == AST_INITIALIZER_LIST) {     
      3747 +            /* Stage 44/72: struct/union global initialized f
           +rom brace list. */                                           
      3748              gv->init_node = init;
      3749              gv->is_initialized = 1;
      3750          }

● Now codegen_emit_data() — handle TYPE_UNION global initialized from brace
  list. For unions, we use emit_global_struct (which handles all the fields the
  same way) but for unions, all fields are at offset 0 so only the first is
  emitted:

● Update(src/codegen.c)
  ⎿  Added 34 lines, removed 3 lines
      3857                  fprintf(cg->output, "%d", b);
      3858              }
      3859              fprintf(cg->output, "\n");
      3860 -        } else if (gv->init_node && gv->kind == TYPE_STRUCT &
           -&                                                            
      3860 +        } else if (gv->init_node &&                          
      3861 +                   (gv->kind == TYPE_STRUCT || gv->kind == TY
           +PE_UNION) &&                                                 
      3862                     gv->init_node->type == AST_INITIALIZER_LIS
            T) {
      3862 -            /* Stage 44: struct global initialized from brace
           --list. */                                                    
      3863 +            /* Stage 44/72: struct/union global initialized f
           +rom brace-list. */                                           
      3864              fprintf(cg->output, "%s:\n", gv->name);
      3864 -            emit_global_struct(cg, gv->full_type, gv->init_no
           -de);                                                         
      3865 +            if (gv->kind == TYPE_UNION) {                    
      3866 +                /* Union: initialize first member, zero-fill 
           +the rest. */                                                 
      3867 +                ASTNode *list = gv->init_node;               
      3868 +                int cur_off = 0;                             
      3869 +                if (list->child_count > 1) {                 
      3870 +                    compile_error( "error: too many initializ
           +ers for union '%s'\n",                                       
      3871 +                            gv->name);                       
      3872 +                }                                            
      3873 +                if (list->child_count == 1 && gv->full_type->
           +field_count > 0) {                                           
      3874 +                    StructField *f = &gv->full_type->fields[0
           +];                                                           
      3875 +                    int fsize = f->full_type ? f->full_type->
           +size : type_kind_bytes(f->kind);                             
      3876 +                    ASTNode *elem = list->children[0];       
      3877 +                    if (elem->type == AST_INT_LITERAL) {     
      3878 +                        long v = strtol(elem->value, NULL, 10
           +);                                                           
      3879 +                        fprintf(cg->output, "    %s %ld\n", d
           +ata_init_directive(f->kind), v);                             
      3880 +                    } else if (elem->type == AST_CHAR_LITERAL
           +) {                                                          
      3881 +                        long v = (long)(unsigned char)elem->v
           +alue[0];                                                     
      3882 +                        fprintf(cg->output, "    %s %ld\n", d
           +ata_init_directive(f->kind), v);                             
      3883 +                    } else {                                 
      3884 +                        compile_error( "error: unsupported in
           +itializer for union '%s'\n",                                 
      3885 +                                gv->name);                   
      3886 +                    }                                        
      3887 +                    cur_off = fsize;                         
      3888 +                }                                            
      3889 +                while (cur_off < gv->full_type->size) {      
      3890 +                    fprintf(cg->output, "    db 0\n");       
      3891 +                    cur_off++;                               
      3892 +                }                                            
      3893 +            } else {                                         
      3894 +                emit_global_struct(cg, gv->full_type, gv->ini
           +t_node);                                                     
      3895 +            }                                                
      3896          } else if (gv->init_node && gv->kind == TYPE_ARRAY &&
      3897                     gv->init_node->type == AST_INITIALIZER_LIS
            T) {
      3898              /* Stage 43: int values[] = {10,20,30} or char *n
            ames[] = {...}.

● Now codegen_emit_bss() — handle struct and union globals with correct byte
  allocation:

● Update(src/codegen.c)
  ⎿  Added 4 lines
      3971                      gv->name,
      3972                      bss_res_directive(gv->full_type->base->ki
            nd),
      3973                      gv->full_type->length);
      3974 +        } else if ((gv->kind == TYPE_STRUCT || gv->kind == TY
           +PE_UNION) &&                                                 
      3975 +                   gv->full_type) {                          
      3976 +            /* Reserve the exact byte count for struct/union 
           +globals. */                                                  
      3977 +            fprintf(cg->output, "%s: resb %d\n", gv->name, gv
           +->full_type->size);                                          
      3978          } else {
      3979              fprintf(cg->output, "%s: %s 1\n",
      3980                      gv->name, bss_res_directive(gv->kind));

● Now build and test:

● Bash(cmake --build build 2>&1 | head -20)
  ⎿  [  9%] Building C object CMakeFiles/ccompiler.dir/src/codegen.c.o
     [ 18%] Linking C executable ccompiler
     [100%] Built target ccompiler

● Bash(./build/ccompiler /tmp/test_union_basic.c && nasm -f elf64
      /tmp/test_union_basic.asm -o /tmp/test_union_basic.o && gcc -no-pie
      /tmp/test_union_basic.o -o /tmp/t…)
  ⎿  compiled: /tmp/test_union_basic.c -> test_union_basic.asm
     nasm: fatal: unable to open input file `/tmp/test_union_basic.asm' No
     such file or directory
     exit: 1

● Bash(./build/ccompiler /tmp/test_union_basic.c && nasm -f elf64
      test_union_basic.asm -o /tmp/test_union_basic.o && gcc -no-pie
      /tmp/test_union_basic.o -o /tmp/test_u…)  ⎿  compiled: /tmp/test_union_basic.c -> test_union_basic.asm
     exit: 42

● Basic union works. Let me now test all the spec tests inline:

● Bash(cat > /tmp/test_union_sizeof.c << 'EOF'
      union U {…)
  ⎿  compiled: /tmp/test_union_sizeof.c -> test_union_sizeof.asm
     exit: 8

● Bash(cat > /tmp/test_union_sizeof_var.c << 'EOF'
      union U {…)
  ⎿  compiled: /tmp/test_union_sizeof_var.c -> test_union_sizeof_var.asm
     exit: 8

● Bash(cat > /tmp/test_union_ptr.c << 'EOF'
      union U {…)
  ⎿  compiled: /tmp/test_union_ptr.c -> test_union_ptr.asm
     exit: 42

● Bash(cat > /tmp/test_union_in_struct.c << 'EOF'
      union U {…)
  ⎿  compiled: /tmp/test_union_in_struct.c -> test_union_in_struct.asm
     exit: 42

● Bash(cat > /tmp/test_struct_in_union.c << 'EOF'
      struct Pair {…)
  ⎿  compiled: /tmp/test_struct_in_union.c -> test_struct_in_union.asm
     exit: 42

● Bash(cat > /tmp/test_union_assign.c << 'EOF'
      union U {…)
  ⎿  compiled: /tmp/test_union_assign.c -> test_union_assign.asm
     exit: 42

● Bash(cat > /tmp/test_union_global.c << 'EOF'
      union U {…)
  ⎿  compiled: /tmp/test_union_global.c -> test_union_global.asm
     exit: 42

● Bash(cat > /tmp/test_union_init.c << 'EOF'
      union U {…)
  ⎿  compiled: /tmp/test_union_init.c -> test_union_init.asm
     exit: 42

● All spec tests pass. Now let me test the invalid test:

● Bash(cat > /tmp/test_union_incomplete.c << 'EOF'
      union U;…)
  ⎿  error: variable 'u' has incomplete union type
     exit: 1

● All tests pass. Now let me run the full test suite:

● Bash(./test/run_all_tests.sh 2>&1 | tail -20)
  ⎿  PASS  test_stage_14_07_libc_puts
     PASS  test_stage_16_01_remainder_int
     PASS  test_stage_16_01_remainder_long
     … +17 lines (ctrl+o to expand)

● Two tests failing. Let me find which ones:

● Bash(./test/run_all_tests.sh 2>&1 | grep "FAIL")
  ⎿  FAIL  test_struct_incomplete_member__has_incomplete_struct_type  (expe
     cted error containing 'has incomplete struct type', got: /home/scott/c
     ode/claude/claudec99/test/invalid/test_struct_incomplete_member__has_i
     … +5 lines (ctrl+o to expand)

● I changed the struct error message which broke existing tests. Let me restore
  it to keep "has incomplete struct type" for struct fields:

● Update(src/parser.c)
  ⎿  Added 6 lines, removed 3 lines
      374  
      375                  /* Stage 37/72: a non-pointer field of an inco
           mplete struct or
      376                   * union type is invalid; pointer-to-incomplet
           e is allowed. */
      377 -                if ((field_type->kind == TYPE_STRUCT || field_
          -type->kind == TYPE_UNION) &&                                  
      378 -                    field_type->size == 0) {                  
      377 +                if (field_type->kind == TYPE_STRUCT && field_t
          +ype->size == 0) {                                             
      378                      PARSER_ERROR(parser,
      380 -                            "error: field '%s' has incomplete 
          -type\n", d.name);                                             
      379 +                            "error: field '%s' has incomplete 
          +struct type\n", d.name);                                      
      380                  }
      381 +                if (field_type->kind == TYPE_UNION && field_ty
          +pe->size == 0) {                                              
      382 +                    PARSER_ERROR(parser,                      
      383 +                            "error: field '%s' has incomplete 
          +union type\n", d.name);                                       
      384 +                }                                             
      385  
      386                  int fsz   = type_size(field_type);
      387                  int falign = type_alignment(field_type);

● Wait, that leaves the if block with a double-close brace problem. Let me read
  the context:

  Read 1 file (ctrl+o to expand)

● Good, the structure is fine. Let me build and run tests:

● Bash(cmake --build build 2>&1 | tail -3 && ./test/run_all_tests.sh 2>&1 | grep
       -E "FAIL|Aggregate")
  ⎿  [  9%] Building C object CMakeFiles/ccompiler.dir/src/parser.c.o
     [ 18%] Linking C executable ccompiler
     [100%] Built target ccompiler
     Aggregate: 1148 passed, 0 failed, 1148 total

● All 1148 existing tests pass. Now let me add the new tests. Let me create the
  test files:

  Step 5: Add tests

  Read 2 files (ctrl+o to expand)
● Now run the test suite to verify:

● Bash(./test/run_all_tests.sh 2>&1 | grep -E "FAIL|Aggregate")
  ⎿  Aggregate: 1158 passed, 0 failed, 1158 total

● All 1158 tests pass (1148 existing + 10 new: 9 valid + 1 invalid). Let me
  check the test breakdown:

● Bash(./test/run_all_tests.sh 2>&1 | grep "Results:")
  ⎿  Results: 718 passed, 0 failed, 718 total
     Results: 214 passed, 0 failed, 214 total
     Results: 67 passed, 0 failed, 67 total
     … +3 lines (ctrl+o to expand)

● Now let me update docs/grammar.md and README.md. Let me first check the
  grammar file:

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Now I'll delegate the docs updates to the haiku agent:

● haiku-stage-artifact-writer(Update grammar.md and README.md for stage 72)
  ⎿  Done (9 tool uses · 23.5k tokens · 30s)
  (ctrl+o to expand)

● Let me verify what was updated:

  Searched for 2 patterns, read 1 file (ctrl+o to expand)

● Let me check the top of grammar.md and the struct section changes:

  Read 1 file (ctrl+o to expand)

● The haiku agent added extraneous comments to grammar.md. Let me remove them
  and check if the original comment style was preserved:

  Read 1 file (ctrl+o to expand)

● Update(docs/grammar.md)
  ⎿  Removed 4 lines
      1  ```ebnf
      2  # Claude C99 Grammar (Current through Stage 72)
      3 -#                                                               
      4 -# Note: Stage 72 adds union support with the same layout and sem
        -antics as structs.                                              
      5 -# Grammar change: <union_specifier> added as a type_specifier al
        -ternative.                                                      
      6 -#                                                               
      3  
      4  
      5  <translation_unit> ::= <external_declaration> { <external_declar
         ation> }

● Now let me also check how the README structs bullet looks — the haiku agent
  may have rewritten it too aggressively:

  Read 1 file (ctrl+o to expand)

● The haiku agent added a redundant trailing sentence after rewriting the
  bullet. Let me remove it:

● Update(README.md)
  ⎿  Added 3 lines, removed 1 line
      300    Struct/union fields of type `char *` initialized from string
            literals in file-scope contexts.
      301    Field-level type checking for aggregate initializers: string
            literal for non-pointer field
      302    and non-null integer for pointer field are rejected.
      303 -  Named `union` types with max-size layout, local and global u
          -nion variables, member access via `.` and `->`, whole-union as
          -signment, first-member initialization, and `sizeof`; union fie
          -lds inside structs and struct fields inside unions are support
          -ed.                                                           
      303 +  Named `union` types with max-size layout (size = max member,
          + all offsets 0), local and global                             
      304 +  union variables, first-member initialization, whole-union as
          +signment, union fields inside structs,                        
      305 +  and struct fields inside unions.                            
      306  - **File-scope objects**: file-scope (global) object declarati
           ons (scalars,
      307    pointers, arrays, structs), both initialized (with constant 
           integer expressions,
      308    string-literal initialization for pointer globals, brace-lis
           t initialization for array globals,

● Now let me run the full test suite one more time to confirm everything is
  clean:

● Bash(./test/run_all_tests.sh 2>&1 | grep -E "FAIL|Aggregate")
  ⎿  Aggregate: 1158 passed, 0 failed, 1158 total

● All 1158 tests pass. Now let me also check that the ast_pretty_printer.c
  handles TYPE_UNION (it may have fallthrough cases):

  Searched for 2 patterns (ctrl+o to expand)

● The pretty printer uses type_kind_name() which now handles TYPE_UNION, so it
  will display "union" correctly. Let me verify:

● Bash(./build/ccompiler --print-ast /tmp/test_union_basic.c 2>&1 | head -20)
  ⎿  TranslationUnit:
       TypedefDecl:
       FunctionDecl: int main
     … +10 lines (ctrl+o to expand)

● Good. Now let me commit the changes. Let me prepare the commit:

● Bash(git status)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +28 lines (ctrl+o to expand)

● Bash(git log --oneline -3)  ⎿  26e2c77 fixed type in file name
     33eca17 stage 72 spec
     45fe7f8 stage 71 session usage and export

● Bash(git add include/token.h include/type.h include/parser.h src/lexer.c
      src/type.c src/parser.c src/codegen.c README.md docs/grammar.md
      docs/kickoffs/stage-72-kicko…)
  ⎿  [master 50d8a02] stage 72: add named union type support
      20 files changed, 607 insertions(+), 102 deletions(-)
      create mode 100644 docs/kickoffs/stage-72-kickoff.md
     … +11 lines (ctrl+o to expand)

● Committed. Now let me generate the milestone and transcript artifacts:

● haiku-stage-artifact-writer(Generate milestone and transcript artifacts for
                             stage 72)
  ⎿  Done (5 tool uses · 11.4k tokens · 23s)
  (ctrl+o to expand)

● Bash(git add docs/milestones/stage-72-milestone.md
      docs/sessions/stage-72-transcript.md && git commit -m "$(cat <<'EOF'
      stage 72: add milestone and transcript artifa…)
  ⎿  [master d65f69a] stage 72: add milestone and transcript artifacts
      2 files changed, 105 insertions(+)
      create mode 100644 docs/milestones/stage-72-milestone.md
      create mode 100644 docs/sessions/stage-72-transcript.md

● Stage 72 is complete. Here's what was implemented:

  Summary

  Stage 72: Named Union Support — all 1158 tests pass (718 valid, 214 invalid,
  67 integration).

  What changed

  ┌──────────────────┬───────────────────────────────────────────────────────┐
  │       File       │                        Change                         │
  ├──────────────────┼───────────────────────────────────────────────────────┤
  │ include/token.h  │ Added TOKEN_UNION                                     │
  ├──────────────────┼───────────────────────────────────────────────────────┤
  │ src/lexer.c      │ "union" keyword recognition                           │
  ├──────────────────┼───────────────────────────────────────────────────────┤
  │ include/type.h   │ Added TYPE_UNION, declared type_union()               │
  ├──────────────────┼───────────────────────────────────────────────────────┤
  │ src/type.c       │ Implemented type_union(), updated kind name and       │
  │                  │ integer check                                         │
  ├──────────────────┼───────────────────────────────────────────────────────┤
  │ include/parser.h │ Added UnionTag, union tag table in Parser             │
  ├──────────────────┼───────────────────────────────────────────────────────┤
  │ src/parser.c     │ parse_union_specifier(), TOKEN_UNION/TYPE_UNION in    │
  │                  │ all declaration and sizeof paths                      │
  ├──────────────────┼───────────────────────────────────────────────────────┤
  │ src/codegen.c    │ TYPE_UNION in member access, sizeof, local/global     │
  │                  │ declarations, assignment, bss emission                │
  ├──────────────────┼───────────────────────────────────────────────────────┤
  │ docs/grammar.md  │ Added <union_specifier> rule                          │
  ├──────────────────┼───────────────────────────────────────────────────────┤
  │ README.md        │ Updated stage line, Structs→"Structs and Unions"      │
  │                  │ bullet, test totals                                   │
  ├──────────────────┼───────────────────────────────────────────────────────┤
  │ 10 new tests     │ 9 valid + 1 invalid                                   │
  └──────────────────┴───────────────────────────────────────────────────────┘

  Spec issue noted: The spec's "Union inside struct" test had a typo (union 
  Value instead of union U); corrected in the test file.

  Side fix: Global struct/union uninitialized BSS allocation now correctly uses
  resb <size> instead of the incorrect resd 1 that existed for structs.

✻ Cogitated for 18m 46s