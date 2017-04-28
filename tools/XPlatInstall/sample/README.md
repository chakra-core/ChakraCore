# ChakraCore Hello-world Sample (for Shared Library)
This sample shows how to embed ChakraCore with [JavaScript Runtime (JSRT) APIs](http://aka.ms/corejsrtref)
on Linux/OS X, and complete the following tasks:

1. Initializing the JavaScript runtime and creating an execution context.
2. Running scripts and handling values.
3. Printing out script results.

## Requirements
Linux amd64 system with GLIBC 2.17+ or OSX 10.9+

## Build and run the sample
```
> make
> ./sample.o
```

if you are on OSX
```
> make PLATFORM=darwin
> ./sample.o
```

You may receive an error message saying that `libChakraCore.so` or `libChakraCore.dylib` wasn't found.
In that case, you should copy the library file (under `ChakraCoreFiles/lib` folder) into  `/usr/lib` or a
corresponding place in your system.

You may also update `DYLD_LIBRARY_PATH` (osx) or `LD_LIBRARY_PATH` (linux) to `fullPath` location
of `ChakraCoreFiles/lib` folder.

i.e.
Linux
```
export LD_LIBRARY_PATH=CHAKRACORE_LIB_FULL_PATH_HERE:$LD_LIBRARY_PATH
```

OSX
```
export DYLD_LIBRARY_PATH=CHAKRACORE_LIB_FULL_PATH_HERE:$DYLD_LIBRARY_PATH
```

## Help us improve our samples
Help us improve out samples by sending us a pull-request or opening a
[GitHub Issue](https://github.com/Microsoft/Chakra-Samples/issues/new).

Copyright (c) Microsoft. All rights reserved.  Licensed under the MIT license.
See LICENSE file in the project root for full license information.
