#!/bin/bash
set -xe

apt update
apt install -y git cmake clang pkg-config
apt install -y libevent libcurl openssl pcre2

git clone https://github.com/jbeder/yaml-cpp
cd yaml-cpp
cmake -DCMAKE_INSTALL_PREFIX=/data/data/com.termux/files/usr -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF .
make install -j3
cd ..

git clone https://github.com/tencent/rapidjson
cd rapidjson
cp -r include/* /data/data/com.termux/files/usr/include/
cd ..
