❯ /implement-stage docs/stages/ClaudeC99-spec-stage-07-03-increment-and-decrement.md

● I'll start by reading the spec file and the supporting skill files.

Read 3 files (ctrl+o to expand)

● Now let me explore the existing codebase to understand the current structure.

Searched for 1 pattern, read 1 file (ctrl+o to expand)

● Now let me check the existing test structure.

Searched for 2 patterns (ctrl+o to expand)

● Let me look at the existing test files for this stage and the test runner.

Read 6 files (ctrl+o to expand)

● Good. Tests already exist from the spec phase. Now I have a complete picture. Here's my summary and plan.
                                                                                                                                                                          
---                                                                                                                                                                     
Stage 7.3: Increment and Decrement Operators — Kickoff

1. Summary of the Spec

Add prefix (++x, --x) and postfix (x++, x--) increment/decrement operators for integer variables only. Prefix evaluates to the new value; postfix evaluates to the old  
value. Only identifiers are valid operands.

2. Required Tokenizer Changes

- Add two new token types: TOKEN_INCREMENT (++) and TOKEN_DECREMENT (--)
- Update the + and - lexer branches to check for a second + or - before falling through

3. Required Parser Changes

- Add a new parse_postfix() function between parse_primary() and parse_unary() — it parses <primary> { "++" | "--" }
- Update parse_unary() to handle ++ and -- as prefix operators (requiring an identifier operand)
- parse_unary() currently calls parse_primary() — it should now call parse_postfix() instead

4. Required AST Changes

- Add two new node types: AST_PREFIX_INC_DEC and AST_POSTFIX_INC_DEC
- Each stores the operator (++/--) in value and the variable name in the child (an AST_VAR_REF)

5. Required Code Generation Changes

- Prefix ++x: load variable, add/sub 1, store back, result in eax is the new value
- Postfix x++: load variable into eax, push old value, add/sub 1 and store back, pop old value into eax

6. Test Plan

5 tests already exist:
- test_postfix_inc__42.c — a=41; a++; return a;
- test_postfix_dec__42.c — a=43; a--; return a;
- test_prefix_inc__42.c — a=41; ++a; return a;
- test_prefix_dec__42.c — a=43; --a; return a;
- test_for_loop_with_inc__55.c — for (i=1;i<=10;i++) sum+=i;

I'll add additional tests for value-returning semantics (e.g., x = i++ returns old value, x = ++i returns new value).   