#!/usr/bin/env bash

FILE="../cloud-morph/pkg/core/go/cloudapp/cloudapp.go"

# arg1: search pattern
# arg2: replace pattern
replace() {
    sed -i -r "s/$1/$2/g" "$FILE"
}

main() {
    cd "$(dirname "$(readlink -f "$0")")" || exit 1

    if [ "$1" == "reset" ]; then
        echo "Resetted ports to original"
        replace 6004 4004
        replace 7004 5004
        replace 9095 9090
    else
        echo "Changed ports to allow proxy"
        replace 4004 6004
        replace 5004 7004
        replace 9090 9095
    fi
}

main "$@"
