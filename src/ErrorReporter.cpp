#include "../include/errorReporter.h"
#include <iostream>

using namespace std;

// Constructor initializes error count to 0
ErrorReporter::ErrorReporter() {
    errorCount = 0;
}

// Adds a lexical error to the array if space is available
void ErrorReporter::reportLexical(int line, string message) {
    if (errorCount < MAX_ERRORS) {
        errors[errorCount].category = "Lexical";
        errors[errorCount].line = line;
        errors[errorCount].message = message;
        errorCount++;
    }
}

// Adds a syntax error to the array
void ErrorReporter::reportSyntax(int line, string message) {
    if (errorCount < MAX_ERRORS) {
        errors[errorCount].category = "Syntax";
        errors[errorCount].line = line;
        errors[errorCount].message = message;
        errorCount++;
    }
}

// Adds a semantic error to the array
void ErrorReporter::reportSemantic(int line, string message) {
    if (errorCount < MAX_ERRORS) {
        errors[errorCount].category = "Semantic";
        errors[errorCount].line = line;
        errors[errorCount].message = message;
        errorCount++;
    }
}

// Returns true if at least one error was recorded
bool ErrorReporter::hasErrors() {
    return errorCount > 0;
}

// Prints every recorded error in the format: [Category Error] Line X: message
void ErrorReporter::printAll() {
    for (int i = 0; i < errorCount; i++) {
        cout << "[" << errors[i].category << " Error] Line "
             << errors[i].line << ": " << errors[i].message << endl;
    }
}