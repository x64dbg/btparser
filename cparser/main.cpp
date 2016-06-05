#include <windows.h>
#include <stdio.h>
#include <string>
#include <stdint.h>
#include <unordered_map>
#include <functional>
#include "filehelper.h"
#include "stringutils.h"
#include "testfiles.h"

using namespace std;

struct Lexer
{
    explicit Lexer()
    {
        SetupKeywordMap();
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
        tok_stringlit //"([^\\"\r\n]|\\[\\"abfnrtv])*"
    };

    string Input;
    string ConsumedInput;
    size_t Index = 0;
    string Error;

    //lexer state
    string IdentifierStr;
    uint64_t NumberVal = 0;
    string StringLit;

    int LastChar = ' ';

    void ResetLexerState()
    {
        Input.clear();
        ConsumedInput.clear();
        Index = 0;
        Error.clear();
        IdentifierStr.clear();
        NumberVal = 0;
        StringLit.clear();
        LastChar = ' ';
    }

    unordered_map<string, Token> KeywordMap;
    unordered_map<Token, string> ReverseKeywordMap;

    void SetupKeywordMap()
    {
#define DEF_KEYWORD(keyword) KeywordMap[#keyword] = tok_##keyword;
#include "keywords.h"
#undef DEF_KEYWORD
#define DEF_KEYWORD(keyword) ReverseKeywordMap[tok_##keyword] = "tok_" #keyword;
#include "keywords.h"
#undef DEF_KEYWORD
    }

    Token ReportError(const String & error)
    {
        Error = error;
        return tok_error;
    }

    String TokString(int tok)
    {
        switch (Token(tok))
        {
        case tok_eof: return "tok_eof";
        case tok_error: return StringUtils::sprintf("tok_error \"%s\"", Error.c_str());
        case tok_identifier: return StringUtils::sprintf("tok_identifier \"%s\"", IdentifierStr.c_str());
        case tok_number: return StringUtils::sprintf("tok_number %llu (0x%llX)", NumberVal, NumberVal);
        case tok_stringlit: return StringUtils::sprintf("tok_stringlit \"%s\"", StringUtils::Escape(StringLit).c_str());
        default:
        {
            auto found = ReverseKeywordMap.find(Token(tok));
            if (found != ReverseKeywordMap.end())
                return found->second;
            if (tok > 0 && tok < 265)
            {
                String s;
                s = tok;
                return s;
            }
            return "<INVALID TOKEN>";
        }
        }
    }

    int PeekChar(int distance = 0)
    {
        if (Index + distance >= Input.length())
            return EOF;
        return Input[Index + distance];
    }

    int ReadChar()
    {
        if (Index == Input.length())
            return EOF;
        ConsumedInput += Input[Index];
        return uint8_t(Input[Index++]); //do not sign-extend to support UTF-8
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

    int NextChar()
    {
        return LastChar = ReadChar();
    }

    int GetToken()
    {
        //skip whitespace
        while (isspace(LastChar))
            NextChar();

        //string literal
        if (LastChar == '\"')
        {
            StringLit.clear();
            while (true)
            {
                NextChar();
                if (LastChar == EOF) //end of file
                    return ReportError("unexpected end of file in string literal");
                if (LastChar == '\"') //end of string literal
                {
                    NextChar();
                    return tok_stringlit;
                }
                if (LastChar == '\\') //escape sequence
                {
                    NextChar();
                    if (LastChar == EOF)
                        return ReportError("unexpected end of file in string literal");
                    if (LastChar == '\\' || LastChar == '\"')
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
                    else
                        return ReportError(StringUtils::sprintf("invalid escape sequence \"\\%c\" in string literal", LastChar));
                }
                StringLit += LastChar;
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
            } while (LastChar != EOF && LastChar != '\n');

            if (LastChar == '\n')
                return GetToken(); //interpret the next line
        }
        else if (LastChar == '/' && PeekChar() == '*') //block comment
        {
            //TODO: implement this
        }

        //end of file
        if (LastChar == EOF)
            return tok_eof;

        //unknown character
        auto ThisChar = LastChar;
        NextChar();
        return ThisChar;
    }

    bool ReadInputFile(const string & filename)
    {
        ResetLexerState();
        return FileHelper::ReadAllText(filename, Input);
    }

    bool TestLex(function<void(const string & line)> lexEnum)
    {
        int tok;
        do
        {
            tok = GetToken();
            lexEnum(TokString(tok));
        } while (tok != tok_eof && tok != tok_error);
        if (tok != tok_error && tok != tok_eof)
            tok = ReportError("lexer did not finish at the end of the file");
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
        actual += line + "\n";
    }))
    {
        printf("lex error in \"%s\": %s\n", filename.c_str(), lexer.Error.c_str());
        return false;
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
    printf("Debugging \"%s\"\n", filename.c_str());
    Lexer lexer;
    if (!lexer.ReadInputFile("tests\\" + filename))
    {
        printf("failed to read \"%s\"\n", filename.c_str());
        return false;
    }
    lexer.TestLex([](const string & line)
    {
        puts(line.c_str());
    });
    puts("");
    return true;
}

int main()
{
    DebugLexer(testFiles[19]);
    RunLexerTests();
    system("pause");
    return 0;
}