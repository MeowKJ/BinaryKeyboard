# BinaryKeyboard firmware builder image
# Contains CMake, Ninja, and the WCH RISC-V toolchain (riscv-wch-elf-gcc)
#
# The toolchain archive is downloaded from this repo's GitHub Release (tag: toolchain-linux).
# See .github/workflows/build-docker-image.yml for build instructions.

FROM ubuntu:22.04

ARG TOOLCHAIN_ARCHIVE=MRS_Toolchain_Linux_x64_V230.tar.xz
ARG TOOLCHAIN_INSTALL=/opt/wch-toolchain

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -q \
    && apt-get install -y --no-install-recommends \
       cmake \
       ninja-build \
       git \
       ca-certificates \
       xz-utils \
    && rm -rf /var/lib/apt/lists/*

# Copy and extract toolchain
COPY ${TOOLCHAIN_ARCHIVE} /tmp/toolchain.tar.xz
RUN mkdir -p ${TOOLCHAIN_INSTALL} \
    && tar -xJf /tmp/toolchain.tar.xz -C ${TOOLCHAIN_INSTALL} \
    && rm /tmp/toolchain.tar.xz

# WCH toolchain bin is at: Toolchain/RISC-V Embedded GCC12/bin/
ENV PATH="${TOOLCHAIN_INSTALL}/Toolchain/RISC-V Embedded GCC12/bin:${PATH}"

WORKDIR /workspace
