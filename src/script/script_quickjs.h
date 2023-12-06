#ifndef SCRIPT_QUICKJS_H_INCLUDED
#define SCRIPT_QUICKJS_H_INCLUDED

#include "parser/config/proxy.h"
#include "utils/defer.h"

#ifndef NO_JS_RUNTIME

#include <quickjspp.hpp>

void script_runtime_init(qjs::Runtime &runtime);
int script_context_init(qjs::Context &context);
int script_cleanup(qjs::Context &context);
void script_print_stack(qjs::Context &context);

inline JSValue JS_NewString(JSContext *ctx, const std::string& str)
{
    return JS_NewStringLen(ctx, str.c_str(), str.size());
}

inline std::string JS_GetPropertyIndexToString(JSContext *ctx, JSValueConst obj, uint32_t index) {
    JSValue val = JS_GetPropertyUint32(ctx, obj, index);
    size_t len;
    const char *str = JS_ToCStringLen(ctx, &len, val);
    std::string result(str, len);
    JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, val);
    return result;
}

namespace qjs
{
    template<typename T>
    static T unwrap_free(JSContext *ctx, JSValue v, const char* key) noexcept
    {
        auto obj = JS_GetPropertyStr(ctx, v, key);
        T t = js_traits<T>::unwrap(ctx, obj);
        JS_FreeValue(ctx, obj);
        return t;
    }

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
            bool defined = unwrap_free<bool>(ctx, v, "isDefined");
            if(defined)
            {
                bool value = unwrap_free<bool>(ctx, v, "value");
                t.set(value);
            }
            return t;
        }
    };

    template<>
    struct js_traits<StringArray>
    {
        static StringArray unwrap(JSContext *ctx, JSValueConst v) {
            StringArray arr;
            auto length = unwrap_free<uint32_t>(ctx, v, "length");
            for (uint32_t i = 0; i < length; i++) {
                arr.push_back(JS_GetPropertyIndexToString(ctx, v, i));
            }
            return arr;
        }

        static JSValue wrap(JSContext *ctx, const StringArray& arr) {
            JSValue jsArray = JS_NewArray(ctx);
            for (std::size_t i = 0; i < arr.size(); i++) {
                JS_SetPropertyUint32(ctx, jsArray, i, JS_NewString(ctx, arr[i]));
            }
            return jsArray;
        }
    };

    template<>
    struct js_traits<Proxy>
    {
        static JSValue wrap(JSContext *ctx, const Proxy &n) noexcept
        {
            JSValue obj = JS_NewObjectProto(ctx, JS_NULL);
            if (JS_IsException(obj)) {
                return obj;
            }

            JS_DefinePropertyValueStr(ctx, obj, "Type", js_traits<ProxyType>::wrap(ctx, n.Type), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Id", JS_NewUint32(ctx, n.Id), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "GroupId", JS_NewUint32(ctx, n.GroupId), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Group", JS_NewString(ctx, n.Group), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Remark", JS_NewString(ctx, n.Remark), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Server", JS_NewString(ctx, n.Hostname), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Port", JS_NewInt32(ctx, n.Port), JS_PROP_C_W_E);

            JS_DefinePropertyValueStr(ctx, obj, "Username", JS_NewString(ctx, n.Username), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Password", JS_NewString(ctx, n.Password), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "EncryptMethod", JS_NewString(ctx, n.EncryptMethod), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Plugin", JS_NewString(ctx, n.Plugin), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "PluginOption", JS_NewString(ctx, n.PluginOption), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Protocol", JS_NewString(ctx, n.Protocol), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "ProtocolParam", JS_NewString(ctx, n.ProtocolParam), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "OBFS", JS_NewString(ctx, n.OBFS), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "OBFSParam", JS_NewString(ctx, n.OBFSParam), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "UserId", JS_NewString(ctx, n.UserId), JS_PROP_C_W_E);

            JS_DefinePropertyValueStr(ctx, obj, "AlterId", JS_NewInt32(ctx, n.AlterId), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "TransferProtocol", JS_NewString(ctx, n.TransferProtocol), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "FakeType", JS_NewString(ctx, n.FakeType), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "TLSSecure", JS_NewBool(ctx, n.TLSSecure), JS_PROP_C_W_E);

            JS_DefinePropertyValueStr(ctx, obj, "Host", JS_NewString(ctx, n.Host), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Path", JS_NewString(ctx, n.Path), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Edge", JS_NewString(ctx, n.Edge), JS_PROP_C_W_E);

            JS_DefinePropertyValueStr(ctx, obj, "QUICSecure", JS_NewString(ctx, n.QUICSecure), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "QUICSecret", JS_NewString(ctx, n.QUICSecret), JS_PROP_C_W_E);

            JS_DefinePropertyValueStr(ctx, obj, "UDP", js_traits<tribool>::wrap(ctx, n.UDP), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "TCPFastOpen", js_traits<tribool>::wrap(ctx, n.TCPFastOpen), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "AllowInsecure", js_traits<tribool>::wrap(ctx, n.AllowInsecure), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "TLS13", js_traits<tribool>::wrap(ctx, n.TLS13), JS_PROP_C_W_E);

            JS_DefinePropertyValueStr(ctx, obj, "SnellVersion", JS_NewInt32(ctx, n.SnellVersion), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "ServerName", JS_NewString(ctx, n.ServerName), JS_PROP_C_W_E);

            JS_DefinePropertyValueStr(ctx, obj, "SelfIP", JS_NewString(ctx, n.SelfIP), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "SelfIPv6", JS_NewString(ctx, n.SelfIPv6), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "PublicKey", JS_NewString(ctx, n.PublicKey), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "PrivateKey", JS_NewString(ctx, n.PrivateKey), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "PreSharedKey", JS_NewString(ctx, n.PreSharedKey), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "DnsServers", js_traits<StringArray>::wrap(ctx, n.DnsServers), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "Mtu", JS_NewUint32(ctx, n.Mtu), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "AllowedIPs", JS_NewString(ctx, n.AllowedIPs), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "KeepAlive", JS_NewUint32(ctx, n.KeepAlive), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "TestUrl", JS_NewString(ctx, n.TestUrl), JS_PROP_C_W_E);
            JS_DefinePropertyValueStr(ctx, obj, "ClientId", JS_NewString(ctx, n.ClientId), JS_PROP_C_W_E);
            return obj;
        }

        static Proxy unwrap(JSContext *ctx, JSValueConst v)
        {
            Proxy node;
            node.Type = unwrap_free<ProxyType>(ctx, v, "Type");
            node.Id = unwrap_free<int32_t>(ctx, v, "Id");
            node.GroupId = unwrap_free<int32_t>(ctx, v, "GroupId");
            node.Group = unwrap_free<std::string>(ctx, v, "Group");
            node.Remark = unwrap_free<std::string>(ctx, v, "Remark");
            node.Hostname = unwrap_free<std::string>(ctx, v, "Server");
            node.Port = unwrap_free<uint32_t>(ctx, v, "Port");

            node.Username = unwrap_free<std::string>(ctx, v, "Username");
            node.Password = unwrap_free<std::string>(ctx, v, "Password");
            node.EncryptMethod = unwrap_free<std::string>(ctx, v, "EncryptMethod");
            node.Plugin = unwrap_free<std::string>(ctx, v, "Plugin");
            node.PluginOption = unwrap_free<std::string>(ctx, v, "PluginOption");
            node.Protocol = unwrap_free<std::string>(ctx, v, "Protocol");
            node.ProtocolParam = unwrap_free<std::string>(ctx, v, "ProtocolParam");
            node.OBFS = unwrap_free<std::string>(ctx, v, "OBFS");
            node.OBFSParam = unwrap_free<std::string>(ctx, v, "OBFSParam");
            node.UserId = unwrap_free<std::string>(ctx, v, "UserId");
            node.AlterId = unwrap_free<uint32_t>(ctx, v, "AlterId");
            node.TransferProtocol = unwrap_free<std::string>(ctx, v, "TransferProtocol");
            node.FakeType = unwrap_free<std::string>(ctx, v, "FakeType");
            node.TLSSecure = unwrap_free<bool>(ctx, v, "TLSSecure");

            node.Host = unwrap_free<std::string>(ctx, v, "Host");
            node.Path = unwrap_free<std::string>(ctx, v, "Path");
            node.Edge = unwrap_free<std::string>(ctx, v, "Edge");

            node.QUICSecure = unwrap_free<std::string>(ctx, v, "QUICSecure");
            node.QUICSecret = unwrap_free<std::string>(ctx, v, "QUICSecret");

            node.UDP = unwrap_free<tribool>(ctx, v, "UDP");
            node.TCPFastOpen = unwrap_free<tribool>(ctx, v, "TCPFastOpen");
            node.AllowInsecure = unwrap_free<tribool>(ctx, v, "AllowInsecure");
            node.TLS13 = unwrap_free<tribool>(ctx, v, "TLS13");

            node.SnellVersion = unwrap_free<int32_t>(ctx, v, "SnellVersion");
            node.ServerName = unwrap_free<std::string>(ctx, v, "ServerName");

            node.SelfIP = unwrap_free<std::string>(ctx, v, "SelfIP");
            node.SelfIPv6 = unwrap_free<std::string>(ctx, v, "SelfIPv6");
            node.PublicKey = unwrap_free<std::string>(ctx, v, "PublicKey");
            node.PrivateKey = unwrap_free<std::string>(ctx, v, "PrivateKey");
            node.PreSharedKey = unwrap_free<std::string>(ctx, v, "PreSharedKey");
            node.DnsServers = unwrap_free<StringArray>(ctx, v, "DnsServers");
            node.Mtu = unwrap_free<uint32_t>(ctx, v, "Mtu");
            node.AllowedIPs = unwrap_free<std::string>(ctx, v, "AllowedIPs");
            node.KeepAlive = unwrap_free<uint32_t>(ctx, v, "KeepAlive");
            node.TestUrl = unwrap_free<std::string>(ctx, v, "TestUrl");
            node.ClientId = unwrap_free<std::string>(ctx, v, "ClientId");
            
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
