#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

// Declares the shared ErrorReporter class used across lexical, syntax, and
// semantic phases to collect and display formatted compiler errors.

#include <string>
#include <vector>

struct CompilerError {
    std::string category; // "Lexical", "Syntax", or "Semantic"
    int line;
    std::string message;
};

class ErrorReporter {
private:
    std::vector<CompilerError> errors;

public:
    void reportLexical(int line, std::string message);
    void reportSyntax(int line, std::string message);
    void reportSemantic(int line, std::string message);

    bool hasErrors();
    void printAll();
};

#endif