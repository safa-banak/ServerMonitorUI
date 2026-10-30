# ==========================================
# مرحله ۱: محیط ساخت (Builder)
# ==========================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# نصب کامپایلر و Qt 6.4 (که توی اوبونتو 24.04 نصبه)
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
# مرحله ۲: محیط اجرا (Runtime)
# ==========================================
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libqt6widgets6 \
    libqt6network6 \
    libqt6sql6 \
    libqt6sql6-sqlite \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/ServerMonitorUI .

CMD ["./ServerMonitorUI","-platform", "offscreen"]
