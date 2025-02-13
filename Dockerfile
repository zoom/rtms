FROM node:latest AS base

ENV CWD=/tmp/rtms-sdk
WORKDIR $CWD

RUN apt-get update  \
    && apt-get install -y \
        cmake \
        python3-full \
        python3-pip \
        pipx \
        tini \
    && chmod +x /usr/bin/tini \
    && python3 -m venv /opt/venv

ENV PATH="/opt/venv/bin:$PATH"
ENV LD_LIBRARY_PATH="${CWD}/lib/rtms_csdk/:$LD_LIBRARY_PATH"

RUN pip install "pybind11[global]"  

ENTRYPOINT ["/usr/bin/tini", "--"]

FROM base AS build

WORKDIR $CWD
CMD ["./bin/entry.sh"]
