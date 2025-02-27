FROM node:latest AS base

ENV CWD=/tmp/rtms
ENV PATH="/opt/venv/bin:$PATH"
ENV LD_LIBRARY_PATH="${CWD}/lib/librtmsdk/:$LD_LIBRARY_PATH"

WORKDIR $CWD

RUN apt-get update  \
    && apt-get install -y \
        cmake \
        python3-full \
        python3-pip \
        pipx \
        tini \
    && chmod +x /usr/bin/tini \
    && npm config set update-notifier false \
    && python3 -m venv /opt/venv
    
RUN  npm install -g prebuild \
&& pip install "pybind11[global]" python-dotenv 

ENTRYPOINT ["/usr/bin/tini", "--"]

FROM base AS build

WORKDIR $CWD
CMD ["npm install"]
