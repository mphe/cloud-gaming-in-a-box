#!/usr/bin/env bash

# 1% Packet loss

export SERVER_LOSS_START=0.01
export SERVER_LOSS_STOP=0.99
export CLIENT_LOSS_START=0.01
export CLIENT_LOSS_STOP=0.99

"$@"
