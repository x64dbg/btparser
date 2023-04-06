#include "types.h"
#include "helpers.h"
#include <algorithm>
#include <unordered_set>
#include <ctime>

using namespace Types;

#define EXCLUSIVE_ACQUIRE(x)
#define SHARED_ACQUIRE(x)

TypeManager::TypeManager()
{
    auto p = [this](const std::string & n, Primitive p, int size)
    {
        primitivesizes[p] = size;
        auto splits = StringUtils::Split(n, ',');
        for (const auto& split : splits)
        {
            primitivenames.emplace(p, split);
            addType("", p, split);
        }
    };
    p("int8_t,int8,char,byte,bool,signed char", Int8, sizeof(char));
    p("uint8_t,uint8,uchar,unsigned char,ubyte", Uint8, sizeof(unsigned char));
    p("int16_t,int16,wchar_t,char16_t,short", Int16, sizeof(short));
    p("uint16_t,uint16,ushort,unsigned short", Int16, sizeof(unsigned short));
    p("int32_t,int32,int,long", Int32, sizeof(int));
    p("uint32_t,uint32,unsigned int,unsigned long", Uint32, sizeof(unsigned int));
    p("int64_t,int64,long long", Int64, sizeof(long long));
    p("uint64_t,uint64,unsigned long long", Uint64, sizeof(unsigned long long));
    p("dsint", Dsint, sizeof(void*));
    p("size_t,duint", Duint, sizeof(void*));
    p("float", Float, sizeof(float));
    p("double", Double, sizeof(double));
    p("void*,ptr", Pointer, sizeof(void*));
    p("char*,const char*", PtrString, sizeof(char*));
    p("wchar_t*,const wchar_t*", PtrWString, sizeof(wchar_t*));
}

std::string Types::TypeManager::PrimitiveName(Primitive primitive)
{
    auto found = primitivenames.find(primitive);
    return found == primitivenames.end() ? "" : found->second;
}

bool TypeManager::AddType(const std::string & owner, const std::string & type, const std::string & name)
{
    if(owner.empty())
        return false;
    validPtr(type);
    auto found = types.find(type);
    if(found == types.end())
        return false;
    return addType(owner, found->second.primitive, name);
}

bool Types::TypeManager::AddEnum(const std::string& owner, const std::string& name, const std::string& type)
{
    Enum e;
    e.name = name;
    e.owner = owner;
    auto found = types.find(type);
    if (found == types.end())
        return false;
    if (!found->second.pointto.empty())
        return false;
    switch (found->second.primitive)
    {
    case Void:
    case Float:
    case Double:
    case Pointer:
    case PtrString:
    case PtrWString:
        return false;
    default:
        break;
    }
    e.type = found->second.primitive;
    return addEnum(e);
}

bool TypeManager::AddStruct(const std::string & owner, const std::string & name)
{
    StructUnion s;
    s.name = name;
    s.owner = owner;
    return addStructUnion(s);
}

bool TypeManager::AddUnion(const std::string & owner, const std::string & name)
{
    StructUnion u;
    u.owner = owner;
    u.name = name;
    u.isunion = true;
    return addStructUnion(u);
}

bool TypeManager::AddMember(const std::string & parent, const std::string & type, const std::string & name, int arrsize, int offset)
{
    if(!isDefined(type) && !validPtr(type))
        return false;
    auto found = structs.find(parent);
    if(arrsize < 0 || found == structs.end() || !isDefined(type) || name.empty() || type.empty() || type == parent)
        return false;
    auto & s = found->second;

    for(const auto & member : s.members)
        if(member.name == name)
            return false;

    auto typeSize = Sizeof(type);
    if(arrsize)
        typeSize *= arrsize;

    Member m;
    m.name = name;
    m.arrsize = arrsize;
    m.type = type;
    m.offset = offset;

    if(offset >= 0) //user-defined offset
    {
        if(offset < s.size)
            return false;
        if(offset > s.size)
        {
            Member pad;
            pad.type = "char";
            pad.arrsize = offset - s.size;
            char padname[32] = "";
            sprintf_s(padname, "padding%d", pad.arrsize);
            pad.name = padname;
            s.members.push_back(pad);
            s.size += pad.arrsize;
        }
    }

    s.members.push_back(m);

    if(s.isunion)
    {
        if(typeSize > s.size)
            s.size = typeSize;
    }
    else
    {
        s.size += typeSize;
    }
    return true;
}

bool TypeManager::AppendMember(const std::string & type, const std::string & name, int arrsize, int offset)
{
    return AddMember(laststruct, type, name, arrsize, offset);
}

bool Types::TypeManager::AddEnumerator(const std::string& enumType, const std::string& name, uint64_t value)
{
    auto found = enums.find(enumType);
    if (found == enums.end())
        return false;
    auto& e = found->second;
    for (const auto& v : e.values)
    {
        if (v.name == name || v.value == value)
            return false;
    }
    EnumValue v;
    v.name = name;
    v.value = value;
    e.values.push_back(v);
    return true;
}

bool TypeManager::AddFunction(const std::string & owner, const std::string & name, const std::string & rettype, CallingConvention callconv, bool noreturn, bool typeonly, const QualifiedType& retqtype)
{
    auto found = functions.find(name);
    if(found != functions.end() || name.empty() || owner.empty())
        return false;
    lastfunction = name;
    Function f;
    f.owner = owner;
    f.name = name;
    if(rettype != "void" && !isDefined(rettype) && !validPtr(rettype))
        return false;
    f.rettype = rettype;
    f.callconv = callconv;
    f.noreturn = noreturn;
    f.typeonly = typeonly;
    f.retqtype = retqtype;
    functions.emplace(f.name, f);
    return true;
}

bool TypeManager::AddArg(const std::string & function, const std::string & type, const std::string & name, const QualifiedType& qtype)
{
    auto found = functions.find(function);
    if (found == functions.end() || name.empty())
        return false;
    if(type != "..." && !isDefined(type) && !validPtr(type))
        return false;
    lastfunction = function;
    Member arg;
    arg.name = name;
    arg.type = type;
    arg.qtype = qtype;
    found->second.args.push_back(arg);
    return true;
}

bool TypeManager::AppendArg(const std::string & type, const std::string & name, const QualifiedType& qtype)
{
    return AddArg(lastfunction, type, name, qtype);
}

int TypeManager::Sizeof(const std::string & type) const
{
    auto foundT = types.find(type);
    if(foundT != types.end())
        return foundT->second.size;
    auto foundS = structs.find(type);
    if(foundS != structs.end())
        return foundS->second.size;
    auto foundF = functions.find(type);
    if(foundF != functions.end())
    {
        const auto foundP = primitivesizes.find(Pointer);
        if(foundP != primitivesizes.end())
            return foundP->second;
        return sizeof(void*);
    }
    return 0;
}

bool TypeManager::Visit(const std::string & type, const std::string & name, Visitor & visitor) const
{
    Member m;
    m.name = name;
    m.type = type;
    return visitMember(m, visitor);
}

template<typename K, typename V>
static void filterOwnerMap(std::unordered_map<K, V> & map, const std::string & owner)
{
    for(auto i = map.begin(); i != map.end();)
    {
        auto j = i++;
        if(j->second.owner.empty())
            continue;
        if(owner.empty() || j->second.owner == owner)
            map.erase(j);
    }
}

void TypeManager::Clear(const std::string & owner)
{
    laststruct.clear();
    lastfunction.clear();
    filterOwnerMap(types, owner);
    filterOwnerMap(structs, owner);
    filterOwnerMap(functions, owner);
}

template<typename K, typename V>
static bool removeType(std::unordered_map<K, V> & map, const std::string & type)
{
    auto found = map.find(type);
    if(found == map.end())
        return false;
    if(found->second.owner.empty())
        return false;
    map.erase(found);
    return true;
}

bool TypeManager::RemoveType(const std::string & type)
{
    return removeType(types, type) || removeType(structs, type) || removeType(functions, type);
}

static std::string getKind(const StructUnion & su)
{
    return su.isunion ? "union" : "struct";
}

static std::string getKind(const Type & t)
{
    return "typedef";
}

static std::string getKind(const Function & f)
{
    return "function";
}

template<typename K, typename V>
static void enumType(const TypeManager& typeManager, const std::unordered_map<K, V> & map, std::vector<TypeManager::Summary> & types)
{
    for(auto i = map.begin(); i != map.end(); ++i)
    {
        TypeManager::Summary s;
        s.kind = getKind(i->second);
        s.name = i->second.name;
        s.owner = i->second.owner;
        s.size = typeManager.Sizeof(s.name);
        types.push_back(s);
    }
}

void TypeManager::Enumerate(std::vector<Summary> & typeList) const
{
    typeList.clear();
    enumType(*this, types, typeList);
    enumType(*this, structs, typeList);
    enumType(*this, functions, typeList);
    //nasty hacks to sort in a nice way
    std::sort(typeList.begin(), typeList.end(), [](const Summary & a, const Summary & b)
    {
        auto kindInt = [](const std::string & kind)
        {
            if(kind == "typedef")
                return 0;
            if(kind == "struct")
                return 1;
            if(kind == "union")
                return 2;
            if(kind == "function")
                return 3;
            __debugbreak();
            return 4;
        };
        if(a.owner < b.owner)
            return true;
        else if(a.owner > b.owner)
            return false;
        auto ka = kindInt(a.kind), kb = kindInt(b.kind);
        if(ka < kb)
            return true;
        else if(ka > kb)
            return false;
        if(a.name < b.name)
            return true;
        else if(a.name > b.name)
            return false;
        return a.size < b.size;
    });
}

std::string Types::TypeManager::StructUnionPtrType(const std::string & pointto) const
{
    auto itr = structs.find(pointto);
    if(itr == structs.end())
        return "";
    return getKind(itr->second);
}

template<typename K, typename V>
static bool mapContains(const std::unordered_map<K, V> & map, const K & k)
{
    return map.find(k) != map.end();
}

bool TypeManager::isDefined(const std::string & id) const
{
    return mapContains(types, id) || mapContains(structs, id) || mapContains(enums, id) || mapContains(functions, id);
}

bool TypeManager::validPtr(const std::string & id)
{
    if(id[id.length() - 1] == '*')
    {
        auto type = id.substr(0, id.length() - 1);
        if(!isDefined(type) && !validPtr(type))
            return false;
        std::string owner("ptr");
        auto foundT = types.find(type);
        if(foundT != types.end())
            owner = foundT->second.owner;
        auto foundS = structs.find(type);
        if(foundS != structs.end())
            owner = foundS->second.owner;
        return addType(owner, Pointer, id, type);
    }
    return false;
}

bool TypeManager::addStructUnion(const StructUnion & s)
{
    laststruct = s.name;
    if(s.owner.empty() || s.name.empty() || isDefined(s.name))
        return false;
    return structs.emplace(s.name, s).second;
}

bool Types::TypeManager::addEnum(const Enum& e)
{
    lastenum = e.name;
    if (e.owner.empty() || e.name.empty() || isDefined(e.name))
        return false;
    return enums.emplace(e.name, e).second;
}

bool TypeManager::addType(const Type & t)
{
    if(t.name.empty() || isDefined(t.name))
        return false;
    return types.emplace(t.name, t).second;
}

bool TypeManager::addType(const std::string & owner, Primitive primitive, const std::string & name, const std::string & pointto)
{
    if(name.empty() || isDefined(name))
        return false;
    Type t;
    t.owner = owner;
    t.name = name;
    t.primitive = primitive;
    t.size = primitivesizes[primitive];
    t.pointto = pointto;
    return addType(t);
}

bool TypeManager::visitMember(const Member & root, Visitor & visitor) const
{
    auto foundT = types.find(root.type);
    if(foundT != types.end())
    {
        const auto & t = foundT->second;
        if(!t.pointto.empty())
        {
            if(!isDefined(t.pointto))
                return false;
            if(visitor.visitPtr(root, t)) //allow the visitor to bail out
            {
                if(!Visit(t.pointto, "*" + root.name, visitor))
                    return false;
                return visitor.visitBack(root);
            }
            return true;
        }
        return visitor.visitType(root, t);
    }
    auto foundS = structs.find(root.type);
    if(foundS != structs.end())
    {
        const auto & s = foundS->second;
        if(!visitor.visitStructUnion(root, s))
            return false;
        for(const auto & child : s.members)
        {
            if(child.arrsize)
            {
                if(!visitor.visitArray(child))
                    return false;
                for(auto i = 0; i < child.arrsize; i++)
                    if(!visitMember(child, visitor))
                        return false;
                if(!visitor.visitBack(child))
                    return false;
            }
            else if(!visitMember(child, visitor))
                return false;
        }
        return visitor.visitBack(root);
    }
    return false;
}

bool TypeManager::GenerateStubs() const
{
    std::unordered_map<std::string, const Function*> fnptrs;
    std::unordered_set<std::string> declared;
    auto formatFunctionPointer = [this](const std::string& name, const Function& fn)
    {
        std::string r = fn.retqtype.pretty();
        r += " (*";
        r += name;
        r += ")(";
        for(size_t i = 0; i < fn.args.size(); i++)
        {
            const auto& arg = fn.args[i];
            if(functions.count(arg.type) != 0)
                __debugbreak();

            r += arg.qtype.pretty();
            if(i + 1 < fn.args.size())
            {
                r += ", ";
            }
        }
        r += ")";
        return r;
    };

    printf("// THIS WAS AUTOGENERATED YOU MORONS %ju\n", time(nullptr));
    puts("#include \"hooks.h\"");
    puts("");

    std::vector<std::string> hooks;
    for(const auto& itr : functions)
    {
        const auto &fn = itr.second;
        // Skip non-declarations
        if (fn.typeonly)
        {
            fnptrs.emplace(fn.name, &fn);
            continue;
        }

        auto variadic = false;
        for (const auto &arg: fn.args)
        {
            if (arg.type == "...")
            {
                variadic = true;
                break;
            }
        }
        if (variadic)
        {
            printf("// Skipping variadic function %s\n", fn.name.c_str());
            continue;
        }

        hooks.push_back(fn.name);
    }
    puts("");

    for(const auto& hook : hooks)
    {
        printf("static decltype(&%s) orig_%s;\n", hook.c_str(), hook.c_str());
    }

    for(const auto& hook : hooks)
    {
        const auto& fn = functions.at(hook);
        puts("");
        printf("static ");
        printf("%s", fn.retqtype.pretty().c_str());
        printf(" hook_%s(\n", fn.name.c_str());
        std::vector<std::string> argtypes;
        for(size_t i = 0; i < fn.args.size(); i++)
        {
            const auto& arg = fn.args[i];
            printf("    ");
            auto argPtr = functions.find(arg.type);
            if(argPtr != functions.end())
            {
                argtypes.push_back(formatFunctionPointer(arg.name, argPtr->second));
                printf("%s", argtypes.back().c_str());
                if(arg.qtype.isConst)
                    __debugbreak();
            }
            else
            {
                argtypes.push_back(arg.type);
                printf("%s", arg.qtype.pretty().c_str());
                printf(" ");
                printf("%s", arg.name.c_str());
            }
            if(i + 1 < fn.args.size())
            {
                printf(",");
            }
            puts("");
        }
        puts(")");
        puts("{");
        printf("    LOG_CALL(\"%s\");\n", fn.name.c_str());
        for(size_t i = 0; i < fn.args.size(); i++)
        {
            printf("    LOG_ARGUMENT(\"%s\", %s);\n", argtypes[i].c_str(), fn.args[i].name.c_str());
        }
        if(fn.rettype == "void")
        {
            printf("    orig_%s(", fn.name.c_str());
        }
        else
        {
            printf("    auto _hook_result = orig_%s(", fn.name.c_str());
        }
        for(size_t i = 0; i < fn.args.size(); i++)
        {
            const auto& arg = fn.args[i];
            printf("%s", arg.name.c_str());
            if(i + 1 < fn.args.size())
            {
                printf(", ");
            }
        }
        puts(");");
        if(fn.rettype == "void")
        {
            puts("    LOG_RETURN_VOID();");
        }
        else
        {
            puts("    LOG_RETURN(_hook_result);");
            puts("    return _hook_result;");
        }
        puts("}");
    }

    puts("");
    puts("void InstallHooks()");
    puts("{");
    for(const auto& hook : hooks)
    {
        printf("    HOOK(%s);\n", hook.c_str());
    }
    puts("}");

    return false;
}
