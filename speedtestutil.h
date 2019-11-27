#ifndef SPEEDTESTUTIL_H_INCLUDED
#define SPEEDTESTUTIL_H_INCLUDED

#include <string>
#include <yaml-cpp/yaml.h>

#include "misc.h"
#include "nodeinfo.h"

std::string vmessConstruct(std::string add, std::string port, std::string type, std::string id, std::string aid, std::string net, std::string cipher, std::string path, std::string host, std::string tls, int local_port);
std::string ssrConstruct(std::string group, std::string remarks, std::string remarks_base64, std::string server, std::string port, std::string protocol, std::string method, std::string obfs, std::string password, std::string obfsparam, std::string protoparam, int local_port, bool libev);
std::string ssConstruct(std::string server, std::string port, std::string password, std::string method, std::string plugin, std::string pluginopts, std::string remarks, int local_port, bool libev);
std::string socksConstruct(std::string remarks, std::string server, std::string port, std::string username, std::string password);
std::string httpConstruct(std::string remarks, std::string server, std::string port, std::string username, std::string password);
void explodeVmess(std::string vmess, std::string custom_port, int local_port, nodeInfo &node);
void explodeSSR(std::string ssr, bool ss_libev, bool libev, std::string custom_port, int local_port, nodeInfo &node);
void explodeSS(std::string ss, bool libev, std::string custom_port, int local_port, nodeInfo &node);
void explodeQuan(std::string quan, std::string custom_port, int local_port, nodeInfo &node);
void explodeShadowrocket(std::string kit, std::string custom_port, int local_port, nodeInfo &node);
void explodeKitsunebi(std::string kit, std::string custom_port, int local_port, nodeInfo &node);
void explode(std::string link, bool sslibev, bool ssrlibev, std::string custom_port, int local_port, nodeInfo &node);
void explodeSSD(std::string link, bool libev, std::string custom_port, int local_port, std::vector<nodeInfo> &nodes);
void explodeSub(std::string sub, bool sslibev, bool ssrlibev, std::string custom_port, int local_port, std::vector<nodeInfo> &nodes);
int explodeConf(std::string filepath, std::string custom_port, int local_port, bool sslibev, bool ssrlibev, std::vector<nodeInfo> &nodes);
int explodeConfContent(std::string content, std::string custom_port, int local_port, bool sslibev, bool ssrlibev, std::vector<nodeInfo> &nodes);
void remarksInit(string_array &exclude, string_array &include);
bool chkIgnore(nodeInfo &node);

#endif // SPEEDTESTUTIL_H_INCLUDED
