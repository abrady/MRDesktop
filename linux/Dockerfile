# syntax=docker/dockerfile:1.4
# MRDesktop Linux Build Container
FROM ubuntu:jammy-20250619

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install basic dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Basic build tools
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    # C++ compiler and tools
    gcc \
    g++ \
    libtool \
    autoconf \
    automake \
    autotools-dev \
    # Assembly and multimedia build tools
    nasm \
    yasm \
    # vcpkg prerequisites
    ca-certificates \
    # X11 and multimedia libraries for Linux desktop support
    libx11-dev \
    libxext-dev \
    libxrandr-dev \
    libxi-dev \
    libxcursor-dev \
    libxinerama-dev \
    # Audio libraries (might be needed for media frameworks)
    libasound2-dev \
    libpulse-dev \
    # Video libraries
    libva-dev \
    libvdpau-dev \
    # Additional tools for development and testing
    gdb \
    strace \
    valgrind \
    # Node.js and npm for development tools
    nodejs \
    npm \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user for building
RUN useradd -m -s /bin/bash mrdesktop

# Set working directory
WORKDIR /workspace

# Copy project files (from parent directory since we're building from linux/ subdir)
COPY .. .

# Install Claude Code globally for development
RUN npm install -g @anthropic-ai/claude-code

# Change ownership to the build user
RUN chown -R mrdesktop:mrdesktop /workspace

# Switch to non-root user
USER mrdesktop

# Default command
CMD ["/bin/bash"]