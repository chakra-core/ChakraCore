//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class Func;

struct CodeGenWorkItem : public JsUtil::Job
{
protected:
    CodeGenWorkItem(
        JsUtil::JobManager *const manager,
        Js::FunctionBody *const functionBody,
        Js::EntryPointInfo* entryPointInfo,
        bool isJitInDebugMode,
        CodeGenWorkItemType type);
    ~CodeGenWorkItem();

    CodeGenWorkItemIDL jitData;

    Js::FunctionBody *const functionBody;
    size_t codeAddress;
    ptrdiff_t codeSize;

public:
    HRESULT codeGenResult;

public:
    virtual uint GetByteCodeCount() const = 0;
    virtual size_t GetDisplayName(_Out_writes_opt_z_(sizeInChars) WCHAR* displayName, _In_ size_t sizeInChars) = 0;
    virtual void GetEntryPointAddress(void** entrypoint, ptrdiff_t *size) = 0;
    virtual uint GetInterpretedCount() const = 0;
    virtual void Delete() = 0;
#if DBG_DUMP | defined(VTUNE_PROFILING)
    virtual void RecordNativeMap(uint32 nativeOffset, uint32 statementIndex) = 0;
#endif
#if DBG_DUMP
    virtual void DumpNativeOffsetMaps() = 0;
    virtual void DumpNativeThrowSpanSequence() = 0;
#endif

    uint GetFunctionNumber() const
    {
        return this->jitData.jitData->bodyData->funcNumber;
    }

    ExecutionMode GetJitMode() const
    {
        return static_cast<ExecutionMode>(this->jitData.jitMode);
    }

    CodeGenWorkItemIDL * GetJITData()
    {
        return &this->jitData;
    }

    CodeGenWorkItemType Type() const { return static_cast<CodeGenWorkItemType>(this->jitData.type); }

    Js::ScriptContext* GetScriptContext()
    {
        return functionBody->GetScriptContext();
    }

    Js::FunctionBody* GetFunctionBody() const
    {
        return functionBody;
    }

    void SetCodeAddress(size_t codeAddress) { this->codeAddress = codeAddress; }
    size_t GetCodeAddress() { return codeAddress; }

    void SetCodeSize(ptrdiff_t codeSize) { this->codeSize = codeSize; }
    ptrdiff_t GetCodeSize() { return codeSize; }

protected:
    virtual uint GetLoopNumber() const
    {
        return Js::LoopHeader::NoLoop;
    }

protected:
    // This reference does not keep the entry point alive, and it's not expected to
    // The entry point is kept alive only if it's in the JIT queue, in which case recyclableData will be allocated and will keep the entry point alive
    // If the entry point is getting collected, it'll actually remove itself from the work item list so this work item will get deleted when the EntryPointInfo goes away
    Js::EntryPointInfo* entryPointInfo;
    Js::CodeGenRecyclableData *recyclableData;

private:
    bool isInJitQueue;                  // indicates if the work item has been added to the global jit queue
    bool isAllocationCommitted;         // Whether the EmitBuffer allocation has been committed

    QueuedFullJitWorkItem *queuedFullJitWorkItem;
    EmitBufferAllocation *allocation;

#ifdef IR_VIEWER
public:
    bool isRejitIRViewerFunction;               // re-JIT function for IRViewer object generation
    Js::DynamicObject *irViewerOutput;          // hold results of IRViewer APIs
    Js::ScriptContext *irViewerRequestContext;  // keep track of the request context

    Js::DynamicObject * GetIRViewerOutput(Js::ScriptContext *scriptContext)
    {
        if (!irViewerOutput)
        {
            irViewerOutput = scriptContext->GetLibrary()->CreateObject();
        }

        return irViewerOutput;
    }

    void SetIRViewerOutput(Js::DynamicObject *output)
    {
        irViewerOutput = output;
    }
#endif
private:
    EmitBufferAllocation *GetAllocation() { return allocation; }

public:
    Js::EntryPointInfo* GetEntryPoint() const
    {
        return this->entryPointInfo;
    }

    Js::CodeGenRecyclableData *RecyclableData() const
    {
        return recyclableData;
    }

    void SetRecyclableData(Js::CodeGenRecyclableData *const recyclableData)
    {
        Assert(recyclableData);
        Assert(!this->recyclableData);

        this->recyclableData = recyclableData;
    }

    void SetEntryPointInfo(Js::EntryPointInfo* entryPointInfo)
    {
        this->entryPointInfo = entryPointInfo;
    }

public:
    void ResetJitMode()
    {
        this->jitData.jitMode = static_cast<uint8>(ExecutionMode::Interpreter);
    }

    void SetJitMode(const ExecutionMode jitMode)
    {
        this->jitData.jitMode = static_cast<uint8>(jitMode);
        VerifyJitMode();
    }

    void VerifyJitMode() const
    {
        Assert(GetJitMode() == ExecutionMode::SimpleJit || GetJitMode() == ExecutionMode::FullJit);
        Assert(GetJitMode() != ExecutionMode::SimpleJit || GetFunctionBody()->DoSimpleJit());
        Assert(GetJitMode() != ExecutionMode::FullJit || !PHASE_OFF(Js::FullJitPhase, GetFunctionBody()));
    }

    void OnAddToJitQueue();
    void OnRemoveFromJitQueue(NativeCodeGenerator* generator);

public:
    bool ShouldSpeculativelyJit(uint byteCodeSizeGenerated) const;
private:
    bool ShouldSpeculativelyJitBasedOnProfile() const;

public:
    bool IsInJitQueue() const
    {
        return isInJitQueue;
    }

    bool IsJitInDebugMode() const
    {
        return jitData.isJitInDebugMode != 0;
    }

    void OnWorkItemProcessFail(NativeCodeGenerator *codeGen);

    void RecordNativeThrowMap(Js::SmallSpanSequenceIter& iter, uint32 nativeOffset, uint32 statementIndex)
    {
        this->functionBody->RecordNativeThrowMap(iter, nativeOffset, statementIndex, this->GetEntryPoint(), GetLoopNumber());
    }

    QueuedFullJitWorkItem *GetQueuedFullJitWorkItem() const;
    QueuedFullJitWorkItem *EnsureQueuedFullJitWorkItem();

private:
    bool ShouldSpeculativelyJit() const;
};

struct JsFunctionCodeGen sealed : public CodeGenWorkItem
{
    JsFunctionCodeGen(
        JsUtil::JobManager *const manager,
        Js::FunctionBody *const functionBody,
        Js::EntryPointInfo* entryPointInfo,
        bool isJitInDebugMode)
        : CodeGenWorkItem(manager, functionBody, entryPointInfo, isJitInDebugMode, JsFunctionType)
    {
        this->jitData.loopNumber = GetLoopNumber();
    }

public:
    uint GetByteCodeCount() const override
    {
        return functionBody->GetByteCodeCount() +  functionBody->GetConstantCount();
    }

    size_t GetDisplayName(_Out_writes_opt_z_(sizeInChars) WCHAR* displayName, _In_ size_t sizeInChars) override
    {
        const WCHAR* name = functionBody->GetExternalDisplayName();
        size_t nameSizeInChars = wcslen(name) + 1;
        size_t sizeInBytes = nameSizeInChars * sizeof(WCHAR);
        if(displayName == NULL || sizeInChars < nameSizeInChars)
        {
           return nameSizeInChars;
        }
        js_memcpy_s(displayName, sizeInChars * sizeof(WCHAR), name, sizeInBytes);
        return nameSizeInChars;
    }

    void GetEntryPointAddress(void** entrypoint, ptrdiff_t *size) override
    {
         Assert(entrypoint);
         *entrypoint = this->GetEntryPoint()->jsMethod;
         *size = this->GetEntryPoint()->GetCodeSize();
    }

    uint GetInterpretedCount() const override
    {
        return this->functionBody->GetInterpretedCount();
    }

    void Delete() override
    {
        HeapDelete(this);
    }

#if DBG_DUMP | defined(VTUNE_PROFILING)
    void RecordNativeMap(uint32 nativeOffset, uint32 statementIndex) override
    {
        Js::FunctionEntryPointInfo* info = (Js::FunctionEntryPointInfo*) this->GetEntryPoint();

        info->RecordNativeMap(nativeOffset, statementIndex);
    }
#endif

#if DBG_DUMP
    virtual void DumpNativeOffsetMaps() override
    {
        this->GetEntryPoint()->DumpNativeOffsetMaps();
    }

    virtual void DumpNativeThrowSpanSequence() override
    {
        this->GetEntryPoint()->DumpNativeThrowSpanSequence();
    }
#endif
};

struct JsLoopBodyCodeGen sealed : public CodeGenWorkItem
{
    JsLoopBodyCodeGen(
        JsUtil::JobManager *const manager, Js::FunctionBody *const functionBody,
        Js::EntryPointInfo* entryPointInfo, bool isJitInDebugMode, Js::LoopHeader * loopHeader) :
        CodeGenWorkItem(manager, functionBody, entryPointInfo, isJitInDebugMode, JsLoopBodyWorkItemType),
        loopHeader(loopHeader)
    {
        this->jitData.loopNumber = GetLoopNumber();
    }

    Js::LoopHeader * loopHeader;

    uint GetLoopNumber() const override
    {
        return functionBody->GetLoopNumberWithLock(loopHeader);
    }

    uint GetByteCodeCount() const override
    {
        return (loopHeader->endOffset - loopHeader->startOffset) + functionBody->GetConstantCount();
    }

    size_t GetDisplayName(_Out_writes_opt_z_(sizeInChars) WCHAR* displayName, _In_ size_t sizeInChars) override
    {
        return this->functionBody->GetLoopBodyName(this->GetLoopNumber(), displayName, sizeInChars);
    }

    void GetEntryPointAddress(void** entrypoint, ptrdiff_t *size) override
    {
        Assert(entrypoint);
        Js::EntryPointInfo * entryPoint = this->GetEntryPoint();
        *entrypoint = reinterpret_cast<void*>(entryPoint->jsMethod);
        *size = entryPoint->GetCodeSize();
    }

    uint GetInterpretedCount() const override
    {
        return loopHeader->interpretCount;
    }

#if DBG_DUMP | defined(VTUNE_PROFILING)
    void RecordNativeMap(uint32 nativeOffset, uint32 statementIndex) override
    {
        this->GetEntryPoint()->RecordNativeMap(nativeOffset, statementIndex);
    }
#endif

#if DBG_DUMP
    virtual void DumpNativeOffsetMaps() override
    {
        this->GetEntryPoint()->DumpNativeOffsetMaps();
    }

    virtual void DumpNativeThrowSpanSequence() override
    {
        this->GetEntryPoint()->DumpNativeThrowSpanSequence();
    }
#endif

    void Delete() override
    {
        HeapDelete(this);
    }

    ~JsLoopBodyCodeGen()
    {
        if (this->jitData.symIdToValueTypeMap != nullptr)
        {
            HeapDeleteArray(this->jitData.symIdToValueTypeMapCount, this->jitData.symIdToValueTypeMap);
            this->jitData.symIdToValueTypeMap = nullptr;
        }
    }
};
