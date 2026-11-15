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