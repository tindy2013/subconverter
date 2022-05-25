#!/usr/local/bin/bash

set -e

lib_dir="$(pwd)/library"
owner_id=`stat -f %u .`

function FetchAndBuild {
    [ ! -d quickjspp ] && git clone https://github.com/ftk/quickjspp --depth=1
    cd quickjspp
    cmake -DCMAKE_BUILD_TYPE=Release .
    make quickjs -j4
    install -d $lib_dir/usr/local/lib/
    install -m644 quickjs/libquickjs.a $lib_dir/usr/local/lib/
    install -d $lib_dir/usr/local/include/quickjs/
    install -m644 quickjs/quickjs.h quickjs/quickjs-libc.h $lib_dir/usr/local/include/quickjs/
    install -m644 quickjspp.hpp $lib_dir/usr/local/include/
    cd ..

    [ ! -d libcron ] && git clone https://github.com/PerMalmberg/libcron --depth=1
    cd libcron
    git submodule update --init
    cmake -DCMAKE_BUILD_TYPE=Release .
    make libcron -j4
    install -d $lib_dir/usr/local/lib/
    install -m644 libcron/out/Release/liblibcron.a $lib_dir/usr/local/lib/
    install -d $lib_dir/usr/local/include/libcron/
    install -m644 libcron/include/libcron/* $lib_dir/usr/local/include/libcron/
    install -d $lib_dir/usr/local/include/date/
    install -m644 libcron/externals/date/include/date/* $lib_dir/usr/local/include/date/
    cd ..
}

function Setup {
    pkg install pkgconf rapidjson libevent pcre2 toml11 yaml-cpp

    export -f FetchAndBuild
    export lib_dir
    su `id -un ${owner_id}` -c "bash -c FetchAndBuild"

    [ ! -f /usr/local/lib/libquickjs.a ] && ln -swi $lib_dir/usr/local/lib/libquickjs.a /usr/local/lib/libquickjs.a
    [ ! -d /usr/local/include/quickjs ] && ln -swi $lib_dir/usr/local/include/quickjs /usr/local/include/quickjs
    [ ! -f /usr/local/include/quickjspp.hpp ] && ln -swi $lib_dir/usr/local/include/quickjspp.hpp /usr/local/include/quickjspp.hpp

    [ ! -f /usr/local/lib/liblibcron.a ] && ln -swi $lib_dir/usr/local/lib/liblibcron.a /usr/local/lib/liblibcron.a
    [ ! -d /usr/local/include/libcron ] && ln -swi $lib_dir/usr/local/include/libcron /usr/local/include/libcron
    [ ! -d /usr/local/include/date ] && ln -swi $lib_dir/usr/local/include/date /usr/local/include/date
    su `id -un ${owner_id}` -c "cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ."
}

function Uninstall {
    [ -L /usr/local/lib/liblibcron.a ] && unlink /usr/local/lib/liblibcron.a
    [ -L /usr/local/include/libcron ] && unlink /usr/local/include/libcron
    [ -L /usr/local/include/date ] && unlink /usr/local/include/date

    [ -L /usr/local/lib/libquickjs.a ] && unlink /usr/local/lib/libquickjs.a
    [ -L /usr/local/include/quickjs ] && unlink /usr/local/include/quickjs
    [ -L /usr/local/include/quickjspp.hpp ] && unlink /usr/local/include/quickjspp.hpp
}

function Help {
   # Display Help
   echo
   echo "Setup development enviroment for subconverter."
   echo
   echo "Syntax: develop.freebsd.sh [-i|u|h]"
   echo "options:"
   echo "       -i  --install      --install-dependencies      Install dependencies."
   echo "       -u  --uninstall    --uninstall-dependencies    Uninstall dependencies."
   echo "       -h  --help                                     Print this Help."
   echo
}

function CheckPrivileges {
  if [[ ! $(id -u) -eq 0 ]]; then
      echo "Root privileges required";
      exit -1;
  fi
}


if [[ $# -gt 0 ]]; then
    case $1 in
        -i|--install|--install-dependencies)
        CheckPrivileges
        Setup
        ;;
        -u|--uninstall|--uninstall-dependencies)
        CheckPrivileges
        Uninstall
        ;;
        -h|--help|-*|--*)
        Help
        ;;
    esac
fi

set +e
