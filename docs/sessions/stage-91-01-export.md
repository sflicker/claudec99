 ▐▛███▜▌   Claude Code v2.1.162
▝▜█████▛▘  Opus 4.8 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

  Feature of the week: /loop — run a prompt or slash command on a recurring interval


❯ /usage 
  ⎿  Settings dialog dismissed

❯ /implement-stage docs/stages/ClaudeC99-spec-stage-91-01-support-struct-by-val 
ue-parameters-and-returns.md                                                    

● I'll start by reading the spec file, supporting files, and README as
  instructed.

  Read 4 files (ctrl+o to expand)

● Let me echo the derived value and look at the skill structure and existing
  artifacts for conventions.

  Listed 2 directories (ctrl+o to expand)

● Bash(python3 .claude/skills/implement-stage/scripts/stage-label.py
      docs/stages/ClaudeC99-spec-stage-91-01-support-struct-by-value-parameters-
      and-returns.md 2>&1; ech…)
  ⎿  stage-91-01
     exit=0

● Bash(python3 .claude/skills/implement-stage/scripts/stage-label.py --help 2>&1
      | head -30; echo "==="; sed -n '1,80p'
      .claude/skills/implement-stage/scripts/stage-la…)
  ⎿  ERROR: filename does not contain '-stage-'
     ===
     #!/usr/bin/env python3
     … +45 lines (ctrl+o to expand)

  Read 1 file (ctrl+o to expand)

● STAGE_DISPLAY: Stage 91-01

  Now let me explore the codebase to understand the current
  function-call/parameter/return architecture. This is a complex SysV ABI
  feature.

  Searched for 6 patterns, read 2 files (ctrl+o to expand)

● Let me check how the parser currently handles struct parameters/returns and
  whether they're rejected anywhere.

  Searched for 4 patterns, read 1 file (ctrl+o to expand)

● Bash(grep -n "int type_size\|type_size(\|int size;\|struct.*size\|StructField"
      include/type.h; echo "---"; sed -n '1,60p' include/type.h)
  ⎿  31:    /* Stage 30: named struct type. size and alignment are computed
      from the
     47:} StructField;
     … +65 lines (ctrl+o to expand)

● I now have a clear picture. Let me present my analysis before implementing.

  1. Spec Summary — Stage 91-01: Struct-by-Value Parameters and Returns

  The compiler must support passing a struct by value as a function parameter
  and returning a struct by value. Semantics: the callee receives a copy,
  mutates the copy locally, and the returned struct is a copy placed into the
  caller's destination — the caller's original is untouched.

  The two spec tests both use this 16-byte struct:
  typedef struct Token { char *value; int length; } Token;   // 8 + 4, padded to
  16
  static Token finalize(Token token) { token.length = strlen(token.value);
  return token; }
  - Test 1: t = finalize(t); return t.length; → exit 5
  - Test 2: proves the caller's original is unchanged: return t.length==99 && 
  u.length==5; → exit 1

  2. What must change from the previous stage

  - Parser (parse_parameter_declaration): a struct/union value parameter
  currently sets decl_type=TYPE_STRUCT but leaves full_type NULL (codegen needs
  the size). Fix to attach full_type.
  - Codegen — function prologue: store incoming struct argument register(s) into
  the parameter's stack slot (currently type_kind_bytes(TYPE_STRUCT)==0, so the
  slot is broken).
  - Codegen — call site (AST_FUNCTION_CALL): the uniform "1 arg = 1 rax
  push/pop" model can't load a struct into one/two registers. Needs per-argument
  struct handling.
  - Codegen — return (AST_RETURN_STATEMENT): load the returned struct's
  eightbyte(s) into rax(:rdx).
  - Codegen — assignment: struct copy-assignment currently only accepts a
  VAR_REF source; t = finalize(t) (a struct-returning call) must be accepted.

  3. Ambiguity / scope issue to resolve before implementing

  The spec's Token is 16 bytes → SysV AMD64 register class (two INTEGER
  eightbytes, passed in rdi:rsi, returned in rax:rdx). That's all the tests
  exercise.

  But the project's real Token (in token.h) is ~290 bytes — a memory-class
  struct that must be passed on the stack and returned via a hidden pointer
  (sret). The stage-92 self-compilation report names exactly this as the
  blocker. The spec title says "support struct-by-value" generally, but its only
  tests cover the register-class case. This is a real scope fork, so I want
  your call before building.
● User answered Claude's questions:
  ⎿  · How wide should the struct-by-value implementation go in this stage? → 
     Full SysV (reg + memory/sret)

● The user chose Full SysV (register + memory/sret). This is a substantial
  codegen change. Let me gather the remaining mechanics I need: frame-size
  computation, struct-copy emit, and how struct var-refs currently evaluate.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● I need to verify the exact struct byte-layout convention. Let me read the
  member-address and struct-declaration code.

  Read 1 file (ctrl+o to expand)

● Let me verify a few facts needed for the plan: function return full_type, the
  function-decl finder, and let me set up task tracking.

● Bash(grep -n "codegen_find_function_decl\|current_return_full_type\|func->full
      _type\|->full_type = .*return\|decl->full_type" src/codegen.c | head; echo
      "=== parser:…)
  ⎿  161:    Type *return_type = (func_decl->decl_type == TYPE_POINTER && f
     unc_decl->full_type)
     162:                        ? func_decl->full_type
     … +89 lines (ctrl+o to expand)

● I now have a complete understanding of the architecture. Let me check the
  kickoff template, then delegate the kickoff artifact while I finalize the
  plan.

  Read 1 file (ctrl+o to expand)

● haiku-stage-artifact-writer(Write stage 91-01 kickoff artifact)
  ⎿  Done (8 tool uses · 11.4k tokens · 25s)
  (ctrl+o to expand)

  Listed 1 directory (ctrl+o to expand)

● The kickoff is written. Here is the plan before I start editing.

  Planned Changes

  src/parser.c (2 small fixes)
  1. parse_parameter_declaration: attach full_type to struct/union value params
  (currently NULL).
  2. Function-decl parsing: set func->full_type for struct/union return types
  (currently only set for pointer returns).

  src/codegen.c (the bulk)
  3. Shared SysV layout helper (eightbyte classification ≤16B reg / >16B memory;
  GP-reg assignment; sret reserves rdi).
  4. Block-copy helper for N struct bytes.
  5. AST_FUNCTION_CALL rewrite: struct args (reg + memory), hidden sret pointer,
  materialize struct return into a frame scratch temp (address left in rax).
  6. Frame sizing: real struct-param sizes + scratch for struct-return temps;
  new CodeGen cursor/sret fields.
  7. Prologue: store reg-class struct params; reference memory-class struct
  params in place (negative rbp offset into the incoming stack copy); save the
  hidden sret pointer; set current_return_full_type for struct returns.
  8. AST_RETURN_STATEMENT: reg-class → rax:rdx; memory-class → copy into [saved 
  sret ptr], rax=ptr.
  9. Struct assignment + struct decl-init: accept a struct-returning call
  (general struct rvalue) as source.

  src/version.c → VERSION_STAGE "00910100".
  Tests: 2 spec tests (exit 5, exit 1) + memory-class (>16B) return &
  pass-through coverage.
  Docs/artifacts: README, grammar (no grammar change expected — by-value is
  semantic), milestone + transcript via haiku.

  The scope gate is settled, so I'll start implementing now (smallest/safest
  first) and checkpoint before committing. Starting with the parser.

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 4 lines
      2888      } else {
      2889          param->decl_type = base_kind;
      2890          param->is_unsigned = !base_type->is_signed;
      2891 +        /* Stage 91-01: a struct/union passed by value needs 
           +its full type                                                
      2892 +         * so codegen knows the object's size and field layou
           +t. */                                                        
      2893 +        if (base_kind == TYPE_STRUCT || base_kind == TYPE_UNI
           +ON)                                                          
      2894 +            param->full_type = base_type;                    
      2895      }
      2896      return param;
      2897  }

● Update(src/parser.c)
  ⎿  Added 5 lines
      3368      func->storage_class = sc;
      3369      if (d.pointer_count > 0)
      3370          func->full_type = full_type;
      3371 +    /* Stage 91-01: a struct/union returned by value needs it
           +s full type so                                               
      3372 +     * codegen knows the object's size (and whether it is reg
           +ister- or                                                    
      3373 +     * memory-class for the SysV AMD64 return convention). */
      3374 +    else if (return_kind == TYPE_STRUCT || return_kind == TYP
           +E_UNION)                                                     
      3375 +        func->full_type = base_type;                         
      3376  
      3377      parser_expect(parser, TOKEN_LPAREN);
      3378      /* `f(void)` — the sole `void` keyword means zero paramet
            ers. */

● Now the codegen. Let me look at the CodeGen struct to add the new fields I
  need.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Update(include/codegen.h)
  ⎿  Added 11 lines
      151      int variadic_reg_save_offset;
      152      int variadic_named_gp_params;
      153      int variadic_named_stack_params;
      154 +    /* Stage 91-01: struct-by-value return support.           
      155 +     * current_sret_offset: rbp-relative offset of the slot ho
          +lding the hidden                                              
      156 +     * return pointer (rdi at entry) when the current function
          + returns a                                                    
      157 +     * memory-class struct (>16 bytes); 0 otherwise.          
      158 +     * struct_ret_scratch_base / struct_ret_scratch_cursor: a 
          +per-function                                                  
      159 +     * scratch region used to materialize struct-returning cal
          +l results. The                                                
      160 +     * base is the highest rbp offset of the region (reserved 
          +in the prologue);                                             
      161 +     * the cursor bumps downward as each call site claims a te
          +mp. */                                                        
      162 +    int current_sret_offset;                                  
      163 +    int struct_ret_scratch_base;                              
      164 +    int struct_ret_scratch_cursor;                            
      165  } CodeGen;
      166  
      167  void codegen_init(CodeGen *cg, FILE *output);

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 3 lines
      267      cg->variadic_reg_save_offset = 0;
      268      cg->variadic_named_gp_params = 0;
      269      cg->variadic_named_stack_params = 0;
      270 +    cg->current_sret_offset = 0;                              
      271 +    cg->struct_ret_scratch_base = 0;                          
      272 +    cg->struct_ret_scratch_cursor = 0;                        
      273  }
      274  
      275  /* Stage 66/70-03: warn with a variable name embedded.

● Now let me add the core SysV helpers. Let me look at the top region to place
  them well.
  Read 1 file (ctrl+o to expand)

● Now I'll add the core SysV helpers (classification, layout, block-copy) after
  type_kind_bytes.

● Update(src/codegen.c)
  ⎿  Added 107 lines
       34      return 4;
       35  }
       36  
       37 +static int is_struct_or_union_kind(TypeKind k) {              
       38 +    return k == TYPE_STRUCT || k == TYPE_UNION;               
       39 +}                                                             
       40 +                                                              
       41  /*
       42 + * Stage 91-01: SysV AMD64 classification for a struct/union p
          +assed or returned                                             
       43 + * by value. This compiler has no floating-point types, so eve
          +ry eightbyte is                                               
       44 + * INTEGER class. A struct of 1..8 bytes occupies one GP eight
          +byte; 9..16 bytes                                             
       45 + * two GP eightbytes; anything larger is MEMORY class (passed 
          +on the stack and                                              
       46 + * returned through a hidden pointer). Returns the GP eightbyt
          +e count for a                                                 
       47 + * register-class struct, or 0 for a memory-class (or unsized)
          + one.                                                         
       48 + */                                                           
       49 +static int struct_reg_eightbytes(Type *st) {                  
       50 +    if (!st) return 0;                                        
       51 +    int sz = st->size;                                        
       52 +    if (sz <= 0 || sz > 16) return 0;                         
       53 +    return (sz > 8) ? 2 : 1;                                  
       54 +}                                                             
       55 +                                                              
       56 +/* True when the function's return type is a memory-class (>16
          + byte) struct,                                                
       57 + * which the SysV ABI returns via a hidden pointer passed in r
          +di. */                                                        
       58 +static int return_is_memory_struct(ASTNode *fn_decl) {        
       59 +    return fn_decl && is_struct_or_union_kind(fn_decl->decl_ty
          +pe) &&                                                        
       60 +           fn_decl->full_type && fn_decl->full_type->size > 16
          +;                                                             
       61 +}                                                             
       62 +                                                              
       63 +/*                                                            
       64 + * Stage 91-01: emit a byte-for-byte block copy of `nbytes` by
          +tes from the                                                  
       65 + * address in rsi to the address in rdi. Uses `rep movsb`, whi
          +ch advances and                                               
       66 + * thus clobbers rcx, rsi, and rdi (the SysV ABI keeps the dir
          +ection flag                                                   
       67 + * clear, and this code never sets it, so a forward copy is co
          +rrect).                                                       
       68 + */                                                           
       69 +static void emit_struct_copy(CodeGen *cg, int nbytes) {       
       70 +    fprintf(cg->output, "    mov rcx, %d\n", nbytes);         
       71 +    fprintf(cg->output, "    rep movsb\n");                   
       72 +}                                                             
       73 +                                                              
       74 +/* Count the leading AST_PARAM children of a function declarat
          +ion node. */                                                  
       75 +static int count_params(ASTNode *fn_decl) {                   
       76 +    int n = 0;                                                
       77 +    if (!fn_decl) return 0;                                   
       78 +    while (n < fn_decl->child_count && fn_decl->children[n]->t
          +ype == AST_PARAM)                                             
       79 +        n++;                                                  
       80 +    return n;                                                 
       81 +}                                                             
       82 +                                                              
       83 +/*                                                            
       84 + * Stage 91-01: per-argument placement under the SysV AMD64 AB
          +I, shared by the                                              
       85 + * call site and the callee prologue so both agree on register
          +/stack layout.                                                
       86 + */                                                           
       87 +typedef struct {                                              
       88 +    int is_struct;   /* argument is a struct/union value */   
       89 +    int mem;         /* passed on the stack (memory class or o
          +ut of registers) */                                           
       90 +    int nbytes;      /* object size in bytes (8 for scalars/po
          +inters) */                                                    
       91 +    int gp_start;    /* first GP register index (0=rdi..5=r9),
          + or -1 if on stack */                                         
       92 +    int gp_count;    /* number of GP registers consumed (>=1) 
          +when in registers */                                          
       93 +    int stack_off;   /* byte offset within the outgoing stack-
          +arg area if on stack */                                       
       94 +} ArgSlot;                                                    
       95 +                                                              
       96 +typedef struct {                                              
       97 +    int sret;        /* hidden return pointer occupies rdi */ 
       98 +    int stack_bytes; /* total bytes of stack-passed arguments 
          +(each 8-aligned) */                                           
       99 +    int count;                                                
      100 +    ArgSlot items[FUNC_MAX_PARAMS + 8];                       
      101 +} CallLayout;                                                 
      102 +                                                              
      103 +/*                                                            
      104 + * Compute argument placement for a call to `fn_decl` (which m
          +ay be NULL for an                                             
      105 + * undeclared/libc callee) with `nargs` actual arguments. Stru
          +ct/union value                                                
      106 + * parameters are classified per their declared full type; arg
          +uments beyond the                                             
      107 + * declared parameters (variadic extras) are treated as scalar
          + GP eightbytes.                                               
      108 + */                                                           
      109 +static void compute_call_layout(CallLayout *L, ASTNode *fn_dec
          +l, int nargs) {                                               
      110 +    int num_params = count_params(fn_decl);                   
      111 +    L->sret = return_is_memory_struct(fn_decl);               
      112 +    L->count = nargs;                                         
      113 +    int gp_next = L->sret ? 1 : 0;                            
      114 +    int stack_off = 0;                                        
      115 +    for (int i = 0; i < nargs; i++) {                         
      116 +        ArgSlot *s = &L->items[i];                            
      117 +        s->gp_start = -1; s->gp_count = 0; s->stack_off = -1; 
          +s->mem = 0;                                                   
      118 +        ASTNode *p = (i < num_params) ? fn_decl->children[i] :
          + NULL;                                                        
      119 +        int is_struct = p && is_struct_or_union_kind(p->decl_t
          +ype);                                                         
      120 +        s->is_struct = is_struct;                             
      121 +        if (is_struct) {                                      
      122 +            int sz = p->full_type ? p->full_type->size : 0;   
      123 +            int ebs = struct_reg_eightbytes(p->full_type);    
      124 +            s->nbytes = sz;                                   
      125 +            if (ebs > 0 && gp_next + ebs <= 6) {              
      126 +                s->gp_start = gp_next; s->gp_count = ebs; gp_n
          +ext += ebs;                                                   
      127 +            } else {                                          
      128 +                s->mem = 1;                                   
      129 +                s->stack_off = stack_off;                     
      130 +                stack_off += (sz + 7) & ~7;                   
      131 +            }                                                 
      132 +        } else {                                              
      133 +            s->nbytes = 8;                                    
      134 +            if (gp_next < 6) {                                
      135 +                s->gp_start = gp_next++; s->gp_count = 1;     
      136 +            } else {                                          
      137 +                s->mem = 1; s->stack_off = stack_off; stack_of
          +f += 8;                                                       
      138 +            }                                                 
      139 +        }                                                     
      140 +    }                                                         
      141 +    L->stack_bytes = stack_off;                               
      142 +}                                                             
      143 +                                                              
      144 +/*                                                            
      145   * Emit instructions to convert the value currently in rax/eax
            from
      146   * `src` to `dst` following assignment-style rules: widen with
      147   * sign-extend, narrow by sign-extending the low byte/word bac
           k into

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Now I'll add the emit_struct_addr helper after emit_arrow_addr.

● Update(src/codegen.c)
  ⎿  Added 65 lines
      1178  }
      1179  
      1180  /*
      1181 + * Stage 91-01: leave the address of a struct/union-valued ex
           +pression in rax and                                          
      1182 + * return its Type. Handles the forms that can appear as a by
           +-value struct                                                
      1183 + * argument, return value, or copy-assignment source: a struc
           +t variable, a                                                
      1184 + * struct member/element, a dereferenced pointer-to-struct, a
           +nd a struct-                                                 
      1185 + * returning function call (which codegen_expression material
           +izes into scratch,                                           
      1186 + * leaving the temp's address in rax).                       
      1187 + */                                                          
      1188 +static Type *emit_struct_addr(CodeGen *cg, ASTNode *node) {  
      1189 +    switch (node->type) {                                    
      1190 +    case AST_VAR_REF: {                                      
      1191 +        LocalVar *lv = codegen_find_var(cg, node->value);    
      1192 +        if (lv) {                                            
      1193 +            if (!is_struct_or_union_kind(lv->kind) || !lv->fu
           +ll_type)                                                     
      1194 +                compile_error( "error: '%s' is not a struct/u
           +nion value\n", node->value);                                 
      1195 +            if (lv->is_static)                               
      1196 +                fprintf(cg->output, "    lea rax, [rel %s]\n"
           +, lv->static_label);                                         
      1197 +            else                                             
      1198 +                fprintf(cg->output, "    lea rax, [rbp - %d]\
           +n", lv->offset);                                             
      1199 +            return lv->full_type;                            
      1200 +        }                                                    
      1201 +        GlobalVar *gv = codegen_find_global(cg, node->value);
      1202 +        if (!gv || !is_struct_or_union_kind(gv->kind) || !gv-
           +>full_type)                                                  
      1203 +            compile_error( "error: '%s' is not a struct/union
           + value\n", node->value);                                     
      1204 +        fprintf(cg->output, "    lea rax, [rel %s]\n", gv->na
           +me);                                                         
      1205 +        return gv->full_type;                                
      1206 +    }                                                        
      1207 +    case AST_MEMBER_ACCESS: {                                
      1208 +        StructField *f = emit_member_addr(cg, node);         
      1209 +        if (!is_struct_or_union_kind(f->kind) || !f->full_typ
           +e)                                                           
      1210 +            compile_error( "error: member '%s' is not a struc
           +t/union value\n", node->value);                              
      1211 +        return f->full_type;                                 
      1212 +    }                                                        
      1213 +    case AST_ARROW_ACCESS: {                                 
      1214 +        StructField *f = emit_arrow_addr(cg, node);          
      1215 +        if (!is_struct_or_union_kind(f->kind) || !f->full_typ
           +e)                                                           
      1216 +            compile_error( "error: member '%s' is not a struc
           +t/union value\n", node->value);                              
      1217 +        return f->full_type;                                 
      1218 +    }                                                        
      1219 +    case AST_ARRAY_INDEX: {                                  
      1220 +        Type *element = emit_array_index_addr(cg, node);     
      1221 +        if (!element || !is_struct_or_union_kind(element->kin
           +d))                                                          
      1222 +            compile_error( "error: subscripted value is not a
           + struct/union\n");                                           
      1223 +        return element;                                      
      1224 +    }                                                        
      1225 +    case AST_DEREF: {                                        
      1226 +        codegen_expression(cg, node->children[0]);           
      1227 +        Type *pt = node->children[0]->full_type;             
      1228 +        if (!pt || pt->kind != TYPE_POINTER || !pt->base ||  
      1229 +            !is_struct_or_union_kind(pt->base->kind))        
      1230 +            compile_error( "error: dereferenced value is not 
           +a pointer to struct/union\n");                               
      1231 +        return pt->base;                                     
      1232 +    }                                                        
      1233 +    case AST_FUNCTION_CALL: {                                
      1234 +        codegen_expression(cg, node);                        
      1235 +        if (!node->full_type || !is_struct_or_union_kind(node
           +->decl_type))                                                
      1236 +            compile_error( "error: call does not return a str
           +uct/union value\n");                                         
      1237 +        return node->full_type;                              
      1238 +    }                                                        
      1239 +    default:                                                 
      1240 +        compile_error( "error: expression is not a usable str
           +uct/union value\n");                                         
      1241 +    }                                                        
      1242 +    return NULL;                                             
      1243 +}                                                            
      1244 +                                                             
      1245 +/*                                                           
      1246   * Conservative upper bound on stack bytes needed for locals:
             8 bytes
      1247   * per scalar/pointer declaration, and the array's actual byt
            e count
      1248   * plus 7 bytes of alignment slack per array declaration. The

● Now the central piece — rewriting AST_FUNCTION_CALL. Let me re-read it at its
  current location.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me check codegen_find_function_decl to ensure it returns a decl with the
  struct full_type populated.

  Read 1 file (ctrl+o to expand)

● Now let me add the scratch helpers near compute_decl_bytes.

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 33 lines
      1269  }
      1270  
      1271  /*
      1272 + * Stage 91-01: bytes of frame scratch needed to materialize 
           +the results of                                               
      1273 + * struct-returning function calls in a body. Each such call 
           +gets its own slot                                            
      1274 + * (the simple bump allocator below never frees), sized to th
           +e rounded struct                                             
      1275 + * size plus alignment slack. Register-class returns need a s
           +lot to spill                                                 
      1276 + * rax:rdx into; memory-class returns use the slot directly a
           +s the sret buffer.                                           
      1277 + */                                                          
      1278 +static int compute_struct_ret_bytes(CodeGen *cg, ASTNode *nod
           +e) {                                                         
      1279 +    if (!node) return 0;                                     
      1280 +    int total = 0;                                           
      1281 +    if (node->type == AST_FUNCTION_CALL) {                   
      1282 +        ASTNode *callee = codegen_find_function_decl(cg, node
           +->value);                                                    
      1283 +        if (callee && is_struct_or_union_kind(callee->decl_ty
           +pe) && callee->full_type) {                                  
      1284 +            int sz = callee->full_type->size;                
      1285 +            total += ((sz + 7) & ~7) + 8;                    
      1286 +        }                                                    
      1287 +    }                                                        
      1288 +    for (int i = 0; i < node->child_count; i++)              
      1289 +        total += compute_struct_ret_bytes(cg, node->children[
           +i]);                                                         
      1290 +    return total;                                            
      1291 +}                                                            
      1292 +                                                             
      1293 +/*                                                           
      1294 + * Claim a struct-return scratch slot of `size` bytes and ret
           +urn its rbp                                                  
      1295 + * offset (the slot occupies [rbp-offset .. rbp-offset+size-1
           +], matching the                                              
      1296 + * struct local layout convention). The region was reserved i
           +n the prologue.                                              
      1297 + */                                                          
      1298 +static int claim_struct_ret_temp(CodeGen *cg, int size) {    
      1299 +    int rs = (size + 7) & ~7;                                
      1300 +    cg->struct_ret_scratch_cursor += rs;                     
      1301 +    return cg->struct_ret_scratch_cursor;                    
      1302 +}                                                            
      1303 +                                                             
      1304 +/*                                                           
      1305   * Integer promotion: char/short are promoted to int for use 
            in
      1306   * arithmetic. int and long stay as-is. Stage 11-03: signed o
            nly.
      1307   */

● Now the main rewrite of the call handler. I'll keep the existing scalar path
  verbatim (to preserve print_asm output) and add a struct-aware path.

● Update(src/codegen.c)
  ⎿  Added 126 lines
      2785      } \
      2786  } while (0)
      2787  
      2788 +        /* Stage 91-01: does this call pass a struct by value
           +, or return one?                                             
      2789 +         * Only then take the general SysV marshalling path b
           +elow; otherwise                                              
      2790 +         * keep the existing scalar push/pop path byte-for-by
           +te. */                                                       
      2791 +        int involves_struct = 0;                             
      2792 +        if (callee) {                                        
      2793 +            if (is_struct_or_union_kind(callee->decl_type))  
      2794 +                involves_struct = 1;                         
      2795 +            int np_chk = count_params(callee);               
      2796 +            for (int i = 0; i < np_chk && i < nargs; i++)    
      2797 +                if (is_struct_or_union_kind(callee->children[
           +i]->decl_type))                                              
      2798 +                    involves_struct = 1;                     
      2799 +        }                                                    
      2800 +        if (involves_struct) {                               
      2801 +            CallLayout L;                                    
      2802 +            compute_call_layout(&L, callee, nargs);          
      2803 +                                                             
      2804 +            /* Allocate a scratch slot for a struct return re
           +sult (any class). */                                         
      2805 +            int ret_struct = is_struct_or_union_kind(callee->
           +decl_type);                                                  
      2806 +            int ret_size   = ret_struct ? callee->full_type->
           +size : 0;                                                    
      2807 +            int ret_temp   = ret_struct ? claim_struct_ret_te
           +mp(cg, ret_size) : 0;                                        
      2808 +                                                             
      2809 +            /* Outgoing stack-argument area, padded so rsp is
           + 16-byte aligned                                             
      2810 +             * at the call given the currently outstanding pu
           +shes. */                                                     
      2811 +            int region = (L.stack_bytes + 7) & ~7;           
      2812 +            if (((cg->push_depth * 8 + region) % 16) != 0)   
      2813 +                region += 8;                                 
      2814 +            if (region > 0) {                                
      2815 +                fprintf(cg->output, "    sub rsp, %d\n", regi
           +on);                                                         
      2816 +                cg->push_depth += region / 8;                
      2817 +            }                                                
      2818 +                                                             
      2819 +            /* Phase 1: evaluate memory-passed (stack) argume
           +nts into the area. */                                        
      2820 +            for (int i = 0; i < nargs; i++) {                
      2821 +                ArgSlot *s = &L.items[i];                    
      2822 +                if (!s->mem) continue;                       
      2823 +                if (s->is_struct) {                          
      2824 +                    Type *st = emit_struct_addr(cg, node->chi
           +ldren[i]);                                                   
      2825 +                    int sz = st ? st->size : s->nbytes;      
      2826 +                    fprintf(cg->output, "    mov rsi, rax\n")
           +;                                                            
      2827 +                    fprintf(cg->output, "    lea rdi, [rsp + 
           +%d]\n", s->stack_off);                                       
      2828 +                    emit_struct_copy(cg, sz);                
      2829 +                } else {                                     
      2830 +                    codegen_expression(cg, node->children[i])
           +;                                                            
      2831 +                    EMIT_ARG_CONVERT(node, callee, i);       
      2832 +                    fprintf(cg->output, "    mov [rsp + %d], 
           +rax\n", s->stack_off);                                       
      2833 +                }                                            
      2834 +            }                                                
      2835 +                                                             
      2836 +            /* Phase 2: evaluate register-passed arguments, s
           +pilling each                                                 
      2837 +             * eightbyte to the stack; the hidden sret pointe
           +r (if any) is the                                            
      2838 +             * first spill and maps to rdi. */               
      2839 +            int spill_regs[8];   /* gp register index per spi
           +lled eightbyte */                                            
      2840 +            int nspill = 0;                                  
      2841 +            if (L.sret) {                                    
      2842 +                fprintf(cg->output, "    lea rax, [rbp - %d]\
           +n", ret_temp);                                               
      2843 +                fprintf(cg->output, "    push rax\n");       
      2844 +                cg->push_depth++;                            
      2845 +                spill_regs[nspill++] = 0;                    
      2846 +            }                                                
      2847 +            for (int i = 0; i < nargs; i++) {                
      2848 +                ArgSlot *s = &L.items[i];                    
      2849 +                if (s->mem) continue;                        
      2850 +                if (s->is_struct) {                          
      2851 +                    emit_struct_addr(cg, node->children[i]); 
      2852 +                    fprintf(cg->output, "    mov r11, rax\n")
           +;                                                            
      2853 +                    fprintf(cg->output, "    mov rax, [r11]\n
           +");                                                          
      2854 +                    fprintf(cg->output, "    push rax\n");   
      2855 +                    cg->push_depth++;                        
      2856 +                    spill_regs[nspill++] = s->gp_start;      
      2857 +                    if (s->gp_count == 2) {                  
      2858 +                        fprintf(cg->output, "    mov rax, [r1
           +1 + 8]\n");                                                  
      2859 +                        fprintf(cg->output, "    push rax\n")
           +;                                                            
      2860 +                        cg->push_depth++;                    
      2861 +                        spill_regs[nspill++] = s->gp_start + 
           +1;                                                           
      2862 +                    }                                        
      2863 +                } else {                                     
      2864 +                    codegen_expression(cg, node->children[i])
           +;                                                            
      2865 +                    EMIT_ARG_CONVERT(node, callee, i);       
      2866 +                    fprintf(cg->output, "    push rax\n");   
      2867 +                    cg->push_depth++;                        
      2868 +                    spill_regs[nspill++] = s->gp_start;      
      2869 +                }                                            
      2870 +            }                                                
      2871 +            /* Load the spilled eightbytes into their argumen
           +t registers. The                                             
      2872 +             * j-th of nspill pushes sits at [rsp + (nspill-1
           +-j)*8]. */                                                   
      2873 +            for (int j = 0; j < nspill; j++) {               
      2874 +                fprintf(cg->output, "    mov %s, [rsp + %d]\n
           +",                                                           
      2875 +                        arg_regs[spill_regs[j]], (nspill - 1 
           +- j) * 8);                                                   
      2876 +            }                                                
      2877 +            if (nspill > 0) {                                
      2878 +                fprintf(cg->output, "    add rsp, %d\n", nspi
           +ll * 8);                                                     
      2879 +                cg->push_depth -= nspill;                    
      2880 +            }                                                
      2881 +            if (callee->is_variadic)                         
      2882 +                fprintf(cg->output, "    xor eax, eax\n");   
      2883 +            fprintf(cg->output, "    call %s\n", node->value)
           +;                                                            
      2884 +            if (region > 0) {                                
      2885 +                fprintf(cg->output, "    add rsp, %d\n", regi
           +on);                                                         
      2886 +                cg->push_depth -= region / 8;                
      2887 +            }                                                
      2888 +                                                             
      2889 +            /* Result handling. */                           
      2890 +            if (ret_struct) {                                
      2891 +                if (ret_size > 16) {                         
      2892 +                    /* Memory class: result already in the sr
           +et buffer. */                                                
      2893 +                    fprintf(cg->output, "    lea rax, [rbp - 
           +%d]\n", ret_temp);                                           
      2894 +                } else {                                     
      2895 +                    /* Register class: spill rax:rdx into the
           + temp, then point                                            
      2896 +                     * rax at it so consumers treat it as a s
           +truct lvalue. */                                             
      2897 +                    fprintf(cg->output, "    mov [rbp - %d], 
           +rax\n", ret_temp);                                           
      2898 +                    if (ret_size > 8)                        
      2899 +                        fprintf(cg->output, "    mov [rbp - %
           +d], rdx\n", ret_temp - 8);                                   
      2900 +                    fprintf(cg->output, "    lea rax, [rbp - 
           +%d]\n", ret_temp);                                           
      2901 +                }                                            
      2902 +                node->result_type = callee->decl_type;       
      2903 +                node->decl_type = callee->decl_type;         
      2904 +                node->full_type = callee->full_type;         
      2905 +            } else {                                         
      2906 +                node->result_type = node->decl_type;         
      2907 +                if (callee->decl_type == TYPE_POINTER)       
      2908 +                    node->full_type = callee->full_type;     
      2909 +            }                                                
      2910 +#undef EMIT_ARG_CONVERT                                      
      2911 +            return;                                          
      2912 +        }                                                    
      2913 +                                                             
      2914          if (nargs <= 6) {
      2915              /* Evaluate all args left-to-right, push, then po
            p into regs. */
      2916              for (int i = 0; i < nargs; i++) {

● I introduced a misplaced #undef (preprocessor directives are textual, not
  control-flow). Let me remove it — the original #undef at the end still covers
  it.

● Update(src/codegen.c)
  ⎿  Removed 1 line
      2907                  if (callee->decl_type == TYPE_POINTER)
      2908                      node->full_type = callee->full_type;
      2909              }
      2910 -#undef EMIT_ARG_CONVERT                                      
      2910              return;
      2911          }
      2912  

● Now the prologue. Let me find its current location and read it.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● The prologue needs the most care. First, update the stack-size computation to
  account for struct params, return scratch, and the sret slot.

● Update(src/codegen.c)
  ⎿  Added 32 lines, removed 7 lines
      4389          cg->user_label_count = 0;
      4390          cg->current_func = node->value;
      4391          cg->current_return_type = node->decl_type;
      4392 +        /* Stage 91-01: a struct/union return (either class) 
           +records its full                                             
      4393 +         * type so the return statement knows the object's si
           +ze. */                                                       
      4394          cg->current_return_full_type =
      4393 -            (node->decl_type == TYPE_POINTER) ? node->full_ty
           -pe : NULL;                                                   
      4395 +            (node->decl_type == TYPE_POINTER ||              
      4396 +             is_struct_or_union_kind(node->decl_type)) ? node
           +->full_type : NULL;                                          
      4397 +        cg->current_sret_offset = 0;                         
      4398  
      4399          /* Pre-walk the body to collect user labels; rejects 
            duplicates. */
      4400          collect_user_labels(cg, body);
      4401  
      4398 -        /* Compute stack space: 8 bytes per parameter (conser
           -vative                                                       
      4399 -         * upper bound covering long plus worst-case alignmen
           -t) plus a                                                    
      4400 -         * conservative 8-byte upper bound per body declarati
           -on.                                                          
      4402 +        /* Stage 91-01: does the prologue need struct-by-valu
           +e binding (a struct                                          
      4403 +         * value parameter, or a hidden sret pointer for a me
           +mory-class return)? */                                       
      4404 +        int prologue_struct = return_is_memory_struct(node); 
      4405 +        for (int i = 0; i < num_params; i++)                 
      4406 +            if (is_struct_or_union_kind(node->children[i]->de
           +cl_type))                                                    
      4407 +                prologue_struct = 1;                         
      4408 +                                                             
      4409 +        /* Compute stack space. Stage 91-01: a struct value p
           +arameter reserves                                            
      4410 +         * its rounded size (register-class) — memory-class s
           +truct params are                                             
      4411 +         * referenced in place and cost nothing; a memory-cla
           +ss return reserves                                           
      4412 +         * 8 bytes for the saved hidden pointer; struct-retur
           +ning calls in the                                            
      4413 +         * body reserve a scratch region for their result tem
           +poraries.                                                    
      4414           * Stage 75-04: variadic functions also reserve 304 b
            ytes for the
      4402 -         * hidden GP/FP register save area.                  
      4403 -         * Rounded up to a 16-byte multiple. */              
      4404 -        int stack_size = num_params * 8 + compute_decl_bytes(
           -body);                                                       
      4415 +         * hidden GP/FP register save area. Rounded up to a 1
           +6-byte multiple. */                                          
      4416 +        int param_bytes = 0;                                 
      4417 +        for (int i = 0; i < num_params; i++) {               
      4418 +            ASTNode *p = node->children[i];                  
      4419 +            if (is_struct_or_union_kind(p->decl_type)) {     
      4420 +                if (struct_reg_eightbytes(p->full_type) > 0) 
      4421 +                    param_bytes += (p->full_type->size + 7) &
           + ~7;                                                         
      4422 +            } else {                                         
      4423 +                param_bytes += 8;                            
      4424 +            }                                                
      4425 +        }                                                    
      4426 +        int sret_bytes = return_is_memory_struct(node) ? 8 : 
           +0;                                                           
      4427 +        int scratch_bytes = compute_struct_ret_bytes(cg, body
           +);                                                           
      4428 +        int stack_size = param_bytes + compute_decl_bytes(bod
           +y) +                                                         
      4429 +                         scratch_bytes + sret_bytes;         
      4430          if (node->is_variadic)
      4431              stack_size += 304;
      4432          if (stack_size % 16 != 0)

● Now insert scratch/sret reservation after the variadic block and rewrite param
  binding. Let me re-read the current region.
  Read 1 file (ctrl+o to expand)

● I'll switch memory-class struct params to the copy-into-slot approach (uniform
  positive offsets, avoids negative-offset NASM quirks). First, update
  param_bytes to reserve space for all struct value params.

● Update(src/codegen.c)
  ⎿  Added 2 lines, removed 3 lines
      4416          int param_bytes = 0;
      4417          for (int i = 0; i < num_params; i++) {
      4418              ASTNode *p = node->children[i];
      4419 -            if (is_struct_or_union_kind(p->decl_type)) {     
      4420 -                if (struct_reg_eightbytes(p->full_type) > 0) 
      4421 -                    param_bytes += (p->full_type->size + 7) &
           - ~7;                                                         
      4419 +            if (is_struct_or_union_kind(p->decl_type) && p->f
           +ull_type) {                                                  
      4420 +                param_bytes += (p->full_type->size + 7) & ~7;
      4421              } else {
      4422                  param_bytes += 8;
      4423              }

● Now insert scratch/sret reservation and rewrite the parameter binding with the
  struct-aware path.

● Update(src/codegen.c)
  ⎿  Added 98 lines
      4472              cg->variadic_named_stack_params = 0;
      4473          }
      4474  
      4475 +        /* Stage 91-01: reserve the struct-return scratch reg
           +ion (above the                                               
      4476 +         * parameter/local area) and, for a memory-class retu
           +rn, an 8-byte slot                                           
      4477 +         * holding the hidden return pointer (rdi at entry). 
           +*/                                                           
      4478 +        cg->struct_ret_scratch_cursor = cg->stack_offset;    
      4479 +        if (scratch_bytes > 0)                               
      4480 +            cg->stack_offset += scratch_bytes;               
      4481 +        if (return_is_memory_struct(node)) {                 
      4482 +            cg->stack_offset += 8;                           
      4483 +            cg->current_sret_offset = cg->stack_offset;      
      4484 +            fprintf(cg->output, "    mov [rbp - %d], rdi\n", 
           +cg->current_sret_offset);                                    
      4485 +        }                                                    
      4486 +                                                             
      4487          /*
      4488           * Parameters share the outermost body scope (scope_s
            tart stays at 0).
      4489           * A body-level declaration that collides with a para
            meter name is
     ...
      4495           * SysV AMD64 argument register so the full declared 
            width is
      4496           * preserved and nothing above it is touched.
      4497           */
      4498 +        if (!prologue_struct) {                              
      4499          int reg_params = num_params < 6 ? num_params : 6;
      4500          for (int i = 0; i < reg_params; i++) {
      4501              /* Stage 75-01: unnamed params in variadic defini
            tions are ignored. */
     ...
      4550              }
      4551              emit_store_local(cg, offset, sz, sz == 8 ? 1 : 0)
            ;
      4552          }
      4553 +        } else {                                             
      4554 +            /* Stage 91-01: struct-by-value-aware parameter b
           +inding driven by the                                         
      4555 +             * shared SysV layout. Register-passed parameters
           + are bound first                                             
      4556 +             * (plain stores, no register clobbering); stack-
           +passed parameters                                            
      4557 +             * are bound second, since copying them clobbers 
           +rdi/rsi/rcx. */                                              
      4558 +            CallLayout L;                                    
      4559 +            compute_call_layout(&L, node, num_params);       
      4560 +            /* Pass A: register-passed parameters. */        
      4561 +            for (int i = 0; i < num_params; i++) {           
      4562 +                ASTNode *p = node->children[i];              
      4563 +                ArgSlot *s = &L.items[i];                    
      4564 +                if (p->value[0] == '\0' || s->mem) continue; 
      4565 +                if (is_struct_or_union_kind(p->decl_type)) { 
      4566 +                    int slot = (p->full_type->size + 7) & ~7;
      4567 +                    int align = p->full_type->alignment < 8 ?
           + 8 : p->full_type->alignment;                                
      4568 +                    int off = codegen_add_var(cg, p->value, s
           +lot, align,                                                  
      4569 +                                              p->decl_type, p
           +->full_type);                                                
      4570 +                    cg->locals[cg->local_count - 1].is_unsign
           +ed = 0;                                                      
      4571 +                    fprintf(cg->output, "    mov [rbp - %d], 
           +%s\n", off, arg_regs[s->gp_start]);                          
      4572 +                    if (s->gp_count == 2)                    
      4573 +                        fprintf(cg->output, "    mov [rbp - %
           +d], %s\n", off - 8,                                          
      4574 +                                arg_regs[s->gp_start + 1]);  
      4575 +                } else {                                     
      4576 +                    int sz = type_kind_bytes(p->decl_type);  
      4577 +                    int off = codegen_add_var(cg, p->value, s
           +z, sz, p->decl_type, p->full_type);                          
      4578 +                    cg->locals[cg->local_count - 1].is_unsign
           +ed = p->is_unsigned;                                         
      4579 +                    const char *reg;                         
      4580 +                    int gi = s->gp_start;                    
      4581 +                    switch (sz) {                            
      4582 +                    case 1: reg = param_regs_8[gi];  break;  
      4583 +                    case 2: reg = param_regs_16[gi]; break;  
      4584 +                    case 8: reg = param_regs_64[gi]; break;  
      4585 +                    case 4:                                  
      4586 +                    default: reg = param_regs_32[gi]; break; 
      4587 +                    }                                        
      4588 +                    fprintf(cg->output, "    mov [rbp - %d], 
           +%s\n", off, reg);                                            
      4589 +                }                                            
      4590 +            }                                                
      4591 +            /* Pass B: stack-passed parameters (incoming at [
           +rbp+16+stack_off]). */                                       
      4592 +            for (int i = 0; i < num_params; i++) {           
      4593 +                ASTNode *p = node->children[i];              
      4594 +                ArgSlot *s = &L.items[i];                    
      4595 +                if (p->value[0] == '\0' || !s->mem) continue;
      4596 +                int src = 16 + s->stack_off;                 
      4597 +                if (is_struct_or_union_kind(p->decl_type)) { 
      4598 +                    /* Memory-class struct: copy the caller's
           + stack copy into a                                           
      4599 +                     * local slot so the callee owns a privat
           +e, mutable copy. */                                          
      4600 +                    int sz = p->full_type->size;             
      4601 +                    int slot = (sz + 7) & ~7;                
      4602 +                    int align = p->full_type->alignment < 8 ?
           + 8 : p->full_type->alignment;                                
      4603 +                    int off = codegen_add_var(cg, p->value, s
           +lot, align,                                                  
      4604 +                                              p->decl_type, p
           +->full_type);                                                
      4605 +                    cg->locals[cg->local_count - 1].is_unsign
           +ed = 0;                                                      
      4606 +                    fprintf(cg->output, "    lea rsi, [rbp + 
           +%d]\n", src);                                                
      4607 +                    fprintf(cg->output, "    lea rdi, [rbp - 
           +%d]\n", off);                                                
      4608 +                    emit_struct_copy(cg, sz);                
      4609 +                } else {                                     
      4610 +                    int sz = type_kind_bytes(p->decl_type);  
      4611 +                    int off = codegen_add_var(cg, p->value, s
           +z, sz, p->decl_type, p->full_type);                          
      4612 +                    cg->locals[cg->local_count - 1].is_unsign
           +ed = p->is_unsigned;                                         
      4613 +                    switch (sz) {                            
      4614 +                    case 1:                                  
      4615 +                        if (p->is_unsigned)                  
      4616 +                            fprintf(cg->output, "    movzx ea
           +x, byte [rbp + %d]\n", src);                                 
      4617 +                        else                                 
      4618 +                            fprintf(cg->output, "    movsx ea
           +x, byte [rbp + %d]\n", src);                                 
      4619 +                        break;                               
      4620 +                    case 2:                                  
      4621 +                        if (p->is_unsigned)                  
      4622 +                            fprintf(cg->output, "    movzx ea
           +x, word [rbp + %d]\n", src);                                 
      4623 +                        else                                 
      4624 +                            fprintf(cg->output, "    movsx ea
           +x, word [rbp + %d]\n", src);                                 
      4625 +                        break;                               
      4626 +                    case 8:                                  
      4627 +                        fprintf(cg->output, "    mov rax, [rb
           +p + %d]\n", src);                                            
      4628 +                        break;                               
      4629 +                    case 4:                                  
      4630 +                    default:                                 
      4631 +                        fprintf(cg->output, "    mov eax, [rb
           +p + %d]\n", src);                                            
      4632 +                        break;                               
      4633 +                    }                                        
      4634 +                    emit_store_local(cg, off, sz, sz == 8 ? 1
           + : 0);                                                       
      4635 +                }                                            
      4636 +            }                                                
      4637 +        }                                                    
      4638  
      4639          /* Generate body statements directly — the function b
            ody acts as the outermost scope. */
      4640          for (int i = 0; i < body->child_count; i++) {

● Let me ensure is_static is cleared for the new struct-path bindings
  (codegen_add_var leaves it stale), since &param consults it.

● Bash(grep -n "is_static" include/codegen.h | head; echo "---does
      codegen_add_var set is_static?---"; sed -n '394,407p' src/codegen.c)
  ⎿  23:    /* Stage 71: set for block-scope static variables. When is_stat
     ic is
     26:    int is_static;
     … +16 lines (ctrl+o to expand)

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● I'll fix the latent is_static staleness in codegen_add_var itself — safe and
  covers the new struct params.

● Update(src/codegen.c)
  ⎿  Added 4 lines
      512      cg->locals[cg->local_count].kind = kind;
      513      cg->locals[cg->local_count].full_type = full_type;
      514      cg->locals[cg->local_count].is_const = 0;
      515 +    /* Stage 91-01: clear is_static so a slot reused from a pr
          +ior function's                                                
      516 +     * block-scope static local is not misread as static (&var
          + / member access                                              
      517 +     * consult this flag). Static locals are registered via a 
          +separate path. */                                             
      518 +    cg->locals[cg->local_count].is_static = 0;                
      519      cg->local_count++;
      520      return cg->stack_offset;
      521  }

● Now the return statement. Let me read its current form.

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 29 lines
      4107                  compile_error(
      4108                          "error: cannot use void function resu
            lt as return value\n");
      4109              }
      4110 +            /* Stage 91-01: returning a struct/union by value
           +. Get the source                                             
      4111 +             * struct's address, then either load its eightby
           +te(s) into rax:rdx                                           
      4112 +             * (register class, <=16 bytes) or copy it into t
           +he hidden return                                             
      4113 +             * buffer and leave the buffer pointer in rax (me
           +mory class). */                                              
      4114 +            if (is_struct_or_union_kind(cg->current_return_ty
           +pe)) {                                                       
      4115 +                Type *st = emit_struct_addr(cg, node->childre
           +n[0]);                                                       
      4116 +                int sz = cg->current_return_full_type        
      4117 +                             ? cg->current_return_full_type->
           +size                                                         
      4118 +                             : (st ? st->size : 0);          
      4119 +                if (sz > 16) {                               
      4120 +                    fprintf(cg->output, "    mov rsi, rax\n")
           +;                                                            
      4121 +                    fprintf(cg->output, "    mov rdi, [rbp - 
           +%d]\n",                                                      
      4122 +                            cg->current_sret_offset);        
      4123 +                    emit_struct_copy(cg, sz);                
      4124 +                    fprintf(cg->output, "    mov rax, [rbp - 
           +%d]\n",                                                      
      4125 +                            cg->current_sret_offset);        
      4126 +                } else {                                     
      4127 +                    fprintf(cg->output, "    mov r11, rax\n")
           +;                                                            
      4128 +                    fprintf(cg->output, "    mov rax, [r11]\n
           +");                                                          
      4129 +                    if (sz > 8)                              
      4130 +                        fprintf(cg->output, "    mov rdx, [r1
           +1 + 8]\n");                                                  
      4131 +                }                                            
      4132 +                if (cg->has_frame) {                         
      4133 +                    fprintf(cg->output, "    mov rsp, rbp\n")
           +;                                                            
      4134 +                    fprintf(cg->output, "    pop rbp\n");    
      4135 +                }                                            
      4136 +                fprintf(cg->output, "    ret\n");            
      4137 +                return;                                      
      4138 +            }                                                
      4139              /* Stage 12-06: a return of the literal `0` from 
            a pointer
      4140               * function is a null pointer constant; accept it
             before the
      4141               * integer-vs-pointer match enforced below. `mov 
            eax, 0`

● Now extend struct assignment and declaration-init to accept a struct-returning
  call (general struct rvalue) as source, while keeping the existing VAR_REF
  byte-copy path verbatim. Let me locate them.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 30 lines, removed 15 lines
      2158              }
      2159              /* Stage 33/72: struct/union-to-struct/union assi
            gnment — byte copy. */
      2160              if ((lv->kind == TYPE_STRUCT || lv->kind == TYPE_
            UNION) && lv->full_type) {
      2161 -                if (node->child_count < 1 || node->children[0
           -]->type != AST_VAR_REF) {                                    
      2162 -                    compile_error( "error: %s assignment requ
           -ires a %s variable\n",                                       
      2161 +                if (node->child_count < 1) {                 
      2162 +                    compile_error( "error: %s assignment requ
           +ires a %s value\n",                                          
      2163                              type_kind_name(lv->kind), type_ki
            nd_name(lv->kind));
      2164                  }
      2165 -                LocalVar *src = codegen_find_var(cg, node->ch
           -ildren[0]->value);                                           
      2166 -                if (!src || src->kind != lv->kind || !src->fu
           -ll_type) {                                                   
      2167 -                    compile_error( "error: cannot assign non-
           -%s to %s '%s'\n",                                            
      2168 -                            type_kind_name(lv->kind), type_ki
           -nd_name(lv->kind), lv->name);                                
      2165 +                if (node->children[0]->type == AST_VAR_REF &&
      2166 +                    codegen_find_var(cg, node->children[0]->v
           +alue)) {                                                     
      2167 +                    LocalVar *src = codegen_find_var(cg, node
           +->children[0]->value);                                       
      2168 +                    if (src->kind != lv->kind || !src->full_t
           +ype) {                                                       
      2169 +                        compile_error( "error: cannot assign 
           +non-%s to %s '%s'\n",                                        
      2170 +                                type_kind_name(lv->kind), typ
           +e_kind_name(lv->kind), lv->name);                            
      2171 +                    }                                        
      2172 +                    if (src->full_type != lv->full_type) {   
      2173 +                        compile_error( "error: incompatible %
           +s types in assignment to '%s'\n",                            
      2174 +                                type_kind_name(lv->kind), lv-
           +>name);                                                      
      2175 +                    }                                        
      2176 +                    int sz = lv->full_type->size;            
      2177 +                    for (int b = 0; b < sz; b++) {           
      2178 +                        fprintf(cg->output, "    movzx eax, b
           +yte [rbp - %d]\n", src->offset - b);                         
      2179 +                        fprintf(cg->output, "    mov [rbp - %
           +d], al\n", lv->offset - b);                                  
      2180 +                    }                                        
      2181 +                } else {                                     
      2182 +                    /* Stage 91-01: source is a struct rvalue
           + (function call,                                             
      2183 +                     * member, dereference, global, ...). Mat
           +erialize its                                                 
      2184 +                     * address and block-copy into the destin
           +ation slot. */                                               
      2185 +                    Type *st = emit_struct_addr(cg, node->chi
           +ldren[0]);                                                   
      2186 +                    if (!st || st != lv->full_type) {        
      2187 +                        compile_error( "error: incompatible %
           +s types in assignment to '%s'\n",                            
      2188 +                                type_kind_name(lv->kind), lv-
           +>name);                                                      
      2189 +                    }                                        
      2190 +                    fprintf(cg->output, "    mov rsi, rax\n")
           +;                                                            
      2191 +                    fprintf(cg->output, "    lea rdi, [rbp - 
           +%d]\n", lv->offset);                                         
      2192 +                    emit_struct_copy(cg, lv->full_type->size)
           +;                                                            
      2193                  }
      2170 -                if (src->full_type != lv->full_type) {       
      2171 -                    compile_error( "error: incompatible %s ty
           -pes in assignment to '%s'\n",                                
      2172 -                            type_kind_name(lv->kind), lv->nam
           -e);                                                          
      2173 -                }                                            
      2174 -                int sz = lv->full_type->size;                
      2175 -                for (int b = 0; b < sz; b++) {               
      2176 -                    fprintf(cg->output, "    movzx eax, byte 
           -[rbp - %d]\n", src->offset - b);                             
      2177 -                    fprintf(cg->output, "    mov [rbp - %d], 
           -al\n", lv->offset - b);                                      
      2178 -                }                                            
      2194                  node->result_type = lv->kind;
      2195                  node->full_type = lv->full_type;
      2196                  return;

● Now the struct declaration-init path. Let me read it.

  Read 1 file (ctrl+o to expand)

● Update(src/codegen.c)
  ⎿  Added 28 lines, removed 19 lines
      3948                                             node->children[0])
            ;
      3949                  }
      3950              } else if (node->child_count > 0) {
      3951 -                /* struct/union T d = c — copy from another v
           -ariable of the same type. */                                 
      3951 +                /* struct/union T d = c — copy from another v
           +alue of the same type. */                                    
      3952                  ASTNode *init = node->children[0];
      3953 -                if (init->type != AST_VAR_REF) {             
      3954 -                    compile_error( "error: %s initializer mus
           -t be a %s variable\n",                                       
      3955 -                            type_kind_name(node->decl_type), 
      3956 -                            type_kind_name(node->decl_type));
      3953 +                if (init->type == AST_VAR_REF &&             
      3954 +                    codegen_find_var(cg, init->value)) {     
      3955 +                    LocalVar *src = codegen_find_var(cg, init
           +->value);                                                    
      3956 +                    if (src->kind != node->decl_type || !src-
           +>full_type) {                                                
      3957 +                        compile_error( "error: %s initializer
           + must be a %s variable\n",                                   
      3958 +                                type_kind_name(node->decl_typ
           +e),                                                          
      3959 +                                type_kind_name(node->decl_typ
           +e));                                                         
      3960 +                    }                                        
      3961 +                    if (src->full_type != node->full_type) { 
      3962 +                        compile_error( "error: incompatible %
           +s types in initializer for '%s'\n",                          
      3963 +                                type_kind_name(node->decl_typ
           +e), node->value);                                            
      3964 +                    }                                        
      3965 +                    for (int b = 0; b < size; b++) {         
      3966 +                        fprintf(cg->output, "    movzx eax, b
           +yte [rbp - %d]\n", src->offset - b);                         
      3967 +                        fprintf(cg->output, "    mov [rbp - %
           +d], al\n", offset - b);                                      
      3968 +                    }                                        
      3969 +                } else {                                     
      3970 +                    /* Stage 91-01: initialize from a struct 
           +rvalue (function                                             
      3971 +                     * call, member, dereference, global, ...
           +). */                                                        
      3972 +                    Type *st = emit_struct_addr(cg, init);   
      3973 +                    if (!st || st != node->full_type) {      
      3974 +                        compile_error( "error: incompatible %
           +s types in initializer for '%s'\n",                          
      3975 +                                type_kind_name(node->decl_typ
           +e), node->value);                                            
      3976 +                    }                                        
      3977 +                    fprintf(cg->output, "    mov rsi, rax\n")
           +;                                                            
      3978 +                    fprintf(cg->output, "    lea rdi, [rbp - 
           +%d]\n", offset);                                             
      3979 +                    emit_struct_copy(cg, size);              
      3980                  }
      3958 -                LocalVar *src = codegen_find_var(cg, init->va
           -lue);                                                        
      3959 -                if (!src || src->kind != node->decl_type || !
           -src->full_type) {                                            
      3960 -                    compile_error( "error: %s initializer mus
           -t be a %s variable\n",                                       
      3961 -                            type_kind_name(node->decl_type), 
      3962 -                            type_kind_name(node->decl_type));
      3963 -                }                                            
      3964 -                if (src->full_type != node->full_type) {     
      3965 -                    compile_error( "error: incompatible %s ty
           -pes in initializer for '%s'\n",                              
      3966 -                            type_kind_name(node->decl_type), 
           -node->value);                                                
      3967 -                }                                            
      3968 -                for (int b = 0; b < size; b++) {             
      3969 -                    fprintf(cg->output, "    movzx eax, byte 
           -[rbp - %d]\n", src->offset - b);                             
      3970 -                    fprintf(cg->output, "    mov [rbp - %d], 
           -al\n", offset - b);                                          
      3971 -                }                                            
      3981              }
      3982              return;
      3983          }

● Now update version.c, then build.

● Update(src/version.c)  ⎿  File must be read first

  Read 1 file (ctrl+o to expand)

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      3  
      4  #define VERSION_MAJOR  "00"
      5  #define VERSION_MINOR  "01"
      6 -#define VERSION_STAGE  "00920000"                               
      6 +#define VERSION_STAGE  "00910100"                               
      7  
      8  #ifndef VERSION_BUILD
      9  #define VERSION_BUILD  0

● Now let me build and fix any compile errors.

● Bash(cmake --build build 2>&1 | tail -30)
  ⎿  [  9%] Building C object CMakeFiles/ccompiler.dir/src/compiler.c.o
     /home/scott/code/claude/claudec99/src/compiler.c: In function ‘token_t
     ype_name’:
     … +9 lines (ctrl+o to expand)

● Clean build. Now let me test the spec examples directly before adding them to
  the suite.

● Bash(cd /tmp && cat > sbv1.c <<'EOF'
      #include <stdio.h>…)
  ⎿  sbv1.c:9:34: error: call to undefined function 'strlen'
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● The spec test uses strlen but only includes <stdio.h>; this compiler requires
  the declaration (strlen lives in <string.h>). I'll note this minor spec
  inconsistency and add #include <string.h> to make the test compile (intent
  preserved).

● Bash(cd /tmp && cat > sbv1.c <<'EOF'
      #include <stdio.h>…)
  ⎿  compiled: sbv1.c -> sbv1.asm
     exit=5
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Test 1 passes (exit 5). Let me check the generated assembly for the
  register-class path, then test 2.

● Bash(cd /tmp && cat > sbv2.c <<'EOF'
      #include <stdio.h>…)
  ⎿  compiled: sbv2.c -> sbv2.asm
     exit=1
     === finalize asm ===
     … +26 lines (ctrl+o to expand)
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Both spec tests pass. Now the critical memory-class (>16 byte) path — the one
  self-compilation needs. Let me test it thoroughly.

● Bash(cd /tmp && cat > big.c <<'EOF'
      #include <stdio.h>…)
  ⎿  compiled: big.c -> big.asm
     exit=42 (expect 42)
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● Excellent — memory-class works. Let me stress-test the real-world shape: a
  Token-like struct with a char array, plus a pointer-param + struct-return
  function (the lexer pattern), and a nested call.

● Bash(cd /tmp && cat > tok.c <<'EOF'
      #include <stdio.h>…)
  ⎿  compiled: tok.c -> tok.asm
     exit=55 (expect 55)
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● All green. A couple more edge cases (register exhaustion forcing a struct to
  the stack; small ≤8-byte struct), then the full suite.

● Bash(cd /tmp && cat > edge.c <<'EOF'
      #include <stdio.h>…)
  ⎿  compiled: edge.c -> edge.asm
     exit=77 (expect 77)
  ⎿  Shell cwd was reset to /home/scott/code/claude/claudec99

● All edge cases pass, including register-exhaustion spilling a register-class
  struct to the stack. Now the full test suite.

● Bash(./test/run_all_tests.sh 2>&1 | tail -25)
  ⎿  PASS  test_stage_14_03_string_literal_one
     PASS  test_stage_14_05_string_literal_escape_n
     PASS  test_stage_14_05_string_literal_escape_null
     … +22 lines (ctrl+o to expand)

● No regressions — all 1302 pass, including print_asm. Now let me add the spec
  tests plus memory-class coverage to the suite.

  Listed 1 directory (ctrl+o to expand)

● Write(test/valid/test_struct_by_value_param_return__5.c)
  ⎿  Wrote 26 lines to test/valid/test_struct_by_value_param_return__5.c
         urned by value. a register-class (16-byte) struct passed and ret
       2  * finalize() receives a copy of the Token, sets its length, and
          returns the
         e: 5. */ied copy; main assigns it back into t. Expected exit cod
       4 #include <stdio.h>
       5 #include <string.h>
       7 typedef struct Token {
       8     char *value;
       9     int length;
      10 } Token;
     … +19 lines (ctrl+o to expand)

● Write(test/valid/test_struct_by_value_proof__1.c)  ⎿  Wrote 27 lines to test/valid/test_struct_by_value_proof__1.c
         y its private1: prove by-value semantics. finalize() mutates onl
       2  * copy, so the caller's original t is unchanged (length still 9
         9) while the
         : 1. */rned copy u has the computed length 5. Expected exit code
       4 #include <stdio.h>
       5 #include <string.h>
       7 typedef struct Token {
       8     char *value;
       9     int length;
      10 } Token;
     … +20 lines (ctrl+o to expand)

● Write(test/valid/test_struct_by_value_memory_class__42.c)
  ⎿  Wrote 34 lines to test/valid/test_struct_by_value_memory_class__42.c
         turned by value a memory-class struct (> 16 bytes) passed and re
       2  * via the SysV hidden-pointer (sret) convention. bump() mutates
          its private
          42. */; the caller's original is unchanged. Expected exit code:
       4 #include <stdio.h>
       5 
       7     int a;ruct Big {
       8     int b;
       9     int c;
      10     int d;
     … +27 lines (ctrl+o to expand)

● Write(test/valid/test_struct_by_value_lexer_pattern__55.c)
  ⎿  Wrote 51 lines to test/valid/test_struct_by_value_lexer_pattern__55.c
         -compilation —: the lexer/parser Token pattern that blocked self
       2  * a large (memory-class) struct with a char-array member, retur
       3  * from a function taking a pointer argument, plus a nested stru
         ct-by-value
       4  * call (relabel(make(...))) and struct decl-initialization from
       5  * Expected exit code: 55. */
       6 #include <stdio.h>
       8 #include <string.h>
       9 typedef struct Token {
      10     int type;
     … +47 lines (ctrl+o to expand)

● Bash(./test/valid/run_tests.sh 2>&1 | grep -iE "struct_by_value|Results:")
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_struct_by_
     value_lexer_pattern__55.c -> test_struct_by_value_lexer_pattern__55.as
     m
     … +11 lines (ctrl+o to expand)

● All 4 new tests pass (valid now 823). Let me run the full suite for the final
  aggregate.

● Bash(./test/run_all_tests.sh 2>&1 | grep -E "Aggregate|passed, [0-9]+ failed"
      | tail -8)
  ⎿  Results: 823 passed, 0 failed, 823 total
     Results: 237 passed, 0 failed, 237 total
     Results: 82 passed, 0 failed, 82 total
     … +4 lines (ctrl+o to expand)

● All 1306 pass. Let me do a quick spot-check that the previously-blocked
  lexer.c now gets past the struct-by-value blocker (evidence for the stage-92
  goal, though full self-compilation is stage 92's scope).

● Bash(./build/ccompiler --sysroot=. -Iinclude -Itest/include src/lexer.c 2>&1 |
      grep -iE "struct|by.value|finalize|'\.'|applied to non-struct" | head;
      echo "--- exit/…)
  ⎿  --- exit/summary ---
     rc=0
     compiled: src/lexer.c -> lexer.asm

● lexer.c now compiles cleanly (rc=0) — the struct-by-value blocker that stalled
  self-compilation is resolved. Let me clean up the stray output file.

● Bash(rm -f lexer.asm && git status --short)
  ⎿   M docs/stages/ClaudeC99-spec-stage-92-self-compile-validation.md
      M include/codegen.h
      M src/codegen.c
     … +7 lines (ctrl+o to expand)

● A stage-92 spec doc shows as modified, which I didn't touch. Let me check what
  changed there.

● Bash(git diff --stat
      docs/stages/ClaudeC99-spec-stage-92-self-compile-validation.md; echo
      "---"; git diff
      docs/stages/ClaudeC99-spec-stage-92-self-compile-validation…)
  ⎿   docs/stages/ClaudeC99-spec-stage-92-self-compile-validation.md | 5 ++
     ++-
      1 file changed, 4 insertions(+), 1 deletion(-)
     … +20 lines (ctrl+o to expand)

● The stage-92 spec change is a user edit unrelated to my work — I'll leave it
  unstaged and commit only the stage-91-01 files.

  Now let me delegate the README, grammar, milestone, and transcript docs to
  haiku with full context.

● haiku-stage-artifact-writer(Write stage 91-01 docs and artifacts)
  ⎿  Done (11 tool uses · 26.8k tokens · 14s)
  (ctrl+o to expand)

● The haiku agent's final message looks truncated. Let me verify what it
  actually produced.

● Bash(ls -la docs/milestones/stage-91-01-milestone.md
      docs/sessions/stage-91-01-transcript.md 2>&1; echo "=== README checks
      ==="; grep -n "823\|1306\|struct by-value\…)
  ⎿  ls: cannot access 'docs/milestones/stage-91-01-milestone.md': No such
     file or directory
     ls: cannot access 'docs/sessions/stage-91-01-transcript.md': No such f
     … +7 lines (ctrl+o to expand)

● The haiku agent stalled mid-task (too many docs). I'll do the
  verification-critical README + grammar edits myself, then re-delegate just the
  milestone + transcript. Let me read the README spots.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Update(README.md)
  ⎿  Added 1 line
      311  - **Functions**: multiple functions per translation unit, forw
           ard
      312    declarations with compatibility checking (return type and pa
           rameter type
      313    matching between declarations and definitions), SysV AMD64 c
           alls with any number of integer, pointer, _Bool, enum, and str
           uct-pointer arguments (args 1–6 in registers per ABI, args 7+ 
           passed on the stack right-to-left with automatic 16-byte align
           ment), typed parameter and return-type conversions at the call
            boundary,
      314 +  struct/union arguments and return values passed **by value**
          + per the SysV AMD64 ABI (register-class objects ≤ 16 bytes use
          + 1–2 INTEGER GP registers for arguments and rax:rdx for return
          +s; memory-class objects > 16 bytes are passed on the stack and
          + returned through a hidden pointer / sret in rdi; the callee r
          +eceives a private copy and the caller's original is unchanged)
          +,                                                             
      315    calls into libc via `extern` emission for declared-but-not-d
           efined functions
      316    with support for void* parameters/returns (e.g., `malloc`, `
           free`), const char*
      317    parameters (e.g., `puts`, `strcmp`, `strlen`), and typedef-b
           ased size_t.

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      478  
      479  ## Not yet supported
      480  
      481 -Anonymous struct/union members (C11 feature), bit-fields, stru
          -ct by-value parameters/return values (pointer-to-struct parame
          -ters are now supported); floating-point; block-scope `extern`;
          - block-scope `static` arrays and structs;                     
      481 +Anonymous struct/union members (C11 feature), bit-fields; floa
          +ting-point; block-scope `extern`; block-scope `static` arrays 
          +and structs;                                                  
      482  floating-point variadic arguments; `va_copy` (va_start/va_end 
           and va_arg extraction for GP types are now implemented);
      483  `#elifdef`/`#elifndef`; pointer-to-function-pointer and functi
           on-returning-function-pointer.
      484  

● Update(README.md)
  ⎿  Added 1 line, removed 1 line
      506  ```
      507  
      508  The runner aggregates per-suite results and prints a final
      509 -`Aggregate: P passed, F failed, T total` line. As of stage 92 
          -all tests pass (819 valid, 237 invalid, 82 integration, 43 pri
          -nt-AST, 100 print-tokens, 21 print-asm; 1302 total).          
      509 +`Aggregate: P passed, F failed, T total` line. As of stage 91-
          +01 all tests pass (823 valid, 237 invalid, 82 integration, 43 
          +print-AST, 100 print-tokens, 21 print-asm; 1306 total).       
      510  
      511  Individual suites can be run directly, e.g. `./test/valid/run_
           tests.sh`.
      512  
● Now let me update the stage-92 narrative block to note the blocker is
  resolved.

  Read 1 file (ctrl+o to expand)

● Update(README.md)
  ⎿  Added 4 lines, removed 3 lines
      206  > Stage 92 began validating self-compilation (using the compil
           er to build
      207  > itself). It fixed a silent AST-child-truncation bug (the `AS
           TNode` child
      208  > array is now dynamically grown) and raised several capacity 
           limits, plus a
      209 -> few stub/source adjustments. Full self-hosting is **not yet 
          -achieved** — it                                               
      210 -> is currently blocked on struct-by-value parameters/returns (
          -used by the                                                   
      211 -> lexer/parser `Token` interface), which remains under "Not ye
          -t supported".                                                 
      209 +> few stub/source adjustments. Full self-hosting is **not yet 
          +achieved**, but                                               
      210 +> the principal remaining blocker — struct-by-value parameters
          +/returns, used by                                             
      211 +> the lexer/parser `Token` interface — was resolved in stage 9
          +1-01, and the                                                 
      212 +> previously-failing `src/lexer.c` now compiles cleanly with t
          +he compiler.                                                  
      213  > See [`docs/self-compilation-report.md`](docs/self-compilatio
           n-report.md).
      214  
      215  Through stage 91 (address-of member lvalues):

● Now verify grammar.md needs no change.

  Searched for 1 pattern (ctrl+o to expand)

● grammar.md productions don't change, but the explanatory notes at lines
  425–427 are now stale. Let me fix them.

  Read 1 file (ctrl+o to expand)

● Update(docs/grammar.md)
  ⎿  Added 5 lines, removed 3 lines
      422  #     structs via typedef aliases and opaque pointer fields.
      423  #   - Non-pointer incomplete struct fields are rejected; only 
           pointer-to-incomplete-struct
      424  #     fields are allowed as struct members.
      425 -#   - Struct-by-value function parameters are not yet supporte
          -d.                                                            
      426 -#     Pointer-to-struct parameters (struct T *p) are supported
          - (stage 34).                                                  
      427 -#   - Struct-by-value return types are not yet supported.     
      425 +#   - Struct-by-value function parameters and return types are
          + supported                                                    
      426 +#     (stage 91-01), passed/returned per the SysV AMD64 ABI: r
          +egister-class                                                 
      427 +#     objects (<= 16 bytes) use GP registers (and rax:rdx for 
          +returns),                                                     
      428 +#     memory-class objects (> 16 bytes) use the stack and a hi
          +dden sret pointer.                                            
      429 +#     Pointer-to-struct parameters (struct T *p) are also supp
          +orted (stage 34).                                             
      430  #   - Anonymous struct/union types are supported (stage 73-01)
           : the tag is optional
      431  #     when a body '{' is present (e.g., `struct { int x; int y
           ; } p;`). Each anonymous
      432  #     definition creates a unique type; type identity is by po
           inter, so two separately

● README and grammar done. Now re-delegate just the two artifact docs to haiku
  (smaller scope).

● haiku-stage-artifact-writer(Write milestone and transcript)
  ⎿  Done (7 tool uses · 14.0k tokens · 35s)
  (ctrl+o to expand)

● Bash(ls docs/milestones/stage-91-01-milestone.md
      docs/sessions/stage-91-01-transcript.md && echo "--- full suite re-run
      after doc edits ---" && ./test/run_all_tests.…)
  ⎿  docs/milestones/stage-91-01-milestone.md
     docs/sessions/stage-91-01-transcript.md
     --- full suite re-run after doc edits ---
     Aggregate: 1306 passed, 0 failed, 1306 total

● All artifacts in place; 1306 tests still pass. Let me review the final diff
  for cleanliness before proposing the commit.

● Bash(git status --short; echo "=== diffstat (tracked) ==="; git diff --stat)
  ⎿   M README.md
      M docs/grammar.md
      M docs/stages/ClaudeC99-spec-stage-92-self-compile-validation.md
     … +20 lines (ctrl+o to expand)

● Everything is complete and verified. The large src/codegen.c diff is expected
  for this ABI feature. I'll stage only the stage-91-01 files (excluding the
  unrelated pre-existing stage-92 spec edit) and commit, following the project's
  per-stage convention.

● Bash(cd /home/scott/code/claude/claudec99
      git add README.md docs/grammar.md include/codegen.h src/codegen.c
      src/parser.c src/version.c \…)
  ⎿  --- committed ---
     0850483 stage 91-01: support struct-by-value parameters and returns
     --- remaining unstaged ---
      M docs/stages/ClaudeC99-spec-stage-92-self-compile-validation.md

● Stage 91-01 is complete and committed. The only remaining change is the
  pre-existing user edit to the stage-92 spec, which I deliberately left
  untouched.

  Summary — Stage 91-01: Struct-by-Value Parameters and Returns

  Scope (your call): full SysV AMD64 — both register-class (≤16 B) and
  memory-class (>16 B / sret) structs.

  What changed
  - Parser (src/parser.c): attach full_type to struct/union value parameters and
  to struct/union return types (both were left NULL).
  - Codegen (src/codegen.c):
    - Shared SysV classification + call-layout helper used by both the call site
  and the prologue, so they agree on register/stack placement.
    - emit_struct_addr (address of any struct rvalue) and emit_struct_copy (rep 
  movsb block copy).
    - Rewrote AST_FUNCTION_CALL with a struct-aware marshalling path — the 
  scalar push/pop path is kept byte-identical, so all 21 print_asm tests still
  pass.
    - Struct-aware prologue: register-class struct stores, memory-class struct
  copied from the incoming stack into a private local, hidden sret pointer saved
  from rdi, return-temp scratch region reserved.
    - Struct returns (rax:rdx vs. sret copy), struct-rvalue sources accepted in
  whole-struct assignment and declaration-init, plus a latent is_static
  staleness fix in codegen_add_var.
  - version.c → 00910100.

  Tests: the 2 spec tests (exit 5, exit 1) + memory-class and lexer-pattern
  coverage. Full suite: 1306 passed, 0 failed (823 valid, 237 invalid, 82
  integration, 43 print-AST, 100 print-tokens, 21 print-asm).

  Artifacts: kickoff, milestone, transcript; README + grammar.md updated.

  Two things to flag:
  1. Spec test inconsistency — both spec tests call strlen but include only
  <stdio.h>; this compiler requires the declaration, so the committed tests add
  #include <string.h> (intent unchanged).
  2. Bonus — src/lexer.c now compiles cleanly with the built compiler, resolving
  the struct-by-value blocker that stalled stage-92 self-compilation (though
  full self-hosting remains stage 92's goal).

✻ Worked for 28m 29s