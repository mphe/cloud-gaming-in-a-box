#!/usr/bin/env bash

NAME="$1"
shift 1

if [ -z "$NAME" ]; then
    echo "Usage: $0 <name>"
    exit 1
fi

dir="$PWD"

sudo -v

trap cleanup EXIT INT HUP TERM

cleanup() {
    trap - EXIT INT HUP TERM
    # shellcheck disable=2046
    sudo kill -INT $(jobs -p)
    sleep 1
    # shellcheck disable=2046
    sudo kill -9 $(jobs -p)
    killall ffplay
    killall ffmpeg
    killall udp-proxy
}

cd ../udp-proxy/ || exit 1
# go run . -l 5004 -r 6004 -d 1000 --csv "$dir/proxy_$NAME.csv" "$@" &
go run . -l 5004 -r 6004 --csv "$dir/proxy_$NAME.csv" "$@" &
cd - || exit 1

# ./ffmpeg_play.sh example_5004.sdp > /dev/null 2>&1 &
./ffmpeg_play.sh example.sdp > /dev/null 2>&1 &

# ./udp.py -l localhost 5004 &
# ./udp.py -l localhost 6004 &

# sudo tcpdump -i lo -U -w "$dir/${NAME}_to_proxy.pcap" udp and dst port 5004 &
# sudo tcpdump -i lo -U -w "$dir/${NAME}_from_proxy.pcap" udp and dst port 6004 &

./log_cpu.sh "$dir/cpu_$NAME.csv" &

# for _ in {0..99}; do
#     ./udp.py localhost 5004
#     sleep 0.5
# done

ffmpeg -re -i example_short.mp4 -an -f rtp rtp://127.0.0.1:5004
sleep 1

cleanup
