# syntax=docker/dockerfile:1
#
# Build de mapreduce-pi para Linux (x64, arm64)
#
# Soporta cross-compilation pasando CC/CXX diferentes.
# Instala asio via vcpkg y compila con CMake + Ninja.

ARG BASE_IMAGE=debian:bookworm-slim
FROM ${BASE_IMAGE} AS builder

ARG COMPILER=gcc
ARG VCPKG_TRIPLET=x64-linux
ARG CC=gcc
ARG CXX=g++

# Instalar dependencias de build
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    ninja-build \
    git \
    curl \
    ca-certificates \
    pkg-config \
    ${COMPILER} \
    && rm -rf /var/lib/apt/lists/*

# Instalar cross-compiler si es necesario
RUN if echo "${CC}" | grep -q "aarch64"; then \
        apt-get update && apt-get install -y --no-install-recommends \
            gcc-aarch64-linux-gnu \
            g++-aarch64-linux-gnu \
        && rm -rf /var/lib/apt/lists/*; \
    fi

# Instalar vcpkg
ARG VCPKG_COMMIT=2024.12.16
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && cd /opt/vcpkg \
    && git checkout ${VCPKG_COMMIT} \
    && ./bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# Copiar código fuente
WORKDIR /app
COPY . .

# Build
RUN cmake --preset=docker \
        -DVCPKG_TARGET_TRIPLET=${VCPKG_TRIPLET} \
        -DCMAKE_C_COMPILER=${CC} \
        -DCMAKE_CXX_COMPILER=${CXX} \
    && cmake --build build/docker --config Release

# Stage final: copiar binarios a una imagen mínima
FROM scratch AS export
COPY --from=builder /app/build/docker/mapreduce-server /
COPY --from=builder /app/build/docker/mapreduce-client /
