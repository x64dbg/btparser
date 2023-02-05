#include "types.h"
#include "helpers.h"

using namespace Types;

#include "lexer.h"

void LoadModel(const std::string& owner, Model& model);

bool ParseTypes(const std::string& parse, const std::string& owner, std::vector<std::string>& errors)
{
	Lexer lexer;
	lexer.SetInputData(parse);
	std::vector<Lexer::TokenState> tokens;
	size_t index = 0;
	auto getToken = [&](size_t i) -> Lexer::TokenState&
	{
		if (index >= tokens.size() - 1)
			i = tokens.size() - 1;
		return tokens[i];
	};
	auto curToken = [&]() -> Lexer::TokenState&
	{
		return getToken(index);
	};
	auto isToken = [&](Lexer::Token token)
	{
		return getToken(index).Token == token;
	};
	auto isTokenList = [&](std::initializer_list<Lexer::Token> il)
	{
		size_t i = 0;
		for (auto l : il)
			if (getToken(index + i++).Token != l)
				return false;
		return true;
	};
	std::string error;
	if (!lexer.DoLexing(tokens, error))
	{
		errors.push_back(error);
		return false;
	}
	Model model;

	auto errLine = [&](const Lexer::TokenState& token, const std::string& message)
	{
		errors.push_back(StringUtils::sprintf("[line %zu:%zu] %s", token.CurLine + 1, token.LineIndex, message.c_str()));
	};
	auto eatSemic = [&]()
	{
		while (curToken().Token == Lexer::tok_semic)
			index++;
	};
	auto parseMember = [&](StructUnion& su)
	{
		std::vector<Lexer::TokenState> memToks;
		while (!isToken(Lexer::tok_semic))
		{
			if (isToken(Lexer::tok_eof))
			{
				errLine(curToken(), "unexpected eof in member");
				return false;
			}
			memToks.push_back(curToken());
			index++;
		}
		if (memToks.empty())
		{
			errLine(curToken(), "unexpected ; in member");
			return false;
		}
		eatSemic();
		if (memToks.size() >= 2) //at least type name;
		{
			Member m;
			for (size_t i = 0; i < memToks.size(); i++)
			{
				const auto& t = memToks[i];
				if (t.Token == Lexer::tok_subopen)
				{
					if (i + 1 >= memToks.size())
					{
						errLine(memToks.back(), "unexpected end after [");
						return false;
					}
					if (memToks[i + 1].Token != Lexer::tok_number)
					{
						errLine(memToks[i + 1], "expected number token");
						return false;
					}
					m.arrsize = int(memToks[i + 1].NumberVal);
					if (i + 2 >= memToks.size())
					{
						errLine(memToks.back(), "unexpected end, expected ]");
						return false;
					}
					if (memToks[i + 2].Token != Lexer::tok_subclose)
					{
						errLine(memToks[i + 2], StringUtils::sprintf("expected ], got %s", lexer.TokString(memToks[i + 2]).c_str()));
						return false;
					}
					if (i + 2 != memToks.size() - 1)
					{
						errLine(memToks[i + 3], "too many tokens");
						return false;
					}
					break;
				}
				else if (i + 1 == memToks.size() || memToks[i + 1].Token == Lexer::tok_subopen) //last = name
				{
					m.name = lexer.TokString(memToks[i]);
				}
				else if (!t.IsType() &&
					t.Token != Lexer::tok_op_mul &&
					t.Token != Lexer::tok_identifier &&
					t.Token != Lexer::tok_void)
				{
					errLine(t, StringUtils::sprintf("token %s is not a type...", lexer.TokString(t).c_str()));
					return false;
				}
				else
				{
					if (!m.type.empty() && t.Token != Lexer::tok_op_mul)
						m.type.push_back(' ');
					m.type += lexer.TokString(t);
				}
			}
			//dprintf("member: %s %s;\n", m.type.c_str(), m.name.c_str());
			su.members.push_back(m);
			return true;
		}
		errLine(memToks.back(), "not enough tokens for member");
		return false;
	};
	auto parseStructUnion = [&]()
	{
		if (isToken(Lexer::tok_struct) || isToken(Lexer::tok_union))
		{
			StructUnion su;
			su.isunion = isToken(Lexer::tok_union);
			index++;
			if (isTokenList({ Lexer::tok_identifier, Lexer::tok_bropen }))
			{
				su.name = lexer.TokString(curToken());
				index += 2;
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
				index++; //eat tok_brclose
				//dprintf("%s %s, members: %d\n", su.isunion ? "union" : "struct", su.name.c_str(), int(su.members.size()));
				model.structUnions.push_back(su);
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
				errLine(curToken(), "invalid struct token sequence!");
				return false;
			}
		}
		return true;
	};
	auto parseEnum = [&]()
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
					v.value = e.values.empty() ? 0 : e.values.back().value + 1;
					e.values.push_back(v);

					index++;
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
	};
	auto parseTypedef = [&]()
	{
		// TODO: support "typedef struct"
		if (isToken(Lexer::tok_typedef))
		{
			index++;
			std::vector<Lexer::TokenState> tdefToks;
			while (!isToken(Lexer::tok_semic))
			{
				if (isToken(Lexer::tok_eof))
				{
					errLine(curToken(), "unexpected eof in typedef");
					return false;
				}
				tdefToks.push_back(curToken());
				index++;
			}
			if (tdefToks.empty())
			{
				errLine(curToken(), "unexpected ; in typedef");
				return false;
			}
			eatSemic();
			if (tdefToks.size() >= 2) //at least typedef a b;
			{
				Member tm;
				tm.name = lexer.TokString(tdefToks[tdefToks.size() - 1]);
				tdefToks.pop_back();
				for (auto& t : tdefToks)
					if (!t.IsType() &&
						t.Token != Lexer::tok_op_mul &&
						t.Token != Lexer::tok_identifier &&
						t.Token != Lexer::tok_void)
					{
						errLine(t, StringUtils::sprintf("token %s is not a type...", lexer.TokString(t).c_str()));
						return false;
					}
					else
					{
						if (!tm.type.empty() && t.Token != Lexer::tok_op_mul)
							tm.type.push_back(' ');
						tm.type += lexer.TokString(t);
					}
				//dprintf("typedef %s:%s\n", tm.type.c_str(), tm.name.c_str());
				model.types.push_back(tm);
				return true;
			}
			errLine(tdefToks.back(), "not enough tokens for typedef");
			return false;
		}
		return true;
	};

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
			errLine(curToken(), StringUtils::sprintf("unexpected token %s", lexer.TokString(curToken()).c_str()));
			return false;
		}
	}

	LoadModel(owner, model);

	return true;
}