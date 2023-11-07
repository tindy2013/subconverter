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

inline JSValue JS_NewString(JSContext *ctx, const std::string& str)
{
    return JS_NewStringLen(ctx, str.c_str(), str.size());
}

inline std::string JS_GetPropertyToString(JSContext *ctx, JSValue v, const char* prop)
{
    auto val = JS_GetPropertyStr(ctx, v, prop);
    size_t len;
    const char *str = JS_ToCStringLen(ctx, &len, val);
    std::string result(str, len);
    JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, val);
    return result;
}

inline std::string JS_GetPropertyToString(JSContext *ctx, JSValueConst obj, uint32_t index) {
    JSValue val = JS_GetPropertyUint32(ctx, obj, index);
    size_t len;
    const char *str = JS_ToCStringLen(ctx, &len, val);
    std::string result(str, len);
    JS_FreeCString(ctx, str);
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

inline uint32_t JS_GetPropertyToUInt32(JSContext *ctx, JSValue v, const char* prop, uint32_t def_value = 0)
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
    struct js_traits<StringArray> {
        static StringArray unwrap(JSContext *ctx, JSValueConst v) {
            StringArray arr;
            uint32_t length = JS_GetPropertyToUInt32(ctx, v, "length");
            for (uint32_t i = 0; i < length; i++) {
                arr.push_back(JS_GetPropertyToString(ctx, v, i));
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

            JS_DefinePropertyValueStr(ctx, obj, "Type", JS_NewInt32(ctx, n.Type), JS_PROP_C_W_E);
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
            node.Type = JS_GetPropertyToInt32(ctx, v, "Type");
            node.Id = JS_GetPropertyToInt32(ctx, v, "Id");
            node.GroupId = JS_GetPropertyToInt32(ctx, v, "GroupId");
            node.Group = JS_GetPropertyToString(ctx, v, "Group");
            node.Remark = JS_GetPropertyToString(ctx, v, "Remark");
            node.Hostname = JS_GetPropertyToString(ctx, v, "Server");
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

            node.SnellVersion = JS_GetPropertyToInt32(ctx, v, "SnellVersion");
            node.ServerName = JS_GetPropertyToString(ctx, v, "ServerName");

            node.SelfIP = JS_GetPropertyToString(ctx, v, "SelfIP");
            node.SelfIPv6 = JS_GetPropertyToString(ctx, v, "SelfIPv6");
            node.PublicKey = JS_GetPropertyToString(ctx, v, "PublicKey");
            node.PrivateKey = JS_GetPropertyToString(ctx, v, "PrivateKey");
            node.PreSharedKey = JS_GetPropertyToString(ctx, v, "PreSharedKey");
            node.DnsServers = js_traits<StringArray>::unwrap(ctx, JS_GetPropertyStr(ctx, v, "DnsServers"));
            node.Mtu = JS_GetPropertyToUInt32(ctx, v, "Mtu");
            node.AllowedIPs = JS_GetPropertyToString(ctx, v, "AllowedIPs");
            node.KeepAlive = JS_GetPropertyToUInt32(ctx, v, "KeepAlive");
            node.TestUrl = JS_GetPropertyToString(ctx, v, "TestUrl");
            node.ClientId = JS_GetPropertyToString(ctx, v, "ClientId");
            
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
