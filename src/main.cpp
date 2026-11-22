#include "../include/lexer.h"
#include "../include/errorReporter.h"
#include "../include/parser.h"
#include "../include/ASTprinter.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "Usage: remora <file.remora>" << endl;
        return 1;
    }

    // Read source file
    ifstream file(argv[1]);

    if (!file) {
        cerr << "Error: Could not open file "
             << argv[1] << endl;
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string sourceCode = buffer.str();
    file.close();

    ErrorReporter reporter;

    // STEP 1: LEXICAL ANALYSIS
    cout << "\n========== LEXICAL ANALYSIS ==========\n";

    Token tokens[MAX_TOKENS];
    int tokenCount = 0;

    Lexer lexer(sourceCode, reporter);
    lexer.tokenize(tokens, tokenCount);

    if (reporter.hasErrors()) {
        reporter.printAll();
        return 1;
    }

    cout << "--- TOKENS ---\n";

    for (int i = 0; i < tokenCount; i++) {
        cout << "Line " << tokens[i].line
             << " | Type: " << tokens[i].type
             << " | Lexeme: " << tokens[i].lexeme
             << endl;
    }

    cout << "\nLexical analysis successful!\n";

    // STEP 2: SYNTAX ANALYSIS
    cout << "\n========== SYNTAX ANALYSIS ==========\n";

    Parser parser(tokens, tokenCount, reporter);
    ProgramNode* program = parser.parseProgram();

    if (reporter.hasErrors() || program == 0) {
        reporter.printAll();
        delete program;
        return 1;
    }

    cout << "Syntax analysis successful!\n";

    // STEP 3: AST PRINTING
    cout << "\n========== AST ==========\n";

    ASTPrinter printer;
    printer.print(program);

    // STEP 4: SEMANTIC ANALYSIS
    // Member 4's Semantic Analyzer will be integrated here.

    cout << "\nSemantic Analyzer integration pending.\n";

    // Free AST memory
    delete program;

    cout << "\n========== COMPILATION FINISHED ==========\n";

    return 0;
}
