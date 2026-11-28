#include "semanticAnalyzer.h"

#include <sstream>

SemanticAnalyzer::SemanticAnalyzer(SymbolTable& table, ErrorReporter& errorReporter)
    : symbolTable(table),
      reporter(errorReporter),
      semanticErrorsFound(false),
      currentReturnType(TypeKind::Nothing) {
}

void SemanticAnalyzer::reportSemantic(int line, const std::string& message) {
    reporter.reportSemantic(line, message);
    semanticErrorsFound = true;
}

bool SemanticAnalyzer::isNumeric(TypeKind type) {
    return type == TypeKind::Whole || type == TypeKind::Decimal;
}

const char* SemanticAnalyzer::typeName(TypeKind type) {
    switch (type) {
        case TypeKind::Whole: return "Whole";
        case TypeKind::Decimal: return "Decimal";
        case TypeKind::Text: return "Text";
        case TypeKind::Truth: return "Truth";
        case TypeKind::Nothing: return "Nothing";
    }
    return "Unknown";
}

bool SemanticAnalyzer::analyze(const ProgramNode* program) {
    semanticErrorsFound = false;

    if (program == 0) {
        reportSemantic(0, "Cannot perform semantic analysis because the AST is null.");
        return false;
    }

    registerFunctionSignatures(program);

    for (std::size_t i = 0; i < program->declarations.size(); ++i) {
        analyzeFunction(program->declarations[i]);
    }

    return !semanticErrorsFound;
}

void SemanticAnalyzer::registerFunctionSignatures(const ProgramNode* program) {
    int launchCount = 0;
    bool validMainFound = false;

    // Pass 1: register every function signature before checking any function body.
    // This permits forward calls and recursive calls.
    for (std::size_t i = 0; i < program->declarations.size(); ++i) {
        const FuncDeclNode* function = program->declarations[i];
        if (function == 0 || function->name == 0) {
            reportSemantic(function == 0 ? program->line : function->line,
                           "Invalid function declaration in AST.");
            continue;
        }

        const std::string& functionName = function->name->name;

        if (function->kind == FunctionKind::Launch) {
            ++launchCount;

            if (functionName != "main") {
                reportSemantic(function->line,
                               "The launch function must be named 'main'.");
            }

            if (functionName == "main" &&
                function->parameters.empty() &&
                function->returnType == TypeKind::Nothing) {
                validMainFound = true;
            }

            if (!function->parameters.empty()) {
                reportSemantic(function->line,
                               "The launch main function must not have parameters.");
            }

            if (function->returnType != TypeKind::Nothing) {
                reportSemantic(function->line,
                               "The launch main function must return Nothing.");
            }
        }

        if (function->parameters.size() > static_cast<std::size_t>(MAX_PARAMETERS)) {
            reportSemantic(function->line,
                           "Function '" + functionName +
                           "' exceeds the maximum supported parameter count.");
            continue;
        }

        TypeKind parameterTypes[MAX_PARAMETERS];
        for (std::size_t p = 0; p < function->parameters.size(); ++p) {
            parameterTypes[p] = function->parameters[p].type;
        }

        if (!symbolTable.insertFunction(
                functionName,
                function->returnType,
                function->parameters.empty() ? 0 : parameterTypes,
                static_cast<int>(function->parameters.size()),
                function->line)) {
            reportSemantic(function->line,
                           "Duplicate function name or unable to register function '" +
                           functionName + "'.");
        }
    }

    if (launchCount == 0) {
        reportSemantic(program->line,
                       "Program must contain exactly one 'launch main() gives Nothing' function; none was found.");
    } else if (launchCount > 1) {
        reportSemantic(program->line,
                       "Program must contain exactly one launch function.");
    }

    if (!validMainFound) {
        reportSemantic(program->line,
                       "Program must define 'launch main() gives Nothing'.");
    }
}

void SemanticAnalyzer::analyzeFunction(const FuncDeclNode* function) {
    if (function == 0 || function->name == 0 || function->body == 0) {
        return;
    }

    currentReturnType = function->returnType;

    const std::string scopeName = "function:" + function->name->name;
    if (!symbolTable.enterScope(scopeName)) {
        reportSemantic(function->line,
                       "Unable to create scope for function '" +
                       function->name->name + "'.");
        return;
    }

    // Parameters and the outermost function body share one scope.
    for (std::size_t i = 0; i < function->parameters.size(); ++i) {
        const Parameter& parameter = function->parameters[i];
        if (!symbolTable.insertParameter(parameter.name, parameter.type, parameter.line)) {
            reportSemantic(parameter.line,
                           "Duplicate parameter name or name conflicts with an enclosing declaration: '" +
                           parameter.name + "'.");
        }
    }

    analyzeBlock(function->body, false);
    symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeBlock(const BlockNode* block, bool createScope) {
    if (block == 0) {
        return;
    }

    bool enteredScope = false;
    if (createScope) {
        std::ostringstream scopeName;
        scopeName << "block@" << block->line;
        enteredScope = symbolTable.enterScope(scopeName.str());
        if (!enteredScope) {
            reportSemantic(block->line, "Unable to create a nested block scope.");
        }
    }

    for (std::size_t i = 0; i < block->statements.size(); ++i) {
        analyzeStatement(block->statements[i]);
    }

    if (enteredScope) {
        symbolTable.exitScope();
    }
}

void SemanticAnalyzer::analyzeStatement(const StmtNode* statement) {
    if (statement == 0) {
        return;
    }

    const VarDeclNode* declaration = dynamic_cast<const VarDeclNode*>(statement);
    if (declaration != 0) {
        if (declaration->name == 0) {
            reportSemantic(declaration->line, "Variable declaration has no identifier.");
            return;
        }

        const std::string& name = declaration->name->name;

        if (symbolTable.lookupCurrentScope(name) != 0) {
            reportSemantic(declaration->line,
                           "Duplicate declaration of '" + name + "' in the same scope.");
            if (declaration->hasInitializer) {
                analyzeExpression(declaration->initializer);
            }
            return;
        }

        if (declaration->hasInitializer) {
            ExpressionResult initializer = analyzeExpression(declaration->initializer);
            if (initializer.valid && initializer.type != declaration->type) {
                reportSemantic(declaration->line,
                               "Initializer for '" + name + "' has type " +
                               typeName(initializer.type) + "; expected " +
                               typeName(declaration->type) + ". Implicit conversions are not allowed.");
            }
        }

        if (!symbolTable.insertVariable(name, declaration->type,
                                        declaration->line, declaration->isFixed)) {
            reportSemantic(declaration->line,
                           "Declaration of '" + name +
                           "' conflicts with an existing declaration or the symbol table is full.");
        }
        return;
    }

    const AssignNode* assignment = dynamic_cast<const AssignNode*>(statement);
    if (assignment != 0) {
        if (assignment->target == 0) {
            reportSemantic(assignment->line, "Assignment has no target identifier.");
            analyzeExpression(assignment->value);
            return;
        }

        const std::string& name = assignment->target->name;
        Symbol* target = symbolTable.lookup(name);
        ExpressionResult value = analyzeExpression(assignment->value);

        if (target == 0) {
            reportSemantic(assignment->line,
                           "Assignment to undeclared identifier '" + name + "'.");
            return;
        }

        if (target->kind == CONSTANT_SYMBOL) {
            reportSemantic(assignment->line,
                           "Cannot reassign fixed constant '" + name + "'.");
            return;
        }

        if (target->kind == FUNCTION_SYMBOL) {
            reportSemantic(assignment->line,
                           "Cannot assign a value to function '" + name + "'.");
            return;
        }

        if (value.valid && value.type != target->type) {
            reportSemantic(assignment->line,
                           "Assignment to '" + name + "' has type " +
                           typeName(value.type) + "; expected " +
                           typeName(target->type) + ".");
        }
        return;
    }

    const IfNode* ifStatement = dynamic_cast<const IfNode*>(statement);
    if (ifStatement != 0) {
        ExpressionResult condition = analyzeExpression(ifStatement->condition);
        if (condition.valid && condition.type != TypeKind::Truth) {
            reportSemantic(ifStatement->line,
                           "'when' condition must have type Truth, not " +
                           std::string(typeName(condition.type)) + ".");
        }
        analyzeBlock(ifStatement->thenBlock, true);
        if (ifStatement->hasElseBlock) {
            analyzeBlock(ifStatement->elseBlock, true);
        }
        return;
    }

    const WhileNode* whileStatement = dynamic_cast<const WhileNode*>(statement);
    if (whileStatement != 0) {
        ExpressionResult condition = analyzeExpression(whileStatement->condition);
        if (condition.valid && condition.type != TypeKind::Truth) {
            reportSemantic(whileStatement->line,
                           "'repeat while' condition must have type Truth, not " +
                           std::string(typeName(condition.type)) + ".");
        }
        analyzeBlock(whileStatement->body, true);
        return;
    }

    const ReturnNode* returnStatement = dynamic_cast<const ReturnNode*>(statement);
    if (returnStatement != 0) {
        if (currentReturnType == TypeKind::Nothing) {
            if (returnStatement->value != 0) {
                analyzeExpression(returnStatement->value);
                reportSemantic(returnStatement->line,
                               "A function returning Nothing must use 'yield;' without a value.");
            }
        } else {
            if (returnStatement->value == 0) {
                reportSemantic(returnStatement->line,
                               "A function returning " +
                               std::string(typeName(currentReturnType)) +
                               " must yield a value.");
            } else {
                ExpressionResult value = analyzeExpression(returnStatement->value);
                if (value.valid && value.type != currentReturnType) {
                    reportSemantic(returnStatement->line,
                                   "'yield' has type " + std::string(typeName(value.type)) +
                                   "; function return type is " +
                                   typeName(currentReturnType) + ".");
                }
            }
        }
        return;
    }

    const DisplayNode* displayStatement = dynamic_cast<const DisplayNode*>(statement);
    if (displayStatement != 0) {
        analyzeExpression(displayStatement->value);
        return;
    }

    const CallStmtNode* callStatement = dynamic_cast<const CallStmtNode*>(statement);
    if (callStatement != 0) {
        analyzeCall(callStatement->call);
        return;
    }

    const BlockNode* nestedBlock = dynamic_cast<const BlockNode*>(statement);
    if (nestedBlock != 0) {
        analyzeBlock(nestedBlock, true);
        return;
    }

    reportSemantic(statement->line, "Unsupported statement node encountered during semantic analysis.");
}

SemanticAnalyzer::ExpressionResult
SemanticAnalyzer::analyzeExpression(const ExprNode* expression) {
    if (expression == 0) {
        return ExpressionResult(TypeKind::Nothing, false);
    }

    const LiteralNode* literal = dynamic_cast<const LiteralNode*>(expression);
    if (literal != 0) {
        switch (literal->kind) {
            case LiteralKind::Whole:
                return ExpressionResult(TypeKind::Whole, true);
            case LiteralKind::Decimal:
                return ExpressionResult(TypeKind::Decimal, true);
            case LiteralKind::Text:
                return ExpressionResult(TypeKind::Text, true);
            case LiteralKind::Truth:
                return ExpressionResult(TypeKind::Truth, true);
        }
    }

    const IdentNode* identifier = dynamic_cast<const IdentNode*>(expression);
    if (identifier != 0) {
        Symbol* symbol = symbolTable.lookup(identifier->name);
        if (symbol == 0) {
            reportSemantic(identifier->line,
                           "Use of undeclared identifier '" + identifier->name + "'.");
            return ExpressionResult(TypeKind::Nothing, false);
        }
        if (symbol->kind == FUNCTION_SYMBOL) {
            reportSemantic(identifier->line,
                           "Function '" + identifier->name + "' must be called with parentheses.");
            return ExpressionResult(TypeKind::Nothing, false);
        }
        return ExpressionResult(symbol->type, true);
    }

    const CallExprNode* call = dynamic_cast<const CallExprNode*>(expression);
    if (call != 0) {
        return analyzeCall(call);
    }

    const UnaryExprNode* unary = dynamic_cast<const UnaryExprNode*>(expression);
    if (unary != 0) {
        ExpressionResult operand = analyzeExpression(unary->operand);
        if (!operand.valid) {
            return ExpressionResult(TypeKind::Nothing, false);
        }

        if (unary->op == OP_NOT) {
            if (operand.type != TypeKind::Truth) {
                reportSemantic(unary->line,
                               "Operator 'not' requires a Truth operand.");
                return ExpressionResult(TypeKind::Nothing, false);
            }
            return ExpressionResult(TypeKind::Truth, true);
        }

        if (unary->op == OP_PLUS || unary->op == OP_MINUS) {
            if (!isNumeric(operand.type)) {
                reportSemantic(unary->line,
                               "Unary '+' and '-' require a Whole or Decimal operand.");
                return ExpressionResult(TypeKind::Nothing, false);
            }
            return ExpressionResult(operand.type, true);
        }

        reportSemantic(unary->line, "Unknown unary operator.");
        return ExpressionResult(TypeKind::Nothing, false);
    }

    const BinaryExprNode* binary = dynamic_cast<const BinaryExprNode*>(expression);
    if (binary != 0) {
        ExpressionResult left = analyzeExpression(binary->left);
        ExpressionResult right = analyzeExpression(binary->right);

        if (!left.valid || !right.valid) {
            return ExpressionResult(TypeKind::Nothing, false);
        }

        switch (binary->op) {
            case OP_PLUS:
            case OP_MINUS:
            case OP_MULT:
            case OP_DIV:
                if (left.type != right.type || !isNumeric(left.type)) {
                    reportSemantic(binary->line,
                                   "Arithmetic operators '+', '-', '*', and '/' require matching Whole or matching Decimal operands.");
                    return ExpressionResult(TypeKind::Nothing, false);
                }
                return ExpressionResult(left.type, true);

            case OP_MOD:
                if (left.type != TypeKind::Whole || right.type != TypeKind::Whole) {
                    reportSemantic(binary->line,
                                   "Operator '%' requires two Whole operands.");
                    return ExpressionResult(TypeKind::Nothing, false);
                }
                return ExpressionResult(TypeKind::Whole, true);

            case OP_ABOVE:
            case OP_BELOW:
            case OP_ATLEAST:
            case OP_ATMOST:
                if (left.type != right.type || !isNumeric(left.type)) {
                    reportSemantic(binary->line,
                                   "Relational operators require matching Whole or matching Decimal operands.");
                    return ExpressionResult(TypeKind::Nothing, false);
                }
                return ExpressionResult(TypeKind::Truth, true);

            case OP_IS:
            case OP_ISNT:
                if (left.type != right.type) {
                    reportSemantic(binary->line,
                                   "Operators 'is' and 'isnt' require operands of matching types.");
                    return ExpressionResult(TypeKind::Nothing, false);
                }
                return ExpressionResult(TypeKind::Truth, true);

            case OP_AND:
            case OP_OR:
                if (left.type != TypeKind::Truth || right.type != TypeKind::Truth) {
                    reportSemantic(binary->line,
                                   "Logical operators 'and' and 'or' require two Truth operands.");
                    return ExpressionResult(TypeKind::Nothing, false);
                }
                return ExpressionResult(TypeKind::Truth, true);

            default:
                reportSemantic(binary->line, "Unknown binary operator.");
                return ExpressionResult(TypeKind::Nothing, false);
        }
    }

    reportSemantic(expression->line, "Unsupported expression node encountered during semantic analysis.");
    return ExpressionResult(TypeKind::Nothing, false);
}

SemanticAnalyzer::ExpressionResult
SemanticAnalyzer::analyzeCall(const CallExprNode* call) {
    if (call == 0 || call->callee == 0) {
        reportSemantic(call == 0 ? 0 : call->line,
                       "Invalid function call in AST.");
        return ExpressionResult(TypeKind::Nothing, false);
    }

    const std::string& name = call->callee->name;
    Symbol* function = symbolTable.lookup(name);

    // Analyze every argument even when the callee is invalid, to report additional errors.
    ExpressionResult argumentResults[MAX_PARAMETERS];
    bool allArgumentsValid = true;
    for (std::size_t i = 0; i < call->arguments.size(); ++i) {
        ExpressionResult argument = analyzeExpression(call->arguments[i]);
        if (i < static_cast<std::size_t>(MAX_PARAMETERS)) {
            argumentResults[i] = argument;
        } else {
            allArgumentsValid = false;
        }
    }

    if (function == 0) {
        reportSemantic(call->line, "Call to undeclared function '" + name + "'.");
        return ExpressionResult(TypeKind::Nothing, false);
    }

    if (function->kind != FUNCTION_SYMBOL) {
        reportSemantic(call->line, "'" + name + "' is not a function and cannot be called.");
        return ExpressionResult(TypeKind::Nothing, false);
    }

    if (call->arguments.size() != static_cast<std::size_t>(function->parameterCount)) {
        std::ostringstream message;
        message << "Function '" << name << "' expects "
                << function->parameterCount << " argument(s), but "
                << call->arguments.size() << " were provided.";
        reportSemantic(call->line, message.str());
    }

    const std::size_t checkedCount =
        call->arguments.size() < static_cast<std::size_t>(function->parameterCount)
            ? call->arguments.size()
            : static_cast<std::size_t>(function->parameterCount);

    for (std::size_t i = 0; i < checkedCount; ++i) {
        if (i < static_cast<std::size_t>(MAX_PARAMETERS) &&
            argumentResults[i].valid &&
            argumentResults[i].type != function->parameterTypes[i]) {
            std::ostringstream message;
            message << "Argument " << (i + 1) << " of function '" << name
                    << "' has type " << typeName(argumentResults[i].type)
                    << "; expected " << typeName(function->parameterTypes[i]) << ".";
            reportSemantic(call->arguments[i]->line, message.str());
        }
    }

    if (!allArgumentsValid) {
        return ExpressionResult(function->type, false);
    }

    return ExpressionResult(function->type, true);
}
