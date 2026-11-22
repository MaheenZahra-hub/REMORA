
#include "symbolTable.h"
#include <iostream>

using namespace std;

// Constructor
SymbolTable::SymbolTable() {
    symbolCount = 0;
    scopeCount = 0;
    currentScopeID = 0;

    // Create Global Scope
    scopes[0].scopeID = 0;
    scopes[0].parentID = -1;
    scopes[0].name = "Global";
    scopes[0].isActive = true;

    scopeCount = 1;

    registerBuiltins();
}

// Find scope using its ID
int SymbolTable::findScope(int scopeID) {
    for (int i = 0; i < scopeCount; i++) {
        if (scopes[i].scopeID == scopeID) {
            return i;
        }
    }

    return -1;
}

// Find a symbol in one particular scope
int SymbolTable::findSymbolInScope(const string& name, int scopeID) {
    for (int i = 0; i < symbolCount; i++) {
        if (symbols[i].name == name &&
            symbols[i].scopeID == scopeID) {
            return i;
        }
    }

    return -1;
}

// Check current scope and all parent scopes
bool SymbolTable::nameExistsInParents(const string& name) {
    int scopeID = currentScopeID;

    while (scopeID != -1) {
        int index = findSymbolInScope(name, scopeID);

        if (index != -1) {
            return true;
        }

        int scopeIndex = findScope(scopeID);

        if (scopeIndex == -1) {
            break;
        }

        scopeID = scopes[scopeIndex].parentID;
    }

    return false;
}

// Enter a new nested scope
bool SymbolTable::enterScope(const string& scopeName) {
    if (scopeCount >= MAX_SCOPES) {
        cout << "Symbol Table Error: Maximum scopes reached.\n";
        return false;
    }

    int newID = scopeCount;

    scopes[scopeCount].scopeID = newID;
    scopes[scopeCount].parentID = currentScopeID;
    scopes[scopeCount].name = scopeName;
    scopes[scopeCount].isActive = true;

    scopeCount++;

    currentScopeID = newID;

    return true;
}

// Exit current scope
void SymbolTable::exitScope() {
    if (currentScopeID == 0) {
        return;
    }

    int index = findScope(currentScopeID);

    if (index != -1) {
        scopes[index].isActive = false;
        currentScopeID = scopes[index].parentID;
    }
}

// Insert variable or constant
bool SymbolTable::insertVariable(
    const string& name,
    TypeKind type,
    int line,
    bool isFixed
) {
    if (symbolCount >= MAX_SYMBOLS) {
        cout << "Symbol Table Error: Maximum symbols reached.\n";
        return false;
    }

    if (nameExistsInParents(name)) {
        return false;
    }

    Symbol& s = symbols[symbolCount];

    s.name = name;
    s.kind = isFixed ? CONSTANT_SYMBOL : VARIABLE_SYMBOL;
    s.type = type;
    s.line = line;
    s.scopeID = currentScopeID;
    s.parameterCount = 0;
    s.isActive = true;

    symbolCount++;

    return true;
}

// Insert function parameter
bool SymbolTable::insertParameter(
    const string& name,
    TypeKind type,
    int line
) {
    if (symbolCount >= MAX_SYMBOLS) {
        cout << "Symbol Table Error: Maximum symbols reached.\n";
        return false;
    }

    if (nameExistsInParents(name)) {
        return false;
    }

    Symbol& s = symbols[symbolCount];

    s.name = name;
    s.kind = PARAMETER_SYMBOL;
    s.type = type;
    s.line = line;
    s.scopeID = currentScopeID;
    s.parameterCount = 0;
    s.isActive = true;

    symbolCount++;

    return true;
}

// Insert function
bool SymbolTable::insertFunction(
    const string& name,
    TypeKind returnType,
    const TypeKind parameterTypes[],
    int parameterCount,
    int line
) {
    if (symbolCount >= MAX_SYMBOLS ||
        parameterCount < 0 ||
        parameterCount > MAX_PARAMETERS) {
        return false;
    }

    // Functions are declared in Global Scope
    if (currentScopeID != 0 ||
        findSymbolInScope(name, 0) != -1) {
        return false;
    }

    Symbol& s = symbols[symbolCount];

    s.name = name;
    s.kind = FUNCTION_SYMBOL;
    s.type = returnType;
    s.line = line;
    s.scopeID = 0;
    s.parameterCount = parameterCount;
    s.isActive = true;

    for (int i = 0; i < parameterCount; i++) {
        s.parameterTypes[i] = parameterTypes[i];
    }

    symbolCount++;

    return true;
}

// Lookup symbol in current scope and parent scopes
Symbol* SymbolTable::lookup(const string& name) {
    int scopeID = currentScopeID;

    while (scopeID != -1) {
        int index = findSymbolInScope(name, scopeID);

        if (index != -1) {
            return &symbols[index];
        }

        int scopeIndex = findScope(scopeID);

        if (scopeIndex == -1) {
            break;
        }

        scopeID = scopes[scopeIndex].parentID;
    }

    return 0;
}

// Lookup only in current scope
Symbol* SymbolTable::lookupCurrentScope(const string& name) {
    int index = findSymbolInScope(name, currentScopeID);

    if (index == -1) {
        return 0;
    }

    return &symbols[index];
}

// Register built-in functions
void SymbolTable::registerBuiltins() {
    insertFunction("captureWhole", TypeKind::Whole, 0, 0, 0);
    insertFunction("captureDecimal", TypeKind::Decimal, 0, 0, 0);
    insertFunction("captureText", TypeKind::Text, 0, 0, 0);
}

// Print type name
static string getTypeName(TypeKind type) {
    switch (type) {
        case TypeKind::Whole: return "Whole";
        case TypeKind::Decimal: return "Decimal";
        case TypeKind::Text: return "Text";
        case TypeKind::Truth: return "Truth";
        case TypeKind::Nothing: return "Nothing";
    }

    return "Unknown";
}

// Print symbol kind
static string getKindName(SymbolKind kind) {
    switch (kind) {
        case VARIABLE_SYMBOL: return "Variable";
        case CONSTANT_SYMBOL: return "Constant";
        case FUNCTION_SYMBOL: return "Function";
        case PARAMETER_SYMBOL: return "Parameter";
    }

    return "Unknown";
}

// Display complete symbol history
void SymbolTable::printSymbolTable() const {
    cout << "\n========== SYMBOL TABLE ==========\n";

    cout << "Name\t\tKind\t\tType\t\tScope\t\tLine\n";
    cout << "-------------------------------------------------------------\n";

    for (int i = 0; i < symbolCount; i++) {
        const Symbol& s = symbols[i];

        int scopeIndex = -1;

        for (int j = 0; j < scopeCount; j++) {
            if (scopes[j].scopeID == s.scopeID) {
                scopeIndex = j;
                break;
            }
        }

        cout << s.name << "\t\t"
             << getKindName(s.kind) << "\t\t"
             << getTypeName(s.type) << "\t\t";

        if (scopeIndex != -1) {
            cout << scopes[scopeIndex].name;
        }

        cout << "\t\t" << s.line << "\n";
    }

    cout << "==================================\n";
}

int SymbolTable::getCurrentScopeID() const {
    return currentScopeID;
}

int SymbolTable::getSymbolCount() const {
    return symbolCount;
}

int SymbolTable::getScopeCount() const {
    return scopeCount;
}

const Symbol* SymbolTable::getSymbolAt(int index) const {
    if (index < 0 || index >= symbolCount) {
        return 0;
    }

    return &symbols[index];
}

const Scope* SymbolTable::getScopeAt(int index) const {
    if (index < 0 || index >= scopeCount) {
        return 0;
    }

    return &scopes[index];
}
