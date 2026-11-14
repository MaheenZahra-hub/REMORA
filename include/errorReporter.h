#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include <string>
#include <vector>

struct CompilerError {
    std::string category; // "Lexical", "Syntax", or "Semantic"
    int line;
    std::string message;
};

#endif