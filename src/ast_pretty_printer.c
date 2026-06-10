#include <stdio.h>
#include <string.h>
#include "ast_pretty_printer.h"
#include "type.h"

static const char *operator_name(const char *op) {
    if (strcmp(op, "+") == 0)  return "ADD";
    if (strcmp(op, "-") == 0)  return "SUBTRACT";
    if (strcmp(op, "*") == 0)  return "MULTIPLY";
    if (strcmp(op, "/") == 0)  return "DIVIDE";
    if (strcmp(op, "==") == 0) return "EQUAL";
    if (strcmp(op, "!=") == 0) return "NOTEQUAL";
    if (strcmp(op, "<") == 0)  return "LESSTHAN";
    if (strcmp(op, "<=") == 0) return "LESSTHANOREQUAL";
    if (strcmp(op, ">") == 0)  return "GREATERTHAN";
    if (strcmp(op, ">=") == 0) return "GREATERTHANOREQUAL";
    if (strcmp(op, "!") == 0)  return "NOT";
    if (strcmp(op, "&&") == 0) return "AND";
    if (strcmp(op, "||") == 0) return "OR";
    if (strcmp(op, "&") == 0)  return "BITAND";
    if (strcmp(op, "^") == 0)  return "BITXOR";
    if (strcmp(op, "|") == 0)  return "BITOR";
    if (strcmp(op, "++") == 0) return "INCREMENT";
    if (strcmp(op, "--") == 0) return "DECREMENT";
    return op;
}

static void ast_print_indent(int depth) {
    for (int i = 0; i < depth * 2; i++) {
        putchar(' ');
    }
}

/*
 * Render the modifiers carried directly on a declaration/parameter
 * AST node: storage class (extern/static), const, unsigned. Order
 * matches typical C source order so the printed declaration reads
 * left-to-right as written.
 */
static void ast_print_decl_qualifiers(ASTNode *node) {
    if (node->storage_class == SC_EXTERN) printf("extern ");
    else if (node->storage_class == SC_STATIC) printf("static ");
    if (node->is_const) printf("const ");
    if (node->is_unsigned) printf("unsigned ");
}

/*
 * Stage 12-01: print a Type chain in C-style notation. The base
 * integer type prints with a trailing space; each pointer level
 * appends a '*' (no separating space), so `int **` reads as
 * "int **" before the variable name.
 */
static void ast_print_type(Type *t) {
    if (!t) return;
    if (t->kind == TYPE_POINTER) {
        ast_print_type(t->base);
        printf("*");
    } else {
        printf("%s ", type_kind_name(t->kind));
    }
}

void ast_pretty_print(ASTNode *node, int depth) {
    if (!node) return;

    ast_print_indent(depth);

    switch (node->type) {
    case AST_TRANSLATION_UNIT:
        printf("TranslationUnit:\n");
        break;
    case AST_FUNCTION_DECL:
        printf("FunctionDecl: ");
        ast_print_decl_qualifiers(node);
        if (node->decl_type == TYPE_POINTER && node->full_type) {
            ast_print_type(node->full_type);
            printf("%s\n", node->value);
        } else {
            printf("%s %s\n",
                   type_kind_name(node->decl_type), node->value);
        }
        break;
    case AST_PARAM:
        printf("Parameter: ");
        ast_print_decl_qualifiers(node);
        if (node->decl_type == TYPE_POINTER && node->full_type) {
            ast_print_type(node->full_type);
            printf("%s\n", node->value);
        } else {
            printf("%s %s\n",
                   type_kind_name(node->decl_type), node->value);
        }
        break;
    case AST_BLOCK:
        printf("Block\n");
        break;
    case AST_DECL_LIST:
        printf("DeclList:\n");
        break;
    case AST_DECLARATION:
        printf("VariableDeclaration: ");
        ast_print_decl_qualifiers(node);
        if (node->decl_type == TYPE_ARRAY && node->full_type) {
            /* Stage 13-01: render `<element-type> <name>[<length>]`.
             * The element type prints with its trailing space (or
             * pointer asterisks) via ast_print_type. */
            ast_print_type(node->full_type->base);
            printf("%s[%d]\n", node->value, node->full_type->length);
        } else if (node->decl_type == TYPE_POINTER && node->full_type) {
            ast_print_type(node->full_type);
            printf("%s\n", node->value);
        } else {
            printf("%s %s\n",
                   type_kind_name(node->decl_type), node->value);
        }
        break;
    case AST_RETURN_STATEMENT:
        printf("ReturnStmt:\n");
        break;
    case AST_INT_LITERAL:
        printf("IntLiteral: %s\n", node->value);
        break;
    case AST_CHAR_LITERAL: {
        /* Stage 15-02: print spelling and evaluated integer value.
         * The decoded byte sits at node->value[0]; re-escape it so the
         * source-form spelling is recoverable in the dump. */
        unsigned char b = (unsigned char)node->value[0];
        printf("CharLiteral: '");
        switch (b) {
        case '\n': printf("\\n");  break;
        case '\t': printf("\\t");  break;
        case '\r': printf("\\r");  break;
        case '\\': printf("\\\\"); break;
        case '\'': printf("\\'");  break;
        case '"':  printf("\\\""); break;
        case 0:    printf("\\0");  break;
        default:   putchar(b);     break;
        }
        printf("' (%d)\n", b);
        break;
    }
    case AST_STRING_LITERAL: {
        /* Stage 14-05: re-escape decoded bytes back to source form so
         * pretty-print output stays line-oriented and stable for diff
         * testing. byte_length is the authoritative count after escape
         * decoding; the payload may contain embedded NUL bytes. */
        int byte_length = node->byte_length;
        printf("StringLiteral: \"");
        for (int i = 0; i < byte_length; i++) {
            unsigned char b = (unsigned char)node->value[i];
            switch (b) {
            case '\n': printf("\\n");  break;
            case '\t': printf("\\t");  break;
            case '\r': printf("\\r");  break;
            case '\\': printf("\\\\"); break;
            case '"':  printf("\\\""); break;
            case 0:    printf("\\0");  break;
            default:   putchar(b);     break;
            }
        }
        printf("\" (length %d)\n", byte_length);
        break;
    }
    case AST_VAR_REF:
        printf("VariableExpression: %s\n", node->value);
        break;
    case AST_BINARY_OP:
        printf("Binary: %s\n", operator_name(node->value));
        break;
    case AST_UNARY_OP:
        printf("Unary: %s\n", operator_name(node->value));
        break;
    case AST_ASSIGNMENT:
        if (!node->value || node->value[0] == '\0') {
            /* Stage 12-03: deref-LHS assignment has no variable
             * name — children carry [LHS, RHS]. */
            printf("Assignment:\n");
        } else {
            printf("Assignment: %s\n", node->value);
        }
        break;
    case AST_EXPRESSION_STMT:
        printf("ExpressionStatement\n");
        break;
    case AST_IF_STATEMENT:
        printf("IfStmt:\n");
        break;
    case AST_WHILE_STATEMENT:
        printf("WhileStmt:\n");
        break;
    case AST_DO_WHILE_STATEMENT:
        printf("DoWhileStmt:\n");
        break;
    case AST_FOR_STATEMENT:
        printf("ForStmt:\n");
        /* For statements have NULL children for omitted clauses */
        for (int i = 0; i < node->child_count; i++) {
            if (node->children[i]) {
                ast_pretty_print(node->children[i], depth + 1);
            }
        }
        return;
    case AST_SWITCH_STATEMENT:
        printf("SwitchStmt:\n");
        break;
    case AST_DEFAULT_SECTION:
        printf("DefaultSection:\n");
        if (node->child_count > 0) {
            ast_pretty_print(node->children[0], depth + 1);
        }
        return;
    case AST_CASE_SECTION:
        printf("CaseSection: %s\n", node->children[0]->value);
        if (node->child_count > 1) {
            ast_pretty_print(node->children[1], depth + 1);
        }
        return;
    case AST_BREAK_STATEMENT:
        printf("BreakStmt\n");
        break;
    case AST_CONTINUE_STATEMENT:
        printf("ContinueStmt\n");
        break;
    case AST_GOTO_STATEMENT:
        printf("GotoStmt: %s\n", node->value);
        break;
    case AST_LABEL_STATEMENT:
        printf("LabelStmt: %s\n", node->value);
        break;
    case AST_PREFIX_INC_DEC:
        printf("PrefixIncDec: %s\n", operator_name(node->value));
        break;
    case AST_POSTFIX_INC_DEC:
        printf("PostfixIncDec: %s\n", operator_name(node->value));
        break;
    case AST_FUNCTION_CALL:
        printf("FunctionCall: %s\n", node->value);
        break;
    case AST_INDIRECT_CALL:
        printf("IndirectCall:\n");
        break;
    case AST_CAST:
        printf("Cast: %s\n", type_kind_name(node->decl_type));
        break;
    case AST_ADDR_OF:
        printf("AddressOf:\n");
        break;
    case AST_DEREF:
        printf("Dereference:\n");
        break;
    case AST_ARRAY_INDEX:
        printf("ArrayIndex:\n");
        break;
    case AST_SIZEOF_TYPE:
        printf("SizeofType: %s\n", type_kind_name(node->decl_type));
        break;
    case AST_SIZEOF_EXPR:
        printf("SizeofExpr:\n");
        break;
    case AST_CONDITIONAL_EXPR:
        printf("Conditional:\n");
        break;
    case AST_COMMA_EXPR:
        printf("CommaExpr:\n");
        break;
    case AST_TYPEDEF_DECL:
        printf("TypedefDecl: %s\n", node->value);
        break;
    case AST_MEMBER_ACCESS:
        printf("MemberAccess: %s\n", node->value);
        break;
    case AST_ARROW_ACCESS:
        printf("ArrowAccess: %s\n", node->value);
        break;
    case AST_INITIALIZER_LIST:
        printf("InitializerList:\n");
        break;
    case AST_DESIGNATED_INIT:
        if (node->value != NULL)
            printf("DESIGNATED_INIT(.%s)\n", node->value);
        else
            printf("DESIGNATED_INIT([%d])\n", node->byte_length);
        break;
    case AST_COMPOUND_LITERAL:
        /* node->value holds a pre-formatted type label set at parse time. */
        printf("COMPOUND_LITERAL(%s)\n", node->value ? node->value : "?");
        break;
    case AST_BUILTIN_VA_START:
        printf("BuiltinVaStart:\n");
        break;
    case AST_BUILTIN_VA_END:
        printf("BuiltinVaEnd:\n");
        break;
    case AST_BUILTIN_VA_COPY:
        printf("BuiltinVaCopy:\n");
        break;
    case AST_BUILTIN_VA_ARG:
        printf("BuiltinVaArg: %s\n", node->value);
        break;
    }

    for (int i = 0; i < node->child_count; i++) {
        ast_pretty_print(node->children[i], depth + 1);
    }
}
