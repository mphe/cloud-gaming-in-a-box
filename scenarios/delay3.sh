#!/usr/bin/env bash

# 200ms RTT

export SERVER_DELAY_MS=100
export CLIENT_DELAY_MS=100

"$@"
