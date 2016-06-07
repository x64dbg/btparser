#pragma once

#include "lexer.h"
#include <memory>

namespace AST
{
    using namespace std;

    template<class T>
    using uptr = unique_ptr<T>;

    template <class T, class... Args>
    static typename enable_if<!is_array<T>::value, unique_ptr<T>>::type make_uptr(Args &&... args)
    {
        return uptr<T>(new T(std::forward<Args>(args)...));
    }

    using Operator = Lexer::Token; //TODO
    using Type = Lexer::Token; //TODO

    class StatDecl //base class for every node
    {
    public:
        virtual ~StatDecl() {}
    };

    class Stat : public StatDecl //statement (expressions, control, block)
    {
    };

    class Block : public Stat //block
    {
        vector<uptr<StatDecl>> mStatDecls;
    public:
        explicit Block(vector<uptr<StatDecl>> statDecls)
            : mStatDecls(move(statDecls)) {}
    };

    class Expr : public Stat //expression
    {
    public:
        virtual ~Expr() {}
    };

    class Return : public Stat
    {
        uptr<Expr> mExpr;
    public:
        explicit Return(uptr<Expr> expr)
            : mExpr(move(expr)) {}
    };

    class Decl : public StatDecl //declaration (variables/types)
    {
    };

    class Builtin : public Decl //built-in declaration (int x)
    {
        Type mType;
        string mId;
    public:
        explicit Builtin(Type type, const string & id)
            : mType(type), mId(id) {}
    };

    class Struct : public Decl //struct (can contain code, not just declarations)
    {
        string mId;
        uptr<Block> mBlock;
    public:
        explicit Struct(const string & id, uptr<Block> block)
            : mId(id), mBlock(move(block)) {}
    };
};