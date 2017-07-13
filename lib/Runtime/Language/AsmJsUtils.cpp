//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// Portions of this file are copyright 2014 Mozilla Foundation, available under the Apache 2.0 license.
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// Copyright 2014 Mozilla Foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#ifdef ASMJS_PLAT
namespace Js
{

    bool ParserWrapper::ParseVarOrConstStatement( AsmJSParser &parser, ParseNode **var )
    {
        Assert( parser );
        *var = nullptr;
        ParseNode *body = parser->sxFnc.pnodeBody;
        if( body )
        {
            ParseNode* lhs = GetBinaryLeft( body );
            ParseNode* rhs = GetBinaryRight( body );
            if( rhs && rhs->nop == knopList )
            {
                AssertMsg( lhs->nop == knopStr, "this should be use asm" );
                *var = rhs;
                return true;
            }
        }
        return false;
    }

    bool ParserWrapper::IsDefinition( ParseNode *arg )
    {
        //TODO, eliminate duplicates
        return true;
    }



    ParseNode* ParserWrapper::NextInList( ParseNode *node )
    {
        Assert( node->nop == knopList );
        return node->sxBin.pnode2;
    }

    ParseNode* ParserWrapper::NextVar( ParseNode *node )
    {
        return node->sxVar.pnodeNext;
    }

    ParseNode* ParserWrapper::FunctionArgsList( ParseNode *node, ArgSlot &numformals )
    {
        Assert( node->nop == knopFncDecl );
        PnFnc func = node->sxFnc;
        ParseNode* first = func.pnodeParams;
        // throws OOM on uint16 overflow
        for( ParseNode* pnode = first; pnode; pnode = pnode->sxVar.pnodeNext, ArgSlotMath::Inc(numformals));
        return first;
    }

    PropertyName ParserWrapper::VariableName( ParseNode *node )
    {
        return node->name();
    }

    PropertyName ParserWrapper::FunctionName( ParseNode *node )
    {
        if( node->nop == knopFncDecl )
        {
            PnFnc function = node->sxFnc;
            if( function.pnodeName && function.pnodeName->nop == knopVarDecl )
            {
                return function.pnodeName->sxVar.pid;
            }
        }
        return nullptr;
    }

    ParseNode * ParserWrapper::GetVarDeclList( ParseNode * pnode )
    {
        ParseNode* varNode = pnode;
        while (varNode->nop == knopList)
        {
            ParseNode * var = GetBinaryLeft(varNode);
            if (var->nop == knopVarDecl)
            {
                return var;
            }
            else if (var->nop == knopList)
            {
                var = GetBinaryLeft(var);
                if (var->nop == knopVarDecl)
                {
                    return var;
                }
            }
            varNode = GetBinaryRight(varNode);
        }
        return nullptr;
    }

    void ParserWrapper::ReachEndVarDeclList( ParseNode** outNode )
    {
        ParseNode* pnode = *outNode;
        // moving down to the last var declaration
        while( pnode->nop == knopList )
        {
            ParseNode* var = GetBinaryLeft( pnode );
            if (var->nop == knopVarDecl)
            {
                pnode = GetBinaryRight( pnode );
                continue;
            }
            else if (var->nop == knopList)
            {
                var = GetBinaryLeft( var );
                if (var->nop == knopVarDecl)
                {
                    pnode = GetBinaryRight( pnode );
                    continue;
                }
            }
            break;
        }
        *outNode = pnode;
    }

    AsmJsCompilationException::AsmJsCompilationException( const char16* _msg, ... )
    {
        va_list arglist;
        va_start( arglist, _msg );
        vswprintf_s( msg_, _msg, arglist );
    }

#if ENABLE_DEBUG_CONFIG_OPTIONS
    int64 ConvertStringToInt64(Var string, ScriptContext* scriptContext)
    {
        JavascriptString* str = JavascriptString::FromVar(string);
        charcount_t length = str->GetLength();
        const char16* buf = str->GetString();
        int radix = 10;
        if (length >= 2 && buf[0] == '0' && buf[1] == 'x')
        {
            radix = 16;
        }
        return (int64)_wcstoui64(buf, nullptr, radix);
    }

    Var CreateI64ReturnObject(int64 val, ScriptContext* scriptContext)
    {
        Js::Var i64Object = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
        Var low = JavascriptNumber::ToVar((uint)val, scriptContext);
        Var high = JavascriptNumber::ToVar(val >> 32, scriptContext);

        JavascriptOperators::OP_SetProperty(i64Object, PropertyIds::low, low, scriptContext);
        JavascriptOperators::OP_SetProperty(i64Object, PropertyIds::high, high, scriptContext);
        return i64Object;
    }
#endif

    void * UnboxAsmJsArguments(ScriptFunction* func, Var * origArgs, char * argDst, CallInfo callInfo)
    {
        void * address = reinterpret_cast<void*>(func->GetEntryPointInfo()->jsMethod);
        Assert(address);
        AsmJsFunctionInfo* info = func->GetFunctionBody()->GetAsmJsFunctionInfo();
        ScriptContext* scriptContext = func->GetScriptContext();

#if ENABLE_DEBUG_CONFIG_OPTIONS
        bool allowTestInputs = CONFIG_FLAG(WasmI64);
#endif

        AsmJsModuleInfo::EnsureHeapAttached(func);

        ArgumentReader reader(&callInfo, origArgs);
        uint actualArgCount = reader.Info.Count - 1; // -1 for ScriptFunction
        argDst = argDst + MachPtr; // add one first so as to skip the ScriptFunction argument
        for (ArgSlot i = 0; i < info->GetArgCount(); i++)
        {

            if (info->GetArgType(i).isInt())
            {
                int32 intVal;
                if (i < actualArgCount)
                {
#if ENABLE_DEBUG_CONFIG_OPTIONS
                    if (allowTestInputs && JavascriptString::Is(*origArgs))
                    {
                        intVal = (int32)ConvertStringToInt64(*origArgs, scriptContext);
                    }
                    else
#endif
                        intVal = JavascriptMath::ToInt32(*origArgs, scriptContext);
                }
                else
                {
                    intVal = 0;
                }

#if TARGET_64
                *(int64*)(argDst) = 0;
#endif
                *(int32*)argDst = intVal;
                argDst = argDst + MachPtr;
            }
            else if (info->GetArgType(i).isInt64())
            {
#if ENABLE_DEBUG_CONFIG_OPTIONS
                if (!allowTestInputs)
#endif
                {
                    JavascriptError::ThrowTypeError(scriptContext, WASMERR_InvalidTypeConversion);
                }

#if ENABLE_DEBUG_CONFIG_OPTIONS
                int64 val;
                if (i < actualArgCount)
                {
                    if (JavascriptString::Is(*origArgs))
                    {
                        val = ConvertStringToInt64(*origArgs, scriptContext);
                    }
                    else if (JavascriptObject::Is(*origArgs))
                    {
                        RecyclableObject* object = RecyclableObject::FromVar(*origArgs);
                        PropertyRecord const * lowPropRecord = nullptr;
                        PropertyRecord const * highPropRecord = nullptr;
                        scriptContext->GetOrAddPropertyRecord(_u("low"), (int)wcslen(_u("low")), &lowPropRecord);
                        scriptContext->GetOrAddPropertyRecord(_u("high"), (int)wcslen(_u("high")), &highPropRecord);
                        Var low = JavascriptOperators::OP_GetProperty(object, lowPropRecord->GetPropertyId(), scriptContext);
                        Var high = JavascriptOperators::OP_GetProperty(object, highPropRecord->GetPropertyId(), scriptContext);

                        uint64 lowVal = JavascriptMath::ToInt32(low, scriptContext);
                        uint64 highVal = JavascriptMath::ToInt32(high, scriptContext);
                        val = (highVal << 32) | (lowVal & 0xFFFFFFFF);
                    }
                    else
                    {
                        int32 intVal = JavascriptMath::ToInt32(*origArgs, scriptContext);
                        val = (int64)intVal;
                    }
                }
                else
                {
                    val = 0;
                }

                *(int64*)(argDst) = val;
                argDst += sizeof(int64);
#endif
            }
            else if (info->GetArgType(i).isFloat())
            {
                float floatVal;
                if (i < actualArgCount)
                {
#if ENABLE_DEBUG_CONFIG_OPTIONS
                    if (allowTestInputs && JavascriptString::Is(*origArgs))
                    {
                        int32 val = (int32)ConvertStringToInt64(*origArgs, scriptContext);
                        floatVal = *(float*)&val;
                    }
                    else
#endif
                        floatVal = (float)(JavascriptConversion::ToNumber(*origArgs, scriptContext));
                }
                else
                {
                    floatVal = (float)(JavascriptNumber::NaN);
                }
#if TARGET_64
                *(int64*)(argDst) = 0;
#endif
                *(float*)argDst = floatVal;
                argDst = argDst + MachPtr;
            }
            else if (info->GetArgType(i).isDouble())
            {
                double doubleVal;
                if (i < actualArgCount)
                {
#if ENABLE_DEBUG_CONFIG_OPTIONS
                    if (allowTestInputs && JavascriptString::Is(*origArgs))
                    {
                        int64 val = ConvertStringToInt64(*origArgs, scriptContext);
                        doubleVal = *(double*)&val;
                    }
                    else
#endif
                        doubleVal = JavascriptConversion::ToNumber(*origArgs, scriptContext);
                }
                else
                {
                    doubleVal = JavascriptNumber::NaN;
                }

                *(double*)argDst = doubleVal;
                argDst = argDst + sizeof(double);
            }
#ifdef ENABLE_SIMDJS
            else if (info->GetArgType(i).isSIMD())
            {
                AsmJsVarType argType = info->GetArgType(i);
                AsmJsSIMDValue simdVal = {0, 0, 0, 0};
                // SIMD values are copied unaligned.
                // SIMD values cannot be implicitly coerced from/to other types. If the SIMD parameter is missing (i.e. Undefined), we throw type error since there is not equivalent SIMD value to coerce to.
                switch (argType.which())
                {
                case AsmJsType::Int32x4:
                    if (!JavascriptSIMDInt32x4::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt32x4TypeMismatch, _u("Int32x4"));
                    }
                    simdVal = ((JavascriptSIMDInt32x4*)(*origArgs))->GetValue();
                    break;
                case AsmJsType::Bool32x4:
                    if (!JavascriptSIMDBool32x4::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool32x4TypeMismatch, _u("Bool32x4"));
                    }
                    simdVal = ((JavascriptSIMDBool32x4*)(*origArgs))->GetValue();
                    break;
                case AsmJsType::Bool16x8:
                    if (!JavascriptSIMDBool16x8::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool16x8TypeMismatch, _u("Bool16x8"));
                    }
                    simdVal = ((JavascriptSIMDBool16x8*)(*origArgs))->GetValue();
                    break;
                case AsmJsType::Bool8x16:
                    if (!JavascriptSIMDBool8x16::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("Bool8x16"));
                    }
                    simdVal = ((JavascriptSIMDBool8x16*)(*origArgs))->GetValue();
                    break;
                case AsmJsType::Float32x4:
                    if (!JavascriptSIMDFloat32x4::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdFloat32x4TypeMismatch, _u("Float32x4"));
                    }
                    simdVal = ((JavascriptSIMDFloat32x4*)(*origArgs))->GetValue();
                    break;
                case AsmJsType::Float64x2:
                    if (!JavascriptSIMDFloat64x2::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdFloat64x2TypeMismatch, _u("Float64x2"));
                    }
                    simdVal = ((JavascriptSIMDFloat64x2*)(*origArgs))->GetValue();
                    break;
                case AsmJsType::Int16x8:
                    if (!JavascriptSIMDInt16x8::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("Int16x8"));
                    }
                    simdVal = ((JavascriptSIMDInt16x8*)(*origArgs))->GetValue();
                    break;
                case AsmJsType::Int8x16:
                    if (!JavascriptSIMDInt8x16::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt8x16TypeMismatch, _u("Int8x16"));
                    }
                    simdVal = ((JavascriptSIMDInt8x16*)(*origArgs))->GetValue();
                    break;
                case AsmJsType::Uint32x4:
                    if (!JavascriptSIMDUint32x4::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("Uint32x4"));
                    }
                    simdVal = ((JavascriptSIMDUint32x4*)(*origArgs))->GetValue();
                    break;
                case AsmJsType::Uint16x8:
                    if (!JavascriptSIMDUint16x8::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("Uint16x8"));
                    }
                    simdVal = ((JavascriptSIMDUint16x8*)(*origArgs))->GetValue();
                    break;
                case AsmJsType::Uint8x16:
                    if (!JavascriptSIMDUint8x16::Is(*origArgs))
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("Uint8x16"));
                    }
                    simdVal = ((JavascriptSIMDUint8x16*)(*origArgs))->GetValue();
                    break;
                default:
                    Assert(UNREACHED);
                }
                *(AsmJsSIMDValue*)argDst = simdVal;
                argDst = argDst + sizeof(AsmJsSIMDValue);
            }
#endif // #ifdef ENABLE_SIMDJS
            else
            {
                Assert(UNREACHED);
            }
            ++origArgs;
        }
        // for convenience, lets take the opportunity to return the asm.js entrypoint address
        return address;
    }

#if _M_X64

    // returns an array containing the size of each argument
    uint *GetArgsSizesArray(ScriptFunction* func)
    {
        AsmJsFunctionInfo* info = func->GetFunctionBody()->GetAsmJsFunctionInfo();
        return info->GetArgsSizesArray();
    }

    int GetStackSizeForAsmJsUnboxing(ScriptFunction* func)
    {
        AsmJsFunctionInfo* info = func->GetFunctionBody()->GetAsmJsFunctionInfo();
        int argSize = info->GetArgByteSize() + MachPtr;
        argSize = ::Math::Align<int32>(argSize, 16);

        if (argSize < 32)
        {
            argSize = 32; // convention is to always allocate spill space for rcx,rdx,r8,r9
        }

        PROBE_STACK_CALL(func->GetScriptContext(), func, argSize + Js::Constants::MinStackDefault);
        return argSize;
    }

    Var BoxAsmJsReturnValue(ScriptFunction* func, int64 intRetVal, double doubleRetVal, float floatRetVal, __m128 simdRetVal)
    {
        // ExternalEntryPoint doesn't know the return value, so it will send garbage for everything except actual return type
        Var returnValue = nullptr;
        // make call and convert primitive type back to Var
        AsmJsFunctionInfo* info = func->GetFunctionBody()->GetAsmJsFunctionInfo();
        ScriptContext* scriptContext = func->GetScriptContext();
        switch (info->GetReturnType().which())
        {
        case AsmJsRetType::Void:
            returnValue = JavascriptOperators::OP_LdUndef(scriptContext);
            break;
        case AsmJsRetType::Signed:
        {
            returnValue = JavascriptNumber::ToVar((int)intRetVal, scriptContext);
            break;
        }
        case AsmJsRetType::Int64:
        {
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (CONFIG_FLAG(WasmI64))
            {
                returnValue = CreateI64ReturnObject(intRetVal, scriptContext);
                break;
            }
#endif
            JavascriptError::ThrowTypeError(scriptContext, WASMERR_InvalidTypeConversion);
        }
        case AsmJsRetType::Double:
        {
            returnValue = JavascriptNumber::NewWithCheck(doubleRetVal, scriptContext);
            break;
        }
        case AsmJsRetType::Float:
        {
            returnValue = JavascriptNumber::NewWithCheck(floatRetVal, scriptContext);
            break;
        }
#ifdef ENABLE_SIMDJS
        case AsmJsRetType::Float32x4:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDFloat32x4::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
        case AsmJsRetType::Int32x4:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDInt32x4::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
        case AsmJsRetType::Bool32x4:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDBool32x4::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
        case AsmJsRetType::Bool16x8:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDBool16x8::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
        case AsmJsRetType::Bool8x16:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDBool8x16::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
        case AsmJsRetType::Float64x2:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDFloat64x2::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
        case AsmJsRetType::Int16x8:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDInt16x8::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
        case AsmJsRetType::Int8x16:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDInt8x16::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
        case AsmJsRetType::Uint32x4:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDUint32x4::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
        case AsmJsRetType::Uint16x8:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDUint16x8::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
        case AsmJsRetType::Uint8x16:
        {
            X86SIMDValue simdVal;
            simdVal.m128_value = simdRetVal;
            returnValue = JavascriptSIMDUint8x16::New(&X86SIMDValue::ToSIMDValue(simdVal), scriptContext);
            break;
        }
#endif // #ifdef ENABLE_SIMDJS
        default:
            Assume(UNREACHED);
        }

        return returnValue;
    }

#elif _M_IX86
    Var AsmJsExternalEntryPoint(RecyclableObject* entryObject, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);
        ScriptFunction* func = (ScriptFunction*)entryObject;
        FunctionBody* body = func->GetFunctionBody();
        AsmJsFunctionInfo* info = body->GetAsmJsFunctionInfo();
        int argSize = info->GetArgByteSize();
        void* dst;
        Var returnValue = 0;

        // TODO (michhol): wasm, heap should not ever be detached
        AsmJsModuleInfo::EnsureHeapAttached(func);

        argSize = ::Math::Align<int32>(argSize, 8);
        // Allocate stack space for args
        PROBE_STACK_CALL(func->GetScriptContext(), func, argSize + Js::Constants::MinStackDefault);

        dst = _alloca(argSize);
        const void * asmJSEntryPoint = UnboxAsmJsArguments(func, args.Values + 1, ((char*)dst) - MachPtr, callInfo);

        // make call and convert primitive type back to Var
        switch (info->GetReturnType().which())
        {
        case AsmJsRetType::Void:
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                push func
                call ecx
            }
            returnValue = JavascriptOperators::OP_LdUndef(func->GetScriptContext());
            break;
        case AsmJsRetType::Signed:{
            int32 ival = 0;
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                push func
                call ecx
                mov ival, eax
            }
            returnValue = JavascriptNumber::ToVar(ival, func->GetScriptContext());
            break;
        }
        case AsmJsRetType::Int64:
        {
            int32 iLow = 0, iHigh = 0;
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                push func
                call ecx
                mov iLow, eax;
                mov iHigh, edx;
            }
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (CONFIG_FLAG(WasmI64))
            {
                returnValue = CreateI64ReturnObject((int64)iLow | ((int64)iHigh << 32), func->GetScriptContext());
                break;
            }
#endif
            JavascriptError::ThrowTypeError(func->GetScriptContext(), WASMERR_InvalidTypeConversion);
        }
        case AsmJsRetType::Double:{
            double dval = 0;
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                push func
                call ecx
                movsd dval, xmm0
            }
            returnValue = JavascriptNumber::NewWithCheck(dval, func->GetScriptContext());
            break;
        }
        case AsmJsRetType::Float:{
            float fval = 0;
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                push func
                call ecx
                movss fval, xmm0
            }
            returnValue = JavascriptNumber::NewWithCheck((double)fval, func->GetScriptContext());
            break;
        }
#ifdef ENABLE_SIMDJS
        case AsmJsRetType::Int32x4:
            AsmJsSIMDValue simdVal;
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                push func
                call ecx
                movups simdVal, xmm0
            }
            returnValue = JavascriptSIMDInt32x4::New(&simdVal, func->GetScriptContext());
            break;
        case AsmJsRetType::Bool32x4:
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                    push func
                    call ecx
                    movups simdVal, xmm0
            }
            returnValue = JavascriptSIMDBool32x4::New(&simdVal, func->GetScriptContext());
            break;
        case AsmJsRetType::Bool16x8:
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                    push func
                    call ecx
                    movups simdVal, xmm0
            }
            returnValue = JavascriptSIMDBool16x8::New(&simdVal, func->GetScriptContext());
            break;
        case AsmJsRetType::Bool8x16:
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                    push func
                    call ecx
                    movups simdVal, xmm0
            }
            returnValue = JavascriptSIMDBool8x16::New(&simdVal, func->GetScriptContext());
            break;
        case AsmJsRetType::Float32x4:
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                push func
                call ecx
                movups simdVal, xmm0
            }
                returnValue = JavascriptSIMDFloat32x4::New(&simdVal, func->GetScriptContext());
                break;

        case AsmJsRetType::Float64x2:
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                push func
                call ecx
                movups simdVal, xmm0
            }
            returnValue = JavascriptSIMDFloat64x2::New(&simdVal, func->GetScriptContext());
            break;

        case AsmJsRetType::Int16x8:
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                    push func
                    call ecx
                    movups simdVal, xmm0
            }
            returnValue = JavascriptSIMDInt16x8::New(&simdVal, func->GetScriptContext());
            break;

        case AsmJsRetType::Int8x16:
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                    push func
                    call ecx
                    movups simdVal, xmm0
            }
            returnValue = JavascriptSIMDInt8x16::New(&simdVal, func->GetScriptContext());
            break;

        case AsmJsRetType::Uint32x4:
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                    push func
                    call ecx
                    movups simdVal, xmm0
            }
            returnValue = JavascriptSIMDUint32x4::New(&simdVal, func->GetScriptContext());
            break;

        case AsmJsRetType::Uint16x8:
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                    push func
                    call ecx
                    movups simdVal, xmm0
            }
            returnValue = JavascriptSIMDUint16x8::New(&simdVal, func->GetScriptContext());
            break;

        case AsmJsRetType::Uint8x16:
            simdVal.Zero();
            __asm
            {
                mov  ecx, asmJSEntryPoint
#ifdef _CONTROL_FLOW_GUARD
                call[__guard_check_icall_fptr]
#endif
                    push func
                    call ecx
                    movups simdVal, xmm0
            }
            returnValue = JavascriptSIMDUint8x16::New(&simdVal, func->GetScriptContext());
            break;
#endif // #ifdef ENABLE_SIMDJS
        default:
            Assume(UNREACHED);
        }
        return returnValue;
    }
#endif

}
#endif
