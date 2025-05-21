#include"front/lexical.h"

#include<map>
#include<cassert>
#include<string>

#define TODO assert(0 && "todo")

// #define DEBUG_DFA
// #define DEBUG_SCANNER


bool isoperator(char c);
bool isoperator(std::string s);
frontend::TokenType get_Ident_type(std::string s);
frontend::TokenType get_op_type(std::string s);

std::string frontend::toString(State s)
{
    switch (s)
    {
        case State::Empty:
            return "Empty";
        case State::Ident:
            return "Ident";
        case State::IntLiteral:
            return "IntLiteral";
        case State::FloatLiteral:
            return "FloatLiteral";
        case State::op:
            return "op";
        default:
            assert(0 && "invalid State");
    }
    return "";
}

std::set<std::string> frontend::keywords =
{
    "const", "int", "float", "if", "else", "while", "continue", "break", "return", "void"
};

frontend::DFA::DFA(): cur_state(frontend::State::Empty), cur_str() {}

frontend::DFA::~DFA() {}

bool frontend::DFA::next(char input, Token& buf)
{
    #ifdef DEBUG_DFA
    #include<iostream>
        std::cout << "in state [" << toString(cur_state) << "], input = \'" << input << "\', str = " << cur_str << "\t";
    #endif
    
    // TODO1.3;
    bool res = 0;
    switch (cur_state)
    {
        case State::Empty:
            if (isalpha(input) || input == '_')
            {
                cur_state = frontend::State::Ident;
                cur_str += input;
            }
            else if (isdigit(input))
            {
                cur_state = frontend::State::IntLiteral;
                cur_str += input;
            }
            else if (input == '.')
            {
                cur_state = frontend::State::FloatLiteral;
                cur_str += input;
            }
            else if (isoperator(input))
            {
                cur_state = frontend::State::op;
                cur_str += input;
            }
            else if(!isspace(input))
                assert(0 && "invalid input");
            break;
        case State::Ident:
            if (isspace(input))
            {
                buf.type = get_Ident_type(cur_str);
                buf.value = cur_str;
                reset();
                res = 1;
            }
            else if (isalpha(input) || isdigit(input) || input == '_')
                cur_str += input;
            else if (isoperator(input))
            {
                buf.type = get_Ident_type(cur_str);
                buf.value = cur_str;
                cur_str = "";
                cur_state = frontend::State::op;
                cur_str += input;
                res = 1;
            }
            else
                assert(0 && "invalid Ident");
            break;
        case State::IntLiteral:
            if (isspace(input))
            {
                buf.type = frontend::TokenType::INTLTR;
                buf.value = cur_str;
                reset();
                res = 1;
            }
            else if (isalnum(input))
                cur_str += input;
            else if (input == '.')
            {
                cur_state = frontend::State::FloatLiteral;
                cur_str += input;
            }
            else if (isoperator(input))
            {
                buf.type = frontend::TokenType::INTLTR;
                buf.value = cur_str;
                cur_str = "";
                cur_state = frontend::State::op;
                cur_str += input;
                res = 1;
            }
            else
                assert(0 && "invalid IntLiteral");
            break;
        case State::FloatLiteral:
            if (isspace(input))
            {
                buf.type = frontend::TokenType::FLOATLTR;
                buf.value = cur_str;
                reset();
                res = 1;
            }
            else if (isdigit(input))
                cur_str += input;
            else if (isoperator(input))
            {
                buf.type = frontend::TokenType::FLOATLTR;
                buf.value = cur_str;
                cur_str = "";
                cur_state = frontend::State::op;
                cur_str += input;
                res = 1;
            }
            else
                assert(0 && "invalid FloatLiteral");
            break;
        case State::op:
            if (isspace(input))
            {
                buf.type = get_op_type(cur_str);
                buf.value = cur_str;
                reset();
                res = 1;
            }
            else if (isalpha(input) || input == '_')
            {
                buf.type = get_op_type(cur_str);
                buf.value = cur_str;
                cur_str = "";
                cur_state = frontend::State::Ident;
                cur_str += input;
                res = 1;
            }
            else if (isdigit(input))
            {
                buf.type = get_op_type(cur_str);
                buf.value = cur_str;
                cur_str = "";
                cur_state = frontend::State::IntLiteral;
                cur_str += input;
                res = 1;
            }
            else if (isoperator(input))
            {
                if (isoperator(cur_str + input))
                    cur_str += input;
                else
                {
                    buf.type = get_op_type(cur_str);
                    buf.value = cur_str;
                    cur_str = "";
                    cur_state = frontend::State::op;
                    cur_str += input;
                    res = 1;
                }
            }
            else if(input == '.')
            {
                buf.type = get_op_type(cur_str);
                buf.value = cur_str;
                cur_str = "";
                cur_state = frontend::State::FloatLiteral;
                cur_str += input;
                res = 1;                
            }
            else
                assert(0 && "invalid op");
            break;
        default:
            assert(0 && "invalid input");
            break;
    }


    #ifdef DEBUG_DFA
        std::cout << "next state is [" << toString(cur_state) << "], next str = " << cur_str << "\t, ret = " << ret << std::endl;
    #endif

    return res;
}

void frontend::DFA::reset() {
    cur_state = State::Empty;
    cur_str = "";
}

frontend::Scanner::Scanner(std::string filename): fin(filename) {
    if(!fin.is_open()) {
        assert(0 && "in Scanner constructor, input file cannot open");
    }
}

frontend::Scanner::~Scanner() {
    fin.close();
}

std::vector<frontend::Token> frontend::Scanner::run()
{
    // TODO1.1;
    std::vector<Token> tokens;
    Token tk;
    DFA dfa;
    std::string s = preproccess(fin); // delete comments
    for(auto c: s)
    {
        if(dfa.next(c, tk))
        {
            tokens.push_back(tk);

            #ifdef DEBUG_SCANNER
            #include<iostream>
            std::cout << "token: " << toString(tk.type) << "\t" << tk.value << std::endl;
            #endif
        }
    }
    return tokens;
}


std::string frontend::preproccess(std::ifstream &fin)
{
    // TODO1.2;
    Comment cur_state = Comment::Empty;
    std::string res;
    std::string curline;
    while(getline(fin, curline))
    {
        curline += '\n';
        for(auto ch: curline)
        {
            switch (cur_state)
            {
                case Comment::Empty:
                    if(ch == '/')
                        cur_state = Comment::Slash;
                    else
                        res += ch;
                    break;
                case Comment::Slash:
                    if(ch == '/')
                        cur_state = Comment::SSlash;
                    else if(ch == '*')
                        cur_state = Comment::SAsterisk;
                    else
                    {
                        cur_state = Comment::Empty;
                        res += '/';
                        res += ch;
                    }
                    break;
                case Comment::SSlash:
                    if(ch == '\n')
                    {
                        cur_state = Comment::Empty;
                        res += '\n';
                    }
                    break;
                case Comment::SAsterisk:
                    if(ch == '*')
                        cur_state = Comment::SAAsterisk;
                    break;
                case Comment::SAAsterisk:
                    if(ch == '/')
                        cur_state = Comment::Empty;
                    else if(ch != '*')
                        cur_state = Comment::SAsterisk;
                    break;
                default:
                    assert(0 && "invalid comment");
                    break;
            }           
        }
    }

    if(cur_state != Comment::Empty)
        assert(0 && "invalid comment");

    return res;
}

bool isoperator(char c)
{
    // TODO1.4;
    return c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '%' || c == '<' || c == '>' || c == '=' ||
           c == ':' || c == ';' || c == '(' || c == ')' ||
           c == '[' || c == ']' || c == '{' || c == '}' ||
           c == '!' || c == '&' || c == '|' || c == ',';
}

bool isoperator(std::string s)
{
    // TODO1.7;
    return s == ">=" || s == "<=" || s == "==" ||
           s == "!=" || s == "&&" || s == "||";    
}

frontend::TokenType get_Ident_type(std::string s)
{
    // TODO1.5;
    if (s == "const")
        return frontend::TokenType::CONSTTK;
    else if (s == "int")
        return frontend::TokenType::INTTK;
    else if (s == "float")
        return frontend::TokenType::FLOATTK;
    else if (s == "if")
        return frontend::TokenType::IFTK;
    else if (s == "else")
        return frontend::TokenType::ELSETK;
    else if (s == "while")
        return frontend::TokenType::WHILETK;
    else if (s == "continue")
        return frontend::TokenType::CONTINUETK;
    else if (s == "break")
        return frontend::TokenType::BREAKTK;
    else if (s == "return")
        return frontend::TokenType::RETURNTK;
    else if (s == "void")
        return frontend::TokenType::VOIDTK;

    return frontend::TokenType::IDENFR;
}

frontend::TokenType get_op_type(std::string s)
{
    // TODO1.6;
    if (s == "+")
        return frontend::TokenType::PLUS;
    else if (s == "-")
        return frontend::TokenType::MINU;
    else if (s == "*")
        return frontend::TokenType::MULT;
    else if (s == "/")
        return frontend::TokenType::DIV;
    else if (s == "%")
        return frontend::TokenType::MOD;
    else if (s == "<")
        return frontend::TokenType::LSS;
    else if (s == ">")
        return frontend::TokenType::GTR;
    else if (s == ":")
        return frontend::TokenType::COLON;
    else if (s == "=")
        return frontend::TokenType::ASSIGN;
    else if (s == ";")
        return frontend::TokenType::SEMICN;
    else if (s == ",")
        return frontend::TokenType::COMMA;
    else if (s == "(")
        return frontend::TokenType::LPARENT;
    else if (s == ")")
        return frontend::TokenType::RPARENT;
    else if (s == "[")
        return frontend::TokenType::LBRACK;
    else if (s == "]")
        return frontend::TokenType::RBRACK;
    else if (s == "{")
        return frontend::TokenType::LBRACE;
    else if (s == "}")
        return frontend::TokenType::RBRACE;
    else if (s == "!")
        return frontend::TokenType::NOT;
    else if (s == "<=")
        return frontend::TokenType::LEQ;
    else if (s == ">=")
        return frontend::TokenType::GEQ;
    else if (s == "==")
        return frontend::TokenType::EQL;
    else if (s == "!=")
        return frontend::TokenType::NEQ;
    else if (s == "&&")
        return frontend::TokenType::AND;
    else if (s == "||")
        return frontend::TokenType::OR;
    else
        assert(0 && "invalid operator!");
}