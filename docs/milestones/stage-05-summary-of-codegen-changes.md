Summary of Code Generation Changes

New data structures:
- LocalVar struct: {name, offset} pair for each variable
- CodeGen extended with: locals[MAX_LOCALS], local_count, stack_offset

New helper functions:
- codegen_find_var() — looks up a variable by name, returns its stack offset (0 = not found)
- codegen_add_var() — registers a new variable, assigns next 4-byte stack slot
- count_declarations() — pre-scans a block to count declarations for stack sizing

Updated functions:

- codegen_expression — added AST_VAR_REF: loads [rbp - offset] into eax, errors on undeclared variable
- codegen_statement — new allows_decl parameter + two new cases:
    - AST_DECLARATION: checks for duplicates, registers in symbol table, optionally stores initializer
    - AST_ASSIGNMENT: checks variable exists, evaluates RHS, stores to stack slot
    - If-branches and nested blocks pass allows_decl=0
- codegen_function — rewritten to:
    - Reset symbol table per function
    - Pre-scan for declaration count
    - Emit stack frame prologue (push rbp; mov rbp, rsp; sub rsp, N) only when variables exist
    - Iterate body block directly with allows_decl=1

Results: 39/39 valid tests pass, 2/2 invalid tests correctly rejected.                                      