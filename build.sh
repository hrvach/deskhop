#!/bin/sh
# =============================================================================
#
# Build deskhop
#
# Copyright 2024 J. Cody Collins
#
# Last Modified:Thu 2024-07-11 16:14:47 (-0400)
#
# =============================================================================
function showhelp {
    echo "Usage: build.sh [OPTION] [DIRECTORY]"
    echo ""
    echo "Build deskhop project and copy files"
    echo ""
    echo "Options:"
    echo ""
    echo "  -o, output directory [DIRECTORY]"
}

OPT_OVERRIDE_OUTPUT_DIR='false'
while getopts "o:h" OPT; do
    case $OPT in
        o)
            OPT_OVERRIDE_OUTPUT_DIR='true'
            OUTPUT_DIR=${OPTARG}
            ;;
        h)
            showhelp
            exit 0
            ;;
        \?)
            echo "Invalid option: -${OPTARG}" >&2
            showhelp
            exit 1
            ;;
        :)
            echo "Option -${OPTARG} requires an argument." >&2
            showhelp
            exit 1
            ;;
        *)
            echo "Unimplemented option -${OPT}" >&2
            showhelp
            exit 1
            ;;
    esac
done

# Build
cmake -S . -B build
cmake --build build

# Copy output to directory (default cwd)
if ${OPT_OVERRIDE_OUTPUT_DIR}; then
    echo "board_A.uf2 copied to ${OUTPUT_DIR}"
    echo "board_B.uf2 copied to ${OUTPUT_DIR}"
else
    OUTPUT_DIR=.
    echo "board_A.uf2 copied to CWD"
    echo "board_B.uf2 copied to CWD"
fi

cp build/board_A.uf2 ${OUTPUT_DIR}
cp build/board_B.uf2 ${OUTPUT_DIR}

echo "Right Shift + F12 + Left Shift + A -- board A upgrade mode"
echo "Right Shift + F12 + Left Shift + B -- board B upgrade mode"
