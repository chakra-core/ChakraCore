//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

struct InliningThreshold
{
    uint nonLoadByteCodeCount;
    int inlineThreshold;
    int constructorInlineThreshold;
    int outsideLoopInlineThreshold;
    int leafInlineThreshold;
    int loopInlineThreshold;
    int polymorphicInlineThreshold;
    int inlineCountMax;
    int maxNumberOfInlineesWithLoop;
    int constantArgumentInlineThreshold;

    InliningThreshold(uint nonLoadByteCodeCount, bool aggressive = false);
    void SetHeuristics();
    void SetAggressiveHeuristics();
    void Reset();
};

class InliningHeuristics
{
    friend class InliningDecider;

    const JITTimeFunctionBody * topFunc;
    InliningThreshold threshold;

public:

public:
    InliningHeuristics(const JITTimeFunctionBody * topFunc) :topFunc(topFunc), threshold(topFunc->GetNonLoadByteCodeCount()) {};
    bool BackendInlineIntoInliner(Func * inlinee,
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
    static bool IsInlineeLeaf(Func * inlinee)
    {
        return inlinee->HasProfileInfo()
            && (!PHASE_OFF(Js::InlineBuiltInCallerPhase, inlinee) ? !inlinee->GetJITFunctionBody()->HasNonBuiltInCallee() : inlinee->GetJITFunctionBody()->GetProfiledCallSiteCount() == 0)
            && !inlinee->GetProfileInfo()->HasLdFldCallSiteInfo();
    }

};


