#include <windows.h>
#include <stdio.h>
#include <string>
#include <stdint.h>
#include <unordered_map>
#include "filehelper.h"
#include "stringutils.h"

using namespace std;

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
    tok_WORD, //"WORD"
    tok_DWORD, //"DWORD"

    //others
    tok_identifier, //[a-zA-Z][a-zA-Z0-9]
    tok_number //(0x[0-9a-fA-F]+)|([0-9]+)
};

unordered_map<string, Token> KeywordMap;

void setup()
{
    KeywordMap["typedef"] = tok_typedef;
    KeywordMap["struct"] = tok_struct;
    KeywordMap["char"] = tok_char;
    KeywordMap["unsigned"] = tok_unsigned;
    KeywordMap["int"] = tok_int;
    KeywordMap["sizeof"] = tok_sizeof;
    KeywordMap["WORD"] = tok_WORD;
    KeywordMap["DWORD"] = tok_DWORD;
}

Token ReportError(const String & error)
{
    Error = error;
    return tok_error;
}

String tokString(int tok)
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

int readChar()
{
    if (Index == Input.length())
        return EOF;
    ConsumedInput += Input[Index];
    return uint8_t(Input[Index++]); //do not sign-extend to support UTF-8
}

int getToken()
{
    //skip whitespace
    while (isspace(LastChar))
        LastChar = readChar();

    //identifier/keyword
    if (isalpha(LastChar)) //[a-zA-Z]
    {
        IdentifierStr = LastChar;
        while (isalnum(LastChar = readChar())) //[0-9a-zA-Z]
            IdentifierStr += LastChar;

        //keywords
        auto found = KeywordMap.find(IdentifierStr);
        if (found != KeywordMap.end())
            return found->second;

        return tok_identifier;
    }

    //(hex) numbers
    if (isdigit(LastChar)) //[0-9]
    {
        string NumStr;
        NumStr = LastChar;
        LastChar = readChar(); //this might not be a digit

        //hexadecimal numbers
        if (NumStr[0] == '0' && LastChar == 'x') //0x
        {
            NumStr = "";
            while (isxdigit(LastChar = readChar())) //[0-9a-fA-F]*
                NumStr += LastChar;

            if (!NumStr.length()) //check for error condition
                return ReportError("no hex digits after \"0x\" prefix");

            if (sscanf_s(NumStr.c_str(), "%llX", &NumberVal) != 1)
                return ReportError("sscanf_s failed on hexadecimal number");
            return tok_number;
        }

        //decimal numbers
        while (isdigit(LastChar)) //[0-9]*
        {
            NumStr += LastChar;
            LastChar = readChar();
        }

        if (sscanf_s(NumStr.c_str(), "%llu", &NumberVal) != 1)
            return ReportError("sscanf_s failed on decimal number");
        return tok_number;
    }

    //comments
    if (LastChar == '/')
    {
        LastChar = readChar();

        //line comment
        if (LastChar == '/')
        {
            do
            {
                LastChar = readChar();
            } while (LastChar != EOF && LastChar != '\n');

            if (LastChar == '\n')
                return getToken(); //interpret the next line
        }
        else
            return ReportError("invalid comment");
    }

    //end of file
    if (LastChar == EOF)
        return tok_eof;

    //unknown character
    auto ThisChar = LastChar;
    LastChar = readChar();
    return ThisChar;
}

bool ReadInputFile(const char* filename)
{
    return FileHelper::ReadAllText(filename, Input);
}

void testLex()
{
    int tok;
    do
    {
        tok = getToken();
        puts(tokString(tok).c_str());
    } while (tok != tok_eof && tok != tok_error);
}

void test()
{
    if (!ReadInputFile("test.bt"))
    {
        puts("failed to read input file");
        return;
    }
    setup();
    testLex();
}

int main()
{
    test();
    system("pause");
    return 0;
}