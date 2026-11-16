#include "../include/errorReporter.h"
#include <iostream>

using namespace std;

void ErrorReporter::reportLexical(int line, string message) {
    errors.push_back({"Lexical", line, message});
}

void ErrorReporter::reportSyntax(int line, string message) {
    errors.push_back({"Syntax", line, message});
}

void ErrorReporter::reportSemantic(int line, string message) {
    errors.push_back({"Semantic", line, message});
}

bool ErrorReporter::hasErrors() {
    return !errors.empty();
}

void ErrorReporter::printAll() {
    for (int i = 0; i < errors.size(); i++) {
        cout << "[" << errors[i].category << " Error] Line "
             << errors[i].line << ": " << errors[i].message << endl;
    }
}