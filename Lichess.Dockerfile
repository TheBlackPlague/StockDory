# syntax=docker/dockerfile:1.7

ARG UBUNTU_VERSION=24.04
ARG BOTLI_REF=6916a09825067862009393250be9dbd39f442dc8
ARG STOCKDORY_IMAGE=ghcr.io/theblackplague/stockdory:latest

# -----------------------------------------------------------------------------
# Prepare BotLi
# -----------------------------------------------------------------------------
FROM ubuntu:${UBUNTU_VERSION} AS botli_prep

ARG BOTLI_REF
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /src_data

RUN apt-get update && apt-get install -y --no-install-recommends git ca-certificates && rm -rf /var/lib/apt/lists/*

RUN git init BotLi && \
    cd BotLi && \
    git remote add origin https://github.com/Torom/BotLi.git && \
    git fetch --depth=1 origin "${BOTLI_REF}" && \
    git -c advice.detachedHead=false checkout FETCH_HEAD && \
    rm -rf .git

# -----------------------------------------------------------------------------
# Lichess Bot (extended from the StockDory image)
# -----------------------------------------------------------------------------
FROM ${STOCKDORY_IMAGE} AS lichess_bot

LABEL org.opencontainers.image.title="StockDory Lichess Bot" \
      org.opencontainers.image.description="StockDory connected to Lichess through BotLi" \
      org.opencontainers.image.source="https://github.com/TheBlackPlague/StockDory" \
      org.opencontainers.image.licenses="LGPL-3.0 AND AGPL-3.0-or-later"

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONUNBUFFERED=1
ENV PYTHONDONTWRITEBYTECODE=1
ENV TZ=America/Chicago

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends python3 && rm -rf /var/lib/apt/lists/*

COPY --from=botli_prep /src_data/BotLi /app
COPY --from=ghcr.io/astral-sh/uv:latest /uv /usr/local/bin/uv

RUN uv pip install --system --no-cache .

COPY .docker/lichess-entrypoint.py /usr/local/bin/lichess-entrypoint.py

RUN chmod 0755 /usr/local/bin/lichess-entrypoint.py

RUN mkdir -p /config && chown -R 1000:1000 /config
VOLUME ["/config"]

ENTRYPOINT ["python3", "/usr/local/bin/lichess-entrypoint.py"]