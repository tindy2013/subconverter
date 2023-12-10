FROM alpine:3.16 AS builder
LABEL maintainer="tindy.it@gmail.com"
ARG THREADS="4"
ARG SHA=""

# build minimized
WORKDIR /subconverter
COPY . /subconverter/
RUN set -xe && \
    apk add --no-cache --virtual .build-tools git g++ build-base linux-headers cmake python3 curl unzip p7zip bash && \
    apk add --no-cache --virtual .build-deps curl-dev rapidjson-dev pcre2-dev yaml-cpp-dev lua5.4-dev luajit-dev && \
    curl -fsSL https://xmake.io/shget.text | bash && \
#    git clone https://github.com/ftk/quickjspp --depth=1 && \
#    cd quickjspp && \
#    git submodule update --init && \
#    cmake -DCMAKE_BUILD_TYPE=Release . && \
#    make quickjs -j $THREADS && \
#    install -d /usr/lib/quickjs/ && \
#    install -m644 quickjs/libquickjs.a /usr/lib/quickjs/ && \
#    install -d /usr/include/quickjs/ && \
#    install -m644 quickjs/quickjs.h quickjs/quickjs-libc.h /usr/include/quickjs/ && \
#    install -m644 quickjspp.hpp /usr/include && \
#    cd .. && \
#    git clone https://github.com/PerMalmberg/libcron --depth=1 && \
#    cd libcron && \
#    git submodule update --init && \
#    cmake -DCMAKE_BUILD_TYPE=Release . && \
#    make libcron -j $THREADS && \
#    install -m644 libcron/out/Release/liblibcron.a /usr/lib/ && \
#    install -d /usr/include/libcron/ && \
#    install -m644 libcron/include/libcron/* /usr/include/libcron/ && \
#    install -d /usr/include/date/ && \
#    install -m644 libcron/externals/date/include/date/* /usr/include/date/ && \
#    cd .. && \
#    git clone https://github.com/ToruNiina/toml11 --branch="v3.7.1" --depth=1 && \
#    cd toml11 && \
#    cmake -DCMAKE_CXX_STANDARD=11 . && \
#    make install -j $THREADS && \
#    cd .. && \
#    git clone https://github.com/tindy2013/subconverter --depth=1 && \
#    cd subconverter && \
#    [ -n "$SHA" ] && sed -i 's/\(v[0-9]\.[0-9]\.[0-9]\)/\1-'"$SHA"'/' src/version.h;\
#    cmake -DCMAKE_BUILD_TYPE=Release -DUSE_EXTERNAL_CURL=ON -DUSE_EXTERNAL_RAPIDJSON=ON -DUSE_EXTERNAL_YAML_CPP=ON . && \
#    make -j $THREADS && \
    ls -alh && \
    source "$HOME/.xmake/profile" && \
    xmake f --root --static=false -m release -y -v && \
    xmake --root -v subconverter && \
    python3 -m ensurepip && \
    python3 -m pip install gitpython && \
    python3 scripts/update_rules.py -c scripts/rules_config.conf

# build final image
FROM alpine:3.16
LABEL maintainer="tindy.it@gmail.com"

RUN apk add --no-cache --virtual subconverter-deps pcre2 libcurl yaml-cpp

COPY --from=builder /subconverter/subconverter /usr/bin/
COPY --from=builder /subconverter/base /base/

# set entry
WORKDIR /base
CMD subconverter

EXPOSE 25500/tcp
