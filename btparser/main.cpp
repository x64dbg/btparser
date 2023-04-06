#include <windows.h>
#include <stdio.h>
#include "testfiles.h"
#include "lexer.h"
#include "parser.h"
#include "helpers.h"
#include "preprocessor.h"
#include "types.h"

bool TestLexer(Lexer & lexer, const std::string & filename)
{
    if(!lexer.ReadInputFile("tests\\" + filename))
    {
        printf("failed to read \"%s\"\n", filename.c_str());
        return false;
    }
    std::string actual;
    actual.reserve(65536);
    auto success = lexer.Test([&](const std::string & line)
    {
        actual.append(line);
    });
    std::string expected;
    if(FileHelper::ReadAllText("tests\\exp_lex\\" + filename, expected) && expected == actual)
    {
        printf("lexer test for \"%s\" success!\n", filename.c_str());
        return true;
    }
    if(success)
        return true;
    printf("lexer test for \"%s\" failed...\n", filename.c_str());
    FileHelper::WriteAllText("expected.out", expected);
    FileHelper::WriteAllText("actual.out", actual);
    return false;
}

bool DebugLexer(Lexer & lexer, const std::string & filename, bool output)
{
    if(!lexer.ReadInputFile("tests\\" + filename))
    {
        printf("failed to read \"%s\"\n", filename.c_str());
        return false;
    }
    auto success = lexer.Test([](const std::string & line)
    {
        printf("%s", line.c_str());
    }, output);
    if(output)
        puts("");
    return success;
}

void GenerateExpected(Lexer & lexer, const std::string & filename)
{
    if(!lexer.ReadInputFile("tests\\" + filename))
    {
        printf("failed to read \"%s\"\n", filename.c_str());
        return;
    }
    std::string actual;
    actual.reserve(65536);
    lexer.Test([&](const std::string & line)
    {
        actual.append(line);
    });
    FileHelper::WriteAllText("tests\\exp_lex\\" + filename, actual);
}

void GenerateExpectedTests()
{
    Lexer lexer;
    for(auto file : testFiles)
        GenerateExpected(lexer, file);
}

void RunLexerTests()
{
    Lexer lexer;
    for(auto file : testFiles)
        TestLexer(lexer, file);
}

void DebugLexerTests(bool output = true)
{
    Lexer lexer;
    for(auto file : testFiles)
        DebugLexer(lexer, file, output);
}

bool DebugParser(const std::string & filename)
{
    std::string data;
    if (!FileHelper::ReadAllText("tests\\" + filename, data))
    {
        printf("Failed to read: %s\n", filename.c_str());
        return false;
    }

    std::string pperror;
    std::unordered_map<std::string, std::string> definitions;
    definitions["WIN32"] = "";
    definitions["_MSC_VER"] = "1337";
    auto ppData = preprocess(data, pperror, definitions);
    if (!pperror.empty())
    {
        printf("Preprocess error: %s\n", pperror.c_str());
        return false;
    }

    FileHelper::WriteAllText("tests\\" + filename + ".pp.h", ppData);

    std::vector<std::string> errors;
    Types::TypeManager typeManager;
    if (!typeManager.ParseTypes(ppData, filename, errors))
    {
        puts("Failed to parse types:");
        for (const auto& error : errors)
            puts(error.c_str());
        return false;
    }

    puts("ParseFile success!");
    return true;
}

int main()
{
    //GenerateExpectedTests();
    auto ticks = GetTickCount();
    DebugParser("cursor.hpp");
    //Lexer lexer;
    //DebugLexer(lexer, "AndroidManifestTemplate.bt", false);
    //RunLexerTests();
    printf("finished in %ums\n", GetTickCount() - ticks);
    system("pause");
    return 0;
}