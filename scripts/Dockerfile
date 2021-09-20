FROM alpine:3.13
LABEL maintainer "tindy.it@gmail.com"

# build minimized
WORKDIR /
RUN apk add --no-cache --virtual .build-tools git g++ build-base linux-headers cmake && \
    apk add --no-cache --virtual .build-deps curl-dev rapidjson-dev libevent-dev pcre2-dev yaml-cpp-dev && \
    git clone https://github.com/ftk/quickjspp --depth=1 && \
    cd quickjspp && \
    git submodule update --init && \
    cmake -DCMAKE_BUILD_TYPE=Release . && \
    make quickjs -j2 && \
    install -m644 quickjs/libquickjs.a /usr/lib && \
    install -d /usr/include/quickjs/ && \
    install -m644 quickjs/quickjs.h quickjs/quickjs-libc.h /usr/include/quickjs/ && \
    install -m644 quickjspp.hpp /usr/include && \
    cd .. && \
    git clone https://github.com/PerMalmberg/libcron --depth=1 && \
    cd libcron && \
    git submodule update --init && \
    cmake -DCMAKE_BUILD_TYPE=Release . && \
    make libcron -j4 && \
    install -m644 libcron/out/Release/liblibcron.a /usr/lib/ && \
    install -d /usr/include/libcron/ && \
    install -m644 libcron/include/libcron/* /usr/include/libcron/ && \
    install -d /usr/include/date/ && \
    install -m644 libcron/externals/date/include/date/* /usr/include/date/ && \
    cd .. && \
    git clone https://github.com/ToruNiina/toml11 --depth=1 && \
    cd toml11 && \
    cmake . && \
    make install -j4 && \
    cd .. && \
    git clone https://github.com/tindy2013/subconverter --depth=1 && \
    cd subconverter && \
    cmake -DCMAKE_BUILD_TYPE=Release . && \
    make -j4 && \
    mv subconverter /usr/bin && \
    mv base ../ && \
    cd .. && \
    rm -rf subconverter quickjspp libcron toml11 && \
    apk add --no-cache --virtual subconverter-deps pcre2 libcurl yaml-cpp libevent && \
    apk del .build-tools .build-deps

# set entry
WORKDIR /base
CMD subconverter
