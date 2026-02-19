#!/usr/bin/env bash

SCRIPT_DIR="$(cd "$(dirname "$(readlink $0)")" && pwd)"
cd $SCRIPT_DIR && ./eqnx -p esx/calendar.esx
