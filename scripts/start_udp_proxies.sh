#!/usr/bin/env bash

DELAY="${1:-0.5}"

cd "$(dirname "$(readlink -f "$0")")/../udp-proxy" || exit 1

go build
tmux new-session -d ./udp-proxy -l 4004 -r 6004 -d "$DELAY"
tmux split-window ./udp-proxy -l 4005 -r 6005 -d "$DELAY"
tmux split-window ./udp-proxy -l 5004 -r 7004 -d "$DELAY"
tmux split-window ./udp-proxy -l 5005 -r 7005 -d "$DELAY"
tmux select-layout tiled
tmux attach



