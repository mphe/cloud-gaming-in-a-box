#!/usr/bin/env bash

echo 'test' | netcat -w 1 -u localhost "${1:-4000}"
