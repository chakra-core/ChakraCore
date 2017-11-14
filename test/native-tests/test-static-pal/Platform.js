//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var isWindows = !WScript.Platform || WScript.Platform.OS == 'win32';
var path_sep = isWindows ? '\\' : '/';
var isStaticBuild = WScript.Platform && WScript.Platform.LINK_TYPE == 'static';

if (!isStaticBuild) {
    // test will be ignored
    print("# IGNORE_THIS_TEST");
} else {
    var platform = WScript.Platform.OS;
    var binaryPath = WScript.Platform.BINARY_PATH;
    // discard `ch` from path
    binaryPath = binaryPath.substr(0, binaryPath.lastIndexOf(path_sep));
    var makefile =
"ROOT=" + binaryPath + "/../..\n\
$(info $$ROOT is [${ROOT})\n\
IDIR=" + binaryPath + "/../../lib/Jsrt\n\
\n\
LIBRARY_PATH=" + binaryPath + "/lib\n\
PLATFORM=" + platform + "\n\
LDIR=$(LIBRARY_PATH)/libChakraCoreStatic.a \n\
\n\
ifeq (darwin, ${PLATFORM})\n\
\tICU4C_LIBRARY_PATH ?= /usr/local/opt/icu4c\n\
\tCFLAGS=-lstdc++ -std=c++11 -I$(IDIR) -I$(ROOT) -I$(ROOT)/pal/inc/rt -I$(ROOT)/pal/inc -I$(ROOT)/pal -fms-extensions\n\
\tFORCE_STARTS=-Wl,-force_load,\n\
\tFORCE_ENDS=\n\
\tLIBS=-framework CoreFoundation -framework Security -lm -ldl -Wno-c++11-compat-deprecated-writable-strings \
    -Wno-deprecated-declarations -Wno-unknown-warning-option -o sample.o\n\
\tLDIR+=$(ICU4C_LIBRARY_PATH)/lib/libicudata.a \
    $(ICU4C_LIBRARY_PATH)/lib/libicuuc.a \
    $(ICU4C_LIBRARY_PATH)/lib/libicui18n.a\n\
else\n\
\tCFLAGS=-lstdc++ -std=c++0x -I$(IDIR) -I$(ROOT) -I$(ROOT)/pal/inc/rt -I$(ROOT)/pal/inc -I$(ROOT)/pal -fms-extensions \n\
\tFORCE_STARTS=-Wl,--whole-archive\n\
\tFORCE_ENDS=-Wl,--no-whole-archive\n\
\tLIBS=-pthread -lm -ldl -licuuc -Wno-c++11-compat-deprecated-writable-strings \
    -Wno-deprecated-declarations -Wno-unknown-warning-option -o sample.o\n\
endif\n\
\n\
testmake:\n\
\t$(CC) sample.cpp $(CFLAGS) $(FORCE_STARTS) $(LDIR) $(FORCE_ENDS) $(LIBS)\n\
\n\
.PHONY: clean\n\
\n\
clean:\n\
\trm sample.o\n";

    print(makefile)
}
