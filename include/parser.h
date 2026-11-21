#ifndef PARSER_H
#define PARSER_H

#include "AST.h"
#include "errorReporter.h"
#include "token.h"

class Parser {
private:
    const Token* tokens;
    int tokenCount;
    int position;
    ErrorReporter& reporter;
    Token eofToken;

    const Token& peek(int lookahead = 0) const;
    Token advance();
    bool match(TokenType type);
    Token expect(TokenType type, const char* description);
    void reportSyntax(const std::string& message);
    void fail(const std::string& message);
    void synchronizeStatement();
    void synchronizeTopLevel();

    FuncDeclNode* parseFunction();
    BlockNode* parseBlock();
    StmtNode* parseStatement();
    StmtNode* parseVariableDeclaration();
    StmtNode* parseAssignment();
    StmtNode* parseIf();
    StmtNode* parseWhile();
    StmtNode* parseReturn();
    StmtNode* parseDisplay();
    StmtNode* parseCallStatement();
    ExprNode* parseCondition();
    TypeKind parseType(bool allowNothing);

    ExprNode* parseExpression();
    ExprNode* parseOr();
    ExprNode* parseAnd();
    ExprNode* parseNot();
    ExprNode* parseEquality();
    ExprNode* parseRelational();
    ExprNode* parseAdditive();
    ExprNode* parseMultiplicative();
    ExprNode* parseUnary();
    ExprNode* parsePrimary();
    CallExprNode* parseCall();

    typedef ExprNode* (Parser::*ExpressionParser)();
    ExprNode* parseBinaryLevel(ExpressionParser nextLevel,
                               const TokenType operators[], int operatorCount);

public:
    Parser(const Token sourceTokens[], int sourceTokenCount, ErrorReporter& errorReporter);

    // The caller owns the returned ProgramNode and must delete it.
    // Recoverable syntax errors are reported while valid constructs are retained.
    ProgramNode* parseProgram();
};

#endif