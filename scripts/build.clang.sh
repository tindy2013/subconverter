#!/bin/bash
mkdir obj

set -xe

c++ -std=c++17 -D_MACOS -Wall -fexceptions -c src/logger.cpp -o obj/logger.o
c++ -std=c++17 -D_MACOS -Wall -fexceptions -c src/main.cpp -o obj/main.o
c++ -std=c++17 -D_MACOS -Wall -fexceptions -c src/misc.cpp -o obj/misc.o
c++ -std=c++17 -D_MACOS -Wall -fexceptions -c src/multithread.cpp -o obj/multithread.o
c++ -std=c++17 -D_MACOS -Wall -fexceptions -c src/nodemanip.cpp -o obj/nodemanip.o
c++ -std=c++17 -D_MACOS -Wall -fexceptions -c src/rapidjson_extra.cpp -o obj/rapidjson_extra.o
c++ -std=c++17 -D_MACOS -Wall -fexceptions -c src/speedtestutil.cpp -o obj/speedtestutil.o
c++ -std=c++17 -D_MACOS -Wall -fexceptions -c src/subexport.cpp -o obj/subexport.o
c++ -std=c++17 -D_MACOS -Wall -fexceptions -c src/webget.cpp -o obj/webget.o
c++ -std=c++17 -D_MACOS -Wall -fexceptions -c src/webserver_libevent.cpp -o obj/webserver_libevent.o
c++ -o subconverter obj/logger.o obj/main.o obj/misc.o obj/multithread.o obj/nodemanip.o obj/rapidjson_extra.o obj/speedtestutil.o obj/subexport.o obj/webget.o obj/webserver_libevent.o -lpcrecpp -lpcre -levent -lpthread -lyaml-cpp -lcurl -lssl -lcrypto -lz -O3 -s 

chmod +x subconverter
