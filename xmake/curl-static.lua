package("curl-static")
    add_deps("cmake")
    add_versions("8.5.0", "7161cb17c01dcff1dc5bf89a18437d9d729f1ecd")
    set_urls("https://github.com/curl/curl.git")

    if is_plat("macosx", "iphoneos") then
        add_frameworks("Security", "CoreFoundation", "SystemConfiguration")
    elseif is_plat("linux") then
        add_deps("mbedtls")
        add_syslinks("pthread")
    elseif is_plat("windows", "mingw") then
        add_deps("zlib")
        add_syslinks("advapi32", "crypt32", "wldap32", "winmm", "ws2_32", "user32", "bcrypt")
    end

    on_install(function (package)
                local configs = {}
                table.insert(configs, "-DCURL_USE_LIBSSH2=OFF")
                table.insert(configs, "-DHAVE_LIBIDN2=OFF")
                table.insert(configs, "-DCURL_USE_LIBPSL=OFF")
                table.insert(configs, "-DBUILD_CURL_EXE=OFF")
                table.insert(configs, "-DBUILD_TESTING=OFF")
                table.insert(configs, "-DCURL_USE_MBEDTLS=" .. (package:is_plat("linux") and "ON" or "OFF"))
                table.insert(configs, "-DCURL_USE_SCHANNEL=" .. (package:is_plat("windows") and "ON" or "OFF"))
                table.insert(configs, "-DHTTP_ONLY=ON")
                table.insert(configs, "-DCURL_USE_LIBSSH2=OFF")
                table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
                table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
                import("package.tools.cmake").install(package, configs)
            end)
package_end()
