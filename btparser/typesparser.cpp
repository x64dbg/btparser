#include "types.h"
#include "helpers.h"

using namespace Types;

#include "lexer.h"

void LoadModel(const std::string& owner, Model& model);

struct Parser
{
	Lexer lexer;
	std::string owner;
	std::vector<std::string>& errors;

	std::vector<Lexer::TokenState> tokens;
	size_t index = 0;
	Model model;

	Parser(const std::string& code, const std::string& owner, std::vector<std::string>& errors)
		: owner(owner), errors(errors)
	{
		lexer.SetInputData(code);
	}

	Lexer::TokenState& getToken(size_t i)
	{
		if (index >= tokens.size() - 1)
			i = tokens.size() - 1;
		return tokens[i];
	}

	Lexer::TokenState& curToken()
	{
		return getToken(index);
	}

	bool isToken(Lexer::Token token)
	{
		return getToken(index).Token == token;
	}

	bool isTokenList(std::initializer_list<Lexer::Token> il)
	{
		size_t i = 0;
		for (auto l : il)
			if (getToken(index + i++).Token != l)
				return false;
		return true;
	}

	void errLine(const Lexer::TokenState& token, const std::string& message)
	{
		auto error = StringUtils::sprintf("[line %zu:%zu] %s", token.CurLine + 1, token.LineIndex, message.c_str());
		errors.push_back(std::move(error));
	}

	void eatSemic()
	{
		while (curToken().Token == Lexer::tok_semic)
			index++;
	}

	bool parseVariable(const std::vector<Lexer::TokenState>& tlist, std::string& type, bool& isConst, std::string& name)
	{
		type.clear();
		isConst = false;
		name.clear();

		bool sawPointer = false;
		bool isKeyword = true;
		size_t i = 0;
		for (; i < tlist.size(); i++)
		{
			const auto& t = tlist[i];
			if (t.Is(Lexer::tok_const))
			{
				isConst = true;
				continue;
			}

			auto isType = t.IsType();
			if (!isType)
			{
				isKeyword = false;
			}

			if (isType)
			{
				if (isKeyword)
				{
					if (!type.empty())
						type += ' ';
					type += lexer.TokString(t);
				}
				else
				{
					errLine(t, "invalid keyword in type");
					return false;
				}
			}
			else if (t.Is(Lexer::tok_identifier))
			{
				if (type.empty())
				{
					type = t.IdentifierStr;
				}
				else if (i + 1 == tlist.size())
				{
					name = t.IdentifierStr;
				}
				else
				{
					errLine(t, "invalid identifier in type");
					return false;
				}
			}
			else if (t.Is(Lexer::tok_op_mul))
			{
				if (type.empty())
				{
					errLine(t, "unexpected * in type");
					return false;
				}

				if (sawPointer && type.back() != '*')
				{
					errLine(t, "unexpected * in type");
					return false;
				}

				// Apply the pointer to the type on the left
				type += '*';
				sawPointer = true;
			}
			else
			{
				errLine(t, "invalid token in type");
				return false;
			}
		}
		if (type.empty())
			__debugbreak();
		return true;
	}

	bool parseFunction(std::vector<Lexer::TokenState>& rettypes, Function& fn, bool ptr)
	{
		if (rettypes.empty())
		{
			errLine(curToken(), "expected return type before function pointer type");
			return false;
		}

		// TODO: calling conventions

		std::string retname;
		bool retconst = false;
		if (!parseVariable(rettypes, fn.rettype, retconst, retname))
			return false;

		if (ptr)
		{
			if (!retname.empty())
			{
				errLine(rettypes.back(), "invalid return type in function pointer");
				return false;
			}

			if (!isToken(Lexer::tok_op_mul))
			{
				errLine(curToken(), "expected * in function pointer type");
				return false;
			}
			index++;

			if (!isToken(Lexer::tok_identifier))
			{
				errLine(curToken(), "expected identifier in function pointer type");
				return false;
			}
			fn.name = lexer.TokString(curToken());
			index++;

			if (!isToken(Lexer::tok_parclose))
			{
				errLine(curToken(), "expected ) after function pointer type name");
				return false;
			}
			index++;

			if (!isToken(Lexer::tok_paropen))
			{
				errLine(curToken(), "expected ( for start of parameter list in function pointer type");
				return false;
			}
			index++;
		}
		else if (retname.empty())
		{
			errLine(rettypes.back(), "function name cannot be empty");
			return false;
		}
		else
		{
			fn.name = retname;
		}

		std::vector<Lexer::TokenState> tlist;
		auto startToken = curToken();
		auto finalizeArgument = [&]()
		{
			Member am;
			if (!parseVariable(tlist, am.type, am.isConst, am.name))
				return false;
			fn.args.push_back(am);
			tlist.clear();
			startToken = curToken();
			return true;
		};
		while (!isToken(Lexer::tok_parclose))
		{
			if (isToken(Lexer::tok_comma))
			{
				index++;
				if (!finalizeArgument())
					return false;
			}

			const auto& t = curToken();
			if (t.IsType() || t.Is(Lexer::tok_identifier) || t.Is(Lexer::tok_const))
			{
				index++;

				// Primitive type
				tlist.push_back(t);
			}
			else if (t.Is(Lexer::tok_op_mul))
			{
				// Pointer to the type on the left
				if (tlist.empty())
				{
					errLine(curToken(), "unexpected * in function type argument list");
					return false;
				}
				index++;

				tlist.push_back(t);
			}
			else if (isTokenList({ Lexer::tok_subopen, Lexer::tok_subclose }))
			{
				if (tlist.empty())
				{
					errLine(curToken(), "unexpected [ in function type argument list");
					return false;
				}
				index += 2;

				Lexer::TokenState fakePtr;
				fakePtr.Token = Lexer::tok_op_mul;
				fakePtr.CurLine = t.CurLine;
				fakePtr.LineIndex = t.LineIndex;
				if (tlist.size() > 1 && tlist.back().Is(Lexer::tok_identifier))
				{
					tlist.insert(tlist.end() - 1, fakePtr);
				}
				else
				{
					tlist.push_back(fakePtr);
				}
			}
			else if (t.Is(Lexer::tok_varargs))
			{
				if (!tlist.empty())
				{
					errLine(t, "unexpected ... in function type argument list");
					return false;
				}

				index++;
				if (!isToken(Lexer::tok_parclose))
				{
					errLine(curToken(), "expected ) after ... in function type argument list");
					return false;
				}

				Member am;
				am.type = "...";
				fn.args.push_back(am);
				break;
			}
			else if (t.Is(Lexer::tok_paropen))
			{
				errLine(curToken(), "function pointer arguments are not supported");
				// TODO: support function pointers (requires recursion)
				return false;
			}
			else
			{
				errLine(curToken(), "unsupported token in function type argument list");
				return false;
			}
		}
		index++;

		if (tlist.empty())
		{
			// Do nothing
		}
		else if (tlist.size() == 1 && tlist[0].Token == Lexer::tok_void)
		{
			if (!fn.args.empty())
			{
				errLine(tlist[0], "invalid argument type: void");
				return false;
			}
			return true;
		}
		else if (!finalizeArgument())
		{
			return false;
		}

		if (!isToken(Lexer::tok_semic))
		{
			errLine(curToken(), "expected ; after function type");
			return false;
		}
		eatSemic();

		return true;
	}

	bool parseMember(StructUnion& su)
	{
		Member m;
		bool sawPointer = false;
		std::vector<Lexer::TokenState> tlist;
		auto startToken = curToken();

		auto finalizeMember = [&]()
		{
			if (tlist.size() < 2)
			{
				errLine(startToken, "not enough tokens in member");
				return false;
			}

			if (!parseVariable(tlist, m.type, m.isConst, m.name))
				return false;

			if (m.type == "void")
			{
				errLine(startToken, "void is not a valid member type");
				return false;
			}

			if (m.type.empty() || m.name.empty())
				__debugbreak();

			su.members.push_back(m);
			return true;
		};

		while (!isToken(Lexer::tok_semic))
		{
			if (isToken(Lexer::tok_eof))
			{
				errLine(curToken(), "unexpected eof in member");
				return false;
			}

			if (isToken(Lexer::tok_struct) || isToken(Lexer::tok_union) || isToken(Lexer::tok_enum))
			{
				if (tlist.empty() && getToken(index + 1).Token == Lexer::tok_identifier)
				{
					index++;
				}
				else
				{
					errLine(curToken(), "unsupported struct/union/enum in member");
					return false;
				}
			}

			const auto& t = curToken();
			if (t.IsType() || t.Is(Lexer::tok_identifier) || t.Is(Lexer::tok_const))
			{
				index++;
				// Primitive type / name
				tlist.push_back(t);
			}
			else if (t.Is(Lexer::tok_op_mul))
			{
				// Pointer to the type on the left
				if (tlist.empty())
				{
					errLine(curToken(), "unexpected * in member");
					return false;
				}

				if (sawPointer && tlist.back().Token != Lexer::tok_op_mul)
				{
					errLine(curToken(), "unexpected * in member");
					return false;
				}

				index++;

				tlist.push_back(t);
				sawPointer = true;
			}
			else if (t.Is(Lexer::tok_subopen))
			{
				index++;

				// Array
				if (!isToken(Lexer::tok_number))
				{
					errLine(curToken(), "expected number token after array");
					return false;
				}
				m.arrsize = (int)curToken().NumberVal;
				index++;

				if (!isToken(Lexer::tok_subclose))
				{
					errLine(curToken(), "expected ] after array size");
					return false;
				}
				index++;

				break;
			}
			else if (t.Is(Lexer::tok_paropen))
			{
				index++;

				// Function pointer type
				Function fn;
				if (!parseFunction(tlist, fn, true))
				{
					return false;
				}
				// TODO: put the function somewhere

				printf("TODO function pointer: %s\n", fn.name.c_str());

				return true;
			}
			else if (t.Is(Lexer::tok_comma))
			{
				// Comma-separated members
				index++;

				if (!finalizeMember())
					return false;

				// Remove the name from the type
				if (tlist.back().Token != Lexer::tok_identifier)
					__debugbreak();
				tlist.pop_back();

				// Remove the pointer from the type
				while (!tlist.empty() && tlist.back().Token == Lexer::tok_op_mul)
					tlist.pop_back();
				sawPointer = false;

				m = Member();
			}
			else
			{
				__debugbreak();
			}
		}

		if (!isToken(Lexer::tok_semic))
		{
			errLine(curToken(), "expected ; after member");
			return false;
		}
		eatSemic();

		if (!finalizeMember())
			return false;

		return true;
	}

	bool parseStructUnion()
	{
		if (isToken(Lexer::tok_struct) || isToken(Lexer::tok_union))
		{
			StructUnion su;
			su.isunion = isToken(Lexer::tok_union);
			index++;
			if (!isToken(Lexer::tok_identifier))
			{
				errLine(curToken(), "expected identifier after struct");
				return false;
			}
			su.name = lexer.TokString(curToken());
			index++;
			if (isToken(Lexer::tok_bropen))
			{
				index++;
				while (!isToken(Lexer::tok_brclose))
				{
					if (isToken(Lexer::tok_eof))
					{
						errLine(curToken(), StringUtils::sprintf("unexpected eof in %s", su.isunion ? "union" : "struct"));
						return false;
					}
					if (isToken(Lexer::tok_bropen))
					{
						errLine(curToken(), "nested blocks are not allowed!");
						return false;
					}
					if (!parseMember(su))
						return false;
				}
				index++;

				model.structUnions.push_back(su);
				if (!isToken(Lexer::tok_semic))
				{
					errLine(curToken(), "expected semicolon!");
					return false;
				}
				eatSemic();
				return true;
			}
			else if (isToken(Lexer::tok_semic))
			{
				// Forward declaration
				model.structUnions.push_back(su);
				eatSemic();
				return true;
			}
			else
			{
				errLine(curToken(), "invalid struct token sequence!");
				return false;
			}
		}
		return true;
	}

	bool parseEnum()
	{
		if (isToken(Lexer::tok_enum))
		{
			Enum e;
			index++;
			if (isTokenList({ Lexer::tok_identifier, Lexer::tok_bropen }))
			{
				e.name = lexer.TokString(curToken());
				index += 2;

				while (!isToken(Lexer::tok_brclose))
				{
					if (isToken(Lexer::tok_eof))
					{
						errLine(curToken(), "unexpected eof in enum");
						return false;
					}
					if (isToken(Lexer::tok_bropen))
					{
						errLine(curToken(), "nested blocks are not allowed!");
						return false;
					}

					if (!e.values.empty())
					{
						if (isToken(Lexer::tok_comma))
						{
							index++;
							if (isToken(Lexer::tok_brclose))
							{
								// Support final comma
								break;
							}
						}
						else
						{
							errLine(curToken(), "expected comma in enum");
							return false;
						}
					}

					if (!isToken(Lexer::tok_identifier))
					{
						errLine(curToken(), StringUtils::sprintf("expected identifier in enum, got '%s'", lexer.TokString(curToken()).c_str()));
						return false;
					}

					EnumValue v;
					v.name = lexer.TokString(curToken());
					index++;

					if (isToken(Lexer::tok_assign))
					{
						bool negative = false;
						index++;
						if (isToken(Lexer::tok_op_min))
						{
							index++;
							negative = true;
						}
						if (!isToken(Lexer::tok_number))
						{
							errLine(curToken(), "expected number after = in enum");
							return false;
						}
						v.value = curToken().NumberVal;
						if (negative)
						{
							v.value = -(int64_t)v.value;
						}
						index++;
					}
					else
					{
						v.value = e.values.empty() ? 0 : e.values.back().value + 1;
					}
					e.values.push_back(v);
				}
				index++; //eat tok_brclose

				model.enums.push_back(e);
				if (!isToken(Lexer::tok_semic))
				{
					errLine(curToken(), "expected semicolon!");
					return false;
				}
				eatSemic();
				return true;
			}
			else
			{
				errLine(curToken(), "invalid enum token sequence!");
				return false;
			}
			__debugbreak();
		}
		return true;
	}

	bool parseTypedef()
	{
		// TODO: support "typedef struct foo { members... };"
		// TODO: support "typedef enum foo { members... };"

		if (isToken(Lexer::tok_typedef))
		{
			index++;

			auto startToken = curToken();

			bool sawPointer = false;
			std::vector<Lexer::TokenState> tlist;
			while (!isToken(Lexer::tok_semic))
			{
				if (isToken(Lexer::tok_eof))
				{
					errLine(curToken(), "unexpected eof in typedef");
					return false;
				}

				if (isToken(Lexer::tok_struct) || isToken(Lexer::tok_union) || isToken(Lexer::tok_enum))
				{
					if (tlist.empty() && getToken(index + 1).Token == Lexer::tok_identifier)
					{
						index++;
					}
					else
					{
						errLine(curToken(), "unsupported struct/union/enum in typedef");
						return false;
					}
				}

				const auto& t = curToken();
				if (t.IsType() || t.Token == Lexer::tok_identifier || t.Token == Lexer::tok_const)
				{
					// Primitive type
					index++;
					tlist.push_back(t);
				}
				else if (t.Token == Lexer::tok_op_mul)
				{
					// Pointer to the type on the left
					if (tlist.empty())
					{
						errLine(curToken(), "unexpected * in member");
						return false;
					}

					if (sawPointer && tlist.back().Token != Lexer::tok_op_mul)
					{
						errLine(curToken(), "unexpected * in member");
						return false;
					}

					tlist.push_back(t);
					sawPointer = true;

					index++;
				}
				else if (t.Token == Lexer::tok_paropen)
				{
					// Function pointer type

					index++;

					Function fn;
					if (!parseFunction(tlist, fn, true))
					{
						return false;
					}
					// TODO: put the function somewhere

					printf("TODO function pointer: %s\n", fn.name.c_str());

					return true;
				}
				else
				{
					__debugbreak();
				}
			}
			eatSemic();

			if (tlist.size() < 2)
			{
				errLine(startToken, "not enough tokens in typedef");
				return false;
			}

			Member tm;
			if (!parseVariable(tlist, tm.type, tm.isConst, tm.name))
				return false;
			model.types.push_back(tm);
		}
		return true;
	}

	bool parseFunctionTop()
	{
		bool sawPointer = false;
		std::vector<Lexer::TokenState> tlist;
		while (!isToken(Lexer::tok_semic))
		{
			if (isToken(Lexer::tok_eof))
			{
				errLine(curToken(), "unexpected eof in function");
				return false;
			}

			if (isToken(Lexer::tok_struct) || isToken(Lexer::tok_union) || isToken(Lexer::tok_enum))
			{
				if (tlist.empty() && getToken(index + 1).Token == Lexer::tok_identifier)
				{
					index++;
				}
				else
				{
					errLine(curToken(), "unexpected struct/union/enum in function");
					return false;
				}
			}

			const auto& t = curToken();
			if (t.IsType() || t.Is(Lexer::tok_identifier) || t.Is(Lexer::tok_const))
			{
				index++;
				// Primitive type / name
				tlist.push_back(t);
			}
			else if (t.Is(Lexer::tok_op_mul))
			{
				// Pointer to the type on the left
				if (tlist.empty())
				{
					errLine(curToken(), "unexpected * in function");
					return false;
				}

				if (sawPointer && tlist.back().Token != Lexer::tok_op_mul)
				{
					errLine(curToken(), "unexpected * in function");
					return false;
				}

				index++;

				tlist.push_back(t);
				sawPointer = true;
			}
			else if (t.Is(Lexer::tok_paropen))
			{
				index++;

				// Function pointer type
				Function fn;
				if (!parseFunction(tlist, fn, false))
				{
					return false;
				}
				// TODO: put the function somewhere

				printf("TODO function: %s\n", fn.name.c_str());

				return true;
			}
			else
			{
				__debugbreak();
			}
		}
		return false;
	}

	bool operator()()
	{
		std::string error;
		if (!lexer.DoLexing(tokens, error))
		{
			errors.push_back(error);
			return false;
		}

		while (!isToken(Lexer::tok_eof))
		{
			auto curIndex = index;
			if (!parseTypedef())
				return false;
			if (!parseStructUnion())
				return false;
			if (!parseEnum())
				return false;
			eatSemic();
			if (curIndex == index)
			{
				if (!parseFunctionTop())
					return false;
				else
					continue;
			}
		}

		LoadModel(owner, model);
		return true;
	}
};

bool ParseTypes(const std::string& code, const std::string& owner, std::vector<std::string>& errors)
{
	return Parser(code, owner, errors)();
}