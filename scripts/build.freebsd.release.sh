#!/usr/local/bin/bash
set -xe

pkg install pkgconf rapidjson libevent pcre2 toml11 yaml-cpp

[ ! -d quickjspp ] && git clone https://github.com/ftk/quickjspp --depth=1
cd quickjspp
cmake -DCMAKE_BUILD_TYPE=Release .
make quickjs -j4
install -d /tmp/usr/local/lib/
install -m644 quickjs/libquickjs.a /tmp/usr/local/lib/
install -d /tmp/usr/local/include/quickjs/
install -m644 quickjs/quickjs.h quickjs/quickjs-libc.h /tmp/usr/local/include/quickjs/
install -m644 quickjspp.hpp /tmp/usr/local/include/
[ ! -f /usr/local/lib/libquickjs.a ] && ln -swi /tmp/usr/local/lib/libquickjs.a /usr/local/lib/libquickjs.a
[ ! -d /usr/local/include/quickjs ] && ln -swi /tmp/usr/local/include/quickjs /usr/local/include/quickjs
[ ! -f /usr/local/include/quickjspp.hpp ] && ln -swi /tmp/usr/local/include/quickjspp.hpp /usr/local/include/quickjspp.hpp
cd ..

[ ! -d libcron ] && git clone https://github.com/PerMalmberg/libcron --depth=1
cd libcron
git submodule update --init
cmake -DCMAKE_BUILD_TYPE=Release .
make libcron -j4
install -d /tmp/usr/local/lib/
install -m644 libcron/out/Release/liblibcron.a /tmp/usr/local/lib/
install -d /tmp/usr/local/include/libcron/
install -m644 libcron/include/libcron/* /tmp/usr/local/include/libcron/
install -d /tmp/usr/local/include/date/
install -m644 libcron/externals/date/include/date/* /tmp/usr/local/include/date/
[ ! -f /usr/local/lib/liblibcron.a ] && ln -swi /tmp/usr/local/lib/liblibcron.a /usr/local/lib/liblibcron.a
[ ! -d /usr/local/include/libcron ] && ln -swi /tmp/usr/local/include/libcron /usr/local/include/libcron
[ ! -d /usr/local/include/date ] && ln -swi /tmp/usr/local/include/date /usr/local/include/date
cd ..

cp /usr/local/lib/libevent.a .
cp /usr/lib/libz.a .
cp /usr/local/lib/libpcre2-8.a .

cmake -DCMAKE_BUILD_TYPE=Release .
make -j4
rm subconverter

c++ -Xlinker -unexported_symbol -o base/subconverter $(find CMakeFiles/subconverter.dir/src/ -name "*.o") $(find . -name "*.a") -lpthread -L/usr/local/lib -lyaml-cpp -lcurl -O3

cd base
chmod +rx subconverter
chmod +r ./*
cd ..
mv base subconverter

[ -L /usr/local/lib/liblibcron.a ] && unlink /usr/local/lib/liblibcron.a
[ -L /usr/local/include/libcron ] && unlink /usr/local/include/libcron
[ -L /usr/local/include/date ] && unlink /usr/local/include/date
[ -L /usr/local/lib/libquickjs.a ] && unlink /usr/local/lib/libquickjs.a
[ -L /usr/local/include/quickjs ] && unlink /usr/local/include/quickjs
[ -L /usr/local/include/quickjspp.hpp ] && unlink /usr/local/include/quickjspp.hpp

set +xe
