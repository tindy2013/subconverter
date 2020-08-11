#!/bin/bash
set -xe

brew reinstall rapidjson libevent zlib pcre2 pkgconfig

#git clone https://github.com/curl/curl --depth=1
#cd curl
#./buildconf > /dev/null
#./configure --with-ssl=/usr/local/opt/openssl@1.1 --without-mbedtls --disable-ldap --disable-ldaps --disable-rtsp --without-libidn2 > /dev/null
#cmake -DCMAKE_USE_SECTRANSP=ON -DHTTP_ONLY=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_USE_LIBSSH2=OFF . > /dev/null
#make -j8 > /dev/null
#cd ..

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DCMAKE_BUILD_TYPE=Release -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF . > /dev/null
make install -j8 > /dev/null
cd ..

git clone https://github.com/svaarala/duktape --depth=1
cd duktape
pip2 install PyYAML
mkdir dist
python2 util/dist.py
cd dist/source/src
cc -c -O3 -o duktape.o duktape.c
cc -c -O3 -o duk_module_node.o -I. ../extras/module-node/duk_module_node.c
ar cr libduktape.a duktape.o
ar cr libduktape_module.a duk_module_node.o
install -m0644 ./*.a /usr/local/lib
install -m0644 ./duk*.h /usr/local/include
install -m0644 ../extras/module-node/duk_module_node.h /usr/local/include
cd ../../../..

cp /usr/local/lib/libevent.a .
cp /usr/local/opt/zlib/lib/libz.a .
cp /usr/local/lib/libpcre2-8.a .

cmake -DCMAKE_BUILD_TYPE=Release .
make -j8
rm subconverter
c++ -Xlinker -unexported_symbol -Xlinker "*" -o base/subconverter -framework CoreFoundation -framework Security $(find CMakeFiles/subconverter.dir/src/ -name "*.o") $(find . -name "*.a") -lcurl -O3

cd base
chmod +rx subconverter
chmod +r ./*
cd ..
mv base subconverter

set +xe
