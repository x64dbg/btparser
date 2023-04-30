#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>

class Lexer
{
public:
    enum Token
    {
        //status tokens
        tok_eof,
        tok_error,

        //keywords
#define DEF_KEYWORD(keyword) tok_##keyword,
#include "keywords.h"
#undef DEF_KEYWORD

        //others
        tok_identifier, //[a-zA-Z_][a-zA-Z0-9_]
        tok_number, //(0x[0-9a-fA-F]+)|([0-9]+)
        tok_stringlit, //"([^\\"]|\\([\\"'?abfnrtv0]|x[0-9a-fA-f]{2}))*"
        tok_charlit, //'([^\\]|\\([\\"'?abfnrtv0]|x[0-9a-fA-f]{2}))'

        //operators
#define DEF_OP_TRIPLE(enumval, ch1, ch2, ch3) tok_##enumval,
#define DEF_OP_DOUBLE(enumval, ch1, ch2) tok_##enumval,
#define DEF_OP_SINGLE(enumval, ch1) tok_##enumval,
#include "operators.h"
#undef DEF_OP_TRIPLE
#undef DEF_OP_DOUBLE
#undef DEF_OP_SINGLE
    };

    struct TokenState
    {
        Token Token = tok_eof;
        std::string IdentifierStr; //tok_identifier
        uint64_t NumberVal = 0; //tok_number
        std::string StringLit; //tok_stringlit
        char CharLit = '\0'; //tok_charlit

        size_t CurLine = 0;
        size_t LineIndex = 0;

        bool IsType() const
        {
            return Token >= tok_signed && Token <= tok_UINT32;
        }
    };

    explicit Lexer();
    bool ReadInputFile(const std::string & filename);
    void SetInputData(const std::string & data);
    bool DoLexing(std::vector<TokenState> & tokens, std::string & error);
    bool Test(const std::function<void(const std::string & line)> & lexEnum, bool output = true);
    std::string TokString(Token tok);
    std::string TokString(const TokenState & ts);

private:
    TokenState mState;
    std::vector<std::string> mWarnings;
    std::string mError;
    std::vector<uint8_t> mInput;
    size_t mIndex = 0;
    bool mIsHexNumberVal = false;
    std::string mNumStr;
    int mLastChar = ' ';
    std::unordered_map<std::string, Token> mKeywordMap;
    std::unordered_map<Token, std::string> mReverseTokenMap;
    std::unordered_map<int, Token> mOpTripleMap;
    std::unordered_map<int, Token> mOpDoubleMap;
    std::unordered_map<int, Token> mOpSingleMap;

    void resetLexerState();
    void setupTokenMaps();
    Token reportError(const std::string & error);
    void reportWarning(const std::string & warning);
    int peekChar(size_t distance = 0);
    int readChar();
    bool checkString(const std::string & expected);
    int nextChar();
    void signalNewLine();
    Token getToken();
};