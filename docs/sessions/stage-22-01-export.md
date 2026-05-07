 ▐▛███▜▌   Claude Code v2.1.132
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

❯ /implement-stage docs/stages/Claudec99-spec-stage-22-01-file-scope-uninitiali 
zed-object-declarations.md                                                      

● I'll read the spec file and supporting files to get started.

  Read 3 files (ctrl+o to expand)

● Now let me look at the current codebase to understand what needs to change.

  Searched for 2 patterns, read 3 files (ctrl+o to expand)

● Now let me read the codegen to understand the current structure.

  Read 2 files (ctrl+o to expand)

● Let me look at the codegen_translation_unit function and grammar docs.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Now let me look at the end of codegen.c to understand the
  codegen_translation_unit function:

  Read 1 file (ctrl+o to expand)

● Now let me look at the VAR_REF and ASSIGNMENT handling in codegen to
  understand how variable access is handled:

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me look at the statement codegen for AST_DECLARATION:

  Read 1 file (ctrl+o to expand)

● Let me look at the inc/dec and more of the expression codegen:

  Read 1 file (ctrl+o to expand)

● Now let me look at the existing test infrastructure and existing tests for
  file-scope declarations:

● Bash(find /home/scott/code/claude/claudec99/test -name "*.sh" | xargs grep -l
      "run\|test" | head -5)
  ⎿  /home/scott/code/claude/claudec99/test/run_all_tests.sh
     /home/scott/code/claude/claudec99/test/print_asm/run_tests.sh
     /home/scott/code/claude/claudec99/test/invalid/run_tests.sh
     … +2 lines (ctrl+o to expand)

  Read 1 file, listed 4 directories (ctrl+o to expand)

● Now let me check the current test count and run tests to establish a baseline:

● Bash(make 2>&1 | tail -5)
  ⎿  make: *** No targets specified and no makefile found.  Stop.

● Bash(bash test/run_all_tests.sh 2>&1 | tail -15)
  ⎿  PASS  test_stage_16_02_complement_int
     PASS  test_stage_16_02_complement_long
     PASS  test_stage_16_03_lshift_int
     … +12 lines (ctrl+o to expand)

● Good — 635 tests passing. Now let me check the build setup:
  Searched for 1 pattern, listed 1 directory (ctrl+o to expand)

● Bash(cd /home/scott/code/claude/claudec99/build && make 2>&1 | tail -5)
  ⎿  Consolidate compiler generated dependencies of target ccompiler
     [100%] Built target ccompiler

● Good. Let me now read the kickoff template and then begin the implementation:

  Read 1 file (ctrl+o to expand)

● I now have a complete picture. Let me summarize my analysis before proposing
  the plan.

  ---
  STAGE_DISPLAY: Stage-22-01

  Spec Summary

  Stage 22-01 adds file-scope (global) object declarations — uninitialized only.
   Supported types: int, char, short, long, pointer (int *p), and array (int 
  arr[10]). Globals go in .bss, are zero-initialized, and are accessed via
  RIP-relative addressing ([rel name]). Locals shadow globals.

  Spec Issue

  One minor typo in the Codegen section: arr; resd 10 — the semicolon after arr
  should be a colon (:). Should read arr: resd 10. Not blocking.

  What needs to change

  Tokenizer: No changes needed.

  Parser (src/parser.c): parse_external_declaration already handles the
  non-function case, but skips the array sub-case (d.is_array). Need to add
  array type construction here — currently arrays at file scope get stored with
  the wrong decl_type.

  AST: No new node types needed. AST_DECLARATION at translation-unit level
  represents globals.

  Codegen (include/codegen.h): Add GlobalVar struct and globals[]/global_count
  to CodeGen.

  Codegen (src/codegen.c):
  - New helpers: codegen_find_global, emit_load_global, emit_store_global,
  global_var_type, codegen_add_global, emit_global_bss
  - codegen_translation_unit: register globals, emit .bss section before .text
  - emit_array_index_addr: add global array/pointer path
  - sizeof_type_of_expr, expr_result_type: add global fallbacks for AST_VAR_REF,
   AST_DEREF, AST_ARRAY_INDEX, AST_ASSIGNMENT
  - codegen_expression: add global fallbacks for AST_VAR_REF, AST_ASSIGNMENT,
  AST_ADDR_OF, AST_PREFIX_INC_DEC, AST_POSTFIX_INC_DEC, AST_SIZEOF_EXPR

  ---
  Now let me delegate the kickoff artifact and propose the full plan:

● haiku-stage-artifact-writer(Generate kickoff artifact for stage-22-01)
  ⎿  Backgrounded agent (↓ to manage · ctrl+o to expand)

● The kickoff artifact is being generated in the background. Let me begin
  implementing.

  ---
  Planned Changes

  ┌───────────────────┬──────────────────────────────────────────────────────┐
  │       File        │                        Change                        │
  ├───────────────────┼──────────────────────────────────────────────────────┤
  │ src/parser.c      │ Handle d.is_array in parse_external_declaration      │
  ├───────────────────┼──────────────────────────────────────────────────────┤
  │ include/codegen.h │ Add GlobalVar, globals[], global_count to CodeGen    │
  ├───────────────────┼──────────────────────────────────────────────────────┤
  │ src/codegen.c     │ Global helpers, .bss emission, global fallbacks      │
  │                   │ throughout                                           │
  ├───────────────────┼──────────────────────────────────────────────────────┤
  │ test/valid/       │ 6+ new valid tests                                   │
  ├───────────────────┼──────────────────────────────────────────────────────┤
  │ test/print_asm/   │ ASM snapshot tests                                   │
  ├───────────────────┼──────────────────────────────────────────────────────┤
  │ docs/grammar.md   │ Remove "no file-scope variable declarations"         │
  │                   │ restriction                                          │
  └───────────────────┴──────────────────────────────────────────────────────┘

  ---
  Step 1 — Parser: fix file-scope array declarations

● Update(src/parser.c)
  ⎿  Added 14 lines, removed 3 lines
      1263              fprintf(stderr, "error: '%s' is not a function de
            clarator\n", d.name);
      1264              exit(1);
      1265          }
      1266 -        /* File-scope object declaration; code generation is 
           -out of scope. */                                             
      1266          Type *full_type = base_type;
      1267          for (int i = 0; i < d.pointer_count; i++)
      1268              full_type = type_pointer(full_type);
      1269          ASTNode *decl = ast_new(AST_DECLARATION, d.name);
      1271 -        decl->decl_type = (d.pointer_count > 0) ? TYPE_POINTE
           -R : base_kind;                                               
      1272 -        if (d.pointer_count > 0)                             
      1270 +        if (d.is_array) {                                    
      1271 +            if (!d.has_size) {                               
      1272 +                fprintf(stderr,                              
      1273 +                        "error: array size required for file-
           +scope declaration '%s'\n",                                   
      1274 +                        d.name);                             
      1275 +                exit(1);                                     
      1276 +            }                                                
      1277 +            decl->decl_type = TYPE_ARRAY;                    
      1278 +            decl->full_type = type_array(full_type, d.array_l
           +ength);                                                      
      1279 +        } else if (d.pointer_count > 0) {                    
      1280 +            decl->decl_type = TYPE_POINTER;                  
      1281              decl->full_type = full_type;
      1282 +        } else {                                             
      1283 +            decl->decl_type = base_kind;                     
      1284 +        }                                                    
      1285          parser_expect(parser, TOKEN_SEMICOLON);
      1286          return decl;
      1287      }

● Agent "Generate kickoff artifact for stage-22-01" completed

● Kickoff artifact is done. Continuing with implementation.

  Step 2 — codegen.h: add GlobalVar and fields to CodeGen

● Update(include/codegen.h)
  ⎿  Added 1 line
       6  #include "type.h"
       7  
       8  #define MAX_LOCALS 64
       9 +#define MAX_GLOBALS 64                                         
      10  #define MAX_BREAK_DEPTH 32
      11  #define MAX_SWITCH_DEPTH 16
      12  #define MAX_SWITCH_LABELS 64

● Update(include/codegen.h)
  ⎿  Added 11 lines
      26      Type *full_type;
      27  } LocalVar;
      28  
      29 +/* Stage 22-01: file-scope (global) variable. Accessed via RIP-
         +relative                                                       
      30 + * addressing ([rel name]) instead of [rbp - offset]. size is t
         +he byte                                                        
      31 + * width for load/store; full_type is non-NULL for pointer and 
         +array                                                          
      32 + * kinds and carries the complete type chain. */               
      33 +typedef struct {                                               
      34 +    char name[256];                                            
      35 +    int size;                                                  
      36 +    TypeKind kind;                                             
      37 +    Type *full_type;                                           
      38 +} GlobalVar;                                                   
      39 +                                                               
      40  /* One entry per breakable construct (loop or switch). Switches
           set
      41   * continue_label to -1 since `continue` is not valid inside a 
          switch
      42   * alone; AST_CONTINUE walks down to the nearest loop entry. */

● Now add the globals array to the CodeGen struct:

● Update(include/codegen.h)
  ⎿  Added 3 lines
      63      int label_count;
      64      LocalVar locals[MAX_LOCALS];
      65      int local_count;
      66 +    /* Stage 22-01: file-scope globals, registered before funct
         +ion codegen. */                                                
      67 +    GlobalVar globals[MAX_GLOBALS];                            
      68 +    int global_count;                                          
      69      int stack_offset;
      70      int scope_start;
      71      int push_depth;

● Step 3 — codegen.c: add global helpers and modify all affected paths

  First, initialize global_count in codegen_init:

● Update(src/codegen.c)
  ⎿  Added 1 line
      130      cg->output = output;
      131      cg->label_count = 0;
      132      cg->local_count = 0;
      133 +    cg->global_count = 0;                                     
      134      cg->stack_offset = 0;
      135      cg->scope_start = 0;
      136      cg->push_depth = 0;

● Now add the global helper functions after codegen_find_var. I'll add them
  right after the local_var_type function:

● Update(src/codegen.c)
  ⎿  Added 49 lines
      290      }
      291  }
      292  
      293 +/* Stage 22-01: global variable helpers. */                   
      294 +static GlobalVar *codegen_find_global(CodeGen *cg, const char 
          +*name) {                                                      
      295 +    for (int i = 0; i < cg->global_count; i++) {              
      296 +        if (strcmp(cg->globals[i].name, name) == 0)           
      297 +            return &cg->globals[i];                           
      298 +    }                                                         
      299 +    return NULL;                                              
      300 +}                                                             
      301 +                                                              
      302 +static Type *global_var_type(GlobalVar *gv) {                 
      303 +    if (gv->full_type) return gv->full_type;                  
      304 +    switch (gv->kind) {                                       
      305 +    case TYPE_CHAR:  return type_char();                      
      306 +    case TYPE_SHORT: return type_short();                     
      307 +    case TYPE_LONG:  return type_long();                      
      308 +    case TYPE_INT:                                            
      309 +    default:         return type_int();                       
      310 +    }                                                         
      311 +}                                                             
      312 +                                                              
      313 +static void emit_load_global(CodeGen *cg, const char *name, in
          +t size) {                                                     
      314 +    switch (size) {                                           
      315 +    case 1: fprintf(cg->output, "    movsx eax, byte [rel %s]\
          +n", name); break;                                             
      316 +    case 2: fprintf(cg->output, "    movsx eax, word [rel %s]\
          +n", name); break;                                             
      317 +    case 8: fprintf(cg->output, "    mov rax, [rel %s]\n", nam
          +e); break;                                                    
      318 +    case 4:                                                   
      319 +    default:                                                  
      320 +        fprintf(cg->output, "    mov eax, [rel %s]\n", name); 
      321 +        break;                                                
      322 +    }                                                         
      323 +}                                                             
      324 +                                                              
      325 +static void emit_store_global(CodeGen *cg, const char *name, i
          +nt size,                                                      
      326 +                              int src_is_long) {              
      327 +    switch (size) {                                           
      328 +    case 1: fprintf(cg->output, "    mov [rel %s], al\n", name
          +); break;                                                     
      329 +    case 2: fprintf(cg->output, "    mov [rel %s], ax\n", name
          +); break;                                                     
      330 +    case 8:                                                   
      331 +        if (!src_is_long)                                     
      332 +            fprintf(cg->output, "    movsxd rax, eax\n");     
      333 +        fprintf(cg->output, "    mov [rel %s], rax\n", name); 
      334 +        break;                                                
      335 +    case 4:                                                   
      336 +    default:                                                  
      337 +        fprintf(cg->output, "    mov [rel %s], eax\n", name); 
      338 +        break;                                                
      339 +    }                                                         
      340 +}                                                             
      341 +                                                              
      342  static void codegen_expression(CodeGen *cg, ASTNode *node);
      343  
      344  /*

● Now update emit_array_index_addr to handle global arrays/pointers:

● Update(src/codegen.c)
  ⎿  Added 27 lines, removed 11 lines
      361          exit(1);
      362      }
      363      LocalVar *lv = codegen_find_var(cg, base_node->value);
      364 -    if (!lv) {                                                
      364 +    GlobalVar *gv = lv ? NULL : codegen_find_global(cg, base_n
          +ode->value);                                                  
      365 +    if (!lv && !gv) {                                         
      366          fprintf(stderr, "error: undeclared variable '%s'\n", b
           ase_node->value);
      367          exit(1);
      368      }
      369      Type *element;
      369 -    if (lv->kind == TYPE_ARRAY) {                             
      370 -        element = lv->full_type->base;                        
      371 -        fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->o
          -ffset);                                                       
      372 -    } else if (lv->kind == TYPE_POINTER) {                    
      373 -        element = lv->full_type->base;                        
      374 -        fprintf(cg->output, "    mov rax, [rbp - %d]\n", lv->o
          -ffset);                                                       
      370 +    if (lv) {                                                 
      371 +        if (lv->kind == TYPE_ARRAY) {                         
      372 +            element = lv->full_type->base;                    
      373 +            fprintf(cg->output, "    lea rax, [rbp - %d]\n", l
          +v->offset);                                                   
      374 +        } else if (lv->kind == TYPE_POINTER) {                
      375 +            element = lv->full_type->base;                    
      376 +            fprintf(cg->output, "    mov rax, [rbp - %d]\n", l
          +v->offset);                                                   
      377 +        } else {                                              
      378 +            fprintf(stderr,                                   
      379 +                    "error: subscript base '%s' is not an arra
          +y or pointer\n",                                              
      380 +                    base_node->value);                        
      381 +            exit(1);                                          
      382 +        }                                                     
      383      } else {
      376 -        fprintf(stderr,                                       
      377 -                "error: subscript base '%s' is not an array or
          - pointer\n",                                                  
      378 -                base_node->value);                            
      379 -        exit(1);                                              
      384 +        if (gv->kind == TYPE_ARRAY) {                         
      385 +            element = gv->full_type->base;                    
      386 +            fprintf(cg->output, "    lea rax, [rel %s]\n", gv-
          +>name);                                                       
      387 +        } else if (gv->kind == TYPE_POINTER) {                
      388 +            element = gv->full_type->base;                    
      389 +            fprintf(cg->output, "    mov rax, [rel %s]\n", gv-
          +>name);                                                       
      390 +        } else {                                              
      391 +            fprintf(stderr,                                   
      392 +                    "error: subscript base '%s' is not an arra
          +y or pointer\n",                                              
      393 +                    base_node->value);                        
      394 +            exit(1);                                          
      395 +        }                                                     
      396      }
      397      int elem_size = type_size(element);
      398  

● Now update sizeof_type_of_expr to add global fallbacks for each relevant case.
   Let me read the current state first:

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 15 lines, removed 2 lines
      497          return TYPE_INT;
      498      case AST_VAR_REF: {
      499          LocalVar *lv = codegen_find_var(cg, node->value);
      500 -        if (!lv) return TYPE_INT;                             
      501 -        return lv->kind;                                      
      500 +        if (lv) return lv->kind;                              
      501 +        GlobalVar *gv = codegen_find_global(cg, node->value); 
      502 +        if (gv) return gv->kind;                              
      503 +        return TYPE_INT;                                      
      504      }
      505      case AST_DEREF: {
      506          ASTNode *operand = node->children[0];
     ...
      510                  lv->full_type->base) {
      511                  return lv->full_type->base->kind;
      512              }
      513 +            GlobalVar *gv = codegen_find_global(cg, operand->v
          +alue);                                                        
      514 +            if (gv && gv->full_type && gv->kind == TYPE_POINTE
          +R &&                                                          
      515 +                gv->full_type->base) {                        
      516 +                return gv->full_type->base->kind;             
      517 +            }                                                 
      518          }
      519          return TYPE_INT;
      520      }
     ...
      527                  lv->full_type->base) {
      528                  return lv->full_type->base->kind;
      529              }
      530 +            GlobalVar *gv = codegen_find_global(cg, base->valu
          +e);                                                           
      531 +            if (gv && gv->full_type &&                        
      532 +                (gv->kind == TYPE_ARRAY || gv->kind == TYPE_PO
          +INTER) &&                                                     
      533 +                gv->full_type->base) {                        
      534 +                return gv->full_type->base->kind;             
      535 +            }                                                 
      536          }
      537          return TYPE_INT;
      538      }

● Now update the AST_ASSIGNMENT case in sizeof_type_of_expr:

● Update(src/codegen.c)
  ⎿  Added 2 lines
      572          if (node->value[0] != '\0') {
      573              LocalVar *lv = codegen_find_var(cg, node->value);
      574              if (lv) return lv->kind;
      575 +            GlobalVar *gv = codegen_find_global(cg, node->valu
          +e);                                                           
      576 +            if (gv) return gv->kind;                          
      577          } else if (node->child_count > 0) {
      578              return sizeof_type_of_expr(cg, node->children[0]);
      579          }

● Now update expr_result_type for AST_VAR_REF:
  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 14 lines, removed 5 lines
      607          break;
      608      case AST_VAR_REF: {
      609          LocalVar *lv = codegen_find_var(cg, node->value);
      610 -        if (lv && (lv->kind == TYPE_POINTER || lv->kind == TYP
          -E_ARRAY)) {                                                   
      611 -            /* Stage 13-03: an array name in a value context d
          -ecays to                                                      
      612 -             * a pointer to its element type. */              
      613 -            t = TYPE_POINTER;                                 
      610 +        if (lv) {                                             
      611 +            if (lv->kind == TYPE_POINTER || lv->kind == TYPE_A
          +RRAY) {                                                       
      612 +                /* Stage 13-03: an array name in a value conte
          +xt decays to                                                  
      613 +                 * a pointer to its element type. */          
      614 +                t = TYPE_POINTER;                             
      615 +            } else {                                          
      616 +                t = promote_kind(type_kind_from_size(lv->size)
          +);                                                            
      617 +            }                                                 
      618          } else {
      615 -            t = lv ? promote_kind(type_kind_from_size(lv->size
          -)) : TYPE_INT;                                                
      619 +            GlobalVar *gv = codegen_find_global(cg, node->valu
          +e);                                                           
      620 +            if (gv && (gv->kind == TYPE_POINTER || gv->kind ==
          + TYPE_ARRAY)) {                                               
      621 +                t = TYPE_POINTER;                             
      622 +            } else {                                          
      623 +                t = gv ? promote_kind(type_kind_from_size(gv->
          +size)) : TYPE_INT;                                            
      624 +            }                                                 
      625          }
      626          break;
      627      }

● Now update codegen_expression — the AST_VAR_REF case. Let me read the current
  location:

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 31 lines, removed 13 lines
      770      }
      771      if (node->type == AST_VAR_REF) {
      772          LocalVar *lv = codegen_find_var(cg, node->value);
      773 -        if (!lv) {                                            
      773 +        if (lv) {                                             
      774 +            /* Stage 13-03: array name in a value context deca
          +ys to a                                                       
      775 +             * pointer to its element type. The value is the a
          +rray's base                                                   
      776 +             * address (lea), not a load from the slot. Whole-
          +array                                                         
      777 +             * assignment is still rejected by the AST_ASSIGNM
          +ENT path,                                                     
      778 +             * which checks the LHS name before reaching this 
          +code. */                                                      
      779 +            if (lv->kind == TYPE_ARRAY) {                     
      780 +                fprintf(cg->output, "    lea rax, [rbp - %d]\n
          +", lv->offset);                                               
      781 +                node->result_type = TYPE_POINTER;             
      782 +                node->full_type = type_pointer(lv->full_type->
          +base);                                                        
      783 +                return;                                       
      784 +            }                                                 
      785 +            if (lv->kind == TYPE_POINTER) {                   
      786 +                node->result_type = TYPE_POINTER;             
      787 +                node->full_type = lv->full_type;              
      788 +            } else {                                          
      789 +                node->result_type = promote_kind(type_kind_fro
          +m_size(lv->size));                                            
      790 +            }                                                 
      791 +            emit_load_local(cg, lv->offset, lv->size);        
      792 +            return;                                           
      793 +        }                                                     
      794 +        /* Stage 22-01: fall back to global table. */         
      795 +        GlobalVar *gv = codegen_find_global(cg, node->value); 
      796 +        if (!gv) {                                            
      797              fprintf(stderr, "error: undeclared variable '%s'\n
           ", node->value);
      798              exit(1);
      799          }
      777 -        /* Stage 13-03: array name in a value context decays t
          -o a                                                           
      778 -         * pointer to its element type. The value is the array
          -'s base                                                       
      779 -         * address (lea), not a load from the slot. Whole-arra
          -y                                                             
      780 -         * assignment is still rejected by the AST_ASSIGNMENT 
          -path,                                                         
      781 -         * which checks the LHS name before reaching this code
          -. */                                                          
      782 -        if (lv->kind == TYPE_ARRAY) {                         
      783 -            fprintf(cg->output, "    lea rax, [rbp - %d]\n", l
          -v->offset);                                                   
      800 +        if (gv->kind == TYPE_ARRAY) {                         
      801 +            fprintf(cg->output, "    lea rax, [rel %s]\n", gv-
          +>name);                                                       
      802              node->result_type = TYPE_POINTER;
      785 -            node->full_type = type_pointer(lv->full_type->base
          -);                                                            
      803 +            node->full_type = type_pointer(gv->full_type->base
          +);                                                            
      804              return;
      805          }
      788 -        if (lv->kind == TYPE_POINTER) {                       
      806 +        if (gv->kind == TYPE_POINTER) {                       
      807              node->result_type = TYPE_POINTER;
      790 -            node->full_type = lv->full_type;                  
      808 +            node->full_type = gv->full_type;                  
      809          } else {
      792 -            node->result_type = promote_kind(type_kind_from_si
          -ze(lv->size));                                                
      810 +            node->result_type = promote_kind(type_kind_from_si
          +ze(gv->size));                                                
      811          }
      794 -        emit_load_local(cg, lv->offset, lv->size);            
      812 +        emit_load_global(cg, gv->name, gv->size);             
      813          return;
      814      }
      815      if (node->type == AST_ASSIGNMENT) {

● Now update AST_ASSIGNMENT to handle globals for the named-assignment case. Let
   me find the exact location:

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 30 lines, removed 11 lines
      877              return;
      878          }
      879          LocalVar *lv = codegen_find_var(cg, node->value);
      880 -        if (!lv) {                                            
      880 +        if (lv) {                                             
      881 +            /* Stage 13-01: arrays are not assignable. */     
      882 +            if (lv->kind == TYPE_ARRAY) {                     
      883 +                fprintf(stderr, "error: arrays are not assigna
          +ble\n");                                                      
      884 +                exit(1);                                      
      885 +            }                                                 
      886 +            codegen_expression(cg, node->children[0]);        
      887 +            /* Pointer RHS values are already in full rax — sk
          +ip the                                                        
      888 +             * 32-to-64 sign-extend that integer values would 
          +need. */                                                      
      889 +            int rhs_is_long = (node->children[0]->result_type 
          +== TYPE_LONG ||                                               
      890 +                               node->children[0]->result_type 
          +== TYPE_POINTER);                                             
      891 +            emit_store_local(cg, lv->offset, lv->size, rhs_is_
          +long);                                                        
      892 +            if (lv->kind == TYPE_POINTER) {                   
      893 +                node->result_type = TYPE_POINTER;             
      894 +                node->full_type = lv->full_type;              
      895 +            } else {                                          
      896 +                node->result_type = type_kind_from_size(lv->si
          +ze);                                                          
      897 +            }                                                 
      898 +            return;                                           
      899 +        }                                                     
      900 +        /* Stage 22-01: fall back to global table. */         
      901 +        GlobalVar *gv = codegen_find_global(cg, node->value); 
      902 +        if (!gv) {                                            
      903              fprintf(stderr, "error: undeclared variable '%s'\n
           ", node->value);
      904              exit(1);
      905          }
      884 -        /* Stage 13-01: arrays are not assignable. */         
      885 -        if (lv->kind == TYPE_ARRAY) {                         
      906 +        if (gv->kind == TYPE_ARRAY) {                         
      907              fprintf(stderr, "error: arrays are not assignable\
           n");
      908              exit(1);
      909          }
      910          codegen_expression(cg, node->children[0]);
      890 -        /* Pointer RHS values are already in full rax — skip t
          -he                                                            
      891 -         * 32-to-64 sign-extend that integer values would need
          -. */                                                          
      892 -        int rhs_is_long = (node->children[0]->result_type == T
          -YPE_LONG ||                                                   
      893 -                           node->children[0]->result_type == T
          -YPE_POINTER);                                                 
      894 -        emit_store_local(cg, lv->offset, lv->size, rhs_is_long
          -);                                                            
      895 -        if (lv->kind == TYPE_POINTER) {                       
      911 +        int rhs_is_long_g = (node->children[0]->result_type ==
          + TYPE_LONG ||                                                 
      912 +                             node->children[0]->result_type ==
          + TYPE_POINTER);                                               
      913 +        emit_store_global(cg, gv->name, gv->size, rhs_is_long_
          +g);                                                           
      914 +        if (gv->kind == TYPE_POINTER) {                       
      915              node->result_type = TYPE_POINTER;
      897 -            node->full_type = lv->full_type;                  
      916 +            node->full_type = gv->full_type;                  
      917          } else {
      899 -            node->result_type = type_kind_from_size(lv->size);
      918 +            node->result_type = type_kind_from_size(gv->size);
      919          }
      920          return;
      921      }

● Now update AST_ADDR_OF to handle globals. Let me read the current state:

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 10 lines, removed 3 lines
      934              return;
      935          }
      936          LocalVar *lv = codegen_find_var(cg, operand->value);
      937 -        if (!lv) {                                            
      937 +        if (lv) {                                             
      938 +            fprintf(cg->output, "    lea rax, [rbp - %d]\n", l
          +v->offset);                                                   
      939 +            node->result_type = TYPE_POINTER;                 
      940 +            node->full_type = type_pointer(local_var_type(lv))
          +;                                                             
      941 +            return;                                           
      942 +        }                                                     
      943 +        GlobalVar *gv = codegen_find_global(cg, operand->value
          +);                                                            
      944 +        if (!gv) {                                            
      945              fprintf(stderr, "error: undeclared variable '%s'\n
           ", operand->value);
      946              exit(1);
      947          }
      941 -        fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->o
          -ffset);                                                       
      948 +        fprintf(cg->output, "    lea rax, [rel %s]\n", gv->nam
          +e);                                                           
      949          node->result_type = TYPE_POINTER;
      943 -        node->full_type = type_pointer(local_var_type(lv));   
      950 +        node->full_type = type_pointer(global_var_type(gv));  
      951          return;
      952      }
      953      if (node->type == AST_DEREF) {

● Now update AST_SIZEOF_EXPR to handle global arrays:

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 8 lines
      1017                  node->result_type = TYPE_LONG;
      1018                  return;
      1019              }
      1020 +            if (!lv) {                                       
      1021 +                GlobalVar *gv = codegen_find_global(cg, child
           +->value);                                                    
      1022 +                if (gv && gv->kind == TYPE_ARRAY && gv->full_
           +type) {                                                      
      1023 +                    fprintf(cg->output, "    mov rax, %d\n", 
           +gv->full_type->size);                                        
      1024 +                    node->result_type = TYPE_LONG;           
      1025 +                    return;                                  
      1026 +                }                                            
      1027 +            }                                                
      1028          }
      1029          TypeKind kind = sizeof_type_of_expr(cg, child);
      1030          int sz;

● Now update AST_PREFIX_INC_DEC and AST_POSTFIX_INC_DEC:

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 42 lines, removed 19 lines
      1089           * declared width. */
      1090          const char *var_name = node->children[0]->value;
      1091          LocalVar *lv = codegen_find_var(cg, var_name);
      1092 -        if (!lv) {                                           
      1093 -            fprintf(stderr, "error: undeclared variable '%s'\
           -n", var_name);                                               
      1094 -            exit(1);                                         
      1095 -        }                                                    
      1096 -        emit_load_local(cg, lv->offset, lv->size);           
      1097 -        if (strcmp(node->value, "++") == 0) {                
      1098 -            fprintf(cg->output, "    add eax, 1\n");         
      1092 +        if (lv) {                                            
      1093 +            emit_load_local(cg, lv->offset, lv->size);       
      1094 +            if (strcmp(node->value, "++") == 0) {            
      1095 +                fprintf(cg->output, "    add eax, 1\n");     
      1096 +            } else {                                         
      1097 +                fprintf(cg->output, "    sub eax, 1\n");     
      1098 +            }                                                
      1099 +            emit_store_local(cg, lv->offset, lv->size, 0);   
      1100          } else {
      1100 -            fprintf(cg->output, "    sub eax, 1\n");         
      1101 +            GlobalVar *gv = codegen_find_global(cg, var_name)
           +;                                                            
      1102 +            if (!gv) {                                       
      1103 +                fprintf(stderr, "error: undeclared variable '
           +%s'\n", var_name);                                           
      1104 +                exit(1);                                     
      1105 +            }                                                
      1106 +            emit_load_global(cg, gv->name, gv->size);        
      1107 +            if (strcmp(node->value, "++") == 0) {            
      1108 +                fprintf(cg->output, "    add eax, 1\n");     
      1109 +            } else {                                         
      1110 +                fprintf(cg->output, "    sub eax, 1\n");     
      1111 +            }                                                
      1112 +            emit_store_global(cg, gv->name, gv->size, 0);    
      1113          }
      1102 -        emit_store_local(cg, lv->offset, lv->size, 0);       
      1114          node->result_type = TYPE_INT;
      1115          return;
      1116      }
     ...
      1118          /* x++ or x--: result is old value, then update varia
            ble */
      1119          const char *var_name = node->children[0]->value;
      1120          LocalVar *lv = codegen_find_var(cg, var_name);
      1110 -        if (!lv) {                                           
      1111 -            fprintf(stderr, "error: undeclared variable '%s'\
           -n", var_name);                                               
      1112 -            exit(1);                                         
      1113 -        }                                                    
      1114 -        emit_load_local(cg, lv->offset, lv->size);           
      1115 -        fprintf(cg->output, "    mov ecx, eax\n");  /* save o
           -ld value */                                                  
      1116 -        if (strcmp(node->value, "++") == 0) {                
      1117 -            fprintf(cg->output, "    add eax, 1\n");         
      1121 +        if (lv) {                                            
      1122 +            emit_load_local(cg, lv->offset, lv->size);       
      1123 +            fprintf(cg->output, "    mov ecx, eax\n");  /* sa
           +ve old value */                                              
      1124 +            if (strcmp(node->value, "++") == 0) {            
      1125 +                fprintf(cg->output, "    add eax, 1\n");     
      1126 +            } else {                                         
      1127 +                fprintf(cg->output, "    sub eax, 1\n");     
      1128 +            }                                                
      1129 +            emit_store_local(cg, lv->offset, lv->size, 0);   
      1130          } else {
      1119 -            fprintf(cg->output, "    sub eax, 1\n");         
      1131 +            GlobalVar *gv = codegen_find_global(cg, var_name)
           +;                                                            
      1132 +            if (!gv) {                                       
      1133 +                fprintf(stderr, "error: undeclared variable '
           +%s'\n", var_name);                                           
      1134 +                exit(1);                                     
      1135 +            }                                                
      1136 +            emit_load_global(cg, gv->name, gv->size);        
      1137 +            fprintf(cg->output, "    mov ecx, eax\n");  /* sa
           +ve old value */                                              
      1138 +            if (strcmp(node->value, "++") == 0) {            
      1139 +                fprintf(cg->output, "    add eax, 1\n");     
      1140 +            } else {                                         
      1141 +                fprintf(cg->output, "    sub eax, 1\n");     
      1142 +            }                                                
      1143 +            emit_store_global(cg, gv->name, gv->size, 0);    
      1144          }
      1121 -        emit_store_local(cg, lv->offset, lv->size, 0);       
      1145          fprintf(cg->output, "    mov eax, ecx\n");  /* restor
            e old value as result */
      1146          node->result_type = TYPE_INT;
      1147          return;

● Now update codegen_translation_unit to register globals and emit .bss. Let me
  read the current state:

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 81 lines
      2189      }
      2190  }
      2191  
      2192 +/*                                                           
      2193 + * Stage 22-01: map a TypeKind to its NASM .bss reserve direc
           +tive mnemonic.                                               
      2194 + * Pointer and long use resq (8 bytes); int uses resd (4 byte
           +s); short uses                                               
      2195 + * resw (2 bytes); char uses resb (1 byte).                  
      2196 + */                                                          
      2197 +static const char *bss_res_directive(TypeKind kind) {        
      2198 +    switch (kind) {                                          
      2199 +    case TYPE_CHAR:    return "resb";                        
      2200 +    case TYPE_SHORT:   return "resw";                        
      2201 +    case TYPE_LONG:                                          
      2202 +    case TYPE_POINTER: return "resq";                        
      2203 +    case TYPE_INT:                                           
      2204 +    default:           return "resd";                        
      2205 +    }                                                        
      2206 +}                                                            
      2207 +                                                             
      2208 +/*                                                           
      2209 + * Stage 22-01: register a file-scope AST_DECLARATION in the 
           +global table                                                 
      2210 + * and emit its .bss storage line.                           
      2211 + */                                                          
      2212 +static void codegen_add_global(CodeGen *cg, ASTNode *decl) { 
      2213 +    if (cg->global_count >= MAX_GLOBALS) {                   
      2214 +        fprintf(stderr, "error: too many global variables (ma
           +x %d)\n", MAX_GLOBALS);                                      
      2215 +        exit(1);                                             
      2216 +    }                                                        
      2217 +    /* Duplicate global check. */                            
      2218 +    for (int i = 0; i < cg->global_count; i++) {             
      2219 +        if (strcmp(cg->globals[i].name, decl->value) == 0) { 
      2220 +            fprintf(stderr, "error: duplicate global declarat
           +ion '%s'\n", decl->value);                                   
      2221 +            exit(1);                                         
      2222 +        }                                                    
      2223 +    }                                                        
      2224 +    GlobalVar *gv = &cg->globals[cg->global_count];          
      2225 +    strncpy(gv->name, decl->value, 255);                     
      2226 +    gv->name[255] = '\0';                                    
      2227 +    gv->kind = decl->decl_type;                              
      2228 +    gv->full_type = decl->full_type;                         
      2229 +    if (decl->decl_type == TYPE_ARRAY && decl->full_type) {  
      2230 +        gv->size = decl->full_type->base                     
      2231 +                   ? type_kind_bytes(decl->full_type->base->k
           +ind)                                                         
      2232 +                   : 4;                                      
      2233 +    } else {                                                 
      2234 +        gv->size = type_kind_bytes(decl->decl_type);         
      2235 +    }                                                        
      2236 +    cg->global_count++;                                      
      2237 +}                                                            
      2238 +                                                             
      2239 +static void codegen_emit_bss(CodeGen *cg, ASTNode *tu) {     
      2240 +    int has_globals = 0;                                     
      2241 +    for (int i = 0; i < tu->child_count; i++) {              
      2242 +        if (tu->children[i]->type == AST_DECLARATION) {      
      2243 +            has_globals = 1;                                 
      2244 +            break;                                           
      2245 +        }                                                    
      2246 +    }                                                        
      2247 +    if (!has_globals) return;                                
      2248 +                                                             
      2249 +    fprintf(cg->output, "section .bss\n");                   
      2250 +    for (int i = 0; i < cg->global_count; i++) {             
      2251 +        GlobalVar *gv = &cg->globals[i];                     
      2252 +        if (gv->kind == TYPE_ARRAY && gv->full_type) {       
      2253 +            int count = gv->full_type->length / gv->size;    
      2254 +            fprintf(cg->output, "%s: %s %d\n",               
      2255 +                    gv->name,                                
      2256 +                    bss_res_directive(gv->full_type->base->ki
           +nd),                                                         
      2257 +                    count);                                  
      2258 +        } else {                                             
      2259 +            fprintf(cg->output, "%s: %s 1\n",                
      2260 +                    gv->name, bss_res_directive(gv->kind));  
      2261 +        }                                                    
      2262 +    }                                                        
      2263 +}                                                            
      2264 +                                                             
      2265  void codegen_translation_unit(CodeGen *cg, ASTNode *node) {
      2266      cg->tu_root = node;
      2267 +    if (node->type == AST_TRANSLATION_UNIT) {                
      2268 +        /* Stage 22-01: first pass — register globals and emi
           +t .bss. */                                                   
      2269 +        for (int i = 0; i < node->child_count; i++) {        
      2270 +            if (node->children[i]->type == AST_DECLARATION)  
      2271 +                codegen_add_global(cg, node->children[i]);   
      2272 +        }                                                    
      2273 +        codegen_emit_bss(cg, node);                          
      2274 +    }                                                        
      2275      fprintf(cg->output, "section .text\n");
      2276      if (node->type == AST_TRANSLATION_UNIT) {
      2277          codegen_emit_externs(cg, node);

● Now let's build and verify the compiler compiles:

● Bash(make 2>&1)
  ⎿  [ 11%] Building C object CMakeFiles/ccompiler.dir/src/compiler.c.o
     [ 22%] Building C object CMakeFiles/ccompiler.dir/src/codegen.c.o
     [ 33%] Building C object CMakeFiles/ccompiler.dir/src/parser.c.o
     … +2 lines (ctrl+o to expand)

● Clean build. Now let's run the full test suite to verify nothing is broken:

● Bash(cd /home/scott/code/claude/claudec99 && bash test/run_all_tests.sh 2>&1 |
       tail -20)
  ⎿  PASS  test_stage_14_06_char_array_string_init_explicit
     PASS  test_stage_14_06_char_array_string_init_inferred
     PASS  test_stage_14_07_libc_puts
     … +17 lines (ctrl+o to expand)

● 635 tests still pass. Now let's add the spec-required tests. First, a quick
  manual test to verify globals work:

● Bash(cat > /tmp/test_global.c << 'EOF'
      int x;…)
  ⎿  Error: Exit code 1
     compiled: /tmp/test_global.c -> test_global.asm
     cat: /tmp/test_global.asm: No such file or directory

● Bash(ls *.asm 2>/dev/null && /home/scott/code/claude/claudec99/build/ccompiler
       /tmp/test_global.c && cat test_global.asm)
  ⎿  test_global.asm
     compiled: /tmp/test_global.c -> test_global.asm
     section .bss
     … +12 lines (ctrl+o to expand)
● Basic global is working correctly. Let me test the other spec examples:

● Bash(cat > /tmp/test_shadow.c << 'EOF'
      int x;…)
  ⎿  compiled: /tmp/test_shadow.c -> test_shadow.asm
     exit: 3

● Bash(cat > /tmp/test_counter.c << 'EOF'
      int counter;…)
  ⎿  compiled: /tmp/test_counter.c -> test_counter.asm
     exit: 2

● Bash(cat > /tmp/test_types.c << 'EOF'
      int g;…)
  ⎿  compiled: /tmp/test_types.c -> test_types.asm
     exit: 8

  Read 1 file (ctrl+o to expand)

● Wait, I see an issue — arr: resd 2 should be arr: resd 10. Let me check how
  type_array stores the length:

  Read 2 files (ctrl+o to expand)

● full_type->length is the element count (10), and gv->size is the element size
  (4). The bug was dividing them. Fix:

● Update(src/codegen.c)
  ⎿  Added 1 line, removed 2 lines
      2250      for (int i = 0; i < cg->global_count; i++) {
      2251          GlobalVar *gv = &cg->globals[i];
      2252          if (gv->kind == TYPE_ARRAY && gv->full_type) {
      2253 -            int count = gv->full_type->length / gv->size;    
      2253              fprintf(cg->output, "%s: %s %d\n",
      2254                      gv->name,
      2255                      bss_res_directive(gv->full_type->base->ki
            nd),
      2257 -                    count);                                  
      2256 +                    gv->full_type->length);                  
      2257          } else {
      2258              fprintf(cg->output, "%s: %s 1\n",
      2259                      gv->name, bss_res_directive(gv->kind));

● Bash(cd /home/scott/code/claude/claudec99/build && make 2>&1 | tail -3 &&
      /home/scott/code/claude/claudec99/build/ccompiler /tmp/test_types.c && cat
       test_types.asm |…)
  ⎿  [ 11%] Building C object CMakeFiles/ccompiler.dir/src/codegen.c.o
     [ 22%] Linking C executable ccompiler
     [100%] Built target ccompiler
     … +11 lines (ctrl+o to expand)

● arr: resd 10 is now correct. Let me verify the runtime result too:

● Bash(nasm -f elf64 test_types.asm -o /tmp/test_types.o && gcc -no-pie
      /tmp/test_types.o -o /tmp/test_types && /tmp/test_types; echo "exit: $?")
  ⎿  exit: 8

● Now let me run all existing tests to confirm no regressions:

● Bash(cd /home/scott/code/claude/claudec99 && bash test/run_all_tests.sh 2>&1 |
       tail -10)
  ⎿  PASS  test_stage_16_04_bitand_int
     PASS  test_stage_16_04_bitand_long
     PASS  test_stage_16_04_bitor_int
     … +7 lines (ctrl+o to expand)

● Now let me write the new test files. First the spec-required valid tests:

● Write(test/valid/test_global_basic__7.c)
  ⎿  Wrote 6 lines to test/valid/test_global_basic__7.c
      1 int x;
      2 
      3 int main() {
      4     x = 7;
      5     return x;
      6 }

● Write(test/valid/test_global_shadow__3.c)
  ⎿  Wrote 6 lines to test/valid/test_global_shadow__3.c
      1 int x;
      2 
      3 int main() {
      4     int x = 3;
      5     return x;
      6 }

● Write(test/valid/test_global_persist__2.c)
  ⎿  Wrote 12 lines to test/valid/test_global_persist__2.c
       1 int counter;
       2 
       3 int inc() {
       4     counter = counter + 1;
       5     return counter;
       6 }
       7 
       8 int main() {
       9     inc();
      10     inc();
     … +2 lines (ctrl+o to expand)

● Write(test/valid/test_global_types__42.c)
  ⎿  Wrote 12 lines to test/valid/test_global_types__42.c
       1 int g;
       2 char c;
       3 short s;
       4 long l;
       5 
       6 int main() {
       7     g = 10;
       8     c = 20;
       9     s = 5;
      10     l = 7;
     … +2 lines (ctrl+o to expand)

● Write(test/valid/test_global_pointer__5.c)
  ⎿  Wrote 8 lines to test/valid/test_global_pointer__5.c
      1 int val;
      2 int *p;
      3 
      4 int main() {
      5     val = 5;
      6     p = &val;
      7     return *p;
      8 }

● Write(test/valid/test_global_array__9.c)
  ⎿  Wrote 7 lines to test/valid/test_global_array__9.c
      1 int arr[10];
      2 
      3 int main() {
      4     arr[0] = 4;
      5     arr[1] = 5;
      6     return arr[0] + arr[1];
      7 }

● Write(test/valid/test_global_zero_init__0.c)
  ⎿  Wrote 5 lines to test/valid/test_global_zero_init__0.c
      1 int g;
      2 
      3 int main() {
      4     return g;
      5 }

● Now let me run these tests to confirm they all pass:
● Bash(bash test/valid/run_tests.sh 2>&1 | grep -E "global|Results")
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_global_arr
     ay__9.c -> test_global_array__9.asm
     PASS  test_global_array__9  (exit code: 9)
     … +14 lines (ctrl+o to expand)

● All 7 new global tests pass. Now let me add a print_asm test for the .bss
  layout. Let me check an existing print_asm test to understand the format:

  Read 2 files (ctrl+o to expand)

● Write(test/print_asm/test_stage_22_01_global_bss_layout.c)
  ⎿  Wrote 11 lines to test/print_asm/test_stage_22_01_global_bss_layout.c
       1 int g;
       2 char c;
       3 short s;
       4 long l;
       5 int *p;
       6 int arr[10];
       7 
       8 int main() {
       9     g = 1;
      10     return g;
     … +1 line (ctrl+o to expand)

● Bash(/home/scott/code/claude/claudec99/build/ccompiler /home/scott/code/claude
      /claudec99/test/print_asm/test_stage_22_01_global_bss_layout.c && cat
      test_stage_22_01_…)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/print_asm/test_stage_
     22_01_global_bss_layout.c -> test_stage_22_01_global_bss_layout.asm
     section .bss
     … +17 lines (ctrl+o to expand)

● Bash(cp test_stage_22_01_global_bss_layout.asm /home/scott/code/claude/claudec
      99/test/print_asm/test_stage_22_01_global_bss_layout.expected)
  ⎿  Done

● Now add a second print_asm test for the RIP-relative read/write pattern:

● Write(test/print_asm/test_stage_22_01_global_rip_relative.c)
  ⎿  Wrote 6 lines to test/print_asm/test_stage_22_01_global_rip_relative.c
      1 int g;
      2 
      3 int main() {
      4     g = 42;
      5     return g;
      6 }

● Bash(/home/scott/code/claude/claudec99/build/ccompiler /home/scott/code/claude
      /claudec99/test/print_asm/test_stage_22_01_global_rip_relative.c && cp
      test_stage_22_01…)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/print_asm/test_stage_
     22_01_global_rip_relative.c -> test_stage_22_01_global_rip_relative.as
     m
     … +13 lines (ctrl+o to expand)

● Now let me add an invalid test for duplicate global declarations:

  Listed 1 directory (ctrl+o to expand)

● Write(test/invalid/test_invalid_109__duplicate_global_declaration.c)
  ⎿  Wrote 6 lines to
     test/invalid/test_invalid_109__duplicate_global_declaration.c
      1 int g;
      2 int g;
      3 
      4 int main() {
      5     return g;
      6 }

● Bash(/home/scott/code/claude/claudec99/build/ccompiler /home/scott/code/claude
      /claudec99/test/invalid/test_invalid_109__duplicate_global_declaration.c
      2>&1; echo "ex…)
  ⎿  error: duplicate global declaration 'g'
     exit: 1

● Now let me also check the invalid test runner to understand what format it
  expects:

  Read 1 file (ctrl+o to expand)

● Good. The test filename test_invalid_109__duplicate_global_declaration.c would
   match the error message "duplicate global declaration" (underscores become
  spaces). Let me verify the test runner will accept it:

● Bash(bash test/invalid/run_tests.sh 2>&1 | grep -E "109|Results")
  ⎿  PASS  test_invalid_109__duplicate_global_declaration  (error contains:
      'duplicate global declaration')
     Results: 111 passed, 0 failed, 111 total

● Now let's run the full test suite to confirm everything still passes:

● Bash(bash test/run_all_tests.sh 2>&1 | tail -10)
  ⎿  PASS  test_stage_16_04_bitor_int
     PASS  test_stage_16_04_bitxor_int
     PASS  test_stage_22_01_global_bss_layout
     … +7 lines (ctrl+o to expand)

● 645 tests pass — up from 635. Now let me update docs/grammar.md to remove the
  restriction and update the grammar:

● Update(docs/grammar.md)
  ⎿  Added 1 line, removed 1 line
      157  <character_escape_sequence> ::= "\a" | "\b" | "\f" | "\n" | "\
           r" | "\t" | "\v"
      158                                | "\\" | "\'" | "\"" | "\?" | "\
           0"
      159  
      160 -# Current Restriction : no file-scope variable declarations   
      160 +# Current Restriction : file-scope object declarations must be
          + uninitialized                                                
      161  # Current Restriction : for-loop initializers are expressions 
           only, not declarations
      162  # Current Restriction : array declarations are limited to a si
           ngle bracket suffix.
      163  # Semantic Restriction : assignment left-hand sides must be va
           lid lvalues.

● Now let me delegate the Milestone Summary, Transcript, and README update to
  haiku-stage-artifact-writer:

● haiku-stage-artifact-writer(Generate milestone, transcript, and README update
                             for stage-22-01)
  ⎿  Done (10 tool uses · 16.0k tokens · 43s)
  (ctrl+o to expand)

● Let me verify the generated artifacts and README are correct:

  Searched for 1 pattern, listed 1 directory (ctrl+o to expand)

● All artifacts exist and the README is updated correctly. Now let me commit:

● Bash(git diff --stat)  ⎿   README.md         |  16 ++-
      docs/grammar.md   |   2 +-
      include/codegen.h |  15 +++
     … +4 lines (ctrl+o to expand)

● Bash(git status)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +20 lines (ctrl+o to expand)

● Bash(git log --oneline -3)
  ⎿  3e18a8c changes from stage 21-03 and stage 22-01 spec
     0de927a stage 21-02 session files
     ab7ab0f stage 21-02: separate function definitions from declarations

● Bash(git add src/parser.c include/codegen.h src/codegen.c docs/grammar.md
      README.md \
        docs/kickoffs/stage-22-01-kickoff.md \…)
  ⎿  [master fb88c91] stage 22-01: file-scope uninitialized object declarat
     ions
      20 files changed, 677 insertions(+), 75 deletions(-)
     … +16 lines (ctrl+o to expand)

● Stage 22-01 is complete. Here's a summary of what was done:

  Stage-22-01 Complete

  What changed:

  - src/parser.c — Fixed parse_external_declaration to correctly handle
  file-scope array declarations (d.is_array), which was silently storing the
  wrong type.
  - include/codegen.h — Added GlobalVar struct and globals[]/global_count fields
   to CodeGen (MAX_GLOBALS = 64).
  - src/codegen.c — Added a separate global symbol table with RIP-relative
  load/store helpers (emit_load_global, emit_store_global). All variable-access
  paths (AST_VAR_REF, AST_ASSIGNMENT, AST_ADDR_OF, prefix/postfix inc/dec,
  sizeof) now fall back to global lookup after failing to find a local.
  codegen_translation_unit registers globals and emits the .bss section before
  .text.
  - docs/grammar.md — Updated restriction to "file-scope object declarations
  must be uninitialized."
  - README.md — Updated stage line, added global variables to supported
  features, updated test totals.

  10 new tests, 645 total (up from 635), 0 failures.

✻ Worked for 11m 56s