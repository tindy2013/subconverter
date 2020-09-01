#!/bin/bash
set -xe

apk add gcc g++ build-base linux-headers cmake make autoconf automake libtool python2
apk add mbedtls-dev mbedtls-static zlib-dev rapidjson-dev libevent-dev libevent-static zlib-static pcre2-dev

git clone https://github.com/curl/curl --depth=1
cd curl
cmake -DCMAKE_USE_MBEDTLS=ON -DHTTP_ONLY=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_USE_LIBSSH2=OFF -DBUILD_CURL_EXE=OFF . > /dev/null
make install -j2 > /dev/null
cd ..

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DCMAKE_BUILD_TYPE=Release -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF . > /dev/null
make install -j2 > /dev/null
cd ..

git clone https://github.com/svaarala/duktape --depth=1
cd duktape
python2 -m ensurepip
pip2 install PyYAML
mkdir dist
python2 util/dist.py
cd dist/source/src
cc -c -O3 -o duktape.o duktape.c
cc -c -O3 -o duk_module_node.o -I. ../extras/module-node/duk_module_node.c
ar cr libduktape.a duktape.o
ar cr libduktape_module.a duk_module_node.o
install -m0644 ./*.a /usr/lib
install -m0644 ./duk*.h /usr/include
install -m0644 ../extras/module-node/duk_module_node.h /usr/include
cd ../../../..

export PKG_CONFIG_PATH=/usr/lib64/pkgconfig
cmake -DCMAKE_BUILD_TYPE=Release .
make -j2
rm subconverter
g++ -o base/subconverter $(find CMakeFiles/subconverter.dir/src/ -name "*.o")  -static -lpcre2-8 -levent -lyaml-cpp -L/usr/lib64 -lcurl -lmbedtls -lmbedcrypto -lmbedx509 -lz -lduktape -lduktape_module -O3 -s  

cd base
chmod +rx subconverter
chmod +r ./*
cd ..
mv base subconverter
