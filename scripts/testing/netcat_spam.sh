#!/usr/bin/env bash

cd "$(dirname "$(readlink -f "$0")")" || exit 1
while true; do
    ./netcat_send_udp.sh "$1"
done
