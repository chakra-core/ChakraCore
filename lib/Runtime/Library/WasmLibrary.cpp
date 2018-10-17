//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM
#include "../WasmReader/WasmReaderPch.h"
// Included for AsmJsDefaultEntryThunk
#include "Language/InterpreterStackFrame.h"

namespace Js
{
    Var WasmLibrary::WasmLazyTrapCallback(RecyclableObject *callee, CallInfo, ...)
    {
        WasmScriptFunction* asmFunction = static_cast<WasmScriptFunction*>(callee);
        Assert(asmFunction);
        ScriptContext * scriptContext = asmFunction->GetScriptContext();
        Assert(scriptContext);
        auto error = asmFunction->GetFunctionBody()->GetAsmJsFunctionInfo()->GetLazyError();
        JavascriptExceptionOperators::Throw(error, scriptContext);
    }

    void WasmLibrary::ResetFunctionBodyDefaultEntryPoint(FunctionBody* body)
    {
        body->GetDefaultFunctionEntryPointInfo()->SetIsAsmJSFunction(true);
        body->GetDefaultFunctionEntryPointInfo()->jsMethod = AsmJsDefaultEntryThunk;
        body->SetOriginalEntryPoint(AsmJsDefaultEntryThunk);
        // Reset jit status for this function
        body->SetIsAsmJsFullJitScheduled(false);
        Assert(body->HasValidEntryPoint());
    }
}

#endif // ENABLE_WASM

Js::JavascriptMethod Js::WasmLibrary::EnsureWasmEntrypoint(Js::ScriptFunction* func)
{
#ifdef ENABLE_WASM
    if (func->GetFunctionBody()->IsWasmFunction())
    {
        FunctionBody* body = func->GetFunctionBody();
        AsmJsFunctionInfo* info = body->GetAsmJsFunctionInfo();
        ScriptContext* scriptContext = func->GetScriptContext();

        Js::FunctionEntryPointInfo * entrypointInfo = (Js::FunctionEntryPointInfo*)func->GetEntryPointInfo();
        if (info->GetLazyError())
        {
            // We might have parsed this in the past and there was an error
            entrypointInfo->jsMethod = WasmLibrary::WasmLazyTrapCallback;
        }
        else if (body->GetByteCodeCount() == 0)
        {
            Wasm::WasmReaderInfo* readerInfo = info->GetWasmReaderInfo();
            AssertOrFailFast(readerInfo);
            try
            {
                Wasm::WasmBytecodeGenerator::GenerateFunctionBytecode(scriptContext, readerInfo);
                entrypointInfo->jsMethod = AsmJsDefaultEntryThunk;
                WAsmJs::JitFunctionIfReady(func);
            }
            catch (Wasm::WasmCompilationException& ex)
            {
                AutoFreeExceptionMessage autoCleanExceptionMessage;
                char16* exceptionMessage = WebAssemblyModule::FormatExceptionMessage(&ex, &autoCleanExceptionMessage, readerInfo->m_module, body);

                JavascriptLibrary *library = scriptContext->GetLibrary();
                JavascriptError *pError = library->CreateWebAssemblyCompileError();
                JavascriptError::SetErrorMessage(pError, WASMERR_WasmCompileError, exceptionMessage, scriptContext);

                entrypointInfo->jsMethod = WasmLibrary::WasmLazyTrapCallback;
                info->SetLazyError(pError);
            }
        }
        // The function has already been parsed, just fix up the entry point
        else if (body->GetIsAsmJsFullJitScheduled())
        {
            Js::FunctionEntryPointInfo* defaultEntryPoint = (Js::FunctionEntryPointInfo*)body->GetDefaultEntryPointInfo();
            func->ChangeEntryPoint(defaultEntryPoint, defaultEntryPoint->jsMethod);
        }
        else
        {
            entrypointInfo->jsMethod = AsmJsDefaultEntryThunk;
        }

        Assert(body->HasValidEntryPoint());
        Js::JavascriptMethod jsMethod = func->GetEntryPointInfo()->jsMethod;
        // We are already in AsmJsDefaultEntryThunk so return null so it just keeps going
        return jsMethod == AsmJsDefaultEntryThunk ? nullptr : jsMethod;
    }
#endif
    return nullptr;
}
