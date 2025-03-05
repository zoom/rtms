FROM node:lts AS base

ARG TARGET=all

ENV CWD=/tmp/rtms
ENV PATH="/opt/venv/bin:$PATH"

WORKDIR $CWD

RUN apt update && apt install -y \
    cmake \
    tini \
    && npm config set update-notifier false

RUN if [ "$TARGET" = "js" ] || [ "$TARGET" = "all" ]; then \
        npm install -g prebuild; \
    fi && \
    if [ "$TARGET" = "py" ] || [ "$TARGET" = "all" ]; then \
        echo "Installing Python dependencies..." && \
        apt install -y python3-full python3-pip pipx && \
        python3 -m venv /opt/venv && \
        python -m pip install "pybind11[global]" python-dotenv pdoc3; \
    fi && \
    if [ "$TARGET" = "go" ] || [ "$TARGET" = "all" ]; then \
        echo "Installing Go dependencies..." && \
        apt install -y golang; \
    fi

ENTRYPOINT ["/usr/bin/tini", "--"]
CMD [ "npm", "install" ]