add_rules("mode.debug", "mode.release")

option("static")
    set_default(false)
    set_showmenu(true)
    set_category("option")
    set_description("Build static binary.")
option_end()

add_requires("pcre2", "yaml-cpp", "rapidjson", "toml11")
includes("xmake/libcron.lua")
includes("xmake/yaml-cpp-static.lua")
includes("xmake/quickjspp.lua")
includes("xmake/curl-static.lua")
add_requires("libcron", {system = false})
add_requires("yaml-cpp-static", {system = false})
add_requires("quickjspp", {system = false})
if not is_plat("macosx") and has_config("static") then
    add_requires("curl-static", {system = false})
else
    add_requires("libcurl")
end

target("subconverter")
    set_kind("binary")
    if is_os("windows") then
        add_syslinks("ws2_32", "wsock32")
    end
    if not is_os("macosx") and has_config("static") then
        add_ldflags("-static")
    end
    add_files("src/**.cpp|lib/wrapper.cpp|server/webserver_libevent.cpp|script/script.cpp|generator/template/template_jinja2.cpp")
    add_includedirs("src")
    add_includedirs("include")
    add_packages("pcre2", "rapidjson", "toml11", "libcron", "quickjspp")
    if is_plat("macosx") then
        add_packages("libcurl")
    else
        add_packages("curl-static")
    end
    if has_config("static") then
        add_packages("yaml-cpp-static")
    else
        add_packages("yaml-cpp")
    end
    add_defines("CURL_STATICLIB")
    add_defines("PCRE2_STATIC")
    add_defines("YAML_CPP_STATIC_DEFINE")
    add_cxxflags("-std=c++20")

target("subconverter_lib")
    set_basename("subconverter")
    set_kind("static")
    add_files("src/**.cpp|handler/**.cpp|server/**.cpp|script/**.cpp|generator/template/template_jinja2.cpp")
    add_includedirs("src")
    add_includedirs("include")
    add_packages("pcre2", "yaml-cpp", "rapidjson", "toml11")
    add_defines("CURL_STATICLIB")
    add_defines("PCRE2_STATIC")
    add_defines("YAML_CPP_STATIC_DEFINE")
    add_defines("NO_JS_RUNTIME")
    add_defines("NO_WEBGET")
    add_cxxflags("-std=c++20")
