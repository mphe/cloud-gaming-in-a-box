#!/bin/bash
cd "$(dirname "$(readlink -f "$0")")" || exit 1
cp -v ./slot2.hat ~/.steam/steam/steamapps/common/HatinTime/HatinTimeGame/SaveData
