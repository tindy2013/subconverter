FROM alpine:3.12
LABEL maintainer "tindy.it@gmail.com"

# build minimized
WORKDIR /
RUN apk add --no-cache --virtual .build-tools git g++ build-base linux-headers cmake python2 && \
    apk add --no-cache --virtual .build-deps curl-dev rapidjson-dev libevent-dev pcre2-dev yaml-cpp-dev && \
    git clone https://github.com/svaarala/duktape --depth=1 && \
    cd duktape && \
    python2 -m ensurepip && \
    pip2 install PyYAML --no-cache-dir && \
    mkdir dist && \
    python2 util/dist.py && \
    cd dist/source/src && \
    cc -c -O3 -o duktape.o duktape.c && \
    cc -c -O3 -o duk_module_node.o -I. ../extras/module-node/duk_module_node.c && \
    ar cr libduktape.a duktape.o && \
    ar cr libduktape_module.a duk_module_node.o && \
    install -m0644 ./*.a /usr/lib && \
    install -m0644 duk*.h /usr/include && \
    install -m0644 ../extras/module-node/duk_module_node.h /usr/include && \
    cd ../../../.. && \
    rm -rf duktape /usr/lib/python2.7 && \
    git clone https://github.com/tindy2013/subconverter --depth=1 && \
    cd subconverter && \
    cmake -DCMAKE_BUILD_TYPE=Release . && \
    make -j4 && \
    mv subconverter /usr/bin && \
    mv base ../ && \
    cd .. && \
    rm -rf subconverter && \
    apk add --no-cache --virtual subconverter-deps pcre2 libcurl yaml-cpp libevent && \
    apk del .build-tools .build-deps

# set entry
WORKDIR /base
CMD subconverter
