#!/usr/bin/env bash

# 0.1% Packet loss

export SERVER_LOSS_START=0.001
export SERVER_LOSS_STOP=0.999
export CLIENT_LOSS_START=0.001
export CLIENT_LOSS_STOP=0.999

"$@"
