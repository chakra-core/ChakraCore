//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "../WasmReader/WasmReaderPch.h"

#ifdef ENABLE_WASM
namespace Js
{
    const unsigned int WasmLibrary::experimentalVersion = Wasm::Binary::experimentalVersion;

    Var WasmLibrary::instantiateModule(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray, _u("[Wasm].instantiateModule(typedArray,)"));
        }
        if (args.Info.Count < 3)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("[Wasm].instantiateModule(,ffi)"));
        }

        const BOOL isTypedArray = Js::TypedArrayBase::Is(args[1]);
        const BOOL isArrayBuffer = Js::ArrayBuffer::Is(args[1]);
        
        if (!isTypedArray && !isArrayBuffer)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray, _u("[Wasm].instantiateModule(typedArray,)"));
        }

        BYTE* buffer;
        uint byteLength;
        if (isTypedArray)
        {
            Js::TypedArrayBase* array = Js::TypedArrayBase::FromVar(args[1]);
            buffer = array->GetByteBuffer();
            byteLength = array->GetByteLength();
        }
        else
        {
            Js::ArrayBuffer* arrayBuffer = Js::ArrayBuffer::FromVar(args[1]);
            buffer = arrayBuffer->GetBuffer();
            byteLength = arrayBuffer->GetByteLength();
        }

        if (!Js::JavascriptObject::Is(args[2]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("[Wasm].instantiateModule(,ffi)"));
        }
        Js::Var ffi = args[2];

        CompileScriptException se;
        Js::Var exportObject;
        Js::Var start = nullptr;
        Js::Utf8SourceInfo* utf8SourceInfo;
        BEGIN_LEAVE_SCRIPT_INTERNAL(scriptContext)
            exportObject = scriptContext->LoadWasmScript(
                (const char16*)buffer,
                nullptr, // source info
                &se,
                false, // isExpression
                false, // disableDeferedParse
                false, // isForNativeCode
                &utf8SourceInfo,
                true, // isBinary
                byteLength,
                Js::Constants::GlobalCode,
                ffi,
                &start
            );
        END_LEAVE_SCRIPT_INTERNAL(scriptContext)

        HRESULT hr = se.ei.scode;
        if (FAILED(hr))
        {
            if (hr == E_OUTOFMEMORY || hr == VBSERR_OutOfMemory || hr == VBSERR_OutOfStack || hr == ERRnoMemory)
            {
                Js::Throw::OutOfMemory();
            }
            JavascriptError::ThrowParserError(scriptContext, hr, &se);
        }

        if (exportObject && start)
        {
            Js::ScriptFunction* f = Js::AsmJsScriptFunction::FromVar(start);
            Js::CallInfo info(Js::CallFlags_New, 1);
            Js::Arguments startArg(info, &start);
            Js::JavascriptFunction::CallFunction<true>(f, f->GetEntryPoint(), startArg);
        }
        return exportObject;
    }
}
#endif