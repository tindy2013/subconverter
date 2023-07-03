#include <string>
#include <vector>
#include <sstream>

#include "../server/socket.h"
#include "string.h"
#include "regexp.h"

std::string hostnameToIPAddr(const std::string &host)
{
    int retVal;
    std::string retAddr;
    char cAddr[128] = {};
    struct sockaddr_in *target;
    struct sockaddr_in6 *target6;
    struct addrinfo hint = {}, *retAddrInfo, *cur;
    retVal = getaddrinfo(host.data(), NULL, &hint, &retAddrInfo);
    if(retVal != 0)
    {
        freeaddrinfo(retAddrInfo);
        return "";
    }

    for(cur = retAddrInfo; cur != NULL; cur = cur->ai_next)
    {
        if(cur->ai_family == AF_INET)
        {
            target = reinterpret_cast<struct sockaddr_in *>(cur->ai_addr);
            inet_ntop(AF_INET, &target->sin_addr, cAddr, sizeof(cAddr));
            break;
        }
        else if(cur->ai_family == AF_INET6)
        {
            target6 = reinterpret_cast<struct sockaddr_in6 *>(cur->ai_addr);
            inet_ntop(AF_INET6, &target6->sin6_addr, cAddr, sizeof(cAddr));
            break;
        }
    }
    retAddr.assign(cAddr);
    freeaddrinfo(retAddrInfo);
    return retAddr;
}

bool isIPv4(const std::string &address)
{
    return regMatch(address, "^(25[0-5]|2[0-4]\\d|[0-1]?\\d?\\d)(\\.(25[0-5]|2[0-4]\\d|[0-1]?\\d?\\d)){3}$");
}

bool isIPv6(const std::string &address)
{
    std::vector<std::string> regLists = {"^(?:[0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$", "^((?:[0-9A-Fa-f]{1,4}(:[0-9A-Fa-f]{1,4})*)?)::((?:([0-9A-Fa-f]{1,4}:)*[0-9A-Fa-f]{1,4})?)$", "^(::(?:[0-9A-Fa-f]{1,4})(?::[0-9A-Fa-f]{1,4}){5})|((?:[0-9A-Fa-f]{1,4})(?::[0-9A-Fa-f]{1,4}){5}::)$"};
    for(unsigned int i = 0; i < regLists.size(); i++)
    {
        if(regMatch(address, regLists[i]))
            return true;
    }
    return false;
}

void urlParse(std::string &url, std::string &host, std::string &path, int &port, bool &isTLS)
{
    std::vector<std::string> args;
    string_size pos;

    if(regMatch(url, "^https://(.*)"))
        isTLS = true;
    url = regReplace(url, "^(http|https)://", "");
    pos = url.find("/");
    if(pos == url.npos)
    {
        host = url;
        path = "/";
    }
    else
    {
        host = url.substr(0, pos);
        path = url.substr(pos);
    }
    pos = host.rfind(":");
    if(regFind(host, "\\[(.*)\\]")) //IPv6
    {
        args = split(regReplace(host, "\\[(.*)\\](.*)", "$1,$2"), ",");
        if(args.size() == 2) //with port
            port = to_int(args[1].substr(1));
        host = args[0];
    }
    else if(pos != host.npos)
    {
        port = to_int(host.substr(pos + 1));
        host = host.substr(0, pos);
    }
    if(port == 0)
    {
        if(isTLS)
            port = 443;
        else
            port = 80;
    }
}

std::string getFormData(const std::string &raw_data)
{
    std::stringstream strstrm;
    std::string line;

    std::string boundary;
    std::string file; /* actual file content */

    int i = 0;

    strstrm<<raw_data;

    while (std::getline(strstrm, line))
    {
        if(i == 0)
            boundary = line.substr(0, line.length() - 1); // Get boundary
        else if(startsWith(line, boundary))
            break; // The end
        else if(line.length() == 1)
        {
            // Time to get raw data
            char c;
            int bl = boundary.length();
            bool endfile = false;
            char buffer[256];
            while(!endfile)
            {
                int j = 0;
                while(j < 256 && strstrm.get(c) && !endfile)
                {
                    buffer[j] = c;
                    int k = 0;
                    // Verify if we are at the end
                    while(boundary[bl - 1 - k] == buffer[j - k])
                    {
                        if(k >= bl - 1)
                        {
                            // We are at the end of the file
                            endfile = true;
                            break;
                        }
                        k++;
                    }
                    j++;
                }
                file.append(buffer, j);
                j = 0;
            };
            file.erase(file.length() - bl);
            break;
        }
        i++;
    }
    return file;
}
