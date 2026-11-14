#ifndef TOKEN_H
#define TOKEN_H

// Description:-
// Defines all token types (keywords, data types, operators, punctuation, literals)
// and the Token struct used to pass scanned tokens to the parser.

#include <string>

enum TokenType {
    // Keywords & Data Types
    KW_LAUNCH, KW_CRAFT, KW_GIVES, KW_FORM, KW_FIXED,
    KW_WHEN, KW_OTHERWISE, KW_REPEAT, KW_WHILE, KW_YIELD, KW_DISPLAY,
    TYPE_WHOLE, TYPE_DECIMAL, TYPE_TEXT, TYPE_TRUTH, TYPE_NOTHING,
    LIT_YES, LIT_NO,

    // Word Operators
    OP_IS, OP_ISNT, OP_ABOVE, OP_BELOW, OP_ATLEAST, OP_ATMOST,
    OP_AND, OP_OR, OP_NOT,

    // Symbol Operators
    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD, OP_ASSIGN,

    // Punctuation
    LPAREN, RPAREN, LBRACE, RBRACE, COLON, SEMICOLON, COMMA,

    // Identifiers & Literals
    IDENTIFIER,
    LIT_WHOLE,
    LIT_DECIMAL,
    LIT_TEXT,

    // End of File
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
};

#endif