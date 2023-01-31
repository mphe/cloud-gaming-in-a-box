#!/usr/bin/env bash

DELAY="${1:-0.5}"

cd "$(dirname "$(readlink -f "$0")")" || exit 1

tmux new-session -d ./udp_proxy.py 4004 6004 -d "$DELAY"
tmux split-window ./udp_proxy.py 4005 6005 -d "$DELAY"
tmux split-window ./udp_proxy.py 5004 7004 -d "$DELAY"
tmux split-window ./udp_proxy.py 5005 7005 -d "$DELAY"
tmux select-layout tiled
tmux attach



