❯ implement-stage docs/stages/ClaudeC99-spec-stage-07-02-for-statement.md

Read 1 file (ctrl+o to expand)

● Now let me understand the existing codebase structure.

Searched for 2 patterns, read 1 file, listed 1 directory (ctrl+o to expand)

● Now let me look at a while test to understand the test naming convention.

Searched for 2 patterns, read 3 files, listed 2 directories (ctrl+o to expand)

● Good, I have a clear picture. Now I'll implement the for statement support. The changes needed are:

1. Lexer: Add TOKEN_FOR keyword
2. AST: Add AST_FOR_STATEMENT node type
3. Parser: Add parse_for_statement function, wire into parse_statement
4. Codegen: Handle AST_FOR_STATEMENT in codegen_statement

The for statement AST will have up to 4 children: init (or NULL placeholder), condition (or NULL), update (or NULL), body. I'll use a sentinel approach — since children can't
be NULL in the array, I'll use a specific convention. Looking at how the spec defines it: for ( [expr] ; [expr] ; [expr] ) stmt — all three clauses are optional expressions.

I'll represent missing clauses as NULL children and track which are present via the node's value field.

Let me implement it:                                                                                                                                                          
                  
