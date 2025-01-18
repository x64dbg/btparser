#include "testfiles.h"
#include "lexer.h"
#include "helpers.h"
#include "preprocessor.h"
#include "types.h"

#include <stdio.h>
#include <unordered_set>
#include <map>
#include <functional>

bool TestLexer(Lexer& lexer, const std::string& filename)
{
	if (!lexer.ReadInputFile("tests\\" + filename))
	{
		printf("failed to read \"%s\"\n", filename.c_str());
		return false;
	}
	std::string actual;
	actual.reserve(65536);
	auto success = lexer.Test([&](const std::string& line)
	{
		actual.append(line);
	});
	std::string expected;
	if (FileHelper::ReadAllText("tests\\exp_lex\\" + filename, expected) && expected == actual)
	{
		printf("lexer test for \"%s\" success!\n", filename.c_str());
		return true;
	}
	if (success)
		return true;
	printf("lexer test for \"%s\" failed...\n", filename.c_str());
	FileHelper::WriteAllText("expected.out", expected);
	FileHelper::WriteAllText("actual.out", actual);
	return false;
}

bool DebugLexer(Lexer& lexer, const std::string& filename, bool output)
{
	if (!lexer.ReadInputFile("tests\\" + filename))
	{
		printf("failed to read \"%s\"\n", filename.c_str());
		return false;
	}
	auto success = lexer.Test([](const std::string& line)
	{
		printf("%s", line.c_str());
	}, output);
	if (output)
		puts("");
	return success;
}

void GenerateExpected(Lexer& lexer, const std::string& filename)
{
	if (!lexer.ReadInputFile("tests\\" + filename))
	{
		printf("failed to read \"%s\"\n", filename.c_str());
		return;
	}
	std::string actual;
	actual.reserve(65536);
	lexer.Test([&](const std::string& line)
	{
		actual.append(line);
	});
	FileHelper::WriteAllText("tests\\exp_lex\\" + filename, actual);
}

void GenerateExpectedTests()
{
	Lexer lexer;
	for (auto file : testFiles)
		GenerateExpected(lexer, file);
}

void RunLexerTests()
{
	Lexer lexer;
	for (auto file : testFiles)
		TestLexer(lexer, file);
}

void DebugLexerTests(bool output = true)
{
	Lexer lexer;
	for (auto file : testFiles)
		DebugLexer(lexer, file, output);
}

static std::vector<std::string> SplitNamespace(const std::string& name, std::string& type)
{
	auto namespaces = StringUtils::Split(name, ':');
	if (namespaces.empty())
	{
		type.clear();
	}
	else
	{
		type = namespaces.back();
		namespaces.pop_back();
	}
	return namespaces;
}

static void HandleVTable(Types::Model& model)
{
	std::unordered_set<std::string> classes;
	std::unordered_set<std::string> visited;
	auto handleType = [&](const Types::QualifiedType& type)
	{
		if (!type.kind.empty())
		{
			const auto& name = type.name;
			const auto& kind = type.kind;
			if (visited.count(name) != 0)
				return;
			visited.insert(name);
			std::string rettype;
			auto namespaces = SplitNamespace(name, rettype);
			if (kind == "class" && rettype[0] == 'I')
				classes.insert(name);
			for (const auto& ns : namespaces)
			{
				printf("namespace %s {\n", ns.c_str());
			}
			printf("    %s %s {\n    };\n", kind.c_str(), rettype.c_str());
			for (const auto& ns : namespaces)
				printf("}\n");
		}
	};
	for (const auto& su : model.structUnions)
	{
		for (const auto& vf : su.vtable)
		{
			handleType(vf.rettype);
			for (const auto& arg : vf.args)
				handleType(arg.type);
		}
	}
	for (const auto& f : model.functions)
	{
		handleType(f.rettype);
		for (const auto& arg : f.args)
			handleType(arg.type);
	}

	puts("Used classes:");
	for (const auto& name : classes)
	{
		printf("%s\n", name.c_str());
	}
	puts("vtable chuj");
}

static void HandleJNI(Types::Model& model)
{
	std::unordered_map<std::string, Types::Function*> fntypes;
	for(auto& fn : model.functions)
		fntypes.emplace(fn.name, &fn);

	for(auto& su : model.structUnions)
	{
		if(su.name == "JNINativeInterface")
		{
			printf("Total functions: %zu\n", su.members.size());

			for(size_t memberIdx = 0; memberIdx < su.members.size(); memberIdx++)
			{
				const auto& m = su.members[memberIdx];
				if(memberIdx < 4)
					continue;

				auto fn = fntypes.at(m.type.name);
				auto prettyRet = fn->rettype.pretty();
				auto isVariadic = false;
				std::string unsupportedReason;
				std::vector<std::function<void()>> postFunctions;
				if(fn->rettype.isPointer())
				{
					if(prettyRet == "const jchar*")
					{
						postFunctions.push_back([] {
							printf("    return PtrTag::malloc<const jchar*>(result, host_strlen(result, HostStr16) + 1);\n");
						});
					}
					else if(prettyRet == "const char*")
					{
						postFunctions.push_back([] {
							printf("    return PtrTag::malloc<const char*>(result, host_strlen(result, HostStr8) + 1);\n");
						});
					}
					else if(m.name.find("PrimitiveArray") != std::string::npos)
					{
						unsupportedReason = "primitive array";
					}
					else if(m.name.find("Array") != std::string::npos)
					{
						postFunctions.push_back([fn] {
							printf("    auto size = GetArrayLength(array) * sizeof(%s);\n", fn->rettype.name.c_str());
							printf("    return PtrTag::malloc<%s>(result, size);\n", fn->rettype.pretty().c_str());
						});
					}
					else if(m.name.find("DirectBuffer") != std::string::npos)
					{
						unsupportedReason = "direct buffer";
						postFunctions.push_back([] {
							printf("    return nullptr;\n");
						});
					}
					else {
						throw std::runtime_error(m.name + " return pointer type not yet supported: " + prettyRet);
					}
				}

				if(fn->args.empty() || fn->args[0].type.pretty() != "JNIEnv*")
				{
					throw std::runtime_error("expected JNIEnv* as first argument");
				}

				std::vector<std::function<void()>> preFunctions;
				std::map<size_t, std::function<void(std::string&)>> argMap;

				std::unordered_map<std::string, int> seen;
				for(size_t argIdx = 1; argIdx < fn->args.size(); argIdx++)
				{
					auto& arg = fn->args[argIdx];
					auto renameArg = [fn, &seen](size_t idx, const std::string& name) {
						auto itr = seen.find(name);
						if(itr == seen.end())
						{
							seen[name]++;
							fn->args[idx].name = name;
						}
						else
						{
							auto n = itr->second++;
							fn->args[idx].name = name + std::to_string(n);
						}
					};
					if(arg.name.empty()) {
						arg.name = "arg" + std::to_string(argIdx);
					}
					const auto& atype = arg.type;
					auto apretty = atype.pretty();
					auto handleRegionFn = [&]
					{
						if(fn->args.size() != 5) {
							throw std::runtime_error("unexpected region");
						}
						renameArg(2, "start");
						renameArg(3, "len");
						renameArg(4, "buf");
						if(m.name.substr(0, 3) == "Get") {
							preFunctions.push_back([] {
								printf("    auto buf_host = mHost.alloc(len * sizeof(buf[0]));\n");
							});
							argMap[argIdx] = [](std::string& r) {
								r += "buf_host";
							};
							postFunctions.push_back([] {
								printf("    buf_host.read(buf, buf_host.size());\n");
							});
						} else {
							argMap[argIdx] = [](std::string& r) {
								r += "mHost.buffer(buf, len * sizeof(buf[0]))";
							};
						}
					};
					if(!atype.name.empty() && atype.name[0] == 'j')
					{
						if(atype.isPointer())
						{
							if(m.name.find("Region") != std::string::npos) {
								handleRegionFn();
							}
							else if(apretty == "const jvalue*" && m.name.back() == 'A') {
								// TODO: super hackery?
								renameArg(argIdx, "args");
								argMap[argIdx] = [](std::string& r) {
									r += "args";
								};
								unsupportedReason = "const jvalue* args";
							}
							else if(apretty == "jboolean*" && m.name.substr(0, 3) == "Get") {
								argMap[argIdx] = [argIdx, fn](std::string& r) {
									r += fn->args[argIdx].name;
									r += "_host";
								};
								preFunctions.push_back([argIdx, fn] {
									auto argName = fn->args[argIdx].name;
									printf("    auto %s_host = mHost.alloc(sizeof(jboolean));\n", argName.c_str());
								});
								postFunctions.push_back([argIdx, fn] {
									auto argName = fn->args[argIdx].name;
									printf("    if(%s != nullptr) {\n", argName.c_str());
									printf("      %s_host.read(isCopy, sizeof(jboolean));\n", argName.c_str());
									printf("    }\n");
								});
								renameArg(argIdx, "isCopy");
							}
							else if(m.name.substr(0, 7) == "Release") {
								// TODO: untag that shit
								if(fn->args.size() == 4)
								{
									renameArg(2, "buf");
									renameArg(3, "type");
									argMap[2] = [](std::string& r) {
										r += "PtrTag::free(buf)";
									};
								}
								else if(m.name.find("ReleaseString") != std::string::npos)
								{
									renameArg(2, "buf");
									argMap[2] = [](std::string& r) {
										r += "PtrTag::free(buf)";
									};
								}
								else
								{
									throw std::runtime_error("Unknown Release fn");
								}
							}
							else {
								size_t sizeArgIdx = -1;
								for (size_t k = argIdx; k < fn->args.size(); k++) {
									const auto &karg = fn->args[k];
									if (karg.type.pretty() == "jsize") {
										if (sizeArgIdx != -1) {
											throw std::runtime_error("multiple jsize found");
										}
										sizeArgIdx = k;
									}
								}
								if (sizeArgIdx == -1) {
									throw std::runtime_error("argument pointer type not yet supported: " + apretty);
								}
								preFunctions.push_back([fn, argIdx, sizeArgIdx] {
									auto argName = fn->args[argIdx].name;
									auto sizeArgName = fn->args[sizeArgIdx].name;
									auto argType = fn->args[argIdx].type.name;
									printf("    auto %s_host = mHost.alloc(%s * sizeof(%s));\n", argName.c_str(), sizeArgName.c_str(), argType.c_str());
									printf("    %s_host.write(%s, %s_host.size());\n", argName.c_str(), argName.c_str(), argName.c_str());
								});
								argMap[argIdx] = [fn, argIdx](std::string& r)
								{
									r += fn->args[argIdx].name;
									r += "_host";
								};
							}
						}
						else if(apretty == "jstring")
						{
							renameArg(argIdx, "str");
						}
						else if(apretty == "jobject")
						{
							renameArg(argIdx, "obj");
						}
						else if(apretty == "jarray")
						{
							renameArg(argIdx, "array");
						}
						else if(apretty == "jclass")
						{
							renameArg(argIdx, "clazz");
						}
						else if(apretty == "jmethodID")
						{
							renameArg(argIdx, "method");
						}
						else if(apretty == "jfieldID")
						{
							renameArg(argIdx, "field");
						}
						else if(apretty == "jweak")
						{
							renameArg(argIdx, "weakObj");
						}
						else if(apretty == "jsize")
						{
							renameArg(argIdx, "size");
						}
						else if(apretty == "jthrowable")
						{
							renameArg(argIdx, "exception");
						}
						else if(apretty == "jint")
						{
							renameArg(argIdx, "n");
						}
						else if(apretty == "jlong")
						{
							renameArg(argIdx, "n");
						}
						else if(apretty.find("Array") != std::string::npos)
						{
							renameArg(argIdx, "array");
						}
						else if(apretty == "jfloat" || apretty == "jdouble")
						{
							renameArg(argIdx, "n");
							unsupportedReason = apretty;
						}
					}
					else if(apretty == "const JNINativeMethod*") {
						// TODO: custom
						renameArg(2, "methods");
						renameArg(3, "nMethods");
						unsupportedReason = "JNINativeMethods";
						argMap[argIdx] = [](std::string& r) {
							r += "methods";
						};
					}
					else if(apretty == "JavaVM**") {
						// TODO: custom
						renameArg(argIdx, "vm");
						unsupportedReason = "JavaVM*";
						argMap[argIdx] = [](std::string& r) {
							r += "vm";
						};
					}
					else if(apretty == "...")
					{
						isVariadic = true;
						argMap[argIdx] = [](std::string& r)
						{
							r += "std::forward<Args>(args)...";
						};
					}
					else if(apretty == "va_list")
					{
						unsupportedReason = "va_list";
						renameArg(argIdx, "args");
						argMap[argIdx] = [](std::string& r)
						{
							r+= "args";
						};
					}
					else if(apretty == "const char*")
					{
						if(m.name.find("Release") != std::string::npos)
						{
							argMap[argIdx] = [fn, argIdx](std::string& r) {
								r += "PtrTag::free(";
								r += fn->args[argIdx].name;
								r += ")";
							};
						}
						else
						{
							argMap[argIdx] = [fn, argIdx](std::string &r) {
								r += "mHost.str(";
								r += fn->args[argIdx].name;
								r += ")";
								return r;
							};
						}
					}
					else if(m.name.find("Region") != std::string::npos && apretty == "char*")
					{
						handleRegionFn();
					}
					else if(apretty == "void*")
					{
						renameArg(argIdx, "buf");
						if(m.name.find("Release") != std::string::npos) {
							argMap[argIdx] = [](std::string& r) {
								r += "PtrTag::free(buf)";
							};
						} else if(m.name == "NewDirectByteBuffer") {
							unsupportedReason = "direct buffer";
							argMap[argIdx] = [](std::string& r) {
								r += "buf";
							};
						} else {
							throw std::runtime_error("unsupported void*");
						}
					}
					else
					{
						throw std::runtime_error("unsupported argument type: " + apretty);
					}
				}

				printf("\n  ");
				if(!unsupportedReason.empty())
				{
					printf("template<bool F = false> ");
				}
				else if(isVariadic)
				{
					printf("template<class... Args> ");
				}
				printf("%s %s(", fn->rettype.pretty().c_str(), m.name.c_str());
				for(size_t argIdx = 1; argIdx < fn->args.size(); argIdx++)
				{
					const auto& arg = fn->args[argIdx];
					if(argIdx > 1)
						printf(", ");
					auto apretty = arg.type.pretty();
					if(apretty == "...")
					{
						printf("Args &&...args");
					}
					else
					{
						printf("%s %s", apretty.c_str(), arg.name.c_str());
					}
				}
				printf(") {\n");
				if(!unsupportedReason.empty())
				{
					printf("    static_assert(F != F, \"Unsupported: %s\");\n", unsupportedReason.c_str());
				}

				for (const auto &fn: preFunctions) {
					fn();
				}
				printf("    ");
				if (prettyRet != "void") {
					if (postFunctions.empty()) {
						if (prettyRet[0] != 'j') {
							throw std::runtime_error("bad return map for function " + m.name);
						}
						printf("return %s(", prettyRet.c_str());
					} else {
						printf("auto result = ");
					}
				}
				printf("Call<Index::%s>(", m.name.c_str());
				for (size_t argIdx = 1; argIdx < fn->args.size(); argIdx++) {
					if (argIdx > 1)
						printf(", ");
					const auto &arg = fn->args[argIdx];
					auto itr = argMap.find(argIdx);
					if (itr == argMap.end()) {
						if (arg.type.pretty()[0] != 'j') {
							throw std::runtime_error("bad passthrough " + arg.name + " for function " + m.name);
						}
						printf("%s", fn->args[argIdx].name.c_str());
					} else {
						std::string expr;
						itr->second(expr);
						if (expr.empty()) {
							throw std::runtime_error("bad argument mapper");
						}
						printf("%s", expr.c_str());
					}
				}
				printf(")");
				if (prettyRet != "void") {
					if (postFunctions.empty()) {
						printf(");\n");
					} else {
						printf(";\n");
						for(auto itr = postFunctions.rbegin(); itr != postFunctions.rend(); ++itr) {
							(*itr)();
						}
					}
				}
				else {
					printf(";\n");
					for(auto itr = postFunctions.rbegin(); itr != postFunctions.rend(); ++itr) {
						(*itr)();
					}
				}

				printf("  }\n");
			}

			puts("};\n");

			break;
		}
	}
}

bool DebugParser(const std::string& filename)
{
	std::string data;
	if (!FileHelper::ReadAllText(filename, data))
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

	auto basename = filename;
	auto lastSlashIdx = basename.find_last_of("\\/");
	if(lastSlashIdx != std::string::npos)
	{
		basename = basename.substr(lastSlashIdx + 1);
	}

	FileHelper::WriteAllText(basename + ".pp.h", ppData);

	std::vector<std::string> errors;
	Types::Model model;
	if (!Types::ParseModel(ppData, filename, errors, model))
	{
		puts("Failed to parse types:");
		for (const auto& error : errors)
			puts(error.c_str());
		return false;
	}
	puts("ParseModel success!");

	HandleJNI(model);

	return true;
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		printf("Usage: test my.h\n");
		return EXIT_FAILURE;
	}
	DebugParser(argv[1]);
	return EXIT_SUCCESS;
}