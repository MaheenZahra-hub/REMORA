
#include "ASTprinter.h"
#include <iostream>
#include <string>

using namespace std;

// Convert data types into readable names
static string typeName(TypeKind type) {
    switch (type) {
        case TypeKind::Whole: return "Whole";
        case TypeKind::Decimal: return "Decimal";
        case TypeKind::Text: return "Text";
        case TypeKind::Truth: return "Truth";
        case TypeKind::Nothing: return "Nothing";
    }
    return "Unknown";
}

// Convert function kinds into readable names
static string functionKindName(FunctionKind kind) {
    switch (kind) {
        case FunctionKind::Launch: return "Launch";
        case FunctionKind::Craft: return "Craft";
    }
    return "Unknown";
}

// Convert operators into readable names
static string operatorName(TokenType op) {
    switch (op) {
        case OP_IS: return "IS";
        case OP_ISNT: return "ISNT";
        case OP_ABOVE: return "ABOVE";
        case OP_BELOW: return "BELOW";
        case OP_ATLEAST: return "ATLEAST";
        case OP_ATMOST: return "ATMOST";
        case OP_AND: return "AND";
        case OP_OR: return "OR";
        case OP_NOT: return "NOT";
        case OP_PLUS: return "PLUS";
        case OP_MINUS: return "MINUS";
        case OP_MULT: return "MULTIPLY";
        case OP_DIV: return "DIVIDE";
        case OP_MOD: return "MODULO";
        case OP_ASSIGN: return "ASSIGN";
        default: return "UNKNOWN_OPERATOR";
    }
}

ASTPrinter::ASTPrinter() {
    indentLevel = 0;
}

// Print indentation according to tree depth
void ASTPrinter::printIndent() const {
    for (int i = 0; i < indentLevel; i++) {
        cout << "  ";
    }
}

// Print the complete program AST
void ASTPrinter::print(const ProgramNode* program) {
    indentLevel = 0;

    if (program == 0) {
        cout << "AST is empty.\n";
        return;
    }

    cout << "========== ABSTRACT SYNTAX TREE ==========\n";

    printIndent();
    cout << "Program\n";

    indentLevel++;

    for (size_t i = 0; i < program->declarations.size(); i++) {
        const FuncDeclNode* function = program->declarations[i];

        if (function == 0) {
            continue;
        }

        printIndent();
        cout << functionKindName(function->kind)
             << " Function: ";

        if (function->name != 0) {
            cout << function->name->name;
        } else {
            cout << "<unnamed>";
        }

        cout << "\n";

        indentLevel++;

        printIndent();
        cout << "Return Type: "
             << typeName(function->returnType) << "\n";

        printIndent();
        cout << "Parameters\n";

        indentLevel++;

        for (size_t j = 0; j < function->parameters.size(); j++) {
            const Parameter& parameter = function->parameters[j];

            printIndent();
            cout << parameter.name << " : "
                 << typeName(parameter.type) << "\n";
        }

        indentLevel--;

        printBlock(function->body);

        indentLevel--;
    }

    indentLevel--;

    cout << "==========================================\n";
}

// Print a block and all its statements
void ASTPrinter::printBlock(const BlockNode* block) {
    if (block == 0) {
        printIndent();
        cout << "Empty Block\n";
        return;
    }

    printIndent();
    cout << "Block\n";

    indentLevel++;

    for (size_t i = 0; i < block->statements.size(); i++) {
        printStatement(block->statements[i]);
    }

    indentLevel--;
}

// Print individual statements
void ASTPrinter::printStatement(const StmtNode* statement) {
    if (statement == 0) {
        return;
    }

    // Variable declaration
    const VarDeclNode* variable =
        dynamic_cast<const VarDeclNode*>(statement);

    if (variable != 0) {
        printIndent();
        cout << (variable->isFixed ? "Fixed Declaration: "
                                   : "Variable Declaration: ");

        if (variable->name != 0) {
            cout << variable->name->name;
        }

        cout << " : " << typeName(variable->type) << "\n";

        if (variable->hasInitializer) {
            indentLevel++;
            printIndent();
            cout << "Initializer\n";
            indentLevel++;
            printExpression(variable->initializer);
            indentLevel -= 2;
        }

        return;
    }

    // Assignment
    const AssignNode* assignment =
        dynamic_cast<const AssignNode*>(statement);

    if (assignment != 0) {
        printIndent();
        cout << "Assignment\n";

        indentLevel++;

        printIndent();
        cout << "Target: ";

        if (assignment->target != 0) {
            cout << assignment->target->name;
        }

        cout << "\n";

        printExpression(assignment->value);

        indentLevel--;
        return;
    }

    // If statement
    const IfNode* ifNode =
        dynamic_cast<const IfNode*>(statement);

    if (ifNode != 0) {
        printIndent();
        cout << "When Statement\n";

        indentLevel++;

        printIndent();
        cout << "Condition\n";

        indentLevel++;
        printExpression(ifNode->condition);
        indentLevel--;

        printIndent();
        cout << "Then Block\n";
        indentLevel++;
        printBlock(ifNode->thenBlock);
        indentLevel--;

        if (ifNode->hasElseBlock) {
            printIndent();
            cout << "Otherwise Block\n";

            indentLevel++;
            printBlock(ifNode->elseBlock);
            indentLevel--;
        }

        indentLevel--;
        return;
    }

    // While statement
    const WhileNode* whileNode =
        dynamic_cast<const WhileNode*>(statement);

    if (whileNode != 0) {
        printIndent();
        cout << "Repeat While Statement\n";

        indentLevel++;

        printIndent();
        cout << "Condition\n";

        indentLevel++;
        printExpression(whileNode->condition);
        indentLevel--;

        printBlock(whileNode->body);

        indentLevel--;
        return;
    }

    // Return statement
    const ReturnNode* returnNode =
        dynamic_cast<const ReturnNode*>(statement);

    if (returnNode != 0) {
        printIndent();
        cout << "Yield Statement\n";

        if (returnNode->value != 0) {
            indentLevel++;
            printExpression(returnNode->value);
            indentLevel--;
        }

        return;
    }

    // Display statement
    const DisplayNode* displayNode =
        dynamic_cast<const DisplayNode*>(statement);

    if (displayNode != 0) {
        printIndent();
        cout << "Display Statement\n";

        indentLevel++;
        printExpression(displayNode->value);
        indentLevel--;

        return;
    }

    // Function call statement
    const CallStmtNode* callStatement =
        dynamic_cast<const CallStmtNode*>(statement);

    if (callStatement != 0) {
        printIndent();
        cout << "Function Call Statement\n";

        indentLevel++;
        printExpression(callStatement->call);
        indentLevel--;

        return;
    }

    // Nested block
    const BlockNode* nestedBlock =
        dynamic_cast<const BlockNode*>(statement);

    if (nestedBlock != 0) {
        printBlock(nestedBlock);
        return;
    }

    printIndent();
    cout << "Unknown Statement\n";
}

// Print expressions recursively
void ASTPrinter::printExpression(const ExprNode* expression) {
    if (expression == 0) {
        printIndent();
        cout << "<null expression>\n";
        return;
    }

    // Literal
    const LiteralNode* literal =
        dynamic_cast<const LiteralNode*>(expression);

    if (literal != 0) {
        printIndent();
        cout << "Literal: " << literal->lexeme << "\n";
        return;
    }

    // Identifier
    const IdentNode* identifier =
        dynamic_cast<const IdentNode*>(expression);

    if (identifier != 0) {
        printIndent();
        cout << "Identifier: " << identifier->name << "\n";
        return;
    }

    // Binary expression
    const BinaryExprNode* binary =
        dynamic_cast<const BinaryExprNode*>(expression);

    if (binary != 0) {
        printIndent();
        cout << "Binary Expression: "
             << operatorName(binary->op) << "\n";

        indentLevel++;

        printIndent();
        cout << "Left Operand\n";
        indentLevel++;
        printExpression(binary->left);
        indentLevel--;

        printIndent();
        cout << "Right Operand\n";
        indentLevel++;
        printExpression(binary->right);
        indentLevel--;

        indentLevel--;
        return;
    }

    // Unary expression
    const UnaryExprNode* unary =
        dynamic_cast<const UnaryExprNode*>(expression);

    if (unary != 0) {
        printIndent();
        cout << "Unary Expression: "
             << operatorName(unary->op) << "\n";

        indentLevel++;
        printExpression(unary->operand);
        indentLevel--;

        return;
    }

    // Function call expression
    const CallExprNode* call =
        dynamic_cast<const CallExprNode*>(expression);

    if (call != 0) {
        printIndent();
        cout << "Function Call: ";

        if (call->callee != 0) {
            cout << call->callee->name;
        } else {
            cout << "<unknown>";
        }

        cout << "\n";

        indentLevel++;

        for (size_t i = 0; i < call->arguments.size(); i++) {
            printIndent();
            cout << "Argument " << i + 1 << "\n";

            indentLevel++;
            printExpression(call->arguments[i]);
            indentLevel--;
        }

        indentLevel--;
        return;
    }

    printIndent();
    cout << "Unknown Expression\n";
}
