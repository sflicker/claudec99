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

## Stage 21 - Declarator Unification and Declaration Compatibility

- [x] parse_declarator extended to detect function form (identifier followed by `(`)
	- [x] is_function field added to ParsedDeclarator
	- [x] Non-function declarator at top level produces clear error
- [x] parse_parameter_declaration uses parse_declarator (removes hand-coded pointer-star loop)
- [x] parse_function_decl uses parse_declarator
- [x] Function definitions and declarations separated as distinct grammar paths
	- [x] `<function_definition>` ::= type + function_declarator + block_statement
	- [x] `<declaration>` at file scope ::= type + init_declarator_list + ";"
- [x] Function declaration compatibility checks
	- [x] Return type mismatch detected between a declaration and later definition/re-declaration
	- [x] Parameter type mismatch detected between declarations

## Stage 22 - File-Scope Variable Declarations

- [x] File-scope (global) variable declarations
	- [x] Scalar types: int, char, short, long
	- [x] Pointer types: int *p;
	- [x] Array types: int arr[N]; (size required)
	- [x] Uninitialized globals zero-initialized to `.bss` section
	- [x] RIP-relative addressing via `[rel name]`
	- [x] GlobalVar table in CodeGen; load/store helpers separate from locals
- [x] File-scope variable initialization
	- [x] Constant integer expressions as initializers emitted to `.data` section
	- [x] Multi-declarator lists at file scope (int a = 1, b, c = 3;)
	- [x] Duplicate declaration detection
	- [x] Type conflict detection between successive declarations
	- [x] Function/object name conflict detection (ordinary identifier namespace)

## Stage 23 - Storage Class Specifiers (File Scope)

- [x] Tokenizer
	- [x] extern keyword
	- [x] static keyword
- [x] AST: StorageClass enum (SC_NONE, SC_EXTERN, SC_STATIC) + storage_class field on ASTNode
- [x] Parser: optional storage class specifier before type in external declarations
	- [x] extern: declares without allocating storage; no `.bss`/`.data` entry emitted
	- [x] static: internal linkage; no `global` NASM directive emitted for functions
	- [x] Repeated extern declarations allowed; extern then definition allowed
	- [x] Conflicting linkage (static vs non-static mixing) rejected
	- [x] Duplicate static definitions rejected
	- [x] extern declarations may not have an initializer
	- [x] Multiple storage class specifiers in one declaration rejected
	- [x] Storage class specifiers rejected at block scope

## Stage 24 - Parenthesized Declarators

- [x] Parser: "(" { "*" } identifier [ "[" size "]" ] ")" grouping form in parse_declarator
	- [x] int (*p) parsed identically to int *p
	- [x] int (**pp) for double-pointer grouping
	- [x] Parenthesized pointer parameters: int read(int (*p))
	- [x] Parenthesized file-scope and static declarations
	- [x] extern with parenthesized pointer: extern int (*p);
- [x] Rejection of unsupported suffixes (deferred until stage 25)
	- [x] Function pointer form (*fp)(int) rejected (no inner-star function suffix)
	- [x] Pointer-to-array form (*p)[10] rejected

## Stage 25 - Function Pointers

- [x] Type system: TYPE_FUNCTION kind
	- [x] type_function() constructor: return type, param_count, params[]
	- [x] type_kind_name() and type_is_integer() updated for TYPE_FUNCTION
- [x] Function pointer declarations
	- [x] Local: int (*fp)(int);
	- [x] File-scope: int (*fp)(int);
	- [x] static: static int (*fp)(int);
	- [x] extern: extern int (*fp)(int); followed by definition
	- [x] Parameter: int apply(int (*fp)(int), int x)
	- [x] Pointer-to-function-returning-pointer: int *(*fp)(int *)
	- [x] Full type compatibility checking on redeclarations (return type, param count, param types)
- [x] Function designators as values (assignment and initialization)
	- [x] Function name used as rvalue: fp = f; (emits lea rax, [rel name])
	- [x] Local function pointer initialization from function name
	- [x] File-scope function pointer initialization from function name (`.data` section, dq label)
	- [x] Type checking: return type and parameter types must match exactly
- [x] Indirect function calls
	- [x] AST_INDIRECT_CALL node type
	- [x] fp(args) form: function-pointer variable called directly
	- [x] (*fp)(args) explicit dereference form
	- [x] Argument count validation against function pointer type
	- [x] Argument type validation (integer promotions and pointer checks)
	- [x] Return type flows from function pointer type to call expression

## Stage 26 - General Declarator Cleanup

- [x] Param-list consumption moved into parse_declarator (inline)
	- [x] parse_func_ptr_param_types and build_func_ptr_type helpers removed
	- [x] fp_param_types[] and fp_param_count added to ParsedDeclarator
- [x] Unnamed parameters in function prototypes and function pointer signatures
	- [x] parse_parameter_declaration: declarator (and name) made optional
	- [x] Function definitions still require named parameters
- [x] Grammar: `<direct_declarator>` suffixes ([...] and (...)) apply recursively to any direct_declarator
- [x] Error message updated: "functions returning function pointers are not supported" for int (*fp())(int)

## Stage 27 - Declaration Specifier Refactor

- [x] parse_declaration_specifiers helper
	- [x] Loop-based collection: storage class and type specifier accepted in any order
	- [x] Duplicate storage class specifier rejected (e.g. `static extern int x;`)
	- [x] Duplicate type specifier rejected (e.g. `int int x;`)
	- [x] Missing type specifier rejected
- [x] TOKEN_TYPEDEF recognized as a storage class specifier in the loop
- [x] Grammar updated: `<declaration_specifiers> ::= <declaration_specifier>+`

## Stage 28 - Typedef Support

- [x] Stage 28-01: Simple scalar typedefs
	- [x] TOKEN_TYPEDEF keyword; SC_TYPEDEF storage class; AST_TYPEDEF_DECL node
	- [x] Scoped typedef table in Parser (scope_depth tracks enter/leave block)
	- [x] Typedef declarations at file scope and block scope
	- [x] Typedef names recognized in: object declarations, function return/parameter types, casts, sizeof(type)
	- [x] Duplicate typedef detection within the same scope
	- [x] Block-scope typedefs expire when the block exits
	- [x] typedef declarations may not have initializers
- [x] Stage 28-02: Pointer typedefs (`typedef int *IP;`)
	- [x] TypedefEntry.full_type carries the full pointer Type* chain
	- [x] Pointer typedef names decay correctly in all declaration contexts
- [x] Stage 28-03: Function-pointer typedefs (`typedef void (*FP)(int);`)
	- [x] Registered as TYPE_POINTER with full TYPE_POINTER → TYPE_FUNCTION chain
- [x] Stage 28-04: Array typedefs (`typedef int A[4];`)
	- [x] Registered as TYPE_ARRAY with full type_array(base, length) chain
	- [x] Multi-declarator list with typedef'd array base (each declarator gets its own array slot)
	- [x] Subscript base in parse_postfix relaxed to accept AST_DEREF (enables `(*p)[i]`)
	- [x] codegen: emit_array_index_addr handles AST_DEREF base for pointer-to-array indexing

## Stage 29 - Enum Support

- [x] Tokenizer: enum keyword
- [x] parse_enum_specifier
	- [x] Named and anonymous enum forms: `enum E { ... }` and `enum { ... }`
	- [x] Explicit enumerator values: integer literal or character literal (Stage 29); extended to full integer constant expressions (Stage 99)
	- [x] Auto-increment for subsequent enumerators
	- [x] Trailing comma in enumerator list
	- [x] Duplicate enumerator name detection
	- [x] Enum tag table; undefined tag reference rejected (Stage 29); forward-declared tags accepted (Stage 99)
- [x] Enum constants folded to AST_INT_LITERAL (TYPE_INT) at parse time
- [x] `enum Tag` used as a type specifier resolves to int
- [x] Standalone enum declarations at file scope and block scope

## Stage 30 - Struct Definitions

- [x] Tokenizer: struct keyword
- [x] Type system: TYPE_STRUCT kind; type_struct(size, alignment) constructor
- [x] parse_struct_specifier
	- [x] Named struct definitions: `struct Tag { field-list }`
	- [x] Natural-alignment field layout: each field padded to its type's alignment
	- [x] Struct size rounded up to the maximum field alignment
	- [x] Empty struct: 1 byte by convention
	- [x] Struct tag table in Parser
- [x] sizeof on struct type and struct instances
- [x] Local struct variable allocation (full struct size reserved on stack)
- [x] `struct Tag` recognized as a type specifier in declarations and sizeof

## Stage 31 - Struct Member Access

- [x] Tokenizer: TOKEN_DOT (.) and TOKEN_ARROW (->)
- [x] AST: AST_MEMBER_ACCESS and AST_ARROW_ACCESS node types
- [x] Parser: parse_struct_specifier records field name and byte offset into StructField entries on the Type
- [x] parse_postfix handles `.` and `->` in the postfix repetition loop
- [x] Both operators accepted as lvalues for assignment
- [x] Code generation
	- [x] emit_member_addr: `expr.field` → `lea rax, [rbp - (offset - field_offset)]`
	- [x] emit_arrow_addr: `expr->field` → load pointer, add field offset
	- [x] Rvalue load and lvalue store paths for both operators

## Stage 32 - Aggregate Initializer Lists

- [x] AST: AST_INITIALIZER_LIST node type
- [x] parse_initializer: handles `{ expr, ... }` recursively; falls through to parse_assignment_expression for scalars
- [x] Array brace initializers: per-element stores; remaining elements zero-filled
- [x] Struct brace initializers: field-by-field stores; remaining fields zero-filled
- [x] Trailing comma allowed in initializer list
- [x] Empty `{}` zero-fills all elements/fields
- [x] Nested brace initializers (recursion)
- [x] Brace initializer rejected for scalar declarations

## Stage 33 - Struct Assignment and Copy Initialization

- [x] Whole-struct assignment: `dst = src` copies the full struct byte-by-byte
- [x] Struct declaration-initializer copy: `struct S b = a;`
- [x] Type mismatch between distinct struct types rejected
- [x] codegen: emit_struct_copy (byte-by-byte movzx/mov loop)

## Stage 34 - Struct Pointer Parameters and Mutation

- [x] `(*p).field` syntax: emit_member_addr handles AST_DEREF as the base of AST_MEMBER_ACCESS
- [x] Struct pointer parameters: passing `&local_struct` to a pointer parameter
- [x] Mutation through pointer parameters visible to the caller via `->` and `(*p).`

## Stage 35 - Nested Structs and Arrays of Structs

- [x] Nested struct member access: `r.origin.x` (emit_member_addr with AST_MEMBER_ACCESS base, recursive address accumulation)
- [x] Array-of-structs indexing: `points[0].x` (emit_member_addr with AST_ARRAY_INDEX base)

## Stage 36 - Struct Typedefs

- [x] `typedef struct Tag Alias;` — alias for a previously defined (complete) struct
- [x] `typedef struct Tag { ... } Alias;` — inline struct definition plus typedef in one declaration
- [x] Typedef carries the full TYPE_STRUCT Type* for correct size/alignment

## Stage 37 - Incomplete Struct Declarations

- [x] Forward declarations: `struct Tag;` creates an incomplete placeholder (size=0, alignment=0)
- [x] In-place completion: when the struct body is later provided, the placeholder is mutated so existing Type* references (e.g. in typedef entries) automatically reflect the completed layout
- [x] Field guard: non-pointer field with incomplete struct type is a compile-time error
- [x] Pointer-to-incomplete struct allowed (enables opaque/forward-declared types)
- [x] Self-referential structs through pointer fields (e.g. linked-list nodes)

## Stage 37-02 - Additional Struct Tests and Codegen Fixes

- [x] codegen: emit_arrow_addr handles AST_MEMBER_ACCESS base — enables chained dot-then-arrow access (`n.next->value`)
- [x] codegen: AST_SIZEOF_TYPE rejects `sizeof(struct Tag)` when the struct is incomplete
- [x] codegen: AST_DECLARATION struct path rejects variable declarations with incomplete struct type

## Stage 38 - void Type and void Pointer

- [x] Tokenizer: TOKEN_VOID keyword
- [x] TYPE_VOID kind in type system; type_void() constructor
- [x] void as return type for functions
- [x] void parameter list: `int f(void)` = zero parameters
- [x] bare `return;` in void functions (no expression)
- [x] void functions emit implicit epilogue on fall-off-end
- [x] void object declaration rejected (no void variables)
- [x] sizeof(void) rejected at parse time
- [x] void* pointer type
	- [x] implicit conversion to/from any non-function pointer type in assignments, returns, and call arguments
	- [x] dereferencing, arithmetic, and subscripting void* rejected

## Stage 39 - Minimal const Qualifier

- [x] Tokenizer: TOKEN_CONST keyword
- [x] type_qualifier production in grammar
- [x] `const` accepted in parse_declaration_specifiers, parse_statement, parse_parameter_declaration, parse_type_name, parse_declarator
- [x] is_const field on ASTNode, LocalVar, GlobalVar
- [x] is_const=1 set on AST_DECLARATION when no pointer depth and no array
- [x] Assignment to const-qualified lvalue rejected at codegen (semantic check)
- [x] Pointer-level const enforcement (writes through const pointers) deferred

## Stage 40 - Unsigned Integer Types and size_t

- [x] Tokenizer: TOKEN_UNSIGNED keyword
- [x] parse_type_specifier handles `unsigned [ char | short | int | long ]` and plain `unsigned` (= unsigned int)
- [x] is_unsigned flag on ASTNode, LocalVar, GlobalVar; propagated through all declaration contexts
- [x] Unsigned variants of all scalar types via is_signed=0 in Type struct
- [x] Usual Arithmetic Conversions for mixed signed/unsigned operands
- [x] movzx (zero-extend) for unsigned loads vs movsx (sign-extend) for signed loads
- [x] div vs idiv for unsigned vs signed division
- [x] shr (logical) vs sar (arithmetic) for right shift
- [x] Unsigned comparison instructions: setb/seta/setbe/setae vs setl/setg/setle/setge
- [x] Integer literal U/u suffix: sets is_unsigned on token and AST_INT_LITERAL node

## Stage 41 - Pointer Arithmetic Completion

- [x] Pointer addition and subtraction scaled by element size (p ± n, n + p)
- [x] Pointer prefix/postfix increment and decrement (++p, p++, --p, p--)
- [x] Pointer compound assignment: p += n, p -= n
- [x] Pointer difference (p1 - p2): signed division by element size, result type long
	- [x] Both operands must have identical pointee types
	- [x] void* and function pointer operands rejected
- [x] Arrow access through computed pointer expressions (e.g., `(p + 1)->field`)

## Stage 42 - Arrays of Pointers and Multi-level Subscript

- [x] parse_postfix: AST_ARRAY_INDEX accepted as subscript base (enables a[i][j])
- [x] codegen: emit_array_index_addr handles AST_ARRAY_INDEX base (loads pointer stored at inner element, then indexes)
- [x] Pointer type compatibility enforced on array-element assignment (allows null constant 0, rejects incompatible pointer types)

## Stage 43 - File-scope Array and String Initializers

- [x] File-scope array declarations with brace-list initializers: `int a[] = {1, 2, 3};`
- [x] File-scope array declarations with string-literal initializers: `char s[] = "hello";`
- [x] File-scope pointer initialized from string literal: `char *p = "str";`
- [x] Array size inference from initializer list for both block-scope and file-scope arrays
- [x] Codegen: char array from string literal (.data with db bytes), char* from string literal (dq address), array of char* from list (dq addresses), integer array from list (dq values)

## Stage 44 - Aggregate Initializers for Structs and Arrays of Structs

- [x] Struct brace-list initializers at file scope: `struct S s = {1, 2};`
- [x] Struct brace-list initializers in local declarations (block scope)
- [x] Array of structs initializer with nested braces
- [x] Explicit array size with more initializers than elements → compile-time error
- [x] Field-level type checking in struct initializers (string literal for non-pointer field rejected, non-null integer for pointer field rejected)
- [x] codegen: emit_local_struct_init and emit_global_struct helpers; recursive initialization with correct padding and offset tracking

## Stage 45 - libc Prototypes and Integration Test Harness

- [x] Integration test harness: test/integration/ subdirectories with companion files (.expected, .libs, .args, .input, .status, .cflags)
- [x] Multi-file test support: all .c files in a test directory compiled and linked together
- [x] void* argument/return compatibility at call sites (pointer_types_assignable replaces pointer_types_equal)
- [x] Abstract pointer parameters in prototypes: `int puts(char *)` — unnamed pointer parameter with no identifier

## Stage 46 - Command-line Arguments (main argc/argv)

- [x] main(int argc, char **argv) pattern confirmed working (no new compiler changes needed)
- [x] Integration test coverage for argv[1] access and argc return

## Stage 47 - Multi-file Integration Test Infrastructure

- [x] Integration tests restructured into per-test subdirectories
- [x] Multiple .c files in one test directory compiled and linked together
- [x] Test runner compiles each .c to .o, links all objects

## Stage 48 - Preprocessor Foundation

- [x] New preprocessor module (include/preprocessor.h, src/preprocessor.c)
- [x] Phase 1: backslash-newline line splicing
- [x] Phase 2: // and /* */ comment removal (replaced with single space; string/char literals protected)
- [x] Preprocessor directives detected and rejected with error (foundation for future directives)
- [x] Lexer simplified: comment handling moved out of lexer into preprocessor
- [x] Compiler flow: preprocess() called before lexer initialization

## Stage 49 - Local Quoted Includes

- [x] `#include "file.h"` directive
- [x] Include file resolved relative to the including file's directory
- [x] Include recursion depth limit (64 levels) with error reporting
- [x] Non-include unsupported directives produce diagnostic error

## Stage 50 - Object-like Macros

- [x] `#define NAME replacement` — object-like macro definition and storage
- [x] MacroTable shared across all preprocessing calls (macros visible across included files)
- [x] Identifier-level expansion in ordinary source text
- [x] Compatible redefinitions silently accepted; incompatible redefinitions → fatal error

## Stage 51 - Function-like Macros

- [x] `#define NAME(params) replacement` — function-like macro with parameter substitution
- [x] Exact argument-count validation
- [x] Arguments may contain nested parentheses and arbitrary token sequences
- [x] Arguments pre-expanded before substitution; nested macro invocations recursively expanded
- [x] Zero-parameter function-like macros: `#define FOO() 42`

## Stage 52-01 - Basic Conditional Preprocessing

- [x] `#ifdef NAME` / `#ifndef NAME` / `#else` / `#endif` directives
- [x] Nesting support (max depth 64) with stack-based CondFrame tracking
- [x] Inactive regions suppress `#define` and `#include` processing
- [x] Newlines preserved inside inactive regions (source line structure maintained)
- [x] Diagnostic errors: missing #endif, duplicate #else, mismatched directives

## Stage 52-02 - Nested Conditional Processing

- [x] All nesting combinations of #ifdef / #ifndef / #else work correctly
- [x] Include guard pattern supported (duplicate #include with guard macro)

## Stage 52-03 - #if Constant Conditionals

- [x] `#if <integer-literal>` directive: 0 → false, nonzero → true
- [x] Nesting with existing #ifdef/#ifndef frame stack

## Stage 52-04 - #elif Constant Conditionals

- [x] `#elif <integer-literal>` directive with first-true-wins branch selection
- [x] branch_taken field in CondFrame ensures later branches skipped after first true
- [x] Chains with #ifdef, #ifndef, #if, and #else

## Stage 53 - Predefined Macros __FILE__ and __LINE__

- [x] `__FILE__` expands to a string literal of the current source file path
- [x] `__LINE__` expands to a decimal integer literal of the current source line number
- [x] Recognized before user-defined macro table lookup

## Stage 54 - #undef Directive

- [x] `#undef NAME` removes a macro from the macro table (no-op if undefined)
- [x] #undef in inactive conditional regions is skipped

## Stage 55-01 - defined() Operator in Preprocessor Conditionals

- [x] `defined(NAME)` expression in `#if` and `#elif`: 1 if defined, 0 otherwise

## Stage 55-02 - Macro Expansion in Preprocessor Conditionals

- [x] Object-like macro identifiers expanded to integer values in `#if`/`#elif`
- [x] `defined NAME` (without parentheses) accepted alongside `defined(NAME)`

## Stage 55-03 - Undefined Identifiers Evaluate to 0

- [x] Bare undefined identifiers in `#if`/`#elif` expressions evaluate to 0

## Stage 55-04 - Unary Operators in Preprocessor Conditionals

- [x] Unary `!`, `-`, `+` in `#if`/`#elif` expressions
- [x] Chained unary operators (e.g., `!-1`)

## Stage 55-05 - Parenthesized Preprocessor Expressions

- [x] `(expr)` grouping in `#if`/`#elif` expressions; recursive evaluation

## Stage 55-06 - Equality and Relational Operators in Preprocessor Conditionals

- [x] `==`, `!=`, `<`, `<=`, `>`, `>=` in `#if`/`#elif` expressions (left-associative)

## Stage 55-07 - Logical Operators in Preprocessor Conditionals

- [x] `&&` and `||` in `#if`/`#elif` expressions with C-like precedence

## Stage 55-08 - Arithmetic Operators in Preprocessor Conditionals

- [x] `+`, `-`, `*`, `/`, `%` in `#if`/`#elif` expressions; division/modulo by zero → fatal error

## Stage 55-09 - Bitwise and Shift Operators in Preprocessor Conditionals

- [x] `~` (unary complement), `&`, `^`, `|`, `<<`, `>>` in `#if`/`#elif` expressions
- [x] Correct precedence: shift > relational; bitwise AND > XOR > OR > logical AND

## Stage 56-01 - #error Directive

- [x] `#error message` halts compilation with the supplied message on stderr (exit 1)
- [x] `#error` in inactive conditional regions is silently skipped

## Stage 56-02 - Command-line -D Macro Definitions

- [x] `-DNAME` defines NAME as 1; `-DNAME=VALUE` defines NAME as VALUE
- [x] .cflags companion file for integration tests to pass compiler flags
- [x] preprocess_with_defines() injects command-line macros before source processing

## Stage 56-03 - Command-line -I Include Search Paths

- [x] `-I<dir>` and `-I <dir>` add directories to the include search path for quoted includes
- [x] resolve_include_path() searches in-directory first, then -I directories in order

## Stage 56-04 - Angle-bracket Includes

- [x] `#include <filename>` searches -I directories only (not the including file's directory)
- [x] Error messages distinguish quoted vs angle-bracket forms

## Stage 56-05 - Standard Predefined Macros

- [x] `__STDC__` expands to 1
- [x] `__STDC_VERSION__` expands to 199901
- [x] `__CLAUDEC99__` expands to 1
- [x] All three behave as ordinary object-like macros (can be #undef'd or compatibly redefined)

## Stage 57-01 - Macro Stringification (#)

- [x] `#param` inside function-like macro replacement list converts argument to string literal
- [x] Whitespace normalized; `"` and `\` escaped in the produced string
- [x] `#` in object-like macros, bare `#`, and `#` not followed by a parameter name → error

## Stage 57-02 - Token Pasting (##)

- [x] `##` inside function-like macro replacement list concatenates adjacent tokens
- [x] `##` at start or end of replacement list → error
- [x] `##` in object-like macros → error

## Stage 57-03 - Variadic Function Declarations and Calls

- [x] Tokenizer: TOKEN_ELLIPSIS (`...` as a single token)
- [x] Grammar/Parser: optional trailing `...` in parameter lists sets is_variadic on AST_FUNCTION_DECL and FuncSig
- [x] Call-site arity: variadic functions require at least the fixed parameter count (not exact)
- [x] Codegen: emits `xor eax, eax` before call for variadic functions (SysV AMD64 AL = float arg count)
- [x] Variadic function *definitions* (va_list, va_start, va_arg, va_end) not yet supported

## Stage 57-04 - Variadic Macros (__VA_ARGS__)

- [x] `#define M(...)` and `#define M(x, ...)` — variadic function-like macros
- [x] `__VA_ARGS__` substituted with comma-separated extra arguments in the replacement list
- [x] Variadic macros accept arg_count >= param_count (fixed args required, extra optional)
- [x] Too few fixed arguments → compile-time error

## Stage 58 - Controlled Standard-Style Test Headers

- [x] Stub headers added to `test/include/` for angle-bracket include support
  - [x] `stdio.h` — `puts`, `printf`
  - [x] `stddef.h` — `NULL`, `size_t`
  - [x] `stdlib.h` — `malloc`, `free`, `exit`
  - [x] `string.h` — `strlen`, `strcmp`, `strcpy`, `memcpy`, `memset`
- [x] Test runner appends `-I${PROJECT_DIR}/test/include` automatically
- [x] Integration tests updated to use angle-bracket forms for standard headers

## Stage 59 - Controlled Header Hardening

- [x] 15 new integration tests exercising preprocessor, stub headers, and compiler features together
- [x] Integration test infrastructure for angle-bracket include validation

## Stage 60-01 - Static Predefined ABI/Target Macros

- [x] 12 ABI-reflecting macros injected unconditionally before user source
  - [x] `__x86_64__`, `__linux__`, `__unix__`, `__LP64__`, `_LP64`
  - [x] `__CHAR_BIT__` (8)
  - [x] `__SIZEOF_CHAR__` (1), `__SIZEOF_SHORT__` (2), `__SIZEOF_INT__` (4), `__SIZEOF_LONG__` (8)
  - [x] `__SIZEOF_POINTER__` (8), `__SIZEOF_SIZE_T__` (8)

## Stage 60-02 - Runtime Context Predefined Macros

- [x] `__DATE__` expands to `"Mmm DD YYYY"` (compile-time date)
- [x] `__TIME__` expands to `"HH:MM:SS"` (compile-time time)
- [x] `__STDC_HOSTED__` expands to `1`
- [x] All three injected before user source is processed

## Stage 61 - Signed Keyword and Suffix Formalization

- [x] `signed` keyword accepted as a type specifier
  - [x] `signed char`, `signed short`, `signed int`, `signed long`
  - [x] Plain `signed` (= `signed int`)
- [x] Trailing `int` forms: `short int`, `long int`, `unsigned short int`, `unsigned long int`, `signed short int`, `signed long int`
- [x] `UL`/`LU` (unsigned long) integer literal suffixes formalized
- [x] Declaration specifier parser updated for all combinations

## Stage 62 - limits.h and stdint.h Stub Headers

- [x] `test/include/limits.h` — complete set including `LLONG_MIN`, `LLONG_MAX`, `ULLONG_MAX`
- [x] `test/include/stdint.h`
  - [x] Exact-width signed types: `int8_t`, `int16_t`, `int32_t`, `int64_t`
  - [x] Exact-width unsigned types: `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
  - [x] Pointer-size types: `intptr_t`, `uintptr_t`
  - [x] Least-width and fast variants for 8/16/32/64 bits

## Stage 63 - _Bool Type and stdbool.h

- [x] `_Bool` type (TYPE_BOOL) added to type system
  - [x] 1-byte storage, unsigned
  - [x] Value-normalization: any nonzero value assigned to `_Bool` becomes 1, zero stays 0
  - [x] Integer promotion: `_Bool` promotes to `int` in expressions
- [x] `test/include/stdbool.h` stub
  - [x] `bool` expands to `_Bool`
  - [x] `true` expands to 1, `false` expands to 0
- [x] `__SIZEOF_LONG_LONG__` added to stage-60-01 ABI macros

## Stage 64 - long long and unsigned long long

- [x] Tokenizer: `long long` recognized as a two-keyword sequence
- [x] Type system: `TYPE_LONG_LONG` and `TYPE_UNSIGNED_LONG_LONG` (8 bytes, alignment 8)
- [x] Codegen: `long long` uses 64-bit register file, identical to `long`
- [x] Integer literal suffixes: `LL`/`ll` (long long), `ULL`/`LLU`/`ull`/`llu` (unsigned long long)
- [x] Usual Arithmetic Conversions updated for `long long` and `unsigned long long` ranks
- [x] `__SIZEOF_LONG_LONG__` predefined macro (= 8)
- [x] `typedef` support for `long long` and `unsigned long long` base types

## Stage 65 - Integer Conversion and Arithmetic Hardening Tests

- [x] 17 new `test/valid/` tests covering:
  - [x] `_Bool` promotion via `stdbool.h` (b = 99 → 1; 1 + 41 = 42)
  - [x] `int8_t` sign-preserving promotion (stdint.h)
  - [x] `uint8_t` promotes to `int`, not `unsigned int` (stdint.h)
  - [x] `int16_t` sign-preserving promotion (stdint.h)
  - [x] `uint16_t` promotes to `int` (stdint.h)
  - [x] Signed/unsigned int UAC: `int -1 < unsigned int 1U` → 0
  - [x] Plain `signed`/`unsigned` UAC comparison → 0
  - [x] Negative literal vs unsigned literal: `-1 < 1U` → 0
  - [x] LP64 rule: `long -1L < unsigned int 1U` → 1 (uint converts to long)
  - [x] Signed long vs unsigned long: `-1L < 1UL` → 0
  - [x] `long long` dominates `int` in UAC
  - [x] `unsigned long long` dominates `long long`: `-1LL < 1ULL` → 0
  - [x] Assignment truncation: `uint8_t x = 257U` → 1
  - [x] `_Bool` assignment normalization (0 stays 0; nonzero → 1)
  - [x] Conditional expression mixed types: `cond ? 42U : 9L` → common type long
  - [x] Compound assignment truncation: `uint8_t 250 += 10` → 4 (wraps)
  - [x] Unsigned multiplication wraparound: `1000000ULL * (ull)(-1) > 0`

## Stage 66 - Const Pointer Compatibility Hardening

- [x] Pointer-level const enforcement (semantic, no grammar changes)
	- [x] Parser tracks `pointer_is_const` in declarators; consumes optional `const` after each `*`
	- [x] `is_const` field on `Type`; `type_const_copy()` for const-qualified copies without mutating singletons
	- [x] Write through a const pointer rejected (error)
	- [x] Reassignment of a const pointer rejected (error)
	- [x] const-to-non-const conversion in assignments: warning, or error under `-Werror`
	- [x] `codegen_warn()` / `codegen_warn_const_discard()` helpers; pointer type derivation carries const

## Stage 67 - File I/O Stub Declarations (stdio.h)

- [x] Stage 67-01: opaque `typedef struct FILE FILE;` and `#define EOF (-1)`
- [x] Stage 67-02: `fopen`, `fclose`, `fgetc` declarations
- [x] Stage 67-03: `fgets` (line input)
- [x] Stage 67-04: `fprintf` (file output)
- [x] Stage 67-05: `snprintf` (formats into buffer via existing variadic mechanism)
- [x] No compiler changes — declarative stub additions only

## Stage 68 - More Than 6 Function-Call Arguments

- [x] Removed hard-coded 6-argument limit (parser and codegen)
- [x] Caller side: System V AMD64 stack-passing for args 7+
	- [x] 16-byte stack alignment padding computed
	- [x] Stack args (6..N-1) pushed right-to-left; register args (0..5) evaluated left-to-right
	- [x] Cleanup with `add rsp, (num_stack_args + padding) * 8`
	- [x] Indirect calls follow same strategy (callee address saved/restored via r10)
- [x] Callee side: stack parameters copied from `[rbp + 16 + (i-6)*8]` with sign/zero extension by type

## Stage 69 - Memory-Related Standard Header Functions

- [x] `stdlib.h`: `realloc` (corrected to `void *` per C99)
- [x] `string.h`: `memcpy`, `memset`, `memcmp`, `strchr`
- [x] No compiler changes — declarative stub additions only

## Stage 70 - Tooling and Diagnostics Infrastructure

- [x] Stage 70-00/70: mini compiler-shaped integration test (exercises existing features end-to-end)
- [x] Stage 70-01: versioning and error-management infrastructure
	- [x] `--version` (format MM.mm.SSSSSSSS.BBBBB; build number from `git rev-list --count`)
	- [x] `--max-errors=N` (default 1 = exit on first error)
	- [x] Parser error recovery via setjmp/longjmp; `parser_sync()` to next declaration boundary
	- [x] ~104 parser + ~116 codegen `exit(1)` pairs replaced with uniform `compile_error()`
- [x] Stage 70-02: line/column tracking in tokens
	- [x] `SourceFile` struct; `line`, `col`, `file` fields on `Token`; `lexer_advance()` / `token_set_pos()`
	- [x] `[line:col]` column in print-tokens output (dynamic zero-padded width)
	- [x] Include-boundary location markers (`\x01<line>:<path>\n`); per-path SourceFile pool
- [x] Stage 70-03: line/column in errors and warnings
	- [x] `compile_error_at(file, line, col, ...)` / `compile_warning_at(...)`; `PARSER_ERROR` macro
	- [x] All 107 parser error sites carry `<file>:<line>:<col>:` prefix
	- [x] Global `g_warnings_are_errors` controlled by `-Werror`
	- [ ] Codegen (non-parser) errors/warnings still print without position prefix (AST nodes lack token info)

## Stage 71 - Block-Scope Static Variables

- [x] `static` accepted at block scope in `parse_statement`; constant-initializer validation
- [x] `is_static` + `static_label[256]` on `LocalVar`; `LocalStaticVar` tracking; scope/shadowing via existing checks
- [x] All access paths (load/store/address-of/inc-dec/subscript/member) emit RIP-relative `[rel Lstatic_func_N]`
- [x] `codegen_emit_local_statics()` emits to `.bss` (uninitialized) or `.data` (initialized)
- [x] Non-constant initializer rejected; arrays/structs out of scope

## Stage 72 - Named Union Support

- [x] Tokenizer: `union` keyword
- [x] `parse_union_specifier()` with union tag table; `TYPE_UNION` in type/declaration-specifier paths
- [x] `StructField` reused for members (all offsets 0); union size = max member size
- [x] Member access (`.` / `->`), sizeof, local/global decls, whole-union assignment, BSS emission
- [x] First-member initialization via brace lists; incomplete union variable declarations rejected

## Stage 73-01 - Anonymous Struct/Union Type Declarations

- [x] Optional tag when a body is present (struct and union specifiers)
- [x] Anonymous definitions allocate fresh unique `Type*`; type identity by pointer
- [x] Distinct anonymous types with identical layouts correctly fail assignment as type mismatch
- [x] Error when neither tag nor body is present

## Stage 74 - Controlled Header Gap Fill

- [x] Stub headers: `ctype.h`, `errno.h`, `time.h`, `setjmp.h`
	- [x] `ctype.h`: classification (isalpha, isdigit, isspace, etc.)
	- [x] `errno.h`: `errno`, `ERANGE`, `EINVAL`, …
	- [x] `time.h`: `time_t`, time functions
	- [x] `setjmp.h`: non-local jump support
- [x] Codegen bugfix: null pointer constant (integer 0) allowed as argument to a pointer parameter (`EMIT_ARG_CONVERT`) — enables `time(0)`

## Stage 75 - Variadic Function Definitions and stdarg.h

- [x] Stage 75-01: variadic function *definitions*
	- [x] Unnamed fixed parameters allowed when function is variadic
	- [x] Caller rule: `actual_arg_count >= fixed_param_count`
	- [x] Prologue skips unnamed parameters (register- and stack-param loops)
- [x] Stage 75-02: `stdarg.h` surface
	- [x] `va_list` typedef as `struct __claudec00_va_list_tag [1]`
	- [x] `va_start`/`va_end`/`va_copy`/`va_arg` macros expand to `__builtin_*` intrinsics
	- [x] Array-to-pointer decay for array-typedef parameters (C99 §6.7.5.3p7)
- [x] Stage 75-03: builtin parsing and semantic recognition
	- [x] `parse_primary` recognizes `__builtin_va_start/end/copy/arg`
	- [x] AST nodes: `AST_BUILTIN_VA_START`, `_END`, `_COPY`, `_ARG`
	- [x] `current_func_is_variadic` on Parser; `va_start` requires variadic context + 2 args; `va_arg` parses type-name second arg
- [x] Stage 75-04: va_start codegen foundation
	- [x] 304-byte register save area allocated for variadic functions; prologue saves rdi–r9
	- [x] `__builtin_va_start` initializes gp_offset/fp_offset/overflow_arg_area/reg_save_area
	- [x] `vfprintf`/`vprintf`/`vsnprintf` added to `stdio.h`; va_list forwarded to libc
	- [x] Parser fix: unnamed array-typedef parameters get pointer decay in early-return path
- [x] Stage 75-05: additional va_list integration tests (0, 6, 10 arguments)
- [x] Stage 75-06: `va_arg` for GP-class types
	- [x] Full SysV AMD64 impl: gp_offset range check, register-save-area path, overflow-stack path
	- [x] int / unsigned int (4-byte) and long / long long / unsigned long long / pointer (8-byte) loads
	- [x] Rejects small promoted types (char, short, _Bool), aggregates by value, arrays, void
- [ ] `va_arg` for floating-point and struct-by-value types (deferred)
- [ ] `va_copy` codegen (still a no-op stub)

## Stage 76 - For-Loop Initializer Declarations

- [x] Grammar: `<for_statement>` uses a `<for_init>` clause
- [x] `parse_for_statement` opens a scope before parsing init
	- [x] Type-specifier lookahead → declaration (parse_statement)
	- [x] Otherwise → expression (parse_expression_statement)
- [x] Declared variables in scope for condition, update, and body
- [x] Codegen saves/restores `scope_start` and `local_count` across the loop
- [x] Scope-leak and void-type declarations rejected

## Stage 77 - Enum and Constant Expressions in Case Labels

- [x] `eval_case_const_primary` / `eval_case_const_unary` / `eval_case_const_expr`
- [x] Case labels accept integer literals, character literals, enum constants
- [x] Unary and binary `+` / `-` over constant operands
- [x] Non-constant case label expressions rejected with a clear diagnostic
- [x] Folded value stored as AST_INT_LITERAL (no codegen change)

## Stage 78 - General Postfix Expression Chaining

- [x] Postfix operators chain on any postfix base (not just identifiers)
- [x] Subscript base accepts AST_MEMBER_ACCESS and AST_ARROW_ACCESS
- [x] Array-typed struct/union members wrapped in `type_array()` (TYPE_ARRAY)
- [x] codegen: `emit_array_index_addr` handles member/arrow bases
	- [x] array-typed fields: result register is the base address
	- [x] pointer-typed fields: dereference load before indexing
- [x] Enables chains like `p->tokens[i].kind`

## Stage 79 - General Lvalue Compound Assignment

- [x] Compound operators (`+= -= *= /= %= <<= >>= &= ^= |=`) on any modifiable lvalue
- [x] Desugar `lhs op= rhs` → `lhs = lhs op rhs` (AST_ASSIGNMENT + AST_BINARY_OP)
- [x] `ast_clone()` deep-copies the lvalue subtree to avoid node aliasing
- [x] Lvalue-validity check applies to compound forms
- [x] No codegen change (reuses two-child AST_ASSIGNMENT path)

## Stage 80 - Prefix/Postfix ++/-- on General Lvalues

- [x] Removed AST_VAR_REF-only restriction in `parse_postfix` and `parse_unary`
- [x] `codegen_inc_dec_general()` handles AST_ARRAY_INDEX, AST_MEMBER_ACCESS, AST_ARROW_ACCESS, AST_DEREF
	- [x] reuses stage-79 address helpers
	- [x] pointers scale by pointee size; old value (postfix) / new value (prefix)
- [x] Non-lvalue and const-qualified operands rejected at codegen
- [x] Unblocks idioms like `g->data[g->len++] = c`

## Stage 81 - Header Updates and Compiler Limits

- [x] `stdio.h`: `putchar`; `stdlib.h`: `calloc`
- [x] Removed restriction blocking `!` on pointer operands (C99 scalar semantics)
- [x] Raised PARSER_MAX_FUNCTIONS, PARSER_MAX_GLOBALS, MAX_GLOBALS, MAX_LOCALS from 64 to 256

## Stage 82 - const/volatile Qualifier Hardening

- [x] Stage 82-01: `const` in struct/union member declarations
	- [x] pointer-to-const members (`const char *name`) via `type_const_copy`
	- [x] const-scalar members via `is_const` on StructField; direct assignment rejected
- [x] Stage 82-02: const-qualified member lvalue rules
	- [x] subscript write through a pointer-to-const member rejected
- [x] Stage 82-03: `const` in type-name contexts (sizeof, cast, va_arg)
	- [x] `parse_type_name` consumes leading const and post-pointer qualifiers
- [x] Stage 82-04: minimal `volatile` handling
	- [x] TOKEN_VOLATILE; `type_qualifier` extended; accepted at all const positions
	- [x] `is_volatile` on Type/StructField; `type_volatile_copy()`; no codegen effect
- [x] Stage 82-05: const member / pointer-to-const-struct diagnostics
	- [x] `member_base_is_const()` rejects assignment to members of const objects

## Stage 83 - Project Source Converted to Strict ISO C99

- [x] Compiler source passes `gcc/clang -std=c99 -pedantic-errors`
- [x] `util_strdup()` replaces non-standard `strdup`
- [x] Portable CC_NORETURN / CC_PRINTF macros replace GNU `__attribute__`
- [x] CMake locked to `CMAKE_C_STANDARD 99`, `CMAKE_C_EXTENSIONS OFF`, `-Wall -Wextra -pedantic`
- [x] Behavior-preserving (no language or codegen change)

## Stage 84 - Standard Streams and extern Objects

- [x] `stdin` / `stdout` / `stderr` as `extern FILE *` in `stdio.h`
- [x] Codegen: `is_extern` flag on globals; extern objects registered but skipped in BSS/data
- [x] `codegen_emit_externs` emits NASM `extern` directives for extern objects
- [x] Stage 84-02: `exit()` declaration in `stdlib.h`

## Stage 85 - Member-Array to Pointer Decay

- [x] Array-typed struct/union members decay to pointer-to-element in value contexts
- [x] `codegen_expression` / `expr_result_type` report TYPE_POINTER for array members
- [x] char-array struct members initialized from string literals (per-byte stores)
- [x] Stage 85-01: `string.h` additions (`strncat`, `strncmp`, `strncpy`, `strrchr`, …)

## Stage 86 - Multidimensional Array Support

- [x] Declarators parse multiple `[N]` suffixes (MAX_ARRAY_DIMS = 8)
	- [x] first dimension may be inferred; inner dimensions required
- [x] `build_array_type_from_dims()` nests array types right-to-left
- [x] All declaration sites use the multi-dimensional builder (members, locals, globals, typedefs)
- [x] `parse_type_name` abstract array declarators (e.g. `sizeof(int[4][8])`)
- [x] codegen: `get_subscript_element_type()`; `emit_array_index_addr` scales by inner array byte size
- [x] Array-to-pointer decay at each level

## Stage 87 - File Position and Read Stubs

- [x] `stdio.h`: `fseek`, `ftell`, `fread` with `SEEK_SET` / `SEEK_CUR` / `SEEK_END`
- [x] Declarative stub additions only (no compiler change)

## Stage 88 - Hex and Octal Character Escapes

- [x] Lexer: hex `\xNN` and octal `\NNN` (1-3 digits) escapes in char and string literals
- [x] `\0` folded into the octal branch
- [x] Decoded to byte values, truncated to 8 bits
- [x] Grammar `<escape_sequence>` / `<character_escape_sequence>` updated

## Stage 89 - Adjacent String Literal Concatenation

- [x] `parse_primary` loops over consecutive TOKEN_STRING_LITERAL tokens
- [x] Decoded bytes appended into one node->value buffer
- [x] Total length checked against MAX_NAME_LEN before each concatenation
- [x] `byte_length`, null terminator, and `full_type` set from accumulated length

## Stage 90 - Hexadecimal Integer Literals

- [x] Lexer: `0x` / `0X` prefix detection in the integer-literal branch
- [x] Hex digits read via `isxdigit()`; parsed with `strtoul(..., 16)`
- [x] Missing hex digits after prefix rejected
- [x] Shares suffix and type-determination logic with the decimal path
- [x] Grammar `<integer_literal>` split into `<decimal_literal>` / `<hex_literal>`

## Stage 91 - Address-of Member Lvalues

- [x] Unary `&` accepts AST_MEMBER_ACCESS and AST_ARROW_ACCESS (plus AST_VAR_REF, AST_ARRAY_INDEX)
- [x] `struct_field_type()` helper converts StructField to Type*
- [x] AST_ADDR_OF dispatches to `emit_member_addr()` / `emit_arrow_addr()`
- [x] Result type set to pointer-to-field
- [x] Enables `&s.member`, `&p->member`, `&arr[i].member`
- [x] Full `src/` tree now self-compiles (last self-compilation parser gap closed)

## Stage 91-01 - Struct/Union By-Value Parameters and Returns

- [x] System V AMD64 ABI struct/union value parameters and return values
	- [x] Register-class (≤16 bytes) vs memory-class (>16 bytes) classification
	- [x] Shared call-layout helper used by both call sites and prologues
	- [x] `emit_struct_addr()` (field addresses) and `emit_struct_copy()` (block copy via `rep movsb`)
	- [x] `compute_struct_ret_bytes()` / `claim_struct_ret_temp()` for struct-return scratch
- [x] `AST_FUNCTION_CALL` rewritten with struct-aware argument marshalling (scalar push/pop preserved)
- [x] Prologue binds register-class structs by direct stores; memory-class structs copied to private local, hidden sret pointer saved
- [x] `AST_RETURN_STATEMENT` returns structs by value (register-class in rax/rdx, memory-class via hidden pointer)
- [x] Whole-struct assignment and declaration-initialization accept struct rvalues (variable, call result, or copy)
- [x] codegen context: `struct_ret_scratch_base`, `struct_ret_scratch_cursor`, `current_sret_offset`
- [x] Parser attaches `full_type` to struct/union value parameters and return types
- [ ] Inline struct literals not yet supported (values must originate from variables, returns, or copies)

## Stage 92 - Self-Compilation (Full Self-Hosting)

- [x] Compiler self-compiles: C0 (gcc-built) → C1 → C2, each passing all 1306 tests
- [x] `ASTNode.children` converted from fixed cap (64) to lazily-allocated doubling dynamic array (fixed silent truncation of large translation units, blocks, switches)
- [x] `codegen_emit_externs` suppresses `extern` for objects defined in the same TU; dedupes repeats
- [x] `is_static_linkage` field on `GlobalVar`; `global` NASM directive emitted for non-static file-scope variables
- [x] Six silent codegen bugs fixed during bootstrap:
	- [x] Struct-by-value assignment via subscript (`arr[i] = f()`), dot (`obj.m = f()`), arrow (`p->m = f()`), and deref (`*p = f()`)
	- [x] `sizeof` of arrow-access array/struct/union members
	- [x] `sizeof` of subscripted-struct members
- [x] `sizeof` of a string literal returns `strlen+1` (was 4)
- [x] `MAX_STRING_LITERALS` 256 → 2048; `MAX_SWITCH_LABELS` 64 → 256; new `MAX_CALL_LAYOUT_ITEMS`
- [x] Six block-scope `static const char *[]` register tables hoisted to file scope (block-scope static arrays still unsupported)
- [x] `main` signature uses `char **argv` form (the `T *name[]` element typing path is not yet correct)
- [x] Stub `stdlib.h`: `strtol`, `strtoul` added

## Stage 93 - Bootstrap Build Workflow

- [x] `build.sh` wrapper with three modes: `--mode=normal` (cmake), `--mode=bootstrap` (self-compile via pre-built ccompiler), `--mode=fallback` (bootstrap if available, else normal)
- [x] Bootstrap mode per-file `--timeout` guard (default 300s) against infinite loops
- [x] `CMakeLists.txt` computes `BUILTBY_TOKEN` from compiler ID/version
- [x] `VERSION_BUILTBY` macro (default `DefaultCcompiler`, stringified via C99 `#` operator); version output extended to two lines with `BuiltBy:` field; `version_buf` 128 → 256 bytes
- [x] `VERSION_STAGE` "00930000"
- [x] Test suites gain `TIMEOUT=${CLAUDEC99_TEST_TIMEOUT:-30}` and timeout-wrapped compiler/program invocations

## Stage 94 - Self-Hosting Validation and Bootstrap Cycle

- [x] `build.sh --mode=self-host`: full C0→C1→C2 cycle (archives prior binaries to `build/previous/`, cmake build → C0, bootstrap → C1, bootstrap → C2; runs the full suite and commits a checkpoint at each step)
- [x] Five build-infrastructure fixes: bootstrap `-I test/include` for stub headers; `-DVERSION_BUILD` computed and passed through; commit checkpoints between C0/C1 and C1/C2; `VERSION_BUILTBY` regex captures all four version groups
- [x] `VERSION_STAGE` "00940000"
- [x] Process/validation stage — no language, parser, or codegen changes; C1 and C2 are behavior-identical

## Stage 95-01 - Fixed-Capacity Inventory (docs only)

- [x] `docs/fixed-capacity-inventory.md` catalogs all 23 fixed-capacity tables/buffers (capacity, location, overflow behavior, pointer aliasing, realloc safety, priority)
- [x] Flagged latent bugs for later stages: `MAX_BREAK_DEPTH` has no bounds check; `PARSER_MAX_STRUCT_FIELDS` uses a hardcoded `64`; tightest commonly-hit limits are `PARSER_MAX_TYPEDEFS` / `PARSER_MAX_FUNCTIONS`
- [x] Documentation-only — no code changes

## Stage 95-02 - Vec Growable-Array Foundation

- [x] `Vec` generic growable-array module (`include/vec.h`, `src/vec.c`): `vec_init`/`free`/`reserve`/`push`/`get`/`pop`/`clear`; lazy doubling growth (initial cap 8), size_t overflow checks, fatal on allocation failure
- [x] New `test/unit/` suite with 106 assertions; unit runner added and aggregated into `run_all_tests.sh`

## Stage 95-03 - StrBuf String-Buffer Module

- [x] `StrBuf` dynamic character buffer (`include/strbuf.h`, `src/strbuf.c`): `strbuf_init`/`free`/`reserve`/`append_char`/`append_str`/`append_n`/`null_terminate` (null-terminate writes `'\0'` without bumping `len`)
- [x] 59-assertion unit suite added (165 unit assertions total)

## Stage 95-04 - Convert Low-Risk Static Arrays to Vec

- [x] `Parser.enum_tags`, `Parser.union_tags`, and `CodeGen.local_statics` converted from fixed arrays to `Vec`
- [x] Bootstrap fixes: `build.sh` SRC_FILES was missing `vec.c`/`strbuf.c`; `vec_reserve` overflow check emitted signed `idiv` for struct-member operands
- [x] (Macros `PARSER_MAX_ENUM_TAGS`/`PARSER_MAX_UNION_TAGS`/`MAX_LOCAL_STATICS` left defined-but-dead here; removed later in commit `9fda93c`)

## Stage 95-05 - Convert Medium-Risk Static Arrays to Vec

- [x] `Parser.globals`, `Parser.enum_consts`, `Parser.struct_tags`, `CodeGen.locals`, `CodeGen.globals`, and `CodeGen.string_pool` converted to `Vec`
- [x] Bootstrap fixes: C0 parser couldn't handle single-line `*(T**)vec_get(...)` cast-of-dereference (split into two statements); `vec_push` signed-division overflow check
- [x] (Six corresponding macros left defined-but-dead here; removed later in commit `9fda93c`)

## Stage 95-06 - Convert High-Risk Static Arrays to Vec

- [x] Struct/union body field parsing (`tmp_fields`), `CodeGen.break_stack`, `Parser.typedefs`, and `Parser.funcs` converted to `Vec`
- [x] Fixed two latent bugs: silent struct-field data loss (fields beyond 64 dropped; hardcoded `64` overflow check) and unchecked `break_stack` overflow
- [x] Removed `PARSER_MAX_STRUCT_FIELDS`, `MAX_BREAK_DEPTH`, `PARSER_MAX_TYPEDEFS`, `PARSER_MAX_FUNCTIONS` from `include/constants.h`

## Stage 95-07 - Convert Remaining Static Usages

- [x] `CodeGen.switch_stack` converted to `Vec` (no `switch`-nesting cap); `MAX_SWITCH_DEPTH` removed
- [x] `compute_call_layout` gains a `compile_error` guard for >24 arguments (`MAX_CALL_LAYOUT_ITEMS`), replacing a silent overflow risk

## Stage 95-08 - Pointer-Based Token Text Storage

- [x] `Token` redefined with `const char *lexeme`/`value` + `size_t` lengths; lexer gains a `Vec str_pool` and a `lexer_store_bytes()` helper (freed by `lexer_free`)
- [x] String literals decoded through a `StrBuf` — the 255-byte limit is gone and embedded NUL bytes are supported via `value_len`
- [x] 4 new valid tests (long strings, embedded NUL, escape sequences, adjacent literals)

## Stage 95-09 - Pointer-Based ASTNode Value

- [x] `ASTNode.value` and `ParsedDeclarator.name` changed from `char[MAX_NAME_LEN]` to `const char *` (pointers into lexer-owned storage)
- [x] Case-label and enum-constant values now stored via `lexer_store_bytes` instead of local stack buffers (fixes a use-after-stack bug that was failing all switch tests)
- [x] String literals longer than 255 bytes now handled correctly; 1 new valid test

## Stage 95-10 - Remove static char arrays from parser.h

- [x] Seven parser.h name/tag fields (`EnumConst.name`, `EnumTag.tag`, `StructTag.tag`, `UnionTag.tag`, `TypedefEntry.name`, `FuncSig.name`, `GlobalObjSig.name`) changed from `char[MAX_NAME_LEN]` to `const char *`; all `strncpy` replaced with direct pointer assignment
- [x] Three `char[256]` specifier-parsing temporaries simplified to `const char *` into the lexer pool

## Stage 95-11 - Remove static char arrays from codegen.h

- [x] Five codegen name/label fields (`LocalVar.name`/`static_label`, `LocalStaticVar.label`, `GlobalVar.name`/`init_label`) changed to `const char *`; generated labels (`Lstatic_*`, `Lstr*`) use `util_strdup`
- [x] `CodeGen.user_labels[MAX_USER_LABELS][MAX_NAME_LEN]` replaced with a `Vec` of pointers (64-label-per-function cap eliminated); `MAX_USER_LABELS` removed from `include/constants.h`
- [x] At stage close `MAX_NAME_LEN` applied only to `StructField.name`; that field was later converted to `const char *` and the now-dead constant removed in commit `9fda93c`

## Stage 95-12 - Fix #if unary overflow and dynamic switch labels

- [x] `eval_cond_unary` (`src/preprocessor.c`): replaced the unchecked fixed `char ops[32]` with a dynamic `StrBuf`; leading `#if`/`#elif` unary operators (`! - + ~`) are appended as consumed and applied right-to-left, freed before return — removes the last unbounded fixed-capacity write in the tree (a >32-operator chain previously SIGSEGV'd)
- [x] `SwitchCtx` (`include/codegen.h`): parallel fixed arrays `nodes[MAX_SWITCH_LABELS]`/`labels[MAX_SWITCH_LABELS]` + `count` replaced with an embedded `Vec entries` of a new `SwitchLabel{node,label}` pair struct; `count` becomes `entries.len`; the per-switch case/default cap (256) is removed and `MAX_SWITCH_LABELS` deleted from `include/constants.h`
  - [x] Lifecycle: `entries` is `vec_init`'d before the `SwitchCtx` is `vec_push`'d (move semantics) and `vec_free`'d before `vec_pop`; the live top element is re-fetched via `vec_get` after the body to avoid a dangling pointer when nested switches reallocate `switch_stack`
- [x] No grammar, parser, or AST changes; switch and `#if` syntax/semantics unchanged

## Stage 96 - Compile multiple source files per invocation

- [x] `src/compiler.c`: `source_file` pointer replaced with a growable `source_files` array (doubling realloc pattern matching `-D`/`-I` lists); zero source files remains a usage error
- [x] `compile_one_file()` static helper (`src/compiler.c`): extracts the per-file read→preprocess→lex→parse→codegen→write body; builds, uses, and fully tears down its own Lexer/Parser/CodeGen/AST and preprocessed buffer; returns 0 on success, 1 on failure
- [x] `main` loop: calls `reset_error_state()` before each file, calls `compile_one_file()`, accumulates `overall_status`; continues on failure so all files are attempted
- [x] `parser_free()` (`include/parser.h`, `src/parser.c`): `vec_free`s the seven Vecs the parser owns (`funcs`, `globals`, `typedefs`, `enum_consts`, `enum_tags`, `struct_tags`, `union_tags`); element strings stay in lexer-owned storage
- [x] `codegen_free()` (`include/codegen.h`, `src/codegen.c`): frees all eight Vecs (`owned_strings`, `locals`, `globals`, `break_stack`, `switch_stack`, `user_labels`, `string_pool`, `local_statics`)
  - [x] `Vec owned_strings` field added to `CodeGen`; `codegen_intern()` static helper routes all `util_strdup` calls through a single owned list, eliminating mixed-ownership on `GlobalVar.init_label` and `LocalStaticVar.label`
  - [x] `user_labels` `vec_free`d as a Vec only (elements are non-owned AST/lexer pointers)
- [x] `reset_error_state()` (`include/util.h`, `src/util.c`): zeroes `g_error_count`, `g_error_src_file/line/col`, `g_error_jmp_valid`; leaves `g_max_errors` and `g_warnings_are_errors` untouched
- [x] Integration runner (`test/integration/run_tests.sh`): added `run_test.sh` support for custom per-directory tests (directories without `<name>.c` but with `run_test.sh` are run and counted)
- [x] Tests: `test_multi_file_success` (single ccompiler invocation with two files that each define same-named static symbol; verifies independent per-file state and correct linked output) and `test_multi_file_error` (bad.c error reported, good.asm still produced, exit non-zero)
- [x] No grammar, AST node, or language-semantic changes; single-file invocation byte-for-byte unchanged

## Stage 97 - Designated Initializers

- [x] `AST_DESIGNATED_INIT` node type added to `ASTNodeType` enum (`include/ast.h`)
  - [x] `node->value != NULL` → member designator (field name string); `node->value == NULL` → array index designator (index in `node->byte_length`); `child_count` always 1
- [x] `parse_initializer_element()` static helper (`src/parser.c`): detects `.IDENT =` (member designator) and `[const_expr] =` (array index designator) at the head of each initializer-list element
  - [x] `.IDENT` designator: consumes `.`, reads identifier, checks for chained designator (emits "chained designators not yet supported" if another `.` or `[` follows), then consumes `=`
  - [x] `[expr]` designator: consumes `[`, calls `eval_case_const_expr` for constant index, rejects negative index, consumes `]`, checks for chaining, then consumes `=`
  - [x] `eval_const_expr` forward-declared before `parse_initializer` to allow use in `parse_initializer_element` (was `eval_case_const_expr` in Stage 97, renamed in Stage 99)
  - [x] Non-designator path falls through to `parse_initializer` unchanged
- [x] `parse_initializer` updated to call `parse_initializer_element` for each list element instead of calling `parse_initializer` directly
- [x] `ast_pretty_printer.c`: `AST_DESIGNATED_INIT` prints `DESIGNATED_INIT(.name)` (member) or `DESIGNATED_INIT([N])` (index)
- [x] `emit_local_struct_init` rewritten with current-field cursor (`src/codegen.c`)
  - [x] Member designator: field looked up by name; `cur_field` set to found index; error if not found
  - [x] Array index designator in struct context: `compile_error` "array index designator in struct initializer"
  - [x] Non-designator: uses sequential `cur_field`; `cur_field` incremented after each element
- [x] Local array init in `codegen_statement` rewritten with two-phase approach (`src/codegen.c`)
  - [x] Phase 1: zero-fill entire array unconditionally (byte-by-byte `mov byte [rbp - N], 0`)
  - [x] Phase 2: walk list with `cur` index cursor; `AST_DESIGNATED_INIT` (index) sets `cur`; member designator in array context: `compile_error`; plain element advances `cur` sequentially
  - [x] Out-of-bounds index: `compile_error` "array designator index %d out of bounds"
- [x] `emit_global_struct` rewritten to use slots array (`src/codegen.c`)
  - [x] `ASTNode *slots[MAX_STRUCT_FIELDS_DESIGNATED]` (fixed 64, no VLA); `slots[]` initialized to `NULL`
  - [x] Walk init list: member designator finds field by name and assigns to `slots[idx]`; plain element assigns to `slots[cur++]`
  - [x] Field-emission loop iterates fields in declared order; `slots[i] != NULL` emits the initializer, `NULL` emits zero bytes
  - [x] `compile_error` if `st->field_count > MAX_STRUCT_FIELDS_DESIGNATED`
- [x] Global array init in `codegen_emit_data` extended with slots approach (`src/codegen.c`)
  - [x] `ASTNode *slots[MAX_ARRAY_ELEMS_DESIGNATED]` (fixed 1024, no VLA); `slots[]` initialized to `NULL`
  - [x] Walk init list: `AST_DESIGNATED_INIT` (index) sets `slots[idx]`; member designator: `compile_error`; plain element assigns `slots[cur++]`
  - [x] Emit loop iterates `0..length-1`; `slots[i] != NULL` uses existing element dispatch; `NULL` emits zero
  - [x] `compile_error` if `arr_len > MAX_ARRAY_ELEMS_DESIGNATED`
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"00970000"`
- [x] Tests: 10 valid (local struct basic/partial/mixed, local array basic/advance/reorder, global struct, global sparse array, array of structs, char-literal index), 5 invalid (unknown member, array index in struct, member in array, index out of bounds, chained designators), 2 print_ast (member designator, index designator), 1 integration (multi-file with designated global struct + global array)
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; 1501 tests pass at all three stages

## Stage 98 - Compound Literals

- [x] `AST_COMPOUND_LITERAL` node type added to `ASTNodeType` enum (`include/ast.h`), after `AST_DESIGNATED_INIT`
  - [x] `node->decl_type` = base type kind; `node->full_type` = struct/union/array/pointer `Type*`, NULL for plain scalars
  - [x] `node->byte_length` = 0 at parse time; pre-scan overwrites with rbp-relative stack offset
  - [x] `node->value` = pre-formatted type label ("int", "struct Point", "int[4]") stored in lexer pool via `lexer_store_bytes`
  - [x] `node->children[0]` = `AST_INITIALIZER_LIST` for aggregate types; plain `parse_assignment_expression` result for scalar
- [x] `ast_pretty_printer.c`: `AST_COMPOUND_LITERAL` prints `COMPOUND_LITERAL(type-label)` followed by the initializer child
- [x] `parse_postfix_suffixes` extracted from `parse_postfix` as a standalone static helper; `parse_postfix` now calls it
- [x] `build_compound_literal` static helper (`src/parser.c`): consumes `{` initializer `}` and builds `AST_COMPOUND_LITERAL` node
  - [x] Rejects void and function types with `"error: invalid type for compound literal"`
  - [x] Struct/union: delegates to `parse_initializer`; sets `decl_type = TYPE_STRUCT`/`TYPE_UNION`, `full_type = struct Type*`
  - [x] Array with explicit length N: delegates to `parse_initializer`; `full_type = type_array(elem, N)`
  - [x] Array with omitted first dimension (`[]`): delegates to `parse_initializer`, then calls `infer_compound_literal_array_length` and rebuilds `full_type = type_array(elem, inferred_length)`
  - [x] Scalar: manually consumes `{`, calls `parse_assignment_expression`, allows trailing comma, consumes `}`
- [x] `infer_compound_literal_array_length` static helper: walks child list, tracks cursor, returns `max(highest [N] designator + 1, plain_count)`
- [x] `parse_cast` updated: after `(type-name)`, if next token is `{` → delegate to `build_compound_literal` + `parse_postfix_suffixes`; else → regular cast
  - [x] `TOKEN_STRUCT` and `TOKEN_UNION` added to the type-start check in `parse_cast`
  - [x] After casting path: `sizeof(int[])` with `length == 0` raises `"error: sizeof applied to incomplete array type"`
  - [x] Compound literal `(type-name){` with `length == 0` array is inferred; cast `(type-name)expr` with `length == 0` raises `"error: array type in cast has omitted size"`
- [x] `parse_postfix` updated: tries compound literal detection (`(type-name){`) before falling back to `parse_primary`; restores lexer state if `{` not found
- [x] `parse_unary` updated: `&` operand check allows `AST_COMPOUND_LITERAL` as a valid lvalue
- [x] `parse_type_name` updated: `[]` (omitted first dimension) accepted for `dim_count == 0` in array-suffix loop (new for stage 98)
- [x] Global initializer: `parse_primary` → `parse_assignment_expression` so compound literals are parsed before the constant-only check; `AST_COMPOUND_LITERAL` triggers `"error: compound literals at file scope are not yet supported"` (Stage 98)
- [x] `compute_compound_literal_bytes` static helper (`src/codegen.c`): conservative upper-bound walk for stack-frame sizing
- [x] `scan_expr_compound_literals` static helper: recursive walk to assign `node->byte_length` offsets to every `AST_COMPOUND_LITERAL` in an expression tree
- [x] Frame-size pre-scan calls `scan_expr_compound_literals(cg, body)` after computing param/decl bytes; `compound_lit_bytes` added to `stack_size`
- [x] `codegen_expression` case `AST_COMPOUND_LITERAL`: file-scope guard, then struct/union path (zero-fill + `emit_local_struct_init` + `lea rax`), array path (zero-fill + cursor-based element init + `lea rax`), scalar path (eval init + store)
  - [x] Union: non-first-member designator → `"error: union compound literal with non-first member designator not yet supported"`
  - [x] Array: out-of-bounds designator → `"error: array designator index %d out of bounds"`; too many initializers → `"error: too many initializers in compound literal"`
- [x] `emit_struct_addr` case `AST_COMPOUND_LITERAL`: calls `codegen_expression`, returns `node->full_type->base` (the struct type)
- [x] `emit_member_addr` case `AST_COMPOUND_LITERAL`: calls `codegen_expression`, adds field offset, returns field; enables `(T){...}.field`
- [x] `emit_array_index_addr` case `AST_COMPOUND_LITERAL`: calls `codegen_expression`, computes scaled index; enables `(T[]){...}[i]`
- [x] `AST_ADDR_OF` codegen: compound literal operand path calls `codegen_expression`, emits `lea rax, [rbp - offset]` for scalar, uses address already in rax for struct/array
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"00980000"`
- [x] Tests: 12 valid (struct arg, struct assign, array explicit, array omitted size, array designator, postfix member, postfix subscript, addr-of scalar, scalar, zero-fill, dot-product, char array), 4 invalid (file scope, void type, too many initializers, array index oob), 3 print_ast (struct, array, scalar), 1 integration (`test_compound_literal_multifile`)
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; 1521 tests pass at all three stages

## Stage 99 - typedef enum Completion

- [x] Extended integer constant expression evaluator (replacing three `eval_case_const_*` helpers)
	- [x] `eval_const_primary`: INT_LITERAL, CHAR_LITERAL, enum-const IDENTIFIER, parenthesized expression
	- [x] `eval_const_unary`: unary `+`, `-`, `~`, `!`
	- [x] `eval_const_multiplicative`: `*`, `/`, `%` (division-by-zero check via `PARSER_ERROR`)
	- [x] `eval_const_shift`: `<<` (`TOKEN_LEFT_SHIFT`), `>>` (`TOKEN_RIGHT_SHIFT`)
	- [x] `eval_const_additive`: `+`, `-` (now calls `eval_const_multiplicative`)
	- [x] `eval_const_bitwise_and`: `&` (`TOKEN_AMPERSAND`)
	- [x] `eval_const_bitwise_xor`: `^` (`TOKEN_CARET`)
	- [x] `eval_const_bitwise_or`: `|` (`TOKEN_PIPE`)
	- [x] `eval_const_expr`: public entry point calling `eval_const_bitwise_or`
	- [x] `const char *context` parameter throughout for context-specific error messages
- [x] `parse_enum_specifier`: enumerator value now calls `eval_const_expr(parser, "enumerator value")`
	- [x] Replaces literal-only check (integer literal / char literal with trailing-comma guard)
	- [x] Enables `FLAG = 1 << 0`, `STEP = BASE + 5`, `TOP = STEP * 2`, `ALL = ~0`, etc.
- [x] `parse_enum_specifier`: forward-declared enum tags accepted (no body, tag not in table)
	- [x] Creates `EnumTag { tag, is_defined = 0 }` entry via `vec_push`; returns `type_int()`
	- [x] Enables `typedef enum Status Status;` before `enum Status { OK, ERR };`
- [x] `parse_initializer_element`: updated to call `eval_const_expr(parser, "array designator index")`
- [x] Case-label handler: updated to call `eval_const_expr(parser, "case label expression")`
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"00990000"`
- [x] Tests: 9 valid (enum shift, prior-const, complement, paren, case-label-shift, 3 forward-ref, regression), 2 invalid (non-const expression, division-by-zero), 1 print_ast (enum const fold to INT_LITERAL); 2 outdated invalid tests removed; all 1531 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; 1531 tests pass at all three stages

## Stage 100 - File-Scope Constant Expressions

- [x] `eval_const_primary`: `TOKEN_SIZEOF` handling added for `sizeof(type-name)` in constant-expression contexts
	- [x] Consumes `sizeof`, requires `(`, checks type-start condition (added `TOKEN_VOID` and `TOKEN_ENUM`), calls `parse_type_name`, returns `(long)type_size(t)`
	- [x] `TOKEN_ENUM` added (pre-existing gap in `parse_primary`'s sizeof arm; `sizeof(enum Color)` = `sizeof(int)`)
	- [x] `TOKEN_VOID` added; bare `void` rejected by `t->kind == TYPE_VOID` check after `parse_type_name`
- [x] `parse_primary` sizeof arm: fixed pre-existing `sizeof(void *)` bug
	- [x] Removed early `TOKEN_VOID` rejection; added `TOKEN_VOID` to type-start condition
	- [x] Added `t->kind == TYPE_VOID` check after `parse_type_name` to still reject bare `sizeof(void)`
- [x] `parse_external_declaration` first-declarator path: replaced literal-only check with `eval_const_expr`
	- [x] `if (decl->decl_type != TYPE_POINTER && != TYPE_STRUCT && != TYPE_UNION)` → calls `eval_const_expr(parser, "file-scope initializer")` and stores result as `AST_INT_LITERAL` via `lexer_store_bytes`
	- [x] Pointer/struct/union globals retain original `parse_assignment_expression` + literal-check path
- [x] `parse_external_declaration` multi-declarator path: replaced `parse_primary` with `eval_const_expr`
	- [x] `if (k2 != TYPE_POINTER)` → calls `eval_const_expr`; pointer path keeps `parse_primary` with literal check
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01000000"`
- [x] Tests: 10 valid (arith, bitwise-or, shift, sizeof-void*, sizeof-int*256, sizeof-struct, enum-op, neg, complement, multi-decl), 2 invalid (variable reference, sizeof-no-parens), 1 print_ast (fold to IntLiteral); 2 pre-existing invalid tests renamed to match new error message; all 1544 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; 1544 tests pass at all three stages

## Stage 101 - Block-Scope Static Arrays and Structs

- [x] `include/codegen.h`: added `ASTNode *init_node` to `LocalStaticVar` struct to carry brace-list and string-literal initializers for aggregate types
- [x] `src/codegen.c` `codegen_statement` SC_STATIC arm: removed guard that rejected `TYPE_ARRAY`/`TYPE_STRUCT`/`TYPE_UNION`
	- [x] Added array static registration block: validates initializer (must be `AST_INITIALIZER_LIST` or string literal for char arrays), generates `Lstatic_func_N` label, registers in `cg->locals` with `is_static=1`, pushes `LocalStaticVar` with `init_node` set
	- [x] Added struct/union static registration block: validates non-zero size (incomplete type check), validates brace-list initializer, same label/registration pattern
	- [x] Scalar block unchanged; now falls through naturally after array/struct branches return early
- [x] `codegen_emit_local_statics()`: extended `.data` loop for aggregate statics
	- [x] `TYPE_ARRAY` + `AST_STRING_LITERAL` → inline `db` bytes (char array from string literal)
	- [x] `TYPE_ARRAY` + `AST_INITIALIZER_LIST` → slots-map pattern (same as global array emit); error if slots `> MAX_ARRAY_ELEMS_DESIGNATED`; error on designated init (not yet supported in this context); each slot emits directive+value or zero-fill
	- [x] `TYPE_STRUCT` + `AST_INITIALIZER_LIST` → calls `emit_global_struct(cg, sv->full_type, sv->init_node)`
	- [x] `TYPE_UNION` + `AST_INITIALIZER_LIST` → inline first-member logic (same as global union emit)
	- [x] Scalar fallthrough: existing `data_init_directive` / `init_value` path
- [x] `codegen_emit_local_statics()`: extended `.bss` loop
	- [x] `TYPE_ARRAY` → `label: resx length` (using `bss_res_directive` of the element kind)
	- [x] `TYPE_STRUCT` / `TYPE_UNION` → `label: resb total_size`
	- [x] Scalar fallthrough unchanged
- [x] `codegen_expression` VAR_REF `TYPE_ARRAY` branch: added `is_static` guard → `lea rax, [rel label]` instead of `lea rax, [rbp - offset]`
- [x] `emit_array_index_addr()`: added `is_static` guard → `lea rax, [rel label]`; removed stale comment that claimed array statics were always rejected
- [x] `emit_member_addr()` local-struct branch: added `is_static` guard → `lea rax, [rel label + offset]` (or `[rel label]` when offset is 0)
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01010000"`
- [x] Tests: 6 valid (uninitialized array, initialized array, uninitialized struct, initialized struct, char-array from string literal, single-element array counter), 2 invalid (non-brace initializer for static array, non-brace initializer for static struct); all 1552 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; no compiler source changes needed; 1552 tests pass at all three stages

## Stage 102 - Complete Static Aggregate Coverage

- [x] `src/codegen.c` `codegen_emit_local_statics()` `.data` loop — Task 1: Replaced Guard A with index-designator handling
	- [x] `AST_DESIGNATED_INIT` with `value == NULL` (index designator): extract `byte_length` as index, set cursor, assign slot — identical to global array path
	- [x] `AST_DESIGNATED_INIT` with `value != NULL` (member designator in array context): `compile_error` "member designator in array initializer"
	- [x] Non-designator children: existing bounds check + `slots[cur++] = child` path
- [x] `src/codegen.c` `codegen_emit_local_statics()` `.data` slot-emit loop — Task 2+3c: Extended non-NULL and NULL branches
	- [x] `TYPE_STRUCT` element + `AST_INITIALIZER_LIST`: calls `emit_global_struct(cg, elem_type, elem)`
	- [x] `TYPE_UNION` element + `AST_INITIALIZER_LIST`: inline first-member init (int/char literal) + byte zero-fill to union size
	- [x] `TYPE_ARRAY` element + `AST_INITIALIZER_LIST` (2D inner row): emit each scalar with `data_init_directive(scalar_type->kind)`, zero-fill missing columns; error if row element type is itself `TYPE_ARRAY` ("deeper than 2D")
	- [x] NULL slot zero-fill: byte-fills (`db 0 × size`) for struct/union/array element types; directive+0 for scalars
- [x] `src/codegen.c` `codegen_emit_local_statics()` `.bss` loop — Task 3a: multidimensional local static arrays
	- [x] When `sv->full_type->base->kind == TYPE_ARRAY`: emit `label: resb total_bytes` (uses `sv->full_type->size`)
	- [x] Single-dimension unchanged: `label: resx length`
- [x] `src/codegen.c` `codegen_emit_bss()` — Task 3b: multidimensional global arrays
	- [x] Same two-case fix for `gv->full_type->base->kind == TYPE_ARRAY`: emit `name: resb total_bytes`
	- [x] Single-dimension unchanged
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01020000"`
- [x] Tests: 7 valid (designated-init array, designated-init persist, struct array uninit, struct array init, 2D array uninit, 2D array init, 2D array persist), 1 invalid (3D static array → "deeper than 2D"); all 1560 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; compiler source uses no block-scope static aggregates of these new types; 1560 tests pass at all three stages

## Stage 100 - File-Scope Constant Expressions

- [x] Parser: extended `eval_const_expr` with `sizeof(type-name)` support in `eval_const_primary`
	- [x] Integer-typed file-scope globals now accept full arithmetic/bitwise/shift/unary constant expressions instead of literal-only
	- [x] `TOKEN_SIZEOF` handling added; consumes `sizeof`, requires `(`, checks type-start condition, calls `parse_type_name`, returns `(long)type_size(t)`
	- [x] `TOKEN_ENUM` and `TOKEN_VOID` added to type-start detection in `eval_const_primary` and `parse_primary` sizeof arm
- [x] No AST/codegen changes
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01000000"`
- [x] Tests: 10 valid (arith, bitwise-or, shift, sizeof-void*, sizeof-int*256, sizeof-struct, enum-op, neg, complement, multi-decl), 2 invalid (variable reference, sizeof-no-parens), 1 print-ast; all 1544 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues

## Stage 101 - Block-Scope Static Aggregates

- [x] AST: added `init_node` field to `LocalStaticVar` to carry brace-list and string-literal initializers for aggregate types
- [x] Codegen: SC_STATIC arm in `codegen_statement` handles TYPE_ARRAY/TYPE_STRUCT/TYPE_UNION
	- [x] Array static registration: validates initializer (AST_INITIALIZER_LIST or string literal for char arrays), generates label, registers with init_node set
	- [x] Struct/union static registration: validates non-zero size, validates brace-list initializer, same label/registration pattern
- [x] `codegen_emit_local_statics` extended for aggregate types with RIP-relative addressing
	- [x] Aggregate statics in `.data` and `.bss` sections
	- [x] Array-to-pointer decay and member addressing work correctly
- [x] `is_static` branches in array decay, subscript addressing, struct member addressing
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01010000"`
- [x] Tests: 6 valid (uninitialized array, initialized array, uninitialized struct, initialized struct, char-array from string literal, single-element array counter), 2 invalid; all 1552 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues

## Stage 102 (continued) - Complete Static Aggregate Coverage

Additional improvements for designated-init and multidimensional static arrays (see above).

## Stage 103 - Block-Scope Static Scalar Constant-Expression Initializers

- [x] Codegen only: add `eval_const_init(ASTNode *node, const char *varname)` static helper in `src/codegen.c` before `codegen_statement`
	- [x] Replaces 3-case ad-hoc check (AST_INT_LITERAL, AST_CHAR_LITERAL, negated-literal) with a single `eval_const_init` call
	- [x] Handles: AST_INT_LITERAL (strtol base 0 for hex), AST_CHAR_LITERAL, AST_SIZEOF_TYPE (type_size if full_type, else type_kind_bytes for scalars)
	- [x] Handles unary ops: `-` (negation), `~` (bitwise complement), `!` (logical not), `+` (unary plus)
	- [x] Handles binary ops: `+`, `-`, `*`, `/`, `%` (with div-by-zero guard), `<<`, `>>`, `&`, `^`, `|`
	- [x] Full constant-expression support for block-scope static scalar initializers
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01030000"`
- [x] Tests: 7 valid (various constant expressions), 2 invalid (non-constant, div-by-zero); all 1569 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; all 1569/1569 tests pass at each step

## Stage 104 - Complete Constant-Expression Evaluator

- [x] `src/parser.c` — token-stream evaluator (`eval_const_expr`) extended:
	- [x] Fix additive/shift precedence bug: swap call order so `eval_const_additive` calls `eval_const_multiplicative` and `eval_const_shift` calls `eval_const_additive` (was inverted — `3+1<<2` now correctly evaluates to 16)
	- [x] Add `eval_const_relational`: handles `<`, `<=`, `>`, `>=`
	- [x] Add `eval_const_equality`: handles `==`, `!=`; update `eval_const_bitwise_and` to call `eval_const_equality`
	- [x] Add `eval_const_logical_and`: handles `&&`
	- [x] Add `eval_const_logical_or`: handles `||`
	- [x] Add `eval_const_conditional`: handles `?:` (right-associative; true branch via `eval_const_expr`, false branch recurses `eval_const_conditional`)
	- [x] Update `eval_const_expr` to call `eval_const_conditional`; update grammar comment block
- [x] `src/codegen.c` — AST evaluator (`eval_const_init`) extended:
	- [x] Add `<`, `<=`, `>`, `>=`, `==`, `!=`, `&&`, `||` to `AST_BINARY_OP` block (eager evaluation — correct for constants)
	- [x] Add `AST_CONDITIONAL_EXPR` case with lazy evaluation (only evaluates selected branch)
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01040000"`
- [x] Tests: 13 valid (enum/file-scope/block-static × relational/equality/logical/ternary + precedence fix + switch case), 2 invalid; all 1584 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; all 1584/1584 tests pass at each step

## Stage 105 - C99 Preprocessor Completion

- [x] `src/preprocessor.c` — extend `MacroTable` for `#pragma once` tracking:
	- [x] Add `once_paths` / `once_count` / `once_cap` fields to `MacroTable` struct
	- [x] `macro_table_init`: zero-initialize new fields; `macro_table_free`: free `once_paths` array
	- [x] Add `macro_table_add_once(t, path)` and `macro_table_is_once(t, path)` helpers
- [x] `src/preprocessor.c` — `preprocess_file`: early-return empty string when path is already in `once_paths`
- [x] `src/preprocessor.c` — `#pragma` directive handler:
	- [x] Match `pragma` keyword (with non-identifier-char guard); skip unknown pragmas silently
	- [x] Recognize `#pragma once`: call `macro_table_add_once(macros, source_path)`
- [x] `src/preprocessor.c` — `#line` directive handler:
	- [x] Add `current_file_override` local (`char *`, NULL by default; freed on return)
	- [x] Parse required digit sequence; set `current_line = new_line - 1` (newline adds 1)
	- [x] Parse optional quoted filename into a `GrowBuf`; assign to `current_file_override`
	- [x] Update `__FILE__` expansion to use `current_file_override` when non-NULL
- [x] `src/preprocessor.c` — null directive: bare `#` followed by whitespace+newline → `continue` (C99 §6.10.7)
- [x] `src/preprocessor.c` — `_Pragma(string)` operator in identifier expansion:
	- [x] Parse `_Pragma("str")`: destringize content; recognize `"once"` → `macro_table_add_once`; ignore everything else
	- [x] `_Pragma(...)` replaced by nothing (zero preprocessing tokens)
- [x] `include/parser.h` — add `current_func_name` field to `Parser` struct (after `current_func_is_variadic`)
- [x] `src/parser.c` — `parser_init`: initialize `current_func_name = NULL`
- [x] `src/parser.c` — function definition: save/restore `current_func_name = d.name` around `parse_block`
- [x] `src/parser.c` — `parse_primary`: handle `__func__` before `__builtin_va_*` check:
	- [x] Error if `current_func_name == NULL` ("used outside of a function body")
	- [x] Synthesize `AST_STRING_LITERAL` with `lexer_store_bytes(fname, len)`, `byte_length=len`, `full_type=type_array(type_char(), len+1)`
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01050000"`
- [x] Tests: 7 valid (pragma ignored, line directive ×2, null directive, _Pragma ignored, __func__ ×2), 1 invalid (__func__ at file scope), 2 integration (#pragma once, _Pragma once); all 1594 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; all 1594/1594 tests pass at each step

## Stage 106 - C99 Header Completion

- [x] `include/token.h` — add `TOKEN_RESTRICT` keyword token (after `TOKEN_VOLATILE`)
- [x] `src/lexer.c` — recognize `"restrict"` identifier → `TOKEN_RESTRICT`; add token display name `"'restrict'"`
- [x] `src/parser.c` — extend all pointer-qualifier positions to consume `TOKEN_RESTRICT` (parse-and-ignore):
	- [x] `parse_declarator` leading-star loop: changed `if` → `while`; added `TOKEN_RESTRICT`
	- [x] `parse_type_name` abstract-declarator star loop: added `TOKEN_RESTRICT`
	- [x] `parse_declarator` fp-param-types loop: added `TOKEN_RESTRICT`
	- [x] `parse_parameter_declaration` leading-qualifier check: added `TOKEN_RESTRICT`
	- [x] `parse_parameter_declaration` pre-consume stars loop: add inner qualifier loop after each `*`
- [x] `test/include/ctype.h` — add `iscntrl`, `isgraph`, `isprint`, `ispunct`
- [x] `test/include/string.h` — add `memmove`, `memchr`, `strcat`, `strcoll`, `strcspn`, `strspn`, `strpbrk`, `strstr`, `strtok`, `strerror`, `strxfrm` (with `restrict` qualifiers per C99)
- [x] `test/include/stdlib.h` — complete rewrite: `div_t`/`ldiv_t`/`lldiv_t` typedefs; `EXIT_SUCCESS`/`EXIT_FAILURE`/`RAND_MAX`/`MB_CUR_MAX` macros; 21 functions (`abort`, `atexit`, `_Exit`, `system`, `getenv`, `rand`, `srand`, `abs`, `labs`, `llabs`, `div`, `ldiv`, `lldiv`, `atoi`, `atol`, `atoll`, `strtoll`, `strtoull`, `bsearch`, `qsort`)
- [x] `test/include/stdio.h` — complete rewrite: `fpos_t` typedef; 7 macros (`BUFSIZ`, `FOPEN_MAX`, `FILENAME_MAX`, `L_tmpnam`, `TMP_MAX`, `_IOFBF`/`_IOLBF`/`_IONBF`); 31 functions including `fwrite`, formatted I/O, character I/O, file positioning, error handling
- [x] `test/include/stdbool.h` — fix `__Bool_true_false_are_defined` → `__bool_true_false_are_defined`
- [x] `test/include/stddef.h` — add `typedef long ptrdiff_t`
- [x] `test/include/limits.h` — add `CHAR_MIN`/`CHAR_MAX`/`MB_LEN_MAX`
- [x] `src/codegen.c` — fix pre-existing bug: add `TYPE_LONG_LONG` and `TYPE_UNSIGNED_LONG_LONG` to 8 `rhs_is_long` checks (declaration initializer, local/global assignment, union/array compound literals, struct field, local array init); prevented 64-bit `long long` return values from being truncated via `movsxd`
- [x] `src/version.c`: `VERSION_STAGE` bumped to `"01060000"`
- [x] Tests: 13 new valid tests (EXIT codes, `labs`/`llabs`, `atoi`, `qsort`, `strtoll`/`strtoull`, `memmove`, `strstr`, `strcat`, `strtok`, `sprintf`, ctype classifiers, restrict parsing); all 1607 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; all 1607/1607 tests pass at each step

## Stage 107 — `inline` keyword, `<assert.h>`, `va_copy` codegen

- [x] `include/token.h` — add `TOKEN_INLINE` keyword token (after `TOKEN_RESTRICT`)
- [x] `src/lexer.c` — recognize `"inline"` identifier → `TOKEN_INLINE`; add token display name `"'inline'"`
- [x] `src/parser.c` — consume `TOKEN_INLINE` in `parse_declaration_specifiers` (parse-and-ignore, same pattern as `volatile` and `restrict`)
- [x] `src/codegen.c` — split `va_end`/`va_copy` combined no-op; implement `va_copy` as three 8-byte `rax` moves copying the 24-byte SysV AMD64 `va_list` struct
- [x] `src/preprocessor.c` — fix `__FILE__`/`__LINE__` expansion in `expand_macros_text`: add static globals `g_expand_source_path`/`g_expand_current_line` set by `preprocess_internal` before each `expand_macros_text` call; `expand_macros_text` now expands both predefined macros
- [x] `test/include/assert.h` — new NDEBUG-aware `assert` macro (stringification, `__FILE__`, `__LINE__`, `fprintf`, `abort`)
- [x] `src/version.c` — `VERSION_STAGE` bumped to `"01070000"`
- [x] `Implicit return in void functions` — already implemented at `src/codegen.c` (fall-off-end emits `ret`); checklist item closed
- [x] Tests: 8 new valid tests (inline func, static inline, extern inline, assert pass, assert NDEBUG, assert fail/134, va_copy basic, void implicit return); all 1615 tests pass
- [x] Self-host C0→C1→C2 passes with no bootstrap issues; all 1615/1615 tests pass at each step

---

## TODO

### Preprocessor
- [x] `#pragma` (unknown pragmas silently ignored; `#pragma once` supported) (Stage 105)
- [ ] #elifdef / #elifndef
- [ ] GNU extension: `__VA_OPT__` and named variadic args

### Types
- [ ] float and double types
- [ ] Floating-point arithmetic and comparisons
- [ ] Floating-point literals (decimal and hex forms)
- [ ] Floating-point conversions (int ↔ float ↔ double)
- [ ] ptrdiff_t, size_t, intptr_t awareness
- [x] Union types (Stage 72; anonymous unions Stage 73-01)
- [x] Multidimensional arrays (Stage 86; up to 8 dimensions)
- [ ] Bit-field members in structs
- [ ] Flexible array members in structs
- [x] Compound literals: `(Type){ ... }` (Stage 98; file-scope and designated union non-first-member not yet supported)
- [x] volatile qualifier (Stage 82-04; parsed and tracked, no codegen effect yet)
- [x] restrict qualifier on pointers (Stage 106; parse-and-ignore, no codegen effect)
- [x] Pointer-level const enforcement: writes through const pointers, const-discard conversions (Stage 66)
- [x] const in struct/union members and type-name contexts (Stage 82-01/02/03/05)
- [ ] Type compatibility and composite type rules

### Declarations and Scope
- [x] static storage class (block scope — local static variables: scalar/pointer Stage 71; arrays/structs/unions Stage 101; designated-init arrays, struct/union element types, 2D arrays Stage 102; full constant-expression initializers Stage 103)
- [ ] register storage class (hint only)
- [ ] auto storage class (explicit)
- [ ] Tentative definitions for file-scope variables
- [x] For-loop initializer declarations: for (int i = 0; ...) (Stage 76)
- [ ] Multiple pointer levels in multi-declarator lists
- [ ] Function declarations at block scope
- [ ] Incomplete array types in extern declarations

### Initializers
- [x] Designated initializers for arrays: [idx] = value (Stage 97)
- [x] Designated initializers for structs: .member = value (Stage 97)
- [ ] Zero-initialization of static-duration objects (global structs/arrays)

### Expressions
- [ ] sizeof with unsigned result type (size_t)
- [ ] Pointer comparison operators (< <= > >= on pointers)
- [ ] Pointer equality with non-null constants
- [x] Integer constant expressions in case labels (Stage 77; Stage 99: extended to full bitwise/shift/multiplicative operators)
- [x] Integer constant expressions in file-scope initializers (Stage 100)
- [x] Integer constant expressions in block-scope static scalar initializers (Stage 103)
- [x] Hexadecimal integer literals: 0x/0X prefix (Stage 90)
- [x] Adjacent string literal concatenation (Stage 89)
- [x] Hex (\xNN) and octal (\NNN) character/string escapes (Stage 88)
- [x] Address-of on member/subscript lvalues: &s.m, &p->m, &a[i].m (Stage 91)
- [x] Compound assignment and ++/-- on general lvalues (Stages 79, 80)
- [x] General integer constant expressions (arithmetic, bitwise, shift, unary, sizeof(type), relational, equality, logical, ternary) — Stages 77, 99–104
- [ ] Floating-point constant expressions
- [ ] Lvalue conversion rules for all expression contexts
- [ ] Unary + on floating-point
- [ ] Mixed integer/floating-point arithmetic (usual arithmetic conversions)
- [ ] Integer and floating-point promotions in function arguments (default argument promotions)

### Statements
- [x] For-loop initializer declarations (Stage 76)
- [ ] switch with long / char / short discriminant (after promotion)
- [ ] Case labels with unsigned and long constant values
- [x] Case labels with full integer constant expressions (Stage 77; Stage 99: extended to shift, bitwise, multiplicative, parenthesized expressions; Stage 104: extended to relational, equality, logical, and ternary)
- [ ] goto across declarations (only legal in C under restrictions)

### Functions
- [x] Variadic function definitions: va_list, va_start, va_end, and va_arg for GP-class types (int/long/long long/pointer) (Stage 75)
  - [ ] va_arg for floating-point and struct-by-value types
  - [x] va_copy codegen — three 8-byte moves copying the 24-byte va_list struct (Stage 107)
- [ ] Old-style (K&R) function definitions
- [x] Implicit return in void functions — fall-off-end emits `ret` (Stage 107 checklist close)
- [ ] Functions returning function pointers: int (*f())(int)
- [x] Inline functions (inline keyword) — parse-and-ignore, no codegen effect (Stage 107)

### Standard Library Headers (stub or pass-through)
- [x] <stdio.h>: `puts`, `printf` (stub; full set including `fprintf`, `fopen`, `fclose`, `fread`, `fgets`, `snprintf` not yet in stub)
- [x] <stdlib.h>: `malloc`, `free`, `exit` (stub; `calloc`, `realloc`, `qsort` etc. not yet in stub)
- [x] <string.h>: `strlen`, `strcmp`, `strcpy`, `memcpy`, `memset` (stub)
- [x] <stddef.h>: `NULL`, `size_t` (stub)
- [x] <stdint.h>: full exact-width, least-width, fast, and pointer-size integer typedefs (stub complete)
- [x] <limits.h>: full set including `LLONG_MIN`, `LLONG_MAX`, `ULLONG_MAX` (stub complete)
- [x] <stdbool.h>: `bool`, `true`, `false` (stub complete)
- [x] <stdio.h>: expanded with opaque `FILE`, `EOF`, `fopen`, `fclose`, `fgetc`, `fgets`, `fprintf`, `snprintf`, `vfprintf`, `vprintf`, `vsnprintf` (Stage 67, 75-04); `putchar` (Stage 81); `stdin`/`stdout`/`stderr` streams (Stage 84); `fseek`/`ftell`/`fread` + `SEEK_*` (Stage 87); complete rewrite Stage 106: `fpos_t`, 7 macros, 31 functions (`fwrite`, `remove`, `rename`, `tmpfile`, `tmpnam`, `freopen`, `fflush`, `setbuf`, `setvbuf`, `sprintf`, `vsprintf`, `scanf`, `fscanf`, `sscanf`, `vscanf`, `vfscanf`, `vsscanf`, `fputc`, `fputs`, `putc`, `getc`, `getchar`, `gets`, `ungetc`, `fwrite`, `fgetpos`, `fsetpos`, `rewind`, `clearerr`, `feof`, `ferror`, `perror`)
- [x] <stdlib.h>: `calloc` (Stage 81); `exit` (Stage 84-02); complete rewrite Stage 106: `div_t`/`ldiv_t`/`lldiv_t`, `EXIT_SUCCESS`/`EXIT_FAILURE`/`RAND_MAX`/`MB_CUR_MAX`, 21 functions (`abort`, `atexit`, `_Exit`, `system`, `getenv`, `rand`, `srand`, `abs`, `labs`, `llabs`, `div`, `ldiv`, `lldiv`, `atoi`, `atol`, `atoll`, `strtoll`, `strtoull`, `bsearch`, `qsort`)
- [x] <string.h>: `strncat`, `strncmp`, `strncpy`, `strrchr`, … (Stage 85-01); Stage 106: `memmove`, `memchr`, `strcat`, `strcoll`, `strcspn`, `strspn`, `strpbrk`, `strstr`, `strtok`, `strerror`, `strxfrm`
- [x] <ctype.h>: classification functions (`isalpha`, `isdigit`, `isspace`, …) (Stage 74); Stage 106: `iscntrl`, `isgraph`, `isprint`, `ispunct`
- [x] <errno.h>: `errno`, `ERANGE`, `EINVAL`, etc. (Stage 74)
- [x] <time.h>: `time_t`, time functions (Stage 74)
- [x] <setjmp.h>: non-local jump support (Stage 74)
- [x] <stdarg.h>: `va_list`, `va_start`, `va_end`, `va_arg`, `va_copy` macros (Stage 75-02)
- [x] <stdio.h>: remaining stub `fwrite` (Stage 106)
- [ ] <math.h>: basic floating-point math functions
- [x] <assert.h>: assert macro — NDEBUG-aware stub in `test/include/assert.h` (Stage 107)

### Code Generation and Optimization
- [ ] Multi-file compilation (compile multiple .c files to .o files)
  - [x] Multiple source files accepted in a single `ccompiler` invocation — each compiled independently to its own `.asm` with full per-file teardown (Stage 96)
- [ ] Object file emission (.o / ELF)
- [ ] Linker invocation
- [ ] Debug information (DWARF)
- [ ] Stack frame alignment (currently assumed 16-byte; verify under all ABI conditions)
- [x] Stack-passing for more than 6 arguments (SysV; caller and callee side) (Stage 68)
- [x] Calling convention for struct arguments and return values (SysV: register-class ≤16 bytes, memory-class via hidden pointer; enabled self-hosting) (Stage 91-01)
- [ ] Floating-point ABI (xmm registers for float/double arguments)
- [ ] Tail-call opportunities
- [x] Constant folding for integer expressions — relational, equality, logical, and ternary operators in constant-expression contexts (Stage 104)
- [ ] Constant folding in general expression codegen (optimizer)
- [ ] Dead code elimination
- [ ] Unreachable code warning
- [ ] Callee-saved register preservation (rbx, rbp, r12–r15)
- [ ] Red-zone usage or avoidance

### Diagnostics and Error Recovery
- [x] Line and column numbers on parser error messages (Stage 70-03; codegen errors still lack position)
- [x] Structured error output (file:line:col: error: message) (Stage 70-03)
- [x] `-Werror` (warnings as errors) (Stage 66 / 70-03)
- [ ] Warning level support (-Wall, -Wextra)
- [x] Multiple errors before aborting (`--max-errors=N`) (Stage 70-01)
- [ ] Pedantic C99 conformance checks
- [ ] Signed integer overflow detection mode

### Tooling and Infrastructure
- [ ] Multi-file / split compilation mode
  - [x] `ccompiler` now accepts one or more source files per invocation, compiling each independently (Stage 96)
- [ ] -o output file option
- [ ] -c compile-only (emit object file) option
- [ ] -S assembly output option (currently the only mode)
- [ ] -O optimization level flags
- [x] Makefile / build system for the compiler itself (`build.sh`: normal / bootstrap / fallback / self-host modes) (Stage 93, 94)
- [ ] Comprehensive conformance test suite against C99 standard
