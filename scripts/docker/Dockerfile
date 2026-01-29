# Dockerfile for building M65832 Linux kernel
# Uses Ubuntu with build dependencies pre-installed

FROM ubuntu:22.04

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    bc \
    bison \
    flex \
    libssl-dev \
    libelf-dev \
    libncurses-dev \
    git \
    wget \
    python3 \
    rsync \
    device-tree-compiler \
    && rm -rf /var/lib/apt/lists/*

# Create build user (same UID as host user for volume permissions)
ARG UID=1000
ARG GID=1000
RUN groupadd -g ${GID} builder && \
    useradd -u ${UID} -g ${GID} -m builder

# Working directory
WORKDIR /build

# Default to builder user
USER builder

# Entry point
CMD ["/bin/bash"]
