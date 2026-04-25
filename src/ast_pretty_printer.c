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
        printf("FunctionDecl: %s %s\n",
               type_kind_name(node->decl_type), node->value);
        break;
    case AST_PARAM:
        printf("Parameter: %s %s\n",
               type_kind_name(node->decl_type), node->value);
        break;
    case AST_BLOCK:
        printf("Block\n");
        break;
    case AST_DECLARATION:
        if (node->decl_type == TYPE_POINTER && node->full_type) {
            printf("VariableDeclaration: ");
            ast_print_type(node->full_type);
            printf("%s\n", node->value);
        } else {
            printf("VariableDeclaration: %s %s\n",
                   type_kind_name(node->decl_type), node->value);
        }
        break;
    case AST_RETURN_STATEMENT:
        printf("ReturnStmt:\n");
        break;
    case AST_INT_LITERAL:
        printf("IntLiteral: %s\n", node->value);
        break;
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
        printf("Assignment: %s\n", node->value);
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
    case AST_CAST:
        printf("Cast: %s\n", type_kind_name(node->decl_type));
        break;
    }

    for (int i = 0; i < node->child_count; i++) {
        ast_pretty_print(node->children[i], depth + 1);
    }
}
