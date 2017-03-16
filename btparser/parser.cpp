#include "parser.h"

using namespace AST;

Parser::Parser()
    : CurToken(Lexer::TokenState())
{
}

bool Parser::ParseFile(const string & filename, string & error)
{
    if(!mLexer.ReadInputFile(filename))
    {
        error = "failed to read input file";
        return false;
    }
    if(!mLexer.DoLexing(mTokens, error))
        return false;
    CurToken = mTokens[0];
    mBinaryTemplate = ParseBinaryTemplate();
    return !!mBinaryTemplate;
}

void Parser::NextToken()
{
    if(mIndex < mTokens.size() - 1)
    {
        mIndex++;
        CurToken = mTokens[mIndex];
    }
}

void Parser::ReportError(const std::string & error)
{
    mErrors.push_back(Error(error));
}

uptr<Block> Parser::ParseBinaryTemplate()
{
    vector<uptr<StatDecl>> statDecls;
    while(true)
    {
        auto statDecl = ParseStatDecl();
        if(!statDecl)
            break;
        statDecls.push_back(move(statDecl));
    }
    auto binaryTemplate = make_uptr<Block>(move(statDecls));
    if(CurToken.Token != Lexer::tok_eof)
    {
        ReportError("last token is not EOF");
        return nullptr;
    }
    return move(binaryTemplate);
}

uptr<StatDecl> Parser::ParseStatDecl()
{
    auto decl = ParseDecl();
    if(decl)
        return move(decl);

    auto stat = ParseStat();
    if(stat)
        return move(stat);

    ReportError("failed to parse StatDecl");
    return nullptr;
}

uptr<Stat> Parser::ParseStat()
{
    auto block = ParseBlock();
    if(block)
        return move(block);

    auto expr = ParseExpr();
    if(expr)
        return move(expr);

    auto ret = ParseReturn();
    if(ret)
        return move(ret);

    ReportError("failed to parse Stat");
    return nullptr;
}

uptr<Block> Parser::ParseBlock()
{
    if(CurToken.Token != Lexer::tok_bropen) //'{'
        return nullptr;
    NextToken();

    vector<uptr<StatDecl>> statDecls;

    if(CurToken.Token == Lexer::tok_brclose) //'}'
    {
        NextToken();
        return make_uptr<Block>(move(statDecls));
    }

    ReportError("failed to parse Block");
    return nullptr;
}

uptr<Expr> Parser::ParseExpr()
{
    return nullptr;
}

uptr<Return> Parser::ParseReturn()
{
    if(CurToken.Token == Lexer::tok_return)
    {
        NextToken();
        auto expr = ParseExpr();
        if(!expr)
        {
            ReportError("failed to parse Return (ParseExpr failed)");
            return nullptr;
        }
        return make_uptr<Return>(move(expr));
    }
    return nullptr;
}

uptr<Decl> Parser::ParseDecl()
{
    auto builtin = ParseBuiltinVar();
    if(builtin)
        return move(builtin);
    auto stru = ParseStruct();
    if(stru)
        return move(stru);
    return nullptr;
}

uptr<BuiltinVar> Parser::ParseBuiltinVar()
{
    if(CurToken.Token == Lexer::tok_uint) //TODO: properly handle types
    {
        auto type = CurToken.Token;
        NextToken();
        if(CurToken.Token != Lexer::tok_identifier)
        {
            ReportError("failed to parse BuiltinVar (no identifier)");
            return nullptr;
        }
        auto id = CurToken.IdentifierStr;
        NextToken();
        if(CurToken.Token != Lexer::tok_semic)
        {
            ReportError("failed to parse BuiltinVar (no semicolon)");
            return nullptr;
        }
        NextToken();
        return make_uptr<BuiltinVar>(type, id);
    }
    return nullptr;
}

uptr<Struct> Parser::ParseStruct()
{
    if(CurToken.Token == Lexer::tok_struct)
    {
        NextToken();
        string id;
        if(CurToken.Token == Lexer::tok_identifier)
        {
            id = CurToken.IdentifierStr;
            NextToken();
        }
        auto block = ParseBlock();
        if(!block)
        {
            ReportError("failed to parse Struct (ParseBlock)");
            return nullptr;
        }
        return make_uptr<Struct>(id, move(block));
    }
    return nullptr;
}

AST::uptr<AST::StructVar> Parser::ParseStructVar()
{
    return nullptr;
}
