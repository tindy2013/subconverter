#!/bin/bash
mkdir obj
set -xe

apk add gcc g++ build-base linux-headers cmake make autoconf automake libtool
apk add openssl-dev openssl-libs-static curl-dev curl-static nghttp2-static zlib-dev rapidjson-dev libevent-dev libevent-static zlib-static pcre2-dev bzip2-static 

git clone https://github.com/jbeder/yaml-cpp
cd yaml-cpp
cmake -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF . > /dev/null
make install -j4 > /dev/null
cd ..

curl -LO https://raw.githubusercontent.com/jpcre2/jpcre2/release/src/jpcre2.hpp
cp jpcre2.hpp /usr/local/include/

cmake .
make -j4
g++ -o base/subconverter CMakeFiles/subconverter.dir/src/*.o  -static -lpcre2-8 -levent -lyaml-cpp -lcurl -lnghttp2 -lssl -lcrypto -lz -lbz2 -ldl -lpthread -O3 -s  

cd base
chmod +rx subconverter
chmod +r *

tar czf ../subconverter_linux64.tar.gz *
