#!/bin/bash
set -xe

apt update
apt install git cmake clang
apt isntall libevent libcurl openssl

git clone https://github.com/jbeder/yaml-cpp
cd yaml-cpp
cmake -DCMAKE_INSTALL_PREFIX=/data/data/com.termux/files/usr .
make install -j3
cd ..

git clone https://github.com/tencent/rapidjson
cd rapidjson
cp -r include/* /data/data/com.termux/files/usr/include/
cd ..
