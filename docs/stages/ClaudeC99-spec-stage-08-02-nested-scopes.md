# Claude C99 Stage 8.2

Task: Add support for nested block scopes and variable shadowing.

## Variable Declaration 
- Block Scopes
  - Each `{ ... }` introduces a new scope.
  - Scopes may be nested arbitrarily.
  
- Variable Declaration Rules
  - Variables must be unique with the same scope. 
  - Variables are visible:
    - Within the scope they are declared
    - Within all nested (child) scopes
  - Variables in inner scopes can shadow ones in outer scopes.

- Name Resolution
  - When resolving a variable:
    1. Look in the current scope
    2. If not found, recursively search parent scopes
  - The nearest (innermost) declaration must be used.

- Redeclaration Rules
  - Redeclaring a variable in the same scope is an error.
  
# No grammar changes
# No tokenizer changes

## 

## Examples

# variables in inner scopes can shadow
`int main() {
   int a = 1;       // a is scope at the function level
   {
      int a = 2;    // this a is in scope for this block
      return a;     // returns a
   }
}
`
# variables from outer scopes can be referenced in inner ones
`
int main() {
  int a = 1;
  {
    int b = a;       // a is referenced in an inner scope
  }
}
`

# Redeclaration error

`
int main() {
  int a = 1;
  int a = 2;   // ERROR: same scope
}`

