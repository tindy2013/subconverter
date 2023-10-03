#!/bin/bash
set -xe

apk add gcc g++ build-base linux-headers cmake make autoconf automake libtool python2 python3
apk add mbedtls-dev mbedtls-static zlib-dev rapidjson-dev libevent-dev libevent-static zlib-static pcre2-dev

git clone https://github.com/curl/curl --depth=1 --branch curl-7_88_1
cd curl
cmake -DCURL_USE_MBEDTLS=ON -DHTTP_ONLY=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_USE_LIBSSH2=OFF -DBUILD_CURL_EXE=OFF . > /dev/null
make install -j2 > /dev/null
cd ..

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DCMAKE_BUILD_TYPE=Release -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF . > /dev/null
make install -j2 > /dev/null
cd ..

git clone https://github.com/ftk/quickjspp --depth=1
cd quickjspp
cmake -DCMAKE_BUILD_TYPE=Release .
make quickjs -j2
install -d /usr/lib/quickjs/
install -m644 quickjs/libquickjs.a /usr/lib/quickjs/
install -d /usr/include/quickjs/
install -m644 quickjs/quickjs.h quickjs/quickjs-libc.h /usr/include/quickjs/
install -m644 quickjspp.hpp /usr/include/
cd ..

git clone https://github.com/PerMalmberg/libcron --depth=1
cd libcron
git submodule update --init
cmake -DCMAKE_BUILD_TYPE=Release .
make libcron install -j2
cd ..

git clone https://github.com/ToruNiina/toml11 --depth=1
cd toml11
cmake -DCMAKE_CXX_STANDARD=11 .
make install -j4
cd ..

export PKG_CONFIG_PATH=/usr/lib64/pkgconfig
cmake -DCMAKE_BUILD_TYPE=Release .
make -j2
rm subconverter
g++ -o base/subconverter $(find CMakeFiles/subconverter.dir/src/ -name "*.o")  -static -lpcre2-8 -levent -lyaml-cpp -L/usr/lib64 -lcurl -lmbedtls -lmbedcrypto -lmbedx509 -lz -l:quickjs/libquickjs.a -llibcron -O3 -s  

python3 -m ensurepip
python3 -m pip install gitpython
python3 scripts/update_rules.py -c scripts/rules_config.conf

cd base
chmod +rx subconverter
chmod +r ./*
cd ..
mv base subconverter
