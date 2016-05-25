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

        if (!Js::TypedArrayBase::Is(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedTypedArray, _u("[Wasm].instantiateModule(typedArray,)"));
        }
        Js::TypedArrayBase* array = Js::TypedArrayBase::FromVar(args[1]);
        BYTE* buffer = array->GetByteBuffer();
        uint byteLength = array->GetByteLength();

        if (!Js::JavascriptObject::Is(args[2]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("[Wasm].instantiateModule(,ffi)"));
        }
        Js::Var ffi = args[2];

        CompileScriptException se;
        Js::Var exportObject;
        Js::Var start;
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

        if (start)
        {
            Js::ScriptFunction* f = Js::AsmJsScriptFunction::FromVar(start);
            Js::CallInfo info(Js::CallFlags_New, 1);
            Js::Arguments startArg(info, &start);
            Js::JavascriptFunction::CallFunction<true>(f, f->GetEntryPoint(), startArg);
        }

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