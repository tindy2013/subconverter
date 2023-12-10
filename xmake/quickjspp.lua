package("quickjspp")

    set_homepage("https://github.com/ftk/quickjspp")
    set_description("QuickJS C++ wrapper")

    add_urls("https://github.com/ftk/quickjspp.git")
    add_versions("2022.7.22", "9cee4b4d27271d54b95f6f42bfdc534ebeaaeb72")

    add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})

    add_includedirs("include", "include/quickjs")
    add_linkdirs("lib/quickjs")
    add_links("quickjs")

    add_deps("cmake")

    if is_plat("linux") then
        add_syslinks("pthread", "dl", "m")
    end

    on_install("linux", "macosx", "windows", function (package)
        local configs = {"-DBUILD_TESTING=OFF"}
        -- TODO, disable lto, maybe we need do it better
        io.replace("CMakeLists.txt", "set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)", "", {plain = true})
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        import("package.tools.cmake").install(package, configs, {})
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <iostream>
            void test() {
                using namespace qjs;
                Runtime runtime;
                Context context(runtime);
                auto rt = runtime.rt;
                auto ctx = context.ctx;
                js_std_init_handlers(rt);
                js_init_module_std(ctx, "std");
                js_init_module_os(ctx, "os");
                context.eval(R"xxx(
                    import * as std from 'std';
                    import * as os from 'os';
                    globalThis.std = std;
                    globalThis.os = os;
                )xxx", "<input>", JS_EVAL_TYPE_MODULE);

                js_std_loop(ctx);
                js_std_free_handlers(rt);

            }
        ]]}, {configs = {languages = "c++17"},
            includes = {"quickjspp.hpp","quickjs/quickjs-libc.h"}}))
    end)