//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// forward decls
class AsmJsJITInfo;
class JITTimeProfileInfo;
class FunctionJITRuntimeInfo;
class JITRecyclableObject;

class JITTimeFunctionBody
{
public:
    JITTimeFunctionBody(FunctionBodyDataIDL * bodyData);

    static void InitializeJITFunctionData(
        __in ArenaAllocator * arena,
        __in Js::FunctionBody * functionBody,
        __out FunctionBodyDataIDL * jitBody);

    intptr_t GetAddr() const;

    uint GetFunctionNumber() const;
    uint GetSourceContextId() const;
    uint GetNestedCount() const;
    uint GetScopeSlotArraySize() const;
    uint GetParamScopeSlotArraySize() const;
    uint GetByteCodeCount() const;
    uint GetByteCodeInLoopCount() const;
    uint GetNonLoadByteCodeCount() const;
    uint GetLoopCount() const;
    uint GetByteCodeLength() const;
    uint GetInnerScopeCount() const;
    uint GetInlineCacheCount() const;
    uint GetRecursiveCallSiteCount() const;
    uint GetForInLoopDepth() const;
    Js::RegSlot GetLocalFrameDisplayReg() const;
    Js::RegSlot GetLocalClosureReg() const;
    Js::RegSlot GetEnvReg() const;
    Js::RegSlot GetFirstTmpReg() const;
    Js::RegSlot GetFirstInnerScopeReg() const;
    Js::RegSlot GetVarCount() const;
    Js::RegSlot GetConstCount() const;
    Js::RegSlot GetLocalsCount() const;
    Js::RegSlot GetTempCount() const;
    Js::RegSlot GetFuncExprScopeReg() const;
    Js::RegSlot GetThisRegForEventHandler() const;
    Js::RegSlot GetFirstNonTempLocalIndex() const;
    Js::RegSlot GetEndNonTempLocalIndex() const;
    Js::RegSlot GetNonTempLocalVarCount() const;
    Js::RegSlot GetRestParamRegSlot() const;
    Js::RegSlot GetParamClosureReg() const;

    Js::PropertyId GetPropertyIdFromCacheId(uint cacheId) const;
    Js::PropertyId GetReferencedPropertyId(uint index) const;

    uint16 GetEnvDepth() const;
    uint16 GetArgUsedForBranch() const;
    Js::ProfileId GetProfiledCallSiteCount() const;
    Js::ArgSlot GetInParamsCount() const;

    bool DoStackNestedFunc() const;
    bool DoStackClosure() const;
    bool DoBackendArgumentsOptimization() const;
    bool IsLibraryCode() const;
    bool HasTry() const;
    bool HasThis() const;
    bool HasFinally() const;
    bool HasOrParentHasArguments() const;
    bool IsGenerator() const;
    bool IsCoroutine() const;
    bool IsLambda() const;
    bool IsAsmJsMode() const;
    bool IsWasmFunction() const;
    bool IsStrictMode() const;
    bool IsEval() const;
    bool HasImplicitArgIns() const;
    bool HasRestParameter() const;
    bool HasScopeObject() const;
    bool HasCachedScopePropIds() const;
    bool HasInlineCachesOnFunctionObject() const;
    bool DoInterruptProbe() const;
    bool IsGlobalFunc() const;
    void DisableInlineApply();
    bool IsInlineApplyDisabled() const;
    bool IsNonTempLocalVar(uint32 varIndex) const;
    bool DoJITLoopBody() const;
    bool IsInlineSpreadDisabled() const;
    void DisableInlineSpread();
    bool HasLoops() const;
    bool HasNonBuiltInCallee() const;
    bool HasNestedLoop() const;
    bool UsesArgumentsObject() const;
    bool IsParamAndBodyScopeMerged() const;
    bool CanInlineRecursively(uint depth, bool tryAggressive = true) const;
    bool NeedScopeObjectForArguments(bool hasNonSimpleParams) const;
    bool GetDoScopeObjectCreation() const;
    void EnsureConsistentConstCount() const;

    const byte * GetByteCodeBuffer() const;
    StatementMapIDL * GetFullStatementMap() const;
    uint GetFullStatementMapCount() const;
    void * ReadFromAuxData(uint offset) const;
    void * ReadFromAuxContextData(uint offset) const;
    Js::FunctionInfoPtrPtr GetNestedFuncRef(uint index) const;
    intptr_t GetConstantVar(Js::RegSlot location) const;
    JITRecyclableObject * GetConstantContent(Js::RegSlot location) const;

    template<class T>
    T* GetConstAsT(Js::RegSlot location) const
    {
        Assert(m_bodyData.constTableContent != nullptr);
        Assert(m_bodyData.constTableContent->content != nullptr);
        Assert(location < GetConstCount());
        Assert(location != 0);

        auto obj = m_bodyData.constTableContent->content[location - Js::FunctionBody::FirstRegSlot];
        Assert(obj);
        obj->vtbl = VirtualTableInfo<T>::Address;
        //Assert(T::Is(obj));
        return (T*)obj;
    }

    template<>
    Js::JavascriptNumber* GetConstAsT<Js::JavascriptNumber>(Js::RegSlot location) const
    {
        Assert(m_bodyData.constTableContent != nullptr);
        Assert(m_bodyData.constTableContent->content != nullptr);
        Assert(location < GetConstCount());
        Assert(location != 0);

#if !FLOATVAR
        auto obj = m_bodyData.constTableContent->content[location - Js::FunctionBody::FirstRegSlot];
        if (!obj)
        {
#endif
            Js::JavascriptNumber* num = (Js::JavascriptNumber*)GetConstantVar(location);
            Assert(Js::TaggedNumber::Is(num));
            return num;
#if !FLOATVAR
        }
        Assert(obj);
        obj->vtbl = VirtualTableInfo<Js::JavascriptNumber>::Address;
        Assert(Js::JavascriptNumber::Is(obj));
        return (Js::JavascriptNumber*)obj;
#endif
    }

    intptr_t GetInlineCache(uint index) const;
    intptr_t GetIsInstInlineCache(uint index) const;
    Js::TypeId GetConstantType(Js::RegSlot location) const;
    void * GetConstTable() const;
    bool IsConstRegPropertyString(Js::RegSlot reg, ScriptContextInfo * context) const;

    intptr_t GetRootObject() const;
    intptr_t GetLoopHeaderAddr(uint loopNum) const;
    const JITLoopHeaderIDL * GetLoopHeaderData(uint loopNum) const;

    intptr_t GetScriptIdAddr() const;
    intptr_t GetProbeCountAddr() const;
    intptr_t GetFlagsAddr() const;
    intptr_t GetRegAllocLoadCountAddr() const;
    intptr_t GetRegAllocStoreCountAddr() const;
    intptr_t GetCallCountStatsAddr() const;
    intptr_t GetFormalsPropIdArrayAddr() const;
    intptr_t GetObjectLiteralTypeRef(uint index) const;
    intptr_t GetLiteralRegexAddr(uint index) const;
    uint GetNestedFuncIndexForSlotIdInCachedScope(uint index) const;
    const AsmJsJITInfo * GetAsmJsInfo() const;
    const JITTimeProfileInfo * GetReadOnlyProfileInfo() const;
    JITTimeProfileInfo * GetProfileInfo() const;
    bool HasProfileInfo() const;
    bool IsRegSlotFormal(Js::RegSlot reg) const;
    bool HasPropIdToFormalsMap() const;

    static bool LoopContains(const JITLoopHeaderIDL * loop1, const JITLoopHeaderIDL * loop2);

    char16* GetDisplayName() const;

    intptr_t GetAuxDataAddr(uint offset) const;
    const Js::PropertyIdArray * ReadPropertyIdArrayFromAuxData(uint offset) const;
    Js::PropertyIdArray * GetFormalsPropIdArray() const;

    Js::ForInCache * GetForInCache(uint profileId) const;
    bool InitializeStatementMap(Js::SmallSpanSequence * statementMap, ArenaAllocator* alloc) const;
private:
    Js::FunctionInfo::Attributes GetAttributes() const;
    Js::FunctionBody::FunctionBodyFlags GetFlags() const;

    FunctionBodyDataIDL m_bodyData;
};
