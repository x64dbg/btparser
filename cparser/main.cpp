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

    string Input;
    string ConsumedInput;
    size_t Index = 0;
    string Error;

    string IdentifierStr = "";
    uint64_t NumberVal = 0;

    int LastChar = ' ';

    enum Token
    {
        //status tokens
        tok_eof = -10000,
        tok_error,

        //keywords
        tok_typedef, //"typedef"
        tok_struct, //"struct"
        tok_char, //"char"
        tok_unsigned, //"unsigned"
        tok_int, //"int"
        tok_sizeof, //"sizeof"
        tok_BYTE, //"BYTE"
        tok_WORD, //"WORD"
        tok_DWORD, //"DWORD"
        tok_ushort, //"ushort"
        tok_uint, //"uint"
        tok_byte, //"byte"
        tok_double, //"double"
        tok_string, //"string"
        tok_return, //"return"
        tok_enum, //"enum"

        //others
        tok_identifier, //[a-zA-Z_][a-zA-Z0-9_]
        tok_number //(0x[0-9a-fA-F]+)|([0-9]+)
    };

    unordered_map<string, Token> KeywordMap;

    void SetupKeywordMap()
    {
        KeywordMap["typedef"] = tok_typedef;
        KeywordMap["struct"] = tok_struct;
        KeywordMap["char"] = tok_char;
        KeywordMap["unsigned"] = tok_unsigned;
        KeywordMap["int"] = tok_int;
        KeywordMap["sizeof"] = tok_sizeof;
        KeywordMap["BYTE"] = tok_BYTE;
        KeywordMap["WORD"] = tok_WORD;
        KeywordMap["DWORD"] = tok_DWORD;
        KeywordMap["byte"] = tok_byte;
        KeywordMap["ushort"] = tok_ushort;
        KeywordMap["uint"] = tok_uint;
        KeywordMap["double"] = tok_double;
        KeywordMap["string"] = tok_string;
        KeywordMap["return"] = tok_return;
        KeywordMap["enum"] = tok_enum;
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
        default:
            for (const auto & itr : KeywordMap)
            {
                if (tok == itr.second)
                    return "tok_" + itr.first;
            }
            if (tok > 0 && tok < 265)
            {
                String s;
                s = tok;
                return s;
            }
            return "<INVALID TOKEN>";
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

    int GetToken()
    {
        //skip whitespace
        while (isspace(LastChar))
            LastChar = ReadChar();

        //identifier/keyword
        if (isalpha(LastChar) || LastChar == '_') //[a-zA-Z_]
        {
            IdentifierStr = LastChar;
            LastChar = ReadChar();
            while (isalnum(LastChar) || LastChar == '_') //[0-9a-zA-Z_]
            {
                IdentifierStr += LastChar;
                LastChar = ReadChar();
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

            while (isxdigit(LastChar = ReadChar())) //[0-9a-fA-F]*
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

            while (isdigit(LastChar = ReadChar())) //[0-9]*
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
                LastChar = ReadChar();
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
        LastChar = ReadChar();
        return ThisChar;
    }

    bool ReadInputFile(const string & filename)
    {
        return FileHelper::ReadAllText(filename, Input);
    }

    void TestLex(function<void(const string & line)> lexEnum)
    {
        int tok;
        do
        {
            tok = GetToken();
            lexEnum(TokString(tok));
        } while (tok != tok_eof && tok != tok_error);
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
    string expected;
    if (!FileHelper::ReadAllText(filename + ".lextest", expected)) //don't fail tests that we didn't specify yet
        return true;
    StringUtils::ReplaceAll(expected, "\r\n", "\n");
    expected = StringUtils::Trim(expected);
    string actual;
    lexer.TestLex([&](const string & line)
    {
        actual += line + "\n";
    });
    actual = StringUtils::Trim(actual);
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
    DebugLexer(testFiles[1]);
    RunLexerTests();
    system("pause");
    return 0;
}