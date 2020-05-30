FROM alpine:latest
MAINTAINER Tindy X <tindy.it@gmail.com>

# build minimized
RUN apk add git g++ build-base linux-headers cmake && \
    apk add libressl-dev curl-dev rapidjson-dev libevent-dev pcre2-dev yaml-cpp-dev python2 && \
    git clone https://github.com/svaarala/duktape --depth=1 && \
    cd duktape && \
    python2 -m ensurepip && \
    pip2 install PyYAML && \
    python2 util/dist.py && \
    cd dist/src && \
    cc -c -O3 -o duktape.o duktape.c && \
    cc -c -O3 -o duk_module_node.o -I. ../extras/module-node/duk_module_node.c && \
    ar cr libduktape.a duktape.o && \
    ar cr libduktape_module.a duk_module_node.o && \
    install -m0644 *.a /usr/lib && \
    install -m0644 duk*.h /usr/include && \
    install -m0644 ../extras/module-node/duk_module_node.h /usr/include && \
    cd ../../.. && \
    rm -rf duktape /usr/lib/python2.7 && \
    git clone https://github.com/tindy2013/subconverter --depth=1 && \
    cd subconverter && \
    cmake . && \
    make -j4 && \
    mv subconverter base/ && \
    mv base ../ && \
    cd .. && \
    rm -rf subconverter && \
    apk add pcre2 libcurl yaml-cpp libevent && \
    apk del git gcc g++ build-base linux-headers cmake libressl-dev curl-dev rapidjson-dev libevent-dev pcre2-dev yaml-cpp-dev python2 py2-pip

# set entry
WORKDIR /base
CMD ./subconverter
