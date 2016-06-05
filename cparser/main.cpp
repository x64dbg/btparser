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
        tok_stringlit, //"([^\\"\r\n]|\\[\\"abfnrtv])*"

        //operators
#define DEF_OP_TRIPLE(enumval, ch1, ch2, ch3) enumval,
#define DEF_OP_DOUBLE(enumval, ch1, ch2) enumval,
#define DEF_OP_SINGLE(enumval, ch1) enumval,
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
#define DEF_OP_TRIPLE(enumval, ch1, ch2, ch3) OpTripleMap[MAKE_OP_TRIPLE(ch1, ch2, ch3)] = enumval;
#define DEF_OP_DOUBLE(enumval, ch1, ch2) OpDoubleMap[MAKE_OP_DOUBLE(ch1, ch2)] = enumval;
#define DEF_OP_SINGLE(enumval, ch1) OpSingleMap[MAKE_OP_SINGLE(ch1)] = enumval;
#include "operators.h"
#undef DEF_OP_TRIPLE
#undef DEF_OP_DOUBLE
#undef DEF_OP_SINGLE

        //setup reverse token maps
#define DEF_KEYWORD(keyword) ReverseTokenMap[tok_##keyword] = "tok_" #keyword;
#include "keywords.h"
#undef DEF_KEYWORD

#define DEF_OP_TRIPLE(enumval, ch1, ch2, ch3) ReverseTokenMap[enumval] = #enumval;
#define DEF_OP_DOUBLE(enumval, ch1, ch2) ReverseTokenMap[enumval] = #enumval;
#define DEF_OP_SINGLE(enumval, ch1) ReverseTokenMap[enumval] = #enumval;
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
        case tok_error: return StringUtils::sprintf("tok_error \"%s\"", Error.c_str());
        case tok_identifier: return StringUtils::sprintf("tok_identifier \"%s\"", IdentifierStr.c_str());
        case tok_number: return StringUtils::sprintf("tok_number %llu (0x%llX)", NumberVal, NumberVal);
        case tok_stringlit: return StringUtils::sprintf("tok_stringlit \"%s\"", StringUtils::Escape(StringLit).c_str());
        default:
        {
            auto found = ReverseTokenMap.find(Token(tok));
            if (found != ReverseTokenMap.end())
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
                    return ReportError("unexpected end of file in string literal (1)");
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
                        LastChar = '\1'; //TODO: handle this properly (vector<uint8_t>)
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
                            LastChar = hexData & 0xFF; //TODO: handle this properly (vector<uint8_t>)
                        }
                        else
                            return ReportError(StringUtils::sprintf("invalid hex sequence \"\\x%c%c\" in string literal", ch1, ch2));
                    }
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
        return FileHelper::ReadAllData(filename, Input);
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
        for (const auto & warning : Warnings)
            lexEnum("Warning: "  + warning);
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
    DebugLexer(testFiles[82]);
    RunLexerTests();
    system("pause");
    return 0;
}