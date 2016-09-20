//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

InliningThreshold::InliningThreshold(uint nonLoadByteCodeCount, bool forLoopBody, bool aggressive) : nonLoadByteCodeCount(nonLoadByteCodeCount)
{
    this->forLoopBody = forLoopBody;
    if (aggressive)
    {
        SetAggressiveHeuristics();
    }
    else
    {
        SetHeuristics();
    }
}
void InliningThreshold::SetAggressiveHeuristics()
{
    Assert(!this->forLoopBody);
    int limit = CONFIG_FLAG(AggressiveInlineThreshold);

    inlineThreshold = limit;
    constructorInlineThreshold = limit;
    outsideLoopInlineThreshold = limit;
    leafInlineThreshold = limit;
    loopInlineThreshold = limit;
    polymorphicInlineThreshold = limit;
    maxNumberOfInlineesWithLoop = CONFIG_FLAG(MaxNumberOfInlineesWithLoop);

    inlineCountMax = CONFIG_FLAG(AggressiveInlineCountMax);
}

void InliningThreshold::Reset()
{
    SetHeuristics();
}

void InliningThreshold::SetHeuristics()
{
    inlineThreshold = CONFIG_FLAG(InlineThreshold);
    // Inline less aggressively in large functions since the register pressure is likely high.
    // Small functions shouldn't be a problem.
    if (nonLoadByteCodeCount > 800)
    {
        inlineThreshold -= CONFIG_FLAG(InlineThresholdAdjustCountInLargeFunction);
    }
    else if (nonLoadByteCodeCount > 200)
    {
        inlineThreshold -= CONFIG_FLAG(InlineThresholdAdjustCountInMediumSizedFunction);
    }
    else if (nonLoadByteCodeCount < 50)
    {
        inlineThreshold += CONFIG_FLAG(InlineThresholdAdjustCountInSmallFunction);
    }

    constructorInlineThreshold = CONFIG_FLAG(ConstructorInlineThreshold);
    outsideLoopInlineThreshold = CONFIG_FLAG(OutsideLoopInlineThreshold);
    leafInlineThreshold = CONFIG_FLAG(LeafInlineThreshold);
    loopInlineThreshold = CONFIG_FLAG(LoopInlineThreshold);
    polymorphicInlineThreshold = CONFIG_FLAG(PolymorphicInlineThreshold);
    maxNumberOfInlineesWithLoop = CONFIG_FLAG(MaxNumberOfInlineesWithLoop);
    constantArgumentInlineThreshold = CONFIG_FLAG(ConstantArgumentInlineThreshold);
    inlineCountMax = !forLoopBody ? CONFIG_FLAG(InlineCountMax) : CONFIG_FLAG(InlineCountMaxInLoopBodies);
}

// Called from background thread to commit inlining.
bool InliningHeuristics::BackendInlineIntoInliner(const FunctionJITTimeInfo * inlinee,
                                Func * inliner,
                                Func *topFunction,
                                Js::ProfileId callSiteId,
                                bool isConstructorCall,
                                bool isFixedMethodCall,                     // Reserved
                                bool isCallOutsideLoopInTopFunc,            // There is a loop for sure and this call is outside loop
                                bool isCallInsideLoop,
                                uint recursiveInlineDepth,
                                uint16 constantArguments
                                )
{
    // We have one piece of additional data in backend, whether  we are outside loop or inside
    // This function decides to inline or not based on that additional data. Most of the filtering is already done by DeciderInlineIntoInliner which is called
    // during work item creation.
    // This is additional filtering during actual inlining phase.

    // Note *order* is important
    // Following are
    // 1. Constructor is always inlined (irrespective of inside or outside)
    // 2. If the inlinee candidate has constant argument and that argument is used for a branch and the inlinee size is within ConstantArgumentInlineThreshold(157) we inline
    // 3. Inside loops:
    //     3a. Leaf function will always get inlined (irrespective of leaf has loop or not)
    //     3b. If the inlinee has loops, don't inline it (Basically avoiding inlining a loop within another loop unless its leaf).
    // 4. Outside loop (inliner has loops):
    //     4a. Only inline small inlinees. Governed by OutsideLoopInlineThreshold (16)
    // 5. Rest are inlined.
#if ENABLE_DEBUG_CONFIG_OPTIONS
    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
    char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
    char16 debugStringBuffer3[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif

    // TODO: OOP JIT, somehow need to track across functions
    bool doBackEndAggressiveInline = (constantArguments & inlinee->GetBody()->GetArgUsedForBranch()) != 0;

    if (!PHASE_OFF(Js::InlineRecursivePhase, inliner)
        && inlinee->GetBody()->GetAddr() == inliner->GetJITFunctionBody()->GetAddr()
        && (!inlinee->GetBody()->CanInlineRecursively(recursiveInlineDepth, doBackEndAggressiveInline)))
    {
        INLINE_TESTTRACE(_u("INLINING: Skip Inline (backend): Recursive inlining\tInlinee: %s (#%s)\tCaller: %s (#%s) \tRoot: %s (#%s) Depth: %d\n"),
            inlinee->GetBody()->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer),
            inliner->GetJITFunctionBody()->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2),
            topFunc->GetBody()->GetDisplayName(), topFunc->GetDebugNumberSet(debugStringBuffer3),
            recursiveInlineDepth);
        return false;
    }

    if(PHASE_FORCE(Js::InlinePhase, this->topFunc) ||
        PHASE_FORCE(Js::InlinePhase, inliner) ||
        PHASE_FORCE(Js::InlinePhase, inlinee))
    {
        return true;
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

    const JITTimeProfileInfo *dynamicProfile = inliner->GetReadOnlyProfileInfo();

    bool doConstantArgumentInlining = (dynamicProfile && dynamicProfile->GetConstantArgInfo(callSiteId) & inlinee->GetBody()->GetArgUsedForBranch()) != 0;
    if (doConstantArgumentInlining && inlinee->GetBody()->GetNonLoadByteCodeCount() <  (uint)threshold.constantArgumentInlineThreshold)
    {
        return true;
    }


    if (topFunction->GetWorkItem()->GetJITTimeInfo()->IsAggressiveInliningEnabled())
    {
        return true;
    }

    if (isConstructorCall)
    {
        return true;
    }


    if (isCallInsideLoop && IsInlineeLeaf(inlinee))
    {
        return true;
    }

    if (isCallInsideLoop && inlinee->GetBody()->HasLoops() )                            // Don't inline function with loops inside another loop unless it is a leaf
    {
        INLINE_TESTTRACE(_u("INLINING: Skip Inline (backend): Recursive loop inlining\tInlinee: %s (#%s)\tCaller: %s (#%s) \tRoot: %s (#%s)\n"),
            inlinee->GetBody()->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer),
            inliner->GetJITFunctionBody()->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2),
            topFunc->GetBody()->GetDisplayName(), topFunc->GetDebugNumberSet(debugStringBuffer3));
        return false;
    }
    byte scale = 1;

    if (doBackEndAggressiveInline)
    {
        scale = 2;
    }

    if (isCallOutsideLoopInTopFunc &&
        (threshold.outsideLoopInlineThreshold < 0 ||
        inlinee->GetBody()->GetNonLoadByteCodeCount() > (uint)threshold.outsideLoopInlineThreshold * scale))
    {
        Assert(!isCallInsideLoop);
        INLINE_TESTTRACE(_u("INLINING: Skip Inline (backend): Inlining outside loop doesn't meet OutsideLoopInlineThreshold: %d \tBytecode size: %d\tInlinee: %s (#%s)\tCaller: %s (#%s) \tRoot: %s (#%s)\n"),
            threshold.outsideLoopInlineThreshold,
            inlinee->GetBody()->GetByteCodeCount(),
            inlinee->GetBody()->GetDisplayName(), inlinee->GetDebugNumberSet(debugStringBuffer),
            inliner->GetJITFunctionBody()->GetDisplayName(), inliner->GetDebugNumberSet(debugStringBuffer2),
            topFunc->GetBody()->GetDisplayName(), topFunc->GetDebugNumberSet(debugStringBuffer3));
        return false;
    }
    return true;
}
