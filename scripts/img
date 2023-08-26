#!/bin/sh
set -e

# This is a simple script that allows you to use ueberzugpp to
# preview images in the terminal by providing the x and y
# coordinates of the image, the width and height of the image,
# and the path to the image file, with $1, $2, $3, $4 and $5
# as arguments, respectively.
# Example usage:
# ./img 0 0 30 30 image.jpg

UB_PID=0
UB_SOCKET=""

case "$(uname -a)" in
    *Darwin*) UEBERZUG_TMP_DIR="$TMPDIR" ;;
    *) UEBERZUG_TMP_DIR="/tmp" ;;
esac

cleanup() {
    ueberzugpp cmd -s "$UB_SOCKET" -a exit
}
trap cleanup HUP INT QUIT TERM EXIT

UB_PID_FILE="$UEBERZUG_TMP_DIR/.$(uuidgen)"
ueberzugpp layer --no-stdin --silent --use-escape-codes --pid-file "$UB_PID_FILE"
UB_PID="$(cat "$UB_PID_FILE")"
export UB_SOCKET="$UEBERZUG_TMP_DIR"/ueberzugpp-"$UB_PID".socket
ueberzugpp cmd -s "$UB_SOCKET" -a add -i PREVIEW -x "$1" -y "$2" --max-width "$3" --max-height "$4" -f "$5"
sleep 2
