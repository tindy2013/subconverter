#ifndef SCRIPT_QUICKJS_H_INCLUDED
#define SCRIPT_QUICKJS_H_INCLUDED

#include "../parser/config/proxy.h"
#include "../utils/defer.h"

#ifndef NO_JS_RUNTIME

#include <quickjspp.hpp>

void script_runtime_init(qjs::Runtime &runtime);
int script_context_init(qjs::Context &context);
int script_cleanup(qjs::Context &context);
void script_print_stack(qjs::Context &context);

inline std::string JS_GetPropertyToString(JSContext *ctx, JSValue v, const char* prop)
{
    auto val = JS_GetPropertyStr(ctx, v, prop);
    auto valData = JS_ToCString(ctx, val);
    std::string result = valData;
    JS_FreeCString(ctx, valData);
    JS_FreeValue(ctx, val);
    return result;
}

inline int JS_GetPropertyToInt32(JSContext *ctx, JSValue v, const char* prop, int32_t def_value = 0)
{
    int32_t result = def_value;
    auto val = JS_GetPropertyStr(ctx, v, prop);
    int32_t ret = JS_ToInt32(ctx, &result, val);
    JS_FreeValue(ctx, val);
    if(ret != 0) return def_value;
    return result;
}

inline int JS_GetPropertyToUInt32(JSContext *ctx, JSValue v, const char* prop, uint32_t def_value = 0)
{
    uint32_t result = def_value;
    auto val = JS_GetPropertyStr(ctx, v, prop);
    int ret = JS_ToUint32(ctx, &result, val);
    JS_FreeValue(ctx, val);
    if(ret != 0) return def_value;
    return result;
}

inline bool JS_GetPropertyToBool(JSContext *ctx, JSValue v, const char* prop, bool def_value = false)
{
    bool result = def_value;
    auto val = JS_GetPropertyStr(ctx, v, prop);
    int ret = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, val);
    if(ret != 0) return def_value;
    return result;
}

namespace qjs
{
    template<>
    struct js_traits<tribool>
    {
        static JSValue wrap(JSContext *ctx, const tribool &t) noexcept
        {
            auto obj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, obj, "value", JS_NewBool(ctx, t.get()));
            JS_SetPropertyStr(ctx, obj, "isDefined", JS_NewBool(ctx, !t.is_undef()));
            return obj;
        }
        static tribool unwrap(JSContext *ctx, JSValueConst v)
        {
            tribool t;
            bool defined = JS_GetPropertyToBool(ctx, v, "isDefined");
            if(defined)
            {
                bool value = JS_GetPropertyToBool(ctx, v, "value");
                t.set(value);
            }
            return t;
        }
        static tribool JS_GetPropertyToTriBool(JSContext *ctx, JSValue v, const char* prop)
        {
            auto obj = JS_GetPropertyStr(ctx, v, prop);
            auto tb = unwrap(ctx, obj);
            JS_FreeValue(ctx, obj);
            return tb;
        }
    };

    template<>
    struct js_traits<Proxy>
    {
        static JSValue wrap(JSContext *ctx, const Proxy &n) noexcept
        {
            auto obj = JS_NewObject(ctx);
            /*
            JS_SetPropertyStr(ctx, obj, "LinkType", JS_NewInt32(ctx, n.linkType));
            JS_SetPropertyStr(ctx, obj, "ID", JS_NewInt32(ctx, n.id));
            JS_SetPropertyStr(ctx, obj, "GroupID", JS_NewInt32(ctx, n.groupID));
            JS_SetPropertyStr(ctx, obj, "Group", JS_NewStringLen(ctx, n.group.c_str(), n.group.size()));
            JS_SetPropertyStr(ctx, obj, "Remark", JS_NewStringLen(ctx, n.remarks.c_str(), n.remarks.size()));
            JS_SetPropertyStr(ctx, obj, "Server", JS_NewStringLen(ctx, n.server.c_str(), n.server.size()));
            JS_SetPropertyStr(ctx, obj, "Port", JS_NewInt32(ctx, n.port));
            JS_SetPropertyStr(ctx, obj, "ProxyInfo", JS_NewStringLen(ctx, n.proxyStr.c_str(), n.proxyStr.size()));
            */
            JS_SetPropertyStr(ctx, obj, "Type", JS_NewInt32(ctx, n.Type));
            JS_SetPropertyStr(ctx, obj, "Id", JS_NewInt32(ctx, n.Id));
            JS_SetPropertyStr(ctx, obj, "GroupId", JS_NewInt32(ctx, n.GroupId));
            JS_SetPropertyStr(ctx, obj, "Group", JS_NewStringLen(ctx, n.Group.c_str(), n.Group.size()));
            JS_SetPropertyStr(ctx, obj, "Remark", JS_NewStringLen(ctx, n.Remark.c_str(), n.Remark.size()));
            JS_SetPropertyStr(ctx, obj, "Server", JS_NewStringLen(ctx, n.Hostname.c_str(), n.Hostname.size()));
            JS_SetPropertyStr(ctx, obj, "Port", JS_NewInt32(ctx, n.Port));

            JS_SetPropertyStr(ctx, obj, "Username", JS_NewStringLen(ctx, n.Username.c_str(), n.Username.size()));
            JS_SetPropertyStr(ctx, obj, "Password", JS_NewStringLen(ctx, n.Password.c_str(), n.Password.size()));
            JS_SetPropertyStr(ctx, obj, "EncryptMethod", JS_NewStringLen(ctx, n.EncryptMethod.c_str(), n.EncryptMethod.size()));
            JS_SetPropertyStr(ctx, obj, "Plugin", JS_NewStringLen(ctx, n.Plugin.c_str(), n.Plugin.size()));
            JS_SetPropertyStr(ctx, obj, "PluginOption", JS_NewStringLen(ctx, n.PluginOption.c_str(), n.PluginOption.size()));
            JS_SetPropertyStr(ctx, obj, "Protocol", JS_NewStringLen(ctx, n.Protocol.c_str(), n.Protocol.size()));
            JS_SetPropertyStr(ctx, obj, "ProtocolParam", JS_NewStringLen(ctx, n.ProtocolParam.c_str(), n.ProtocolParam.size()));
            JS_SetPropertyStr(ctx, obj, "OBFS", JS_NewStringLen(ctx, n.OBFS.c_str(), n.OBFS.size()));
            JS_SetPropertyStr(ctx, obj, "OBFSParam", JS_NewStringLen(ctx, n.OBFSParam.c_str(), n.OBFSParam.size()));
            JS_SetPropertyStr(ctx, obj, "UserId", JS_NewStringLen(ctx, n.UserId.c_str(), n.UserId.size()));

            JS_SetPropertyStr(ctx, obj, "AlterId", JS_NewInt32(ctx, n.AlterId));
            JS_SetPropertyStr(ctx, obj, "TransferProtocol", JS_NewStringLen(ctx, n.TransferProtocol.c_str(), n.TransferProtocol.size()));
            JS_SetPropertyStr(ctx, obj, "FakeType", JS_NewStringLen(ctx, n.FakeType.c_str(), n.FakeType.size()));
            JS_SetPropertyStr(ctx, obj, "TLSSecure", JS_NewBool(ctx, n.TLSSecure));

            JS_SetPropertyStr(ctx, obj, "Host", JS_NewStringLen(ctx, n.Host.c_str(), n.Host.size()));
            JS_SetPropertyStr(ctx, obj, "Path", JS_NewStringLen(ctx, n.Path.c_str(), n.Path.size()));
            JS_SetPropertyStr(ctx, obj, "Edge", JS_NewStringLen(ctx, n.Edge.c_str(), n.Edge.size()));

            JS_SetPropertyStr(ctx, obj, "QUICSecure", JS_NewStringLen(ctx, n.QUICSecure.c_str(), n.QUICSecure.size()));
            JS_SetPropertyStr(ctx, obj, "QUICSecret", JS_NewStringLen(ctx, n.QUICSecret.c_str(), n.QUICSecret.size()));

            JS_SetPropertyStr(ctx, obj, "UDP", js_traits<tribool>::wrap(ctx, n.UDP));
            JS_SetPropertyStr(ctx, obj, "TCPFastOpen", js_traits<tribool>::wrap(ctx, n.TCPFastOpen));
            JS_SetPropertyStr(ctx, obj, "AllowInsecure", js_traits<tribool>::wrap(ctx, n.AllowInsecure));
            JS_SetPropertyStr(ctx, obj, "TLS13", js_traits<tribool>::wrap(ctx, n.TLS13));

            return obj;
        }
        static Proxy unwrap(JSContext *ctx, JSValueConst v)
        {
            Proxy node;
            /*
            node.linkType = JS_GetPropertyToInt32(ctx, v, "LinkType");
            node.id = JS_GetPropertyToInt32(ctx, v, "ID");
            node.groupID = JS_GetPropertyToInt32(ctx, v, "GroupID");
            node.group = JS_GetPropertyToString(ctx, v, "Group");
            node.remarks = JS_GetPropertyToString(ctx, v, "Remark");
            node.server = JS_GetPropertyToString(ctx, v, "Server");
            node.port = JS_GetPropertyToInt32(ctx, v, "Port");
            node.proxyStr = JS_GetPropertyToString(ctx, v, "ProxyInfo");
            */
            node.Type = JS_GetPropertyToInt32(ctx, v, "Type");
            node.Id = JS_GetPropertyToInt32(ctx, v, "Id");
            node.GroupId = JS_GetPropertyToInt32(ctx, v, "GroupId");
            node.Group = JS_GetPropertyToString(ctx, v, "Group");
            node.Remark = JS_GetPropertyToString(ctx, v, "Remark");
            node.Hostname = JS_GetPropertyToString(ctx, v, "Hostname");
            node.Port = JS_GetPropertyToUInt32(ctx, v, "Port");

            node.Username = JS_GetPropertyToString(ctx, v, "Username");
            node.Password = JS_GetPropertyToString(ctx, v, "Password");
            node.EncryptMethod = JS_GetPropertyToString(ctx, v, "EncryptMethod");
            node.Plugin = JS_GetPropertyToString(ctx, v, "Plugin");
            node.PluginOption = JS_GetPropertyToString(ctx, v, "PluginOption");
            node.Protocol = JS_GetPropertyToString(ctx, v, "Protocol");
            node.ProtocolParam = JS_GetPropertyToString(ctx, v, "ProtocolParam");
            node.OBFS = JS_GetPropertyToString(ctx, v, "OBFS");
            node.OBFSParam = JS_GetPropertyToString(ctx, v, "OBFSParam");
            node.UserId = JS_GetPropertyToString(ctx, v, "UserId");
            node.AlterId = JS_GetPropertyToUInt32(ctx, v, "AlterId");
            node.TransferProtocol = JS_GetPropertyToString(ctx, v, "TransferProtocol");
            node.FakeType = JS_GetPropertyToString(ctx, v, "FakeType");
            node.TLSSecure = JS_GetPropertyToBool(ctx, v, "TLSSecure");

            node.Host = JS_GetPropertyToString(ctx, v, "Host");
            node.Path = JS_GetPropertyToString(ctx, v, "Path");
            node.Edge = JS_GetPropertyToString(ctx, v, "Edge");

            node.QUICSecure = JS_GetPropertyToString(ctx, v, "QUICSecure");
            node.QUICSecret = JS_GetPropertyToString(ctx, v, "QUICSecret");

            node.UDP = js_traits<tribool>::JS_GetPropertyToTriBool(ctx, v, "UDP");
            node.TCPFastOpen = js_traits<tribool>::JS_GetPropertyToTriBool(ctx, v, "TCPFastOpen");
            node.AllowInsecure = js_traits<tribool>::JS_GetPropertyToTriBool(ctx, v, "AllowInsecure");
            node.TLS13 = js_traits<tribool>::JS_GetPropertyToTriBool(ctx, v, "TLS13");

            return node;
        }
    };
}

template <typename Fn>
void script_safe_runner(qjs::Runtime *runtime, qjs::Context *context, Fn runnable, bool clean_context = false)
{
    qjs::Runtime *internal_runtime = runtime;
    qjs::Context *internal_context = context;
    defer(if(clean_context) {delete internal_context; delete internal_runtime;} )
    if(clean_context)
    {
        internal_runtime = new qjs::Runtime();
        script_runtime_init(*internal_runtime);
        internal_context = new qjs::Context(*internal_runtime);
        script_context_init(*internal_context);
    }
    if(internal_runtime && internal_context)
        runnable(*internal_context);
}

#else
template <typename... Args>
void script_safe_runner(Args... args) { }
#endif // NO_JS_RUNTIME

#endif // SCRIPT_QUICKJS_H_INCLUDED
