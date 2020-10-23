#!/bin/bash
set -xe

git clone https://github.com/curl/curl --depth=1
cd curl
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_USE_LIBSSH2=OFF -DHTTP_ONLY=ON -DCMAKE_USE_SCHANNEL=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_CURL_EXE=OFF -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" -G "Unix Makefiles" .
make install -j4
cd ..

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DCMAKE_BUILD_TYPE=Release -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" -G "Unix Makefiles" .
make install -j4
cd ..

git clone https://github.com/svaarala/duktape --depth=1
cd duktape
make -C src-tools
node src-tools/index.js dist --output-directory dist
cd dist/src
gcc -c -O3 -o duktape.o duktape.c
gcc -c -O3 -o duk_module_node.o -I. ../extras/module-node/duk_module_node.c
ar cr libduktape.a duktape.o
ar cr libduktape_module.a duk_module_node.o
install -m0644 ./*.a "$MINGW_PREFIX/lib"
install -m0644 ./duk*.h "$MINGW_PREFIX/include"
install -m0644 ../extras/module-node/duk_module_node.h "$MINGW_PREFIX/include"
cd ../../..

git clone https://github.com/Tencent/rapidjson --depth=1
cd rapidjson
cmake -DRAPIDJSON_BUILD_DOC=OFF -DRAPIDJSON_BUILD_EXAMPLES=OFF -DRAPIDJSON_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" -G "Unix Makefiles" .
make install -j4
cd ..

cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
make -j4
rm subconverter.exe
g++ $(find CMakeFiles/subconverter.dir/src -name "*.obj") curl/lib/libcurl.a -o base/subconverter.exe -static -levent -lpcre2-8 -lduktape -lduktape_module -lyaml-cpp -lidn2 -lunistring -liconv -liphlpapi -lcrypt32 -lws2_32 -lwsock32 -lz -s
mv base subconverter