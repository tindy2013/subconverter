/* *****************************************************************************
 * ******************* C++ wrapper for PCRE2 Library ****************************
 * *****************************************************************************
 *            Copyright (c) Md. Jahidul Hamid
 *
 * -----------------------------------------------------------------------------
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * The names of its contributors may not be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Disclaimer:
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *     AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *     IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *     ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *     LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *     CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *     SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *     INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *     ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 * */

/** @file jpcre2.hpp
 * @brief Main header file for JPCRE2 library to be included by programs that uses its functionalities.
 * It includes the `pcre2.h` header, therefore you shouldn't include `pcre2.h`, neither should you define `PCRE2_CODE_UNIT_WIDTH` before including
 * `jpcre2.hpp`.
 * If your `pcre2.h` header is not in standard include paths, you may include `pcre2.h` with correct path before including `jpcre2.hpp`
 * manually. In this case you will have to define `PCRE2_CODE_UNIT_WIDTH` before including `pcre2.h`.
 * Make sure to link required PCRE2 libraries when compiling.
 *
 * @author [Md Jahidul Hamid](https://github.com/neurobin)
 */

#ifndef JPCRE2_HPP
#define JPCRE2_HPP

#ifndef PCRE2_CODE_UNIT_WIDTH

///@def PCRE2_CODE_UNIT_WIDTH
///This macro does not have any significance in JPCRE2 context.
///It is defined as 0 by default. Defining it before including jpcre2.hpp
///will override the default (discouraged as it will make it harder for you to detect problems),
///but still it will have no effect in a JPCRE2 perspective.
///Defining it with an invalid value will yield to compile error.
#define PCRE2_CODE_UNIT_WIDTH 0
#endif

//previous inclusion of pcre2.h will be respected and we won't try to include it twice.
//Thus one can pre-include pcre2.h from an arbitrary/non-standard path.
#ifndef PCRE2_MAJOR
    #include <pcre2.h>  // pcre2 header
#endif
#include <string>       // std::string, std::wstring
#include <vector>       // std::vector
#include <map>          // std::map
#include <cstdio>       // std::fprintf
#include <climits>      // CHAR_BIT
#include <cstdlib>      // std::abort()

#if __cplusplus >= 201103L || _MSVC_LANG >= 201103L
    #define JPCRE2_USE_MINIMUM_CXX_11 1
    #include <utility>
    #ifndef JPCRE2_USE_FUNCTION_POINTER_CALLBACK
        #include <functional>   // std::function
    #endif
#endif
#if __cplusplus >= 201703L || _MSVC_LANG >= 201703L
    #define JPCRE2_USE_MINIMUM_CXX_17 1
    #include <optional>
#else
    #ifdef JPCRE2_UNSET_CAPTURES_NULL
        #error JPCRE2_UNSET_CAPTURES_NULL requires C++17
    #endif
#endif

#define JPCRE2_UNUSED(x) ((void)(x))
#if defined(NDEBUG) || defined(JPCRE2_NDEBUG)
    #define JPCRE2_ASSERT(cond, msg) ((void)0)
    #define JPCRE2_VECTOR_DATA_ASSERT(cond, name) ((void)0)
#else
    #define JPCRE2_ASSERT(cond, msg) jpcre2::jassert(cond, msg, __FILE__, __LINE__)
    #define JPCRE2_VECTOR_DATA_ASSERT(cond, name) jpcre2::_jvassert(cond, name, __FILE__, __LINE__)
#endif

// In Windows, Windows.h defines ERROR macro
// It conflicts with our jpcre2::ERROR namespace
#ifdef ERROR
#undef ERROR
#endif


/** @namespace jpcre2
 *  Top level namespace of JPCRE2.
 *
 *  All functions, classes/structs, constants, enums that are provided by JPCRE2 belong to this namespace while
 *  **PCRE2** structs, functions, constants remain outside of its scope.
 *
 *  If you want to use any PCRE2 functions or constants,
 *  remember that they are in the global scope and should be used as such.
 */
namespace jpcre2 {


///Define for JPCRE2 version.
///It can be used to support changes in different versions of the lib.
#define JPCRE2_VERSION 103201L

/** @namespace jpcre2::INFO
 *  Namespace to provide information about JPCRE2 library itself.
 *  Contains constant Strings with version info.
 */
namespace INFO {
    static const char NAME[] = "JPCRE2";               ///< Name of the project
    static const char FULL_VERSION[] = "10.32.01";     ///< Full version string
    static const char VERSION_GENRE[] = "10";          ///< Generation, depends on original PCRE2 version
    static const char VERSION_MAJOR[] = "32";          ///< Major version, updated when API change is made
    static const char VERSION_MINOR[] = "01";          ///< Minor version, includes bug fix or minor feature upgrade
    static const char VERSION_PRE_RELEASE[] = "";      ///< Alpha or beta (testing) release version
}


typedef PCRE2_SIZE SIZE_T;                          ///< Used for match count and vector size
typedef uint32_t Uint;                              ///< Used for options (bitwise operation)
typedef uint8_t Ush;                                ///< 8 bit unsigned integer.
typedef std::vector<SIZE_T> VecOff;                 ///< vector of size_t.
typedef std::vector<Uint> VecOpt;                   ///< vector for Uint option values.

/// @namespace jpcre2::ERROR
/// Namespace for error codes.
namespace ERROR {
    /** Error numbers for JPCRE2.
     *  JPCRE2 error numbers are positive integers while
     *  PCRE2 error numbers are negative integers.
     */
    enum {
        INVALID_MODIFIER        = 2,  ///< Invalid modifier was detected
        INSUFFICIENT_OVECTOR    = 3   ///< Ovector was not big enough during a match
    };
}


/** These constants provide JPCRE2 options.
 */
enum {
    NONE                    = 0x0000000u,           ///< Option 0 (zero)
    FIND_ALL                = 0x0000002u,           ///< Find all during match (global match)
    JIT_COMPILE             = 0x0000004u            ///< Perform JIT compilation for optimization
};


//enableif and is_same implementation
template<bool B, typename T = void>
struct EnableIf{};
template<typename T>
struct EnableIf<true, T>{typedef T Type;};

template<typename T1, typename T2>
struct IsSame{ static const bool value = false; };
template<typename T>
struct IsSame<T,T>{ static const bool value = true; };


///JPCRE2 assert function.
///Aborts with an error message if condition fails.
///@param cond boolean condition
///@param msg message (std::string)
///@param f file where jassert was called.
///@param line line number where jassert was called.
static inline void jassert(bool cond, const char* msg, const char* f, size_t line){
    if(!cond) {
        std::fprintf(stderr,"\n\tE: AssertionFailure\n%s\nAssertion failed in file: %s\t at line: %u\n", msg, f, (unsigned)line);
        std::abort();
    }
}

static inline void _jvassert(bool cond, char const * name, const char* f, size_t line){
    jassert(cond, (std::string("ValueError: \n\
    Required data vector of type ")+std::string(name)+" is empty.\n\
    Your MatchEvaluator callback function is not\n\
    compatible with existing data!!\n\
    You are trying to use a vector that does not\n\
    have any match data. Either call nreplace() or replace()\n\
    with true or perform a match with appropriate\n\
    callback function. For more details, refer to\n\
    the doc in MatchEvaluator section.").c_str(), f, line);
}

static inline std::string _tostdstring(unsigned x){
    char buf[128];
    int written = std::snprintf(buf, 128, "%u", x);
    return (written > 0) ? std::string(buf, buf + written) : std::string();
}


////////////////////////// The following are type and function mappings from PCRE2 interface to JPCRE2 interface /////////////////////////

//forward declaration

template<Ush BS> struct Pcre2Type;
template<Ush BS> struct Pcre2Func;

//PCRE2 types
//These templated types will be used in place of actual types
template<Ush BS> struct Pcre2Type {};

template<> struct Pcre2Type<8>{
    //typedefs used
    typedef PCRE2_UCHAR8 Pcre2Uchar;
    typedef PCRE2_SPTR8 Pcre2Sptr;
    typedef pcre2_code_8 Pcre2Code;
    typedef pcre2_compile_context_8 CompileContext;
    typedef pcre2_match_data_8 MatchData;
    typedef pcre2_general_context_8 GeneralContext;
    typedef pcre2_match_context_8 MatchContext;
    typedef pcre2_jit_callback_8 JitCallback;
    typedef pcre2_jit_stack_8 JitStack;
};

template<> struct Pcre2Type<16>{
    //typedefs used
    typedef PCRE2_UCHAR16 Pcre2Uchar;
    typedef PCRE2_SPTR16 Pcre2Sptr;
    typedef pcre2_code_16 Pcre2Code;
    typedef pcre2_compile_context_16 CompileContext;
    typedef pcre2_match_data_16 MatchData;
    typedef pcre2_general_context_16 GeneralContext;
    typedef pcre2_match_context_16 MatchContext;
    typedef pcre2_jit_callback_16 JitCallback;
    typedef pcre2_jit_stack_16 JitStack;
};

template<> struct Pcre2Type<32>{
    //typedefs used
    typedef PCRE2_UCHAR32 Pcre2Uchar;
    typedef PCRE2_SPTR32 Pcre2Sptr;
    typedef pcre2_code_32 Pcre2Code;
    typedef pcre2_compile_context_32 CompileContext;
    typedef pcre2_match_data_32 MatchData;
    typedef pcre2_general_context_32 GeneralContext;
    typedef pcre2_match_context_32 MatchContext;
    typedef pcre2_jit_callback_32 JitCallback;
    typedef pcre2_jit_stack_32 JitStack;
};

//wrappers for PCRE2 functions
template<Ush BS> struct Pcre2Func{};

//8-bit version
template<> struct Pcre2Func<8> {
    static Pcre2Type<8>::CompileContext* compile_context_create(Pcre2Type<8>::GeneralContext *gcontext){
        return pcre2_compile_context_create_8(gcontext);
    }
    static void compile_context_free(Pcre2Type<8>::CompileContext *ccontext){
        pcre2_compile_context_free_8(ccontext);
    }
    static Pcre2Type<8>::CompileContext* compile_context_copy(Pcre2Type<8>::CompileContext* ccontext){
    return pcre2_compile_context_copy_8(ccontext);
    }
    static const unsigned char * maketables(Pcre2Type<8>::GeneralContext* gcontext){
        return pcre2_maketables_8(gcontext);
    }
    static int set_character_tables(Pcre2Type<8>::CompileContext * ccontext, const unsigned char * table){
        return pcre2_set_character_tables_8(ccontext, table);
    }
    static Pcre2Type<8>::Pcre2Code * compile(Pcre2Type<8>::Pcre2Sptr pattern,
                                     PCRE2_SIZE length,
                                     uint32_t options,
                                     int *errorcode,
                                     PCRE2_SIZE *erroroffset,
                                     Pcre2Type<8>::CompileContext *ccontext){
        return pcre2_compile_8(pattern, length, options, errorcode, erroroffset, ccontext);
    }
    static int jit_compile(Pcre2Type<8>::Pcre2Code *code, uint32_t options){
        return pcre2_jit_compile_8(code, options);
    }
    static int substitute( const Pcre2Type<8>::Pcre2Code *code,
                    Pcre2Type<8>::Pcre2Sptr subject,
                    PCRE2_SIZE length,
                    PCRE2_SIZE startoffset,
                    uint32_t options,
                    Pcre2Type<8>::MatchData *match_data,
                    Pcre2Type<8>::MatchContext *mcontext,
                    Pcre2Type<8>::Pcre2Sptr replacement,
                    PCRE2_SIZE rlength,
                    Pcre2Type<8>::Pcre2Uchar *outputbuffer,
                    PCRE2_SIZE *outlengthptr){
        return pcre2_substitute_8( code, subject, length, startoffset, options, match_data,
                                   mcontext, replacement, rlength, outputbuffer, outlengthptr);
    }
    //~ static int substring_get_bynumber(Pcre2Type<8>::MatchData *match_data,
                                        //~ uint32_t number,
                                        //~ Pcre2Type<8>::Pcre2Uchar **bufferptr,
                                        //~ PCRE2_SIZE *bufflen){
        //~ return pcre2_substring_get_bynumber_8(match_data, number, bufferptr, bufflen);
    //~ }
    //~ static int substring_get_byname(Pcre2Type<8>::MatchData *match_data,
                                        //~ Pcre2Type<8>::Pcre2Sptr name,
                                        //~ Pcre2Type<8>::Pcre2Uchar **bufferptr,
                                        //~ PCRE2_SIZE *bufflen){
        //~ return pcre2_substring_get_byname_8(match_data, name, bufferptr, bufflen);
    //~ }
    //~ static void substring_free(Pcre2Type<8>::Pcre2Uchar *buffer){
        //~ pcre2_substring_free_8(buffer);
    //~ }
    //~ static Pcre2Type<8>::Pcre2Code * code_copy(const Pcre2Type<8>::Pcre2Code *code){
        //~ return pcre2_code_copy_8(code);
    //~ }
    static void code_free(Pcre2Type<8>::Pcre2Code *code){
        pcre2_code_free_8(code);
    }
    static int get_error_message(  int errorcode,
                            Pcre2Type<8>::Pcre2Uchar *buffer,
                            PCRE2_SIZE bufflen){
        return pcre2_get_error_message_8(errorcode, buffer, bufflen);
    }
    static Pcre2Type<8>::MatchData * match_data_create_from_pattern(
                              const Pcre2Type<8>::Pcre2Code *code,
                              Pcre2Type<8>::GeneralContext *gcontext){
        return pcre2_match_data_create_from_pattern_8(code, gcontext);

    }
    static int match(  const Pcre2Type<8>::Pcre2Code *code,
                            Pcre2Type<8>::Pcre2Sptr subject,
                            PCRE2_SIZE length,
                            PCRE2_SIZE startoffset,
                            uint32_t options,
                            Pcre2Type<8>::MatchData *match_data,
                            Pcre2Type<8>::MatchContext *mcontext){
        return pcre2_match_8(code, subject, length, startoffset, options, match_data, mcontext);
    }
    static void match_data_free(Pcre2Type<8>::MatchData *match_data){
        pcre2_match_data_free_8(match_data);
    }
    static PCRE2_SIZE * get_ovector_pointer(Pcre2Type<8>::MatchData *match_data){
        return pcre2_get_ovector_pointer_8(match_data);
    }
    static int pattern_info(const Pcre2Type<8>::Pcre2Code *code, uint32_t what, void *where){
        return pcre2_pattern_info_8(code, what, where);
    }
    static int set_newline(Pcre2Type<8>::CompileContext *ccontext, uint32_t value){
        return pcre2_set_newline_8(ccontext, value);
    }
    //~ static void jit_stack_assign(Pcre2Type<8>::MatchContext *mcontext,
                                 //~ Pcre2Type<8>::JitCallback callback_function,
                                 //~ void *callback_data){
        //~ pcre2_jit_stack_assign_8(mcontext, callback_function, callback_data);
    //~ }
    //~ static Pcre2Type<8>::JitStack *jit_stack_create(PCRE2_SIZE startsize, PCRE2_SIZE maxsize,
                                                             //~ Pcre2Type<8>::GeneralContext *gcontext){
    //~ return pcre2_jit_stack_create_8(startsize, maxsize, gcontext);
    //~ }
    //~ static void jit_stack_free(Pcre2Type<8>::JitStack *jit_stack){
        //~ pcre2_jit_stack_free_8(jit_stack);
    //~ }
    //~ static void jit_free_unused_memory(Pcre2Type<8>::GeneralContext *gcontext){
        //~ pcre2_jit_free_unused_memory_8(gcontext);
    //~ }
    //~ static Pcre2Type<8>::MatchContext *match_context_create(Pcre2Type<8>::GeneralContext *gcontext){
        //~ return pcre2_match_context_create_8(gcontext);
    //~ }
    //~ static Pcre2Type<8>::MatchContext *match_context_copy(Pcre2Type<8>::MatchContext *mcontext){
        //~ return pcre2_match_context_copy_8(mcontext);
    //~ }
    //~ static void match_context_free(Pcre2Type<8>::MatchContext *mcontext){
        //~ pcre2_match_context_free_8(mcontext);
    //~ }
    static uint32_t get_ovector_count(Pcre2Type<8>::MatchData *match_data){
        return pcre2_get_ovector_count_8(match_data);
    }
};

//16-bit version
template<> struct Pcre2Func<16> {
    static Pcre2Type<16>::CompileContext* compile_context_create(Pcre2Type<16>::GeneralContext *gcontext){
        return pcre2_compile_context_create_16(gcontext);
    }
    static void compile_context_free(Pcre2Type<16>::CompileContext *ccontext){
        pcre2_compile_context_free_16(ccontext);
    }
    static Pcre2Type<16>::CompileContext* compile_context_copy(Pcre2Type<16>::CompileContext* ccontext){
    return pcre2_compile_context_copy_16(ccontext);
    }
    static const unsigned char * maketables(Pcre2Type<16>::GeneralContext* gcontext){
        return pcre2_maketables_16(gcontext);
    }
    static int set_character_tables(Pcre2Type<16>::CompileContext * ccontext, const unsigned char * table){
        return pcre2_set_character_tables_16(ccontext, table);
    }
    static Pcre2Type<16>::Pcre2Code * compile(Pcre2Type<16>::Pcre2Sptr pattern,
                                     PCRE2_SIZE length,
                                     uint32_t options,
                                     int *errorcode,
                                     PCRE2_SIZE *erroroffset,
                                     Pcre2Type<16>::CompileContext *ccontext){
        return pcre2_compile_16(pattern, length, options, errorcode, erroroffset, ccontext);
    }
    static int jit_compile(Pcre2Type<16>::Pcre2Code *code, uint32_t options){
        return pcre2_jit_compile_16(code, options);
    }
    static int substitute( const Pcre2Type<16>::Pcre2Code *code,
                    Pcre2Type<16>::Pcre2Sptr subject,
                    PCRE2_SIZE length,
                    PCRE2_SIZE startoffset,
                    uint32_t options,
                    Pcre2Type<16>::MatchData *match_data,
                    Pcre2Type<16>::MatchContext *mcontext,
                    Pcre2Type<16>::Pcre2Sptr replacement,
                    PCRE2_SIZE rlength,
                    Pcre2Type<16>::Pcre2Uchar *outputbuffer,
                    PCRE2_SIZE *outlengthptr){
        return pcre2_substitute_16( code, subject, length, startoffset, options, match_data,
                                   mcontext, replacement, rlength, outputbuffer, outlengthptr);
    }
    //~ static int substring_get_bynumber(Pcre2Type<16>::MatchData *match_data,
                                        //~ uint32_t number,
                                        //~ Pcre2Type<16>::Pcre2Uchar **bufferptr,
                                        //~ PCRE2_SIZE *bufflen){
        //~ return pcre2_substring_get_bynumber_16(match_data, number, bufferptr, bufflen);
    //~ }
    //~ static int substring_get_byname(Pcre2Type<16>::MatchData *match_data,
                                        //~ Pcre2Type<16>::Pcre2Sptr name,
                                        //~ Pcre2Type<16>::Pcre2Uchar **bufferptr,
                                        //~ PCRE2_SIZE *bufflen){
        //~ return pcre2_substring_get_byname_16(match_data, name, bufferptr, bufflen);
    //~ }
    //~ static void substring_free(Pcre2Type<16>::Pcre2Uchar *buffer){
        //~ pcre2_substring_free_16(buffer);
    //~ }
    //~ static Pcre2Type<16>::Pcre2Code * code_copy(const Pcre2Type<16>::Pcre2Code *code){
        //~ return pcre2_code_copy_16(code);
    //~ }
    static void code_free(Pcre2Type<16>::Pcre2Code *code){
        pcre2_code_free_16(code);
    }
    static int get_error_message(  int errorcode,
                            Pcre2Type<16>::Pcre2Uchar *buffer,
                            PCRE2_SIZE bufflen){
        return pcre2_get_error_message_16(errorcode, buffer, bufflen);
    }
    static Pcre2Type<16>::MatchData * match_data_create_from_pattern(
                              const Pcre2Type<16>::Pcre2Code *code,
                              Pcre2Type<16>::GeneralContext *gcontext){
        return pcre2_match_data_create_from_pattern_16(code, gcontext);

    }
    static int match(  const Pcre2Type<16>::Pcre2Code *code,
                            Pcre2Type<16>::Pcre2Sptr subject,
                            PCRE2_SIZE length,
                            PCRE2_SIZE startoffset,
                            uint32_t options,
                            Pcre2Type<16>::MatchData *match_data,
                            Pcre2Type<16>::MatchContext *mcontext){
        return pcre2_match_16(code, subject, length, startoffset, options, match_data, mcontext);
    }
    static void match_data_free(Pcre2Type<16>::MatchData *match_data){
        pcre2_match_data_free_16(match_data);
    }
    static PCRE2_SIZE * get_ovector_pointer(Pcre2Type<16>::MatchData *match_data){
        return pcre2_get_ovector_pointer_16(match_data);
    }
    static int pattern_info(const Pcre2Type<16>::Pcre2Code *code, uint32_t what, void *where){
        return pcre2_pattern_info_16(code, what, where);
    }
    static int set_newline(Pcre2Type<16>::CompileContext *ccontext, uint32_t value){
        return pcre2_set_newline_16(ccontext, value);
    }
    //~ static void jit_stack_assign(Pcre2Type<16>::MatchContext *mcontext,
                                 //~ Pcre2Type<16>::JitCallback callback_function,
                                 //~ void *callback_data){
        //~ pcre2_jit_stack_assign_16(mcontext, callback_function, callback_data);
    //~ }
    //~ static Pcre2Type<16>::JitStack *jit_stack_create(PCRE2_SIZE startsize, PCRE2_SIZE maxsize,
                                                             //~ Pcre2Type<16>::GeneralContext *gcontext){
    //~ return pcre2_jit_stack_create_16(startsize, maxsize, gcontext);
    //~ }
    //~ static void jit_stack_free(Pcre2Type<16>::JitStack *jit_stack){
        //~ pcre2_jit_stack_free_16(jit_stack);
    //~ }
    //~ static void jit_free_unused_memory(Pcre2Type<16>::GeneralContext *gcontext){
        //~ pcre2_jit_free_unused_memory_16(gcontext);
    //~ }
    //~ static Pcre2Type<16>::MatchContext *match_context_create(Pcre2Type<16>::GeneralContext *gcontext){
        //~ return pcre2_match_context_create_16(gcontext);
    //~ }
    //~ static Pcre2Type<16>::MatchContext *match_context_copy(Pcre2Type<16>::MatchContext *mcontext){
        //~ return pcre2_match_context_copy_16(mcontext);
    //~ }
    //~ static void match_context_free(Pcre2Type<16>::MatchContext *mcontext){
        //~ pcre2_match_context_free_16(mcontext);
    //~ }
    static uint32_t get_ovector_count(Pcre2Type<16>::MatchData *match_data){
        return pcre2_get_ovector_count_16(match_data);
    }
};

//32-bit version
template<> struct Pcre2Func<32> {
    static Pcre2Type<32>::CompileContext* compile_context_create(Pcre2Type<32>::GeneralContext *gcontext){
        return pcre2_compile_context_create_32(gcontext);
    }
    static void compile_context_free(Pcre2Type<32>::CompileContext *ccontext){
        pcre2_compile_context_free_32(ccontext);
    }
    static Pcre2Type<32>::CompileContext* compile_context_copy(Pcre2Type<32>::CompileContext* ccontext){
    return pcre2_compile_context_copy_32(ccontext);
    }
    static const unsigned char * maketables(Pcre2Type<32>::GeneralContext* gcontext){
        return pcre2_maketables_32(gcontext);
    }
    static int set_character_tables(Pcre2Type<32>::CompileContext * ccontext, const unsigned char * table){
        return pcre2_set_character_tables_32(ccontext, table);
    }
    static Pcre2Type<32>::Pcre2Code * compile(Pcre2Type<32>::Pcre2Sptr pattern,
                                     PCRE2_SIZE length,
                                     uint32_t options,
                                     int *errorcode,
                                     PCRE2_SIZE *erroroffset,
                                     Pcre2Type<32>::CompileContext *ccontext){
        return pcre2_compile_32(pattern, length, options, errorcode, erroroffset, ccontext);
    }
    static int jit_compile(Pcre2Type<32>::Pcre2Code *code, uint32_t options){
        return pcre2_jit_compile_32(code, options);
    }
    static int substitute( const Pcre2Type<32>::Pcre2Code *code,
                    Pcre2Type<32>::Pcre2Sptr subject,
                    PCRE2_SIZE length,
                    PCRE2_SIZE startoffset,
                    uint32_t options,
                    Pcre2Type<32>::MatchData *match_data,
                    Pcre2Type<32>::MatchContext *mcontext,
                    Pcre2Type<32>::Pcre2Sptr replacement,
                    PCRE2_SIZE rlength,
                    Pcre2Type<32>::Pcre2Uchar *outputbuffer,
                    PCRE2_SIZE *outlengthptr){
        return pcre2_substitute_32( code, subject, length, startoffset, options, match_data,
                                   mcontext, replacement, rlength, outputbuffer, outlengthptr);
    }
    //~ static int substring_get_bynumber(Pcre2Type<32>::MatchData *match_data,
                                        //~ uint32_t number,
                                        //~ Pcre2Type<32>::Pcre2Uchar **bufferptr,
                                        //~ PCRE2_SIZE *bufflen){
        //~ return pcre2_substring_get_bynumber_32(match_data, number, bufferptr, bufflen);
    //~ }
    //~ static int substring_get_byname(Pcre2Type<32>::MatchData *match_data,
                                        //~ Pcre2Type<32>::Pcre2Sptr name,
                                        //~ Pcre2Type<32>::Pcre2Uchar **bufferptr,
                                        //~ PCRE2_SIZE *bufflen){
        //~ return pcre2_substring_get_byname_32(match_data, name, bufferptr, bufflen);
    //~ }
    //~ static void substring_free(Pcre2Type<32>::Pcre2Uchar *buffer){
        //~ pcre2_substring_free_32(buffer);
    //~ }
    //~ static Pcre2Type<32>::Pcre2Code * code_copy(const Pcre2Type<32>::Pcre2Code *code){
        //~ return pcre2_code_copy_32(code);
    //~ }
    static void code_free(Pcre2Type<32>::Pcre2Code *code){
        pcre2_code_free_32(code);
    }
    static int get_error_message(  int errorcode,
                            Pcre2Type<32>::Pcre2Uchar *buffer,
                            PCRE2_SIZE bufflen){
        return pcre2_get_error_message_32(errorcode, buffer, bufflen);
    }
    static Pcre2Type<32>::MatchData * match_data_create_from_pattern(
                              const Pcre2Type<32>::Pcre2Code *code,
                              Pcre2Type<32>::GeneralContext *gcontext){
        return pcre2_match_data_create_from_pattern_32(code, gcontext);

    }
    static int match(  const Pcre2Type<32>::Pcre2Code *code,
                            Pcre2Type<32>::Pcre2Sptr subject,
                            PCRE2_SIZE length,
                            PCRE2_SIZE startoffset,
                            uint32_t options,
                            Pcre2Type<32>::MatchData *match_data,
                            Pcre2Type<32>::MatchContext *mcontext){
        return pcre2_match_32(code, subject, length, startoffset, options, match_data, mcontext);
    }
    static void match_data_free(Pcre2Type<32>::MatchData *match_data){
        pcre2_match_data_free_32(match_data);
    }
    static PCRE2_SIZE * get_ovector_pointer(Pcre2Type<32>::MatchData *match_data){
        return pcre2_get_ovector_pointer_32(match_data);
    }
    static int pattern_info(const Pcre2Type<32>::Pcre2Code *code, uint32_t what, void *where){
        return pcre2_pattern_info_32(code, what, where);
    }
    static int set_newline(Pcre2Type<32>::CompileContext *ccontext, uint32_t value){
        return pcre2_set_newline_32(ccontext, value);
    }
    //~ static void jit_stack_assign(Pcre2Type<32>::MatchContext *mcontext,
                                 //~ Pcre2Type<32>::JitCallback callback_function,
                                 //~ void *callback_data){
        //~ pcre2_jit_stack_assign_32(mcontext, callback_function, callback_data);
    //~ }
    //~ static Pcre2Type<32>::JitStack *jit_stack_create(PCRE2_SIZE startsize, PCRE2_SIZE maxsize,
                                                             //~ Pcre2Type<32>::GeneralContext *gcontext){
    //~ return pcre2_jit_stack_create_32(startsize, maxsize, gcontext);
    //~ }
    //~ static void jit_stack_free(Pcre2Type<32>::JitStack *jit_stack){
        //~ pcre2_jit_stack_free_32(jit_stack);
    //~ }
    //~ static void jit_free_unused_memory(Pcre2Type<32>::GeneralContext *gcontext){
        //~ pcre2_jit_free_unused_memory_32(gcontext);
    //~ }
    //~ static Pcre2Type<32>::MatchContext *match_context_create(Pcre2Type<32>::GeneralContext *gcontext){
        //~ return pcre2_match_context_create_32(gcontext);
    //~ }
    //~ static Pcre2Type<32>::MatchContext *match_context_copy(Pcre2Type<32>::MatchContext *mcontext){
        //~ return pcre2_match_context_copy_32(mcontext);
    //~ }
    //~ static void match_context_free(Pcre2Type<32>::MatchContext *mcontext){
        //~ pcre2_match_context_free_32(mcontext);
    //~ }
    static uint32_t get_ovector_count(Pcre2Type<32>::MatchData *match_data){
        return pcre2_get_ovector_count_32(match_data);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


///Class to take a std::string modifier value with null safety.
///You don't need to make an instance of this class to pass modifier,
///just pass std::string or char const*, whatever seems feasible,
///implicit conversion will kick in and take care of things for you.
class Modifier{
    std::string mod;

    public:
    ///Default constructor.
    Modifier(){}

    ///Constructor that takes a std::string.
    ///@param x std::string as a reference.
    Modifier(std::string const& x):mod(x){}

    ///Constructor that takes char const * (null safety is provided by this one)
    ///@param x char const *
    Modifier(char const *x):mod(x?x:""){}

    ///Returns the modifier string
    ///@return modifier string (std::string)
    std::string str() const { return mod; }

    ///Returns the c_str() of modifier string
    ///@return char const *
    char const * c_str() const { return mod.c_str(); }

    ///Returns the length of the modifier string
    ///@return length
    SIZE_T length() const{ return mod.length(); }

    ///operator[] overload to access character by index.
    ///@param i index
    ///@return character at index i.
    char operator[](SIZE_T i) const { return mod[i]; }
};


// Namespace for modifier constants.
// For each modifier constant there is a jpcre2::Uint option value.
// Some modifiers may have multiple values set together (ORed in bitwise operation) and
// thus they may include other modifiers. Such an example is the 'n' modifier. It is combined together with 'u'.
namespace MOD {

    // Define modifiers for compile
    // String of compile modifier characters for PCRE2 options
    static const char C_N[] = "eijmnsuxADJU";
    // Array of compile modifier values for PCRE2 options
    // Uint is being used in getModifier() in for loop to get the number of element in this array,
    // be sure to chnage there if you change here.
    static const jpcre2::Uint C_V[12] = { PCRE2_MATCH_UNSET_BACKREF,                  // Modifier e
                                          PCRE2_CASELESS,                             // Modifier i
                                          PCRE2_ALT_BSUX | PCRE2_MATCH_UNSET_BACKREF, // Modifier j
                                          PCRE2_MULTILINE,                            // Modifier m
                                          PCRE2_UTF | PCRE2_UCP,                      // Modifier n (includes u)
                                          PCRE2_DOTALL,                               // Modifier s
                                          PCRE2_UTF,                                  // Modifier u
                                          PCRE2_EXTENDED,                             // Modifier x
                                          PCRE2_ANCHORED,                             // Modifier A
                                          PCRE2_DOLLAR_ENDONLY,                       // Modifier D
                                          PCRE2_DUPNAMES,                             // Modifier J
                                          PCRE2_UNGREEDY                              // Modifier U
                                        };


    // String of compile modifier characters for JPCRE2 options
    static const char CJ_N[] = "S";
    // Array of compile modifier values for JPCRE2 options
    static const jpcre2::Uint CJ_V[1] = { JIT_COMPILE,                                // Modifier S
                                        };


    // Define modifiers for replace
    // String of action (replace) modifier characters for PCRE2 options
    static const char R_N[] = "eEgx";
    // Array of action (replace) modifier values for PCRE2 options
    static const jpcre2::Uint R_V[4]  = {  PCRE2_SUBSTITUTE_UNSET_EMPTY,                // Modifier  e
                                           PCRE2_SUBSTITUTE_UNKNOWN_UNSET | PCRE2_SUBSTITUTE_UNSET_EMPTY,   // Modifier E (includes e)
                                           PCRE2_SUBSTITUTE_GLOBAL,                     // Modifier g
                                           PCRE2_SUBSTITUTE_EXTENDED                    // Modifier x
                                        };


    // String of action (replace) modifier characters for JPCRE2 options
    static const char RJ_N[] = "";
    // Array of action (replace) modifier values for JPCRE2 options
    static const jpcre2::Uint RJ_V[1] = { NONE  //placeholder
                                        };

    // Define modifiers for match
    // String of action (match) modifier characters for PCRE2 options
    static const char M_N[] = "A";
    // Array of action (match) modifier values for PCRE2 options
    static const jpcre2::Uint M_V[1]  = { PCRE2_ANCHORED                               // Modifier  A
                                        };


    // String of action (match) modifier characters for JPCRE2 options
    static const char MJ_N[] = "g";
    // Array of action (match) modifier values for JPCRE2 options
    static const jpcre2::Uint MJ_V[1] = { FIND_ALL,                                   // Modifier  g
                                        };

    static inline void toOption(Modifier const& mod, bool x,
                                Uint const * J_V, char const * J_N, SIZE_T SJ,
                                Uint const * V, char const * N, SIZE_T S,
                                Uint* po, Uint* jo,
                                int* en, SIZE_T* eo
                                ){
        //loop through mod
        SIZE_T n = mod.length();
        for (SIZE_T i = 0; i < n; ++i) {
            //First check for JPCRE2 mods
            for(SIZE_T j = 0; j < SJ; ++j){
                if(J_N[j] == mod[i]) {
                    if(x) *jo |= J_V[j];
                    else  *jo &= ~J_V[j];
                    goto endfor;
                }
            }

            //Now check for PCRE2 mods
            for(SIZE_T j = 0; j< S; ++j){
                if(N[j] == mod[i]){
                    if(x) *po |= V[j];
                    else  *po &= ~V[j];
                    goto endfor;
                }
            }

            //Modifier didn't match, invalid modifier
            *en = (int)ERROR::INVALID_MODIFIER;
            *eo = (int)mod[i];

            endfor:;
        }
    }

    static inline void toMatchOption(Modifier const& mod, bool x, Uint* po, Uint* jo, int* en, SIZE_T* eo){
        toOption(mod, x,
                 MJ_V, MJ_N, sizeof(MJ_V)/sizeof(Uint),
                 M_V, M_N, sizeof(M_V)/sizeof(Uint),
                 po, jo, en, eo);
    }

    static inline void toReplaceOption(Modifier const& mod, bool x, Uint* po, Uint* jo, int* en, SIZE_T* eo){
        toOption(mod, x,
                 RJ_V, RJ_N, sizeof(RJ_V)/sizeof(Uint),
                 R_V, R_N, sizeof(R_V)/sizeof(Uint),
                 po, jo, en, eo);
    }

    static inline void toCompileOption(Modifier const& mod, bool x, Uint* po, Uint* jo, int* en, SIZE_T* eo){
        toOption(mod, x,
                 CJ_V, CJ_N, sizeof(CJ_V)/sizeof(Uint),
                 C_V, C_N, sizeof(C_V)/sizeof(Uint),
                 po, jo, en, eo);
    }

    static inline std::string fromOption(Uint const * J_V, char const * J_N, SIZE_T SJ,
                                         Uint const * V, char const * N, SIZE_T S,
                                         Uint po, Uint jo
                                         ){
        std::string mod;
        //Calculate PCRE2 mod
        for(SIZE_T i = 0; i < S; ++i){
            if( (V[i] & po) != 0 &&
                (V[i] & po) == V[i]) //One option can include other
                mod += N[i];
        }
        //Calculate JPCRE2 mod
        for(SIZE_T i = 0; i < SJ; ++i){
            if( (J_V[i] & jo) != 0 &&
                (J_V[i] & jo) == J_V[i]) //One option can include other
                mod += J_N[i];
        }
        return mod;
    }

    static inline std::string fromMatchOption(Uint po, Uint jo){
        return fromOption(MJ_V, MJ_N, sizeof(MJ_V)/sizeof(Uint),
                          M_V, M_N, sizeof(M_V)/sizeof(Uint),
                          po, jo);
    }

    static inline std::string fromReplaceOption(Uint po, Uint jo){
        return fromOption(RJ_V, RJ_N, sizeof(RJ_V)/sizeof(Uint),
                          R_V, R_N, sizeof(R_V)/sizeof(Uint),
                          po, jo);
    }

    static inline std::string fromCompileOption(Uint po, Uint jo){
        return fromOption(CJ_V, CJ_N, sizeof(CJ_V)/sizeof(Uint),
                          C_V, C_N, sizeof(C_V)/sizeof(Uint),
                          po, jo);
    }

} //MOD namespace ends

///Lets you create custom modifier tables.
///An instance of this class can be passed to
///match, replace or compile related class objects.
class ModifierTable{

    std::string tabjms;
    std::string tabms;
    std::string tabjrs;
    std::string tabrs;
    std::string tabjcs;
    std::string tabcs;
    VecOpt tabjmv;
    VecOpt tabmv;
    VecOpt tabjrv;
    VecOpt tabrv;
    VecOpt tabjcv;
    VecOpt tabcv;

    void toOption(Modifier const& mod, bool x,
                  VecOpt const& J_V, std::string const& J_N,
                  VecOpt const& V, std::string const& N,
                  Uint* po, Uint* jo, int* en, SIZE_T* eo
                  ) const{
        SIZE_T SJ = J_V.size();
        SIZE_T S = V.size();
        JPCRE2_ASSERT(SJ == J_N.length(), ("ValueError: Modifier character and value table must be of the same size (" + _tostdstring(SJ) + " == " + _tostdstring(J_N.length()) + ").").c_str());
        JPCRE2_ASSERT(S == N.length(), ("ValueError: Modifier character and value table must be of the same size (" + _tostdstring(S) + " == " + _tostdstring(N.length()) + ").").c_str());
        MOD::toOption(mod, x,
                     J_V.empty()?0:&J_V[0], J_N.c_str(), SJ,
                     V.empty()?0:&V[0], N.c_str(), S,
                     po, jo, en, eo
                     );
    }

    std::string fromOption(VecOpt const& J_V, std::string const& J_N,
                           VecOpt const& V, std::string const& N,
                           Uint po, Uint jo) const{
        SIZE_T SJ = J_V.size();
        SIZE_T S = V.size();
        JPCRE2_ASSERT(SJ == J_N.length(), ("ValueError: Modifier character and value table must be of the same size (" + _tostdstring(SJ) + " == " + _tostdstring(J_N.length()) + ").").c_str());
        JPCRE2_ASSERT(S == N.length(), ("ValueError: Modifier character and value table must be of the same size (" + _tostdstring(S) + " == " + _tostdstring(N.length()) + ").").c_str());
        return MOD::fromOption(J_V.empty()?0:&J_V[0], J_N.c_str(), SJ,
                     V.empty()?0:&V[0], N.c_str(), S,
                     po, jo);
    }

    void parseModifierTable(std::string& tabjs, VecOpt& tabjv,
                            std::string& tab_s, VecOpt& tab_v,
                            std::string const& tabs, VecOpt const& tabv);
    public:

    ///Default constructor that creates an empty modifier table.
    ModifierTable(){}

    ///@overload
    ///@param deflt Initialize with default table if true, otherwise keep empty.
    ModifierTable(bool deflt){
        if(deflt) setAllToDefault();
    }

    ///Reset the match modifier table to its initial (empty) state including memory.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& resetMatchModifierTable(){
        std::string().swap(tabjms);
        std::string().swap(tabms);
        VecOpt().swap(tabjmv);
        VecOpt().swap(tabmv);
        return *this;
    }

    ///Reset the replace modifier table to its initial (empty) state including memory.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& resetReplaceModifierTable(){
        std::string().swap(tabjrs);
        std::string().swap(tabrs);
        VecOpt().swap(tabjrv);
        VecOpt().swap(tabrv);
        return *this;
    }

    ///Reset the compile modifier table to its initial (empty) state including memory.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& resetCompileModifierTable(){
        std::string().swap(tabjcs);
        std::string().swap(tabcs);
        VecOpt().swap(tabjcv);
        VecOpt().swap(tabcv);
        return *this;
    }

    ///Reset the modifier tables to their initial (empty) state including memory.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& reset(){
        resetMatchModifierTable();
        resetReplaceModifierTable();
        resetCompileModifierTable();
        return *this;
    }

    ///Clear the match modifier table to its initial (empty) state.
    ///Memory may retain for further use.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& clearMatchModifierTable(){
        tabjms.clear();
        tabms.clear();
        tabjmv.clear();
        tabmv.clear();
        return *this;
    }

    ///Clear the replace modifier table to its initial (empty) state.
    ///Memory may retain for further use.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& clearReplaceModifierTable(){
        tabjrs.clear();
        tabrs.clear();
        tabjrv.clear();
        tabrv.clear();
        return *this;
    }

    ///Clear the compile modifier table to its initial (empty) state.
    ///Memory may retain for further use.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& clearCompileModifierTable(){
        tabjcs.clear();
        tabcs.clear();
        tabjcv.clear();
        tabcv.clear();
        return *this;
    }

    ///Clear the modifier tables to their initial (empty) state.
    ///Memory may retain for further use.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& clear(){
        clearMatchModifierTable();
        clearReplaceModifierTable();
        clearCompileModifierTable();
        return *this;
    }

    ///Modifier parser for match related options.
    ///@param mod modifier string
    ///@param x whether to add or remove the modifers.
    ///@param po pointer to PCRE2 match option that will be modified.
    ///@param jo pointer to JPCRE2 match option that will be modified.
    ///@param en where to put the error number.
    ///@param eo where to put the error offset.
    void toMatchOption(Modifier const& mod, bool x, Uint* po, Uint* jo, int* en, SIZE_T* eo) const {
        toOption(mod, x,tabjmv,tabjms,tabmv, tabms,po,jo,en,eo);
    }

    ///Modifier parser for replace related options.
    ///@param mod modifier string
    ///@param x whether to add or remove the modifers.
    ///@param po pointer to PCRE2 replace option that will be modified.
    ///@param jo pointer to JPCRE2 replace option that will be modified.
    ///@param en where to put the error number.
    ///@param eo where to put the error offset.
    void toReplaceOption(Modifier const& mod, bool x, Uint* po, Uint* jo, int* en, SIZE_T* eo) const {
        return toOption(mod, x,tabjrv,tabjrs,tabrv,tabrs,po,jo,en,eo);
    }

    ///Modifier parser for compile related options.
    ///@param mod modifier string
    ///@param x whether to add or remove the modifers.
    ///@param po pointer to PCRE2 compile option that will be modified.
    ///@param jo pointer to JPCRE2 compile option that will be modified.
    ///@param en where to put the error number.
    ///@param eo where to put the error offset.
    void toCompileOption(Modifier const& mod, bool x, Uint* po, Uint* jo, int* en, SIZE_T* eo) const {
        return toOption(mod, x,tabjcv,tabjcs,tabcv,tabcs,po,jo,en,eo);
    }

    ///Take match related option value and convert to modifier string.
    ///@param po PCRE2 option.
    ///@param jo JPCRE2 option.
    ///@return modifier string (std::string)
    std::string fromMatchOption(Uint po, Uint jo) const {
        return fromOption(tabjmv,tabjms,tabmv,tabms,po,jo);
    }

    ///Take replace related option value and convert to modifier string.
    ///@param po PCRE2 option.
    ///@param jo JPCRE2 option.
    ///@return modifier string (std::string)
    std::string fromReplaceOption(Uint po, Uint jo) const {
        return fromOption(tabjrv,tabjrs,tabrv,tabrs,po,jo);
    }

    ///Take compile related option value and convert to modifier string.
    ///@param po PCRE2 option.
    ///@param jo JPCRE2 option.
    ///@return modifier string (std::string)
    std::string fromCompileOption(Uint po, Uint jo) const {
        return fromOption(tabjcv,tabjcs,tabcv,tabcs,po,jo);
    }

    ///Set modifier table for match.
    ///Takes a string and a vector of sequential options.
    ///@param tabs modifier string (list of modifiers)
    ///@param tabv vector of Uint (options).
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setMatchModifierTable(std::string const& tabs, VecOpt const& tabv){
        parseModifierTable(tabjms, tabjmv, tabms, tabmv, tabs, tabv);
        return *this;
    }

    ///Set modifier table for match.
    ///Takes a string and an array of sequential options.
    ///@param tabs modifier string (list of modifiers)
    ///@param tabvp array of Uint (options). If null, table is set to empty.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setMatchModifierTable(std::string const& tabs, const Uint* tabvp){
        if(tabvp) {
            VecOpt tabv(tabvp, tabvp + tabs.length());
            setMatchModifierTable(tabs, tabv);
        } else clearMatchModifierTable();
        return *this;
    }

    ///@overload
    ///...
    ///This one takes modifier and value by array.
    ///If the arrays are not of the same length, the behavior is undefined.
    ///If any of the argument is null, the table is set empty.
    ///@param tabsp modifier string (list of modifiers).
    ///@param tabvp array of Uint (options).
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setMatchModifierTable(const char* tabsp, const Uint* tabvp){
        if(tabsp && tabvp) {
            std::string tabs(tabsp);
            VecOpt tabv(tabvp, tabvp + tabs.length());
            setMatchModifierTable(tabs, tabv);
        } else clearMatchModifierTable();
        return *this;
    }

    ///Set modifier table for replace.
    ///Takes a string and a vector of sequential options.
    ///@param tabs modifier string (list of modifiers)
    ///@param tabv vector of Uint (options).
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setReplaceModifierTable(std::string const& tabs, VecOpt const& tabv){
        parseModifierTable(tabjrs, tabjrv, tabrs, tabrv, tabs, tabv);
        return *this;
    }

    ///Set modifier table for replace.
    ///Takes a string and an array of sequential options.
    ///@param tabs modifier string (list of modifiers)
    ///@param tabvp array of Uint (options). If null, table is set to empty.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setReplaceModifierTable(std::string const& tabs, const Uint* tabvp){
        if(tabvp) {
            VecOpt tabv(tabvp, tabvp + tabs.length());
            setReplaceModifierTable(tabs, tabv);
        } else clearReplaceModifierTable();
        return *this;
    }

    ///@overload
    ///...
    ///This one takes modifier and value by array.
    ///If the arrays are not of the same length, the behavior is undefined.
    ///If any of the argument is null, the table is set empty.
    ///@param tabsp modifier string (list of modifiers).
    ///@param tabvp array of Uint (options).
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setReplaceModifierTable(const char* tabsp, const Uint* tabvp){
        if(tabsp && tabvp) {
            std::string tabs(tabsp);
            VecOpt tabv(tabvp, tabvp + tabs.length());
            setReplaceModifierTable(tabs, tabv);
        } else clearReplaceModifierTable();
        return *this;
    }

    ///Set modifier table for compile.
    ///Takes a string and a vector of sequential options.
    ///@param tabs modifier string (list of modifiers)
    ///@param tabv vector of Uint (options).
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setCompileModifierTable(std::string const& tabs, VecOpt const& tabv){
        parseModifierTable(tabjcs, tabjcv, tabcs, tabcv, tabs, tabv);
        return *this;
    }

    ///Set modifier table for compile.
    ///Takes a string and an array of sequential options.
    ///@param tabs modifier string (list of modifiers)
    ///@param tabvp array of Uint (options). If null, table is set to empty.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setCompileModifierTable(std::string const& tabs, const Uint* tabvp){
        if(tabvp) {
            VecOpt tabv(tabvp, tabvp + tabs.length());
            setCompileModifierTable(tabs, tabv);
        } else clearCompileModifierTable();
        return *this;
    }

    ///@overload
    ///...
    ///This one takes modifier and value by array.
    ///If the arrays are not of the same length, the behavior is undefined.
    ///If any of the argument is null, the table is set empty.
    ///@param tabsp modifier string (list of modifiers).
    ///@param tabvp array of Uint (options).
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setCompileModifierTable(const char* tabsp, const Uint* tabvp){
        if(tabsp && tabvp) {
            std::string tabs(tabsp);
            VecOpt tabv(tabvp, tabvp + tabs.length());
            setCompileModifierTable(tabs, tabv);
        } else clearCompileModifierTable();
        return *this;
    }

    ///Set match modifie table to default
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setMatchModifierTableToDefault(){
        tabjms = std::string(MOD::MJ_N, MOD::MJ_N + sizeof(MOD::MJ_V)/sizeof(Uint));
        tabms = std::string(MOD::M_N, MOD::M_N  + sizeof(MOD::M_V)/sizeof(Uint));
        tabjmv = VecOpt(MOD::MJ_V, MOD::MJ_V + sizeof(MOD::MJ_V)/sizeof(Uint));
        tabmv = VecOpt(MOD::M_V, MOD::M_V + sizeof(MOD::M_V)/sizeof(Uint));
        return *this;
    }

    ///Set replace modifier table to default.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setReplaceModifierTableToDefault(){
        tabjrs = std::string(MOD::RJ_N, MOD::RJ_N  + sizeof(MOD::RJ_V)/sizeof(Uint));
        tabrs = std::string(MOD::R_N, MOD::R_N  + sizeof(MOD::R_V)/sizeof(Uint));
        tabjrv = VecOpt(MOD::RJ_V, MOD::RJ_V + sizeof(MOD::RJ_V)/sizeof(Uint));
        tabrv = VecOpt(MOD::R_V, MOD::R_V + sizeof(MOD::R_V)/sizeof(Uint));
        return *this;
    }

    ///Set compile modifier table to default.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setCompileModifierTableToDefault(){
        tabjcs = std::string(MOD::CJ_N, MOD::CJ_N  + sizeof(MOD::CJ_V)/sizeof(Uint));
        tabcs = std::string(MOD::C_N, MOD::C_N  + sizeof(MOD::C_V)/sizeof(Uint));
        tabjcv = VecOpt(MOD::CJ_V, MOD::CJ_V + sizeof(MOD::CJ_V)/sizeof(Uint));
        tabcv = VecOpt(MOD::C_V, MOD::C_V + sizeof(MOD::C_V)/sizeof(Uint));
        return *this;
    }

    ///Set all tables to default.
    ///@return A reference to the calling ModifierTable object.
    ModifierTable& setAllToDefault(){
        setMatchModifierTableToDefault();
        setReplaceModifierTableToDefault();
        setCompileModifierTableToDefault();
        return *this;
    }
};


//These message strings are used for error/warning message construction.
//take care to prevent multiple definition
template<typename Char_T> struct MSG{
    static std::basic_string<Char_T> INVALID_MODIFIER(void);
    static std::basic_string<Char_T> INSUFFICIENT_OVECTOR(void);
};
//specialization
template<> inline std::basic_string<char> MSG<char>::INVALID_MODIFIER(){ return "Invalid modifier: "; }
template<> inline std::basic_string<wchar_t> MSG<wchar_t>::INVALID_MODIFIER(){ return L"Invalid modifier: "; }
template<> inline std::basic_string<char> MSG<char>::INSUFFICIENT_OVECTOR(){ return "ovector wasn't big enough"; }
template<> inline std::basic_string<wchar_t> MSG<wchar_t>::INSUFFICIENT_OVECTOR(){ return L"ovector wasn't big enough"; }
#ifdef JPCRE2_USE_MINIMUM_CXX_11
template<> inline std::basic_string<char16_t> MSG<char16_t>::INVALID_MODIFIER(){ return u"Invalid modifier: "; }
template<> inline std::basic_string<char32_t> MSG<char32_t>::INVALID_MODIFIER(){ return U"Invalid modifier: "; }
template<> inline std::basic_string<char16_t> MSG<char16_t>::INSUFFICIENT_OVECTOR(){ return u"ovector wasn't big enough"; }
template<> inline std::basic_string<char32_t> MSG<char32_t>::INSUFFICIENT_OVECTOR(){ return U"ovector wasn't big enough"; }
#endif

///struct to select the types.
///
///@tparam Char_T Character type (`char`, `wchar_t`, `char16_t`, `char32_t`)
///@tparam Map Optional parameter (Only `>= C++11`) to specify a map container (`std::map`, `std::unordered_map` etc..). Default is `std::map`.
///
///The character type (`Char_T`) must be in accordance with the PCRE2 library you are linking against.
///If not sure which library you need, link against all 3 PCRE2 libraries and they will be used as needed.
///
///If you want to be specific, then here's the rule:
///
///1. If `Char_T` is 8 bit, you need 8 bit PCRE2 library
///2. If `Char_T` is 16 bit, you need 16 bit PCRE2 library
///3. If `Char_T` is 32 bit, you need 32 bit PCRE2 library
///4. if `Char_T` is not 8 or 16 or 32 bit, you will get compile error.
///
///In `>= C++11` you get an additional optional template parameter to specify a map container.
///For example, you can use `std::unordered_map` instead of the default `std::map`:
/// ```cpp
/// #include <unordered_map>
/// typedef jpcre2::select<char, std::unordered_map> jp;
/// ```
///
///We will use the following typedef throughout this doc:
///```cpp
///typedef jpcre2::select<Char_T> jp;
///```
#ifdef JPCRE2_USE_MINIMUM_CXX_11
template<typename Char_T, template<typename...> class Map=std::map>
#else
template<typename Char_T>
#endif
struct select{

    ///Typedef for character (`char`, `wchar_t`, `char16_t`, `char32_t`)
    typedef Char_T Char;

    //typedef Char_T Char;
    ///Typedef for string (`std::string`, `std::wstring`, `std::u16string`, `std::u32string`).
    ///Defined as `std::basic_string<Char_T>`.
    ///May be this list will make more sense:
    ///Character  | String
    ///---------  | -------
    ///char | std::string
    ///wchar_t | std::wstring
    ///char16_t | std::u16string (>=C++11)
    ///char32_t | std::u32string (>=C++11)
    typedef typename std::basic_string<Char_T> String;

    #ifdef JPCRE2_USE_MINIMUM_CXX_11
    ///Map for Named substrings.
    typedef class Map<String, String> MapNas;
    ///Substring name to Substring number map.
    typedef class Map<String, SIZE_T> MapNtN;
    #else
    ///Map for Named substrings.
    typedef typename std::map<String, String> MapNas;
    ///Substring name to Substring number map.
    typedef typename std::map<String, SIZE_T> MapNtN;
    #endif

    ///Allow spelling mistake of MapNtN as MapNtn.
    typedef MapNtN MapNtn;

    ///Vector for Numbered substrings (Sub container).
    #ifdef JPCRE2_UNSET_CAPTURES_NULL
    typedef typename std::vector<std::optional<String>> NumSub;
    #else
    typedef typename std::vector<String> NumSub;
    #endif
    ///Vector of matches with named substrings.
    typedef typename std::vector<MapNas> VecNas;
    ///Vector of substring name to substring number map.
    typedef typename std::vector<MapNtN> VecNtN;
    ///Allow spelling mistake of VecNtN as VecNtn.
    typedef VecNtN VecNtn;
    ///Vector of matches with numbered substrings.
    typedef typename std::vector<NumSub> VecNum;

    //These are to shorten the code
    typedef typename Pcre2Type<sizeof( Char_T ) * CHAR_BIT>::Pcre2Uchar Pcre2Uchar;
    typedef typename Pcre2Type<sizeof( Char_T ) * CHAR_BIT>::Pcre2Sptr Pcre2Sptr;
    typedef typename Pcre2Type<sizeof( Char_T ) * CHAR_BIT>::Pcre2Code Pcre2Code;
    typedef typename Pcre2Type<sizeof( Char_T ) * CHAR_BIT>::CompileContext CompileContext;
    typedef typename Pcre2Type<sizeof( Char_T ) * CHAR_BIT>::MatchData MatchData;
    typedef typename Pcre2Type<sizeof( Char_T ) * CHAR_BIT>::GeneralContext GeneralContext;
    typedef typename Pcre2Type<sizeof( Char_T ) * CHAR_BIT>::MatchContext MatchContext;
    typedef typename Pcre2Type<sizeof( Char_T ) * CHAR_BIT>::JitCallback JitCallback;
    typedef typename Pcre2Type<sizeof( Char_T ) * CHAR_BIT>::JitStack JitStack;

    template<typename T>
    static String toString(T); //prevent implicit type conversion of T

    ///Converts a Char_T to jpcre2::select::String
    ///@param a Char_T
    ///@return jpcre2::select::String
    static String toString(Char a){
        return a?String(1, a):String();
    }

    ///@overload
    ///...
    ///Converts a Char_T const * to jpcre2::select::String
    ///@param a Char_T const *
    ///@return jpcre2::select::String
    static String toString(Char const *a){
        return a?String(a):String();
    }

    ///@overload
    ///...
    ///Converts a Char_T* to jpcre2::select::String
    ///@param a Char_T const *
    ///@return jpcre2::select::String
    static String toString(Char* a){
        return a?String(a):String();
    }

    ///@overload
    ///...
    ///Converts a PCRE2_UCHAR to String
    ///@param a PCRE2_UCHAR
    ///@return jpcre2::select::String
    static String toString(Pcre2Uchar* a) {
        return a?String((Char*) a):String();
    }

    ///Retruns error message from PCRE2 error number
    ///@param err_num error number (negative)
    ///@return message as jpcre2::select::String.
    static String getPcre2ErrorMessage(int err_num) {
        Pcre2Uchar buffer[sizeof(Char)*CHAR_BIT*1024];
        Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::get_error_message(err_num, buffer, sizeof(buffer));
        return toString((Pcre2Uchar*) buffer);
    }

    ///Returns error message (either JPCRE2 or PCRE2) from error number and error offset
    ///@param err_num error number (negative for PCRE2, positive for JPCRE2)
    ///@param err_off error offset
    ///@return message as jpcre2::select::String.
    static String getErrorMessage(int err_num, int err_off)  {
        if(err_num == (int)ERROR::INVALID_MODIFIER){
            return MSG<Char>::INVALID_MODIFIER() + toString((Char)err_off);
        } else if(err_num == (int)ERROR::INSUFFICIENT_OVECTOR){
            return MSG<Char>::INSUFFICIENT_OVECTOR();
        } else if(err_num != 0) {
            return getPcre2ErrorMessage((int) err_num);
        } else return String();
    }

    //forward declaration
    class Regex;
    class RegexMatch;
    class RegexReplace;
    class MatchEvaluator;

    /** Provides public constructors to create RegexMatch objects.
     * Every RegexMatch object should be associated with a Regex object.
     * This class stores a pointer to its' associated Regex object, thus when
     * the content of the associated Regex object is changed, there will be no need to
     * set the pointer again.
     *
     * Examples:
     *
     * ```cpp
     * jp::Regex re;
     * jp::RegexMatch rm;
     * rm.setRegexObject(&re);
     * rm.match("subject", "g");  // 0 match
     * re.compile("\\w");
     * rm.match();  // 7 matches
     * ```
     */
    class RegexMatch {

    private:

        friend class MatchEvaluator;

        Regex const *re;

        String m_subject;
        String const *m_subject_ptr;
        Uint match_opts;
        Uint jpcre2_match_opts;
        MatchContext *mcontext;
        ModifierTable const * modtab;
        MatchData * mdata;

        PCRE2_SIZE _start_offset; //name collision, use _ at start

        VecNum* vec_num;
        VecNas* vec_nas;
        VecNtN* vec_ntn;

        VecOff* vec_soff;
        VecOff* vec_eoff;

        bool getNumberedSubstrings(int, Pcre2Sptr, PCRE2_SIZE*, uint32_t);

        bool getNamedSubstrings(int, int, Pcre2Sptr, Pcre2Sptr, PCRE2_SIZE*);

        void init_vars() {
            re = 0;
            vec_num = 0;
            vec_nas = 0;
            vec_ntn = 0;
            vec_soff = 0;
            vec_eoff = 0;
            match_opts = 0;
            jpcre2_match_opts = 0;
            error_number = 0;
            error_offset = 0;
            _start_offset = 0;
            m_subject_ptr = &m_subject;
            mcontext = 0;
            modtab = 0;
            mdata = 0;
        }

        void onlyCopy(RegexMatch const &rm){
            re = rm.re; //only pointer should be copied

            //pointer to subject may point to m_subject or other user data
            m_subject_ptr = (rm.m_subject_ptr == &rm.m_subject) ? &m_subject  //not &rm.m_subject
                                                                : rm.m_subject_ptr;

            //underlying data of vectors are not handled by RegexMatch
            //thus it's safe to just copy the pointers.
            vec_num = rm.vec_num;
            vec_nas = rm.vec_nas;
            vec_ntn = rm.vec_ntn;
            vec_soff = rm.vec_soff;
            vec_eoff = rm.vec_eoff;

            match_opts = rm.match_opts;
            jpcre2_match_opts = rm.jpcre2_match_opts;
            error_number = rm.error_number;
            error_offset = rm.error_offset;
            _start_offset = rm._start_offset;
            mcontext = rm.mcontext;
            modtab = rm.modtab;
            mdata = rm.mdata;
        }

        void deepCopy(RegexMatch const &rm){
            m_subject = rm.m_subject;
            onlyCopy(rm);
        }

        #ifdef JPCRE2_USE_MINIMUM_CXX_11
        void deepMove(RegexMatch& rm){
            m_subject = std::move_if_noexcept(rm.m_subject);
            onlyCopy(rm);
        }
        #endif

        friend class Regex;

        protected:

        int error_number;
        PCRE2_SIZE error_offset;

    public:

        ///Default constructor.
        RegexMatch(){
            init_vars();
        }

        ///@overload
        ///...
        ///Creates a RegexMatch object associating a Regex object.
        ///Underlying data is not modified.
        ///@param r pointer to a Regex object
        RegexMatch(Regex const *r) {
            init_vars();
            re = r;
        }

        ///@overload
        ///...
        ///Copy constructor.
        ///@param rm Reference to RegexMatch object
        RegexMatch(RegexMatch const &rm){
            init_vars();
            deepCopy(rm);
        }

        ///Overloaded copy-assignment operator.
        ///@param rm RegexMatch object
        ///@return A reference to the calling RegexMatch object.
        virtual RegexMatch& operator=(RegexMatch const &rm){
            if(this == &rm) return *this;
            deepCopy(rm);
            return *this;
        }

        #ifdef JPCRE2_USE_MINIMUM_CXX_11
        ///@overload
        ///...
        ///Move constructor.
        ///This constructor steals resources from the argument.
        ///It leaves the argument in a valid but indeterminate sate.
        ///The indeterminate state can be returned to normal by calling reset() on that object.
        ///@param rm rvalue reference to a RegexMatch object
        RegexMatch(RegexMatch&& rm){
            init_vars();
            deepMove(rm);
        }

        ///@overload
        ///...
        ///Overloaded move-assignment operator.
        ///This constructor steals resources from the argument.
        ///It leaves the argument in a valid but indeterminate sate.
        ///The indeterminate state can be returned to normal by calling reset() on that object.
        ///@param rm rvalue reference to a RegexMatch object
        ///@return A reference to the calling RegexMatch object.
        virtual RegexMatch& operator=(RegexMatch&& rm){
            if(this == &rm) return *this;
            deepMove(rm);
            return *this;
        }
        #endif

        ///Destructor
        ///Frees all internal memories that were used.
        virtual ~RegexMatch() {}

        ///Reset all class variables to its default (initial) state including memory.
        ///Data in the vectors will retain (as it's external)
        ///You will need to pass vector pointers again after calling this function to get match results.
        ///@return Reference to the calling RegexMatch object.
        virtual RegexMatch& reset() {
            String().swap(m_subject); //not ptr , external string won't be modified.
            init_vars();
            return *this;
        }

        ///Clear all class variables (may retain some memory for further use).
        ///Data in the vectors will retain (as it's external)
        ///You will need to pass vector pointers again after calling this function to get match results.
        ///@return Reference to the calling RegexMatch object.
        virtual RegexMatch& clear(){
            m_subject.clear(); //not ptr , external string won't be modified.
            init_vars();
            return *this;
        }

        ///reset match related errors to zero.
        ///If you want to examine the error status of a function call in the method chain,
        ///add this function just before your target function so that the error is set to zero
        ///before that target function is called, and leave everything out after the target
        ///function so that there will be no additional errors from other function calls.
        ///@return A reference to the RegexMatch object
        ///@see Regex::resetErrors()
        ///@see RegexReplace::resetErrors()
        virtual RegexMatch& resetErrors(){
            error_number = 0;
            error_offset = 0;
            return *this;
        }

        /// Returns the last error number
        ///@return Last error number
        virtual int getErrorNumber() const {
            return error_number;
        }

        /// Returns the last error offset
        ///@return Last error offset
        virtual int getErrorOffset() const  {
            return (int)error_offset;
        }

        /// Returns the last error message
        ///@return Last error message
        virtual String getErrorMessage() const  {
            #ifdef JPCRE2_USE_MINIMUM_CXX_11
            return select<Char, Map>::getErrorMessage(error_number, error_offset);
            #else
            return select<Char>::getErrorMessage(error_number, error_offset);
            #endif
        }

        ///Get subject string (by value).
        ///@return subject string
        ///@see RegexReplace::getSubject()
        virtual String getSubject() const  {
            return *m_subject_ptr;
        }

        ///Get pointer to subject string.
        ///Data can not be changed with this pointer.
        ///@return constant subject string pointer
        ///@see RegexReplace::getSubjectPointer()
        virtual String const * getSubjectPointer() const  {
            return m_subject_ptr;
        }


        /// Calculate modifier string from PCRE2 and JPCRE2 options and return it.
        ///
        /// Do remember that modifiers (or PCRE2 and JPCRE2 options) do not change or get initialized
        /// as long as you don't do that explicitly. Calling RegexMatch::setModifier() will re-set them.
        ///
        /// **Mixed or combined modifier**.
        ///
        /// Some modifier may include other modifiers i.e they have the same meaning of some modifiers
        /// combined together. For example, the 'n' modifier includes the 'u' modifier and together they
        /// are equivalent to `PCRE2_UTF | PCRE2_UCP`. When you set a modifier like this, both options
        /// get set, and when you remove the 'n' modifier (with `RegexMatch::changeModifier()`), both will get removed.
        ///@return Calculated modifier string (std::string)
        ///@see Regex::getModifier()
        ///@see RegexReplace::getModifier()
        virtual std::string getModifier() const {
            return modtab ? modtab->fromMatchOption(match_opts, jpcre2_match_opts)
                          : MOD::fromMatchOption(match_opts, jpcre2_match_opts);
        }

        ///Get the modifier table that is set,
        ///@return pointer to constant ModifierTable.
        virtual ModifierTable const* getModifierTable(){
            return modtab;
        }


        ///Get PCRE2 option
        ///@return PCRE2 option for match operation
        ///@see Regex::getPcre2Option()
        ///@see RegexReplace::getPcre2Option()
        virtual Uint getPcre2Option() const  {
            return match_opts;
        }

        /// Get JPCRE2 option
        ///@return JPCRE2 options for math operation
        ///@see Regex::getJpcre2Option()
        ///@see RegexReplace::getJpcre2Option()
        virtual Uint getJpcre2Option() const  {
            return jpcre2_match_opts;
        }

        /// Get offset from where match will start in the subject.
        /// @return Start offset
        virtual PCRE2_SIZE getStartOffset() const  {
            return _start_offset;
        }

        ///Get pre-set match start offset vector pointer.
        ///The pointer must be set with RegexMatch::setMatchStartOffsetVector() beforehand
        ///for this to work i.e it is just a convenience method to get the pre-set vector pointer.
        ///@return pointer to the const match start offset vector
        virtual VecOff const* getMatchStartOffsetVector() const {
            return vec_soff;
        }

        ///Get pre-set match end offset vector pointer.
        ///The pointer must be set with RegexMatch::setMatchEndOffsetVector() beforehand
        ///for this to work i.e it is just a convenience method to get the pre-set vector pointer.
        ///@return pointer to the const end offset vector
        virtual VecOff const* getMatchEndOffsetVector() const {
            return vec_eoff;
        }

        ///Get a pointer to the associated Regex object.
        ///If no actual Regex object is associated, null is returned.
        ///@return A pointer to the associated constant Regex object or null.
        virtual Regex const * getRegexObject() const {
            return re;
        }

        ///Get pointer to numbered substring vector.
        ///@return Pointer to const numbered substring vector.
        virtual VecNum const* getNumberedSubstringVector() const {
            return vec_num;
        }

        ///Get pointer to named substring vector.
        ///@return Pointer to const named substring vector.
        virtual VecNas const* getNamedSubstringVector() const {
            return vec_nas;
        }

        ///Get pointer to name to number map vector.
        ///@return Pointer to const name to number map vector.
        virtual VecNtN const* getNameToNumberMapVector() const {
            return vec_ntn;
        }

        ///Set the associated regex object.
        ///Null pointer unsets it.
        ///Underlying data is not modified.
        ///@param r Pointer to a Regex object.
        ///@return Reference to the calling RegexMatch object.
        virtual RegexMatch& setRegexObject(Regex const *r){
            re = r;
            return *this;
        }

        /// Set a pointer to the numbered substring vector.
        /// Null pointer unsets it.
        ///
        /// This vector will be filled with numbered (indexed) captured groups.
        /// @param v pointer to the numbered substring vector
        /// @return Reference to the calling RegexMatch object
        virtual RegexMatch& setNumberedSubstringVector(VecNum* v) {
            vec_num = v;
            return *this;
        }

        /// Set a pointer to the named substring vector.
        /// Null pointer unsets it.
        ///
        /// This vector will be populated with named captured groups.
        /// @param v pointer to the named substring vector
        /// @return Reference to the calling RegexMatch object
        virtual RegexMatch& setNamedSubstringVector(VecNas* v) {
            vec_nas = v;
            return *this;
        }

        /// Set a pointer to the name to number map vector.
        /// Null pointer unsets it.
        ///
        /// This vector will be populated with name to number map for captured groups.
        /// @param v pointer to the name to number map vector
        /// @return Reference to the calling RegexMatch object
        virtual RegexMatch& setNameToNumberMapVector(VecNtN* v) {
            vec_ntn = v;
            return *this;
        }

        /// Set the pointer to a vector to store the offsets where matches
        /// start in the subject.
        /// Null pointer unsets it.
        /// @param v Pointer to a jpcre2::VecOff vector (std::vector<size_t>)
        /// @return Reference to the calling RegexMatch object
        virtual RegexMatch& setMatchStartOffsetVector(VecOff* v){
            vec_soff = v;
            return *this;
        }

        /// Set the pointer to a vector to store the offsets where matches
        /// end in the subject.
        /// Null pointer unsets it.
        /// @param v Pointer to a VecOff vector (std::vector<size_t>)
        /// @return Reference to the calling RegexMatch object
        virtual RegexMatch& setMatchEndOffsetVector(VecOff* v){
            vec_eoff = v;
            return *this;
        }

        ///Set the subject string for match.
        ///This makes a copy of the subject string.
        /// @param s Subject string
        /// @return Reference to the calling RegexMatch object
        /// @see RegexReplace::setSubject()
        virtual RegexMatch& setSubject(String const &s) {
            m_subject = s;
            m_subject_ptr = &m_subject; //must overwrite
            return *this;
        }

        ///@overload
        ///...
        /// Works with the original without modifying it. Null pointer unsets the subject.
        /// @param s Pointer to subject string
        /// @return Reference to the calling RegexMatch object
        /// @see RegexReplace::setSubject()
        virtual RegexMatch& setSubject(String const *s) {
            if(s) m_subject_ptr = s;
            else {
                m_subject.clear();
                m_subject_ptr = &m_subject;
            }
            return *this;
        }


        /// Set the modifier (resets all JPCRE2 and PCRE2 options) by calling RegexMatch::changeModifier().
        /// Re-initializes the option bits for PCRE2 and JPCRE2 options, then parses the modifier to set their equivalent options.
        /// @param s Modifier string.
        /// @return Reference to the calling RegexMatch object
        /// @see RegexReplace::setModifier()
        /// @see Regex::setModifier()
        virtual RegexMatch& setModifier(Modifier const& s) {
            match_opts = 0;
            jpcre2_match_opts = 0;
            changeModifier(s, true);
            return *this;
        }

        ///Set a custom modifier table to be used.
        ///@param mdt pointer to ModifierTable object.
        ///@return Reference to the calling RegexMatch object.
        virtual RegexMatch& setModifierTable(ModifierTable const * mdt){
            modtab = mdt;
            return *this;
        }

        /// Set JPCRE2 option for match (resets all)
        /// @param x Option value
        /// @return Reference to the calling RegexMatch object
        /// @see RegexReplace::setJpcre2Option()
        /// @see Regex::setJpcre2Option()
        virtual RegexMatch& setJpcre2Option(Uint x) {
            jpcre2_match_opts = x;
            return *this;
        }

        ///Set PCRE2 option match (overwrite existing option)
        /// @param x Option value
        /// @return Reference to the calling RegexMatch object
        /// @see RegexReplace::setPcre2Option()
        /// @see Regex::setPcre2Option()
        virtual RegexMatch& setPcre2Option(Uint x) {
            match_opts = x;
            return *this;
        }

        /// Set whether to perform global match
        /// @param x True or False
        /// @return Reference to the calling RegexMatch object
        virtual RegexMatch& setFindAll(bool x) {
            jpcre2_match_opts = x?jpcre2_match_opts | FIND_ALL:jpcre2_match_opts & ~FIND_ALL;
            return *this;
        }

        ///@overload
        ///...
        ///This function just calls RegexMatch::setFindAll(bool x) with `true` as the parameter
        ///@return Reference to the calling RegexMatch object
        virtual RegexMatch& setFindAll() {
            return setFindAll(true);
        }

        /// Set offset from where match starts.
        /// When FIND_ALL is set, a global match would not be performed on all positions on the subject,
        /// rather it will be performed from the start offset and onwards.
        /// @param offset Start offset
        /// @return Reference to the calling RegexMatch object
        virtual RegexMatch& setStartOffset(PCRE2_SIZE offset) {
            _start_offset = offset;
            return *this;
        }

        ///Set the match context.
        ///You can create match context using the native PCRE2 API.
        ///The memory is not handled by RegexMatch object and not freed.
        ///User will be responsible for freeing the memory of the match context.
        ///@param match_context Pointer to the match context.
        ///@return Reference to the calling RegexMatch object
        virtual RegexMatch& setMatchContext(MatchContext *match_context){
            mcontext = match_context;
            return *this;
        }

        ///Return pointer to the match context that was previously set with setMatchContext().
        ///Handling memory is the callers' responsibility.
        ///@return pointer to the match context (default: null).
        virtual MatchContext* getMatchContext(){
            return mcontext;
        }

        ///Set the match data block to be used.
        ///The memory is not handled by RegexMatch object and not freed.
        ///User will be responsible for freeing the memory of the match data block.
        ///@param madt Pointer to a match data block.
        ///@return Reference to the calling RegexMatch object
        virtual RegexMatch& setMatchDataBlock(MatchData* madt){
            mdata = madt;
            return *this;
        }

        ///Get the pointer to the match data block that was set previously with setMatchData()
        ///Handling memory is the callers' responsibility.
        ///@return pointer to the match data (default: null).
        virtual MatchData* getMatchDataBlock(){
            return mdata;
        }

        /// Parse modifier and add/remove equivalent PCRE2 and JPCRE2 options.
        /// This function does not initialize or re-initialize options.
        /// If you want to set options from scratch, initialize them to 0 before calling this function.
        /// If invalid modifier is detected, then the error number for the RegexMatch
        /// object will be jpcre2::ERROR::INVALID_MODIFIER and error offset will be the modifier character.
        /// You can get the message with RegexMatch::getErrorMessage() function.
        ///
        /// @param mod Modifier string.
        /// @param x Whether to add or remove option
        /// @return Reference to the RegexMatch object
        /// @see Regex::changeModifier()
        /// @see RegexReplace::changeModifier()
        virtual RegexMatch& changeModifier(Modifier const& mod, bool x){
            modtab ? modtab->toMatchOption(mod, x, &match_opts, &jpcre2_match_opts, &error_number, &error_offset)
                   : MOD::toMatchOption(mod, x, &match_opts, &jpcre2_match_opts, &error_number, &error_offset);
            return *this;
        }

        /// Add or remove a JPCRE2 option
        /// @param opt JPCRE2 option value
        /// @param x Add the option if it's true, remove otherwise.
        /// @return Reference to the calling RegexMatch object
        /// @see RegexReplace::changeJpcre2Option()
        /// @see Regex::changeJpcre2Option()
        virtual RegexMatch& changeJpcre2Option(Uint opt, bool x) {
            jpcre2_match_opts = x ? jpcre2_match_opts | opt : jpcre2_match_opts & ~opt;
            return *this;
        }

        /// Add or remove a PCRE2 option
        /// @param opt PCRE2 option value
        /// @param x Add the option if it's true, remove otherwise.
        /// @return Reference to the calling RegexMatch object
        /// @see RegexReplace::changePcre2Option()
        /// @see Regex::changePcre2Option()
        virtual RegexMatch& changePcre2Option(Uint opt, bool x) {
            match_opts = x ? match_opts | opt : match_opts & ~opt;
            return *this;
        }

        /// Parse modifier string and add equivalent PCRE2 and JPCRE2 options.
        /// This is just a wrapper of the original function RegexMatch::changeModifier()
        /// @param mod Modifier string.
        /// @return Reference to the calling RegexMatch object
        /// @see RegexReplace::addModifier()
        /// @see Regex::addModifier()
        virtual RegexMatch& addModifier(Modifier const& mod){
            return changeModifier(mod, true);
        }

        /// Add option to existing JPCRE2 options for match
        /// @param x Option value
        /// @return Reference to the calling RegexMatch object
        /// @see RegexReplace::addJpcre2Option()
        /// @see Regex::addJpcre2Option()
        virtual RegexMatch& addJpcre2Option(Uint x) {
            jpcre2_match_opts |= x;
            return *this;
        }

        /// Add option to existing PCRE2 options for match
        /// @param x Option value
        /// @return Reference to the calling RegexMatch object
        /// @see RegexReplace::addPcre2Option()
        /// @see Regex::addPcre2Option()
        virtual RegexMatch& addPcre2Option(Uint x) {
            match_opts |= x;
            return *this;
        }


        /// Perform match operation using info from class variables and return the match count and
        /// store the results in specified vectors.
        ///
        /// Note: This function uses pcre2_match() function to do the match.
        ///@return Match count
        virtual SIZE_T match(void);
    };


    ///This class contains a typedef of a function pointer or a templated function wrapper (`std::function`)
    ///to provide callback function to the `MatchEvaluator`.
    ///`std::function` is used when `>=C++11` is being used , otherwise function pointer is used.
    ///You can force using function pointer instead of `std::function` when `>=C++11` is used by defining  the macro
    ///`JPCRE2_USE_FUNCTION_POINTER_CALLBACK` before including jpcre2.hpp.
    ///If you are using lambda function with capture, you must use the `std::function` approach.
    ///
    ///The callback function takes exactly three positional arguments:
    ///@tparam T1 The first argument must be `jp::NumSub const &` aka `std::vector<String> const &` (or `void*` if not needed).
    ///@tparam T2 The second argument must be `jp::MapNas const &` aka `std::map<String, size_t> const &` (or `void*` if not needed).
    ///@tparam T3 The third argument must be `jp::MapNtN const &` aka `std::map<String, String> const &` (or `void*` if not needed).
    ///
    /// **Examples:**
    /// ```cpp
    /// typedef jpcre2::select<char> jp;
    /// jp::String myCallback1(jp::NumSub const &m1, void*, void*){
    ///     return "("+m1[0]+")";
    /// }
    ///
    /// jp::String myCallback2(jp::NumSub const &m1, jp::MapNas const &m2, void*){
    ///     return "("+m1[0]+"/"+m2.at("total")+")";
    /// }
    /// //Now you can pass these functions in MatchEvaluator constructors to create a match evaluator
    /// jp::MatchEvaluator me1(myCallback1);
    ///
    /// //Examples with lambda (>=C++11)
    /// jp::MatchEvaluator me2([](jp::NumSub const &m1, void*, void*)
    ///                         {
    ///                             return "("+m1[0]+")";
    ///                         });
    /// ```
    ///@see MatchEvaluator
    template<typename T1, typename T2, typename T3>
    struct MatchEvaluatorCallback{
        #if !defined JPCRE2_USE_FUNCTION_POINTER_CALLBACK && JPCRE2_USE_MINIMUM_CXX_11
        typedef std::function<String (T1,T2,T3)> Callback;
        #else
        typedef String (*Callback)(T1,T2,T3);
        #endif
    };

    ///Provides some default static callback functions.
    ///The primary goal of this class is to provide default
    ///callback function to MatchEvaluator default constructor which is
    ///essentially callback::erase.
    ///This class does not allow object instantiation.
    struct callback{
        ///Callback function that removes the matched part/s in the subject string
        /// and takes all match vectors as argument.
        ///Even though this function itself does not use the vectors, it still takes them
        ///so that the caller can perform a match and populate all the match data to perform
        ///further evaluation of other callback functions without doing the match again.
        ///@param num jp::NumSub vector.
        ///@param nas jp::MapNas map.
        ///@param ntn jp::MapNtN map.
        ///@return empty string.
        static String eraseFill(NumSub const &num, MapNas const &nas, MapNtN const &ntn){
            return String();
        }

        ///Callback function that removes the matched part/s in the subject string
        ///and does not take any match vector.
        ///This is a minimum cost pattern deleting callback function.
        ///
        ///It's the default callback function when you Instantiate
        ///a MatchEvaluator object with its default constructor:
        ///```cpp
        ///MatchEvaluator me;
        ///```
        ///@return empty string.
        static String erase(void*, void*, void*){
            return String();
        }

        ///Callback function for populating match vectors that does not modify the subject string.
        ///It always returns the total matched part and thus the subject string remains the same.
        ///@param num jp::NumSub vector.
        ///@param nas jp::MapNas map.
        ///@param ntn jp::MapNtN map.
        ///@return total match (group 0) of current match.
        static String fill(NumSub const &num, MapNas const &nas, MapNtn const &ntn){
            #ifdef JPCRE2_UNSET_CAPTURES_NULL
            return *num[0];
            #else
            return num[0];
            #endif
        }

        private:
        //prevent object instantiation.
        callback();
        callback(callback const &);
        #ifdef JPCRE2_USE_MINIMUM_CXX_11
        callback(callback&&);
        #endif
        ~callback();
    };

    ///This class inherits RegexMatch and provides a similar functionality.
    ///All public member functions from RegexMatch class are publicly available except the following:
    ///* setNumberedSubstringVector
    ///* setNamedSubstringVector
    ///* setNameToNumberMapVector
    ///* setMatchStartOffsetVector
    ///* setMatchEndOffsetVector
    ///
    ///The use of above functions is not allowed as the vectors are created according to the callback function you pass.
    ///
    ///Each constructor of this class takes a callback function as argument (see `MatchEvaluatorCallback`).
    ///
    ///It provides a MatchEvaluator::nreplace() function to perform replace operation using native JPCRE2 approach
    ///and `MatchEvaluator::replace()` function for PCRE2 compatible replace operation.
    ///
    ///An instance of this class can also be passed with `RegexReplace::nreplace()` or `RegexReplace::replace()` function to perform replacement
    ///according to this match evaluator.
    ///
    ///Match data is stored in vectors, and the vectors are populated according to the callback functions.
    ///Populated vector data is never deleted but they get overwritten. Vector data can be manually zeroed out
    ///by calling `MatchEvaluator::clearMatchData()`. If the capacities of those match vectors are desired to
    ///to be shrinked too instead of just clearing them, use `MatchEvaluator::resetMatchData()` instead.
    ///
    /// # Re-usability of Match Data
    /// A match data populated with a callback function that takes only a jp::NumSub vector is not compatible
    /// with the data created according to callback function with a jp::MapNas vector.
    /// Because, for this later callback, jp::MapNas data is required but is not available (only jp::NumSub is available).
    /// In such cases, previous Match data can not be used to perform a new replacment operation with this second callback function.
    ///
    /// To populate the match vectors, one must call the `MatchEvaluator::match()` or `MatchEvaluator::nreplace()` function, they will populate
    /// vectors with match data according to call back function.
    ///
    /// ## Example:
    ///
    /// ```cpp
    /// jp::String callback5(NumSub const &m, void*, MapNtn const &n){
    ///     return m[0];
    /// }
    /// jp::String callback4(void*, void*, MapNtn const &n){
    ///     return std::to_string(n.at("name")); //position of group 'name'.
    /// }
    /// jp::String callback2(void*, MapNas const &m, void*){
    ///     return m.at('name'); //substring by name
    /// }
    ///
    /// jp::MatchEvaluator me;
    /// me.setRegexObject(&re).setSubject("string").setCallback(callback5).nreplace();
    /// //In above, nreplace() populates jp::NumSub and jp::MapNtn with match data.
    ///
    /// me.setCallback(callback4).nreplace(false);
    /// //the above uses previous match result (note the 'false') which is OK,
    /// //because, callback4 requires jp::MapNtn which was made available in the previous operation.
    ///
    /// //but the following is not OK: (assertion failure)
    /// me.setCallback(callback2).nreplace(false);
    /// //because, callback2 requires jp::MapNas data which is not available.
    /// //now, this is OK:
    /// me.setCallback(callback2).nreplace();
    /// //because, it will recreate those match data including this one (jp::MapNas).
    /// ```
    ///
    /// # Replace options
    /// MatchEvaluator can not take replace options.
    /// Replace options are taken directly by the replace functions: `nreplace()` and `replace()`.
    ///
    /// # Using as a match object
    /// As it's just a subclass of RegexMatch, it can do all the things that RegexMatch can do, with some restrictions:
    /// * matching options are modified to strip off bad options according to replacement (PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT).
    /// * match depends on the callback function. Only those vectors will be populated that are implemented by the callback functions so far
    ///   (multiple callback function will set multiple match data vectors.)
    /// * match vectors are internal to this class, you can not set them manually (without callback function). (you can get pointers to these vectors
    ///   with `getNumberedSubstringVector()` and related functions).
    ///
    ///@see MatchEvaluatorCallback
    ///@see RegexReplace::nreplace()
    class MatchEvaluator: virtual public RegexMatch{
        private:
        friend class RegexReplace;

        VecNum vec_num;
        VecNas vec_nas;
        VecNtN vec_ntn;
        VecOff vec_soff;
        VecOff vec_eoff;
        int callbackn;
        typename MatchEvaluatorCallback<void*, void*, void*>::Callback callback0;
        typename MatchEvaluatorCallback<NumSub const &, void*, void*>::Callback callback1;
        typename MatchEvaluatorCallback<void*, MapNas const &, void*>::Callback callback2;
        typename MatchEvaluatorCallback<NumSub const &, MapNas const &, void*>::Callback callback3;
        typename MatchEvaluatorCallback<void*, void*, MapNtN const &>::Callback callback4;
        typename MatchEvaluatorCallback<NumSub const &, void*, MapNtN const &>::Callback callback5;
        typename MatchEvaluatorCallback<void*, MapNas const &, MapNtN const &>::Callback callback6;
        typename MatchEvaluatorCallback<NumSub const &, MapNas const &, MapNtN const &>::Callback callback7;
        //Q: Why the callback names seem random? is it random?
        //A: No, it's not random, NumSub = 1, MapNas = 2, MapNtn = 4, thus:
        //     NumSub + MapNas = 3
        //     NumSub + MapNtn = 5
        //     MapNas + MapNtn = 6
        //     NumSub + MapNas + MapNtn = 7
        //Q: Why is it like this?
        //A: It's historical. Once, there was not this many callback declaration, there was only one (a templated one).
        //   The nreplace function itself used to calculate a mode value according to available vectors
        //   and determine what kind of callback function needed to be called.
        //Q: Why the history changed?
        //A: We had some compatibility issues with the single templated callback.
        //   Also, this approach proved to be more readable and robust.

        PCRE2_SIZE buffer_size;


        void init(){
            callbackn = 0;
            callback0 = callback::erase;
            callback1 = 0;
            callback2 = 0;
            callback3 = 0;
            callback4 = 0;
            callback5 = 0;
            callback6 = 0;
            callback7 = 0;
            setMatchStartOffsetVector(&vec_soff);
            setMatchEndOffsetVector(&vec_eoff);
            buffer_size = 0;
        }

        void setVectorPointersAccordingToCallback(){
            switch(callbackn){
                case 0: break;
                case 1: setNumberedSubstringVector(&vec_num);break;
                case 2: setNamedSubstringVector(&vec_nas);break;
                case 3: setNumberedSubstringVector(&vec_num).setNamedSubstringVector(&vec_nas);break;
                case 4: setNameToNumberMapVector(&vec_ntn);break;
                case 5: setNumberedSubstringVector(&vec_num).setNameToNumberMapVector(&vec_ntn);break;
                case 6: setNamedSubstringVector(&vec_nas).setNameToNumberMapVector(&vec_ntn);break;
                case 7: setNumberedSubstringVector(&vec_num).setNamedSubstringVector(&vec_nas).setNameToNumberMapVector(&vec_ntn);break;
            }
        }

        void onlyCopy(MatchEvaluator const &me){
            callbackn = me.callbackn;
            callback0 = me.callback0;
            callback1 = me.callback1;
            callback2 = me.callback2;
            callback3 = me.callback3;
            callback4 = me.callback4;
            callback5 = me.callback5;
            callback6 = me.callback6;
            callback7 = me.callback7;
            //must update the pointers to point to this class vectors.
            setVectorPointersAccordingToCallback();
            buffer_size = me.buffer_size;
        }

        void deepCopy(MatchEvaluator const &me) {
            vec_num = me.vec_num;
            vec_nas = me.vec_nas;
            vec_ntn = me.vec_ntn;
            vec_soff = me.vec_soff;
            vec_eoff = me.vec_eoff;
            onlyCopy(me);
        }

        #ifdef JPCRE2_USE_MINIMUM_CXX_11
        void deepMove(MatchEvaluator& me){
            vec_num = std::move_if_noexcept(me.vec_num);
            vec_nas = std::move_if_noexcept(me.vec_nas);
            vec_ntn = std::move_if_noexcept(me.vec_ntn);
            vec_soff = std::move_if_noexcept(me.vec_soff);
            vec_eoff = std::move_if_noexcept(me.vec_eoff);
            onlyCopy(me);
        }
        #endif

        //prevent public access to some funcitons
        MatchEvaluator& setNumberedSubstringVector(VecNum* v){
            RegexMatch::setNumberedSubstringVector(v);
            return *this;
        }
        MatchEvaluator& setNamedSubstringVector(VecNas* v){
            RegexMatch::setNamedSubstringVector(v);
            return *this;
        }
        MatchEvaluator& setNameToNumberMapVector(VecNtN* v){
            RegexMatch::setNameToNumberMapVector(v);
            return *this;
        }
        MatchEvaluator& setMatchStartOffsetVector(VecOff* v){
            RegexMatch::setMatchStartOffsetVector(v);
            return *this;
        }
        MatchEvaluator& setMatchEndOffsetVector(VecOff* v){
            RegexMatch::setMatchEndOffsetVector(v);
            return *this;
        }

        public:

        ///Default constructor.
        ///Sets callback::erase as the callback function.
        ///Removes matched part/s from the subject string if the callback is not
        ///changed.
        /// ```cpp
        /// jp::Regex re("\s*string");
        /// jp::MatchEvaluator me;
        /// std::cout<<
        /// me.setRegexObject(&re);
        ///   .setSubject("I am a   string");
        ///   .nreplace();
        /// //The above will delete '   string' from the subject
        /// //thus the result will be 'I am a'
        /// ```
        explicit
        MatchEvaluator():RegexMatch(){
            init();
        }

        ///@overload
        ///...
        ///Constructor taking a Regex object pointer.
        ///It sets the associated Regex object and
        ///initializes the MatchEvaluator object with
        ///callback::erase callback function.
        ///Underlying data is not modified.
        ///@param r constant Regex pointer.
        explicit
        MatchEvaluator(Regex const *r):RegexMatch(r){
            init();
        }

        ///@overload
        ///...
        ///Constructor taking a callback function.
        ///It calls a corresponding MatchEvaluator::setCallback() function to set the callback function.
        ///@param mef Callback function.
        explicit
        MatchEvaluator(typename MatchEvaluatorCallback<void*, void*, void*>::Callback mef): RegexMatch(){
            init();
            setCallback(mef);
        }

        ///@overload
        /// ...
        ///It calls a corresponding MatchEvaluator::setCallback() function to set the callback function.
        ///@param mef Callback function.
        explicit
        MatchEvaluator(typename MatchEvaluatorCallback<NumSub const &, void*, void*>::Callback mef): RegexMatch(){
            init();
            setCallback(mef);
        }

        ///@overload
        /// ...
        ///It calls a corresponding MatchEvaluator::setCallback() function to set the callback function.
        ///@param mef Callback function.
        explicit
        MatchEvaluator(typename MatchEvaluatorCallback<NumSub const &, MapNas const &, void*>::Callback mef): RegexMatch(){
            init();
            setCallback(mef);
        }

        ///@overload
        /// ...
        ///It calls a corresponding MatchEvaluator::setCallback() function to set the callback function.
        ///@param mef Callback function.
        explicit
        MatchEvaluator(typename MatchEvaluatorCallback<NumSub const &, void*,  MapNtN const &>::Callback mef): RegexMatch(){
            init();
            setCallback(mef);
        }

        ///@overload
        /// ...
        ///It calls a corresponding MatchEvaluator::setCallback() function to set the callback function.
        ///@param mef Callback function.
        explicit
        MatchEvaluator(typename MatchEvaluatorCallback<NumSub const &, MapNas const &, MapNtN const &>::Callback mef): RegexMatch(){
            init();
            setCallback(mef);
        }

        ///@overload
        /// ...
        ///It calls a corresponding MatchEvaluator::setCallback() function to set the callback function.
        ///@param mef Callback function.
        explicit
        MatchEvaluator(typename MatchEvaluatorCallback<void*, MapNas const &, void*>::Callback mef): RegexMatch(){
            init();
            setCallback(mef);
        }


        ///@overload
        /// ...
        ///It calls a corresponding MatchEvaluator::setCallback() function to set the callback function.
        ///@param mef Callback function.
        explicit
        MatchEvaluator(typename MatchEvaluatorCallback<void*, MapNas const &,  MapNtN const &>::Callback mef): RegexMatch(){
            init();
            setCallback(mef);
        }



        ///@overload
        /// ...
        ///It calls a corresponding MatchEvaluator::setCallback() function to set the callback function.
        ///@param mef Callback function.
        explicit
        MatchEvaluator(typename MatchEvaluatorCallback<void*, void*,  MapNtN const &>::Callback mef): RegexMatch(){
            init();
            setCallback(mef);
        }



        ///@overload
        /// ...
        ///Copy constructor.
        ///@param me Reference to MatchEvaluator object
        MatchEvaluator(MatchEvaluator const &me): RegexMatch(me){
            init();
            deepCopy(me);
        }

        ///Overloaded copy-assignment operator
        ///@param me MatchEvaluator object
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& operator=(MatchEvaluator const &me){
            if(this == &me) return *this;
            RegexMatch::operator=(me);
            deepCopy(me);
            return *this;
        }

        #ifdef JPCRE2_USE_MINIMUM_CXX_11

        ///@overload
        /// ...
        ///Move constructor.
        ///This constructor steals resources from the argument.
        ///It leaves the argument in a valid but indeterminate sate.
        ///The indeterminate state can be returned to normal by calling reset() on that object.
        ///@param me rvalue reference to a MatchEvaluator object
        MatchEvaluator(MatchEvaluator&& me): RegexMatch(me){
            init();
            deepMove(me);
        }

        ///@overload
        ///...
        ///Overloaded move-assignment operator.
        ///It steals resources from the argument.
        ///It leaves the argument in a valid but indeterminate sate.
        ///The indeterminate state can be returned to normal by calling reset() on that object.
        ///@param me rvalue reference to a MatchEvaluator object
        ///@return A reference to the calling MatchEvaluator object.
        ///@see MatchEvaluator(MatchEvaluator&& me)
        MatchEvaluator& operator=(MatchEvaluator&& me){
            if(this == &me) return *this;
            RegexMatch::operator=(me);
            deepMove(me);
            return *this;
        }

        #endif

        virtual ~MatchEvaluator(){}

        ///Member function to set a callback function with no vector reference.
        ///Callback function is always overwritten. The implemented vectors are set to be filled with match data.
        ///Other vectors that were set previously, are not unset and thus they will be filled with match data too
        ///when `match()` or `nreplace()` is called.
        ///@param mef Callback function.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setCallback(typename MatchEvaluatorCallback<void*, void*, void*>::Callback mef){
            callback0 = mef;
            callbackn = 0;
            return *this;
        }

        ///@overload
        /// ...
        ///Sets a callback function with a jp::NumSub vector.
        ///You will be working with a reference to the constant vector.
        ///@param mef Callback function.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setCallback(typename MatchEvaluatorCallback<NumSub const &, void*, void*>::Callback mef){
            callback1 = mef;
            callbackn = 1;
            setNumberedSubstringVector(&vec_num);
            return *this;
        }

        ///@overload
        /// ...
        ///Sets a callback function with a jp::NumSub and jp::MapNas.
        ///You will be working with references of the constant vectors.
        ///For maps, you won't be able to use `[]` operator with reference to constant map, use at() instead:
        ///```cpp
        ///map_nas["word"]; //wrong
        ///map_nas.at("word"); //ok
        ///```
        ///If you want to use `[]` operator with maps, make a copy:
        ///```cpp
        ///jp::MapNas mn = map_nas;
        ///mn["word"]; //ok
        ///```
        ///@param mef Callback function.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setCallback(typename MatchEvaluatorCallback<NumSub const &, MapNas const &, void*>::Callback mef){
            callback3 = mef;
            callbackn = 3;
            setNumberedSubstringVector(&vec_num);
            setNamedSubstringVector(&vec_nas);
            return *this;
        }

        ///@overload
        /// ...
        ///Sets a callback function with a jp::NumSub and jp::MapNtN.
        ///You will be working with references of the constant vectors.
        ///For maps, you won't be able to use `[]` operator with reference to constant map, use at() instead:
        ///```cpp
        ///map_ntn["word"]; //wrong
        ///map_ntn.at("word"); //ok
        ///```
        ///If you want to use `[]` operator with maps, make a copy:
        ///```cpp
        ///jp::MapNtN mn = map_ntn;
        ///mn["word"]; //ok
        ///```
        ///@param mef Callback function.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setCallback(typename MatchEvaluatorCallback<NumSub const &, void*,  MapNtN const &>::Callback mef){
            callback5 = mef;
            callbackn = 5;
            setNumberedSubstringVector(&vec_num);
            setNameToNumberMapVector(&vec_ntn);
            return *this;
        }


        ///@overload
        /// ...
        ///Sets a callback function with a jp::NumSub, jp::MapNas, jp::MapNtN.
        ///You will be working with references of the constant vectors.
        ///For maps, you won't be able to use `[]` operator with reference to constant map, use at() instead:
        ///```cpp
        ///map_nas["word"]; //wrong
        ///map_nas.at("word"); //ok
        ///```
        ///If you want to use `[]` operator with maps, make a copy:
        ///```cpp
        ///jp::MapNas mn = map_nas;
        ///mn["word"]; //ok
        ///```
        ///@param mef Callback function.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setCallback(typename MatchEvaluatorCallback<NumSub const &, MapNas const &, MapNtN const &>::Callback mef){
            callback7 = mef;
            callbackn = 7;
            setNumberedSubstringVector(&vec_num);
            setNamedSubstringVector(&vec_nas);
            setNameToNumberMapVector(&vec_ntn);
            return *this;
        }

        ///@overload
        /// ...
        ///Sets a callback function with a jp::MapNas.
        ///You will be working with reference of the constant vector.
        ///For maps, you won't be able to use `[]` operator with reference to constant map, use at() instead:
        ///```cpp
        ///map_nas["word"]; //wrong
        ///map_nas.at("word"); //ok
        ///```
        ///If you want to use `[]` operator with maps, make a copy:
        ///```cpp
        ///jp::MapNas mn = map_nas;
        ///mn["word"]; //ok
        ///```
        ///@param mef Callback function.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setCallback(typename MatchEvaluatorCallback<void*, MapNas const &, void*>::Callback mef){
            callback2 = mef;
            callbackn = 2;
            setNamedSubstringVector(&vec_nas);
            return *this;
        }

        ///@overload
        /// ...
        ///Sets a callback function with a jp::MapNas, jp::MapNtN.
        ///You will be working with reference of the constant vector.
        ///For maps, you won't be able to use `[]` operator with reference to constant map, use at() instead:
        ///```cpp
        ///map_nas["word"]; //wrong
        ///map_nas.at("word"); //ok
        ///```
        ///If you want to use `[]` operator with maps, make a copy:
        ///```cpp
        ///jp::MapNas mn = map_nas;
        ///mn["word"]; //ok
        ///```
        ///@param mef Callback function.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setCallback(typename MatchEvaluatorCallback<void*, MapNas const &,  MapNtN const &>::Callback mef){
            callback6 = mef;
            callbackn = 6;
            setNamedSubstringVector(&vec_nas);
            setNameToNumberMapVector(&vec_ntn);
            return *this;
        }

        ///@overload
        /// ...
        ///Sets a callback function with a jp::MapNtN.
        ///You will be working with references of the constant vectors.
        ///For maps, you won't be able to use `[]` operator with reference to constant map, use at() instead:
        ///```cpp
        ///map_ntn["word"]; //wrong
        ///map_ntn.at("word"); //ok
        ///```
        ///If you want to use `[]` operator with maps, make a copy:
        ///```cpp
        ///jp::MapNtN mn = map_ntn;
        ///mn["word"]; //ok
        ///```
        ///@param mef Callback function.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setCallback(typename MatchEvaluatorCallback<void*, void*,  MapNtN const &>::Callback mef){
            callback4 = mef;
            callbackn = 4;
            setNameToNumberMapVector(&vec_ntn);
            return *this;
        }

        ///Clear match data.
        ///It clears all match data from all vectors (without shrinking).
        ///For shrinking the vectors, use `resetMatchData()`
        ///A call to `match()`  or nreplace() will be required to produce match data again.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& clearMatchData(){
            vec_num.clear();
            vec_nas.clear();
            vec_ntn.clear();
            vec_soff.clear();
            vec_eoff.clear();
            return *this;
        }

        ///Reset match data to initial state.
        ///It deletes all match data from all vectors shrinking their capacity.
        ///A call to `match()`  or nreplace() will be required to produce match data again.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& resetMatchData(){
            VecNum().swap(vec_num);
            VecNas().swap(vec_nas);
            VecNtN().swap(vec_ntn);
            VecOff().swap(vec_soff);
            VecOff().swap(vec_eoff);
            return *this;
        }


        ///Reset MatchEvaluator to initial state including memory.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& reset(){
            RegexMatch::reset();
            resetMatchData();
            init();
            return *this;
        }

        ///Clears MatchEvaluator.
        ///Returns everything to initial state (some memory may retain for further and faster use).
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& clear(){
            RegexMatch::clear();
            clearMatchData();
            init();
            return *this;
        }

        ///Call RegexMatch::resetErrors().
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& resetErrors(){
            RegexMatch::resetErrors();
            return *this;
        }

        ///Call RegexMatch::setRegexObject(r).
        ///@param r constant Regex object pointer
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setRegexObject (Regex const *r){
            RegexMatch::setRegexObject(r);
            return *this;
        }

        ///Call RegexMatch::setSubject(String const &s).
        ///@param s subject string.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setSubject (String const &s){
            RegexMatch::setSubject(s);
            return *this;
        }

        ///@overload
        ///@param s constant subject string by pointer
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setSubject (String const *s){
            RegexMatch::setSubject(s);
            return *this;
        }

        ///Call RegexMatch::setModifier(Modifier const& s).
        ///@param s modifier string.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setModifier (Modifier const& s){
            RegexMatch::setModifier(s);
            return *this;
        }

        ///Call RegexMatch::setModifierTable(ModifierTable const * s).
        ///@param mdt pointer to ModifierTable object.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setModifierTable (ModifierTable const * mdt){
            RegexMatch::setModifierTable(mdt);
            return *this;
        }

        ///Call RegexMatch::setJpcre2Option(Uint x).
        ///@param x JPCRE2 option value.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setJpcre2Option (Uint x){
            RegexMatch::setJpcre2Option(x);
            return *this;
        }

        ///Call RegexMatch::setPcre2Option (Uint x).
        ///@param x PCRE2 option value.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setPcre2Option (Uint x){
            RegexMatch::setPcre2Option(x);
            return *this;
        }

        ///Call RegexMatch::setFindAll(bool x).
        ///@param x true if global match, false otherwise.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setFindAll (bool x){
            RegexMatch::setFindAll(x);
            return *this;
        }

        ///Call RegexMatch::setFindAll().
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setFindAll(){
            RegexMatch::setFindAll();
            return *this;
        }

        ///Call RegexMatch::setStartOffset (PCRE2_SIZE offset).
        ///@param offset match start offset in the subject.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setStartOffset (PCRE2_SIZE offset){
            RegexMatch::setStartOffset(offset);
            return *this;
        }

        ///Call RegexMatch::setMatchContext(MatchContext *match_context).
        ///@param match_context pointer to match context.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setMatchContext (MatchContext *match_context){
            RegexMatch::setMatchContext(match_context);
            return *this;
        }

        ///Call RegexMatch::setMatchDataBlock(MatchContext * mdt);
        ///@param mdt pointer to match data block
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setMatchDataBlock(MatchData* mdt){
            RegexMatch::setMatchDataBlock(mdt);
            return *this;
        }

        ///Set the buffer size that will be used by pcre2_substitute (replace()).
        ///If buffer size proves to be enough to fit the resultant string
        ///from each match (not the total resultant string), it will yield one less call
        ///to pcre2_substitute for each match.
        ///@param x buffer size.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& setBufferSize(PCRE2_SIZE x){
            buffer_size = x;
            return *this;
        }

        ///Get the initial buffer size that is being used by internal function pcre2_substitute
        ///@return buffer_size
        PCRE2_SIZE getBufferSize(){
            return buffer_size;
        }

        ///Call RegexMatch::changeModifier(Modifier const& mod, bool x).
        ///@param mod modifier string.
        ///@param x true (add) or false (remove).
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& changeModifier (Modifier const& mod, bool x){
            RegexMatch::changeModifier(mod, x);
            return *this;
        }

        ///Call RegexMatch::changeJpcre2Option(Uint opt, bool x).
        ///@param opt JPCRE2 option
        ///@param x true (add) or false (remove).
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& changeJpcre2Option (Uint opt, bool x){
            RegexMatch::changeJpcre2Option(opt, x);
            return *this;
        }

        ///Call RegexMatch::changePcre2Option(Uint opt, bool x).
        ///@param opt PCRE2 option.
        ///@param x true (add) or false (remove).
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& changePcre2Option (Uint opt, bool x){
            RegexMatch::changePcre2Option(opt, x);
            return *this;
        }

        ///Call RegexMatch::addModifier(Modifier const& mod).
        ///@param mod modifier string.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& addModifier (Modifier const& mod){
            RegexMatch::addModifier(mod);
            return *this;
        }

        ///Call RegexMatch::addJpcre2Option(Uint x).
        ///@param x JPCRE2 option.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& addJpcre2Option (Uint x){
            RegexMatch::addJpcre2Option(x);
            return *this;
        }

        ///Call RegexMatch::addPcre2Option(Uint x).
        ///@param x PCRE2 option.
        ///@return A reference to the calling MatchEvaluator object.
        MatchEvaluator& addPcre2Option (Uint x){
            RegexMatch::addPcre2Option(x);
            return *this;
        }

        ///Perform match and return the match count.
        ///This function strips off matching options (PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT) that are considered
        ///bad options for replacement operation and then calls
        ///RegexMatch::match() to perform the match.
        ///@return match count.
        SIZE_T match(void){
            //remove bad matching options
            RegexMatch::changePcre2Option(PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT, false);
            return RegexMatch::match();
        }

        ///Perform regex replace with this match evaluator.
        ///This is a JPCRE2 native replace function (thus the name nreplace).
        ///It uses the `MatchEvaluatorCallback` function that was set with a constructor or `MatchEvaluator::setCallback()` function
        ///to generate the replacement strings on the fly.
        ///The string returned by the callback function will be treated as literal and will
        ///not go through any further processing.
        ///
        ///This function performs a new match everytime it is called unless it is passed with a boolean `false` as the first argument.
        ///To use existing match data that was created by a previous `MatchEvaluator::nreplace()` or `MatchEvaluator::match()`, call this
        ///function with boolean `false` as the first argument.
        ///
        ///## Complexity
        /// 1. Changes in replace related option takes effect without a re-match.
        /// 2. Changes in match related option (e.g start offset) needs a re-match to take effect.
        /// 3. To re-use existing match data, callback function must be compatible with the data, otherwise assertion error.
        /// 4. If the associated Regex object or subject string changes, a new match must be performed,
        ///    trying to use the existing match data in such cases is undefined behavior.
        ///
        ///@param do_match Perform a new matching operation if true, otherwise use existing match data.
        ///@param jro JPCRE2 replace options.
        ///@param counter Pointer to a counter to store the number of replacement done.
        ///@return resultant string after replace.
        ///@see MatchEvaluator.
        ///@see MatchEvaluatorCallback.
        String nreplace(bool do_match=true, Uint jro=0, SIZE_T* counter=0);

        ///PCRE2 compatible replace function that uses this MatchEvaluator.
        ///Performs regex replace with pcre2_substitute function
        ///by generating the replacement strings dynamically with MatchEvaluator callback.
        ///The string returned by callback function is processed by internal pcre2_substitute, thus allowing
        ///all options that are provided by PCRE2 itself.
        ///
        ///This function performs a new match everytime it is called unless it is passed with a boolean `false` as the first argument.
        ///
        ///## Complexity
        /// 1. Changes in replace related option takes effect without a re-match.
        /// 2. Changes in match related option (e.g start offset) needs a re-match to take effect.
        /// 3. To re-use existing match data, callback function must be compatible with the data, otherwise assertion error.
        /// 4. If the associated Regex object or subject string changes, a new match must be performed,
        ///    trying to use the existing match data in such cases is undefined behavior.
        ///
        ///@param do_match perform a new match if true, otherwise use existing data.
        ///@param ro replace related PCRE2 options.
        ///@param counter Pointer to a counter to store the number of replacement done.
        ///@return resultant string after replacement.
        String replace(bool do_match=true, Uint ro=0, SIZE_T* counter=0);
    };

    /** Provides public constructors to create RegexReplace objects.
     * Every RegexReplace object should be associated with a Regex object.
     * This class stores a pointer to its' associated Regex object, thus when
     * the content of the associated Regex object is changed, there's no need to
     * set the pointer again.
     *
     * Examples:
     *
     * ```cpp
     * jp::Regex re;
     * jp::RegexReplace rr;
     * rr.setRegexObject(&re);
     * rr.replace("subject", "me");  // returns 'subject'
     * re.compile("\\w+");
     * rr.replace();  // replaces 'subject' with 'me' i.e returns 'me'
     * ```
     */
    class RegexReplace {

    private:

        friend class Regex;

        Regex const *re;

        String r_subject;
        String *r_subject_ptr; //preplace method modifies it in-place
        String r_replw;
        String const *r_replw_ptr;
        Uint replace_opts;
        Uint jpcre2_replace_opts;
        PCRE2_SIZE buffer_size;
        PCRE2_SIZE _start_offset;
        MatchData *mdata;
        MatchContext *mcontext;
        ModifierTable const * modtab;
        SIZE_T last_replace_count;
        SIZE_T* last_replace_counter;

        void init_vars() {
            re = 0;
            r_subject_ptr = &r_subject;
            r_replw_ptr = &r_replw;
            replace_opts = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH;
            jpcre2_replace_opts = 0;
            buffer_size = 0;
            error_number = 0;
            error_offset = 0;
            _start_offset = 0;
            mdata = 0;
            mcontext = 0;
            modtab = 0;
            last_replace_count = 0;
            last_replace_counter = &last_replace_count;
        }

        void onlyCopy(RegexReplace const &rr){
            re = rr.re; //only pointer should be copied.

            //rr.r_subject_ptr may point to rr.r_subject or other user data
            r_subject_ptr = (rr.r_subject_ptr == &rr.r_subject) ? &r_subject //not rr.r_subject
                                                                : rr.r_subject_ptr; //other user data

            r_replw = rr.r_replw;
            //rr.r_replw_ptr may point to rr.r_replw or other user data
            r_replw_ptr = (rr.r_replw_ptr == &rr.r_replw) ? &r_replw //not rr.r_replw
                                                          : rr.r_replw_ptr; //other user data

            replace_opts = rr.replace_opts;
            jpcre2_replace_opts = rr.jpcre2_replace_opts;
            buffer_size = rr.buffer_size;
            error_number = rr.error_number;
            error_offset = rr.error_offset;
            _start_offset = rr._start_offset;
            mdata = rr.mdata;
            mcontext = rr.mcontext;
            modtab = rr.modtab;
            last_replace_count = rr.last_replace_count;
            last_replace_counter = (rr.last_replace_counter == &rr.last_replace_count) ? &last_replace_count
                                                                                       : rr.last_replace_counter;
        }

        void deepCopy(RegexReplace const &rr){
            r_subject = rr.r_subject;
            onlyCopy(rr);
        }

        #ifdef JPCRE2_USE_MINIMUM_CXX_11
        void deepMove(RegexReplace& rr){
            r_subject = std::move_if_noexcept(rr.r_subject);
            onlyCopy(rr);
        }
        #endif


        protected:

        int error_number;
        PCRE2_SIZE error_offset;

    public:

        ///Default constructor
        RegexReplace(){
            init_vars();
        }

        ///@overload
        /// ...
        ///Creates a RegexReplace object associating a Regex object.
        ///Regex object is not modified.
        ///@param r pointer to a Regex object
        RegexReplace(Regex const *r) {
            init_vars();
            re = r;
        }

        ///@overload
        ///...
        ///Copy constructor.
        ///@param rr RegexReplace object reference
        RegexReplace(RegexReplace const &rr){
            init_vars();
            deepCopy(rr);
        }

        ///Overloaded Copy assignment operator.
        ///@param rr RegexReplace object reference
        ///@return A reference to the calling RegexReplace object
        RegexReplace& operator=(RegexReplace const &rr){
            if(this == &rr) return *this;
            deepCopy(rr);
            return *this;
        }

        #ifdef JPCRE2_USE_MINIMUM_CXX_11

        ///@overload
        ///...
        ///Move constructor.
        ///This constructor steals resources from the argument.
        ///It leaves the argument in a valid but indeterminate sate.
        ///The indeterminate state can be returned to normal by calling reset() on that object.
        ///@param rr rvalue reference to a RegexReplace object reference
        RegexReplace(RegexReplace&& rr){
            init_vars();
            deepMove(rr);
        }

        ///@overload
        ///...
        ///Overloaded move assignment operator.
        ///This constructor steals resources from the argument.
        ///It leaves the argument in a valid but indeterminate sate.
        ///The indeterminate state can be returned to normal by calling reset() on that object.
        ///@param rr rvalue reference to a RegexReplace object reference
        ///@return A reference to the calling RegexReplace object
        RegexReplace& operator=(RegexReplace&& rr){
            if(this == &rr) return *this;
            deepMove(rr);
            return *this;
        }

        #endif

        virtual ~RegexReplace() {}

        ///Reset all class variables to its default (initial) state including memory.
        ///@return Reference to the calling RegexReplace object.
        RegexReplace& reset() {
            String().swap(r_subject);
            String().swap(r_replw);
            init_vars();
            return *this;
        }

        ///Clear all class variables to its default (initial) state (some memory may retain for further use).
        ///@return Reference to the calling RegexReplace object.
        RegexReplace& clear() {
            r_subject.clear();
            r_replw.clear();
            init_vars();
            return *this;
        }

        ///Reset replace related errors to zero.
        ///@return Reference to the calling RegexReplace object
        ///@see Regex::resetErrors()
        ///@see RegexMatch::resetErrors()
        RegexReplace& resetErrors(){
            error_number = 0;
            error_offset = 0;
            return *this;
        }

        /// Returns the last error number
        ///@return Last error number
        int getErrorNumber() const {
            return error_number;
        }

        /// Returns the last error offset
        ///@return Last error offset
        int getErrorOffset() const  {
            return (int)error_offset;
        }

        /// Returns the last error message
        ///@return Last error message
        String getErrorMessage() const  {
            #ifdef JPCRE2_USE_MINIMUM_CXX_11
            return select<Char, Map>::getErrorMessage(error_number, error_offset);
            #else
            return select<Char>::getErrorMessage(error_number, error_offset);
            #endif
        }

        /// Get replacement string
        ///@return replacement string
        String getReplaceWith() const  {
            return *r_replw_ptr;
        }

        /// Get pointer to replacement string
        ///@return pointer to replacement string
        String const * getReplaceWithPointer() const  {
            return r_replw_ptr;
        }

        /// Get subject string
        ///@return subject string
        ///@see RegexMatch::getSubject()
        String getSubject() const  {
            return *r_subject_ptr;
        }

        /// Get pointer to subject string
        ///@return Pointer to constant subject string
        ///@see RegexMatch::getSubjectPointer()
        String const *  getSubjectPointer() const  {
            return r_subject_ptr;
        }


        /// Calculate modifier string from PCRE2 and JPCRE2 options and return it.
        ///
        /// Do remember that modifiers (or PCRE2 and JPCRE2 options) do not change or get initialized
        /// as long as you don't do that explicitly. Calling RegexReplace::setModifier() will re-set them.
        ///
        /// **Mixed or combined modifier**.
        ///
        /// Some modifier may include other modifiers i.e they have the same meaning of some modifiers
        /// combined together. For example, the 'n' modifier includes the 'u' modifier and together they
        /// are equivalent to `PCRE2_UTF | PCRE2_UCP`. When you set a modifier like this, both options
        /// get set, and when you remove the 'n' modifier (with `RegexReplace::changeModifier()`), both will get removed.
        /// @return Calculated modifier string (std::string)
        ///@see RegexMatch::getModifier()
        ///@see Regex::getModifier()
        std::string getModifier() const {
            return modtab ? modtab->fromReplaceOption(replace_opts, jpcre2_replace_opts)
                          : MOD::fromReplaceOption(replace_opts, jpcre2_replace_opts);
        }

        ///Get the modifier table that is set,
        ///@return constant ModifierTable pointer.
        ModifierTable const* getModifierTable(){
            return modtab;
        }

        ///Get start offset.
        ///@return the start offset where matching starts for replace operation
        PCRE2_SIZE getStartOffset() const {
            return _start_offset;
        }

        /// Get PCRE2 option
        ///@return PCRE2 option for replace
        ///@see Regex::getPcre2Option()
        ///@see RegexMatch::getPcre2Option()
        Uint getPcre2Option() const  {
            return replace_opts;
        }

        /// Get JPCRE2 option
        ///@return JPCRE2 option  for replace
        ///@see Regex::getJpcre2Option()
        ///@see RegexMatch::getJpcre2Option()
        Uint getJpcre2Option() const  {
            return jpcre2_replace_opts;
        }

        ///Get a pointer to the associated Regex object.
        ///If no actual Regex object is associated, null is returned
        ///@return A pointer to the associated constant Regex object or null
        Regex const * getRegexObject() const {
            return re;
        }

        ///Return pointer to the match context that was previously set with setMatchContext().
        ///Handling memory is the callers' responsibility.
        ///@return pointer to the match context (default: null).
        MatchContext* getMatchContext(){
            return mcontext;
        }

        ///Get the pointer to the match data block that was set previously with setMatchData()
        ///Handling memory is the callers' responsibility.
        ///@return pointer to the match data (default: null).
        virtual MatchData* getMatchDataBlock(){
            return mdata;
        }

        ///Get the initial buffer size that is being used by internal function pcre2_substitute
        ///@return buffer_size
        PCRE2_SIZE getBufferSize(){
            return buffer_size;
        }

        ///Get the number of replacement in last replace operation.
        ///If you set an external counter with RegexReplace::setReplaceCounter(),
        ///a call to this getter method will dereference the pointer to the external counter
        ///and return the value.
        ///@return Last replace count
        SIZE_T getLastReplaceCount(){
            return *last_replace_counter;
        }

        ///Set an external counter variable to store the replacement count.
        ///This counter will be updated after each replacement operation on this object.
        ///A call to this method will reset the internal counter to 0, thus when you reset the counter
        ///to internal counter (by giving null as param), the previous replace count won't be available.
        ///@param counter Pointer to a counter variable. Null sets the counter to default internal counter.
        ///@return Reference to the calling RegexReplace object.
        RegexReplace& setReplaceCounter(SIZE_T* counter){
            last_replace_count = 0;
            last_replace_counter = counter ? counter : &last_replace_count;
            return *this;
        }

        ///Set the associated Regex object.
        ///Regex object is not modified.
        ///@param r Pointer to a Regex object.
        ///@return Reference to the calling RegexReplace object.
        RegexReplace& setRegexObject(Regex const *r){
            re = r;
            return *this;
        }

        /// Set the subject string for replace.
        ///This makes a copy of the string. If no copy is desired or you are working
        ///with big text, consider passing by pointer.
        ///@param s Subject string
        ///@return Reference to the calling RegexReplace object
        ///@see RegexMatch::setSubject()
        RegexReplace& setSubject(String const &s) {
            r_subject = s;
            r_subject_ptr = &r_subject; //must overwrite
            return *this;
        }

        ///@overload
        ///...
        /// Set pointer to the subject string for replace, null pointer unsets it.
        /// The underlined data is not modified unless RegexReplace::preplace() method is used.
        ///@param s Pointer to subject string
        ///@return Reference to the calling RegexReplace object
        ///@see RegexMatch::setSubject()
        RegexReplace& setSubject(String *s) {
            if(s) r_subject_ptr = s;
            else {
                r_subject.clear();
                r_subject_ptr = &r_subject;
            }
            return *this;
        }

        /// Set the replacement string.
        ///`$` is a special character which implies captured group.
        ///
        ///1. A numbered substring can be referenced with `$n` or `${n}` where n is the group number.
        ///2. A named substring can be referenced with `${name}`, where 'name' is the group name.
        ///3. A literal `$` can be given as `$$`.
        ///
        ///**Note:** This function makes a copy of the string. If no copy is desired or
        ///you are working with big text, consider passing the string with pointer.
        ///
        ///@param s String to replace with
        ///@return Reference to the calling RegexReplace object
        RegexReplace& setReplaceWith(String const &s) {
            r_replw = s;
            r_replw_ptr = &r_replw; //must overwrite
            return *this;
        }

        ///@overload
        ///...
        ///@param s Pointer to the string to replace with, null pointer unsets it.
        ///@return Reference to the calling RegexReplace object
        RegexReplace& setReplaceWith(String const *s) {
            if(s) r_replw_ptr = s;
            else {
                r_replw.clear();
                r_replw_ptr = &r_replw;
            }
            return *this;
        }

        /// Set the modifier string (resets all JPCRE2 and PCRE2 options) by calling RegexReplace::changeModifier().
        ///@param s Modifier string.
        ///@return Reference to the calling RegexReplace object
        ///@see RegexMatch::setModifier()
        ///@see Regex::setModifier()
        RegexReplace& setModifier(Modifier const& s) {
            replace_opts = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH; /* must not be initialized to 0 */
            jpcre2_replace_opts = 0;
            return changeModifier(s, true);
        }

        ///Set a custom modifier table to be used.
        ///@param mdt pointer to ModifierTable object.
        /// @return Reference to the calling RegexReplace object.
        RegexReplace& setModifierTable(ModifierTable const * mdt){
            modtab = mdt;
            return *this;
        }

        /// Set the initial buffer size to be allocated for replaced string (used by PCRE2)
        ///@param x Buffer size
        ///@return Reference to the calling RegexReplace object
        RegexReplace& setBufferSize(PCRE2_SIZE x) {
            buffer_size = x;
            return *this;
        }

        ///Set start offset.
        ///Set the offset where matching starts for replace operation
        ///@param start_offset The offset where matching starts for replace operation
        ///@return Reference to the calling RegexReplace object
        RegexReplace& setStartOffset(PCRE2_SIZE start_offset){
            _start_offset = start_offset;
            return *this;
        }

        /// Set JPCRE2 option for replace (overwrite existing option)
        ///@param x Option value
        ///@return Reference to the calling RegexReplace object
        ///@see RegexMatch::setJpcre2Option()
        ///@see Regex::setJpcre2Option()

        RegexReplace& setJpcre2Option(Uint x) {
            jpcre2_replace_opts = x;
            return *this;
        }

        /// Set PCRE2 option replace (overwrite existing option)
        ///@param x Option value
        ///@return Reference to the calling RegexReplace object
        ///@see RegexMatch::setPcre2Option()
        ///@see Regex::setPcre2Option()

        RegexReplace& setPcre2Option(Uint x) {
            replace_opts = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH | x;
            return *this;
        }

        ///Set the match context to be used.
        ///Native PCRE2 API may be used to create match context.
        ///The memory of the match context is not handled by RegexReplace object and not freed.
        ///User will be responsible for freeing memory.
        ///@param match_context Pointer to match context.
        ///@return Reference to the calling RegexReplace object.
        RegexReplace& setMatchContext(MatchContext * match_context){
            mcontext = match_context;
            return *this;
        }

        ///Set the match data block to be used.
        ///Native PCRE2 API may be used to create match data block.
        ///The memory of the match data is not handled by RegexReplace object and not freed.
        ///User will be responsible for creating/freeing memory.
        ///@param match_data Pointer to match data.
        ///@return Reference to the calling RegexReplace object.
        RegexReplace& setMatchDataBlock(MatchData *match_data){
            mdata = match_data;
            return *this;
        }

        /// After a call to this function PCRE2 and JPCRE2 options will be properly set.
        /// This function does not initialize or re-initialize options.
        /// If you want to set options from scratch, initialize them to 0 before calling this function.
        ///
        /// If invalid modifier is detected, then the error number for the RegexReplace
        /// object will be jpcre2::ERROR::INVALID_MODIFIER and error offset will be the modifier character.
        /// You can get the message with RegexReplace::getErrorMessage() function.
        /// @param mod Modifier string.
        /// @param x Whether to add or remove option
        /// @return Reference to the RegexReplace object
        /// @see Regex::changeModifier()
        /// @see RegexMatch::changeModifier()
        RegexReplace& changeModifier(Modifier const& mod, bool x){
            modtab ? modtab->toReplaceOption(mod, x, &replace_opts, &jpcre2_replace_opts, &error_number, &error_offset)
                   : MOD::toReplaceOption(mod, x, &replace_opts, &jpcre2_replace_opts, &error_number, &error_offset);
            return *this;
        }

        /// Parse modifier and add/remove equivalent PCRE2 and JPCRE2 options.
        /// Add or remove a JPCRE2 option
        /// @param opt JPCRE2 option value
        /// @param x Add the option if it's true, remove otherwise.
        /// @return Reference to the calling RegexReplace object
        /// @see RegexMatch::changeJpcre2Option()
        /// @see Regex::changeJpcre2Option()
        RegexReplace& changeJpcre2Option(Uint opt, bool x) {
            jpcre2_replace_opts = x ? jpcre2_replace_opts | opt : jpcre2_replace_opts & ~opt;
            return *this;
        }

        /// Add or remove a PCRE2 option
        /// @param opt PCRE2 option value
        /// @param x Add the option if it's true, remove otherwise.
        /// @return Reference to the calling RegexReplace object
        /// @see RegexMatch::changePcre2Option()
        /// @see Regex::changePcre2Option()
        RegexReplace& changePcre2Option(Uint opt, bool x) {
            replace_opts = x ? replace_opts | opt : replace_opts & ~opt;
            //replace_opts |= PCRE2_SUBSTITUTE_OVERFLOW_LENGTH; /* It's important, but let user override it. */
            return *this;
        }

        /// Parse modifier string and add equivalent PCRE2 and JPCRE2 options.
        /// This is just a wrapper of the original function RegexReplace::changeModifier()
        /// provided for convenience.
        /// @param mod Modifier string.
        /// @return Reference to the calling RegexReplace object
        /// @see RegexMatch::addModifier()
        /// @see Regex::addModifier()
        RegexReplace& addModifier(Modifier const& mod){
            return changeModifier(mod, true);
        }

        /// Add specified JPCRE2 option to existing options for replace.
        ///@param x Option value
        ///@return Reference to the calling RegexReplace object
        ///@see RegexMatch::addJpcre2Option()
        ///@see Regex::addJpcre2Option()
        RegexReplace& addJpcre2Option(Uint x) {
            jpcre2_replace_opts |= x;
            return *this;
        }

        /// Add specified PCRE2 option to existing options for replace
        ///@param x Option value
        ///@return Reference to the calling RegexReplace object
        ///@see RegexMatch::addPcre2Option()
        ///@see Regex::addPcre2Option()
        RegexReplace& addPcre2Option(Uint x) {
            replace_opts |= x;
            return *this;
        }

        /// Perform regex replace by retrieving subject string, replacement string, modifier and other options from class variables.
        /// In the replacement string (see RegexReplace::setReplaceWith()) `$` is a special character which implies captured group.
        /// 1. A numbered substring can be referenced with `$n` or `${n}` where n is the group number.
        /// 2. A named substring can be referenced with `${name}`, where 'name' is the group name.
        /// 3. A literal `$` can be given as `$$`.
        /// 4. Bash like features: ${<n>:-<string>} and ${<n>:+<string1>:<string2>}, where <n> is a group number or name.
        ///
        ///All options supported by pcre2_substitute is available.
        ///
        /// Note: This function calls pcre2_substitute() to do the replacement.
        ///@return Replaced string
        String replace(void);

        /// Perl compatible replace method.
        /// Modifies subject string in-place and returns replace count.
        ///
        /// The replacement is performed with `RegexReplace::replace()` which uses `pcre2_substitute()`.
        /// @return replace count
        SIZE_T preplace(void){
            *r_subject_ptr = replace();
            return *last_replace_counter;
        }

        /// Perl compatible replace method with match evaluator.
        /// Modifies subject string in-place and returns replace count.
        /// MatchEvaluator class does not have a implementation of this replace method, thus it is not possible
        /// to re-use match data with preplace() method.
        /// Re-using match data with preplace doesn't actually make any sense, because new subject will
        /// always require new match data.
        ///
        /// The replacement is performed with `RegexReplace::replace()` which uses `pcre2_substitute()`.
        /// @param me MatchEvaluator object.
        /// @return replace count
        SIZE_T preplace(MatchEvaluator me){
            *r_subject_ptr = me.setRegexObject(getRegexObject())
                               .setSubject(r_subject_ptr) //do not use method
                               .setFindAll((getPcre2Option() & PCRE2_SUBSTITUTE_GLOBAL)!=0)
                               .setMatchContext(getMatchContext())
                               .setMatchDataBlock(getMatchDataBlock())
                               .setBufferSize(getBufferSize())
                               .setStartOffset(getStartOffset())
                               .replace(true, getPcre2Option(), last_replace_counter);
            return *last_replace_counter;
        }

        ///JPCRE2 native replace function.
        ///A different name is adopted to
        ///distinguish itself from the regular replace() function which
        ///uses pcre2_substitute() to do the replacement; contrary to that,
        ///it will provide a JPCRE2 native way of replacement operation.
        ///It takes a MatchEvaluator object which provides a callback function that is used
        ///to generate replacement string on the fly. Any replacement string set with
        ///`RegexReplace::setReplaceWith()` function will have no effect.
        ///The string returned by the callback function will be treated as literal and will
        ///not go through any further processing.
        ///
        ///This function works on a copy of the MatchEvaluator, and thus makes no changes
        ///to the original. The copy is modified as below:
        ///
        ///1. Global replacement will set FIND_ALL for match, unset otherwise.
        ///2. Bad matching options such as `PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT` will be removed.
        ///3. subject, start_offset and Regex object will change according to the RegexReplace object.
        ///4. match context, and match data block will be changed according to the RegexReplace object.
        ///
        ///It calls MatchEvaluator::nreplace() on the MatchEvaluator object to perform the replacement.
        ///
        ///It always performs a new match.
        ///@param me A MatchEvaluator object.
        ///@return The resultant string after replacement.
        ///@see MatchEvaluator::nreplace()
        ///@see MatchEvaluator
        ///@see MatchEvaluatorCallback
        String nreplace(MatchEvaluator me){
            return me.setRegexObject(getRegexObject())
                     .setSubject(getSubjectPointer())
                     .setFindAll((getPcre2Option() & PCRE2_SUBSTITUTE_GLOBAL)!=0)
                     .setMatchContext(getMatchContext())
                     .setMatchDataBlock(getMatchDataBlock())
                     .setStartOffset(getStartOffset())
                     .nreplace(true, getJpcre2Option(), last_replace_counter);
        }

        ///PCRE2 compatible replace function that takes a MatchEvaluator.
        ///String returned by callback function is processed by pcre2_substitute,
        ///thus all PCRE2 substitute options are supported by this replace function.
        ///
        ///It always performs a new match.
        ///@param me MatchEvaluator instance, (copied and modified according to this object).
        ///@return resultant string.
        ///@see replace()
        String replace(MatchEvaluator me){
            return me.setRegexObject(getRegexObject())
                     .setSubject(getSubjectPointer())
                     .setFindAll((getPcre2Option() & PCRE2_SUBSTITUTE_GLOBAL)!=0)
                     .setMatchContext(getMatchContext())
                     .setMatchDataBlock(getMatchDataBlock())
                     .setBufferSize(getBufferSize())
                     .setStartOffset(getStartOffset())
                     .replace(true, getPcre2Option(), last_replace_counter);
        }
    };


    /** Provides public constructors to create Regex object.
     * Each regex pattern needs an object of this class and each pattern needs to be compiled.
     * Pattern compilation can be done using one of its' overloaded constructors or the `Regex::compile()`
     * member function.
     *
     * Examples:
     *
     * ```cpp
     * jp::Regex re; //does not perform a compile
     * re.compile("pattern", "modifier");
     * jp::Regex re2("pattern", "modifier"); //performs a compile
     * ```
     *
     */
    class Regex {

    private:

        friend class RegexMatch;
        friend class RegexReplace;
        friend class MatchEvaluator;

        String pat_str;
        String const *pat_str_ptr;
        Pcre2Code *code;
        Uint compile_opts;
        Uint jpcre2_compile_opts;
        ModifierTable const * modtab;

        CompileContext *ccontext;
        std::vector<unsigned char> tabv;


        void init_vars() {
            jpcre2_compile_opts = 0;
            compile_opts = 0;
            error_number = 0;
            error_offset = 0;
            code = 0;
            pat_str_ptr = &pat_str;
            ccontext = 0;
            modtab = 0;
        }

        void freeRegexMemory(void) {
            Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::code_free(code);
            code = 0; //we may use it again
        }

        void freeCompileContext(){
            Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::compile_context_free(ccontext);
            ccontext = 0;
        }

        void onlyCopy(Regex const &r){
            //r.pat_str_ptr may point to other user data
            pat_str_ptr = (r.pat_str_ptr == &r.pat_str) ? &pat_str //not r.pat_str
                                                        : r.pat_str_ptr; //other user data

            compile_opts = r.compile_opts;
            jpcre2_compile_opts = r.jpcre2_compile_opts;
            error_number = r.error_number;
            error_offset = r.error_offset;
            modtab = r.modtab;
        }

        void deepCopy(Regex const &r) {
            pat_str = r.pat_str; //must not use setPattern() here

            onlyCopy(r);

            //copy tables
            tabv = r.tabv;
            //copy ccontext if it's not null
            freeCompileContext();
            ccontext = (r.ccontext) ? Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::compile_context_copy(r.ccontext) : 0;
            //if tabv is not empty and ccontext is ok (not null) set the table pointer to ccontext
            if(ccontext  && !tabv.empty()) Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::set_character_tables(ccontext, &tabv[0]);

            //table pointer must be updated in the compiled code itself, jit memory copy is not available.
            //copy is not going to work, we need a recompile.
            //as all vars are already copied, we can just call compile()
            r.code ? compile() //compile frees previous memory.
                   : freeRegexMemory();
        }

        #ifdef JPCRE2_USE_MINIMUM_CXX_11

        void deepMove(Regex& r) {
            pat_str = std::move_if_noexcept(r.pat_str);

            onlyCopy(r);

            //steal tables
            tabv = std::move_if_noexcept(r.tabv);

            //steal ccontext
            freeCompileContext();
            ccontext = r.ccontext; r.ccontext = 0; //must set this to 0
            if(ccontext && !tabv.empty()) Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::set_character_tables(ccontext, &tabv[0]);

            //steal the code
            freeRegexMemory();
            code = r.code; r.code = 0; //must set this to 0
        }

        #endif

        protected:

        int error_number;
        PCRE2_SIZE error_offset;

    public:

        /// Default Constructor.
        /// Initializes all class variables to defaults.
        /// Does not perform any pattern compilation.
        Regex() {
            init_vars();
        }

        ///Compile pattern with initialization.
        /// @param re Pattern string
        Regex(String const &re) {
            init_vars();
            compile(re);
        }

        ///  @overload
        /// @param re Pointer to pattern string. A null pointer will unset the pattern and perform a compile with empty pattern.
        Regex(String const *re) {
            init_vars();
            compile(re);
        }

        ///@overload
        /// @param re Pattern string .
        /// @param mod Modifier string.
        Regex(String const &re, Modifier const& mod) {
            init_vars();
            compile(re, mod);
        }

        ///@overload
        /// @param re Pointer to pattern string. A null pointer will unset the pattern and perform a compile with empty pattern.
        /// @param mod Modifier string.
        Regex(String const *re, Modifier const& mod) {
            init_vars();
            compile(re, mod);
        }

        ///@overload
        /// @param re Pattern string .
        /// @param po PCRE2 option value
        Regex(String const &re, Uint po) {
            init_vars();
            compile(re, po);
        }

        ///@overload
        /// @param re Pointer to pattern string. A null pointer will unset the pattern and perform a compile with empty pattern.
        /// @param po PCRE2 option value
        Regex(String const *re, Uint po) {
            init_vars();
            compile(re, po);
        }

        ///@overload
        /// @param re Pattern string .
        /// @param po    PCRE2 option value
        /// @param jo    JPCRE2 option value
        Regex(String const &re, Uint po, Uint jo) {
            init_vars();
            compile(re, po, jo);
        }

        ///@overload
        /// @param re Pointer to pattern string. A null pointer will unset the pattern and perform a compile with empty pattern.
        /// @param po    PCRE2 option value
        /// @param jo    JPCRE2 option value
        Regex(String const *re, Uint po, Uint jo) {
            init_vars();
            compile(re, po, jo);
        }

        /// @overload
        ///...
        /// Copy constructor.
        /// A separate and new compile is performed from the copied options.
        ///
        /// @param r Constant Regex object reference.
        Regex(Regex const &r) {
            init_vars();
            deepCopy(r);
        }

        /// Overloaded assignment operator.
        /// @param r Regex const &
        /// @return *this
        Regex& operator=(Regex const &r) {
            if (this == &r) return *this;
            deepCopy(r);
            return *this;
        }


        #ifdef JPCRE2_USE_MINIMUM_CXX_11


        /// @overload
        ///...
        /// Move constructor.
        ///This constructor steals resources from the argument.
        ///It leaves the argument in a valid but indeterminate sate.
        ///The indeterminate state can be returned to normal by calling reset() on that object.
        /// @param r rvalue reference to a Regex object.
        Regex(Regex&& r) {
            init_vars();
            deepMove(r);
        }

        ///@overload
        ///...
        /// Overloaded move-assignment operator.
        ///This constructor steals resources from the argument.
        ///It leaves the argument in a valid but indeterminate sate.
        ///The indeterminate state can be returned to normal by calling reset() on that object.
        /// @param r Regex&&
        /// @return *this
        Regex& operator=(Regex&& r) {
            if (this == &r) return *this;
            deepMove(r);
            return *this;
        }

        /// Provides boolean check for the status of the object.
        /// This overloaded boolean operator needs to be declared
        /// explicit to prevent implicit conversion and overloading issues.
        ///
        /// We will only enable it if >=C++11 is being used, as the explicit keyword
        /// for a function other than constructor is not supported in older compilers.
        ///
        /// If you are dealing with legacy code/compilers use the Double bang trick mentioned
        /// in Regex::operator!().
        ///
        /// This helps us to check the status of the compiled regex like this:
        ///
        /// ```
        /// jpcre2::select<char>::Regex re("pat", "mod");
        /// if(re) {
        ///     std::cout<<"Compile success";
        /// } else {
        ///     std::cout<<"Compile failed";
        /// }
        /// ```
        ///@return true if regex compiled successfully, false otherwise.
        ///
        explicit operator bool() const {
            return (code != 0);
        }
        #endif

        /// Provides boolean check for the status of the object.
        /// This is a safe boolean approach (no implicit conversion  or overloading).
        /// We don't need the explicit keyword here and thus it's the preferable method
        /// to check for object status that will work well with older compilers.
        /// e.g:
        ///
        /// ```
        /// jpcre2::select<char>::Regex re("pat","mod");
        /// if(!re) {
        ///     std::cout<<"Compile failed";
        /// } else {
        ///     std::cout<<"Compiled successfully";
        /// }
        /// ```
        /// Double bang trick:
        ///
        /// ```
        /// jpcre2::select<char>::Regex re("pat","mod");
        /// if(!!re) {
        ///     std::cout<<"Compiled successfully";
        /// } else {
        ///     std::cout<<"Compile failed";
        /// }
        /// ```
        /// @return true if regex compile failed, false otherwise.
        bool operator!() const {
            return (code == 0);
        }

        virtual ~Regex() {
            freeRegexMemory();
            freeCompileContext();
        }

        ///Reset all class variables to its default (initial) state including memory.
        ///@return Reference to the calling Regex object.
        Regex& reset() {
            freeRegexMemory();
            freeCompileContext();
            String().swap(pat_str);
            init_vars();
            return *this;
        }

        ///Clear all class variables to its default (initial) state (some memory may retain for further use).
        ///@return Reference to the calling Regex object.
        Regex& clear() {
            freeRegexMemory();
            freeCompileContext();
            pat_str.clear();
            init_vars();
            return *this;
        }

        ///Reset regex compile related errors to zero.
        ///@return A reference to the Regex object
        ///@see  RegexReplace::resetErrors()
        ///@see  RegexMatch::resetErrors()
        Regex& resetErrors() {
            error_number = 0;
            error_offset = 0;
            return *this;
        }

        /// Recreate character tables used by PCRE2.
        /// You should call this function after changing the locale to remake the
        /// character tables according to the new locale.
        /// These character tables are used to compile the regex and used by match
        /// and replace operation. A separate call to compile() will be required
        /// to apply the new character tables.
        /// @return Reference to the calling Regex object.
        Regex& resetCharacterTables() {
            const unsigned char* tables = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::maketables(0); //must pass 0, we are using free() to free the tables.
            tabv = std::vector<unsigned char>(tables, tables+1088);
            ::free((void*)tables); //must free memory
            if(!ccontext)
                ccontext = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::compile_context_create(0);
            Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::set_character_tables(ccontext, &tabv[0]);
            return *this;
        }

        ///Get Pcre2 raw compiled code pointer.
        ///@return pointer to constant pcre2_code or null.
        Pcre2Code const* getPcre2Code() const{
            return code;
        }

        /// Get pattern string
        ///@return pattern string of type jpcre2::select::String
        String getPattern() const  {
            return *pat_str_ptr;
        }

        /// Get pointer to pattern string
        ///@return Pointer to constant pattern string
        String const * getPatternPointer() const  {
            return pat_str_ptr;
        }

        ///Get number of captures from compiled code.
        ///@return New line option value or 0.
        Uint getNumCaptures() {
            if(!code) return 0;
            Uint numCaptures = 0;
            int ret = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::pattern_info(code, PCRE2_INFO_CAPTURECOUNT, &numCaptures);
            if(ret < 0) error_number = ret;
            return numCaptures;
        }

        /// Calculate modifier string from PCRE2 and JPCRE2 options and return it.
        ///
        /// **Mixed or combined modifier**.
        ///
        /// Some modifier may include other modifiers i.e they have the same meaning of some modifiers
        /// combined together. For example, the 'n' modifier includes the 'u' modifier and together they
        /// are equivalent to `PCRE2_UTF | PCRE2_UCP`. When you set a modifier like this, both options
        /// get set, and when you remove the 'n' modifier (with `Regex::changeModifier()`), both will get removed.
        ///@tparam Char_T Character type
        ///@return Calculated modifier string (std::string)
        ///@see RegexMatch::getModifier()
        ///@see RegexReplace::getModifier()
        std::string getModifier() const {
            return modtab ? modtab->fromCompileOption(compile_opts, jpcre2_compile_opts)
                          : MOD::fromCompileOption(compile_opts, jpcre2_compile_opts);
        }

        /// Get PCRE2 option
        /// @return Compile time PCRE2 option value
        ///@see RegexReplace::getPcre2Option()
        ///@see RegexMatch::getPcre2Option()
        Uint getPcre2Option() const  {
            return compile_opts;
        }

        /// Get JPCRE2 option
        /// @return Compile time JPCRE2 option value
        ///@see RegexReplace::getJpcre2Option()
        ///@see RegexMatch::getJpcre2Option()
        Uint getJpcre2Option() const  {
            return jpcre2_compile_opts;
        }

        /// Returns the last error number
        ///@return Last error number
        int getErrorNumber() const {
            return error_number;
        }

        /// Returns the last error offset
        ///@return Last error offset
        int getErrorOffset() const  {
            return (int)error_offset;
        }

        /// Returns the last error message
        ///@return Last error message
        String getErrorMessage() const  {
            #ifdef JPCRE2_USE_MINIMUM_CXX_11
            return select<Char, Map>::getErrorMessage(error_number, error_offset);
            #else
            return select<Char>::getErrorMessage(error_number, error_offset);
            #endif
        }

        ///Get new line convention from compiled code.
        ///@return New line option value or 0.
        ///```
        ///PCRE2_NEWLINE_CR        Carriage return only
        ///PCRE2_NEWLINE_LF        Linefeed only
        ///PCRE2_NEWLINE_CRLF      CR followed by LF only
        ///PCRE2_NEWLINE_ANYCRLF   Any of the above
        ///PCRE2_NEWLINE_ANY       Any Unicode newline sequence
        ///```
        Uint getNewLine() {
            if(!code) return 0;
            Uint newline = 0;
            int ret = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::pattern_info(code, PCRE2_INFO_NEWLINE, &newline);
            if(ret < 0) error_number = ret;
            return newline;
        }

        ///Get the modifier table that is set,
        ///@return constant ModifierTable pointer.
        ModifierTable const* getModifierTable(){
            return modtab;
        }


        ///Set new line convention.
        ///@param value New line option value.
        ///```
        ///PCRE2_NEWLINE_CR        Carriage return only
        ///PCRE2_NEWLINE_LF        Linefeed only
        ///PCRE2_NEWLINE_CRLF      CR followed by LF only
        ///PCRE2_NEWLINE_ANYCRLF   Any of the above
        ///PCRE2_NEWLINE_ANY       Any Unicode newline sequence
        ///```
        ///@return Reference to the calling Regex object
        Regex& setNewLine(Uint value){
            if(!ccontext)
                ccontext = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::compile_context_create(0);
            int ret = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::set_newline(ccontext, value);
            if(ret < 0) error_number = ret;
            return *this;
        }

        /// Set the pattern string to compile
        /// @param re Pattern string
        /// @return Reference to the calling Regex object.
        Regex& setPattern(String const &re) {
            pat_str = re;
            pat_str_ptr = &pat_str; //must overwrite
            return *this;
        }

        /// @overload
        /// @param re Pattern string pointer, null pointer will unset it.
        /// @return Reference to the calling Regex object.
        Regex& setPattern(String const *re) {
            if(re) pat_str_ptr = re;
            else {
                pat_str.clear();
                pat_str_ptr = &pat_str;
            }
            return *this;
        }

        /// set the modifier (resets all JPCRE2 and PCRE2 options) by calling Regex::changeModifier().
        /// Re-initializes the option bits for PCRE2 and JPCRE2 options, then parses the modifier and sets
        /// equivalent PCRE2 and JPCRE2 options.
        /// @param x Modifier string.
        /// @return Reference to the calling Regex object.
        /// @see RegexMatch::setModifier()
        /// @see RegexReplace::setModifier()
        Regex& setModifier(Modifier const& x) {
            compile_opts = 0;
            jpcre2_compile_opts = 0;
            return changeModifier(x, true);
        }

        ///Set a custom modifier table to be used.
        ///@param mdt pointer to ModifierTable object.
        /// @return Reference to the calling Regex object.
        Regex& setModifierTable(ModifierTable const * mdt){
            modtab = mdt;
            return *this;
        }

        /// Set JPCRE2 option for compile (overwrites existing option)
        /// @param x Option value
        /// @return Reference to the calling Regex object.
        /// @see RegexMatch::setJpcre2Option()
        /// @see RegexReplace::setJpcre2Option()
        Regex& setJpcre2Option(Uint x) {
            jpcre2_compile_opts = x;
            return *this;
        }

        ///  Set PCRE2 option for compile (overwrites existing option)
        /// @param x Option value
        /// @return Reference to the calling Regex object.
        /// @see RegexMatch::setPcre2Option()
        /// @see RegexReplace::setPcre2Option()
        Regex& setPcre2Option(Uint x) {
            compile_opts = x;
            return *this;
        }

        /// Parse modifier and add/remove equivalent PCRE2 and JPCRE2 options.
        /// This function does not initialize or re-initialize options.
        /// If you want to set options from scratch, initialize them to 0 before calling this function.
        ///
        /// If invalid modifier is detected, then the error number for the Regex
        /// object will be jpcre2::ERROR::INVALID_MODIFIER and error offset will be the modifier character.
        /// You can get the message with Regex::getErrorMessage() function.
        /// @param mod Modifier string.
        /// @param x Whether to add or remove option
        /// @return Reference to the calling Regex object
        /// @see RegexMatch::changeModifier()
        /// @see RegexReplace::changeModifier()
        Regex& changeModifier(Modifier const& mod, bool x){
            modtab ? modtab->toCompileOption(mod, x, &compile_opts, &jpcre2_compile_opts, &error_number, &error_offset)
                   : MOD::toCompileOption(mod, x, &compile_opts, &jpcre2_compile_opts, &error_number, &error_offset);
            return *this;
        }

        ///  Add or remove a JPCRE2 option
        /// @param opt JPCRE2 option value
        /// @param x Add the option if it's true, remove otherwise.
        /// @return Reference to the calling Regex object
        /// @see RegexMatch::changeJpcre2Option()
        /// @see RegexReplace::changeJpcre2Option()
        Regex& changeJpcre2Option(Uint opt, bool x) {
            jpcre2_compile_opts = x ? jpcre2_compile_opts | opt : jpcre2_compile_opts & ~opt;
            return *this;
        }

        /// Add or remove a PCRE2 option
        /// @param opt PCRE2 option value
        /// @param x Add the option if it's true, remove otherwise.
        /// @return Reference to the calling Regex object
        /// @see RegexMatch::changePcre2Option()
        /// @see RegexReplace::changePcre2Option()
        Regex& changePcre2Option(Uint opt, bool x) {
            compile_opts = x ? compile_opts | opt : compile_opts & ~opt;
            return *this;
        }

        /// Parse modifier string and add equivalent PCRE2 and JPCRE2 options.
        /// This is just a wrapper of the original function Regex::changeModifier()
        /// provided for convenience.
        /// @param mod Modifier string.
        /// @return Reference to the calling Regex object
        /// @see RegexMatch::addModifier()
        /// @see RegexReplace::addModifier()
        Regex& addModifier(Modifier const& mod){
            return changeModifier(mod, true);
        }

        /// Add option to existing JPCRE2 options for compile
        /// @param x Option value
        /// @return Reference to the calling Regex object
        /// @see RegexMatch::addJpcre2Option()
        /// @see RegexReplace::addJpcre2Option()
        Regex& addJpcre2Option(Uint x) {
            jpcre2_compile_opts |= x;
            return *this;
        }

        ///  Add option to existing PCRE2 options for compile
        /// @param x Option value
        /// @return Reference to the calling Regex object
        /// @see RegexMatch::addPcre2Option()
        /// @see RegexReplace::addPcre2Option()
        Regex& addPcre2Option(Uint x) {
            compile_opts |= x;
            return *this;
        }

        ///Compile pattern using info from class variables.
        ///@see Regex::compile(String const &re, Uint po, Uint jo)
        ///@see Regex::compile(String const &re, Uint po)
        ///@see Regex::compile(String const &re, Modifier mod)
        ///@see Regex::compile(String const &re)
        void compile(void);

        ///@overload
        ///...
        /// Set the specified parameters, then compile the pattern using information from class variables.
        /// @param re Pattern string
        /// @param po PCRE2 option
        /// @param jo JPCRE2 option
        void compile(String const &re, Uint po, Uint jo) {
            setPattern(re).setPcre2Option(po).setJpcre2Option(jo);
            compile();
        }


        ///@overload
        /// @param re Pointer to pattern string. A null pointer will unset the pattern and perform a compile with empty pattern.
        /// @param po PCRE2 option
        /// @param jo JPCRE2 option
        void compile(String const *re, Uint po, Uint jo) {
            setPattern(re).setPcre2Option(po).setJpcre2Option(jo);
            compile();
        }

        ///@overload
        /// @param re Pattern string
        /// @param po PCRE2 option
        void compile(String const &re, Uint po) {
            setPattern(re).setPcre2Option(po);
            compile();
        }

        ///@overload
        /// @param re  Pointer to pattern string. A null pointer will unset the pattern and perform a compile with empty pattern.
        /// @param po PCRE2 option
        void compile(String const *re, Uint po) {
            setPattern(re).setPcre2Option(po);
            compile();
        }

        /// @overload
        /// @param re Pattern string
        /// @param mod Modifier string.
        void compile(String const &re, Modifier const& mod) {
            setPattern(re).setModifier(mod);
            compile();
        }

        ///@overload
        /// @param re Pointer to pattern string. A null pointer will unset the pattern and perform a compile with empty pattern.
        /// @param mod Modifier string.
        void compile(String const *re, Modifier const& mod) {
            setPattern(re).setModifier(mod);
            compile();
        }

        ///@overload
        /// @param re Pattern string .
        void compile(String const &re) {
            setPattern(re);
            compile();
        }

        ///@overload
        /// @param re Pointer to pattern string. A null pointer will unset the pattern and perform a compile with empty pattern.
        void compile(String const *re) {
            setPattern(re);
            compile();
        }

        ///Returns a default constructed RegexMatch object by value.
        ///This object is initialized with the same modifier table
        ///as this Regex object.
        ///@return RegexMatch object.
        RegexMatch initMatch(){
            RegexMatch rm(this);
            rm.setModifierTable(modtab);
            return rm;
        }

        ///Synonym for initMatch()
        ///@return RegexMatch object by value.
        RegexMatch getMatchObject(){
            return initMatch();
        }

        /// Perform regex match and return match count using a temporary match object.
        /// This temporary match object will get available options from this Regex object,
        /// that includes modifier table.
        /// @param s Subject string .
        /// @param mod Modifier string.
        /// @param start_offset Offset from where matching will start in the subject string.
        /// @return Match count
        /// @see RegexMatch::match()
        SIZE_T match(String const &s, Modifier const& mod, PCRE2_SIZE start_offset=0) {
            return initMatch().setStartOffset(start_offset).setSubject(s).setModifier(mod).match();
        }

        ///@overload
        ///...
        ///@param s Pointer to subject string. A null pointer will unset the subject and perform a match with empty subject.
        ///@param mod Modifier string.
        ///@param start_offset Offset from where matching will start in the subject string.
        ///@return Match count
        SIZE_T match(String const *s, Modifier const& mod, PCRE2_SIZE start_offset=0) {
            return initMatch().setStartOffset(start_offset).setSubject(s).setModifier(mod).match();
        }

        ///@overload
        ///...
        /// @param s Subject string .
        /// @param start_offset Offset from where matching will start in the subject string.
        /// @return Match count
        /// @see RegexMatch::match()
        SIZE_T match(String const &s,  PCRE2_SIZE start_offset=0) {
            return initMatch().setStartOffset(start_offset).setSubject(s).match();
        }

        ///@overload
        ///...
        /// @param s Pointer to subject string. A null pointer will unset the subject and perform a match with empty subject.
        /// @param start_offset Offset from where matching will start in the subject string.
        /// @return Match count
        /// @see RegexMatch::match()
        SIZE_T match(String const *s,  PCRE2_SIZE start_offset=0) {
            return initMatch().setStartOffset(start_offset).setSubject(s).match();
        }

        ///Returns a default constructed RegexReplace object by value.
        ///This object is initialized with the same modifier table as this Regex object.
        ///@return RegexReplace object.
        RegexReplace initReplace(){
            RegexReplace rr(this);
            rr.setModifierTable(modtab);
            return rr;
        }

        ///Synonym for initReplace()
        ///@return RegexReplace object.
        RegexReplace getReplaceObject(){
            return initReplace();
        }

        /// Perform regex replace and return the replaced string using a temporary replace object.
        /// This temporary replace object will get available options from this Regex object,
        /// that includes modifier table.
        /// @param mains Subject string.
        /// @param repl String to replace with
        /// @param mod Modifier string.
        ///@param counter Pointer to a counter to store the number of replacement done.
        /// @return Resultant string after regex replace
        /// @see RegexReplace::replace()
        String replace(String const &mains, String const &repl, Modifier const& mod="", SIZE_T* counter=0) {
            return initReplace().setSubject(mains).setReplaceWith(repl).setModifier(mod).setReplaceCounter(counter).replace();
        }

        ///@overload
        /// @param mains Pointer to subject string
        /// @param repl String to replace with
        /// @param mod Modifier string.
        ///@param counter Pointer to a counter to store the number of replacement done.
        /// @return Resultant string after regex replace
        /// @see RegexReplace::replace()
        String replace(String *mains, String const &repl, Modifier const& mod="", SIZE_T* counter=0) {
            return initReplace().setSubject(mains).setReplaceWith(repl).setModifier(mod).setReplaceCounter(counter).replace();
        }

        ///@overload
        ///...
        /// @param mains Subject string
        /// @param repl Pointer to string to replace with
        /// @param mod Modifier string.
        ///@param counter Pointer to a counter to store the number of replacement done.
        /// @return Resultant string after regex replace
        /// @see RegexReplace::replace()
        String replace(String const &mains, String const *repl, Modifier const& mod="", SIZE_T* counter=0) {
            return initReplace().setSubject(mains).setReplaceWith(repl).setModifier(mod).setReplaceCounter(counter).replace();
        }

        ///@overload
        ///...
        /// @param mains Pointer to subject string
        /// @param repl Pointer to string to replace with
        /// @param mod Modifier string.
        ///@param counter Pointer to a counter to store the number of replacement done.
        /// @return Resultant string after regex replace
        /// @see RegexReplace::replace()
        String replace(String *mains, String const *repl, Modifier const& mod="", SIZE_T* counter=0) {
            return initReplace().setSubject(mains).setReplaceWith(repl).setModifier(mod).setReplaceCounter(counter).replace();
        }

        /// Perl compatible replace method.
        /// Modifies subject string in-place and returns replace count.
        ///
        /// It's a shorthand method to `RegexReplace::preplace()`.
        /// @param mains Pointer to subject string.
        /// @param repl Replacement string (string to replace with).
        /// @param mod Modifier string.
        /// @return replace count.
        SIZE_T preplace(String * mains, String const& repl, Modifier const& mod=""){
            SIZE_T counter = 0;
            if(mains) *mains = initReplace().setSubject(mains).setReplaceWith(repl).setModifier(mod).setReplaceCounter(&counter).replace();
            return counter;
        }

        /// @overload
        ///
        /// Perl compatible replace method.
        /// Modifies subject string in-place and returns replace count.
        ///
        /// It's a shorthand method to `RegexReplace::preplace()`.
        /// @param mains Pointer to subject string.
        /// @param repl Pointer to replacement string (string to replace with).
        /// @param mod Modifier string.
        /// @return replace count.
        SIZE_T preplace(String * mains, String const* repl, Modifier const& mod=""){
            SIZE_T counter = 0;
            if(mains) *mains = initReplace().setSubject(mains).setReplaceWith(repl).setModifier(mod).setReplaceCounter(&counter).replace();
            return counter;
        }

        /// @overload
        ///
        /// Perl compatible replace method.
        /// Returns replace count and discards subject string.
        ///
        /// It's a shorthand method to `RegexReplace::preplace()`.
        /// @param mains Subject string.
        /// @param repl Replacement string (string to replace with).
        /// @param mod Modifier string.
        /// @return replace count.
        SIZE_T preplace(String const& mains, String const& repl, Modifier const& mod=""){
            SIZE_T counter = 0;
            initReplace().setSubject(mains).setReplaceWith(repl).setModifier(mod).setReplaceCounter(&counter).replace();
            return counter;
        }

        /// @overload
        ///
        /// Perl compatible replace method.
        /// Returns replace count and discards subject string.
        ///
        /// It's a shorthand method to `RegexReplace::preplace()`.
        /// @param mains Subject string.
        /// @param repl Pointer to replacement string (string to replace with).
        /// @param mod Modifier string.
        /// @return replace count.
        SIZE_T preplace(String const& mains, String const* repl, Modifier const& mod=""){
            SIZE_T counter = 0;
            initReplace().setSubject(mains).setReplaceWith(repl).setModifier(mod).setReplaceCounter(&counter).replace();
            return counter;
        }
    };

    private:
    //prevent object instantiation of select class
    select();
    select(select const &);
    #ifdef JPCRE2_USE_MINIMUM_CXX_11
    select(select&&);
    #endif
    ~select();
};//struct select
}//jpcre2 namespace


inline void jpcre2::ModifierTable::parseModifierTable(std::string& tabjs, VecOpt& tabjv,
                                                     std::string& tab_s, VecOpt& tab_v,
                                                     std::string const& tabs, VecOpt const& tabv){
    SIZE_T n = tabs.length();
    JPCRE2_ASSERT(n == tabv.size(), ("ValueError: Could not set Modifier table.\
    Modifier character and value tables are not of the same size (" + _tostdstring(n) + " == " + _tostdstring(tabv.size()) + ").").c_str());
    tabjs.clear();
    tab_s.clear(); tab_s.reserve(n);
    tabjv.clear();
    tab_v.clear(); tab_v.reserve(n);
    for(SIZE_T i=0;i<n;++i){
        switch(tabv[i]){
            case JIT_COMPILE:
            case FIND_ALL: //JPCRE2 options are unique, so it's not necessary to check if it's compile or replace or match.
                tabjs.push_back(tabs[i]); tabjv.push_back(tabv[i]);break;
            default: tab_s.push_back(tabs[i]); tab_v.push_back(tabv[i]); break;
        }
    }
}


#ifdef JPCRE2_USE_MINIMUM_CXX_11
template<typename Char_T, template<typename...> class Map>
void jpcre2::select<Char_T, Map>::Regex::compile() {
#else
template<typename Char_T>
void jpcre2::select<Char_T>::Regex::compile() {
#endif
    //Get c_str of pattern
    Pcre2Sptr c_pattern = (Pcre2Sptr) pat_str_ptr->c_str();
    int err_number = 0;
    PCRE2_SIZE err_offset = 0;

    /**************************************************************************
     * Compile the regular expression pattern, and handle
     * any errors that are detected.
     *************************************************************************/

    //first release any previous memory
    freeRegexMemory();
    code = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::compile(  c_pattern,              /* the pattern */
                                    PCRE2_ZERO_TERMINATED,  /* indicates pattern is zero-terminated */
                                    compile_opts,           /* default options */
                                    &err_number,            /* for error number */
                                    &err_offset,            /* for error offset */
                                    ccontext);              /* use compile context */

    if (code == 0) {
        /* Compilation failed */
        //must not free regex memory, the only function has that right is the destructor
        error_number = err_number;
        error_offset = err_offset;
        return;
    } else if ((jpcre2_compile_opts & JIT_COMPILE) != 0) {
        ///perform JIT compilation it it's enabled
        int jit_ret = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::jit_compile(code, PCRE2_JIT_COMPLETE);
        if(jit_ret < 0) error_number = jit_ret;
    }
    //everything's OK
}


#ifdef JPCRE2_USE_MINIMUM_CXX_11
template<typename Char_T, template<typename...> class Map>
typename jpcre2::select<Char_T, Map>::String jpcre2::select<Char_T, Map>::MatchEvaluator::replace(bool do_match, Uint replace_opts, SIZE_T * counter) {
#else
template<typename Char_T>
typename jpcre2::select<Char_T>::String jpcre2::select<Char_T>::MatchEvaluator::replace(bool do_match, Uint replace_opts, SIZE_T * counter) {
#endif
    if(counter) *counter = 0;

    replace_opts |= PCRE2_SUBSTITUTE_OVERFLOW_LENGTH;
    replace_opts &= ~PCRE2_SUBSTITUTE_GLOBAL;
    Regex const * re = RegexMatch::getRegexObject();
    // If re or re->code is null, return the subject string unmodified.
    if (!re || re->code == 0)
        return RegexMatch::getSubject();

    Pcre2Sptr r_subject_ptr = (Pcre2Sptr) RegexMatch::getSubjectPointer()->c_str();
    //~ SIZE_T totlen = RegexMatch::getSubjectPointer()->length();

    if(do_match) match();
    SIZE_T mcount = vec_soff.size();
    // if mcount is 0, return the subject string. (there's no need to worry about re)
    if(!mcount) return RegexMatch::getSubject();
    SIZE_T current_offset = 0; //needs to be zero, not start_offset, because it's from where unmatched parts will be copied.
    String res, tmp;

    //A check, this check is not fullproof.
    SIZE_T last = vec_eoff.size();
    last = (last>0)?last-1:0;
    JPCRE2_ASSERT(vec_eoff[last] <= RegexMatch::getSubject().size(), "ValueError: subject string is not of the required size, may be it's changed!!!\
    If you are using existing match data, try a new match.");

    //loop through the matches
    for(SIZE_T i=0;i<mcount;++i){
        //first copy the unmatched part.
        //Matches that use \K to end before they start are not supported.
        if(vec_soff[i] < current_offset || vec_eoff[i] < vec_soff[i]){
            RegexMatch::error_number = PCRE2_ERROR_BADSUBSPATTERN;
            return RegexMatch::getSubject();
        } else {
            //~ res += RegexMatch::getSubject().substr(current_offset, vec_soff[i]-current_offset);
            res += String(r_subject_ptr+current_offset, r_subject_ptr+vec_soff[i]);
        }
        //now process the matched part
        switch(callbackn){
            case 0: tmp = callback0((void*)0, (void*)0, (void*)0); break;
            case 1: JPCRE2_VECTOR_DATA_ASSERT(vec_num.size() == mcount, "VecNum");
                    tmp = callback1(vec_num[i], (void*)0, (void*)0); break;
            case 2: JPCRE2_VECTOR_DATA_ASSERT(vec_nas.size() == mcount, "VecNas");
                    tmp = callback2((void*)0, vec_nas[i], (void*)0); break;
            case 3: JPCRE2_VECTOR_DATA_ASSERT(vec_num.size() == mcount && vec_nas.size() == mcount, "VecNum or VecNas");
                    tmp = callback3(vec_num[i], vec_nas[i], (void*)0); break;
            case 4: JPCRE2_VECTOR_DATA_ASSERT(vec_ntn.size() == mcount, "VecNtn");
                    tmp = callback4((void*)0, (void*)0, vec_ntn[i]); break;
            case 5: JPCRE2_VECTOR_DATA_ASSERT(vec_num.size() == mcount && vec_ntn.size() == mcount, "VecNum or VecNtn");
                    tmp = callback5(vec_num[i], (void*)0, vec_ntn[i]); break;
            case 6: JPCRE2_VECTOR_DATA_ASSERT(vec_nas.size() == mcount && vec_ntn.size() == mcount, "VecNas or VecNtn");
                    tmp = callback6((void*)0, vec_nas[i], vec_ntn[i]); break;
            case 7: JPCRE2_VECTOR_DATA_ASSERT(vec_num.size() == mcount && vec_nas.size() == mcount && vec_ntn.size() == mcount, "VecNum\n or VecNas or VecNtn");
                    tmp = callback7(vec_num[i], vec_nas[i], vec_ntn[i]); break;
            default: JPCRE2_ASSERT(2 == 1, "Invalid callbackn. Please file a bug report (must include the line number from below)."); break;
        }
        //reset the current offset
        current_offset = vec_eoff[i];

        //second part
        ///the matched part is the subject
        //~ Pcre2Sptr subject = (Pcre2Sptr) RegexMatch::getSubjectPointer()->c_str();
        //substr(vec_soff[i], vec_eoff[i] - vec_soff[i]).c_str();
        Pcre2Sptr subject = r_subject_ptr + vec_soff[i];
        PCRE2_SIZE subject_length = vec_eoff[i] - vec_soff[i];

        ///the string returned from the callback is the replacement string.
        Pcre2Sptr replace = (Pcre2Sptr) tmp.c_str();
        PCRE2_SIZE replace_length = tmp.length();
        bool retry = true;
        int ret = 0;
        PCRE2_SIZE outlengthptr = 0;
        Pcre2Uchar* output_buffer = new Pcre2Uchar[outlengthptr + 1]();

        while (true) {
            ret = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::substitute(
                        re->code,               /*Points to the compiled pattern*/
                        subject,                /*Points to the subject string*/
                        subject_length,         /*Length of the subject string*/
                        0,                      /*Offset in the subject at which to start matching*/ //must be zero
                        replace_opts,           /*Option bits*/
                        RegexMatch::mdata,      /*Points to a match data block, or is NULL*/
                        RegexMatch::mcontext,   /*Points to a match context, or is NULL*/
                        replace,                /*Points to the replacement string*/
                        replace_length,         /*Length of the replacement string*/
                        output_buffer,          /*Points to the output buffer*/
                        &outlengthptr           /*Points to the length of the output buffer*/
                        );

            if (ret < 0) {
                //Handle errors
                if ((replace_opts & PCRE2_SUBSTITUTE_OVERFLOW_LENGTH) != 0
                        && ret == (int) PCRE2_ERROR_NOMEMORY && retry) {
                    retry = false;
                    /// If initial #buffer_size wasn't big enough for resultant string,
                    /// we will try once more with a new buffer size adjusted to the length of the resultant string.
                    delete[] output_buffer;
                    output_buffer = new Pcre2Uchar[outlengthptr + 1]();
                    // Go and try to perform the substitute again
                    continue;
                } else {
                    RegexMatch::error_number = ret;
                    delete[] output_buffer;
                    return RegexMatch::getSubject();
                }
            }
            //If everything's ok exit the loop
            break;
        }
        res += String((Char*) output_buffer,(Char*) (output_buffer + outlengthptr) );
        delete[] output_buffer;
        if(counter) *counter += ret;
        //if FIND_ALL is not set, single match will be performed
        if((RegexMatch::getJpcre2Option() & FIND_ALL) == 0) break;
    }
    //All matched parts have been dealt with.
    //now copy rest of the string from current_offset
    res += RegexMatch::getSubject().substr(current_offset, String::npos);
    return res;
}


#ifdef JPCRE2_USE_MINIMUM_CXX_11
template<typename Char_T, template<typename...> class Map>
typename jpcre2::select<Char_T, Map>::String jpcre2::select<Char_T, Map>::MatchEvaluator::nreplace(bool do_match, Uint jo, SIZE_T* counter){
#else
template<typename Char_T>
typename jpcre2::select<Char_T>::String jpcre2::select<Char_T>::MatchEvaluator::nreplace(bool do_match, Uint jo, SIZE_T* counter){
#endif
    if(counter) *counter = 0;
    if(do_match) match();
    SIZE_T mcount = vec_soff.size();
    // if mcount is 0, return the subject string. (there's no need to worry about re)
    if(!mcount) return RegexMatch::getSubject();
    SIZE_T current_offset = 0; //no need for worrying about start offset, it's handled by match and we get valid offsets out of it.
    String res;

    //A check, this check is not fullproof
    SIZE_T last = vec_eoff.size();
    last = (last>0)?last-1:0;
    JPCRE2_ASSERT(vec_eoff[last] <= RegexMatch::getSubject().size(), "ValueError: subject string is not of the required size, may be it's changed!!!\
    If you are using existing match data, try a new match.");

    //loop through the matches
    for(SIZE_T i=0;i<mcount;++i){
        //first copy the unmatched part.
        //Matches that use \K to end before they start are not supported.
        if(vec_soff[i] < current_offset){
            RegexMatch::error_number = PCRE2_ERROR_BADSUBSPATTERN;
            return RegexMatch::getSubject();
        } else {
            res += RegexMatch::getSubject().substr(current_offset, vec_soff[i]-current_offset);
        }
        //now process the matched part
        switch(callbackn){
            case 0: res += callback0((void*)0, (void*)0, (void*)0); break;
            case 1: JPCRE2_VECTOR_DATA_ASSERT(vec_num.size() == mcount, "VecNum");
                    res += callback1(vec_num[i], (void*)0, (void*)0); break;
            case 2: JPCRE2_VECTOR_DATA_ASSERT(vec_nas.size() == mcount, "VecNas");
                    res += callback2((void*)0, vec_nas[i], (void*)0); break;
            case 3: JPCRE2_VECTOR_DATA_ASSERT(vec_num.size() == mcount && vec_nas.size() == mcount, "VecNum or VecNas");
                    res += callback3(vec_num[i], vec_nas[i], (void*)0); break;
            case 4: JPCRE2_VECTOR_DATA_ASSERT(vec_ntn.size() == mcount, "VecNtn");
                    res += callback4((void*)0, (void*)0, vec_ntn[i]); break;
            case 5: JPCRE2_VECTOR_DATA_ASSERT(vec_num.size() == mcount && vec_ntn.size() == mcount, "VecNum or VecNtn");
                    res += callback5(vec_num[i], (void*)0, vec_ntn[i]); break;
            case 6: JPCRE2_VECTOR_DATA_ASSERT(vec_nas.size() == mcount && vec_ntn.size() == mcount, "VecNas or VecNtn");
                    res += callback6((void*)0, vec_nas[i], vec_ntn[i]); break;
            case 7: JPCRE2_VECTOR_DATA_ASSERT(vec_num.size() == mcount && vec_nas.size() == mcount && vec_ntn.size() == mcount, "VecNum\n or VecNas or VecNtn");
                    res += callback7(vec_num[i], vec_nas[i], vec_ntn[i]); break;
            default: JPCRE2_ASSERT(2 == 1, "Invalid callbackn. Please file a bug report (must include the line number from below)."); break;
        }
        //reset the current offset
        current_offset = vec_eoff[i];
        if(counter) *counter += 1;
        //if FIND_ALL is not set, single match will be performd
        if((RegexMatch::getJpcre2Option() & FIND_ALL) == 0) break;
    }
    //All matched parts have been dealt with.
    //now copy rest of the string from current_offset
    res += RegexMatch::getSubject().substr(current_offset, String::npos);
    return res;
}


#ifdef JPCRE2_USE_MINIMUM_CXX_11
template<typename Char_T, template<typename...> class Map>
typename jpcre2::select<Char_T, Map>::String jpcre2::select<Char_T, Map>::RegexReplace::replace() {
#else
template<typename Char_T>
typename jpcre2::select<Char_T>::String jpcre2::select<Char_T>::RegexReplace::replace() {
#endif
    *last_replace_counter = 0;

    // If re or re->code is null, return the subject string unmodified.
    if (!re || re->code == 0)
        return *r_subject_ptr;

    Pcre2Sptr subject = (Pcre2Sptr) r_subject_ptr->c_str();
    PCRE2_SIZE subject_length = r_subject_ptr->length();
    Pcre2Sptr replace = (Pcre2Sptr) r_replw_ptr->c_str();
    PCRE2_SIZE replace_length = r_replw_ptr->length();
    PCRE2_SIZE outlengthptr = (PCRE2_SIZE) buffer_size;
    bool retry = true;
    int ret = 0;
    Pcre2Uchar* output_buffer = new Pcre2Uchar[outlengthptr + 1]();

    while (true) {
        ret = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::substitute(
                    re->code,               /*Points to the compiled pattern*/
                    subject,                /*Points to the subject string*/
                    subject_length,         /*Length of the subject string*/
                    _start_offset,          /*Offset in the subject at which to start matching*/
                    replace_opts,           /*Option bits*/
                    mdata,                  /*Points to a match data block, or is NULL*/
                    mcontext,               /*Points to a match context, or is NULL*/
                    replace,                /*Points to the replacement string*/
                    replace_length,         /*Length of the replacement string*/
                    output_buffer,          /*Points to the output buffer*/
                    &outlengthptr           /*Points to the length of the output buffer*/
                    );

        if (ret < 0) {
            //Handle errors
            if ((replace_opts & PCRE2_SUBSTITUTE_OVERFLOW_LENGTH) != 0
                    && ret == (int) PCRE2_ERROR_NOMEMORY && retry) {
                retry = false;
                /// If initial #buffer_size wasn't big enough for resultant string,
                /// we will try once more with a new buffer size adjusted to the length of the resultant string.
                delete[] output_buffer;
                output_buffer = new Pcre2Uchar[outlengthptr + 1]();
                // Go and try to perform the substitute again
                continue;
            } else {
                error_number = ret;
                delete[] output_buffer;
                return *r_subject_ptr;
            }
        }
        //If everything's ok exit the loop
        break;
    }
    *last_replace_counter += ret;
    String result = String((Char*) output_buffer,(Char*) (output_buffer + outlengthptr) );
    delete[] output_buffer;
    return result;
}


#ifdef JPCRE2_USE_MINIMUM_CXX_11
template<typename Char_T, template<typename...> class Map>
bool jpcre2::select<Char_T, Map>::RegexMatch::getNumberedSubstrings(int rc, Pcre2Sptr subject, PCRE2_SIZE* ovector, uint32_t ovector_count) {
#else
template<typename Char_T>
bool jpcre2::select<Char_T>::RegexMatch::getNumberedSubstrings(int rc, Pcre2Sptr subject, PCRE2_SIZE* ovector, uint32_t ovector_count) {
#endif
    NumSub num_sub;
    uint32_t rcu = rc;
    num_sub.reserve(rcu); //we know exactly how many elements it will have.
    uint32_t i;
    for (i = 0u; i < ovector_count; i++) {
        if (ovector[2*i] != PCRE2_UNSET)
            num_sub.push_back(String((Char*)(subject + ovector[2*i]), ovector[2*i+1] - ovector[2*i]));
        else
        #ifdef JPCRE2_UNSET_CAPTURES_NULL
            num_sub.push_back(std::nullopt);
        #else
            num_sub.push_back(String());
        #endif
    }
    vec_num->push_back(num_sub); //this function shouldn't be called if this vector is null
    return true;
}


#ifdef JPCRE2_USE_MINIMUM_CXX_11
template<typename Char_T, template<typename...> class Map>
bool jpcre2::select<Char_T, Map>::RegexMatch::getNamedSubstrings(int namecount, int name_entry_size,
                                                            Pcre2Sptr name_table,
                                                            Pcre2Sptr subject, PCRE2_SIZE* ovector ) {
#else
template<typename Char_T>
bool jpcre2::select<Char_T>::RegexMatch::getNamedSubstrings(int namecount, int name_entry_size,
                                                            Pcre2Sptr name_table,
                                                            Pcre2Sptr subject, PCRE2_SIZE* ovector ) {
#endif
    Pcre2Sptr tabptr = name_table;
    String key;
    MapNas map_nas;
    MapNtN map_ntn;
    for (int i = 0; i < namecount; i++) {
        int n;
        if(sizeof( Char_T ) * CHAR_BIT == 8){
            n = (int)((tabptr[0] << 8) | tabptr[1]);
            key = toString((Char*) (tabptr + 2));
        }
        else{
            n = (int)tabptr[0];
            key = toString((Char*) (tabptr + 1));
        }
        //Use of tabptr is finished for this iteration, let's increment it now.
        tabptr += name_entry_size;
        String value((Char*)(subject + ovector[2*n]), ovector[2*n+1] - ovector[2*n]); //n, not i.
        if(vec_nas) map_nas[key] = value;
        if(vec_ntn) map_ntn[key] = n;
    }
    //push the maps into vectors:
    if(vec_nas) vec_nas->push_back(map_nas);
    if(vec_ntn) vec_ntn->push_back(map_ntn);
    return true;
}


#ifdef JPCRE2_USE_MINIMUM_CXX_11
template<typename Char_T, template<typename...> class Map>
jpcre2::SIZE_T jpcre2::select<Char_T, Map>::RegexMatch::match() {
#else
template<typename Char_T>
jpcre2::SIZE_T jpcre2::select<Char_T>::RegexMatch::match() {
#endif

    // If re or re->code is null, return 0 as the match count
    if (!re || re->code == 0)
        return 0;

    Pcre2Sptr subject = (Pcre2Sptr) m_subject_ptr->c_str();
    Pcre2Sptr name_table = 0;
    int crlf_is_newline = 0;
    int namecount = 0;
    int name_entry_size = 0;
    int rc = 0;
    uint32_t ovector_count = 0;
    int utf = 0;
    SIZE_T count = 0;
    Uint option_bits;
    Uint newline = 0;
    PCRE2_SIZE *ovector = 0;
    SIZE_T subject_length = 0;
    MatchData *match_data = 0;
    subject_length = m_subject_ptr->length();
    bool mdc = false; //mdata created.


    if (vec_num) vec_num->clear();
    if (vec_nas) vec_nas->clear();
    if (vec_ntn) vec_ntn->clear();
    if(vec_soff) vec_soff->clear();
    if(vec_eoff) vec_eoff->clear();


    /* Using this function ensures that the block is exactly the right size for
     the number of capturing parentheses in the pattern. */
    if(mdata) match_data = mdata;
    else {
        match_data = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::match_data_create_from_pattern(re->code, 0);
        mdc = true;
    }

    rc = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::match(  re->code,       /* the compiled pattern */
                                subject,        /* the subject string */
                                subject_length, /* the length of the subject */
                                _start_offset,  /* start at offset 'start_offset' in the subject */
                                match_opts,     /* default options */
                                match_data,     /* block for storing the result */
                                mcontext);      /* use default match context */

    /* Matching failed: handle error cases */

    if (rc < 0) {
        if(mdc)
            Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::match_data_free(match_data); /* Release memory used for the match */
        //must not free code. This function has no right to modify regex
        switch (rc) {
            case PCRE2_ERROR_NOMATCH:
                return count;
                /*
                 Handle other special cases if you like
                 */
            default:;
        }
        error_number = rc;
        return count;
    }

    ++count; //Increment the counter
    /* Match succeded. Get a pointer to the output vector, where string offsets are
     stored. */
    ovector = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::get_ovector_pointer(match_data);
    ovector_count = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::get_ovector_count(match_data);

    /************************************************************************//*
     * We have found the first match within the subject string. If the output *
     * vector wasn't big enough, say so. Then output any substrings that were *
     * captured.                                                              *
     *************************************************************************/

    /* The output vector wasn't big enough. This should not happen, because we used
     pcre2_match_data_create_from_pattern() above. */

    if (rc == 0) {
        //ovector was not big enough for all the captured substrings;
        error_number = (int)ERROR::INSUFFICIENT_OVECTOR;
        rc = ovector_count;
        // TODO: We may throw exception at this point.
    }
    //match succeeded at offset ovector[0]
    if(vec_soff) vec_soff->push_back(ovector[0]);
    if(vec_eoff) vec_eoff->push_back(ovector[1]);

    // Get numbered substrings if vec_num isn't null
    if (vec_num) { //must do null check
        if(!getNumberedSubstrings(rc, subject, ovector, ovector_count))
            return count;
    }

    //get named substrings if either vec_nas or vec_ntn is given.
    if (vec_nas || vec_ntn) {
        /* See if there are any named substrings, and if so, show them by name. First
         we have to extract the count of named parentheses from the pattern. */

        (void) Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::pattern_info( re->code,               /* the compiled pattern */
                                            PCRE2_INFO_NAMECOUNT,   /* get the number of named substrings */
                                            &namecount);            /* where to put the answer */

        if (namecount <= 0); /*No named substrings*/

        else {
            /* Before we can access the substrings, we must extract the table for
             translating names to numbers, and the size of each entry in the table. */

            (void) Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::pattern_info( re->code,               /* the compiled pattern */
                                                PCRE2_INFO_NAMETABLE,   /* address of the table */
                                                &name_table);           /* where to put the answer */

            (void) Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::pattern_info( re->code,                   /* the compiled pattern */
                                                PCRE2_INFO_NAMEENTRYSIZE,   /* size of each entry in the table */
                                                &name_entry_size);          /* where to put the answer */

            /* Now we can scan the table and, for each entry, print the number, the name,
             and the substring itself. In the 8-bit library the number is held in two
             bytes, most significant first. */


            // Get named substrings if vec_nas isn't null.
            // Get name to number map if vec_ntn isn't null.
        }
        //the following must be outside the above if-else
        if(!getNamedSubstrings(namecount, name_entry_size, name_table, subject, ovector))
            return count;
    }

    /***********************************************************************//*
     * If the "g" modifier was given, we want to continue                     *
     * to search for additional matches in the subject string, in a similar   *
     * way to the /g option in Perl. This turns out to be trickier than you   *
     * might think because of the possibility of matching an empty string.    *
     * What happens is as follows:                                            *
     *                                                                        *
     * If the previous match was NOT for an empty string, we can just start   *
     * the next match at the end of the previous one.                         *
     *                                                                        *
     * If the previous match WAS for an empty string, we can't do that, as it *
     * would lead to an infinite loop. Instead, a call of pcre2_match() is    *
     * made with the PCRE2_NOTEMPTY_ATSTART and PCRE2_ANCHORED flags set. The *
     * first of these tells PCRE2 that an empty string at the start of the    *
     * subject is not a valid match; other possibilities must be tried. The   *
     * second flag restricts PCRE2 to one match attempt at the initial string *
     * position. If this match succeeds, an alternative to the empty string   *
     * match has been found, and we can print it and proceed round the loop,  *
     * advancing by the length of whatever was found. If this match does not  *
     * succeed, we still stay in the loop, advancing by just one character.   *
     * In UTF-8 mode, which can be set by (*UTF) in the pattern, this may be  *
     * more than one byte.                                                    *
     *                                                                        *
     * However, there is a complication concerned with newlines. When the     *
     * newline convention is such that CRLF is a valid newline, we must       *
     * advance by two characters rather than one. The newline convention can  *
     * be set in the regex by (*CR), etc.; if not, we must find the default.  *
     *************************************************************************/

    if ((jpcre2_match_opts & FIND_ALL) == 0) {
        if(mdc)
            Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::match_data_free(match_data); /* Release the memory that was used */
        // Must not free code. This function has no right to modify regex.
        return count; /* Exit the program. */
    }

    /* Before running the loop, check for UTF-8 and whether CRLF is a valid newline
     sequence. First, find the options with which the regex was compiled and extract
     the UTF state. */

    (void) Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::pattern_info(re->code, PCRE2_INFO_ALLOPTIONS, &option_bits);
    utf = ((option_bits & PCRE2_UTF) != 0);

    /* Now find the newline convention and see whether CRLF is a valid newline
     sequence. */

    (void) Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::pattern_info(re->code, PCRE2_INFO_NEWLINE, &newline);
    crlf_is_newline = newline == PCRE2_NEWLINE_ANY
            || newline == PCRE2_NEWLINE_CRLF
            || newline == PCRE2_NEWLINE_ANYCRLF;

    /** We got the first match. Now loop for second and subsequent matches. */

    for (;;) {

        Uint options = match_opts; /* Normally no options */
        PCRE2_SIZE start_offset = ovector[1]; /* Start at end of previous match */

        /* If the previous match was for an empty string, we are finished if we are
         at the end of the subject. Otherwise, arrange to run another match at the
         same point to see if a non-empty match can be found. */

        if (ovector[0] == ovector[1]) {
            if (ovector[0] == subject_length)
                break;
            options |= PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
        }

        /// Run the next matching operation */

        rc = Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::match(  re->code,       /* the compiled pattern */
                                    subject,        /* the subject string */
                                    subject_length, /* the length of the subject */
                                    start_offset,   /* starting offset in the subject */
                                    options,        /* options */
                                    match_data,     /* block for storing the result */
                                    mcontext);      /* use match context */

        /* This time, a result of NOMATCH isn't an error. If the value in "options"
         is zero, it just means we have found all possible matches, so the loop ends.
         Otherwise, it means we have failed to find a non-empty-string match at a
         point where there was a previous empty-string match. In this case, we do what
         Perl does: advance the matching position by one character, and continue. We
         do this by setting the "end of previous match" offset, because that is picked
         up at the top of the loop as the point at which to start again.

         There are two complications: (a) When CRLF is a valid newline sequence, and
         the current position is just before it, advance by an extra byte. (b)
         Otherwise we must ensure that we skip an entire UTF character if we are in
         UTF mode. */

        if (rc == PCRE2_ERROR_NOMATCH) {
            if (options == 0)
                break;                          /* All matches found */
            ovector[1] = start_offset + 1; /* Advance one code unit */
            if (crlf_is_newline &&                      /* If CRLF is newline & */
                start_offset < subject_length - 1 &&    /* we are at CRLF, */
                subject[start_offset] == '\r' && subject[start_offset + 1] == '\n')
                ovector[1] += 1;                        /* Advance by one more. */
            else if (utf) { /* advance a whole UTF (8 or 16), for UTF-32, it's not needed */
                while (ovector[1] < subject_length) {
                    if(sizeof( Char_T ) * CHAR_BIT == 8 && (subject[ovector[1]] & 0xc0) != 0x80) break;
                    else if(sizeof( Char_T ) * CHAR_BIT == 16 && (subject[ovector[1]] & 0xfc00) != 0xdc00) break;
                    else if(sizeof( Char_T ) * CHAR_BIT == 32) break; //must be else if
                    ovector[1] += 1;
                }
            }
            continue; /* Go round the loop again */
        }

        /* Other matching errors are not recoverable. */

        if (rc < 0) {
            if(mdc)
                Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::match_data_free(match_data);
            // Must not free code. This function has no right to modify regex.
            error_number = rc;
            return count;
        }

        /* match succeeded */
        ++count; //Increment the counter

        if (rc == 0) {
            /* The match succeeded, but the output vector wasn't big enough. This
             should not happen. */
            error_number = (int)ERROR::INSUFFICIENT_OVECTOR;
            rc = ovector_count;
            // TODO: We may throw exception at this point.
        }

        //match succeded at ovector[0]
        if(vec_soff) vec_soff->push_back(ovector[0]);
        if(vec_eoff) vec_eoff->push_back(ovector[1]);

        /* As before, get substrings stored in the output vector by number, and then
         also any named substrings. */

        // Get numbered substrings if vec_num isn't null
        if (vec_num) { //must do null check
            if(!getNumberedSubstrings(rc, subject, ovector, ovector_count))
                return count;
        }

        if (vec_nas || vec_ntn) {
            //must call this whether we have named substrings or not:
            if(!getNamedSubstrings(namecount, name_entry_size, name_table, subject, ovector))
                return count;
        }
    } /* End of loop to find second and subsequent matches */

    if(mdc)
        Pcre2Func<sizeof( Char_T ) * CHAR_BIT>::match_data_free(match_data);
    // Must not free code. This function has no right to modify regex.
    return count;
}

#undef JPCRE2_VECTOR_DATA_ASSERT
#undef JPCRE2_UNUSED
#undef JPCRE2_USE_MINIMUM_CXX_11

//some macro documentation for doxygen

#ifdef __DOXYGEN__


#ifndef JPCRE2_USE_FUNCTION_POINTER_CALLBACK
#define JPCRE2_USE_FUNCTION_POINTER_CALLBACK
#endif

#ifndef JPCRE2_NDEBUG
#define JPCRE2_NDEBUG
#endif


///@def JPCRE2_USE_FUNCTION_POINTER_CALLBACK
///Use function pointer in all cases for MatchEvaluatorCallback function.
///By default function pointer is used for callback in MatchEvaluator when using <C++11 compiler, but for
///`>=C++11` compiler `std::function` instead of function pointer is used.
///If this macro is defined before including jpcre2.hpp, function pointer will be used in all cases.
///It you are using lambda function with captures, stick with `std::function`, on the other hand, if
///you are using older compilers, you might want to use function pointer instead.
///
///For example, with gcc-4.7, `std::function` will give compile error in C++11 mode, in such cases where full C++11
///support is not available, use function pointer.


///@def JPCRE2_ASSERT(cond, msg)
///Macro to call `jpcre2::jassert()` with file path and line number.
///When `NDEBUG` or `JPCRE2_NDEBUG` is defined before including this header, this macro will
///be defined as `((void)0)` thus eliminating this assertion.
///@param cond condtion (boolean)
///@param msg message


///@def JPCRE2_NDEBUG
///Macro to remove debug codes.
///Using this macro is discouraged even in production mode but provided for completeness.
///You should not use this macro to bypass any error in your program.
///Define this macro before including this header if you want to remove debug codes included in this library.
///
///Using the standard `NDEBUG` macro will have the same effect,
///but it is recommended that you use `JPCRE2_NDEBUG` to strip out debug codes specifically for this library.


///@def JPCRE2_UNSET_CAPTURES_NULL
///Define to change the type of NumSub so that captures are recorded
///with std::optional. It is undefined by default. This feature requires C++17.

#endif


#endif
