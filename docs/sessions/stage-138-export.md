 ▐▛███▜▌   Claude Code v2.1.181
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99


❯ /usage 
  ⎿  Settings dialog dismissed

❯ /implement-stage                                                              
docs/stages/ClaudeC99-spec-stage-138-support-auto-and-register.md               

● I'll start by reading the spec and supporting files in parallel.

  Read 4 files (ctrl+o to expand)

● STAGE_DISPLAY: Stage 138

  Now let me read the kickoff template and examine the relevant source files.

  Searched for 6 patterns, read 2 files (ctrl+o to expand)

● Now let me look at the lexer's keyword list and the parser's storage class
  handling:

  Searched for 4 patterns (ctrl+o to expand)

● Now let me look at the relevant sections of the parser more carefully:

  Read 1 file (ctrl+o to expand)

● Bash(sed -n '3860,3960p' /home/scott/code/claude/claudec99/src/parser.c)
  ⎿          ast_add_child(func, next);
         }
     }
     … +98 lines (ctrl+o to expand)

● Now let me look at parameter declaration parsing and the block-scope local
  variable parsing:

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Bash(sed -n '3700,3780p' /home/scott/code/claude/claudec99/src/parser.c)
  ⎿   * the parameter is unnamed and only the base type is recorded.  Unnam
     ed
      * parameters are allowed in function prototypes and function pointer
     … +78 lines (ctrl+o to expand)

● Bash(sed -n '3240,3310p' /home/scott/code/claude/claudec99/src/parser.c)
  ⎿      parser->switch_depth++;
         ASTNode *body = parse_statement(parser);
         parser->switch_depth--;
     … +68 lines (ctrl+o to expand)

● Let me look at the rest of parse_statement to understand how block
  declarations are handled:

● Bash(sed -n '3310,3450p' /home/scott/code/claude/claudec99/src/parser.c)
  ⎿          if (d.is_func_pointer) {
                 Type *fp_type = build_fp_type(base_type, &d);
                 parser_register_typedef(parser, d.name, TYPE_POINTER, fp_t
     … +138 lines (ctrl+o to expand)

● Let me check where parse_declaration_specifiers is called and how block-scope
  vars work:

  Searched for 1 pattern (ctrl+o to expand)

● Bash(sed -n '3955,3975p' /home/scott/code/claude/claudec99/src/parser.c)
  ⎿   * <storage_class_specifier>::= "extern" | "static"
      * <type_qualifier>         ::= "const"
      *
     … +19 lines (ctrl+o to expand)

● Let me examine the LocalVar struct and the codegen address-of handling:

  Searched for 2 patterns, read 1 file (ctrl+o to expand)
● Bash(sed -n '1905,1945p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿          }
             return TYPE_INT; /* ! */
         }
     … +38 lines (ctrl+o to expand)

● Bash(sed -n '2080,2150p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿                  t = gv->kind;
                 } else {
                     t = gv ? promote_kind(type_kind_from_size(gv->size)) :
     … +68 lines (ctrl+o to expand)

● Let me look at the codegen AST_ADDR_OF expression emission:

● Bash(sed -n '3155,3220p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿          if (gv->kind == TYPE_POINTER) {
                 node->result_type = TYPE_POINTER;
                 node->full_type = gv->full_type;
     … +63 lines (ctrl+o to expand)

  Searched for 2 patterns (ctrl+o to expand)

● Bash(sed -n '720,760p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿   * size is element_size * length and is not a power of two in general)
      * align to the element's natural alignment instead of their total
      * byte count.
     … +38 lines (ctrl+o to expand)

● Bash(sed -n '5665,5720p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿      }
         compile_error(
     "error: initializer for block-scope static '%s' must be a "
     … +53 lines (ctrl+o to expand)

● Let me look at how variables are registered in the pre-scan phase vs the
  statement emission phase:

  Searched for 1 pattern (ctrl+o to expand)

● Bash(sed -n '6540,6620p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿              /* Stage 122: the rbx slot shifts the save-area base by 8;
      add 8 bytes
                  * of alignment padding so the variadic register save area
     … +78 lines (ctrl+o to expand)

● Let me look at the pre-scan loop that registers locals:

● Bash(sed -n '5835,5935p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿              node->full_type) {
                 /* Stage 30/32: struct local. Stage 32 adds brace-initiali
     zer support.
     … +98 lines (ctrl+o to expand)

● Let me look at where scalar declarations are registered to understand where to
  add the register check:

● Bash(sed -n '6010,6050p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿              {
                     LocalVar *last_lv = (LocalVar *)vec_get(&cg->locals, c
     g->locals.len - 1);
     … +38 lines (ctrl+o to expand)

● Let me look at the codegen_function to understand the pre-scan approach:

● Bash(sed -n '6430,6510p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿          if (!user_label_defined(cg, node->value)) {
                 compile_error( "error: undefined label '%s' in function '%
     s'\n",
     … +78 lines (ctrl+o to expand)

● Bash(sed -n '6510,6560p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿              TypeKind pt = node->children[i]->decl_type;
                 if (is_struct_or_union_kind(pt) || type_is_fp(pt))
                     prologue_struct = 1;
     … +48 lines (ctrl+o to expand)

  Searched for 1 pattern (ctrl+o to expand)

● Bash(sed -n '1625,1710p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿      if (node->type == AST_DECLARATION) {
             if (node->decl_type == TYPE_ARRAY && node->full_type) {
                 total = node->full_type->size + 7;
     … +83 lines (ctrl+o to expand)

● Now I have enough to understand the full scope. Let me check the existing test
  structure and the lexer keyword section more carefully:

  Listed 1 directory (ctrl+o to expand)
● Bash(sed -n '650,690p' /home/scott/code/claude/claudec99/src/lexer.c)
  ⎿          else if (strcmp(ident_buf.data, "do")       == 0) { token.type
      = TOKEN_DO;       token.value = "do";       token.value_len = 2; }
             else if (strcmp(ident_buf.data, "for")      == 0) { token.type
     … +54 lines (ctrl+o to expand)

● Bash(sed -n '700,760p' /home/scott/code/claude/claudec99/src/lexer.c)
  ⎿  const char *token_display_name(TokenType type) {
         switch (type) {
             case TOKEN_EOF:              return "<EOF>";
     … +58 lines (ctrl+o to expand)

● Now I have all the information needed. Let me delegate the kickoff artifact to
  haiku and start implementing in parallel.

● haiku-stage-artifact-writer(Generate stage-138 kickoff artifact)
  ⎿  Backgrounded agent (↓ to manage · ctrl+o to expand)

● Planned Changes:

  ┌──────────────────────────┬───────────────────────────────────────────────┐
  │           File           │                    Change                     │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ include/token.h          │ Add TOKEN_AUTO, TOKEN_REGISTER                │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ src/lexer.c              │ Keyword recognition + display names           │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ include/ast.h            │ Add SC_AUTO=8, SC_REGISTER=16                 │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ include/codegen.h        │ Add is_register to LocalVar                   │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ src/codegen.c            │ Init is_register; set for SC_REGISTER vars;   │
  │                          │ reject &register_var                          │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ src/parser.c             │ Reject both at file scope; handle both at     │
  │                          │ block scope; register as param qualifier      │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ src/ast_pretty_printer.c │ Display auto/register storage classes         │
  ├──────────────────────────┼───────────────────────────────────────────────┤
  │ src/version.c            │ Bump to 13800000                              │
  └──────────────────────────┴───────────────────────────────────────────────┘

  Starting implementation now.

  Step 1: Add tokens to token.h

  Read 1 file (ctrl+o to expand)

● Update(include/token.h)
  ⎿  Added 2 lines
      41      TOKEN_VOLATILE,
      42      TOKEN_RESTRICT,
      43      TOKEN_INLINE,
      44 +    TOKEN_AUTO,                                                
      45 +    TOKEN_REGISTER,                                            
      46      TOKEN_SIGNED,
      47      TOKEN_UNSIGNED,
      48      TOKEN_IDENTIFIER,

● Step 2: Update lexer.c - keyword recognition and display names

● Update(src/lexer.c)
  ⎿  Added 2 lines
      669          else if (strcmp(ident_buf.data, "volatile") == 0) { to
           ken.type = TOKEN_VOLATILE; token.value = "volatile"; token.val
           ue_len = 8; }
      670          else if (strcmp(ident_buf.data, "restrict") == 0) { to
           ken.type = TOKEN_RESTRICT; token.value = "restrict"; token.val
           ue_len = 8; }
      671          else if (strcmp(ident_buf.data, "inline")   == 0) { to
           ken.type = TOKEN_INLINE;   token.value = "inline";   token.val
           ue_len = 6; }
      672 +        else if (strcmp(ident_buf.data, "auto")     == 0) { to
          +ken.type = TOKEN_AUTO;     token.value = "auto";     token.val
          +ue_len = 4; }                                                 
      673 +        else if (strcmp(ident_buf.data, "register") == 0) { to
          +ken.type = TOKEN_REGISTER; token.value = "register"; token.val
          +ue_len = 8; }                                                 
      674          else if (strcmp(ident_buf.data, "signed")   == 0) { to
           ken.type = TOKEN_SIGNED;   token.value = "signed";   token.val
           ue_len = 6; }
      675          else if (strcmp(ident_buf.data, "unsigned") == 0) { to
           ken.type = TOKEN_UNSIGNED; token.value = "unsigned"; token.val
           ue_len = 8; }
      676          else if (strcmp(ident_buf.data, "_Bool")    == 0) { to
           ken.type = TOKEN_BOOL;     token.value = "_Bool";    token.val
           ue_len = 5; }

● Now add the display names in token_display_name:

● Update(src/lexer.c)
  ⎿  Added 2 lines
      733          case TOKEN_VOLATILE:         return "'volatile'";
      734          case TOKEN_RESTRICT:         return "'restrict'";
      735          case TOKEN_INLINE:           return "'inline'";
      736 +        case TOKEN_AUTO:             return "'auto'";         
      737 +        case TOKEN_REGISTER:         return "'register'";     
      738          case TOKEN_SIGNED:           return "'signed'";
      739          case TOKEN_UNSIGNED:         return "'unsigned'";
      740          case TOKEN_IDENTIFIER:       return "identifier";

● Step 3: Add SC_AUTO and SC_REGISTER to ast.h

● Update(include/ast.h)
  ⎿  Added 6 lines, removed 4 lines
       4  #include "type.h"
       5  
       6  typedef enum {
       7 -    SC_NONE    = 0,                                            
       8 -    SC_EXTERN  = 1,                                            
       9 -    SC_STATIC  = 2,                                            
      10 -    SC_TYPEDEF = 4                                             
       7 +    SC_NONE     = 0,                                           
       8 +    SC_EXTERN   = 1,                                           
       9 +    SC_STATIC   = 2,                                           
      10 +    SC_TYPEDEF  = 4,                                           
      11 +    SC_AUTO     = 8,                                           
      12 +    SC_REGISTER = 16                                           
      13  } StorageClass;
      14  
      15  typedef enum {

● Step 4: Add is_register to LocalVar in codegen.h

● Update(include/codegen.h)
  ⎿  Added 2 lines
      27      int is_static;
      28      /* Stage 95-11: pointer into util_strdup'd storage for the 
          generated label. */
      29      const char *static_label;
      30 +    /* Stage 138: set when declared register; address-of is for
         +bidden. */                                                     
      31 +    int is_register;                                           
      32  } LocalVar;
      33  
      34  /* Stage 71: one entry per block-scope static variable declared
           during

● Step 5: Update codegen.c

  Initialize is_register = 0 in codegen_add_var:

● Update(src/codegen.c)
  ⎿  Added 1 line
      738       * consult this flag). Static locals are registered via a 
           separate path. */
      739      new_lv.is_static = 0;
      740      new_lv.static_label = NULL;
      741 +    new_lv.is_register = 0;                                   
      742      vec_push(&cg->locals, &new_lv);
      743      return cg->stack_offset;
      744  }

● Agent "Generate stage-138 kickoff artifact" came to rest · 32s

● The kickoff artifact was generated. Now let me continue with the is_register
  tracking in codegen_statement. I need to find the places where
  last_lv->is_const is set after codegen_add_var and add a parallel is_register
  assignment.

  Let me look at all the places that set is_const after codegen_add_var:

  Searched for 1 pattern (ctrl+o to expand)

● Now I'll add is_register tracking at each declaration site in
  codegen_statement. Let me read the four contexts:

● Bash(sed -n '5847,5855p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿                                           node->decl_type, node->full_t
     ype);
                 {
     … +10 lines (ctrl+o to expand)

● Bash(sed -n '5923,5930p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿                                           node->decl_type, node->full_t
     ype);
                 {
     … +7 lines (ctrl+o to expand)

● Bash(sed -n '6010,6035p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿                                              node->decl_type, node->ful
     l_type);
                 {
     … +23 lines (ctrl+o to expand)
● Now updating all four is_const blocks to also set is_register. The
  struct/union, array, FP, and scalar cases each need updating. I'll update them
  one at a time using their unique surrounding context:

  Searched for 1 pattern (ctrl+o to expand)

● Bash(sed -n '5835,5855p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿          if ((node->decl_type == TYPE_STRUCT || node->decl_type == TYPE
     _UNION) &&
                 node->full_type) {
     … +18 lines (ctrl+o to expand)

● I'll update each is_const block to also set is_register. Each has distinct
  surrounding context:

● Update(src/codegen.c)
  ⎿  Added 1 line
      5848              {
      5849                  LocalVar *last_lv = (LocalVar *)vec_get(&cg->
            locals, cg->locals.len - 1);
      5850                  last_lv->is_const = node->is_const;
      5851 +                last_lv->is_register = (node->storage_class =
           += SC_REGISTER);                                              
      5852              }
      5853              if (node->child_count > 0 &&
      5854                  node->children[0]->type == AST_INITIALIZER_LI
            ST) {

● Bash(sed -n '5922,5932p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿              int length = node->full_type->length;
                 int offset = codegen_add_var(cg, node->value, size, align,
                                              node->decl_type, node->full_t
     … +11 lines (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 1 line
      5925              {
      5926                  LocalVar *last_lv = (LocalVar *)vec_get(&cg->
            locals, cg->locals.len - 1);
      5927                  last_lv->is_const = node->is_const;
      5928 +                last_lv->is_register = (node->storage_class =
           += SC_REGISTER);                                              
      5929              }
      5930              if (node->child_count > 0 &&
      5931                  node->children[0]->type == AST_INITIALIZER_LI
            ST) {

● Bash(sed -n '6009,6022p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿          if (type_is_fp(node->decl_type)) {
                 int fp_size = type_kind_bytes(node->decl_type);
                 int fp_offset = codegen_add_var(cg, node->value, fp_size,
     … +15 lines (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 1 line
      6013              {
      6014                  LocalVar *last_lv = (LocalVar *)vec_get(&cg->
            locals, cg->locals.len - 1);
      6015                  last_lv->is_const = node->is_const;
      6016 +                last_lv->is_register = (node->storage_class =
           += SC_REGISTER);                                              
      6017              }
      6018              if (node->child_count > 0) {
      6019                  codegen_expression(cg, node->children[0]);

● Now the scalar case (line 6028 originally, now shifted):

  Searched for 1 pattern (ctrl+o to expand)

● Bash(sed -n '6026,6036p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿          int size = type_kind_bytes(node->decl_type);
             int offset = codegen_add_var(cg, node->value, size, size,
                                          node->decl_type, node->full_type)
     … +10 lines (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 1 line
      6030              LocalVar *last_lv = (LocalVar *)vec_get(&cg->loca
            ls, cg->locals.len - 1);
      6031              last_lv->is_const = node->is_const;
      6032              last_lv->is_unsigned = node->is_unsigned;
      6033 +            last_lv->is_register = (node->storage_class == SC
           +_REGISTER);                                                  
      6034          }
      6035          if (node->child_count > 0) {
      6036              codegen_expression(cg, node->children[0]);

● Now add the address-of check for register variables in the AST_ADDR_OF
  handler:

● Bash(sed -n '3215,3235p' /home/scott/code/claude/claudec99/src/codegen.c)
  ⎿          if (lv) {
                 if (lv->is_static)
                     fprintf(cg->output, "    lea rax, [rel %s]\n", lv->sta
     … +21 lines (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 5 lines
      3213          }
      3214          LocalVar *lv = codegen_find_var(cg, operand->value);
      3215          if (lv) {
      3216 +            if (lv->is_register) {                           
      3217 +                compile_error(                               
      3218 +                    "error: cannot take address of register v
           +ariable '%s'\n",                                             
      3219 +                    lv->name);                               
      3220 +            }                                                
      3221              if (lv->is_static)
      3222                  fprintf(cg->output, "    lea rax, [rel %s]\n"
            , lv->static_label);
      3223              else

● Step 6: Update parser.c

  First, reject TOKEN_AUTO and TOKEN_REGISTER in parse_declaration_specifiers
  (file scope):

● Update(src/parser.c)
  ⎿  Added 7 lines, removed 1 line
      3898          } else if (parser->current.type == TOKEN_INLINE) {
      3899              /* C99 §6.7.4 function-specifier: parse and ignor
            e (no codegen effect) */
      3900              parser->current = lexer_next_token(parser->lexer)
            ;
      3901 +        } else if (parser->current.type == TOKEN_AUTO) {     
      3902 +            PARSER_ERROR(parser,                             
      3903 +                    "error: 'auto' is not valid at file scope
           +\n");                                                        
      3904 +        } else if (parser->current.type == TOKEN_REGISTER) { 
      3905 +            PARSER_ERROR(parser,                             
      3906 +                    "error: 'register' is not valid at file s
           +cope\n");                                                    
      3907          } else if (parser->current.type == TOKEN_EXTERN ||
      3908              parser->current.type == TOKEN_STATIC ||
      3909              parser->current.type == TOKEN_TYPEDEF) {
      3910              if (has_sc) {
      3905 -                PARSER_ERROR(parser,                         
      3911 +                PARSER_ERROR(parser,                         
      3912                          "error: multiple storage class specif
            iers are not allowed\n");
      3913              }
      3914              has_sc = 1;

● Now handle TOKEN_AUTO and TOKEN_REGISTER in parse_statement at block scope.
  They go right after the TOKEN_EXTERN check (before TOKEN_STATIC):

● Bash(sed -n '3262,3300p' /home/scott/code/claude/claudec99/src/parser.c)
  ⎿      /* Stage 23: extern is not allowed at block scope. */
         if (parser->current.type == TOKEN_EXTERN) {
             PARSER_ERROR(parser,
     … +36 lines (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 20 lines
      3264          PARSER_ERROR(parser,
      3265                  "error: storage class specifier not allowed i
            n block scope\n");
      3266      }
      3267 +    /* Stage 138: auto at block scope is equivalent to defaul
           +t automatic storage. */                                      
      3268 +    if (parser->current.type == TOKEN_AUTO) {                
      3269 +        parser->current = lexer_next_token(parser->lexer);   
      3270 +        return parse_statement(parser);                      
      3271 +    }                                                        
      3272 +    /* Stage 138: register at block scope — allocate like aut
           +omatic; forbid address-of. */                                
      3273 +    if (parser->current.type == TOKEN_REGISTER) {            
      3274 +        parser->current = lexer_next_token(parser->lexer);   
      3275 +        ASTNode *inner = parse_statement(parser);            
      3276 +        if (inner->type == AST_DECLARATION) {                
      3277 +            inner->storage_class = SC_REGISTER;              
      3278 +        } else if (inner->type == AST_DECL_LIST) {           
      3279 +            int i;                                           
      3280 +            for (i = 0; i < inner->child_count; i++) {       
      3281 +                if (inner->children[i]->type == AST_DECLARATI
           +ON)                                                          
      3282 +                    inner->children[i]->storage_class = SC_RE
           +GISTER;                                                      
      3283 +            }                                                
      3284 +        }                                                    
      3285 +        return inner;                                        
      3286 +    }                                                        
      3287      /* Stage 71: static is allowed at block scope — consume t
            he keyword,
      3288       * parse the declaration, and mark all resulting AST_DECL
            ARATION nodes
      3289       * with SC_STATIC so codegen emits them to static storage
            . */

● Now update parse_parameter_declaration to accept TOKEN_REGISTER as a leading
  qualifier:

● Update(src/parser.c)
  ⎿  Added 6 lines, removed 4 lines
      3728  static ASTNode *parse_parameter_declaration(Parser *parser) {
      3729      /* Stage 39: consume optional leading const qualifier on 
            parameter types.
      3730       * Stage 82-04: also consume optional volatile qualifier.
      3731 -     * Stage 106: also consume optional restrict qualifier. *
           -/                                                            
      3732 -    if (parser->current.type == TOKEN_CONST   ||             
      3733 -        parser->current.type == TOKEN_VOLATILE ||            
      3734 -        parser->current.type == TOKEN_RESTRICT)              
      3731 +     * Stage 106: also consume optional restrict qualifier.  
      3732 +     * Stage 138: also consume optional register storage-clas
           +s specifier. */                                              
      3733 +    if (parser->current.type == TOKEN_CONST     ||           
      3734 +        parser->current.type == TOKEN_VOLATILE   ||          
      3735 +        parser->current.type == TOKEN_RESTRICT   ||          
      3736 +        parser->current.type == TOKEN_REGISTER)              
      3737          parser->current = lexer_next_token(parser->lexer);
      3738      TypeKind base_kind;
      3739      Type *base_type = parse_type_specifier(parser, &base_kind
            );

● Step 7: Update ast_pretty_printer.c

● Bash(sed -n '38,48p'
      /home/scott/code/claude/claudec99/src/ast_pretty_printer.c)  ⎿   * left-to-right as written.
      */
     static void ast_print_decl_qualifiers(ASTNode *node) {
     … +8 lines (ctrl+o to expand)

● Update(src/ast_pretty_printer.c)
  ⎿  Added 2 lines
      40  static void ast_print_decl_qualifiers(ASTNode *node) {
      41      if (node->storage_class == SC_EXTERN) printf("extern ");
      42      else if (node->storage_class == SC_STATIC) printf("static "
          );
      43 +    else if (node->storage_class == SC_AUTO) printf("auto ");  
      44 +    else if (node->storage_class == SC_REGISTER) printf("regist
         +er ");                                                         
      45      if (node->is_const) printf("const ");
      46      if (node->is_unsigned) printf("unsigned ");
      47  }

● Step 8: Update version.c

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      3  
      4  #define VERSION_MAJOR  "00"
      5  #define VERSION_MINOR  "02"
      6 -#define VERSION_STAGE  "13700000"                               
      6 +#define VERSION_STAGE  "13800000"                               
      7  
      8  #ifndef VERSION_BUILD
      9  #define VERSION_BUILD  0

● Now let's build and verify it compiles:

● Bash(cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -5 && cmake
      --build build 2>&1 | tail -20)
  ⎿  -- Configuring done (0.0s)
     -- Generating done (0.0s)
     -- Build files have been written to: /home/scott/code/claude/claudec99
     … +20 lines (ctrl+o to expand)

● The compiler builds successfully. Now let me write the test programs and run
  the existing tests:

● Write(test/valid/declarations/test_auto_storage_class__27.c)
  ⎿  Wrote 21 lines to
     test/valid/declarations/test_auto_storage_class__27.c
      1 int compute(int seed) {
      2     auto int value = seed + 3;
      3     auto int values[3] = { 2, 4, 6 };
      4     auto int i;
      5     auto int total = value;
      6 
      7     for (i = 0; i < 3; i = i + 1) {
      8         total = total + values[i];
      9     }
     … +11 lines (ctrl+o to expand)

● Write(test/valid/declarations/test_register_storage_class__27.c)
  ⎿  Wrote 21 lines to
     test/valid/declarations/test_register_storage_class__27.c
      1 int compute(register int seed) {
      2     register int value = seed + 3;
      3     register int i;
      4     int values[3] = { 2, 4, 6 };
      5     register int total = value;
      6 
      7     for (i = 0; i < 3; i = i + 1) {
      8         total = total + values[i];
      9     }
     … +11 lines (ctrl+o to expand)

● Write(test/invalid/declarations/test_auto_file_scope__auto_is_not_valid_at_fil
       e_scope.c)
  ⎿  Wrote 5 lines to test/invalid/declarations/test_auto_file_scope__auto_
     is_not_valid_at_file_scope.c
      1 auto int x = 5;
      2 
      3 int main(void) {
      4     return x;
      5 }

● Write(test/invalid/declarations/test_register_file_scope__register_is_not_vali
       d_at_file_scope.c)
  ⎿  Wrote 5 lines to test/invalid/declarations/test_register_file_scope__r
     egister_is_not_valid_at_file_scope.c
      1 register int x = 5;
      2 
      3 int main(void) {
      4     return x;
      5 }

● Write(test/invalid/declarations/test_register_address_of__cannot_take_address_
       of_register_variable.c)
  ⎿  Wrote 5 lines to test/invalid/declarations/test_register_address_of__c
     annot_take_address_of_register_variable.c
      1 int main(void) {
      2     register int x = 42;
      3     int *p = &x;
      4     return *p;
      5 }

● Now let's test the new test cases first to make sure they work correctly:

● Bash(bin/cc99 -o /tmp/test_auto
      test/valid/declarations/test_auto_storage_class__27.c && /tmp/test_auto;
      echo "Exit: $?")
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/declarations/te
     st_auto_storage_class__27.c -> test_auto_storage_class__27.asm
     Exit: 27

● Bash(bin/cc99 -o /tmp/test_register
      test/valid/declarations/test_register_storage_class__27.c &&
      /tmp/test_register; echo "Exit: $?")
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/declarations/te
     st_register_storage_class__27.c -> test_register_storage_class__27.asm
     Exit: 27

● Both return 27 as expected. Now test the invalid cases:

● Bash(build/ccompiler test/invalid/declarations/test_auto_file_scope__auto_is_n
      ot_valid_at_file_scope.c 2>&1; echo "Exit: $?")
  ⎿  test/invalid/declarations/test_auto_file_scope__auto_is_not_valid_at_f
     ile_scope.c:1:1: error: 'auto' is not valid at file scope
     Exit: 1

● Bash(build/ccompiler test/invalid/declarations/test_register_file_scope__regis
      ter_is_not_valid_at_file_scope.c 2>&1; echo "Exit: $?")
  ⎿  test/invalid/declarations/test_register_file_scope__register_is_not_va
     lid_at_file_scope.c:1:1: error: 'register' is not valid at file scope
     Exit: 1

● Bash(build/ccompiler test/invalid/declarations/test_register_address_of__canno
      t_take_address_of_register_variable.c 2>&1; echo "Exit: $?")
  ⎿  test/invalid/declarations/test_register_address_of__cannot_take_addres
     s_of_register_variable.c:3:16: error: cannot take address of register
     variable 'x'
     Exit: 1

● All test cases pass. Now run the full test suite:

● Bash(./test/run_all_tests.sh 2>&1 | tail -20)
  ⎿  Running suite: print_ast
     ====================================================
     … +17 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● There are 2 failures. Let me see which ones:

● Bash(./test/run_all_tests.sh 2>&1 | grep "FAIL\|FAILED\|failed" | head -20)
  ⎿  Results: 106 passed, 0 failed, 106 total
     Results: 59 passed, 0 failed, 59 total
     Results: 165 passed, 0 failed, 165 total
     … +13 lines (ctrl+o to expand)
  ⎿  (timeout 5m)
● The issue is the test filename uses auto_is_not_valid_at_file_scope but our
  error has 'auto' with quotes. Let me check how the invalid test runner
  matches:

  Read 1 file (ctrl+o to expand)

● The test runner converts underscores to spaces and greps for the fragment
  without quotes. Our error message has 'auto' with quotes, so "auto is not
  valid" doesn't match "'auto' is not valid". I'll remove the quotes from the
  error messages to match the filename fragment:

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 2 lines
      3922              parser->current = lexer_next_token(parser->lexer)
            ;
      3923          } else if (parser->current.type == TOKEN_AUTO) {
      3924              PARSER_ERROR(parser,
      3925 -                    "error: 'auto' is not valid at file scope
           -\n");                                                        
      3925 +                    "error: auto is not valid at file scope\n
           +");                                                          
      3926          } else if (parser->current.type == TOKEN_REGISTER) {
      3927              PARSER_ERROR(parser,
      3928 -                    "error: 'register' is not valid at file s
           -cope\n");                                                    
      3928 +                    "error: register is not valid at file sco
           +pe\n");                                                      
      3929          } else if (parser->current.type == TOKEN_EXTERN ||
      3930              parser->current.type == TOKEN_STATIC ||
      3931              parser->current.type == TOKEN_TYPEDEF) {

● Bash(cmake --build build 2>&1 | grep -E "error:|warning:|Built" | tail -10)
  ⎿  /home/scott/code/claude/claudec99/src/parser.c:3158:52: warning: "/*"
     within comment [-Wcomment]
     [100%] Built target ccompiler

● Bash(./test/run_all_tests.sh 2>&1 | tail -5)
  ⎿  Results: 21 passed, 0 failed, 21 total

     ====================================================
     … +2 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● All 1970 tests pass. Now let's run the self-host test:

● Bash(./build.sh --mode=self-host 2>&1 | tail -30)
  ⎿  PASS  test_va_start_no_varargs  (exit code: 0, output matched)

     Results: 88 passed, 0 failed, 88 total
     … +27 lines (ctrl+o to expand)
  ⎿  (timeout 10m)

● C0→C1→C2 all pass with all 1970 tests. Now let's update the self-compilation
  report and commit the code:

  Read 1 file (ctrl+o to expand)

● Update(docs/self-compilation-report.md)
  ⎿  Added 23 lines
       1  # Self-Compilation Diagnostic Report
       2  
       3 +## Issues found during stage 138 self-hosting test             
       4 +                                                               
       5 +None. Stage 138 adds `TOKEN_AUTO` and `TOKEN_REGISTER` keywords
         + to the lexer,                                                 
       6 +`SC_AUTO` and `SC_REGISTER` to the `StorageClass` enum, and `is
         +_register` to                                                  
       7 +`LocalVar`. The parser accepts `auto` and `register` at block s
         +cope (lowered to                                               
       8 +automatic storage; `register` marks the variable so address-of 
         +is rejected) and                                               
       9 +rejects both at file scope. The compiler's own source uses no `
         +auto` or                                                       
      10 +`register` declarations, so all new code paths are dormant duri
         +ng bootstrap.                                                  
      11 +All 1970 tests passed at C0, C1, and C2 with no source changes 
         +needed during                                                  
      12 +the bootstrap.                                                 
      13 +                                                               
      14 +## Result (stage 138)                                          
      15 +                                                               
      16 +**Date:** 2026-06-18                                           
      17 +                                                               
      18 +| Step | Binary | Version | BuiltBy | Tests |                  
      19 +|------|--------|---------|---------|-------|                  
      20 +| C0 | `build/ccompiler-c0` | `00.02.13800000.01031` | `GNU_13_
         +3_0` | 1970/1970 |                                             
      21 +| C1 | `build/ccompiler-c1` | `00.02.13800000.01032` | `ClaudeC
         +99_v00_02_13800000_01031` | 1970/1970 |                        
      22 +| C2 | `build/ccompiler-c2` | `00.02.13800000.01033` | `ClaudeC
         +99_v00_02_13800000_01032` | 1970/1970 |                        
      23 +                                                               
      24 +---                                                            
      25 +                                                               
      26  ## Issues found during stage 137 self-hosting test
      27  
      28  None. Stage 137 adds `is_func_returning_fp`, `own_param_types`,

● Now commit the implementation changes:

● Bash(git status)
  ⎿  On branch master
     Your branch is ahead of 'origin/master' by 2 commits.
       (use "git push" to publish your local commits)
     … +20 lines (ctrl+o to expand)

● Bash(git diff --stat)
  ⎿   docs/self-compilation-report.md | 23 +++++++++++++++++++++++
      1 file changed, 23 insertions(+)

● The self-host script made a checkpoint commit. Let me check the recent log:

● Bash(git log --oneline -5)
  ⎿  ffd9940 self-host C1 verified: all tests pass
     6b45698 self-host C0 verified: all tests pass
     cb61819 stage 138 spec
     … +3 lines (ctrl+o to expand)

● The self-host script committed the source changes. Let me check what was
  included:

● Bash(git show --stat ffd9940 && git show --stat 6b45698)
  ⎿  commit ffd99400f4f15b4c4ba60e85df79216623347dd5
     Author: Scott Flicker <sflicker2@earthlink.net>
     Date:   Thu Jun 18 17:22:31 2026 -0400
     … +25 lines (ctrl+o to expand)

● The source changes were captured in the C0 checkpoint commit. Now I need to
  commit the remaining files (test files, kickoff, and self-compilation report)
  plus create a proper stage commit:

● Bash(git add docs/self-compilation-report.md
      docs/kickoffs/stage-138-kickoff.md
      test/valid/declarations/test_auto_storage_class__27.c
      test/valid/declarations/test_re…)
  ⎿  [master b621d88] stage 138: support auto and register storage-class sp
     ecifiers
      7 files changed, 186 insertions(+)
     … +11 lines (ctrl+o to expand)

● Now delegate the milestone, transcript, grammar update, README update, and
  checklist update to haiku:

● haiku-stage-artifact-writer(Generate stage-138 milestone, transcript, grammar,
                             checklist, README)
  ⎿  Done (13 tool uses · 69.7k tokens · 28s)
  (ctrl+o to expand)

● Let me check if the haiku agent created the artifacts:

  Listed 1 directory (ctrl+o to expand)
● The haiku agent didn't complete. Let me re-delegate:

● haiku-stage-artifact-writer(Generate stage-138 final artifacts)
  ⎿  Done (12 tool uses · 62.2k tokens · 24s)
  (ctrl+o to expand)

● Bash(ls -la docs/milestones/stage-138* docs/sessions/stage-138* 2>/dev/null;
      echo "---"; git status --short | head -20)
  ⎿  ---

● The haiku agent isn't completing. I'll generate the artifacts directly.

  Read 2 files (ctrl+o to expand)

● Write(docs/milestones/stage-138-milestone.md)
  ⎿  Wrote 13 lines to docs/milestones/stage-138-milestone.md
      2 # Milestone Summary
        te Stage 138: auto and register Storage-Class Specifiers - Comple
      5 stage-138 ships support for the `auto` and `register` C99 storage
        -class specifiers (CC99-011 and CC99-012).
      7 - Tokenizer: Added `TOKEN_AUTO` and `TOKEN_REGISTER` keywords wit
      8 - Grammar/Parser: `parse_declaration_specifiers` rejects both at 
        file scope; `parse_statement` handles `auto` (treated as default 
        `) at block scope; `parse_parameter_declaration` accepts `registe
      9 - AST/Semantics: `SC_AUTO=8` and `SC_REGISTER=16` added to `Stora
        geClass` enum in `ast.h`; `ast_pr
     … +15 lines (ctrl+o to expand)

● Write(docs/sessions/stage-138-transcript.md)
  ⎿  Wrote 73 lines to docs/sessions/stage-138-transcript.md
        s stage-138 Transcript: auto and register Storage-Class Specifier
      3 ## Summary
      5 Stage 138 resolves two defects reported in the stage spec: CC99-0
        ge class rejected). Both are valid C99 storage-class specifiers f
        or block-scope object declarations.
      7 `auto` declares a variable with automatic storage duration, ident
        block scope; file-scope `auto` is a constraint violation. The spe
        E` after parsing.ect on code generation and is treated as `SC_NON
      9 `register` also has automatic storage duration but carries the ad
        ditional semantic restriction that
     … +114 lines (ctrl+o to expand)

● Now update the grammar, checklist, and README. Let me read them first:

  Searched for 2 patterns (ctrl+o to expand)

● Bash(sed -n '1916,1960p'
      /home/scott/code/claude/claudec99/docs/outlines/checklist.md)
  ⎿  ## Stage 137 - Function Return Function Pointers

     - [x] CC99-010: `int (*get_adder())(int)` declarator form accepted by
     … +42 lines (ctrl+o to expand)

● Bash(sed -n '1960,1990p'
      /home/scott/code/claude/claudec99/docs/outlines/checklist.md)
  ⎿  - [ ] register storage class (hint only)
     - [ ] auto storage class (explicit)
     - [x] Tentative definitions for file-scope variables (Stage 126)
     … +32 lines (ctrl+o to expand)

● Now let me update the grammar.md, checklist.md, and README.md:

● Bash(sed -n '24,28p' /home/scott/code/claude/claudec99/docs/grammar.md)
  ⎿  <type_qualifier> ::= "const" | "volatile" | "restrict"

     <storage_class_specifier>   ::= "extern" | "static" | "typedef"
     … +2 lines (ctrl+o to expand)

● Update(docs/grammar.md)
  ⎿  Added 5 lines, removed 1 line
      23  
      24  <type_qualifier> ::= "const" | "volatile" | "restrict"
      25  
      26 -<storage_class_specifier>   ::= "extern" | "static" | "typedef"
      26 +<storage_class_specifier>   ::= "extern" | "static" | "typedef"
         + | "auto" | "register"                                         
      27 +                            # "auto" and "register" are valid o
         +nly at block scope.                                            
      28 +                            # "register" is also valid as a fun
         +ction parameter qualifier.                                     
      29 +                            # "extern" and "static" are valid a
         +t file scope and block scope                                   
      30 +                            # ("extern" at block scope not yet 
         +supported).                                                    
      31  
      32  <parameter_list> ::= "void"
      33                     | <parameter_declarator> { "," <parameter_de
          clarator> } [ "," "..." ]

● Now update the grammar notes at the bottom of the file as well:

● Bash(sed -n '410,420p' /home/scott/code/claude/claudec99/docs/grammar.md)
  ⎿  #     `__builtin_va_arg` are recognized as primary expressions.
     #   - `__builtin_va_start` must appear inside the body of a variadic f
     unction.
     … +13 lines (ctrl+o to expand)

● Update(docs/grammar.md)
  ⎿  Added 3 lines
      416  #   - In a function definition, the declarator must be a funct
           ion declarator.
      417  #   - Function declarations may not have initializers.
      418  #   - The `extern` storage-class specifier is supported only a
           t file scope.
      419 +#   - The `auto` specifier is supported only at block scope (e
          +quivalent to default automatic storage).                      
      420 +#   - The `register` specifier is supported at block scope and
          + as a function parameter qualifier.                           
      421 +#     Address-of (`&`) on a `register` variable is a compile e
          +rror.                                                         
      422  #   - The `static` specifier is supported at file scope and bl
           ock scope
      423  #     (scalar and pointer types only; block-scope static array
           s and structs
      424  #     are not yet supported).

● Now update the checklist - add Stage 138 section and flip the TODO items:

● Update(docs/outlines/checklist.md)
  ⎿  Added 15 lines
      1927  - [x] Typedef spelling equivalent: `typedef int (*Adder)(int)
            ; Adder get_fn(void)`
      1928    - Bug fix: `func->full_type` assignment condition changed f
            rom `d.pointer_count > 0` to `return_kind == TYPE_POINTER`
      1929  
      1930 +## Stage 138 - auto and register Storage-Class Specifiers    
      1931 +                                                             
      1932 +- [x] CC99-011: `auto` storage-class specifier accepted at bl
           +ock scope                                                    
      1933 +  - TOKEN_AUTO added to lexer; SC_AUTO=8 added to StorageClas
           +s enum                                                       
      1934 +  - parse_statement consumes `auto`, delegates to parse_state
           +ment with no storage-class change (SC_NONE behavior)         
      1935 +  - File-scope `auto` rejected: "error: auto is not valid at 
           +file scope"                                                  
      1936 +- [x] CC99-012: `register` storage-class specifier accepted a
           +t block scope and function parameters                        
      1937 +  - TOKEN_REGISTER added to lexer; SC_REGISTER=16 added to St
           +orageClass enum                                              
      1938 +  - parse_statement consumes `register`, stamps SC_REGISTER o
           +n resulting AST_DECLARATION nodes                            
      1939 +  - parse_parameter_declaration accepts TOKEN_REGISTER as a l
           +eading qualifier (consumed and ignored)                      
      1940 +  - File-scope `register` rejected: "error: register is not v
           +alid at file scope"                                          
      1941 +- [x] `register` address-of restriction: `&register_var` emit
           +s "error: cannot take address of register variable 'NAME'"   
      1942 +  - `is_register` field added to LocalVar; set from SC_REGIST
           +ER in all four declaration paths                             
      1943 +  - AST_ADDR_OF codegen checks `lv->is_register` before emitt
           +ing lea                                                      
      1944 +                                                             
      1945  ---
      1946  
      1947  ## TODO

● Now flip the TODO items for register and auto:

● Update(docs/outlines/checklist.md)
  ⎿  Added 2 lines, removed 2 lines
      1972  
      1973  ### Declarations and Scope
      1974  - [x] static storage class (block scope — local static variab
            les: scalar/pointer Stage 71; arrays/structs/unions Stage 101
            ; designated-init arrays, struct/union element types, 2D arra
            ys Stage 102; full constant-expression initializers Stage 103
            )
      1975 -- [ ] register storage class (hint only)                     
      1976 -- [ ] auto storage class (explicit)                          
      1975 +- [x] register storage class (hint only) (Stage 138)         
      1976 +- [x] auto storage class (explicit) (Stage 138)              
      1977  - [x] Tentative definitions for file-scope variables (Stage 1
            26)
      1978  - [x] For-loop initializer declarations: for (int i = 0; ...)
             (Stage 76)
      1979  - [ ] Multiple pointer levels in multi-declarator lists

● Now update README.md - change "Through stage 137" to "Through stage 138" and
  update the test totals:

● Update(README.md)
  ⎿  Added 4 lines
      223  
      224  ## What the compiler currently supports
      225  
      226 +Through stage 138 (auto and register storage-class specifiers)
          +:                                                             
      227 +                                                              
      228 +> Stage 138 adds `auto` and `register` storage-class specifier
          +s (CC99-011 and CC99-012). **Tokenizer**: `TOKEN_AUTO` and `TO
          +KEN_REGISTER` added. **AST**: `SC_AUTO=8` and `SC_REGISTER=16`
          + added to `StorageClass` enum. **Parser**: `auto` at block sco
          +pe treated as default automatic storage; `register` at block s
          +cope allocates identically but marks the variable `SC_REGISTER
          +`; both are rejected at file scope; `register` is also accepte
          +d as a leading qualifier in function parameter declarations. *
          +*Codegen**: `is_register` field added to `LocalVar`; `AST_ADDR
          +_OF` rejects `&register_var` with a compile error. **Tests**: 
          +5 new tests (2 valid returning 27, 3 invalid). Version bumped 
          +to 13800000. All 1970 tests pass (1284 valid, 262 invalid, 88 
          +integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165
          + unit). Self-host C0→C1→C2 verified with all tests passing at 
          +every stage, no source changes needed during bootstrap.       
      229 +                                                              
      230  Through stage 137 (functions returning function pointers):
      231  
      232  > Stage 137 fixes the parser to accept functions returning fun
           ction pointers — the `int (*get_adder())(int)` declarator form
            (CC99-010), previously rejected with "functions returning fun
           ction pointers are not supported". **Parser**: `ParsedDeclarat
           or` extended with `is_func_returning_fp`, `own_param_types[FUN
           C_TYPE_MAX_PARAMS]`, `own_param_count`, `own_is_no_prototype` 
           fields to track the inner function's signature. `parse_declara
           tor` replaces the rejection with full parsing of `(*name())(pa
           rams)` form (guard on `inner_stars == 0` remains for the inval
           id direct-function-return case). **Semantic**: `parse_external
           _declaration` builds the nested pointer-to-function type, crea
           tes `AST_FUNCTION_DECL` with `decl_type = TYPE_POINTER`, and r
           egisters/optionally parses the function body. **Bug fix**: typ
           edef'd pointer return types recognized via `func->full_type` a
           ssignment condition. **Tests**: removed 1 test (now valid), ad
           ded 4 valid tests and 1 invalid test. Version bumped to 137000
           00. All 1965 tests pass (1282 valid, 259 invalid, 88 integrati
           on, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). S
           elf-host C0→C1→C2 verified with all tests passing at every sta
           ge, no source changes needed during bootstrap.

● Now update the declarations bullet in the "What the compiler currently
  supports" section and the aggregate test totals line:
● Update(README.md)
  ⎿  Added 2 lines
      494    enum constants are available as compile-time integer values 
           throughout the translation unit.
      495    Forward-declared enum tags (`typedef enum Status Status;` be
           fore the body) are supported.
      496    Block-scope `static` variables (scalar, pointer, array, and 
           struct/union types) persist values across function calls and a
           re stored in .bss or .data with constant-only initializers.
      497 +  `auto` storage-class specifier is accepted at block scope (e
          +quivalent to default automatic storage).                      
      498 +  `register` storage-class specifier is accepted at block scop
          +e and as a function parameter qualifier (allocates like automa
          +tic; address-of on a register variable is a compile error).   
      499  - **Integer types**: `char`, `short`, `int`, `long` and their 
           `unsigned` variants
      500    (`unsigned char`, `unsigned short`, `unsigned int`, `unsigne
           d long`, plain `unsigned`).
      501    `signed` keyword support (`signed char`, `signed short`, `si
           gned int`, `signed long`,

● Now update the test totals line in the Tests section:

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      727  ```
      728  
      729  The runner aggregates per-suite results and prints a final
      730 -`Aggregate: P passed, F failed, T total` line. As of stage 136
          - all 1961 tests pass (1277 valid, 259 invalid, 88 integration,
          - 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).     
      730 +`Aggregate: P passed, F failed, T total` line. As of stage 138
          + all 1970 tests pass (1284 valid, 262 invalid, 88 integration,
          + 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).     
      731  
      732  Individual suites can be run directly, e.g. `./test/valid/run_
           tests.sh`.
      733  

● Now commit the documentation artifacts:

● Bash(git add docs/milestones/stage-138-milestone.md
      docs/sessions/stage-138-transcript.md docs/grammar.md
      docs/outlines/checklist.md README.md && git commit -m "$(ca…)
  ⎿  [master 315ddfd] docs: stage 138 milestone, transcript, grammar, check
     list, README
      5 files changed, 118 insertions(+), 4 deletions(-)
     … +2 lines (ctrl+o to expand)

● Bash(git log --oneline -6)
  ⎿  315ddfd docs: stage 138 milestone, transcript, grammar, checklist, REA
     DME
     b621d88 stage 138: support auto and register storage-class specifiers
     … +4 lines (ctrl+o to expand)

● Stage 138 is complete. Here's a summary:

  Stage 138 Complete

  What was added: auto and register C99 storage-class specifiers (CC99-011,
  CC99-012).

  Changes:
  - Tokenizer/AST: TOKEN_AUTO, TOKEN_REGISTER, SC_AUTO=8, SC_REGISTER=16
  - Parser: auto at block scope → default automatic storage; register at block
  scope → SC_REGISTER; both rejected at file scope; register accepted as
  function parameter qualifier
  - Codegen: is_register field on LocalVar; &register_var is a compile error
  - Version: bumped to 13800000

  Tests: 5 new (2 valid returning 27, 3 invalid). All 1970 tests pass. Self-host
  C0→C1→C2 verified with no source changes during bootstrap.

✻ Cogitated for 16m 28s