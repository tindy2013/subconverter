#ifndef SUBPARSER_H_INCLUDED
#define SUBPARSER_H_INCLUDED

#include <string>

#include "config/proxy.h"

enum class ConfType
{
    Unknow,
    SS,
    SSR,
    V2Ray,
    SSConf,
    SSTap,
    Netch,
    SOCKS,
    HTTP,
    SUB,
    Local
};
void hysteriaConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &add, const std::string &port, const std::string &type, const std::string &auth, const std::string &host, const std::string &up, const std::string &down, const std::string &alpn, const std::string &obfsParam, const std::string &insecure ,tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
void hysteria2Construct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &add, const std::string &port, const std::string &password, const std::string &host, const std::string &up, const std::string &down, const std::string &alpn, const std::string &obfsParam, const std::string &obfsPassword, const std::string &insecure ,tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
void vlessConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &add, const std::string &port, const std::string &type, const std::string &id, const std::string &aid, const std::string &net, const std::string &cipher, const std::string &flow, const std::string &mode, const std::string &path, const std::string &host, const std::string &edge, const std::string &tls,const std::string &pkd, const std::string &sid, const std::string &fp, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
void vmessConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &add, const std::string &port, const std::string &type, const std::string &id, const std::string &aid, const std::string &net, const std::string &cipher, const std::string &path, const std::string &host, const std::string &edge, const std::string &tls, const std::string &sni, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
void ssrConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &protocol, const std::string &method, const std::string &obfs, const std::string &password, const std::string &obfsparam, const std::string &protoparam, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
void ssConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &method, const std::string &plugin, const std::string &pluginopts, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
void socksConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &username, const std::string &password, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
void httpConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &username, const std::string &password, bool tls, tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
void trojanConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &network, const std::string &host, const std::string &path, bool tlssecure, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool(), tribool tls13 = tribool());
void snellConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &obfs, const std::string &host, uint16_t version = 0, tribool udp = tribool(), tribool tfo = tribool(), tribool scv = tribool());
void explodeVmess(std::string vmess, Proxy &node);
void explodeSSR(std::string ssr, Proxy &node);
void explodeSS(std::string ss, Proxy &node);
void explodeTrojan(std::string trojan, Proxy &node);
void explodeQuan(const std::string &quan, Proxy &node);
void explodeStdVMess(std::string vmess, Proxy &node);
void explodeStdVless(std::string vless, Proxy &node);
void explodeStdHysteria(std::string hysteria, Proxy &node);
void explodeStdHysteria2(std::string hysteria2, Proxy &node);
void explodeShadowrocket(std::string kit, Proxy &node);
void explodeKitsunebi(std::string kit, Proxy &node);
void explodeVless(std::string vless, Proxy &node);
void explodeHysteria(std::string hysteria, Proxy &node);
void explodeHysteria2(std::string hysteria2, Proxy &node);
/// Parse a link
void explode(const std::string &link, Proxy &node);
void explodeSSD(std::string link, std::vector<Proxy> &nodes);
void explodeSub(std::string sub, std::vector<Proxy> &nodes);
int explodeConf(const std::string &filepath, std::vector<Proxy> &nodes);
int explodeConfContent(const std::string &content, std::vector<Proxy> &nodes);

#endif // SUBPARSER_H_INCLUDED
