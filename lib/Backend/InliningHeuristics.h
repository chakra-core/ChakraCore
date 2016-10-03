//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class InliningDecider;

struct InliningThreshold
{
    uint nonLoadByteCodeCount;
    bool forLoopBody;
    int inlineThreshold;
    int constructorInlineThreshold;
    int outsideLoopInlineThreshold;
    int leafInlineThreshold;
    int loopInlineThreshold;
    int polymorphicInlineThreshold;
    int inlineCountMax;
    int maxNumberOfInlineesWithLoop;
    int constantArgumentInlineThreshold;

    InliningThreshold(uint nonLoadByteCodeCount, bool forLoopBody, bool aggressive = false);
    void SetHeuristics();
    void SetAggressiveHeuristics();
    void Reset();
};

class InliningHeuristics
{
    friend class ::InliningDecider;

    const FunctionJITTimeInfo * topFunc;
    InliningThreshold threshold;

public:

public:
    InliningHeuristics(const FunctionJITTimeInfo * topFunc, bool forLoopBody) :topFunc(topFunc), threshold(topFunc->GetBody()->GetNonLoadByteCodeCount(), forLoopBody) {};
    bool BackendInlineIntoInliner(const FunctionJITTimeInfo * inlinee,
                                Func * inliner,
                                Func *topFunc,
                                Js::ProfileId,
                                bool isConstructorCall,
                                bool isFixedMethodCall,
                                bool isCallOutsideLoopInTopFunc,
                                bool isCallInsideLoop,
                                uint recursiveInlineDepth,
                                uint16 constantArguments
                                );
private:
    static bool IsInlineeLeaf(const FunctionJITTimeInfo * inlinee)
    {
        return inlinee->GetBody()->HasProfileInfo()
            && (!PHASE_OFF(Js::InlineBuiltInCallerPhase, inlinee) ? !inlinee->GetBody()->HasNonBuiltInCallee() : inlinee->GetBody()->GetProfiledCallSiteCount() == 0)
            && !inlinee->GetBody()->GetReadOnlyProfileInfo()->HasLdFldCallSiteInfo();
    }

};


