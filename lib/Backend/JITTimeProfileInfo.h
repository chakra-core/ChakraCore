//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITTimeProfileInfo
{
public:
    JITTimeProfileInfo(ProfileDataIDL * profileData);

    static void InitializeJITProfileData(
        __in ArenaAllocator * alloc,
        __in Js::DynamicProfileInfo * profileInfo,
        __in Js::FunctionBody *functionBody,
        __out ProfileDataIDL * data,
        bool isForegroundJIT);

    const Js::LdElemInfo * GetLdElemInfo(Js::ProfileId ldElemId) const;
    const Js::StElemInfo * GetStElemInfo(Js::ProfileId stElemId) const;

    Js::ArrayCallSiteInfo * GetArrayCallSiteInfo(Js::ProfileId index) const;
    intptr_t GetArrayCallSiteInfoAddr(Js::ProfileId index) const;
    Js::FldInfo * GetFldInfo(uint fieldAccessId) const;
    intptr_t GetFldInfoAddr(uint fieldAccessId) const;
    Js::ThisInfo GetThisInfo() const;
    ValueType GetSlotLoad(Js::ProfileId slotLoadId) const;
    ValueType GetReturnType(Js::OpCode opcode, Js::ProfileId callSiteId) const;
    ValueType GetDivProfileInfo(Js::ProfileId divideId) const;
    ValueType GetSwitchProfileInfo(Js::ProfileId switchId) const;
    ValueType GetParameterInfo(Js::ArgSlot index) const;
    Js::ImplicitCallFlags GetLoopImplicitCallFlags(uint loopNum) const;
    Js::ImplicitCallFlags GetImplicitCallFlags() const;
    Js::LoopFlags GetLoopFlags(uint loopNum) const;

    uint GetLoopCount() const;

    uint16 GetConstantArgInfo(Js::ProfileId callSiteId) const;

    bool IsModulusOpByPowerOf2(Js::ProfileId profileId) const;
    bool IsAggressiveIntTypeSpecDisabled(const bool isJitLoopBody) const;
    bool IsSwitchOptDisabled() const;
    bool IsEquivalentObjTypeSpecDisabled() const;
    bool IsObjTypeSpecDisabledInJitLoopBody() const;
    bool IsAggressiveMulIntTypeSpecDisabled(const bool isJitLoopBody) const;
    bool IsDivIntTypeSpecDisabled(const bool isJitLoopBody) const;
    bool IsLossyIntTypeSpecDisabled() const;
    bool IsMemOpDisabled() const;
    bool IsTrackCompoundedIntOverflowDisabled() const;
    bool IsFloatTypeSpecDisabled() const;
    bool IsCheckThisDisabled() const;
    bool IsArrayCheckHoistDisabled(const bool isJitLoopBody) const;
    bool IsArrayMissingValueCheckHoistDisabled(const bool isJitLoopBody) const;
    bool IsJsArraySegmentHoistDisabled(const bool isJitLoopBody) const;
    bool IsArrayLengthHoistDisabled(const bool isJitLoopBody) const;
    bool IsTypedArrayTypeSpecDisabled(const bool isJitLoopBody) const;
    bool IsLdLenIntSpecDisabled() const;
    bool IsBoundCheckHoistDisabled(const bool isJitLoopBody) const;
    bool IsLoopCountBasedBoundCheckHoistDisabled(const bool isJitLoopBody) const;
    bool IsFloorInliningDisabled() const;
    bool IsNoProfileBailoutsDisabled() const;
    bool HasLdFldCallSiteInfo() const;
    bool IsStackArgOptDisabled() const;
    bool IsLoopImplicitCallInfoDisabled() const;
    bool IsPowIntIntTypeSpecDisabled() const;
    bool IsTagCheckDisabled() const;
    bool IsOptimizeTryFinallyDisabled() const;

private:
    enum ProfileDataFlags : int64
    {
        Flags_None = 0,
        Flags_disableAggressiveIntTypeSpec = 1,
        Flags_disableAggressiveIntTypeSpec_jitLoopBody = 1 << 1,
        Flags_disableAggressiveMulIntTypeSpec = 1 << 2,
        Flags_disableAggressiveMulIntTypeSpec_jitLoopBody = 1 << 3,
        Flags_disableDivIntTypeSpec = 1 << 4,
        Flags_disableDivIntTypeSpec_jitLoopBody = 1 << 5,
        Flags_disableLossyIntTypeSpec = 1 << 6,
        Flags_disableTrackCompoundedIntOverflow = 1 << 7,
        Flags_disableFloatTypeSpec = 1 << 8,
        Flags_disableArrayCheckHoist = 1 << 9,
        Flags_disableArrayCheckHoist_jitLoopBody = 1 << 10,
        Flags_disableArrayMissingValueCheckHoist = 1 << 11,
        Flags_disableArrayMissingValueCheckHoist_jitLoopBody = 1 << 12,
        Flags_disableJsArraySegmentHoist = 1 << 13,
        Flags_disableJsArraySegmentHoist_jitLoopBody = 1 << 14,
        Flags_disableArrayLengthHoist = 1 << 15,
        Flags_disableArrayLengthHoist_jitLoopBody = 1 << 16,
        Flags_disableTypedArrayTypeSpec = 1 << 17,
        Flags_disableTypedArrayTypeSpec_jitLoopBody = 1 << 18,
        Flags_disableLdLenIntSpec = 1 << 19,
        Flags_disableBoundCheckHoist = 1 << 20,
        Flags_disableBoundCheckHoist_jitLoopBody = 1 << 21,
        Flags_disableLoopCountBasedBoundCheckHoist = 1 << 22,
        Flags_disableLoopCountBasedBoundCheckHoist_jitLoopBody = 1 << 23,
        Flags_disableFloorInlining = 1 << 24,
        Flags_disableNoProfileBailouts = 1 << 25,
        Flags_disableSwitchOpt = 1 << 26,
        Flags_disableEquivalentObjTypeSpec = 1 << 27,
        Flags_disableObjTypeSpec_jitLoopBody = 1 << 28,
        Flags_disableMemOp = 1 << 29,
        Flags_disableCheckThis = 1 << 30,
        Flags_hasLdFldCallSiteInfo = 1ll << 31,
        Flags_disableStackArgOpt = 1ll << 32,
        Flags_disableLoopImplicitCallInfo = 1ll << 33,
        Flags_disablePowIntIntTypeSpec = 1ll << 34,
        Flags_disableTagCheck = 1ll << 35,
        Flags_disableOptimizeTryFinally = 1ll << 36
    };

    Js::ProfileId GetProfiledArrayCallSiteCount() const;
    Js::ProfileId GetProfiledCallSiteCount() const;
    Js::ProfileId GetProfiledReturnTypeCount() const;
    Js::ProfileId GetProfiledDivOrRemCount() const;
    Js::ProfileId GetProfiledSwitchCount() const;
    Js::ProfileId GetProfiledSlotCount() const;
    Js::ArgSlot GetProfiledInParamsCount() const;
    uint GetProfiledFldCount() const;
    BVFixed * GetLoopFlags() const;

    bool TestFlag(ProfileDataFlags flag) const;

    Js::CallSiteInfo * GetCallSiteInfo() const;

    ProfileDataIDL m_profileData;
};
