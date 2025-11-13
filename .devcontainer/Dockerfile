FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-22.04

RUN apt-get update && apt-get install -y \
    valgrind \
    clang-format \
    clang-tidy \
    irssi \
    htop \
    netcat-openbsd \
    telnet \
    ngircd \
    python3 \
    python3-pip \
    curl \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install uv for Python package management
RUN curl -LsSf https://astral.sh/uv/install.sh | sh && \
    echo 'export PATH="$HOME/.cargo/bin:$PATH"' >> /etc/bash.bashrc
