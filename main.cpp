#include <iostream>
#include <fstream>
#include <map>
#include "val.h"
#include "parseRun.h"
bool g_valCheck = false;
bool g_print = true;
bool g_allow = true;

int main(int argc, char** argv) {
    if (argc == 2) {
        std::fstream file(argv[1]);
        int lineNumber = 1;

        if (file.is_open()) {
            bool errorsFound = Prog(file, lineNumber);
            std::cout << std::endl;
            if (errorsFound) {
                std::cout << "Unsuccessful Execution" << std::endl;
                std::cout << std::endl;
                std::cout << "Number of Syntax Errors: " << error_count << std::endl;
            }
            else
                std::cout << "Successful Execution" << std::endl;
        }
        else {
            std::cout << "CANNOT OPEN FILE " << argv[1] << std::endl;
            std::exit(1);
        }
    }
    else {
        std::cout << (argc > 2 ? "ONLY ONE FILE NAME ALLOWED" : "NO FILE NAME GIVEN") << std::endl;
        std::exit(1);
    }
    return 0;
}
bool Prog(istream& in, int& line) {
    LexItem item = Parser::GetNextToken(in, line);
    bool errorsFound = false;

    if (item.GetToken() != BEGIN) {
        ParseError(line, "No Begin Token");
        Parser::PushBackToken(item);
        errorsFound = true;
    }

    if (StmtList(in, line))
        errorsFound = true;

    return errorsFound;
}

bool StmtList(istream& in, int& line) {
    LexItem item;
    bool errorsFound = false;

    while (true) {
        item = Parser::GetNextToken(in, line);
        if (item.GetToken() == DONE || item.GetToken() == END)
            return errorsFound;
        else
            Parser::PushBackToken(item);
        if (Stmt(in, line)) {
            errorsFound = true;
        }
    }
    return errorsFound;
}

bool Stmt(istream& in, int& line) {
    LexItem item = Parser::GetNextToken(in, line);
    Parser::PushBackToken(item);

    switch (item.GetToken()) {
        case PRINT:
            return PrintStmt(in, line);
        case IF:
            return IfStmt(in, line);
        case IDENT:
            return AssignStmt(in, line);
        default:
            const int currentLine = item.GetLinenum();
            while (currentLine == line) {
                item = Parser::GetNextToken(in, line);
            }
            Parser::PushBackToken(item);
            ParseError(currentLine, "Unrecognized Input");
            return true;
    }
    return false;
}

bool PrintStmt(istream& in, int& line) {
    const int lineNumber = line;
    g_valCheck = true;
    ValQue = new queue<Value>;
    if (ExprList(in, line)) {
        ParseError(lineNumber, "Invalid Print Statement Expression");
        line++;

        while(!(*ValQue).empty()){
            ValQue->pop();
        }
        delete ValQue;
        return true;
    }


    if (g_print && g_allow) {
        while (g_print && !(*ValQue).empty()) {
            std::cout << ValQue->front();
            ValQue->pop();
        }
        std::cout << std::endl;
    }
    g_allow = true;
    return false;
}

bool IfStmt(istream& in, int& line) {
    LexItem item = Parser::GetNextToken(in, line);
    bool errorsFound = false;
    g_valCheck = true;

    if (item.GetToken() != IF) {
        ParseError(line, "Missing If Statement Expression");
        errorsFound = true;
    }

    item = Parser::GetNextToken(in, line);
    if (item.GetToken() != LPAREN) {
        ParseError(line, "Missing Left Parenthesis");
        errorsFound = true;
    }
    Value expressionValue;
    if (Expr(in, line, expressionValue)) {
        // ParseError(line, "Invalid if statement Expression");
        errorsFound = true;
    }

    if (!expressionValue.IsInt()) {
        ParseError(line, "Run-Time: Non Integer If Statement Expression");
        errorsFound = true;
    }
    else {
        if (expressionValue.GetInt() == 0)
            g_allow = false;

    }

    item = Parser::GetNextToken(in, line);
    if (item.GetToken() != RPAREN) {
        ParseError(line, "Missing Right Parenthesis");
        errorsFound = true;
    }

    item = Parser::GetNextToken(in, line);
    if (item.GetToken() != THEN) {
        ParseError(line, "Missing THEN");
        errorsFound = true;
    }

    if (errorsFound)
        g_print = false;

    if (Stmt(in, line)) {
        ParseError(line, "Invalid If Statement Expression");
        errorsFound = true;
    }


    if (errorsFound)
        g_print = false;

    return errorsFound;
}

bool AssignStmt(istream& in, int& line) {
    const int currentLine = line;
    bool errorsFound = false;
    LexItem tok;
    std::string identifier = "";

    if (Var(in, line, tok)) {
        ParseError(currentLine, "Invalid Assignment Statement Identifier");
        errorsFound = true;
    }
    else
        identifier = tok.GetLexeme();

    LexItem item = Parser::GetNextToken(in, line);
    if (item.GetToken() != EQ) {
        ParseError(currentLine, "Missing Assignment Operator =");
        errorsFound = true;
    }

    Value val;
    if (Expr(in, line, val)) {
        ParseError(currentLine, "Invalid Assignment Statement Expression");
        errorsFound = true;
    }

    if (!errorsFound && identifier != "" && g_allow) {

        if (symbolTable.find(identifier) == symbolTable.end()) {
            symbolTable.insert(std::make_pair(identifier, val));
        }
        else {
            Value& actualType = symbolTable.find(identifier)->second;

            if (actualType.IsInt() && val.IsInt() || actualType.IsReal() && val.IsReal()
                || actualType.IsStr() && val.IsStr())
                symbolTable[identifier] = val;
            else if (actualType.IsInt() && val.IsReal())
                symbolTable[identifier] = Value((int) val.GetReal());
            else if (actualType.IsReal() && val.IsInt())
                symbolTable[identifier] = Value((float) val.GetInt());
            else {
                ParseError(currentLine, "Run-Time: Illegal Type Reassignment");
                errorsFound = true;
            }
        }
    }

    item = Parser::GetNextToken(in, line);
    if (item.GetToken() != SCOMA) {
        Parser::PushBackToken(item);
        ParseError(currentLine, "Missing Semicolon");
        errorsFound = true;
    }

    if (errorsFound)
        g_print = false;
    g_allow = true;
    return errorsFound;
}

bool ExprList(istream& in, int& line) {
    LexItem item = Parser::GetNextToken(in, line);
    bool errorsFound = false;

    if (item.GetToken() != PRINT) {

        errorsFound = true;
    }

    while (true) {
        Value value;
        if (Expr(in, line, value)) {
            errorsFound = true;
        }
        else
            ValQue->push(value);
        item = Parser::GetNextToken(in, line);

        if (item.GetToken() != COMA) {
            if (item.GetToken() == SCOMA)
                return errorsFound;
            Parser::PushBackToken(item);
            return true;
        }
    }
    return errorsFound;
}

bool Expr(istream& in, int& line, Value & retVal) {
    bool errorsFound = false;
    Value accumulator;
    Value current;
    int operation = -1;

    while (true) {
        if (Term(in, line, current)) {
            return true;
        }
        else {
            if (accumulator.IsErr())
                accumulator = current;
            else if (operation == 0) {
                if (accumulator.IsStr() || current.IsStr()) {
                    ParseError(line, "Run-Time: Invalid String Operation");
                    errorsFound = true;
                }
                else
                    accumulator = accumulator + current;
            }
            else if (operation == 1) {
                if (accumulator.IsStr() || current.IsStr()) {
                    ParseError(line, "Run-Time: Invalid Arithmetic Operation");
                    errorsFound = true;
                }
                else
                    accumulator = accumulator - current;
            }
        }

        LexItem item = Parser::GetNextToken(in, line);
        if (item.GetToken() == ERR)
            errorsFound = true;
        else if (item.GetToken() == PLUS)
            operation = 0;
        else if (item.GetToken() == MINUS)
            operation = 1;
        else {
            Parser::PushBackToken(item);
            retVal = accumulator;
            return errorsFound;
        }
    }
    return errorsFound;
}

bool Term(istream& in, int& line, Value & retVal) {
    bool errorsFound = false;
    Value accumulator;
    Value current;
    int operation = -1;
    while (true) {
        if (Factor(in, line, current)) {
            return true;
        }
        else {
            if (accumulator.IsErr())
                accumulator = current;
            else if (operation == 0) {
                if (accumulator.IsStr() || current.IsStr()) {
                    ParseError(line, "Run-Time: Invalid String Operation");
                    errorsFound = true;
                }
                else
                    accumulator = accumulator * current;
            }
            else if (operation == 1) {
                if (accumulator.IsStr() || current.IsStr()) {
                    ParseError(line, "Run-Time: Invalid Arithmetic Operation");
                    errorsFound = true;
                }
                else
                    accumulator = accumulator / current;
            }
        }

        LexItem item = Parser::GetNextToken(in, line);
        if (item.GetToken() == ERR)
            errorsFound = true;
        else if (item.GetToken() == MULT)
            operation = 0;
        else if (item.GetToken() == DIV)
            operation = 1;
        else {
            Parser::PushBackToken(item);
            retVal = accumulator;
            return errorsFound;
        }
    }
    return errorsFound;
}

bool Var(istream& in, int& line, LexItem &tok) {
    LexItem item = Parser::GetNextToken(in, line);
    tok = item;
    if (item.GetToken() == IDENT) {
        if (defVar.find(item.GetLexeme()) != defVar.end()) {
            defVar[item.GetLexeme()] = false;
            g_valCheck = false;
        }
        else {
            if (g_valCheck) {
                ParseError(line, "Undefined variable used in expression");
                g_valCheck = false;
                g_print = false;
                return true;
            }
            defVar[item.GetLexeme()] = true;
        }
    }
    else {
        ParseError(line, "Invalid Identifier Expression");
        g_valCheck = false;
        return true;
    }
    return false;
}
bool Factor(istream& in, int& line, Value &retVal) {
    LexItem item = Parser::GetNextToken(in, line);
    bool errorsFound = false;
    const int lineNumber = item.GetLinenum();

    switch (item.GetToken()) {
        case ERR:
            item = Parser::GetNextToken(in, line);
            if (item.GetLinenum() == lineNumber)
                line++;
            Parser::PushBackToken(item);
            g_valCheck = false;
            return true;
        case IDENT:
            Parser::PushBackToken(item);
            if (Var(in, line, item))
                return true;
            retVal = symbolTable[item.GetLexeme()];
            return false;
        case ICONST:
            g_valCheck = false;
            retVal = Value(std::stoi(item.GetLexeme()));
            return false;
        case RCONST:
            g_valCheck = false;
            retVal = Value(std::stof(item.GetLexeme()));
            return false;
        case SCONST:
            g_valCheck = false;
            retVal = Value(item.GetLexeme());
            return false;
        case LPAREN:
            g_valCheck = false;
            if (Expr(in, line, retVal)) {
                errorsFound = true;
            }
            item = Parser::GetNextToken(in, line);
            if (item.GetToken() == RPAREN)
                return errorsFound;
            else {
                Parser::PushBackToken(item);
                ParseError(line, "Missing Right Parenthesis");
                return true;
            }
    }
    return true;
}