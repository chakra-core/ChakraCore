//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Trace light
// Standalone solution to trace function calls in a *nix application

// Notes to Embedder;

// This file is originally implemented for ChakraCore.
// However it is not dependent to any part of the project (and supposed to keep it that way)
// To use with your project, simple copy-paste of this file is sufficient.
// Required compiler args are -finstrument-functions
// -finstrument-functions enables GCC/Clang to inject __cyg_profile_func_enter/exit
// As of 3.9, clang doesn't support finstrument_functions_exclude_file_list or similar (reason for IN_TRACE)
// This version doesn't support multithread profiling. Records the entries from initial thread only

// In case of bug-fix and/or improvements, please contribute back to https://github.com/Microsoft/ChakraCore

// Dealing with output;

// Converting from Address
// OSX ->    atos -o <executable folder> <address here>
// Linux ->  addr2line -e <executable folder> <address here>

// gdb -> info symbol <address here>

// DEFINITIONS

// #define TRACE_OUTPUT_TARGET_FILE // <- uncomment this if you want the trace is written to a file (TraceOutput.txt)
// #define TRACE_OUTPUT_TO_LOGCAT // <- uncomment this if you want to target Logcat for Android instead of stderr
// #define TRACE_FIND_DLI_FNAME // <- uncomment this if you need a binary path for each method address

// #define ENABLE_CC_XPLAT_TRACE // uncomment this to use this code with your project or add ENABLE_CC_XPLAT_TRACE to your compile definitions
#ifdef ENABLE_CC_XPLAT_TRACE
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <string>
#define _GNU_SOURCE
#include <dlfcn.h> // dladdr

#if defined(__APPLE__)
#include <mach-o/dyld.h> // _NSGetExecutablePath
#elif defined(__linux__)
#include <unistd.h> // readlink
#endif


#if defined(__ANDROID__) && defined(TRACE_OUTPUT_TO_LOGCAT)
#include <android/log.h>
#define PRINT_ERROR(...) \
    __android_log_print(ANDROID_LOG_ERROR, "TRACE_LIGHT", __VA_ARGS__)
#else
#define PRINT_ERROR(...) \
    fprintf(stderr, __VA_ARGS__);
#endif

#define TRACE_VERSION_STRING "Trace light v0.01"
#define FILE_BUFFER_SIZE 16384
#define SELF_PATH_SIZE    2048
static int SELF_PATH_LENGTH = 0;
static FILE *FTRACE = NULL;
static _Thread_local size_t LAST_TICK = -1;
static bool IN_TRACE = 0;
static unsigned long CALL_IN_COUNTER = 0;
static char SELF_PATH[SELF_PATH_SIZE];
static char FILE_BUFFER[FILE_BUFFER_SIZE];
static int BUFFER_POS = 0;

__attribute__((no_instrument_function))
static void
GetBinaryLocation()
{
#if defined(__APPLE__)
    uint32_t path_size = SELF_PATH_SIZE;
    char *tmp = nullptr;

    if (_NSGetExecutablePath(SELF_PATH, &path_size))
    {
        SELF_PATH_LENGTH = 0;
        SELF_PATH[0] = char(0); // failed
        return;
    }

    tmp = (char*)malloc(SELF_PATH_SIZE);
    char *result = realpath(SELF_PATH, tmp);
    SELF_PATH_LENGTH = strlen(result);
    memcpy(SELF_PATH, result, SELF_PATH_LENGTH);
    free(tmp);
    SELF_PATH[SELF_PATH_LENGTH] = char(0);
#elif defined(__linux__)
    SELF_PATH_LENGTH = readlink("/proc/self/exe", SELF_PATH, SELF_PATH_SIZE - 1);
    if (SELF_PATH_LENGTH <= 0)
    {
        SELF_PATH_LENGTH = 0;
        SELF_PATH[0] = char(0); // failed
        return;
    }
    SELF_PATH[SELF_PATH_LENGTH] = char(0);
#else
#warning "Implement GetBinaryLocation for this platform"
#endif
}

template <class T>
__attribute__((no_instrument_function))
static void
print_to_buffer(const char* format, const T data)
{
#ifndef TRACE_OUTPUT_TARGET_FILE
    PRINT_ERROR(format, data);
#else
    char TEMP[FILE_BUFFER_SIZE];
    int length = 0;

    if (format)
    {
        length = snprintf(TEMP, FILE_BUFFER_SIZE, format, data);
    }

    if (format && (length + BUFFER_POS < FILE_BUFFER_SIZE - 1))
    {
        memcpy(FILE_BUFFER + BUFFER_POS, TEMP, length);
        BUFFER_POS += length;
    }
    else
    {
        FILE_BUFFER[BUFFER_POS] = 0;
        fprintf(FTRACE, "%s", FILE_BUFFER);
        BUFFER_POS = 0;

        if (format)
        {
            print_to_buffer(format, data);
        }
    }
#endif
}

__attribute__((no_instrument_function))
static inline size_t
rdtsc()
{
#if !defined(_X86_) && !defined(__AMD64__) && !defined(__x86_64__)
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<int64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
#else
    size_t H, L;
    __asm volatile ("rdtsc":"=a"(L), "=d"(H));
#ifdef _X86_
    return L;
#else
    return (H << 32) | L;
#endif
#endif // __x86_64__
}

__attribute__((no_instrument_function))
static double
CPUFreq()
{
    struct timeval tstart, tend;
    size_t start, end;

    struct timezone tzone;
    memset(&tzone, 0, sizeof(tzone));

    start = rdtsc();
    gettimeofday(&tstart, &tzone);

    usleep(1000); // 1ms

    end = rdtsc();
    gettimeofday(&tend, &tzone);

    size_t usec = ((tend.tv_sec - tstart.tv_sec)*1e6)
                + (tend.tv_usec - tstart.tv_usec);

    if (!usec) return 0;
    return (end - start) / usec;
}

__attribute__ ((constructor, no_instrument_function))
void
INIT_TRACE_FILE ()
{
    GetBinaryLocation();
    print_to_buffer("%s\n", TRACE_VERSION_STRING);
    print_to_buffer("CPU Freq at ~%f\n", CPUFreq());
    print_to_buffer("Binary Path: %s\n\n", SELF_PATH);

#ifdef TRACE_OUTPUT_TARGET_FILE
    FTRACE = fopen("TraceOutput.txt", "w");
#else
    FTRACE = (FILE*) 1;
#endif
    LAST_TICK = rdtsc();
}

__attribute__ ((destructor, no_instrument_function))
void
CLOSE_TRACE_FILE ()
{
    IN_TRACE = 1; // no_instrument_function does not block sub calls
#ifdef TRACE_OUTPUT_TARGET_FILE
    if(FTRACE != NULL)
    {
        print_to_buffer(nullptr, 0); // force flush
        fclose(FTRACE);
        printf("\nTrace output is available under ./TraceOutput.txt\n");
    }
#endif
    IN_TRACE = 0;
}

#if defined(TRACE_FIND_DLI_FNAME)
__attribute__((no_instrument_function))
static bool
CMP_REVERSE(const char * fname)
{
    if (SELF_PATH_LENGTH == 0) return false;

    // linux dli_fname may not return the full path
    // compare from the end.
    int pos = strlen(fname) - 1;
    int self_pos = SELF_PATH_LENGTH - 1;
    if (self_pos < pos) return false;
    while(pos >= 0)
    {
        if (fname[pos--] != SELF_PATH[self_pos--]) return false;
    }

    return true;
}
#endif

__attribute__((no_instrument_function))
static void
print_function(void *func)
{
    print_to_buffer("[%p]", func);

#ifdef TRACE_FIND_DLI_FNAME
    Dl_info info;
    if (dladdr(func, &info) != 0)
    {
        if(CMP_REVERSE(info.dli_fname))
        {
            print_to_buffer("<%s>", "self");
        }
        else
        {
            print_to_buffer("<%s>", info.dli_fname);
        }
    }
#endif
}

__attribute__((no_instrument_function))
static void
print_indent()
{
    char space[512 + 1]; // max spaces
    int length = (CALL_IN_COUNTER * 2) + 1;
    if (length > 512) length = 512;

    for(int i = 0; i < length; i++) space[i] = ' ';
    space[length - 1] = 0;
    print_to_buffer("%s", space);
}

__attribute__((no_instrument_function))
static void
print_trace(void *func, void *caller, bool is_enter)
{
    if (FTRACE == NULL) return;
    if (LAST_TICK == -1) return; // wrong thread
    if (IN_TRACE) return;        // clang doesn't support finstrument_functions_exclude_file_list
    IN_TRACE = 1;

    size_t tick = rdtsc();
    if (!is_enter)
    {
        CALL_IN_COUNTER--;
        LAST_TICK = tick;
        IN_TRACE = 0;
        return;
    }

    print_indent();
    print_function(func);
    print_to_buffer("(%lu)\n", tick - LAST_TICK);

    LAST_TICK = rdtsc();

    if (is_enter) CALL_IN_COUNTER++;
    IN_TRACE = 0;
}

#define PROFILE_FUNC_ENTER __cyg_profile_func_enter(void *func,  void *caller)
#define PROFILE_FUNC_EXIT  __cyg_profile_func_exit(void *func,  void *caller)

extern "C"
{
    __attribute__((no_instrument_function))
    extern void
    PROFILE_FUNC_ENTER
    {
        print_trace(func, caller, true);
    }

    __attribute__((no_instrument_function))
    extern void
    PROFILE_FUNC_EXIT
    {
        print_trace(func, caller, false);
    }
}
#endif // ENABLE_CC_XPLAT_TRACE
