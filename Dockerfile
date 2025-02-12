FROM node:latest AS base

ENV CWD=/tmp/rtms-sdk
WORKDIR $CWD

RUN apt-get update  \
    && apt-get install -y \
        cmake \
        curl \
        python3-full \
        python3-pip \
        tini \
        unzip \
        zip \
    && chmod +x /usr/bin/tini \
    && npm install -g cmake-js \
    && python3 -m venv /opt/venv

ENV PATH="/opt/venv/bin:$PATH"
RUN pip install pybind11

ENTRYPOINT ["/usr/bin/tini", "--"]

FROM base AS build

WORKDIR $CWD
CMD ["./bin/entry.sh"]
