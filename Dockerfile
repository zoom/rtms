FROM debian:bookworm-slim AS base

ARG TARGET=all

ENV CWD=/tmp/rtms
WORKDIR $CWD

# Install system dependencies
RUN apt update && apt install -y \
    build-essential \
    cmake \
    curl \
    git \
    tini \
    ca-certificates \
    libssl-dev \
    zlib1g-dev \
    libbz2-dev \
    libreadline-dev \
    libsqlite3-dev \
    libncursesw5-dev \
    xz-utils \
    tk-dev \
    libxml2-dev \
    libxmlsec1-dev \
    libffi-dev \
    liblzma-dev \
    && rm -rf /var/lib/apt/lists/*

# Install nvm (Node Version Manager) and Node.js
ENV NVM_DIR="/root/.nvm"
ENV NODE_VERSION="24"
RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash \
    && . "$NVM_DIR/nvm.sh" \
    && nvm install $NODE_VERSION \
    && nvm alias default $NODE_VERSION \
    && nvm use default \
    && npm config set update-notifier false \
    && ln -sf "$NVM_DIR/versions/node/$(nvm version default)" /usr/local/node

# Add Node.js to PATH
ENV PATH="/usr/local/node/bin:$PATH"

# Install pyenv (Python Version Manager)
ENV PYENV_ROOT="/root/.pyenv"
ENV PYTHON_VERSION="3.13"
RUN git clone https://github.com/pyenv/pyenv.git $PYENV_ROOT \
    && cd $PYENV_ROOT && src/configure && make -C src \
    && echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.bashrc \
    && echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.bashrc \
    && echo 'eval "$(pyenv init -)"' >> ~/.bashrc

# Add pyenv to PATH and install Python
ENV PATH="$PYENV_ROOT/shims:$PYENV_ROOT/bin:$PATH"
RUN pyenv install $PYTHON_VERSION \
    && pyenv global $PYTHON_VERSION \
    && python --version

# Install Task (go-task)
RUN sh -c "$(curl --location https://taskfile.dev/install.sh)" -- -d -b /usr/local/bin

# TARGET-based conditional installations
RUN if [ "$TARGET" = "js" ] || [ "$TARGET" = "all" ]; then \
        echo "Installing Node.js build tools..." && \
        npm install -g prebuild; \
    fi

RUN if [ "$TARGET" = "py" ] || [ "$TARGET" = "all" ]; then \
        echo "Installing Python build tools..." && \
        pip install --no-cache-dir --upgrade pip && \
        pip install --no-cache-dir build "pybind11[global]" python-dotenv pdoc3 auditwheel twine; \
    fi

RUN if [ "$TARGET" = "go" ] || [ "$TARGET" = "all" ]; then \
        echo "Go bindings not yet implemented..."; \
    fi

ENTRYPOINT ["/usr/bin/tini", "--"]
