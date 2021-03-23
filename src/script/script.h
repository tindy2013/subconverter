#ifndef SCRIPT_H_INCLUDED
#define SCRIPT_H_INCLUDED

#include <string>
#include <chaiscript/chaiscript.hpp>

template <typename... input_type, typename return_type> int evalScript(const std::string &script, return_type &return_value, input_type&... input_value)
{
    chaiscript::ChaiScript chai;
    try
    {
        auto fun = chai.eval<std::function<return_type (input_type...)>>(script);
        return_value = fun(input_value...);
    }
    catch (std::exception&)
    {
        return -1;
    }
    return 0;
}

#endif // SCRIPT_H_INCLUDED
