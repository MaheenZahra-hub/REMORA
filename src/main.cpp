#include "../include/lexer.h"
#include "../include/errorReporter.h"
#include <iostream>
#include <fstream>

// Entry point: reads a .remora file character-by-character, runs the lexer,
// and prints either the token list or any lexical errors.

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: remora <file.remora>" << endl;
        return 1;
    }

    // Open the .remora source file
    ifstream file(argv[1]);
    if (!file) {
        cout << "Error: Could not open file " << argv[1] << endl;
        return 1;
    }

    // Read the entire file character-by-character into sourceCode
    string sourceCode = "";
    char ch;
    while (file.get(ch)) {
        sourceCode += ch;
    }
    file.close();

    // Run the lexical analyzer
    ErrorReporter reporter;
    Lexer lexer(sourceCode, reporter);

    Token tokens[MAX_TOKENS];
    int tokenCount = 0;
    lexer.tokenize(tokens, tokenCount);

    // Print errors if any occurred; otherwise print all recognized tokens
    if (reporter.hasErrors()) {
        reporter.printAll();
    } else {
        cout << "--- TOKENS ---" << endl;
        for (int i = 0; i < tokenCount; i++) {
            cout << "Line " << tokens[i].line
                 << " | Type: " << tokens[i].type
                 << " | Lexeme: " << tokens[i].lexeme << endl;
        }
    }

    return 0;
}