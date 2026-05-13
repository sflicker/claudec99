What works today: statements (if/else, while/do-while/for, switch/case/default, break/continue/goto, blocks with shadowing, comments), full signed integer hierarchy (char/short/int/long) with promotions,
casts, and integer literal typing with L suffix, multi-function programs with forward declarations and SysV-AMD64 calls (≤6 args) including boundary conversions, pointers (declarations, &/* as both rvalue and
lvalue, assign-through-pointer, pointer parameters and return types, NULL as a null pointer constant), arrays (declarations, indexing, decay-to-pointer, pointer arithmetic, char-array initialization from string
literals with explicit or inferred size), string literals (tokenization, static-data emission, decay to char *, decoded escapes \n/\t/\r/\\/\"/\0), character literals (tokenization, AST, codegen, full
simple-escape set \a/\b/\f/\n/\r/\t/\v/\\/\'/\"/\?/\0 with int typing in any integer context), the full set of bitwise operators (& ^ | ~ << >>), the remainder operator (%), all eleven compound assignment
operators (= += -= *= /= %= <<= >>= &= ^= |=), sizeof (type names, expressions, and whole arrays/structs — operand not evaluated), the conditional (ternary) operator with common-type branches, the comma
operator, comma-separated init-declarator lists (int a, *b, c = 3;), function-declaration compatibility checks (return type and parameter type mismatch detected), file-scope global variable declarations
(uninitialized in .bss / initialized in .data with constant expressions, RIP-relative addressing), storage-class specifiers extern and static with linkage consistency enforcement, parenthesized declarators,
function pointer declarations/assignment/indirect calls (fp(args) and (*fp)(args)), a unified general declarator with declaration specifier list parsing, typedef aliases for scalar/pointer/array/function-pointer
and complete struct types with block-scope shadowing, enum declarations with compile-time constant folding, structs (definition, natural-alignment layout, sizeof, dot/arrow member access in rvalue and lvalue
contexts, brace initializer lists with zero-fill, whole-struct assignment and copy-initialization, pointer-to-struct parameters with field mutation, nested structs, arrays of structs, typedef aliases, incomplete
forward declarations for self-referential types), void type and void * generic pointer (implicit conversion to/from any non-function pointer type, void functions, bare return, int f(void) zero-parameter form,
dereferencing/arithmetic/subscripting void* rejected), and minimal const qualifier (base-type scalars, direct assignment to const lvalues rejected).

What's still out of scope: floats, unsigned types, for-loop initializer declarations, variadics, preprocessor, octal/hex character escapes, multi-character and wide-character literals, global array initialization
(beyond .bss zero-fill), >6-arg calls, pointer subtraction, multi-dimensional arrays, unions, pointer-level const enforcement (writes through const *, conversion const-correctness), typedef enum, enum constant
expressions beyond integer/character literals, pointer-to-function-pointer and functions returning function pointers, and calling through arbitrary function-pointer expressions.
