#ifndef TRIBOOL_H_INCLUDED
#define TRIBOOL_H_INCLUDED

#include <string>

#include "string.h"
#include "string_hash.h"

class tribool
{
private:

    char _M_VALUE = 0;

public:

    tribool() { clear(); }

    template <typename T> tribool(const T &value) { set(value); }

    tribool(const tribool &value) { _M_VALUE = value._M_VALUE; }

    ~tribool() = default;

    tribool& operator=(const tribool &src)
    {
        _M_VALUE = src._M_VALUE;
        return *this;
    }

    template <typename T> tribool& operator=(const T &value)
    {
        set(value);
        return *this;
    }

    inline bool operator==(const tribool& rhs){ return _M_VALUE == rhs._M_VALUE; }

    operator bool() const { return _M_VALUE == 3; }

    bool is_undef() const { return _M_VALUE <= 1; }

    template <typename T> tribool& define(const T &value)
    {
        if(_M_VALUE <= 1)
            *this = value;
        return *this;
    }

    template <typename T> tribool& parse(const T &value)
    {
        return define(value);
    }

    tribool reverse()
    {
        if(_M_VALUE > 1)
            _M_VALUE = _M_VALUE > 2 ? 2 : 3;
        return *this;
    }

    bool get(const bool &def_value = false) const
    {
        if(_M_VALUE <= 1)
            return def_value;
        return _M_VALUE == 3;
    }

    std::string get_str() const
    {
        switch(_M_VALUE)
        {
        case 2:
            return "false";
        case 3:
            return "true";
        }
        return "undef";
    }

    template <typename T> bool set(const T &value)
    {
        _M_VALUE = (bool)value + 2;
        return _M_VALUE > 2;
    }

    bool set(const std::string &str)
    {
        switch(hash_(str))
        {
        case "true"_hash:
        case "1"_hash:
            _M_VALUE = 3;
            break;
        case "false"_hash:
        case "0"_hash:
            _M_VALUE = 2;
            break;
        default:
            if(to_int(str, 0) > 1)
                _M_VALUE = 3;
            else
                _M_VALUE = 0;
            break;
        }
        return _M_VALUE;
    }

    void clear() { _M_VALUE = 0; }
};

#endif // TRIBOOL_H_INCLUDED
