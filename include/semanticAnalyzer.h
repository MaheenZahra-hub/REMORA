#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "AST.h"
#include "errorReporter.h"
#include "symbolTable.h"

#include <string>

class SemanticAnalyzer {
private:
    struct ExpressionResult {
        TypeKind type;
        bool valid;

        ExpressionResult(TypeKind expressionType, bool isValid)
            : type(expressionType), valid(isValid) {}

        ExpressionResult() : type(TypeKind::Nothing), valid(false) {}
    };

    SymbolTable& symbolTable;
    ErrorReporter& reporter;
    bool semanticErrorsFound;
    TypeKind currentReturnType;

    void reportSemantic(int line, const std::string& message);
    void registerFunctionSignatures(const ProgramNode* program);
    void analyzeFunction(const FuncDeclNode* function);
    void analyzeBlock(const BlockNode* block, bool createScope);
    void analyzeStatement(const StmtNode* statement);

    ExpressionResult analyzeExpression(const ExprNode* expression);
    ExpressionResult analyzeCall(const CallExprNode* call);

    static bool isNumeric(TypeKind type);
    static const char* typeName(TypeKind type);

public:
    SemanticAnalyzer(SymbolTable& table, ErrorReporter& errorReporter);

    // Returns true only when semantic analysis reports no semantic errors.
    bool analyze(const ProgramNode* program);
};

#endif
