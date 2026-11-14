#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum TokenType {
    // Keywords & Data Types
    KW_LAUNCH, KW_CRAFT, KW_GIVES, KW_FORM, KW_FIXED,
    KW_WHEN, KW_OTHERWISE, KW_REPEAT, KW_WHILE, KW_YIELD, KW_DISPLAY,
    TYPE_WHOLE, TYPE_DECIMAL, TYPE_TEXT, TYPE_TRUTH, TYPE_NOTHING,
    LIT_YES, LIT_NO,
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
};

#endif