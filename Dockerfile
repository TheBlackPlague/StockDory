# syntax=docker/dockerfile:1.7

ARG UBUNTU_VERSION=24.04

# -----------------------------------------------------------------------------
# StockDory runtime and native build environment
# -----------------------------------------------------------------------------
FROM ubuntu:${UBUNTU_VERSION} AS stockdory_runtime

LABEL org.opencontainers.image.title="StockDory" \
      org.opencontainers.image.description="Strong Neural Network Chess Engine" \
      org.opencontainers.image.source="https://github.com/TheBlackPlague/StockDory" \
      org.opencontainers.image.licenses="AGPL-3.0-only"

ENV DEBIAN_FRONTEND=noninteractive
ENV CPM_SOURCE_CACHE=/opt/stockdory-cpm

WORKDIR /opt/stockdory

RUN echo 'Acquire::ForceIPv4 "true";' > /etc/apt/apt.conf.d/99force-ipv4

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates cmake curl git gnupg ninja-build lsb-release software-properties-common build-essential && \
    rm -rf /var/lib/apt/lists/*

RUN curl -4 -fsSL https://apt.llvm.org/llvm.sh -o /tmp/llvm.sh && \
    chmod 0755 /tmp/llvm.sh && \
    /tmp/llvm.sh 22 && \
    rm -f /tmp/llvm.sh

RUN clang-22 --version

COPY . .

# Resolve and cache build dependencies while the image is built. The engine
# itself is intentionally not compiled here; it is built natively on startup.
RUN cmake -S . -B /tmp/stockdory-configure -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang-22 \
    -DCMAKE_CXX_COMPILER=clang++-22 \
    -DBUILD_NATIVE=OFF \
    -DBUILD_PRODUCTION=ON && \
    rm -rf /tmp/stockdory-configure

COPY docker/stockdory-entrypoint.sh /usr/local/bin/stockdory-entrypoint
RUN chmod 0755 /usr/local/bin/stockdory-entrypoint

ENTRYPOINT ["/usr/local/bin/stockdory-entrypoint"]
