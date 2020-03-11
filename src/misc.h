#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <string>
#include <vector>
#include <sstream>

#include <yaml-cpp/yaml.h>

#ifdef _WIN32
#include <unistd.h>
#define PATH_SLASH "\\"
#else
#include <sys/types.h>
#include <sys/stat.h>
#define PATH_SLASH "//"
#endif // _WIN32

typedef std::string::size_type string_size;
typedef std::vector<std::string> string_array;
typedef const std::string &refCnstStr;

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string UrlEncode(const std::string& str);
std::string UrlDecode(const std::string& str);
std::string base64_decode(const std::string &encoded_string, bool accept_urlsafe = false);
std::string base64_encode(const std::string &string_to_encode);

std::vector<std::string> split(const std::string &s, const std::string &seperator);
std::string getUrlArg(const std::string &url, const std::string &request);
std::string replace_all_distinct(std::string str, const std::string &old_value, const std::string &new_value);
std::string urlsafe_base64(const std::string &encoded_string);
std::string urlsafe_base64_reverse(const std::string &encoded_string);
std::string urlsafe_base64_decode(const std::string &encoded_string);
std::string urlsafe_base64_encode(const std::string &string_to_encode);
std::string UTF8ToACP(const std::string &str_src);
std::string ACPToUTF8(const std::string &str_src);
std::string trim_of(const std::string& str, char target);
std::string trim(const std::string& str);
std::string trim_quote(const std::string &str);
std::string getSystemProxy();
std::string rand_str(const int len);
bool is_str_utf8(const std::string &data);
std::string getFormData(const std::string &raw_data);

void sleep(int interval);
bool regValid(const std::string &target);
bool regFind(const std::string &src, const std::string &match);
std::string regReplace(const std::string &src, const std::string &match, const std::string &rep);
bool regMatch(const std::string &src, const std::string &match);
std::string regTrim(const std::string &src);
std::string speedCalc(double speed);
std::string getMD5(const std::string &data);
bool isIPv4(const std::string &address);
bool isIPv6(const std::string &address);
void urlParse(const std::string &url, std::string &host, std::string &path, int &port, bool &isTLS);
void removeUTF8BOM(std::string &data);
int shortAssemble(unsigned short num_a, unsigned short num_b);
void shortDisassemble(int source, unsigned short &num_a, unsigned short &num_b);
std::string UTF8ToCodePoint(const std::string &data);
std::string GetEnv(const std::string &name);

std::string fileGet(const std::string &path, bool scope_limit = false);
int fileWrite(const std::string &path, const std::string &content, bool overwrite);
bool fileExist(const std::string &path);
bool fileCopy(const std::string &source, const std::string &dest);
std::string fileToBase64(const std::string &filepath);
std::string fileGetMD5(const std::string &filepath);

static inline bool strFind(const std::string &str, const std::string &target)
{
    return str.find(target) != str.npos;
}

static inline bool startsWith(const std::string &hay, const std::string &needle)
{
    return hay.substr(0, needle.length()) == needle;
}

static inline bool endsWith(const std::string &hay, const std::string &needle)
{
    std::string::size_type hl = hay.length(), nl = needle.length();
    return hl >= nl && hay.substr(hl - nl, nl) == needle;
}

template <typename T> static inline void eraseElements(std::vector<T> &target)
{
    target.clear();
    target.shrink_to_fit();
}

template <typename T> static inline void eraseElements(T &target)
{
    T().swap(target);
}

template <typename T> static inline T to_number(const std::string &str, T def_value)
{
    T retval = 0.0;
    char c;
    std::stringstream ss(str);
    if(!(ss >> retval))
        return def_value;
    else if(ss >> c)
        return def_value;
    else
        return retval;
}

int to_int(const std::string &str, int def_value = 0);

#ifndef HAVE_TO_STRING
namespace std
{
template <typename T> std::string to_string(const T& n)
{
    std::ostringstream ss;
    ss << n;
    return ss.str();
}
}
#endif // HAVE_TO_STRING

static inline int md(const char *path)
{
#ifdef _WIN32
    return mkdir(path);
#else
    return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif // _WIN32
}

#ifdef _WIN32
void StringToWstring(std::wstring& szDst, const std::string &str);
#endif // _WIN32

#endif // MISC_H_INCLUDED
