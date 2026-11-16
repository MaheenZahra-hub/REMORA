#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "errorReporter.h"
#include <string>
#include <vector>

class Lexer {
private:
    std::string source;
    int pos;
    int line;
    ErrorReporter& reporter;

};

#endif