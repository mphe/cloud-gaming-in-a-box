#!/usr/bin/env bash
cd "$(dirname "$(readlink -f "$0")")" || exit 1
cd ../cloud-morph || exit 1
go run server.go
sudo docker stop appvm
