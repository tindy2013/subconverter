#!/bin/bash
set -xe

brew reinstall rapidjson libevent zlib pcre2 bzip2 libssh2 pkgconfig

git clone https://github.com/curl/curl
cd curl
#./buildconf > /dev/null
#./configure --with-ssl=/usr/local/opt/openssl@1.1 --without-mbedtls --disable-ldap --disable-ldaps --disable-rtsp --without-libidn2 > /dev/null
cmake -DHTTP_ONLY=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 . > /dev/null
make -j8 > /dev/null
cd ..

git clone https://github.com/jbeder/yaml-cpp
cd yaml-cpp
cmake -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF . > /dev/null
make install -j8 > /dev/null
cd ..

cp curl/lib/libcurl.a .
cp yaml-cpp/libyaml-cpp.a .
cp /usr/local/lib/libevent.a .
cp /usr/local/opt/zlib/lib/libz.a .
cp /usr/local/opt/openssl@1.1/lib/libssl.a .
cp /usr/local/opt/openssl@1.1/lib/libcrypto.a .
cp /usr/local/lib/libpcre2-8.a .
cp /usr/local/opt/bzip2/lib/libbz2.a .
cp /usr/local/lib/libssh2.a .

export CMAKE_CXX_FLAGS="-I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include"
cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 .
make -j8
rm subconverter
c++ -Xlinker -unexported_symbol -Xlinker "*" -o base/subconverter CMakeFiles/subconverter.dir/src/*.o libpcre2-8.a libevent.a libcurl.a libz.a libssl.a libcrypto.a libyaml-cpp.a libbz2.a libssh2.a -ldl -lpthread -O3

cd base
chmod +rx subconverter
chmod +r *
cd ..
mv base subconverter

set +xe
