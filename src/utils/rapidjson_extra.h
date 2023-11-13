#ifndef RAPIDJSON_EXTRA_H_INCLUDED
#define RAPIDJSON_EXTRA_H_INCLUDED

#include <stdexcept>

template <typename T> void exception_thrower(T e, const std::string &cond, const std::string &file, int line)
{
    if(!e)
        throw std::runtime_error("rapidjson assertion failed: " + cond + " (" + file + ":" + std::to_string(line) + ")");
}

#ifdef RAPIDJSON_ASSERT
#undef RAPIDJSON_ASSERT
#endif // RAPIDJSON_ASSERT
#define VALUE(x) #x
#define RAPIDJSON_ASSERT(x) exception_thrower(x, VALUE(x), __FILE__, __LINE__)
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>
#include <string>

inline void operator >> (const rapidjson::Value &value, std::string &i)
{
    if(value.IsNull())
        i = "";
    else if(value.IsString())
        i = std::string(value.GetString());
    else if(value.IsInt64())
        i = std::to_string(value.GetInt64());
    else if(value.IsBool())
        i = value.GetBool() ? "true" : "false";
    else if(value.IsDouble())
        i = std::to_string(value.GetDouble());
    else
        i = "";
}

inline void operator >> (const rapidjson::Value &value, int &i)
{
    if(value.IsNull())
        i = 0;
    else if(value.IsInt())
        i = value.GetInt();
    else if(value.IsString())
        i = std::stoi(value.GetString());
    else if(value.IsBool())
        i = value.GetBool() ? 1 : 0;
    else
        i = 0;
}

inline std::string GetMember(const rapidjson::Value &value, const std::string &member)
{
    std::string retStr;
    if(value.IsObject() && value.HasMember(member.data()))
        value[member.data()] >> retStr;
    return retStr;
}

inline void GetMember(const rapidjson::Value &value, const std::string &member, std::string &target)
{
    std::string retStr = GetMember(value, member);
    if(!retStr.empty())
        target.assign(retStr);
}

template <typename ...Args>
inline rapidjson::Value buildObject(rapidjson::MemoryPoolAllocator<> &allocator, Args... kvs)
{
    static_assert(sizeof...(kvs) % 2 == 0, "buildObject requires an even number of arguments");
    static_assert((std::is_same<Args, const char*>::value && ...), "buildObject requires all arguments to be const char*");
    rapidjson::Value ret(rapidjson::kObjectType);
    auto args = {kvs...};
    auto it = args.begin();
    while (it != args.end())
    {
        const char *key = *it++, *value = *it++;
        ret.AddMember(rapidjson::StringRef(key), rapidjson::StringRef(value), allocator);
    }
    return ret;
}

inline rapidjson::Value buildBooleanValue(bool value)
{
    return value ? rapidjson::Value(rapidjson::kTrueType) : rapidjson::Value(rapidjson::kFalseType);
}

namespace rapidjson_ext {
    template <typename ReturnType>
    struct ExtensionFunction {
        virtual ReturnType operator() (rapidjson::Value &root) const = 0;
        virtual ReturnType operator() (rapidjson::Value &&root) const
        {
            return (*this)(root);
        };

        friend ReturnType operator| (rapidjson::Value &root, const ExtensionFunction<ReturnType> &func)
        {
            return func(root);
        }

        friend ReturnType operator| (rapidjson::Value &&root, const ExtensionFunction<ReturnType> &func)
        {
            return func(root);
        }
    };

    struct AddMemberOrReplace : public ExtensionFunction<rapidjson::Value &> {
        rapidjson::Value &value;
        const rapidjson::Value::Ch *name;
        rapidjson::MemoryPoolAllocator<> &allocator;
        AddMemberOrReplace(const rapidjson::Value::Ch *name, rapidjson::Value &value,
                           rapidjson::MemoryPoolAllocator<> &allocator) : value(value), name(name), allocator(allocator) {}
        AddMemberOrReplace(const rapidjson::Value::Ch *name, rapidjson::Value &&value,
                           rapidjson::MemoryPoolAllocator<> &allocator) : value(value), name(name), allocator(allocator) {}

        inline rapidjson::Value & operator() (rapidjson::Value &root) const override
        {
            if (root.HasMember(name))
                root[name] = value;
            else
                root.AddMember(rapidjson::Value(name, allocator), value, allocator);
            return root;
        }
    };

    struct AppendToArray : public ExtensionFunction<rapidjson::Value &>
    {
        rapidjson::Value &value;
        rapidjson::GenericValue<rapidjson::UTF8<>> name;
        rapidjson::MemoryPoolAllocator<> &allocator;

        AppendToArray(const rapidjson::Value::Ch *name, rapidjson::Value &value,
                      rapidjson::MemoryPoolAllocator<> &allocator): value(value), name(rapidjson::Value(name, allocator)), allocator(allocator) {}

        AppendToArray(const rapidjson::Value::Ch *name, rapidjson::Value &&value,
                      rapidjson::MemoryPoolAllocator<> &allocator): value(value), name(rapidjson::Value(name, allocator)), allocator(allocator) {}

        AppendToArray(const rapidjson::Value::Ch *name, std::size_t length, rapidjson::Value &value,
                      rapidjson::MemoryPoolAllocator<> &allocator): value(value), name(rapidjson::Value(name, length, allocator)), allocator(allocator) {}

        AppendToArray(const rapidjson::Value::Ch *name, std::size_t length, rapidjson::Value &&value,
                      rapidjson::MemoryPoolAllocator<> &allocator): value(value), name(rapidjson::Value(name, length, allocator)), allocator(allocator) {}

        AppendToArray(rapidjson::Value &&name, rapidjson::Value &value,
                      rapidjson::MemoryPoolAllocator<> &allocator): value(value), allocator(allocator) { this->name.Swap(name); }

        inline rapidjson::Value &operator()(rapidjson::Value &root) const override
        {
            if (root.HasMember(name))
            {
                if (root[name].IsArray())
                {
                    root[name].PushBack(value, allocator);
                }
                else
                {
                    root[name] = rapidjson::Value(rapidjson::kArrayType).PushBack(value, allocator);
                }
            }
            else
            {
                root.AddMember(rapidjson::Value(name, allocator), rapidjson::Value(rapidjson::kArrayType).PushBack(value, allocator), allocator);
            }
            return root;
        }
    };

    struct SerializeObject : public ExtensionFunction<std::string> {
        inline std::string operator() (rapidjson::Value &root) const override
        {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            root.Accept(writer);
            return buffer.GetString();
        }
    };
}


#endif // RAPIDJSON_EXTRA_H_INCLUDED
