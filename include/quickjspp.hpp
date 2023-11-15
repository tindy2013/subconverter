#pragma once

#include "quickjs/quickjs.h"

#include <vector>
#include <string_view>
#include <string>
#include <cassert>
#include <memory>
#include <cstddef>
#include <algorithm>
#include <tuple>
#include <functional>
#include <stdexcept>
#include <variant>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <fstream>
#include <ios>
#include <sstream>
#include <filesystem>


#if defined(__cpp_rtti)
#define QJSPP_TYPENAME(...) (typeid(__VA_ARGS__).name())
#else
#define QJSPP_TYPENAME(...) #__VA_ARGS__
#endif


namespace qjs {

class Context;
class Value;

/** Exception type.
 * Indicates that exception has occured in JS context.
 */
class exception {
    JSContext * ctx;
public:
    exception(JSContext * ctx) : ctx(ctx) {}
    Context & context() const;

    /// Clears and returns the occurred exception.
    Value get();
};

/** std::shared_ptr, for compatibility with quickjspp v2. */
template <class T> using shared_ptr = std::shared_ptr<T>;
/** std::make_shared, for compatibility with quickjspp v2. */
template <class T, typename... Args>
shared_ptr<T> make_shared(JSContext *, Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/** Javascript conversion traits.
 * Describes how to convert type R to/from JSValue. Second template argument can be used for SFINAE/enable_if type filters.
 */
template <typename R, typename /*_SFINAE*/ = void>
struct js_traits
{
    /** Create an object of C++ type R given JSValue v and JSContext.
     * This function is intentionally not implemented. User should implement this function for their own type.
     * @param v This value is passed as JSValueConst so it should be freed by the caller.
     * @throws exception in case of conversion error
     */
    static R unwrap(JSContext * ctx, JSValueConst v) = delete;

    /** Create JSValue from an object of type R and JSContext.
     * This function is intentionally not implemented. User should implement this function for their own type.
     * @return Returns JSValue which should be freed by the caller or JS_EXCEPTION in case of error.
     */
    static JSValue wrap(JSContext * ctx, R value) = delete;
};

/** Conversion traits for JSValue (identity).
 */
template <>
struct js_traits<JSValue>
{
    static JSValue unwrap(JSContext * ctx, JSValueConst v) noexcept
    {
        return JS_DupValue(ctx, v);
    }

    static JSValue wrap(JSContext * ctx, JSValue&& v) noexcept
    {
        return v;
    }
};

/** Conversion traits for integers.
 * Intentionally doesn't define traits for uint64_t since it can be typedefed to JSValue. (@see JS_NAN_BOXING)
 */
template <typename Int>
struct js_traits<Int, std::enable_if_t<std::is_integral_v<Int> && sizeof(Int) <= sizeof(int64_t) && !std::is_same_v<Int, uint64_t>>>
{
    /// @throws exception
    static Int unwrap(JSContext * ctx, JSValueConst v)
    {
        if constexpr (sizeof(Int) > sizeof(int32_t))
        {
            int64_t r;
            if(JS_ToInt64(ctx, &r, v))
                throw exception{ctx};
            return static_cast<Int>(r);
        }
        else
        {
            int32_t r;
            if(JS_ToInt32(ctx, &r, v))
                throw exception{ctx};
            return static_cast<Int>(r);
        }
    }

    static JSValue wrap(JSContext * ctx, Int i) noexcept
    {
        if constexpr (std::is_same_v<Int, uint32_t> || sizeof(Int) > sizeof(int32_t))
            return JS_NewInt64(ctx, static_cast<Int>(i));
        else
            return JS_NewInt32(ctx, static_cast<Int>(i));
    }
};

/** Conversion traits for boolean.
 */
template <>
struct js_traits<bool>
{
    static bool unwrap(JSContext * ctx, JSValueConst v) noexcept
    {
        // TODO: is this behaviour correct?
        return JS_ToBool(ctx, v) > 0;
    }

    static JSValue wrap(JSContext * ctx, bool i) noexcept
    {
        return JS_NewBool(ctx, i);
    }
};

/** Conversion trait for void.
 */
template <>
struct js_traits<void>
{
    /// @throws exception if jsvalue is neither undefined nor null
    static void unwrap(JSContext * ctx, JSValueConst value)
    {
        if(JS_IsException(value))
            throw exception{ctx};
    }
};

/** Conversion traits for float64/double.
 */
template <>
struct js_traits<double>
{
    /// @throws exception
    static double unwrap(JSContext * ctx, JSValueConst v)
    {
        double r;
        if(JS_ToFloat64(ctx, &r, v))
            throw exception{ctx};
        return r;
    }

    static JSValue wrap(JSContext * ctx, double i) noexcept
    {
        return JS_NewFloat64(ctx, i);
    }
};

namespace detail {
/** Fake std::string_view which frees the string on destruction.
*/
class js_string : public std::string_view
{
    using Base = std::string_view;
    JSContext * ctx = nullptr;

    friend struct js_traits<std::string_view>;

    js_string(JSContext * ctx, const char * ptr, std::size_t len) : Base(ptr, len), ctx(ctx) {}

public:

    template <typename... Args>
    js_string(Args&& ... args) : Base(std::forward<Args>(args)...), ctx(nullptr) {}

    js_string(const js_string& other) = delete;

    operator const char *() const
    {
        return this->data();
    }

    ~js_string()
    {
        if(ctx)
            JS_FreeCString(ctx, this->data());
    }
};
} // namespace detail

/** Conversion traits from std::string_view and to detail::js_string. */
template <>
struct js_traits<std::string_view>
{
    static detail::js_string unwrap(JSContext * ctx, JSValueConst v)
    {
        size_t plen;
        const char * ptr = JS_ToCStringLen(ctx, &plen, v);
        if(!ptr)
            throw exception{ctx};
        return detail::js_string{ctx, ptr, plen};
    }

    static JSValue wrap(JSContext * ctx, std::string_view str) noexcept
    {
        return JS_NewStringLen(ctx, str.data(), str.size());
    }
};

/** Conversion traits for std::string */
template <> // slower
struct js_traits<std::string>
{
    static std::string unwrap(JSContext * ctx, JSValueConst v)
    {
        auto str_view = js_traits<std::string_view>::unwrap(ctx, v);
        return std::string{str_view.data(), str_view.size()};
    }

    static JSValue wrap(JSContext * ctx, const std::string& str) noexcept
    {
        return JS_NewStringLen(ctx, str.data(), str.size());
    }
};

/** Conversion from const char * */
template <>
struct js_traits<const char *>
{
    static JSValue wrap(JSContext * ctx, const char * str) noexcept
    {
        return JS_NewString(ctx, str);
    }

    static detail::js_string unwrap(JSContext * ctx, JSValueConst v)
    {
        return js_traits<std::string_view>::unwrap(ctx, v);
    }
};


/** Conversion from const std::variant */
template <typename ... Ts>
struct js_traits<std::variant<Ts...>>
{
    static JSValue wrap(JSContext * ctx, std::variant<Ts...> value) noexcept
    {
        return std::visit([ctx](auto&& value) {
            using T = std::decay_t<decltype(value)>;
            return js_traits<T>::wrap(ctx, value);
        }, std::move(value));
    }


    /* Useful type traits */
    template <typename T> struct is_shared_ptr : std::false_type {};
    template <typename T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
    template <typename T> struct is_string
    {
        static constexpr bool value = std::is_same_v<T, const char *> || std::is_same_v<std::decay_t<T>, std::string> ||
                                      std::is_same_v<std::decay_t<T>, std::string_view>;
    };
    template <typename T> struct is_boolean { static constexpr bool value = std::is_same_v<std::decay_t<T>, bool>; };
    template <typename T> struct is_double { static constexpr bool value = std::is_same_v<std::decay_t<T>, double>; };
    template <typename T> struct is_vector : std::false_type {};
    template <typename T> struct is_vector<std::vector<T>> : std::true_type {};
    template <typename T> struct is_pair : std::false_type {};
    template <typename U, typename V> struct is_pair<std::pair<U, V>> : std::true_type {};
    template <typename T> struct is_variant : std::false_type {};
    template <typename ... Us> struct is_variant<std::variant<Us...>> : std::true_type {};

    /** Attempt to match common types (integral, floating-point, string, etc.) */
    template <template <typename R> typename Trait, typename U, typename ... Us>
    static std::optional<std::variant<Ts...>> unwrapImpl(JSContext * ctx, JSValueConst v)
    {
        if constexpr (Trait<U>::value)
        {
            return js_traits<U>::unwrap(ctx, v);
        }
        if constexpr ((sizeof ... (Us)) > 0)
        {
            return unwrapImpl<Trait, Us...>(ctx, v);
        }
        return std::nullopt;
    }

    /** Attempt to match class ID with type */
    template <typename U, typename ... Us>
    static std::optional<std::variant<Ts...>> unwrapObj(JSContext * ctx, JSValueConst v, JSClassID class_id)
    {
        if constexpr (is_shared_ptr<U>::value)
        {
            if(class_id == js_traits<U>::QJSClassId)
            {
                return js_traits<U>::unwrap(ctx, v);
            }
        }

        // try to unwrap embedded variant (variant<variant<...>>), might be slow
        if constexpr (is_variant<U>::value)
        {
            if(auto opt = js_traits<std::optional<U>>::unwrap(ctx, v))
                return *opt;
        }

        if constexpr (is_vector<U>::value)
        {
            if(JS_IsArray(ctx, v) == 1)
            {
                auto firstElement = JS_GetPropertyUint32(ctx, v, 0);
                bool ok = isCompatible<std::decay_t<typename U::value_type>>(ctx, firstElement);
                JS_FreeValue(ctx, firstElement);
                if(ok)
                {
                    return U{js_traits<U>::unwrap(ctx, v)};
                }
            }
        }

        if constexpr (is_pair<U>::value)
        {
            if(JS_IsArray(ctx, v) == 1)
            {
                // todo: check length?
                auto firstElement = JS_GetPropertyUint32(ctx, v, 0);
                auto secondElement = JS_GetPropertyUint32(ctx, v, 1);
                bool ok = isCompatible<std::decay_t<typename U::first_type>>(ctx, firstElement)
                          && isCompatible<std::decay_t<typename U::second_type>>(ctx, secondElement);
                JS_FreeValue(ctx, firstElement);
                JS_FreeValue(ctx, secondElement);
                if(ok)
                {
                    return U{js_traits<U>::unwrap(ctx, v)};
                }
            }
        }

        if constexpr ((sizeof ... (Us)) > 0)
        {
            return unwrapObj<Us...>(ctx, v, class_id);
        }
        return std::nullopt;
    }

    /** Attempt to cast to types satisfying traits, ordered in terms of priority */
    template <template <typename T> typename Trait, template <typename T> typename ... Traits>
    static std::variant<Ts...> unwrapPriority(JSContext * ctx, JSValueConst v)
    {
        if(auto result = unwrapImpl<Trait, Ts...>(ctx, v))
        {
            return *result;
        }
        if constexpr ((sizeof ... (Traits)) > 0)
        {
            return unwrapPriority<Traits...>(ctx, v);
        }
        JS_ThrowTypeError(ctx, "Expected type %s", QJSPP_TYPENAME(std::variant<Ts...>));
        throw exception{ctx};
    }

    template <typename T>
    static bool isCompatible(JSContext * ctx, JSValueConst v) noexcept
    {
        //const char * type_name = typeid(T).name();
        switch(JS_VALUE_GET_TAG(v))
        {
            case JS_TAG_STRING:
                return is_string<T>::value;

            case JS_TAG_FUNCTION_BYTECODE:
                return std::is_function<T>::value;
            case JS_TAG_OBJECT:
                if(JS_IsArray(ctx, v) == 1)
                    return is_vector<T>::value || is_pair<T>::value;
                if constexpr (is_shared_ptr<T>::value)
                {
                    if(JS_GetClassID(v) == js_traits<T>::QJSClassId)
                        return true;
                }
                return false;

            case JS_TAG_INT:
                [[fallthrough]];
            case JS_TAG_BIG_INT:
                return std::is_integral_v<T> || std::is_floating_point_v<T>;
            case JS_TAG_BOOL:
                return is_boolean<T>::value || std::is_integral_v<T> || std::is_floating_point_v<T>;

            case JS_TAG_BIG_DECIMAL:
                [[fallthrough]];
            case JS_TAG_BIG_FLOAT:
                [[fallthrough]];
            case JS_TAG_FLOAT64:
            default: // >JS_TAG_FLOAT64 (JS_NAN_BOXING)
                return is_double<T>::value || std::is_floating_point_v<T>;

            case JS_TAG_SYMBOL:
                [[fallthrough]];
            case JS_TAG_MODULE:
                [[fallthrough]];
            case JS_TAG_NULL:
                [[fallthrough]];
            case JS_TAG_UNDEFINED:
                [[fallthrough]];
            case JS_TAG_UNINITIALIZED:
                [[fallthrough]];
            case JS_TAG_CATCH_OFFSET:
                [[fallthrough]];
            case JS_TAG_EXCEPTION:
                break;
        }
        return false;
    }

    static std::variant<Ts...> unwrap(JSContext * ctx, JSValueConst v)
    {
        const auto tag = JS_VALUE_GET_TAG(v);
        switch(tag)
        {
            case JS_TAG_STRING:
                return unwrapPriority<is_string>(ctx, v);

            case JS_TAG_FUNCTION_BYTECODE:
                return unwrapPriority<std::is_function>(ctx, v);
            case JS_TAG_OBJECT:
                if(auto result = unwrapObj<Ts...>(ctx, v, JS_GetClassID(v)))
                {
                    return *result;
                }
                JS_ThrowTypeError(ctx, "Expected type %s, got object with classid %d",
                                  QJSPP_TYPENAME(std::variant<Ts...>), JS_GetClassID(v));
                break;

            case JS_TAG_INT:
                [[fallthrough]];
            case JS_TAG_BIG_INT:
                return unwrapPriority<std::is_integral, std::is_floating_point>(ctx, v);
            case JS_TAG_BOOL:
                return unwrapPriority<is_boolean, std::is_integral, std::is_floating_point>(ctx, v);

            case JS_TAG_SYMBOL:
                [[fallthrough]];
            case JS_TAG_MODULE:
                [[fallthrough]];
            case JS_TAG_NULL:
                [[fallthrough]];
            case JS_TAG_UNDEFINED:
                [[fallthrough]];
            case JS_TAG_UNINITIALIZED:
                [[fallthrough]];
            case JS_TAG_CATCH_OFFSET:
                JS_ThrowTypeError(ctx, "Expected type %s, got tag %d", QJSPP_TYPENAME(std::variant<Ts...>), tag);
                [[fallthrough]];
            case JS_TAG_EXCEPTION:
                break;

            case JS_TAG_BIG_DECIMAL:
                [[fallthrough]];
            case JS_TAG_BIG_FLOAT:
                [[fallthrough]];

            case JS_TAG_FLOAT64:
                [[fallthrough]];
            default: // more than JS_TAG_FLOAT64 (nan boxing)
                return unwrapPriority<is_double, std::is_floating_point>(ctx, v);
        }

        throw exception{ctx};
    }
};

template <typename T>
struct rest : std::vector<T>
{
    using std::vector<T>::vector;
    using std::vector<T>::operator=;
};

namespace detail {

/** Helper function to convert and then free JSValue. */
template <typename T>
T unwrap_free(JSContext * ctx, JSValue val)
{
    if constexpr(std::is_same_v<T, void>)
    {
        JS_FreeValue(ctx, val);
        return js_traits<T>::unwrap(ctx, val);
    }
    else
    {
        try
        {
            T result = js_traits<std::decay_t<T>>::unwrap(ctx, val);
            JS_FreeValue(ctx, val);
            return result;
        }
        catch(...)
        {
            JS_FreeValue(ctx, val);
            throw;
        }
    }
}

template <typename T, size_t I, size_t NArgs>
struct unwrap_arg_impl {
    static auto unwrap(JSContext * ctx, int argc, JSValueConst * argv)
    {
        if (size_t(argc) <= I) {
            JS_ThrowTypeError(ctx, "Expected at least %lu arguments but received %d",
                              (unsigned long)NArgs, argc);
            throw exception{ctx};
        }
        return js_traits<std::decay_t<T>>::unwrap(ctx, argv[I]);
    }
};

template <typename T, size_t I, size_t NArgs>
struct unwrap_arg_impl<rest<T>, I, NArgs> {
    static rest<T> unwrap(JSContext * ctx, int argc, JSValueConst * argv) {
        static_assert(I == NArgs - 1, "The `rest` argument must be the last function argument.");
        rest<T> result;
        result.reserve(argc - I);
        for (size_t i = I; i < size_t(argc); ++i)
            result.push_back(js_traits<T>::unwrap(ctx, argv[i]));
        return result;
    }
};

template <class Tuple, std::size_t... I>
Tuple unwrap_args_impl(JSContext * ctx, int argc, JSValueConst * argv, std::index_sequence<I...>)
{
    return Tuple{unwrap_arg_impl<std::tuple_element_t<I, Tuple>, I, sizeof...(I)>::unwrap(ctx, argc, argv)...};
}

/** Helper function to convert an array of JSValues to a tuple.
 * @tparam Args C++ types of the argv array
 */
template <typename... Args>
std::tuple<std::decay_t<Args>...> unwrap_args(JSContext * ctx, int argc, JSValueConst * argv)
{
    return unwrap_args_impl<std::tuple<std::decay_t<Args>...>>(ctx, argc, argv, std::make_index_sequence<sizeof...(Args)>());
}

/** Helper function to call f with an array of JSValues.
 * @tparam R return type of f
 * @tparam Args argument types of f
 * @tparam Callable type of f (inferred)
 * @param ctx JSContext
 * @param f callable object
 * @param argv array of JSValue's
 * @return converted return value of f or JS_NULL if f returns void
 */
template <typename R, typename... Args, typename Callable>
JSValue wrap_call(JSContext * ctx, Callable&& f, int argc, JSValueConst * argv) noexcept
{
    try
    {
        if constexpr(std::is_same_v<R, void>)
        {
            std::apply(std::forward<Callable>(f), unwrap_args<Args...>(ctx, argc, argv));
            return JS_NULL;
        }
        else
        {
            return js_traits<std::decay_t<R>>::wrap(ctx,
                                                    std::apply(std::forward<Callable>(f),
                                                               unwrap_args<Args...>(ctx, argc, argv)));
        }
    }
    catch(exception)
    {
        return JS_EXCEPTION;
    }
    catch (std::exception const & err)
    {
        JS_ThrowInternalError(ctx, "%s", err.what());
        return JS_EXCEPTION;
    }
    catch (...)
    {
        JS_ThrowInternalError(ctx, "Unknown error");
        return JS_EXCEPTION;
    }
}

/** Same as wrap_call, but pass this_value as first argument.
 * @tparam FirstArg type of this_value
 */
template <typename R, typename FirstArg, typename... Args, typename Callable>
JSValue wrap_this_call(JSContext * ctx, Callable&& f, JSValueConst this_value, int argc, JSValueConst * argv) noexcept
{
    try
    {
        if constexpr(std::is_same_v<R, void>)
        {
            std::apply(std::forward<Callable>(f), std::tuple_cat(unwrap_args<FirstArg>(ctx, 1, &this_value),
                                                                 unwrap_args<Args...>(ctx, argc, argv)));
            return JS_NULL;
        }
        else
        {
            return js_traits<std::decay_t<R>>::wrap(ctx,
                                                    std::apply(std::forward<Callable>(f),
                                                               std::tuple_cat(
                                                                       unwrap_args<FirstArg>(ctx, 1, &this_value),
                                                                       unwrap_args<Args...>(ctx, argc, argv))));
        }
    }
    catch(exception)
    {
        return JS_EXCEPTION;
    }
    catch (std::exception const & err)
    {
        JS_ThrowInternalError(ctx, "%s", err.what());
        return JS_EXCEPTION;
    }
    catch (...)
    {
        JS_ThrowInternalError(ctx, "Unknown error");
        return JS_EXCEPTION;
    }
}

template <class Tuple, std::size_t... I>
void wrap_args_impl(JSContext * ctx, JSValue * argv, Tuple tuple, std::index_sequence<I...>)
{
    ((argv[I] = js_traits<std::decay_t<std::tuple_element_t<I, Tuple>>>::wrap(ctx, std::get<I>(tuple))), ...);
}

/** Converts C++ args to JSValue array.
 * @tparam Args argument types
 * @param argv array of size at least sizeof...(Args)
 */
template <typename... Args>
void wrap_args(JSContext * ctx, JSValue * argv, Args&& ... args)
{
    wrap_args_impl(ctx, argv, std::make_tuple(std::forward<Args>(args)...),
                   std::make_index_sequence<sizeof...(Args)>());
}

// Helper trait to obtain `T` in `T::*` expressions
template<typename T> struct class_from_member_pointer { using type = void; };
template<typename T, typename U> struct class_from_member_pointer<T U::*> { using type = U; };
template<typename T> using class_from_member_pointer_t = typename class_from_member_pointer<T>::type;

} // namespace detail

/** A wrapper type for free and class member functions.
 * Pointer to function F is a template argument.
 * @tparam F either a pointer to free function or a pointer to class member function
 * @tparam PassThis if true and F is a pointer to free function, passes Javascript "this" value as first argument:
 */
template <auto F, bool PassThis = false /* pass this as the first argument */>
struct fwrapper
{
    /// "name" property of the JS function object (not defined if nullptr)
    const char * name = nullptr;
};

/** Conversion to JSValue for free function in fwrapper. */
template <typename R, typename... Args, R (* F)(Args...), bool PassThis>
struct js_traits<fwrapper<F, PassThis>>
{
    static JSValue wrap(JSContext * ctx, fwrapper<F, PassThis> fw) noexcept
    {
        return JS_NewCFunction(ctx, [](JSContext * ctx, JSValueConst this_value, int argc,
                                       JSValueConst * argv) noexcept -> JSValue {
            if constexpr(PassThis)
                return detail::wrap_this_call<R, Args...>(ctx, F, this_value, argc, argv);
            else
                return detail::wrap_call<R, Args...>(ctx, F, argc, argv);
        }, fw.name, sizeof...(Args));

    }
};

/** Conversion to JSValue for class member function in fwrapper. PassThis is ignored and treated as true */
template <typename R, class T, typename... Args, R (T::*F)(Args...), bool PassThis/*=ignored*/>
struct js_traits<fwrapper<F, PassThis>>
{
    static JSValue wrap(JSContext * ctx, fwrapper<F, PassThis> fw) noexcept
    {
        return JS_NewCFunction(ctx, [](JSContext * ctx, JSValueConst this_value, int argc,
                                       JSValueConst * argv) noexcept -> JSValue {
            return detail::wrap_this_call<R, std::shared_ptr<T>, Args...>(ctx, F, this_value, argc, argv);
        }, fw.name, sizeof...(Args));

    }
};

/** Conversion to JSValue for const class member function in fwrapper. PassThis is ignored and treated as true */
template <typename R, class T, typename... Args, R (T::*F)(Args...) const, bool PassThis/*=ignored*/>
struct js_traits<fwrapper<F, PassThis>>
{
    static JSValue wrap(JSContext * ctx, fwrapper<F, PassThis> fw) noexcept
    {
        return JS_NewCFunction(ctx, [](JSContext * ctx, JSValueConst this_value, int argc,
                                       JSValueConst * argv) noexcept -> JSValue {
            return detail::wrap_this_call<R, std::shared_ptr<T>, Args...>(ctx, F, this_value, argc, argv);
        }, fw.name, sizeof...(Args));

    }
};

/** A wrapper type for constructor of type T with arguments Args.
 * Compilation fails if no such constructor is defined.
 * @tparam Args constructor arguments
 */
template <class T, typename... Args>
struct ctor_wrapper
{
    static_assert(std::is_constructible<T, Args...>::value, "no such constructor!");
    /// "name" property of JS constructor object
    const char * name = nullptr;
};

namespace detail {
/// equivalent to JS_GetPropertyStr(ctx, this_value, "prototype");
inline JSValue GetPropertyPrototype(JSContext * ctx, JSValueConst this_value)
{
    // constant atom: doesn't need to be freed and doesn't change with context
    static const JSAtom JS_ATOM_prototype = JS_NewAtom(ctx, "prototype");
    return JS_GetProperty(ctx, this_value, JS_ATOM_prototype);
}
} // namespace detail

/** Conversion to JSValue for ctor_wrapper. */
template <class T, typename... Args>
struct js_traits<ctor_wrapper<T, Args...>>
{
    static JSValue wrap(JSContext * ctx, ctor_wrapper<T, Args...> cw) noexcept
    {
        return JS_NewCFunction2(ctx, [](JSContext * ctx, JSValueConst this_value, int argc,
                                        JSValueConst * argv) noexcept -> JSValue {

            if(js_traits<std::shared_ptr<T>>::QJSClassId == 0) // not registered
            {
#if defined(__cpp_rtti)
                // automatically register class on first use (no prototype)
                js_traits<std::shared_ptr<T>>::register_class(ctx, typeid(T).name());
#else
                JS_ThrowTypeError(ctx, "quickjspp ctor_wrapper<T>::wrap: Class is not registered");
                return JS_EXCEPTION;
#endif
            }

            auto proto = detail::GetPropertyPrototype(ctx, this_value);
            if(JS_IsException(proto))
                return proto;
            auto jsobj = JS_NewObjectProtoClass(ctx, proto, js_traits<std::shared_ptr<T>>::QJSClassId);
            JS_FreeValue(ctx, proto);
            if(JS_IsException(jsobj))
                return jsobj;

            try
            {
                std::shared_ptr<T> ptr = std::apply(std::make_shared<T, Args...>, detail::unwrap_args<Args...>(ctx, argc, argv));
                JS_SetOpaque(jsobj, new std::shared_ptr<T>(std::move(ptr)));
                return jsobj;
            }
            catch (exception)
            {
                JS_FreeValue(ctx, jsobj);
                return JS_EXCEPTION;
            }
            catch (std::exception const & err)
            {
                JS_FreeValue(ctx, jsobj);
                JS_ThrowInternalError(ctx, "%s", err.what());
                return JS_EXCEPTION;
            }
            catch (...)
            {
                JS_FreeValue(ctx, jsobj);
                JS_ThrowInternalError(ctx, "Unknown error");
                return JS_EXCEPTION;
            }

            // return detail::wrap_call<std::shared_ptr<T>, Args...>(ctx, std::make_shared<T, Args...>, argv);
        }, cw.name, sizeof...(Args), JS_CFUNC_constructor, 0);
    }
};


/** Conversions for std::shared_ptr<T>. Empty shared_ptr corresponds to JS_NULL.
 * T should be registered to a context before conversions.
 * @tparam T class type
 */
template <class T>
struct js_traits<std::shared_ptr<T>>
{
    /// Registered class id in QuickJS.
    inline static JSClassID QJSClassId = 0;

    /// Signature of the function to obtain the std::shared_ptr from the JSValue.
    using ptr_cast_fcn_t = std::function<std::shared_ptr<T>(JSContext*, JSValueConst)>;

    /// Used by registerDerivedClass to register new derived classes with this class' base type.
    inline static std::function<void(JSClassID, ptr_cast_fcn_t)> registerWithBase;

    /// Mapping between derived class' JSClassID and function to obtain the std::shared_ptr from the JSValue.
    inline static std::unordered_map<JSClassID, ptr_cast_fcn_t> ptrCastFcnMap;

    /** Register a class as a derived class.
     * 
     * @tparam D type of the derived class
     * @param derived_class_id class id of the derived class
     * @param ptr_cast_fcn function to obtain a std::shared_ptr from the JSValue
     */
    template<typename D>
    static void registerDerivedClass(JSClassID derived_class_id, ptr_cast_fcn_t ptr_cast_fcn) {
        static_assert(std::is_base_of<T,D>::value && !std::is_same<T,D>::value, "Type is not a derived class");
        using derived_ptr_cast_fcn_t = typename js_traits<std::shared_ptr<D>>::ptr_cast_fcn_t;

        // Register how to obtain the std::shared_ptr from the derived class.
        ptrCastFcnMap[derived_class_id] = ptr_cast_fcn;

        // Propagate the registration to our base class (if any).
        if (registerWithBase) registerWithBase(derived_class_id, ptr_cast_fcn);

        // Instrument the derived class so that it can propagate new derived classes to us.
        auto old_registerWithBase = js_traits<std::shared_ptr<D>>::registerWithBase;
        js_traits<std::shared_ptr<D>>::registerWithBase =
            [old_registerWithBase = std::move(old_registerWithBase)]
            (JSClassID derived_class_id, derived_ptr_cast_fcn_t derived_ptr_cast_fcn){
                if (old_registerWithBase) old_registerWithBase(derived_class_id, derived_ptr_cast_fcn);
                registerDerivedClass<D>(derived_class_id, [derived_cast_fcn = std::move(derived_ptr_cast_fcn)](JSContext * ctx, JSValueConst v) {
                    return std::shared_ptr<T>(derived_cast_fcn(ctx, v));
                });
            };
    }

    template <typename B>
    static
    std::enable_if_t<std::is_same_v<B, T> || std::is_same_v<B, void>>
    ensureCanCastToBase() { }

    template <typename B>
    static
    std::enable_if_t<!std::is_same_v<B, T> && !std::is_same_v<B, void>>
    ensureCanCastToBase() {
        static_assert(std::is_base_of_v<B, T>, "Type is not a derived class");

        if(js_traits<std::shared_ptr<T>>::QJSClassId == 0)
            JS_NewClassID(&js_traits<std::shared_ptr<T>>::QJSClassId);

        js_traits<std::shared_ptr<B>>::template registerDerivedClass<T>(QJSClassId, unwrap);
    }

    template <auto M>
    static void ensureCanCastToBase() {
        ensureCanCastToBase<detail::class_from_member_pointer_t<decltype(M)>>();
    }

    /** Stores offsets to qjs::Value members of T.
     * These values should be marked by class_registrar::mark for QuickJS garbage collector
     * so that the cycle removal algorithm can find the other objects referenced by this object.
     */
    static inline std::vector<Value T::*> markOffsets;

    /** Register class in QuickJS context.
     *
     * @param ctx context
     * @param name class name
     * @param proto class prototype or JS_NULL
     * @param call QJS call function. see quickjs doc
     * @param exotic pointer to QJS exotic methods(static lifetime) which allow custom property handling. see quickjs doc
     * @throws exception
     */
    static void register_class(JSContext * ctx, const char * name, JSValue proto = JS_NULL,
                               JSClassCall * call = nullptr, JSClassExoticMethods * exotic = nullptr)
    {
        if(QJSClassId == 0)
        {
            JS_NewClassID(&QJSClassId);
        }
        auto rt = JS_GetRuntime(ctx);
        if(!JS_IsRegisteredClass(rt, QJSClassId))
        {
            JSClassGCMark * marker = nullptr;
            if(!markOffsets.empty())
            {
                marker = [](JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func) {
                    auto pptr = static_cast<std::shared_ptr<T> *>(JS_GetOpaque(val, QJSClassId));
                    assert(pptr);
                    const T * ptr = pptr->get();
                    assert(ptr);
                    for(Value T::* member : markOffsets)
                    {
                        JS_MarkValue(rt, (*ptr.*member).v, mark_func);
                    }
                };
            }
            JSClassDef def{
                    name,
                    // destructor (finalizer)
                    [](JSRuntime * rt, JSValue obj) noexcept {
                        auto pptr = static_cast<std::shared_ptr<T> *>(JS_GetOpaque(obj, QJSClassId));
                        delete pptr;
                    },
                    // mark
                    marker,
                    // call
                    call,
                    // exotic
                    exotic
            };
            int e = JS_NewClass(rt, QJSClassId, &def);
            if(e < 0)
            {
                JS_ThrowInternalError(ctx, "Can't register class %s", name);
                throw exception{ctx};
            }
        }
        JS_SetClassProto(ctx, QJSClassId, proto);
    }

    /** Create a JSValue from std::shared_ptr<T>.
     * Creates an object with class if #QJSClassId and sets its opaque pointer to a new copy of #ptr.
     */
    static JSValue wrap(JSContext * ctx, std::shared_ptr<T> ptr)
    {
        if(!ptr)
            return JS_NULL;
        if(QJSClassId == 0) // not registered
        {
#if defined(__cpp_rtti)
            // automatically register class on first use (no prototype)
            register_class(ctx, typeid(T).name());
#else
            JS_ThrowTypeError(ctx, "quickjspp std::shared_ptr<T>::wrap: Class is not registered");
            return JS_EXCEPTION;
#endif
        }
        auto jsobj = JS_NewObjectClass(ctx, QJSClassId);
        if(JS_IsException(jsobj))
            return jsobj;

        auto pptr = new std::shared_ptr<T>(std::move(ptr));
        JS_SetOpaque(jsobj, pptr);
        return jsobj;
    }

    /// @throws exception if #v doesn't have the correct class id
    static std::shared_ptr<T> unwrap(JSContext * ctx, JSValueConst v)
    {
        std::shared_ptr<T> ptr = nullptr;
        if (JS_IsNull(v)) {
            return ptr;
        }
        auto obj_class_id = JS_GetClassID(v);

        if (obj_class_id == QJSClassId) {
            // The JS object is of class T
            void * opaque = JS_GetOpaque2(ctx, v, obj_class_id);
            assert(opaque && "No opaque pointer in object");
            ptr = *static_cast<std::shared_ptr<T> *>(opaque);
        } else if (ptrCastFcnMap.count(obj_class_id)) {
            // The JS object is of a class derived from T
            ptr = ptrCastFcnMap[obj_class_id](ctx, v);
        } else {
            // The JS object does not derives from T
            JS_ThrowTypeError(ctx, "Expected type %s, got object with classid %d",
                              QJSPP_TYPENAME(T), obj_class_id);
            throw exception{ctx};
        }
        if(!ptr) {
            JS_ThrowInternalError(ctx, "Object's opaque pointer is NULL");
            throw exception{ctx};
        }
        return ptr;
    }
};

/** Conversions for non-owning pointers to class T. nullptr corresponds to JS_NULL.
 * @tparam T class type
 */
template <class T>
struct js_traits<T *, std::enable_if_t<std::is_class_v<T>>>
{
    static JSValue wrap(JSContext * ctx, T * ptr)
    {
        if (ptr == nullptr) {
            return JS_NULL;
        }   
        if(js_traits<std::shared_ptr<T>>::QJSClassId == 0) // not registered
        {
#if defined(__cpp_rtti)
            // If you have an error here with T=JSValueConst
            // it probably means you are passing JSValueConst to where JSValue is expected
            js_traits<std::shared_ptr<T>>::register_class(ctx, typeid(T).name());
#else
            JS_ThrowTypeError(ctx, "quickjspp js_traits<T *>::wrap: Class is not registered");
            return JS_EXCEPTION;
#endif
        }
        auto jsobj = JS_NewObjectClass(ctx, js_traits<std::shared_ptr<T>>::QJSClassId);
        if(JS_IsException(jsobj))
            return jsobj;

        // shared_ptr with empty deleter since we don't own T*
        auto pptr = new std::shared_ptr<T>(ptr, [](T *) {});
        JS_SetOpaque(jsobj, pptr);
        return jsobj;
    }

    static T * unwrap(JSContext * ctx, JSValueConst v)
    {
        if (JS_IsNull(v)) {
            return nullptr;
        }
        auto ptr = js_traits<std::shared_ptr<T>>::unwrap(ctx, v);
        return ptr.get();
    }
};

/** Conversions for enums. */
template <typename E>
struct js_traits<E, std::enable_if_t<std::is_enum_v<E>>> {
    using T = std::underlying_type_t<E>;
    static E unwrap(JSContext* ctx, JSValue v) noexcept {
        return static_cast<E>(js_traits<T>::unwrap(ctx, v));
    }

    static JSValue wrap(JSContext* ctx, E t) noexcept {
        return js_traits<T>::wrap(ctx, static_cast<T>(t));;
    }
};

namespace detail {
/** A faster std::function-like object with type erasure.
 * Used to convert any callable objects (including lambdas) to JSValue.
 */
struct function
{
    JSValue
    (* invoker)(function * self, JSContext * ctx, JSValueConst this_value, int argc, JSValueConst * argv) = nullptr;

    void (* destroyer)(function * self) = nullptr;

    alignas(std::max_align_t) char functor[];

    template <typename Functor>
    static function * create(JSRuntime * rt, Functor&& f)
    {
        using Functor_t = std::decay_t<Functor>;
        auto fptr = static_cast<function *>(js_malloc_rt(rt, sizeof(function) + sizeof(Functor_t)));
        if(!fptr)
            throw std::bad_alloc{};
        new(fptr) function;
        auto functorptr = reinterpret_cast<Functor_t *>(fptr->functor);
        new(functorptr) Functor_t(std::forward<Functor>(f));
        fptr->destroyer = nullptr;
        if constexpr(!std::is_trivially_destructible_v<Functor_t>)
        {
            fptr->destroyer = [](function * fptr) {
                auto functorptr = reinterpret_cast<Functor_t *>(fptr->functor);
                functorptr->~Functor_t();
            };
        }
        return fptr;
    }
};

static_assert(std::is_trivially_destructible_v<function>);
}

template <>
struct js_traits<detail::function>
{
    inline static JSClassID QJSClassId = 0;

    // TODO: replace ctx with rt
    static void register_class(JSContext * ctx, const char * name)
    {
        if(QJSClassId == 0)
        {
            JS_NewClassID(&QJSClassId);
        }
        auto rt = JS_GetRuntime(ctx);
        if(JS_IsRegisteredClass(rt, QJSClassId))
            return;
        JSClassDef def{
                name,
                // destructor
                [](JSRuntime * rt, JSValue obj) noexcept {
                    auto fptr = static_cast<detail::function *>(JS_GetOpaque(obj, QJSClassId));
                    assert(fptr);
                    if(fptr->destroyer)
                        fptr->destroyer(fptr);
                    js_free_rt(rt, fptr);
                },
                nullptr, // mark
                // call
                [](JSContext * ctx, JSValueConst func_obj, JSValueConst this_val, int argc,
                   JSValueConst * argv, int flags) -> JSValue {
                    auto ptr = static_cast<detail::function *>(JS_GetOpaque2(ctx, func_obj, QJSClassId));
                    if(!ptr)
                        return JS_EXCEPTION;
                    return ptr->invoker(ptr, ctx, this_val, argc, argv);
                },
                nullptr
        };
        int e = JS_NewClass(rt, QJSClassId, &def);
        if(e < 0)
            throw std::runtime_error{"Cannot register C++ function class"};
    }
};


/** Traits for accessing object properties.
 * @tparam Key property key type (uint32 and strings are supported)
 */
template <typename Key>
struct js_property_traits
{
    static void set_property(JSContext * ctx, JSValue this_obj, Key key, JSValue value);

    static JSValue get_property(JSContext * ctx, JSValue this_obj, Key key);
};

template <>
struct js_property_traits<const char *>
{
    static void set_property(JSContext * ctx, JSValue this_obj, const char * name, JSValue value)
    {
        int err = JS_SetPropertyStr(ctx, this_obj, name, value);
        if(err < 0)
            throw exception{ctx};
    }

    static JSValue get_property(JSContext * ctx, JSValue this_obj, const char * name) noexcept
    {
        return JS_GetPropertyStr(ctx, this_obj, name);
    }
};

template <>
struct js_property_traits<uint32_t>
{
    static void set_property(JSContext * ctx, JSValue this_obj, uint32_t idx, JSValue value)
    {
        int err = JS_SetPropertyUint32(ctx, this_obj, idx, value);
        if(err < 0)
            throw exception{ctx};
    }

    static JSValue get_property(JSContext * ctx, JSValue this_obj, uint32_t idx) noexcept
    {
        return JS_GetPropertyUint32(ctx, this_obj, idx);
    }
};

template <>
struct js_property_traits<int> : js_property_traits<uint32_t> {};


namespace detail {
template <typename Key>
struct property_proxy
{
    JSContext * ctx;
    JSValue this_obj;
    Key key;

    /** Conversion helper function */
    template <typename T>
    T as() const
    {
        return unwrap_free<T>(ctx, js_property_traits<Key>::get_property(ctx, this_obj, key));
    }

    /** Explicit conversion operator (to any type) */
    template <typename T>
    explicit operator T() const { return as<T>(); }

    /** Implicit converion to qjs::Value */
    operator Value() const; // defined later due to Value being incomplete type

    /// noncopyable
    property_proxy& operator =(property_proxy) = delete;

    template <typename T>
    property_proxy& operator =(T&& value)
    {
        js_property_traits<Key>::set_property(ctx, this_obj, key,
                                              js_traits<std::decay_t<T>>::wrap(ctx, std::forward<T>(value)));
        return *this;
    }

    template <typename Key2>
    property_proxy<Key2> operator[](Key2 key2) const
    {
        return {ctx, as<JSValue>(), std::move(key2)};
    }

    ~property_proxy() noexcept { JS_FreeValue(ctx, this_obj); }
};


// class member variable getter/setter
template <auto M>
struct get_set {};

// M -  member object
template <class T, typename R, R T::*M>
struct get_set<M>
{
    using is_const = std::is_const<R>;

    static const R& get(std::shared_ptr<T> ptr)
    {
        return *ptr.*M;
    }

    static R& set(std::shared_ptr<T> ptr, R value)
    {
        return *ptr.*M = std::move(value);
    }

};

// M - static member object
template <typename R, R *M>
struct get_set<M>
{
    using is_const = std::is_const<R>;

    static const R& get(bool)
    {
        return *M;
    }

    static R& set(bool, R value)
    {
        return *M = std::move(value);
    }

};

} // namespace detail

/** JSValue with RAAI semantics.
 * A wrapper over (JSValue v, JSContext * ctx).
 * Calls JS_FreeValue(ctx, v) on destruction. Can be copied and moved.
 * A JSValue can be released by either JSValue x = std::move(value); or JSValue x = value.release(), then the Value becomes invalid and FreeValue won't be called
 * Can be converted to C++ type, for example: auto string = value.as<std::string>(); qjs::exception would be thrown on error
 * Properties can be accessed (read/write): value["property1"] = 1; value[2] = "2";
 */
class Value
{
public:
    JSValue v;
    JSContext * ctx = nullptr;

public:
    /** Use context.newValue(val) instead */
    template <typename T>
    Value(JSContext * ctx, T&& val) : ctx(ctx)
    {
        v = js_traits<std::decay_t<T>>::wrap(ctx, std::forward<T>(val));
        if(JS_IsException(v))
            throw exception{ctx};
    }

    Value(JSValue&& v) noexcept : v(std::move(v)), ctx(nullptr) {}

    Value(const Value& rhs) noexcept
    {
        ctx = rhs.ctx;
        v = JS_DupValue(ctx, rhs.v);
    }

    Value(Value&& rhs) noexcept
    {
        std::swap(ctx, rhs.ctx);
        v = rhs.v;
    }

    Value& operator =(Value rhs) noexcept
    {
        std::swap(ctx, rhs.ctx);
        std::swap(v, rhs.v);
        return *this;
    }

    bool operator ==(JSValueConst other) const
    {
        return JS_VALUE_GET_TAG(v) == JS_VALUE_GET_TAG(other) && JS_VALUE_GET_PTR(v) == JS_VALUE_GET_PTR(other);
    }

    bool operator !=(JSValueConst other) const { return !((*this) == other); }

    /** Returns true if 2 values are the same (equality for arithmetic types or point to the same object) */
    bool operator ==(const Value& rhs) const
    {
        return (*this == rhs.v);
    }

    bool operator !=(const Value& rhs) const { return !((*this) == rhs); }


    ~Value()
    {
        if(ctx) JS_FreeValue(ctx, v);
    }

    bool isError() const { return JS_IsError(ctx, v); }

    /** Conversion helper function: value.as<T>()
     * @tparam T type to convert to
     * @return type returned by js_traits<std::decay_t<T>>::unwrap that should be implicitly convertible to T
     * */
    template <typename T>
    auto as() const { return js_traits<std::decay_t<T>>::unwrap(ctx, v); }

    /** Explicit conversion: static_cast<T>(value) or (T)value */
    template <typename T>
    explicit operator T() const { return as<T>(); }

    JSValue release() noexcept// dont call freevalue
    {
        ctx = nullptr;
        return v;
    }

    /** Implicit conversion to JSValue (rvalue only). Example: JSValue v = std::move(value); */
    operator JSValue()&& noexcept { return release(); }


    /** Access JS properties. Returns proxy type which is implicitly convertible to qjs::Value */
    template <typename Key>
    detail::property_proxy<Key> operator [](Key key)
    {
        assert(ctx && "Trying to access properties of Value with no JSContext");
        return {ctx, JS_DupValue(ctx, v), std::move(key)};
    }


    // add("f", []() {...});
    template <typename Function>
    Value& add(const char * name, Function&& f)
    {
        (*this)[name] = js_traits<decltype(std::function{std::forward<Function>(f)})>::wrap(ctx,
                                                                                            std::forward<Function>(f));
        return *this;
    }

    // add<&f>("f");
    // add<&T::f>("f");
    template <auto F>
    std::enable_if_t<std::is_member_function_pointer_v<decltype(F)> || std::is_function_v<std::remove_pointer_t<decltype(F)>>, Value&>
    add(const char * name)
    {
        (*this)[name] = fwrapper<F>{name};
        return *this;
    }

    // add_getter_setter<&T::get_member, &T::set_member>("member");
    template <auto FGet, auto FSet>
    Value& add_getter_setter(const char * name)
    {
        auto prop = JS_NewAtom(ctx, name);
        using fgetter = fwrapper<FGet, true>;
        using fsetter = fwrapper<FSet, true>;
        int ret = JS_DefinePropertyGetSet(ctx, v, prop,
                                          js_traits<fgetter>::wrap(ctx, fgetter{name}),
                                          js_traits<fsetter>::wrap(ctx, fsetter{name}),
                                          JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE | JS_PROP_ENUMERABLE
        );
        JS_FreeAtom(ctx, prop);
        if(ret < 0)
            throw exception{ctx};
        return *this;
    }

    // add_getter<&T::get_member>("member");
    template <auto FGet>
    Value& add_getter(const char * name)
    {
        auto prop = JS_NewAtom(ctx, name);
        using fgetter = fwrapper<FGet, true>;
        int ret = JS_DefinePropertyGetSet(ctx, v, prop,
                                          js_traits<fgetter>::wrap(ctx, fgetter{name}),
                                          JS_UNDEFINED,
                                          JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE
        );
        JS_FreeAtom(ctx, prop);
        if(ret < 0)
            throw exception{ctx};
        return *this;
    }

    // add<&T::member>("member");
    template <auto M>
    std::enable_if_t<std::is_member_object_pointer_v<decltype(M)>, Value&>
    add(const char * name)
    {
        if constexpr (detail::get_set<M>::is_const::value)
            return add_getter<detail::get_set<M>::get>(name);
        else
            return add_getter_setter<detail::get_set<M>::get, detail::get_set<M>::set>(name);
    }

    // add<&T::static_member>("static_member");
    template <auto M>
    std::enable_if_t<std::is_pointer_v<decltype(M)> && !std::is_function_v<std::remove_pointer_t<decltype(M)>> , Value&>
    add(const char * name)
    {
        if constexpr (detail::get_set<M>::is_const::value)
            return add_getter<detail::get_set<M>::get>(name);
        else
            return add_getter_setter<detail::get_set<M>::get, detail::get_set<M>::set>(name);
    }

    std::string
    toJSON(const Value& replacer = JS_UNDEFINED, const Value& space = JS_UNDEFINED)
    {
        assert(ctx);
        assert(!replacer.ctx || ctx == replacer.ctx);
        assert(!space.ctx || ctx == space.ctx);
        return (std::string) Value{ctx, JS_JSONStringify(ctx, v, replacer.v, space.v)};
    }

    /** same as Context::eval() but with this Value as 'this' */
    Value evalThis(std::string_view buffer, const char * filename = "<evalThis>", int flags = 0)
    {
        assert(buffer.data()[buffer.size()] == '\0' && "eval buffer is not null-terminated"); // JS_Eval requirement
        assert(ctx);
        return Value{ctx, JS_EvalThis(ctx, v, buffer.data(), buffer.size(), filename, flags)};
    }

};

/** Thin wrapper over JSRuntime * rt
 * Calls JS_FreeRuntime on destruction. noncopyable.
 */
class Runtime
{
public:
    JSRuntime * rt;

    Runtime()
    {
        rt = JS_NewRuntime();
        if(!rt)
            throw std::runtime_error{"qjs: Cannot create runtime"};

        JS_SetHostUnhandledPromiseRejectionTracker(rt, promise_unhandled_rejection_tracker, NULL);
        JS_SetModuleLoaderFunc(rt, nullptr, module_loader, nullptr);
    }

    // noncopyable
    Runtime(const Runtime&) = delete;

    ~Runtime()
    {
        JS_FreeRuntime(rt);
    }

    /// @return pointer to qjs::Context of the executed job or nullptr if no job is pending
    Context * executePendingJob();

    bool isJobPending() const {
        return JS_IsJobPending(rt);
    }

private:
    static void promise_unhandled_rejection_tracker(JSContext *ctx, JSValueConst promise,
                                                    JSValueConst reason, JS_BOOL is_handled, void *opaque);

    static JSModuleDef *module_loader(JSContext *ctx,
                                      const char *module_name, void *opaque);
};

namespace detail {

inline std::optional<std::string> readFile(std::filesystem::path const & filepath)
{
    if (!std::filesystem::exists(filepath)) return std::nullopt;
    std::ifstream f(filepath, std::ios::in | std::ios::binary);
    if (!f.is_open()) return std::nullopt;
    std::stringstream sstream;
    sstream << f.rdbuf();
    return sstream.str();
}

inline std::string toUri(std::string_view filename) {
    auto fname = std::string{filename};
    if (fname.find("://") < fname.find("/")) return fname;

    auto fpath = std::filesystem::path(fname);
    if (!fpath.is_absolute()) {
        fpath = "." / fpath;
    }
    fpath = std::filesystem::weakly_canonical(fpath);
    fname = "file://" + fpath.generic_string();
    return fname;
}

}

/** Wrapper over JSContext * ctx
 * Calls JS_SetContextOpaque(ctx, this); on construction and JS_FreeContext on destruction
 */
class Context
{
public:
    JSContext * ctx;

    /** Module wrapper
     * Workaround for lack of opaque pointer for module load function by keeping a list of modules in qjs::Context.
     */
    class Module
    {
        friend class Context;

        JSModuleDef * m;
        JSContext * ctx;
        const char * name;

        using nvp = std::pair<const char *, Value>;
        std::vector<nvp> exports;
    public:
        Module(JSContext * ctx, const char * name) : ctx(ctx), name(name)
        {
            m = JS_NewCModule(ctx, name, [](JSContext * ctx, JSModuleDef * m) noexcept {
                auto& context = Context::get(ctx);
                auto it = std::find_if(context.modules.begin(), context.modules.end(),
                                       [m](const Module& module) { return module.m == m; });
                if(it == context.modules.end())
                    return -1;
                for(const auto& e : it->exports)
                {
                    if(JS_SetModuleExport(ctx, m, e.first, JS_DupValue(ctx, e.second.v)) != 0)
                        return -1;
                }
                return 0;
            });
            if(!m)
                throw exception{ctx};
        }

        Module& add(const char * name, JSValue&& value)
        {
            exports.push_back({name, {ctx, std::move(value)}});
            JS_AddModuleExport(ctx, m, name);
            return *this;
        }


        template <typename T>
        Module& add(const char * name, T&& value)
        {
            return add(name, js_traits<T>::wrap(ctx, std::forward<T>(value)));
        }

        Module(const Module&) = delete;

        Module(Module&&) = default;
        //Module& operator=(Module&&) = default;


        // function wrappers

        /** Add free function F.
         * Example:
         * module.function<static_cast<double (*)(double)>(&::sin)>("sin");
         */
        template <auto F>
        Module& function(const char * name)
        {
            return add(name, qjs::fwrapper<F>{name});
        }

        /** Add function object f.
         * Slower than template version.
         * Example: module.function("sin", [](double x) { return ::sin(x); });
         */
        template <typename F>
        Module& function(const char * name, F&& f)
        {
            return add(name, js_traits<decltype(std::function{std::forward<F>(f)})>::wrap(ctx, std::forward<F>(f)));
        }

        // class register wrapper
    private:
        /** Helper class to register class members and constructors.
         * See fun, constructor.
         * Actual registration occurs at object destruction.
         */
        template <class T>
        class class_registrar
        {
            const char * name;
            qjs::Value prototype;
            qjs::Context::Module& module;
            qjs::Context& context;
            qjs::Value ctor; // last added constructor
        public:
            explicit class_registrar(const char * name, qjs::Context::Module& module, qjs::Context& context) :
                    name(name),
                    prototype(context.newObject()),
                    module(module),
                    context(context),
                    ctor(JS_NULL)
            {
            }

            class_registrar(const class_registrar&) = delete;

            /** Add functional object f
             */
            template <typename F>
            class_registrar& fun(const char * name, F&& f)
            {
                prototype[name] = std::forward<F>(f);
                return *this;
            }

            /** Add class member function or class member variable F
             * Example:
             * struct T { int var; int func(); }
             * auto& module = context.addModule("module");
             * module.class_<T>("T").fun<&T::var>("var").fun<&T::func>("func");
             */
            template <auto F>
            class_registrar& fun(const char * name)
            {
                js_traits<std::shared_ptr<T>>::template ensureCanCastToBase<F>();
                prototype.add<F>(name);
                return *this;
            }

            /** Add a static member or function to the last added constructor.
             * Example:
             *  struct T { static int var; static int func(); }
             *  module.class_<T>("T").contructor<>("T").static_fun<&T::var>("var").static_fun<&T::func>("func");
             */
            template <auto F>
            class_registrar& static_fun(const char * name)
            {
                assert(!JS_IsNull(ctor.v) && "You should call .constructor before .static_fun");
                js_traits<qjs::shared_ptr<T>>::template ensureCanCastToBase<F>();
                ctor.add<F>(name);
                return *this;
            }

            /** Add a property with custom getter and setter.
             * Example:
             * module.class_<T>("T").property<&T::getX, &T::setX>("x");
             */
            template <auto FGet, auto FSet = nullptr>
            class_registrar& property(const char * name)
            {
                js_traits<std::shared_ptr<T>>::template ensureCanCastToBase<FGet>();
                js_traits<std::shared_ptr<T>>::template ensureCanCastToBase<FSet>();
                if constexpr (std::is_same_v<decltype(FSet), std::nullptr_t>)
                    prototype.add_getter<FGet>(name);
                else
                    prototype.add_getter_setter<FGet, FSet>(name);
                return *this;
            }

            /** Add class constructor
             * @tparam Args contructor arguments
             * @param name constructor name (if not specified class name will be used)
             */
            template <typename... Args>
            class_registrar& constructor(const char * name = nullptr)
            {
                if(!name)
                    name = this->name;
                ctor = context.newValue(qjs::ctor_wrapper<T, Args...>{name});
                JS_SetConstructor(context.ctx, ctor.v, prototype.v);
                module.add(name, qjs::Value{ctor});
                return *this;
            }

            /** Sets the base class
             * @tparam B base class
             */
            template <class B>
            class_registrar& base()
            {
                static_assert(!std::is_same_v<B, T>, "Type cannot be a base of itself");
                assert(js_traits<std::shared_ptr<B>>::QJSClassId && "base class is not registered");
                js_traits<std::shared_ptr<T>>::template ensureCanCastToBase<B>();
                auto base_proto = JS_GetClassProto(context.ctx, js_traits<std::shared_ptr<B>>::QJSClassId);
                int err = JS_SetPrototype(context.ctx, prototype.v, base_proto);
                JS_FreeValue(context.ctx, base_proto);
                if(err < 0)
                    throw exception{context.ctx};
                return *this;
            }

            /** All qjs::Value members of T should be marked by mark<> for QuickJS garbage collector
             * so that the cycle removal algorithm can find the other objects referenced by this object.
             */
            template <Value T::* V>
            class_registrar& mark()
            {
                js_traits<std::shared_ptr<T>>::markOffsets.push_back(V);
                return *this;
            }


            ~class_registrar()
            {
                context.registerClass<T>(name, std::move(prototype));
            }
        };

    public:
        /** Add class to module.
         * See \ref class_registrar.
         */
        template <class T>
        class_registrar<T> class_(const char * name)
        {
            return class_registrar<T>{name, *this, qjs::Context::get(ctx)};
        }

    };

    std::vector<Module> modules;
private:
    void init()
    {
        JS_SetContextOpaque(ctx, this);
        js_traits<detail::function>::register_class(ctx, "C++ function");
    }

public:
    Context(Runtime& rt) : Context(rt.rt) {}

    Context(JSRuntime * rt)
    {
        ctx = JS_NewContext(rt);
        if(!ctx)
            throw std::runtime_error{"qjs: Cannot create context"};
        init();
    }

    Context(JSContext * ctx) : ctx{ctx}
    {
        init();
    }

    // noncopyable
    Context(const Context&) = delete;

    ~Context()
    {
        modules.clear();
        JS_FreeContext(ctx);
    }

    /** Callback triggered when a Promise rejection won't ever be handled */
    std::function<void(Value)> onUnhandledPromiseRejection;

    /** Data type returned by the moduleLoader function */
    struct ModuleData {
        std::optional<std::string> source, url;
        ModuleData() : source(std::nullopt), url(std::nullopt) {}
        ModuleData(std::optional<std::string> source) : source(std::move(source)), url(std::nullopt) {}
        ModuleData(std::optional<std::string> url, std::optional<std::string> source) : source(std::move(source)), url(std::move(url)) {}
    };

    /** Function called to obtain the source of a module */
    std::function<ModuleData(std::string_view)> moduleLoader =
        [](std::string_view filename) -> ModuleData {
            return ModuleData{ detail::toUri(filename), detail::readFile(filename) };
        };

    template <typename Function>
    void enqueueJob(Function && job);

    /** Create module and return a reference to it */
    Module& addModule(const char * name)
    {
        modules.emplace_back(ctx, name);
        return modules.back();
    }

    /** returns globalThis */
    Value global() { return Value{ctx, JS_GetGlobalObject(ctx)}; }

    /** returns new Object() */
    Value newObject() { return Value{ctx, JS_NewObject(ctx)}; }

    /** returns JS value converted from c++ object val */
    template <typename T>
    Value newValue(T&& val) { return Value{ctx, std::forward<T>(val)}; }

    /** returns current exception associated with context and clears it. Should be called when qjs::exception is caught */
    Value getException() { return Value{ctx, JS_GetException(ctx)}; }

    /** Register class T for conversions to/from std::shared_ptr<T> to work.
     * Wherever possible module.class_<T>("T")... should be used instead.
     * @tparam T class type
     * @param name class name in JS engine
     * @param proto JS class prototype or JS_UNDEFINED
     */
    template <class T>
    void registerClass(const char * name, JSValue proto = JS_NULL)
    {
        js_traits<std::shared_ptr<T>>::register_class(ctx, name, proto);
    }

    /// @see JS_Eval
    Value eval(std::string_view buffer, const char * filename = "<eval>", int flags = 0)
    {
        assert(buffer.data()[buffer.size()] == '\0' && "eval buffer is not null-terminated"); // JS_Eval requirement
        JSValue v = JS_Eval(ctx, buffer.data(), buffer.size(), filename, flags);
        return Value{ctx, std::move(v)};
    }

    Value evalFile(const char * filename, int flags = 0)
    {
        auto buf = detail::readFile(filename);
        if (!buf)
            throw std::runtime_error{std::string{"evalFile: can't read file: "} + filename};
        return eval(*buf, filename, flags);
    }

    /// @see JS_ParseJSON2
    Value fromJSON(std::string_view buffer, const char * filename = "<fromJSON>", int flags = 0)
    {
        assert(buffer.data()[buffer.size()] == '\0' &&
               "fromJSON buffer is not null-terminated"); // JS_ParseJSON requirement
        return Value{ctx, JS_ParseJSON2(ctx, buffer.data(), buffer.size(), filename, flags)};
    }

    /** Get qjs::Context from JSContext opaque pointer */
    static Context& get(JSContext * ctx)
    {
        void * ptr = JS_GetContextOpaque(ctx);
        assert(ptr);
        return *static_cast<Context *>(ptr);
    }
};

/** Conversion traits for Value.
 */
template <>
struct js_traits<Value>
{
    static Value unwrap(JSContext * ctx, JSValueConst v)
    {
        return Value{ctx, JS_DupValue(ctx, v)};
    }

    static JSValue wrap(JSContext * ctx, Value v) noexcept
    {
        assert(JS_GetRuntime(ctx) == JS_GetRuntime(v.ctx));
        return v.release();
    }
};

/** Convert to/from std::function. Actually accepts/returns callable object that is compatible with function<R (Args...)>.
 * @tparam R return type
 * @tparam Args argument types
 */
template <typename R, typename... Args>
struct js_traits<std::function<R(Args...)>, int>
{
    static auto unwrap(JSContext * ctx, JSValueConst fun_obj)
    {
        const int argc = sizeof...(Args);
        if constexpr(argc == 0)
        {
            return [jsfun_obj = Value{ctx, JS_DupValue(ctx, fun_obj)}]() -> R {
                JSValue result = JS_Call(jsfun_obj.ctx, jsfun_obj.v, JS_UNDEFINED, 0, nullptr);
                if(JS_IsException(result))
                    throw exception{jsfun_obj.ctx};
                return detail::unwrap_free<R>(jsfun_obj.ctx, result);
            };
        }
        else
        {
            return [jsfun_obj = Value{ctx, JS_DupValue(ctx, fun_obj)}](Args ... args) -> R {
                const int argc = sizeof...(Args);
                JSValue argv[argc];
                detail::wrap_args(jsfun_obj.ctx, argv, std::forward<Args>(args)...);
                JSValue result = JS_Call(jsfun_obj.ctx, jsfun_obj.v, JS_UNDEFINED, argc,
                                         const_cast<JSValueConst *>(argv));
                for(int i = 0; i < argc; i++) JS_FreeValue(jsfun_obj.ctx, argv[i]);
                if(JS_IsException(result))
                    throw exception{jsfun_obj.ctx};
                return detail::unwrap_free<R>(jsfun_obj.ctx, result);
            };
        }
    }

    /** Convert from function object functor to JSValue.
     * Uses detail::function for type-erasure.
     */
    template <typename Functor>
    static JSValue wrap(JSContext * ctx, Functor&& functor) noexcept
    {
        using detail::function;
        assert(js_traits<function>::QJSClassId);
        auto obj = JS_NewObjectClass(ctx, js_traits<function>::QJSClassId);
        if(JS_IsException(obj))
            return obj;
        try
        {
            auto fptr = function::create(JS_GetRuntime(ctx), std::forward<Functor>(functor));
            fptr->invoker = [](function * self, JSContext * ctx, JSValueConst this_value, int argc,
                               JSValueConst * argv) {
                assert(self);
                auto f = reinterpret_cast<std::decay_t<Functor> *>(&self->functor);
                return detail::wrap_call<R, Args...>(ctx, *f, argc, argv);
            };
            JS_SetOpaque(obj, fptr);
            return obj;
        }
        catch(const std::exception& e)
        {
            JS_ThrowInternalError(ctx, "%s", e.what());
            return JS_EXCEPTION;
        }
        catch(...)
        {
            JS_ThrowInternalError(ctx, "Unknown errror");
            return JS_EXCEPTION;
        }
    }
};

namespace detail {

template<typename T, typename = void>
struct is_callable : std::is_function<T> { };

template<typename T>
struct is_callable<T, std::enable_if_t<std::is_same_v<decltype(void(&T::operator())), void>>> : std::true_type { };

template<typename T>
inline constexpr bool is_callable_v = is_callable<T>::value;

}

template <typename Function>
struct js_traits<Function, std::enable_if_t<detail::is_callable_v<Function>>> {
    static auto unwrap(JSContext * ctx, JSValueConst fun_obj)
    {
        return js_traits<
            decltype(std::function{std::declval<Function>()}),
            int
        >::unwrap(ctx, fun_obj);
    }

    template <typename Functor>
    static JSValue wrap(JSContext * ctx, Functor&& functor)
    {
        return js_traits<
            decltype(std::function{std::declval<Function>()}),
            int
        >::wrap(ctx, std::forward<Functor>(functor));
    }
};

/** Convert from std::vector<T> to Array and vice-versa. If Array holds objects that are non-convertible to T throws qjs::exception */
template <class T>
struct js_traits<std::vector<T>>
{
    static JSValue wrap(JSContext * ctx, const std::vector<T>& arr) noexcept
    {
        try
        {
            auto jsarray = Value{ctx, JS_NewArray(ctx)};
            for(uint32_t i = 0; i < (uint32_t) arr.size(); i++)
                jsarray[i] = arr[i];
            return jsarray.release();
        }
        catch(exception)
        {
            return JS_EXCEPTION;
        }
        catch (std::exception const & err)
        {
            JS_ThrowInternalError(ctx, "%s", err.what());
            return JS_EXCEPTION;
        }
        catch (...)
        {
            JS_ThrowInternalError(ctx, "Unknown error");
            return JS_EXCEPTION;
        }
    }

    static std::vector<T> unwrap(JSContext * ctx, JSValueConst jsarr)
    {
        int e = JS_IsArray(ctx, jsarr);
        if(e == 0)
            JS_ThrowTypeError(ctx, "js_traits<std::vector<T>>::unwrap expects array");
        if(e <= 0)
            throw exception{ctx};
        Value jsarray{ctx, JS_DupValue(ctx, jsarr)};
        std::vector<T> arr;
        auto len = static_cast<int32_t>(jsarray["length"]);
        arr.reserve((uint32_t) len);
        for(uint32_t i = 0; i < (uint32_t) len; i++)
            arr.push_back(static_cast<T>(jsarray[i]));
        return arr;
    }
};


template <typename U, typename V>
struct js_traits<std::pair<U, V>>
{
    static JSValue wrap(JSContext * ctx, std::pair<U, V> obj) noexcept
    {
        try
        {
            auto jsarray = Value{ctx, JS_NewArray(ctx)};
            jsarray[uint32_t(0)] = std::move(obj.first);
            jsarray[uint32_t(1)] = std::move(obj.second);
            return jsarray.release();
        }
        catch(exception)
        {
            return JS_EXCEPTION;
        }
        catch (std::exception const & err)
        {
            JS_ThrowInternalError(ctx, "%s", err.what());
            return JS_EXCEPTION;
        }
        catch (...)
        {
            JS_ThrowInternalError(ctx, "Unknown error");
            return JS_EXCEPTION;
        }
    }

    static std::pair<U, V> unwrap(JSContext * ctx, JSValueConst jsarr)
    {
        int e = JS_IsArray(ctx, jsarr);
        if(e == 0)
            JS_ThrowTypeError(ctx, "js_traits<%s>::unwrap expects array", QJSPP_TYPENAME(std::pair<U, V>));
        if(e <= 0)
            throw exception{ctx};
        Value jsarray{ctx, JS_DupValue(ctx, jsarr)};
        const auto len = static_cast<uint32_t>(jsarray["length"]);
        if(len != 2)
        {
            JS_ThrowTypeError(ctx, "js_traits<%s>::unwrap expected array of length 2, got length %d",
                              QJSPP_TYPENAME(std::pair<U, V>), len);
            throw exception{ctx};
        }
        return std::pair<U, V>{
                static_cast<U>(jsarray[uint32_t(0)]),
                static_cast<V>(jsarray[uint32_t(1)])
        };
    }
};

/** Conversions for std::optional.
 * Unlike other types does not throw on unwrap but returns nullopt.
 * Converts std::nullopt to null.
 */
template <typename T>
struct js_traits<std::optional<T>>
{
    /** Wraps T or null. */
    static JSValue wrap(JSContext * ctx, std::optional<T> obj) noexcept
    {
        if(obj)
            return js_traits<std::decay_t<T>>::wrap(ctx, *obj);
        return JS_NULL;
    }

    /** If conversion to T fails returns std::nullopt. */
    static auto unwrap(JSContext * ctx, JSValueConst v) noexcept -> std::optional<decltype(js_traits<std::decay_t<T>>::unwrap(ctx, v))>
    {
        try
        {
            if(JS_IsNull(v))
                return std::nullopt;
            return js_traits<std::decay_t<T>>::unwrap(ctx, v);
        }
        catch(exception)
        {
            // ignore and clear exception
            JS_FreeValue(ctx, JS_GetException(ctx));
        }
        return std::nullopt;
    }
};


namespace detail {
template <typename Key>
property_proxy<Key>::operator Value() const
{
    return as<Value>();
}
}

template <typename Function>
void Context::enqueueJob(Function && job) {
    JSValue job_val = js_traits<std::function<void()>>::wrap(ctx, std::forward<Function>(job));
    JSValueConst arg = job_val;
    int err = JS_EnqueueJob(ctx, [](JSContext *ctx, int argc, JSValueConst *argv){
        try
        {
            assert(argc >= 1);
            js_traits<std::function<void()>>::unwrap(ctx, argv[0])();
        }
        catch (exception)
        {
            return JS_EXCEPTION;
        }
        catch (std::exception const & err)
        {
            JS_ThrowInternalError(ctx, "%s", err.what());
            return JS_EXCEPTION;
        }
        catch (...)
        {
            JS_ThrowInternalError(ctx, "Unknown error");
            return JS_EXCEPTION;
        }
        return JS_UNDEFINED;
    }, 1, &arg);
    JS_FreeValue(ctx, job_val);
    if(err < 0)
        throw exception{ctx};
}

inline Context & exception::context() const {
    return Context::get(ctx);
}

inline Value exception::get() {
    return context().getException();
}

inline void Runtime::promise_unhandled_rejection_tracker(JSContext *ctx, JSValueConst promise,
                                                         JSValueConst reason, JS_BOOL is_handled, void *opaque)
{
    auto & context = Context::get(ctx);
    if (context.onUnhandledPromiseRejection) {
        context.onUnhandledPromiseRejection(context.newValue(JS_DupValue(ctx, reason)));
    }
}

inline JSModuleDef * Runtime::module_loader(JSContext *ctx,
                                            const char *module_name, void *opaque)
{
    Context::ModuleData data;
    auto & context = Context::get(ctx);

    try {
        if (context.moduleLoader) data = context.moduleLoader(module_name);

        if (!data.source) {
            JS_ThrowReferenceError(ctx, "could not load module filename '%s'", module_name);
            return NULL;
        }

        if (!data.url) data.url = module_name;

        // compile the module
        auto func_val = context.eval(*data.source, module_name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
        assert(JS_VALUE_GET_TAG(func_val.v) == JS_TAG_MODULE);
        JSModuleDef * m = reinterpret_cast<JSModuleDef *>(JS_VALUE_GET_PTR(func_val.v));

        // set import.meta
        auto meta = context.newValue(JS_GetImportMeta(ctx, m));
        meta["url"] = *data.url;
        meta["main"] = false;

        return m;
    }
    catch(exception)
    {
        return NULL;
    }
    catch (std::exception const & err)
    {
        JS_ThrowInternalError(ctx, "%s", err.what());
        return NULL;
    }
    catch (...)
    {
        JS_ThrowInternalError(ctx, "Unknown error");
        return NULL;
    }
}


inline Context * Runtime::executePendingJob() {
    JSContext * ctx;
    auto err = JS_ExecutePendingJob(rt, &ctx);
    if (err == 0) {
        // There was no job to run
        return nullptr;
    } else if (err < 0) {
        throw exception{ctx};
    }
    return &Context::get(ctx);
}

} // namespace qjs
