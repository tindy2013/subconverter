#!/bin/bash
set -xe
# remove all old deps
sudo rm -rf curl libcron quickjspp rapidjson toml11 yaml-cpp
# remove tmp folder
sudo rm -rf tmp

sudo apt install gcc build-essential g++ cmake make autoconf automake libtool python2 python3 python3-pip
sudo apt install libmbedtls-dev zlib1g-dev rapidjson-dev libpcre2-dev

git clone https://github.com/curl/curl --depth=1 --branch curl-8_4_0
cd curl
cmake -DCURL_USE_MBEDTLS=ON -DHTTP_ONLY=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_USE_LIBSSH2=OFF -DBUILD_CURL_EXE=OFF . > /dev/null
sudo make install -j2 > /dev/null
cd ..

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DCMAKE_BUILD_TYPE=Release -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF . > /dev/null
sudo make install -j3 > /dev/null
cd ..

git clone https://github.com/ftk/quickjspp --depth=1
cd quickjspp
cmake -DCMAKE_BUILD_TYPE=Release .
make quickjs -j3 > /dev/null
sudo install -d /usr/lib/quickjs/
sudo install -m644 quickjs/libquickjs.a /usr/lib/quickjs/
sudo install -d /usr/include/quickjs/
sudo install -m644 quickjs/quickjs.h quickjs/quickjs-libc.h /usr/include/quickjs/
sudo install -m644 quickjspp.hpp /usr/include/
cd ..

git clone https://github.com/PerMalmberg/libcron --depth=1
cd libcron
git submodule update --init
cmake -DCMAKE_BUILD_TYPE=Release .
sudo make libcron install -j3
cd ..

git clone https://github.com/ToruNiina/toml11 --branch="v3.7.1" --depth=1
cd toml11
cmake -DCMAKE_CXX_STANDARD=11 .
sudo make install -j4
cd ..

# export PKG_CONFIG_PATH=/usr/lib/pkgconfig
cmake -DCMAKE_BUILD_TYPE=Release .
make -j3
rm subconverter
# shellcheck disable=SC2046
g++ -o base/subconverter $(find CMakeFiles/subconverter.dir/src/ -name "*.o")  -static -lpcre2-8 -lyaml-cpp -L/usr/lib64 -lcurl -lmbedtls -lmbedcrypto -lmbedx509 -lz -l:quickjs/libquickjs.a -llibcron -O3 -s

python3 -m pip install gitpython
python3 scripts/update_rules.py -c scripts/rules_config.conf

cd base
chmod +rx subconverter
chmod +r ./*
cd ..
mv base subconverter
