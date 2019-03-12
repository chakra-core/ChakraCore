//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#if ENABLE_OOP_NATIVE_CODEGEN
#include "JITServer/JITServer.h"

ServerScriptContext::ThreadContextHolder::ThreadContextHolder(ServerThreadContext* threadContextInfo) : threadContextInfo(threadContextInfo)
{
    threadContextInfo->AddRef();
}
ServerScriptContext::ThreadContextHolder::~ThreadContextHolder()
{
    threadContextInfo->Release();
}

ServerScriptContext::ServerScriptContext(ScriptContextDataIDL * contextData, ServerThreadContext* threadContextInfo) :
    m_contextData(*contextData),
    threadContextHolder(threadContextInfo),
    m_isPRNGSeeded(false),
    m_sourceCodeArena(_u("JITSourceCodeArena"), threadContextInfo->GetForegroundPageAllocator(), Js::Throw::OutOfMemory, nullptr),
    m_interpreterThunkBufferManager(&m_sourceCodeArena, threadContextInfo->GetThunkPageAllocators(), nullptr, threadContextInfo, _u("Interpreter thunk buffer"), GetThreadContext()->GetProcessHandle()),
    m_asmJsInterpreterThunkBufferManager(&m_sourceCodeArena, threadContextInfo->GetThunkPageAllocators(), nullptr, threadContextInfo, _u("Asm.js interpreter thunk buffer"), GetThreadContext()->GetProcessHandle()),
    m_domFastPathHelperMap(nullptr),
    m_moduleRecords(&HeapAllocator::Instance),
    m_codeGenAlloc(nullptr, nullptr, threadContextInfo, threadContextInfo->GetCodePageAllocators(), threadContextInfo->GetProcessHandle()),
    m_globalThisAddr(0),
#ifdef PROFILE_EXEC
    codeGenProfiler(nullptr),
#endif
    m_refCount(0),
    m_isClosed(false)
{

#if !TARGET_64 && _CONTROL_FLOW_GUARD
    m_codeGenAlloc.canCreatePreReservedSegment = threadContextInfo->CanCreatePreReservedSegment();
#endif

    m_domFastPathHelperMap = HeapNew(JITDOMFastPathHelperMap, &HeapAllocator::Instance, 17);
}

ServerScriptContext::~ServerScriptContext()
{
    HeapDelete(m_domFastPathHelperMap);
    m_moduleRecords.Map([](uint, Js::ServerSourceTextModuleRecord* record)
    {
        HeapDelete(record);
    });

#ifdef PROFILE_EXEC
    while (this->codeGenProfiler)
    {
        Js::ScriptContextProfiler* profiler = this->codeGenProfiler;
        this->codeGenProfiler = this->codeGenProfiler->next;
        HeapDelete(profiler);
    }
#endif
}

intptr_t
ServerScriptContext::GetNullAddr() const
{
    return m_contextData.nullAddr;
}

intptr_t
ServerScriptContext::GetUndefinedAddr() const
{
    return m_contextData.undefinedAddr;
}

intptr_t
ServerScriptContext::GetTrueAddr() const
{
    return m_contextData.trueAddr;
}

intptr_t
ServerScriptContext::GetFalseAddr() const
{
    return m_contextData.falseAddr;
}

intptr_t
ServerScriptContext::GetUndeclBlockVarAddr() const
{
    return m_contextData.undeclBlockVarAddr;
}

intptr_t
ServerScriptContext::GetEmptyStringAddr() const
{
    return m_contextData.emptyStringAddr;
}

intptr_t
ServerScriptContext::GetNegativeZeroAddr() const
{
    return m_contextData.negativeZeroAddr;
}

intptr_t
ServerScriptContext::GetNumberTypeStaticAddr() const
{
    return m_contextData.numberTypeStaticAddr;
}

intptr_t
ServerScriptContext::GetStringTypeStaticAddr() const
{
    return m_contextData.stringTypeStaticAddr;
}

intptr_t
ServerScriptContext::GetSymbolTypeStaticAddr() const
{
    return m_contextData.symbolTypeStaticAddr;
}

intptr_t
ServerScriptContext::GetObjectTypeAddr() const
{
    return m_contextData.objectTypeAddr;
}

intptr_t
ServerScriptContext::GetObjectHeaderInlinedTypeAddr() const
{
    return m_contextData.objectHeaderInlinedTypeAddr;
}

intptr_t
ServerScriptContext::GetRegexTypeAddr() const
{
    return m_contextData.regexTypeAddr;
}

intptr_t
ServerScriptContext::GetArrayTypeAddr() const
{
    return m_contextData.arrayTypeAddr;
}

intptr_t
ServerScriptContext::GetNativeIntArrayTypeAddr() const
{
    return m_contextData.nativeIntArrayTypeAddr;
}

intptr_t
ServerScriptContext::GetNativeFloatArrayTypeAddr() const
{
    return m_contextData.nativeFloatArrayTypeAddr;
}

intptr_t
ServerScriptContext::GetArrayConstructorAddr() const
{
    return m_contextData.arrayConstructorAddr;
}

intptr_t
ServerScriptContext::GetCharStringCacheAddr() const
{
    return m_contextData.charStringCacheAddr;
}

intptr_t
ServerScriptContext::GetSideEffectsAddr() const
{
    return m_contextData.sideEffectsAddr;
}

intptr_t
ServerScriptContext::GetArraySetElementFastPathVtableAddr() const
{
    return m_contextData.arraySetElementFastPathVtableAddr;
}

intptr_t
ServerScriptContext::GetIntArraySetElementFastPathVtableAddr() const
{
    return m_contextData.intArraySetElementFastPathVtableAddr;
}

intptr_t
ServerScriptContext::GetFloatArraySetElementFastPathVtableAddr() const
{
    return m_contextData.floatArraySetElementFastPathVtableAddr;
}

intptr_t
ServerScriptContext::GetLibraryAddr() const
{
    return m_contextData.libraryAddr;
}

intptr_t
ServerScriptContext::GetGlobalObjectAddr() const
{
    return m_contextData.globalObjectAddr;
}

intptr_t
ServerScriptContext::GetGlobalObjectThisAddr() const
{
    return m_globalThisAddr;
}

void
ServerScriptContext::UpdateGlobalObjectThisAddr(intptr_t globalThis)
{
    m_globalThisAddr = globalThis;
}

intptr_t
ServerScriptContext::GetNumberAllocatorAddr() const
{
    return m_contextData.numberAllocatorAddr;
}

intptr_t
ServerScriptContext::GetRecyclerAddr() const
{
    return m_contextData.recyclerAddr;
}

bool
ServerScriptContext::GetRecyclerAllowNativeCodeBumpAllocation() const
{
    return m_contextData.recyclerAllowNativeCodeBumpAllocation != 0;
}

intptr_t
ServerScriptContext::GetBuiltinFunctionsBaseAddr() const
{
    return m_contextData.builtinFunctionsBaseAddr;
}

intptr_t
ServerScriptContext::GetAddr() const
{
    return m_contextData.scriptContextAddr;
}

intptr_t
ServerScriptContext::GetVTableAddress(VTableValue vtableType) const
{
    Assert(vtableType < VTableValue::Count);
    return m_contextData.vtableAddresses[vtableType];
}

bool
ServerScriptContext::IsRecyclerVerifyEnabled() const
{
    return m_contextData.isRecyclerVerifyEnabled != FALSE;
}

uint
ServerScriptContext::GetRecyclerVerifyPad() const
{
    return m_contextData.recyclerVerifyPad;
}

bool
ServerScriptContext::IsPRNGSeeded() const
{
    return m_isPRNGSeeded;
}

#ifdef ENABLE_SCRIPT_DEBUGGING
intptr_t
ServerScriptContext::GetDebuggingFlagsAddr() const
{
    return static_cast<intptr_t>(m_contextData.debuggingFlagsAddr);
}

intptr_t
ServerScriptContext::GetDebugStepTypeAddr() const
{
    return static_cast<intptr_t>(m_contextData.debugStepTypeAddr);
}

intptr_t
ServerScriptContext::GetDebugFrameAddressAddr() const
{
    return static_cast<intptr_t>(m_contextData.debugFrameAddressAddr);
}

intptr_t
ServerScriptContext::GetDebugScriptIdWhenSetAddr() const
{
    return static_cast<intptr_t>(m_contextData.debugScriptIdWhenSetAddr);
}
#endif

intptr_t
ServerScriptContext::GetChakraLibAddr() const
{
    return m_contextData.chakraLibAddr;
}

bool
ServerScriptContext::IsClosed() const
{
    return m_isClosed;
}

void
ServerScriptContext::AddToDOMFastPathHelperMap(intptr_t funcInfoAddr, IR::JnHelperMethod helper)
{
    AutoCriticalSection cs(&m_cs);
    m_domFastPathHelperMap->Add(funcInfoAddr, helper);
}

ArenaAllocator *
ServerScriptContext::GetSourceCodeArena()
{
    return &m_sourceCodeArena;
}

void
ServerScriptContext::DecommitEmitBufferManager(bool asmJsManager)
{
    GetEmitBufferManager(asmJsManager)->Decommit();
}

OOPEmitBufferManagerWithLock *
ServerScriptContext::GetEmitBufferManager(bool asmJsManager)
{
    if (asmJsManager)
    {
        return &m_asmJsInterpreterThunkBufferManager;
    }
    else
    {
        return &m_interpreterThunkBufferManager;
    }
}

IR::JnHelperMethod
ServerScriptContext::GetDOMFastPathHelper(intptr_t funcInfoAddr)
{
    AutoCriticalSection cs(&m_cs);

    IR::JnHelperMethod helper = IR::HelperInvalid;

    m_domFastPathHelperMap->TryGetValue(funcInfoAddr, &helper);

    return helper;
}

void
ServerScriptContext::Close()
{
    Assert(!IsClosed());
    m_isClosed = true;
    
    m_codeGenAlloc.emitBufferManager.Decommit();

#ifdef STACK_BACK_TRACE
    ServerContextManager::RecordCloseContext(this);
#endif
}

void
ServerScriptContext::AddRef()
{
    InterlockedExchangeAdd(&m_refCount, 1u);
}

void
ServerScriptContext::Release()
{
    InterlockedExchangeSubtract(&m_refCount, 1u);
    if (m_isClosed && m_refCount == 0)
    {
        // Not freeing here, we'll expect explicit ServerCleanupScriptContext() call to do the free
        // otherwise after free, the CodeGen call can still get same scriptContext if there's another 
        // ServerInitializeScriptContext call
    }
}

OOPCodeGenAllocators *
ServerScriptContext::GetCodeGenAllocators()
{
    return &m_codeGenAlloc;
}

Field(Js::Var)*
ServerScriptContext::GetModuleExportSlotArrayAddress(uint moduleIndex, uint slotIndex)
{
    AutoCriticalSection cs(&m_cs);
    AssertOrFailFast(m_moduleRecords.ContainsKey(moduleIndex));
    auto record = m_moduleRecords.Item(moduleIndex);
    return record->localExportSlotsAddr;
}

void
ServerScriptContext::SetIsPRNGSeeded(bool value)
{
    m_isPRNGSeeded = value;
}

void
ServerScriptContext::AddModuleRecordInfo(unsigned int moduleId, __int64 localExportSlotsAddr)
{
    AutoCriticalSection cs(&m_cs);
    Js::ServerSourceTextModuleRecord* record = HeapNewStructZ(Js::ServerSourceTextModuleRecord);
    record->moduleId = moduleId;
    record->localExportSlotsAddr = (Field(Js::Var)*)localExportSlotsAddr;
    m_moduleRecords.Add(moduleId, record);
}

#ifdef PROFILE_EXEC
Js::ScriptContextProfiler*
ServerScriptContext::GetCodeGenProfiler(_In_ PageAllocator* pageAllocator)
{
    if (Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag))
    {
        AutoCriticalSection cs(&this->profilerCS);

        Js::ScriptContextProfiler* profiler = this->codeGenProfiler;
        while (profiler)
        {
            if (profiler->pageAllocator == pageAllocator)
            {
                if (!profiler->IsInitialized())
                {
                    profiler->Initialize(pageAllocator, nullptr);
                }
                return profiler;
            }
            profiler = profiler->next;
        }

        // If we didn't find a profiler, allocate a new one

        profiler = HeapNew(Js::ScriptContextProfiler);
        profiler->Initialize(pageAllocator, nullptr);
        profiler->next = this->codeGenProfiler;

        this->codeGenProfiler = profiler;

        return profiler;
    }
    return nullptr;
}

Js::ScriptContextProfiler*
ServerScriptContext::GetFirstCodeGenProfiler() const
{
    return this->codeGenProfiler;
}
#endif // PROFILE_EXEC

#endif // ENABLE_OOP_NATIVE_CODEGEN
