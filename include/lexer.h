#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "errorReporter.h"
#include <string>
#include <vector>

// Declares the Lexer class responsible for scanning .remora source code
// character-by-character into a list of tokens.

class Lexer {
private:
    std::string source;
    int pos;
    int line;
    ErrorReporter& reporter;

};

#endif