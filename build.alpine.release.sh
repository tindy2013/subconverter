#!/bin/bash
mkdir obj
set -xe

apk add gcc g++ build-base linux-headers cmake make autoconf automake libtool
apk add libressl-dev zlib-dev rapidjson-dev libevent-dev libevent-static

git clone https://github.com/curl/curl
cd curl
./buildconf
./configure --with-ssl --disable-ldap --disable-ldaps --disable-rtsp --without-libidn2 > /dev/null
make install -j4
cd ..

git clone https://github.com/jbeder/yaml-cpp
cd yaml-cpp
cmake .
make install -j4
cd ..

git clone git://sourceware.org/git/bzip2.git
cd bzip2
make install -j4
cd ..

g++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -c logger.cpp -o obj\logger.o
g++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -c main.cpp -o obj\main.o
g++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -c misc.cpp -o obj\misc.o
g++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -c nodemanip.cpp -o obj\nodemanip.o
g++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -c rapidjson_extra.cpp -o obj\rapidjson_extra.o
g++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -c speedtestutil.cpp -o obj\speedtestutil.o
g++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -c subexport.cpp -o obj\subexport.o
g++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -c webget.cpp -o obj\webget.o
g++ -Wall -std=c++17 -fexceptions -DCURL_STATICLIB -c webserver_libevent.cpp -o obj\webserver_libevent.o
g++ -o subconverter obj\logger.o obj\main.o obj\misc.o obj\nodemanip.o obj\rapidjson_extra.o obj\speedtestutil.o obj\subexport.o obj\webget.o obj\webserver_libevent.o  -static -levent -lyaml-cpp -lcurl -lssl -lcrypto -lz -lbz2 -ldl -lpthread -O3 -s  

chmod +rx subconverter pref.ini *.yml
tar czf subconverter_linux64.tar.gz subconverter pref.ini *.yml
