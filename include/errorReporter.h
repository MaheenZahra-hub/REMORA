#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include <string>

// Stores and prints compiler errors using a simple fixed-size array.

const int MAX_ERRORS = 500;

struct CompilerError {
    std::string category; // "Lexical", "Syntax", or "Semantic"
    int line;
    std::string message;
};

class ErrorReporter {
private:
    CompilerError errors[MAX_ERRORS]; // Array to store all errors
    int errorCount;                   // Tracks how many errors have been added

public:
    ErrorReporter();

    void reportLexical(int line, std::string message);
    void reportSyntax(int line, std::string message);
    void reportSemantic(int line, std::string message);

    bool hasErrors();
    void printAll();
};

#endif