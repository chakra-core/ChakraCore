//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef ENABLE_TRACE
namespace Js
{
    // Logs in memory/inproc into StringBuilder.
    // Thread safe.
    // Uses Arena but can be templatized for other allocators.
    // TODO: Consider unchaining and using circular buffer when reaching limit.
    class MemoryLogger : public ILogger
    {
        typedef ArenaAllocator TAllocator;
    private:
        ULONG m_current;
        ULONG m_capacity;   // The number of elements in circular buffer.
        char16** m_log;    // Points to a circular buffer of char16*.
        TAllocator* m_alloc;
        CriticalSection m_criticalSection;

    public:
        static MemoryLogger* Create(TAllocator* alloc, ULONG elementCount);
        MemoryLogger(TAllocator* alloc, ULONG elementCount);
        ~MemoryLogger();
        void Write(const char16* msg) override;
    };

#ifdef STACK_BACK_TRACE
    // Used by output.cpp to print stack trace.
    // Separate class to aboid build errors with jscript9diag.dll which doesn't link with memory.lib.
    class StackTraceHelper : public IStackTraceHelper
    {
        typedef ArenaAllocator TAllocator;
    private:
        TAllocator* m_alloc;
        THREAD_LOCAL static StackBackTrace* s_stackBackTrace;
        StackBackTrace* GetStackBackTrace(ULONG frameCount);
    public:
        static StackTraceHelper* Create(TAllocator* alloc);
        virtual size_t PrintStackTrace(ULONG framesToSkip, ULONG framesToCapture) override;
        virtual ULONG GetStackTrace(ULONG framesToSkip, ULONG framesToCapture, void** stackFrames) override;
    private:
        StackTraceHelper(TAllocator* alloc) : m_alloc(alloc) {}
        void Print();
    };
#endif // STACK_BACK_TRACE
} // namespace Js.
#endif // ENABLE_TRACE
