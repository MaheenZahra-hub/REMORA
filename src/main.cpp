#include "../include/lexer.h"
#include "../include/errorReporter.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: remora <file.remora>" << endl;
        return 1;
    }

    ifstream file(argv[1]);
    if (!file) {
        cout << "Error: Could not open file " << argv[1] << endl;
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string sourceCode = buffer.str();

    ErrorReporter reporter;
    Lexer lexer(sourceCode, reporter);
    vector<Token> tokens = lexer.tokenize();

    if (reporter.hasErrors()) {
        reporter.printAll();
    } else {
        cout << "--- TOKENS ---" << endl;
        for (int i = 0; i < tokens.size(); i++) {
            cout << "Line " << tokens[i].line
                 << " | Type: " << tokens[i].type
                 << " | Lexeme: " << tokens[i].lexeme << endl;
        }
    }

    return 0;
}