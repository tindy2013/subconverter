#!/bin/bash
mkdir obj
set -xe

brew reinstall yaml-cpp rapidjson libevent zlib

git clone https://github.com/curl/curl
cd curl
./buildconf > /dev/null
./configure --with-ssl=/usr/local/opt/openssl@1.1 --without-mbedtls --disable-ldap --disable-ldaps --disable-rtsp --without-libidn2 > /dev/null
make -j8 > /dev/null
cd ..

curl -L -o bzip2-1.0.6.tar.gz https://sourceforge.net/projects/bzip2/files/bzip2-1.0.6.tar.gz/download
tar xvf bzip2-1.0.6.tar.gz
cd bzip2-1.0.6
make -j8 > /dev/null
cd ..

cp /usr/local/lib/libevent.a .
cp /usr/local/opt/zlib/lib/libz.a .
cp /usr/local/opt/openssl@1.1/lib/libssl.a .
cp /usr/local/opt/openssl@1.1/lib/libcrypto.a .
cp /usr/local/lib/libyaml-cpp.a .

c++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -D_MACOS -I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include -c logger.cpp -o obj/logger.o
c++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -D_MACOS -I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include -c main.cpp -o obj/main.o
c++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -D_MACOS -I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include -c misc.cpp -o obj/misc.o
c++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -D_MACOS -I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include -c multithread.cpp -o obj/multithread.o
c++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -D_MACOS -I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include -c nodemanip.cpp -o obj/nodemanip.o
c++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -D_MACOS -I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include -c rapidjson_extra.cpp -o obj/rapidjson_extra.o
c++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -D_MACOS -I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include -c speedtestutil.cpp -o obj/speedtestutil.o
c++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -D_MACOS -I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include -c subexport.cpp -o obj/subexport.o
c++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -D_MACOS -I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include -c webget.cpp -o obj/webget.o
c++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -D_MACOS -I/usr/local/include -I/usr/local/opt/openssl@1.1/include -I/usr/local/opt/curl/include -c webserver_libevent.cpp -o obj/webserver_libevent.o
c++ -Xlinker -unexported_symbol -Xlinker "*" -o subconverter obj/logger.o obj/main.o obj/misc.o obj/multithread.o obj/nodemanip.o obj/rapidjson_extra.o obj/speedtestutil.o obj/subexport.o obj/webget.o obj/webserver_libevent.o libevent.a curl/lib/.libs/libcurl.a libz.a libssl.a libcrypto.a libyaml-cpp.a bzip2-1.0.6/libbz2.a -ldl -lpthread -O3 -s 

chmod +rx subconverter
chmod +r pref.ini *.yml *.conf README* rules/*
tar czf subconverter_darwin64.tar.gz subconverter pref.ini *.yml *.conf README* rules/

set +xe
