#include "../include/lexer.h"

using namespace std;

// Constructor sets initial position to 0 and line counter to 1
Lexer::Lexer(string src, ErrorReporter& rep) : reporter(rep) {
    source = src;
    pos = 0;
    line = 1;
}

// Checks if character is a letter (a-z, A-Z) or underscore (_)
bool Lexer::isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Checks if character is a digit (0-9)
bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

// Matches a word against remora's reserved keywords, types, and word operators
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

// Reads the entire source string and stores recognized tokens in tokens[]
void Lexer::tokenize(Token tokens[], int& tokenCount) {
    tokenCount = 0;
    int len = source.length();

    while (pos < len && tokenCount < MAX_TOKENS - 1) {
        char c = source[pos];

        // 1. Skip spaces, tabs, and carriage returns
        if (c == ' ' || c == '\t' || c == '\r') {
            pos++;
        }
        // 2. Increment line counter on newline
        else if (c == '\n') {
            line++;
            pos++;
        }
        // 3. Skip single-line comments starting with //
        else if (c == '/' && pos + 1 < len && source[pos + 1] == '/') {
            while (pos < len && source[pos] != '\n') {
                pos++;
            }
        }
        // 4. Scan identifiers and keywords
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

            tokens[tokenCount] = {type, word, line};
            tokenCount++;
        }
        // 5. Scan Whole and Decimal numbers
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

            // Flag trailing dot (12.), multiple dots (12.3.4), or letters (12abc)
            if (dotCount > 1 || num[num.length() - 1] == '.') {
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
            tokens[tokenCount] = {type, num, line};
            tokenCount++;
        }
        // 6. Scan Text literals ("...")
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

            tokens[tokenCount] = {LIT_TEXT, str, startLine};
            tokenCount++;
        }
        // 7. Scan assignment (:=) or colon (:)
        else if (c == ':') {
            if (pos + 1 < len && source[pos + 1] == '=') {
                tokens[tokenCount] = {OP_ASSIGN, ":=", line};
                tokenCount++;
                pos += 2;
            } else {
                tokens[tokenCount] = {COLON, ":", line};
                tokenCount++;
                pos++;
            }
        }
        // 8. Scan single-character operators and punctuation
        else if (c == '+') { tokens[tokenCount++] = {OP_PLUS, "+", line}; pos++; }
        else if (c == '-') { tokens[tokenCount++] = {OP_MINUS, "-", line}; pos++; }
        else if (c == '*') { tokens[tokenCount++] = {OP_MULT, "*", line}; pos++; }
        else if (c == '/') { tokens[tokenCount++] = {OP_DIV, "/", line}; pos++; }
        else if (c == '%') { tokens[tokenCount++] = {OP_MOD, "%", line}; pos++; }
        else if (c == '(') { tokens[tokenCount++] = {LPAREN, "(", line}; pos++; }
        else if (c == ')') { tokens[tokenCount++] = {RPAREN, ")", line}; pos++; }
        else if (c == '{') { tokens[tokenCount++] = {LBRACE, "{", line}; pos++; }
        else if (c == '}') { tokens[tokenCount++] = {RBRACE, "}", line}; pos++; }
        else if (c == ';') { tokens[tokenCount++] = {SEMICOLON, ";", line}; pos++; }
        else if (c == ',') { tokens[tokenCount++] = {COMMA, ",", line}; pos++; }
        // 9. Report any unknown symbol
        else {
            string badChar = "";
            badChar += c;
            reporter.reportLexical(line, "Unknown character '" + badChar + "'.");
            pos++;
        }
    }

    // Add END_OF_FILE token at the end
    tokens[tokenCount] = {END_OF_FILE, "EOF", line};
    tokenCount++;
}