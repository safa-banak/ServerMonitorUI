# ==========================================
# Stage 1: Builder
# ==========================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN mkdir build && cd build && cmake .. && make -j4

# ==========================================
# Stage 2: Runtime (with VNC)
# ==========================================
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install Qt libraries + VNC tools (Xvfb, x11vnc, fluxbox)
RUN apt-get update && apt-get install -y \
    libqt6widgets6 \
    libqt6network6 \
    libqt6sql6 \
    libqt6sql6-sqlite \
    xvfb \
    x11vnc \
    fluxbox \
    libgl1 \
    libglx-mesa0 \
    libgl1-mesa-dri \
    libxkbcommon-x11-0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled binary and the VNC startup script
COPY --from=builder /app/build/ServerMonitorUI .
COPY start-vnc.sh .

RUN chmod +x start-vnc.sh

# Expose the application port and the VNC port
EXPOSE 12345
EXPOSE 5900

# Start the VNC script
CMD ["./start-vnc.sh"]
