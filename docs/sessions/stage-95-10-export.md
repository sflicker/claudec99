 ▐▛███▜▌   Claude Code v2.1.168
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

  Feature of the week: /loop — run a prompt or slash command on a recurring interval


❯ /usage 
  ⎿  Settings dialog dismissed

❯ /implement-stage docs/stages/ClaudeC99-spec-stage-95-10-remove-static-char-ar 
rays-from-parser-h.md                                                           

● I'll start by reading the spec file and supporting files in parallel.

  Read 4 files (ctrl+o to expand)

● Now let me read parser.h and the fixed-capacity-inventory to understand the
  scope of changes.

  Searched for 2 patterns, read 2 files (ctrl+o to expand)

● Let me also check codegen.h and the full parser.c to understand all the
  usages.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me read the key sections of parser.c to understand the copy operations.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me check the type.h file to understand StructField and look at specific
  parser.c sections more carefully.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Let me check codegen.c usage of the parser.h structs to understand the full
  impact.

  Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Now I have a complete picture. Let me derive the stage label and produce the
  summary.

  Read 1 file (ctrl+o to expand)

● STAGE_DISPLAY: Stage 95-10

  Now I have the full picture. Let me delegate the kickoff artifact and start
  planning.

  Stage 95-10 Summary:

  Convert 7 char field[MAX_NAME_LEN] embedded arrays in parser.h structs to
  const char * pointers, and replace all strncpy/copy operations in parser.c
  with direct pointer assignments. The data is safe to point to because
  Token.value (since stage 95-08) is persistent lexer-pool storage.

  Structs to convert (7 items):
  1. TypedefEntry.name
  2. FuncSig.name
  3. GlobalObjSig.name
  4. StructTag.tag
  5. UnionTag.tag
  6. EnumTag.tag
  7. EnumConst.name

  Planned Changes:
  - include/parser.h: 7 field type changes
  - src/parser.c: replace strncpy/memset patterns with pointer assignments;
  local char tag[256] / char ename[256] buffers → const char * pointing directly
  to lexer storage
  - src/version.c: update stage to 00951000
  - docs/fixed-capacity-inventory.md: mark MAX_NAME_LEN (parser.h portion) as
  done

  No ambiguities - the spec is clear. StructField (in type.h) is out of scope
  since the spec says "parser.h has numerous struct".

  Delegating the kickoff artifact now, then implementing one struct at a time.

● haiku-stage-artifact-writer(Generate stage-95-10 kickoff artifact)
  ⎿  Done (10 tool uses · 8.5k tokens · 37m 58s)
  (ctrl+o to expand)

● Now I'll implement the conversions one at a time. Starting with item 1:
  TypedefEntry.name.

  Read 1 file (ctrl+o to expand)

● Conversion 1: TypedefEntry.name

  Change the header field, then update the strncpy in parser_register_typedef.

● Update(include/parser.h)
  ⎿  Added 1 line, removed 1 line
      32  
      33  /* Stage 28-01/28-02: entry in the parser-level typedef table. 
          */
      34  typedef struct {
      35 -    char         name[MAX_NAME_LEN];                           
      35 +    const char  *name;                                         
      36      TypeKind     kind;       /* outermost kind; TYPE_POINTER fo
          r pointer typedefs */
      37      struct Type *full_type;  /* NULL for scalar typedefs; point
          er chain for pointer typedefs */
      38      int          scope_depth; /* 0 = file scope, 1+ = block sco
          pe */

● Now update the strncpy in parser_register_typedef (lines 114-115):
● Update(src/parser.c)
  ⎿  Added 1 line, removed 2 lines
      111      {
      112          TypedefEntry entry;
      113          memset(&entry, 0, sizeof(entry));
      114 -        strncpy(entry.name, name, sizeof(entry.name) - 1);    
      115 -        entry.name[sizeof(entry.name) - 1] = '\0';            
      114 +        entry.name = name;                                    
      115          entry.kind = kind;
      116          entry.full_type = full_type;
      117          entry.scope_depth = parser->scope_depth;

● Bash(cmake -S . -B build -q 2>&1 | tail -5 && cmake --build build 2>&1 | grep
      -E "error:|warning:|Error" | head -20)
  ⎿  CMake Error: Unknown argument -q
     CMake Error: Run 'cmake --help' for all supported options.
     /home/scott/code/claude/claudec99/src/compiler.c:22:5: warning: enumer
     ation value ‘TOKEN_UNION’ not handled in switch [-Wswitch]

● Bash(cmake --build build 2>&1 | grep "error:" | head -20)
  ⎿  (No output)

● Bash(./test/run_all_tests.sh 2>&1 | tail -5)
  ⎿  Results: 21 passed, 0 failed, 21 total

     ====================================================
     … +2 lines (ctrl+o to expand)

● All 1479 tests pass. Committing conversion 1.

● Bash(git add include/parser.h src/parser.c && git commit -m "$(cat <<'EOF'
      stage 95-10: convert TypedefEntry.name from char[] to const char *…)
  ⎿  [master 05c964b] stage 95-10: convert TypedefEntry.name from char[] to
      const char *
      2 files changed, 2 insertions(+), 3 deletions(-)

● Conversion 2: FuncSig.name

● Update(include/parser.h)
  ⎿  Added 1 line, removed 1 line
      39  } TypedefEntry;
      40  
      41  typedef struct {
      42 -    char name[MAX_NAME_LEN];                                   
      42 +    const char *name;                                          
      43      int param_count;
      44      int has_definition;
      45      TypeKind return_type;

● Update(src/parser.c)
  ⎿  Added 1 line, removed 2 lines
      301      {
      302          FuncSig sig;
      303          memset(&sig, 0, sizeof(sig));
      304 -        strncpy(sig.name, name, sizeof(sig.name) - 1);        
      305 -        sig.name[sizeof(sig.name) - 1] = '\0';                
      304 +        sig.name = name;                                      
      305          sig.param_count = param_count;
      306          sig.has_definition = is_definition;
      307          sig.return_type = return_type;

● Bash(cmake --build build 2>&1 | grep "error:" | head -20 &&
      ./test/run_all_tests.sh 2>&1 | tail -3)
  ⎿  ====================================================
     Aggregate: 1479 passed, 0 failed, 1479 total
     ====================================================

● Bash(git add include/parser.h src/parser.c && git commit -m "$(cat <<'EOF'
      stage 95-10: convert FuncSig.name from char[] to const char *…)
  ⎿  [master e5e1b67] stage 95-10: convert FuncSig.name from char[] to cons
     t char *
      2 files changed, 2 insertions(+), 3 deletions(-)

● Conversion 3: GlobalObjSig.name

● Update(include/parser.h)
  ⎿  Added 1 line, removed 1 line
      57   * Stage 25-01: full_type carries the complete type chain for f
          unction-pointer
      58   * globals so compatibility between successive declarations can
           be verified. */
      59  typedef struct {
      60 -    char name[MAX_NAME_LEN];                                   
      60 +    const char *name;                                          
      61      TypeKind kind;
      62      StorageClass storage_class;
      63      struct Type *full_type;

● Update(src/parser.c)
  ⎿  Added 1 line, removed 2 lines
      226          return;
      227      }
      228      GlobalObjSig new_g;
      229 -    strncpy(new_g.name, name, 255);                           
      230 -    new_g.name[255] = '\0';                                   
      229 +    new_g.name = name;                                        
      230      new_g.kind = kind;
      231      new_g.storage_class = sc;
      232      new_g.full_type = full_type;

● Bash(cmake --build build 2>&1 | grep "error:" | head -20 &&
      ./test/run_all_tests.sh 2>&1 | tail -3)
  ⎿  ====================================================
     Aggregate: 1479 passed, 0 failed, 1479 total
     ====================================================

● Bash(git add include/parser.h src/parser.c && git commit -m "$(cat <<'EOF'
      stage 95-10: convert GlobalObjSig.name from char[] to const char *…)
  ⎿  [master 7115946] stage 95-10: convert GlobalObjSig.name from char[] to
      const char *
      2 files changed, 2 insertions(+), 3 deletions(-)

● Conversion 4: StructTag.tag

  This one also requires changing the local char tag[256] in
  parse_struct_specifier to const char *.

● Update(include/parser.h)
  ⎿  Added 1 line, removed 1 line
      20  
      21  /* Stage 30: one entry per named struct tag in the translation 
          unit. */
      22  typedef struct {
      23 -    char         tag[MAX_NAME_LEN];                            
      23 +    const char  *tag;                                          
      24      struct Type *type;   /* NULL until the struct body is parse
          d */
      25  } StructTag;
      26  

● Now update parser_register_struct_tag in parser.c:

● Update(src/parser.c)
  ⎿  Added 1 line, removed 2 lines
      344      StructTag *st = parser_find_struct_tag(parser, tag);
      345      if (st) return st;
      346      StructTag new_st;
      347 +    new_st.tag = tag;                                         
      348      new_st.type = NULL;
      348 -    strncpy(new_st.tag, tag, sizeof(new_st.tag) - 1);         
      349 -    new_st.tag[sizeof(new_st.tag) - 1] = '\0';                
      349      vec_push(&parser->struct_tags, &new_st);
      350      return (StructTag *)vec_get(&parser->struct_tags, parser->
           struct_tags.len - 1);
      351  }

● Now update parse_struct_specifier to use const char *tag instead of char 
  tag[256]:

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 3 lines
      384      parser->current = lexer_next_token(parser->lexer);
      385  
      386      int has_tag = (parser->current.type == TOKEN_IDENTIFIER);
      387 -    char tag[256] = "";                                       
      387 +    const char *tag = NULL;                                   
      388      StructTag *st = NULL;
      389  
      390      if (has_tag) {
      391 -        strncpy(tag, parser->current.value, sizeof(tag) - 1); 
      392 -        tag[sizeof(tag) - 1] = '\0';                          
      391 +        tag = parser->current.value;                          
      392          parser->current = lexer_next_token(parser->lexer);
      393          st = parser_register_struct_tag(parser, tag);
      394      } else if (parser->current.type != TOKEN_LBRACE) {

● Bash(cmake --build build 2>&1 | grep "error:" | head -20 &&
      ./test/run_all_tests.sh 2>&1 | tail -3)
  ⎿  ====================================================
     Aggregate: 1479 passed, 0 failed, 1479 total
     ====================================================
● Bash(git add include/parser.h src/parser.c && git commit -m "$(cat <<'EOF'
      stage 95-10: convert StructTag.tag from char[] to const char *…)
  ⎿  [master f77d3ca] stage 95-10: convert StructTag.tag from char[] to con
     st char *
      2 files changed, 4 insertions(+), 6 deletions(-)

● Conversion 5: UnionTag.tag

● Update(include/parser.h)
  ⎿  Added 1 line, removed 1 line
      26  
      27  /* Stage 72: one entry per named union tag in the translation u
          nit. */
      28  typedef struct {
      29 -    char         tag[MAX_NAME_LEN];                            
      29 +    const char  *tag;                                          
      30      struct Type *type;   /* NULL until the union body is parsed
           */
      31  } UnionTag;
      32  

● Now update parser_register_union_tag and parse_union_specifier:

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 6 lines
      563      UnionTag *ut = parser_find_union_tag(parser, tag);
      564      if (ut) return ut;
      565      UnionTag new_ut;
      566 -    new_ut.tag[0] = '\0';                                     
      566 +    new_ut.tag = tag;                                         
      567      new_ut.type = NULL;
      568      vec_push(&parser->union_tags, &new_ut);
      569 -    ut = (UnionTag *)vec_get(&parser->union_tags, parser->unio
          -n_tags.len - 1);                                              
      570 -    strncpy(ut->tag, tag, sizeof(ut->tag) - 1);               
      571 -    ut->tag[sizeof(ut->tag) - 1] = '\0';                      
      572 -    ut->type = NULL;                                          
      573 -    return ut;                                                
      569 +    return (UnionTag *)vec_get(&parser->union_tags, parser->un
          +ion_tags.len - 1);                                            
      570  }
      571  
      572  /*

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 3 lines
      584      parser->current = lexer_next_token(parser->lexer);
      585  
      586      int has_tag = (parser->current.type == TOKEN_IDENTIFIER);
      587 -    char tag[256] = "";                                       
      587 +    const char *tag = NULL;                                   
      588      UnionTag *ut = NULL;
      589  
      590      if (has_tag) {
      591 -        strncpy(tag, parser->current.value, sizeof(tag) - 1); 
      592 -        tag[sizeof(tag) - 1] = '\0';                          
      591 +        tag = parser->current.value;                          
      592          parser->current = lexer_next_token(parser->lexer);
      593          ut = parser_register_union_tag(parser, tag);
      594      } else if (parser->current.type != TOKEN_LBRACE) {

● Bash(cmake --build build 2>&1 | grep "error:" | head -20 &&
      ./test/run_all_tests.sh 2>&1 | tail -3)
  ⎿  ====================================================
     Aggregate: 1479 passed, 0 failed, 1479 total
     ====================================================

● Bash(git add include/parser.h src/parser.c && git commit -m "$(cat <<'EOF'
      stage 95-10: convert UnionTag.tag from char[] to const char *…)
  ⎿  [master 58fef0a] stage 95-10: convert UnionTag.tag from char[] to cons
     t char *
      2 files changed, 5 insertions(+), 10 deletions(-)

● Conversion 6: EnumTag.tag

● Update(include/parser.h)
  ⎿  Added 2 lines, removed 2 lines
      14  } EnumConst;
      15  
      16  typedef struct {
      17 -    char tag[MAX_NAME_LEN];                                    
      18 -    int  is_defined;                                           
      17 +    const char *tag;                                           
      18 +    int         is_defined;                                    
      19  } EnumTag;
      20  
      21  /* Stage 30: one entry per named struct tag in the translation 
          unit. */

● Now update parse_enum_specifier — the local char tag[256] buffer and the
  EnumTag registration:

  Read 1 file (ctrl+o to expand)

● Update(src/parser.c)
  ⎿  Added 2 lines, removed 3 lines
      747      /* consume 'enum' */
      748      parser->current = lexer_next_token(parser->lexer);
      749  
      750 -    char tag[256] = "";                                       
      750 +    const char *tag = NULL;                                   
      751      int has_tag = 0;
      752  
      753      if (parser->current.type == TOKEN_IDENTIFIER) {
      754 -        strncpy(tag, parser->current.value, sizeof(tag) - 1); 
      755 -        tag[sizeof(tag) - 1] = '\0';                          
      754 +        tag = parser->current.value;                          
      755          has_tag = 1;
      756          parser->current = lexer_next_token(parser->lexer);
      757      }

● Now fix the EnumTag registration block (the two-step push+strncpy):

● Update(src/parser.c)
  ⎿  Added 1 line, removed 3 lines
      771              }
      772              if (!et) {
      773                  EnumTag new_et;
      774 -                new_et.tag[0] = '\0';                         
      774 +                new_et.tag = tag;                             
      775                  new_et.is_defined = 0;
      776                  vec_push(&parser->enum_tags, &new_et);
      777                  et = (EnumTag *)vec_get(&parser->enum_tags,
      778                                          parser->enum_tags.len 
           - 1);
      779 -                strncpy(et->tag, tag, sizeof(et->tag) - 1);   
      780 -                et->tag[sizeof(et->tag) - 1] = '\0';          
      779              }
      780              et->is_defined = 1;
      781          }

● Bash(cmake --build build 2>&1 | grep "error:" | head -20 &&
      ./test/run_all_tests.sh 2>&1 | tail -3)
  ⎿  ====================================================
     Aggregate: 1479 passed, 0 failed, 1479 total
     ====================================================

● Bash(git add include/parser.h src/parser.c && git commit -m "$(cat <<'EOF'
      stage 95-10: convert EnumTag.tag from char[] to const char *…)
  ⎿  [master aae2614] stage 95-10: convert EnumTag.tag from char[] to const
      char *
      2 files changed, 5 insertions(+), 8 deletions(-)

● Conversion 7: EnumConst.name

● Update(include/parser.h)
  ⎿  Added 2 lines, removed 2 lines
       9  #include <stdio.h>
      10  
      11  typedef struct {
      12 -    char name[MAX_NAME_LEN];                                   
      13 -    long value;                                                
      12 +    const char *name;                                          
      13 +    long        value;                                         
      14  } EnumConst;
      15  
      16  typedef struct {

● Now update parse_enum_specifier — the local char ename[256] buffer and the
  EnumConst registration:

● Update(src/parser.c)
  ⎿  Added 1 line, removed 3 lines
      787                  PARSER_ERROR(parser, "error: expected identifi
           er in enumerator list, got '%s'\n",
      788                          parser->current.value);
      789              }
      790 -            char ename[256];                                  
      791 -            strncpy(ename, parser->current.value, sizeof(ename
          -) - 1);                                                       
      792 -            ename[sizeof(ename) - 1] = '\0';                  
      790 +            const char *ename = parser->current.value;        
      791              parser->current = lexer_next_token(parser->lexer);
      792  
      793              for (size_t i = 0; i < parser->enum_consts.len; i+
           +) {

● Update(src/parser.c)
  ⎿  Added 1 line, removed 2 lines
      818              }
      819  
      820              EnumConst new_ec;
      821 -            strncpy(new_ec.name, ename, sizeof(new_ec.name) - 
          -1);                                                           
      822 -            new_ec.name[sizeof(new_ec.name) - 1] = '\0';      
      821 +            new_ec.name = ename;                              
      822              new_ec.value = next_val++;
      823              vec_push(&parser->enum_consts, &new_ec);
      824  

● Bash(cmake --build build 2>&1 | grep "error:" | head -20 &&
      ./test/run_all_tests.sh 2>&1 | tail -3)
  ⎿  ====================================================
     Aggregate: 1479 passed, 0 failed, 1479 total
     ====================================================

● All 7 conversions complete. Committing the last one:

● Bash(git add include/parser.h src/parser.c && git commit -m "$(cat <<'EOF'
      stage 95-10: convert EnumConst.name from char[] to const char *…)  ⎿  [master 51738c9] stage 95-10: convert EnumConst.name from char[] to co
     nst char *
      2 files changed, 4 insertions(+), 7 deletions(-)

● All 7 conversions done. Now update version.c and the fixed-capacity inventory.

  Update version.c (stage 95-10 → 00951000):

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      3  
      4  #define VERSION_MAJOR  "00"
      5  #define VERSION_MINOR  "02"
      6 -#define VERSION_STAGE  "00950900"                               
      6 +#define VERSION_STAGE  "00951000"                               
      7  
      8  #ifndef VERSION_BUILD
      9  #define VERSION_BUILD  0

● Bash(gcc --version 2>&1 | head -1)
  ⎿  gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      16  #define CC99_STR(x) CC99_STR_HELPER(x)
      17  
      18  #ifndef VERSION_BUILTBY
      19 -#define VERSION_BUILTBY gcc_Ubuntu_13_3_0                      
      19 +#define VERSION_BUILTBY gcc_Ubuntu_13_3_0_stage_95_10          
      20  #endif
      21  
      22  static char version_buf[256];

● Bash(cmake --build build 2>&1 | grep "error:" && ./build/ccompiler --version)
  ⎿  Error: Exit code 1

● Bash(cmake --build build 2>&1 | tail -5 && ./build/ccompiler --version)
  ⎿  [100%] Built target ccompiler
     ClaudeC99 version 00.02.00951000.00741
     BuiltBy: GNU_13_3_0

  Searched for 1 pattern (ctrl+o to expand)

● The CMake build overrides VERSION_BUILTBY via -D flag. Let me revert the
  fallback in version.c to its standard form:

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      16  #define CC99_STR(x) CC99_STR_HELPER(x)
      17  
      18  #ifndef VERSION_BUILTBY
      19 -#define VERSION_BUILTBY gcc_Ubuntu_13_3_0_stage_95_10          
      19 +#define VERSION_BUILTBY gcc_Ubuntu_13_3_0                      
      20  #endif
      21  
      22  static char version_buf[256];

● Bash(cmake --build build 2>&1 | grep "error:" | head -5 &&
      ./test/run_all_tests.sh 2>&1 | tail -3)
  ⎿  ====================================================
     Aggregate: 1479 passed, 0 failed, 1479 total
     ====================================================

● Bash(git add src/version.c && git commit -m "$(cat <<'EOF'
      stage 95-10: update VERSION_STAGE to 00951000…)
  ⎿  [master bb0df27] stage 95-10: update VERSION_STAGE to 00951000
      1 file changed, 1 insertion(+), 1 deletion(-)

● Now update the fixed-capacity inventory to mark the parser.h portion of
  MAX_NAME_LEN as done:

● Update(docs/fixed-capacity-inventory.md)
  ⎿  Added 1 line, removed 1 line
      18  
      19  | Name | Max | Module | On Overflow | Ext Ptr Refs | Safe Reall
          oc | Priority | Status |
      20  |------|-----|--------|-------------|--------------|-----------
          ---|----------|--------|
      21 -| `MAX_NAME_LEN` | 256 bytes | `include/constants.h`; used in `
         -char name[]`, `char tag[]`, `char label[]`, `char static_label[
         -]`, `char init_label[]` fields in `LocalVar`, `GlobalVar`, `Loc
         -alStaticVar`, `FuncSig`, `GlobalObjSig`, `TypedefEntry`, `EnumC
         -onst`, `EnumTag`, `StructTag`, `UnionTag`, `StructField`. **`To
         -ken.value` was migrated to a pointer+length into lexer-owned st
         -orage in stage 95-08. `ASTNode.value` was migrated to a `const 
         -char *` pointer into lexer-owned storage in stage 95-09 and is 
         -no longer subject to this limit.** | `strncpy` silently truncat
         -es to 255 bytes; the identifier stored in the struct is silentl
         -y wrong | No | N/A (embedded `char[]`) | LOW | PENDING (ASTNode
         -.value ✓ DONE stage 95-09) |                                   
      21 +| `MAX_NAME_LEN` | 256 bytes | `include/constants.h`; used in `
         +char label[]`, `char static_label[]`, `char init_label[]` field
         +s in `LocalVar`, `GlobalVar`, `LocalStaticVar`, and `StructFiel
         +d`. **`Token.value` was migrated to a pointer+length into lexer
         +-owned storage in stage 95-08. `ASTNode.value` was migrated to 
         +a `const char *` pointer into lexer-owned storage in stage 95-0
         +9. All parser.h struct name/tag fields (`FuncSig.name`, `Global
         +ObjSig.name`, `TypedefEntry.name`, `EnumConst.name`, `EnumTag.t
         +ag`, `StructTag.tag`, `UnionTag.tag`) were migrated to `const c
         +har *` in stage 95-10.** | `strncpy` silently truncates to 255 
         +bytes; the identifier stored in the struct is silently wrong | 
         +No | N/A (embedded `char[]`) | LOW | PENDING (ASTNode.value ✓ D
         +ONE stage 95-09; parser.h structs ✓ DONE stage 95-10) |        
      22  
      23  ---
      24  

● Also update the LOW priority summary section:

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
      187 -| `MAX_NAME_LEN` | Embedded `char name[256]` in `LocalVar`, `G
          -lobalVar`, `FuncSig`, and other parser/codegen structs. `Token
          -.value` was migrated to pointer+length (stage 95-08) and `ASTN
          -ode.value` to `const char *` (stage 95-09); both are no longer
          - bounded. Remaining uses in `LocalVar`, `GlobalVar`, `FuncSig`
          -, etc. still require per-struct migration. N/A. |             
      187 +| `MAX_NAME_LEN` | Embedded `char name[256]` / `char label[256
          +]` in `LocalVar`, `GlobalVar`, `LocalStaticVar` (codegen) and 
          +`StructField` (type.h). `Token.value` migrated (stage 95-08), 
          +`ASTNode.value` migrated (stage 95-09), all parser.h struct na
          +me/tag fields migrated (stage 95-10). Remaining codegen and ty
          +pe.h uses require per-struct migration. N/A. |                
      188  | `MAX_ARRAY_DIMS` | Local `#define` and stack variable in par
           ser.c; only affects nested array dimensions. N/A (stack). |
      189  | `MAX_INCLUDE_DEPTH` | Recursion depth counter in preprocesso
           r.c; not an array. N/A. |
      190  | `MAX_COND_DEPTH` | Local stack variable `CondFrame cond_stac
           k[64]` in preprocessor.c. N/A. |

● Bash(git add docs/fixed-capacity-inventory.md && git commit -m "$(cat <<'EOF'
      stage 95-10: update fixed-capacity-inventory for parser.h MAX_NAME_LEN
      migration…)
  ⎿  [master 48b7f18] stage 95-10: update fixed-capacity-inventory for pars
     er.h MAX_NAME_LEN migration
      1 file changed, 2 insertions(+), 2 deletions(-)

● All implementation and tests done. Now running the self-host test:

● Bash(./build.sh --mode=self-host 2>&1 | tail -40)
  ⎿  PASS  test_token_volatile
     PASS  test_token_while
     … +37 lines (ctrl+o to expand)
  ⎿  (timeout 10m)

● C0→C1→C2 all pass with 1479 tests. No issues during bootstrap.

  Now update the self-compilation report:

  Read 1 file (ctrl+o to expand)

● Update(docs/self-compilation-report.md)
  ⎿  Added 15 lines
      136  
      137  After both fixes all 1478 tests passed at C0, C1, and C2.
      138  
      139 +## Issues found during stage 95-10 self-hosting test          
      140 +                                                              
      141 +None. The parser.h `const char *` pointer migration produced n
          +o bootstrap failures.                                         
      142 +All 1479 tests passed at C0, C1, and C2.                      
      143 +                                                              
      144 +## Result (stage 95-10)                                       
      145 +                                                              
      146 +**Date:** 2026-06-07                                          
      147 +                                                              
      148 +| Step | Binary | Version | BuiltBy | Tests |                 
      149 +|------|--------|---------|---------|-------|                 
      150 +| C0 | `build/ccompiler-c0` | `00.02.00951000.00756` | `GNU_13
          +_3_0` | 1479/1479 |                                           
      151 +| C1 | `build/ccompiler-c1` | `00.02.00951000.00757` | `Claude
          +C99_v00_02_00951000_00756` | 1479/1479 |                      
      152 +| C2 | `build/ccompiler-c2` | `00.02.00951000.00758` | `Claude
          +C99_v00_02_00951000_00757` | 1479/1479 |                      
      153 +                                                              
      154  ## Issues found during stage 95-09 self-hosting test
      155  
      156  None. The `ASTNode.value` pointer migration produced no bootst
           rap failures.

● Bash(git add docs/self-compilation-report.md && git commit -m "$(cat <<'EOF'
      stage 95-10: self-host report — all 1479 tests pass at C0/C1/C2…)
  ⎿  [master edd8051] stage 95-10: self-host report — all 1479 tests pass a
     t C0/C1/C2
      1 file changed, 15 insertions(+)

● Now delegate the milestone, transcript, and README update to haiku:
● haiku-stage-artifact-writer(Generate stage-95-10 milestone, transcript, and
                             README update)
  ⎿  Done (10 tool uses · 34.4k tokens · 41s)
  (ctrl+o to expand)

  Searched for 1 pattern, listed 1 directory (ctrl+o to expand)

● README and artifacts look good. Let me check the kickoff file:

  Listed 1 directory (ctrl+o to expand)

● The kickoff agent generated it but didn't save to a file. Let me create it:

● Write(docs/kickoffs/stage-95-10-kickoff.md)
  ⎿  Wrote 44 lines to docs/kickoffs/stage-95-10-kickoff.md
       2 # Stage 95-10 Kickoff — Remove Static Char Arrays from parser.h
       3 ## Spec Summary
       5 Convert all `char field[MAX_NAME_LEN]` embedded arrays in parser
         .h structs to `const char *` pointers. Replace strncpy/copy oper
         ocal `char[256]` buffers in parsing functions by using `parser->
         current.value` directly (persistent lexer-pool storage since sta
       6 ge 95-08).
       7 ## Checklist
       9 - [ ] `TypedefEntry.name`
      10 - [ ] `FuncSig.name`
     … +51 lines (ctrl+o to expand)

● Bash(git add docs/kickoffs/stage-95-10-kickoff.md
      docs/milestones/stage-95-10-milestone.md
      docs/sessions/stage-95-10-transcript.md README.md && git commit -m
      "$(cat…)
  ⎿  [master a8f227f] stage 95-10: add kickoff, milestone, transcript, and
     update README
      4 files changed, 137 insertions(+), 1 deletion(-)
     … +3 lines (ctrl+o to expand)

● Stage 95-10 is complete. Here's what was done:

  7 struct fields converted in include/parser.h — all char field[MAX_NAME_LEN]
  embedded arrays replaced with const char *:
  1. TypedefEntry.name
  2. FuncSig.name
  3. GlobalObjSig.name
  4. StructTag.tag
  5. UnionTag.tag
  6. EnumTag.tag
  7. EnumConst.name

  Each conversion: changed the header field type, replaced all strncpy/copy
  operations in src/parser.c with direct pointer assignments, and simplified
  local char[256] buffers in parse_struct_specifier, parse_union_specifier, and
  parse_enum_specifier to const char * pointing directly to the lexer string
  pool.

  All 1479 tests pass after each individual commit, and the full C0→C1→C2
  self-host cycle completes cleanly with no bootstrap failures.

✻ Worked for 15m 9s

※ recap: Stage 95-10 is fully complete: all 7 char array fields in parser.h structs converted to const char pointers, all 1479 tests passing at C0/C1/C2. No next action needed for this stage. (disable recaps in /config)