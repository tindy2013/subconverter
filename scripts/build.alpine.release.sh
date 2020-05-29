#!/bin/bash
set -xe

apk add gcc g++ build-base linux-headers cmake make autoconf automake libtool python2 py2-pip
apk add openssl-dev openssl-libs-static curl curl-dev curl-static nghttp2-static zlib-dev rapidjson-dev libevent-dev libevent-static zlib-static pcre2-dev bzip2-static 

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF . > /dev/null
make install -j2 > /dev/null
cd ..

git clone https://github.com/svaarala/duktape --depth=1
cd duktape
pip2 install PyYAML
python2 util/dist.py
cd dist/src
cc -c -O3 -o duktape.o duktape.c
cc -c -O3 -o duk_module_node.o -I. ../extras/module-node/duk_module_node.c
ar cr libduktape.a duktape.o
ar cr libduktape_module.a duk_module_node.o
install -m0644 *.a /usr/lib
install -m0644 duk*.h /usr/include
install -m0644 ../extras/module-node/duk_module_node.h /usr/include
cd ../../..

cmake .
make -j2
rm subconverter
g++ -o base/subconverter CMakeFiles/subconverter.dir/src/*.o  -static -lpcre2-8 -levent -lyaml-cpp -lcurl -lnghttp2 -lssl -lcrypto -lz -lbz2 -lduktape -lduktape_module -ldl -lpthread -O3 -s  

cd base
chmod +rx subconverter
chmod +r *
cd ..
mv base subconverter
