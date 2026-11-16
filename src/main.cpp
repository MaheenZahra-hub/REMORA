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

    return 0;
}