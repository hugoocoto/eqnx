#!/usr/bin/env bash

EXE="$HOME/.local/bin/$1"
DIR="$PWD/$1"
NAME=$(basename "$2")
DESK="$HOME/.local/share/applications/$1.desktop"
SCRIPT_DIR="$(cd "$(dirname "$(readlink $0)")" && pwd)"

if [[ -f "$EXE" ]]; then
    echo "Filename $EXE already exists!"
    exit 1

else if [[ -f "$DEDK" ]]; then
    echo "Filename $DESK already exists!"
    exit 1

else if [[ -z "$1" || -z "$2" ]]; then
    echo "Usage $0 OUTPUT_NAME PROGRAM.esx"
    exit 2
fi
fi
fi

mkdir -p $(dirname "$EXE")
touch $EXE
echo '#!/usr/bin/env bash' > $EXE
echo "SCRIPT_DIR=$SCRIPT_DIR" >> $EXE
echo 'cd $SCRIPT_DIR && ./eqnx -p' "$2" >> $EXE
echo "create: $EXE"
chmod +x $EXE

mkdir -p $(dirname "$DESK")
touch $DESK
echo '[Desktop Entry]' > $DESK
echo "Name=$NAME" >> $DESK
echo "Exec=bash -c \"$EXE\"" >> $DESK
echo 'Icon=' >> $DESK
echo 'Type=Application' >> $DESK
echo "create: $DESK"
