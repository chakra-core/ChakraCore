#/usr/bin/env bash

if [ ! -d "BuildLinux" ]; then
    mkdir BuildLinux;
fi

pushd BuildLinux > /dev/null

DEBUG_BUILD=0
while getopts ":d" opt; do
    case $opt in
        d)
        DEBUG_BUILD=1
        ;;
    esac
done

if [ $DEBUG_BUILD -eq 1 ]; then
    echo Generating Debug makefiles
    cmake -DCMAKE_BUILD_TYPE=Debug ..
else
    echo Generating Retail makefiles
    echo Building Retail;
    cmake ..
fi

make |& tee build.log
popd > /dev/null
