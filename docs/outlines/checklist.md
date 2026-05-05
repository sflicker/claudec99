# ClaudeC99 Feature Checklist

## Stage 1 - Minimal Compiler

- [x] Parse single function
- [x] Support return <int_literal>;
- [x] Tokenizer
	- [x] int
	- [x] return
	- [x] identifiers
	- [x] integer literals
	- [x] punctuation ( ) { } ;
- [x] AST
	- [x] program node
	- [x] function node
	- [x] return statement node
	- [x] integer literal node
- [x] Code Generation
	- [x] emit constant return value
- [x] Basic Test Harness

## Stage 2 - Arithmetic Expressions

- [x] Add Tokens: + - * / ( )
- [x] Extend Grammar for expressions
  - [x] addition/subtraction
  - [x] multiplication/divison
  - [x] paratheses
- [x] AST
  - Binary Expressions nodes
- [x] Parser
  - [x] precedence handling ( term, factor )
- [x] Code generation
  - [x] arithmetic operations
- [x] tests for expressions evaluation

## Stage 3 - Equality and Relational Expressions

- [x] Tokenizer
	- [x] == (equality)
	- [x] != (inequality)
	- [x] < (less than)
	- [x] <= (less than or equal)
	- [x] > (greater than)
	- [x] >= (greater than or equal)
- [x] Parser
	- [x] Equality expressions (left-associative)
	- [x] Relational expressions (left-associative)
- [x] AST
	- [x] Binary operator nodes for comparisons
- [x] Code Generation
	- [x] Comparison operators evaluate to 0 or 1
	- [x] x86-64 comparison instructions
- [x] Tests
	- [x] Test file naming convention: `test_<name>__<expected>.c`

## Stage 4 - Unary Operators, If/Else, and Block Statements

- [x] Tokenizer
	- [x] if, else keywords
	- [x] ! (logical not)
- [x] Parser
	- [x] Unary expressions: + - !
	- [x] If/else statement
	- [x] Block statement
	- [x] Expression statements
- [x] AST
	- [x] Unary operator nodes
	- [x] If/else statement nodes
	- [x] Block statement nodes
- [x] Code Generation
	- [x] Unary operator codegen
	- [x] If/else branching with labels

## Stage 5 - Integer Variables and Assignment

- [x] Tokenizer
	- [x] = (assignment, distinct from ==)
- [x] Parser
	- [x] Variable declarations with optional initialization
	- [x] Assignment expressions
- [x] AST
	- [x] Declaration nodes
	- [x] Assignment expression nodes
- [x] Code Generation
	- [x] Stack allocation per variable
	- [x] Local symbol table with stack offsets
	- [x] Load/store variable values
- [x] Semantic Analysis
	- [x] Undeclared variable detection
	- [x] Duplicate declaration detection

## Stage 6 - While Loops

- [x] Tokenizer
	- [x] while keyword
- [x] Parser
	- [x] While statement
- [x] AST
	- [x] While statement node (condition + body)
- [x] Code Generation
	- [x] Loop start label
	- [x] Condition check before each iteration
	- [x] Conditional branch to loop end
	- [x] Unconditional jump back to loop start

## Stage 7 - Compound Assignment, For Loops, Increment/Decrement, AST Printer

- [x] Compound assignment operators: += -=
	- [x] Tokenizer tokens
	- [x] Assignment as expression (not just statement)
	- [x] Codegen for compound assignment
- [x] For loops
	- [x] for (init; cond; update) statement
	- [x] Optional init/condition/update expressions
	- [x] AST node with four child slots: [init|NULL, cond|NULL, update|NULL, body]
- [x] Increment/Decrement operators
	- [x] Tokenizer: ++ --
	- [x] Prefix: ++x, --x (update first, return new value)
	- [x] Postfix: x++, x-- (return old value, then update)
	- [x] Lvalue validation (identifiers only)
- [x] AST Pretty Printer
	- [x] --print-ast command-line option
	- [x] Hierarchical indented output with readable node labels

## Stage 8 - Logical Operators and Nested Block Scopes

- [x] Tokenizer
	- [x] && (logical and)
	- [x] || (logical or)
- [x] Parser
	- [x] Logical AND/OR with correct precedence
- [x] Code Generation
	- [x] Short-circuit evaluation
	- [x] Result normalized to 0 or 1
- [x] Nested block scopes
	- [x] Variable shadowing support
	- [x] Duplicate detection within same scope only
	- [x] Variable visibility across scope hierarchy

## Stage 9 - Multi-Function Support

- [x] Code organization refactoring
	- [x] Separate lexer, parser, codegen, AST modules
	- [x] Public headers in include/
- [x] Translation unit as top-level construct
- [x] Multiple function definitions in a single file
- [x] Parameter lists
	- [x] Tokenizer: , (comma)
	- [x] Parameter name uniqueness validation
- [x] Function calls
	- [x] Call expressions in primary position
	- [x] System V AMD64 ABI: rdi rsi rdx rcx r8 r9 for arguments, rax for return
	- [x] Maximum 6 arguments enforced at parse time
	- [x] Argument evaluation left-to-right
- [x] Function declarations (forward prototypes)
	- [x] Declaration with semicolon termination
	- [x] Signature consistency checking across declarations/definitions
	- [x] Self-calls resolved because function registered before body is parsed

## Stage 10 - Control Flow Jump Statements and Comments

- [x] Break and continue
	- [x] Tokenizer: break, continue
	- [x] break jumps to loop/switch end; continue jumps to condition (while) or update (for)
	- [x] Compile-time validation: break requires loop_depth > 0 or switch_depth > 0
- [x] Do-while loops
	- [x] Tokenizer: do
	- [x] Body executes before condition check
	- [x] AST children: [body, condition] (body-first order)
- [x] Switch statements
	- [x] Tokenizer: switch, case, default
	- [x] case labels with integer literal values
	- [x] default label fallback
	- [x] Stacked case labels and fallthrough behavior
	- [x] switch_depth tracking for case/default/break validation
	- [x] Linear compare-and-branch dispatch codegen
- [x] Goto statements
	- [x] Tokenizer: goto
	- [x] goto <identifier> ;
	- [x] Labeled statements: <identifier> : <statement>
	- [x] Label uniqueness and goto target validation within function
- [x] Comments
	- [x] // single-line comments
	- [x] /* */ multi-line comments
	- [x] Skipped entirely by lexer (not passed to parser)

## Stage 11 - Integer Type System

- [x] Tokenizer
	- [x] char, short, long keywords
- [x] Type representation
	- [x] TypeKind enum
	- [x] Type struct with size information
	- [x] AST type annotations (decl_type, full_type)
- [x] Storage sizes
	- [x] char: 1 byte
	- [x] short: 2 bytes
	- [x] int: 4 bytes
	- [x] long: 8 bytes
	- [x] Sign-extension on load; truncation on store
- [x] Integer promotions
	- [x] char and short promoted to int in arithmetic
	- [x] Common integer type for binary operations
	- [x] Assignment narrowing/widening conversions
- [x] Typed function signatures
	- [x] Return type and parameter type storage in FuncSig
	- [x] Call expression typed with callee return type
	- [x] Argument-to-parameter conversion at call sites
	- [x] Return value conversion at return statements
- [x] Cast expressions
	- [x] (type) expr syntax
	- [x] AST_CAST nodes; codegen reuses assignment conversion
- [x] Integer literal typing
	- [x] L/l suffix for long literals
	- [x] Literals exceeding int range automatically typed as long
	- [x] Overflow detection

## Stage 12 - Pointer Types

- [x] Pointer declarations
	- [x] { * } prefix in declarators
	- [x] Multi-level pointers (int **)
	- [x] 8-byte stack allocation for all pointers
- [x] Address-of and dereference operators
	- [x] Unary & (address-of): produces pointer type
	- [x] Unary * (dereference): unwraps pointer type
	- [x] lea for address computation; sized load for dereference
- [x] Assignment through pointers
	- [x] Dereference as valid assignment lvalue
	- [x] Sized store to pointed-to address
- [x] Pointer parameters and return types
	- [x] Pointer types in parameter declarations
	- [x] Type mismatch rejection (e.g. int passed where int* expected)
	- [x] Pointer return type validation (exact match, no implicit conversion)
- [x] Null pointer constant
	- [x] Integer 0 as null in pointer contexts
	- [x] Pointer equality/inequality comparisons

## Stage 13 - Arrays

- [x] Tokenizer
	- [x] [ ] (left and right bracket)
- [x] Array declarations
	- [x] int a[N] with explicit positive integer size
	- [x] Stack allocation: element_size × length
- [x] Array indexing
	- [x] a[i] postfix subscript operator
	- [x] AST_ARRAY_INDEX node: children [base, index]
	- [x] Address calculation: base + index × element_size
- [x] Array-to-pointer decay
	- [x] Array decays to pointer in value contexts
	- [x] Decay in assignments to pointer variables
	- [x] Decay in function call arguments
	- [x] Whole-array assignment rejected
- [x] Pointer arithmetic
	- [x] pointer + integer and integer + pointer (scaled by element size)
	- [x] pointer - integer (scaled)
	- [x] Invalid combinations rejected (pointer + pointer, etc.)
- [x] Pointer subscripting
	- [x] p[i] on pointers (equivalent to *(p + i))

## Stage 14 - String Literals

- [x] Tokenizer
	- [x] Double-quoted string literal tokens
	- [x] Content extraction without surrounding quotes
	- [x] Byte length tracking (for embedded NUL support)
	- [x] Unterminated string error
- [x] String literal as primary expression
	- [x] AST_STRING_LITERAL nodes; type char[N+1]
- [x] Static data emission
	- [x] Emitted to .rodata section with NUL terminator
	- [x] Unique label per string literal
	- [x] Address loaded with lea
- [x] String literal decay to char*
	- [x] Assignment/initialization of char* from string literal
- [x] Escape sequences in string literals
	- [x] \n \t \r \\ \" \0
	- [x] Decoded bytes emitted in assembly output
- [x] Char array initialization from string literal
	- [x] char a[] = "..." (size inferred from literal length + 1)
	- [x] char a[N] = "..." (size checked against literal + NUL)
	- [x] String initializer restricted to char element type

## Stage 15 - Character Literals

- [x] Tokenizer
	- [x] Single-quoted character literals
	- [x] Escape sequences: \a \b \f \n \r \t \v \\ \' \" \? \0
	- [x] Decoded byte stored in token; long_value for integer value
- [x] Parser
	- [x] Character literal as primary expression
- [x] AST
	- [x] AST_CHAR_LITERAL nodes; type int (C semantics)
- [x] Integration
	- [x] Participates in arithmetic and assignment
	- [x] Constant expression support for case labels

## Stage 16 - Bitwise Operators and Remaining Compound Assignments

- [x] Remainder (modulo) operator %
	- [x] Tokenizer: %
	- [x] Multiplicative precedence
	- [x] idiv/idivq; extract remainder from rdx/edx
- [x] Unary bitwise complement ~
	- [x] Tokenizer: ~
	- [x] Unary expression; integer operands only
	- [x] not instruction; integer promotion applied first
- [x] Shift operators << >>
	- [x] Tokenizer: << >> (before < > to avoid mismatch)
	- [x] Shift expression precedence level in hierarchy
	- [x] Integer promotion on left operand; result type = promoted left
	- [x] shl for left shift; sar for arithmetic right shift
	- [x] Shift count in cl register
- [x] Bitwise binary operators & ^ |
	- [x] Tokenizer: & (distinct from address-of by context), ^ , |
	- [x] Three new expression levels: bitwise AND > bitwise XOR > bitwise OR
	- [x] Integer promotion; common type for mixed operands
	- [x] and, xor, or instructions
- [x] Remaining compound assignment operators
	- [x] Tokenizer: *= /= %= <<= >>= &= ^= |=
	- [x] All compound operators fully supported

## Stage 17 - Sizeof Operator

- [x] Tokenizer
	- [x] sizeof keyword
- [x] sizeof(type)
	- [x] Parser: sizeof "(" <type_name> ")"
	- [x] Emits constant value for type size
- [x] sizeof expr
	- [x] Parser: sizeof <unary_expr> (no parentheses required)
	- [x] Expression type determined without evaluation; no code emitted for operand
	- [x] Integer promotion applied to result type determination
- [x] sizeof array
	- [x] Whole-array size: element_size × element_count
	- [x] Array decay suppressed in sizeof context

## Stage 18 - Conditional (Ternary) Operator

- [x] Tokenizer
	- [x] ? (question mark)
- [x] Parser
	- [x] cond ? expr_true : expr_false
	- [x] Right-associative; precedence below logical OR, above assignment
	- [x] True branch re-enters parse_expression (top level)
- [x] AST
	- [x] AST_CONDITIONAL nodes; children: [condition, true_expr, false_expr]
- [x] Semantic Analysis
	- [x] Common type for true/false branches
	- [x] Pointer type compatibility; null pointer constant support
- [x] Code Generation
	- [x] Conditional branching; only selected branch evaluated

## Stage 19 - Comma Operator

- [x] Parser
	- [x] Comma as lowest-precedence operator in expressions
	- [x] Left-associative; distinct from comma separators in calls/parameters
- [x] AST
	- [x] AST_BINARY_OP (",") nodes
- [x] Semantic Analysis
	- [x] Result type = right operand type
- [x] Code Generation
	- [x] Left expression evaluated and discarded; right result kept

## Stage 20 - Declarator Refactoring and Multi-Declarator Lists

- [x] Declarator parsing refactored
	- [x] parse_declarator separate from parse_type_specifier
	- [x] Handles { * } prefix, identifier, optional array suffix
	- [x] Function declarator separated from variable declarator
- [x] Comma-separated init-declarator lists
	- [x] int a, b, c; and int a = 1, *b, c = 3;
	- [x] AST_DECL_LIST node wrapping multiple AST_DECLARATION children
	- [x] Per-declarator optional initializer (parse_assignment_expression)
	- [x] Array declarators rejected in multi-declarator lists
- [x] Regression test suite for declaration forms

---

## TODO

### Preprocessor
- [ ] #include <file> and #include "file"
- [ ] #define object-like macros
- [ ] #define function-like macros with parameters
- [ ] #undef
- [ ] #if / #ifdef / #ifndef / #elif / #else / #endif
- [ ] #pragma (at minimum #pragma once)
- [ ] Macro stringification (#) and token pasting (##)
- [ ] Predefined macros: __FILE__, __LINE__, __DATE__, __TIME__, __STDC__, __STDC_VERSION__
- [ ] Line splicing (backslash-newline)
- [ ] Multiline macros

### Types
- [ ] unsigned integer types: unsigned char, unsigned short, unsigned int, unsigned long
- [ ] unsigned arithmetic and comparisons (separate from signed)
- [ ] Integer suffix tokens: U/u, UL/ul, LL/ll, ULL/ull
- [ ] Type suffixes for all integer literal forms
- [ ] _Bool type
- [ ] float and double types
- [ ] Floating-point arithmetic and comparisons
- [ ] Floating-point literals (decimal and hex forms)
- [ ] Floating-point conversions (int ↔ float ↔ double)
- [ ] long long type (64-bit on all platforms)
- [ ] ptrdiff_t, size_t, intptr_t awareness
- [ ] void type (for void functions and void*)
- [ ] void* pointer type and implicit conversion to/from typed pointers
- [ ] Enum types (enum E { A, B, C })
- [ ] Struct types (struct S { int x; char y; })
- [ ] Union types
- [ ] typedef
- [ ] Struct and union member access (. and ->)
- [ ] Bit-field members in structs
- [ ] Flexible array members in structs
- [ ] Compound literals: (Type){ ... }
- [ ] Designated initializers for structs: .member = value
- [ ] const and volatile qualifiers
- [ ] restrict qualifier on pointers
- [ ] Type compatibility and composite type rules

### Declarations and Scope
- [ ] File-scope (global) variable declarations
- [ ] File-scope variable initialization
- [ ] static storage class (file scope and block scope)
- [ ] extern storage class
- [ ] register storage class (hint only)
- [ ] auto storage class (explicit)
- [ ] Tentative definitions for file-scope variables
- [ ] For-loop initializer declarations: for (int i = 0; ...)
- [ ] Multiple pointer levels in multi-declarator lists
- [ ] Function declarations at block scope
- [ ] Nested function-scope struct/enum declarations
- [ ] Incomplete array types in extern declarations

### Initializers
- [ ] Brace-enclosed initializers for arrays: int a[3] = {1, 2, 3}
- [ ] Partial initialization of arrays (remaining elements zero-initialized)
- [ ] Designated initializers for arrays: [idx] = value
- [ ] Struct initializers with braces
- [ ] Nested initializers
- [ ] Zero-initialization of static-duration objects

### Expressions
- [ ] sizeof with unsigned result type (size_t)
- [ ] Pointer subtraction (ptrdiff_t result)
- [ ] Pointer comparison operators (< <= > >= on pointers)
- [ ] Pointer equality with non-null constants
- [ ] Integer constant expressions (evaluated at compile time)
- [ ] Floating-point constant expressions
- [ ] Multi-dimensional arrays and indexing: a[i][j]
- [ ] Implicit conversion from array of T to pointer to T in all expression contexts
- [ ] Lvalue conversion rules for all expression contexts
- [ ] Assignment to struct lvalue (whole-struct copy)
- [ ] Unary + on floating-point
- [ ] Mixed integer/floating-point arithmetic (usual arithmetic conversions)
- [ ] Integer and floating-point promotions in function arguments (default argument promotions)

### Statements
- [ ] For-loop initializer declarations
- [ ] switch with long / char / short discriminant (after promotion)
- [ ] Case labels with unsigned and long constant values
- [ ] goto across declarations (only legal in C under restrictions)

### Functions
- [ ] Variadic functions: va_list, va_start, va_arg, va_end (<stdarg.h>)
- [ ] Old-style (K&R) function definitions
- [ ] Implicit return in void functions
- [ ] Recursive structs through pointers
- [ ] Function pointers: type (*fp)(params)
- [ ] Function pointer calls: (*fp)(args) and fp(args)
- [ ] Inline functions (_Bool inline keyword)

### Standard Library Headers (minimal implementations or pass-through)
- [ ] <stdio.h>: printf, scanf, fprintf, fopen, fclose, fread, fwrite, fgets, fputs, perror, sscanf, snprintf
- [ ] <stdlib.h>: malloc, calloc, realloc, free, exit, abort, atoi, atol, strtol, strtoul, qsort, bsearch
- [ ] <string.h>: strlen, strcpy, strncpy, strcat, strncat, strcmp, strncmp, memcpy, memmove, memset, memcmp, strchr, strrchr, strstr
- [ ] <math.h>: basic floating-point math functions
- [ ] <ctype.h>: isalpha, isdigit, isspace, toupper, tolower, etc.
- [ ] <stddef.h>: NULL, size_t, ptrdiff_t, offsetof
- [ ] <stdint.h>: int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, INT_MAX, etc.
- [ ] <limits.h>: CHAR_MAX, INT_MAX, LONG_MAX, etc.
- [ ] <assert.h>: assert macro
- [ ] <errno.h>: errno, ENOENT, EINVAL, etc.
- [ ] <stdbool.h>: bool, true, false

### Code Generation and Optimization
- [ ] Multi-file compilation (compile multiple .c files to .o files)
- [ ] Object file emission (.o / ELF)
- [ ] Linker invocation
- [ ] Debug information (DWARF)
- [ ] Stack frame alignment (currently assumed 16-byte; verify under all ABI conditions)
- [ ] Spill-and-restore for more than 6 arguments
- [ ] Calling convention for struct arguments and return values
- [ ] Floating-point ABI (xmm registers for float/double arguments)
- [ ] Tail-call opportunities
- [ ] Constant folding for integer expressions
- [ ] Dead code elimination
- [ ] Unreachable code warning
- [ ] Callee-saved register preservation (rbx, rbp, r12–r15)
- [ ] Red-zone usage or avoidance

### Diagnostics and Error Recovery
- [ ] Line and column numbers on all error messages
- [ ] Structured error output (file:line:col: error: message)
- [ ] Warning level support (-Wall, -Wextra)
- [ ] Multiple errors before aborting
- [ ] Pedantic C99 conformance checks
- [ ] Signed integer overflow detection mode

### Tooling and Infrastructure
- [ ] Multi-file / split compilation mode
- [ ] -o output file option
- [ ] -c compile-only (emit object file) option
- [ ] -S assembly output option (currently the only mode)
- [ ] -I include path option
- [ ] -D and -U macro definition flags
- [ ] -O optimization level flags
- [ ] Makefile / build system for the compiler itself
- [ ] Comprehensive conformance test suite against C99 standard
