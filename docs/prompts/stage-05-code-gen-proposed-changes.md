● Step 3: Code Generation

This is the largest change. Here's what needs to happen and why:

Symbol table: A per-function array mapping variable names to stack offsets ([rbp - N]). Each int variable gets 4 bytes of stack space.

Stack frame prologue: Functions now need push rbp; mov rbp, rsp; sub rsp, N to allocate space for local variables. We pre-scan the function body to   
count declarations so we know N upfront.

New codegen handlers:
- AST_DECLARATION — registers the variable in the symbol table; if initialized, evaluates the expression and stores to [rbp - offset]
- AST_ASSIGNMENT — evaluates RHS, stores to the variable's stack slot
- AST_VAR_REF — loads value from [rbp - offset] into eax

Semantic checks (during codegen):
- Undeclared variable → error with "undeclared variable"
- Duplicate declaration → error with "duplicate declaration"
- Declaration in nested scope → error

Scope enforcement: codegen_function iterates the function body block directly (with allows_decl=1). Any nested block or if-branch encountered in      
codegen_statement passes allows_decl=0, so declarations inside nested structures are rejected.

Let me implement all of this.                  
