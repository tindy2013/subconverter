#!/bin/bash
mkdir obj

set -xe

c++ -D_MACOS -Wall -fexceptions -c logger.cpp -o obj/logger.o
c++ -D_MACOS -Wall -fexceptions -c main.cpp -o obj/main.o
c++ -D_MACOS -Wall -fexceptions -c misc.cpp -o obj/misc.o
c++ -D_MACOS -Wall -fexceptions -c nodemanip.cpp -o obj/nodemanip.o
c++ -D_MACOS -Wall -fexceptions -c rapidjson_extra.cpp -o obj/rapidjson_extra.o
c++ -D_MACOS -Wall -fexceptions -c speedtestutil.cpp -o obj/speedtestutil.o
c++ -D_MACOS -Wall -fexceptions -c subexport.cpp -o obj/subexport.o
c++ -D_MACOS -Wall -fexceptions -c webget.cpp -o obj/webget.o
c++ -D_MACOS -Wall -fexceptions -c webserver_libevent.cpp -o obj/webserver_libevent.o
c++ -o subconverter obj/logger.o obj/main.o obj/misc.o obj/nodemanip.o obj/rapidjson_extra.o obj/speedtestutil.o obj/subexport.o obj/webget.o obj/webserver_libevent.o  -levent -lpthread -lyaml-cpp -lcurl -lssl -lcrypto -lz -O3 -s 

chmod +x subconverter
