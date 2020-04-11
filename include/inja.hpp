// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_INJA_HPP_
#define INCLUDE_INJA_INJA_HPP_

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// #include "exceptions.hpp"
// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_EXCEPTIONS_HPP_
#define INCLUDE_INJA_EXCEPTIONS_HPP_

#include <stdexcept>
#include <string>


namespace inja {

struct SourceLocation {
  size_t line;
  size_t column;
};

struct InjaError : public std::runtime_error {
  std::string type;
  std::string message;

  bool has_location {false};
  SourceLocation location;

  InjaError(const std::string& type, const std::string& message)
    : std::runtime_error("[inja.exception." + type + "] " + message), type(type), message(message) { }

  InjaError(const std::string& type, const std::string& message, SourceLocation location)
    : std::runtime_error(
      "[inja.exception." + type + "] (at " + std::to_string(location.line) + ":" + std::to_string(location.column) + ") " + message
    ), type(type), message(message), has_location(true), location(location) { }
};

struct ParserError : public InjaError {
  ParserError(const std::string& message) : InjaError("parser_error", message) { }
  ParserError(const std::string& message, SourceLocation location)
    : InjaError("parser_error", message, location) { }
};

struct RenderError : public InjaError {
  RenderError(const std::string& message) : InjaError("render_error", message) { }
  RenderError(const std::string& message, SourceLocation location)
    : InjaError("render_error", message, location) { }
};

struct FileError : public InjaError {
  FileError(const std::string& message) : InjaError("file_error", message) { }
  FileError(const std::string& message, SourceLocation location)
    : InjaError("file_error", message, location) { }
};

struct JsonError : public InjaError {
  JsonError(const std::string& message) : InjaError("json_error", message) { }
  JsonError(const std::string& message, SourceLocation location)
    : InjaError("json_error", message, location) { }
};

}  // namespace inja

#endif  // INCLUDE_INJA_EXCEPTIONS_HPP_

// #include "environment.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_ENVIRONMENT_HPP_
#define INCLUDE_INJA_ENVIRONMENT_HPP_

#include <memory>
#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

// #include "config.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_CONFIG_HPP_
#define INCLUDE_INJA_CONFIG_HPP_

#include <functional>
#include <string>

// #include "string_view.hpp"
// Copyright 2017-2019 by Martin Moene
//
// string-view lite, a C++17-like string_view for C++98 and later.
// For more information see https://github.com/martinmoene/string-view-lite
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)



#ifndef NONSTD_SV_LITE_H_INCLUDED
#define NONSTD_SV_LITE_H_INCLUDED

#define string_view_lite_MAJOR  1
#define string_view_lite_MINOR  4
#define string_view_lite_PATCH  0

#define string_view_lite_VERSION  nssv_STRINGIFY(string_view_lite_MAJOR) "." nssv_STRINGIFY(string_view_lite_MINOR) "." nssv_STRINGIFY(string_view_lite_PATCH)

#define nssv_STRINGIFY(  x )  nssv_STRINGIFY_( x )
#define nssv_STRINGIFY_( x )  #x

// string-view lite configuration:

#define nssv_STRING_VIEW_DEFAULT  0
#define nssv_STRING_VIEW_NONSTD   1
#define nssv_STRING_VIEW_STD      2

#if !defined( nssv_CONFIG_SELECT_STRING_VIEW )
# define nssv_CONFIG_SELECT_STRING_VIEW  ( nssv_HAVE_STD_STRING_VIEW ? nssv_STRING_VIEW_STD : nssv_STRING_VIEW_NONSTD )
#endif

#if defined( nssv_CONFIG_SELECT_STD_STRING_VIEW ) || defined( nssv_CONFIG_SELECT_NONSTD_STRING_VIEW )
# error nssv_CONFIG_SELECT_STD_STRING_VIEW and nssv_CONFIG_SELECT_NONSTD_STRING_VIEW are deprecated and removed, please use nssv_CONFIG_SELECT_STRING_VIEW=nssv_STRING_VIEW_...
#endif

#ifndef  nssv_CONFIG_STD_SV_OPERATOR
# define nssv_CONFIG_STD_SV_OPERATOR  0
#endif

#ifndef  nssv_CONFIG_USR_SV_OPERATOR
# define nssv_CONFIG_USR_SV_OPERATOR  1
#endif

#ifdef   nssv_CONFIG_CONVERSION_STD_STRING
# define nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS   nssv_CONFIG_CONVERSION_STD_STRING
# define nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS  nssv_CONFIG_CONVERSION_STD_STRING
#endif

#ifndef  nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS
# define nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS  1
#endif

#ifndef  nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS
# define nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS  1
#endif

// Control presence of exception handling (try and auto discover):

#ifndef nssv_CONFIG_NO_EXCEPTIONS
# if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#  define nssv_CONFIG_NO_EXCEPTIONS  0
# else
#  define nssv_CONFIG_NO_EXCEPTIONS  1
# endif
#endif

// C++ language version detection (C++20 is speculative):
// Note: VC14.0/1900 (VS2015) lacks too much from C++14.

#ifndef   nssv_CPLUSPLUS
# if defined(_MSVC_LANG ) && !defined(__clang__)
#  define nssv_CPLUSPLUS  (_MSC_VER == 1900 ? 201103L : _MSVC_LANG )
# else
#  define nssv_CPLUSPLUS  __cplusplus
# endif
#endif

#define nssv_CPP98_OR_GREATER  ( nssv_CPLUSPLUS >= 199711L )
#define nssv_CPP11_OR_GREATER  ( nssv_CPLUSPLUS >= 201103L )
#define nssv_CPP11_OR_GREATER_ ( nssv_CPLUSPLUS >= 201103L )
#define nssv_CPP14_OR_GREATER  ( nssv_CPLUSPLUS >= 201402L )
#define nssv_CPP17_OR_GREATER  ( nssv_CPLUSPLUS >= 201703L )
#define nssv_CPP20_OR_GREATER  ( nssv_CPLUSPLUS >= 202000L )

// use C++17 std::string_view if available and requested:

#if nssv_CPP17_OR_GREATER && defined(__has_include )
# if __has_include( <string_view> )
#  define nssv_HAVE_STD_STRING_VIEW  1
# else
#  define nssv_HAVE_STD_STRING_VIEW  0
# endif
#else
# define  nssv_HAVE_STD_STRING_VIEW  0
#endif

#define  nssv_USES_STD_STRING_VIEW  ( (nssv_CONFIG_SELECT_STRING_VIEW == nssv_STRING_VIEW_STD) || ((nssv_CONFIG_SELECT_STRING_VIEW == nssv_STRING_VIEW_DEFAULT) && nssv_HAVE_STD_STRING_VIEW) )

#define nssv_HAVE_STARTS_WITH ( nssv_CPP20_OR_GREATER || !nssv_USES_STD_STRING_VIEW )
#define nssv_HAVE_ENDS_WITH     nssv_HAVE_STARTS_WITH

//
// Use C++17 std::string_view:
//

#if nssv_USES_STD_STRING_VIEW

#include <string_view>

// Extensions for std::string:

#if nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

namespace nonstd {

template< class CharT, class Traits, class Allocator = std::allocator<CharT> >
std::basic_string<CharT, Traits, Allocator>
to_string( std::basic_string_view<CharT, Traits> v, Allocator const & a = Allocator() )
{
    return std::basic_string<CharT,Traits, Allocator>( v.begin(), v.end(), a );
}

template< class CharT, class Traits, class Allocator >
std::basic_string_view<CharT, Traits>
to_string_view( std::basic_string<CharT, Traits, Allocator> const & s )
{
    return std::basic_string_view<CharT, Traits>( s.data(), s.size() );
}

// Literal operators sv and _sv:

#if nssv_CONFIG_STD_SV_OPERATOR

using namespace std::literals::string_view_literals;

#endif

#if nssv_CONFIG_USR_SV_OPERATOR

inline namespace literals {
inline namespace string_view_literals {


constexpr std::string_view operator "" _sv( const char* str, size_t len ) noexcept  // (1)
{
    return std::string_view{ str, len };
}

constexpr std::u16string_view operator "" _sv( const char16_t* str, size_t len ) noexcept  // (2)
{
    return std::u16string_view{ str, len };
}

constexpr std::u32string_view operator "" _sv( const char32_t* str, size_t len ) noexcept  // (3)
{
    return std::u32string_view{ str, len };
}

constexpr std::wstring_view operator "" _sv( const wchar_t* str, size_t len ) noexcept  // (4)
{
    return std::wstring_view{ str, len };
}

}} // namespace literals::string_view_literals

#endif // nssv_CONFIG_USR_SV_OPERATOR

} // namespace nonstd

#endif // nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

namespace nonstd {

using std::string_view;
using std::wstring_view;
using std::u16string_view;
using std::u32string_view;
using std::basic_string_view;

// literal "sv" and "_sv", see above

using std::operator==;
using std::operator!=;
using std::operator<;
using std::operator<=;
using std::operator>;
using std::operator>=;

using std::operator<<;

} // namespace nonstd

#else // nssv_HAVE_STD_STRING_VIEW

//
// Before C++17: use string_view lite:
//

// Compiler versions:
//
// MSVC++ 6.0  _MSC_VER == 1200 (Visual Studio 6.0)
// MSVC++ 7.0  _MSC_VER == 1300 (Visual Studio .NET 2002)
// MSVC++ 7.1  _MSC_VER == 1310 (Visual Studio .NET 2003)
// MSVC++ 8.0  _MSC_VER == 1400 (Visual Studio 2005)
// MSVC++ 9.0  _MSC_VER == 1500 (Visual Studio 2008)
// MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
// MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
// MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
// MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
// MSVC++ 14.1 _MSC_VER >= 1910 (Visual Studio 2017)

#if defined(_MSC_VER ) && !defined(__clang__)
# define nssv_COMPILER_MSVC_VER      (_MSC_VER )
# define nssv_COMPILER_MSVC_VERSION  (_MSC_VER / 10 - 10 * ( 5 + (_MSC_VER < 1900 ) ) )
#else
# define nssv_COMPILER_MSVC_VER      0
# define nssv_COMPILER_MSVC_VERSION  0
#endif

#define nssv_COMPILER_VERSION( major, minor, patch )  ( 10 * ( 10 * (major) + (minor) ) + (patch) )

#if defined(__clang__)
# define nssv_COMPILER_CLANG_VERSION  nssv_COMPILER_VERSION(__clang_major__, __clang_minor__, __clang_patchlevel__)
#else
# define nssv_COMPILER_CLANG_VERSION    0
#endif

#if defined(__GNUC__) && !defined(__clang__)
# define nssv_COMPILER_GNUC_VERSION  nssv_COMPILER_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
# define nssv_COMPILER_GNUC_VERSION    0
#endif

// half-open range [lo..hi):
#define nssv_BETWEEN( v, lo, hi ) ( (lo) <= (v) && (v) < (hi) )

// Presence of language and library features:

#ifdef _HAS_CPP0X
# define nssv_HAS_CPP0X  _HAS_CPP0X
#else
# define nssv_HAS_CPP0X  0
#endif

// Unless defined otherwise below, consider VC14 as C++11 for variant-lite:

#if nssv_COMPILER_MSVC_VER >= 1900
# undef  nssv_CPP11_OR_GREATER
# define nssv_CPP11_OR_GREATER  1
#endif

#define nssv_CPP11_90   (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1500)
#define nssv_CPP11_100  (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1600)
#define nssv_CPP11_110  (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1700)
#define nssv_CPP11_120  (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1800)
#define nssv_CPP11_140  (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1900)
#define nssv_CPP11_141  (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1910)

#define nssv_CPP14_000  (nssv_CPP14_OR_GREATER)
#define nssv_CPP17_000  (nssv_CPP17_OR_GREATER)

// Presence of C++11 language features:

#define nssv_HAVE_CONSTEXPR_11          nssv_CPP11_140
#define nssv_HAVE_EXPLICIT_CONVERSION   nssv_CPP11_140
#define nssv_HAVE_INLINE_NAMESPACE      nssv_CPP11_140
#define nssv_HAVE_NOEXCEPT              nssv_CPP11_140
#define nssv_HAVE_NULLPTR               nssv_CPP11_100
#define nssv_HAVE_REF_QUALIFIER         nssv_CPP11_140
#define nssv_HAVE_UNICODE_LITERALS      nssv_CPP11_140
#define nssv_HAVE_USER_DEFINED_LITERALS nssv_CPP11_140
#define nssv_HAVE_WCHAR16_T             nssv_CPP11_100
#define nssv_HAVE_WCHAR32_T             nssv_CPP11_100

#if ! ( ( nssv_CPP11_OR_GREATER && nssv_COMPILER_CLANG_VERSION ) || nssv_BETWEEN( nssv_COMPILER_CLANG_VERSION, 300, 400 ) )
# define nssv_HAVE_STD_DEFINED_LITERALS  nssv_CPP11_140
#else
# define nssv_HAVE_STD_DEFINED_LITERALS  0
#endif

// Presence of C++14 language features:

#define nssv_HAVE_CONSTEXPR_14          nssv_CPP14_000

// Presence of C++17 language features:

#define nssv_HAVE_NODISCARD             nssv_CPP17_000

// Presence of C++ library features:

#define nssv_HAVE_STD_HASH              nssv_CPP11_120

// C++ feature usage:

#if nssv_HAVE_CONSTEXPR_11
# define nssv_constexpr  constexpr
#else
# define nssv_constexpr  /*constexpr*/
#endif

#if  nssv_HAVE_CONSTEXPR_14
# define nssv_constexpr14  constexpr
#else
# define nssv_constexpr14  /*constexpr*/
#endif

#if nssv_HAVE_EXPLICIT_CONVERSION
# define nssv_explicit  explicit
#else
# define nssv_explicit  /*explicit*/
#endif

#if nssv_HAVE_INLINE_NAMESPACE
# define nssv_inline_ns  inline
#else
# define nssv_inline_ns  /*inline*/
#endif

#if nssv_HAVE_NOEXCEPT
# define nssv_noexcept  noexcept
#else
# define nssv_noexcept  /*noexcept*/
#endif

//#if nssv_HAVE_REF_QUALIFIER
//# define nssv_ref_qual  &
//# define nssv_refref_qual  &&
//#else
//# define nssv_ref_qual  /*&*/
//# define nssv_refref_qual  /*&&*/
//#endif

#if nssv_HAVE_NULLPTR
# define nssv_nullptr  nullptr
#else
# define nssv_nullptr  NULL
#endif

#if nssv_HAVE_NODISCARD
# define nssv_nodiscard  [[nodiscard]]
#else
# define nssv_nodiscard  /*[[nodiscard]]*/
#endif

// Additional includes:

#include <algorithm>
#include <cassert>
#include <iterator>
#include <limits>
#include <ostream>
#include <string>   // std::char_traits<>

#if ! nssv_CONFIG_NO_EXCEPTIONS
# include <stdexcept>
#endif

#if nssv_CPP11_OR_GREATER
# include <type_traits>
#endif

// Clang, GNUC, MSVC warning suppression macros:

#if defined(__clang__)
# pragma clang diagnostic ignored "-Wreserved-user-defined-literal"
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wuser-defined-literals"
#elif defined(__GNUC__)
# pragma  GCC  diagnostic push
# pragma  GCC  diagnostic ignored "-Wliteral-suffix"
#endif // __clang__

#if nssv_COMPILER_MSVC_VERSION >= 140
# define nssv_SUPPRESS_MSGSL_WARNING(expr)        [[gsl::suppress(expr)]]
# define nssv_SUPPRESS_MSVC_WARNING(code, descr)  __pragma(warning(suppress: code) )
# define nssv_DISABLE_MSVC_WARNINGS(codes)        __pragma(warning(push))  __pragma(warning(disable: codes))
#else
# define nssv_SUPPRESS_MSGSL_WARNING(expr)
# define nssv_SUPPRESS_MSVC_WARNING(code, descr)
# define nssv_DISABLE_MSVC_WARNINGS(codes)
#endif

#if defined(__clang__)
# define nssv_RESTORE_WARNINGS()  _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
# define nssv_RESTORE_WARNINGS()  _Pragma("GCC diagnostic pop")
#elif nssv_COMPILER_MSVC_VERSION >= 140
# define nssv_RESTORE_WARNINGS()  __pragma(warning(pop ))
#else
# define nssv_RESTORE_WARNINGS()
#endif

// Suppress the following MSVC (GSL) warnings:
// - C4455, non-gsl   : 'operator ""sv': literal suffix identifiers that do not
//                      start with an underscore are reserved
// - C26472, gsl::t.1 : don't use a static_cast for arithmetic conversions;
//                      use brace initialization, gsl::narrow_cast or gsl::narow
// - C26481: gsl::b.1 : don't use pointer arithmetic. Use span instead

nssv_DISABLE_MSVC_WARNINGS( 4455 26481 26472 )
//nssv_DISABLE_CLANG_WARNINGS( "-Wuser-defined-literals" )
//nssv_DISABLE_GNUC_WARNINGS( -Wliteral-suffix )

namespace nonstd { namespace sv_lite {

#if nssv_CPP11_OR_GREATER

namespace detail {

// Expect tail call optimization to make length() non-recursive:

template< typename CharT >
inline constexpr std::size_t length( CharT * s, std::size_t result = 0 )
{
    return *s == '\0' ? result : length( s + 1, result + 1 );
}

} // namespace detail

#endif // nssv_CPP11_OR_GREATER

template
<
    class CharT,
    class Traits = std::char_traits<CharT>
>
class basic_string_view;

//
// basic_string_view:
//

template
<
    class CharT,
    class Traits /* = std::char_traits<CharT> */
>
class basic_string_view
{
public:
    // Member types:

    typedef Traits traits_type;
    typedef CharT  value_type;

    typedef CharT       * pointer;
    typedef CharT const * const_pointer;
    typedef CharT       & reference;
    typedef CharT const & const_reference;

    typedef const_pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator< const_iterator > reverse_iterator;
    typedef	std::reverse_iterator< const_iterator > const_reverse_iterator;

    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  difference_type;

    // 24.4.2.1 Construction and assignment:

    nssv_constexpr basic_string_view() nssv_noexcept
        : data_( nssv_nullptr )
        , size_( 0 )
    {}

#if nssv_CPP11_OR_GREATER
    nssv_constexpr basic_string_view( basic_string_view const & other ) nssv_noexcept = default;
#else
    nssv_constexpr basic_string_view( basic_string_view const & other ) nssv_noexcept
        : data_( other.data_)
        , size_( other.size_)
    {}
#endif

    nssv_constexpr basic_string_view( CharT const * s, size_type count ) nssv_noexcept // non-standard noexcept
        : data_( s )
        , size_( count )
    {}

    nssv_constexpr basic_string_view( CharT const * s) nssv_noexcept // non-standard noexcept
        : data_( s )
#if nssv_CPP17_OR_GREATER
        , size_( Traits::length(s) )
#elif nssv_CPP11_OR_GREATER
        , size_( detail::length(s) )
#else
        , size_( Traits::length(s) )
#endif
    {}

    // Assignment:

#if nssv_CPP11_OR_GREATER
    nssv_constexpr14 basic_string_view & operator=( basic_string_view const & other ) nssv_noexcept = default;
#else
    nssv_constexpr14 basic_string_view & operator=( basic_string_view const & other ) nssv_noexcept
    {
        data_ = other.data_;
        size_ = other.size_;
        return *this;
    }
#endif

    // 24.4.2.2 Iterator support:

    nssv_constexpr const_iterator begin()  const nssv_noexcept { return data_;         }
    nssv_constexpr const_iterator end()    const nssv_noexcept { return data_ + size_; }

    nssv_constexpr const_iterator cbegin() const nssv_noexcept { return begin(); }
    nssv_constexpr const_iterator cend()   const nssv_noexcept { return end();   }

    nssv_constexpr const_reverse_iterator rbegin()  const nssv_noexcept { return const_reverse_iterator( end() );   }
    nssv_constexpr const_reverse_iterator rend()    const nssv_noexcept { return const_reverse_iterator( begin() ); }

    nssv_constexpr const_reverse_iterator crbegin() const nssv_noexcept { return rbegin(); }
    nssv_constexpr const_reverse_iterator crend()   const nssv_noexcept { return rend();   }

    // 24.4.2.3 Capacity:

    nssv_constexpr size_type size()     const nssv_noexcept { return size_; }
    nssv_constexpr size_type length()   const nssv_noexcept { return size_; }
    nssv_constexpr size_type max_size() const nssv_noexcept { return (std::numeric_limits< size_type >::max)(); }

    // since C++20
    nssv_nodiscard nssv_constexpr bool empty() const nssv_noexcept
    {
        return 0 == size_;
    }

    // 24.4.2.4 Element access:

    nssv_constexpr const_reference operator[]( size_type pos ) const
    {
        return data_at( pos );
    }

    nssv_constexpr14 const_reference at( size_type pos ) const
    {
#if nssv_CONFIG_NO_EXCEPTIONS
        assert( pos < size() );
#else
        if ( pos >= size() )
        {
            throw std::out_of_range("nonstd::string_view::at()");
        }
#endif
        return data_at( pos );
    }

    nssv_constexpr const_reference front() const { return data_at( 0 );          }
    nssv_constexpr const_reference back()  const { return data_at( size() - 1 ); }

    nssv_constexpr const_pointer   data()  const nssv_noexcept { return data_; }

    // 24.4.2.5 Modifiers:

    nssv_constexpr14 void remove_prefix( size_type n )
    {
        assert( n <= size() );
        data_ += n;
        size_ -= n;
    }

    nssv_constexpr14 void remove_suffix( size_type n )
    {
        assert( n <= size() );
        size_ -= n;
    }

    nssv_constexpr14 void swap( basic_string_view & other ) nssv_noexcept
    {
        using std::swap;
        swap( data_, other.data_ );
        swap( size_, other.size_ );
    }

    // 24.4.2.6 String operations:

    size_type copy( CharT * dest, size_type n, size_type pos = 0 ) const
    {
#if nssv_CONFIG_NO_EXCEPTIONS
        assert( pos <= size() );
#else
        if ( pos > size() )
        {
            throw std::out_of_range("nonstd::string_view::copy()");
        }
#endif
        const size_type rlen = (std::min)( n, size() - pos );

        (void) Traits::copy( dest, data() + pos, rlen );

        return rlen;
    }

    nssv_constexpr14 basic_string_view substr( size_type pos = 0, size_type n = npos ) const
    {
#if nssv_CONFIG_NO_EXCEPTIONS
        assert( pos <= size() );
#else
        if ( pos > size() )
        {
            throw std::out_of_range("nonstd::string_view::substr()");
        }
#endif
        return basic_string_view( data() + pos, (std::min)( n, size() - pos ) );
    }

    // compare(), 6x:

    nssv_constexpr14 int compare( basic_string_view other ) const nssv_noexcept // (1)
    {
        if ( const int result = Traits::compare( data(), other.data(), (std::min)( size(), other.size() ) ) )
        {
            return result;
        }

        return size() == other.size() ? 0 : size() < other.size() ? -1 : 1;
    }

    nssv_constexpr int compare( size_type pos1, size_type n1, basic_string_view other ) const // (2)
    {
        return substr( pos1, n1 ).compare( other );
    }

    nssv_constexpr int compare( size_type pos1, size_type n1, basic_string_view other, size_type pos2, size_type n2 ) const // (3)
    {
        return substr( pos1, n1 ).compare( other.substr( pos2, n2 ) );
    }

    nssv_constexpr int compare( CharT const * s ) const // (4)
    {
        return compare( basic_string_view( s ) );
    }

    nssv_constexpr int compare( size_type pos1, size_type n1, CharT const * s ) const // (5)
    {
        return substr( pos1, n1 ).compare( basic_string_view( s ) );
    }

    nssv_constexpr int compare( size_type pos1, size_type n1, CharT const * s, size_type n2 ) const // (6)
    {
        return substr( pos1, n1 ).compare( basic_string_view( s, n2 ) );
    }

    // 24.4.2.7 Searching:

    // starts_with(), 3x, since C++20:

    nssv_constexpr bool starts_with( basic_string_view v ) const nssv_noexcept  // (1)
    {
        return size() >= v.size() && compare( 0, v.size(), v ) == 0;
    }

    nssv_constexpr bool starts_with( CharT c ) const nssv_noexcept  // (2)
    {
        return starts_with( basic_string_view( &c, 1 ) );
    }

    nssv_constexpr bool starts_with( CharT const * s ) const  // (3)
    {
        return starts_with( basic_string_view( s ) );
    }

    // ends_with(), 3x, since C++20:

    nssv_constexpr bool ends_with( basic_string_view v ) const nssv_noexcept  // (1)
    {
        return size() >= v.size() && compare( size() - v.size(), npos, v ) == 0;
    }

    nssv_constexpr bool ends_with( CharT c ) const nssv_noexcept  // (2)
    {
        return ends_with( basic_string_view( &c, 1 ) );
    }

    nssv_constexpr bool ends_with( CharT const * s ) const  // (3)
    {
        return ends_with( basic_string_view( s ) );
    }

    // find(), 4x:

    nssv_constexpr14 size_type find( basic_string_view v, size_type pos = 0 ) const nssv_noexcept  // (1)
    {
        return assert( v.size() == 0 || v.data() != nssv_nullptr )
            , pos >= size()
            ? npos
            : to_pos( std::search( cbegin() + pos, cend(), v.cbegin(), v.cend(), Traits::eq ) );
    }

    nssv_constexpr14 size_type find( CharT c, size_type pos = 0 ) const nssv_noexcept  // (2)
    {
        return find( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr14 size_type find( CharT const * s, size_type pos, size_type n ) const  // (3)
    {
        return find( basic_string_view( s, n ), pos );
    }

    nssv_constexpr14 size_type find( CharT const * s, size_type pos = 0 ) const  // (4)
    {
        return find( basic_string_view( s ), pos );
    }

    // rfind(), 4x:

    nssv_constexpr14 size_type rfind( basic_string_view v, size_type pos = npos ) const nssv_noexcept  // (1)
    {
        if ( size() < v.size() )
        {
            return npos;
        }

        if ( v.empty() )
        {
            return (std::min)( size(), pos );
        }

        const_iterator last   = cbegin() + (std::min)( size() - v.size(), pos ) + v.size();
        const_iterator result = std::find_end( cbegin(), last, v.cbegin(), v.cend(), Traits::eq );

        return result != last ? size_type( result - cbegin() ) : npos;
    }

    nssv_constexpr14 size_type rfind( CharT c, size_type pos = npos ) const nssv_noexcept  // (2)
    {
        return rfind( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr14 size_type rfind( CharT const * s, size_type pos, size_type n ) const  // (3)
    {
        return rfind( basic_string_view( s, n ), pos );
    }

    nssv_constexpr14 size_type rfind( CharT const * s, size_type pos = npos ) const  // (4)
    {
        return rfind( basic_string_view( s ), pos );
    }

    // find_first_of(), 4x:

    nssv_constexpr size_type find_first_of( basic_string_view v, size_type pos = 0 ) const nssv_noexcept  // (1)
    {
        return pos >= size()
            ? npos
            : to_pos( std::find_first_of( cbegin() + pos, cend(), v.cbegin(), v.cend(), Traits::eq ) );
    }

    nssv_constexpr size_type find_first_of( CharT c, size_type pos = 0 ) const nssv_noexcept  // (2)
    {
        return find_first_of( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr size_type find_first_of( CharT const * s, size_type pos, size_type n ) const  // (3)
    {
        return find_first_of( basic_string_view( s, n ), pos );
    }

    nssv_constexpr size_type find_first_of(  CharT const * s, size_type pos = 0 ) const  // (4)
    {
        return find_first_of( basic_string_view( s ), pos );
    }

    // find_last_of(), 4x:

    nssv_constexpr size_type find_last_of( basic_string_view v, size_type pos = npos ) const nssv_noexcept  // (1)
    {
        return empty()
            ? npos
            : pos >= size()
            ? find_last_of( v, size() - 1 )
            : to_pos( std::find_first_of( const_reverse_iterator( cbegin() + pos + 1 ), crend(), v.cbegin(), v.cend(), Traits::eq ) );
    }

    nssv_constexpr size_type find_last_of( CharT c, size_type pos = npos ) const nssv_noexcept  // (2)
    {
        return find_last_of( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr size_type find_last_of( CharT const * s, size_type pos, size_type count ) const  // (3)
    {
        return find_last_of( basic_string_view( s, count ), pos );
    }

    nssv_constexpr size_type find_last_of( CharT const * s, size_type pos = npos ) const  // (4)
    {
        return find_last_of( basic_string_view( s ), pos );
    }

    // find_first_not_of(), 4x:

    nssv_constexpr size_type find_first_not_of( basic_string_view v, size_type pos = 0 ) const nssv_noexcept  // (1)
    {
        return pos >= size()
            ? npos
            : to_pos( std::find_if( cbegin() + pos, cend(), not_in_view( v ) ) );
    }

    nssv_constexpr size_type find_first_not_of( CharT c, size_type pos = 0 ) const nssv_noexcept  // (2)
    {
        return find_first_not_of( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr size_type find_first_not_of( CharT const * s, size_type pos, size_type count ) const  // (3)
    {
        return find_first_not_of( basic_string_view( s, count ), pos );
    }

    nssv_constexpr size_type find_first_not_of( CharT const * s, size_type pos = 0 ) const  // (4)
    {
        return find_first_not_of( basic_string_view( s ), pos );
    }

    // find_last_not_of(), 4x:

    nssv_constexpr size_type find_last_not_of( basic_string_view v, size_type pos = npos ) const nssv_noexcept  // (1)
    {
        return empty()
            ? npos
            : pos >= size()
            ? find_last_not_of( v, size() - 1 )
            : to_pos( std::find_if( const_reverse_iterator( cbegin() + pos + 1 ), crend(), not_in_view( v ) ) );
    }

    nssv_constexpr size_type find_last_not_of( CharT c, size_type pos = npos ) const nssv_noexcept  // (2)
    {
        return find_last_not_of( basic_string_view( &c, 1 ), pos );
    }

    nssv_constexpr size_type find_last_not_of( CharT const * s, size_type pos, size_type count ) const  // (3)
    {
        return find_last_not_of( basic_string_view( s, count ), pos );
    }

    nssv_constexpr size_type find_last_not_of( CharT const * s, size_type pos = npos ) const  // (4)
    {
        return find_last_not_of( basic_string_view( s ), pos );
    }

    // Constants:

#if nssv_CPP17_OR_GREATER
    static nssv_constexpr size_type npos = size_type(-1);
#elif nssv_CPP11_OR_GREATER
    enum : size_type { npos = size_type(-1) };
#else
    enum { npos = size_type(-1) };
#endif

private:
    struct not_in_view
    {
        const basic_string_view v;

        nssv_constexpr explicit not_in_view( basic_string_view v ) : v( v ) {}

        nssv_constexpr bool operator()( CharT c ) const
        {
            return npos == v.find_first_of( c );
        }
    };

    nssv_constexpr size_type to_pos( const_iterator it ) const
    {
        return it == cend() ? npos : size_type( it - cbegin() );
    }

    nssv_constexpr size_type to_pos( const_reverse_iterator it ) const
    {
        return it == crend() ? npos : size_type( crend() - it - 1 );
    }

    nssv_constexpr const_reference data_at( size_type pos ) const
    {
#if nssv_BETWEEN( nssv_COMPILER_GNUC_VERSION, 1, 500 )
        return data_[pos];
#else
        return assert( pos < size() ), data_[pos];
#endif
    }

private:
    const_pointer data_;
    size_type     size_;

public:
#if nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS

    template< class Allocator >
    basic_string_view( std::basic_string<CharT, Traits, Allocator> const & s ) nssv_noexcept
        : data_( s.data() )
        , size_( s.size() )
    {}

#if nssv_HAVE_EXPLICIT_CONVERSION

    template< class Allocator >
    explicit operator std::basic_string<CharT, Traits, Allocator>() const
    {
        return to_string( Allocator() );
    }

#endif // nssv_HAVE_EXPLICIT_CONVERSION

#if nssv_CPP11_OR_GREATER

    template< class Allocator = std::allocator<CharT> >
    std::basic_string<CharT, Traits, Allocator>
    to_string( Allocator const & a = Allocator() ) const
    {
        return std::basic_string<CharT, Traits, Allocator>( begin(), end(), a );
    }

#else

    std::basic_string<CharT, Traits>
    to_string() const
    {
        return std::basic_string<CharT, Traits>( begin(), end() );
    }

    template< class Allocator >
    std::basic_string<CharT, Traits, Allocator>
    to_string( Allocator const & a ) const
    {
        return std::basic_string<CharT, Traits, Allocator>( begin(), end(), a );
    }

#endif // nssv_CPP11_OR_GREATER

#endif // nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS
};

//
// Non-member functions:
//

// 24.4.3 Non-member comparison functions:
// lexicographically compare two string views (function template):

template< class CharT, class Traits >
nssv_constexpr bool operator== (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) == 0 ; }

template< class CharT, class Traits >
nssv_constexpr bool operator!= (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) != 0 ; }

template< class CharT, class Traits >
nssv_constexpr bool operator< (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) < 0 ; }

template< class CharT, class Traits >
nssv_constexpr bool operator<= (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) <= 0 ; }

template< class CharT, class Traits >
nssv_constexpr bool operator> (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) > 0 ; }

template< class CharT, class Traits >
nssv_constexpr bool operator>= (
    basic_string_view <CharT, Traits> lhs,
    basic_string_view <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) >= 0 ; }

// Let S be basic_string_view<CharT, Traits>, and sv be an instance of S.
// Implementations shall provide sufficient additional overloads marked
// constexpr and noexcept so that an object t with an implicit conversion
// to S can be compared according to Table 67.

#if ! nssv_CPP11_OR_GREATER || nssv_BETWEEN( nssv_COMPILER_MSVC_VERSION, 100, 141 )

// accomodate for older compilers:

// ==

template< class CharT, class Traits>
nssv_constexpr bool operator==(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) == 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator==(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) == 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator==(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.size() == rhs.size() && lhs.compare( rhs ) == 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator==(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return lhs.size() == rhs.size() && lhs.compare( rhs ) == 0; }

// !=

template< class CharT, class Traits>
nssv_constexpr bool operator!=(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) != 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator!=(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) != 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator!=(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.size() != rhs.size() && lhs.compare( rhs ) != 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator!=(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return lhs.size() != rhs.size() || rhs.compare( lhs ) != 0; }

// <

template< class CharT, class Traits>
nssv_constexpr bool operator<(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) < 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) > 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) < 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return rhs.compare( lhs ) > 0; }

// <=

template< class CharT, class Traits>
nssv_constexpr bool operator<=(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) <= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<=(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) >= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<=(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) <= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator<=(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return rhs.compare( lhs ) >= 0; }

// >

template< class CharT, class Traits>
nssv_constexpr bool operator>(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) > 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) < 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) > 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return rhs.compare( lhs ) < 0; }

// >=

template< class CharT, class Traits>
nssv_constexpr bool operator>=(
    basic_string_view<CharT, Traits> lhs,
    char const * rhs ) nssv_noexcept
{ return lhs.compare( rhs ) >= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>=(
    char const * lhs,
    basic_string_view<CharT, Traits> rhs ) nssv_noexcept
{ return rhs.compare( lhs ) <= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>=(
    basic_string_view<CharT, Traits> lhs,
    std::basic_string<CharT, Traits> rhs ) nssv_noexcept
{ return lhs.compare( rhs ) >= 0; }

template< class CharT, class Traits>
nssv_constexpr bool operator>=(
    std::basic_string<CharT, Traits> rhs,
    basic_string_view<CharT, Traits> lhs ) nssv_noexcept
{ return rhs.compare( lhs ) <= 0; }

#else // newer compilers:

#define nssv_BASIC_STRING_VIEW_I(T,U)  typename std::decay< basic_string_view<T,U> >::type

#if nssv_BETWEEN( nssv_COMPILER_MSVC_VERSION, 140, 150 )
# define nssv_MSVC_ORDER(x)  , int=x
#else
# define nssv_MSVC_ORDER(x)  /*, int=x*/
#endif

// ==

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator==(
         basic_string_view  <CharT, Traits> lhs,
    nssv_BASIC_STRING_VIEW_I(CharT, Traits) rhs ) nssv_noexcept
{ return lhs.compare( rhs ) == 0; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator==(
    nssv_BASIC_STRING_VIEW_I(CharT, Traits) lhs,
         basic_string_view  <CharT, Traits> rhs ) nssv_noexcept
{ return lhs.size() == rhs.size() && lhs.compare( rhs ) == 0; }

// !=

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator!= (
         basic_string_view  < CharT, Traits > lhs,
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) rhs ) nssv_noexcept
{ return lhs.size() != rhs.size() || lhs.compare( rhs ) != 0 ; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator!= (
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) lhs,
         basic_string_view  < CharT, Traits > rhs ) nssv_noexcept
{ return lhs.compare( rhs ) != 0 ; }

// <

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator< (
         basic_string_view  < CharT, Traits > lhs,
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) rhs ) nssv_noexcept
{ return lhs.compare( rhs ) < 0 ; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator< (
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) lhs,
         basic_string_view  < CharT, Traits > rhs ) nssv_noexcept
{ return lhs.compare( rhs ) < 0 ; }

// <=

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator<= (
         basic_string_view  < CharT, Traits > lhs,
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) rhs ) nssv_noexcept
{ return lhs.compare( rhs ) <= 0 ; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator<= (
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) lhs,
         basic_string_view  < CharT, Traits > rhs ) nssv_noexcept
{ return lhs.compare( rhs ) <= 0 ; }

// >

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator> (
         basic_string_view  < CharT, Traits > lhs,
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) rhs ) nssv_noexcept
{ return lhs.compare( rhs ) > 0 ; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator> (
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) lhs,
         basic_string_view  < CharT, Traits > rhs ) nssv_noexcept
{ return lhs.compare( rhs ) > 0 ; }

// >=

template< class CharT, class Traits  nssv_MSVC_ORDER(1) >
nssv_constexpr bool operator>= (
         basic_string_view  < CharT, Traits > lhs,
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) rhs ) nssv_noexcept
{ return lhs.compare( rhs ) >= 0 ; }

template< class CharT, class Traits  nssv_MSVC_ORDER(2) >
nssv_constexpr bool operator>= (
    nssv_BASIC_STRING_VIEW_I( CharT, Traits ) lhs,
         basic_string_view  < CharT, Traits > rhs ) nssv_noexcept
{ return lhs.compare( rhs ) >= 0 ; }

#undef nssv_MSVC_ORDER
#undef nssv_BASIC_STRING_VIEW_I

#endif // compiler-dependent approach to comparisons

// 24.4.4 Inserters and extractors:

namespace detail {

template< class Stream >
void write_padding( Stream & os, std::streamsize n )
{
    for ( std::streamsize i = 0; i < n; ++i )
        os.rdbuf()->sputc( os.fill() );
}

template< class Stream, class View >
Stream & write_to_stream( Stream & os, View const & sv )
{
    typename Stream::sentry sentry( os );

    if ( !os )
        return os;

    const std::streamsize length = static_cast<std::streamsize>( sv.length() );

    // Whether, and how, to pad:
    const bool      pad = ( length < os.width() );
    const bool left_pad = pad && ( os.flags() & std::ios_base::adjustfield ) == std::ios_base::right;

    if ( left_pad )
        write_padding( os, os.width() - length );

    // Write span characters:
    os.rdbuf()->sputn( sv.begin(), length );

    if ( pad && !left_pad )
        write_padding( os, os.width() - length );

    // Reset output stream width:
    os.width( 0 );

    return os;
}

} // namespace detail

template< class CharT, class Traits >
std::basic_ostream<CharT, Traits> &
operator<<(
    std::basic_ostream<CharT, Traits>& os,
    basic_string_view <CharT, Traits> sv )
{
    return detail::write_to_stream( os, sv );
}

// Several typedefs for common character types are provided:

typedef basic_string_view<char>      string_view;
typedef basic_string_view<wchar_t>   wstring_view;
#if nssv_HAVE_WCHAR16_T
typedef basic_string_view<char16_t>  u16string_view;
typedef basic_string_view<char32_t>  u32string_view;
#endif

}} // namespace nonstd::sv_lite

//
// 24.4.6 Suffix for basic_string_view literals:
//

#if nssv_HAVE_USER_DEFINED_LITERALS

namespace nonstd {
nssv_inline_ns namespace literals {
nssv_inline_ns namespace string_view_literals {

#if nssv_CONFIG_STD_SV_OPERATOR && nssv_HAVE_STD_DEFINED_LITERALS

nssv_constexpr nonstd::sv_lite::string_view operator "" sv( const char* str, size_t len ) nssv_noexcept  // (1)
{
    return nonstd::sv_lite::string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::u16string_view operator "" sv( const char16_t* str, size_t len ) nssv_noexcept  // (2)
{
    return nonstd::sv_lite::u16string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::u32string_view operator "" sv( const char32_t* str, size_t len ) nssv_noexcept  // (3)
{
    return nonstd::sv_lite::u32string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::wstring_view operator "" sv( const wchar_t* str, size_t len ) nssv_noexcept  // (4)
{
    return nonstd::sv_lite::wstring_view{ str, len };
}

#endif // nssv_CONFIG_STD_SV_OPERATOR && nssv_HAVE_STD_DEFINED_LITERALS

#if nssv_CONFIG_USR_SV_OPERATOR

nssv_constexpr nonstd::sv_lite::string_view operator "" _sv( const char* str, size_t len ) nssv_noexcept  // (1)
{
    return nonstd::sv_lite::string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::u16string_view operator "" _sv( const char16_t* str, size_t len ) nssv_noexcept  // (2)
{
    return nonstd::sv_lite::u16string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::u32string_view operator "" _sv( const char32_t* str, size_t len ) nssv_noexcept  // (3)
{
    return nonstd::sv_lite::u32string_view{ str, len };
}

nssv_constexpr nonstd::sv_lite::wstring_view operator "" _sv( const wchar_t* str, size_t len ) nssv_noexcept  // (4)
{
    return nonstd::sv_lite::wstring_view{ str, len };
}

#endif // nssv_CONFIG_USR_SV_OPERATOR

}}} // namespace nonstd::literals::string_view_literals

#endif

//
// Extensions for std::string:
//

#if nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

namespace nonstd {
namespace sv_lite {

// Exclude MSVC 14 (19.00): it yields ambiguous to_string():

#if nssv_CPP11_OR_GREATER && nssv_COMPILER_MSVC_VERSION != 140

template< class CharT, class Traits, class Allocator = std::allocator<CharT> >
std::basic_string<CharT, Traits, Allocator>
to_string( basic_string_view<CharT, Traits> v, Allocator const & a = Allocator() )
{
    return std::basic_string<CharT,Traits, Allocator>( v.begin(), v.end(), a );
}

#else

template< class CharT, class Traits >
std::basic_string<CharT, Traits>
to_string( basic_string_view<CharT, Traits> v )
{
    return std::basic_string<CharT, Traits>( v.begin(), v.end() );
}

template< class CharT, class Traits, class Allocator >
std::basic_string<CharT, Traits, Allocator>
to_string( basic_string_view<CharT, Traits> v, Allocator const & a )
{
    return std::basic_string<CharT, Traits, Allocator>( v.begin(), v.end(), a );
}

#endif // nssv_CPP11_OR_GREATER

template< class CharT, class Traits, class Allocator >
basic_string_view<CharT, Traits>
to_string_view( std::basic_string<CharT, Traits, Allocator> const & s )
{
    return basic_string_view<CharT, Traits>( s.data(), s.size() );
}

}} // namespace nonstd::sv_lite

#endif // nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

//
// make types and algorithms available in namespace nonstd:
//

namespace nonstd {

using sv_lite::basic_string_view;
using sv_lite::string_view;
using sv_lite::wstring_view;

#if nssv_HAVE_WCHAR16_T
using sv_lite::u16string_view;
#endif
#if nssv_HAVE_WCHAR32_T
using sv_lite::u32string_view;
#endif

// literal "sv"

using sv_lite::operator==;
using sv_lite::operator!=;
using sv_lite::operator<;
using sv_lite::operator<=;
using sv_lite::operator>;
using sv_lite::operator>=;

using sv_lite::operator<<;

#if nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS
using sv_lite::to_string;
using sv_lite::to_string_view;
#endif

} // namespace nonstd

// 24.4.5 Hash support (C++11):

// Note: The hash value of a string view object is equal to the hash value of
// the corresponding string object.

#if nssv_HAVE_STD_HASH

#include <functional>

namespace std {

template<>
struct hash< nonstd::string_view >
{
public:
    std::size_t operator()( nonstd::string_view v ) const nssv_noexcept
    {
        return std::hash<std::string>()( std::string( v.data(), v.size() ) );
    }
};

template<>
struct hash< nonstd::wstring_view >
{
public:
    std::size_t operator()( nonstd::wstring_view v ) const nssv_noexcept
    {
        return std::hash<std::wstring>()( std::wstring( v.data(), v.size() ) );
    }
};

template<>
struct hash< nonstd::u16string_view >
{
public:
    std::size_t operator()( nonstd::u16string_view v ) const nssv_noexcept
    {
        return std::hash<std::u16string>()( std::u16string( v.data(), v.size() ) );
    }
};

template<>
struct hash< nonstd::u32string_view >
{
public:
    std::size_t operator()( nonstd::u32string_view v ) const nssv_noexcept
    {
        return std::hash<std::u32string>()( std::u32string( v.data(), v.size() ) );
    }
};

} // namespace std

#endif // nssv_HAVE_STD_HASH

nssv_RESTORE_WARNINGS()

#endif // nssv_HAVE_STD_STRING_VIEW
#endif // NONSTD_SV_LITE_H_INCLUDED



namespace inja {

enum class ElementNotation {
  Dot,
  Pointer
};

/*!
 * \brief Class for lexer configuration.
 */
struct LexerConfig {
  std::string statement_open {"{%"};
  std::string statement_close {"%}"};
  std::string line_statement {"##"};
  std::string expression_open {"{{"};
  std::string expression_close {"}}"};
  std::string comment_open {"{#"};
  std::string comment_close {"#}"};
  std::string open_chars {"#{"};

  bool trim_blocks {false};
  bool lstrip_blocks {false};

  void update_open_chars() {
    open_chars = "";
    if (open_chars.find(line_statement[0]) == std::string::npos) {
      open_chars += line_statement[0];
    }
    if (open_chars.find(statement_open[0]) == std::string::npos) {
      open_chars += statement_open[0];
    }
    if (open_chars.find(expression_open[0]) == std::string::npos) {
      open_chars += expression_open[0];
    }
    if (open_chars.find(comment_open[0]) == std::string::npos) {
      open_chars += comment_open[0];
    }
  }
};

/*!
 * \brief Class for parser configuration.
 */
struct ParserConfig {
  ElementNotation notation {ElementNotation::Dot};
  /// Extra options for limiting included files' scope
  bool include_scope_limit = false;
  std::string include_scope = "templates";
};

}  // namespace inja

#endif  // INCLUDE_INJA_CONFIG_HPP_

// #include "function_storage.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_FUNCTION_STORAGE_HPP_
#define INCLUDE_INJA_FUNCTION_STORAGE_HPP_

#include <vector>

// #include "bytecode.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_BYTECODE_HPP_
#define INCLUDE_INJA_BYTECODE_HPP_

#include <string>
#include <utility>

#include <nlohmann/json.hpp>

// #include "string_view.hpp"



namespace inja {

using json = nlohmann::json;


struct Bytecode {
  enum class Op : uint8_t {
    Nop,
    // print StringRef (always immediate)
    PrintText,
    // print value
    PrintValue,
    // push value onto stack (always immediate)
    Push,

    // builtin functions
    // result is pushed to stack
    // args specify number of arguments
    // all functions can take their "last" argument either immediate
    // or popped off stack (e.g. if immediate, it's like the immediate was
    // just pushed to the stack)
    Not,
    And,
    Or,
    In,
    Equal,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    At,
    Different,
    DivisibleBy,
    Even,
    First,
    Float,
    Int,
    Last,
    Length,
    Lower,
    Max,
    Min,
    Odd,
    Range,
    Result,
    Round,
    Sort,
    Upper,
    Exists,
    ExistsInObject,
    IsBoolean,
    IsNumber,
    IsInteger,
    IsFloat,
    IsObject,
    IsArray,
    IsString,
    Default,

    // include another template
    // value is the template name
    Include,

    // callback function
    // str is the function name (this means it cannot be a lookup)
    // args specify number of arguments
    // as with builtin functions, "last" argument can be immediate
    Callback,

    // unconditional jump
    // args is the index of the bytecode to jump to.
    Jump,

    // conditional jump
    // value popped off stack is checked for truthyness
    // if false, args is the index of the bytecode to jump to.
    // if true, no action is taken (falls through)
    ConditionalJump,

    // start loop
    // value popped off stack is what is iterated over
    // args is index of bytecode after end loop (jumped to if iterable is
    // empty)
    // immediate value is key name (for maps)
    // str is value name
    StartLoop,

    // end a loop
    // args is index of the first bytecode in the loop body
    EndLoop,
  };

  enum Flag {
    // location of value for value-taking ops (mask)
    ValueMask = 0x03,
    // pop value off stack
    ValuePop = 0x00,
    // value is immediate rather than on stack
    ValueImmediate = 0x01,
    // lookup immediate str (dot notation)
    ValueLookupDot = 0x02,
    // lookup immediate str (json pointer notation)
    ValueLookupPointer = 0x03,
  };

  Op op {Op::Nop};
  uint32_t args: 30;
  uint32_t flags: 2;

  json value;
  std::string str;

  Bytecode(): args(0), flags(0) {}
  explicit Bytecode(Op op, unsigned int args = 0): op(op), args(args), flags(0) {}
  explicit Bytecode(Op op, nonstd::string_view str, unsigned int flags): op(op), args(0), flags(flags), str(str) {}
  explicit Bytecode(Op op, json&& value, unsigned int flags): op(op), args(0), flags(flags), value(std::move(value)) {}
};

}  // namespace inja

#endif  // INCLUDE_INJA_BYTECODE_HPP_

// #include "string_view.hpp"



namespace inja {

using json = nlohmann::json;

using Arguments = std::vector<const json*>;
using CallbackFunction = std::function<json(Arguments& args)>;

/*!
 * \brief Class for builtin functions and user-defined callbacks.
 */
class FunctionStorage {
 public:
  void add_builtin(nonstd::string_view name, unsigned int num_args, Bytecode::Op op) {
    auto& data = get_or_new(name, num_args);
    data.op = op;
  }

  void add_callback(nonstd::string_view name, unsigned int num_args, const CallbackFunction& function) {
    auto& data = get_or_new(name, num_args);
    data.function = function;
  }

  Bytecode::Op find_builtin(nonstd::string_view name, unsigned int num_args) const {
    if (auto ptr = get(name, num_args)) {
      return ptr->op;
    }
    return Bytecode::Op::Nop;
  }

  CallbackFunction find_callback(nonstd::string_view name, unsigned int num_args) const {
    if (auto ptr = get(name, num_args)) {
      return ptr->function;
    }
    return nullptr;
  }

 private:
  struct FunctionData {
    unsigned int num_args {0};
    Bytecode::Op op {Bytecode::Op::Nop}; // for builtins
    CallbackFunction function; // for callbacks
  };

  FunctionData& get_or_new(nonstd::string_view name, unsigned int num_args) {
    auto &vec = m_map[static_cast<std::string>(name)];
    for (auto &i: vec) {
      if (i.num_args == num_args) {
        return i;
      }
    }
    vec.emplace_back();
    vec.back().num_args = num_args;
    return vec.back();
  }

  const FunctionData* get(nonstd::string_view name, unsigned int num_args) const {
    auto it = m_map.find(static_cast<std::string>(name));
    if (it == m_map.end()) {
      return nullptr;
    }
    for (auto &&i: it->second) {
      if (i.num_args == num_args) return &i;
    }
    return nullptr;
  }

  std::map<std::string, std::vector<FunctionData>> m_map;
};

}

#endif  // INCLUDE_INJA_FUNCTION_STORAGE_HPP_

// #include "parser.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_PARSER_HPP_
#define INCLUDE_INJA_PARSER_HPP_

#include <limits>
#include <string>
#include <utility>
#include <vector>

// #include "bytecode.hpp"

// #include "config.hpp"

// #include "exceptions.hpp"

// #include "function_storage.hpp"

// #include "lexer.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_LEXER_HPP_
#define INCLUDE_INJA_LEXER_HPP_

#include <cctype>
#include <locale>

// #include "config.hpp"

// #include "token.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_TOKEN_HPP_
#define INCLUDE_INJA_TOKEN_HPP_

#include <string>

// #include "string_view.hpp"



namespace inja {

/*!
 * \brief Helper-class for the inja Parser.
 */
struct Token {
  enum class Kind {
    Text,
    ExpressionOpen,      // {{
    ExpressionClose,     // }}
    LineStatementOpen,   // ##
    LineStatementClose,  // \n
    StatementOpen,       // {%
    StatementClose,      // %}
    CommentOpen,         // {#
    CommentClose,        // #}
    Id,                  // this, this.foo
    Number,              // 1, 2, -1, 5.2, -5.3
    String,              // "this"
    Comma,               // ,
    Colon,               // :
    LeftParen,           // (
    RightParen,          // )
    LeftBracket,         // [
    RightBracket,        // ]
    LeftBrace,           // {
    RightBrace,          // }
    Equal,               // ==
    GreaterThan,         // >
    GreaterEqual,        // >=
    LessThan,            // <
    LessEqual,           // <=
    NotEqual,            // !=
    Unknown,
    Eof
  } kind {Kind::Unknown};

  nonstd::string_view text;

  constexpr Token() = default;
  constexpr Token(Kind kind, nonstd::string_view text): kind(kind), text(text) {}

  std::string describe() const {
    switch (kind) {
      case Kind::Text:
        return "<text>";
      case Kind::LineStatementClose:
        return "<eol>";
      case Kind::Eof:
        return "<eof>";
      default:
        return static_cast<std::string>(text);
    }
  }
};

}

#endif  // INCLUDE_INJA_TOKEN_HPP_

// #include "utils.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_UTILS_HPP_
#define INCLUDE_INJA_UTILS_HPP_

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>

// #include "exceptions.hpp"

// #include "string_view.hpp"



namespace inja {

inline std::ifstream open_file_or_throw(const std::string& path) {
  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try {
    file.open(path);
  } catch(const std::ios_base::failure& /*e*/) {
    throw FileError("failed accessing file at '" + path + "'");
  }
  return file;
}

namespace string_view {
  inline nonstd::string_view slice(nonstd::string_view view, size_t start, size_t end) {
    start = std::min(start, view.size());
    end = std::min(std::max(start, end), view.size());
    return view.substr(start, end - start);  // StringRef(Data + Start, End - Start);
  }

  inline std::pair<nonstd::string_view, nonstd::string_view> split(nonstd::string_view view, char Separator) {
    size_t idx = view.find(Separator);
    if (idx == nonstd::string_view::npos) {
      return std::make_pair(view, nonstd::string_view());
    }
    return std::make_pair(slice(view, 0, idx), slice(view, idx + 1, nonstd::string_view::npos));
  }

  inline bool starts_with(nonstd::string_view view, nonstd::string_view prefix) {
    return (view.size() >= prefix.size() && view.compare(0, prefix.size(), prefix) == 0);
  }
}  // namespace string_view

}  // namespace inja

#endif  // INCLUDE_INJA_UTILS_HPP_



namespace inja {

/*!
 * \brief Class for lexing an inja Template.
 */
class Lexer {
  enum class State {
    Text,
    ExpressionStart,
    ExpressionBody,
    LineStart,
    LineBody,
    StatementStart,
    StatementBody,
    CommentStart,
    CommentBody
  } m_state;

  const LexerConfig& m_config;
  nonstd::string_view m_in;
  size_t m_tok_start;
  size_t m_pos;

 public:
  explicit Lexer(const LexerConfig& config) : m_config(config) {}

  SourceLocation current_position() const {
    // Get line and offset position (starts at 1:1)
    auto sliced = string_view::slice(m_in, 0, m_tok_start);
    std::size_t last_newline = sliced.rfind("\n");

    if (last_newline == nonstd::string_view::npos) {
      return {1, sliced.length() + 1};
    }

    // Count newlines
    size_t count_lines = 0;
    size_t search_start = 0;
    while (search_start < sliced.size()) {
      search_start = sliced.find("\n", search_start + 1);
      count_lines += 1;
    }

    return {count_lines + 1, sliced.length() - last_newline + 1};
  }

  void start(nonstd::string_view in) {
    m_in = in;
    m_tok_start = 0;
    m_pos = 0;
    m_state = State::Text;
  }

  Token scan() {
    m_tok_start = m_pos;

  again:
    if (m_tok_start >= m_in.size()) return make_token(Token::Kind::Eof);

    switch (m_state) {
      default:
      case State::Text: {
        // fast-scan to first open character
        size_t open_start = m_in.substr(m_pos).find_first_of(m_config.open_chars);
        if (open_start == nonstd::string_view::npos) {
          // didn't find open, return remaining text as text token
          m_pos = m_in.size();
          return make_token(Token::Kind::Text);
        }
        m_pos += open_start;

        // try to match one of the opening sequences, and get the close
        nonstd::string_view open_str = m_in.substr(m_pos);
        bool must_lstrip = false;
        if (inja::string_view::starts_with(open_str, m_config.expression_open)) {
          m_state = State::ExpressionStart;
        } else if (inja::string_view::starts_with(open_str, m_config.statement_open)) {
          m_state = State::StatementStart;
          must_lstrip = m_config.lstrip_blocks;
        } else if (inja::string_view::starts_with(open_str, m_config.comment_open)) {
          m_state = State::CommentStart;
          must_lstrip = m_config.lstrip_blocks;
        } else if ((m_pos == 0 || m_in[m_pos - 1] == '\n') &&
                   inja::string_view::starts_with(open_str, m_config.line_statement)) {
          m_state = State::LineStart;
        } else {
          m_pos += 1;  // wasn't actually an opening sequence
          goto again;
        }

        nonstd::string_view text = string_view::slice(m_in, m_tok_start, m_pos);
        if (must_lstrip)
          text = clear_final_line_if_whitespace(text);

        if (text.empty()) goto again;  // don't generate empty token
        return Token(Token::Kind::Text, text);
      }
      case State::ExpressionStart: {
        m_state = State::ExpressionBody;
        m_pos += m_config.expression_open.size();
        return make_token(Token::Kind::ExpressionOpen);
      }
      case State::LineStart: {
        m_state = State::LineBody;
        m_pos += m_config.line_statement.size();
        return make_token(Token::Kind::LineStatementOpen);
      }
      case State::StatementStart: {
        m_state = State::StatementBody;
        m_pos += m_config.statement_open.size();
        return make_token(Token::Kind::StatementOpen);
      }
      case State::CommentStart: {
        m_state = State::CommentBody;
        m_pos += m_config.comment_open.size();
        return make_token(Token::Kind::CommentOpen);
      }
      case State::ExpressionBody:
        return scan_body(m_config.expression_close, Token::Kind::ExpressionClose);
      case State::LineBody:
        return scan_body("\n", Token::Kind::LineStatementClose);
      case State::StatementBody:
        return scan_body(m_config.statement_close, Token::Kind::StatementClose, m_config.trim_blocks);
      case State::CommentBody: {
        // fast-scan to comment close
        size_t end = m_in.substr(m_pos).find(m_config.comment_close);
        if (end == nonstd::string_view::npos) {
          m_pos = m_in.size();
          return make_token(Token::Kind::Eof);
        }
        // return the entire comment in the close token
        m_state = State::Text;
        m_pos += end + m_config.comment_close.size();
        Token tok = make_token(Token::Kind::CommentClose);
        if (m_config.trim_blocks)
          skip_newline();
        return tok;
      }
    }
  }

  const LexerConfig& get_config() const { return m_config; }

 private:
  Token scan_body(nonstd::string_view close, Token::Kind closeKind, bool trim = false) {
  again:
    // skip whitespace (except for \n as it might be a close)
    if (m_tok_start >= m_in.size()) return make_token(Token::Kind::Eof);
    char ch = m_in[m_tok_start];
    if (ch == ' ' || ch == '\t' || ch == '\r') {
      m_tok_start += 1;
      goto again;
    }

    // check for close
    if (inja::string_view::starts_with(m_in.substr(m_tok_start), close)) {
      m_state = State::Text;
      m_pos = m_tok_start + close.size();
      Token tok = make_token(closeKind);
      if (trim)
        skip_newline();
      return tok;
    }

    // skip \n
    if (ch == '\n') {
      m_tok_start += 1;
      goto again;
    }

    m_pos = m_tok_start + 1;
    if (std::isalpha(ch)) {
      return scan_id();
    }
    switch (ch) {
      case ',':
        return make_token(Token::Kind::Comma);
      case ':':
        return make_token(Token::Kind::Colon);
      case '(':
        return make_token(Token::Kind::LeftParen);
      case ')':
        return make_token(Token::Kind::RightParen);
      case '[':
        return make_token(Token::Kind::LeftBracket);
      case ']':
        return make_token(Token::Kind::RightBracket);
      case '{':
        return make_token(Token::Kind::LeftBrace);
      case '}':
        return make_token(Token::Kind::RightBrace);
      case '>':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          m_pos += 1;
          return make_token(Token::Kind::GreaterEqual);
        }
        return make_token(Token::Kind::GreaterThan);
      case '<':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          m_pos += 1;
          return make_token(Token::Kind::LessEqual);
        }
        return make_token(Token::Kind::LessThan);
      case '=':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          m_pos += 1;
          return make_token(Token::Kind::Equal);
        }
        return make_token(Token::Kind::Unknown);
      case '!':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          m_pos += 1;
          return make_token(Token::Kind::NotEqual);
        }
        return make_token(Token::Kind::Unknown);
      case '\"':
        return scan_string();
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-':
        return scan_number();
      case '_':
        return scan_id();
      default:
        return make_token(Token::Kind::Unknown);
    }
  }

  Token scan_id() {
    for (;;) {
      if (m_pos >= m_in.size()) {
        break;
      }
      char ch = m_in[m_pos];
      if (!std::isalnum(ch) && ch != '.' && ch != '/' && ch != '_' && ch != '-') {
        break;
      }
      m_pos += 1;
    }
    return make_token(Token::Kind::Id);
  }

  Token scan_number() {
    for (;;) {
      if (m_pos >= m_in.size()) {
        break;
      }
      char ch = m_in[m_pos];
      // be very permissive in lexer (we'll catch errors when conversion happens)
      if (!std::isdigit(ch) && ch != '.' && ch != 'e' && ch != 'E' && ch != '+' && ch != '-') {
        break;
      }
      m_pos += 1;
    }
    return make_token(Token::Kind::Number);
  }

  Token scan_string() {
    bool escape {false};
    for (;;) {
      if (m_pos >= m_in.size()) break;
      char ch = m_in[m_pos++];
      if (ch == '\\') {
        escape = true;
      } else if (!escape && ch == m_in[m_tok_start]) {
        break;
      } else {
        escape = false;
      }
    }
    return make_token(Token::Kind::String);
  }

  Token make_token(Token::Kind kind) const {
    return Token(kind, string_view::slice(m_in, m_tok_start, m_pos));
  }

  void skip_newline() {
    if (m_pos < m_in.size()) {
      char ch = m_in[m_pos];
      if (ch == '\n')
        m_pos += 1;
      else if (ch == '\r') {
        m_pos += 1;
        if (m_pos < m_in.size() && m_in[m_pos] == '\n')
          m_pos += 1;
      }
    }
  }

  static nonstd::string_view clear_final_line_if_whitespace(nonstd::string_view text) {
    nonstd::string_view result = text;
    while (!result.empty()) {
      char ch = result.back();
      if (ch == ' ' || ch == '\t')
       result.remove_suffix(1);
      else if (ch == '\n' || ch == '\r')
        break;
      else
        return text;
    }
    return result;
  }
};

}

#endif  // INCLUDE_INJA_LEXER_HPP_

// #include "template.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_TEMPLATE_HPP_
#define INCLUDE_INJA_TEMPLATE_HPP_

#include <map>
#include <string>
#include <vector>

// #include "bytecode.hpp"



namespace inja {

/*!
 * \brief The main inja Template.
 */
struct Template {
  std::vector<Bytecode> bytecodes;
  std::string content;
};

using TemplateStorage = std::map<std::string, Template>;

}  // namespace inja

#endif  // INCLUDE_INJA_TEMPLATE_HPP_

// #include "token.hpp"

// #include "utils.hpp"


#include <nlohmann/json.hpp>


namespace inja {

class ParserStatic {
  ParserStatic() {
    functions.add_builtin("at", 2, Bytecode::Op::At);
    functions.add_builtin("default", 2, Bytecode::Op::Default);
    functions.add_builtin("divisibleBy", 2, Bytecode::Op::DivisibleBy);
    functions.add_builtin("even", 1, Bytecode::Op::Even);
    functions.add_builtin("first", 1, Bytecode::Op::First);
    functions.add_builtin("float", 1, Bytecode::Op::Float);
    functions.add_builtin("int", 1, Bytecode::Op::Int);
    functions.add_builtin("last", 1, Bytecode::Op::Last);
    functions.add_builtin("length", 1, Bytecode::Op::Length);
    functions.add_builtin("lower", 1, Bytecode::Op::Lower);
    functions.add_builtin("max", 1, Bytecode::Op::Max);
    functions.add_builtin("min", 1, Bytecode::Op::Min);
    functions.add_builtin("odd", 1, Bytecode::Op::Odd);
    functions.add_builtin("range", 1, Bytecode::Op::Range);
    functions.add_builtin("round", 2, Bytecode::Op::Round);
    functions.add_builtin("sort", 1, Bytecode::Op::Sort);
    functions.add_builtin("upper", 1, Bytecode::Op::Upper);
    functions.add_builtin("exists", 1, Bytecode::Op::Exists);
    functions.add_builtin("existsIn", 2, Bytecode::Op::ExistsInObject);
    functions.add_builtin("isBoolean", 1, Bytecode::Op::IsBoolean);
    functions.add_builtin("isNumber", 1, Bytecode::Op::IsNumber);
    functions.add_builtin("isInteger", 1, Bytecode::Op::IsInteger);
    functions.add_builtin("isFloat", 1, Bytecode::Op::IsFloat);
    functions.add_builtin("isObject", 1, Bytecode::Op::IsObject);
    functions.add_builtin("isArray", 1, Bytecode::Op::IsArray);
    functions.add_builtin("isString", 1, Bytecode::Op::IsString);
  }

 public:
  ParserStatic(const ParserStatic&) = delete;
  ParserStatic& operator=(const ParserStatic&) = delete;

  static const ParserStatic& get_instance() {
    static ParserStatic inst;
    return inst;
  }

  FunctionStorage functions;
};

/*!
 * \brief Class for parsing an inja Template.
 */
class Parser {
 public:
  explicit Parser(const ParserConfig& parser_config, const LexerConfig& lexer_config, TemplateStorage& included_templates): m_config(parser_config), m_lexer(lexer_config), m_included_templates(included_templates), m_static(ParserStatic::get_instance()) { }

  bool parse_expression(Template& tmpl) {
    if (!parse_expression_and(tmpl)) return false;
    if (m_tok.kind != Token::Kind::Id || m_tok.text != static_cast<decltype(m_tok.text)>("or")) return true;
    get_next_token();
    if (!parse_expression_and(tmpl)) return false;
    append_function(tmpl, Bytecode::Op::Or, 2);
    return true;
  }

  bool parse_expression_and(Template& tmpl) {
    if (!parse_expression_not(tmpl)) return false;
    if (m_tok.kind != Token::Kind::Id || m_tok.text != static_cast<decltype(m_tok.text)>("and")) return true;
    get_next_token();
    if (!parse_expression_not(tmpl)) return false;
    append_function(tmpl, Bytecode::Op::And, 2);
    return true;
  }

  bool parse_expression_not(Template& tmpl) {
    if (m_tok.kind == Token::Kind::Id && m_tok.text == static_cast<decltype(m_tok.text)>("not")) {
      get_next_token();
      if (!parse_expression_not(tmpl)) return false;
      append_function(tmpl, Bytecode::Op::Not, 1);
      return true;
    } else {
      return parse_expression_comparison(tmpl);
    }
  }

  bool parse_expression_comparison(Template& tmpl) {
    if (!parse_expression_datum(tmpl)) return false;
    Bytecode::Op op;
    switch (m_tok.kind) {
      case Token::Kind::Id:
        if (m_tok.text == static_cast<decltype(m_tok.text)>("in"))
          op = Bytecode::Op::In;
        else
          return true;
        break;
      case Token::Kind::Equal:
        op = Bytecode::Op::Equal;
        break;
      case Token::Kind::GreaterThan:
        op = Bytecode::Op::Greater;
        break;
      case Token::Kind::LessThan:
        op = Bytecode::Op::Less;
        break;
      case Token::Kind::LessEqual:
        op = Bytecode::Op::LessEqual;
        break;
      case Token::Kind::GreaterEqual:
        op = Bytecode::Op::GreaterEqual;
        break;
      case Token::Kind::NotEqual:
        op = Bytecode::Op::Different;
        break;
      default:
        return true;
    }
    get_next_token();
    if (!parse_expression_datum(tmpl)) return false;
    append_function(tmpl, op, 2);
    return true;
  }

  bool parse_expression_datum(Template& tmpl) {
    nonstd::string_view json_first;
    size_t bracket_level = 0;
    size_t brace_level = 0;

    for (;;) {
      switch (m_tok.kind) {
        case Token::Kind::LeftParen: {
          get_next_token();
          if (!parse_expression(tmpl)) return false;
          if (m_tok.kind != Token::Kind::RightParen) {
            throw_parser_error("unmatched '('");
          }
          get_next_token();
          return true;
        }
        case Token::Kind::Id:
          get_peek_token();
          if (m_peek_tok.kind == Token::Kind::LeftParen) {
            // function call, parse arguments
            Token func_token = m_tok;
            get_next_token();  // id
            get_next_token();  // leftParen
            unsigned int num_args = 0;
            if (m_tok.kind == Token::Kind::RightParen) {
              // no args
              get_next_token();
            } else {
              for (;;) {
                if (!parse_expression(tmpl)) {
                  throw_parser_error("expected expression, got '" + m_tok.describe() + "'");
                }
                num_args += 1;
                if (m_tok.kind == Token::Kind::RightParen) {
                  get_next_token();
                  break;
                }
                if (m_tok.kind != Token::Kind::Comma) {
                  throw_parser_error("expected ')' or ',', got '" + m_tok.describe() + "'");
                }
                get_next_token();
              }
            }

            auto op = m_static.functions.find_builtin(func_token.text, num_args);

            if (op != Bytecode::Op::Nop) {
              // swap arguments for default(); see comment in RenderTo()
              if (op == Bytecode::Op::Default)
                std::swap(tmpl.bytecodes.back(), *(tmpl.bytecodes.rbegin() + 1));
              append_function(tmpl, op, num_args);
              return true;
            } else {
              append_callback(tmpl, func_token.text, num_args);
              return true;
            }
          } else if (m_tok.text == static_cast<decltype(m_tok.text)>("true") ||
              m_tok.text == static_cast<decltype(m_tok.text)>("false") ||
              m_tok.text == static_cast<decltype(m_tok.text)>("null")) {
            // true, false, null are json literals
            if (brace_level == 0 && bracket_level == 0) {
              json_first = m_tok.text;
              goto returnJson;
            }
            break;
          } else {
            // normal literal (json read)
            tmpl.bytecodes.emplace_back(
                Bytecode::Op::Push, m_tok.text,
                m_config.notation == ElementNotation::Pointer ? Bytecode::Flag::ValueLookupPointer : Bytecode::Flag::ValueLookupDot);
            get_next_token();
            return true;
          }
        // json passthrough
        case Token::Kind::Number:
        case Token::Kind::String:
          if (brace_level == 0 && bracket_level == 0) {
            json_first = m_tok.text;
            goto returnJson;
          }
          break;
        case Token::Kind::Comma:
        case Token::Kind::Colon:
          if (brace_level == 0 && bracket_level == 0) {
            throw_parser_error("unexpected token '" + m_tok.describe() + "'");
          }
          break;
        case Token::Kind::LeftBracket:
          if (brace_level == 0 && bracket_level == 0) {
            json_first = m_tok.text;
          }
          bracket_level += 1;
          break;
        case Token::Kind::LeftBrace:
          if (brace_level == 0 && bracket_level == 0) {
            json_first = m_tok.text;
          }
          brace_level += 1;
          break;
        case Token::Kind::RightBracket:
          if (bracket_level == 0) {
            throw_parser_error("unexpected ']'");
          }
          --bracket_level;
          if (brace_level == 0 && bracket_level == 0) goto returnJson;
          break;
        case Token::Kind::RightBrace:
          if (brace_level == 0) {
            throw_parser_error("unexpected '}'");
          }
          --brace_level;
          if (brace_level == 0 && bracket_level == 0) goto returnJson;
          break;
        default:
          if (brace_level != 0) {
            throw_parser_error("unmatched '{'");
          }
          if (bracket_level != 0) {
            throw_parser_error("unmatched '['");
          }
          return false;
      }

      get_next_token();
    }

  returnJson:
    // bridge across all intermediate tokens
    nonstd::string_view json_text(json_first.data(), m_tok.text.data() - json_first.data() + m_tok.text.size());
    tmpl.bytecodes.emplace_back(Bytecode::Op::Push, json::parse(json_text), Bytecode::Flag::ValueImmediate);
    get_next_token();
    return true;
  }

  bool parse_statement(Template& tmpl, nonstd::string_view path) {
    if (m_tok.kind != Token::Kind::Id) return false;

    if (m_tok.text == static_cast<decltype(m_tok.text)>("if")) {
      get_next_token();

      // evaluate expression
      if (!parse_expression(tmpl)) return false;

      // start a new if block on if stack
      m_if_stack.emplace_back(static_cast<decltype(m_if_stack)::value_type::jump_t>(tmpl.bytecodes.size()));

      // conditional jump; destination will be filled in by else or endif
      tmpl.bytecodes.emplace_back(Bytecode::Op::ConditionalJump);
    } else if (m_tok.text == static_cast<decltype(m_tok.text)>("endif")) {
      if (m_if_stack.empty()) {
        throw_parser_error("endif without matching if");
      }
      auto& if_data = m_if_stack.back();
      get_next_token();

      // previous conditional jump jumps here
      if (if_data.prev_cond_jump != std::numeric_limits<unsigned int>::max()) {
        tmpl.bytecodes[if_data.prev_cond_jump].args = tmpl.bytecodes.size();
      }

      // update all previous unconditional jumps to here
      for (size_t i: if_data.uncond_jumps) {
        tmpl.bytecodes[i].args = tmpl.bytecodes.size();
      }

      // pop if stack
      m_if_stack.pop_back();
    } else if (m_tok.text == static_cast<decltype(m_tok.text)>("else")) {
      if (m_if_stack.empty())
        throw_parser_error("else without matching if");
      auto& if_data = m_if_stack.back();
      get_next_token();

      // end previous block with unconditional jump to endif; destination will be
      // filled in by endif
      if_data.uncond_jumps.push_back(tmpl.bytecodes.size());
      tmpl.bytecodes.emplace_back(Bytecode::Op::Jump);

      // previous conditional jump jumps here
      tmpl.bytecodes[if_data.prev_cond_jump].args = tmpl.bytecodes.size();
      if_data.prev_cond_jump = std::numeric_limits<unsigned int>::max();

      // chained else if
      if (m_tok.kind == Token::Kind::Id && m_tok.text == static_cast<decltype(m_tok.text)>("if")) {
        get_next_token();

        // evaluate expression
        if (!parse_expression(tmpl)) return false;

        // update "previous jump"
        if_data.prev_cond_jump = tmpl.bytecodes.size();

        // conditional jump; destination will be filled in by else or endif
        tmpl.bytecodes.emplace_back(Bytecode::Op::ConditionalJump);
      }
    } else if (m_tok.text == static_cast<decltype(m_tok.text)>("for")) {
      get_next_token();

      // options: for a in arr; for a, b in obj
      if (m_tok.kind != Token::Kind::Id)
        throw_parser_error("expected id, got '" + m_tok.describe() + "'");
      Token value_token = m_tok;
      get_next_token();

      Token key_token;
      if (m_tok.kind == Token::Kind::Comma) {
        get_next_token();
        if (m_tok.kind != Token::Kind::Id)
          throw_parser_error("expected id, got '" + m_tok.describe() + "'");
        key_token = std::move(value_token);
        value_token = m_tok;
        get_next_token();
      }

      if (m_tok.kind != Token::Kind::Id || m_tok.text != static_cast<decltype(m_tok.text)>("in"))
        throw_parser_error(
                   "expected 'in', got '" + m_tok.describe() + "'");
      get_next_token();

      if (!parse_expression(tmpl)) return false;

      m_loop_stack.push_back(tmpl.bytecodes.size());

      tmpl.bytecodes.emplace_back(Bytecode::Op::StartLoop);
      if (!key_token.text.empty()) {
        tmpl.bytecodes.back().value = key_token.text;
      }
      tmpl.bytecodes.back().str = static_cast<std::string>(value_token.text);
    } else if (m_tok.text == static_cast<decltype(m_tok.text)>("endfor")) {
      get_next_token();
      if (m_loop_stack.empty()) {
        throw_parser_error("endfor without matching for");
      }

      // update loop with EndLoop index (for empty case)
      tmpl.bytecodes[m_loop_stack.back()].args = tmpl.bytecodes.size();

      tmpl.bytecodes.emplace_back(Bytecode::Op::EndLoop);
      tmpl.bytecodes.back().args = m_loop_stack.back() + 1;  // loop body
      m_loop_stack.pop_back();
    } else if (m_tok.text == static_cast<decltype(m_tok.text)>("include")) {
      get_next_token();

      if (m_tok.kind != Token::Kind::String) {
        throw_parser_error("expected string, got '" + m_tok.describe() + "'");
      }

      // build the relative path
      json json_name = json::parse(m_tok.text);
      std::string pathname = static_cast<std::string>(path);
      pathname += json_name.get_ref<const std::string&>();
      if (pathname.compare(0, 2, "./") == 0) {
        pathname.erase(0, 2);
      }
      // sys::path::remove_dots(pathname, true, sys::path::Style::posix);

      if(m_config.include_scope_limit)
      {
#ifdef _WIN32
        if(pathname.find(":\\") != pathname.npos || pathname.find("..") != pathname.npos)
          throw FileError("access denied when trying to include '" + pathname + "': out of scope");
#else
        if(pathname.find("/") == 0 || pathname.find("..") != pathname.npos)
          throw FileError("access denied when trying to include '" + pathname + "': out of scope");
#endif // _WIN32
        if(m_config.include_scope.size() && pathname.find(m_config.include_scope) != 0)
          throw FileError("access denied when trying to include '" + pathname + "': out of scope");
      }

      if (m_included_templates.find(pathname) == m_included_templates.end()) {
        Template include_template = parse_template(pathname);
        m_included_templates.emplace(pathname, include_template);
      }

      // generate a reference bytecode
      tmpl.bytecodes.emplace_back(Bytecode::Op::Include, json(pathname), Bytecode::Flag::ValueImmediate);

      get_next_token();
    } else {
      return false;
    }
    return true;
  }

  void append_function(Template& tmpl, Bytecode::Op op, unsigned int num_args) {
    // we can merge with back-to-back push
    if (!tmpl.bytecodes.empty()) {
      Bytecode& last = tmpl.bytecodes.back();
      if (last.op == Bytecode::Op::Push) {
        last.op = op;
        last.args = num_args;
        return;
      }
    }

    // otherwise just add it to the end
    tmpl.bytecodes.emplace_back(op, num_args);
  }

  void append_callback(Template& tmpl, nonstd::string_view name, unsigned int num_args) {
    // we can merge with back-to-back push value (not lookup)
    if (!tmpl.bytecodes.empty()) {
      Bytecode& last = tmpl.bytecodes.back();
      if (last.op == Bytecode::Op::Push &&
          (last.flags & Bytecode::Flag::ValueMask) == Bytecode::Flag::ValueImmediate) {
        last.op = Bytecode::Op::Callback;
        last.args = num_args;
        last.str = static_cast<std::string>(name);
        return;
      }
    }

    // otherwise just add it to the end
    tmpl.bytecodes.emplace_back(Bytecode::Op::Callback, num_args);
    tmpl.bytecodes.back().str = static_cast<std::string>(name);
  }

  void parse_into(Template& tmpl, nonstd::string_view path) {
    m_lexer.start(tmpl.content);

    for (;;) {
      get_next_token();
      switch (m_tok.kind) {
        case Token::Kind::Eof:
          if (!m_if_stack.empty()) throw_parser_error("unmatched if");
          if (!m_loop_stack.empty()) throw_parser_error("unmatched for");
          return;
        case Token::Kind::Text:
          tmpl.bytecodes.emplace_back(Bytecode::Op::PrintText, m_tok.text, 0u);
          break;
        case Token::Kind::StatementOpen:
          get_next_token();
          if (!parse_statement(tmpl, path)) {
            throw_parser_error("expected statement, got '" + m_tok.describe() + "'");
          }
          if (m_tok.kind != Token::Kind::StatementClose) {
            throw_parser_error("expected statement close, got '" + m_tok.describe() + "'");
          }
          break;
        case Token::Kind::LineStatementOpen:
          get_next_token();
          parse_statement(tmpl, path);
          if (m_tok.kind != Token::Kind::LineStatementClose &&
              m_tok.kind != Token::Kind::Eof) {
            throw_parser_error("expected line statement close, got '" + m_tok.describe() + "'");
          }
          break;
        case Token::Kind::ExpressionOpen:
          get_next_token();
          if (!parse_expression(tmpl)) {
            throw_parser_error("expected expression, got '" + m_tok.describe() + "'");
          }
          append_function(tmpl, Bytecode::Op::PrintValue, 1);
          if (m_tok.kind != Token::Kind::ExpressionClose) {
            throw_parser_error("expected expression close, got '" + m_tok.describe() + "'");
          }
          break;
        case Token::Kind::CommentOpen:
          get_next_token();
          if (m_tok.kind != Token::Kind::CommentClose) {
            throw_parser_error("expected comment close, got '" + m_tok.describe() + "'");
          }
          break;
        default:
          throw_parser_error("unexpected token '" + m_tok.describe() + "'");
          break;
      }
    }
  }

  Template parse(nonstd::string_view input, nonstd::string_view path) {
    Template result;
    result.content = static_cast<std::string>(input);
    parse_into(result, path);
    return result;
  }

  Template parse(nonstd::string_view input) {
    return parse(input, "./");
  }

  Template parse_template(nonstd::string_view filename) {
    Template result;
    result.content = load_file(filename);

    nonstd::string_view path = filename.substr(0, filename.find_last_of("/\\") + 1);
      // StringRef path = sys::path::parent_path(filename);
    Parser(m_config, m_lexer.get_config(), m_included_templates).parse_into(result, path);
    return result;
  }

  std::string load_file(nonstd::string_view filename) {
    std::ifstream file = open_file_or_throw(static_cast<std::string>(filename));
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return text;
  }

 private:
  const ParserConfig& m_config;
  Lexer m_lexer;
  Token m_tok;
  Token m_peek_tok;
  bool m_have_peek_tok {false};
  TemplateStorage& m_included_templates;
  const ParserStatic& m_static;

  struct IfData {
    using jump_t = size_t;
    jump_t prev_cond_jump;
    std::vector<jump_t> uncond_jumps;

    explicit IfData(jump_t condJump)
      : prev_cond_jump(condJump)
    {
    }
  };

  std::vector<IfData> m_if_stack;
  std::vector<size_t> m_loop_stack;

  void throw_parser_error(const std::string& message) {
    throw ParserError(message, m_lexer.current_position());
  }

  void get_next_token() {
    if (m_have_peek_tok) {
      m_tok = m_peek_tok;
      m_have_peek_tok = false;
    } else {
      m_tok = m_lexer.scan();
    }
  }

  void get_peek_token() {
    if (!m_have_peek_tok) {
      m_peek_tok = m_lexer.scan();
      m_have_peek_tok = true;
    }
  }
};

}  // namespace inja

#endif  // INCLUDE_INJA_PARSER_HPP_

// #include "polyfill.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_POLYFILL_HPP_
#define INCLUDE_INJA_POLYFILL_HPP_


#if __cplusplus < 201402L

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>


namespace stdinja {

template<class T> struct _Unique_if {
  typedef std::unique_ptr<T> _Single_object;
};

template<class T> struct _Unique_if<T[]> {
  typedef std::unique_ptr<T[]> _Unknown_bound;
};

template<class T, size_t N> struct _Unique_if<T[N]> {
  typedef void _Known_bound;
};

template<class T, class... Args>
typename _Unique_if<T>::_Single_object
make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class T>
typename _Unique_if<T>::_Unknown_bound
make_unique(size_t n) {
  typedef typename std::remove_extent<T>::type U;
  return std::unique_ptr<T>(new U[n]());
}

template<class T, class... Args>
typename _Unique_if<T>::_Known_bound
make_unique(Args&&...) = delete;

}  // namespace stdinja

#else

namespace stdinja = std;

#endif  // memory */

#endif  // INCLUDE_INJA_POLYFILL_HPP_

// #include "renderer.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_RENDERER_HPP_
#define INCLUDE_INJA_RENDERER_HPP_

#include <algorithm>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

// #include "bytecode.hpp"

// #include "exceptions.hpp"

// #include "template.hpp"

// #include "utils.hpp"



namespace inja {

inline nonstd::string_view convert_dot_to_json_pointer(nonstd::string_view dot, std::string& out) {
  out.clear();
  do {
    nonstd::string_view part;
    std::tie(part, dot) = string_view::split(dot, '.');
    out.push_back('/');
    out.append(part.begin(), part.end());
  } while (!dot.empty());
  return nonstd::string_view(out.data(), out.size());
}

/*!
 * \brief Class for rendering a Template with data.
 */
class Renderer {
  std::vector<const json*>& get_args(const Bytecode& bc) {
    m_tmp_args.clear();

    bool has_imm = ((bc.flags & Bytecode::Flag::ValueMask) != Bytecode::Flag::ValuePop);

    // get args from stack
    unsigned int pop_args = bc.args;
    if (has_imm) {
      pop_args -= 1;
    }

    for (auto i = std::prev(m_stack.end(), pop_args); i != m_stack.end(); i++) {
      m_tmp_args.push_back(&(*i));
    }

    // get immediate arg
    if (has_imm) {
      m_tmp_args.push_back(get_imm(bc));
    }

    return m_tmp_args;
  }

  void pop_args(const Bytecode& bc) {
    unsigned int popArgs = bc.args;
    if ((bc.flags & Bytecode::Flag::ValueMask) != Bytecode::Flag::ValuePop) {
      popArgs -= 1;
    }
    for (unsigned int i = 0; i < popArgs; ++i) {
      m_stack.pop_back();
    }
  }

  const json* get_imm(const Bytecode& bc) {
    std::string ptr_buffer;
    nonstd::string_view ptr;
    switch (bc.flags & Bytecode::Flag::ValueMask) {
      case Bytecode::Flag::ValuePop:
        return nullptr;
      case Bytecode::Flag::ValueImmediate:
        return &bc.value;
      case Bytecode::Flag::ValueLookupDot:
        ptr = convert_dot_to_json_pointer(bc.str, ptr_buffer);
        break;
      case Bytecode::Flag::ValueLookupPointer:
        ptr_buffer += '/';
        ptr_buffer += bc.str;
        ptr = ptr_buffer;
        break;
    }
    try {
      return &m_data->at(json::json_pointer(ptr.data()));
    } catch (std::exception&) {
      // try to evaluate as a no-argument callback
      if (auto callback = m_callbacks.find_callback(bc.str, 0)) {
        std::vector<const json*> arguments {};
        m_tmp_val = callback(arguments);
        return &m_tmp_val;
      }
      throw RenderError("variable '" + static_cast<std::string>(bc.str) + "' not found");
      return nullptr;
    }
  }

  bool truthy(const json& var) const {
    if (var.empty()) {
      return false;
    } else if (var.is_number()) {
      return (var != 0);
    } else if (var.is_string()) {
      return !var.empty();
    }

    try {
      return var.get<bool>();
    } catch (json::type_error& e) {
      throw JsonError(e.what());
    }
  }

  void update_loop_data()  {
    LoopLevel& level = m_loop_stack.back();

    if (level.loop_type == LoopLevel::Type::Array) {
      level.data[static_cast<std::string>(level.value_name)] = level.values.at(level.index);  // *level.it;
      auto& loopData = level.data["loop"];
      loopData["index"] = level.index;
      loopData["index1"] = level.index + 1;
      loopData["is_first"] = (level.index == 0);
      loopData["is_last"] = (level.index == level.size - 1);
    } else {
      level.data[static_cast<std::string>(level.key_name)] = level.map_it->first;
      level.data[static_cast<std::string>(level.value_name)] = *level.map_it->second;
    }
  }

  const TemplateStorage& m_included_templates;
  const FunctionStorage& m_callbacks;

  std::vector<json> m_stack;


  struct LoopLevel {
    enum class Type { Map, Array };

    Type loop_type;
    nonstd::string_view key_name;    // variable name for keys
    nonstd::string_view value_name;  // variable name for values
    json data;                       // data with loop info added

    json values;                     // values to iterate over

    // loop over list
    size_t index;                    // current list index
    size_t size;                     // length of list

    // loop over map
    using KeyValue = std::pair<nonstd::string_view, json*>;
    using MapValues = std::vector<KeyValue>;
    MapValues map_values;            // values to iterate over
    MapValues::iterator map_it;      // iterator over values
  };

  std::vector<LoopLevel> m_loop_stack;
  const json* m_data;

  std::vector<const json*> m_tmp_args;
  json m_tmp_val;


 public:
  Renderer(const TemplateStorage& included_templates, const FunctionStorage& callbacks): m_included_templates(included_templates), m_callbacks(callbacks) {
    m_stack.reserve(16);
    m_tmp_args.reserve(4);
    m_loop_stack.reserve(16);
  }

  void render_to(std::ostream& os, const Template& tmpl, const json& data) {
    m_data = &data;

    for (size_t i = 0; i < tmpl.bytecodes.size(); ++i) {
      const auto& bc = tmpl.bytecodes[i];

      switch (bc.op) {
        case Bytecode::Op::Nop: {
          break;
        }
        case Bytecode::Op::PrintText: {
          os << bc.str;
          break;
        }
        case Bytecode::Op::PrintValue: {
          const json& val = *get_args(bc)[0];
          if (val.is_string()) {
            os << val.get_ref<const std::string&>();
          } else {
            os << val.dump();
          }
          pop_args(bc);
          break;
        }
        case Bytecode::Op::Push: {
          try{m_stack.emplace_back(*get_imm(bc));}
          catch(std::exception &e){m_stack.emplace_back(json());}
          //m_stack.emplace_back(*get_imm(bc));
          break;
        }
        case Bytecode::Op::Upper: {
          auto result = get_args(bc)[0]->get<std::string>();
          std::transform(result.begin(), result.end(), result.begin(), ::toupper);
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Lower: {
          auto result = get_args(bc)[0]->get<std::string>();
          std::transform(result.begin(), result.end(), result.begin(), ::tolower);
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Range: {
          int number = get_args(bc)[0]->get<int>();
          std::vector<int> result(number);
          std::iota(std::begin(result), std::end(result), 0);
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Length: {
          const json& val = *get_args(bc)[0];

          size_t result;
          if (val.is_string()) {
            result = val.get_ref<const std::string&>().length();
          } else {
            result = val.size();
          }

          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Sort: {
          auto result = get_args(bc)[0]->get<std::vector<json>>();
          std::sort(result.begin(), result.end());
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::At: {
          auto args = get_args(bc);
          auto result = args[0]->at(args[1]->get<int>());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::First: {
          auto result = get_args(bc)[0]->front();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Last: {
          auto result = get_args(bc)[0]->back();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Round: {
          auto args = get_args(bc);
          double number = args[0]->get<double>();
          int precision = args[1]->get<int>();
          pop_args(bc);
          m_stack.emplace_back(std::round(number * std::pow(10.0, precision)) / std::pow(10.0, precision));
          break;
        }
        case Bytecode::Op::DivisibleBy: {
          auto args = get_args(bc);
          int number = args[0]->get<int>();
          int divisor = args[1]->get<int>();
          pop_args(bc);
          m_stack.emplace_back((divisor != 0) && (number % divisor == 0));
          break;
        }
        case Bytecode::Op::Odd: {
          int number = get_args(bc)[0]->get<int>();
          pop_args(bc);
          m_stack.emplace_back(number % 2 != 0);
          break;
        }
        case Bytecode::Op::Even: {
          int number = get_args(bc)[0]->get<int>();
          pop_args(bc);
          m_stack.emplace_back(number % 2 == 0);
          break;
        }
        case Bytecode::Op::Max: {
          auto args = get_args(bc);
          auto result = *std::max_element(args[0]->begin(), args[0]->end());
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Min: {
          auto args = get_args(bc);
          auto result = *std::min_element(args[0]->begin(), args[0]->end());
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Not: {
          bool result = !truthy(*get_args(bc)[0]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::And: {
          auto args = get_args(bc);
          bool result = truthy(*args[0]) && truthy(*args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Or: {
          auto args = get_args(bc);
          bool result = truthy(*args[0]) || truthy(*args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::In: {
          auto args = get_args(bc);
          bool result = std::find(args[1]->begin(), args[1]->end(), *args[0]) !=
                        args[1]->end();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Equal: {
          auto args = get_args(bc);
          bool result = (*args[0] == *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Greater: {
          auto args = get_args(bc);
          bool result = (*args[0] > *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Less: {
          auto args = get_args(bc);
          bool result = (*args[0] < *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::GreaterEqual: {
          auto args = get_args(bc);
          bool result = (*args[0] >= *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::LessEqual: {
          auto args = get_args(bc);
          bool result = (*args[0] <= *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Different: {
          auto args = get_args(bc);
          bool result = (*args[0] != *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Float: {
          double result =
              std::stod(get_args(bc)[0]->get_ref<const std::string&>());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Int: {
          int result = std::stoi(get_args(bc)[0]->get_ref<const std::string&>());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Exists: {
          auto&& name = get_args(bc)[0]->get_ref<const std::string&>();
          //bool result = (data.find(name) != data.end());
          bool result = false;
          try
          {
            const std::string pointer = "/" + [](std::string str){std::string new_value = "/", old_value = ".";for(std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()){if((pos = str.find(old_value, pos)) != std::string::npos)str.replace(pos, old_value.length(), new_value);else break;}return str;}(name);
            data.at(json::json_pointer(pointer));
            result = true;
          }
          catch (json::out_of_range &e)
          {
            result = false;
          }
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::ExistsInObject: {
          auto args = get_args(bc);
          auto&& name = args[1]->get_ref<const std::string&>();
          bool result = (args[0]->find(name) != args[0]->end());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsBoolean: {
          bool result = get_args(bc)[0]->is_boolean();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsNumber: {
          bool result = get_args(bc)[0]->is_number();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsInteger: {
          bool result = get_args(bc)[0]->is_number_integer();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsFloat: {
          bool result = get_args(bc)[0]->is_number_float();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsObject: {
          bool result = get_args(bc)[0]->is_object();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsArray: {
          bool result = get_args(bc)[0]->is_array();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsString: {
          bool result = get_args(bc)[0]->is_string();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Default: {
          // default needs to be a bit "magic"; we can't evaluate the first
          // argument during the push operation, so we swap the arguments during
          // the parse phase so the second argument is pushed on the stack and
          // the first argument is in the immediate
          try {
            const json* imm = get_imm(bc);
            // if no exception was raised, replace the stack value with it
            m_stack.back() = *imm;
          } catch (std::exception&) {
            // couldn't read immediate, just leave the stack as is
          }
          break;
        }
        case Bytecode::Op::Include:
          Renderer(m_included_templates, m_callbacks).render_to(os, m_included_templates.find(get_imm(bc)->get_ref<const std::string&>())->second, *m_data);
          break;
        case Bytecode::Op::Callback: {
          auto callback = m_callbacks.find_callback(bc.str, bc.args);
          if (!callback) {
            throw RenderError("function '" + static_cast<std::string>(bc.str) + "' (" + std::to_string(static_cast<unsigned int>(bc.args)) + ") not found");
          }
          json result = callback(get_args(bc));
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Jump: {
          i = bc.args - 1;  // -1 due to ++i in loop
          break;
        }
        case Bytecode::Op::ConditionalJump: {
          if (!truthy(m_stack.back())) {
            i = bc.args - 1;  // -1 due to ++i in loop
          }
          m_stack.pop_back();
          break;
        }
        case Bytecode::Op::StartLoop: {
          // jump past loop body if empty
          if (m_stack.back().empty()) {
            m_stack.pop_back();
            i = bc.args;  // ++i in loop will take it past EndLoop
            break;
          }

          m_loop_stack.emplace_back();
          LoopLevel& level = m_loop_stack.back();
          level.value_name = bc.str;
          level.values = std::move(m_stack.back());
          level.data = (*m_data);
          m_stack.pop_back();

          if (bc.value.is_string()) {
            // map iterator
            if (!level.values.is_object()) {
              m_loop_stack.pop_back();
              throw RenderError("for key, value requires object");
            }
            level.loop_type = LoopLevel::Type::Map;
            level.key_name = bc.value.get_ref<const std::string&>();

            // sort by key
            for (auto it = level.values.begin(), end = level.values.end(); it != end; ++it) {
              level.map_values.emplace_back(it.key(), &it.value());
            }
            auto sort_lambda = [](const LoopLevel::KeyValue& a, const LoopLevel::KeyValue& b) { return a.first < b.first; };
            std::sort(level.map_values.begin(), level.map_values.end(), sort_lambda);
            level.map_it = level.map_values.begin();
          } else {
            if (!level.values.is_array()) {
              m_loop_stack.pop_back();
              throw RenderError("type must be array");
            }

            // list iterator
            level.loop_type = LoopLevel::Type::Array;
            level.index = 0;
            level.size = level.values.size();
          }

          // provide parent access in nested loop
          auto parent_loop_it = level.data.find("loop");
          if (parent_loop_it != level.data.end()) {
            json loop_copy = *parent_loop_it;
            (*parent_loop_it)["parent"] = std::move(loop_copy);
          }

          // set "current" data to loop data
          m_data = &level.data;
          update_loop_data();
          break;
        }
        case Bytecode::Op::EndLoop: {
          if (m_loop_stack.empty()) {
            throw RenderError("unexpected state in renderer");
          }
          LoopLevel& level = m_loop_stack.back();

          bool done;
          if (level.loop_type == LoopLevel::Type::Array) {
            level.index += 1;
            done = (level.index == level.values.size());
          } else {
            level.map_it += 1;
            done = (level.map_it == level.map_values.end());
          }

          if (done) {
            m_loop_stack.pop_back();
            // set "current" data to outer loop data or main data as appropriate
            if (!m_loop_stack.empty()) {
              m_data = &m_loop_stack.back().data;
            } else {
              m_data = &data;
            }
            break;
          }

          update_loop_data();

          // jump back to start of loop
          i = bc.args - 1;  // -1 due to ++i in loop
          break;
        }
        default: {
          throw RenderError("unknown op in renderer: " + std::to_string(static_cast<unsigned int>(bc.op)));
        }
      }
    }
  }
};

}  // namespace inja

#endif  // INCLUDE_INJA_RENDERER_HPP_

// #include "string_view.hpp"

// #include "template.hpp"

// #include "utils.hpp"



namespace inja {

using json = nlohmann::json;

/*!
 * \brief Class for changing the configuration.
 */
class Environment {
 public:
  Environment(): Environment("") { }

  explicit Environment(const std::string& global_path): m_input_path(global_path), m_output_path(global_path) {}

  Environment(const std::string& input_path, const std::string& output_path): m_input_path(input_path), m_output_path(output_path) {}

  /// Sets the opener and closer for template statements
  void set_statement(const std::string& open, const std::string& close) {
    m_lexer_config.statement_open = open;
    m_lexer_config.statement_close = close;
    m_lexer_config.update_open_chars();
  }

  /// Sets the opener for template line statements
  void set_line_statement(const std::string& open) {
    m_lexer_config.line_statement = open;
    m_lexer_config.update_open_chars();
  }

  /// Sets the opener and closer for template expressions
  void set_expression(const std::string& open, const std::string& close) {
    m_lexer_config.expression_open = open;
    m_lexer_config.expression_close = close;
    m_lexer_config.update_open_chars();
  }

  /// Sets the opener and closer for template comments
  void set_comment(const std::string& open, const std::string& close) {
    m_lexer_config.comment_open = open;
    m_lexer_config.comment_close = close;
    m_lexer_config.update_open_chars();
  }

  /// Sets whether to remove the first newline after a block
  void set_trim_blocks(bool trim_blocks) {
    m_lexer_config.trim_blocks = trim_blocks;
  }

  /// Sets whether to strip the spaces and tabs from the start of a line to a block
  void set_lstrip_blocks(bool lstrip_blocks) {
    m_lexer_config.lstrip_blocks = lstrip_blocks;
  }

  /// Sets the element notation syntax
  void set_element_notation(ElementNotation notation) {
    m_parser_config.notation = notation;
  }


  Template parse(nonstd::string_view input) {
    Parser parser(m_parser_config, m_lexer_config, m_included_templates);
    return parser.parse(input);
  }

  Template parse_template(const std::string& filename) {
    Parser parser(m_parser_config, m_lexer_config, m_included_templates);
    return parser.parse_template(m_input_path + static_cast<std::string>(filename));
  }

  std::string render(nonstd::string_view input, const json& data) {
    return render(parse(input), data);
  }

  std::string render(const Template& tmpl, const json& data) {
    std::stringstream os;
    render_to(os, tmpl, data);
    return os.str();
  }

  std::string render_file(const std::string& filename, const json& data) {
    return render(parse_template(filename), data);
  }

  std::string render_file_with_json_file(const std::string& filename, const std::string& filename_data) {
    const json data = load_json(filename_data);
    return render_file(filename, data);
  }

  void write(const std::string& filename, const json& data, const std::string& filename_out) {
    std::ofstream file(m_output_path + filename_out);
    file << render_file(filename, data);
    file.close();
  }

  void write(const Template& temp, const json& data, const std::string& filename_out) {
    std::ofstream file(m_output_path + filename_out);
    file << render(temp, data);
    file.close();
  }

  void write_with_json_file(const std::string& filename, const std::string& filename_data, const std::string& filename_out) {
    const json data = load_json(filename_data);
    write(filename, data, filename_out);
  }

  void write_with_json_file(const Template& temp, const std::string& filename_data, const std::string& filename_out) {
    const json data = load_json(filename_data);
    write(temp, data, filename_out);
  }

  std::ostream& render_to(std::ostream& os, const Template& tmpl, const json& data) {
    Renderer(m_included_templates, m_callbacks).render_to(os, tmpl, data);
    return os;
  }

  std::string load_file(const std::string& filename) {
    Parser parser(m_parser_config, m_lexer_config, m_included_templates);
    return parser.load_file(m_input_path + filename);
  }

  json load_json(const std::string& filename) {
    std::ifstream file = open_file_or_throw(m_input_path + filename);
    json j;
    file >> j;
    return j;
  }

  void add_callback(const std::string& name, unsigned int numArgs, const CallbackFunction& callback) {
    m_callbacks.add_callback(name, numArgs, callback);
  }

  /** Includes a template with a given name into the environment.
   * Then, a template can be rendered in another template using the
   * include "<name>" syntax.
   */
  void include_template(const std::string& name, const Template& tmpl) {
    m_included_templates[name] = tmpl;
  }

 private:
  std::string m_input_path;
  std::string m_output_path;

  LexerConfig m_lexer_config;
  ParserConfig m_parser_config;

  FunctionStorage m_callbacks;
  TemplateStorage m_included_templates;
};

/*!
@brief render with default settings to a string
*/
inline std::string render(nonstd::string_view input, const json& data) {
  return Environment().render(input, data);
}

/*!
@brief render with default settings to the given output stream
*/
inline void render_to(std::ostream& os, nonstd::string_view input, const json& data) {
  Environment env;
  env.render_to(os, env.parse(input), data);
}

}

#endif  // INCLUDE_INJA_ENVIRONMENT_HPP_

// #include "string_view.hpp"

// #include "template.hpp"

// #include "parser.hpp"

// #include "renderer.hpp"



#endif  // INCLUDE_INJA_INJA_HPP_
