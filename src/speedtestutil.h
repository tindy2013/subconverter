#ifndef SPEEDTESTUTIL_H_INCLUDED
#define SPEEDTESTUTIL_H_INCLUDED

#include <string>

#include "misc.h"
#include "nodeinfo.h"

std::string vmessConstruct(const std::string &group, const std::string &remarks, const std::string &add, const std::string &port, const std::string &type, const std::string &id, const std::string &aid, const std::string &net, const std::string &cipher, const std::string &path, const std::string &host, const std::string &edge, const std::string &tls, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
std::string ssrConstruct(const std::string &group, const std::string &remarks, const std::string &remarks_base64, const std::string &server, const std::string &port, const std::string &protocol, const std::string &method, const std::string &obfs, const std::string &password, const std::string &obfsparam, const std::string &protoparam, bool libev, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
std::string ssConstruct(const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &method, const std::string &plugin, const std::string &pluginopts, bool libev, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
std::string socksConstruct(const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &username, const std::string &password, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
std::string httpConstruct(const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &username, const std::string &password, bool tls, tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
std::string trojanConstruct(const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &host, bool tlssecure, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
std::string snellConstruct(const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &obfs, const std::string &host, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
void explodeVmess(std::string vmess, const std::string &custom_port, nodeInfo &node);
void explodeSSR(std::string ssr, bool ss_libev, bool libev, const std::string &custom_port, nodeInfo &node);
void explodeSS(std::string ss, bool libev, const std::string &custom_port, nodeInfo &node);
void explodeTrojan(std::string trojan, const std::string &custom_port, nodeInfo &node);
void explodeQuan(const std::string &quan, const std::string &custom_port, nodeInfo &node);
void explodeStdVMess(std::string vmess, const std::string &custom_port, nodeInfo &node);
void explodeShadowrocket(std::string kit, const std::string &custom_port, nodeInfo &node);
void explodeKitsunebi(std::string kit, const std::string &custom_port, nodeInfo &node);
/// Parse a link
void explode(const std::string &link, bool sslibev, bool ssrlibev, const std::string &custom_port, nodeInfo &node);
void explodeSSD(std::string link, bool libev, const std::string &custom_port, std::vector<nodeInfo> &nodes);
void explodeSub(std::string sub, bool sslibev, bool ssrlibev, const std::string &custom_port, std::vector<nodeInfo> &nodes);
int explodeConf(std::string filepath, const std::string &custom_port, bool sslibev, bool ssrlibev, std::vector<nodeInfo> &nodes);
int explodeConfContent(const std::string &content, const std::string &custom_port, bool sslibev, bool ssrlibev, std::vector<nodeInfo> &nodes);
bool chkIgnore(const nodeInfo &node, string_array &exclude_remarks, string_array &include_remarks);
void filterNodes(std::vector<nodeInfo> &nodes, string_array &exclude_remarks, string_array &include_remarks, int groupID);
bool getSubInfoFromHeader(const std::string &header, std::string &result);
bool getSubInfoFromNodes(const std::vector<nodeInfo> &nodes, const string_array &stream_rules, const string_array &time_rules, std::string &result);
bool getSubInfoFromSSD(const std::string &sub, std::string &result);
unsigned long long streamToInt(const std::string &stream);

#endif // SPEEDTESTUTIL_H_INCLUDED
