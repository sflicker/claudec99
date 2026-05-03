## Project layout

This project uses a split C source/header layout:

- C source files live in `src/`
- Public/project headers live in `include/`
- Do not look for project `.h` files under `src/`

Important headers:

- `include/token.h`
- `include/ast.h`
- `include/lexer.h`
- `include/parser.h`
- `include/codegen.h`
- `include/ast_pretty_printer.h`
- `include/util.h`

When inspecting declarations, start in `include/`.
When inspecting implementations, start in `src/`.

Useful discovery command:

```bash
find include src -maxdepth 2 -type f | sort

