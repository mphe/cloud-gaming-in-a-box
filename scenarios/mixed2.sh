#!/usr/bin/env bash

# 50ms RTT
export SERVER_DELAY_MS=25
export CLIENT_DELAY_MS=25

# 1% Packet loss
export SERVER_LOSS_START=0.01
export SERVER_LOSS_STOP=0.99
export CLIENT_LOSS_START=0.01
export CLIENT_LOSS_STOP=0.99

"$@"
