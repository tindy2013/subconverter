#!/bin/bash
mkdir obj

set -xe

c++ -std=c++17 -Wall -fexceptions -c src/interfaces.cpp -o obj/interfaces.o
c++ -std=c++17 -Wall -fexceptions -c src/logger.cpp -o obj/logger.o
c++ -std=c++17 -Wall -fexceptions -c src/main.cpp -o obj/main.o
c++ -std=c++17 -Wall -fexceptions -c src/misc.cpp -o obj/misc.o
c++ -std=c++17 -Wall -fexceptions -c src/multithread.cpp -o obj/multithread.o
c++ -std=c++17 -Wall -fexceptions -c src/nodemanip.cpp -o obj/nodemanip.o
c++ -std=c++17 -Wall -fexceptions -c src/rapidjson_extra.cpp -o obj/rapidjson_extra.o
c++ -std=c++17 -Wall -fexceptions -c src/speedtestutil.cpp -o obj/speedtestutil.o
c++ -std=c++17 -Wall -fexceptions -c src/subexport.cpp -o obj/subexport.o
c++ -std=c++17 -Wall -fexceptions -c src/webget.cpp -o obj/webget.o
c++ -std=c++17 -Wall -fexceptions -c src/webserver_libevent.cpp -o obj/webserver_libevent.o
c++ -o subconverter obj/*.o -lpcre2-8 -levent -lpthread -lyaml-cpp -lcurl -lssl -lcrypto -lz -O3 -s 

chmod +x subconverter
