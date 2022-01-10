#!/bin/bash
set -xe

git clone https://github.com/curl/curl --depth=1
cd curl
cmake -DCMAKE_BUILD_TYPE=Release -DCURL_USE_LIBSSH2=OFF -DHTTP_ONLY=ON -DCURL_USE_SCHANNEL=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_CURL_EXE=OFF -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" -G "Unix Makefiles" -DHAVE_LIBIDN2=OFF .
make install -j4
cd ..

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DCMAKE_BUILD_TYPE=Release -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" -G "Unix Makefiles" .
make install -j4
cd ..

git clone https://github.com/ftk/quickjspp --depth=1
cd quickjspp
patch quickjs/quickjs-libc.c -i ../scripts/patches/0001-quickjs-libc-add-realpath-for-Windows.patch
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release .
make quickjs -j4
install -m644 quickjs/libquickjs.a "$MINGW_PREFIX/lib/"
install -d "$MINGW_PREFIX/include/quickjs"
install -m644 quickjs/quickjs.h quickjs/quickjs-libc.h "$MINGW_PREFIX/include/quickjs/"
install -m644 quickjspp.hpp "$MINGW_PREFIX/include/"
cd ..

git clone https://github.com/PerMalmberg/libcron --depth=1
cd libcron
git submodule update --init
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release .
make libcron -j4
install -m644 libcron/out/Release/liblibcron.a "$MINGW_PREFIX/lib/"
install -d "$MINGW_PREFIX/include/libcron/"
install -m644 libcron/include/libcron/* "$MINGW_PREFIX/include/libcron/"
install -d "$MINGW_PREFIX/include/date/"
install -m644 libcron/externals/date/include/date/* "$MINGW_PREFIX/include/date/"
cd ..

git clone https://github.com/Tencent/rapidjson --depth=1
cd rapidjson
cmake -DRAPIDJSON_BUILD_DOC=OFF -DRAPIDJSON_BUILD_EXAMPLES=OFF -DRAPIDJSON_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" -G "Unix Makefiles" .
make install -j4
cd ..

git clone https://github.com/ToruNiina/toml11 --depth=1
cd toml11
cmake -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" -G "Unix Makefiles" .
make install -j4
cd ..

rm -f C:/Strawberry/perl/bin/pkg-config C:/Strawberry/perl/bin/pkg-config.bat
cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
make -j4
rm subconverter.exe
g++ $(find CMakeFiles/subconverter.dir/src -name "*.obj") curl/lib/libcurl.a -o base/subconverter.exe -static -levent -lpcre2-8 -lquickjs -llibcron -lyaml-cpp -liphlpapi -lcrypt32 -lws2_32 -lwsock32 -lz -s
mv base subconverter
