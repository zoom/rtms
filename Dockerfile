FROM node:latest AS base

ARG DEBIAN_FRONTEND=noninteractive

ENV PROJECT=rtms-sdk
ENV CWD=/tmp/$PROJECT
ENV TINI_DIR=/usr/bin/tini

WORKDIR $CWD

RUN apt-get update  \
    && apt-get install -y \
    cmake \
    curl \
    tini \
    unzip \
    zip \
    && chmod +x ${TINI_DIR} \
    && npm install -g cmake-js

ENTRYPOINT ["/usr/bin/tini", "--"]

FROM base AS build

WORKDIR $CWD
CMD ["./bin/entry.sh"]
