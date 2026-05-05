  What works today: statements (if/else, while/do-while/for, switch/case/default, break/continue/goto, blocks with shadowing, comments), full signed integer hierarchy (char/short/int/long) with promotions,
  casts, and integer literal typing with L suffix, multi-function programs with forward declarations and SysV-AMD64 calls (≤6 args) including boundary conversions, pointers (declarations, &/* as both rvalue and
  lvalue, assign-through-pointer, pointer parameters and return types, NULL as a null pointer constant), arrays (declarations, indexing, decay-to-pointer, pointer arithmetic, char-array initialization from string
  literals with explicit or inferred size), string literals (tokenization, static-data emission, decay to char *, decoded escapes \n/\t/\r/\\/\"/\0), character literals (tokenization, AST, codegen, full
  simple-escape set \a/\b/\f/\n/\r/\t/\v/\\/\'/\"/\?/\0 with int typing in any integer context), the full set of bitwise operators (& ^ | ~ << >>), the remainder operator (%), all eleven compound assignment
  operators (= += -= *= /= %= <<= >>= &= ^= |=), sizeof (type names, expressions, and whole arrays — operand not evaluated), the conditional (ternary) operator with common-type branches, the comma operator, and
  comma-separated init-declarator lists (int a, *b, c = 3;).

  What's still out of scope: structs/unions/enums, floats, unsigned types, typedef/storage-classes, file-scope variable declarations, for-loop initializer declarations, variadics, preprocessor, function pointers,
  octal/hex character escapes, multi-character constants, wide character literals (L'A', u'A', U'A'), and >6-arg calls.
