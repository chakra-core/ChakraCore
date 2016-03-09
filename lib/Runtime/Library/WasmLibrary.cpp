//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM
namespace Js
{
    Var WasmLibrary::instantiateModule(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count < 2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray, L"[Wasm].instantiateModule(typedArray,)");
        }
        if (args.Info.Count < 3)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, L"[Wasm].instantiateModule(,ffi)");
        }

        if (!Js::TypedArrayBase::Is(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray, L"[Wasm].instantiateModule(typedArray,)");
        }
        Js::TypedArrayBase* array = Js::TypedArrayBase::FromVar(args[1]);
        BYTE* buffer = array->GetByteBuffer();
        uint byteLength = array->GetByteLength();

        if (!Js::JavascriptObject::Is(args[2]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, L"[Wasm].instantiateModule(,ffi)");
        }
        Js::Var ffi = args[2];

        CompileScriptException se;
        Js::Var exportObject;
        Js::Utf8SourceInfo* utf8SourceInfo;
        BEGIN_LEAVE_SCRIPT_INTERNAL(scriptContext)
            exportObject = scriptContext->LoadWasmScript(
                (const wchar_t*)buffer,
                nullptr, // source info
                &se,
                false, // isExpression
                false, // disableDeferedParse
                false, // isForNativeCode
                &utf8SourceInfo,
                true, // isBinary
                byteLength,
                Js::Constants::GlobalCode,
                ffi
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
        return exportObject;
    }
}
#endif