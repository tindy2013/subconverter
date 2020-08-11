#!/bin/bash
set -xe

apt update
apt install -y git cmake clang pkg-config
apt install -y libevent libcurl openssl pcre2 python2

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DCMAKE_INSTALL_PREFIX=/data/data/com.termux/files/usr -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF .
make install -j3
cd ..

git clone https://github.com/tencent/rapidjson --depth=1
cd rapidjson
cp -r include/* /data/data/com.termux/files/usr/include/
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
install -m0644 ./*.a /data/data/com.termux/files/usr/lib
install -m0644 duk*.h /data/data/com.termux/files/usr/include
install -m0644 ../extras/module-node/duk_module_node.h /data/data/com.termux/files/usr/include
cd ../../..
