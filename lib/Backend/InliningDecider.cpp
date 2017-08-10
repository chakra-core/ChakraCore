//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

InliningDecider::InliningDecider(Js::FunctionBody *const topFunc, bool isLoopBody, bool isInDebugMode, const ExecutionMode jitMode)
    : topFunc(topFunc), isLoopBody(isLoopBody), isInDebugMode(isInDebugMode), jitMode(jitMode), bytecodeInlinedCount(0), numberOfInlineesWithLoop (0), threshold(topFunc->GetByteCodeWithoutLDACount(), isLoopBody)
{
    Assert(topFunc);
}

InliningDecider::~InliningDecider()
{
    INLINE_FLUSH();
}

bool InliningDecider::InlineIntoTopFunc() const
{
    if (this->jitMode == ExecutionMode::SimpleJit ||
        PHASE_OFF(Js::InlinePhase, this->topFunc) ||
        PHASE_OFF(Js::GlobOptPhase, this->topFunc))
    {
        return false;
    }

    if (this->topFunc->GetHasTry())
    {
#if defined(DBG_DUMP) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif
        INLINE_TESTTRACE(_u("INLINING: Skip Inline: Has try\tCaller: %s (%s)\n"), this->topFunc->GetDisplayName(),
            this->topFunc->GetDebugNumberSet(debugStringBuffer));
        // Glob opt doesn't run on function with try, so we can't generate bailout for it
        return false;
    }

    return InlineIntoInliner(topFunc);
}

bool InliningDecider::InlineIntoInliner(Js::FunctionBody *const inliner) const
{
    Assert(inliner);
    Assert(this->jitMode == ExecutionMode::FullJit);

#if defined(DBG_DUMP) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif

    if (PHASE_OFF(Js::InlinePhase, inliner) ||
        PHASE_OFF(Js::GlobOptPhase, inliner))
    {
        return false;
    }

    if (!inliner->HasDynamicProfileInfo())
    {
        INLINE_TESTTRACE(_u("INLINING: Skip Inline: No dynamic profile info\tCaller: %s (%s)\n"), inliner->GetDisplayName(),
            inliner->GetDebugNumberSet(debugStringBuffer));
        return false;
    }

    if (inliner->GetProfiledCallSiteCount() == 0 && !inliner->GetAnyDynamicProfileInfo()->HasLdFldCallSiteInfo())
    {
        INLINE_TESTTRACE_VERBOSE(_u("INLINING: Skip Inline: Leaf function\tCaller: %s (%s)\n"), inliner->GetDisplayName(),
            inliner->GetDebugNumberSet(debugStringBuffer));
        // Nothing to do
        return false;
    }

    if (!inliner->GetAnyDynamicProfileInfo()->HasCallSiteInfo(inliner))
    {
        INLINE_TESTTRACE(_u("INLINING: Skip Inline: No call site info\tCaller: %s (#%d)\n"), inliner->GetDisplayName(),
            inliner->GetDebugNumberSet(debugStringBuffer));
        return false;
    }

    return true;
}

Js::FunctionInfo *InliningDecider::GetCallSiteFuncInfo(Js::FunctionBody *const inliner, const Js::ProfileId profiledCallSiteId, bool* isConstructorCall, bool* isPolymorphicCall)
{
    Assert(inliner);
    Assert(profiledCallSiteId < inliner->GetProfiledCallSiteCount());

    const auto profileData = inliner->GetAnyDynamicProfileInfo();
    Assert(profileData);

    return profileData->GetCallSiteInfo(inliner, profiledCallSiteId, isConstructorCall, isPolymorphicCall);
}

uint16 InliningDecider::GetConstantArgInfo(Js::FunctionBody *const inliner, const Js::ProfileId profiledCallSiteId)
{
    Assert(inliner);
    Assert(profiledCallSiteId < inliner->GetProfiledCallSiteCount());

    const auto profileData = inliner->GetAnyDynamicProfileInfo();
    Assert(profileData);

    return profileData->GetConstantArgInfo(profiledCallSiteId);
}


bool InliningDecider::HasCallSiteInfo(Js::FunctionBody *const inliner, const Js::ProfileId profiledCallSiteId)
{
    Assert(inliner);
    Assert(profiledCallSiteId < inliner->GetProfiledCallSiteCount());

    const auto profileData = inliner->GetAnyDynamicProfileInfo();
    Assert(profileData);

    return profileData->HasCallSiteInfo(inliner, profiledCallSiteId);
}


Js::FunctionInfo *InliningDecider::InlineCallSite(Js::FunctionBody *const inliner, const Js::ProfileId profiledCallSiteId, uint recursiveInlineDepth)
{
    bool isConstructorCall;
    bool isPolymorphicCall;
    Js::FunctionInfo *functionInfo = GetCallSiteFuncInfo(inliner, profiledCallSiteId, &isConstructorCall, &isPolymorphicCall);
    if (functionInfo)
    {
        return Inline(inliner, functionInfo, isConstructorCall, false, GetConstantArgInfo(inliner, profiledCallSiteId), profiledCallSiteId, recursiveInlineDepth, true);
    }
    return nullptr;
}

uint InliningDecider::InlinePolymorphicCallSite(Js::FunctionBody *const inliner, const Js::ProfileId profiledCallSiteId,
    Js::FunctionBody** functionBodyArray, uint functionBodyArrayLength, bool* canInlineArray, uint recursiveInlineDepth)
{
    Assert(inliner);
    Assert(profiledCallSiteId < inliner->GetProfiledCallSiteCount());
    Assert(functionBodyArray);

    const auto profileData = inliner->GetAnyDynamicProfileInfo();
    Assert(profileData);

    bool isConstructorCall;
    if (!profileData->GetPolymorphicCallSiteInfo(inliner, profiledCallSiteId, &isConstructorCall, functionBodyArray, functionBodyArrayLength))
    {
        return 0;
    }

    uint inlineeCount = 0;
    uint actualInlineeCount  = 0;

    for (inlineeCount = 0; inlineeCount < functionBodyArrayLength; inlineeCount++)
    {
        if (!functionBodyArray[inlineeCount])
        {
            AssertMsg(inlineeCount >= 2, "There are at least two polymorphic call site");
            break;
        }
        if (Inline(inliner, functionBodyArray[inlineeCount]->GetFunctionInfo(), isConstructorCall, true /*isPolymorphicCall*/, 0, profiledCallSiteId, recursiveInlineDepth, false))
        {
            canInlineArray[inlineeCount] = true;
            actualInlineeCount++;
        }
    }
    if (inlineeCount != actualInlineeCount)
    {
        // We generate polymorphic dispatch and call even if there are no inlinees as it's seen to provide a perf boost
        // Skip loop bodies for now as we do not handle re-jit scenarios for the bailouts from them
        if (!PHASE_OFF(Js::PartialPolymorphicInlinePhase, inliner) && !this->isLoopBody)
        {
#if defined(DBG_DUMP) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
            char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif
            INLINE_TESTTRACE(_u("Partial inlining of polymorphic call: %s (%s)\tCaller: %s (%s)\n"),
                functionBodyArray[inlineeCount - 1]->GetDisplayName(), functionBodyArray[inlineeCount - 1]->GetDebugNumberSet(debugStringBuffer),
                inliner->GetDisplayName(),
                inliner->GetDebugNumberSet(debugStringBuffer2));
        }
        else
        {
            return 0;
        }
    }
    return inlineeCount;
}

Js::FunctionInfo *InliningDecider::Inline(Js::FunctionBody *const inliner, Js::FunctionInfo* functionInfo,
    bool isConstructorCall, bool isPolymorphicCall, uint16 constantArgInfo, Js::ProfileId callSiteId, uint recursiveInlineDepth, bool allowRecursiveInlining)
{
#if defined(DBG_DUMP) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
    char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif
    Js::FunctionProxy * proxy = functionInfo->GetFunctionProxy();
    if (proxy && proxy->IsFunctionBody())
    {
        if (isLoopBody && PHASE_OFF(Js::InlineInJitLoopBodyPhase, this->topFunc))
        {
            INLINE_TESTTRACE_VERBOSE(_u("INLINING: Skip Inline: Jit loop body: %s (%s)\n"), this->topFunc->GetDisplayName(),
                this->topFunc->GetDebugNumberSet(debugStringBuffer));
            return nullptr;
        }

        // Note: disable inline for debugger, as we can't bailout at return from function.
        // Alternative can be generate this bailout as part of inline, which can be done later as perf improvement.
        const auto inlinee = proxy->GetFunctionBody();
        Assert(this->jitMode == ExecutionMode::FullJit);
        if (PHASE_OFF(Js::InlinePhase, inlinee) ||
            PHASE_OFF(Js::GlobOptPhase, inlinee) ||
            !ContinueInliningUserDefinedFunctions(this->bytecodeInlinedCount) ||
            this->isInDebugMode)
        {
            return nullptr;
        }

        if (functionInfo->IsDeferred() || inlinee->GetByteCode() == nullptr)
        {
            // DeferredParse...
            INLINE_TESTTRACE(_u("INLINING: Skip Inline: No bytecode\tInlinee: %s (%s)\tCaller: %s (%s)\n"),
                inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer), inliner->GetDisplayName(),
                inliner->GetDebugNumberSet(debugStringBuffer2));
            return nullptr;
        }

        if (inlinee->GetHasTry())
        {
            INLINE_TESTTRACE(_u("INLINING: Skip Inline: Has try\tInlinee: %s (%s)\tCaller: %s (%s)\n"),
                inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer), inliner->GetDisplayName(),
                inliner->GetDebugNumberSet(debugStringBuffer2));
            return nullptr;
        }

        // This is a hard limit as the argOuts array is statically sized.
        if (inlinee->GetInParamsCount() > Js::InlineeCallInfo::MaxInlineeArgoutCount)
        {
            INLINE_TESTTRACE(_u("INLINING: Skip Inline: Params count greater then MaxInlineeArgoutCount\tInlinee: %s (%s)\tParamcount: %d\tMaxInlineeArgoutCount: %d\tCaller: %s (%s)\n"),
                inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer), inlinee->GetInParamsCount(), Js::InlineeCallInfo::MaxInlineeArgoutCount,
                inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2));
            return nullptr;
        }

        if (inlinee->GetInParamsCount() == 0)
        {
            // Inline candidate has no params, not even a this pointer.  This can only be the global function,
            // which we shouldn't inline.
            INLINE_TESTTRACE(_u("INLINING: Skip Inline: Params count is zero!\tInlinee: %s (%s)\tParamcount: %d\tCaller: %s (%s)\n"),
                inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer), inlinee->GetInParamsCount(),
                inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2));
            return nullptr;
        }

        if (inlinee->GetDontInline())
        {
            INLINE_TESTTRACE(_u("INLINING: Skip Inline: Do not inline\tInlinee: %s (%s)\tCaller: %s (%s)\n"),
                inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer), inliner->GetDisplayName(),
                inliner->GetDebugNumberSet(debugStringBuffer2));
            return nullptr;
        }

        // Do not inline a call to a class constructor if it isn't part of a new expression since the call will throw a TypeError anyway.
        if (inlinee->IsClassConstructor() && !isConstructorCall)
        {
            INLINE_TESTTRACE(_u("INLINING: Skip Inline: Class constructor without new keyword\tInlinee: %s (%s)\tCaller: %s (%s)\n"),
                inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer), inliner->GetDisplayName(),
                inliner->GetDebugNumberSet(debugStringBuffer2));
            return nullptr;
        }

        if (!DeciderInlineIntoInliner(inlinee, inliner, isConstructorCall, isPolymorphicCall, constantArgInfo, recursiveInlineDepth, allowRecursiveInlining))
        {
            return nullptr;
        }

#if defined(ENABLE_DEBUG_CONFIG_OPTIONS)
        TraceInlining(inliner, inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer), inlinee->GetByteCodeCount(), this->topFunc, this->bytecodeInlinedCount, inlinee, callSiteId, this->isLoopBody);
#endif

        this->bytecodeInlinedCount += inlinee->GetByteCodeCount();
        return inlinee->GetFunctionInfo();
    }

    Js::OpCode builtInInlineCandidateOpCode;
    ValueType builtInReturnType;
    GetBuiltInInfo(functionInfo, &builtInInlineCandidateOpCode, &builtInReturnType);

    if(builtInInlineCandidateOpCode == 0 && builtInReturnType.IsUninitialized())
    {
        return nullptr;
    }

    Assert(this->jitMode == ExecutionMode::FullJit);
    if (builtInInlineCandidateOpCode != 0 &&
        (
            PHASE_OFF(Js::InlinePhase, inliner) ||
            PHASE_OFF(Js::GlobOptPhase, inliner) ||
            isConstructorCall
        ))
    {
        return nullptr;
    }

    // Note: for built-ins at this time we don't have enough data (the instr) to decide whether it's going to be inlined.
    return functionInfo;
}


// TODO OOP JIT: add FunctionInfo interface so we can combine these?
/* static */
bool InliningDecider::GetBuiltInInfo(
    const FunctionJITTimeInfo *const funcInfo,
    Js::OpCode *const inlineCandidateOpCode,
    ValueType *const returnType
)
{
    Assert(funcInfo);
    Assert(inlineCandidateOpCode);
    Assert(returnType);

    *inlineCandidateOpCode = (Js::OpCode)0;
    *returnType = ValueType::Uninitialized;

    if (funcInfo->HasBody())
    {
        return false;
    }
    return InliningDecider::GetBuiltInInfoCommon(
        funcInfo->GetLocalFunctionId(),
        inlineCandidateOpCode,
        returnType);
}

/* static */
bool InliningDecider::GetBuiltInInfo(
    Js::FunctionInfo *const funcInfo,
    Js::OpCode *const inlineCandidateOpCode,
    ValueType *const returnType
)
{
    Assert(funcInfo);
    Assert(inlineCandidateOpCode);
    Assert(returnType);

    *inlineCandidateOpCode = (Js::OpCode)0;
    *returnType = ValueType::Uninitialized;

    if (funcInfo->HasBody())
    {
        return false;
    }
    return InliningDecider::GetBuiltInInfoCommon(
        funcInfo->GetLocalFunctionId(),
        inlineCandidateOpCode,
        returnType);
}

bool InliningDecider::GetBuiltInInfoCommon(
    uint localFuncId,
    Js::OpCode *const inlineCandidateOpCode,
    ValueType *const returnType
    )
{
    // TODO: consider adding another column to JavascriptBuiltInFunctionList.h/LibraryFunction.h
    // and getting helper method from there instead of multiple switch labels. And for return value types too.
    switch (localFuncId)
    {
    case Js::JavascriptBuiltInFunction::Math_Abs:
        *inlineCandidateOpCode = Js::OpCode::InlineMathAbs;
        break;

    case Js::JavascriptBuiltInFunction::Math_Acos:
        *inlineCandidateOpCode = Js::OpCode::InlineMathAcos;
        break;

    case Js::JavascriptBuiltInFunction::Math_Asin:
        *inlineCandidateOpCode = Js::OpCode::InlineMathAsin;
        break;

    case Js::JavascriptBuiltInFunction::Math_Atan:
        *inlineCandidateOpCode = Js::OpCode::InlineMathAtan;
        break;

    case Js::JavascriptBuiltInFunction::Math_Atan2:
        *inlineCandidateOpCode = Js::OpCode::InlineMathAtan2;
        break;

    case Js::JavascriptBuiltInFunction::Math_Cos:
        *inlineCandidateOpCode = Js::OpCode::InlineMathCos;
        break;

    case Js::JavascriptBuiltInFunction::Math_Exp:
        *inlineCandidateOpCode = Js::OpCode::InlineMathExp;
        break;

    case Js::JavascriptBuiltInFunction::Math_Log:
        *inlineCandidateOpCode = Js::OpCode::InlineMathLog;
        break;

    case Js::JavascriptBuiltInFunction::Math_Pow:
        *inlineCandidateOpCode = Js::OpCode::InlineMathPow;
        break;

    case Js::JavascriptBuiltInFunction::Math_Sin:
        *inlineCandidateOpCode = Js::OpCode::InlineMathSin;
        break;

    case Js::JavascriptBuiltInFunction::Math_Sqrt:
        *inlineCandidateOpCode = Js::OpCode::InlineMathSqrt;
        break;

    case Js::JavascriptBuiltInFunction::Math_Tan:
        *inlineCandidateOpCode = Js::OpCode::InlineMathTan;
        break;

    case Js::JavascriptBuiltInFunction::Math_Floor:
        *inlineCandidateOpCode = Js::OpCode::InlineMathFloor;
        break;

    case Js::JavascriptBuiltInFunction::Math_Ceil:
        *inlineCandidateOpCode = Js::OpCode::InlineMathCeil;
        break;

    case Js::JavascriptBuiltInFunction::Math_Round:
        *inlineCandidateOpCode = Js::OpCode::InlineMathRound;
        break;

    case Js::JavascriptBuiltInFunction::Math_Min:
        *inlineCandidateOpCode = Js::OpCode::InlineMathMin;
        break;

    case Js::JavascriptBuiltInFunction::Math_Max:
        *inlineCandidateOpCode = Js::OpCode::InlineMathMax;
        break;

    case Js::JavascriptBuiltInFunction::Math_Imul:
        *inlineCandidateOpCode = Js::OpCode::InlineMathImul;
        break;

    case Js::JavascriptBuiltInFunction::Math_Clz32:
        *inlineCandidateOpCode = Js::OpCode::InlineMathClz;
        break;

    case Js::JavascriptBuiltInFunction::Math_Random:
        *inlineCandidateOpCode = Js::OpCode::InlineMathRandom;
        break;

    case Js::JavascriptBuiltInFunction::Math_Fround:
        *inlineCandidateOpCode = Js::OpCode::InlineMathFround;
        *returnType = ValueType::Float;
        break;

    case Js::JavascriptBuiltInFunction::JavascriptArray_Push:
        *inlineCandidateOpCode = Js::OpCode::InlineArrayPush;
        break;

    case Js::JavascriptBuiltInFunction::JavascriptArray_Pop:
        *inlineCandidateOpCode = Js::OpCode::InlineArrayPop;
        break;

    case Js::JavascriptBuiltInFunction::JavascriptArray_Concat:
    case Js::JavascriptBuiltInFunction::JavascriptArray_Reverse:
    case Js::JavascriptBuiltInFunction::JavascriptArray_Shift:
    case Js::JavascriptBuiltInFunction::JavascriptArray_Slice:
    case Js::JavascriptBuiltInFunction::JavascriptArray_Splice:

    case Js::JavascriptBuiltInFunction::JavascriptString_Link:
    case Js::JavascriptBuiltInFunction::JavascriptString_LocaleCompare:
        goto CallDirectCommon;

    case Js::JavascriptBuiltInFunction::JavascriptArray_Join:
    case Js::JavascriptBuiltInFunction::JavascriptString_CharAt:
    case Js::JavascriptBuiltInFunction::JavascriptString_Concat:
    case Js::JavascriptBuiltInFunction::JavascriptString_FromCharCode:
    case Js::JavascriptBuiltInFunction::JavascriptString_FromCodePoint:
    case Js::JavascriptBuiltInFunction::JavascriptString_Replace:
    case Js::JavascriptBuiltInFunction::JavascriptString_Slice:
    case Js::JavascriptBuiltInFunction::JavascriptString_Substr:
    case Js::JavascriptBuiltInFunction::JavascriptString_Substring:
    case Js::JavascriptBuiltInFunction::JavascriptString_ToLocaleLowerCase:
    case Js::JavascriptBuiltInFunction::JavascriptString_ToLocaleUpperCase:
    case Js::JavascriptBuiltInFunction::JavascriptString_ToLowerCase:
    case Js::JavascriptBuiltInFunction::JavascriptString_ToUpperCase:
    case Js::JavascriptBuiltInFunction::JavascriptString_Trim:
    case Js::JavascriptBuiltInFunction::JavascriptString_TrimLeft:
    case Js::JavascriptBuiltInFunction::JavascriptString_TrimRight:
    case Js::JavascriptBuiltInFunction::JavascriptString_PadStart:
    case Js::JavascriptBuiltInFunction::JavascriptString_PadEnd:
        *returnType = ValueType::String;
        goto CallDirectCommon;

    case Js::JavascriptBuiltInFunction::JavascriptArray_Includes:
    case Js::JavascriptBuiltInFunction::JavascriptObject_HasOwnProperty:
    case Js::JavascriptBuiltInFunction::JavascriptArray_IsArray:
        *returnType = ValueType::Boolean;
        goto CallDirectCommon;

    case Js::JavascriptBuiltInFunction::JavascriptArray_IndexOf:
    case Js::JavascriptBuiltInFunction::JavascriptArray_LastIndexOf:
    case Js::JavascriptBuiltInFunction::JavascriptArray_Unshift:
    case Js::JavascriptBuiltInFunction::JavascriptString_CharCodeAt:
    case Js::JavascriptBuiltInFunction::JavascriptString_IndexOf:
    case Js::JavascriptBuiltInFunction::JavascriptString_LastIndexOf:
    case Js::JavascriptBuiltInFunction::JavascriptString_Search:
    case Js::JavascriptBuiltInFunction::JavascriptRegExp_SymbolSearch:
    case Js::JavascriptBuiltInFunction::GlobalObject_ParseInt:
        *returnType = ValueType::GetNumberAndLikelyInt(true);
        goto CallDirectCommon;

    case Js::JavascriptBuiltInFunction::JavascriptString_Split:
        *returnType = ValueType::GetObject(ObjectType::Array).SetHasNoMissingValues(true).SetArrayTypeId(Js::TypeIds_Array);
        goto CallDirectCommon;

    case Js::JavascriptBuiltInFunction::JavascriptString_Match:
    case Js::JavascriptBuiltInFunction::JavascriptRegExp_Exec:
        *returnType =
            ValueType::GetObject(ObjectType::Array).SetHasNoMissingValues(true).SetArrayTypeId(Js::TypeIds_Array)
                .Merge(ValueType::Null);
        goto CallDirectCommon;

    CallDirectCommon:
        *inlineCandidateOpCode = Js::OpCode::CallDirect;
        break;

    case Js::JavascriptBuiltInFunction::JavascriptFunction_Apply:
        *inlineCandidateOpCode = Js::OpCode::InlineFunctionApply;
        break;
    case Js::JavascriptBuiltInFunction::JavascriptFunction_Call:
        *inlineCandidateOpCode = Js::OpCode::InlineFunctionCall;
        break;

    // The following are not currently inlined, but are tracked for their return type
    // TODO: Add more built-ins that return objects. May consider tracking all built-ins.

    case Js::JavascriptBuiltInFunction::JavascriptArray_NewInstance:
        *returnType = ValueType::GetObject(ObjectType::Array).SetHasNoMissingValues(true).SetArrayTypeId(Js::TypeIds_Array);
        break;

    case Js::JavascriptBuiltInFunction::Int8Array_NewInstance:
#ifdef _M_X64
        *returnType = (!PHASE_OFF1(Js::TypedArrayVirtualPhase)) ? ValueType::GetObject(ObjectType::Int8MixedArray) : ValueType::GetObject(ObjectType::Int8Array);
#else
        *returnType = ValueType::GetObject(ObjectType::Int8Array);
#endif
        break;

    case Js::JavascriptBuiltInFunction::Uint8Array_NewInstance:
#ifdef _M_X64
        *returnType = (!PHASE_OFF1(Js::TypedArrayVirtualPhase)) ? ValueType::GetObject(ObjectType::Uint8MixedArray) : ValueType::GetObject(ObjectType::Uint8Array);
#else
        *returnType = ValueType::GetObject(ObjectType::Uint8Array);
#endif
        break;

    case Js::JavascriptBuiltInFunction::Uint8ClampedArray_NewInstance:
#ifdef _M_X64
        *returnType = (!PHASE_OFF1(Js::TypedArrayVirtualPhase)) ? ValueType::GetObject(ObjectType::Uint8ClampedMixedArray) : ValueType::GetObject(ObjectType::Uint8ClampedArray);
#else
        *returnType = ValueType::GetObject(ObjectType::Uint8ClampedArray);
#endif
        break;

    case Js::JavascriptBuiltInFunction::Int16Array_NewInstance:
#ifdef _M_X64
        *returnType = (!PHASE_OFF1(Js::TypedArrayVirtualPhase)) ? ValueType::GetObject(ObjectType::Int16MixedArray) : ValueType::GetObject(ObjectType::Int16Array);
#else
        *returnType = ValueType::GetObject(ObjectType::Int16Array);
#endif
        break;

    case Js::JavascriptBuiltInFunction::Uint16Array_NewInstance:
#ifdef _M_X64
        *returnType = (!PHASE_OFF1(Js::TypedArrayVirtualPhase)) ? ValueType::GetObject(ObjectType::Uint16MixedArray) : ValueType::GetObject(ObjectType::Uint16Array);
#else
        *returnType = ValueType::GetObject(ObjectType::Uint16Array);
#endif
        break;

    case Js::JavascriptBuiltInFunction::Int32Array_NewInstance:
#ifdef _M_X64
        *returnType = (!PHASE_OFF1(Js::TypedArrayVirtualPhase)) ? ValueType::GetObject(ObjectType::Int32MixedArray) : ValueType::GetObject(ObjectType::Int32Array);
#else
        *returnType = ValueType::GetObject(ObjectType::Int32Array);
#endif
        break;

    case Js::JavascriptBuiltInFunction::Uint32Array_NewInstance:
#ifdef _M_X64
        *returnType = (!PHASE_OFF1(Js::TypedArrayVirtualPhase)) ? ValueType::GetObject(ObjectType::Uint32MixedArray) : ValueType::GetObject(ObjectType::Uint32Array);
#else
        *returnType = ValueType::GetObject(ObjectType::Uint32Array);
#endif
        break;

    case Js::JavascriptBuiltInFunction::Float32Array_NewInstance:
#ifdef _M_X64
        *returnType = (!PHASE_OFF1(Js::TypedArrayVirtualPhase)) ? ValueType::GetObject(ObjectType::Float32MixedArray) : ValueType::GetObject(ObjectType::Float32Array);
#else
        *returnType = ValueType::GetObject(ObjectType::Float32Array);
#endif
        break;

    case Js::JavascriptBuiltInFunction::Float64Array_NewInstance:
#ifdef _M_X64
        *returnType = (!PHASE_OFF1(Js::TypedArrayVirtualPhase)) ? ValueType::GetObject(ObjectType::Float64MixedArray) : ValueType::GetObject(ObjectType::Float64Array);
#else
        *returnType = ValueType::GetObject(ObjectType::Float64Array);
#endif
        break;

    case Js::JavascriptBuiltInFunction::Int64Array_NewInstance:
        *returnType = ValueType::GetObject(ObjectType::Int64Array);
        break;

    case Js::JavascriptBuiltInFunction::Uint64Array_NewInstance:
        *returnType = ValueType::GetObject(ObjectType::Uint64Array);
        break;

    case Js::JavascriptBuiltInFunction::BoolArray_NewInstance:
        *returnType = ValueType::GetObject(ObjectType::BoolArray);
        break;

    case Js::JavascriptBuiltInFunction::CharArray_NewInstance:
        *returnType = ValueType::GetObject(ObjectType::CharArray);
        break;

#ifdef ENABLE_DOM_FAST_PATH
    case Js::JavascriptBuiltInFunction::DOMFastPathGetter:
        *inlineCandidateOpCode = Js::OpCode::DOMFastPathGetter;
        break;
#endif

    default:
        return false;
    }
    return true;
}

bool InliningDecider::CanRecursivelyInline(Js::FunctionBody * inlinee, Js::FunctionBody *inliner, bool allowRecursiveInlining, uint recursiveInlineDepth)
{
#if defined(DBG_DUMP) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
    char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif


    if (!PHASE_OFF(Js::InlineRecursivePhase, inliner)
        && allowRecursiveInlining
        &&  inlinee == inliner
        &&  inlinee->CanInlineRecursively(recursiveInlineDepth))
    {
        INLINE_TESTTRACE(_u("INLINING: Inlined recursively\tInlinee: %s (%s)\tCaller: %s (%s)\tDepth: %d\n"),
            inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer),
            inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2), recursiveInlineDepth);
        return true;
    }

    if (!inlinee->CanInlineAgain())
    {
        INLINE_TESTTRACE(_u("INLINING: Skip Inline: Do not inline recursive functions\tInlinee: %s (%s)\tCaller: %s (%s)\n"),
            inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer),
            inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2));
        return false;
    }

    return true;
}

// This only enables collection of the inlinee data, we are much more aggressive here.
// Actual decision of whether something is inlined or not is taken in CommitInlineIntoInliner
bool InliningDecider::DeciderInlineIntoInliner(Js::FunctionBody * inlinee, Js::FunctionBody * inliner, bool isConstructorCall, bool isPolymorphicCall, uint16 constantArgInfo, uint recursiveInlineDepth, bool allowRecursiveInlining)
{

    if (!CanRecursivelyInline(inlinee, inliner, allowRecursiveInlining, recursiveInlineDepth))
    {
        return false;
    }

    if (inlinee->GetIsAsmjsMode() || inliner->GetIsAsmjsMode())
    {
        return false;
    }

    if (PHASE_FORCE(Js::InlinePhase, this->topFunc) ||
        PHASE_FORCE(Js::InlinePhase, inliner) ||
        PHASE_FORCE(Js::InlinePhase, inlinee))
    {
        return true;
    }

    if (PHASE_OFF(Js::InlinePhase, this->topFunc) ||
        PHASE_OFF(Js::InlinePhase, inliner) ||
        PHASE_OFF(Js::InlinePhase, inlinee))
    {
        return false;
    }

    if (PHASE_FORCE(Js::InlineTreePhase, this->topFunc) ||
        PHASE_FORCE(Js::InlineTreePhase, inliner))
    {
        return true;
    }

    if (PHASE_FORCE(Js::InlineAtEveryCallerPhase, inlinee))
    {
        return true;
    }

    uint inlineeByteCodeCount = inlinee->GetByteCodeWithoutLDACount();

    // Heuristics are hit in the following order (Note *order* is important)
    // 1. Leaf function:  If the inlinee is a leaf (but not a constructor or a polymorphic call) inline threshold is LeafInlineThreshold (60). Also it can have max 1 loop
    // 2. Constant Function Argument: If the inlinee candidate has a constant argument and that argument is used for branching, then the inline threshold is ConstantArgumentInlineThreshold (157)
    // 3. InlineThreshold: If an inlinee candidate exceeds InlineThreshold just don't inline no matter what.

    // Following are additional constraint for an inlinee which meets InlineThreshold (Rule no 3)
    // 4. Rule for inlinee with loops:
    //      4a. Only single loop in inlinee is permitted.
    //      4b. Should not have polymorphic field access.
    //      4c. Should not be a constructor.
    //      4d. Should meet LoopInlineThreshold (25)
    // 5. Rule for polymorphic inlinee:
    //      4a. Should meet PolymorphicInlineThreshold (32)
    // 6. Rule for constructors:
    //       5a. Always inline if inlinee has polymorphic field access (as we have cloned runtime data).
    //       5b. If inlinee is monomorphic, inline only small constructors. They are governed by ConstructorInlineThreshold (21)
    // 7. Rule for inlinee which is not interpreted enough (as we might not have all the profile data):
    //       7a. As of now it is still governed by the InlineThreshold. Plan to play with this in future.
    // 8. Rest should be inlined.

    uint16 mask = constantArgInfo &  inlinee->m_argUsedForBranch;
    if (mask && inlineeByteCodeCount <  (uint)CONFIG_FLAG(ConstantArgumentInlineThreshold))
    {
        return true;
    }

    int inlineThreshold = threshold.inlineThreshold;
    if (!isPolymorphicCall && !isConstructorCall && IsInlineeLeaf(inlinee) && (inlinee->GetLoopCount() <= 2))
    {
        // Inlinee is a leaf function
        if (inlinee->GetLoopCount() == 0 || GetNumberOfInlineesWithLoop() <= (uint)threshold.maxNumberOfInlineesWithLoop) // Don't inlinee too many inlinees with loops.
        {
            // Negative LeafInlineThreshold disable the threshold
            if (threshold.leafInlineThreshold >= 0)
            {
                inlineThreshold += threshold.leafInlineThreshold - threshold.inlineThreshold;
            }
        }
    }

#if ENABLE_DEBUG_CONFIG_OPTIONS
    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
    char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
    char16 debugStringBuffer3[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif

    if (inlinee->GetHasLoops())
    {
        if (threshold.loopInlineThreshold < 0 ||                                     // Negative LoopInlineThreshold disable inlining with loop
            GetNumberOfInlineesWithLoop()  >(uint)threshold.maxNumberOfInlineesWithLoop || // See if we are inlining too many inlinees with loops.
            (inlinee->GetLoopCount() > 2) ||                                         // Allow at most 2 loops.
            inlinee->GetHasNestedLoop() ||                                           // Nested loops are not a good inlinee candidate
            isConstructorCall ||                                                     // If the function is constructor with loops, don't inline.
            PHASE_OFF(Js::InlineFunctionsWithLoopsPhase, this->topFunc))
        {
            INLINE_TESTTRACE(_u("INLINING: Skip Inline: Has loops \tBytecode size: %d \tgetNumberOfInlineesWithLoop: %d\tloopCount: %d\thasNestedLoop: %B\tisConstructorCall:%B\tInlinee: %s (%s)\tCaller: %s (%s) \tRoot: %s (%s)\n"),
                inlinee->GetByteCodeCount(),
                GetNumberOfInlineesWithLoop(),
                inlinee->GetLoopCount(),
                inlinee->GetHasNestedLoop(),
                isConstructorCall,
                inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer),
                inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2),
                topFunc->GetDisplayName(), topFunc->GetDebugNumberSet(debugStringBuffer3));
            // Don't inline function with loops
            return false;
        }
        else
        {
            inlineThreshold -= (threshold.inlineThreshold > threshold.loopInlineThreshold) ? threshold.inlineThreshold - threshold.loopInlineThreshold : 0;
        }
    }

    if (isPolymorphicCall)
    {
        if (threshold.polymorphicInlineThreshold < 0 ||                              // Negative PolymorphicInlineThreshold disable inlining
            isConstructorCall)
        {
            INLINE_TESTTRACE(_u("INLINING: Skip Inline: Polymorphic call under PolymorphicInlineThreshold: %d \tBytecode size: %d\tInlinee: %s (%s)\tCaller: %s (%s) \tRoot: %s (%s)\n"),
                threshold.polymorphicInlineThreshold,
                inlinee->GetByteCodeCount(),
                inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer),
                inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2),
                topFunc->GetDisplayName(), topFunc->GetDebugNumberSet(debugStringBuffer3));
            return false;
        }
        else
        {
            inlineThreshold -= (threshold.inlineThreshold > threshold.polymorphicInlineThreshold) ? threshold.inlineThreshold - threshold.polymorphicInlineThreshold : 0;
        }
    }

    if (isConstructorCall)
    {
#pragma prefast(suppress: 6285, "logical-or of constants is by design")
        if (PHASE_OFF(Js::InlineConstructorsPhase, this->topFunc) ||
            PHASE_OFF(Js::InlineConstructorsPhase, inliner) ||
            PHASE_OFF(Js::InlineConstructorsPhase, inlinee) ||
            !CONFIG_FLAG(CloneInlinedPolymorphicCaches))
        {
            return false;
        }

        if (PHASE_FORCE(Js::InlineConstructorsPhase, this->topFunc) ||
            PHASE_FORCE(Js::InlineConstructorsPhase, inliner) ||
            PHASE_FORCE(Js::InlineConstructorsPhase, inlinee))
        {
            return true;
        }

        if (inlinee->HasDynamicProfileInfo() && inlinee->GetAnyDynamicProfileInfo()->HasPolymorphicFldAccess())
        {
            // As of now this is not dependent on bytecodeInlinedThreshold.
            return true;
        }

        // Negative ConstructorInlineThreshold always disable constructor inlining
        if (threshold.constructorInlineThreshold < 0)
        {
            INLINE_TESTTRACE(_u("INLINING: Skip Inline: Constructor with no polymorphic field access \tBytecode size: %d\tInlinee: %s (%s)\tCaller: %s (%s) \tRoot: %s (%s)\n"),
                inlinee->GetByteCodeCount(),
                inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer),
                inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2),
                topFunc->GetDisplayName(), topFunc->GetDebugNumberSet(debugStringBuffer3));
            // Don't inline constructor that does not have a polymorphic field access, or if cloning polymorphic inline
            // caches is disabled
            return false;
        }
        else
        {
            inlineThreshold -= (threshold.inlineThreshold > threshold.constructorInlineThreshold) ? threshold.inlineThreshold - threshold.constructorInlineThreshold : 0;
        }
    }

    if (threshold.forLoopBody)
    {
        inlineThreshold /= CONFIG_FLAG(InlineInLoopBodyScaleDownFactor);
    }

    if (inlineThreshold > 0 && inlineeByteCodeCount <= (uint)inlineThreshold)
    {
        if (inlinee->GetLoopCount())
        {
            IncrementNumberOfInlineesWithLoop();
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool InliningDecider::ContinueInliningUserDefinedFunctions(uint32 bytecodeInlinedCount) const
{
#if ENABLE_DEBUG_CONFIG_OPTIONS
    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif
    if (PHASE_FORCE(Js::InlinePhase, this->topFunc) || bytecodeInlinedCount <= (uint)this->threshold.inlineCountMax)
    {
        return true;
    }

    INLINE_TESTTRACE(_u("INLINING: Skip Inline: InlineCountMax threshold %d, reached: %s (#%s)\n"),
        (uint)this->threshold.inlineCountMax,
        this->topFunc->GetDisplayName(), this->topFunc->GetDebugNumberSet(debugStringBuffer));

    return false;
}

#if defined(ENABLE_DEBUG_CONFIG_OPTIONS)
// static
void InliningDecider::TraceInlining(Js::FunctionBody *const inliner, const char16* inlineeName, const char16* inlineeFunctionIdandNumberString, uint inlineeByteCodeCount,
    Js::FunctionBody* topFunc, uint inlinedByteCodeCount, Js::FunctionBody *const inlinee, uint callSiteId, bool inLoopBody, uint builtIn)
{
    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
    char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
    char16 debugStringBuffer3[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
    if (inlineeName == nullptr)
    {
        int len = swprintf_s(debugStringBuffer3, MAX_FUNCTION_BODY_DEBUG_STRING_SIZE, _u("built In Id: %u"), builtIn);
        Assert(len > 14);
        inlineeName = debugStringBuffer3;
    }
    INLINE_TESTTRACE(_u("INLINING %s: Inlinee: %s (%s)\tSize: %d\tCaller: %s (%s)\tSize: %d\tInlineCount: %d\tRoot: %s (%s)\tSize: %d\tCallSiteId: %d\n"),
        inLoopBody ? _u("IN LOOP BODY") : _u(""),
        inlineeName, inlineeFunctionIdandNumberString, inlineeByteCodeCount,
        inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer), inliner->GetByteCodeCount(),
        inlinedByteCodeCount,
        topFunc->GetDisplayName(), topFunc->GetDebugNumberSet(debugStringBuffer2), topFunc->GetByteCodeCount(),
        callSiteId
        );

    INLINE_TRACE(_u("INLINING %s: Inlinee: %s(%s)\tSize : %d\tCaller : %s(%s)\tSize : %d\tInlineCount : %d\tRoot : %s(%s)\tSize : %d\tCallSiteId : %d\n"),
        inLoopBody ? _u("IN LOOP BODY") : _u(""),
        inlineeName, inlineeFunctionIdandNumberString, inlineeByteCodeCount,
        inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer), inliner->GetByteCodeCount(),
        inlinedByteCodeCount,
        topFunc->GetDisplayName(), topFunc->GetDebugNumberSet(debugStringBuffer2), topFunc->GetByteCodeCount(),
        callSiteId
        );

    // Now Trace inlining across files cases

    if (builtIn != -1)  // built-in functions
    {
        return;
    }

    Assert(inliner && inlinee);

    if (inliner->GetSourceContextId() != inlinee->GetSourceContextId())
    {
        INLINE_TESTTRACE(_u("INLINING_ACROSS_FILES: Inlinee: %s (%s)\tSize: %d\tCaller: %s (%s)\tSize: %d\tInlineCount: %d\tRoot: %s (%s)\tSize: %d\n"),
            inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer), inlinee->GetByteCodeCount(),
            inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2), inliner->GetByteCodeCount(), inlinedByteCodeCount,
            topFunc->GetDisplayName(), topFunc->GetDebugNumberSet(debugStringBuffer3), topFunc->GetByteCodeCount()
            );

        INLINE_TRACE(_u("INLINING_ACROSS_FILES: Inlinee: %s (%s)\tSize: %d\tCaller: %s (%s)\tSize: %d\tInlineCount: %d\tRoot: %s (%s)\tSize: %d\n"),
            inlinee->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer), inlinee->GetByteCodeCount(),
            inliner->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2), inliner->GetByteCodeCount(), inlinedByteCodeCount,
            topFunc->GetDisplayName(), topFunc->GetDebugNumberSet(debugStringBuffer3), topFunc->GetByteCodeCount()
            );
    }

}
#endif
