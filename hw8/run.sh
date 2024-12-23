#!/usr/bin/env bash
if [ $# -eq 6 ]
then
    echo "Launching version with environment map"
    ./build/main "$1" "$2" "$3" "$4" "$5" "$6"
else
    echo "Launching version without environment map"
    ./build/main "$1" "$2" "$3" "$4" "$5"
fi;