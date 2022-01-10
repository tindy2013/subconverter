#!/bin/bash
set -xe

apt update
apt install -y git cmake clang pkg-config
apt install -y libevent libcurl openssl pcre2 python2

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF .
make install -j3
cd ..

git clone https://github.com/tencent/rapidjson --depth=1
cd rapidjson
cp -r include/* $PREFIX/include/
cd ..

git clone https://github.com/ftk/quickjspp --depth=1
cd quickjspp
cmake -DCMAKE_BUILD_TYPE=Release .
make quickjs -j3
install -m644 quickjs/libquickjs.a $PREFIX/lib/
install -d $PREFIX/include/quickjs/
install -m644 quickjs/quickjs.h quickjs/quickjs-libc.h $PREFIX/include/quickjs/
install -m644 quickjspp.hpp $PREFIX/include/
cd ..

git clone https://github.com/PerMalmberg/libcron --depth=1
cd libcron
git submodule update --init
cmake -DCMAKE_BUILD_TYPE=Release .
make libcron -j3
install -m644 libcron/out/Release/liblibcron.a $PREFIX/lib/
install -d $PREFIX/include/libcron/
install -m644 libcron/include/libcron/* $PREFIX/include/libcron/
install -d $PREFIX/include/date/
install -m644 libcron/externals/date/include/date/* $PREFIX/include/date/
cd ..
