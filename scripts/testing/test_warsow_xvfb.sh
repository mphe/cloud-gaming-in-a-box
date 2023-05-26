#!/usr/bin/env bash

cd "$(dirname "$(readlink -f "$0")")" || exit 1

WIDTH=684
HEIGHT=384
# WIDTH=1920
# HEIGHT=1080
FPS=30
OTHER_DISPLAY=:100

tmux new-session -d Xvfb "$OTHER_DISPLAY" -screen 0 "${WIDTH}x${HEIGHT}x24"
tmux split-window env DISPLAY="$OTHER_DISPLAY" vglrun warsow
tmux split-window ffmpeg -framerate "$FPS" -f x11grab -draw_mouse 1 -s "${WIDTH}x${HEIGHT}" -i "$OTHER_DISPLAY" -pix_fmt yuv420p -tune zerolatency -filter:v "crop=$WIDTH:$HEIGHT:0:0" -c:v libx264 -quality realtime -f rtp rtp://127.0.0.1:5004
tmux split-window ffplay -protocol_whitelist "file,rtp,udp" -i stream.sdp
tmux select-layout tiled
tmux attach

