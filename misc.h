#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <string>
#include <vector>
#include <sstream>

#include <yaml-cpp/yaml.h>

#ifdef _WIN32
#define PATH_SLASH "\\"
#else
#define PATH_SLASH "//"
#endif // _WIN32

typedef std::vector<std::string> string_array;

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string UrlDecode(const std::string& str);
std::string base64_decode(std::string encoded_string);
std::string base64_encode(std::string string_to_encode);

std::vector<std::string> split(const std::string &s, const std::string &seperator);
std::string getUrlArg(std::string url, std::string request);
std::string replace_all_distinct(std::string str, std::string old_value, std::string new_value);
std::string urlsafe_base64_reverse(std::string encoded_string);
std::string urlsafe_base64_decode(std::string encoded_string);
std::string UTF8ToGBK(std::string str_src);
std::string GBKToUTF8(std::string str_src);
std::string trim(const std::string& str);
std::string getSystemProxy();
std::string rand_str(const int len);
bool is_str_utf8(std::string data);
std::string getFormData(std::string &raw_data);

void sleep(int interval);
bool regFind(std::string src, std::string target);
std::string regReplace(std::string src, std::string match, std::string rep);
bool regMatch(std::string src, std::string match);
std::string speedCalc(double speed);
std::string getMD5(std::string data);
bool isIPv4(std::string address);
bool isIPv6(std::string address);
void urlParse(std::string url, std::string &host, std::string &path, int &port, bool &isTLS);
void removeUTF8BOM(std::string &data);
int shortAssemble(unsigned short num_a, unsigned short num_b);
void shortDisassemble(int source, unsigned short &num_a, unsigned short &num_b);
std::string to_string(YAML::Node &node);

std::string fileGet(std::string path, bool binary = true);
int fileWrite(std::string path, std::string content, bool overwrite);
bool fileExist(std::string path);
bool fileCopy(std::string source,std::string dest);
std::string fileToBase64(std::string filepath);
std::string fileGetMD5(std::string filepath);

static inline bool strFind(std::string str, std::string target)
{
    return str.find(target) != str.npos;
}

template <typename T> static inline void eraseElements(T &target)
{
    T().swap(target);
}

#ifdef _MACOS
namespace std
{
    namespace __cxx11
    {
        template <typename T> std::string to_string(const T& n)
        {
            std::ostringstream ss;
            ss << n ;
            return ss.str();
        }
    }
}
#endif // _MACOS

#ifdef _WIN32
void StringToWstring(std::wstring& szDst, std::string str);
#endif // _WIN32

#endif // MISC_H_INCLUDED
