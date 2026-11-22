
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "AST.h"
#include <string>

const int MAX_SYMBOLS = 500;
const int MAX_SCOPES = 100;
const int MAX_PARAMETERS = 50;

enum SymbolKind {
    VARIABLE_SYMBOL,
    CONSTANT_SYMBOL,
    FUNCTION_SYMBOL,
    PARAMETER_SYMBOL
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    TypeKind type;
    int line;
    int scopeID;

    int parameterCount;
    TypeKind parameterTypes[MAX_PARAMETERS];

    bool isActive;
};

struct Scope {
    int scopeID;
    int parentID;
    std::string name;
    bool isActive;
};

class SymbolTable {
private:
    Symbol symbols[MAX_SYMBOLS];
    Scope scopes[MAX_SCOPES];

    int symbolCount;
    int scopeCount;
    int currentScopeID;

    int findScope(int scopeID);
    int findSymbolInScope(const std::string& name, int scopeID);
    bool nameExistsInParents(const std::string& name);

public:
    SymbolTable();

    // Scope management
    bool enterScope(const std::string& scopeName);
    void exitScope();

    // Symbol insertion
    bool insertVariable(const std::string& name, TypeKind type,
                        int line, bool isFixed = false);

    bool insertParameter(const std::string& name, TypeKind type,
                         int line);

    bool insertFunction(const std::string& name, TypeKind returnType,
                        const TypeKind parameterTypes[],
                        int parameterCount, int line);

    // Symbol lookup
    Symbol* lookup(const std::string& name);
    Symbol* lookupCurrentScope(const std::string& name);

    // Built-in functions
    void registerBuiltins();

    // Utility
    void printSymbolTable() const;

    int getCurrentScopeID() const;
    int getSymbolCount() const;
    int getScopeCount() const;

    const Symbol* getSymbolAt(int index) const;
    const Scope* getScopeAt(int index) const;
};

#endif
