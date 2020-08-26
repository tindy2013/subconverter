// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_INJA_HPP_
#define INCLUDE_INJA_INJA_HPP_

#include <nlohmann/json.hpp>

// #include "environment.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_ENVIRONMENT_HPP_
#define INCLUDE_INJA_ENVIRONMENT_HPP_

#include <fstream>
#include <iostream>
#include <memory>
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

#define string_view_lite_MAJOR 1
#define string_view_lite_MINOR 4
#define string_view_lite_PATCH 0

#define string_view_lite_VERSION                                                                                       \
  nssv_STRINGIFY(string_view_lite_MAJOR) "." nssv_STRINGIFY(string_view_lite_MINOR) "." nssv_STRINGIFY(                \
      string_view_lite_PATCH)

#define nssv_STRINGIFY(x) nssv_STRINGIFY_(x)
#define nssv_STRINGIFY_(x) #x

// string-view lite configuration:

#define nssv_STRING_VIEW_DEFAULT 0
#define nssv_STRING_VIEW_NONSTD 1
#define nssv_STRING_VIEW_STD 2

#if !defined(nssv_CONFIG_SELECT_STRING_VIEW)
#define nssv_CONFIG_SELECT_STRING_VIEW (nssv_HAVE_STD_STRING_VIEW ? nssv_STRING_VIEW_STD : nssv_STRING_VIEW_NONSTD)
#endif

#if defined(nssv_CONFIG_SELECT_STD_STRING_VIEW) || defined(nssv_CONFIG_SELECT_NONSTD_STRING_VIEW)
#error nssv_CONFIG_SELECT_STD_STRING_VIEW and nssv_CONFIG_SELECT_NONSTD_STRING_VIEW are deprecated and removed, please use nssv_CONFIG_SELECT_STRING_VIEW=nssv_STRING_VIEW_...
#endif

#ifndef nssv_CONFIG_STD_SV_OPERATOR
#define nssv_CONFIG_STD_SV_OPERATOR 0
#endif

#ifndef nssv_CONFIG_USR_SV_OPERATOR
#define nssv_CONFIG_USR_SV_OPERATOR 1
#endif

#ifdef nssv_CONFIG_CONVERSION_STD_STRING
#define nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS nssv_CONFIG_CONVERSION_STD_STRING
#define nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS nssv_CONFIG_CONVERSION_STD_STRING
#endif

#ifndef nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS
#define nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS 1
#endif

#ifndef nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS
#define nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS 1
#endif

// Control presence of exception handling (try and auto discover):

#ifndef nssv_CONFIG_NO_EXCEPTIONS
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#define nssv_CONFIG_NO_EXCEPTIONS 0
#else
#define nssv_CONFIG_NO_EXCEPTIONS 1
#endif
#endif

// C++ language version detection (C++20 is speculative):
// Note: VC14.0/1900 (VS2015) lacks too much from C++14.

#ifndef nssv_CPLUSPLUS
#if defined(_MSVC_LANG) && !defined(__clang__)
#define nssv_CPLUSPLUS (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
#else
#define nssv_CPLUSPLUS __cplusplus
#endif
#endif

#define nssv_CPP98_OR_GREATER (nssv_CPLUSPLUS >= 199711L)
#define nssv_CPP11_OR_GREATER (nssv_CPLUSPLUS >= 201103L)
#define nssv_CPP11_OR_GREATER_ (nssv_CPLUSPLUS >= 201103L)
#define nssv_CPP14_OR_GREATER (nssv_CPLUSPLUS >= 201402L)
#define nssv_CPP17_OR_GREATER (nssv_CPLUSPLUS >= 201703L)
#define nssv_CPP20_OR_GREATER (nssv_CPLUSPLUS >= 202000L)

// use C++17 std::string_view if available and requested:

#if nssv_CPP17_OR_GREATER && defined(__has_include)
#if __has_include(<string_view> )
#define nssv_HAVE_STD_STRING_VIEW 1
#else
#define nssv_HAVE_STD_STRING_VIEW 0
#endif
#else
#define nssv_HAVE_STD_STRING_VIEW 0
#endif

#define nssv_USES_STD_STRING_VIEW                                                                                      \
  ((nssv_CONFIG_SELECT_STRING_VIEW == nssv_STRING_VIEW_STD) ||                                                         \
   ((nssv_CONFIG_SELECT_STRING_VIEW == nssv_STRING_VIEW_DEFAULT) && nssv_HAVE_STD_STRING_VIEW))

#define nssv_HAVE_STARTS_WITH (nssv_CPP20_OR_GREATER || !nssv_USES_STD_STRING_VIEW)
#define nssv_HAVE_ENDS_WITH nssv_HAVE_STARTS_WITH

//
// Use C++17 std::string_view:
//

#if nssv_USES_STD_STRING_VIEW

#include <string_view>

// Extensions for std::string:

#if nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

namespace nonstd {

template <class CharT, class Traits, class Allocator = std::allocator<CharT>>
std::basic_string<CharT, Traits, Allocator> to_string(std::basic_string_view<CharT, Traits> v,
                                                      Allocator const &a = Allocator()) {
  return std::basic_string<CharT, Traits, Allocator>(v.begin(), v.end(), a);
}

template <class CharT, class Traits, class Allocator>
std::basic_string_view<CharT, Traits> to_string_view(std::basic_string<CharT, Traits, Allocator> const &s) {
  return std::basic_string_view<CharT, Traits>(s.data(), s.size());
}

// Literal operators sv and _sv:

#if nssv_CONFIG_STD_SV_OPERATOR

using namespace std::literals::string_view_literals;

#endif

#if nssv_CONFIG_USR_SV_OPERATOR

inline namespace literals {
inline namespace string_view_literals {

constexpr std::string_view operator"" _sv(const char *str, size_t len) noexcept // (1)
{
  return std::string_view {str, len};
}

constexpr std::u16string_view operator"" _sv(const char16_t *str, size_t len) noexcept // (2)
{
  return std::u16string_view {str, len};
}

constexpr std::u32string_view operator"" _sv(const char32_t *str, size_t len) noexcept // (3)
{
  return std::u32string_view {str, len};
}

constexpr std::wstring_view operator"" _sv(const wchar_t *str, size_t len) noexcept // (4)
{
  return std::wstring_view {str, len};
}

} // namespace string_view_literals
} // namespace literals

#endif // nssv_CONFIG_USR_SV_OPERATOR

} // namespace nonstd

#endif // nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

namespace nonstd {

using std::basic_string_view;
using std::string_view;
using std::u16string_view;
using std::u32string_view;
using std::wstring_view;

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

#if defined(_MSC_VER) && !defined(__clang__)
#define nssv_COMPILER_MSVC_VER (_MSC_VER)
#define nssv_COMPILER_MSVC_VERSION (_MSC_VER / 10 - 10 * (5 + (_MSC_VER < 1900)))
#else
#define nssv_COMPILER_MSVC_VER 0
#define nssv_COMPILER_MSVC_VERSION 0
#endif

#define nssv_COMPILER_VERSION(major, minor, patch) (10 * (10 * (major) + (minor)) + (patch))

#if defined(__clang__)
#define nssv_COMPILER_CLANG_VERSION nssv_COMPILER_VERSION(__clang_major__, __clang_minor__, __clang_patchlevel__)
#else
#define nssv_COMPILER_CLANG_VERSION 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define nssv_COMPILER_GNUC_VERSION nssv_COMPILER_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
#define nssv_COMPILER_GNUC_VERSION 0
#endif

// half-open range [lo..hi):
#define nssv_BETWEEN(v, lo, hi) ((lo) <= (v) && (v) < (hi))

// Presence of language and library features:

#ifdef _HAS_CPP0X
#define nssv_HAS_CPP0X _HAS_CPP0X
#else
#define nssv_HAS_CPP0X 0
#endif

// Unless defined otherwise below, consider VC14 as C++11 for variant-lite:

#if nssv_COMPILER_MSVC_VER >= 1900
#undef nssv_CPP11_OR_GREATER
#define nssv_CPP11_OR_GREATER 1
#endif

#define nssv_CPP11_90 (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1500)
#define nssv_CPP11_100 (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1600)
#define nssv_CPP11_110 (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1700)
#define nssv_CPP11_120 (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1800)
#define nssv_CPP11_140 (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1900)
#define nssv_CPP11_141 (nssv_CPP11_OR_GREATER_ || nssv_COMPILER_MSVC_VER >= 1910)

#define nssv_CPP14_000 (nssv_CPP14_OR_GREATER)
#define nssv_CPP17_000 (nssv_CPP17_OR_GREATER)

// Presence of C++11 language features:

#define nssv_HAVE_CONSTEXPR_11 nssv_CPP11_140
#define nssv_HAVE_EXPLICIT_CONVERSION nssv_CPP11_140
#define nssv_HAVE_INLINE_NAMESPACE nssv_CPP11_140
#define nssv_HAVE_NOEXCEPT nssv_CPP11_140
#define nssv_HAVE_NULLPTR nssv_CPP11_100
#define nssv_HAVE_REF_QUALIFIER nssv_CPP11_140
#define nssv_HAVE_UNICODE_LITERALS nssv_CPP11_140
#define nssv_HAVE_USER_DEFINED_LITERALS nssv_CPP11_140
#define nssv_HAVE_WCHAR16_T nssv_CPP11_100
#define nssv_HAVE_WCHAR32_T nssv_CPP11_100

#if !((nssv_CPP11_OR_GREATER && nssv_COMPILER_CLANG_VERSION) || nssv_BETWEEN(nssv_COMPILER_CLANG_VERSION, 300, 400))
#define nssv_HAVE_STD_DEFINED_LITERALS nssv_CPP11_140
#else
#define nssv_HAVE_STD_DEFINED_LITERALS 0
#endif

// Presence of C++14 language features:

#define nssv_HAVE_CONSTEXPR_14 nssv_CPP14_000

// Presence of C++17 language features:

#define nssv_HAVE_NODISCARD nssv_CPP17_000

// Presence of C++ library features:

#define nssv_HAVE_STD_HASH nssv_CPP11_120

// C++ feature usage:

#if nssv_HAVE_CONSTEXPR_11
#define nssv_constexpr constexpr
#else
#define nssv_constexpr /*constexpr*/
#endif

#if nssv_HAVE_CONSTEXPR_14
#define nssv_constexpr14 constexpr
#else
#define nssv_constexpr14 /*constexpr*/
#endif

#if nssv_HAVE_EXPLICIT_CONVERSION
#define nssv_explicit explicit
#else
#define nssv_explicit /*explicit*/
#endif

#if nssv_HAVE_INLINE_NAMESPACE
#define nssv_inline_ns inline
#else
#define nssv_inline_ns /*inline*/
#endif

#if nssv_HAVE_NOEXCEPT
#define nssv_noexcept noexcept
#else
#define nssv_noexcept /*noexcept*/
#endif

//#if nssv_HAVE_REF_QUALIFIER
//# define nssv_ref_qual  &
//# define nssv_refref_qual  &&
//#else
//# define nssv_ref_qual  /*&*/
//# define nssv_refref_qual  /*&&*/
//#endif

#if nssv_HAVE_NULLPTR
#define nssv_nullptr nullptr
#else
#define nssv_nullptr NULL
#endif

#if nssv_HAVE_NODISCARD
#define nssv_nodiscard [[nodiscard]]
#else
#define nssv_nodiscard /*[[nodiscard]]*/
#endif

// Additional includes:

#include <algorithm>
#include <cassert>
#include <iterator>
#include <limits>
#include <ostream>
#include <string> // std::char_traits<>

#if !nssv_CONFIG_NO_EXCEPTIONS
#include <stdexcept>
#endif

#if nssv_CPP11_OR_GREATER
#include <type_traits>
#endif

// Clang, GNUC, MSVC warning suppression macros:

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wreserved-user-defined-literal"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-literals"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wliteral-suffix"
#endif // __clang__

#if nssv_COMPILER_MSVC_VERSION >= 140
#define nssv_SUPPRESS_MSGSL_WARNING(expr) [[gsl::suppress(expr)]]
#define nssv_SUPPRESS_MSVC_WARNING(code, descr) __pragma(warning(suppress : code))
#define nssv_DISABLE_MSVC_WARNINGS(codes) __pragma(warning(push)) __pragma(warning(disable : codes))
#else
#define nssv_SUPPRESS_MSGSL_WARNING(expr)
#define nssv_SUPPRESS_MSVC_WARNING(code, descr)
#define nssv_DISABLE_MSVC_WARNINGS(codes)
#endif

#if defined(__clang__)
#define nssv_RESTORE_WARNINGS() _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define nssv_RESTORE_WARNINGS() _Pragma("GCC diagnostic pop")
#elif nssv_COMPILER_MSVC_VERSION >= 140
#define nssv_RESTORE_WARNINGS() __pragma(warning(pop))
#else
#define nssv_RESTORE_WARNINGS()
#endif

// Suppress the following MSVC (GSL) warnings:
// - C4455, non-gsl   : 'operator ""sv': literal suffix identifiers that do not
//                      start with an underscore are reserved
// - C26472, gsl::t.1 : don't use a static_cast for arithmetic conversions;
//                      use brace initialization, gsl::narrow_cast or gsl::narow
// - C26481: gsl::b.1 : don't use pointer arithmetic. Use span instead

nssv_DISABLE_MSVC_WARNINGS(4455 26481 26472)
    // nssv_DISABLE_CLANG_WARNINGS( "-Wuser-defined-literals" )
    // nssv_DISABLE_GNUC_WARNINGS( -Wliteral-suffix )

    namespace nonstd {
  namespace sv_lite {

#if nssv_CPP11_OR_GREATER

  namespace detail {

  // Expect tail call optimization to make length() non-recursive:

  template <typename CharT> inline constexpr std::size_t length(CharT *s, std::size_t result = 0) {
    return *s == '\0' ? result : length(s + 1, result + 1);
  }

  } // namespace detail

#endif // nssv_CPP11_OR_GREATER

  template <class CharT, class Traits = std::char_traits<CharT>> class basic_string_view;

  //
  // basic_string_view:
  //

  template <class CharT, class Traits /* = std::char_traits<CharT> */
            >
  class basic_string_view {
  public:
    // Member types:

    typedef Traits traits_type;
    typedef CharT value_type;

    typedef CharT *pointer;
    typedef CharT const *const_pointer;
    typedef CharT &reference;
    typedef CharT const &const_reference;

    typedef const_pointer iterator;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator<const_iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // 24.4.2.1 Construction and assignment:

    nssv_constexpr basic_string_view() nssv_noexcept : data_(nssv_nullptr), size_(0) {}

#if nssv_CPP11_OR_GREATER
    nssv_constexpr basic_string_view(basic_string_view const &other) nssv_noexcept = default;
#else
    nssv_constexpr basic_string_view(basic_string_view const &other) nssv_noexcept : data_(other.data_),
                                                                                     size_(other.size_) {}
#endif

    nssv_constexpr basic_string_view(CharT const *s, size_type count) nssv_noexcept // non-standard noexcept
        : data_(s),
          size_(count) {}

    nssv_constexpr basic_string_view(CharT const *s) nssv_noexcept // non-standard noexcept
        : data_(s)
#if nssv_CPP17_OR_GREATER
        ,
          size_(Traits::length(s))
#elif nssv_CPP11_OR_GREATER
        ,
          size_(detail::length(s))
#else
        ,
          size_(Traits::length(s))
#endif
    {
    }

    // Assignment:

#if nssv_CPP11_OR_GREATER
    nssv_constexpr14 basic_string_view &operator=(basic_string_view const &other) nssv_noexcept = default;
#else
    nssv_constexpr14 basic_string_view &operator=(basic_string_view const &other) nssv_noexcept {
      data_ = other.data_;
      size_ = other.size_;
      return *this;
    }
#endif

    // 24.4.2.2 Iterator support:

    nssv_constexpr const_iterator begin() const nssv_noexcept { return data_; }
    nssv_constexpr const_iterator end() const nssv_noexcept { return data_ + size_; }

    nssv_constexpr const_iterator cbegin() const nssv_noexcept { return begin(); }
    nssv_constexpr const_iterator cend() const nssv_noexcept { return end(); }

    nssv_constexpr const_reverse_iterator rbegin() const nssv_noexcept { return const_reverse_iterator(end()); }
    nssv_constexpr const_reverse_iterator rend() const nssv_noexcept { return const_reverse_iterator(begin()); }

    nssv_constexpr const_reverse_iterator crbegin() const nssv_noexcept { return rbegin(); }
    nssv_constexpr const_reverse_iterator crend() const nssv_noexcept { return rend(); }

    // 24.4.2.3 Capacity:

    nssv_constexpr size_type size() const nssv_noexcept { return size_; }
    nssv_constexpr size_type length() const nssv_noexcept { return size_; }
    nssv_constexpr size_type max_size() const nssv_noexcept { return (std::numeric_limits<size_type>::max)(); }

    // since C++20
    nssv_nodiscard nssv_constexpr bool empty() const nssv_noexcept { return 0 == size_; }

    // 24.4.2.4 Element access:

    nssv_constexpr const_reference operator[](size_type pos) const { return data_at(pos); }

    nssv_constexpr14 const_reference at(size_type pos) const {
#if nssv_CONFIG_NO_EXCEPTIONS
      assert(pos < size());
#else
      if (pos >= size()) {
        throw std::out_of_range("nonstd::string_view::at()");
      }
#endif
      return data_at(pos);
    }

    nssv_constexpr const_reference front() const { return data_at(0); }
    nssv_constexpr const_reference back() const { return data_at(size() - 1); }

    nssv_constexpr const_pointer data() const nssv_noexcept { return data_; }

    // 24.4.2.5 Modifiers:

    nssv_constexpr14 void remove_prefix(size_type n) {
      assert(n <= size());
      data_ += n;
      size_ -= n;
    }

    nssv_constexpr14 void remove_suffix(size_type n) {
      assert(n <= size());
      size_ -= n;
    }

    nssv_constexpr14 void swap(basic_string_view &other) nssv_noexcept {
      using std::swap;
      swap(data_, other.data_);
      swap(size_, other.size_);
    }

    // 24.4.2.6 String operations:

    size_type copy(CharT *dest, size_type n, size_type pos = 0) const {
#if nssv_CONFIG_NO_EXCEPTIONS
      assert(pos <= size());
#else
      if (pos > size()) {
        throw std::out_of_range("nonstd::string_view::copy()");
      }
#endif
      const size_type rlen = (std::min)(n, size() - pos);

      (void)Traits::copy(dest, data() + pos, rlen);

      return rlen;
    }

    nssv_constexpr14 basic_string_view substr(size_type pos = 0, size_type n = npos) const {
#if nssv_CONFIG_NO_EXCEPTIONS
      assert(pos <= size());
#else
      if (pos > size()) {
        throw std::out_of_range("nonstd::string_view::substr()");
      }
#endif
      return basic_string_view(data() + pos, (std::min)(n, size() - pos));
    }

    // compare(), 6x:

    nssv_constexpr14 int compare(basic_string_view other) const nssv_noexcept // (1)
    {
      if (const int result = Traits::compare(data(), other.data(), (std::min)(size(), other.size()))) {
        return result;
      }

      return size() == other.size() ? 0 : size() < other.size() ? -1 : 1;
    }

    nssv_constexpr int compare(size_type pos1, size_type n1, basic_string_view other) const // (2)
    {
      return substr(pos1, n1).compare(other);
    }

    nssv_constexpr int compare(size_type pos1, size_type n1, basic_string_view other, size_type pos2,
                               size_type n2) const // (3)
    {
      return substr(pos1, n1).compare(other.substr(pos2, n2));
    }

    nssv_constexpr int compare(CharT const *s) const // (4)
    {
      return compare(basic_string_view(s));
    }

    nssv_constexpr int compare(size_type pos1, size_type n1, CharT const *s) const // (5)
    {
      return substr(pos1, n1).compare(basic_string_view(s));
    }

    nssv_constexpr int compare(size_type pos1, size_type n1, CharT const *s, size_type n2) const // (6)
    {
      return substr(pos1, n1).compare(basic_string_view(s, n2));
    }

    // 24.4.2.7 Searching:

    // starts_with(), 3x, since C++20:

    nssv_constexpr bool starts_with(basic_string_view v) const nssv_noexcept // (1)
    {
      return size() >= v.size() && compare(0, v.size(), v) == 0;
    }

    nssv_constexpr bool starts_with(CharT c) const nssv_noexcept // (2)
    {
      return starts_with(basic_string_view(&c, 1));
    }

    nssv_constexpr bool starts_with(CharT const *s) const // (3)
    {
      return starts_with(basic_string_view(s));
    }

    // ends_with(), 3x, since C++20:

    nssv_constexpr bool ends_with(basic_string_view v) const nssv_noexcept // (1)
    {
      return size() >= v.size() && compare(size() - v.size(), npos, v) == 0;
    }

    nssv_constexpr bool ends_with(CharT c) const nssv_noexcept // (2)
    {
      return ends_with(basic_string_view(&c, 1));
    }

    nssv_constexpr bool ends_with(CharT const *s) const // (3)
    {
      return ends_with(basic_string_view(s));
    }

    // find(), 4x:

    nssv_constexpr14 size_type find(basic_string_view v, size_type pos = 0) const nssv_noexcept // (1)
    {
      return assert(v.size() == 0 || v.data() != nssv_nullptr),
             pos >= size() ? npos : to_pos(std::search(cbegin() + pos, cend(), v.cbegin(), v.cend(), Traits::eq));
    }

    nssv_constexpr14 size_type find(CharT c, size_type pos = 0) const nssv_noexcept // (2)
    {
      return find(basic_string_view(&c, 1), pos);
    }

    nssv_constexpr14 size_type find(CharT const *s, size_type pos, size_type n) const // (3)
    {
      return find(basic_string_view(s, n), pos);
    }

    nssv_constexpr14 size_type find(CharT const *s, size_type pos = 0) const // (4)
    {
      return find(basic_string_view(s), pos);
    }

    // rfind(), 4x:

    nssv_constexpr14 size_type rfind(basic_string_view v, size_type pos = npos) const nssv_noexcept // (1)
    {
      if (size() < v.size()) {
        return npos;
      }

      if (v.empty()) {
        return (std::min)(size(), pos);
      }

      const_iterator last = cbegin() + (std::min)(size() - v.size(), pos) + v.size();
      const_iterator result = std::find_end(cbegin(), last, v.cbegin(), v.cend(), Traits::eq);

      return result != last ? size_type(result - cbegin()) : npos;
    }

    nssv_constexpr14 size_type rfind(CharT c, size_type pos = npos) const nssv_noexcept // (2)
    {
      return rfind(basic_string_view(&c, 1), pos);
    }

    nssv_constexpr14 size_type rfind(CharT const *s, size_type pos, size_type n) const // (3)
    {
      return rfind(basic_string_view(s, n), pos);
    }

    nssv_constexpr14 size_type rfind(CharT const *s, size_type pos = npos) const // (4)
    {
      return rfind(basic_string_view(s), pos);
    }

    // find_first_of(), 4x:

    nssv_constexpr size_type find_first_of(basic_string_view v, size_type pos = 0) const nssv_noexcept // (1)
    {
      return pos >= size() ? npos
                           : to_pos(std::find_first_of(cbegin() + pos, cend(), v.cbegin(), v.cend(), Traits::eq));
    }

    nssv_constexpr size_type find_first_of(CharT c, size_type pos = 0) const nssv_noexcept // (2)
    {
      return find_first_of(basic_string_view(&c, 1), pos);
    }

    nssv_constexpr size_type find_first_of(CharT const *s, size_type pos, size_type n) const // (3)
    {
      return find_first_of(basic_string_view(s, n), pos);
    }

    nssv_constexpr size_type find_first_of(CharT const *s, size_type pos = 0) const // (4)
    {
      return find_first_of(basic_string_view(s), pos);
    }

    // find_last_of(), 4x:

    nssv_constexpr size_type find_last_of(basic_string_view v, size_type pos = npos) const nssv_noexcept // (1)
    {
      return empty() ? npos
                     : pos >= size() ? find_last_of(v, size() - 1)
                                     : to_pos(std::find_first_of(const_reverse_iterator(cbegin() + pos + 1), crend(),
                                                                 v.cbegin(), v.cend(), Traits::eq));
    }

    nssv_constexpr size_type find_last_of(CharT c, size_type pos = npos) const nssv_noexcept // (2)
    {
      return find_last_of(basic_string_view(&c, 1), pos);
    }

    nssv_constexpr size_type find_last_of(CharT const *s, size_type pos, size_type count) const // (3)
    {
      return find_last_of(basic_string_view(s, count), pos);
    }

    nssv_constexpr size_type find_last_of(CharT const *s, size_type pos = npos) const // (4)
    {
      return find_last_of(basic_string_view(s), pos);
    }

    // find_first_not_of(), 4x:

    nssv_constexpr size_type find_first_not_of(basic_string_view v, size_type pos = 0) const nssv_noexcept // (1)
    {
      return pos >= size() ? npos : to_pos(std::find_if(cbegin() + pos, cend(), not_in_view(v)));
    }

    nssv_constexpr size_type find_first_not_of(CharT c, size_type pos = 0) const nssv_noexcept // (2)
    {
      return find_first_not_of(basic_string_view(&c, 1), pos);
    }

    nssv_constexpr size_type find_first_not_of(CharT const *s, size_type pos, size_type count) const // (3)
    {
      return find_first_not_of(basic_string_view(s, count), pos);
    }

    nssv_constexpr size_type find_first_not_of(CharT const *s, size_type pos = 0) const // (4)
    {
      return find_first_not_of(basic_string_view(s), pos);
    }

    // find_last_not_of(), 4x:

    nssv_constexpr size_type find_last_not_of(basic_string_view v, size_type pos = npos) const nssv_noexcept // (1)
    {
      return empty() ? npos
                     : pos >= size()
                           ? find_last_not_of(v, size() - 1)
                           : to_pos(std::find_if(const_reverse_iterator(cbegin() + pos + 1), crend(), not_in_view(v)));
    }

    nssv_constexpr size_type find_last_not_of(CharT c, size_type pos = npos) const nssv_noexcept // (2)
    {
      return find_last_not_of(basic_string_view(&c, 1), pos);
    }

    nssv_constexpr size_type find_last_not_of(CharT const *s, size_type pos, size_type count) const // (3)
    {
      return find_last_not_of(basic_string_view(s, count), pos);
    }

    nssv_constexpr size_type find_last_not_of(CharT const *s, size_type pos = npos) const // (4)
    {
      return find_last_not_of(basic_string_view(s), pos);
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
    struct not_in_view {
      const basic_string_view v;

      nssv_constexpr explicit not_in_view(basic_string_view v) : v(v) {}

      nssv_constexpr bool operator()(CharT c) const { return npos == v.find_first_of(c); }
    };

    nssv_constexpr size_type to_pos(const_iterator it) const { return it == cend() ? npos : size_type(it - cbegin()); }

    nssv_constexpr size_type to_pos(const_reverse_iterator it) const {
      return it == crend() ? npos : size_type(crend() - it - 1);
    }

    nssv_constexpr const_reference data_at(size_type pos) const {
#if nssv_BETWEEN(nssv_COMPILER_GNUC_VERSION, 1, 500)
      return data_[pos];
#else
      return assert(pos < size()), data_[pos];
#endif
    }

  private:
    const_pointer data_;
    size_type size_;

  public:
#if nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS

    template <class Allocator>
    basic_string_view(std::basic_string<CharT, Traits, Allocator> const &s) nssv_noexcept : data_(s.data()),
                                                                                            size_(s.size()) {}

#if nssv_HAVE_EXPLICIT_CONVERSION

    template <class Allocator> explicit operator std::basic_string<CharT, Traits, Allocator>() const {
      return to_string(Allocator());
    }

#endif // nssv_HAVE_EXPLICIT_CONVERSION

#if nssv_CPP11_OR_GREATER

    template <class Allocator = std::allocator<CharT>>
    std::basic_string<CharT, Traits, Allocator> to_string(Allocator const &a = Allocator()) const {
      return std::basic_string<CharT, Traits, Allocator>(begin(), end(), a);
    }

#else

    std::basic_string<CharT, Traits> to_string() const { return std::basic_string<CharT, Traits>(begin(), end()); }

    template <class Allocator> std::basic_string<CharT, Traits, Allocator> to_string(Allocator const &a) const {
      return std::basic_string<CharT, Traits, Allocator>(begin(), end(), a);
    }

#endif // nssv_CPP11_OR_GREATER

#endif // nssv_CONFIG_CONVERSION_STD_STRING_CLASS_METHODS
  };

  //
  // Non-member functions:
  //

  // 24.4.3 Non-member comparison functions:
  // lexicographically compare two string views (function template):

  template <class CharT, class Traits>
  nssv_constexpr bool operator==(basic_string_view<CharT, Traits> lhs,
                                 basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) == 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator!=(basic_string_view<CharT, Traits> lhs,
                                 basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) != 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator<(basic_string_view<CharT, Traits> lhs,
                                basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) < 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator<=(basic_string_view<CharT, Traits> lhs,
                                 basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) <= 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator>(basic_string_view<CharT, Traits> lhs,
                                basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) > 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator>=(basic_string_view<CharT, Traits> lhs,
                                 basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) >= 0;
  }

  // Let S be basic_string_view<CharT, Traits>, and sv be an instance of S.
  // Implementations shall provide sufficient additional overloads marked
  // constexpr and noexcept so that an object t with an implicit conversion
  // to S can be compared according to Table 67.

#if !nssv_CPP11_OR_GREATER || nssv_BETWEEN(nssv_COMPILER_MSVC_VERSION, 100, 141)

  // accomodate for older compilers:

  // ==

  template <class CharT, class Traits>
  nssv_constexpr bool operator==(basic_string_view<CharT, Traits> lhs, char const *rhs) nssv_noexcept {
    return lhs.compare(rhs) == 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator==(char const *lhs, basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return rhs.compare(lhs) == 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator==(basic_string_view<CharT, Traits> lhs,
                                 std::basic_string<CharT, Traits> rhs) nssv_noexcept {
    return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator==(std::basic_string<CharT, Traits> rhs,
                                 basic_string_view<CharT, Traits> lhs) nssv_noexcept {
    return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
  }

  // !=

  template <class CharT, class Traits>
  nssv_constexpr bool operator!=(basic_string_view<CharT, Traits> lhs, char const *rhs) nssv_noexcept {
    return lhs.compare(rhs) != 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator!=(char const *lhs, basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return rhs.compare(lhs) != 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator!=(basic_string_view<CharT, Traits> lhs,
                                 std::basic_string<CharT, Traits> rhs) nssv_noexcept {
    return lhs.size() != rhs.size() && lhs.compare(rhs) != 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator!=(std::basic_string<CharT, Traits> rhs,
                                 basic_string_view<CharT, Traits> lhs) nssv_noexcept {
    return lhs.size() != rhs.size() || rhs.compare(lhs) != 0;
  }

  // <

  template <class CharT, class Traits>
  nssv_constexpr bool operator<(basic_string_view<CharT, Traits> lhs, char const *rhs) nssv_noexcept {
    return lhs.compare(rhs) < 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator<(char const *lhs, basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return rhs.compare(lhs) > 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator<(basic_string_view<CharT, Traits> lhs,
                                std::basic_string<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) < 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator<(std::basic_string<CharT, Traits> rhs,
                                basic_string_view<CharT, Traits> lhs) nssv_noexcept {
    return rhs.compare(lhs) > 0;
  }

  // <=

  template <class CharT, class Traits>
  nssv_constexpr bool operator<=(basic_string_view<CharT, Traits> lhs, char const *rhs) nssv_noexcept {
    return lhs.compare(rhs) <= 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator<=(char const *lhs, basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return rhs.compare(lhs) >= 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator<=(basic_string_view<CharT, Traits> lhs,
                                 std::basic_string<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) <= 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator<=(std::basic_string<CharT, Traits> rhs,
                                 basic_string_view<CharT, Traits> lhs) nssv_noexcept {
    return rhs.compare(lhs) >= 0;
  }

  // >

  template <class CharT, class Traits>
  nssv_constexpr bool operator>(basic_string_view<CharT, Traits> lhs, char const *rhs) nssv_noexcept {
    return lhs.compare(rhs) > 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator>(char const *lhs, basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return rhs.compare(lhs) < 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator>(basic_string_view<CharT, Traits> lhs,
                                std::basic_string<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) > 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator>(std::basic_string<CharT, Traits> rhs,
                                basic_string_view<CharT, Traits> lhs) nssv_noexcept {
    return rhs.compare(lhs) < 0;
  }

  // >=

  template <class CharT, class Traits>
  nssv_constexpr bool operator>=(basic_string_view<CharT, Traits> lhs, char const *rhs) nssv_noexcept {
    return lhs.compare(rhs) >= 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator>=(char const *lhs, basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return rhs.compare(lhs) <= 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator>=(basic_string_view<CharT, Traits> lhs,
                                 std::basic_string<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) >= 0;
  }

  template <class CharT, class Traits>
  nssv_constexpr bool operator>=(std::basic_string<CharT, Traits> rhs,
                                 basic_string_view<CharT, Traits> lhs) nssv_noexcept {
    return rhs.compare(lhs) <= 0;
  }

#else // newer compilers:

#define nssv_BASIC_STRING_VIEW_I(T, U) typename std::decay<basic_string_view<T, U>>::type

#if nssv_BETWEEN(nssv_COMPILER_MSVC_VERSION, 140, 150)
#define nssv_MSVC_ORDER(x) , int = x
#else
#define nssv_MSVC_ORDER(x) /*, int=x*/
#endif

  // ==

  template <class CharT, class Traits nssv_MSVC_ORDER(1)>
  nssv_constexpr bool operator==(basic_string_view<CharT, Traits> lhs,
                                 nssv_BASIC_STRING_VIEW_I(CharT, Traits) rhs) nssv_noexcept {
    return lhs.compare(rhs) == 0;
  }

  template <class CharT, class Traits nssv_MSVC_ORDER(2)>
  nssv_constexpr bool operator==(nssv_BASIC_STRING_VIEW_I(CharT, Traits) lhs,
                                 basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
  }

  // !=

  template <class CharT, class Traits nssv_MSVC_ORDER(1)>
  nssv_constexpr bool operator!=(basic_string_view<CharT, Traits> lhs,
                                 nssv_BASIC_STRING_VIEW_I(CharT, Traits) rhs) nssv_noexcept {
    return lhs.size() != rhs.size() || lhs.compare(rhs) != 0;
  }

  template <class CharT, class Traits nssv_MSVC_ORDER(2)>
  nssv_constexpr bool operator!=(nssv_BASIC_STRING_VIEW_I(CharT, Traits) lhs,
                                 basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) != 0;
  }

  // <

  template <class CharT, class Traits nssv_MSVC_ORDER(1)>
  nssv_constexpr bool operator<(basic_string_view<CharT, Traits> lhs,
                                nssv_BASIC_STRING_VIEW_I(CharT, Traits) rhs) nssv_noexcept {
    return lhs.compare(rhs) < 0;
  }

  template <class CharT, class Traits nssv_MSVC_ORDER(2)>
  nssv_constexpr bool operator<(nssv_BASIC_STRING_VIEW_I(CharT, Traits) lhs,
                                basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) < 0;
  }

  // <=

  template <class CharT, class Traits nssv_MSVC_ORDER(1)>
  nssv_constexpr bool operator<=(basic_string_view<CharT, Traits> lhs,
                                 nssv_BASIC_STRING_VIEW_I(CharT, Traits) rhs) nssv_noexcept {
    return lhs.compare(rhs) <= 0;
  }

  template <class CharT, class Traits nssv_MSVC_ORDER(2)>
  nssv_constexpr bool operator<=(nssv_BASIC_STRING_VIEW_I(CharT, Traits) lhs,
                                 basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) <= 0;
  }

  // >

  template <class CharT, class Traits nssv_MSVC_ORDER(1)>
  nssv_constexpr bool operator>(basic_string_view<CharT, Traits> lhs,
                                nssv_BASIC_STRING_VIEW_I(CharT, Traits) rhs) nssv_noexcept {
    return lhs.compare(rhs) > 0;
  }

  template <class CharT, class Traits nssv_MSVC_ORDER(2)>
  nssv_constexpr bool operator>(nssv_BASIC_STRING_VIEW_I(CharT, Traits) lhs,
                                basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) > 0;
  }

  // >=

  template <class CharT, class Traits nssv_MSVC_ORDER(1)>
  nssv_constexpr bool operator>=(basic_string_view<CharT, Traits> lhs,
                                 nssv_BASIC_STRING_VIEW_I(CharT, Traits) rhs) nssv_noexcept {
    return lhs.compare(rhs) >= 0;
  }

  template <class CharT, class Traits nssv_MSVC_ORDER(2)>
  nssv_constexpr bool operator>=(nssv_BASIC_STRING_VIEW_I(CharT, Traits) lhs,
                                 basic_string_view<CharT, Traits> rhs) nssv_noexcept {
    return lhs.compare(rhs) >= 0;
  }

#undef nssv_MSVC_ORDER
#undef nssv_BASIC_STRING_VIEW_I

#endif // compiler-dependent approach to comparisons

  // 24.4.4 Inserters and extractors:

  namespace detail {

  template <class Stream> void write_padding(Stream &os, std::streamsize n) {
    for (std::streamsize i = 0; i < n; ++i)
      os.rdbuf()->sputc(os.fill());
  }

  template <class Stream, class View> Stream &write_to_stream(Stream &os, View const &sv) {
    typename Stream::sentry sentry(os);

    if (!os)
      return os;

    const std::streamsize length = static_cast<std::streamsize>(sv.length());

    // Whether, and how, to pad:
    const bool pad = (length < os.width());
    const bool left_pad = pad && (os.flags() & std::ios_base::adjustfield) == std::ios_base::right;

    if (left_pad)
      write_padding(os, os.width() - length);

    // Write span characters:
    os.rdbuf()->sputn(sv.begin(), length);

    if (pad && !left_pad)
      write_padding(os, os.width() - length);

    // Reset output stream width:
    os.width(0);

    return os;
  }

  } // namespace detail

  template <class CharT, class Traits>
  std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os,
                                                basic_string_view<CharT, Traits> sv) {
    return detail::write_to_stream(os, sv);
  }

  // Several typedefs for common character types are provided:

  typedef basic_string_view<char> string_view;
  typedef basic_string_view<wchar_t> wstring_view;
#if nssv_HAVE_WCHAR16_T
  typedef basic_string_view<char16_t> u16string_view;
  typedef basic_string_view<char32_t> u32string_view;
#endif

  } // namespace sv_lite
} // namespace nonstd::sv_lite

//
// 24.4.6 Suffix for basic_string_view literals:
//

#if nssv_HAVE_USER_DEFINED_LITERALS

namespace nonstd {
nssv_inline_ns namespace literals {
  nssv_inline_ns namespace string_view_literals {

#if nssv_CONFIG_STD_SV_OPERATOR && nssv_HAVE_STD_DEFINED_LITERALS

    nssv_constexpr nonstd::sv_lite::string_view operator"" sv(const char *str, size_t len) nssv_noexcept // (1)
    {
      return nonstd::sv_lite::string_view {str, len};
    }

    nssv_constexpr nonstd::sv_lite::u16string_view operator"" sv(const char16_t *str, size_t len) nssv_noexcept // (2)
    {
      return nonstd::sv_lite::u16string_view {str, len};
    }

    nssv_constexpr nonstd::sv_lite::u32string_view operator"" sv(const char32_t *str, size_t len) nssv_noexcept // (3)
    {
      return nonstd::sv_lite::u32string_view {str, len};
    }

    nssv_constexpr nonstd::sv_lite::wstring_view operator"" sv(const wchar_t *str, size_t len) nssv_noexcept // (4)
    {
      return nonstd::sv_lite::wstring_view {str, len};
    }

#endif // nssv_CONFIG_STD_SV_OPERATOR && nssv_HAVE_STD_DEFINED_LITERALS

#if nssv_CONFIG_USR_SV_OPERATOR

    nssv_constexpr nonstd::sv_lite::string_view operator"" _sv(const char *str, size_t len) nssv_noexcept // (1)
    {
      return nonstd::sv_lite::string_view {str, len};
    }

    nssv_constexpr nonstd::sv_lite::u16string_view operator"" _sv(const char16_t *str, size_t len) nssv_noexcept // (2)
    {
      return nonstd::sv_lite::u16string_view {str, len};
    }

    nssv_constexpr nonstd::sv_lite::u32string_view operator"" _sv(const char32_t *str, size_t len) nssv_noexcept // (3)
    {
      return nonstd::sv_lite::u32string_view {str, len};
    }

    nssv_constexpr nonstd::sv_lite::wstring_view operator"" _sv(const wchar_t *str, size_t len) nssv_noexcept // (4)
    {
      return nonstd::sv_lite::wstring_view {str, len};
    }

#endif // nssv_CONFIG_USR_SV_OPERATOR
  }
}
} // namespace nonstd

#endif

//
// Extensions for std::string:
//

#if nssv_CONFIG_CONVERSION_STD_STRING_FREE_FUNCTIONS

namespace nonstd {
namespace sv_lite {

// Exclude MSVC 14 (19.00): it yields ambiguous to_string():

#if nssv_CPP11_OR_GREATER && nssv_COMPILER_MSVC_VERSION != 140

template <class CharT, class Traits, class Allocator = std::allocator<CharT>>
std::basic_string<CharT, Traits, Allocator> to_string(basic_string_view<CharT, Traits> v,
                                                      Allocator const &a = Allocator()) {
  return std::basic_string<CharT, Traits, Allocator>(v.begin(), v.end(), a);
}

#else

template <class CharT, class Traits> std::basic_string<CharT, Traits> to_string(basic_string_view<CharT, Traits> v) {
  return std::basic_string<CharT, Traits>(v.begin(), v.end());
}

template <class CharT, class Traits, class Allocator>
std::basic_string<CharT, Traits, Allocator> to_string(basic_string_view<CharT, Traits> v, Allocator const &a) {
  return std::basic_string<CharT, Traits, Allocator>(v.begin(), v.end(), a);
}

#endif // nssv_CPP11_OR_GREATER

template <class CharT, class Traits, class Allocator>
basic_string_view<CharT, Traits> to_string_view(std::basic_string<CharT, Traits, Allocator> const &s) {
  return basic_string_view<CharT, Traits>(s.data(), s.size());
}

} // namespace sv_lite
} // namespace nonstd

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

template <> struct hash<nonstd::string_view> {
public:
  std::size_t operator()(nonstd::string_view v) const nssv_noexcept {
    return std::hash<std::string>()(std::string(v.data(), v.size()));
  }
};

template <> struct hash<nonstd::wstring_view> {
public:
  std::size_t operator()(nonstd::wstring_view v) const nssv_noexcept {
    return std::hash<std::wstring>()(std::wstring(v.data(), v.size()));
  }
};

template <> struct hash<nonstd::u16string_view> {
public:
  std::size_t operator()(nonstd::u16string_view v) const nssv_noexcept {
    return std::hash<std::u16string>()(std::u16string(v.data(), v.size()));
  }
};

template <> struct hash<nonstd::u32string_view> {
public:
  std::size_t operator()(nonstd::u32string_view v) const nssv_noexcept {
    return std::hash<std::u32string>()(std::u32string(v.data(), v.size()));
  }
};

} // namespace std

#endif // nssv_HAVE_STD_HASH

nssv_RESTORE_WARNINGS()

#endif // nssv_HAVE_STD_STRING_VIEW
#endif // NONSTD_SV_LITE_H_INCLUDED


namespace inja {

/*!
 * \brief Class for lexer configuration.
 */
struct LexerConfig {
  std::string statement_open {"{%"};
  std::string statement_open_no_lstrip {"{%+"};
  std::string statement_open_force_lstrip {"{%-"};
  std::string statement_close {"%}"};
  std::string statement_close_force_rstrip {"-%}"};
  std::string line_statement {"##"};
  std::string expression_open {"{{"};
  std::string expression_open_force_lstrip {"{{-"};
  std::string expression_close {"}}"};
  std::string expression_close_force_rstrip {"-}}"};
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
    if (open_chars.find(statement_open_no_lstrip[0]) == std::string::npos) {
      open_chars += statement_open_no_lstrip[0];
    }
    if (open_chars.find(statement_open_force_lstrip[0]) == std::string::npos) {
      open_chars += statement_open_force_lstrip[0];
    }
    if (open_chars.find(expression_open[0]) == std::string::npos) {
      open_chars += expression_open[0];
    }
    if (open_chars.find(expression_open_force_lstrip[0]) == std::string::npos) {
      open_chars += expression_open_force_lstrip[0];
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
  bool search_included_templates_in_files {true};
  /// Extra options for limiting included files' scope
  bool include_scope_limit = false;
  std::string include_scope = "templates";
};

/*!
 * \brief Class for render configuration.
 */
struct RenderConfig {
  bool throw_at_missing_includes {true};
};

} // namespace inja

#endif // INCLUDE_INJA_CONFIG_HPP_

// #include "function_storage.hpp"
// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_FUNCTION_STORAGE_HPP_
#define INCLUDE_INJA_FUNCTION_STORAGE_HPP_

#include <vector>

// #include "string_view.hpp"


namespace inja {

using json = nlohmann::json;

using Arguments = std::vector<const json *>;
using CallbackFunction = std::function<json(Arguments &args)>;
using VoidCallbackFunction = std::function<void(Arguments &args)>;

/*!
 * \brief Class for builtin functions and user-defined callbacks.
 */
class FunctionStorage {
public:
  enum class Operation {
    Not,
    And,
    Or,
    In,
    Equal,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Add,
    Subtract,
    Multiplication,
    Division,
    Power,
    Modulo,
    AtId,
    At,
    Default,
    DivisibleBy,
    Even,
    Exists,
    ExistsInObject,
    First,
    Float,
    Int,
    IsArray,
    IsBoolean,
    IsFloat,
    IsInteger,
    IsNumber,
    IsObject,
    IsString,
    Last,
    Length,
    Lower,
    Max,
    Min,
    Odd,
    Range,
    Round,
    Sort,
    Upper,
    Callback,
    ParenLeft,
    ParenRight,
    None,
  };

  struct FunctionData {
    explicit FunctionData(const Operation &op, const CallbackFunction &cb = CallbackFunction{}) : operation(op), callback(cb) {}
    const Operation operation;
    const CallbackFunction callback;
  };

private:
  const int VARIADIC {-1};

  std::map<std::pair<std::string, int>, FunctionData> function_storage = {
    {std::make_pair("at", 2), FunctionData { Operation::At }},
    {std::make_pair("default", 2), FunctionData { Operation::Default }},
    {std::make_pair("divisibleBy", 2), FunctionData { Operation::DivisibleBy }},
    {std::make_pair("even", 1), FunctionData { Operation::Even }},
    {std::make_pair("exists", 1), FunctionData { Operation::Exists }},
    {std::make_pair("existsIn", 2), FunctionData { Operation::ExistsInObject }},
    {std::make_pair("first", 1), FunctionData { Operation::First }},
    {std::make_pair("float", 1), FunctionData { Operation::Float }},
    {std::make_pair("int", 1), FunctionData { Operation::Int }},
    {std::make_pair("isArray", 1), FunctionData { Operation::IsArray }},
    {std::make_pair("isBoolean", 1), FunctionData { Operation::IsBoolean }},
    {std::make_pair("isFloat", 1), FunctionData { Operation::IsFloat }},
    {std::make_pair("isInteger", 1), FunctionData { Operation::IsInteger }},
    {std::make_pair("isNumber", 1), FunctionData { Operation::IsNumber }},
    {std::make_pair("isObject", 1), FunctionData { Operation::IsObject }},
    {std::make_pair("isString", 1), FunctionData { Operation::IsString }},
    {std::make_pair("last", 1), FunctionData { Operation::Last }},
    {std::make_pair("length", 1), FunctionData { Operation::Length }},
    {std::make_pair("lower", 1), FunctionData { Operation::Lower }},
    {std::make_pair("max", 1), FunctionData { Operation::Max }},
    {std::make_pair("min", 1), FunctionData { Operation::Min }},
    {std::make_pair("odd", 1), FunctionData { Operation::Odd }},
    {std::make_pair("range", 1), FunctionData { Operation::Range }},
    {std::make_pair("round", 2), FunctionData { Operation::Round }},
    {std::make_pair("sort", 1), FunctionData { Operation::Sort }},
    {std::make_pair("upper", 1), FunctionData { Operation::Upper }},
  };

public:
  void add_builtin(nonstd::string_view name, int num_args, Operation op) {
    function_storage.emplace(std::make_pair(static_cast<std::string>(name), num_args), FunctionData { op });
  }

  void add_callback(nonstd::string_view name, int num_args, const CallbackFunction &callback) {
    function_storage.emplace(std::make_pair(static_cast<std::string>(name), num_args), FunctionData { Operation::Callback, callback });
  }

  FunctionData find_function(nonstd::string_view name, int num_args) const {
    auto it = function_storage.find(std::make_pair(static_cast<std::string>(name), num_args));
    if (it != function_storage.end()) {
      return it->second;

    // Find variadic function
    } else if (num_args > 0) {
      it = function_storage.find(std::make_pair(static_cast<std::string>(name), VARIADIC));
      if (it != function_storage.end()) {
        return it->second;
      }
    }

    return FunctionData { Operation::None };
  }
};

} // namespace inja

#endif // INCLUDE_INJA_FUNCTION_STORAGE_HPP_

// #include "parser.hpp"
// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_PARSER_HPP_
#define INCLUDE_INJA_PARSER_HPP_

#include <limits>
#include <stack>
#include <string>
#include <utility>
#include <queue>
#include <vector>

// #include "config.hpp"

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
  const std::string type;
  const std::string message;

  const SourceLocation location;

  explicit InjaError(const std::string &type, const std::string &message)
      : std::runtime_error("[inja.exception." + type + "] " + message), type(type), message(message), location({0, 0}) {}

  explicit InjaError(const std::string &type, const std::string &message, SourceLocation location)
      : std::runtime_error("[inja.exception." + type + "] (at " + std::to_string(location.line) + ":" +
                           std::to_string(location.column) + ") " + message),
        type(type), message(message), location(location) {}
};

struct ParserError : public InjaError {
  explicit ParserError(const std::string &message, SourceLocation location) : InjaError("parser_error", message, location) {}
};

struct RenderError : public InjaError {
  explicit RenderError(const std::string &message, SourceLocation location) : InjaError("render_error", message, location) {}
};

struct FileError : public InjaError {
  explicit FileError(const std::string &message) : InjaError("file_error", message) {}
  explicit FileError(const std::string &message, SourceLocation location) : InjaError("file_error", message, location) {}
};

struct JsonError : public InjaError {
  explicit JsonError(const std::string &message, SourceLocation location) : InjaError("json_error", message, location) {}
};

} // namespace inja

#endif // INCLUDE_INJA_EXCEPTIONS_HPP_

// #include "function_storage.hpp"

// #include "lexer.hpp"
// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_LEXER_HPP_
#define INCLUDE_INJA_LEXER_HPP_

#include <cctype>
#include <locale>

// #include "config.hpp"

// #include "token.hpp"
// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_TOKEN_HPP_
#define INCLUDE_INJA_TOKEN_HPP_

#include <string>

// #include "string_view.hpp"


namespace inja {

/*!
 * \brief Helper-class for the inja Lexer.
 */
struct Token {
  enum class Kind {
    Text,
    ExpressionOpen,     // {{
    ExpressionClose,    // }}
    LineStatementOpen,  // ##
    LineStatementClose, // \n
    StatementOpen,      // {%
    StatementClose,     // %}
    CommentOpen,        // {#
    CommentClose,       // #}
    Id,                 // this, this.foo
    Number,             // 1, 2, -1, 5.2, -5.3
    String,             // "this"
    Plus,               // +
    Minus,              // -
    Times,              // *
    Slash,              // /
    Percent,            // %
    Power,              // ^
    Comma,              // ,
    Dot,                // .
    Colon,              // :
    LeftParen,          // (
    RightParen,         // )
    LeftBracket,        // [
    RightBracket,       // ]
    LeftBrace,          // {
    RightBrace,         // }
    Equal,              // ==
    NotEqual,           // !=
    GreaterThan,        // >
    GreaterEqual,       // >=
    LessThan,           // <
    LessEqual,          // <=
    Unknown,
    Eof,
  };

  Kind kind {Kind::Unknown};
  nonstd::string_view text;

  explicit constexpr Token() = default;
  explicit constexpr Token(Kind kind, nonstd::string_view text) : kind(kind), text(text) {}

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

} // namespace inja

#endif // INCLUDE_INJA_TOKEN_HPP_

// #include "utils.hpp"
// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_UTILS_HPP_
#define INCLUDE_INJA_UTILS_HPP_

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>

// #include "exceptions.hpp"

// #include "string_view.hpp"


namespace inja {

inline void open_file_or_throw(const std::string &path, std::ifstream &file) {
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try {
    file.open(path);
  } catch (const std::ios_base::failure & /*e*/) {
    throw FileError("failed accessing file at '" + path + "'");
  }
}

namespace string_view {
inline nonstd::string_view slice(nonstd::string_view view, size_t start, size_t end) {
  start = std::min(start, view.size());
  end = std::min(std::max(start, end), view.size());
  return view.substr(start, end - start);
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
} // namespace string_view

inline SourceLocation get_source_location(nonstd::string_view content, size_t pos) {
  // Get line and offset position (starts at 1:1)
  auto sliced = string_view::slice(content, 0, pos);
  std::size_t last_newline = sliced.rfind("\n");

  if (last_newline == nonstd::string_view::npos) {
    return {1, sliced.length() + 1};
  }

  // Count newlines
  size_t count_lines = 0;
  size_t search_start = 0;
  while (search_start <= sliced.size()) {
    search_start = sliced.find("\n", search_start) + 1;
    if (search_start == 0) {
      break;
    }
    count_lines += 1;
  }

  return {count_lines + 1, sliced.length() - last_newline};
}

} // namespace inja

#endif // INCLUDE_INJA_UTILS_HPP_


namespace inja {

/*!
 * \brief Class for lexing an inja Template.
 */
class Lexer {
  enum class State {
    Text,
    ExpressionStart,
    ExpressionStartForceLstrip,
    ExpressionBody,
    LineStart,
    LineBody,
    StatementStart,
    StatementStartNoLstrip,
    StatementStartForceLstrip,
    StatementBody,
    CommentStart,
    CommentBody,
  };

  enum class MinusState {
    Operator,
    Number,
  };

  const LexerConfig &config;

  State state;
  MinusState minus_state;
  nonstd::string_view m_in;
  size_t tok_start;
  size_t pos;

  Token scan_body(nonstd::string_view close, Token::Kind closeKind, nonstd::string_view close_trim = nonstd::string_view(), bool trim = false) {
  again:
    // skip whitespace (except for \n as it might be a close)
    if (tok_start >= m_in.size()) {
      return make_token(Token::Kind::Eof);
    }
    char ch = m_in[tok_start];
    if (ch == ' ' || ch == '\t' || ch == '\r') {
      tok_start += 1;
      goto again;
    }

    // check for close
    if (!close_trim.empty() && inja::string_view::starts_with(m_in.substr(tok_start), close_trim)) {
      state = State::Text;
      pos = tok_start + close_trim.size();
      Token tok = make_token(closeKind);
      skip_whitespaces_and_newlines();
      return tok;
    }

    if (inja::string_view::starts_with(m_in.substr(tok_start), close)) {
      state = State::Text;
      pos = tok_start + close.size();
      Token tok = make_token(closeKind);
      if (trim) {
        skip_whitespaces_and_first_newline();
      }
      return tok;
    }

    // skip \n
    if (ch == '\n') {
      tok_start += 1;
      goto again;
    }

    pos = tok_start + 1;
    if (std::isalpha(ch)) {
      minus_state = MinusState::Operator;
      return scan_id();
    }

    MinusState current_minus_state = minus_state;
    if (minus_state == MinusState::Operator) {
      minus_state = MinusState::Number;
    }

    switch (ch) {
    case '+':
      return make_token(Token::Kind::Plus);
    case '-':
      if (current_minus_state == MinusState::Operator) {
        return make_token(Token::Kind::Minus);
      }
      return scan_number();
    case '*':
      return make_token(Token::Kind::Times);
    case '/':
      return make_token(Token::Kind::Slash);
    case '^':
      return make_token(Token::Kind::Power);
    case '%':
      return make_token(Token::Kind::Percent);
    case '.':
      return make_token(Token::Kind::Dot);
    case ',':
      return make_token(Token::Kind::Comma);
    case ':':
      return make_token(Token::Kind::Colon);
    case '(':
      return make_token(Token::Kind::LeftParen);
    case ')':
      minus_state = MinusState::Operator;
      return make_token(Token::Kind::RightParen);
    case '[':
      return make_token(Token::Kind::LeftBracket);
    case ']':
      minus_state = MinusState::Operator;
      return make_token(Token::Kind::RightBracket);
    case '{':
      return make_token(Token::Kind::LeftBrace);
    case '}':
      minus_state = MinusState::Operator;
      return make_token(Token::Kind::RightBrace);
    case '>':
      if (pos < m_in.size() && m_in[pos] == '=') {
        pos += 1;
        return make_token(Token::Kind::GreaterEqual);
      }
      return make_token(Token::Kind::GreaterThan);
    case '<':
      if (pos < m_in.size() && m_in[pos] == '=') {
        pos += 1;
        return make_token(Token::Kind::LessEqual);
      }
      return make_token(Token::Kind::LessThan);
    case '=':
      if (pos < m_in.size() && m_in[pos] == '=') {
        pos += 1;
        return make_token(Token::Kind::Equal);
      }
      return make_token(Token::Kind::Unknown);
    case '!':
      if (pos < m_in.size() && m_in[pos] == '=') {
        pos += 1;
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
      minus_state = MinusState::Operator;
      return scan_number();
    case '_':
      minus_state = MinusState::Operator;
      return scan_id();
    default:
      return make_token(Token::Kind::Unknown);
    }
  }

  Token scan_id() {
    for (;;) {
      if (pos >= m_in.size()) {
        break;
      }
      char ch = m_in[pos];
      if (!std::isalnum(ch) && ch != '.' && ch != '/' && ch != '_' && ch != '-') {
        break;
      }
      pos += 1;
    }
    return make_token(Token::Kind::Id);
  }

  Token scan_number() {
    for (;;) {
      if (pos >= m_in.size()) {
        break;
      }
      char ch = m_in[pos];
      // be very permissive in lexer (we'll catch errors when conversion happens)
      if (!std::isdigit(ch) && ch != '.' && ch != 'e' && ch != 'E' && ch != '+' && ch != '-') {
        break;
      }
      pos += 1;
    }
    return make_token(Token::Kind::Number);
  }

  Token scan_string() {
    bool escape {false};
    for (;;) {
      if (pos >= m_in.size()) {
        break;
      }
      char ch = m_in[pos++];
      if (ch == '\\') {
        escape = true;
      } else if (!escape && ch == m_in[tok_start]) {
        break;
      } else {
        escape = false;
      }
    }
    return make_token(Token::Kind::String);
  }

  Token make_token(Token::Kind kind) const { return Token(kind, string_view::slice(m_in, tok_start, pos)); }

  void skip_whitespaces_and_newlines() {
    if (pos < m_in.size()) {
      while (pos < m_in.size() && (m_in[pos] == ' ' || m_in[pos] == '\t' || m_in[pos] == '\n' || m_in[pos] == '\r')) {
        pos += 1;
      }
    }
  }

  void skip_whitespaces_and_first_newline() {
    if (pos < m_in.size()) {
      while (pos < m_in.size() && (m_in[pos] == ' ' || m_in[pos] == '\t')) {
        pos += 1;
      }
    }

    if (pos < m_in.size()) {
      char ch = m_in[pos];
      if (ch == '\n') {
        pos += 1;
      } else if (ch == '\r') {
        pos += 1;
        if (pos < m_in.size() && m_in[pos] == '\n') {
          pos += 1;
        }
      }
    }
  }

  static nonstd::string_view clear_final_line_if_whitespace(nonstd::string_view text) {
    nonstd::string_view result = text;
    while (!result.empty()) {
      char ch = result.back();
      if (ch == ' ' || ch == '\t') {
        result.remove_suffix(1);
      } else if (ch == '\n' || ch == '\r') {
        break;
      } else {
        return text;
      }
    }
    return result;
  }

public:
  explicit Lexer(const LexerConfig &config) : config(config), state(State::Text), minus_state(MinusState::Number) {}

  SourceLocation current_position() const {
    return get_source_location(m_in, tok_start);
  }

  void start(nonstd::string_view input) {
    m_in = input;
    tok_start = 0;
    pos = 0;
    state = State::Text;
    minus_state = MinusState::Number;

    // Consume byte order mark (BOM) for UTF-8
    if (inja::string_view::starts_with(m_in, "\xEF\xBB\xBF")) {
      m_in = m_in.substr(3);
    }
  }

  Token scan() {
    tok_start = pos;

  again:
    if (tok_start >= m_in.size()) {
      return make_token(Token::Kind::Eof);
    }

    switch (state) {
    default:
    case State::Text: {
      // fast-scan to first open character
      size_t open_start = m_in.substr(pos).find_first_of(config.open_chars);
      if (open_start == nonstd::string_view::npos) {
        // didn't find open, return remaining text as text token
        pos = m_in.size();
        return make_token(Token::Kind::Text);
      }
      pos += open_start;

      // try to match one of the opening sequences, and get the close
      nonstd::string_view open_str = m_in.substr(pos);
      bool must_lstrip = false;
      if (inja::string_view::starts_with(open_str, config.expression_open)) {
        if (inja::string_view::starts_with(open_str, config.expression_open_force_lstrip)) {
          state = State::ExpressionStartForceLstrip;
          must_lstrip = true;
        } else {
          state = State::ExpressionStart;
        }
      } else if (inja::string_view::starts_with(open_str, config.statement_open)) {
        if (inja::string_view::starts_with(open_str, config.statement_open_no_lstrip)) {
          state = State::StatementStartNoLstrip;
        } else if (inja::string_view::starts_with(open_str, config.statement_open_force_lstrip )) {
          state = State::StatementStartForceLstrip;
          must_lstrip = true;
        } else {
          state = State::StatementStart;
          must_lstrip = config.lstrip_blocks;
        }
      } else if (inja::string_view::starts_with(open_str, config.comment_open)) {
        state = State::CommentStart;
        must_lstrip = config.lstrip_blocks;
      } else if ((pos == 0 || m_in[pos - 1] == '\n') && inja::string_view::starts_with(open_str, config.line_statement)) {
        state = State::LineStart;
      } else {
        pos += 1; // wasn't actually an opening sequence
        goto again;
      }

      nonstd::string_view text = string_view::slice(m_in, tok_start, pos);
      if (must_lstrip) {
        text = clear_final_line_if_whitespace(text);
      }

      if (text.empty()) {
        goto again; // don't generate empty token
      }
      return Token(Token::Kind::Text, text);
    }
    case State::ExpressionStart: {
      state = State::ExpressionBody;
      pos += config.expression_open.size();
      return make_token(Token::Kind::ExpressionOpen);
    }
    case State::ExpressionStartForceLstrip: {
      state = State::ExpressionBody;
      pos += config.expression_open_force_lstrip.size();
      return make_token(Token::Kind::ExpressionOpen);
    }
    case State::LineStart: {
      state = State::LineBody;
      pos += config.line_statement.size();
      return make_token(Token::Kind::LineStatementOpen);
    }
    case State::StatementStart: {
      state = State::StatementBody;
      pos += config.statement_open.size();
      return make_token(Token::Kind::StatementOpen);
    }
    case State::StatementStartNoLstrip: {
      state = State::StatementBody;
      pos += config.statement_open_no_lstrip.size();
      return make_token(Token::Kind::StatementOpen);
    }
    case State::StatementStartForceLstrip: {
      state = State::StatementBody;
      pos += config.statement_open_force_lstrip.size();
      return make_token(Token::Kind::StatementOpen);
    }
    case State::CommentStart: {
      state = State::CommentBody;
      pos += config.comment_open.size();
      return make_token(Token::Kind::CommentOpen);
    }
    case State::ExpressionBody:
      return scan_body(config.expression_close, Token::Kind::ExpressionClose, config.expression_close_force_rstrip);
    case State::LineBody:
      return scan_body("\n", Token::Kind::LineStatementClose);
    case State::StatementBody:
      return scan_body(config.statement_close, Token::Kind::StatementClose, config.statement_close_force_rstrip, config.trim_blocks);
    case State::CommentBody: {
      // fast-scan to comment close
      size_t end = m_in.substr(pos).find(config.comment_close);
      if (end == nonstd::string_view::npos) {
        pos = m_in.size();
        return make_token(Token::Kind::Eof);
      }
      // return the entire comment in the close token
      state = State::Text;
      pos += end + config.comment_close.size();
      Token tok = make_token(Token::Kind::CommentClose);
      if (config.trim_blocks) {
        skip_whitespaces_and_first_newline();
      }
      return tok;
    }
    }
  }

  const LexerConfig &get_config() const {
    return config;
  }
};

} // namespace inja

#endif // INCLUDE_INJA_LEXER_HPP_

// #include "node.hpp"
// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_NODE_HPP_
#define INCLUDE_INJA_NODE_HPP_

#include <string>
#include <utility>

#include <nlohmann/json.hpp>

// #include "function_storage.hpp"

// #include "string_view.hpp"



namespace inja {

class NodeVisitor;
class BlockNode;
class TextNode;
class ExpressionNode;
class LiteralNode;
class JsonNode;
class FunctionNode;
class ExpressionListNode;
class StatementNode;
class ForStatementNode;
class ForArrayStatementNode;
class ForObjectStatementNode;
class IfStatementNode;
class IncludeStatementNode;
class SetStatementNode;


class NodeVisitor {
public:
  virtual void visit(const BlockNode& node) = 0;
  virtual void visit(const TextNode& node) = 0;
  virtual void visit(const ExpressionNode& node) = 0;
  virtual void visit(const LiteralNode& node) = 0;
  virtual void visit(const JsonNode& node) = 0;
  virtual void visit(const FunctionNode& node) = 0;
  virtual void visit(const ExpressionListNode& node) = 0;
  virtual void visit(const StatementNode& node) = 0;
  virtual void visit(const ForStatementNode& node) = 0;
  virtual void visit(const ForArrayStatementNode& node) = 0;
  virtual void visit(const ForObjectStatementNode& node) = 0;
  virtual void visit(const IfStatementNode& node) = 0;
  virtual void visit(const IncludeStatementNode& node) = 0;
  virtual void visit(const SetStatementNode& node) = 0;
};

/*!
 * \brief Base node class for the abstract syntax tree (AST).
 */
class AstNode {
public:
  virtual void accept(NodeVisitor& v) const = 0;

  size_t pos;

  AstNode(size_t pos) : pos(pos) { }
  virtual ~AstNode() { };
};


class BlockNode : public AstNode {
public:
  std::vector<std::shared_ptr<AstNode>> nodes;

  explicit BlockNode() : AstNode(0) {}

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class TextNode : public AstNode {
public:
  const size_t length;

  explicit TextNode(size_t pos, size_t length): AstNode(pos), length(length) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class ExpressionNode : public AstNode {
public:
  explicit ExpressionNode(size_t pos) : AstNode(pos) {}

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class LiteralNode : public ExpressionNode {
public:
  const nlohmann::json value;

  explicit LiteralNode(const nlohmann::json& value, size_t pos) : ExpressionNode(pos), value(value) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class JsonNode : public ExpressionNode {
public:
  const std::string name;
  const json::json_pointer ptr;

  static std::string convert_dot_to_json_ptr(nonstd::string_view ptr_name) {
    std::string result;
    do {
      nonstd::string_view part;
      std::tie(part, ptr_name) = string_view::split(ptr_name, '.');
      result.push_back('/');
      result.append(part.begin(), part.end());
    } while (!ptr_name.empty());
    return result;
  }

  explicit JsonNode(nonstd::string_view ptr_name, size_t pos) : ExpressionNode(pos), name(ptr_name), ptr(json::json_pointer(convert_dot_to_json_ptr(ptr_name))) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class FunctionNode : public ExpressionNode {
  using Op = FunctionStorage::Operation;

public:
  enum class Associativity {
    Left,
    Right,
  };

  unsigned int precedence;
  Associativity associativity;

  Op operation;

  std::string name;
  int number_args; // Should also be negative -> -1 for unknown number
  CallbackFunction callback;

  explicit FunctionNode(nonstd::string_view name, size_t pos) : ExpressionNode(pos), precedence(8), associativity(Associativity::Left), operation(Op::Callback), name(name), number_args(1) { }
  explicit FunctionNode(Op operation, size_t pos) : ExpressionNode(pos), operation(operation), number_args(1) {
    switch (operation) {
      case Op::Not: {
        precedence = 4;
        associativity = Associativity::Left;
      } break;
      case Op::And: {
        precedence = 1;
        associativity = Associativity::Left;
      } break;
      case Op::Or: {
        precedence = 1;
        associativity = Associativity::Left;
      } break;
      case Op::In: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::Equal: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::NotEqual: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::Greater: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::GreaterEqual: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::Less: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::LessEqual: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::Add: {
        precedence = 3;
        associativity = Associativity::Left;
      } break;
      case Op::Subtract: {
        precedence = 3;
        associativity = Associativity::Left;
      } break;
      case Op::Multiplication: {
        precedence = 4;
        associativity = Associativity::Left;
      } break;
      case Op::Division: {
        precedence = 4;
        associativity = Associativity::Left;
      } break;
      case Op::Power: {
        precedence = 5;
        associativity = Associativity::Right;
      } break;
      case Op::Modulo: {
        precedence = 4;
        associativity = Associativity::Left;
      } break;
      case Op::AtId: {
        precedence = 8;
        associativity = Associativity::Left;
      } break;
      default: {
        precedence = 1;
        associativity = Associativity::Left;
      }
    }
  }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class ExpressionListNode : public AstNode {
public:
  std::vector<std::shared_ptr<ExpressionNode>> rpn_output;

  explicit ExpressionListNode() : AstNode(0) { }
  explicit ExpressionListNode(size_t pos) : AstNode(pos) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class StatementNode : public AstNode {
public:
  StatementNode(size_t pos) : AstNode(pos) { }

  virtual void accept(NodeVisitor& v) const = 0;
};

class ForStatementNode : public StatementNode {
public:
  ExpressionListNode condition;
  BlockNode body;
  BlockNode *const parent;

  ForStatementNode(BlockNode *const parent, size_t pos) : StatementNode(pos), parent(parent) { }

  virtual void accept(NodeVisitor& v) const = 0;
};

class ForArrayStatementNode : public ForStatementNode {
public:
  const std::string value;

  explicit ForArrayStatementNode(const std::string& value, BlockNode *const parent, size_t pos) : ForStatementNode(parent, pos), value(value) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class ForObjectStatementNode : public ForStatementNode {
public:
  const std::string key;
  const std::string value;

  explicit ForObjectStatementNode(const std::string& key, const std::string& value, BlockNode *const parent, size_t pos) : ForStatementNode(parent, pos), key(key), value(value) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class IfStatementNode : public StatementNode {
public:
  ExpressionListNode condition;
  BlockNode true_statement;
  BlockNode false_statement;
  BlockNode *const parent;

  const bool is_nested;
  bool has_false_statement {false};

  explicit IfStatementNode(BlockNode *const parent, size_t pos) : StatementNode(pos), parent(parent), is_nested(false) { }
  explicit IfStatementNode(bool is_nested, BlockNode *const parent, size_t pos) : StatementNode(pos), parent(parent), is_nested(is_nested) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class IncludeStatementNode : public StatementNode {
public:
  const std::string file;

  explicit IncludeStatementNode(const std::string& file, size_t pos) : StatementNode(pos), file(file) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  };
};

class SetStatementNode : public StatementNode {
public:
  const std::string key;
  ExpressionListNode expression;

  explicit SetStatementNode(const std::string& key, size_t pos) : StatementNode(pos), key(key) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  };
};

} // namespace inja

#endif // INCLUDE_INJA_NODE_HPP_

// #include "template.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_TEMPLATE_HPP_
#define INCLUDE_INJA_TEMPLATE_HPP_

#include <map>
#include <memory>
#include <string>
#include <vector>

// #include "node.hpp"

// #include "statistics.hpp"
// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_STATISTICS_HPP_
#define INCLUDE_INJA_STATISTICS_HPP_

// #include "node.hpp"



namespace inja {

/*!
 * \brief A class for counting statistics on a Template.
 */
class StatisticsVisitor : public NodeVisitor {
  void visit(const BlockNode& node) {
    for (auto& n : node.nodes) {
      n->accept(*this);
    }
  }

  void visit(const TextNode&) { }
  void visit(const ExpressionNode&) { }
  void visit(const LiteralNode&) { }

  void visit(const JsonNode&) {
    variable_counter += 1;
  }

  void visit(const FunctionNode&) { }

  void visit(const ExpressionListNode& node) {
    for (auto& n : node.rpn_output) {
      n->accept(*this);
    }
  }

  void visit(const StatementNode&) { }
  void visit(const ForStatementNode&) { }

  void visit(const ForArrayStatementNode& node) {
    node.condition.accept(*this);
    node.body.accept(*this);
  }

  void visit(const ForObjectStatementNode& node) {
    node.condition.accept(*this);
    node.body.accept(*this);
  }

  void visit(const IfStatementNode& node) {
    node.condition.accept(*this);
    node.true_statement.accept(*this);
    node.false_statement.accept(*this);
  }

  void visit(const IncludeStatementNode&) { }

  void visit(const SetStatementNode&) { }

public:
  unsigned int variable_counter;

  explicit StatisticsVisitor() : variable_counter(0) { }
};

} // namespace inja

#endif // INCLUDE_INJA_STATISTICS_HPP_



namespace inja {

/*!
 * \brief The main inja Template.
 */
struct Template {
  BlockNode root;
  std::string content;

  explicit Template() { }
  explicit Template(const std::string& content): content(content) { }

  /// Return number of variables (total number, not distinct ones) in the template
  int count_variables() {
    auto statistic_visitor = StatisticsVisitor();
    root.accept(statistic_visitor);
    return statistic_visitor.variable_counter;
  }
};

using TemplateStorage = std::map<std::string, Template>;

} // namespace inja

#endif // INCLUDE_INJA_TEMPLATE_HPP_

// #include "token.hpp"

// #include "utils.hpp"


#include <nlohmann/json.hpp>

namespace inja {

/*!
 * \brief Class for parsing an inja Template.
 */
class Parser {
  const ParserConfig &config;

  Lexer lexer;
  TemplateStorage &template_storage;
  const FunctionStorage &function_storage;

  Token tok, peek_tok;
  bool have_peek_tok {false};

  size_t current_paren_level {0};
  size_t current_bracket_level {0};
  size_t current_brace_level {0};

  nonstd::string_view json_literal_start;

  BlockNode *current_block {nullptr};
  ExpressionListNode *current_expression_list {nullptr};
  std::stack<std::pair<FunctionNode*, size_t>> function_stack;

  std::stack<std::shared_ptr<FunctionNode>> operator_stack;
  std::stack<IfStatementNode*> if_statement_stack;
  std::stack<ForStatementNode*> for_statement_stack;

  void throw_parser_error(const std::string &message) {
    throw ParserError(message, lexer.current_position());
  }

  void get_next_token() {
    if (have_peek_tok) {
      tok = peek_tok;
      have_peek_tok = false;
    } else {
      tok = lexer.scan();
    }
  }

  void get_peek_token() {
    if (!have_peek_tok) {
      peek_tok = lexer.scan();
      have_peek_tok = true;
    }
  }

  void add_json_literal(const char* content_ptr) {
    nonstd::string_view json_text(json_literal_start.data(), tok.text.data() - json_literal_start.data() + tok.text.size());
    current_expression_list->rpn_output.emplace_back(std::make_shared<LiteralNode>(json::parse(json_text), json_text.data() - content_ptr));
  }

  bool parse_expression(Template &tmpl, Token::Kind closing) {
    while (tok.kind != closing && tok.kind != Token::Kind::Eof) {
      // Literals
      switch (tok.kind) {
      case Token::Kind::String: {
        if (current_brace_level == 0 && current_bracket_level == 0) {
          json_literal_start = tok.text;
          add_json_literal(tmpl.content.c_str());
        }

      } break;
      case Token::Kind::Number: {
        if (current_brace_level == 0 && current_bracket_level == 0) {
          json_literal_start = tok.text;
          add_json_literal(tmpl.content.c_str());
        }

      } break;
      case Token::Kind::LeftBracket: {
        if (current_brace_level == 0 && current_bracket_level == 0) {
          json_literal_start = tok.text;
        }
        current_bracket_level += 1;

      } break;
      case Token::Kind::LeftBrace: {
        if (current_brace_level == 0 && current_bracket_level == 0) {
          json_literal_start = tok.text;
        }
        current_brace_level += 1;

      } break;
      case Token::Kind::RightBracket: {
        if (current_bracket_level == 0) {
          throw_parser_error("unexpected ']'");
        }

        current_bracket_level -= 1;
        if (current_brace_level == 0 && current_bracket_level == 0) {
          add_json_literal(tmpl.content.c_str());
        }

      } break;
      case Token::Kind::RightBrace: {
        if (current_brace_level == 0) {
          throw_parser_error("unexpected '}'");
        }

        current_brace_level -= 1;
        if (current_brace_level == 0 && current_bracket_level == 0) {
          add_json_literal(tmpl.content.c_str());
        }

      } break;
      case Token::Kind::Id: {
        get_peek_token();

        // Json Literal
        if (tok.text == static_cast<decltype(tok.text)>("true") || tok.text == static_cast<decltype(tok.text)>("false") || tok.text == static_cast<decltype(tok.text)>("null")) {
          if (current_brace_level == 0 && current_bracket_level == 0) {
            json_literal_start = tok.text;
            add_json_literal(tmpl.content.c_str());
          }

	      // Operator
        } else if (tok.text == "and" || tok.text == "or" || tok.text == "in" || tok.text == "not") {
          goto parse_operator;

        // Functions
        } else if (peek_tok.kind == Token::Kind::LeftParen) {
          operator_stack.emplace(std::make_shared<FunctionNode>(static_cast<std::string>(tok.text), tok.text.data() - tmpl.content.c_str()));
          function_stack.emplace(operator_stack.top().get(), current_paren_level);

        // Variables
        } else {
          current_expression_list->rpn_output.emplace_back(std::make_shared<JsonNode>(static_cast<std::string>(tok.text), tok.text.data() - tmpl.content.c_str()));
        }

      // Operators
      } break;
      case Token::Kind::Equal:
      case Token::Kind::NotEqual:
      case Token::Kind::GreaterThan:
      case Token::Kind::GreaterEqual:
      case Token::Kind::LessThan:
      case Token::Kind::LessEqual:
      case Token::Kind::Plus:
      case Token::Kind::Minus:
      case Token::Kind::Times:
      case Token::Kind::Slash:
      case Token::Kind::Power:
      case Token::Kind::Percent:
      case Token::Kind::Dot: {

  parse_operator:
        FunctionStorage::Operation operation;
        switch (tok.kind) {
        case Token::Kind::Id: {
          if (tok.text == "and") {
            operation = FunctionStorage::Operation::And;
          } else if (tok.text == "or") {
            operation = FunctionStorage::Operation::Or;
          } else if (tok.text == "in") {
            operation = FunctionStorage::Operation::In;
          } else if (tok.text == "not") {
            operation = FunctionStorage::Operation::Not;
          } else {
            throw_parser_error("unknown operator in parser.");
          }
        } break;
        case Token::Kind::Equal: {
          operation = FunctionStorage::Operation::Equal;
        } break;
        case Token::Kind::NotEqual: {
          operation = FunctionStorage::Operation::NotEqual;
        } break;
        case Token::Kind::GreaterThan: {
          operation = FunctionStorage::Operation::Greater;
        } break;
        case Token::Kind::GreaterEqual: {
          operation = FunctionStorage::Operation::GreaterEqual;
        } break;
        case Token::Kind::LessThan: {
          operation = FunctionStorage::Operation::Less;
        } break;
        case Token::Kind::LessEqual: {
          operation = FunctionStorage::Operation::LessEqual;
        } break;
        case Token::Kind::Plus: {
          operation = FunctionStorage::Operation::Add;
        } break;
        case Token::Kind::Minus: {
          operation = FunctionStorage::Operation::Subtract;
        } break;
        case Token::Kind::Times: {
          operation = FunctionStorage::Operation::Multiplication;
        } break;
        case Token::Kind::Slash: {
          operation = FunctionStorage::Operation::Division;
        } break;
        case Token::Kind::Power: {
          operation = FunctionStorage::Operation::Power;
        } break;
        case Token::Kind::Percent: {
          operation = FunctionStorage::Operation::Modulo;
        } break;
        case Token::Kind::Dot: {
          operation = FunctionStorage::Operation::AtId;
        } break;
        default: {
          throw_parser_error("unknown operator in parser.");
        }
        }
        auto function_node = std::make_shared<FunctionNode>(operation, tok.text.data() - tmpl.content.c_str());

        while (!operator_stack.empty() && ((operator_stack.top()->precedence > function_node->precedence) || (operator_stack.top()->precedence == function_node->precedence && function_node->associativity == FunctionNode::Associativity::Left)) && (operator_stack.top()->operation != FunctionStorage::Operation::ParenLeft)) {
          current_expression_list->rpn_output.emplace_back(operator_stack.top());
          operator_stack.pop();
        }

        operator_stack.emplace(function_node);

      } break;
      case Token::Kind::Comma: {
        if (current_brace_level == 0 && current_bracket_level == 0) {
          if (function_stack.empty()) {
            throw_parser_error("unexpected ','");
          }

          function_stack.top().first->number_args += 1;
        }

      } break;
      case Token::Kind::Colon: {
        if (current_brace_level == 0 && current_bracket_level == 0) {
          throw_parser_error("unexpected ':'");
        }

      } break;
      case Token::Kind::LeftParen: {
        current_paren_level += 1;
        operator_stack.emplace(std::make_shared<FunctionNode>(FunctionStorage::Operation::ParenLeft, tok.text.data() - tmpl.content.c_str()));

        get_peek_token();
        if (peek_tok.kind == Token::Kind::RightParen) {
          if (!function_stack.empty() && function_stack.top().second == current_paren_level - 1) {
            function_stack.top().first->number_args = 0;
          }
        }

      } break;
      case Token::Kind::RightParen: {
        current_paren_level -= 1;
        while (!operator_stack.empty() && operator_stack.top()->operation != FunctionStorage::Operation::ParenLeft) {
          current_expression_list->rpn_output.emplace_back(operator_stack.top());
          operator_stack.pop();
        }

        if (!operator_stack.empty() && operator_stack.top()->operation == FunctionStorage::Operation::ParenLeft) {
          operator_stack.pop();
        }

        if (!function_stack.empty() && function_stack.top().second == current_paren_level) {
          auto func = function_stack.top().first;
          auto function_data = function_storage.find_function(func->name, func->number_args);
          if (function_data.operation == FunctionStorage::Operation::None) {
            throw_parser_error("unknown function " + func->name);
          }
          func->operation = function_data.operation;
          if (function_data.operation == FunctionStorage::Operation::Callback) {
            func->callback = function_data.callback;
          }

          if (operator_stack.empty()) {
            throw_parser_error("internal error at function " + func->name);
          }

          current_expression_list->rpn_output.emplace_back(operator_stack.top());
          operator_stack.pop();
          function_stack.pop();
        }
      }
      default:
        break;
      }

      get_next_token();
    }

    while (!operator_stack.empty()) {
      current_expression_list->rpn_output.emplace_back(operator_stack.top());
      operator_stack.pop();
    }

    return true;
  }

  bool parse_statement(Template &tmpl, Token::Kind closing, nonstd::string_view path) {
    if (tok.kind != Token::Kind::Id) {
      return false;
    }

    if (tok.text == static_cast<decltype(tok.text)>("if")) {
      get_next_token();

      auto if_statement_node = std::make_shared<IfStatementNode>(current_block, tok.text.data() - tmpl.content.c_str());
      current_block->nodes.emplace_back(if_statement_node);
      if_statement_stack.emplace(if_statement_node.get());
      current_block = &if_statement_node->true_statement;
      current_expression_list = &if_statement_node->condition;

      if (!parse_expression(tmpl, closing)) {
        return false;
      }

    } else if (tok.text == static_cast<decltype(tok.text)>("else")) {
      if (if_statement_stack.empty()) {
        throw_parser_error("else without matching if");
      }
      auto &if_statement_data = if_statement_stack.top();
      get_next_token();

      if_statement_data->has_false_statement = true;
      current_block = &if_statement_data->false_statement;

      // Chained else if
      if (tok.kind == Token::Kind::Id && tok.text == static_cast<decltype(tok.text)>("if")) {
        get_next_token();

        auto if_statement_node = std::make_shared<IfStatementNode>(true, current_block, tok.text.data() - tmpl.content.c_str());
        current_block->nodes.emplace_back(if_statement_node);
        if_statement_stack.emplace(if_statement_node.get());
        current_block = &if_statement_node->true_statement;
        current_expression_list = &if_statement_node->condition;

        if (!parse_expression(tmpl, closing)) {
          return false;
        }
      }

    } else if (tok.text == static_cast<decltype(tok.text)>("endif")) {
      if (if_statement_stack.empty()) {
        throw_parser_error("endif without matching if");
      }

      // Nested if statements
      while (if_statement_stack.top()->is_nested) {
        if_statement_stack.pop();
      }

      auto &if_statement_data = if_statement_stack.top();
      get_next_token();

      current_block = if_statement_data->parent;
      if_statement_stack.pop();

    } else if (tok.text == static_cast<decltype(tok.text)>("for")) {
      get_next_token();

      // options: for a in arr; for a, b in obj
      if (tok.kind != Token::Kind::Id) {
        throw_parser_error("expected id, got '" + tok.describe() + "'");
      }

      Token value_token = tok;
      get_next_token();

      // Object type
      std::shared_ptr<ForStatementNode> for_statement_node;
      if (tok.kind == Token::Kind::Comma) {
        get_next_token();
        if (tok.kind != Token::Kind::Id) {
          throw_parser_error("expected id, got '" + tok.describe() + "'");
        }

        Token key_token = std::move(value_token);
        value_token = tok;
        get_next_token();

        for_statement_node = std::make_shared<ForObjectStatementNode>(static_cast<std::string>(key_token.text), static_cast<std::string>(value_token.text), current_block, tok.text.data() - tmpl.content.c_str());

      // Array type
      } else {
        for_statement_node = std::make_shared<ForArrayStatementNode>(static_cast<std::string>(value_token.text), current_block, tok.text.data() - tmpl.content.c_str());
      }

      current_block->nodes.emplace_back(for_statement_node);
      for_statement_stack.emplace(for_statement_node.get());
      current_block = &for_statement_node->body;
      current_expression_list = &for_statement_node->condition;

      if (tok.kind != Token::Kind::Id || tok.text != static_cast<decltype(tok.text)>("in")) {
        throw_parser_error("expected 'in', got '" + tok.describe() + "'");
      }
      get_next_token();

      if (!parse_expression(tmpl, closing)) {
        return false;
      }

    } else if (tok.text == static_cast<decltype(tok.text)>("endfor")) {
      if (for_statement_stack.empty()) {
        throw_parser_error("endfor without matching for");
      }

      auto &for_statement_data = for_statement_stack.top();
      get_next_token();

      current_block = for_statement_data->parent;
      for_statement_stack.pop();

    } else if (tok.text == static_cast<decltype(tok.text)>("include")) {
      get_next_token();

      if (tok.kind != Token::Kind::String) {
        throw_parser_error("expected string, got '" + tok.describe() + "'");
      }

      // Build the relative path
      json json_name = json::parse(tok.text);
      std::string pathname = static_cast<std::string>(path);
      pathname += json_name.get_ref<const std::string &>();
      if (pathname.compare(0, 2, "./") == 0) {
        pathname.erase(0, 2);
      }
      // sys::path::remove_dots(pathname, true, sys::path::Style::posix);

      if(config.include_scope_limit)
      {
#ifdef _WIN32
        if(pathname.find(":\\") != pathname.npos || pathname.find("..") != pathname.npos)
          throw FileError("access denied when trying to include '" + pathname + "': out of scope");
#else
        if(pathname.find("/") == 0 || pathname.find("..") != pathname.npos)
          throw FileError("access denied when trying to include '" + pathname + "': out of scope");
#endif // _WIN32
        if(config.include_scope.size() && pathname.find(config.include_scope) != 0)
          throw FileError("access denied when trying to include '" + pathname + "': out of scope");
      }

      if (config.search_included_templates_in_files && template_storage.find(pathname) == template_storage.end()) {
        auto include_template = Template(load_file(pathname));
        template_storage.emplace(pathname, include_template);
        parse_into_template(template_storage[pathname], pathname);
      }

      current_block->nodes.emplace_back(std::make_shared<IncludeStatementNode>(pathname, tok.text.data() - tmpl.content.c_str()));

      get_next_token();

    } else if (tok.text == static_cast<decltype(tok.text)>("set")) {
      get_next_token();

      if (tok.kind != Token::Kind::Id) {
        throw_parser_error("expected variable name, got '" + tok.describe() + "'");
      }

      std::string key = static_cast<std::string>(tok.text);
      get_next_token();

      auto set_statement_node = std::make_shared<SetStatementNode>(key, tok.text.data() - tmpl.content.c_str());
      current_block->nodes.emplace_back(set_statement_node);
      current_expression_list = &set_statement_node->expression;

      if (tok.text != static_cast<decltype(tok.text)>("=")) {
        throw_parser_error("expected '=', got '" + tok.describe() + "'");
      }
      get_next_token();

      if (!parse_expression(tmpl, closing)) {
        return false;
      }

    } else {
      return false;
    }
    return true;
  }

  void parse_into(Template &tmpl, nonstd::string_view path) {
    lexer.start(tmpl.content);
    current_block = &tmpl.root;

    for (;;) {
      get_next_token();
      switch (tok.kind) {
      case Token::Kind::Eof: {
        if (!if_statement_stack.empty()) {
          throw_parser_error("unmatched if");
        }
        if (!for_statement_stack.empty()) {
          throw_parser_error("unmatched for");
        }
      } return;
      case Token::Kind::Text: {
        current_block->nodes.emplace_back(std::make_shared<TextNode>(tok.text.data() - tmpl.content.c_str(), tok.text.size()));
      } break;
      case Token::Kind::StatementOpen: {
        get_next_token();
        if (!parse_statement(tmpl, Token::Kind::StatementClose, path)) {
          throw_parser_error("expected statement, got '" + tok.describe() + "'");
        }
        if (tok.kind != Token::Kind::StatementClose) {
          throw_parser_error("expected statement close, got '" + tok.describe() + "'");
        }
      } break;
      case Token::Kind::LineStatementOpen: {
        get_next_token();
        if (!parse_statement(tmpl, Token::Kind::LineStatementClose, path)) {
          throw_parser_error("expected statement, got '" + tok.describe() + "'");
        }
        if (tok.kind != Token::Kind::LineStatementClose && tok.kind != Token::Kind::Eof) {
          throw_parser_error("expected line statement close, got '" + tok.describe() + "'");
        }
      } break;
      case Token::Kind::ExpressionOpen: {
        get_next_token();

        auto expression_list_node = std::make_shared<ExpressionListNode>(tok.text.data() - tmpl.content.c_str());
        current_block->nodes.emplace_back(expression_list_node);
        current_expression_list = expression_list_node.get();

        if (!parse_expression(tmpl, Token::Kind::ExpressionClose)) {
          throw_parser_error("expected expression, got '" + tok.describe() + "'");
        }

        if (tok.kind != Token::Kind::ExpressionClose) {
          throw_parser_error("expected expression close, got '" + tok.describe() + "'");
        }
      } break;
      case Token::Kind::CommentOpen: {
        get_next_token();
        if (tok.kind != Token::Kind::CommentClose) {
          throw_parser_error("expected comment close, got '" + tok.describe() + "'");
        }
      } break;
      default: {
        throw_parser_error("unexpected token '" + tok.describe() + "'");
      } break;
      }
    }
  }


public:
  explicit Parser(const ParserConfig &parser_config, const LexerConfig &lexer_config,
                  TemplateStorage &template_storage, const FunctionStorage &function_storage)
      : config(parser_config), lexer(lexer_config), template_storage(template_storage), function_storage(function_storage) { }

  Template parse(nonstd::string_view input, nonstd::string_view path) {
    auto result = Template(static_cast<std::string>(input));
    parse_into(result, path);
    return result;
  }

  Template parse(nonstd::string_view input) {
    return parse(input, "./");
  }

  void parse_into_template(Template& tmpl, nonstd::string_view filename) {
    nonstd::string_view path = filename.substr(0, filename.find_last_of("/\\") + 1);

    // StringRef path = sys::path::parent_path(filename);
    auto sub_parser = Parser(config, lexer.get_config(), template_storage, function_storage);
    sub_parser.parse_into(tmpl, path);
  }

  std::string load_file(nonstd::string_view filename) {
    std::ifstream file;
    open_file_or_throw(static_cast<std::string>(filename), file);
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return text;
  }
};

} // namespace inja

#endif // INCLUDE_INJA_PARSER_HPP_

// #include "renderer.hpp"
// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_RENDERER_HPP_
#define INCLUDE_INJA_RENDERER_HPP_

#include <algorithm>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

// #include "config.hpp"

// #include "exceptions.hpp"

// #include "node.hpp"

// #include "template.hpp"

// #include "utils.hpp"


namespace inja {

/*!
 * \brief Class for rendering a Template with data.
 */
class Renderer : public NodeVisitor  {
  using Op = FunctionStorage::Operation;

  const RenderConfig config;
  const Template *current_template;
  const TemplateStorage &template_storage;
  const FunctionStorage &function_storage;

  const json *json_input;
  std::ostream *output_stream;

  json json_additional_data;
  json* current_loop_data = &json_additional_data["loop"];

  std::vector<std::shared_ptr<json>> json_tmp_stack;
  std::stack<const json*> json_eval_stack;
  std::stack<const JsonNode*> not_found_stack;

  bool truthy(const json* data) const {
    if (data->is_boolean()) {
      return data->get<bool>();
    } else if (data->is_number()) {
      return (*data != 0);
    } else if (data->is_null()) {
      return false;
    }
    return !data->empty();
  }

  void print_json(const std::shared_ptr<json> value) {
    if (value->is_string()) {
      *output_stream << value->get_ref<const json::string_t&>();
    } else if (value->is_number_integer()) {
      *output_stream << value->get<const json::number_integer_t>();
    } else if (value->is_null()) {
    } else {
      *output_stream << value->dump();
    }
  }

  const std::shared_ptr<json> eval_expression_list(const ExpressionListNode& expression_list) {
    for (auto& expression : expression_list.rpn_output) {
      expression->accept(*this);
    }

    if (json_eval_stack.empty()) {
      throw_renderer_error("empty expression", expression_list);
    } else if (json_eval_stack.size() != 1) {
      throw_renderer_error("malformed expression", expression_list);
    }

    auto result = json_eval_stack.top();
    json_eval_stack.pop();

    if (!result) {
      if (not_found_stack.empty()) {
        throw_renderer_error("expression could not be evaluated", expression_list);
      }

      auto node = not_found_stack.top();
      not_found_stack.pop();

      throw_renderer_error("variable '" + static_cast<std::string>(node->name) + "' not found", *node);
    }
    return std::make_shared<json>(*result);
  }

  void throw_renderer_error(const std::string &message, const AstNode& node) {
    SourceLocation loc = get_source_location(current_template->content, node.pos);
    throw RenderError(message, loc);
  }

  template<size_t N, bool throw_not_found=true>
  std::array<const json*, N> get_arguments(const AstNode& node) {
    if (json_eval_stack.size() < N) {
      throw_renderer_error("function needs " + std::to_string(N) + " variables, but has only found " + std::to_string(json_eval_stack.size()), node);
    }

    std::array<const json*, N> result;
    for (size_t i = 0; i < N; i += 1) {
      result[N - i - 1] = json_eval_stack.top();
      json_eval_stack.pop();

      if (!result[N - i - 1]) {
        auto json_node = not_found_stack.top();
        not_found_stack.pop();

        if (throw_not_found) {
          throw_renderer_error("variable '" + static_cast<std::string>(json_node->name) + "' not found", *json_node);
        }
      }
    }
    return result;
  }

  template<bool throw_not_found=true>
  Arguments get_argument_vector(size_t N, const AstNode& node) {
    Arguments result {N};
    for (size_t i = 0; i < N; i += 1) {
      result[N - i - 1] = json_eval_stack.top();
      json_eval_stack.pop();

      if (!result[N - i - 1]) {
        auto json_node = not_found_stack.top();
        not_found_stack.pop();

        if (throw_not_found) {
          throw_renderer_error("variable '" + static_cast<std::string>(json_node->name) + "' not found", *json_node);
        }
      }
    }
    return result;
  }

  void visit(const BlockNode& node) {
    for (auto& n : node.nodes) {
      n->accept(*this);
    }
  }

  void visit(const TextNode& node) {
    output_stream->write(current_template->content.c_str() + node.pos, node.length);
  }

  void visit(const ExpressionNode&) { }

  void visit(const LiteralNode& node) {
    json_eval_stack.push(&node.value);
  }

  void visit(const JsonNode& node) {
    if (json_additional_data.contains(node.ptr)) {
      json_eval_stack.push(&(json_additional_data[node.ptr]));
    
    } else if (json_input->contains(node.ptr)) {
      json_eval_stack.push(&(*json_input)[node.ptr]);
    
    } else {
      // Try to evaluate as a no-argument callback
      auto function_data = function_storage.find_function(node.name, 0);
      if (function_data.operation == FunctionStorage::Operation::Callback) {
        Arguments empty_args {};
        auto value = std::make_shared<json>(function_data.callback(empty_args));
        json_tmp_stack.push_back(value);
        json_eval_stack.push(value.get());

      } else {
        json_eval_stack.push(nullptr);
        not_found_stack.emplace(&node);
      }
    }
  }

  void visit(const FunctionNode& node) {
    std::shared_ptr<json> result_ptr;

    switch (node.operation) {
    case Op::Not: {
      auto args = get_arguments<1>(node);
      result_ptr = std::make_shared<json>(!truthy(args[0]));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::And: {
      auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(truthy(args[0]) && truthy(args[1]));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Or: {
      auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(truthy(args[0]) || truthy(args[1]));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::In: {
      auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(std::find(args[1]->begin(), args[1]->end(), *args[0]) != args[1]->end());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Equal: {
      auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] == *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::NotEqual: {
      auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] != *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Greater: {
      auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] > *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::GreaterEqual: {
      auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] >= *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Less: {
      auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] < *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::LessEqual: {
      auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(*args[0] <= *args[1]);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Add: {
      auto args = get_arguments<2>(node);
      if (args[0]->is_string() && args[1]->is_string()) {
        result_ptr = std::make_shared<json>(args[0]->get_ref<const std::string&>() + args[1]->get_ref<const std::string&>());
        json_tmp_stack.push_back(result_ptr);
      } else if (args[0]->is_number_integer() && args[1]->is_number_integer()) {
        result_ptr = std::make_shared<json>(args[0]->get<int>() + args[1]->get<int>());
        json_tmp_stack.push_back(result_ptr);
      } else {
        result_ptr = std::make_shared<json>(args[0]->get<double>() + args[1]->get<double>());
        json_tmp_stack.push_back(result_ptr);
      }
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Subtract: {
      auto args = get_arguments<2>(node);
      if (args[0]->is_number_integer() && args[1]->is_number_integer()) {
        result_ptr = std::make_shared<json>(args[0]->get<int>() - args[1]->get<int>());
        json_tmp_stack.push_back(result_ptr);
      } else {
        result_ptr = std::make_shared<json>(args[0]->get<double>() - args[1]->get<double>());
        json_tmp_stack.push_back(result_ptr);
      }
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Multiplication: {
      auto args = get_arguments<2>(node);
      if (args[0]->is_number_integer() && args[1]->is_number_integer()) {
        result_ptr = std::make_shared<json>(args[0]->get<int>() * args[1]->get<int>());
        json_tmp_stack.push_back(result_ptr);
      } else {
        result_ptr = std::make_shared<json>(args[0]->get<double>() * args[1]->get<double>());
        json_tmp_stack.push_back(result_ptr);
      }
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Division: {
      auto args = get_arguments<2>(node);
      if (args[1]->get<double>() == 0) {
        throw_renderer_error("division by zero", node);
      }
      result_ptr = std::make_shared<json>(args[0]->get<double>() / args[1]->get<double>());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Power: {
      auto args = get_arguments<2>(node);
      if (args[0]->is_number_integer() && args[1]->get<int>() >= 0) {
        int result = std::pow(args[0]->get<int>(), args[1]->get<int>());
        result_ptr = std::make_shared<json>(std::move(result));
        json_tmp_stack.push_back(result_ptr);
      } else {
        double result = std::pow(args[0]->get<double>(), args[1]->get<int>());
        result_ptr = std::make_shared<json>(std::move(result));
        json_tmp_stack.push_back(result_ptr);
      }
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Modulo: {
      auto args = get_arguments<2>(node);
      result_ptr = std::make_shared<json>(args[0]->get<int>() % args[1]->get<int>());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::AtId: {
      json_eval_stack.pop(); // Pop id nullptr
      auto container = get_arguments<1, false>(node)[0];
      if (not_found_stack.empty()) {
        throw_renderer_error("could not find element with given name", node);
      }
      auto id_node = not_found_stack.top();
      not_found_stack.pop();
      json_eval_stack.push(&container->at(id_node->name));
    } break;
    case Op::At: {
      auto args = get_arguments<2>(node);
      json_eval_stack.push(&args[0]->at(args[1]->get<int>()));
    } break;
    case Op::Default: {
      auto default_arg = get_arguments<1>(node)[0];
      auto test_arg = get_arguments<1, false>(node)[0];
      json_eval_stack.push(test_arg ? test_arg : default_arg);
    } break;
    case Op::DivisibleBy: {
      auto args = get_arguments<2>(node);
      int divisor = args[1]->get<int>();
      result_ptr = std::make_shared<json>((divisor != 0) && (args[0]->get<int>() % divisor == 0));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Even: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->get<int>() % 2 == 0);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Exists: {
      auto &&name = get_arguments<1>(node)[0]->get_ref<const std::string &>();
      result_ptr = std::make_shared<json>(json_input->contains(json::json_pointer(JsonNode::convert_dot_to_json_ptr(name))));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::ExistsInObject: {
      auto args = get_arguments<2>(node);
      auto &&name = args[1]->get_ref<const std::string &>();
      result_ptr = std::make_shared<json>(args[0]->find(name) != args[0]->end());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::First: {
      auto result = &get_arguments<1>(node)[0]->front();
      json_eval_stack.push(result);
    } break;
    case Op::Float: {
      result_ptr = std::make_shared<json>(std::stod(get_arguments<1>(node)[0]->get_ref<const std::string &>()));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Int: {
      result_ptr = std::make_shared<json>(std::stoi(get_arguments<1>(node)[0]->get_ref<const std::string &>()));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Last: {
      auto result = &get_arguments<1>(node)[0]->back();
      json_eval_stack.push(result);
    } break;
    case Op::Length: {
      auto val = get_arguments<1>(node)[0];
      if (val->is_string()) {
        result_ptr = std::make_shared<json>(val->get_ref<const std::string &>().length());
      } else {
        result_ptr = std::make_shared<json>(val->size());
      }
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Lower: {
      std::string result = get_arguments<1>(node)[0]->get<std::string>();
      std::transform(result.begin(), result.end(), result.begin(), ::tolower);
      result_ptr = std::make_shared<json>(std::move(result));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Max: {
      auto args = get_arguments<1>(node);
      auto result = std::max_element(args[0]->begin(), args[0]->end());
      json_eval_stack.push(&(*result));
    } break;
    case Op::Min: {
      auto args = get_arguments<1>(node);
      auto result = std::min_element(args[0]->begin(), args[0]->end());
      json_eval_stack.push(&(*result));
    } break;
    case Op::Odd: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->get<int>() % 2 != 0);
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Range: {
      std::vector<int> result(get_arguments<1>(node)[0]->get<int>());
      std::iota(result.begin(), result.end(), 0);
      result_ptr = std::make_shared<json>(std::move(result));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Round: {
      auto args = get_arguments<2>(node);
      int precision = args[1]->get<int>();
      double result = std::round(args[0]->get<double>() * std::pow(10.0, precision)) / std::pow(10.0, precision);
      result_ptr = std::make_shared<json>(std::move(result));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Sort: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->get<std::vector<json>>());
      std::sort(result_ptr->begin(), result_ptr->end());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Upper: {
      std::string result = get_arguments<1>(node)[0]->get<std::string>();
      std::transform(result.begin(), result.end(), result.begin(), ::toupper);
      result_ptr = std::make_shared<json>(std::move(result));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsBoolean: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_boolean());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsNumber: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_number());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsInteger: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_number_integer());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsFloat: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_number_float());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsObject: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_object());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsArray: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_array());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::IsString: {
      result_ptr = std::make_shared<json>(get_arguments<1>(node)[0]->is_string());
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::Callback: {
      auto args = get_argument_vector(node.number_args, node);
      result_ptr = std::make_shared<json>(node.callback(args));
      json_tmp_stack.push_back(result_ptr);
      json_eval_stack.push(result_ptr.get());
    } break;
    case Op::ParenLeft:
    case Op::ParenRight:
    case Op::None:
      break;
    }
  }

  void visit(const ExpressionListNode& node) {
    print_json(eval_expression_list(node));
  }

  void visit(const StatementNode&) { }

  void visit(const ForStatementNode&) { }

  void visit(const ForArrayStatementNode& node) {
    auto result = eval_expression_list(node.condition);
    if (!result->is_array()) {
      throw_renderer_error("object must be an array", node);
    }

    if (!current_loop_data->empty()) {
      auto tmp = *current_loop_data; // Because of clang-3
      (*current_loop_data)["parent"] = std::move(tmp);
    }

    size_t index = 0;
    (*current_loop_data)["is_first"] = true;
    (*current_loop_data)["is_last"] = (result->size() <= 1);
    for (auto it = result->begin(); it != result->end(); ++it) {
      json_additional_data[static_cast<std::string>(node.value)] = *it;

      (*current_loop_data)["index"] = index;
      (*current_loop_data)["index1"] = index + 1;
      if (index == 1) {
        (*current_loop_data)["is_first"] = false;
      }
      if (index == result->size() - 1) {
        (*current_loop_data)["is_last"] = true;
      }

      node.body.accept(*this);
      ++index;
    }

    json_additional_data[static_cast<std::string>(node.value)].clear();
    if (!(*current_loop_data)["parent"].empty()) {
      auto tmp = (*current_loop_data)["parent"];
      *current_loop_data = std::move(tmp);
    } else {
      current_loop_data = &json_additional_data["loop"];
    }
  }

  void visit(const ForObjectStatementNode& node) {
    auto result = eval_expression_list(node.condition);
    if (!result->is_object()) {
      throw_renderer_error("object must be an object", node);
    }

    if (!current_loop_data->empty()) {
      (*current_loop_data)["parent"] = std::move(*current_loop_data);
    }

    size_t index = 0;
    (*current_loop_data)["is_first"] = true;
    (*current_loop_data)["is_last"] = (result->size() <= 1);
    for (auto it = result->begin(); it != result->end(); ++it) {
      json_additional_data[static_cast<std::string>(node.key)] = it.key();
      json_additional_data[static_cast<std::string>(node.value)] = it.value();

      (*current_loop_data)["index"] = index;
      (*current_loop_data)["index1"] = index + 1;
      if (index == 1) {
        (*current_loop_data)["is_first"] = false;
      }
      if (index == result->size() - 1) {
        (*current_loop_data)["is_last"] = true;
      }

      node.body.accept(*this);
      ++index;
    }

    json_additional_data[static_cast<std::string>(node.key)].clear();
    json_additional_data[static_cast<std::string>(node.value)].clear();
    if (!(*current_loop_data)["parent"].empty()) {
      *current_loop_data = std::move((*current_loop_data)["parent"]);
    } else {
      current_loop_data = &json_additional_data["loop"];
    }
  }

  void visit(const IfStatementNode& node) {
    auto result = eval_expression_list(node.condition);
    if (truthy(result.get())) {
      node.true_statement.accept(*this);
    } else if (node.has_false_statement) {
      node.false_statement.accept(*this);
    }
  }

  void visit(const IncludeStatementNode& node) {
    auto sub_renderer = Renderer(config, template_storage, function_storage);
    auto included_template_it = template_storage.find(node.file);

    if (included_template_it != template_storage.end()) {
      sub_renderer.render_to(*output_stream, included_template_it->second, *json_input, &json_additional_data);
    } else if (config.throw_at_missing_includes) {
      throw_renderer_error("include '" + node.file + "' not found", node);
    }
  }

  void visit(const SetStatementNode& node) {
    json_additional_data[node.key] = *eval_expression_list(node.expression);
  }

public:
  Renderer(const RenderConfig& config, const TemplateStorage &template_storage, const FunctionStorage &function_storage)
      : config(config), template_storage(template_storage), function_storage(function_storage) { }

  void render_to(std::ostream &os, const Template &tmpl, const json &data, json *loop_data = nullptr) {
    output_stream = &os;
    current_template = &tmpl;
    json_input = &data;
    if (loop_data) {
      json_additional_data = *loop_data;
      current_loop_data = &json_additional_data["loop"];
    }

    current_template->root.accept(*this);

    json_tmp_stack.clear();
  }
};

} // namespace inja

#endif // INCLUDE_INJA_RENDERER_HPP_

// #include "string_view.hpp"

// #include "template.hpp"

// #include "utils.hpp"


namespace inja {

using json = nlohmann::json;

/*!
 * \brief Class for changing the configuration.
 */
class Environment {
  std::string input_path;
  std::string output_path;

  LexerConfig lexer_config;
  ParserConfig parser_config;
  RenderConfig render_config;

  FunctionStorage function_storage;
  TemplateStorage template_storage;

public:
  Environment() : Environment("") {}

  explicit Environment(const std::string &global_path) : input_path(global_path), output_path(global_path) {}

  Environment(const std::string &input_path, const std::string &output_path)
      : input_path(input_path), output_path(output_path) {}

  /// Sets the opener and closer for template statements
  void set_statement(const std::string &open, const std::string &close) {
    lexer_config.statement_open = open;
    lexer_config.statement_open_no_lstrip = open + "+";
    lexer_config.statement_open_force_lstrip = open + "-";
    lexer_config.statement_close = close;
    lexer_config.statement_close_force_rstrip = "-" + close;
    lexer_config.update_open_chars();
  }

  /// Sets the opener for template line statements
  void set_line_statement(const std::string &open) {
    lexer_config.line_statement = open;
    lexer_config.update_open_chars();
  }

  /// Sets the opener and closer for template expressions
  void set_expression(const std::string &open, const std::string &close) {
    lexer_config.expression_open = open;
    lexer_config.expression_open_force_lstrip = open + "-";
    lexer_config.expression_close = close;
    lexer_config.expression_close_force_rstrip = "-" + close;
    lexer_config.update_open_chars();
  }

  /// Sets the opener and closer for template comments
  void set_comment(const std::string &open, const std::string &close) {
    lexer_config.comment_open = open;
    lexer_config.comment_close = close;
    lexer_config.update_open_chars();
  }

  /// Sets whether to remove the first newline after a block
  void set_trim_blocks(bool trim_blocks) {
    lexer_config.trim_blocks = trim_blocks;
  }

  /// Sets whether to strip the spaces and tabs from the start of a line to a block
  void set_lstrip_blocks(bool lstrip_blocks) {
    lexer_config.lstrip_blocks = lstrip_blocks;
  }

  /// Sets the element notation syntax
  void set_search_included_templates_in_files(bool search_in_files) {
    parser_config.search_included_templates_in_files = search_in_files;
  }

  /// Sets whether a missing include will throw an error
  void set_throw_at_missing_includes(bool will_throw) {
    render_config.throw_at_missing_includes = will_throw;
  }

  Template parse(nonstd::string_view input) {
    Parser parser(parser_config, lexer_config, template_storage, function_storage);
    return parser.parse(input);
  }

  Template parse_template(const std::string &filename) {
    Parser parser(parser_config, lexer_config, template_storage, function_storage);
    auto result = Template(parser.load_file(input_path + static_cast<std::string>(filename)));
    parser.parse_into_template(result, input_path + static_cast<std::string>(filename));
    return result;
  }

  Template parse_file(const std::string &filename) {
    return parse_template(filename);
  }

  std::string render(nonstd::string_view input, const json &data) { return render(parse(input), data); }

  std::string render(const Template &tmpl, const json &data) {
    std::stringstream os;
    render_to(os, tmpl, data);
    return os.str();
  }

  std::string render_file(const std::string &filename, const json &data) {
    return render(parse_template(filename), data);
  }

  std::string render_file_with_json_file(const std::string &filename, const std::string &filename_data) {
    const json data = load_json(filename_data);
    return render_file(filename, data);
  }

  void write(const std::string &filename, const json &data, const std::string &filename_out) {
    std::ofstream file(output_path + filename_out);
    file << render_file(filename, data);
    file.close();
  }

  void write(const Template &temp, const json &data, const std::string &filename_out) {
    std::ofstream file(output_path + filename_out);
    file << render(temp, data);
    file.close();
  }

  void write_with_json_file(const std::string &filename, const std::string &filename_data,
                            const std::string &filename_out) {
    const json data = load_json(filename_data);
    write(filename, data, filename_out);
  }

  void write_with_json_file(const Template &temp, const std::string &filename_data, const std::string &filename_out) {
    const json data = load_json(filename_data);
    write(temp, data, filename_out);
  }

  std::ostream &render_to(std::ostream &os, const Template &tmpl, const json &data) {
    Renderer(render_config, template_storage, function_storage).render_to(os, tmpl, data);
    return os;
  }

  std::string load_file(const std::string &filename) {
    Parser parser(parser_config, lexer_config, template_storage, function_storage);
    return parser.load_file(input_path + filename);
  }

  json load_json(const std::string &filename) {
    std::ifstream file;
    open_file_or_throw(input_path + filename, file);
    json j;
    file >> j;
    return j;
  }

  /*!
  @brief Adds a variadic callback
  */
  void add_callback(const std::string &name, const CallbackFunction &callback) {
    add_callback(name, -1, callback);
  }

  /*!
  @brief Adds a variadic void callback
  */
  void add_void_callback(const std::string &name, const VoidCallbackFunction &callback) {
    add_void_callback(name, -1, callback);
  }

  /*!
  @brief Adds a callback with given number or arguments
  */
  void add_callback(const std::string &name, int num_args, const CallbackFunction &callback) {
    function_storage.add_callback(name, num_args, callback);
  }

  /*!
  @brief Adds a void callback with given number or arguments
  */
  void add_void_callback(const std::string &name, int num_args, const VoidCallbackFunction &callback) {
    function_storage.add_callback(name, num_args, [callback](Arguments& args) { callback(args); return json(); });
  }

  /** Includes a template with a given name into the environment.
   * Then, a template can be rendered in another template using the
   * include "<name>" syntax.
   */
  void include_template(const std::string &name, const Template &tmpl) {
    template_storage[name] = tmpl;
  }
};

/*!
@brief render with default settings to a string
*/
inline std::string render(nonstd::string_view input, const json &data) {
  return Environment().render(input, data);
}

/*!
@brief render with default settings to the given output stream
*/
inline void render_to(std::ostream &os, nonstd::string_view input, const json &data) {
  Environment env;
  env.render_to(os, env.parse(input), data);
}

} // namespace inja

#endif // INCLUDE_INJA_ENVIRONMENT_HPP_

// #include "exceptions.hpp"

// #include "parser.hpp"

// #include "renderer.hpp"

// #include "string_view.hpp"

// #include "template.hpp"


#endif // INCLUDE_INJA_INJA_HPP_
