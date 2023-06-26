#!/usr/bin/env bash

# 100ms RTT

export SERVER_DELAY_MS=50
export CLIENT_DELAY_MS=50

"$@"
