#!/bin/bash

# 1. Start virtual display (Xvfb)
Xvfb :99 -screen 0 1024x768x24 &

# 2. Wait a bit for Xvfb to start
sleep 2

# 3. Start window manager (fluxbox)
fluxbox &

# 4. Start VNC server
x11vnc -display :99 -nopw -forever -shared -rfbport 5900 &

# 5. Wait a bit for VNC to start
sleep 2

# 6. Set display and run the app
export DISPLAY=:99
./ServerMonitorUI
