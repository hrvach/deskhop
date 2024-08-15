#!/bin/sh
# =============================================================================
#
# Build deskhop
#
# Copyright 2024 J. Cody Collins
#
# Last Modified:Fri 2024-08-09 12:35:42 (-0400)
#
# =============================================================================
BUILDFILE=deskhop.uf2

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
    echo "${BUILDFILE} copied to ${OUTPUT_DIR}"
else
    OUTPUT_DIR=.
    echo "${BUILDFILE} copied to CWD"
fi

cp build/${BUILDFILE} ${OUTPUT_DIR}

echo "Right Shift + F12 + Left Shift + A -- board A upgrade mode"
echo "Right Shift + F12 + Left Shift + B -- board B upgrade mode"
