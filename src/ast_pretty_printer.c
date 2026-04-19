#include <stdio.h>
#include <string.h>
#include "ast_pretty_printer.h"

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

void ast_pretty_print(ASTNode *node, int depth) {
    if (!node) return;

    ast_print_indent(depth);

    switch (node->type) {
    case AST_TRANSLATION_UNIT:
        printf("TranslationUnit:\n");
        break;
    case AST_FUNCTION_DECL:
        printf("FunctionDecl: %s\n", node->value);
        break;
    case AST_PARAM:
        printf("Parameter: %s\n", node->value);
        break;
    case AST_BLOCK:
        printf("Block\n");
        break;
    case AST_DECLARATION:
        printf("VariableDeclaration: %s\n", node->value);
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
    case AST_FOR_STATEMENT:
        printf("ForStmt:\n");
        /* For statements have NULL children for omitted clauses */
        for (int i = 0; i < node->child_count; i++) {
            if (node->children[i]) {
                ast_pretty_print(node->children[i], depth + 1);
            }
        }
        return;
    case AST_PREFIX_INC_DEC:
        printf("PrefixIncDec: %s\n", operator_name(node->value));
        break;
    case AST_POSTFIX_INC_DEC:
        printf("PostfixIncDec: %s\n", operator_name(node->value));
        break;
    case AST_FUNCTION_CALL:
        printf("FunctionCall: %s\n", node->value);
        break;
    }

    for (int i = 0; i < node->child_count; i++) {
        ast_pretty_print(node->children[i], depth + 1);
    }
}
