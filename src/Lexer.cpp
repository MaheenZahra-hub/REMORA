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

TokenType Lexer::checkKeyword(string word) {
    if (word == "launch")    return KW_LAUNCH;
    if (word == "craft")     return KW_CRAFT;
    if (word == "gives")     return KW_GIVES;
    if (word == "form")      return KW_FORM;
    if (word == "fixed")     return KW_FIXED;
    if (word == "when")      return KW_WHEN;
    if (word == "otherwise") return KW_OTHERWISE;
    if (word == "repeat")    return KW_REPEAT;
    if (word == "while")     return KW_WHILE;
    if (word == "yield")     return KW_YIELD;
    if (word == "display")   return KW_DISPLAY;

    if (word == "Whole")     return TYPE_WHOLE;
    if (word == "Decimal")   return TYPE_DECIMAL;
    if (word == "Text")      return TYPE_TEXT;
    if (word == "Truth")     return TYPE_TRUTH;
    if (word == "Nothing")   return TYPE_NOTHING;

    if (word == "yes")       return LIT_YES;
    if (word == "no")        return LIT_NO;

    if (word == "is")        return OP_IS;
    if (word == "isnt")      return OP_ISNT;
    if (word == "above")     return OP_ABOVE;
    if (word == "below")     return OP_BELOW;
    if (word == "atleast")   return OP_ATLEAST;
    if (word == "atmost")    return OP_ATMOST;
    if (word == "and")       return OP_AND;
    if (word == "or")        return OP_OR;
    if (word == "not")       return OP_NOT;

    return IDENTIFIER;
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
        // 3. Identifiers & Keywords
        else if (isLetter(c)) {
            string word = "";
            while (pos < len && (isLetter(source[pos]) || isDigit(source[pos]))) {
                word += source[pos];
                pos++;
            }

            TokenType type = checkKeyword(word);
            if (type == IDENTIFIER && word.length() > 20) {
                reporter.reportLexical(line, "Identifier '" + word + "' exceeds 20 characters.");
            }
            tokens.push_back({type, word, line});
        }
        // 4. Numbers (Whole & Decimal)
        else if (isDigit(c)) {
            string num = "";
            int dotCount = 0;
            bool badNumber = false;

            while (pos < len && (isDigit(source[pos]) || source[pos] == '.')) {
                if (source[pos] == '.') {
                    dotCount++;
                }
                num += source[pos];
                pos++;
            }

            // Check trailing dot (12.), multiple dots (12.3.4), or letters attached (12abc)
            if (dotCount > 1 || num.back() == '.') {
                badNumber = true;
            }
            while (pos < len && isLetter(source[pos])) {
                badNumber = true;
                num += source[pos];
                pos++;
            }

            if (badNumber) {
                reporter.reportLexical(line, "Malformed number '" + num + "'.");
            }

            TokenType type = (dotCount == 1) ? LIT_DECIMAL : LIT_WHOLE;
            tokens.push_back({type, num, line});
        }
        // 5. Text Literals ("...")
        else if (c == '"') {
            int startLine = line;
            string str = "";
            str += source[pos++]; // opening quote

            while (pos < len && source[pos] != '"' && source[pos] != '\n') {
                str += source[pos];
                pos++;
            }

            if (pos >= len || source[pos] == '\n') {
                reporter.reportLexical(startLine, "Unclosed string literal.");
            } else {
                str += source[pos++]; // closing quote
            }
            tokens.push_back({LIT_TEXT, str, startLine});
        }
        // 6. Assignment (:=) or Colon (:)
        else if (c == ':') {
            if (pos + 1 < len && source[pos + 1] == '=') {
                tokens.push_back({OP_ASSIGN, ":=", line});
                pos += 2;
            } else {
                tokens.push_back({COLON, ":", line});
                pos++;
            }
        }
        // 7. Single-Character Operators & Punctuation
        else if (c == '+') { tokens.push_back({OP_PLUS, "+", line}); pos++; }
        else if (c == '-') { tokens.push_back({OP_MINUS, "-", line}); pos++; }
        else if (c == '*') { tokens.push_back({OP_MULT, "*", line}); pos++; }
        else if (c == '/') { tokens.push_back({OP_DIV, "/", line}); pos++; }
        else if (c == '%') { tokens.push_back({OP_MOD, "%", line}); pos++; }
        else if (c == '(') { tokens.push_back({LPAREN, "(", line}); pos++; }
        else if (c == ')') { tokens.push_back({RPAREN, ")", line}); pos++; }
        else if (c == '{') { tokens.push_back({LBRACE, "{", line}); pos++; }
        else if (c == '}') { tokens.push_back({RBRACE, "}", line}); pos++; }
        else if (c == ';') { tokens.push_back({SEMICOLON, ";", line}); pos++; }
        else if (c == ',') { tokens.push_back({COMMA, ",", line}); pos++; }
        // 8. Unknown Character
        else {
            string badChar(1, c);
            reporter.reportLexical(line, "Unknown character '" + badChar + "'.");
            pos++;
        }
    }

    tokens.push_back({END_OF_FILE, "EOF", line});
    return tokens;
}