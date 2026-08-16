#!/bin/sh
set -eu

SOURCE_DIR=/opt/stockdory
BUILD_DIR=/tmp/stockdory-build
ENGINE=/usr/local/bin/StockDory
BUILD_JOBS="${STOCKDORY_BUILD_JOBS:-$(nproc)}"

printf 'Compiling StockDory natively for this host (%s)...\n' "$(uname -m)"

rm -rf "$BUILD_DIR"

cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang-20 \
    -DCMAKE_CXX_COMPILER=clang++-20 \
    -DBUILD_NATIVE=ON \
    -DBUILD_PRODUCTION=ON

cmake --build "$BUILD_DIR" --parallel "$BUILD_JOBS"
install -m 0755 "$BUILD_DIR/StockDory" "$ENGINE"

printf 'uci\nisready\nquit\n' | "$ENGINE" | grep -q 'readyok'
rm -rf "$BUILD_DIR"

if [ "${1:-}" = "--" ]; then
    shift
    exec "$@"
fi

exec "$ENGINE" "$@"
