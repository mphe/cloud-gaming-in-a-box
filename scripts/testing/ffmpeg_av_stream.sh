#!/usr/bin/env bash

# WIDTH=800
# HEIGHT=600
FPS=30
WIDTH=1920
HEIGHT=1080
# FPS=60

ffmpeg \
    -framerate "$FPS" -draw_mouse 1 -s "${WIDTH}x${HEIGHT}" \
    -f x11grab -i :0 -r "$FPS" \
    -f pulse -i 'alsa_input.pci-0000_00_1f.3.analog-stereo' \
    -c:a libopus -b:a 48000 -payload_type 111 \
    -pix_fmt yuv420p \
    -filter:v "crop=$WIDTH:$HEIGHT:0:0" \
    -preset ultrafast -tune zerolatency -c:v libx264 -quality realtime \
    -f rtp rtp://127.0.0.1:5004

