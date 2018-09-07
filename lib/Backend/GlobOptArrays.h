//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class GlobOpt::ArraySrcOpt
{
public:
    ArraySrcOpt(GlobOpt * glob, IR::Instr ** instrRef, Value ** _src1Val, Value ** _src2Val) :
        globOpt(glob),
        func(glob->func),
        instr(*instrRef),
        src1Val(*_src1Val),
        src2Val(*_src2Val)
    {
        Assert(instr != nullptr);
    }

    ~ArraySrcOpt();

    void Optimize();

private:
    bool CheckOpCode();

    void TypeSpecIndex();
    void UpdateValue(StackSym * newHeadSegmentSym, StackSym * newHeadSegmentLengthSym, StackSym * newLengthSym);
    void CheckVirtualArrayBounds();
    void TryEliminiteBoundsCheck();
    void CheckLoops();
    void DoArrayChecks();
    void DoLengthLoad();
    void DoHeadSegmentLengthLoad();
    void DoExtractBoundChecks();
    void DoLowerBoundCheck();
    void DoUpperBoundCheck();

    void UpdateHoistedValueInfo();

    void InsertHeadSegmentLoad();
    void InsertInstrInLandingPad(IR::Instr *const instr, Loop *const hoistOutOfLoop);
    void ShareBailOut();

    Func * func;
    GlobOpt * globOpt;
    IR::Instr *& instr;
    Value *& src1Val;
    Value *& src2Val;

    IR::Instr * baseOwnerInstr = nullptr;
    IR::IndirOpnd * baseOwnerIndir = nullptr;
    IR::RegOpnd * baseOpnd = nullptr;
    IR::Opnd * indexOpnd = nullptr;
    IR::Opnd * originalIndexOpnd = nullptr;
    bool isProfilableLdElem = false;
    bool isProfilableStElem = false;
    bool isLoad = false;
    bool isStore = false;
    bool needsHeadSegment = false;
    bool needsHeadSegmentLength = false;
    bool needsLength = false;
    bool needsBoundChecks = false;

    Value * baseValue = nullptr;
    ValueInfo * baseValueInfo = nullptr;
    ValueType baseValueType;
    ValueType newBaseValueType;
    ArrayValueInfo * baseArrayValueInfo = nullptr;

    bool isLikelyJsArray = false;
    bool isLikelyVirtualTypedArray = false;
    bool doArrayChecks = false;
    bool doArraySegmentHoist = false;
    bool headSegmentIsAvailable = false;
    bool doHeadSegmentLoad = false;
    bool doArraySegmentLengthHoist = false;
    bool headSegmentLengthIsAvailable = false;
    bool doHeadSegmentLengthLoad = false;
    bool lengthIsAvailable = false;
    bool doLengthLoad = false;

    StackSym * newHeadSegmentSym = nullptr;
    StackSym * newHeadSegmentLengthSym = nullptr;
    StackSym * newLengthSym = nullptr;

    bool canBailOutOnArrayAccessHelperCall = false;

    bool doExtractBoundChecks = false;
    bool eliminatedLowerBoundCheck = false;
    bool eliminatedUpperBoundCheck = false;
    StackSym * indexVarSym = nullptr;
    Value * indexValue = nullptr;
    IntConstantBounds indexConstantBounds;
    Value * headSegmentLengthValue = nullptr;
    IntConstantBounds headSegmentLengthConstantBounds;

    Loop * hoistChecksOutOfLoop = nullptr;
    Loop * hoistHeadSegmentLoadOutOfLoop = nullptr;
    Loop * hoistHeadSegmentLengthLoadOutOfLoop = nullptr;
    Loop * hoistLengthLoadOutOfLoop = nullptr;

    GlobOpt::ArrayLowerBoundCheckHoistInfo lowerBoundCheckHoistInfo;
    GlobOpt::ArrayUpperBoundCheckHoistInfo upperBoundCheckHoistInfo;

    bool failedToUpdateCompatibleLowerBoundCheck = false;
    bool failedToUpdateCompatibleUpperBoundCheck = false;

    StackSym * headSegmentLengthSym = nullptr;

    IR::Instr * insertBeforeInstr = nullptr;
    BailOutInfo * shareableBailOutInfo = nullptr;
    IR::Instr * shareableBailOutInfoOriginalOwner = nullptr;
};
