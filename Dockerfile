FROM node:latest AS base

ENV CWD=/tmp/rtms-sdk
WORKDIR $CWD

RUN apt-get update  \
    && apt-get install -y \
    cmake \
    curl \
    tini \
    python3 \
    python3-pip \
    python3-dev \
    unzip \
    zip \
    && chmod +x /usr/bin/tini \
    && npm install -g cmake-js

ENTRYPOINT ["/usr/bin/tini", "--"]

FROM base AS build

WORKDIR $CWD
CMD ["./bin/entry.sh"]
