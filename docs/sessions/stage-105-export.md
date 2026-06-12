 ▐▛███▜▌   Claude Code v2.1.174
▝▜█████▛▘  Sonnet 4.6 · Claude Pro
  ▘▘ ▝▝    ~/code/claude/claudec99

 ▎ Meet Fable 5, our newest model for complex, long-running work. Switch anytime with /model.
 ▎ Included in your plan limits until Jun 22, then switch to usage credits to continue.

❯  `/implement-stage                                                            
  docs/stages/ClaudeC99-spec-stage-105-c99-preprocessor-completion.md`          

● Let me read the spec document first.

  Read 1 file (ctrl+o to expand)

● Now let me look at the relevant source files to understand the current state.

  Searched for 2 patterns, read 1 file (ctrl+o to expand)

● Now let me check the parser files.

  Searched for 4 patterns, read 2 files (ctrl+o to expand)

● Now I have a complete picture. Let me create tasks and implement all changes.

● Starting with the MacroTable extension and related helpers in preprocessor.c.

● Update(src/preprocessor.c)
  ⎿  Added 12 lines, removed 3 lines
      79      MacroDef *defs;
      80      size_t    count;
      81      size_t    cap;
      82 +    char    **once_paths;   /* canonical paths where #pragma on
         +ce was seen */                                                 
      83 +    size_t    once_count;                                      
      84 +    size_t    once_cap;                                        
      85  } MacroTable;
      86  
      87  static void macro_table_init(MacroTable *t) {
      85 -    t->defs  = NULL;                                           
      86 -    t->count = 0;                                              
      87 -    t->cap   = 0;                                              
      88 +    t->defs        = NULL;                                     
      89 +    t->count       = 0;                                        
      90 +    t->cap         = 0;                                        
      91 +    t->once_paths  = NULL;                                     
      92 +    t->once_count  = 0;                                        
      93 +    t->once_cap    = 0;                                        
      94  }
      95  
      96  static void macro_table_free(MacroTable *t) {
     ...
      104          }
      105      }
      106      free(t->defs);
      107 +    for (size_t i = 0; i < t->once_count; i++)                
      108 +        free(t->once_paths[i]);                               
      109 +    free(t->once_paths);                                      
      110  }
      111  
      112  static MacroDef *macro_find(MacroTable *t, const char *name, s
           ize_t len) {

● Now add the macro_table_add_once and macro_table_is_once helpers after
  macro_table_free. Let me find the exact end of that function:

  Read 1 file (ctrl+o to expand)
● Update(src/preprocessor.c)
  ⎿  Added 18 lines
      109      free(t->once_paths);
      110  }
      111  
      112 +static void macro_table_add_once(MacroTable *t, const char *pa
          +th) {                                                         
      113 +    if (t->once_count >= t->once_cap) {                       
      114 +        size_t new_cap = t->once_cap == 0 ? 8 : t->once_cap * 
          +2;                                                            
      115 +        char **np = realloc(t->once_paths, new_cap * sizeof(ch
          +ar *));                                                       
      116 +        if (!np) { fprintf(stderr, "error: out of memory\n"); 
          +exit(1); }                                                    
      117 +        t->once_paths = np;                                   
      118 +        t->once_cap   = new_cap;                              
      119 +    }                                                         
      120 +    t->once_paths[t->once_count++] = util_strdup(path);       
      121 +}                                                             
      122 +                                                              
      123 +static int macro_table_is_once(const MacroTable *t, const char
          + *path) {                                                     
      124 +    for (size_t i = 0; i < t->once_count; i++)                
      125 +        if (strcmp(t->once_paths[i], path) == 0)              
      126 +            return 1;                                         
      127 +    return 0;                                                 
      128 +}                                                             
      129 +                                                              
      130  static MacroDef *macro_find(MacroTable *t, const char *name, s
           ize_t len) {
      131      for (size_t i = 0; i < t->count; i++) {
      132          if (strlen(t->defs[i].name) == len &&

● Now add the #pragma once guard at the top of preprocess_file. Let me find it:

  Read 1 file (ctrl+o to expand)

● Update(src/preprocessor.c)
  ⎿  Added 7 lines
      232  
      233  static char *preprocess_file(const char *path, int depth, Macr
           oTable *macros,
      234                                const char **include_dirs, int n
           _include_dirs) {
      235 +    /* #pragma once: return empty string for a file already pr
          +ocessed */                                                    
      236 +    if (macro_table_is_once(macros, path)) {                  
      237 +        char *empty = malloc(1);                              
      238 +        if (!empty) { fprintf(stderr, "error: out of memory\n"
          +); exit(1); }                                                 
      239 +        empty[0] = '\0';                                      
      240 +        return empty;                                         
      241 +    }                                                         
      242      FILE *f = fopen(path, "r");
      243      if (!f) {
      244          fprintf(stderr, "error: cannot open include file '%s'\
           n", path);

● Now add the current_line variable declaration with current_file_override. Let
  me find the right location:

  Read 1 file (ctrl+o to expand)

● Update(src/preprocessor.c)
  ⎿  Added 2 lines, removed 1 line
      1151  
      1152      size_t in = 0;
      1153      int line_has_content = 0;
      1154 -    int current_line = 1;                                    
      1154 +    int   current_line          = 1;                         
      1155 +    char *current_file_override = NULL;                      
      1156  
      1157      while (s[in]) {
      1158          char c = s[in];

● Now I need to find all exit(1) paths in preprocess_internal and add
  free(current_file_override) before them. Let me also find the return
  statement. Let me look at the end of the function and all error paths:

  Searched for 1 pattern (ctrl+o to expand)

● Since free(NULL) is a no-op and exit paths free memory anyway, I'll add
  free(current_file_override) only to the final return path and the new
  handlers. Let me find the final return:

  Read 1 file (ctrl+o to expand)

● Update(src/preprocessor.c)
  ⎿  Added 1 line
      1873          exit(1);
      1874      }
      1875      gbuf_push(&out, '\0');
      1876 +    free(current_file_override);                             
      1877      free(spliced);
      1878      return out.data;
      1879  }

● Now update the __FILE__ expansion to use current_file_override. Let me find
  it:

  Read 1 file (ctrl+o to expand)

● Update(src/preprocessor.c)
  ⎿  Added 2 lines, removed 1 line
      1729              size_t id_len = in - id_start;
      1730              if (emitting) {
      1731                  if (id_len == 8 && strncmp(s + id_start, "__F
            ILE__", 8) == 0) {
      1732 +                    const char *fname = current_file_override
           + ? current_file_override : source_path;                      
      1733                      gbuf_push(&out, '"');
      1733 -                    for (const char *fp = source_path; fp && 
           -*fp; fp++) {                                                 
      1734 +                    for (const char *fp = fname; fp && *fp; f
           +p++) {                                                       
      1735                          if (*fp == '"' || *fp == '\\') gbuf_p
            ush(&out, '\\');
      1736                          gbuf_push(&out, *fp);
      1737                      }

● Now add the _Pragma handler in the identifier expansion section. Let me find
  the right spot - after __LINE__ and before the general macro expand:

  Read 1 file (ctrl+o to expand)

● Update(src/preprocessor.c)
  ⎿  Added 42 lines
      1740                      char lbuf[32];
      1741                      int llen = snprintf(lbuf, sizeof(lbuf), "
            %d", current_line);
      1742                      gbuf_append(&out, lbuf, (size_t)llen);
      1743 +                } else if (id_len == 7 && strncmp(s + id_star
           +t, "_Pragma", 7) == 0) {                                     
      1744 +                    /* C99 §6.10.9: _Pragma("string") — destr
           +ingize and apply as pragma,                                  
      1745 +                     * replace the entire _Pragma(...) expres
           +sion with nothing. */                                        
      1746 +                    size_t peek = in;                        
      1747 +                    while (s[peek] == ' ' || s[peek] == '\t')
           + peek++;                                                     
      1748 +                    if (s[peek] != '(') {                    
      1749 +                        fprintf(stderr, "error: expected '(' 
           +after _Pragma\n");                                           
      1750 +                        free(out.data); free(spliced); free(c
           +urrent_file_override); exit(1);                              
      1751 +                    }                                        
      1752 +                    in = peek + 1; /* skip '(' */            
      1753 +                    while (s[in] == ' ' || s[in] == '\t') in+
           ++;                                                           
      1754 +                    if (s[in] != '"') {                      
      1755 +                        fprintf(stderr, "error: _Pragma argum
           +ent must be a string literal\n");                            
      1756 +                        free(out.data); free(spliced); free(c
           +urrent_file_override); exit(1);                              
      1757 +                    }                                        
      1758 +                    in++; /* skip opening '"' */             
      1759 +                    GrowBuf pbuf;                            
      1760 +                    gbuf_init(&pbuf, 32);                    
      1761 +                    while (s[in] && s[in] != '"' && s[in] != 
           +'\n') {                                                      
      1762 +                        if (s[in] == '\\' && s[in + 1] == '"'
           +) {                                                          
      1763 +                            gbuf_push(&pbuf, '"');           
      1764 +                            in += 2;                         
      1765 +                        } else if (s[in] == '\\' && s[in + 1]
           + == '\\') {                                                  
      1766 +                            gbuf_push(&pbuf, '\\');          
      1767 +                            in += 2;                         
      1768 +                        } else {                             
      1769 +                            gbuf_push(&pbuf, s[in++]);       
      1770 +                        }                                    
      1771 +                    }                                        
      1772 +                    if (s[in] == '"') in++;                  
      1773 +                    while (s[in] == ' ' || s[in] == '\t') in+
           ++;                                                           
      1774 +                    if (s[in] != ')') {                      
      1775 +                        fprintf(stderr, "error: expected ')' 
           +after _Pragma string\n");                                    
      1776 +                        free(pbuf.data); free(out.data); free
           +(spliced);                                                   
      1777 +                        free(current_file_override); exit(1);
      1778 +                    }                                        
      1779 +                    in++; /* skip ')' */                     
      1780 +                    gbuf_push(&pbuf, '\0');                  
      1781 +                    if (strcmp(pbuf.data, "once") == 0 && sou
           +rce_path)                                                    
      1782 +                        macro_table_add_once(macros, source_p
           +ath);                                                        
      1783 +                    free(pbuf.data);                         
      1784 +                    /* _Pragma(...) is replaced by nothing. *
           +/                                                            
      1785                  } else {
      1786                      MacroDef *def = macro_find(macros, s + id
            _start, id_len);
      1787                      if (def && def->param_count == -1) {

● Now add the #line, #pragma, and null-directive handlers before the
  "unsupported" error. Let me find the current unsupported error location:

  Read 1 file (ctrl+o to expand)

● Update(src/preprocessor.c)
  ⎿  Added 57 lines
      1674                  free(out.data); free(spliced); exit(1);
      1675              }
      1676  
      1677 +            /* #line digit-sequence ["filename"] */          
      1678 +            if (strncmp(s + in, "line", 4) == 0 &&           
      1679 +                !isalnum((unsigned char)s[in + 4]) && s[in + 
           +4] != '_') {                                                 
      1680 +                in += 4;                                     
      1681 +                while (s[in] == ' ' || s[in] == '\t') in++;  
      1682 +                if (!isdigit((unsigned char)s[in])) {        
      1683 +                    fprintf(stderr, "error: #line requires a 
           +positive integer\n");                                        
      1684 +                    free(out.data); free(spliced); free(curre
           +nt_file_override); exit(1);                                  
      1685 +                }                                            
      1686 +                long new_line = 0;                           
      1687 +                while (isdigit((unsigned char)s[in]))        
      1688 +                    new_line = new_line * 10 + (s[in++] - '0'
           +);                                                           
      1689 +                /* The \n ending this directive will incremen
           +t current_line by 1,                                         
      1690 +                 * so set it to new_line - 1 so it reaches ne
           +w_line after the \n. */                                      
      1691 +                current_line = (int)(new_line - 1);          
      1692 +                while (s[in] == ' ' || s[in] == '\t') in++;  
      1693 +                if (s[in] == '"') {                          
      1694 +                    in++; /* skip opening '"' */             
      1695 +                    GrowBuf fbuf;                            
      1696 +                    gbuf_init(&fbuf, 32);                    
      1697 +                    while (s[in] && s[in] != '"' && s[in] != 
           +'\n') {                                                      
      1698 +                        if (s[in] == '\\' && s[in + 1]) {    
      1699 +                            in++;                            
      1700 +                            gbuf_push(&fbuf, s[in]);         
      1701 +                        } else {                             
      1702 +                            gbuf_push(&fbuf, s[in]);         
      1703 +                        }                                    
      1704 +                        in++;                                
      1705 +                    }                                        
      1706 +                    if (s[in] == '"') in++;                  
      1707 +                    gbuf_push(&fbuf, '\0');                  
      1708 +                    free(current_file_override);             
      1709 +                    current_file_override = fbuf.data; /* fbu
           +f.data now owned here */                                     
      1710 +                }                                            
      1711 +                while (s[in] && s[in] != '\n') in++;         
      1712 +                continue;                                    
      1713 +            }                                                
      1714 +                                                             
      1715 +            /* #pragma ... */                                
      1716 +            if (strncmp(s + in, "pragma", 6) == 0 &&         
      1717 +                !isalnum((unsigned char)s[in + 6]) && s[in + 
           +6] != '_') {                                                 
      1718 +                in += 6;                                     
      1719 +                while (s[in] == ' ' || s[in] == '\t') in++;  
      1720 +                if (strncmp(s + in, "once", 4) == 0 &&       
      1721 +                    (s[in + 4] == '\n' || s[in + 4] == '\0' |
           +|                                                            
      1722 +                     s[in + 4] == ' '  || s[in + 4] == '\t'))
           + {                                                           
      1723 +                    if (source_path)                         
      1724 +                        macro_table_add_once(macros, source_p
           +ath);                                                        
      1725 +                }                                            
      1726 +                while (s[in] && s[in] != '\n') in++;         
      1727 +                continue;                                    
      1728 +            }                                                
      1729 +                                                             
      1730 +            /* Null directive: '#' followed by optional white
           +space then newline — C99 §6.10.7. */                         
      1731 +            if (s[in] == '\n' || s[in] == '\0')              
      1732 +                continue;                                    
      1733 +                                                             
      1734              /* All other directives are unsupported. */
      1735              fprintf(stderr, "error: unsupported preprocessor 
            directive\n");
      1736              free(out.data);

● Now add current_func_name to the Parser struct in include/parser.h:

● Update(include/parser.h)
  ⎿  Added 3 lines
      89      /* Stage 75-03: set while parsing the body of a variadic fu
          nction
      90       * definition so __builtin_va_start can verify its context.
           */
      91      int current_func_is_variadic;
      92 +    /* Stage 105: set to the enclosing function name while pars
         +ing a function                                                 
      93 +     * body, so __func__ can be resolved to a string literal. *
         +/                                                              
      94 +    const char *current_func_name;                             
      95  } Parser;
      96  
      97  void parser_init(Parser *parser, Lexer *lexer);
● Now initialize current_func_name in parser_init:

● Update(src/parser.c)
  ⎿  Added 2 lines
      86      vec_init(&parser->enum_tags, sizeof(EnumTag));
      87      vec_init(&parser->struct_tags, sizeof(StructTag));
      88      vec_init(&parser->union_tags, sizeof(UnionTag));
      89 +    parser->current_func_is_variadic = 0;                      
      90 +    parser->current_func_name        = NULL;                   
      91  }
      92  
      93  /* Stage 96: release all Vec backing buffers owned by the parse
          r.

● Now update the save/restore block around parse_block to include
  current_func_name:

● Update(src/parser.c)
  ⎿  Added 4 lines, removed 1 line
      3910          /* Stage 75-03: expose the variadic flag to nested ex
            pression parsers
      3911           * so __builtin_va_start can enforce its "must be ins
            ide variadic
      3912           * function" rule. */
      3913 -        int saved_variadic = parser->current_func_is_variadic
           -;                                                            
      3913 +        int         saved_variadic  = parser->current_func_is
           +_variadic;                                                   
      3914 +        const char *saved_func_name = parser->current_func_na
           +me;                                                          
      3915          parser->current_func_is_variadic = func->is_variadic;
      3916 +        parser->current_func_name        = d.name;           
      3917          ast_add_child(func, parse_block(parser));
      3918          parser->current_func_is_variadic = saved_variadic;
      3919 +        parser->current_func_name        = saved_func_name;  
      3920      } else {
      3921          parser_expect(parser, TOKEN_SEMICOLON);
      3922      }

● Now add the __func__ handler in parse_primary. I need to add it inside the
  TOKEN_IDENTIFIER block, before the __builtin_va_* checks:

● Update(src/parser.c)
  ⎿  Added 17 lines
      1406          return node;
      1407      }
      1408      if (parser->current.type == TOKEN_IDENTIFIER) {
      1409 +        /* C99 §6.4.2.2: __func__ predefined identifier — the
           + name of the                                                 
      1410 +         * lexically-enclosing function as a static const cha
           +r array. */                                                  
      1411 +        if (strcmp(parser->current.value, "__func__") == 0) {
      1412 +            if (!parser->current_func_name)                  
      1413 +                PARSER_ERROR(parser,                         
      1414 +                    "error: '__func__' used outside of a func
           +tion body\n");                                               
      1415 +            parser->current = lexer_next_token(parser->lexer)
           +;                                                            
      1416 +            const char *fname = parser->current_func_name;   
      1417 +            int len = (int)strlen(fname);                    
      1418 +            ASTNode *node = parser_node(parser, AST_STRING_LI
           +TERAL,                                                       
      1419 +                                        lexer_store_bytes(par
           +ser->lexer,                                                  
      1420 +                                                          fna
           +me, (size_t)len));                                           
      1421 +            node->byte_length = len;                         
      1422 +            node->decl_type   = TYPE_ARRAY;                  
      1423 +            node->full_type   = type_array(type_char(), len +
           + 1);                                                         
      1424 +            return node;                                     
      1425 +        }                                                    
      1426          /* Stage 75-03: recognize __builtin_va_* intrinsics b
            efore general
      1427           * identifier resolution.  Each form has fixed argume
            nt counts and
      1428           * specific rules; all produce typed AST nodes. */

● Now bump the version and write the tests:

● Update(src/version.c)
  ⎿  Added 1 line, removed 1 line
      3  
      4  #define VERSION_MAJOR  "00"
      5  #define VERSION_MINOR  "02"
      6 -#define VERSION_STAGE  "01040000"                               
      6 +#define VERSION_STAGE  "01050000"                               
      7  
      8  #ifndef VERSION_BUILD
      9  #define VERSION_BUILD  0

● Now let me check the test directory structure:

  Listed 4 directories (ctrl+o to expand)

● Now let me write all the test files:

● Write(test/valid/test_pragma_unknown_ignored__0.c)
  ⎿  Wrote 3 lines to test/valid/test_pragma_unknown_ignored__0.c
      1 #pragma GCC optimize("O2")
      2 #pragma pack(push, 1)
      3 int main(void) { return 0; }

● Write(test/valid/test_line_directive_line_number__0.c)
  ⎿  Wrote 6 lines to test/valid/test_line_directive_line_number__0.c
      1 int main(void) {
      2     int before = __LINE__;
      3 #line 100
      4     int after = __LINE__;
      5     return (before < 100 && after == 100) ? 0 : 1;
      6 }

● Write(test/valid/test_line_directive_file_and_line__0.c)
  ⎿  Wrote 4 lines to test/valid/test_line_directive_file_and_line__0.c
      1 #line 1 "virtual.c"
      2 int main(void) {
      3     return (__LINE__ == 2) ? 0 : 1;
      4 }

● Write(test/valid/test_null_directive__0.c)
  ⎿  Wrote 6 lines to test/valid/test_null_directive__0.c
      1 #
      2 int main(void) {
      3 #
      4     return 0;
      5 #
      6 }

● Write(test/valid/test_pragma_operator_ignored__0.c)
  ⎿  Wrote 2 lines to test/valid/test_pragma_operator_ignored__0.c
      1 _Pragma("GCC diagnostic ignored \"-Wunused\"")
      2 int main(void) { return 0; }

● Write(test/valid/test_func_predefined_identifier__0.c)
  ⎿  Wrote 5 lines to test/valid/test_func_predefined_identifier__0.c
      1 #include <string.h>
      2 int check(void) {
      3     return strcmp(__func__, "check");
      4 }
      5 int main(void) { return check(); }

● Write(test/valid/test_func_predefined_identifier_multi__0.c)
  ⎿  Wrote 4 lines to test/valid/test_func_predefined_identifier_multi__0.c
      1 #include <string.h>
      2 int foo(void) { return strcmp(__func__, "foo"); }
      3 int bar(void) { return strcmp(__func__, "bar"); }
      4 int main(void) { return foo() + bar(); }

● Now the invalid test:

● Write(test/invalid/test_invalid_func_at_file_scope__used_outside_of_a_function
       _body.c)
  ⎿  Wrote 2 lines to test/invalid/test_invalid_func_at_file_scope__used_ou
     tside_of_a_function_body.c
      1 const char *name = __func__;
      2 int main(void) { return 0; }

● Now the integration tests. Let me check how integration tests are structured:

  Read 1 file, listed 4 directories (ctrl+o to expand)

● Let me look at how include-path tests with headers work to understand how to
  structure #pragma once tests:

  Read 1 file (ctrl+o to expand)
● The runner uses cd "$test_dir" and compiles $src (full path). Quoted includes
  resolve relative to the source file's directory, so helper headers work. Main
  file must be named <dirname>.c. Let me create the integration tests:

● Bash(mkdir -p
      /home/scott/code/claude/claudec99/test/integration/test_pragma_once /home/
      scott/code/claude/claudec99/test/integration/test_pragma_operator_once)
  ⎿  Done

● Write(test/integration/test_pragma_once/test_pragma_once.c)
  ⎿  Wrote 3 lines to test/integration/test_pragma_once/test_pragma_once.c
      1 #include "helper.h"
      2 #include "helper.h"
      3 int main(void) { return VALUE - 42; }

● Write(test/integration/test_pragma_once/helper.h)
  ⎿  Wrote 2 lines to test/integration/test_pragma_once/helper.h
      1 #pragma once
      2 #define VALUE 42

● Write(test/integration/test_pragma_operator_once/test_pragma_operator_once.c)
  ⎿  Wrote 3 lines to
     test/integration/test_pragma_operator_once/test_pragma_operator_once.c
      1 #include "guarded.h"
      2 #include "guarded.h"
      3 int main(void) { return FLAG - 1; }

● Write(test/integration/test_pragma_operator_once/guarded.h)
  ⎿  Wrote 2 lines to test/integration/test_pragma_operator_once/guarded.h
      1 _Pragma("once")
      2 #define FLAG 1

● Now build and run the tests:

● Bash(./build.sh 2>&1 | tail -20)
  ⎿        | ^~~~~~
     /home/scott/code/claude/claudec99/include/type.h:60:9: note: ‘param_co
     unt’ declared here
     … +17 lines (ctrl+o to expand)
  ⎿  (timeout 1m)

● Build succeeded. Now run the full test suite:

● Bash(./test/run_all_tests.sh 2>&1 | tail -30)
  ⎿  ====================================================
     Running suite: print_asm
     ====================================================
     … +27 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● All 1594 tests pass (10 new). Now run the self-host cycle:

● Bash(./build.sh --mode=self-host 2>&1 | tail -20)
  ⎿  PASS  test_stage_16_03_lshift_int
     PASS  test_stage_16_03_lshift_long
     PASS  test_stage_16_03_rshift_int
     … +17 lines (ctrl+o to expand)
  ⎿  (timeout 5m)

● Self-host passes cleanly (C0→C1→C2, all 1594 tests). Let me mark task 4
  complete and move on to documentation.

● Skill(update-supplemental-docs)
  ⎿  Successfully loaded skill

  Read 1 file, listed 4 directories (ctrl+o to expand)

● Bash(ls src/*.c | wc -l && ls include/*.h | wc -l && cat src/*.c include/*.h |
      wc -l && git rev-list --count HEAD && git rev-parse --short HEAD)
  ⎿  12
     13
     15232
     … +2 lines (ctrl+o to expand)

  Listed 2 directories (ctrl+o to expand)

● Bash(# Verify actual test suite breakdown from run
      ./test/run_all_tests.sh 2>&1 | grep -E "^Results:|^======")
  ⎿  ====================================================
     ====================================================
     ==============
     … +24 lines (ctrl+o to expand)

● Good. Totals: valid 914, invalid 256, integration 88, print-ast 50,
  print-tokens 100, print-asm 21, unit 165 → 1594 total. Now read the templates.

  Read 3 files (ctrl+o to expand)
● Now let me read the tail of the checklist:

  Searched for 2 patterns, read 1 file (ctrl+o to expand)

● Good — __func__ wasn't in the TODO. Now I'll write all four documents.

● Update(docs/outlines/checklist.md)
  ⎿  Added 30 lines, removed 1 line
      1524  - [x] Tests: 13 valid (enum/file-scope/block-static × relatio
            nal/equality/logical/ternary + precedence fix + switch case),
             2 invalid; all 1584 tests pass
      1525  - [x] Self-host C0→C1→C2 passes with no bootstrap issues; all
             1584/1584 tests pass at each step
      1526  
      1527 +## Stage 105 - C99 Preprocessor Completion                   
      1528 +                                                             
      1529 +- [x] `src/preprocessor.c` — extend `MacroTable` for `#pragma
           + once` tracking:                                             
      1530 +  - [x] Add `once_paths` / `once_count` / `once_cap` fields t
           +o `MacroTable` struct                                        
      1531 +  - [x] `macro_table_init`: zero-initialize new fields; `macr
           +o_table_free`: free `once_paths` array                       
      1532 +  - [x] Add `macro_table_add_once(t, path)` and `macro_table_
           +is_once(t, path)` helpers                                    
      1533 +- [x] `src/preprocessor.c` — `preprocess_file`: early-return 
           +empty string when path is already in `once_paths`            
      1534 +- [x] `src/preprocessor.c` — `#pragma` directive handler:    
      1535 +  - [x] Match `pragma` keyword (with non-identifier-char guar
           +d); skip unknown pragmas silently                            
      1536 +  - [x] Recognize `#pragma once`: call `macro_table_add_once(
           +macros, source_path)`                                        
      1537 +- [x] `src/preprocessor.c` — `#line` directive handler:      
      1538 +  - [x] Add `current_file_override` local (`char *`, NULL by 
           +default; freed on return)                                    
      1539 +  - [x] Parse required digit sequence; set `current_line = ne
           +w_line - 1` (newline adds 1)                                 
      1540 +  - [x] Parse optional quoted filename into a `GrowBuf`; assi
           +gn to `current_file_override`                                
      1541 +  - [x] Update `__FILE__` expansion to use `current_file_over
           +ride` when non-NULL                                          
      1542 +- [x] `src/preprocessor.c` — null directive: bare `#` followe
           +d by whitespace+newline → `continue` (C99 §6.10.7)           
      1543 +- [x] `src/preprocessor.c` — `_Pragma(string)` operator in id
           +entifier expansion:                                          
      1544 +  - [x] Parse `_Pragma("str")`: destringize content; recogniz
           +e `"once"` → `macro_table_add_once`; ignore everything else  
      1545 +  - [x] `_Pragma(...)` replaced by nothing (zero preprocessin
           +g tokens)                                                    
      1546 +- [x] `include/parser.h` — add `current_func_name` field to `
           +Parser` struct (after `current_func_is_variadic`)            
      1547 +- [x] `src/parser.c` — `parser_init`: initialize `current_fun
           +c_name = NULL`                                               
      1548 +- [x] `src/parser.c` — function definition: save/restore `cur
           +rent_func_name = d.name` around `parse_block`                
      1549 +- [x] `src/parser.c` — `parse_primary`: handle `__func__` bef
           +ore `__builtin_va_*` check:                                  
      1550 +  - [x] Error if `current_func_name == NULL` ("used outside o
           +f a function body")                                          
      1551 +  - [x] Synthesize `AST_STRING_LITERAL` with `lexer_store_byt
           +es(fname, len)`, `byte_length=len`, `full_type=type_array(typ
           +e_char(), len+1)`                                            
      1552 +- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01050000"`
      1553 +- [x] Tests: 7 valid (pragma ignored, line directive ×2, null
           + directive, _Pragma ignored, __func__ ×2), 1 invalid (__func_
           +_ at file scope), 2 integration (#pragma once, _Pragma once);
           + all 1594 tests pass                                         
      1554 +- [x] Self-host C0→C1→C2 passes with no bootstrap issues; all
           + 1594/1594 tests pass at each step                           
      1555 +                                                             
      1556  ---
      1557  
      1558  ## TODO
      1559  
      1560  ### Preprocessor
      1532 -- [ ] #pragma (at minimum #pragma once)                      
      1561 +- [x] `#pragma` (unknown pragmas silently ignored; `#pragma o
           +nce` supported) (Stage 105)                                  
      1562  - [ ] #elifdef / #elifndef
      1563  - [ ] GNU extension: `__VA_OPT__` and named variadic args
      1564  

● Now write the parse-tree document for stage 105:

● Write(docs/other/stage-105-parse-tree.md)
  ⎿  Wrote 969 lines to docs/other/stage-105-parse-tree.md
       2 # Full Grammar Parse Tree — Stage 105
         ns. Each levelrchy of parser functions, grouped into four sectio
         ks call depth;l(s) below it for sub-constructs. Indentation trac
       5 `└─►` indicates the primary descent path, `├─►` indicates a bran
       6 ch.
         ro, which calls raised through the `PARSER_ERROR(parser, …)` mac
       8 `compile_error_at` with the current token's `file:line:col` posi
       9 70-02/70-03). Under `--max-errors=N` the parser recovers via set
      10 `parser_sync()` to the next declaration boundary instead of exit
         ing on the first
     … +1532 lines (ctrl+o to expand)

● Now write the status snapshot:

● Write(status/project-status-through-stage-105.md)
  ⎿  Wrote 534 lines to status/project-status-through-stage-105.md
       2 # ClaudeC99 Project Status — Through Stage 105
       3 _As of 2026-06-12 (commit `d2732c6`)_
       5 ## Overview
       6 
       7 ClaudeC99 is a from-scratch C99 subset compiler written in C, ta
       8 x86_64 Linux via NASM (Intel syntax). The compiler is built incr
         ementally
         edrough small, spec-driven stages — each stage is a self-contain
      10 specification followed by a kickoff, implementation, and milesto
         ne /
     … +762 lines (ctrl+o to expand)

● Now write the features snapshot:

● Write(status/project-features-through-stage-105.md)
  ⎿  Wrote 3 lines to status/project-features-through-stage-105.md
        omment removal, `#include "file.h"` and `#include <file.h>`, obje
        ct-like and function-like `#define` with parameter substitution a
        nd recursive expansion, variadic macros with `__VA_ARGS__`, macro
         stringification `#param`, token pasting `##`, `#undef`, conditio
        nal compilation `#ifdef`/`#ifndef`/`#if`/`#elif`/`#else`/`#endif`
         with full recursive-descent expression evaluation including `def
        ined()`, macro expansion, arithmetic, bitwise, shift, logical, an
        d comparison operators, `#error`, **`#pragma`** (unknown pragmas 
        silently ignored; **`#pragma once`** records the file path and sk
        ips subsequent includes), **`_Pragma("st
     … +187 lines (ctrl+o to expand)

● Now verify all four documents were created and do a final sanity check:

  Listed 1 directory (ctrl+o to expand)

● All four documents are in place. The update is complete:

  update-supplemental-docs: refreshed through stage 105
    docs/outlines/checklist.md            (+1 stage section with 16 checked
  items; 1 TODO flipped to done)
    docs/other/stage-105-parse-tree.md    (new)
    status/project-status-through-stage-105.md   (new)
    status/project-features-through-stage-105.md (new)

✻ Sautéed for 21m 48s

※ recap: Stage 105 C99 preprocessor completion is fully implemented and documented. All 1594 tests pass including self-host. No further action needed. (disable recaps in /config)

❯ commit this                                                                   

● Bash(git status && git diff --stat)
  ⎿  On branch master
     Your branch is ahead of 'origin/master' by 2 commits.
       (use "git push" to publish your local commits)
     … +26 lines (ctrl+o to expand)

● Bash(git log --oneline -3)
  ⎿  d2732c6 self-host C1 verified: all tests pass
     a04376c self-host C0 verified: all tests pass
     91e4d48 stage 104 session usage and export

● Bash(git add \
        src/preprocessor.c \…)
  ⎿  (No output)

● Bash(git commit -m "$(cat <<'EOF'
      stage 105: C99 preprocessor completion (#pragma/once, _Pragma, #line, null
      directive, __func__)…)
  ⎿  [master bc33ae4] stage 105: C99 preprocessor completion (#pragma/once,
      _Pragma, #line, null directive, __func__)
      17 files changed, 2315 insertions(+), 1 deletion(-)
     … +16 lines (ctrl+o to expand)

● Committed as bc33ae4. Stage 105 is complete — 17 files, 2315 insertions, all
  1594 tests passing.

✻ Brewed for 30s