 ▐▛███▜▌   Claude Code v2.1.143
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

❯ /implement-stage docs/stages/ClaudeC99-spec-stage-51-function-like-mcaros.md  

● I'll start by reading the spec file and supporting materials in parallel.

  Read 4 files (ctrl+o to expand)

● Now let me look at the existing preprocessor implementation from stage 50.

  Searched for 1 pattern, read 2 files, listed 1 directory (ctrl+o to expand)

● Let me also look at existing test files and the stage 50 milestone for naming
  patterns.

  Read 1 file, listed 6 directories (ctrl+o to expand)

● Now I have everything I need. Let me check the stage label script and existing
   template files.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Bash(python3 /home/scott/code/claude/claudec99/.claude/skills/implement-stage/
      scripts/stage-label.py
      docs/stages/ClaudeC99-spec-stage-51-function-like-mcaros.md)
  ⎿  stage-51

● STAGE_DISPLAY: Stage 51

  ---
  Stage 51 Spec Summary

  Task: Add function-like macro support to the preprocessor.

  Key requirements:
  - Parse #define NAME(arg1, arg2) replacement — no whitespace between name and
  (
  - #define F (x) (with space) remains an object-like macro with (x) as
  replacement
  - Expand macro invocations in source, substituting arguments
  - Arguments may be arbitrary token sequences; comma splits only at depth 0
  - Nested parentheses in arguments: ADD(1, ADD(2,3))
  - Zero-argument macros: #define VALUE() 42
  - Exact argument count enforced
  - Object-like macros continue to work

  ---
  Spec Issues / Ambiguities

  Before implementation, I'm calling out these issues in the spec:

  1. Test typo (line 58): #define SQUARE(x) ((x) * (X)) uses uppercase X in the
  replacement but defines lowercase x. The expected result is 64 (8×8), so it
  should be ((x) * (x)). I'll write the test with the corrected form ((x) * 
  (x)).
  2. Missing closing brace (line 86–87): The nested macros test is missing the
  closing } on the function body. I'll add it in the test file.
  3. Typo line 31: "marco arguments" → "macro arguments" (spec comment only, no
  action needed).
  4. Comment typo line 62: "epect" → "expect" (spec typo, no action needed).

  ---
  Planned Changes

  File: src/preprocessor.c
  Change: Extend MacroDef with param list; parse function-like #define; expand
    function-like calls
  ────────────────────────────────────────
  File: test/valid/test_pp_fn_macro_add__42.c
  Change: New test
  ────────────────────────────────────────
  File: test/valid/test_pp_fn_macro_square__64.c
  Change: New test
  ────────────────────────────────────────
  File: test/valid/test_pp_fn_macro_zero_arg__42.c
  Change: New test
  ────────────────────────────────────────
  File: test/valid/test_pp_fn_macro_expr_arg__6.c
  Change: New test
  ────────────────────────────────────────
  File: test/valid/test_pp_fn_macro_nested__6.c
  Change: New test
  ────────────────────────────────────────
  File: test/invalid/test_invalid_139__wrong_arg_count_for_function_like_macro.c
  Change: New invalid test
  ────────────────────────────────────────
  File: docs/grammar.md
  Change: No grammar change (preprocessor only)
  ────────────────────────────────────────
  File: README.md
  Change: Update to mention function-like macros

  ---
  Generating the Kickoff artifact now.

● haiku-stage-artifact-writer(Generate stage-51 kickoff artifact)
  ⎿  Done (3 tool uses · 7.4k tokens · 11s)
  (ctrl+o to expand)

● Now I have everything needed. Let me implement.

  STAGE_DISPLAY: Stage 51

  ---
  Spec Issues (calling out before implementation)

  1. Test typo (line 58): #define SQUARE(x) ((x) * (X)) — X should be x. I'll
  use the corrected form ((x) * (x)) in the test.
  2. Missing closing brace (line 86): The nested macro test is missing the }
  closing the main function body. I'll add it.
  3. Minor typos in spec: "marco" → "macro" (line 31), "epect" → "expect"
  (line 62) — noted, no action.

  ---
  Now implementing. All changes are in src/preprocessor.c and new test files.

● Write(src/preprocessor.c)
  ⎿  Added 380 lines, removed 12 lines
      59  /* ---- Macro table -------------------------------------------
          ---------- */
      60  
      61  typedef struct {
      62 -    char *name;                                                
      63 -    char *replacement;                                         
      62 +    char  *name;                                               
      63 +    char **params;      /* NULL for object-like; owned array of
         + owned strings */                                              
      64 +    int    param_count; /* -1 = object-like, >= 0 = function-li
         +ke           */                                                
      65 +    char  *replacement;                                        
      66  } MacroDef;
      67  
      68  typedef struct {
     ...
      81      for (size_t i = 0; i < t->count; i++) {
      82          free(t->defs[i].name);
      83          free(t->defs[i].replacement);
      84 +        if (t->defs[i].params) {                               
      85 +            for (int j = 0; j < t->defs[i].param_count; j++)   
      86 +                free(t->defs[i].params[j]);                    
      87 +            free(t->defs[i].params);                           
      88 +        }                                                      
      89      }
      90      free(t->defs);
      91  }
     ...
       99      return NULL;
      100  }
      101  
      102 +/* params and param_count describe the parameter list:        
      103 + *   param_count == -1, params == NULL  → object-like         
      104 + *   param_count >= 0                   → function-like (param
          +s owned)                                                      
      105 + * On compatible redefinition, incoming params are freed. */  
      106  static void macro_define(MacroTable *t, const char *name, size
           _t nlen,
      107 +                          char **params, int param_count,     
      108                            const char *repl, size_t rlen) {
      109      MacroDef *existing = macro_find(t, name, nlen);
      110      if (existing) {
      111 +        /* free any incoming params regardless of outcome */  
      112 +        if (params) {                                         
      113 +            for (int i = 0; i < param_count; i++) free(params[
          +i]);                                                          
      114 +            free(params);                                     
      115 +        }                                                     
      116          if (strlen(existing->replacement) == rlen &&
      117              strncmp(existing->replacement, repl, rlen) == 0)
      101 -            return;                                           
      118 +            return; /* compatible redefinition */             
      119          char tmp[256];
      120          size_t copy = nlen < sizeof(tmp) - 1 ? nlen : sizeof(t
           mp) - 1;
      121          memcpy(tmp, name, copy);
     ...
      140  
      141      t->defs[t->count].name        = nname;
      142      t->defs[t->count].replacement = nrepl;
      143 +    t->defs[t->count].params      = params; /* takes ownership
          + */                                                           
      144 +    t->defs[t->count].param_count = param_count;              
      145      t->count++;
      146  }
      147  
     ...
      163      return out;
      164  }
      165  
      147 -/* ---- Forward declaration for mutual recursion -------------
          ------------- */                                               
      166 +/* ---- Forward declarations ---------------------------------
          +----------- */                                                
      167  
      168  static char *preprocess_internal(const char *source, const cha
           r *source_path,
      169                                    int depth, MacroTable *macro
           s);
      170 +static char *expand_macros_text(const char *text, MacroTable *
          +macros);                                                      
      171  
      172  /* ---- Read and recursively preprocess an included file -----
           ----------- */
      173  
     ...
      190      return result;
      191  }
      192  
      193 +/* ---- Argument collection for function-like macro calls ----
          +----------- */                                                
      194 +                                                              
      195 +/* Collects arguments from s[*pos..], which must be positioned
          + immediately                                                  
      196 + * after the opening '('.  Advances *pos past the closing ')'.
      197 + * Returns args and lens arrays (caller frees elements and arr
          +ays). */                                                      
      198 +static void collect_args(const char *s, size_t *pos,          
      199 +                          char ***args_out, size_t **arg_lens_
          +out, int *count_out) {                                        
      200 +    size_t in = *pos;                                         
      201 +    size_t cap = 4;                                           
      202 +    char   **args = malloc(cap * sizeof(char *));             
      203 +    size_t  *lens = malloc(cap * sizeof(size_t));             
      204 +    if (!args || !lens) { fprintf(stderr, "error: out of memor
          +y\n"); exit(1); }                                             
      205 +    int count = 0;                                            
      206 +                                                              
      207 +    /* skip leading whitespace (including newlines after line 
          +splicing) */                                                  
      208 +    while (s[in] == ' ' || s[in] == '\t' || s[in] == '\n') in+
          ++;                                                            
      209 +                                                              
      210 +    if (s[in] == ')') {                                       
      211 +        /* empty argument list */                             
      212 +        in++;                                                 
      213 +        *pos = in;                                            
      214 +        *args_out = args;                                     
      215 +        *arg_lens_out = lens;                                 
      216 +        *count_out = 0;                                       
      217 +        return;                                               
      218 +    }                                                         
      219 +                                                              
      220 +    GrowBuf arg;                                              
      221 +    gbuf_init(&arg, 16);                                      
      222 +    int depth = 0;                                            
      223 +                                                              
      224 +    while (s[in]) {                                           
      225 +        char c = s[in];                                       
      226 +                                                              
      227 +        /* string literal: pass through verbatim, commas/paren
          +s inside don't count */                                       
      228 +        if (c == '"') {                                       
      229 +            gbuf_push(&arg, s[in++]);                         
      230 +            while (s[in] && s[in] != '"' && s[in] != '\n') {  
      231 +                if (s[in] == '\\' && s[in + 1]) gbuf_push(&arg
          +, s[in++]);                                                   
      232 +                gbuf_push(&arg, s[in++]);                     
      233 +            }                                                 
      234 +            if (s[in] == '"') gbuf_push(&arg, s[in++]);       
      235 +            continue;                                         
      236 +        }                                                     
      237 +                                                              
      238 +        /* character literal */                               
      239 +        if (c == '\'') {                                      
      240 +            gbuf_push(&arg, s[in++]);                         
      241 +            while (s[in] && s[in] != '\'' && s[in] != '\n') { 
      242 +                if (s[in] == '\\' && s[in + 1]) gbuf_push(&arg
          +, s[in++]);                                                   
      243 +                gbuf_push(&arg, s[in++]);                     
      244 +            }                                                 
      245 +            if (s[in] == '\'') gbuf_push(&arg, s[in++]);      
      246 +            continue;                                         
      247 +        }                                                     
      248 +                                                              
      249 +        if (c == '(') {                                       
      250 +            depth++;                                          
      251 +            gbuf_push(&arg, c);                               
      252 +            in++;                                             
      253 +        } else if (c == ')' && depth > 0) {                   
      254 +            depth--;                                          
      255 +            gbuf_push(&arg, c);                               
      256 +            in++;                                             
      257 +        } else if (c == ')') {                                
      258 +            /* depth == 0: close of argument list */          
      259 +            in++;                                             
      260 +            /* trim trailing whitespace */                    
      261 +            while (arg.len > 0 &&                             
      262 +                   (arg.data[arg.len - 1] == ' ' || arg.data[a
          +rg.len - 1] == '\t' ||                                        
      263 +                    arg.data[arg.len - 1] == '\n'))           
      264 +                arg.len--;                                    
      265 +            gbuf_push(&arg, '\0');                            
      266 +            if ((size_t)count == cap) {                       
      267 +                cap *= 2;                                     
      268 +                args = realloc(args, cap * sizeof(char *));   
      269 +                lens = realloc(lens, cap * sizeof(size_t));   
      270 +                if (!args || !lens) { fprintf(stderr, "error: 
          +out of memory\n"); exit(1); }                                 
      271 +            }                                                 
      272 +            size_t alen = arg.len - 1;                        
      273 +            args[count] = strdup(arg.data);                   
      274 +            lens[count] = alen;                               
      275 +            count++;                                          
      276 +            free(arg.data);                                   
      277 +            break;                                            
      278 +        } else if (c == ',' && depth == 0) {                  
      279 +            /* argument separator */                          
      280 +            while (arg.len > 0 &&                             
      281 +                   (arg.data[arg.len - 1] == ' ' || arg.data[a
          +rg.len - 1] == '\t' ||                                        
      282 +                    arg.data[arg.len - 1] == '\n'))           
      283 +                arg.len--;                                    
      284 +            gbuf_push(&arg, '\0');                            
      285 +            if ((size_t)count == cap) {                       
      286 +                cap *= 2;                                     
      287 +                args = realloc(args, cap * sizeof(char *));   
      288 +                lens = realloc(lens, cap * sizeof(size_t));   
      289 +                if (!args || !lens) { fprintf(stderr, "error: 
          +out of memory\n"); exit(1); }                                 
      290 +            }                                                 
      291 +            size_t alen = arg.len - 1;                        
      292 +            args[count] = strdup(arg.data);                   
      293 +            lens[count] = alen;                               
      294 +            count++;                                          
      295 +            free(arg.data);                                   
      296 +            gbuf_init(&arg, 16);                              
      297 +            in++;                                             
      298 +            /* skip leading whitespace for the next argument *
          +/                                                             
      299 +            while (s[in] == ' ' || s[in] == '\t' || s[in] == '
          +\n') in++;                                                    
      300 +        } else {                                              
      301 +            gbuf_push(&arg, c);                               
      302 +            in++;                                             
      303 +        }                                                     
      304 +    }                                                         
      305 +                                                              
      306 +    *pos = in;                                                
      307 +    *args_out = args;                                         
      308 +    *arg_lens_out = lens;                                     
      309 +    *count_out = count;                                       
      310 +}                                                             
      311 +                                                              
      312 +/* ---- Substitute parameters in replacement text ------------
          +----------- */                                                
      313 +                                                              
      314 +static char *substitute_args(const char *repl,                
      315 +                              char **params, int param_count, 
      316 +                              char **args, const size_t *arg_l
          +ens) {                                                        
      317 +    GrowBuf out;                                              
      318 +    gbuf_init(&out, strlen(repl) * 2 + 1);                    
      319 +    const char *r = repl;                                     
      320 +    while (*r) {                                              
      321 +        if (isalpha((unsigned char)*r) || *r == '_') {        
      322 +            const char *id_start = r;                         
      323 +            while (*r && (isalnum((unsigned char)*r) || *r == 
          +'_')) r++;                                                    
      324 +            size_t id_len = (size_t)(r - id_start);           
      325 +            int found = 0;                                    
      326 +            for (int i = 0; i < param_count; i++) {           
      327 +                if (strlen(params[i]) == id_len &&            
      328 +                    strncmp(params[i], id_start, id_len) == 0)
          + {                                                            
      329 +                    gbuf_append(&out, args[i], arg_lens[i]);  
      330 +                    found = 1;                                
      331 +                    break;                                    
      332 +                }                                             
      333 +            }                                                 
      334 +            if (!found) gbuf_append(&out, id_start, id_len);  
      335 +        } else {                                              
      336 +            gbuf_push(&out, *r++);                            
      337 +        }                                                     
      338 +    }                                                         
      339 +    gbuf_push(&out, '\0');                                    
      340 +    return out.data;                                          
      341 +}                                                             
      342 +                                                              
      343 +/* ---- Expand macro invocations in a plain text string ------
          +----------- */                                                
      344 +                                                              
      345 +/* Handles identifiers (object-like and function-like macros),
          + string and                                                   
      346 + * character literals (passed through verbatim).  Used to pre-
          +expand                                                        
      347 + * function-like macro arguments and to rescan substituted rep
          +lacement text. */                                             
      348 +static char *expand_macros_text(const char *text, MacroTable *
          +macros) {                                                     
      349 +    const char *s = text;                                     
      350 +    GrowBuf out;                                              
      351 +    gbuf_init(&out, strlen(s) + 1);                           
      352 +    size_t in = 0;                                            
      353 +                                                              
      354 +    while (s[in]) {                                           
      355 +        char c = s[in];                                       
      356 +                                                              
      357 +        /* string literal: pass through without expanding macr
          +os inside */                                                  
      358 +        if (c == '"') {                                       
      359 +            gbuf_push(&out, s[in++]);                         
      360 +            while (s[in] && s[in] != '"' && s[in] != '\n') {  
      361 +                if (s[in] == '\\' && s[in + 1]) gbuf_push(&out
          +, s[in++]);                                                   
      362 +                gbuf_push(&out, s[in++]);                     
      363 +            }                                                 
      364 +            if (s[in] == '"') gbuf_push(&out, s[in++]);       
      365 +            continue;                                         
      366 +        }                                                     
      367 +                                                              
      368 +        /* character literal */                               
      369 +        if (c == '\'') {                                      
      370 +            gbuf_push(&out, s[in++]);                         
      371 +            while (s[in] && s[in] != '\'' && s[in] != '\n') { 
      372 +                if (s[in] == '\\' && s[in + 1]) gbuf_push(&out
          +, s[in++]);                                                   
      373 +                gbuf_push(&out, s[in++]);                     
      374 +            }                                                 
      375 +            if (s[in] == '\'') gbuf_push(&out, s[in++]);      
      376 +            continue;                                         
      377 +        }                                                     
      378 +                                                              
      379 +        /* identifier: check for macro */                     
      380 +        if (isalpha((unsigned char)c) || c == '_') {          
      381 +            size_t id_start = in;                             
      382 +            while (s[in] && (isalnum((unsigned char)s[in]) || 
          +s[in] == '_'))                                                
      383 +                in++;                                         
      384 +            size_t id_len = in - id_start;                    
      385 +            MacroDef *def = macro_find(macros, s + id_start, i
          +d_len);                                                       
      386 +            if (def && def->param_count == -1) {              
      387 +                /* object-like macro */                       
      388 +                gbuf_append(&out, def->replacement, strlen(def
          +->replacement));                                              
      389 +            } else if (def && def->param_count >= 0) {        
      390 +                /* function-like macro: look ahead for '(' */ 
      391 +                size_t peek = in;                             
      392 +                while (s[peek] == ' ' || s[peek] == '\t') peek
          +++;                                                           
      393 +                if (s[peek] == '(') {                         
      394 +                    in = peek + 1; /* skip whitespace and '(' 
          +*/                                                            
      395 +                    char  **args     = NULL;                  
      396 +                    size_t *arg_lens = NULL;                  
      397 +                    int     arg_count = 0;                    
      398 +                    collect_args(s, &in, &args, &arg_lens, &ar
          +g_count);                                                     
      399 +                    if (arg_count != def->param_count) {      
      400 +                        fprintf(stderr,                       
      401 +                                "error: macro '%.*s' called wi
          +th %d argument%s,"                                            
      402 +                                " expected %d\n",             
      403 +                                (int)id_len, s + id_start,    
      404 +                                arg_count, arg_count == 1 ? ""
          + : "s",                                                       
      405 +                                def->param_count);            
      406 +                        for (int i = 0; i < arg_count; i++) fr
          +ee(args[i]);                                                  
      407 +                        free(args); free(arg_lens); free(out.d
          +ata);                                                         
      408 +                        exit(1);                              
      409 +                    }                                         
      410 +                    /* pre-expand each argument before substit
          +ution */                                                      
      411 +                    for (int i = 0; i < arg_count; i++) {     
      412 +                        char *ea = expand_macros_text(args[i],
          + macros);                                                     
      413 +                        free(args[i]);                        
      414 +                        args[i]     = ea;                     
      415 +                        arg_lens[i] = strlen(ea);             
      416 +                    }                                         
      417 +                    /* substitute args into replacement */    
      418 +                    char *subst = substitute_args(def->replace
          +ment,                                                         
      419 +                                                   def->params
          +, def->param_count,                                           
      420 +                                                   args, arg_l
          +ens);                                                         
      421 +                    /* rescan the substituted text */         
      422 +                    char *rescanned = expand_macros_text(subst
          +, macros);                                                    
      423 +                    gbuf_append(&out, rescanned, strlen(rescan
          +ned));                                                        
      424 +                    free(rescanned); free(subst);             
      425 +                    for (int i = 0; i < arg_count; i++) free(a
          +rgs[i]);                                                      
      426 +                    free(args); free(arg_lens);               
      427 +                } else {                                      
      428 +                    /* not followed by '(' — emit name unchang
          +ed */                                                         
      429 +                    gbuf_append(&out, s + id_start, id_len);  
      430 +                }                                             
      431 +            } else {                                          
      432 +                gbuf_append(&out, s + id_start, id_len);      
      433 +            }                                                 
      434 +            continue;                                         
      435 +        }                                                     
      436 +                                                              
      437 +        gbuf_push(&out, s[in++]);                             
      438 +    }                                                         
      439 +                                                              
      440 +    gbuf_push(&out, '\0');                                    
      441 +    return out.data;                                          
      442 +}                                                             
      443 +                                                              
      444  /* ---- Phase 2: strip comments, expand directives and macros 
           ----------- */
      445  
      446  static char *preprocess_internal(const char *source, const cha
           r *source_path,
     ...
      550                  continue;
      551              }
      552  
      282 -            /* #define NAME [replacement] */                  
      553 +            /* #define NAME[(params)] [replacement] */        
      554              if (strncmp(s + in, "define", 6) == 0 &&
      555                  !isalnum((unsigned char)s[in + 6]) && s[in + 6
           ] != '_') {
      556                  in += 6;
     ...
      567                      in++;
      568                  size_t name_len = in - name_start;
      569  
      299 -                /* Reject function-like macros. */            
      570 +                /* Determine if function-like: '(' immediately
          + after name (no space). */                                    
      571 +                char  **params      = NULL;                   
      572 +                int     param_count = -1; /* -1 = object-like 
          +*/                                                            
      573 +                                                              
      574                  if (s[in] == '(') {
      301 -                    fprintf(stderr, "error: function-like macr
          -os are not supported\n");                                     
      302 -                    free(out.data);                           
      303 -                    free(spliced);                            
      304 -                    exit(1);                                  
      575 +                    in++; /* skip '(' */                      
      576 +                    param_count = 0;                          
      577 +                    size_t params_cap = 4;                    
      578 +                    params = malloc(params_cap * sizeof(char *
          +));                                                           
      579 +                    if (!params) { fprintf(stderr, "error: out
          + of memory\n"); exit(1); }                                    
      580 +                                                              
      581 +                    /* skip whitespace */                     
      582 +                    while (s[in] == ' ' || s[in] == '\t') in++
          +;                                                             
      583 +                                                              
      584 +                    if (s[in] == ')') {                       
      585 +                        /* zero-parameter macro */            
      586 +                        in++;                                 
      587 +                    } else {                                  
      588 +                        /* parse parameter name list */       
      589 +                        while (1) {                           
      590 +                            if (!isalpha((unsigned char)s[in])
          + && s[in] != '_') {                                           
      591 +                                fprintf(stderr,               
      592 +                                        "error: expected param
          +eter name in macro definition\n");                            
      593 +                                for (int i = 0; i < param_coun
          +t; i++) free(params[i]);                                      
      594 +                                free(params);                 
      595 +                                free(out.data); free(spliced);
      596 +                                exit(1);                      
      597 +                            }                                 
      598 +                            size_t pstart = in;               
      599 +                            while (s[in] && (isalnum((unsigned
          + char)s[in]) || s[in] == '_'))                                
      600 +                                in++;                         
      601 +                            size_t plen = in - pstart;        
      602 +                            char *pname = malloc(plen + 1);   
      603 +                            if (!pname) { fprintf(stderr, "err
          +or: out of memory\n"); exit(1); }                             
      604 +                            memcpy(pname, s + pstart, plen);  
      605 +                            pname[plen] = '\0';               
      606 +                                                              
      607 +                            if ((size_t)param_count == params_
          +cap) {                                                        
      608 +                                params_cap *= 2;              
      609 +                                params = realloc(params, param
          +s_cap * sizeof(char *));                                      
      610 +                                if (!params) { fprintf(stderr,
          + "error: out of memory\n"); exit(1); }                        
      611 +                            }                                 
      612 +                            params[param_count++] = pname;    
      613 +                                                              
      614 +                            while (s[in] == ' ' || s[in] == '\
          +t') in++;                                                     
      615 +                            if (s[in] == ')') { in++; break; }
      616 +                            if (s[in] == ',') {               
      617 +                                in++;                         
      618 +                                while (s[in] == ' ' || s[in] =
          += '\t') in++;                                                 
      619 +                                continue;                     
      620 +                            }                                 
      621 +                            fprintf(stderr,                   
      622 +                                    "error: expected ',' or ')
          +' in macro parameter list\n");                                
      623 +                            for (int i = 0; i < param_count; i
          +++) free(params[i]);                                          
      624 +                            free(params);                     
      625 +                            free(out.data); free(spliced);    
      626 +                            exit(1);                          
      627 +                        }                                     
      628 +                    }                                         
      629                  }
      630  
      307 -                /* Skip whitespace separating name from replac
          -ement. */                                                     
      631 +                /* Skip whitespace separating name/params from
          + replacement. */                                              
      632                  while (s[in] == ' ' || s[in] == '\t') in++;
      633  
      634                  /* Replacement: rest of line, trailing whitesp
           ace trimmed. */
     ...
      641  
      642                  macro_define(macros,
      643                               s + name_start, name_len,
      644 +                             params, param_count,             
      645                               s + repl_start, repl_end - repl_s
           tart);
      646                  /* Directive line consumed; newline handled on
            next iteration. */
      647                  continue;
     ...
      689                  in++;
      690              size_t id_len = in - id_start;
      691              MacroDef *def = macro_find(macros, s + id_start, i
           d_len);
      367 -            if (def) {                                        
      692 +            if (def && def->param_count == -1) {              
      693 +                /* object-like macro: expand replacement direc
          +tly */                                                        
      694                  gbuf_append(&out, def->replacement, strlen(def
           ->replacement));
      695 +            } else if (def && def->param_count >= 0) {        
      696 +                /* function-like macro: look ahead for '(' */ 
      697 +                size_t peek = in;                             
      698 +                while (s[peek] == ' ' || s[peek] == '\t') peek
          +++;                                                           
      699 +                if (s[peek] == '(') {                         
      700 +                    in = peek + 1; /* skip optional whitespace
          + and '(' */                                                   
      701 +                    char  **args      = NULL;                 
      702 +                    size_t *arg_lens  = NULL;                 
      703 +                    int     arg_count = 0;                    
      704 +                    collect_args(s, &in, &args, &arg_lens, &ar
          +g_count);                                                     
      705 +                    if (arg_count != def->param_count) {      
      706 +                        fprintf(stderr,                       
      707 +                                "error: macro '%.*s' called wi
          +th %d argument%s,"                                            
      708 +                                " expected %d\n",             
      709 +                                (int)id_len, s + id_start,    
      710 +                                arg_count, arg_count == 1 ? ""
          + : "s",                                                       
      711 +                                def->param_count);            
      712 +                        for (int i = 0; i < arg_count; i++) fr
          +ee(args[i]);                                                  
      713 +                        free(args); free(arg_lens);           
      714 +                        free(out.data); free(spliced);        
      715 +                        exit(1);                              
      716 +                    }                                         
      717 +                    /* pre-expand each argument */            
      718 +                    for (int i = 0; i < arg_count; i++) {     
      719 +                        char *ea = expand_macros_text(args[i],
          + macros);                                                     
      720 +                        free(args[i]);                        
      721 +                        args[i]     = ea;                     
      722 +                        arg_lens[i] = strlen(ea);             
      723 +                    }                                         
      724 +                    /* substitute into replacement, then resca
          +n */                                                          
      725 +                    char *subst     = substitute_args(def->rep
          +lacement,                                                     
      726 +                                                       def->pa
          +rams, def->param_count,                                       
      727 +                                                       args, a
          +rg_lens);                                                     
      728 +                    char *rescanned = expand_macros_text(subst
          +, macros);                                                    
      729 +                    gbuf_append(&out, rescanned, strlen(rescan
          +ned));                                                        
      730 +                    free(rescanned); free(subst);             
      731 +                    for (int i = 0; i < arg_count; i++) free(a
          +rgs[i]);                                                      
      732 +                    free(args); free(arg_lens);               
      733 +                } else {                                      
      734 +                    /* function-like macro not followed by '('
          + — pass through */                                            
      735 +                    gbuf_append(&out, s + id_start, id_len);  
      736 +                }                                             
      737              } else {
      738                  gbuf_append(&out, s + id_start, id_len);
      739              }

● Now build and check for compile errors:

● Bash(cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -5 && cmake
      --build build 2>&1)
  ⎿  -- Configuring done
     -- Generating done
     -- Build files have been written to: /home/scott/code/claude/claudec99
     … +5 lines (ctrl+o to expand)

● Clean build. Now write the test files:

● Write(test/valid/test_pp_fn_macro_add__42.c)
  ⎿  Wrote 5 lines to test/valid/test_pp_fn_macro_add__42.c
      1 #define ADD(a,b)  ((a) + (b))
      2 
      3 int main() {
      4     return ADD(20,22);
      5 }

● Write(test/valid/test_pp_fn_macro_square__64.c)
  ⎿  Wrote 5 lines to test/valid/test_pp_fn_macro_square__64.c
      1 #define SQUARE(x)  ((x) * (x))
      2 
      3 int main() {
      4     return SQUARE(8);
      5 }

● Write(test/valid/test_pp_fn_macro_zero_arg__42.c)
  ⎿  Wrote 5 lines to test/valid/test_pp_fn_macro_zero_arg__42.c
      1 #define ANSWER() 42
      2 
      3 int main() {
      4     return ANSWER();
      5 }

● Write(test/valid/test_pp_fn_macro_expr_arg__6.c)
  ⎿  Wrote 5 lines to test/valid/test_pp_fn_macro_expr_arg__6.c
      1 #define ADD(a,b) ((a) + (b))
      2 
      3 int main() {
      4     return ADD(1+2,3);
      5 }

● Write(test/valid/test_pp_fn_macro_nested__6.c)
  ⎿  Wrote 5 lines to test/valid/test_pp_fn_macro_nested__6.c
      1 #define ADD(a,b) ((a) + (b))
      2 
      3 int main() {
      4     return ADD(ADD(1,2),3);
      5 }

● Now find the next available invalid test number and write the argument count
  mismatch test:

● Bash(ls /home/scott/code/claude/claudec99/test/invalid/ | grep -o
      'test_invalid_[0-9]*' | sed 's/test_invalid_//' | sort -n | tail -3)  ⎿  136
     137
     138

● Write(test/invalid/test_invalid_139__wrong_arg_count_for_function_like_macro.c
       )
  ⎿  Wrote 5 lines to test/invalid/test_invalid_139__wrong_arg_count_for_fu
     nction_like_macro.c
      1 #define ADD(a,b) ((a) + (b))
      2 
      3 int main() {
      4     return ADD(1,2,3,4);
      5 }

● Now run the full test suite:

● Bash(./test/run_all_tests.sh 2>&1 | tail -20)
  ⎿  PASS  test_stage_14_07_libc_puts
     PASS  test_stage_16_01_remainder_int
     PASS  test_stage_16_01_remainder_long
     … +17 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● One failure. Let me find it:

● Bash(./test/run_all_tests.sh 2>&1 | grep FAIL)
  ⎿  FAIL  test_invalid_139__wrong_arg_count_for_function_like_macro  (expe
     cted error containing 'wrong arg count for function like macro', got:
     error: macro 'ADD' called with 4 arguments, expected 2)
  ⎿  (timeout 5m)

● The invalid test runner checks that the error message contains a string
  derived from the filename. Let me look at how invalid tests work:

  Read 1 file (ctrl+o to expand)

● The invalid test's filename pattern doesn't match the error message. Let me
  check a similar test to see the pattern:

  Read 1 file (ctrl+o to expand)

● Bash(./build/ccompiler
      test/invalid/test_invalid_132__indirect_call_expects_2_arguments.c 2>&1 |
      head -3)
  ⎿  error: indirect call expects 2 arguments, got 1

● The filename fragment wrong arg count for function like macro doesn't match
  the actual error. I'll rename the test file and adjust the error message to
  align:

● Bash(mv /home/scott/code/claude/claudec99/test/invalid/test_invalid_139__wrong
      _arg_count_for_function_like_macro.c \
         /home/scott/code/claude/claudec99/test/invali…)
  ⎿  Done

● Now update the error messages in preprocessor.c to say "argument count
  mismatch for macro":

● Update(src/preprocessor.c)
  ⎿  Added 3 lines, removed 4 lines
      398                      collect_args(s, &in, &args, &arg_lens, &ar
           g_count);
      399                      if (arg_count != def->param_count) {
      400                          fprintf(stderr,
      401 -                                "error: macro '%.*s' called wi
          -th %d argument%s,"                                            
      402 -                                " expected %d\n",             
      401 +                                "error: argument count mismatc
          +h for macro '%.*s':"                                          
      402 +                                " expected %d, got %d\n",     
      403                                  (int)id_len, s + id_start,
      404 -                                arg_count, arg_count == 1 ? ""
          - : "s",                                                       
      405 -                                def->param_count);            
      404 +                                def->param_count, arg_count); 
      405                          for (int i = 0; i < arg_count; i++) fr
           ee(args[i]);
      406                          free(args); free(arg_lens); free(out.d
           ata);
      407                          exit(1);

● There are two error-message sites (one in expand_macros_text, one in
  preprocess_internal). Update the second one too:

● Update(src/preprocessor.c)
  ⎿  Added 3 lines, removed 4 lines
      703                      collect_args(s, &in, &args, &arg_lens, &ar
           g_count);
      704                      if (arg_count != def->param_count) {
      705                          fprintf(stderr,
      706 -                                "error: macro '%.*s' called wi
          -th %d argument%s,"                                            
      707 -                                " expected %d\n",             
      706 +                                "error: argument count mismatc
          +h for macro '%.*s':"                                          
      707 +                                " expected %d, got %d\n",     
      708                                  (int)id_len, s + id_start,
      709 -                                arg_count, arg_count == 1 ? ""
          - : "s",                                                       
      710 -                                def->param_count);            
      709 +                                def->param_count, arg_count); 
      710                          for (int i = 0; i < arg_count; i++) fr
           ee(args[i]);
      711                          free(args); free(arg_lens);
      712                          free(out.data); free(spliced);

● Bash(cmake --build build 2>&1 && echo "BUILD OK")
  ⎿  Consolidate compiler generated dependencies of target ccompiler
     [ 10%] Building C object CMakeFiles/ccompiler.dir/src/preprocessor.c.o
     [ 20%] Linking C executable ccompiler
     … +2 lines (ctrl+o to expand)

● Bash(./test/run_all_tests.sh 2>&1 | tail -10)
  ⎿  PASS  test_stage_16_04_bitor_int
     PASS  test_stage_16_04_bitxor_int
     PASS  test_stage_22_01_global_bss_layout
     … +7 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● All 913 tests pass. Let me verify the new tests individually:

● Bash(for f in test/valid/test_pp_fn_macro_*.c; do
        name=$(basename "$f" .c)…)
  ⎿  usage: ccompiler [--print-ast | --print-tokens] <source.c>
     FAIL test_pp_fn_macro_add__42 (expected=42, got=127)
     usage: ccompiler [--print-ast | --print-tokens] <source.c>
     … +7 lines (ctrl+o to expand)

● The compiler doesn't take -o. Let me check how the valid test runner works:

  Read 1 file (ctrl+o to expand)

● The test runner works from SCRIPT_DIR, which is important. Let me run the
  valid tests directly:

● Bash(./test/valid/run_tests.sh 2>&1 | grep "pp_fn_macro")  ⎿  compiled: /home/scott/code/claude/claudec99/test/valid/test_pp_fn_macr
     o_add__42.c -> test_pp_fn_macro_add__42.asm
     PASS  test_pp_fn_macro_add__42  (exit code: 42)
     … +12 lines (ctrl+o to expand)
  ⎿  (timeout 1m)

● Bash(./test/invalid/run_tests.sh 2>&1 | grep "139")
  ⎿  PASS  test_invalid_139__argument_count_mismatch_for_macro  (error cont
     ains: 'argument count mismatch for macro')

● All 5 valid tests and the 1 invalid test pass. Full suite: 913/913. Now
  delegate the milestone, transcript, grammar, and README updates to haiku:

● haiku-stage-artifact-writer(Generate stage-51 milestone, transcript, grammar
                             and README updates)
  ⎿  Done (11 tool uses · 19.0k tokens · 37s)
  (ctrl+o to expand)

● Stage 51 is complete. Here's a summary of what was done:

  Implementation — all changes in src/preprocessor.c:
  - Extended MacroDef with params[] and param_count (-1 = object-like, ≥0 =
  function-like)
  - #define parsing now detects NAME( (no space) and parses the parameter list
  - New collect_args(): scans argument lists, splits at depth-0 commas, handles
  nested parentheses
  - New substitute_args(): replaces parameter names in replacement text with
  argument text
  - New expand_macros_text(): recursively expands macros in a text string;
  pre-expands arguments and rescans substituted text (handles ADD(ADD(1,2),3)
  naturally)
  - Removed the old "Reject function-like macros" error block

  Tests added (6 new, all pass):
  - test_pp_fn_macro_add__42 — basic two-arg macro
  - test_pp_fn_macro_square__64 — single-arg macro (spec typo X → x corrected)
  - test_pp_fn_macro_zero_arg__42 — zero-parameter macro
  - test_pp_fn_macro_expr_arg__6 — expression as argument
  - test_pp_fn_macro_nested__6 — nested invocation
  - test_invalid_139__argument_count_mismatch_for_macro — too many args rejected

  Final result: 913 passed, 0 failed, 913 total (up from 907).

✻ Cooked for 12m 1s

※ recap: Implemented Stage 51 function-like macros in the preprocessor. All 913 tests pass. Next: commit the changes. (disable recaps in /config)

❯ have the changes been committed                                               

● Bash(git status && git log --oneline -3)
  ⎿  On branch master
     Your branch is up to date with 'origin/master'.
     … +21 lines (ctrl+o to expand)

● Not committed yet. Shall I commit these changes now?

✻ Worked for 4s

❯ yes commit the changes                                                        

● Bash(git diff README.md src/preprocessor.c | head -60)
  ⎿  diff --git a/README.md b/README.md
     index 51c7ee4..15fe9ae 100644
     --- a/README.md
     … +52 lines (ctrl+o to expand)

● Bash(git log --oneline -5)
  ⎿  4830003 stage 51 spec
     65e6734 stage 50 session export and usage
     328ff7e stage 50 session export and usage
     … +2 lines (ctrl+o to expand)

● Bash(git add src/preprocessor.c README.md \
        docs/kickoffs/stage-51-kickoff.md \…)
  ⎿  [master d96aba4] stage 51 function-like macros
      11 files changed, 561 insertions(+), 18 deletions(-)
      create mode 100644 docs/kickoffs/stage-51-kickoff.md
     … +9 lines (ctrl+o to expand)

● Committed as d96aba4 — 11 files, 561 insertions.

✻ Crunched for 11s