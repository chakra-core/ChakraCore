#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

IDIR=../include
CC=g++

ifeq (darwin, ${PLATFORM})
LDIR=../lib/libChakraCore.dylib
ICU4C_LIBRARY_PATH ?= /usr/local/opt/icu4c
CFLAGS=-lstdc++ -std=c++11 -I$(IDIR)
FORCE_STARTS=-Wl,-force_load,
FORCE_ENDS=
LIBS=-framework CoreFoundation -framework Security -lm -ldl -Wno-c++11-compat-deprecated-writable-strings \
	-Wno-deprecated-declarations -Wno-unknown-warning-option -o sample.o
LDIR+=$(ICU4C_LIBRARY_PATH)/lib/libicudata.a \
	$(ICU4C_LIBRARY_PATH)/lib/libicuuc.a \
	$(ICU4C_LIBRARY_PATH)/lib/libicui18n.a
else
LDIR=../lib/libChakraCore.so
CFLAGS=-lstdc++ -std=c++0x -I$(IDIR)
FORCE_STARTS=-Wl,--whole-archive
FORCE_ENDS=-Wl,--no-whole-archive
LIBS=-pthread -lm -ldl -licuuc -Wno-c++11-compat-deprecated-writable-strings \
	-Wno-deprecated-declarations -Wno-unknown-warning-option -o sample.o
endif

testmake:
	$(CC) sample.cpp $(CFLAGS) $(FORCE_STARTS) $(LDIR) $(FORCE_ENDS) $(LIBS)

.PHONY: clean

clean:
	rm sample.o
