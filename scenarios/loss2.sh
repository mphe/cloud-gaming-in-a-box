#!/usr/bin/env bash

export SERVER_DELAY_MS=0
export SERVER_JITTER_MS=0
export SERVER_LOSS_START=0.01
export SERVER_LOSS_STOP=0.9
export CLIENT_DELAY_MS=0
export CLIENT_JITTER_MS=0
export CLIENT_LOSS_START=0.01
export CLIENT_LOSS_STOP=0.9

"$@"