#!/bin/bash
set -xe

apk add gcc g++ build-base linux-headers cmake make autoconf automake libtool
apk add openssl-dev openssl-libs-static curl curl-dev curl-static nghttp2-static zlib-dev rapidjson-dev libevent-dev libevent-static zlib-static pcre2-dev bzip2-static 

git clone https://github.com/jbeder/yaml-cpp
cd yaml-cpp
cmake -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF . > /dev/null
make install -j2 > /dev/null
cd ..

cmake .
make -j2
rm subconverter
g++ -o base/subconverter CMakeFiles/subconverter.dir/src/*.o  -static -lpcre2-8 -levent -lyaml-cpp -lcurl -lnghttp2 -lssl -lcrypto -lz -lbz2 -ldl -lpthread -O3 -s  

cd base
chmod +rx subconverter
chmod +r *
cd ..
mv base subconverter
