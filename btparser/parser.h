#pragma once

#include "lexer.h"
#include "ast.h"

class Parser
{
public:
    struct Error
    {
        explicit Error(const std::string & text)
            : text(text) {}

        std::string text;
    };

    explicit Parser();
    bool ParseFile(const std::string & filename, std::string & error);

private:
    Lexer mLexer;
    std::vector<Lexer::TokenState> mTokens;
    size_t mIndex = 0;
    AST::uptr<AST::Block> mBinaryTemplate = nullptr;
    std::vector<Error> mErrors;

    Lexer::TokenState CurToken;
    void NextToken();
    void ReportError(const std::string & error);

    AST::uptr<AST::Block> ParseBinaryTemplate();
    AST::uptr<AST::StatDecl> ParseStatDecl();

    AST::uptr<AST::Stat> ParseStat();
    AST::uptr<AST::Block> ParseBlock();
    AST::uptr<AST::Expr> ParseExpr();
    AST::uptr<AST::Return> ParseReturn();

    AST::uptr<AST::Decl> ParseDecl();
    AST::uptr<AST::Builtin> ParseBuiltin();
    AST::uptr<AST::Struct> ParseStruct();
};