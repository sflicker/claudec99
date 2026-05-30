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
	- [x] Explicit enumerator values: integer literal or character literal
	- [x] Auto-increment for subsequent enumerators
	- [x] Trailing comma in enumerator list
	- [x] Duplicate enumerator name detection
	- [x] Enum tag table; undefined tag reference rejected
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

---

## TODO

### Preprocessor
- [ ] #pragma (at minimum #pragma once)
- [ ] #elifdef / #elifndef
- [ ] GNU extension: `__VA_OPT__` and named variadic args

### Types
- [ ] float and double types
- [ ] Floating-point arithmetic and comparisons
- [ ] Floating-point literals (decimal and hex forms)
- [ ] Floating-point conversions (int ↔ float ↔ double)
- [ ] ptrdiff_t, size_t, intptr_t awareness
- [x] Union types (Stage 72; anonymous unions Stage 73-01)
- [ ] Bit-field members in structs
- [ ] Flexible array members in structs
- [ ] Compound literals: (Type){ ... }
- [ ] volatile qualifier
- [ ] restrict qualifier on pointers
- [x] Pointer-level const enforcement: writes through const pointers, const-discard conversions (Stage 66)
- [ ] Type compatibility and composite type rules

### Declarations and Scope
- [x] static storage class (block scope — local static variables) (Stage 71)
- [ ] register storage class (hint only)
- [ ] auto storage class (explicit)
- [ ] Tentative definitions for file-scope variables
- [ ] For-loop initializer declarations: for (int i = 0; ...)
- [ ] Multiple pointer levels in multi-declarator lists
- [ ] Function declarations at block scope
- [ ] Incomplete array types in extern declarations

### Initializers
- [ ] Designated initializers for arrays: [idx] = value
- [ ] Designated initializers for structs: .member = value
- [ ] Zero-initialization of static-duration objects (global structs/arrays)

### Expressions
- [ ] sizeof with unsigned result type (size_t)
- [ ] Pointer comparison operators (< <= > >= on pointers)
- [ ] Pointer equality with non-null constants
- [ ] Integer constant expressions (evaluated at compile time)
- [ ] Floating-point constant expressions
- [ ] Lvalue conversion rules for all expression contexts
- [ ] Unary + on floating-point
- [ ] Mixed integer/floating-point arithmetic (usual arithmetic conversions)
- [ ] Integer and floating-point promotions in function arguments (default argument promotions)

### Statements
- [ ] For-loop initializer declarations
- [ ] switch with long / char / short discriminant (after promotion)
- [ ] Case labels with unsigned and long constant values
- [ ] goto across declarations (only legal in C under restrictions)

### Functions
- [x] Variadic function definitions: va_list, va_start, va_end, and va_arg for GP-class types (int/long/long long/pointer) (Stage 75)
  - [ ] va_arg for floating-point and struct-by-value types
  - [ ] va_copy codegen (still a no-op stub)
- [ ] Old-style (K&R) function definitions
- [ ] Implicit return in void functions
- [ ] Functions returning function pointers: int (*f())(int)
- [ ] Inline functions (inline keyword)

### Standard Library Headers (stub or pass-through)
- [x] <stdio.h>: `puts`, `printf` (stub; full set including `fprintf`, `fopen`, `fclose`, `fread`, `fgets`, `snprintf` not yet in stub)
- [x] <stdlib.h>: `malloc`, `free`, `exit` (stub; `calloc`, `realloc`, `qsort` etc. not yet in stub)
- [x] <string.h>: `strlen`, `strcmp`, `strcpy`, `memcpy`, `memset` (stub)
- [x] <stddef.h>: `NULL`, `size_t` (stub)
- [x] <stdint.h>: full exact-width, least-width, fast, and pointer-size integer typedefs (stub complete)
- [x] <limits.h>: full set including `LLONG_MIN`, `LLONG_MAX`, `ULLONG_MAX` (stub complete)
- [x] <stdbool.h>: `bool`, `true`, `false` (stub complete)
- [x] <stdio.h>: expanded with opaque `FILE`, `EOF`, `fopen`, `fclose`, `fgetc`, `fgets`, `fprintf`, `snprintf`, `vfprintf`, `vprintf`, `vsnprintf` (Stage 67, 75-04); `fread`/`fwrite`/`stderr` still pending
- [x] <ctype.h>: classification functions (`isalpha`, `isdigit`, `isspace`, …) (Stage 74)
- [x] <errno.h>: `errno`, `ERANGE`, `EINVAL`, etc. (Stage 74)
- [x] <time.h>: `time_t`, time functions (Stage 74)
- [x] <setjmp.h>: non-local jump support (Stage 74)
- [x] <stdarg.h>: `va_list`, `va_start`, `va_end`, `va_arg`, `va_copy` macros (Stage 75-02)
- [ ] <stdio.h>: remaining stubs `fread`, `fwrite`, `stderr` (needed for self-hosting)
- [ ] <math.h>: basic floating-point math functions
- [ ] <assert.h>: assert macro

### Code Generation and Optimization
- [ ] Multi-file compilation (compile multiple .c files to .o files)
- [ ] Object file emission (.o / ELF)
- [ ] Linker invocation
- [ ] Debug information (DWARF)
- [ ] Stack frame alignment (currently assumed 16-byte; verify under all ABI conditions)
- [x] Stack-passing for more than 6 arguments (SysV; caller and callee side) (Stage 68)
- [ ] Calling convention for struct arguments and return values (hidden-pointer ABI for large structs; prerequisite for self-hosting)
- [ ] Floating-point ABI (xmm registers for float/double arguments)
- [ ] Tail-call opportunities
- [ ] Constant folding for integer expressions
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
- [ ] -o output file option
- [ ] -c compile-only (emit object file) option
- [ ] -S assembly output option (currently the only mode)
- [ ] -O optimization level flags
- [ ] Makefile / build system for the compiler itself
- [ ] Comprehensive conformance test suite against C99 standard
