ARG UBUNTU_VERSION=24.04
ARG BOTLI_REF=6916a09825067862009393250be9dbd39f442dc8
ARG STOCKDORY_IMAGE=ghcr.io/theblackplague/stockdory:latest

# -----------------------------------------------------------------------------
# Lichess Configuration
# -----------------------------------------------------------------------------
FROM ubuntu:${UBUNTU_VERSION} AS lichess_configuration

ARG BOTLI_REF
ENV DEBIAN_FRONTEND=noninteractive

RUN echo 'Acquire::ForceIPv4 "true";' > /etc/apt/apt.conf.d/99force-ipv4

WORKDIR /src_data

RUN apt-get update && apt-get install -y --no-install-recommends git ca-certificates && rm -rf /var/lib/apt/lists/*

RUN git init BotLi && \
    cd BotLi && \
    git remote add origin https://github.com/Torom/BotLi.git && \
    git fetch --depth=1 origin "${BOTLI_REF}" && \
    git -c advice.detachedHead=false checkout FETCH_HEAD && \
    rm -rf .git

# -----------------------------------------------------------------------------
# Lichess Runtime
# -----------------------------------------------------------------------------
FROM ${STOCKDORY_IMAGE} AS lichess_runtime

USER root

LABEL org.opencontainers.image.title="StockDory Lichess Bot" \
      org.opencontainers.image.description="StockDory connected to Lichess through BotLi" \
      org.opencontainers.image.source="https://github.com/TheBlackPlague/StockDory" \
      org.opencontainers.image.licenses="AGPL-3.0-only AND AGPL-3.0-or-later"

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1
ENV PYTHONDONTWRITEBYTECODE=1

RUN echo 'Acquire::ForceIPv4 "true";' > /etc/apt/apt.conf.d/99force-ipv4

RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates python3 && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /config && chown stockdory:stockdory /config

WORKDIR /app

COPY --from=lichess_configuration --chown=stockdory:stockdory /src_data/BotLi /app
COPY --from=ghcr.io/astral-sh/uv:0.12.17 /uv /usr/local/bin/uv

COPY --from=lichess_configuration --chown=stockdory:stockdory /src_data/BotLi/config.yml.default /config/config.yml
RUN sed -i \
    -e 's|token: "XXXXXXXXXXXXXXXXXXXXXXXX"|token: ""|' \
    -e 's|dir: "./engines"|dir: "/opt/stockdory-bin"|' \
    -e 's|name: "engine_executable"|name: "StockDory"|' \
    -e 's|      Threads: 4|      Threads: 1|' \
    /config/config.yml

RUN uv sync

USER stockdory

VOLUME ["/config"]

ENTRYPOINT ["/usr/local/bin/stockdory-entrypoint", "--", "/app/.venv/bin/python", "/app/user_interface.py", "--config", "/config/config.yml"]
