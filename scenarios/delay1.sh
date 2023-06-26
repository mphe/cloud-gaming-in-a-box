#!/usr/bin/env bash

# 50ms RTT

export SERVER_DELAY_MS=25
export CLIENT_DELAY_MS=25

"$@"
