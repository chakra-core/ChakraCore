//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#ifdef ENABLE_SCRIPT_DEBUGGING
namespace Js
{
    DebugContext::DebugContext(Js::ScriptContext * scriptContext) :
        scriptContext(scriptContext),
        hostDebugContext(nullptr),
        diagProbesContainer(nullptr),
        isClosed(false),
        debuggerMode(DebuggerMode::NotDebugging),
        isDebuggerRecording(true),
        isReparsingSource(false)
    {
        Assert(scriptContext != nullptr);
    }

    DebugContext::~DebugContext()
    {
        Assert(this->scriptContext != nullptr);
        Assert(this->hostDebugContext == nullptr);
        Assert(this->diagProbesContainer == nullptr);
    }

    void DebugContext::Initialize()
    {
        Assert(this->diagProbesContainer == nullptr);
        this->diagProbesContainer = HeapNew(ProbeContainer);
        this->diagProbesContainer->Initialize(this->scriptContext);
    }

    void DebugContext::Close()
    {
        if (this->isClosed)
        {
            return;
        }

        AssertMsg(this->scriptContext->IsActuallyClosed(), "Closing DebugContext before ScriptContext close might have consequences");

        this->isClosed = true;

        // Release all memory and do all cleanup. No operation should be done after isClosed is set

        Assert(this->scriptContext != nullptr);

        if (this->diagProbesContainer != nullptr)
        {
            this->diagProbesContainer->Close();
            HeapDelete(this->diagProbesContainer);
            this->diagProbesContainer = nullptr;
        }

        if (this->hostDebugContext != nullptr)
        {
            this->hostDebugContext->Delete();
            this->hostDebugContext = nullptr;
        }
    }

    bool DebugContext::IsSelfOrScriptContextClosed() const
    {
        return (this->IsClosed() || this->scriptContext->IsClosed());
    }

    void DebugContext::SetHostDebugContext(HostDebugContext * hostDebugContext)
    {
        Assert(this->hostDebugContext == nullptr);
        Assert(hostDebugContext != nullptr);

        this->hostDebugContext = hostDebugContext;
    }

    bool DebugContext::CanRegisterFunction() const
    {
        if (this->IsSelfOrScriptContextClosed() ||
            this->hostDebugContext == nullptr ||
            this->IsDebugContextInNonDebugMode())
        {
            return false;
        }

        return true;
    }

    void DebugContext::RegisterFunction(Js::ParseableFunctionInfo * func, LPCWSTR title)
    {
        if (!this->CanRegisterFunction())
        {
            return;
        }

        this->RegisterFunction(func, func->GetHostSourceContext(), title);
    }

    void DebugContext::RegisterFunction(Js::ParseableFunctionInfo * func, DWORD_PTR dwDebugSourceContext, LPCWSTR title)
    {
        if (!this->CanRegisterFunction())
        {
            return;
        }

        FunctionBody * functionBody = nullptr;
        if (func->IsDeferredParseFunction())
        {
            HRESULT hr = S_OK;
            Assert(!this->scriptContext->GetThreadContext()->IsScriptActive());

            BEGIN_JS_RUNTIME_CALL_EX_AND_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT_NESTED(this->scriptContext, false)
            {
                functionBody = func->Parse();
            }
            END_JS_RUNTIME_CALL_AND_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT(hr);

            if (FAILED(hr))
            {
                return;
            }
        }
        else
        {
            functionBody = func->GetFunctionBody();
        }
        this->RegisterFunction(functionBody, dwDebugSourceContext, title);
    }

    void DebugContext::RegisterFunction(Js::FunctionBody * functionBody, DWORD_PTR dwDebugSourceContext, LPCWSTR title)
    {
        if (!this->CanRegisterFunction())
        {
            return;
        }

        this->hostDebugContext->DbgRegisterFunction(this->scriptContext, functionBody, dwDebugSourceContext, title);
    }

    // Sets the specified mode for the debugger.  The mode is used to inform
    // the runtime of whether or not functions should be JITed or interpreted
    // when they are defer parsed.
    // Note: Transitions back to NotDebugging are not allowed.  Once the debugger
    // is in SourceRundown or Debugging mode, it can only transition between those
    // two modes.
    void DebugContext::SetDebuggerMode(DebuggerMode mode)
    {
        if (this->debuggerMode == mode)
        {
            // Already in this mode so return.
            return;
        }

        if (mode == DebuggerMode::NotDebugging)
        {
            AssertMsg(false, "Transitioning to non-debug mode is not allowed.");
            return;
        }

        this->debuggerMode = mode;
    }

    HRESULT DebugContext::RundownSourcesAndReparse(bool shouldPerformSourceRundown, bool shouldReparseFunctions)
    {
        struct AutoRestoreIsReparsingSource
        {
            AutoRestoreIsReparsingSource(DebugContext* debugContext, bool shouldReparseFunctions)
                : debugContext(debugContext)
                , shouldReparseFunctions(shouldReparseFunctions)
            {
                if (this->shouldReparseFunctions)
                {
                    this->debugContext->isReparsingSource = true;
                }
            }
            ~AutoRestoreIsReparsingSource()
            {
                if (this->shouldReparseFunctions)
                {
                    this->debugContext->isReparsingSource = false;
                }
            }

        private:
            DebugContext* debugContext;
            bool shouldReparseFunctions;
        } autoRestoreIsReparsingSource(this, shouldReparseFunctions);

        OUTPUT_TRACE(Js::DebuggerPhase, _u("DebugContext::RundownSourcesAndReparse scriptContext 0x%p, shouldPerformSourceRundown %d, shouldReparseFunctions %d\n"),
            this->scriptContext, shouldPerformSourceRundown, shouldReparseFunctions);

        Js::TempArenaAllocatorObject *tempAllocator = nullptr;
        JsUtil::List<Js::FunctionInfo *, Recycler>* pFunctionsToRegister = nullptr;
        JsUtil::List<Js::Utf8SourceInfo *, Recycler, false, Js::CopyRemovePolicy, RecyclerPointerComparer>* utf8SourceInfoList = nullptr;

        HRESULT hr = S_OK;
        ThreadContext* threadContext = this->scriptContext->GetThreadContext();

        BEGIN_TRANSLATE_OOM_TO_HRESULT_NESTED
        tempAllocator = threadContext->GetTemporaryAllocator(_u("debuggerAlloc"));

        utf8SourceInfoList = JsUtil::List<Js::Utf8SourceInfo *, Recycler, false, Js::CopyRemovePolicy, RecyclerPointerComparer>::New(this->scriptContext->GetRecycler());

        this->MapUTF8SourceInfoUntil([&](Js::Utf8SourceInfo * sourceInfo) -> bool
        {
            WalkAndAddUtf8SourceInfo(sourceInfo, utf8SourceInfoList);
            return false;
        });
        END_TRANSLATE_OOM_TO_HRESULT(hr);

        if (FAILED(hr))
        {
            Assert(hr == E_OUTOFMEMORY);
            return hr;
        }

        utf8SourceInfoList->MapUntil([&](int index, Js::Utf8SourceInfo * sourceInfo) -> bool
        {
            if (this->IsSelfOrScriptContextClosed())
            {
                hr = E_FAIL;
                return true;
            }

            OUTPUT_TRACE(Js::DebuggerPhase, _u("DebugContext::RundownSourcesAndReparse scriptContext 0x%p, sourceInfo 0x%p, HasDebugDocument %d\n"),
                this->scriptContext, sourceInfo, sourceInfo->HasDebugDocument());

            if (sourceInfo->GetIsLibraryCode())
            {
                // Not putting the internal library code to the debug mode, but need to reinitialize execution mode limits of each
                // function body upon debugger detach, even for library code at the moment.
                if (shouldReparseFunctions)
                {
                    sourceInfo->MapFunction([](Js::FunctionBody *const pFuncBody)
                    {
                        if (pFuncBody->IsFunctionParsed())
                        {
                            pFuncBody->ReinitializeExecutionModeAndLimits();
                        }
                    });
                }
                return false;
            }

            Assert(sourceInfo->GetSrcInfo() && sourceInfo->GetSrcInfo()->sourceContextInfo);

#if DBG
            if (shouldPerformSourceRundown)
            {
                // We shouldn't have a debug document if we're running source rundown for the first time.
                Assert(!sourceInfo->HasDebugDocument());
            }
#endif // DBG

            DWORD_PTR dwDebugHostSourceContext = Js::Constants::NoHostSourceContext;

            if (shouldPerformSourceRundown && this->hostDebugContext != nullptr)
            {
                dwDebugHostSourceContext = this->hostDebugContext->GetHostSourceContext(sourceInfo);
            }

            pFunctionsToRegister = sourceInfo->GetTopLevelFunctionInfoList();

            if (pFunctionsToRegister == nullptr || pFunctionsToRegister->Count() == 0)
            {
                // This could happen if there are no functions to re-compile.
                return false;
            }

            if (this->hostDebugContext != nullptr && sourceInfo->GetSourceContextInfo())
            {
                // This call goes out of engine
                this->hostDebugContext->SetThreadDescription(sourceInfo->GetSourceContextInfo()->url); // the HRESULT is omitted.
            }

            bool fHasDoneSourceRundown = false;
            for (int i = 0; i < pFunctionsToRegister->Count(); i++)
            {
                if (this->IsSelfOrScriptContextClosed())
                {
                    hr = E_FAIL;
                    return true;
                }

                Js::FunctionInfo *functionInfo = pFunctionsToRegister->Item(i);
                if (functionInfo == nullptr)
                {
                    continue;
                }

                if (shouldReparseFunctions)
                {
                    BEGIN_JS_RUNTIME_CALL_EX_AND_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT_NESTED(this->scriptContext, false)
                    {
                        functionInfo->GetParseableFunctionInfo()->Parse();
                        // This is the first call to the function, ensure dynamic profile info
#if ENABLE_PROFILE_INFO
                        functionInfo->GetFunctionBody()->EnsureDynamicProfileInfo();
#endif
                    }
                    END_JS_RUNTIME_CALL_AND_TRANSLATE_EXCEPTION_AND_ERROROBJECT_TO_HRESULT(hr);

                    // Debugger attach/detach failure is catastrophic, take down the process
                    DEBUGGER_ATTACHDETACH_FATAL_ERROR_IF_FAILED(hr);
                }

                // Parsing the function may change its FunctionProxy.
                Js::ParseableFunctionInfo *parseableFunctionInfo = functionInfo->GetParseableFunctionInfo();

                if (!fHasDoneSourceRundown && shouldPerformSourceRundown && !this->IsSelfOrScriptContextClosed())
                {
                    BEGIN_TRANSLATE_OOM_TO_HRESULT_NESTED
                    {
                        this->RegisterFunction(parseableFunctionInfo, dwDebugHostSourceContext, parseableFunctionInfo->GetSourceName());
                    }
                    END_TRANSLATE_OOM_TO_HRESULT(hr);

                    DEBUGGER_ATTACHDETACH_FATAL_ERROR_IF_FAILED(hr);

                    fHasDoneSourceRundown = true;
                }
            }

            if (shouldReparseFunctions)
            {
                sourceInfo->MapFunction([](Js::FunctionBody *const pFuncBody)
                {
                    if (pFuncBody->IsFunctionParsed())
                    {
                        pFuncBody->ReinitializeExecutionModeAndLimits();
                    }
                });
            }

            return false;
        });

        if (!this->IsSelfOrScriptContextClosed())
        {
            if (shouldPerformSourceRundown && this->scriptContext->HaveCalleeSources() && this->hostDebugContext != nullptr)
            {
                this->scriptContext->MapCalleeSources([=](Js::Utf8SourceInfo* calleeSourceInfo)
                {
                    if (!this->IsSelfOrScriptContextClosed())
                    {
                        // This call goes out of engine
                        this->hostDebugContext->ReParentToCaller(calleeSourceInfo);
                    }
                });
            }
        }
        else
        {
            hr = E_FAIL;
        }

        threadContext->ReleaseTemporaryAllocator(tempAllocator);

        return hr;
    }

    // Create an ordered flat list of sources to reparse. Caller of a source should be added to the list before we add the source itself.
    void DebugContext::WalkAndAddUtf8SourceInfo(Js::Utf8SourceInfo* sourceInfo, JsUtil::List<Js::Utf8SourceInfo *, Recycler, false, Js::CopyRemovePolicy, RecyclerPointerComparer> *utf8SourceInfoList)
    {
        Js::Utf8SourceInfo* callerUtf8SourceInfo = sourceInfo->GetCallerUtf8SourceInfo();
        if (callerUtf8SourceInfo)
        {
            Js::ScriptContext* callerScriptContext = callerUtf8SourceInfo->GetScriptContext();
            OUTPUT_TRACE(Js::DebuggerPhase, _u("DebugContext::WalkAndAddUtf8SourceInfo scriptContext 0x%p, sourceInfo 0x%p, callerUtf8SourceInfo 0x%p, sourceInfo scriptContext 0x%p, callerUtf8SourceInfo scriptContext 0x%p\n"),
                this->scriptContext, sourceInfo, callerUtf8SourceInfo, sourceInfo->GetScriptContext(), callerScriptContext);

            if (sourceInfo->GetScriptContext() == callerScriptContext)
            {
                WalkAndAddUtf8SourceInfo(callerUtf8SourceInfo, utf8SourceInfoList);
            }
            else if (callerScriptContext->IsScriptContextInNonDebugMode())
            {
                // The caller scriptContext is not in run down/debug mode so let's save the relationship so that we can re-parent callees afterwards.
                callerScriptContext->AddCalleeSourceInfoToList(sourceInfo);
            }
        }
        if (!utf8SourceInfoList->Contains(sourceInfo))
        {
            OUTPUT_TRACE(Js::DebuggerPhase, _u("DebugContext::WalkAndAddUtf8SourceInfo Adding to utf8SourceInfoList scriptContext 0x%p, sourceInfo 0x%p, sourceInfo scriptContext 0x%p\n"),
                this->scriptContext, sourceInfo, sourceInfo->GetScriptContext());
#if DBG
            bool found = false;
            this->MapUTF8SourceInfoUntil([&](Js::Utf8SourceInfo * sourceInfoTemp) -> bool
            {
                if (sourceInfoTemp == sourceInfo)
                {
                    found = true;
                }
                return found;
            });
            AssertMsg(found, "Parented eval feature have extra source");
#endif
            utf8SourceInfoList->Add(sourceInfo);
        }
    }

    template<class TMapFunction>
    void DebugContext::MapUTF8SourceInfoUntil(TMapFunction map)
    {
        this->scriptContext->MapScript([=](Js::Utf8SourceInfo* sourceInfo) -> bool {
            return map(sourceInfo);
        });
    }
}
#endif