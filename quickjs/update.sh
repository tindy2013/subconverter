#!/bin/bash
# Download and extract latest version of quickjs
set -euxo pipefail

ver="$(curl -s --compressed -L https://bellard.org/quickjs/ | grep --perl-regexp --only-matching --max-count=1 '\d{4}-\d{2}-\d{2}')"

# list of files to extract if no arguments are provided
[ $# == 0 ] && set -- *.c *.h VERSION

curl -s -L "https://bellard.org/quickjs/quickjs-${ver}.tar.xz" | unxz | tar --strip-components=1 -xf - -- "${@/#/quickjs-${ver}\/}"

git apply --verbose -- patches/*.patch

# commit (interactive)
git commit -m "update quickjs to ${ver}" -e -- $@
