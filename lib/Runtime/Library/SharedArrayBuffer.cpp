//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
#if DBG
    void SharedContents::AddAgent(DWORD_PTR agent)
    {
        AutoCriticalSection autoCS(&csAgent);
        if (allowedAgents == nullptr)
        {
            allowedAgents = HeapNew(SharableAgents, &HeapAllocator::Instance);
        }

        allowedAgents->Add(agent);
    }

    bool SharedContents::IsValidAgent(DWORD_PTR agent)
    {
        AutoCriticalSection autoCS(&csAgent);
        return allowedAgents != nullptr && allowedAgents->Contains(agent);
    }
#endif

    long SharedContents::AddRef()
    {
        return InterlockedIncrement(&refCount);
    }
    long SharedContents::Release()
    {
        long ret = InterlockedDecrement(&refCount);
        AssertOrFailFastMsg(ret >= 0, "Buffer already freed");
        return ret;
    }

    void SharedContents::Cleanup()
    {
        Assert(refCount == 0);
        buffer = nullptr;
        bufferLength = 0;
#if DBG
        {
            AutoCriticalSection autoCS(&csAgent);
            if (allowedAgents != nullptr)
            {
                HeapDelete(allowedAgents);
                allowedAgents = nullptr;
            }
        }
#endif

        if (indexToWaiterList != nullptr)
        {
            // TODO: the map should be empty here?
            // or we need to wake all the waiters from current context?
            indexToWaiterList->Map([](uint index, WaiterList *waiters)
            {
                if (waiters != nullptr)
                {
                    waiters->Cleanup();
                    HeapDelete(waiters);
                    waiters = nullptr;
                }
            });

            HeapDelete(indexToWaiterList);
            indexToWaiterList = nullptr;
        }
    }

    Var SharedArrayBuffer::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Var newTarget = args.GetNewTarget();
        bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);

        if (!(callInfo.Flags & CallFlags_New) || (newTarget && JavascriptOperators::IsUndefinedObject(newTarget)))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("SharedArrayBuffer"));
        }

        uint32 byteLength = 0;
        if (args.Info.Count > 1)
        {
            byteLength = ArrayBuffer::ToIndex(args[1], JSERR_ArrayLengthConstructIncorrect, scriptContext, MaxSharedArrayBufferLength);
        }

        RecyclableObject* newArr = scriptContext->GetLibrary()->CreateSharedArrayBuffer(byteLength);

        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), newArr, nullptr, scriptContext) :
            newArr;
    }

    // SharedArrayBuffer.prototype.byteLength
    Var SharedArrayBuffer::EntryGetterByteLength(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<SharedArrayBuffer>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedSharedArrayBufferObject);
        }

        SharedArrayBuffer* sharedArrayBuffer = VarTo<SharedArrayBuffer>(args[0]);
        return JavascriptNumber::ToVar(sharedArrayBuffer->GetByteLength(), scriptContext);
    }

    // SharedArrayBuffer.prototype.slice
    Var SharedArrayBuffer::EntrySlice(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Assert(!(callInfo.Flags & CallFlags_New));

        if (!VarIs<SharedArrayBuffer>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedSharedArrayBufferObject);
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        SharedArrayBuffer* currentBuffer = VarTo<SharedArrayBuffer>(args[0]);

        int64 currentLen = (int64)currentBuffer->GetByteLength();
        int64 start = 0, end = 0;
        int64 newLen = 0;

        // If no start or end arguments, use the entire length
        if (args.Info.Count < 2)
        {
            newLen = currentLen;
        }
        else
        {
            start = JavascriptArray::GetIndexFromVar(args[1], currentLen, scriptContext);

            // If no end argument, use length as the end
            if (args.Info.Count < 3 || args[2] == library->GetUndefined())
            {
                end = currentLen;
            }
            else
            {
                end = JavascriptArray::GetIndexFromVar(args[2], currentLen, scriptContext);
            }

            newLen = end > start ? end - start : 0;
        }

        // We can't have allocated an SharedArrayBuffer with byteLength > MaxArrayBufferLength.
        // start and end are clamped to valid indices, so the new length also cannot exceed MaxArrayBufferLength.
        // Therefore, should be safe to cast down newLen to uint32.
        Assert(newLen < MaxSharedArrayBufferLength);
        uint32 newbyteLength = static_cast<uint32>(newLen);

        SharedArrayBuffer* newBuffer = nullptr;

        if (scriptContext->GetConfig()->IsES6SpeciesEnabled())
        {
            JavascriptFunction* defaultConstructor = scriptContext->GetLibrary()->GetSharedArrayBufferConstructor();
            RecyclableObject* constructor = JavascriptOperators::SpeciesConstructor(currentBuffer, defaultConstructor, scriptContext);
            AssertOrFailFast(JavascriptOperators::IsConstructor(constructor));

            bool isDefaultConstructor = constructor == defaultConstructor;
            Js::Var newVar = JavascriptOperators::NewObjectCreationHelper_ReentrancySafe(constructor, isDefaultConstructor, scriptContext->GetThreadContext(), [=]()->Js::Var
            {
                Js::Var constructorArgs[] = { constructor, JavascriptNumber::ToVar(newbyteLength, scriptContext) };
                Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
                return JavascriptOperators::NewScObject(constructor, Js::Arguments(constructorCallInfo, constructorArgs), scriptContext);
            });

            if (!VarIs<SharedArrayBuffer>(newVar))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedSharedArrayBufferObject);
            }

            newBuffer = VarTo<SharedArrayBuffer>(newVar);

            if (newBuffer == currentBuffer)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedSharedArrayBufferObject);
            }

            if (newBuffer->GetByteLength() < newbyteLength)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_ArgumentOutOfRange, _u("SharedArrayBuffer.prototype.slice"));
            }
        }
        else
        {
            newBuffer = library->CreateSharedArrayBuffer(newbyteLength);
        }

        Assert(newBuffer);
        Assert(newBuffer->GetByteLength() >= newbyteLength);

        // Don't bother doing memcpy if we aren't copying any elements
        if (newbyteLength > 0)
        {
            AssertMsg(currentBuffer->GetBuffer() != nullptr, "buffer must not be null when we copy from it");

            js_memcpy_s(newBuffer->GetBuffer(), newbyteLength, currentBuffer->GetBuffer() + start, newbyteLength);
        }

        return newBuffer;
    }

    Var SharedArrayBuffer::EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);

        Assert(args.Info.Count > 0);

        return args[0];
    }

    BYTE* SharedArrayBuffer::AllocBuffer(uint32 length, uint32 maxLength)
    {
        Unused(maxLength); // WebAssembly only
#if ENABLE_FAST_ARRAYBUFFER
        if (this->IsValidVirtualBufferLength(length))
        {
            return (BYTE*)AsmJsVirtualAllocator(length);
        }
        else
#endif
        {
            return HeapNewNoThrowArray(BYTE, length);
        }
    }

    void SharedArrayBuffer::FreeBuffer(BYTE* buffer, uint32 length, uint32 maxLength)
    {
        Unused(maxLength); // WebAssembly only
#if ENABLE_FAST_ARRAYBUFFER
        //AsmJS Virtual Free
        if (this->IsValidVirtualBufferLength(length))
        {
            FreeMemAlloc(buffer);
        }
        else
#endif
        {
            HeapDeleteArray(length, buffer);
        }
    }

    void SharedArrayBuffer::Init(uint32 length, uint32 maxLength)
    {
        AssertOrFailFast(!sharedContents && length <= maxLength);
        BYTE * buffer = nullptr;
        if (length > MaxSharedArrayBufferLength)
        {
            // http://tc39.github.io/ecmascript_sharedmem/shmem.html#DataTypesValues.SpecTypes.DataBlocks.CreateSharedByteDataBlock
            // Let db be a new Shared Data Block value consisting of size bytes.
            // If it is impossible to create such a Shared Data Block, throw a RangeError exception.
            JavascriptError::ThrowRangeError(GetScriptContext(), JSERR_FunctionArgument_Invalid);
        }
        SharedContents* localSharedContents = HeapNewNoThrow(SharedContents, nullptr, length, maxLength);
        if (localSharedContents == nullptr)
        {
            JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
        }
        struct AutoCleanupSharedContents
        {
            SharedContents* sharedContents;
            bool allocationCompleted = false;
            AutoCleanupSharedContents(SharedContents* sharedContents) : sharedContents(sharedContents) {}
            ~AutoCleanupSharedContents()
            {
                if (!allocationCompleted)
                {
                    HeapDelete(sharedContents);
                }
            }
        } autoCleanupSharedContents(localSharedContents);

        Recycler* recycler = GetType()->GetLibrary()->GetRecycler();

        if (maxLength != 0)
        {
            if (recycler->RequestExternalMemoryAllocation(length))
            {
                buffer = this->AllocBuffer(length, maxLength);
                if (buffer == nullptr)
                {
                    recycler->CollectNow<CollectOnTypedArrayAllocation>();

                    buffer = this->AllocBuffer(length, maxLength);
                    if (buffer == nullptr)
                    {
                        recycler->ReportExternalMemoryFailure(length);
                        JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
                    }
                }
            }
            else
            {
                JavascriptError::ThrowOutOfMemoryError(GetScriptContext());
            }

            Assert(buffer != nullptr);
            ZeroMemory(buffer, length);
        }
        localSharedContents->buffer = buffer;
#if DBG
        localSharedContents->AddAgent((DWORD_PTR)GetScriptContext());
#endif
        sharedContents = localSharedContents;
        autoCleanupSharedContents.allocationCompleted = true;
    }

    SharedArrayBuffer::SharedArrayBuffer(DynamicType * type) :
        ArrayBufferBase(type), sharedContents(nullptr)
    {
    }

    SharedArrayBuffer::SharedArrayBuffer(SharedContents * contents, DynamicType * type) :
        ArrayBufferBase(type), sharedContents(nullptr)
    {
        if (contents == nullptr || contents->bufferLength > MaxSharedArrayBufferLength)
        {
            JavascriptError::ThrowTypeError(GetScriptContext(), JSERR_FunctionArgument_Invalid);
        }

        if (contents->AddRef() > 1)
        {
            sharedContents = contents;
        }
        else
        {
            Js::Throw::FatalInternalError();
        }
#if DBG
        sharedContents->AddAgent((DWORD_PTR)GetScriptContext());
#endif
    }

    SharedArrayBuffer * SharedArrayBuffer::GetAsSharedArrayBuffer()
    {
        AssertOrFailFast(VarIsCorrectType(this));
        return this;
    }

    CriticalSection SharedArrayBuffer::csSharedArrayBuffer;

    WaiterList *SharedArrayBuffer::GetWaiterList(uint index)
    {
        if (sharedContents != nullptr)
        {
            // REVIEW: only lock creating the map and pass the lock to the map?
            //         use one lock per instance?
            AutoCriticalSection autoCS(&csSharedArrayBuffer);

            if (sharedContents->indexToWaiterList == nullptr)
            {
                sharedContents->indexToWaiterList = HeapNew(IndexToWaitersMap, &HeapAllocator::Instance);
            }

            WaiterList * waiters = nullptr;
            if (!sharedContents->indexToWaiterList->TryGetValue(index, &waiters))
            {
                waiters = HeapNew(WaiterList);
                sharedContents->indexToWaiterList->Add(index, waiters);
            }
            return waiters;
        }

        Assert(false);
        return nullptr;
    }

    uint32 SharedArrayBuffer::GetByteLength() const
    {
        return sharedContents != nullptr ? sharedContents->bufferLength : 0;
    }

    BYTE* SharedArrayBuffer::GetBuffer() const
    {
        return sharedContents != nullptr ? sharedContents->buffer : nullptr;
    }

    BOOL SharedArrayBuffer::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Object, (SharedArrayBuffer)"));
        return TRUE;
    }

    BOOL SharedArrayBuffer::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("[object SharedArrayBuffer]"));
        return TRUE;
    }

    JavascriptSharedArrayBuffer::JavascriptSharedArrayBuffer(DynamicType * type) :
        SharedArrayBuffer(type)
    {
    }
    JavascriptSharedArrayBuffer::JavascriptSharedArrayBuffer(SharedContents *sharedContents, DynamicType * type) :
        SharedArrayBuffer(sharedContents, type)
    {
    }

    JavascriptSharedArrayBuffer* JavascriptSharedArrayBuffer::Create(uint32 length, DynamicType * type)
    {
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        JavascriptSharedArrayBuffer* result = RecyclerNewFinalized(recycler, JavascriptSharedArrayBuffer, type);
        result->Init(length, length);
        recycler->AddExternalMemoryUsage(length);
        return result;
    }

    JavascriptSharedArrayBuffer* JavascriptSharedArrayBuffer::Create(SharedContents *sharedContents, DynamicType * type)
    {
        AssertOrFailFast(!sharedContents || !sharedContents->IsWebAssembly());
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        JavascriptSharedArrayBuffer* result = RecyclerNewFinalized(recycler, JavascriptSharedArrayBuffer, sharedContents, type);
        return result;
    }

    bool SharedArrayBuffer::IsValidVirtualBufferLength(uint length) const
    {
#if ENABLE_FAST_ARRAYBUFFER
        /*
        1. length >= 2^16
        2. length is power of 2 or (length > 2^24 and length is multiple of 2^24)
        3. length is a multiple of 4K
        */
        return !PHASE_OFF1(Js::TypedArrayVirtualPhase) &&
            JavascriptArrayBuffer::IsValidAsmJsBufferLengthAlgo(length, true);
#else
        return false;
#endif
    }

    void JavascriptSharedArrayBuffer::Finalize(bool isShutdown)
    {
        if (sharedContents == nullptr)
        {
            return;
        }

        uint ref = sharedContents->Release();
        if (ref == 0)
        {
            this->FreeBuffer(sharedContents->buffer, sharedContents->bufferLength, sharedContents->maxBufferLength);

            Recycler* recycler = GetType()->GetLibrary()->GetRecycler();
            recycler->ReportExternalMemoryFree(sharedContents->bufferLength);

            sharedContents->Cleanup();
            HeapDelete(sharedContents);
        }

        sharedContents = nullptr;
    }

    void JavascriptSharedArrayBuffer::Dispose(bool isShutdown)
    {
        /* See JavascriptArrayBuffer::Finalize */
    }

#ifdef ENABLE_WASM_THREADS
    WebAssemblySharedArrayBuffer::WebAssemblySharedArrayBuffer(DynamicType * type):
        JavascriptSharedArrayBuffer(type)
    {
        AssertOrFailFast(Wasm::Threads::IsEnabled());
    }

    WebAssemblySharedArrayBuffer::WebAssemblySharedArrayBuffer(SharedContents *sharedContents, DynamicType * type) :
        JavascriptSharedArrayBuffer(sharedContents, type)
    {
        AssertOrFailFast(Wasm::Threads::IsEnabled());
        ValidateBuffer();
    }

    void WebAssemblySharedArrayBuffer::ValidateBuffer()
    {
#if DBG && _WIN32
        if (CONFIG_FLAG(WasmSharedArrayVirtualBuffer))
        {
            MEMORY_BASIC_INFORMATION info = { 0 };
            size_t size = 0;
            size_t allocationSize = 0;
            // Make sure the beggining of the buffer is committed memory to the expected size
            if (sharedContents->bufferLength > 0)
            {
                size = VirtualQuery((LPCVOID)sharedContents->buffer, &info, sizeof(info));
                Assert(size > 0);
                allocationSize = info.RegionSize + ((uintptr_t)info.BaseAddress - (uintptr_t)info.AllocationBase);
                Assert(allocationSize == sharedContents->bufferLength && info.State == MEM_COMMIT && info.Type == MEM_PRIVATE);
            }

            // Make sure the end of the buffer is reserved memory to the expected size
            size_t expectedAllocationSize = sharedContents->maxBufferLength;
#if ENABLE_FAST_ARRAYBUFFER
            if (CONFIG_FLAG(WasmFastArray))
            {
                expectedAllocationSize = MAX_WASM__ARRAYBUFFER_LENGTH;
            }
#endif
            // If the whole buffer has been committed, no need to verify this
            if (expectedAllocationSize > sharedContents->bufferLength)
            {
                size = VirtualQuery((LPCVOID)(sharedContents->buffer + sharedContents->bufferLength), &info, sizeof(info));
                Assert(size > 0);
                allocationSize = info.RegionSize + ((uintptr_t)info.BaseAddress - (uintptr_t)info.AllocationBase);
                Assert(allocationSize == expectedAllocationSize && info.State == MEM_RESERVE && info.Type == MEM_PRIVATE);
            }
        }
#endif
    }

    WebAssemblySharedArrayBuffer* WebAssemblySharedArrayBuffer::Create(uint32 length, uint32 maxLength, DynamicType * type)
    {
        AssertOrFailFast(Wasm::Threads::IsEnabled());
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        WebAssemblySharedArrayBuffer* result = RecyclerNewFinalized(recycler, WebAssemblySharedArrayBuffer, type);
        result->Init(length, maxLength);
        result->sharedContents->SetIsWebAssembly();
        result->ValidateBuffer();
        recycler->AddExternalMemoryUsage(length);
        return result;
    }

    WebAssemblySharedArrayBuffer* WebAssemblySharedArrayBuffer::Create(SharedContents *sharedContents, DynamicType * type)
    {
        AssertOrFailFast(Wasm::Threads::IsEnabled());
        AssertOrFailFast(sharedContents && sharedContents->IsWebAssembly());
        Recycler* recycler = type->GetScriptContext()->GetRecycler();
        WebAssemblySharedArrayBuffer* result = RecyclerNewFinalized(recycler, WebAssemblySharedArrayBuffer, sharedContents, type);
        return result;
    }

    bool WebAssemblySharedArrayBuffer::IsValidVirtualBufferLength(uint length) const
    {
#if ENABLE_FAST_ARRAYBUFFER
        if (CONFIG_FLAG(WasmFastArray))
        {
            return true;
        }
#endif
#ifdef _WIN32
        if (CONFIG_FLAG(WasmSharedArrayVirtualBuffer))
        {
            return true;
        }
#endif
        return false;
    }

    _Must_inspect_result_ bool WebAssemblySharedArrayBuffer::GrowMemory(uint32 newBufferLength)
    {
        uint32 bufferLength = sharedContents->bufferLength;
        BYTE* buffer = sharedContents->buffer;
        if (newBufferLength < bufferLength || newBufferLength > sharedContents->maxBufferLength)
        {
            AssertMsg(newBufferLength <= sharedContents->maxBufferLength, "This shouldn't happen");
            Assert(UNREACHED);
            JavascriptError::ThrowTypeError(GetScriptContext(), WASMERR_BufferGrowOnly);
        }

        uint32 growSize = newBufferLength - bufferLength;

        // We're not growing the buffer, do nothing
        if (growSize == 0)
        {
            return true;
        }

        AssertOrFailFast(buffer);
        if (IsValidVirtualBufferLength(newBufferLength))
        {
            auto virtualAllocFunc = [=]
            {
                return !!VirtualAlloc(buffer + bufferLength, growSize, MEM_COMMIT, PAGE_READWRITE);
            };
            if (!this->GetRecycler()->DoExternalAllocation(growSize, virtualAllocFunc))
            {
                return false;
            }

            this->GetRecycler()->AddExternalMemoryUsage(growSize);
        }
        else
        {
            // We have already allocated maxLength in the heap if we're not using virtual alloc
        }
        ZeroMemory(buffer + bufferLength, growSize);
        sharedContents->bufferLength = newBufferLength;
        ValidateBuffer();
        return true;
    }

    BYTE* WebAssemblySharedArrayBuffer::AllocBuffer(uint32 length, uint32 maxLength)
    {
#if ENABLE_FAST_ARRAYBUFFER
        if (CONFIG_FLAG(WasmFastArray))
        {
            return (BYTE*)WasmVirtualAllocator(length);
        }
#endif
#ifdef _WIN32
        if (CONFIG_FLAG(WasmSharedArrayVirtualBuffer))
        {
            return (BYTE*)AllocWrapper(length, maxLength);
        }
#endif
        AssertOrFailFast(maxLength >= length);
        uint32 additionalSize = maxLength - length;
        if (additionalSize > 0)
        {
            // SharedArrayBuffer::Init already requested External Memory for `length`, we need to request the balance
            if (!this->GetRecycler()->RequestExternalMemoryAllocation(additionalSize))
            {
                // Failed to request for more memory
                return nullptr;
            }
        }
        // Allocate the full size of the buffer if we can't do VirtualAlloc
        return HeapNewNoThrowArray(BYTE, maxLength);
    }

    void WebAssemblySharedArrayBuffer::FreeBuffer(BYTE* buffer, uint32 length, uint32 maxLength)
    {
        if (IsValidVirtualBufferLength(length))
        {
            FreeMemAlloc(buffer);
        }
        else
        {
            HeapDeleteArray(maxLength, buffer);

            AssertOrFailFast(maxLength >= length);
            // JavascriptSharedArrayBuffer::Finalize will only report freeing `length`, we have to take care of the balance
            uint32 additionalSize = maxLength - length;
            if (additionalSize > 0)
            {
                Recycler* recycler = GetType()->GetLibrary()->GetRecycler();
                recycler->ReportExternalMemoryFree(additionalSize);
            }
        }
    }
#endif

    WaiterList::WaiterList()
        : m_waiters(nullptr)
    {
        InitWaiterList();
    }

    void WaiterList::InitWaiterList()
    {
        Assert(m_waiters == nullptr);
        m_waiters = HeapNew(Waiters, &HeapAllocator::Instance);
    }

    void WaiterList::Cleanup()
    {
        if (m_waiters != nullptr)
        {
            HeapDelete(m_waiters);
            m_waiters = nullptr;
        }
    }

    bool WaiterList::Contains(DWORD_PTR agent)
    {
        Assert(m_waiters != nullptr);
        for (int i = 0; i < m_waiters->Count(); i++)
        {
            if (m_waiters->Item(i).identity == agent)
            {
                return true;
            }
        }

        return false;
    }

    bool _Requires_lock_held_(csForAccess.cs) WaiterList::AddAndSuspendWaiter(DWORD_PTR waiter, uint32 timeout)
    {
#ifdef _WIN32
        Assert(m_waiters != nullptr);
        Assert(waiter != NULL);
        Assert(!Contains(waiter));

        AgentOfBuffer agent(waiter, CreateEvent(NULL, TRUE, FALSE, NULL));
        m_waiters->Add(agent);

        csForAccess.Leave();
        DWORD result = WaitForSingleObject(agent.event, timeout);
        csForAccess.Enter();
        return result == WAIT_OBJECT_0;
#else
        // TODO for xplat
        return false;
#endif
    }

    void WaiterList::RemoveWaiter(DWORD_PTR waiter)
    {
#ifdef _WIN32
        Assert(m_waiters != nullptr);
        for (int i = m_waiters->Count() - 1; i >= 0; i--)
        {
            if (m_waiters->Item(i).identity == waiter)
            {
                CloseHandle(m_waiters->Item(i).event);
                m_waiters->RemoveAt(i);
                return;
            }
        }

        Assert(false);
#endif
        // TODO for xplat
    }

    uint32 WaiterList::RemoveAndWakeWaiters(int32 count)
    {
        Assert(m_waiters != nullptr);
        Assert(count >= 0);
        uint32 removed = 0;
#ifdef _WIN32
        while (count > 0 && m_waiters->Count() > 0)
        {
            AgentOfBuffer agent = m_waiters->Item(0);
            m_waiters->RemoveAt(0);
            count--; removed++;
            SetEvent(agent.event);
            // This agent will be closed when their respective call to wait has returned
        }
#endif
        return removed;
    }

    bool AgentOfBuffer::AgentCanSuspend(ScriptContext *scriptContext)
    {
        // Currently we are allowing every agent (including main page) to suspend. If we want only web-worker to suspend un-comment below code.
        // return scriptContext != nullptr && scriptContext->webWorkerId != Constants::NonWebWorkerContextId;

        return true;
    }
}
