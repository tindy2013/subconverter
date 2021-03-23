#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

// ANSI code page (GBK on 936) to UTF8
std::string acpToUTF8(const std::string &str_src)
{
#ifdef _WIN32
    const char* strGBK = str_src.c_str();
    int len = MultiByteToWideChar(CP_ACP, 0, strGBK, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len + 1];
    memset(wstr, 0, len + 1);
    MultiByteToWideChar(CP_ACP, 0, strGBK, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[len + 1];
    memset(str, 0, len + 1);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    std::string strTemp = str;
    delete[] wstr;
    delete[] str;
    return strTemp;
#else
    return str_src;
#endif // _WIN32
    /*
    std::vector<wchar_t> buffer(str_src.size());
#ifdef _MSC_VER
    std::locale loc("zh-CN");
#else
    std::locale loc{"zh_CN.GB2312"};
#endif // _MSC_VER
    wchar_t *pwszNew = nullptr;
    const char *pszNew = nullptr;
    mbstate_t state = {};
    int res = std::use_facet<std::codecvt<wchar_t, char, mbstate_t> >
    (loc).in(state, str_src.data(), str_src.data() + str_src.size(), pszNew,
    buffer.data(), buffer.data() + buffer.size(), pwszNew);

    if(res == std::codecvt_base::ok)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> cutf8;
        return cutf8.to_bytes(std::wstring(buffer.data(), pwszNew));
    }
    return str_src;
    */
}

// UTF8 to ANSI code page (GBK on 936)
std::string utf8ToACP(const std::string &str_src)
{
#ifdef _WIN32
    const char* strUTF8 = str_src.data();
    int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, NULL, 0);
    wchar_t* wszGBK = new wchar_t[len + 1];
    memset(wszGBK, 0, len * 2 + 2);
    MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, wszGBK, len);
    len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
    char* szGBK = new char[len + 1];
    memset(szGBK, 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
    std::string strTemp(szGBK);
    if (wszGBK)
        delete[] wszGBK;
    if (szGBK)
        delete[] szGBK;
    return strTemp;
#else
    return str_src;
#endif
}

#ifdef _WIN32
// std::string to wstring
void stringToWstring(std::wstring& szDst, const std::string &str)
{
    std::string temp = str;
    int len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, NULL,0);
    wchar_t* wszUtf8 = new wchar_t[len + 1];
    memset(wszUtf8, 0, len * 2 + 2);
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, (LPWSTR)wszUtf8, len);
    szDst = wszUtf8;
    //std::wstring r = wszUtf8;
    delete[] wszUtf8;
}

std::string wstringToString(LPWSTR str)
{
    std::string result;
    size_t srclen = wcslen(str);
    size_t len = WideCharToMultiByte(CP_ACP, 0, str, srclen, 0, 0, 0, 0);
    result.resize(len);
    WideCharToMultiByte(CP_ACP, 0, str, srclen, result.data(), len, 0, 0);
    return result;
}
#else
/* Unimplemented: std::codecvt_utf8 */
#endif // _WIN32
