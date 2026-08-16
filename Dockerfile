# syntax=docker/dockerfile:1.7

ARG UBUNTU_VERSION=24.04

# -----------------------------------------------------------------------------
# StockDory runtime and native build environment
# -----------------------------------------------------------------------------
FROM ubuntu:${UBUNTU_VERSION} AS stockdory_runtime

LABEL org.opencontainers.image.title="StockDory" \
      org.opencontainers.image.description="Strong Neural Network Chess Engine" \
      org.opencontainers.image.source="https://github.com/TheBlackPlague/StockDory" \
      org.opencontainers.image.licenses="LGPL-3.0"

ENV DEBIAN_FRONTEND=noninteractive
ENV CPM_SOURCE_CACHE=/opt/stockdory-cpm

WORKDIR /opt/stockdory

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates cmake curl git gnupg ninja-build lsb-release software-properties-common build-essential && \
    rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://apt.llvm.org/llvm.sh -o /tmp/llvm.sh && \
    chmod 0755 /tmp/llvm.sh && \
    /tmp/llvm.sh 20 && \
    rm -f /tmp/llvm.sh

RUN clang-20 --version

COPY . .

# Resolve and cache build dependencies while the image is built. The engine
# itself is intentionally not compiled here; it is built natively on startup.
RUN cmake -S . -B /tmp/stockdory-configure -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang-20 \
    -DCMAKE_CXX_COMPILER=clang++-20 \
    -DBUILD_NATIVE=OFF \
    -DBUILD_PRODUCTION=ON && \
    rm -rf /tmp/stockdory-configure

COPY docker/stockdory-entrypoint.sh /usr/local/bin/stockdory-entrypoint
RUN chmod 0755 /usr/local/bin/stockdory-entrypoint

ENTRYPOINT ["/usr/local/bin/stockdory-entrypoint"]
