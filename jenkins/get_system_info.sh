#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

if [[ $# -eq 0 ]]; then
    echo "No platform passed in- assuming Linux"
    _PLATFORM="linux"
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
    --linux)
        _PLATFORM="linux"
        ;;
    --osx)
        _PLATFORM="osx"
        ;;
    esac

    shift
done

echo
echo "=================================================="
echo

if [[ $_PLATFORM =~ "linux" ]]; then
    echo "Number of processors (nproc):"
    echo
    nproc
elif [[ $_PLATFORM =~ "osx" ]]; then
    echo "Number of processors (sysctl -n hw.logicalcpu):"
    echo
    sysctl -n hw.logicalcpu
else
    echo "Unknown platform"
    exit 1
fi

echo
echo "--------------------------------------------------"
echo

if [[ $_PLATFORM =~ "linux" ]]; then
    echo "Linux version (lsb_release -a):"
    echo
    lsb_release -a
elif [[ $_PLATFORM =~ "osx" ]]; then
    echo "OS X version (sw_vers -productVersion):"
    echo
    sw_vers -productVersion
fi


echo
echo "--------------------------------------------------"
echo

echo "Clang version (clang --version):"
echo
clang --version

echo
echo "--------------------------------------------------"
echo

echo "cmake version (cmake --version):"
echo
cmake --version

echo
echo "=================================================="
echo
