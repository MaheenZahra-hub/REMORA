#include "../include/parser.h"

#include <utility>

namespace {

class ParseFailure {};

template <class T>
class ScopedNode {
private:
    T* pointer;

public:
    explicit ScopedNode(T* node = 0) : pointer(node) {}

    ~ScopedNode() {
        delete pointer;
    }

    ScopedNode(const ScopedNode&) = delete;
    ScopedNode& operator=(const ScopedNode&) = delete;

    T* get() const {
        return pointer;
    }

    T* release() {
        T* result = pointer;
        pointer = 0;
        return result;
    }

    void reset(T* node) {
        if (pointer != node) {
            delete pointer;
            pointer = node;
        }
    }
};

template <class T>
class OwnedPointerArray {
private:
    DynamicArray<T*> items;
    bool ownsItems;

public:
    OwnedPointerArray() : ownsItems(true) {}

    ~OwnedPointerArray() {
        if (ownsItems) {
            for (std::size_t i = 0; i < items.size(); ++i) {
                delete items[i];
            }
        }
    }

    OwnedPointerArray(const OwnedPointerArray&) = delete;
    OwnedPointerArray& operator=(const OwnedPointerArray&) = delete;

    void pushBack(T* item) {
        items.pushBack(item);
    }

    std::size_t size() const {
        return items.size();
    }

    DynamicArray<T*> releaseArray() {
        ownsItems = false;
        return std::move(items);
    }
};

bool containsOperator(TokenType type, const TokenType operators[], int operatorCount) {
    for (int i = 0; i < operatorCount; ++i) {
        if (type == operators[i]) {
            return true;
        }
    }
    return false;
}

} // namespace

Parser::Parser(const Token sourceTokens[], int sourceTokenCount, ErrorReporter& errorReporter)
    : tokens(sourceTokens), tokenCount(sourceTokenCount > 0 ? sourceTokenCount : 0),
      position(0), reporter(errorReporter),
      eofToken{END_OF_FILE, "EOF", 1} {}

const Token& Parser::peek(int lookahead) const {
    if (tokens == 0 || tokenCount == 0 || lookahead < 0 ||
        position >= tokenCount || lookahead >= tokenCount - position) {
        return eofToken;
    }
    return tokens[position + lookahead];
}

Token Parser::advance() {
    Token result = peek();
    if (tokens != 0 && position < tokenCount &&
        tokens[position].type != END_OF_FILE) {
        ++position;
    }
    return result;
}

bool Parser::match(TokenType type) {
    if (peek().type != type) {
        return false;
    }
    advance();
    return true;
}

Token Parser::expect(TokenType type, const char* description) {
    if (peek().type != type) {
        fail(std::string("Expected ") + description + ".");
    }
    return advance();
}

void Parser::reportSyntax(const std::string& message) {
    reporter.reportSyntax(peek().line, message);
}

void Parser::fail(const std::string& message) {
    reportSyntax(message);
    throw ParseFailure();
}

ProgramNode* Parser::parseProgram() {
    position = 0;
    if (tokens == 0 || tokenCount == 0 || peek().type == END_OF_FILE) {
        reportSyntax("Expected a function declaration.");
        return 0;
    }

    const int programLine = peek().line;
    OwnedPointerArray<FuncDeclNode> functions;

    while (peek().type != END_OF_FILE) {
        const int startPosition = position;
        try {
            ScopedNode<FuncDeclNode> function(parseFunction());
            functions.pushBack(function.get());
            function.release();
        } catch (const ParseFailure&) {
            synchronizeTopLevel();
        }

        if (position == startPosition && peek().type != END_OF_FILE) {
            advance();
        }
    }

    if (functions.size() == 0) {
        return 0;
    }

    ScopedNode<ProgramNode> program(
        new ProgramNode(programLine, functions.releaseArray()));
    return program.release();
}

void Parser::synchronizeStatement() {
    int nestedBraces = 0;
    while (peek().type != END_OF_FILE) {
        if (peek().type == SEMICOLON && nestedBraces == 0) {
            advance();
            return;
        }
        if (peek().type == RBRACE) {
            if (nestedBraces == 0) {
                return;
            }
            advance();
            --nestedBraces;
            if (nestedBraces == 0) {
                return;
            }
        } else if (peek().type == LBRACE) {
            ++nestedBraces;
            advance();
        } else {
            advance();
        }
    }
}

void Parser::synchronizeTopLevel() {
    int nestedBraces = 0;
    while (peek().type != END_OF_FILE) {
        if (nestedBraces == 0 &&
            (peek().type == KW_LAUNCH || peek().type == KW_CRAFT)) {
            return;
        }
        if (peek().type == LBRACE) {
            ++nestedBraces;
            advance();
        } else if (peek().type == RBRACE) {
            advance();
            if (nestedBraces > 0) {
                --nestedBraces;
                if (nestedBraces == 0) {
                    return;
                }
            } else {
                return;
            }
        } else if (peek().type == SEMICOLON && nestedBraces == 0) {
            advance();
            return;
        } else {
            advance();
        }
    }
}

FuncDeclNode* Parser::parseFunction() {
    Token declarationToken = peek();
    FunctionKind functionKind = FunctionKind::Launch;
    if (peek().type == KW_LAUNCH) {
        declarationToken = advance();
        functionKind = FunctionKind::Launch;
    } else if (peek().type == KW_CRAFT) {
        declarationToken = advance();
        functionKind = FunctionKind::Craft;
    } else {
        fail("Expected 'launch' or 'craft' function declaration.");
    }

    Token nameToken = expect(IDENTIFIER, "function name");
    ScopedNode<IdentNode> name(new IdentNode(nameToken.line, nameToken.lexeme));
    expect(LPAREN, "'('");

    DynamicArray<Parameter> parameters;
    if (peek().type != RPAREN) {
        do {
            Token parameterName = expect(IDENTIFIER, "parameter name");
            expect(COLON, "':'");
            TypeKind parameterType = parseType(false);
            parameters.pushBack(Parameter(parameterName.line, parameterName.lexeme, parameterType));
        } while (match(COMMA));
    }
    expect(RPAREN, "')'");
    expect(KW_GIVES, "'gives'");
    TypeKind returnType = parseType(true);

    ScopedNode<BlockNode> body(parseBlock());
    ScopedNode<FuncDeclNode> function(
        new FuncDeclNode(declarationToken.line, functionKind, name.get(),
                         std::move(parameters), returnType, body.get()));
    name.release();
    body.release();
    return function.release();
}

BlockNode* Parser::parseBlock() {
    Token openingBrace = expect(LBRACE, "'{'");
    OwnedPointerArray<StmtNode> statements;

    while (peek().type != RBRACE) {
        if (peek().type == END_OF_FILE) {
            reportSyntax("Expected '}' before end of file.");
            break;
        }
        const int startPosition = position;
        try {
            ScopedNode<StmtNode> statement(parseStatement());
            statements.pushBack(statement.get());
            statement.release();
        } catch (const ParseFailure&) {
            synchronizeStatement();
        }
        if (position == startPosition && peek().type != RBRACE &&
            peek().type != END_OF_FILE) {
            advance();
        }
    }
    if (peek().type == RBRACE) {
        advance();
    }

    ScopedNode<BlockNode> block(
        new BlockNode(openingBrace.line, statements.releaseArray()));
    return block.release();
}

StmtNode* Parser::parseStatement() {
    switch (peek().type) {
    case KW_FORM:
    case KW_FIXED:
        return parseVariableDeclaration();
    case IDENTIFIER:
        if (peek(1).type == OP_ASSIGN) {
            return parseAssignment();
        }
        if (peek(1).type == LPAREN) {
            return parseCallStatement();
        }
        break;
    case KW_WHEN:
        return parseIf();
    case KW_REPEAT:
        return parseWhile();
    case KW_YIELD:
        return parseReturn();
    case KW_DISPLAY:
        return parseDisplay();
    case LBRACE:
        return parseBlock();
    default:
        break;
    }

    fail("Expected a declaration, assignment, control-flow statement, display, function call, yield, or block.");
    return 0;
}

StmtNode* Parser::parseVariableDeclaration() {
    Token declaration = advance();
    const bool isFixed = declaration.type == KW_FIXED;
    Token nameToken = expect(IDENTIFIER, "variable name");
    ScopedNode<IdentNode> name(new IdentNode(nameToken.line, nameToken.lexeme));
    expect(COLON, "':'");
    TypeKind type = parseType(false);

    const bool hasInitializer = match(OP_ASSIGN);
    ScopedNode<ExprNode> initializer;
    if (hasInitializer) {
        initializer.reset(parseExpression());
    }
    expect(SEMICOLON, "';'");

    ScopedNode<VarDeclNode> declarationNode(
        new VarDeclNode(declaration.line, isFixed, name.get(), type,
                        hasInitializer, initializer.get()));
    name.release();
    initializer.release();
    return declarationNode.release();
}

StmtNode* Parser::parseAssignment() {
    Token nameToken = expect(IDENTIFIER, "assignment target");
    ScopedNode<IdentNode> target(new IdentNode(nameToken.line, nameToken.lexeme));
    expect(OP_ASSIGN, "':='");
    ScopedNode<ExprNode> value(parseExpression());
    expect(SEMICOLON, "';'");

    ScopedNode<AssignNode> assignment(
        new AssignNode(nameToken.line, target.get(), value.get()));
    target.release();
    value.release();
    return assignment.release();
}

StmtNode* Parser::parseIf() {
    Token whenToken = expect(KW_WHEN, "'when'");
    ScopedNode<ExprNode> condition(parseCondition());
    ScopedNode<BlockNode> thenBlock(parseBlock());

    const bool hasElseBlock = match(KW_OTHERWISE);
    ScopedNode<BlockNode> elseBlock;
    if (hasElseBlock) {
        elseBlock.reset(parseBlock());
    }

    ScopedNode<IfNode> ifNode(
        new IfNode(whenToken.line, condition.get(), thenBlock.get(),
                   hasElseBlock, elseBlock.get()));
    condition.release();
    thenBlock.release();
    elseBlock.release();
    return ifNode.release();
}

StmtNode* Parser::parseWhile() {
    Token repeatToken = expect(KW_REPEAT, "'repeat'");
    expect(KW_WHILE, "'while' after 'repeat'");
    ScopedNode<ExprNode> condition(parseCondition());
    ScopedNode<BlockNode> body(parseBlock());

    ScopedNode<WhileNode> whileNode(
        new WhileNode(repeatToken.line, condition.get(), body.get()));
    condition.release();
    body.release();
    return whileNode.release();
}

StmtNode* Parser::parseReturn() {
    Token yieldToken = expect(KW_YIELD, "'yield'");
    ScopedNode<ExprNode> value;
    if (peek().type != SEMICOLON) {
        value.reset(parseExpression());
    }
    expect(SEMICOLON, "';'");

    ScopedNode<ReturnNode> returnNode(new ReturnNode(yieldToken.line, value.get()));
    value.release();
    return returnNode.release();
}

StmtNode* Parser::parseDisplay() {
    Token displayToken = expect(KW_DISPLAY, "'display'");
    expect(LPAREN, "'('");
    ScopedNode<ExprNode> value(parseExpression());
    expect(RPAREN, "')'");
    expect(SEMICOLON, "';'");

    ScopedNode<DisplayNode> displayNode(new DisplayNode(displayToken.line, value.get()));
    value.release();
    return displayNode.release();
}

StmtNode* Parser::parseCallStatement() {
    ScopedNode<CallExprNode> call(parseCall());
    expect(SEMICOLON, "';'");

    ScopedNode<CallStmtNode> statement(
        new CallStmtNode(call.get()->line, call.get()));
    call.release();
    return statement.release();
}

ExprNode* Parser::parseCondition() {
    const bool parenthesized = match(LPAREN);
    ScopedNode<ExprNode> condition(parseExpression());
    if (parenthesized) {
        expect(RPAREN, "')'");
    }
    return condition.release();
}

TypeKind Parser::parseType(bool allowNothing) {
    Token typeToken = peek();
    switch (typeToken.type) {
    case TYPE_WHOLE:
        advance();
        return TypeKind::Whole;
    case TYPE_DECIMAL:
        advance();
        return TypeKind::Decimal;
    case TYPE_TEXT:
        advance();
        return TypeKind::Text;
    case TYPE_TRUTH:
        advance();
        return TypeKind::Truth;
    case TYPE_NOTHING:
        if (allowNothing) {
            advance();
            return TypeKind::Nothing;
        }
        break;
    default:
        break;
    }
    fail(allowNothing
        ? "Expected a return type (Whole, Decimal, Text, Truth, or Nothing)."
        : "Expected a parameter or variable type (Whole, Decimal, Text, or Truth).");
    return TypeKind::Nothing;
}

ExprNode* Parser::parseExpression() {
    return parseOr();
}

ExprNode* Parser::parseOr() {
    const TokenType operators[] = { OP_OR };
    return parseBinaryLevel(&Parser::parseAnd, operators, 1);
}

ExprNode* Parser::parseAnd() {
    const TokenType operators[] = { OP_AND };
    return parseBinaryLevel(&Parser::parseNot, operators, 1);
}

ExprNode* Parser::parseNot() {
    if (peek().type == OP_NOT) {
        Token operatorToken = advance();
        ScopedNode<ExprNode> operand(parseNot());
        ScopedNode<UnaryExprNode> unary(
            new UnaryExprNode(operatorToken.line, operatorToken.type, operand.get()));
        operand.release();
        return unary.release();
    }
    return parseEquality();
}

ExprNode* Parser::parseEquality() {
    const TokenType operators[] = { OP_IS, OP_ISNT };
    return parseBinaryLevel(&Parser::parseRelational, operators, 2);
}

ExprNode* Parser::parseRelational() {
    const TokenType operators[] = { OP_ABOVE, OP_BELOW, OP_ATLEAST, OP_ATMOST };
    return parseBinaryLevel(&Parser::parseAdditive, operators, 4);
}

ExprNode* Parser::parseAdditive() {
    const TokenType operators[] = { OP_PLUS, OP_MINUS };
    return parseBinaryLevel(&Parser::parseMultiplicative, operators, 2);
}

ExprNode* Parser::parseMultiplicative() {
    const TokenType operators[] = { OP_MULT, OP_DIV, OP_MOD };
    return parseBinaryLevel(&Parser::parseUnary, operators, 3);
}

ExprNode* Parser::parseUnary() {
    if (peek().type == OP_PLUS || peek().type == OP_MINUS) {
        Token operatorToken = advance();
        ScopedNode<ExprNode> operand(parseUnary());
        ScopedNode<UnaryExprNode> unary(
            new UnaryExprNode(operatorToken.line, operatorToken.type, operand.get()));
        operand.release();
        return unary.release();
    }
    return parsePrimary();
}

ExprNode* Parser::parsePrimary() {
    Token token = peek();
    switch (token.type) {
    case LIT_WHOLE:
        advance();
        return new LiteralNode(token.line, LiteralKind::Whole, token.lexeme);
    case LIT_DECIMAL:
        advance();
        return new LiteralNode(token.line, LiteralKind::Decimal, token.lexeme);
    case LIT_TEXT:
        advance();
        return new LiteralNode(token.line, LiteralKind::Text, token.lexeme);
    case LIT_YES:
    case LIT_NO:
        advance();
        return new LiteralNode(token.line, LiteralKind::Truth, token.lexeme);
    case IDENTIFIER: {
        if (peek(1).type == LPAREN) {
            return parseCall();
        }
        advance();
        return new IdentNode(token.line, token.lexeme);
    }
    case LPAREN: {
        advance();
        ScopedNode<ExprNode> expression(parseExpression());
        expect(RPAREN, "')'");
        return expression.release();
    }
    default:
        fail("Expected a literal, identifier, function call, or parenthesized expression.");
        return 0;
    }
}

CallExprNode* Parser::parseCall() {
    Token nameToken = expect(IDENTIFIER, "function name");
    ScopedNode<IdentNode> identifier(new IdentNode(nameToken.line, nameToken.lexeme));
    expect(LPAREN, "'('");

    OwnedPointerArray<ExprNode> arguments;
    if (peek().type != RPAREN) {
        do {
            ScopedNode<ExprNode> argument(parseExpression());
            arguments.pushBack(argument.get());
            argument.release();
        } while (match(COMMA));
    }
    expect(RPAREN, "')'");

    ScopedNode<CallExprNode> call(
        new CallExprNode(nameToken.line, identifier.get(), arguments.releaseArray()));
    identifier.release();
    return call.release();
}

ExprNode* Parser::parseBinaryLevel(ExpressionParser nextLevel,
                                   const TokenType operators[], int operatorCount) {
    ScopedNode<ExprNode> left((this->*nextLevel)());

    while (containsOperator(peek().type, operators, operatorCount)) {
        Token operatorToken = advance();
        ScopedNode<ExprNode> right((this->*nextLevel)());
        ScopedNode<BinaryExprNode> binary(
            new BinaryExprNode(operatorToken.line, operatorToken.type,
                               left.get(), right.get()));
        left.release();
        right.release();
        left.reset(binary.release());
    }
    return left.release();
}