# Claude C99 Stage 11.01

## Task
- Add Lexer, Parser, and AST type annotation support for basic integer types keywords (char, short and long)

## Requirements
- New tokens will be added for integer types
  - char
  - short
  - long
- Update Lexer to support these new tokens
- Update declaration parsing so local variable declarations may use:
  - char
  - short
  - int
  - long
- Add A type enum, such as TypeKind, type represent supported integer types.
- Variable declaration AST Nodes must record the declared type.
- AST printer will be updated to reflect new types
- For this stage. Do NOT change codegen. codegen will continue handling only `int` types
- For this stage. Do NOT change function parameter/argument/return related grammar rules.
- For this stage, Do NOT handle mixed type arithmetic
- For this stage, DO NOT handle signed/unsiged type variants

## New Tokens
- char
- short
- long

## Grammar Updates
```ebnf
    UPDATE:
    <declaration> ::= <integer-type> <identifier> [ "=" <expression> ] ";"
    
    ADD:
    <integer-type> ::= "char" | "short" | "int" | "long" 
```
In this stage, <integer-type> applies only to variable declarations.
Function declarations, Function definitions, parameter declarations, 
and return types remain restricted to `int`.

## Type System
- This stage is the first part of adding a type system to the compiler.
- New code modules should added in this stage to handle types although some will be stubs at this stage.
- example type system code

```
typedef enum {
TYPE_CHAR,
TYPE_SHORT,
TYPE_INT,
TYPE_LONG
} TypeKind;

typedef struct Type {
TypeKind kind;
int size;
int alignment;
bool is_signed;
} Type;

potential helper functions for types. maybe be stubbed out for this stage.

Type * type_char(void);
Type * type_short(void);
Type * type_int(void);
Type * type_long(void);

const char * type_kind_name(TypeKind kind);
int type_size(Type *t);
int type_alignment(Type *t);
bool type_is_integer(Type *t);
... more added in future stages.
```

## Parser updates
- The parser will handle the new `<integer-type>` in `<declaration>` and add type appropriate type information to AST nodes.

## Semantic Helper Ideas
Potential helper functions that could be added
```
   Type *promote_integer_type(Type *t);
   Type *common_integer_type(Type *a, Type *b);
   bool type_is_integer(Type * t);
   Type * get_expression_type(ASTNode * expr);
```

## Codegen Update
- Codegen output remains unchanged. 
- Codegen will accept the new type in AST nodes for variable declarations but handle as int for this stage.

## Codegen Expectations
1. Keep stage 11.2 behavior
  - `char` load -> sign-extend to working register
  - `short` load -> sign-extend
  - `int` load -> 32 bit load
  - `long` load -> 64 bit load
2. Operate using expression type
  - `int` expressions use 32-bit arithmetic
  - `long` expression use 64-bit arithmetic
3. Store using destination type
  - Assignment truncates/narrows as needed using Stage 11.2 typed store logic.

## Testing
- several new tests have been added to valid cases
- Add additional tests to fill testing gaps
- Add tests should pass.
