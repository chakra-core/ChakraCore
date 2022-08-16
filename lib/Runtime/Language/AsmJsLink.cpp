//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#ifdef ASMJS_PLAT
#include "Library/Functions/BoundFunction.h"
namespace Js{
    bool ASMLink::CheckArrayBuffer(ScriptContext* scriptContext, Var bufferView, const AsmJsModuleInfo * info)
    {
        if (!bufferView)
        {
            return true;
        }

        if (!VarIs<ArrayBuffer>(bufferView))
        {
            AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : Buffer parameter is not an Array buffer"));
            return false;
        }
        JavascriptArrayBuffer* buffer = (JavascriptArrayBuffer*)bufferView;
        if (buffer->GetByteLength() <= info->GetMaxHeapAccess())
        {
            AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : Buffer bytelength is smaller than constant accesses"));
            return false;
        }
        if (!buffer->IsValidAsmJsBufferLength(buffer->GetByteLength(), true))
        {
            AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : Buffer bytelength is not a valid size for asm.js"));
            return false;
        }

        return true;
    }

    bool ASMLink::CheckFFI(ScriptContext* scriptContext, AsmJsModuleInfo* info, const Var foreign)
    {
        if (info->GetFunctionImportCount() == 0 && info->GetVarImportCount() == 0)
        {
            return true;
        }
        Assert(foreign);
        if (!VarIs<RecyclableObject>(foreign))
        {
            AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : FFI is not an object"));
            return false;
        }
        TypeId foreignObjType = VarTo<RecyclableObject>(foreign)->GetTypeId();
        if (StaticType::Is(foreignObjType) || TypeIds_Proxy == foreignObjType)
        {
            AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : FFI is not an object"));
            return false;
        }
        return true;
    }

    bool ASMLink::CheckStdLib(ScriptContext* scriptContext, const AsmJsModuleInfo* info, const Var stdlib)
    {
        // We should already prevent implicit calls, but just to be safe in case someone changes that
        JS_REENTRANCY_LOCK(lock, scriptContext->GetThreadContext());

        BVStatic<ASMMATH_BUILTIN_SIZE> mathBuiltinUsed = info->GetAsmMathBuiltinUsed();
        BVStatic<ASMARRAY_BUILTIN_SIZE> arrayBuiltinUsed = info->GetAsmArrayBuiltinUsed();

        if (mathBuiltinUsed.IsAllClear() && arrayBuiltinUsed.IsAllClear())
        {
            return true;
        }
        Assert(stdlib);
        if (!VarIs<RecyclableObject>(stdlib))
        {
            AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : StdLib is not an object"));
            return false;
        }
        TypeId stdLibObjType = VarTo<RecyclableObject>(stdlib)->GetTypeId();
        if (StaticType::Is(stdLibObjType) || TypeIds_Proxy == stdLibObjType)
        {
            AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : StdLib is not an object"));
            return false;
        }

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        if (mathBuiltinUsed.TestAndClear(AsmJSMathBuiltinFunction::AsmJSMathBuiltin_infinity))
        {
            Var asmInfinityObj = JavascriptOperators::OP_GetProperty(stdlib, PropertyIds::Infinity, scriptContext);
            if (!JavascriptConversion::SameValue(asmInfinityObj, library->GetPositiveInfinite()))
            {
                AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : Math constant Infinity is invalid"));
                return false;
            }
        }
        if (mathBuiltinUsed.TestAndClear(AsmJSMathBuiltinFunction::AsmJSMathBuiltin_nan))
        {
            Var asmNaNObj = JavascriptOperators::OP_GetProperty(stdlib, PropertyIds::NaN, scriptContext);
            if (!JavascriptConversion::SameValue(asmNaNObj, library->GetNaN()))
            {
                AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : Math constant NaN is invalid"));
                return false;
            }
        }
        if (!mathBuiltinUsed.IsAllClear())
        {
            Var asmMathObject = JavascriptOperators::OP_GetProperty(stdlib, PropertyIds::Math, scriptContext);
            for (int i = 0; i < AsmJSMathBuiltinFunction::AsmJSMathBuiltin_COUNT; i++)
            {
                //check if bit is set
                if (!mathBuiltinUsed.Test(i))
                {
                    continue;
                }
                AsmJSMathBuiltinFunction mathBuiltinFunc = (AsmJSMathBuiltinFunction)i;
                if (!CheckMathLibraryMethod(scriptContext, asmMathObject, mathBuiltinFunc))
                {
                    AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : Math builtin function is invalid"));
                    return false;
                }
            }
        }
        for (int i = 0; i < AsmJSTypedArrayBuiltinFunction::AsmJSTypedArrayBuiltin_COUNT; i++)
        {
            //check if bit is set
            if (!arrayBuiltinUsed.Test(i))
            {
                continue;
            }
            AsmJSTypedArrayBuiltinFunction arrayBuiltinFunc = (AsmJSTypedArrayBuiltinFunction)i;
            if (!CheckArrayLibraryMethod(scriptContext, stdlib, arrayBuiltinFunc))
            {
                AsmJSCompiler::OutputError(scriptContext, _u("Asm.js Runtime Error : Array builtin function is invalid"));
                return false;
            }
        }

        return true;
    }

    bool ASMLink::CheckArrayLibraryMethod(ScriptContext* scriptContext, const Var stdlib, const AsmJSTypedArrayBuiltinFunction arrayLibMethod)
    {
        switch (arrayLibMethod)
        {
#define ASMJS_TYPED_ARRAY_NAMES(name, propertyName) case AsmJSTypedArrayBuiltinFunction::AsmJSTypedArrayBuiltin_##name: \
            return CheckIsBuiltinFunction(scriptContext, stdlib, PropertyIds::##propertyName, propertyName##::EntryInfo::NewInstance);
#include "AsmJsBuiltInNames.h"
        default:
            Assume(UNREACHED);
        }
        return false;
    }

    bool ASMLink::CheckMathLibraryMethod(ScriptContext* scriptContext, const Var asmMathObject, const AsmJSMathBuiltinFunction mathLibMethod)
    {
        switch (mathLibMethod)
        {
#define ASMJS_MATH_FUNC_NAMES(name, propertyName, funcInfo) case AsmJSMathBuiltinFunction::AsmJSMathBuiltin_##name: \
            return CheckIsBuiltinFunction(scriptContext, asmMathObject, PropertyIds::##propertyName, funcInfo);
#include "AsmJsBuiltInNames.h"
#define ASMJS_MATH_DOUBLE_CONST_NAMES(name, propertyName, value) case AsmJSMathBuiltinFunction::AsmJSMathBuiltin_##name: \
            return CheckIsBuiltinValue(scriptContext, asmMathObject, PropertyIds::##propertyName, value);
#include "AsmJsBuiltInNames.h"
        default:
            Assume(UNREACHED);
        }
        return false;
    }

    bool ASMLink::CheckIsBuiltinFunction(ScriptContext* scriptContext, const Var object, PropertyId propertyId, const FunctionInfo& funcInfo)
    {
        Var mathFuncObj = JavascriptOperators::OP_GetProperty(object, propertyId, scriptContext);
#ifdef ENABLE_JS_BUILTINS
        if (scriptContext->IsJsBuiltInEnabled())
        {
            switch (propertyId)
            {
#define ASMJS_JSBUILTIN_MATH_FUNC_NAMES(propertyId, funcName) case propertyId: \
                return VarIs<JavascriptFunction>(mathFuncObj) && \
                    VarTo<JavascriptFunction>(mathFuncObj) == scriptContext->GetLibrary()->GetMath##funcName##Function();
#include "AsmJsBuiltInNames.h"
            }
        }
#endif
        return VarIs<JavascriptFunction>(mathFuncObj) &&
            VarTo<JavascriptFunction>(mathFuncObj)->GetFunctionInfo()->GetOriginalEntryPoint() == funcInfo.GetOriginalEntryPoint();
    }

    bool ASMLink::CheckIsBuiltinValue(ScriptContext* scriptContext, const Var object, PropertyId propertyId, double value)
    {
        Var mathValue = JavascriptOperators::OP_GetProperty(object, propertyId, scriptContext);
        return JavascriptNumber::Is(mathValue) &&
            JavascriptNumber::GetValue(mathValue) == value;
    }

    bool ASMLink::CheckParams(ScriptContext* scriptContext, AsmJsModuleInfo* info, const Var stdlib, const Var foreign, const Var bufferView)
    {
        if (CheckStdLib(scriptContext, info, stdlib) && CheckArrayBuffer(scriptContext, bufferView, info) && CheckFFI(scriptContext, info, stdlib))
        {
            return true;
        }
        Output::Flush();
        return false;
    }
}
#endif
