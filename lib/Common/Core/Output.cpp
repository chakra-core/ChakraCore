//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonCorePch.h"

#ifndef USING_PAL_STDLIB
#include <string.h>
#include <stdarg.h>
#endif

// Initialization order
//  AB AutoSystemInfo
//  AD PerfCounter
//  AE PerfCounterSet
//  AM Output/Configuration
//  AN MemProtectHeap
//  AP DbgHelpSymbolManager
//  AQ CFGLogger
//  AR LeakReport
//  AS JavascriptDispatch/RecyclerObjectDumper
//  AT HeapAllocator/RecyclerHeuristic
//  AU RecyclerWriteBarrierManager
#pragma warning(disable:4075)       // initializers put in unrecognized initialization area on purpose
#pragma init_seg(".CRT$XCAM")

bool                Output::s_useDebuggerWindow = false;
CriticalSection     Output::s_critsect;
AutoFILE            Output::s_outputFile; // Create a separate output file that is not thread-local.
#ifdef ENABLE_TRACE
Js::ILogger*        Output::s_inMemoryLogger = nullptr;
#ifdef STACK_BACK_TRACE
Js::IStackTraceHelper* Output::s_stackTraceHelper = nullptr;
#endif
unsigned int Output::s_traceEntryId = 0;
#endif

THREAD_ST FILE*    Output::s_file = nullptr;
#ifdef _WIN32
THREAD_ST char16* Output::buffer = nullptr;
THREAD_ST size_t   Output::bufferAllocSize = 0;
THREAD_ST size_t   Output::bufferFreeSize = 0;
#endif
THREAD_ST size_t   Output::s_Column  = 0;
THREAD_ST WORD     Output::s_color = 0;
THREAD_ST bool     Output::s_hasColor = false;
THREAD_ST bool     Output::s_capture = false;

THREAD_ST bool     Output::hasDoneAlignPrefixForThisLine = false;
THREAD_ST bool     Output::usingCustomAlignAndPrefix = false;
THREAD_ST size_t   Output::align = 0;
THREAD_ST const char16* Output::prefix = nullptr;

#define MAX_OUTPUT_BUFFER_SIZE 10 * 1024 * 1024  // 10 MB maximum before we force a flush

size_t __cdecl
Output::VerboseNote(const char16 * format, ...)
{
#ifdef ENABLE_TRACE
    if (Js::Configuration::Global.flags.Verbose)
    {
        AutoCriticalSection autocs(&s_critsect);
        va_list argptr;
        va_start(argptr, format);
        size_t size = vfwprintf(stdout, format, argptr);
        fflush(stdout);
        va_end(argptr);
        return size;
    }
#endif
    return 0;
}

#ifdef ENABLE_TRACE
size_t __cdecl
Output::Trace(Js::Phase phase, const char16 *form, ...)
{
    size_t retValue = 0;

    if(Js::Configuration::Global.flags.Trace.IsEnabled(phase))
    {
        va_list argptr;
        va_start(argptr, form);
        retValue += Output::VTrace(_u("%s: "), Js::PhaseNames[static_cast<int>(phase)], form, argptr);
        va_end(argptr);
    }

    return retValue;
}

size_t __cdecl
Output::Trace2(Js::Phase phase, const char16 *form, ...)
{
    size_t retValue = 0;

    if (Js::Configuration::Global.flags.Trace.IsEnabled(phase))
    {
        va_list argptr;
        va_start(argptr, form);
        retValue += Output::VPrint(form, argptr);
        va_end(argptr);
    }

    return retValue;
}

size_t __cdecl
Output::TraceWithPrefix(Js::Phase phase, const char16 prefix[], const char16 *form, ...)
{
    size_t retValue = 0;

    if (Js::Configuration::Global.flags.Trace.IsEnabled(phase))
    {
        va_list argptr;
        va_start(argptr, form);
        WCHAR prefixValue[512];
        _snwprintf_s(prefixValue, _countof(prefixValue), _TRUNCATE, _u("%s: %s: "), Js::PhaseNames[static_cast<int>(phase)], prefix);
        retValue += Output::VTrace(_u("%s"), prefixValue, form, argptr);
        va_end(argptr);
    }

    return retValue;
}

size_t __cdecl
Output::TraceWithFlush(Js::Phase phase, const char16 *form, ...)
{
    size_t retValue = 0;

    if(Js::Configuration::Global.flags.Trace.IsEnabled(phase))
    {
        va_list argptr;
        va_start(argptr, form);
        retValue += Output::VTrace(_u("%s:"), Js::PhaseNames[static_cast<int>(phase)], form, argptr);
        Output::Flush();
        va_end(argptr);
    }

    return retValue;
}

size_t __cdecl
Output::TraceWithFlush(Js::Flag flag, const char16 *form, ...)
{
    size_t retValue = 0;

    if (Js::Configuration::Global.flags.IsEnabled(flag))
    {
        va_list argptr;
        va_start(argptr, form);
        retValue += Output::VTrace(_u("[-%s]::"), Js::FlagNames[static_cast<int>(flag)], form, argptr);
        Output::Flush();
        va_end(argptr);
    }

    return retValue;
}

size_t
Output::VTrace(const char16* shortPrefixFormat, const char16* prefix, const char16 *form, va_list argptr)
{
    size_t retValue = 0;

#if CONFIG_RICH_TRACE_FORMAT
    if (CONFIG_FLAG(RichTraceFormat))
    {
        InterlockedIncrement(&s_traceEntryId);
        retValue += Output::Print(_u("[%d ~%d %s] "), s_traceEntryId, ::GetCurrentThreadId(), prefix);
    }
    else
#endif
    {
        retValue += Output::Print(shortPrefixFormat, prefix);
    }
    retValue += Output::VPrint(form, argptr);

#ifdef STACK_BACK_TRACE
    // Print stack trace.
    if (s_stackTraceHelper)
    {
        const ULONG c_framesToSkip = 2; // Skip 2 frames -- Output::VTrace and Output::Trace.
        const ULONG c_frameCount = 10;  // TODO: make it configurable.
        const char16 callStackPrefix[] = _u("call stack:");
        if (s_inMemoryLogger)
        {
            // Trace just addresses of functions, avoid symbol info as it takes too much memory.
            // One line for whole stack trace for easier parsing on the jd side.
            const size_t c_msgCharCount = _countof(callStackPrefix) + (1 + sizeof(void*) * 2) * c_frameCount; // 2 hexadecimal digits per byte + 1 for space.
            char16 callStackMsg[c_msgCharCount];
            void* frames[c_frameCount];
            size_t start = 0;
            size_t temp;

            temp = _snwprintf_s(callStackMsg, _countof(callStackMsg), _TRUNCATE, _u("%s"), callStackPrefix);
            Assert(temp != -1);
            start += temp;

            ULONG framesObtained = s_stackTraceHelper->GetStackTrace(c_framesToSkip, c_frameCount, frames);
            Assert(framesObtained <= c_frameCount);
            for (ULONG i = 0; i < framesObtained && i < c_frameCount; ++i)
            {
                Assert(_countof(callStackMsg) >= start);
                temp = _snwprintf_s(callStackMsg + start, _countof(callStackMsg) - start, _TRUNCATE, _u(" %p"), frames[i]);
                Assert(temp != -1);
                start += temp;
            }

            retValue += Output::Print(_u("%s\n"), callStackMsg);
        }
        else
        {
            // Trace with full symbol info.
            retValue += Output::Print(_u("%s\n"), callStackPrefix);
            retValue += s_stackTraceHelper->PrintStackTrace(c_framesToSkip, c_frameCount);
        }
    }
#endif

    return retValue;
}

#ifdef BGJIT_STATS
size_t __cdecl
Output::TraceStats(Js::Phase phase, const char16 *form, ...)
{
    if(PHASE_STATS1(phase))
    {
        va_list argptr;
        va_start(argptr, form);
        size_t ret_val = Output::VPrint(form, argptr);
        va_end(argptr);
        return ret_val;
    }
    return 0;
}
#endif
#endif // ENABLE_TRACE

///----------------------------------------------------------------------------
///
/// Output::Print
///
///     Print the given format string.
///
///
///----------------------------------------------------------------------------

size_t __cdecl
Output::Print(const char16 *form, ...)
{
    va_list argptr;
    va_start(argptr, form);
    size_t ret_val = Output::VPrint(form, argptr);
    va_end(argptr);
    return ret_val;
}

size_t __cdecl
Output::Print(int column, const char16 *form, ...)
{
    Output::SkipToColumn(column);
    va_list argptr;
    va_start(argptr, form);
    size_t ret_val = Output::VPrint(form, argptr);
    va_end(argptr);
    return ret_val;
}

size_t __cdecl
Output::VPrint(const char16 *form, va_list argptr)
{
    char16 buf[2048];
    size_t size;

    size = _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, form, argptr);
    if(size == -1)
    {
        size = _countof(buf) - 1;  // characters written, excluding the terminating null
    }
    return Output::PrintBuffer(buf, size);
}

//
// buf: a null terminated string
// size: characters in buf, excluding the terminating null ==> wcslen(buf)
//
size_t __cdecl
Output::PrintBuffer(const char16 * buf, size_t size)
{
    // Handle custom line prefixing
    bool internallyAllocatedBuffer = false;
    if (usingCustomAlignAndPrefix)
    {
        if (hasDoneAlignPrefixForThisLine && wcschr(buf, '\n') == nullptr)
        {
            // no newlines, and we've already prefixed this line, so nothing to do
        }
        else
        {
            size_t newbufsize = size + align;
            char16* newbuf = (char16*)calloc(newbufsize, sizeof(char16));
            AssertOrFailFastMsg(newbuf != nullptr, "Ran out of memory while printing output");
            internallyAllocatedBuffer = true;
            const char16* currentReadIndex = buf;
            char16* currentWriteIndex = newbuf;
            auto ensureSpace = [&currentWriteIndex, &newbuf, &newbufsize](size_t numCharsWantToWrite)
            {
                size_t charsWritten = (currentWriteIndex - newbuf); // pointer subtraction is number of elements of pointed type between pointers
                size_t remaining = newbufsize - charsWritten;
                if (numCharsWantToWrite + 1 > remaining)
                {
                    char16* tempbuf = (char16*)realloc(newbuf, newbufsize * sizeof(char16) * 2);
                    AssertOrFailFastMsg(tempbuf != nullptr, "Ran out of memory while printing output");
                    newbuf = tempbuf;
                    newbufsize = newbufsize * 2;
                    currentWriteIndex = newbuf + charsWritten;
                }
            };
            const size_t prefixlength = wcslen(prefix);
            size_t oldS_Column = Output::s_Column;
            while (currentReadIndex < buf + size)
            {
                if (!hasDoneAlignPrefixForThisLine)
                {
                    // attempt to write the alignment
                    {
                        unsigned int alignspacesneeded = 1; // always put at least one space
                        if (oldS_Column < align)
                        {
                            alignspacesneeded = (unsigned int)(align - oldS_Column);
                        }
                        ensureSpace(alignspacesneeded);
                        for (unsigned int i = 0; i < alignspacesneeded; i++)
                        {
                            *(currentWriteIndex++) = ' ';
                        }
                    }
                    // attempt to write the prefix
                    ensureSpace(prefixlength);
                    js_wmemcpy_s(currentWriteIndex, (newbuf + newbufsize) - currentWriteIndex, Output::prefix, prefixlength);
                    currentWriteIndex += prefixlength;
                    oldS_Column = align + prefixlength;
                    hasDoneAlignPrefixForThisLine = true;
                }
                const char16* endOfLine = wcschr(currentReadIndex, '\n');
                size_t charsToCopy = 0;
                if (endOfLine != nullptr)
                {
                    charsToCopy = (endOfLine - currentReadIndex) + 1; // We want to grab the newline character as part of this line
                    oldS_Column = 0; // We're ending this line, and want the next to be calculated properly
                    hasDoneAlignPrefixForThisLine = false; // The next line will need this
                }
                else
                {
                    charsToCopy = (buf + size) - currentReadIndex; // the rest of the input buffer
                    oldS_Column += charsToCopy; // Will be reset anyway later on
                }
                ensureSpace(endOfLine - currentReadIndex);
                js_wmemcpy_s(currentWriteIndex, (newbuf + newbufsize) - currentWriteIndex, currentReadIndex, charsToCopy);
                currentReadIndex += charsToCopy;
                currentWriteIndex += charsToCopy;
            }
            // null terminate becuase there's no real reason not to
            ensureSpace(1);
            *(currentWriteIndex++) = '\0';
            buf = newbuf;
            size = (currentWriteIndex - newbuf) - 1; // not counting the terminator here though, to align with vsnwprintf_s's behavior
        }
    }
    Output::s_Column += size;
    const char16 * endbuf = wcschr(buf, '\n');
    while (endbuf != nullptr)
    {
        Output::s_Column = size - (endbuf - buf) - 1;
        endbuf = wcschr(endbuf + 1, '\n');
    }

    bool useConsoleOrFile = true;
    if (!Output::s_capture)
    {
        if (Output::s_useDebuggerWindow)
        {
            OutputDebugStringW(buf);
            useConsoleOrFile = false;
        }
#ifdef ENABLE_TRACE
        if (Output::s_inMemoryLogger)
        {
            s_inMemoryLogger->Write(buf);
            useConsoleOrFile = false;
        }
#endif
    }

    if (useConsoleOrFile)
    {
        if (s_file == nullptr || Output::s_capture)
        {
#ifdef _WIN32
            bool addToBuffer = true;
            if (Output::bufferFreeSize < size + 1)
            {
                if (Output::bufferAllocSize > MAX_OUTPUT_BUFFER_SIZE && !Output::s_capture)
                {
                    Output::Flush();
                    if (Output::bufferFreeSize < size + 1)
                    {
                        DirectPrint(buf);
                        addToBuffer = false;
                    }
                }
                else
                {
                    size_t oldBufferSize = bufferAllocSize - bufferFreeSize;
                    size_t newBufferAllocSize = (bufferAllocSize + size + 1) * 4 / 3;
                    char16 * newBuffer = (char16 *)realloc(buffer, (newBufferAllocSize * sizeof(char16)));
                    if (newBuffer == nullptr)
                    {
                        // See if I can just flush it and print directly
                        Output::Flush();

                        // Reset the buffer
                        free(Output::buffer);
                        Output::buffer = nullptr;
                        Output::bufferAllocSize = 0;
                        Output::bufferFreeSize = 0;

                        // Print it directly
                        DirectPrint(buf);
                        addToBuffer = false;
                    }
                    else
                    {
                        bufferAllocSize = newBufferAllocSize;
                        buffer = newBuffer;
                        bufferFreeSize = bufferAllocSize - oldBufferSize;
                    }
                }
            }
            if (addToBuffer)
            {
                Assert(Output::bufferFreeSize >= size + 1);
                memcpy_s(Output::buffer + Output::bufferAllocSize - Output::bufferFreeSize, Output::bufferFreeSize * sizeof(char16),
                    buf, size * sizeof(char16));
                bufferFreeSize -= size;
                Output::buffer[Output::bufferAllocSize - Output::bufferFreeSize] = _u('\0');  // null terminate explicitly
            }
#else
            DirectPrint(buf);
#endif
        }
        else
        {
            fwprintf_s(Output::s_file, _u("%s"), buf);
        }

        if(s_outputFile != nullptr && !Output::s_capture)
        {
            fwprintf_s(s_outputFile, _u("%s"), buf);
        }
    }

    Output::Flush();

    return size;
}

void Output::Flush()
{
    if (s_capture)
    {
        return;
    }
#ifdef _WIN32
    if (bufferFreeSize != bufferAllocSize)
    {
        DirectPrint(Output::buffer);
        bufferFreeSize = bufferAllocSize;
    }
#endif
    if(s_outputFile != nullptr)
    {
        fflush(s_outputFile);
    }
    _flushall();
}

void Output::DirectPrint(char16 const * string)
{
    AutoCriticalSection autocs(&s_critsect);

    // xplat-todo: support console color
#ifdef _WIN32
    WORD oldValue = 0;
    BOOL restoreColor = FALSE;
    HANDLE hConsole = NULL;
    if (Output::s_hasColor)
    {
        _CONSOLE_SCREEN_BUFFER_INFO info;
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        if (hConsole && GetConsoleScreenBufferInfo(hConsole, &info))
        {
            oldValue = info.wAttributes;
            restoreColor = SetConsoleTextAttribute(hConsole, Output::s_color);
        }
    }
#endif // _WIN32

    fwprintf(stdout, _u("%s"), string);

    // xplat-todo: support console color
#ifdef _WIN32
    if (restoreColor)
    {
        SetConsoleTextAttribute(hConsole, oldValue);
    }
#endif // _WIN32
}
///----------------------------------------------------------------------------
///
/// Output::SkipToColumn
///
///     Inserts spaces up to the column passed in.
///
///----------------------------------------------------------------------------

void
Output::SkipToColumn(size_t column)
{
    size_t columnbias = 0;
    // If we're using a custom alignment and prefix, we want to do this relative to that
    if (usingCustomAlignAndPrefix)
    {
        // If we've already added the alignment and prefix, we need to add the alignment to our column number here
        columnbias = align + wcslen(prefix);
    }
    size_t reference = 0;
    if (Output::s_Column > columnbias)
    {
        reference = Output::s_Column - columnbias;
    }
    if (column <= reference)
    {
        Output::Print(_u(" "));
        return;
    }

    // compute distance to our destination

    size_t dist = column - reference;

    // Print at least one space
    while (dist > 0)
    {
        Output::Print(_u(" "));
        dist--;
    }
}

FILE*
Output::GetFile()
{
    return Output::s_file;
}

FILE*
Output::SetFile(FILE *file)
{
    Output::Flush();
    FILE *oldfile = Output::s_file;
    Output::s_file = file;
    return oldfile;
}

void
Output::SetOutputFile(FILE* file)
{
    if(s_outputFile != nullptr)
    {
        AssertMsg(false, "Output file is being set twice.");
    }
    else
    {
        s_outputFile = file;
    }
}

FILE*
Output::GetOutputFile()
{
    return s_outputFile;
}

#ifdef ENABLE_TRACE
void
Output::SetInMemoryLogger(Js::ILogger* logger)
{
    AssertMsg(s_inMemoryLogger == nullptr, "This cannot be called more than once.");
    s_inMemoryLogger = logger;
}

#ifdef STACK_BACK_TRACE
void
Output::SetStackTraceHelper(Js::IStackTraceHelper* helper)
{
    AssertMsg(s_stackTraceHelper == nullptr, "This cannot be called more than once.");
    s_stackTraceHelper = helper;
}
#endif

#endif // ENABLE_TRACE

//
// Sets the foreground color and returns the old color. Returns 0 on failure
//

WORD
Output::SetConsoleForeground(WORD color)
{
    AutoCriticalSection autocs(&s_critsect);

    // xplat-todo: support console color
#ifdef _WIN32
    _CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hConsole && GetConsoleScreenBufferInfo(hConsole, &info))
    {
        Output::Flush();
        Output::s_color = color | (info.wAttributes & ~15);
        Output::s_hasColor = Output::s_color != info.wAttributes;
        return info.wAttributes;
    }
#endif // _WIN32

    return 0;
}

void
Output::CaptureStart()
{
    Assert(!s_capture);
    Output::Flush();
    s_capture = true;
}

char16 *
Output::CaptureEnd()
{
    Assert(s_capture);
    s_capture = false;
#ifdef _WIN32
    bufferFreeSize = 0;
    bufferAllocSize = 0;
    char16 * returnBuffer = buffer;
    buffer = nullptr;
#else
    char16 * returnBuffer = nullptr;
#endif

    return returnBuffer;
}

void
Output::SetAlignAndPrefix(unsigned int align, const char16 *prefix)
{
    Output::hasDoneAlignPrefixForThisLine = false;
    Output::usingCustomAlignAndPrefix = true;
    Output::prefix = prefix;
    Output::align = align;
}
void
Output::ResetAlignAndPrefix()
{
    Output::hasDoneAlignPrefixForThisLine = false;
    Output::usingCustomAlignAndPrefix = false;
    Output::prefix = nullptr;
    Output::align = 0;
}
