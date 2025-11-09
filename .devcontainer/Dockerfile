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
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
