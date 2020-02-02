#!/bin/bash
mkdir obj
set -xe

brew reinstall yaml-cpp rapidjson libevent zlib pcre2 bzip2 pkgconfig

git clone https://github.com/curl/curl
cd curl
./buildconf > /dev/null
./configure --with-ssl=/usr/local/opt/openssl@1.1 --without-mbedtls --disable-ldap --disable-ldaps --disable-rtsp --without-libidn2 > /dev/null
make -j8 > /dev/null
cd ..

curl -LO https://raw.githubusercontent.com/jpcre2/jpcre2/release/src/jpcre2.hpp
cp jpcre2.hpp /usr/local/include/

cp curl/lib/.libs/libcurl.a .
cp /usr/local/lib/libevent.a .
cp /usr/local/opt/zlib/lib/libz.a .
cp /usr/local/opt/openssl@1.1/lib/libssl.a .
cp /usr/local/opt/openssl@1.1/lib/libcrypto.a .
cp /usr/local/lib/libyaml-cpp.a .
cp /usr/local/lib/libpcre2-8.a .
cp /usr/local/opt/bzip2/lib/libbz2.a .

export CMAKE_CXX_FLAGS="-I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include"
cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 .
make -j8
c++ -Xlinker -unexported_symbol -Xlinker "*" -o base/subconverter CMakeFiles/subconverter.dir/src/*.o libpcre2-8.a libevent.a libcurl.a libz.a libssl.a libcrypto.a libyaml-cpp.a libbz2.a -ldl -lpthread -O3

cd base
chmod +rx subconverter
chmod +r *
tar czf ../subconverter_darwin64.tar.gz *
cd ..

set +xe
