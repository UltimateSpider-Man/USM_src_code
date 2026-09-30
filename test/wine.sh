#!/bin/bash

RUNNER="$HOME/.local/share/lutris/runners/wine/wine-ge-8-26-x86_64"
export WINE="${RUNNER}/bin/wine"
export WINEDBG="${RUNNER}/bin/winedbg"

export WINEPREFIX="/home/$USER/Games"
export WINEARCH=win64
export WINEDEBUG=fixme-all

