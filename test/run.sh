#!/bin/bash

if [[ -z "$WINE" ]]; then
    source ./wine.sh
fi

${WINE} $1
