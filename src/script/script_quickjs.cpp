#include <string>
#include <map>
#include <iostream>
#include <quickjspp.hpp>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#include "../handler/multithread.h"
#include "../handler/webget.h"
#include "../handler/settings.h"
#include "../parser/config/proxy.h"
#include "../utils/map_extra.h"
#include "../utils/system.h"
#include "script_quickjs.h"

std::string parseProxy(const std::string &source);

static const std::string qjs_require_module {R"(import * as std from 'std'
import * as os from 'os'

let modules = {}

let debug = console.log
{
	let _debugOptions = std.getenv('DEBUG')
	if (typeof _debugOptions == 'undefined' || _debugOptions.split(',').indexOf('require') === -1) {
		debug = function () {}
	}
}

class CJSModule {
	constructor (id) {
		this.id = id
		this._failed = null
		this._loaded = false
		this.exports = {}
	}

	load () {
		const __file = this.id
		const __dir = _basename(this.id)
		const _require = require

		let ctx = { exports: {} }
		// Prevents modules from changing exports
		Object.seal(ctx)

		const _mark = '<<SCRIPT>>'
		let _loaderTemplate = `(function _loader (exports, require, module, __filename, __dirname) {${_mark}})(ctx.exports, _require, ctx, __file, __dir)`

		let _script = std.loadFile(__file)
		this._failed = _script === null
		if (this._failed) {
			return new Error(`Can't load script ${__file}`)
		}

		_script = _loaderTemplate.replace('<<SCRIPT>>', _script)
		eval(_script)

		this.exports = ctx.exports
		this._loaded = true
		return true
	}

}

function _basename (path) {
	let idx = path.lastIndexOf('/')
	if (idx === 0)
		return '/'
	return path.substring(0, idx)
}

function _statPath (path) {
	const [fstat, err] = os.stat(path)
	return {
		errno: err,
		isFile: fstat && (fstat.mode & os.S_IFREG) && true,
		isDir: fstat && (fstat.mode & os.S_IFDIR) && true
	}
}

function _loadModule (path) {
	debug(`_loadModule# Module ${path}`)
	const [id, err] = os.realpath(path)
	if (err) {
		throw new Error(`Module require error: Can't get real module path for ${path}`)
		return
	}

	debug(`_loadModule# id ${id}`)
	if (modules.hasOwnProperty(id)) {
		return modules[id]
	}

	let _module = new CJSModule(id)
	modules[id] = _module

	let _result = _module.load()
	if (_result !== true) {
		throw _result
		return
	}
	return _module
}

function _lookupModule (path) {
	let fstat = _statPath(path)

	debug(`_lookupModule# Looking for ${path}`)
	// Path found
	if (fstat.isFile) {
		debug(`_lookupModule# Found module file`)
		return path
	}

	// Path not found
	if (fstat.errno) {
		debug(`_lookupModule# Not found module file`)
		// Try with '.js' extension
		if (!path.endsWith('.js') && _statPath(`${path}.js`).isFile) {
			debug(`_lookupModule# Found appending .js to file name`)
			return `${path}.js`
		}
		return new Error(`Error: Module ${path} not found!`)
	}

	// Path found and it isn't a dir
	if (!fstat.isDir) {
		return new Error(`Error: Module file type not supported for ${path}`)
	}

	// Path it's a dir
	let _path = null	// Real path to module
	let _tryOthers = true	// Keep trying?

	debug(`_lookupModule# Path is a directory, trying options...`)
	// Try with package.json for NPM or YARN modules
	if (_statPath(`${path}/package.json`).isFile) {
		debug(`_lookupModule# It has package.json, looking for main script...`)
		let _pkg = JSON.parse(std.loadFile(`${path}/package.json`))
		if (_pkg && Object.keys(_pkg).indexOf('main') !== -1 && _pkg.main !== '' && _statPath(`${path}/${_pkg.main}`).isFile) {
			_tryOthers = false
			_path = `${path}/${_pkg.main}`
			debug(`_lookupModule# Found package main script!`)
		}
	}
	// Try other options
	if (_tryOthers && _statPath(`${path}/index.js`).isFile) {
		_tryOthers = false
		_path = `${path}/index.js`
		debug(`_lookupModule# Found package index.js file`)
	}
	if (_tryOthers && _statPath(`${path}/main.js`).isFile) {
		_tryOthers = false
		_path = `${path}/main.js`
		debug(`_lookupModule# Found package main.js file`)
	}

	if (_path === null) {
		return new Error(`Error: Module ${path} is a directory, but not a package`)
	}

	debug(`_lookupModule# Found module file: ${_path}`)
	// Returns what it founded
	return _path
}

export function require (path) {
	if (typeof __filename == 'undefined') {
		debug('require# Calling from main script')
	} else {
		debug(`require# Calling from ${__filename} parent module`)
	}
	let _path = _lookupModule(path)

	// Module not found
	if (_path instanceof Error) {
		throw _path
		return
	}

	let _module = _loadModule(_path)

	return _module.exports
})"};

class qjs_fetch_Headers
{
public:
    qjs_fetch_Headers() = default;

    string_icase_map headers;

    void append(const std::string &key, const std::string &value)
    {
        headers[key] = value;
    }
    void parse_from_string(const std::string &data)
    {
        headers.clear();
        string_array all_kv = split(data, "\r\n");
        for(std::string &x : all_kv)
        {
            size_t pos_colon = x.find(':');
            if(pos_colon == std::string::npos)
                continue;
            else if(pos_colon >= x.size() - 1)
                headers[x.substr(0, pos_colon)] = "";
            else
                headers[x.substr(0, pos_colon)] = x.substr(pos_colon + 2, x.size() - pos_colon);
        }
    }
};

class qjs_fetch_Request
{
public:
    qjs_fetch_Request() = default;
    std::string method = "GET";
    std::string url;
    std::string proxy;
    qjs_fetch_Headers headers;
    std::string cookies;
    std::string postdata;
    explicit qjs_fetch_Request(const std::string &url) : url(url) {}
};

class qjs_fetch_Response
{
public:
    qjs_fetch_Response() = default;
    int status_code = 200;
    std::string content;
    std::string cookies;
    qjs_fetch_Headers headers;
};

namespace qjs
{
    namespace detail
    {
        using string_map = std::map<std::string, std::string>;
        using string_icase_map = std::map<std::string, std::string, strICaseComp>;
    }

    template<>
    struct js_traits<detail::string_icase_map>
    {
        static detail::string_icase_map unwrap(JSContext *ctx, JSValueConst v)
        {
            string_icase_map res;
            JSPropertyEnum *props = nullptr, *props_begin;
            uint32_t len = 0;
            JS_GetOwnPropertyNames(ctx, &props, &len, v, 1);
            props_begin = props;
            while(len > 0)
            {
                auto key = JS_AtomToCString(ctx, props->atom);
                auto val = JS_GetProperty(ctx, v, props->atom);
                auto valData = JS_ToCString(ctx, val);
                res[key] = valData;
                JS_FreeCString(ctx, valData);
                JS_FreeValue(ctx, val);
                JS_FreeCString(ctx, key);
                JS_FreeAtom(ctx, props->atom);
                props++;
                len--;
            }
            js_free(ctx, props_begin);
            return res;
        }
        static JSValue wrap(JSContext *ctx, const detail::string_icase_map &m) noexcept
        {
            auto obj = JS_NewObject(ctx);

            for(auto &kv : m)
            {
                auto value = JS_NewStringLen(ctx, kv.second.c_str(), kv.second.size());
                JS_SetPropertyStr(ctx, obj, kv.first.c_str(), value);
            }
            return obj;
        }
    };

    template<>
    struct js_traits<qjs_fetch_Headers>
    {
        static qjs_fetch_Headers unwrap(JSContext *ctx, JSValueConst v)
        {
            qjs_fetch_Headers result;
            auto headers = JS_GetPropertyStr(ctx, v, "headers");
            result.headers = js_traits<detail::string_icase_map>::unwrap(ctx, headers);
            JS_FreeValue(ctx, headers);
            return result;
        }
        static JSValue wrap(JSContext *ctx, const qjs_fetch_Headers &h)
        {
            auto obj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, obj, "headers", js_traits<detail::string_icase_map>::wrap(ctx, h.headers));
            return obj;
        }
    };

    template<>
    struct js_traits<qjs_fetch_Request>
    {
        static qjs_fetch_Request unwrap(JSContext *ctx, JSValueConst v)
        {
            qjs_fetch_Request request;
            auto headers = JS_GetPropertyStr(ctx, v, "headers");
            request.method = JS_GetPropertyToString(ctx, v, "method");
            request.url = JS_GetPropertyToString(ctx, v, "url");
            request.postdata = JS_GetPropertyToString(ctx, v, "data");
            request.proxy = JS_GetPropertyToString(ctx, v, "proxy");
            request.cookies = JS_GetPropertyToString(ctx, v, "cookies");
            request.headers = js_traits<qjs_fetch_Headers>::unwrap(ctx, headers);
            JS_FreeValue(ctx, headers);
            return request;
        }
    };

    template<>
    struct js_traits<qjs_fetch_Response>
    {
        static JSValue wrap(JSContext *ctx, const qjs_fetch_Response &r) noexcept
        {
            auto obj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, obj, "status_code", JS_NewInt32(ctx, r.status_code));
            JS_SetPropertyStr(ctx, obj, "headers", js_traits<qjs_fetch_Headers>::wrap(ctx, r.headers));
            JS_SetPropertyStr(ctx, obj, "data", JS_NewStringLen(ctx, r.content.c_str(), r.content.size()));
            JS_SetPropertyStr(ctx, obj, "cookies", JS_NewStringLen(ctx, r.cookies.c_str(), r.cookies.size()));
            return obj;
        }
    };
}

static std::string makeDataURI(const std::string &content, bool shouldBase64 = false)
{
    if(shouldBase64)
        return "data:text/plain;base64," + base64Encode(content);
    else
        return "data:text/plain," + content;
}

static qjs_fetch_Response qjs_fetch(qjs_fetch_Request request)
{
    qjs_fetch_Response response;
    http_method method;
    switch(hash_(toUpper(request.method)))
    {
    case "GET"_hash:
        method = request.postdata.empty() ? HTTP_GET : HTTP_POST;
        break;
    case "POST"_hash:
        method = HTTP_POST;
        break;
    case "PATCH"_hash:
        method = HTTP_PATCH;
        break;
    case "HEAD"_hash:
        method = HTTP_HEAD;
        break;
    default:
        return response;
    }

    std::string response_headers;
    FetchArgument argument {method, request.url, request.proxy, &request.postdata, &request.headers.headers, &request.cookies, 0};
    FetchResult result {&response.status_code, &response.content, &response_headers, &response.cookies};

    webGet(argument, result);
    response.headers.parse_from_string(response_headers);

    return response;
}

std::string getGeoIP(const std::string &address, const std::string &proxy)
{
    return fetchFile("https://api.ip.sb/geoip/" + address, parseProxy(proxy), global.cacheConfig);
}

void script_runtime_init(qjs::Runtime &runtime)
{
    js_std_init_handlers(runtime.rt);
    JS_SetModuleLoaderFunc(runtime.rt, nullptr, js_module_loader, nullptr);
}

int ShowMsgbox(const std::string &title, std::string content, uint16_t type = 0)
{
#ifdef _WIN32
    if(!type)
        type = MB_ICONINFORMATION;
    return MessageBoxA(NULL, utf8ToACP(content).c_str(), utf8ToACP(title).c_str(), type);
#else
    return -1;
#endif // _WIN32
}

template<typename... Targs>
struct Lambda {
    template<typename Tret, typename T>
    static Tret lambda_ptr_exec(Targs... args) {
        return (Tret) (*(T*)fn<T>())(args...);
    }

    template<typename Tret = void, typename Tfp = Tret(*)(Targs...), typename T>
    static Tfp ptr(T& t) {
        fn<T>(&t);
        return (Tfp) lambda_ptr_exec<Tret, T>;
    }

    template<typename T>
    static void* fn(void* new_fn = nullptr) {
        static void* fn;
        if (new_fn != nullptr)
            fn = new_fn;
        return fn;
    }
};

uint32_t currentTime()
{
    return time(NULL);
}

int script_context_init(qjs::Context &context)
{
    try
    {
        js_init_module_os(context.ctx, "os");
        js_init_module_std(context.ctx, "std");
        js_std_add_helpers(context.ctx, 0, nullptr);
        context.eval(qjs_require_module, "<require>", JS_EVAL_TYPE_MODULE);
        auto &module = context.addModule("interUtils");
        module.class_<qjs_fetch_Headers>("Headers")
            .constructor<>()
            .fun<&qjs_fetch_Headers::headers>("headers")
            .fun<&qjs_fetch_Headers::append>("append")
            .fun<&qjs_fetch_Headers::parse_from_string>("parse");
        module.class_<qjs_fetch_Request>("Request")
            .constructor<>()
            .constructor<const std::string&>("Request")
            .fun<&qjs_fetch_Request::method>("method")
            .fun<&qjs_fetch_Request::url>("url")
            .fun<&qjs_fetch_Request::proxy>("proxy")
            .fun<&qjs_fetch_Request::postdata>("data")
            .fun<&qjs_fetch_Request::headers>("headers")
            .fun<&qjs_fetch_Request::cookies>("cookies");
        module.class_<qjs_fetch_Response>("Response")
            .constructor<>()
            .fun<&qjs_fetch_Response::status_code>("code")
            .fun<&qjs_fetch_Response::content>("data")
            .fun<&qjs_fetch_Response::cookies>("cookies")
            .fun<&qjs_fetch_Response::headers>("headers");
            /*
        module.class_<nodeInfo>("NodeInfo")
            .constructor<>()
            .fun<&nodeInfo::linkType>("LinkType")
            .fun<&nodeInfo::id>("ID")
            .fun<&nodeInfo::groupID>("GroupID")
            .fun<&nodeInfo::group>("Group")
            .fun<&nodeInfo::remarks>("Remark")
            .fun<&nodeInfo::server>("Hostname")
            .fun<&nodeInfo::port>("Port")
            .fun<&nodeInfo::proxyStr>("ProxyInfo");
            */
        module.class_<Proxy>("Proxy")
            .constructor<>()
            .fun<&Proxy::Type>("Type")
            .fun<&Proxy::Id>("Id")
            .fun<&Proxy::GroupId>("GroupId")
            .fun<&Proxy::Group>("Group")
            .fun<&Proxy::Remark>("Remark")
            .fun<&Proxy::Hostname>("Hostname")
            .fun<&Proxy::Port>("Port")
            .fun<&Proxy::Username>("Username")
            .fun<&Proxy::Password>("Password")
            .fun<&Proxy::EncryptMethod>("EncryptMethod")
            .fun<&Proxy::Plugin>("Plugin")
            .fun<&Proxy::PluginOption>("PluginOption")
            .fun<&Proxy::Protocol>("Protocol")
            .fun<&Proxy::ProtocolParam>("ProtocolParam")
            .fun<&Proxy::OBFS>("OBFS")
            .fun<&Proxy::OBFSParam>("OBFSParam")
            .fun<&Proxy::UserId>("UserId")
            .fun<&Proxy::AlterId>("AlterId")
            .fun<&Proxy::TransferProtocol>("TransferProtocol")
            .fun<&Proxy::FakeType>("FakeType")
            .fun<&Proxy::TLSSecure>("TLSSecure")
            .fun<&Proxy::Host>("Host")
            .fun<&Proxy::Path>("Path")
            .fun<&Proxy::Edge>("Edge")
            .fun<&Proxy::QUICSecure>("QUICSecure")
            .fun<&Proxy::QUICSecret>("QUICSecret")
            .fun<&Proxy::UDP>("UDP")
            .fun<&Proxy::TCPFastOpen>("TCPFastOpen")
            .fun<&Proxy::AllowInsecure>("AllowInsecure")
            .fun<&Proxy::TLS13>("TLS13");
        context.global().add<&makeDataURI>("makeDataURI")
            .add<&qjs_fetch>("fetch")
            .add<&base64Encode>("atob")
            .add<&base64Decode>("btoa")
            .add<&currentTime>("time")
            .add<&sleepMs>("sleep")
            .add<&ShowMsgbox>("msgbox")
            .add<&getUrlArg>("getUrlArg")
            .add<&fileGet>("fileGet")
            .add<&fileWrite>("fileWrite");
        context.eval(R"(
        import * as interUtils from 'interUtils'
        globalThis.Request = interUtils.Request
        globalThis.Response = interUtils.Response
        globalThis.Headers = interUtils.Headers
        globalThis.NodeInfo = interUtils.NodeInfo
        import * as std from 'std'
        import * as os from 'os'
        globalThis.std = std
        globalThis.os = os
        import { require } from '<require>'
        globalThis.require = require
        )", "<import>", JS_EVAL_TYPE_MODULE);
        return 0;
    }
    catch(qjs::exception)
    {
        script_print_stack(context);
        return 1;
    }
}

int script_cleanup(qjs::Context &context)
{
    js_std_loop(context.ctx);
    js_std_free_handlers(JS_GetRuntime(context.ctx));
    return 0;
}

void script_print_stack(qjs::Context &context)
{
    auto exc = context.getException();
    std::cerr << (std::string) exc << std::endl;
    if((bool) exc["stack"])
        std::cerr << (std::string) exc["stack"] << std::endl;
}
