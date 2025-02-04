FROM node:latest AS base

ARG DEBIAN_FRONTEND=noninteractie

ENV PROJECT=rtms-js-sdk
ENV CWD=/tmp/$PROJECT
ENV TINI_DIR=/usr/bin/tini

WORKDIR $CWD

RUN apt-get update  \
    && apt-get install -y \
    cmake \
    tini \
    && chmod +x ${TINI_DIR}

ENTRYPOINT ["/usr/bin/tini", "--"]

FROM base AS build

RUN npm install -g cmake-js

CMD ["./bin/entry.sh"]
