add_rules("mode.debug", "mode.release")

option("static")
    set_default(false)
    set_showmenu(true)
    set_category("option")
    set_description("Build static binary.")
option_end()

add_requires("libcurl >=8.4.0", "pcre2", "yaml-cpp", "rapidjson", "toml11", "quickjspp")
includes("xmake/libcron.lua")
includes("xmake/yaml-cpp-static.lua")
add_requires("libcron", {system = false})
add_requires("yaml-cpp-static", {system = false})

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
    add_packages("libcurl", "pcre2", "rapidjson", "toml11", "libcron", "quickjspp")
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
