#ifndef TRIBOOL_H_INCLUDED
#define TRIBOOL_H_INCLUDED

#include <string>

#include "string.h"
#include "string_hash.h"

class tribool
{
public:
    tribool() : value_(indeterminate) {}
    tribool(bool value) : value_(value ? true_value : false_value) {}
    tribool(const std::string& str) { set(str); }

    tribool(const tribool& other) = default;
    tribool& operator=(const tribool& other) = default;

    tribool& operator=(bool value)
    {
        value_ = value ? true_value : false_value;
        return *this;
    }

    bool operator==(const tribool& other) const { return value_ == other.value_; }

    operator bool() const { return value_ == true_value; }

    bool is_undef() const { return value_ == indeterminate; }

    template <typename T> tribool& define(const T& value)
    {
        if (is_undef())
            *this = value;
        return *this;
    }

    template <typename T> tribool& parse(const T& value)
    {
        return define(value);
    }

    tribool reverse()
    {
        if (value_ == false_value)
            value_ = true_value;
        else if (value_ == true_value)
            value_ = false_value;
        return *this;
    }

    bool get(const bool& def_value = false) const
    {
        if (is_undef())
            return def_value;
        return value_ == true_value;
    }

    std::string get_str() const
    {
        switch (value_)
        {
            case indeterminate:
                return "undef";
            case false_value:
                return "false";
            case true_value:
                return "true";
            default:
                return "";
        }
    }

    template <typename T> bool set(const T& value)
    {
        value_ = (bool)value ? true_value : false_value;
        return value_;
    }

    bool set(const std::string& str)
    {
        switch (hash_(str))
        {
            case "true"_hash:
            case "1"_hash:
                value_ = true_value;
                break;
            case "false"_hash:
            case "0"_hash:
                value_ = false_value;
                break;
            default:
                if (to_int(str, 0) > 1)
                    value_ = true_value;
                else
                    value_ = indeterminate;
                break;
        }
        return !is_undef();
    }

    void clear() { value_ = indeterminate; }

private:
    enum value_type : char { indeterminate = 0, false_value = 1, true_value = 2 };
    value_type value_;
};

#endif // TRIBOOL_H_INCLUDED
