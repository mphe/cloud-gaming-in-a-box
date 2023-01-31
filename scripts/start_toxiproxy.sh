#!/usr/bin/env bash

LATENCY="${1:-500}"

cd "$(dirname "$(readlink -f "$0")")" || exit 1
cd ../toxiproxy/dist/ || exit 1

echo "Starting toxiproxy server..."
./toxiproxy-server &
sleep 1

echo
echo "Creating proxy"
./toxiproxy-cli create --listen localhost:9090 --upstream localhost:9095 input_proxy

echo
echo "Adding $LATENCY ms latency"
./toxiproxy-cli toxic add --type latency --attribute latency="$LATENCY" input_proxy

echo
echo "Done"

# Wait until server shuts down
wait
