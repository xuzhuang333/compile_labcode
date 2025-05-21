#include"front/syntax.h"

#include<iostream>
#include<cassert>

using frontend::Parser;

// #define DEBUG_PARSER
#define TODO assert(0 && "todo")
#define CUR_TOKEN_IS(tk_type) (token_stream[index].type == TokenType::tk_type)
#define PARSE_TOKEN root->children.push_back(parseTerm(root));
#define PARSE(type) {auto name = new type(root); assert(parse##type(name)); root->children.push_back(name);}

#define FOL_TOKEN_IS(tk_type, offset) (token_stream[index+offset].type == TokenType::tk_type)
#define CHECK(type) {if(!CUR_TOKEN_IS(type)) return 0; PARSE_TOKEN}
// #define CHECK(type) {assert(CUR_TOKEN_IS(type)); PARSE_TOKEN}
#define MATCH(nd_type) ((nd_type >> static_cast<int>(token_stream[index].type)) & 1)
#define SHIFT(tk_type) (1ull << static_cast<int>(frontend::TokenType::tk_type))
#define ull unsigned long long
ull CONSTDECL = SHIFT(CONSTTK);
ull BTYPE = SHIFT(INTTK) | SHIFT(FLOATTK);
ull CONSTDEF = SHIFT(IDENFR);
ull VARDEF = SHIFT(IDENFR);
ull FUNCTYPE = SHIFT(VOIDTK) | SHIFT(INTTK) | SHIFT(FLOATTK);
ull LVAL = SHIFT(IDENFR);
ull NUMBER = SHIFT(INTLTR) | SHIFT(FLOATLTR);
ull UNARYOP = SHIFT(PLUS) | SHIFT(MINU) | SHIFT(NOT);
ull BLOCK = SHIFT(LBRACE);
ull FUNCDEF = FUNCTYPE;
ull VARDECL = BTYPE;
ull DECL = CONSTDECL | VARDECL;
ull COMPUINT = DECL | FUNCDEF;
ull FUNCFPARAM = BTYPE;
ull FUNCFPARAMS = FUNCFPARAM;
ull PRIMARYEXP = SHIFT(LPARENT) | LVAL | NUMBER;
ull UNARYEXP = PRIMARYEXP | SHIFT(IDENFR) | UNARYOP;
ull MULEXP = UNARYEXP;
ull ADDEXP = MULEXP;
ull EXP = ADDEXP;
ull STMT = LVAL | BLOCK | SHIFT(IFTK) | SHIFT(WHILETK) | SHIFT(BREAKTK) | SHIFT(CONTINUETK) | SHIFT(RETURNTK) | EXP | SHIFT(SEMICN);
ull BLOCKITEM = DECL | STMT;
ull INITVAL = EXP | SHIFT(LBRACE);
ull FUNCRPARAMS = EXP;
ull RELEXP = ADDEXP;
ull EQEXP = RELEXP;
ull LANDEXP = EQEXP;
ull LOREXP = LANDEXP;
ull COND = LOREXP;
ull CONSTEXP = ADDEXP;
ull CONSTINITVAL = CONSTEXP | SHIFT(LBRACE);


Parser::Parser(const std::vector<frontend::Token>& tokens): index(0), token_stream(tokens) {}

Parser::~Parser() {}

frontend::CompUnit* Parser::get_abstract_syntax_tree(){
    // TODO1.8;
    auto it = new CompUnit(nullptr);
    assert(parseCompUnit(it));
    return it;
}

bool Parser::parseCompUnit(AstNode *root)
{    
    // TODO1.10;
    if(MATCH(FUNCDEF) && FOL_TOKEN_IS(LPARENT, 2))
        PARSE(FuncDef)
    else PARSE(Decl)

    if(MATCH(COMPUINT))
        PARSE(CompUnit)
    
    return 1;
}
bool Parser::parseDecl(AstNode *root)
{
    if(MATCH(CONSTDECL))
        PARSE(ConstDecl)
    else PARSE(VarDecl)
    
    return 1;   
}
bool Parser::parseConstDecl(AstNode *root)
{
    CHECK(CONSTTK)
    PARSE(BType)
    PARSE(ConstDef)
    while(CUR_TOKEN_IS(COMMA))
    {
        PARSE_TOKEN
        PARSE(ConstDef)
    }        
    CHECK(SEMICN)

    return 1;
}
bool Parser::parseBType(AstNode *root)
{
    if(!CUR_TOKEN_IS(INTTK) && !CUR_TOKEN_IS(FLOATTK))
        return 0;
    PARSE_TOKEN

    return 1;
}
bool Parser::parseConstDef(AstNode *root)
{
    CHECK(IDENFR)
    while(CUR_TOKEN_IS(LBRACK))
    {
        PARSE_TOKEN
        PARSE(ConstExp)
        PARSE_TOKEN
    }
    CHECK(ASSIGN)
    PARSE(ConstInitVal)

    return 1;
}
bool Parser::parseConstInitVal(AstNode *root)
{
    if(MATCH(CONSTEXP))
        PARSE(ConstExp)
    else
    {
        CHECK(LBRACE)
        PARSE(ConstInitVal)
        while(CUR_TOKEN_IS(COMMA))
        {
            PARSE_TOKEN
            PARSE(ConstInitVal)
        }
        CHECK(RBRACE)
    }

    return 1;
}
bool Parser::parseVarDecl(AstNode *root)
{
    PARSE(BType)
    PARSE(VarDef)
    while(CUR_TOKEN_IS(COMMA))
    {
        PARSE_TOKEN
        PARSE(VarDef)
    }
    CHECK(SEMICN)

    return 1;
}
bool Parser::parseVarDef(AstNode *root)
{
    CHECK(IDENFR)
    while(CUR_TOKEN_IS(LBRACK))
    {
        PARSE_TOKEN
        PARSE(ConstExp)
        CHECK(RBRACK)
    }
    if(CUR_TOKEN_IS(ASSIGN))
    {
        PARSE_TOKEN
        PARSE(InitVal)
    }

    return 1;
}
bool Parser::parseInitVal(AstNode *root)
{
    if(MATCH(EXP))
        PARSE(Exp)
    else
    {
        CHECK(LBRACE)
        if(MATCH(INITVAL))
        {
            PARSE(InitVal)
            while(CUR_TOKEN_IS(COMMA))
            {
                PARSE_TOKEN
                PARSE(InitVal)
            }
        }
        CHECK(RBRACE)
    }

    return 1;
}
bool Parser::parseFuncDef(AstNode *root)
{
    PARSE(FuncType)
    CHECK(IDENFR)
    CHECK(LPARENT)
    if(MATCH(FUNCFPARAMS))
        PARSE(FuncFParams)
    CHECK(RPARENT)
    PARSE(Block)

    return 1;
}
bool Parser::parseFuncType(AstNode *root)
{
    if(!CUR_TOKEN_IS(VOIDTK) && !CUR_TOKEN_IS(INTTK) && !CUR_TOKEN_IS(FLOATTK))
        return 0;
    PARSE_TOKEN

    return 1;
}
bool Parser::parseFuncFParam(AstNode *root)
{
    PARSE(BType)
    CHECK(IDENFR)
    if(CUR_TOKEN_IS(LBRACK))
    {
        PARSE_TOKEN
        CHECK(RBRACK)
        while(CUR_TOKEN_IS(LBRACK))
        {
            PARSE_TOKEN
            PARSE(Exp)
            CHECK(RBRACK)
        }
    }

    return 1;
}
bool Parser::parseFuncFParams(AstNode *root)
{
    PARSE(FuncFParam)
    while(CUR_TOKEN_IS(COMMA))
    {
        PARSE_TOKEN
        PARSE(FuncFParam)
    }

    return 1;
}
bool Parser::parseBlock(AstNode *root)
{
    CHECK(LBRACE)
    while(MATCH(BLOCKITEM))
        PARSE(BlockItem)
    CHECK(RBRACE)

    return 1;
}
bool Parser::parseBlockItem(AstNode *root)
{
    if(MATCH(DECL))
        PARSE(Decl)
    else PARSE(Stmt)

    return 1;
}
bool Parser::parseStmt(AstNode *root)
{
    // TODO1.12;
    if(MATCH(LVAL))
    {
        int ofs = 0;
        while(!FOL_TOKEN_IS(ASSIGN, ofs) && !FOL_TOKEN_IS(SEMICN, ofs))
            ofs++;
        if(FOL_TOKEN_IS(ASSIGN, ofs))
        {
            PARSE(LVal)
            CHECK(ASSIGN)
            PARSE(Exp)
            CHECK(SEMICN)
        }
        else
        {
            if(MATCH(EXP))
                PARSE(Exp)
            CHECK(SEMICN)
        }
    }
    else if(MATCH(BLOCK))
        PARSE(Block)
    else if(CUR_TOKEN_IS(IFTK))
    {
        PARSE_TOKEN
        CHECK(LPARENT)
        PARSE(Cond)
        CHECK(RPARENT)
        PARSE(Stmt)
        if(CUR_TOKEN_IS(ELSETK))
        {
            PARSE_TOKEN
            PARSE(Stmt)
        }
    }
    else if(CUR_TOKEN_IS(WHILETK))
    {
        PARSE_TOKEN
        CHECK(LPARENT)
        PARSE(Cond)
        CHECK(RPARENT)
        PARSE(Stmt)   
    }
    else if(CUR_TOKEN_IS(BREAKTK))
    {
        PARSE_TOKEN
        CHECK(SEMICN)
    }
    else if(CUR_TOKEN_IS(CONTINUETK))
    {
        PARSE_TOKEN
        CHECK(SEMICN)
    }
    else if(CUR_TOKEN_IS(RETURNTK))
    {
        PARSE_TOKEN
        if(MATCH(EXP))
            PARSE(Exp)
        CHECK(SEMICN)
    }
    else
    {
        if(MATCH(EXP))
            PARSE(Exp)
        CHECK(SEMICN)      
    }

    return 1;
}
bool Parser::parseExp(AstNode *root)
{
    PARSE(AddExp)

    return 1;
}
bool Parser::parseCond(AstNode *root)
{
    PARSE(LOrExp)

    return 1;
}
bool Parser::parseLVal(AstNode *root)
{
    CHECK(IDENFR)
    while(CUR_TOKEN_IS(LBRACK))
    {
        PARSE_TOKEN
        PARSE(Exp)
        CHECK(RBRACK)
    }

    return 1;
}
bool Parser::parseNumber(AstNode *root)
{
    if(!CUR_TOKEN_IS(INTLTR) && !CUR_TOKEN_IS(FLOATLTR))
        return 0;
    PARSE_TOKEN
    
    return 1;
}
bool Parser::parsePrimaryExp(AstNode *root)
{
    if(CUR_TOKEN_IS(LPARENT))
    {
        PARSE_TOKEN
        PARSE(Exp)
        CHECK(RPARENT)
    }
    else if(MATCH(LVAL))
        PARSE(LVal)
    else PARSE(Number)

    return 1;
}
bool Parser::parseUnaryExp(AstNode *root)
{
    // TODO1.11;
    if(CUR_TOKEN_IS(IDENFR) && FOL_TOKEN_IS(LPARENT, 1))
    {
        PARSE_TOKEN
        CHECK(LPARENT)
        if(MATCH(FUNCRPARAMS))
            PARSE(FuncRParams)
        CHECK(RPARENT)
    }
    else if(MATCH(PRIMARYEXP))
        PARSE(PrimaryExp)
    else
    {
        PARSE(UnaryOp)
        PARSE(UnaryExp)
    }

    return 1;
}
bool Parser::parseUnaryOp(AstNode *root)
{
    if(!CUR_TOKEN_IS(PLUS) && !CUR_TOKEN_IS(MINU) && !CUR_TOKEN_IS(NOT))
        return 0;
    PARSE_TOKEN

    return 1;
}
bool Parser::parseFuncRParams(AstNode *root)
{
    PARSE(Exp)
    while(CUR_TOKEN_IS(COMMA))
    {
        PARSE_TOKEN
        PARSE(Exp)
    }

    return 1;
}
bool Parser::parseMulExp(AstNode *root)
{
    PARSE(UnaryExp)
    while(CUR_TOKEN_IS(MULT) || CUR_TOKEN_IS(DIV) || CUR_TOKEN_IS(MOD))
    {
        PARSE_TOKEN
        PARSE(UnaryExp)
    }

    return 1;
}
bool Parser::parseAddExp(AstNode *root)
{
    PARSE(MulExp)
    while(CUR_TOKEN_IS(PLUS) || CUR_TOKEN_IS(MINU))
    {
        PARSE_TOKEN
        PARSE(MulExp)
    }

    return 1;
}
bool Parser::parseRelExp(AstNode *root)
{
    PARSE(AddExp)
    while(CUR_TOKEN_IS(LSS) || CUR_TOKEN_IS(GTR) || CUR_TOKEN_IS(LEQ) || CUR_TOKEN_IS(GEQ))
    {
        PARSE_TOKEN
        PARSE(AddExp)
    }

    return 1;
}
bool Parser::parseEqExp(AstNode *root)
{
    PARSE(RelExp)
    while(CUR_TOKEN_IS(EQL) || CUR_TOKEN_IS(NEQ))
    {
        PARSE_TOKEN
        PARSE(RelExp)
    }

    return 1;
}
bool Parser::parseLAndExp(AstNode *root)
{
    PARSE(EqExp)
    if(CUR_TOKEN_IS(AND))
    {
        PARSE_TOKEN
        PARSE(LAndExp)
    }

    return 1;
}
bool Parser::parseLOrExp(AstNode *root)
{
    PARSE(LAndExp)
    if(CUR_TOKEN_IS(OR))
    {
        PARSE_TOKEN
        PARSE(LOrExp)
    }

    return 1;
}
bool Parser::parseConstExp(AstNode *root)
{
    PARSE(AddExp)

    return 1;
}
frontend::Term *Parser::parseTerm(AstNode *root)
{
    return new Term(token_stream[index++], root);
}
void Parser::log(AstNode* node){
#ifdef DEBUG_PARSER
        std::cout << "in parse" << toString(node->type) << ", cur_token_type::" << toString(token_stream[index].type) << ", token_val::" << token_stream[index].value << '\n';
#endif
}