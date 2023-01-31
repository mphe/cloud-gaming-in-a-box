#!/usr/bin/env bash

netcat -vulp "${1:-5000}"
