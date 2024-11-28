#!/bin/bash
set -xe

brew reinstall rapidjson zlib pcre2 pkgconfig

#git clone https://github.com/curl/curl --depth=1 --branch curl-7_88_1
#cd curl
#./buildconf > /dev/null
#./configure --with-ssl=/usr/local/opt/openssl@1.1 --without-mbedtls --disable-ldap --disable-ldaps --disable-rtsp --without-libidn2 > /dev/null
#cmake -DCMAKE_USE_SECTRANSP=ON -DHTTP_ONLY=ON -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_USE_LIBSSH2=OFF . > /dev/null
#make -j8 > /dev/null
#cd ..

git clone https://github.com/jbeder/yaml-cpp --depth=1
cd yaml-cpp
cmake -DCMAKE_BUILD_TYPE=Release -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF . > /dev/null
make -j6 > /dev/null
sudo make install > /dev/null
cd ..

git clone https://github.com/ftk/quickjspp --depth=1
cd quickjspp
cmake -DCMAKE_BUILD_TYPE=Release .
make quickjs -j6 > /dev/null
sudo install -d /usr/local/lib/quickjs/
sudo install -m644 quickjs/libquickjs.a /usr/local/lib/quickjs/
sudo install -d /usr/local/include/quickjs/
sudo install -m644 quickjs/quickjs.h quickjs/quickjs-libc.h /usr/local/include/quickjs/
sudo install -m644 quickjspp.hpp /usr/local/include/
cd ..

git clone https://github.com/PerMalmberg/libcron --depth=1
cd libcron
git submodule update --init
cmake -DCMAKE_BUILD_TYPE=Release .
make libcron -j6
sudo install -m644 libcron/out/Release/liblibcron.a /usr/local/lib/
sudo install -d /usr/local/include/libcron/
sudo install -m644 libcron/include/libcron/* /usr/local/include/libcron/
sudo install -d /usr/local/include/date/
sudo install -m644 libcron/externals/date/include/date/* /usr/local/include/date/
cd ..

git clone https://github.com/ToruNiina/toml11 --branch="v3.7.1" --depth=1
cd toml11
cmake -DCMAKE_CXX_STANDARD=11 .
sudo make install -j6 > /dev/null
cd ..

cmake -DCMAKE_BUILD_TYPE=Release .
make -j6
rm subconverter
# shellcheck disable=SC2046
c++ -Xlinker -unexported_symbol -Xlinker "*" -o base/subconverter -framework CoreFoundation -framework Security $(find CMakeFiles/subconverter.dir/src/ -name "*.o") "$(brew --prefix zlib)/lib/libz.a" "$(brew --prefix pcre2)/lib/libpcre2-8.a" $(find . -name "*.a") -lcurl -O3

python -m ensurepip
sudo python -m pip install gitpython
python scripts/update_rules.py -c scripts/rules_config.conf

cd base
chmod +rx subconverter
chmod +r ./*
cd ..
mv base subconverter

set +xe
