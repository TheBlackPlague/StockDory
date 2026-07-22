# syntax=docker/dockerfile:1.7

ARG UBUNTU_VERSION=24.04

# -----------------------------------------------------------------------------
# Compile StockDory
# -----------------------------------------------------------------------------
FROM ubuntu:${UBUNTU_VERSION} AS stockdory_compiler

ARG BUILD_JOBS=1
ARG BUILD_NATIVE=OFF

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake curl wget git gnupg ninja-build lsb-release software-properties-common build-essential

RUN rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://apt.llvm.org/llvm.sh -o /tmp/llvm.sh && chmod 0755 /tmp/llvm.sh && /tmp/llvm.sh 20
RUN rm -f /tmp/llvm.sh

RUN clang-20 --version

COPY . .

RUN cmake -S . -B Build -G Ninja -DCMAKE_BUILD_TYPE=Release  \
    -DCMAKE_C_COMPILER=clang-20 -DCMAKE_CXX_COMPILER=clang++-20 \
    -DBUILD_NATIVE="${BUILD_NATIVE}" -DBUILD_PRODUCTION=ON
RUN cmake --build Build --parallel "${BUILD_JOBS}"

RUN test -x Build/StockDory && printf 'uci\nisready\nquit\n' | Build/StockDory | grep -q 'uciok'

# -----------------------------------------------------------------------------
# Minimal standalone UCI runtime
# -----------------------------------------------------------------------------
FROM ubuntu:${UBUNTU_VERSION} AS stockdory_runtime

LABEL org.opencontainers.image.title="StockDory" \
      org.opencontainers.image.description="Strong Neural Network Chess Engine" \
      org.opencontainers.image.source="https://github.com/TheBlackPlague/StockDory" \
      org.opencontainers.image.licenses="LGPL-3.0"

COPY --from=stockdory_compiler --chown=root:root /app/Build/StockDory /usr/local/bin/StockDory

RUN chmod 0755 /usr/local/bin/StockDory && printf 'uci\nisready\nquit\n' | /usr/local/bin/StockDory | grep -q 'readyok'

ENTRYPOINT ["/usr/local/bin/StockDory"]
