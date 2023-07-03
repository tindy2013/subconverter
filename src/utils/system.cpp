#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include "string.h"

void sleepMs(int interval)
{
    /*
    #ifdef _WIN32
        Sleep(interval);
    #else
        // Portable sleep for platforms other than Windows.
        struct timeval wait = { 0, interval * 1000 };
        select(0, NULL, NULL, NULL, &wait);
    #endif
    */
    //upgrade to c++11 standard
    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
}

std::string getEnv(const std::string &name)
{
    std::string retVal;
#ifdef _WIN32
    char chrData[1024] = {};
    if(GetEnvironmentVariable(name.c_str(), chrData, 1023))
        retVal.assign(chrData);
#else
    char *env = getenv(name.c_str());
    if(env != NULL)
        retVal.assign(env);
#endif // _WIN32
    return retVal;
}

std::string getSystemProxy()
{
#ifdef _WIN32
    HKEY key;
    auto ret = RegOpenKeyEx(HKEY_CURRENT_USER, R"(Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings)", 0, KEY_ALL_ACCESS, &key);
    if(ret != ERROR_SUCCESS)
    {
        //std::cout << "open failed: " << ret << std::endl;
        return "";
    }

    DWORD values_count, max_value_name_len, max_value_len;
    ret = RegQueryInfoKey(key, NULL, NULL, NULL, NULL, NULL, NULL,
                          &values_count, &max_value_name_len, &max_value_len, NULL, NULL);
    if(ret != ERROR_SUCCESS)
    {
        //std::cout << "query failed" << std::endl;
        return "";
    }

    std::vector<std::tuple<std::shared_ptr<char>, DWORD, std::shared_ptr<BYTE>>> values;
    for(DWORD i = 0; i < values_count; i++)
    {
        std::shared_ptr<char> value_name(new char[max_value_name_len + 1],
                                         std::default_delete<char[]>());
        DWORD value_name_len = max_value_name_len + 1;
        DWORD value_type, value_len;
        RegEnumValue(key, i, value_name.get(), &value_name_len, NULL, &value_type, NULL, &value_len);
        std::shared_ptr<BYTE> value(new BYTE[value_len],
                                    std::default_delete<BYTE[]>());
        value_name_len = max_value_name_len + 1;
        RegEnumValue(key, i, value_name.get(), &value_name_len, NULL, &value_type, value.get(), &value_len);
        values.push_back(std::make_tuple(value_name, value_type, value));
    }

    DWORD ProxyEnable = 0;
    for (auto x : values)
    {
        if (strcmp(std::get<0>(x).get(), "ProxyEnable") == 0)
        {
            ProxyEnable = *(DWORD*)(std::get<2>(x).get());
        }
    }

    if (ProxyEnable)
    {
        for (auto x : values)
        {
            if (strcmp(std::get<0>(x).get(), "ProxyServer") == 0)
            {
                //std::cout << "ProxyServer: " << (char*)(std::get<2>(x).get()) << std::endl;
                return std::string((char*)(std::get<2>(x).get()));
            }
        }
    }
    /*
    else {
    	//std::cout << "Proxy not Enabled" << std::endl;
    }
    */
    //return 0;
    return "";
#else
    string_array proxy_env = {"all_proxy", "ALL_PROXY", "http_proxy", "HTTP_PROXY", "https_proxy", "HTTPS_PROXY"};
    for(std::string &x : proxy_env)
    {
        char* proxy = getenv(x.c_str());
        if(proxy != NULL)
            return std::string(proxy);
    }
    return "";
#endif // _WIN32
}
