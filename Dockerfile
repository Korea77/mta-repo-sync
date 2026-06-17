FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    build-essential \
    cmake \
    g++ \
    make \
    file \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /project

CMD rm -rf build-linux && \
    cmake -B build-linux -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build-linux -- -j$(nproc) && \
    file build-linux/mta-repo-sync.so && \
    ldd build-linux/mta-repo-sync.so