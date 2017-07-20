//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

#ifdef VTUNE_PROFILING

#include "VTuneChakraProfile.h"
#include "jitprofiling.h"

static const char LoopStr[] = "Loop";
const utf8char_t VTuneChakraProfile::DynamicCode[] = "Dynamic code";
bool VTuneChakraProfile::isJitProfilingActive = false;

//
// Registers the VTune profiler, if VTune sampling is running, we will get  
// true value for isJitProfilingActive variable.
//
void VTuneChakraProfile::Register()
{
#if ENABLE_NATIVE_CODEGEN
    isJitProfilingActive = (iJIT_IsProfilingActive() == iJIT_SAMPLING_ON);
#endif
}

//
// Unregister and notify VTune that even sampling is done.
//
void VTuneChakraProfile::UnRegister()
{
#if ENABLE_NATIVE_CODEGEN
    if(isJitProfilingActive)
    {
        iJIT_NotifyEvent(iJVM_EVENT_TYPE_SHUTDOWN, NULL);
    }
#endif
}

//
// Log JIT method native load event to VTune 
//
void VTuneChakraProfile::LogMethodNativeLoadEvent(Js::FunctionBody* body, Js::FunctionEntryPointInfo* entryPoint)
{
#if ENABLE_NATIVE_CODEGEN
    if (isJitProfilingActive)
    {
        iJIT_Method_Load methodInfo;
        memset(&methodInfo, 0, sizeof(iJIT_Method_Load));
        const char16* methodName = body->GetExternalDisplayName();
        // Append function line number info to method name so that VTune can distinguish between polymorphic methods
        char16 methodNameBuffer[_MAX_PATH];
        ULONG lineNumber = body->GetLineNumber();
        char16 numberBuffer[20];
        _ltow_s(lineNumber, numberBuffer, 10);
        wcscpy_s(methodNameBuffer, methodName);
        if (entryPoint->GetJitMode() == ExecutionMode::SimpleJit)
        {
            wcscat_s(methodNameBuffer, _u(" Simple"));
        }
        wcscat_s(methodNameBuffer, _u(" {line:"));
        wcscat_s(methodNameBuffer, numberBuffer);
        wcscat_s(methodNameBuffer, _u("}"));

        size_t methodLength = wcslen(methodNameBuffer);
        Assert(methodLength < _MAX_PATH);
        size_t length = methodLength * 3 + 1;
        utf8char_t* utf8MethodName = HeapNewNoThrowArray(utf8char_t, length);
        if (utf8MethodName)
        {
            methodInfo.method_id = iJIT_GetNewMethodID();
            utf8::EncodeIntoAndNullTerminate(utf8MethodName, methodNameBuffer, (charcount_t)methodLength);
            methodInfo.method_name = (char*)utf8MethodName;
            methodInfo.method_load_address = (void*)entryPoint->GetNativeAddress();
            methodInfo.method_size = (uint)entryPoint->GetCodeSize();        // Size in memory - Must be exact

            LineNumberInfo numberInfo[1];

            uint lineCount = (entryPoint->GetNativeOffsetMapCount()) * 2 + 1; // may need to record both .begin and .end for all elements
            LineNumberInfo* pLineInfo = HeapNewNoThrowArray(LineNumberInfo, lineCount);

            if (pLineInfo == NULL || Js::Configuration::Global.flags.DisableVTuneSourceLineInfo)
            {
                // resort to original implementation, attribute all samples to first line
                numberInfo[0].LineNumber = lineNumber;
                numberInfo[0].Offset = 0;
                methodInfo.line_number_size = 1;
                methodInfo.line_number_table = numberInfo;
            }
            else
            {
                int size = entryPoint->PopulateLineInfo(pLineInfo, body);
                methodInfo.line_number_size = size;
                methodInfo.line_number_table = pLineInfo;
            }

            size_t urlLength = 0;
            utf8char_t* utf8Url = GetUrl(body, &urlLength);
            methodInfo.source_file_name = (char*)utf8Url;
            OUTPUT_TRACE(Js::ProfilerPhase, _u("Method load event: %s\n"), methodNameBuffer);
            iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, &methodInfo);

            HeapDeleteArray(lineCount, pLineInfo);

            if (urlLength > 0)
            {
                HeapDeleteArray(urlLength, utf8Url);
            }

            HeapDeleteArray(length, utf8MethodName);
        }
    }
#endif
}

//
// Log loop body load event to VTune 
//
void VTuneChakraProfile::LogLoopBodyLoadEvent(Js::FunctionBody* body, Js::LoopEntryPointInfo* entryPoint, uint16 loopNumber)
{
#if ENABLE_NATIVE_CODEGEN
    if (isJitProfilingActive)
    {
        iJIT_Method_Load methodInfo;
        memset(&methodInfo, 0, sizeof(iJIT_Method_Load));
        const char16* methodName = body->GetExternalDisplayName();
        size_t methodLength = wcslen(methodName);
        methodLength = min(methodLength, (size_t)UINT_MAX); // Just truncate if it is too big
        size_t length = methodLength * 3 + /* spaces */ 2 + _countof(LoopStr) + /*size of loop number*/ 10 + /*NULL*/ 1;
        utf8char_t* utf8MethodName = HeapNewNoThrowArray(utf8char_t, length);
        if(utf8MethodName)
        {
            methodInfo.method_id = iJIT_GetNewMethodID();
            size_t len = utf8::EncodeInto(utf8MethodName, methodName, (charcount_t)methodLength);
            sprintf_s((char*)(utf8MethodName + len), length - len," %s %d", LoopStr, loopNumber + 1);
            methodInfo.method_name = (char*)utf8MethodName;
            methodInfo.method_load_address = (void*)entryPoint->GetNativeAddress();
            methodInfo.method_size = (uint)entryPoint->GetCodeSize();        // Size in memory - Must be exact

            size_t urlLength  = 0;
            utf8char_t* utf8Url = GetUrl(body, &urlLength);
            methodInfo.source_file_name = (char*)utf8Url;

            iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, &methodInfo);
            OUTPUT_TRACE(Js::ProfilerPhase, _u("Loop body load event: %s Loop %d\n"), methodName, loopNumber + 1);

            if(urlLength > 0)
            {
                HeapDeleteArray(urlLength, utf8Url);
            }
            HeapDeleteArray(length, utf8MethodName);
        }
    }
#endif
}

//
// Get URL from source context, called by LogMethodNativeLoadEvent and LogLoopBodyLoadEvent
//
utf8char_t* VTuneChakraProfile::GetUrl(Js::FunctionBody* body, size_t* urlBufferLength )
{
    utf8char_t* utf8Url = NULL;
    if (!body->GetSourceContextInfo()->IsDynamic())
    {
        const wchar* url = body->GetSourceContextInfo()->url;
        if (url)
        {
            size_t urlCharLength = wcslen(url);
            urlCharLength = min(urlCharLength, (size_t)UINT_MAX);       // Just truncate if it is too big

            *urlBufferLength = urlCharLength * 3 + 1;
            utf8Url = HeapNewNoThrowArray(utf8char_t, *urlBufferLength);
            if (utf8Url)
            {
                utf8::EncodeIntoAndNullTerminate(utf8Url, url, (charcount_t)urlCharLength);
            }
        }
    }
    else
    {
        utf8Url = (utf8char_t*)VTuneChakraProfile::DynamicCode;
    }
    return utf8Url;
}

#endif /* VTUNE_PROFILING */
