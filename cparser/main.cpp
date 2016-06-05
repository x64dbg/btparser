#include <windows.h>
#include <stdio.h>
#include <string>
#include <stdint.h>
#include <unordered_map>
#include <functional>
#include <vector>
#include <tuple>
#include "filehelper.h"
#include "stringutils.h"
#include "testfiles.h"

#define MAKE_OP_TRIPLE(ch1, ch2, ch3) (ch3 << 16 | ch2 << 8 | ch1)
#define MAKE_OP_DOUBLE(ch1, ch2) (ch2 << 8 | ch1)
#define MAKE_OP_SINGLE(ch1) (ch1)

using namespace std;

struct Lexer
{
    explicit Lexer()
    {
        SetupTokenMaps();
    }

    enum Token
    {
        //status tokens
        tok_eof = -10000,
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

    vector<uint8_t> Input;
    string ConsumedInput;
    size_t Index = 0;
    string Error;
    vector<String> Warnings;

    //lexer state
    string IdentifierStr;
    uint64_t NumberVal = 0;
    string StringLit;
    char CharLit = '\0';
    int LastChar = ' ';
    int CurLine = 0;

    void ResetLexerState()
    {
        Input.clear();
        ConsumedInput.clear();
        Index = 0;
        Error.clear();
        IdentifierStr.clear();
        NumberVal = 0;
        StringLit.clear();
        CharLit = '\0';
        LastChar = ' ';
        CurLine = 0;
    }

    unordered_map<string, Token> KeywordMap;
    unordered_map<Token, string> ReverseTokenMap;
    unordered_map<int, Token> OpTripleMap;
    unordered_map<int, Token> OpDoubleMap;
    unordered_map<int, Token> OpSingleMap;

    void SetupTokenMaps()
    {
        //setup keyword map
#define DEF_KEYWORD(keyword) KeywordMap[#keyword] = tok_##keyword;
#include "keywords.h"
#undef DEF_KEYWORD

        //setup token maps
#define DEF_OP_TRIPLE(enumval, ch1, ch2, ch3) OpTripleMap[MAKE_OP_TRIPLE(ch1, ch2, ch3)] = tok_##enumval;
#define DEF_OP_DOUBLE(enumval, ch1, ch2) OpDoubleMap[MAKE_OP_DOUBLE(ch1, ch2)] = tok_##enumval;
#define DEF_OP_SINGLE(enumval, ch1) OpSingleMap[MAKE_OP_SINGLE(ch1)] = tok_##enumval;
#include "operators.h"
#undef DEF_OP_TRIPLE
#undef DEF_OP_DOUBLE
#undef DEF_OP_SINGLE

        //setup reverse token maps
#define DEF_KEYWORD(keyword) ReverseTokenMap[tok_##keyword] = #keyword;
#include "keywords.h"
#undef DEF_KEYWORD

#define DEF_OP_TRIPLE(enumval, ch1, ch2, ch3) ReverseTokenMap[tok_##enumval] = string({ch1, ch2, ch3});
#define DEF_OP_DOUBLE(enumval, ch1, ch2) ReverseTokenMap[tok_##enumval] = string({ch1, ch2});
#define DEF_OP_SINGLE(enumval, ch1) ReverseTokenMap[tok_##enumval] = string({ch1});
#include "operators.h"
#undef DEF_OP_TRIPLE
#undef DEF_OP_DOUBLE
#undef DEF_OP_SINGLE
    }

    Token ReportError(const String & error)
    {
        Error = error;
        return tok_error;
    }

    void ReportWarning(const String & warning)
    {
        Warnings.push_back(warning);
    }

    String TokString(int tok)
    {
        switch (Token(tok))
        {
        case tok_eof: return "tok_eof";
        case tok_error: return StringUtils::sprintf("error(\"%s\")", Error.c_str());
        case tok_identifier: return StringUtils::sprintf("id(\"%s\")", IdentifierStr.c_str());
        case tok_number: return StringUtils::sprintf("num(%llu, 0x%llX)", NumberVal, NumberVal);
        case tok_stringlit: return StringUtils::sprintf("\"%s\"", StringUtils::Escape(StringLit).c_str());
        case tok_charlit:
        {
            String s;
            s = CharLit;
            return StringUtils::sprintf("\'%s\'", StringUtils::Escape(s).c_str());
        }
        default:
        {
            auto found = ReverseTokenMap.find(Token(tok));
            if (found != ReverseTokenMap.end())
                return found->second;
            return "<INVALID TOKEN>";
        }
        }
    }

    int PeekChar(int distance = 0)
    {
        if (Index + distance >= Input.size())
            return EOF;
        auto ch = Input[Index + distance];
        if (ch == '\0')
        {
            ReportWarning(StringUtils::sprintf("\\0 character in file data"));
            return PeekChar(distance + 1);
        }
        return ch;
    }

    int ReadChar()
    {
        if (Index == Input.size())
            return EOF;
        auto ch = Input[Index++];
        if (ch == '\0')
        {
            ReportWarning(StringUtils::sprintf("\\0 character in file data"));
            return ReadChar();
        }
        ConsumedInput += ch;
        return ch;
    }

    bool CheckString(const string & expected)
    {
        for (size_t i = 0; i < expected.size(); i++)
        {
            auto ch = PeekChar(i);
            if (ch == EOF)
                return false;
            if (ch != uint8_t(expected[i]))
                return false;
        }
        Index += expected.size();
        return true;
    }

    int NextChar(int count = 1)
    {
        for (auto i = 0; i < count; i++)
            LastChar = ReadChar();
        return LastChar;
    }

    int GetToken()
    {
        //skip whitespace
        while (isspace(LastChar))
        {
            if (LastChar == '\n')
                CurLine++;
            NextChar();
        }

        //skip \\[\r\n]
        if (LastChar == '\\' && (PeekChar() == '\r' || PeekChar() == '\n'))
        {
            NextChar();
            return GetToken();
        }

        //character literal
        if (LastChar == '\'')
        {
            string charLit;
            while (true)
            {
                NextChar();
                if (LastChar == EOF) //end of file
                    return ReportError("unexpected end of file in character literal (1)");
                if (LastChar == '\r' || LastChar == '\n')
                    return ReportError("unexpected newline in character literal (1)");
                if (LastChar == '\'') //end of character literal
                {
                    NextChar();
                    return tok_charlit;
                }
                if (LastChar == '\\') //escape sequence
                {
                    NextChar();
                    if (LastChar == EOF)
                        return ReportError("unexpected end of file in character literal (2)");
                    if (LastChar == '\r' || LastChar == '\n')
                        return ReportError("unexpected newline in character literal (2)");
                    if (LastChar == '\'' || LastChar == '\"' || LastChar == '?' || LastChar == '\\')
                        LastChar = LastChar;
                    else if (LastChar == 'a')
                        LastChar = '\a';
                    else if (LastChar == 'b')
                        LastChar = '\b';
                    else if (LastChar == 'f')
                        LastChar = '\f';
                    else if (LastChar == 'n')
                        LastChar = '\n';
                    else if (LastChar == 'r')
                        LastChar = '\r';
                    else if (LastChar == 't')
                        LastChar = '\t';
                    else if (LastChar == 'v')
                        LastChar = '\v';
                    else if (LastChar == '0')
                        LastChar = '\0';
                    else if (LastChar == 'x') //\xHH
                    {
                        auto ch1 = NextChar();
                        auto ch2 = NextChar();
                        if (isxdigit(ch1) && isxdigit(ch2))
                        {
                            char byteStr[3] = "";
                            byteStr[0] = ch1;
                            byteStr[1] = ch2;
                            unsigned int hexData;
                            if (sscanf_s(byteStr, "%X", &hexData) != 1)
                                return ReportError(StringUtils::sprintf("sscanf_s failed for hex sequence \"\\x%c%c\" in character literal", ch1, ch2));
                            LastChar = hexData & 0xFF;
                        }
                        else
                            return ReportError(StringUtils::sprintf("invalid hex sequence \"\\x%c%c\" in character literal", ch1, ch2));
                    }
                    else
                        return ReportError(StringUtils::sprintf("invalid escape sequence \"\\%c\" in character literal", LastChar));
                }
                charLit += LastChar;
            }
        }

        //string literal
        if (LastChar == '\"')
        {
            StringLit.clear();
            while (true)
            {
                NextChar();
                if (LastChar == EOF) //end of file
                    return ReportError("unexpected end of file in string literal (1)");
                if (LastChar == '\r' || LastChar == '\n')
                    return ReportError("unexpected newline in string literal (1)");
                if (LastChar == '\"') //end of string literal
                {
                    NextChar();
                    return tok_stringlit;
                }
                if (LastChar == '\\') //escape sequence
                {
                    NextChar();
                    if (LastChar == EOF)
                        return ReportError("unexpected end of file in string literal (2)");
                    if (LastChar == '\r' || LastChar == '\n')
                        return ReportError("unexpected newline in string literal (2)");
                    if (LastChar == '\'' || LastChar == '\"' || LastChar == '?' || LastChar == '\\')
                        LastChar = LastChar;
                    else if (LastChar == 'a')
                        LastChar = '\a';
                    else if (LastChar == 'b')
                        LastChar = '\b';
                    else if (LastChar == 'f')
                        LastChar = '\f';
                    else if (LastChar == 'n')
                        LastChar = '\n';
                    else if (LastChar == 'r')
                        LastChar = '\r';
                    else if (LastChar == 't')
                        LastChar = '\t';
                    else if (LastChar == 'v')
                        LastChar = '\v';
                    else if (LastChar == '0')
                        LastChar = '\0';
                    else if (LastChar == 'x') //\xHH
                    {
                        auto ch1 = NextChar();
                        auto ch2 = NextChar();
                        if (isxdigit(ch1) && isxdigit(ch2))
                        {
                            char byteStr[3] = "";
                            byteStr[0] = ch1;
                            byteStr[1] = ch2;
                            unsigned int hexData;
                            if (sscanf_s(byteStr, "%X", &hexData) != 1)
                                return ReportError(StringUtils::sprintf("sscanf_s failed for hex sequence \"\\x%c%c\" in string literal", ch1, ch2));
                            LastChar = hexData & 0xFF;
                        }
                        else
                            return ReportError(StringUtils::sprintf("invalid hex sequence \"\\x%c%c\" in string literal", ch1, ch2));
                    }
                    else
                        return ReportError(StringUtils::sprintf("invalid escape sequence \"\\%c\" in string literal", LastChar));
                }
                StringLit.push_back(LastChar);
            }
        }

        //identifier/keyword
        if (isalpha(LastChar) || LastChar == '_') //[a-zA-Z_]
        {
            IdentifierStr = LastChar;
            NextChar();
            while (isalnum(LastChar) || LastChar == '_') //[0-9a-zA-Z_]
            {
                IdentifierStr += LastChar;
                NextChar();
            }

            //keywords
            auto found = KeywordMap.find(IdentifierStr);
            if (found != KeywordMap.end())
                return found->second;

            return tok_identifier;
        }

        //hex numbers
        if (LastChar == '0' && PeekChar() == 'x') //0x
        {
            string NumStr;
            ReadChar(); //consume the 'x'

            while (isxdigit(NextChar())) //[0-9a-fA-F]*
                NumStr += LastChar;

            if (!NumStr.length()) //check for error condition
                return ReportError("no hex digits after \"0x\" prefix");

            if (sscanf_s(NumStr.c_str(), "%llX", &NumberVal) != 1)
                return ReportError("sscanf_s failed on hexadecimal number");
            return tok_number;
        }
        if (isdigit(LastChar)) //[0-9]
        {
            string NumStr;
            NumStr = LastChar;

            while (isdigit(NextChar())) //[0-9]*
                NumStr += LastChar;

            if (sscanf_s(NumStr.c_str(), "%llu", &NumberVal) != 1)
                return ReportError("sscanf_s failed on decimal number");
            return tok_number;
        }

        //comments
        if (LastChar == '/' && PeekChar() == '/') //line comment
        {
            do
            {
                NextChar();
                if (LastChar == '\n')
                    CurLine++;
            } while (!(LastChar == EOF || LastChar == '\n'));

            NextChar();
            return GetToken(); //interpret the next line
        }
        if (LastChar == '/' && PeekChar() == '*') //block comment
        {
            do
            {
                NextChar();
                if (LastChar == '\n')
                    CurLine++;
            } while (!(LastChar == EOF || LastChar == '*' && PeekChar() == '/'));

            if (LastChar == EOF) //unexpected end of file
                return ReportError("unexpected end of file in block comment");

            NextChar(2);
            return GetToken(); //get the next non-comment token
        }

        //operators
        auto opFound = OpTripleMap.find(MAKE_OP_TRIPLE(LastChar, PeekChar(), PeekChar(1)));
        if (opFound != OpTripleMap.end())
        {
            NextChar(3);
            return opFound->second;
        }
        opFound = OpDoubleMap.find(MAKE_OP_DOUBLE(LastChar, PeekChar()));
        if (opFound != OpDoubleMap.end())
        {
            NextChar(2);
            return opFound->second;
        }
        opFound = OpSingleMap.find(MAKE_OP_SINGLE(LastChar));
        if (opFound != OpSingleMap.end())
        {
            NextChar(1);
            return opFound->second;
        }

        //end of file
        if (LastChar == EOF)
            return tok_eof;

        //unknown character
        return ReportError(StringUtils::sprintf("unexpected character \"%c\"", LastChar));
    }

    bool ReadInputFile(const string & filename)
    {
        ResetLexerState();
        return FileHelper::ReadAllData(filename, Input);
    }

    bool TestLex(function<void(const string & line)> lexEnum)
    {
        auto line = 0;
        lexEnum("1: ");
        int tok;
        do
        {
            tok = GetToken();
            string toks;
            while (line < CurLine)
            {
                line++;
                toks += StringUtils::sprintf("\n%d: ", line + 1);
            }
            lexEnum(toks + TokString(tok) + " ");
        } while (tok != tok_eof && tok != tok_error);
        if (tok != tok_error && tok != tok_eof)
            tok = ReportError("lexer did not finish at the end of the file");
        for (const auto & warning : Warnings)
            lexEnum("\nwarning: "  + warning);
        return tok != tok_error;
    }
};

bool TestLexer(const string & filename)
{
    Lexer lexer;
    if (!lexer.ReadInputFile("tests\\" + filename))
    {
        printf("failed to read \"%s\"\n", filename.c_str());
        return false;
    }
    string actual;
    if(!lexer.TestLex([&](const string & line)
    {
        actual += line;
    }))
    {
        actual += StringUtils::sprintf("lex error in \"%s\": %s\n", filename.c_str(), lexer.Error.c_str());
    }
    actual = StringUtils::Trim(actual);
    string expected;
    if (!FileHelper::ReadAllText("tests\\expected\\" + filename + ".lextest", expected)) //don't fail tests that we didn't specify yet
        return true;
    StringUtils::ReplaceAll(expected, "\r\n", "\n");
    expected = StringUtils::Trim(expected);
    if (expected == actual)
    {
        printf("lexer test for \"%s\" success!\n", filename.c_str());
        return true;
    }
    printf("lexer test for \"%s\" failed\n", filename.c_str());
    FileHelper::WriteAllText("expected.out", expected);
    FileHelper::WriteAllText("actual.out", actual);
    return false;
}

void RunLexerTests()
{
    for (auto file : testFiles)
        TestLexer(file);
}

bool DebugLexer(const string & filename)
{
    Lexer lexer;
    if (!lexer.ReadInputFile("tests\\" + filename))
    {
        printf("failed to read \"%s\"\n", filename.c_str());
        return false;
    }
    lexer.TestLex([](const string & line)
    {
        printf("%s", line.c_str());
    });
    puts("");
    return true;
}

void GenerateExpected(const string & filename)
{
    Lexer lexer;
    if (!lexer.ReadInputFile("tests\\" + filename))
    {
        printf("failed to read \"%s\"\n", filename.c_str());
        return;
    }
    string actual;
    if (!lexer.TestLex([&](const string & line)
    {
        actual += line;
    }))
    {
        actual += StringUtils::sprintf("lex error in \"%s\": %s\n", filename.c_str(), lexer.Error.c_str());
    }
    FileHelper::WriteAllText("tests\\expected\\" + filename + ".lextest", actual);
}

int main()
{
    RunLexerTests();
    system("pause");
    return 0;
}