#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "errorReporter.h"
#include <string>

// Declares the Lexer class that scans .remora source text into a Token array.

const int MAX_TOKENS = 2000;

class Lexer {
private:
    std::string source;
    int pos;
    int line;
    ErrorReporter& reporter;

    bool isLetter(char c);
    bool isDigit(char c);
    TokenType checkKeyword(std::string word);

public:
    Lexer(std::string src, ErrorReporter& rep);

    // Scans source code into tokens[] and sets tokenCount to total tokens found
    void tokenize(Token tokens[], int& tokenCount);
};

#endif