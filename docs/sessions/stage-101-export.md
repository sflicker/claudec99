 ▐▛███▜▌   Claude Code v2.1.172
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

 ▎ Meet Fable 5, our newest model for complex, long-running work. Try anytime with /model.
 ▎ Included in your plan limits until Jun 22, then switch to usage credits to continue.

❯ /clear                                                                        

❯ /implement-stage                                                              
docs/stages/ClaudeC99-spec-stage-101-block-scope-static-arrays-structs.md       

● I'll start by reading the spec and supporting files to understand what needs
  to be implemented.

  Read 4 files (ctrl+o to expand)

● Now let me read the kickoff template and current codegen files.

  Read 2 files (ctrl+o to expand)

● Good. Now let me look at the relevant codegen sections.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Now let me look at the global array emit code in codegen_emit_data to
  understand the slots-map pattern.

  Read 1 file (ctrl+o to expand)

● Let me also look at emit_global_struct and the union emission code:

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Now let me check version.c:

  Read 1 file (ctrl+o to expand)

● I have all I need. Let me produce the kickoff artifact in parallel while I
  begin on the implementation plan.

  STAGE_DISPLAY: Stage 101

  Spec Summary

  Stage 101 is a codegen-only stage that lifts the guard that rejects
  TYPE_ARRAY, TYPE_STRUCT, and TYPE_UNION for block-scope static locals. No
  tokenizer, parser, or AST changes. Six tasks in src/codegen.c and one in
  include/codegen.h:

  1. Add ASTNode *init_node to LocalStaticVar
  2. Remove the guard; add array/struct/union static registration in the
  SC_STATIC arm
  3. Extend codegen_emit_local_statics for array/struct/union in .data and .bss
  4. Fix array decay in codegen_expression VAR_REF (is_static → lea [rel label])
  5. Fix subscript base in emit_array_index_addr (is_static branch + remove
  stale comment)
  6. Fix member address in emit_member_addr local struct branch (is_static →
  RIP-relative)

  Ambiguities / Issues

  None found. The spec is specific and consistent.

  Planned Changes

  ┌───────────────────┬──────────────────────────────────────────┐
  │       File        │                  Change                  │
  ├───────────────────┼──────────────────────────────────────────┤
  │ include/codegen.h │ Add ASTNode *init_node to LocalStaticVar │
  ├───────────────────┼──────────────────────────────────────────┤
  │ src/codegen.c     │ 6 targeted edits (Tasks 2–6)             │
  ├───────────────────┼──────────────────────────────────────────┤
  │ src/version.c     │ Bump VERSION_STAGE to "01010000"         │
  ├───────────────────┼──────────────────────────────────────────┤
  │ test/valid/       │ 6 new valid tests                        │
  ├───────────────────┼──────────────────────────────────────────┤
  │ test/invalid/     │ 2 new invalid tests                      │
  └───────────────────┴──────────────────────────────────────────┘

  Generating the kickoff artifact now, then ready to begin implementation.

● haiku-stage-artifact-writer(Generate stage-101 kickoff artifact)
  ⎿  Backgrounded agent (↓ to manage · ctrl+o to expand)

● Good. Now let me implement. Starting with Task 1 — adding init_node to
  LocalStaticVar.

● Update(include/codegen.h)
  ⎿  Added 3 lines
      41      int is_initialized;
      42      long init_value;
      43      int is_unsigned;
      44 +    /* Stage 101: array/struct brace-list or string-literal ini
         +tializer node;                                                 
      45 +     * NULL for scalars and for any uninitialized (.bss) entry.
         + */                                                            
      46 +    struct ASTNode *init_node;                                 
      47  } LocalStaticVar;
      48  
      49  /* Stage 22-01: file-scope (global) variable. Accessed via RIP-
          relative

● Now Task 2 — remove the guard and add array/struct static registration. The
  existing scalar block ends at the return; on line ~4335. I'll remove the
  guard, keep the scalar path unchanged, then add array and struct/union
  branches after it.

● Update(src/codegen.c)
  ⎿  Added 87 lines, removed 5 lines
      4276          }
      4277          /* Stage 71: block-scope static variable. */
      4278          if (node->storage_class == SC_STATIC) {
      4279 -            if (node->decl_type == TYPE_ARRAY || node->decl_t
           -ype == TYPE_STRUCT ||                                        
      4280 -                node->decl_type == TYPE_UNION) {             
      4281 -                compile_error(                               
      4282 -                        "error: static local arrays, structs 
           -and unions are not yet supported\n");                        
      4279 +            /* Stage 101: array static. */                   
      4280 +            if (node->decl_type == TYPE_ARRAY && node->full_t
           +ype) {                                                       
      4281 +                ASTNode *init_node = node->child_count > 0 ? 
           +node->children[0] : NULL;                                    
      4282 +                int is_initialized = (init_node != NULL);    
      4283 +                if (init_node != NULL) {                     
      4284 +                    int is_string = (node->full_type->base &&
      4285 +                                     node->full_type->base->k
           +ind == TYPE_CHAR &&                                          
      4286 +                                     init_node->type == AST_S
           +TRING_LITERAL);                                              
      4287 +                    if (init_node->type != AST_INITIALIZER_LI
           +ST && !is_string) {                                          
      4288 +                        compile_error(                       
      4289 +                            "error: initializer for block-sco
           +pe static '%s' "                                             
      4290 +                            "must be a brace-enclosed constan
           +t list\n",                                                   
      4291 +                            node->value);                    
      4292 +                    }                                        
      4293 +                }                                            
      4294 +                char label[256];                             
      4295 +                snprintf(label, sizeof(label), "Lstatic_%s_%d
           +",                                                           
      4296 +                         cg->current_func, cg->label_count++)
           +;                                                            
      4297 +                const char *label_ptr = codegen_intern(cg, la
           +bel);                                                        
      4298 +                LocalVar new_static_lv;                      
      4299 +                new_static_lv.name = node->value;            
      4300 +                new_static_lv.offset = 0;                    
      4301 +                new_static_lv.size = type_kind_bytes(node->fu
           +ll_type->base->kind);                                        
      4302 +                new_static_lv.kind = TYPE_ARRAY;             
      4303 +                new_static_lv.full_type = node->full_type;   
      4304 +                new_static_lv.is_const = node->is_const;     
      4305 +                new_static_lv.is_unsigned = 0;               
      4306 +                new_static_lv.is_static = 1;                 
      4307 +                new_static_lv.static_label = label_ptr;      
      4308 +                vec_push(&cg->locals, &new_static_lv);       
      4309 +                LocalStaticVar new_sv;                       
      4310 +                new_sv.label = label_ptr;                    
      4311 +                new_sv.kind = TYPE_ARRAY;                    
      4312 +                new_sv.full_type = node->full_type;          
      4313 +                new_sv.size = node->full_type->size;         
      4314 +                new_sv.is_initialized = is_initialized;      
      4315 +                new_sv.init_value = 0;                       
      4316 +                new_sv.is_unsigned = 0;                      
      4317 +                new_sv.init_node = init_node;                
      4318 +                vec_push(&cg->local_statics, &new_sv);       
      4319 +                return;                                      
      4320              }
      4284 -            /* Validate that the initializer (if any) is a co
           -mpile-time constant. */                                      
      4321 +            /* Stage 101: struct/union static. */            
      4322 +            if ((node->decl_type == TYPE_STRUCT || node->decl
           +_type == TYPE_UNION) &&                                      
      4323 +                node->full_type) {                           
      4324 +                if (node->full_type->size == 0) {            
      4325 +                    compile_error(                           
      4326 +                        "error: variable '%s' has incomplete 
           +%s type\n",                                                  
      4327 +                        node->value, type_kind_name(node->dec
           +l_type));                                                    
      4328 +                }                                            
      4329 +                ASTNode *init_node = node->child_count > 0 ? 
           +node->children[0] : NULL;                                    
      4330 +                int is_initialized = (init_node != NULL);    
      4331 +                if (init_node != NULL &&                     
      4332 +                    init_node->type != AST_INITIALIZER_LIST) 
           +{                                                            
      4333 +                    compile_error(                           
      4334 +                        "error: initializer for block-scope s
           +tatic '%s' "                                                 
      4335 +                        "must be a brace-enclosed constant li
           +st\n",                                                       
      4336 +                        node->value);                        
      4337 +                }                                            
      4338 +                char label[256];                             
      4339 +                snprintf(label, sizeof(label), "Lstatic_%s_%d
           +",                                                           
      4340 +                         cg->current_func, cg->label_count++)
           +;                                                            
      4341 +                const char *label_ptr = codegen_intern(cg, la
           +bel);                                                        
      4342 +                LocalVar new_static_lv;                      
      4343 +                new_static_lv.name = node->value;            
      4344 +                new_static_lv.offset = 0;                    
      4345 +                new_static_lv.size = node->full_type->size;  
      4346 +                new_static_lv.kind = node->decl_type;        
      4347 +                new_static_lv.full_type = node->full_type;   
      4348 +                new_static_lv.is_const = node->is_const;     
      4349 +                new_static_lv.is_unsigned = 0;               
      4350 +                new_static_lv.is_static = 1;                 
      4351 +                new_static_lv.static_label = label_ptr;      
      4352 +                vec_push(&cg->locals, &new_static_lv);       
      4353 +                LocalStaticVar new_sv;                       
      4354 +                new_sv.label = label_ptr;                    
      4355 +                new_sv.kind = node->decl_type;               
      4356 +                new_sv.full_type = node->full_type;          
      4357 +                new_sv.size = node->full_type->size;         
      4358 +                new_sv.is_initialized = is_initialized;      
      4359 +                new_sv.init_value = 0;                       
      4360 +                new_sv.is_unsigned = 0;                      
      4361 +                new_sv.init_node = init_node;                
      4362 +                vec_push(&cg->local_statics, &new_sv);       
      4363 +                return;                                      
      4364 +            }                                                
      4365 +            /* Scalar static: validate that the initializer (
           +if any) is a compile-time constant. */                       
      4366              long init_value = 0;
      4367              int is_initialized = 0;
      4368              if (node->child_count > 0) {
     ...
      4412              new_sv.is_initialized = is_initialized;
      4413              new_sv.init_value = init_value;
      4414              new_sv.is_unsigned = node->is_unsigned;
      4415 +            new_sv.init_node = NULL;                         
      4416              vec_push(&cg->local_statics, &new_sv);
      4417              return;
      4418          }

● Agent "Generate stage-101 kickoff artifact" completed · 27s

● Kickoff artifact generated. Continuing with Task 3 — extending
  codegen_emit_local_statics.

● Update(src/codegen.c)
  ⎿  Added 125 lines, removed 4 lines
      5842          for (i = 0; i < cg->local_statics.len; i++) {
      5843              LocalStaticVar *sv = (LocalStaticVar *)vec_get(&c
            g->local_statics, i);
      5844              if (!sv->is_initialized) continue;
      5845 -            fprintf(cg->output, "%s: %s %ld\n",              
      5846 -                    sv->label, data_init_directive(sv->kind),
           - sv->init_value);                                            
      5845 +            /* Stage 101: char array initialized from string 
           +literal. */                                                  
      5846 +            if (sv->kind == TYPE_ARRAY && sv->init_node &&   
      5847 +                sv->init_node->type == AST_STRING_LITERAL) { 
      5848 +                ASTNode *str = sv->init_node;                
      5849 +                int arr_len = sv->full_type ? sv->full_type->
           +length                                                       
      5850 +                                            : str->byte_lengt
           +h + 1;                                                       
      5851 +                int j;                                       
      5852 +                fprintf(cg->output, "%s: db ", sv->label);   
      5853 +                for (j = 0; j < arr_len; j++) {              
      5854 +                    unsigned char b = (j < str->byte_length) 
      5855 +                                      ? (unsigned char)str->v
           +alue[j] : 0;                                                 
      5856 +                    if (j > 0) fprintf(cg->output, ", ");    
      5857 +                    fprintf(cg->output, "%d", b);            
      5858 +                }                                            
      5859 +                fprintf(cg->output, "\n");                   
      5860 +            /* Stage 101: array initialized from brace-list. 
           +*/                                                           
      5861 +            } else if (sv->kind == TYPE_ARRAY && sv->init_nod
           +e &&                                                         
      5862 +                       sv->init_node->type == AST_INITIALIZER
           +_LIST) {                                                     
      5863 +                ASTNode *list = sv->init_node;               
      5864 +                int arr_len = sv->full_type ? sv->full_type->
           +length                                                       
      5865 +                                            : list->child_cou
           +nt;                                                          
      5866 +                Type *elem_type = sv->full_type ? sv->full_ty
           +pe->base : NULL;                                             
      5867 +                TypeKind elem_kind = elem_type ? elem_type->k
           +ind : TYPE_INT;                                              
      5868 +                const char *dir = data_init_directive(elem_ki
           +nd);                                                         
      5869 +                if (arr_len > MAX_ARRAY_ELEMS_DESIGNATED) {  
      5870 +                    compile_error(                           
      5871 +                        "error: array too large for designate
           +d initializer "                                              
      5872 +                        "emission\n");                       
      5873 +                }                                            
      5874 +                ASTNode *slots[MAX_ARRAY_ELEMS_DESIGNATED];  
      5875 +                int si;                                      
      5876 +                for (si = 0; si < arr_len; si++) slots[si] = 
           +NULL;                                                        
      5877 +                int cur = 0;                                 
      5878 +                int jj;                                      
      5879 +                for (jj = 0; jj < list->child_count; jj++) { 
      5880 +                    ASTNode *child = list->children[jj];     
      5881 +                    if (child->type == AST_DESIGNATED_INIT) {
      5882 +                        compile_error(                       
      5883 +                            "error: designated initializers i
           +n block-scope "                                              
      5884 +                            "static array not yet supported\n
           +");                                                          
      5885 +                    }                                        
      5886 +                    if (cur >= arr_len) {                    
      5887 +                        compile_error(                       
      5888 +                            "error: too many initializers for
           + static array\n");                                           
      5889 +                    }                                        
      5890 +                    slots[cur++] = child;                    
      5891 +                }                                            
      5892 +                int j;                                       
      5893 +                for (j = 0; j < arr_len; j++) {              
      5894 +                    if (j == 0) fprintf(cg->output, "%s:\n", 
           +sv->label);                                                  
      5895 +                    ASTNode *elem = slots[j];                
      5896 +                    if (elem != NULL) {                      
      5897 +                        if (elem->type == AST_INT_LITERAL) { 
      5898 +                            long v = strtol(elem->value, NULL
           +, 10);                                                       
      5899 +                            fprintf(cg->output, "    %s %ld\n
           +", dir, v);                                                  
      5900 +                        } else if (elem->type == AST_CHAR_LIT
           +ERAL) {                                                      
      5901 +                            long v = (long)(unsigned char)ele
           +m->value[0];                                                 
      5902 +                            fprintf(cg->output, "    %s %ld\n
           +", dir, v);                                                  
      5903 +                        } else {                             
      5904 +                            compile_error(                   
      5905 +                                "error: unsupported initializ
           +er element in "                                              
      5906 +                                "block-scope static array\n")
           +;                                                            
      5907 +                        }                                    
      5908 +                    } else {                                 
      5909 +                        fprintf(cg->output, "    %s 0\n", dir
           +);                                                           
      5910 +                    }                                        
      5911 +                }                                            
      5912 +            /* Stage 101: struct initialized from brace-list.
           + */                                                          
      5913 +            } else if (sv->kind == TYPE_STRUCT && sv->init_no
           +de) {                                                        
      5914 +                fprintf(cg->output, "%s:\n", sv->label);     
      5915 +                emit_global_struct(cg, sv->full_type, sv->ini
           +t_node);                                                     
      5916 +            /* Stage 101: union initialized from brace-list. 
           +*/                                                           
      5917 +            } else if (sv->kind == TYPE_UNION && sv->init_nod
           +e) {                                                         
      5918 +                fprintf(cg->output, "%s:\n", sv->label);     
      5919 +                {                                            
      5920 +                    ASTNode *list = sv->init_node;           
      5921 +                    int cur_off = 0;                         
      5922 +                    if (list->child_count > 1) {             
      5923 +                        compile_error(                       
      5924 +                            "error: too many initializers for
           + static union\n");                                           
      5925 +                    }                                        
      5926 +                    if (list->child_count == 1 &&            
      5927 +                        sv->full_type->field_count > 0) {    
      5928 +                        StructField *f = &sv->full_type->fiel
           +ds[0];                                                       
      5929 +                        int fsize = f->full_type ? f->full_ty
           +pe->size                                                     
      5930 +                                                 : type_kind_
           +bytes(f->kind);                                              
      5931 +                        ASTNode *elem = list->children[0];   
      5932 +                        if (elem->type == AST_INT_LITERAL) { 
      5933 +                            long v = strtol(elem->value, NULL
           +, 10);                                                       
      5934 +                            fprintf(cg->output, "    %s %ld\n
           +",                                                           
      5935 +                                    data_init_directive(f->ki
           +nd), v);                                                     
      5936 +                        } else if (elem->type == AST_CHAR_LIT
           +ERAL) {                                                      
      5937 +                            long v = (long)(unsigned char)ele
           +m->value[0];                                                 
      5938 +                            fprintf(cg->output, "    %s %ld\n
           +",                                                           
      5939 +                                    data_init_directive(f->ki
           +nd), v);                                                     
      5940 +                        } else {                             
      5941 +                            compile_error(                   
      5942 +                                "error: unsupported initializ
           +er for static "                                              
      5943 +                                "union\n");                  
      5944 +                        }                                    
      5945 +                        cur_off = fsize;                     
      5946 +                    }                                        
      5947 +                    while (cur_off < sv->full_type->size) {  
      5948 +                        fprintf(cg->output, "    db 0\n");   
      5949 +                        cur_off++;                           
      5950 +                    }                                        
      5951 +                }                                            
      5952 +            } else {                                         
      5953 +                /* Scalar fallthrough. */                    
      5954 +                fprintf(cg->output, "%s: %s %ld\n",          
      5955 +                        sv->label, data_init_directive(sv->ki
           +nd), sv->init_value);                                        
      5956 +            }                                                
      5957          }
      5958      }
      5959      if (has_bss) {
     ...
      5961          for (i = 0; i < cg->local_statics.len; i++) {
      5962              LocalStaticVar *sv = (LocalStaticVar *)vec_get(&c
            g->local_statics, i);
      5963              if (sv->is_initialized) continue;
      5854 -            fprintf(cg->output, "%s: %s 1\n",                
      5855 -                    sv->label, bss_res_directive(sv->kind)); 
      5964 +            /* Stage 101: array/struct/union bss. */         
      5965 +            if (sv->kind == TYPE_ARRAY && sv->full_type) {   
      5966 +                fprintf(cg->output, "%s: %s %d\n",           
      5967 +                        sv->label,                           
      5968 +                        bss_res_directive(sv->full_type->base
           +->kind),                                                     
      5969 +                        sv->full_type->length);              
      5970 +            } else if ((sv->kind == TYPE_STRUCT || sv->kind =
           += TYPE_UNION) &&                                             
      5971 +                       sv->full_type) {                      
      5972 +                fprintf(cg->output, "%s: resb %d\n", sv->labe
           +l, sv->full_type->size);                                     
      5973 +            } else {                                         
      5974 +                fprintf(cg->output, "%s: %s 1\n",            
      5975 +                        sv->label, bss_res_directive(sv->kind
           +));                                                          
      5976 +            }                                                
      5977          }
      5978      }
      5979  }

● Task 4 — fix array decay in codegen_expression VAR_REF:

● Update(src/codegen.c)  ⎿  Added 4 lines, removed 1 line
      2066               * assignment is still rejected by the AST_ASSIGN
            MENT path,
      2067               * which checks the LHS name before reaching this
             code. */
      2068              if (lv->kind == TYPE_ARRAY) {
      2069 -                fprintf(cg->output, "    lea rax, [rbp - %d]\
           -n", lv->offset);                                             
      2069 +                if (lv->is_static)                           
      2070 +                    fprintf(cg->output, "    lea rax, [rel %s
           +]\n", lv->static_label);                                     
      2071 +                else                                         
      2072 +                    fprintf(cg->output, "    lea rax, [rbp - 
           +%d]\n", lv->offset);                                         
      2073                  node->result_type = TYPE_POINTER;
      2074                  node->full_type = type_pointer(lv->full_type-
            >base);
      2075                  return;

● Task 5 — fix emit_array_index_addr subscript base, remove stale comment:

● Update(src/codegen.c)
  ⎿  Added 4 lines, removed 3 lines
      984      if (lv) {
      985          if (lv->kind == TYPE_ARRAY) {
      986              element = lv->full_type->base;
      987 -            /* Array statics are rejected at declaration time;
          - is_static is                                                 
      988 -             * always false here, so no static-label branch ne
          -eded. */                                                      
      989 -            fprintf(cg->output, "    lea rax, [rbp - %d]\n", l
          -v->offset);                                                   
      987 +            if (lv->is_static)                                
      988 +                fprintf(cg->output, "    lea rax, [rel %s]\n",
          + lv->static_label);                                           
      989 +            else                                              
      990 +                fprintf(cg->output, "    lea rax, [rbp - %d]\n
          +", lv->offset);                                               
      991          } else if (lv->kind == TYPE_POINTER) {
      992              element = lv->full_type->base;
      993              /* Stage 38: subscript on void * is not allowed. *
           /

● Task 6 — fix emit_member_addr local struct branch:

● Update(src/codegen.c)
  ⎿  Added 9 lines, removed 1 line
      1188              compile_error( "error: '%s' has no member '%s'\n"
            ,
      1189                      type_kind_name(lv->kind), field_name);
      1190          }
      1191 -        fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->
           -offset - f->offset);                                         
      1191 +        if (lv->is_static) {                                 
      1192 +            if (f->offset != 0)                              
      1193 +                fprintf(cg->output, "    lea rax, [rel %s + %
           +d]\n",                                                       
      1194 +                        lv->static_label, f->offset);        
      1195 +            else                                             
      1196 +                fprintf(cg->output, "    lea rax, [rel %s]\n"
           +, lv->static_label);                                         
      1197 +        } else {                                             
      1198 +            fprintf(cg->output, "    lea rax, [rbp - %d]\n", 
           +lv->offset - f->offset);                                     
      1199 +        }                                                    
      1200          return f;
      1201      }
      1202      /* Stage 44: fall back to global struct/union variable. *
            /

● Now update version.c to stage 101 (01010000):

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      3  
      4  #define VERSION_MAJOR  "00"
      5  #define VERSION_MINOR  "02"
      6 -#define VERSION_STAGE  "01000000"                               
      6 +#define VERSION_STAGE  "01010000"                               
      7  
      8  #ifndef VERSION_BUILD
      9  #define VERSION_BUILD  0

● Now build and run the existing tests to verify nothing broke:

● Bash(cmake -S . -B build -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -5 && cmake
      --build build 2>&1 | tail -10)
  ⎿  -- Configuring done (0.0s)
     -- Generating done (0.0s)
     -- Build files have been written to: /home/scott/code/claude/claudec99
     … +13 lines (ctrl+o to expand)
  ⎿  (timeout 1m)

● Good — clean build. Now let me add the tests. Let me check existing test
  conventions first:

  Listed 2 directories (ctrl+o to expand)

● Now create the valid tests:

● Write(test/valid/test_block_static_array_uninitialized__0.c)
  ⎿  Wrote 14 lines to
     test/valid/test_block_static_array_uninitialized__0.c
      1 void bump(int i) {
      2     static int arr[4];
      3     arr[i]++;
      4 }
      5 
      6 int get(int i) {
      7     static int arr[4];
      8     return arr[i];
      9 }
     … +4 lines (ctrl+o to expand)

● Actually that test won't work — separate functions have separate static
  arrays. Let me fix:

● Write(test/valid/test_block_static_array_uninitialized__0.c)
  ⎿  Added 3 lines, removed 7 lines
       1 -void bump(int i) {                                             
       1 +int bump_and_get(int i) {                                      
       2      static int arr[4];
       3      arr[i]++;
       4 -}                                                              
       5 -                                                               
       6 -int get(int i) {                                               
       7 -    static int arr[4];                                         
       4      return arr[i];
       5  }
       6  
       7  int main() {
      12 -    bump(0); bump(0); bump(1);                                 
      13 -    return (get(0) == 2 && get(1) == 1 && get(2) == 0) ? 0 : 1;
       8 +    bump_and_get(0); bump_and_get(0); bump_and_get(1);         
       9 +    return (bump_and_get(0) == 3 && bump_and_get(1) == 2) ? 0 :
         + 1;                                                            
      10  }

● Write(test/valid/test_block_static_array_initialized__0.c)
  ⎿  Wrote 8 lines to test/valid/test_block_static_array_initialized__0.c
      1 int get_step(int i) {
      2     static int steps[3] = {10, 20, 30};
      3     return steps[i];
      4 }
      5 
      6 int main() {
      7     return get_step(0) + get_step(1) + get_step(2) - 60;
      8 }

● Write(test/valid/test_block_static_struct_uninitialized__0.c)
  ⎿  Wrote 12 lines to
     test/valid/test_block_static_struct_uninitialized__0.c
       1 struct Counter { int n; int total; };
       2 
       3 void tick(int v) {
       4     static struct Counter c;
       5     c.n++;
       6     c.total += v;
       7 }
       8 
       9 int main() {
      10     tick(5); tick(3);
     … +2 lines (ctrl+o to expand)

● Write(test/valid/test_block_static_struct_initialized__0.c)
  ⎿  Wrote 10 lines to test/valid/test_block_static_struct_initialized__0.c
       1 struct Point { int x; int y; };
       2 
       3 int sum_coords(void) {
       4     static struct Point p = {3, 4};
       5     return p.x + p.y;
       6 }
       7 
       8 int main() {
       9     return sum_coords() - 7;
      10 }

● Write(test/valid/test_block_static_char_array_string__0.c)
  ⎿  Wrote 9 lines to test/valid/test_block_static_char_array_string__0.c
      1 char *greeting(void) {
      2     static char msg[] = "hi";
      3     return msg;
      4 }
      5 
      6 int main() {
      7     char *s = greeting();
      8     return (s[0] == 'h' && s[1] == 'i' && s[2] == '\0') ? 0 : 1;
      9 }

● Write(test/valid/test_block_static_array_persists__0.c)
  ⎿  Wrote 10 lines to test/valid/test_block_static_array_persists__0.c
       1 int count_calls(void) {
       2     static int calls[1];
       3     calls[0]++;
       4     return calls[0];
       5 }
       6 
       7 int main() {
       8     count_calls(); count_calls();
       9     return count_calls() - 3;
      10 }

● Now the two invalid tests:

● Write(test/invalid/test_block_static_array_nonconstant_init.c)
  ⎿  Wrote 8 lines to
     test/invalid/test_block_static_array_nonconstant_init.c
      1 int n = 5;
      2 
      3 int f(void) {
      4     static int arr[3] = n;
      5     return arr[0];
      6 }
      7 
      8 int main() { return f(); }

● Write(test/invalid/test_block_static_struct_nonconstant_init.c)
  ⎿  Wrote 9 lines to
     test/invalid/test_block_static_struct_nonconstant_init.c
      1 struct P { int x; int y; };
      2 struct P g = {1, 2};
      3 
      4 int f(void) {
      5     static struct P p = g;
      6     return p.x;
      7 }
      8 
      9 int main() { return f(); }

● Now run the full test suite:

● Bash(./test/run_all_tests.sh 2>&1 | tail -30)
  ⎿  ====================================================
     Running suite: print_asm
     ====================================================
     … +27 lines (ctrl+o to expand)
  ⎿  (timeout 5m)
● The aggregate shows 1552 total but only 1550 passed with 0 failed — that's 2
  unaccounted. Let me check what's happening:

● Bash(./test/valid/run_tests.sh 2>&1 | tail -20)
  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_while_coun
     tdown__42.c -> test_while_countdown__42.asm
     PASS  test_while_countdown__42  (exit code: 42)
     … +20 lines (ctrl+o to expand)
  ⎿  (timeout 3m)

● Bash(./test/invalid/run_tests.sh 2>&1 | tail -5 &&
      ./test/integration/run_tests.sh 2>&1 | tail -5)
  ⎿  PASS  test_void_ptr_deref__cannot_dereference  (error contains: 'canno
     t dereference')
     PASS  test_void_ptr_subscript__cannot_subscript  (error contains: 'can
     … +12 lines (ctrl+o to expand)
  ⎿  (timeout 3m)

● The invalid suite shows "248 passed, 0 failed, 250 total" — 2 tests not
  counted. Let me check:

● Bash(./test/invalid/run_tests.sh 2>&1 | grep -E
      "static_array|static_struct_nonconstant")
  ⎿  SKIP  test_block_static_array_nonconstant_init  (no __<expected> suffi
     x)
     SKIP  test_block_static_struct_nonconstant_init  (no __<expected> suff
     ix)

● The invalid tests need the __<keyword> suffix in their filenames. Let me check
  the naming pattern:

  Read 2 files, listed 1 directory (ctrl+o to expand)

● The invalid tests need the __<fragment> suffix. Let me rename them:

● Bash(mv /home/scott/code/claude/claudec99/test/invalid/test_block_static_array
      _nonconstant_init.c \
         /home/scott/code/claude/claudec99/test/invalid/test_block_stat…)
  ⎿  Done

● Bash(./test/invalid/run_tests.sh 2>&1 | grep -E
      "static_array_nonconstant|static_struct_nonconstant")
  ⎿  FAIL  test_block_static_array_nonconstant_init__brace_enclosed_constan
     t_list  (expected error containing 'brace enclosed constant list', got
     : /home/scott/code/claude/claudec99/test/invalid/test_block_static_arr
     … +8 lines (ctrl+o to expand)

● The fragment matching fails because the runner converts underscores to spaces
  but the error messages use hyphens. Let me use a simpler fragment:

● Bash(cd /home/scott/code/claude/claudec99/test/invalid && \
      mv
      test_block_static_array_nonconstant_init__brace_enclosed_constant_list.c
      \…)
  ⎿  Done

● Bash(./test/invalid/run_tests.sh 2>&1 | grep -E
      "static_array_nonconstant|static_struct_nonconstant")
  ⎿  No matches found

● Bash(./test/invalid/run_tests.sh 2>&1 | grep -E "nonconstant")
  ⎿  No matches found

  Listed 1 directory (ctrl+o to expand)

● Bash(./test/invalid/run_tests.sh 2>&1 | grep -E "brace|nonconstant")
  ⎿  No matches found

● Bash(./test/invalid/run_tests.sh 2>&1 | grep "block_static" )
  ⎿  No matches found

● Bash(./test/invalid/run_tests.sh 2>&1 | grep "static")
  ⎿  No matches found

● Bash(./test/invalid/run_tests.sh 2>&1 | grep -i "static" | head -10)
  ⎿  (No output)

● Bash(./test/invalid/run_tests.sh 2>&1 | tail -10)
  ⎿  /bin/bash: line 1: ./test/invalid/run_tests.sh: No such file or direct
     ory

  Listed 1 directory (ctrl+o to expand)
● Bash(cd /home/scott/code/claude/claudec99 && ./test/invalid/run_tests.sh 2>&1
      | tail -10)
  ⎿  PASS  test_void_fn_result_in_assign__cannot  (error contains: 'cannot'
     )
     PASS  test_void_fn_result_in_return__cannot_use  (error contains: 'can
     … +15 lines (ctrl+o to expand)

● All 250 invalid tests pass. Now run the full suite:

● Bash(./test/run_all_tests.sh 2>&1 | tail -10)
  ⎿  PASS  test_stage_16_04_bitor_int
     PASS  test_stage_16_04_bitxor_int
     PASS  test_stage_22_01_global_bss_layout
     … +7 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● All 1552 tests pass. Now commit the implementation:

● Bash(git add include/codegen.h src/codegen.c src/version.c \
        test/valid/test_block_static_array_uninitialized__0.c \…)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +16 lines (ctrl+o to expand)

● Bash(git commit -m "$(cat <<'EOF'
      stage 101: block-scope static arrays and structs…)
  ⎿  [master 89c8ff5] stage 101: block-scope static arrays and structs
      12 files changed, 423 insertions(+), 15 deletions(-)
      create mode 100644 docs/kickoffs/stage-101-kickoff.md
     … +15 lines (ctrl+o to expand)

● Implementation committed. Now running the self-host cycle:

● Bash(./build.sh --mode=self-host 2>&1)
  ⎿  === Self-host test ===
     Archiving build/ccompiler-c0 -> build/previous/ccompiler-c0
     Archiving build/ccompiler-c1 -> build/previous/ccompiler-c1
     … +517 lines (ctrl+o to expand)
  ⎿  (timeout 10m)

  Read 1 file (ctrl+o to expand)

● Self-host passes cleanly — C0→C1→C2 all 1552 tests. Let me check if build.sh
  made a checkpoint commit:

● Bash(git log --oneline -4)
  ⎿  a0fbb8a self-host C1 verified: all tests pass
     ce25178 self-host C0 verified: all tests pass
     89c8ff5 stage 101: block-scope static arrays and structs
     5c04f45 docs: stage 101 spec — block-scope static arrays and structs

● Good. Now update the self-compilation report and kick off the documentation
  phase:

  Read 1 file (ctrl+o to expand)

● Now I'll update the self-compilation report with the stage 101 results:

● Update(docs/self-compilation-report.md)
  ⎿  Added 20 lines
      150  
      151  None. The designated-initializer implementation (parser, codeg
           en for local/global structs and arrays) compiled cleanly under
            C0. All new codegen code uses fixed-size arrays (`MAX_STRUCT_
           FIELDS_DESIGNATED = 64`, `MAX_ARRAY_ELEMS_DESIGNATED = 1024`) 
           rather than VLAs to maintain self-hosting compatibility. All 1
           501 tests passed at C0, C1, and C2.
      152  
      153 +## Issues found during stage 101 self-hosting test            
      154 +                                                              
      155 +None. The new array/struct/union static-local registration pat
          +hs use                                                        
      156 +`vec_push` (the established pattern), and `codegen_emit_local_
          +statics`                                                      
      157 +uses fixed-size `slots[]` arrays bounded by `MAX_ARRAY_ELEMS_D
          +ESIGNATED`                                                    
      158 +(already present for the global array path). The compiler's ow
          +n source                                                      
      159 +uses block-scope `static` scalars and pointers extensively but
          + no                                                           
      160 +`static` array or struct locals, so the bootstrap cycle requir
          +ed no                                                         
      161 +source changes. All 1552 tests passed at C0, C1, and C2.      
      162 +                                                              
      163 +## Result (stage 101)                                         
      164 +                                                              
      165 +**Date:** 2026-06-10                                          
      166 +                                                              
      167 +| Step | Binary | Version | BuiltBy | Tests |                 
      168 +|------|--------|---------|---------|-------|                 
      169 +| C0 | `build/ccompiler-c0` | `00.02.01010000.00819` | `GNU_13
          +_3_0` | 1552/1552 |                                           
      170 +| C1 | `build/ccompiler-c1` | `00.02.01010000.00820` | `Claude
          +C99_v00_02_01010000_00819` | 1552/1552 |                      
      171 +| C2 | `build/ccompiler-c2` | `00.02.01010000.00821` | `Claude
          +C99_v00_02_01010000_00820` | 1552/1552 |                      
      172 +                                                              
      173  ## Issues found during stage 100 self-hosting test
      174  
      175  None. The `eval_const_primary` sizeof extension and the `parse
           _external_declaration` path replacements are purely parser-lev
           el changes with no new AST node types, no dynamic allocation, 
           and no codegen changes. All new code uses the existing `parse_
           type_name` / `type_size` / `lexer_store_bytes` helpers already
            verified self-hosting. The `sizeof(void *)` fix in `parse_pri
           mary` uses `parse_type_name` followed by a `t->kind == TYPE_VO
           ID` check — the same pattern used for array incomplete-type de
           tection. All 1544 tests passed at C0, C1, and C2.

● Bash(git add docs/self-compilation-report.md && git commit -m "$(cat <<'EOF'
      docs: stage 101 self-host results — C0→C1→C2 all 1552 tests pass…)
  ⎿  [master d03dc8f] docs: stage 101 self-host results — C0→C1→C2 all 1552
      tests pass
      1 file changed, 20 insertions(+)

● Now delegate the documentation artifacts and supplemental docs to haiku:

● haiku-stage-artifact-writer(Generate stage-101 milestone, transcript, update
                             README and grammar)
  ⎿  Done (12 tool uses · 43.7k tokens · 46s)
  (ctrl+o to expand)

● Let me check what the agent generated:

  Searched for 3 patterns, read 1 file, listed 1 directory (ctrl+o to expand)

● The "Not yet supported" section correctly doesn't mention block-scope static
  arrays/structs anymore. Let me check the declarations bullet:

  Searched for 1 pattern (ctrl+o to expand)

● The declarations bullet on line 405 correctly now says "(scalar, pointer,
  array, and struct/union types)". README looks correct.

  Now let me check the test totals in README:

  Searched for 1 pattern (ctrl+o to expand)

● README is fully updated. Now run the update-supplemental-docs skill:

● Skill(update-supplemental-docs)
  ⎿  Successfully loaded skill
  Searched for 1 pattern, read 2 files, listed 1 directory (ctrl+o to expand)

● Bash(sed -n '1406,1430p' docs/outlines/checklist.md)
  ⎿  ## Stage 100 - File-Scope Constant Expressions

     - [x] `eval_const_primary`: `TOKEN_SIZEOF` handling added for `sizeof(
     … +26 lines (ctrl+o to expand)

  Searched for 2 patterns (ctrl+o to expand)

● Bash(sed -n '1448,1475p' docs/outlines/checklist.md)
  ⎿  - [ ] Type compatibility and composite type rules

     ### Declarations and Scope
     … +25 lines (ctrl+o to expand)

  Searched for 2 patterns (ctrl+o to expand)

● Bash(sed -n '985,1000p' docs/outlines/checklist.md)
  ⎿     - [ ] Codegen (non-parser) errors/warnings still print without positio
     n prefix (AST nodes lack token info)
     … +18 lines (ctrl+o to expand)


● Now I have all I need. Let me also gather fresh metrics and read the template
  files.

● Bash(echo "=== Source files ===" && ls src/*.c | wc -l && \
      echo "=== Header files ===" && ls include/*.h | wc -l && \…)
  ⎿  === Source files ===
     12
     === Header files ===
     … +19 lines (ctrl+o to expand)

  Read 1 file (ctrl+o to expand)

● Bash(sed -n '180,280p' status/project-status-through-stage-100.md)
  ⎿    integer/character literals, enum constants, and all of `* / % << >>
     + - & ^ |`
       with unary `~ !` and parenthesized sub-expressions (stage 77; extend
     … +98 lines (ctrl+o to expand)

● Bash(sed -n '280,380p' status/project-status-through-stage-100.md)
  ⎿  constant-expression context (enum values, `case` labels, array
     designator indices, and file-scope initializers), and a pre-existing
     bug that rejected `sizeof(void *)` in general expressions is fixed.
     … +98 lines (ctrl+o to expand)

● Bash(sed -n '380,450p' status/project-status-through-stage-100.md)
  ⎿  | 95-07      | Remaining static usages → `Vec`; call-layout bounds gua
     rd     |
     | 95-08      | Token text → pointer + length (255-byte string cap remo
     … +68 lines (ctrl+o to expand)

  Read 1 file (ctrl+o to expand)

● Now I have everything I need. Let me write all four updated documents.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Update(docs/outlines/checklist.md)
  ⎿  Added 24 lines
      1421  - [x] Tests: 10 valid (arith, bitwise-or, shift, sizeof-void*
            , sizeof-int*256, sizeof-struct, enum-op, neg, complement, mu
            lti-decl), 2 invalid (variable reference, sizeof-no-parens), 
            1 print_ast (fold to IntLiteral); 2 pre-existing invalid test
            s renamed to match new error message; all 1544 tests pass
      1422  - [x] Self-host C0→C1→C2 passes with no bootstrap issues; 154
            4 tests pass at all three stages
      1423  
      1424 +## Stage 101 - Block-Scope Static Arrays and Structs         
      1425 +                                                             
      1426 +- [x] `include/codegen.h`: added `ASTNode *init_node` to `Loc
           +alStaticVar` struct to carry brace-list and string-literal in
           +itializers for aggregate types                               
      1427 +- [x] `src/codegen.c` `codegen_statement` SC_STATIC arm: remo
           +ved guard that rejected `TYPE_ARRAY`/`TYPE_STRUCT`/`TYPE_UNIO
           +N`                                                           
      1428 +  - [x] Added array static registration block: validates init
           +ializer (must be `AST_INITIALIZER_LIST` or string literal for
           + char arrays), generates `Lstatic_func_N` label, registers in
           + `cg->locals` with `is_static=1`, pushes `LocalStaticVar` wit
           +h `init_node` set                                            
      1429 +  - [x] Added struct/union static registration block: validat
           +es non-zero size (incomplete type check), validates brace-lis
           +t initializer, same label/registration pattern               
      1430 +  - [x] Scalar block unchanged; now falls through naturally a
           +fter array/struct branches return early                      
      1431 +- [x] `codegen_emit_local_statics()`: extended `.data` loop f
           +or aggregate statics                                         
      1432 +  - [x] `TYPE_ARRAY` + `AST_STRING_LITERAL` → inline `db` byt
           +es (char array from string literal)                          
      1433 +  - [x] `TYPE_ARRAY` + `AST_INITIALIZER_LIST` → slots-map pat
           +tern (same as global array emit); error if slots `> MAX_ARRAY
           +_ELEMS_DESIGNATED`; error on designated init (not yet support
           +ed in this context); each slot emits directive+value or zero-
           +fill                                                         
      1434 +  - [x] `TYPE_STRUCT` + `AST_INITIALIZER_LIST` → calls `emit_
           +global_struct(cg, sv->full_type, sv->init_node)`             
      1435 +  - [x] `TYPE_UNION` + `AST_INITIALIZER_LIST` → inline first-
           +member logic (same as global union emit)                     
      1436 +  - [x] Scalar fallthrough: existing `data_init_directive` / 
           +`init_value` path                                            
      1437 +- [x] `codegen_emit_local_statics()`: extended `.bss` loop   
      1438 +  - [x] `TYPE_ARRAY` → `label: resx length` (using `bss_res_d
           +irective` of the element kind)                               
      1439 +  - [x] `TYPE_STRUCT` / `TYPE_UNION` → `label: resb total_siz
           +e`                                                           
      1440 +  - [x] Scalar fallthrough unchanged                         
      1441 +- [x] `codegen_expression` VAR_REF `TYPE_ARRAY` branch: added
           + `is_static` guard → `lea rax, [rel label]` instead of `lea r
           +ax, [rbp - offset]`                                          
      1442 +- [x] `emit_array_index_addr()`: added `is_static` guard → `l
           +ea rax, [rel label]`; removed stale comment that claimed arra
           +y statics were always rejected                               
      1443 +- [x] `emit_member_addr()` local-struct branch: added `is_sta
           +tic` guard → `lea rax, [rel label + offset]` (or `[rel label]
           +` when offset is 0)                                          
      1444 +- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01010000"`
      1445 +- [x] Tests: 6 valid (uninitialized array, initialized array,
           + uninitialized struct, initialized struct, char-array from st
           +ring literal, single-element array counter), 2 invalid (non-b
           +race initializer for static array, non-brace initializer for 
           +static struct); all 1552 tests pass                          
      1446 +- [x] Self-host C0→C1→C2 passes with no bootstrap issues; no 
           +compiler source changes needed; 1552 tests pass at all three 
           +stages                                                       
      1447 +                                                             
      1448  ---
      1449  
      1450  ## TODO

● Now update the TODO item for static local variables to reflect stage 101
  completing aggregate statics:

  Searched for 2 patterns, read 1 file (ctrl+o to expand)

● Update(docs/outlines/checklist.md)
  ⎿  Added 1 line, removed 1 line
      1472  - [ ] Type compatibility and composite type rules
      1473  
      1474  ### Declarations and Scope
      1475 -- [x] static storage class (block scope — local static variab
           -les) (Stage 71)                                              
      1475 +- [x] static storage class (block scope — local static variab
           +les, scalar/pointer: Stage 71; arrays/structs/unions: Stage 1
           +01)                                                          
      1476  - [ ] register storage class (hint only)
      1477  - [ ] auto storage class (explicit)
      1478  - [ ] Tentative definitions for file-scope variables

● Now write the parse tree document. Stage 101 is codegen-only, so it doesn't
  change parser functions — but I should note the LocalStaticVar extension and
  the codegen functions touched:

  Read 1 file (ctrl+o to expand)

● Bash(sed -n '80,200p' docs/other/stage-100-parse-tree.md)
  ⎿  Codegen changes (Tasks 3–6) update `emit_local_struct_init`,
     `codegen_statement` (local array), `emit_global_struct`, and
     `codegen_emit_data` (global array) to handle `AST_DESIGNATED_INIT` chi
     … +118 lines (ctrl+o to expand)

● Bash(sed -n '200,280p' docs/other/stage-100-parse-tree.md)
  ⎿  2. **`parse_external_declaration` first-declarator path updated** (`sr
     c/parser.c`):
        the non-brace initializer arm is replaced with a branch on declarat
     … +78 lines (ctrl+o to expand)

● Bash(sed -n '280,400p' docs/other/stage-100-parse-tree.md)
  ⎿          │           └─► parse_logical_and_expression
             │                 └─► parse_bitwise_or_expression
             │                       └─► parse_bitwise_xor_expression
     … +118 lines (ctrl+o to expand)
  Read 1 file (ctrl+o to expand)

● Now I have all I need to write the documents. Let me create them:

● Write(docs/other/stage-101-parse-tree.md)
  ⎿  Wrote 403 lines to docs/other/stage-101-parse-tree.md
       2 # Full Grammar Parse Tree — Stage 101
         ns. Each levelrchy of parser functions, grouped into four sectio
         ks call depth;l(s) below it for sub-constructs. Indentation trac
       5 `└─►` indicates the primary descent path, `├─►` indicates a bran
       6 ch.
         ro, which calls raised through the `PARSER_ERROR(parser, …)` mac
       8 `compile_error_at` with the current token's `file:line:col` posi
       9 70-02/70-03). Under `--max-errors=N` the parser recovers via set
      10 `parser_sync()` to the next declaration boundary instead of exit
         ing on the first
     … +610 lines (ctrl+o to expand)

● Now write the project status snapshot:

● Write(status/project-status-through-stage-101.md)
  ⎿  Wrote 473 lines to status/project-status-through-stage-101.md
       2 # ClaudeC99 Project Status — Through Stage 101
       3 _As of 2026-06-10 (commit `d03dc8f`)_
       5 ## Overview
       6 
       7 ClaudeC99 is a from-scratch C99 subset compiler written in C, ta
       8 x86_64 Linux via NASM (Intel syntax). The compiler is built incr
         ementally
         edrough small, spec-driven stages — each stage is a self-contain
      10 specification followed by a kickoff, implementation, and milesto
         ne /
     … +637 lines (ctrl+o to expand)

● Now write the features snapshot:

● Write(status/project-features-through-stage-101.md)
  ⎿  Wrote 3 lines to status/project-features-through-stage-101.md
        omment removal, `#include "file.h"` and `#include <file.h>`, obje
        ct-like and function-like `#define` with parameter substitution a
        nd recursive expansion, variadic macros with `__VA_ARGS__`, macro
         stringification `#param`, token pasting `##`, `#undef`, conditio
        nal compilation `#ifdef`/`#ifndef`/`#if`/`#elif`/`#else`/`#endif`
         with full recursive-descent expression evaluation including `def
        ined()`, macro expansion, arithmetic, bitwise, shift, logical, an
        d comparison operators, `#error`, predefined macros `__FILE__`/`_
        _LINE__`/`__DATE__`/`__TIME__`/`__STDC__`/`__STDC_VERSION__`/`__S
        TDC_HOSTED__`/`__CLAUDEC99__`, ABI-refle
     … +171 lines (ctrl+o to expand)

● Now verify all four documents exist and commit everything:

  Listed 1 directory (ctrl+o to expand)

● All four documents created/updated. Now commit and finish:

● Bash(git add docs/outlines/checklist.md \
        docs/other/stage-101-parse-tree.md \…)
  ⎿  On branch master
     Your branch is ahead of 'origin/master' by 4 commits.
       (use "git push" to publish your local commits)
     … +10 lines (ctrl+o to expand)

● Bash(git commit -m "$(cat <<'EOF'
      docs: stage 101 milestone, transcript, supplemental snapshots…)
  ⎿  [master 7295e5c] docs: stage 101 milestone, transcript, supplemental s
     napshots
      7 files changed, 1011 insertions(+), 4 deletions(-)
     … +5 lines (ctrl+o to expand)

● update-supplemental-docs: refreshed through stage 101
  - docs/outlines/checklist.md — Stage 101 section added (17 items); static
  local variables TODO bullet updated to include aggregates
  - docs/other/stage-101-parse-tree.md — new; notes 6 codegen-only changes, no
  parser/AST changes
  - status/project-status-through-stage-101.md — new; 1552 tests, 14,837 LOC,
  822 commits
  - status/project-features-through-stage-101.md — new; stage 101 aggregate
  statics folded in, 3 new out-of-scope items added

✻ Sautéed for 19m 56s