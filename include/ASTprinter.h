
#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "AST.h"

class ASTPrinter {
private:
    int indentLevel;

    void printIndent() const;
    void printBlock(const BlockNode* block);
    void printStatement(const StmtNode* statement);
    void printExpression(const ExprNode* expression);

public:
    ASTPrinter();

    void print(const ProgramNode* program);
};

#endif
