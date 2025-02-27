FROM node:lts-slim AS base

ENV CWD=/tmp/rtms
ENV PATH="/opt/venv/bin:$PATH"
ENV LD_LIBRARY_PATH="${CWD}/lib/librtmsdk/:$LD_LIBRARY_PATH"

WORKDIR $CWD

RUN apt update && \
    apt install -y --no-install-recommend \
        cmake \
        python3-full \
        python3-pip \
        pipx \
        tini \
    && apt-get clean \
    && apt-get autoremove -y \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* \
    && npm config set update-notifier false \
    && npm install -g prebuild \
    && pip install --no-cache-dir "pybind11[global]" python-dotenv pdoc3

ENTRYPOINT ["/usr/bin/tini", "--"]

FROM base AS run

WORKDIR $CWD
CMD ["npm install"]