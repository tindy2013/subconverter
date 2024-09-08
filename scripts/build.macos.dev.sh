#!/bin/bash
set -xe

# remove all old deps
rm -rf curl libcron quickjspp rapidjson toml11 yaml-cpp
# remove tmp folder
rm -rf tmp

# brew reinstall rapidjson zlib pcre2 pkgconfig

#git clone https://github.com/curl/curl --depth=1 --branch curl-7_88_1
#cd curl
#./buildconf > /dev/null
#./configure --with-ssl=/usr/local/opt/openssl@1.1 --without-mbedtls --disable-ldap --disable-ldaps --disable-rtsp --without-libidn2 > /dev/null
#cmake -DCMAKE_USE_SECTRANSP=ON -DHTTP_ONLY=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_USE_LIBSSH2=OFF . > /dev/null
#make -j8 > /dev/null
#cd ..

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DCMAKE_BUILD_TYPE=Debug -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF . > /dev/null
make install -j8 > /dev/null
cd ..

git clone https://github.com/ftk/quickjspp --depth=1
cd quickjspp
cmake -DCMAKE_BUILD_TYPE=Debug .
make quickjs -j8
install -d /usr/local/lib/quickjs/
install -m644 quickjs/libquickjs.a /usr/local/lib/quickjs/
install -d /usr/local/include/quickjs/
install -m644 quickjs/quickjs.h quickjs/quickjs-libc.h /usr/local/include/quickjs/
install -m644 quickjspp.hpp /usr/local/include/
cd ..

git clone https://github.com/PerMalmberg/libcron --depth=1
cd libcron
git submodule update --init
cmake -DCMAKE_BUILD_TYPE=Debug .
make libcron install -j8
install -m644 libcron/out/Debug/liblibcron.a /usr/local/lib/
install -d /usr/local/include/libcron/
install -m644 libcron/include/libcron/* /usr/local/include/libcron/
install -d /usr/local/include/date/
install -m644 libcron/externals/date/include/date/* /usr/local/include/date/
cd ..

git clone https://github.com/ToruNiina/toml11 --depth=1
cd toml11
cmake -DCMAKE_CXX_STANDARD=11 .
make install -j4
cd ..

cp /usr/local/opt/zlib/lib/libz.a .
cp /usr/local/lib/libpcre2-8.a .

cmake -DCMAKE_BUILD_TYPE=Debug .
make -j8
rm subconverter
# shellcheck disable=SC2046
c++ -Xlinker -unexported_symbol -Xlinker "*" -o base/subconverter -framework CoreFoundation -framework Security $(find CMakeFiles/subconverter.dir/src/ -name "*.o") $(find . -name "*.a") -lcurl -O3

python -m ensurepip
python -m pip install gitpython
python scripts/update_rules.py -c scripts/rules_config.conf

cd base
chmod +rx subconverter
chmod +r ./*
cd ..
cp base subconverter

set +xe
