#include "../include/lexer.h"

using namespace std;

Lexer::Lexer(string src, ErrorReporter& rep) : reporter(rep) {
    source = src;
    pos = 0;
    line = 1;
}

bool Lexer::isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

vector<Token> Lexer::tokenize() {
    vector<Token> tokens;
    int len = source.length();

    while (pos < len) {
        char c = source[pos];

        // 1. Whitespace & Newlines
        if (c == ' ' || c == '\t' || c == '\r') {
            pos++;
        }
        else if (c == '\n') {
            line++;
            pos++;
        }
        // 2. Single-line Comments (//)
        else if (c == '/' && pos + 1 < len && source[pos + 1] == '/') {
            while (pos < len && source[pos] != '\n') {
                pos++;
            }
        }
    }