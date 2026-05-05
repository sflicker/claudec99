# Stage 20-03 Kickoff: Declaration Regression Tests

## Summary of the Spec

Stage 20-03 adds regression tests for multi-declarator declarations. No tokenizer, grammar, parser, or codegen changes are required. The tests exercise:
- Multi-declarator lists with plain int, pointer, and double-pointer declarators
- Pointer initializers with forward references within a declarator list
- Three-variable forward-reference chains
- Comma expressions as initializers in declarator lists
- Comma expressions in assignment statements
- Invalid forms: leading comma, trailing comma, and literal-as-declarator

## Required Changes

**Tokenizer:** None  
**Grammar:** None  
**Parser:** None  
**AST:** None  
**Codegen:** None  
**Tests:** 6 new valid tests, 4 new invalid tests

## Spec Issues Identified

1. **Valid Case 2** — Missing closing `}`. Fixed by adding the brace.

2. **Valid Case 3** — Two problems:
   - `a[1] = 4'` is a typo; should be `a[1] = 4;`
   - `int a[2], b;` cannot compile. The parser handles an array as the first declarator by immediately consuming a semicolon — it never reaches the `, b` continuation. Skipping this case; parser support for mixed array/scalar declarator lists is out of scope for this stage.

3. **Valid Case 4** — Two problems:
   - `char s[] = "abc", c = 'x';` fails for the same reason (array first declarator + comma not supported by the parser)
   - Comment says `//expect 218` but `'x'` = 120 in ASCII; `return s[1] = c` would return 120. Skipping this case.

4. **Valid Case 6** — Transcription error: uses `;` where `,` is needed, and `*p`/`b` lack type specifiers. Interpreted as `int a = 9, *p = &a, b = *p;` returning 9.

5. **Valid Case 8** — Missing closing `}`. Fixed by adding the brace.

6. **Invalid Case 4** — Missing `//` before the `INVALID` comment in the spec. The test source itself is correct.

## Test Plan

**Valid tests (6):**

| Filename | Source | Expected |
|---|---|---|
| `test_decl_list_double_ptr_chain__42.c` | `int a, *p, **q; a=42; p=&a; q=&p; return **q;` | 42 |
| `test_decl_list_ptr_init_fwd__7.c` | `int a=5, *p=&a, b=*p+2; return b;` | 7 |
| `test_decl_list_three_chain__22.c` | `int a=10, b=a+5, c=b+7; return c;` | 22 |
| `test_decl_list_ptr_deref_init__9.c` | `int a=9, *p=&a, b=*p; return b;` | 9 |
| `test_decl_list_comma_expr_both__6.c` | `int a=(1,2), b=(3,4); return a+b;` | 6 |
| `test_decl_list_comma_assign__3.c` | `int a; a=(1,2,3); return a;` | 3 |

**Invalid tests (4):**

| Filename | Source | Error fragment |
|---|---|---|
| `test_invalid_101__expected_token_type.c` | `int a,;` | expected token type |
| `test_invalid_102__expected_token_type.c` | `int , a;` | expected token type |
| `test_invalid_103__expected_token_type.c` | `int a = 1, 2;` | expected token type |
| `test_invalid_104__expected_token_type.c` | `int a = 1, b = 2, ;` | expected token type |

**Baseline:** 381 valid / 102 invalid  
**After:** 387 valid / 106 invalid

## Implementation Order

1. Add 6 valid test files under `test/valid/`
2. Add 4 invalid test files under `test/invalid/`
3. Run `bash test/valid/run_tests.sh` and `bash test/invalid/run_tests.sh` to confirm all pass
4. Commit

## Notes

- No README update needed (no user-visible capability change)
- No grammar.md update needed (no grammar changes)
- Cases 3 and 4 from the spec are skipped — they require parser support for array declarators as the leading entry in a multi-declarator list, which the stage-20-02 implementation explicitly does not support
