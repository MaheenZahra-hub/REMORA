#ifndef AST_H
#define AST_H

#include "token.h"

#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

template <class T>
class DynamicArray {
private:
    T* items;
    std::size_t count;
    std::size_t capacity;

    void reserve(std::size_t minimumCapacity) {
        std::size_t newCapacity = capacity == 0 ? 4 : capacity;
        while (newCapacity < minimumCapacity) {
            if (newCapacity > std::numeric_limits<std::size_t>::max() / 2) {
                newCapacity = minimumCapacity;
                break;
            }
            newCapacity *= 2;
        }

        if (newCapacity > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::length_error("DynamicArray allocation size overflow.");
        }

        T* replacement = static_cast<T*>(::operator new(sizeof(T) * newCapacity));
        std::size_t constructed = 0;
        try {
            for (std::size_t i = 0; i < count; ++i) {
                new (&replacement[i]) T(items[i]);
                ++constructed;
            }
        } catch (...) {
            while (constructed > 0) {
                replacement[--constructed].~T();
            }
            ::operator delete(replacement);
            throw;
        }

        for (std::size_t i = 0; i < count; ++i) {
            items[i].~T();
        }
        ::operator delete(items);
        items = replacement;
        capacity = newCapacity;
    }

public:
    // Owns only the backing array; pointer elements remain owned by their AST parent.
    DynamicArray() : items(0), count(0), capacity(0) {}

    DynamicArray(DynamicArray&& other) noexcept
        : items(other.items), count(other.count), capacity(other.capacity) {
        other.items = 0;
        other.count = 0;
        other.capacity = 0;
    }

    ~DynamicArray() {
        for (std::size_t i = 0; i < count; ++i) {
            items[i].~T();
        }
        ::operator delete(items);
    }

    DynamicArray(const DynamicArray&) = delete;
    DynamicArray& operator=(const DynamicArray&) = delete;
    DynamicArray& operator=(DynamicArray&&) = delete;

    void pushBack(const T& value) {
        if (count == std::numeric_limits<std::size_t>::max()) {
            throw std::length_error("DynamicArray capacity overflow.");
        }

        T copy(value);
        if (count == capacity) {
            reserve(count + 1);
        }
        new (&items[count]) T(copy);
        ++count;
    }

    std::size_t size() const {
        return count;
    }

    bool empty() const {
        return count == 0;
    }

    T& operator[](std::size_t index) {
        if (index >= count) {
            throw std::out_of_range("DynamicArray index out of range.");
        }
        return items[index];
    }

    const T& operator[](std::size_t index) const {
        if (index >= count) {
            throw std::out_of_range("DynamicArray index out of range.");
        }
        return items[index];
    }
};

enum class TypeKind {
    Whole,
    Decimal,
    Text,
    Truth,
    Nothing
};

enum class LiteralKind {
    Whole,
    Decimal,
    Text,
    Truth
};

enum class FunctionKind {
    Launch,
    Craft
};

class Node {
public:
    int line;

    explicit Node(int sourceLine) : line(sourceLine) {}
    virtual ~Node() {}

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
};

class StmtNode : public Node {
public:
    explicit StmtNode(int sourceLine) : Node(sourceLine) {}
    virtual ~StmtNode() {}
};

class ExprNode : public Node {
public:
    explicit ExprNode(int sourceLine) : Node(sourceLine) {}
    virtual ~ExprNode() {}
};

class ProgramNode;
class FuncDeclNode;
class BlockNode;
class VarDeclNode;
class AssignNode;
class IfNode;
class WhileNode;
class ReturnNode;
class DisplayNode;
class CallStmtNode;
class BinaryExprNode;
class UnaryExprNode;
class CallExprNode;
class LiteralNode;
class IdentNode;

struct Parameter {
    int line;
    std::string name;
    TypeKind type;

    Parameter(int sourceLine, const std::string& parameterName, TypeKind parameterType)
        : line(sourceLine), name(parameterName), type(parameterType) {}
};

class ProgramNode : public Node {
public:
    DynamicArray<FuncDeclNode*> declarations;

    // Pointer-taking AST constructors transfer exclusive ownership to the new node.
    ProgramNode(int sourceLine, DynamicArray<FuncDeclNode*>&& functionDeclarations) noexcept
        : Node(sourceLine), declarations(std::move(functionDeclarations)) {}

    ~ProgramNode();
};

class FuncDeclNode : public Node {
public:
    FunctionKind kind;
    IdentNode* name;
    DynamicArray<Parameter> parameters;
    TypeKind returnType;
    BlockNode* body;

    FuncDeclNode(int sourceLine, FunctionKind functionKind, IdentNode* functionName,
                 DynamicArray<Parameter>&& functionParameters, TypeKind functionReturnType,
                 BlockNode* functionBody) noexcept
        : Node(sourceLine), kind(functionKind), name(functionName),
          parameters(std::move(functionParameters)), returnType(functionReturnType),
          body(functionBody) {}

    ~FuncDeclNode();
};

class BlockNode : public StmtNode {
public:
    DynamicArray<StmtNode*> statements;

    BlockNode(int sourceLine, DynamicArray<StmtNode*>&& blockStatements) noexcept
        : StmtNode(sourceLine), statements(std::move(blockStatements)) {}

    ~BlockNode();
};

class VarDeclNode : public StmtNode {
public:
    bool isFixed;
    IdentNode* name;
    TypeKind type;
    bool hasInitializer;
    ExprNode* initializer;

    VarDeclNode(int sourceLine, bool fixed, IdentNode* variableName, TypeKind variableType,
                bool initializerPresent, ExprNode* variableInitializer)
        : StmtNode(sourceLine), isFixed(fixed), name(variableName), type(variableType),
          hasInitializer(initializerPresent), initializer(variableInitializer) {
        if (hasInitializer != (initializer != 0)) {
            throw std::invalid_argument("Initializer presence must match the initializer pointer.");
        }
    }

    ~VarDeclNode();
};

class AssignNode : public StmtNode {
public:
    IdentNode* target;
    ExprNode* value;

    AssignNode(int sourceLine, IdentNode* assignmentTarget, ExprNode* assignedValue) noexcept
        : StmtNode(sourceLine), target(assignmentTarget), value(assignedValue) {}

    ~AssignNode();
};

class IfNode : public StmtNode {
public:
    ExprNode* condition;
    BlockNode* thenBlock;
    bool hasElseBlock;
    BlockNode* elseBlock;

    IfNode(int sourceLine, ExprNode* ifCondition, BlockNode* consequent,
           bool elsePresent, BlockNode* alternative)
        : StmtNode(sourceLine), condition(ifCondition), thenBlock(consequent),
          hasElseBlock(elsePresent), elseBlock(alternative) {
        if (hasElseBlock != (elseBlock != 0)) {
            throw std::invalid_argument("Else-block presence must match the else-block pointer.");
        }
    }

    ~IfNode();
};

class WhileNode : public StmtNode {
public:
    ExprNode* condition;
    BlockNode* body;

    WhileNode(int sourceLine, ExprNode* whileCondition, BlockNode* loopBody) noexcept
        : StmtNode(sourceLine), condition(whileCondition), body(loopBody) {}

    ~WhileNode();
};

class ReturnNode : public StmtNode {
public:
    ExprNode* value;

    ReturnNode(int sourceLine, ExprNode* returnValue) noexcept
        : StmtNode(sourceLine), value(returnValue) {}

    ~ReturnNode();
};

class DisplayNode : public StmtNode {
public:
    ExprNode* value;

    DisplayNode(int sourceLine, ExprNode* displayedValue) noexcept
        : StmtNode(sourceLine), value(displayedValue) {}

    ~DisplayNode();
};

class CallStmtNode : public StmtNode {
public:
    CallExprNode* call;

    CallStmtNode(int sourceLine, CallExprNode* functionCall) noexcept
        : StmtNode(sourceLine), call(functionCall) {}

    ~CallStmtNode();
};

class BinaryExprNode : public ExprNode {
public:
    TokenType op;
    ExprNode* left;
    ExprNode* right;

    BinaryExprNode(int sourceLine, TokenType binaryOperator, ExprNode* leftOperand,
                   ExprNode* rightOperand) noexcept
        : ExprNode(sourceLine), op(binaryOperator), left(leftOperand), right(rightOperand) {}

    ~BinaryExprNode();
};

class UnaryExprNode : public ExprNode {
public:
    TokenType op;
    ExprNode* operand;

    UnaryExprNode(int sourceLine, TokenType unaryOperator, ExprNode* expression) noexcept
        : ExprNode(sourceLine), op(unaryOperator), operand(expression) {}

    ~UnaryExprNode();
};

class CallExprNode : public ExprNode {
public:
    IdentNode* callee;
    DynamicArray<ExprNode*> arguments;

    CallExprNode(int sourceLine, IdentNode* functionName,
                 DynamicArray<ExprNode*>&& callArguments) noexcept
        : ExprNode(sourceLine), callee(functionName), arguments(std::move(callArguments)) {}

    ~CallExprNode();
};

class LiteralNode : public ExprNode {
public:
    LiteralKind kind;
    std::string lexeme;

    LiteralNode(int sourceLine, LiteralKind literalKind, const std::string& literalLexeme)
        : ExprNode(sourceLine), kind(literalKind), lexeme(literalLexeme) {}
};

class IdentNode : public ExprNode {
public:
    std::string name;

    IdentNode(int sourceLine, const std::string& identifierName)
        : ExprNode(sourceLine), name(identifierName) {}
};

inline ProgramNode::~ProgramNode() {
    for (std::size_t i = 0; i < declarations.size(); ++i) {
        delete declarations[i];
    }
}

inline FuncDeclNode::~FuncDeclNode() {
    delete name;
    delete body;
}

inline BlockNode::~BlockNode() {
    for (std::size_t i = 0; i < statements.size(); ++i) {
        delete statements[i];
    }
}

inline VarDeclNode::~VarDeclNode() {
    delete name;
    delete initializer;
}

inline AssignNode::~AssignNode() {
    delete target;
    delete value;
}

inline IfNode::~IfNode() {
    delete condition;
    delete thenBlock;
    delete elseBlock;
}

inline WhileNode::~WhileNode() {
    delete condition;
    delete body;
}

inline ReturnNode::~ReturnNode() {
    delete value;
}

inline DisplayNode::~DisplayNode() {
    delete value;
}

inline CallStmtNode::~CallStmtNode() {
    delete call;
}

inline BinaryExprNode::~BinaryExprNode() {
    delete left;
    delete right;
}

inline UnaryExprNode::~UnaryExprNode() {
    delete operand;
}

inline CallExprNode::~CallExprNode() {
    delete callee;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        delete arguments[i];
    }
}

#endif