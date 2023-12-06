#include <string>
#include <iostream>
#include <duktape.h>
#include <duk_module_node.h>

#include "utils/string.h"
#include "utils/string_hash.h"
#include "handler/webget.h"
#include "handler/multithread.h"
#include "utils/base64/base64.h"
#include "utils/network.h"

extern int gCacheConfig;
extern std::string gProxyConfig;

std::string parseProxy(const std::string &source);

std::string foldPathString(const std::string &path)
{
    std::string output = path;
    string_size pos_up, pos_slash, pos_unres = 0;
    do
    {
        pos_up = output.find("../", pos_unres);
        if(pos_up == output.npos)
            break;
        if(pos_up == 0)
        {
            pos_unres = pos_up + 3;
            continue;
        }
        pos_slash = output.rfind("/", pos_up - 1);
        if(pos_slash != output.npos)
        {
            pos_slash = output.rfind("/", pos_slash - 1);
            if(pos_slash != output.npos)
                output.erase(pos_slash + 1, pos_up - pos_slash + 2);
            else
                output.erase(0, pos_up + 3);
        }
        else
            pos_unres = pos_up + 3;
    } while(pos_up != output.npos);
    return output;
}

static int duktape_get_arguments_str(duk_context *ctx, duk_idx_t min_count, duk_idx_t max_count, ...)
{
    duk_idx_t nargs = duk_get_top(ctx);
    if((min_count >= 0 && nargs < min_count) || (max_count >= 0 && nargs > max_count))
        return 0;
    va_list vl;
    va_start(vl, max_count);
    for(duk_idx_t idx = 0; idx < nargs; idx++)
    {
        std::string *arg = va_arg(vl, std::string*);
        if(arg)
            *arg = duk_safe_to_string(ctx, idx);
    }
    va_end(vl);
    return 1;
}

duk_ret_t cb_resolve_module(duk_context *ctx)
{
    const char *requested_id = duk_get_string(ctx, 0);
    const char *parent_id = duk_get_string(ctx, 1);  /* calling module */
    //const char *resolved_id;
    std::string resolved_id, parent_path = parent_id;
    if(!parent_path.empty())
    {
        string_size pos = parent_path.rfind("/");
        if(pos != parent_path.npos)
            resolved_id += parent_path.substr(0, pos + 1);
    }
    resolved_id += requested_id;
    if(!endsWith(resolved_id, ".js"))
        resolved_id += ".js";
    resolved_id = foldPathString(resolved_id);

    /* Arrive at the canonical module ID somehow. */
    if(!fileExist(resolved_id))
        duk_push_undefined(ctx);
    else
        duk_push_string(ctx, resolved_id.c_str());
    return 1;  /*nrets*/
}

duk_ret_t cb_load_module(duk_context *ctx)
{
    const char *resolved_id = duk_get_string(ctx, 0);
    std::string module_source = fileGet(resolved_id, true);

    /* Arrive at the JS source code for the module somehow. */

    duk_push_string(ctx, module_source.c_str());
    return 1;  /*nrets*/
}

static duk_ret_t native_print(duk_context *ctx)
{
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, duk_get_top(ctx) - 1);
	printf("%s\n", duk_safe_to_string(ctx, -1));
	return 0;
}

static duk_ret_t fetch(duk_context *ctx)
{
    /*
    std::string filepath, proxy;
    duktape_get_arguments_str(ctx, 1, 2, &filepath, &proxy);
    std::string content = fetchFile(filepath, proxy, gCacheConfig);
    duk_push_lstring(ctx, content.c_str(), content.size());
    */
    std::string filepath, proxy, method, postdata, content;
    if(duktape_get_arguments_str(ctx, 1, 4, &filepath, &proxy, &method, &postdata) == 0)
        return 0;
    switch(hash_(method))
    {
    case "POST"_hash:
        webPost(filepath, postdata, proxy, string_array{}, &content);
        break;
    default:
        content = fetchFile(filepath, proxy, gCacheConfig);
        break;
    }
    duk_push_lstring(ctx, content.c_str(), content.size());
    return 1;
}

static duk_ret_t atob(duk_context *ctx)
{
    std::string data = duk_safe_to_string(ctx, -1);
    duk_push_string(ctx, base64Encode(data).c_str());
    return 1;
}

static duk_ret_t btoa(duk_context *ctx)
{
    std::string data = duk_safe_to_string(ctx, -1);
    data = base64Decode(data, true);
    duk_push_lstring(ctx, data.c_str(), data.size());
    return 1;
}

static duk_ret_t getGeoIP(duk_context *ctx)
{
    std::string address, proxy;
    duktape_get_arguments_str(ctx, 1, 2, &address, &proxy);
    if(!isIPv4(address) && !isIPv6(address))
        address = hostnameToIPAddr(address);
    if(address.empty())
        duk_push_undefined(ctx);
    else
        duk_push_string(ctx, fetchFile("https://api.ip.sb/geoip/" + address, parseProxy(proxy), gCacheConfig).c_str());
    return 1;
}

duk_context *duktape_init()
{
    duk_context *ctx = duk_create_heap_default();
    if(!ctx)
        return NULL;
    /// init module
    duk_push_object(ctx);
    duk_push_c_function(ctx, cb_resolve_module, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "resolve");
    duk_push_c_function(ctx, cb_load_module, DUK_VARARGS);
    duk_put_prop_string(ctx, -2, "load");
    duk_module_node_init(ctx);

    duk_push_c_function(ctx, native_print, DUK_VARARGS);
    duk_put_global_string(ctx, "print");
    duk_push_c_function(ctx, fetch, DUK_VARARGS);
    duk_put_global_string(ctx, "fetch");
    duk_push_c_function(ctx, atob, 1);
    duk_put_global_string(ctx, "atob");
    duk_push_c_function(ctx, btoa, 1);
    duk_put_global_string(ctx, "btoa");
    duk_push_c_function(ctx, getGeoIP, DUK_VARARGS);
    duk_put_global_string(ctx, "geoip");
    return ctx;
}

int duktape_peval(duk_context *ctx, const std::string &script)
{
    return duk_peval_string(ctx, script.c_str());
}

int duktape_call_function(duk_context *ctx, const std::string &name, size_t nargs, ...)
{
    duk_get_global_string(ctx, name.c_str());
    va_list vl;
    va_start(vl, nargs);
    size_t index = 0;
    while(index < nargs)
    {
        std::string *arg = va_arg(vl, std::string*);
        if(arg != NULL)
            duk_push_string(ctx, arg->c_str());
        else
            duk_push_undefined(ctx);
        index++;
    }
    va_end(vl);
    return duk_pcall(ctx, nargs);
}

int duktape_push_nodeinfo(duk_context *ctx, const nodeInfo &node)
{
    duk_push_object(ctx);
    duk_push_string(ctx, node.group.c_str());
    duk_put_prop_string(ctx, -2, "Group");
    duk_push_int(ctx, node.groupID);
    duk_put_prop_string(ctx, -2, "GroupID");
    duk_push_int(ctx, node.id);
    duk_put_prop_string(ctx, -2, "Index");
    duk_push_string(ctx, node.remarks.c_str());
    duk_put_prop_string(ctx, -2, "Remark");
    duk_push_string(ctx, node.proxyStr.c_str());
    duk_put_prop_string(ctx, -2, "ProxyInfo");
    return 0;
}

int duktape_push_nodeinfo_arr(duk_context *ctx, const nodeInfo &node, duk_idx_t index)
{
    duk_push_object(ctx);
    duk_push_string(ctx, "Group");
    duk_push_string(ctx, node.group.c_str());
    duk_def_prop(ctx, index - 2, DUK_DEFPROP_HAVE_VALUE);
    duk_push_string(ctx, "GroupID");
    duk_push_int(ctx, node.groupID);
    duk_def_prop(ctx, index - 2, DUK_DEFPROP_HAVE_VALUE);
    duk_push_string(ctx, "Index");
    duk_push_int(ctx, node.id);
    duk_def_prop(ctx, index - 2, DUK_DEFPROP_HAVE_VALUE);
    duk_push_string(ctx, "Remark");
    duk_push_string(ctx, node.remarks.c_str());
    duk_def_prop(ctx, index - 2, DUK_DEFPROP_HAVE_VALUE);
    duk_push_string(ctx, "ProxyInfo");
    duk_push_string(ctx, node.proxyStr.c_str());
    duk_def_prop(ctx, index - 2, DUK_DEFPROP_HAVE_VALUE);
    return 0;
}

int duktape_get_res_int(duk_context *ctx)
{
    int retval = duk_to_int(ctx, -1);
    duk_pop(ctx);
    return retval;
}

std::string duktape_get_res_str(duk_context *ctx)
{
    if(duk_is_null_or_undefined(ctx, -1))
        return "";
    std::string retstr = duk_safe_to_string(ctx, -1);
    duk_pop(ctx);
    return retstr;
}

bool duktape_get_res_bool(duk_context *ctx)
{
    bool ret = duk_to_boolean(ctx, -1);
    duk_pop(ctx);
    return ret;
}

std::string duktape_get_err_stack(duk_context *ctx)
{
    duk_get_prop_string(ctx, -1, "stack");
    std::string stackstr = duk_get_string(ctx, -1);
    duk_pop(ctx);
    return stackstr;
}
