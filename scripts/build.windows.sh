#!/bin/bash
set -xe

git clone https://github.com/curl/curl
cd curl
cmake -DHTTP_ONLY=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -G "Unix Makefiles" .
make -j4
cd ..

cmake -G "Unix Makefiles" .
make -j2
rm subconverter.exe
g++ -o base/subconverter CMakeFiles/subconverter.dir/src/*.obj curl/lib/libcurl.a -static -lpcre2-8 -levent -lyaml-cpp -lssl -lcrypto -lwsock32 -lws2_32 -lz

mv base subconverter
zip -q -r subconverter_win64.zip subconverter/

set +xe
