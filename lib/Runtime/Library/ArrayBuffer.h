//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//  Implements ArrayBuffer according to Khronos spec.
//----------------------------------------------------------------------------

#pragma once
namespace Js
{
    class ArrayBufferParent;
    class ArrayBuffer;
    class SharedArrayBuffer;
    class ArrayBufferContentForDelayedFreeBase;

    class ArrayBufferBase : public DynamicObject
    {
    protected:
#if ENABLE_FAST_ARRAYBUFFER
#define MAX_ASMJS_ARRAYBUFFER_LENGTH 0x100000000 // 4GB
#define MAX_WASM__ARRAYBUFFER_LENGTH 0x200000000 // 8GB
        typedef void*(*AllocWrapperType)(size_t);
#define AsmJsVirtualAllocator ((AllocWrapperType)Js::ArrayBuffer::AllocWrapper<MAX_ASMJS_ARRAYBUFFER_LENGTH>)
#define WasmVirtualAllocator ((AllocWrapperType)Js::ArrayBuffer::AllocWrapper<MAX_WASM__ARRAYBUFFER_LENGTH>)
#else
#define AsmJsVirtualAllocator Js::ArrayBuffer::BadAllocCall
#define WasmVirtualAllocator Js::ArrayBuffer::BadAllocCall
        static void* __cdecl BadAllocCall(DECLSPEC_GUARD_OVERFLOW size_t length)
        {
            // This allocator should never be used
            Js::Throw::FatalInternalError();
        }
#endif
#ifdef _WIN32
        static void* __cdecl AllocWrapper(DECLSPEC_GUARD_OVERFLOW size_t length, size_t MaxVirtualSize)
        {
            LPVOID address = VirtualAlloc(nullptr, MaxVirtualSize, MEM_RESERVE, PAGE_NOACCESS);
            //throw out of memory
            if (!address)
            {
                return nullptr;
            }

            if (length == 0)
            {
                return address;
            }

            LPVOID arrayAddress = VirtualAlloc(address, length, MEM_COMMIT, PAGE_READWRITE);
            if (!arrayAddress)
            {
                VirtualFree(address, 0, MEM_RELEASE);
                return nullptr;
            }
            return arrayAddress;
        }
        template<size_t MaxVirtualSize>
        static void* __cdecl AllocWrapper(DECLSPEC_GUARD_OVERFLOW size_t length)
        {
            return AllocWrapper(length, MaxVirtualSize);
        }

        static void FreeMemAlloc(Var ptr)
        {
            BOOL fSuccess = VirtualFree((LPVOID)ptr, 0, MEM_RELEASE);
            Assert(fSuccess);
        }
#else
        static void FreeMemAlloc(Var ptr)
        {
            // This free function should never be used
            Js::Throw::FatalInternalError();
        }
#endif
    public:
        DEFINE_VTABLE_CTOR_ABSTRACT(ArrayBufferBase, DynamicObject);

        virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext) = 0;

        ArrayBufferBase(DynamicType *type) : DynamicObject(type), isDetached(false) { }
        bool IsDetached() { return isDetached; }

#if ENABLE_TTD
        virtual void MarshalCrossSite_TTDInflate() = 0;
#endif

        virtual bool IsArrayBuffer() = 0;
        virtual bool IsSharedArrayBuffer() = 0;
        virtual bool IsWebAssemblyArrayBuffer() { return false; }
        virtual ArrayBuffer * GetAsArrayBuffer() = 0;
        virtual SharedArrayBuffer * GetAsSharedArrayBuffer() { return nullptr; }
        virtual void AddParent(ArrayBufferParent* parent) { }
        virtual uint32 GetByteLength() const = 0;
        virtual BYTE* GetBuffer() const = 0;
        virtual bool IsValidVirtualBufferLength(uint length) const { return false; };

        static int GetIsDetachedOffset() { return offsetof(ArrayBufferBase, isDetached); }

    protected:
        Field(bool) isDetached;
    };

    template <> bool VarIsImpl<ArrayBufferBase>(RecyclableObject* obj);

    // This encapsulate buffer blob and the refCount.
    class RefCountedBuffer
    {
    private:
        FieldNoBarrier(BYTE*) buffer; // Points to a heap allocated RGBA buffer, can be null

        // Addref/release counter for current buffer, this is needed hold the current buffer alive
        Field(long) refCount;
    public:
        long AddRef();
        long Release();
        BYTE* GetBuffer() { return buffer; };
        long GetRefCount() { return refCount; }

        static int GetBufferOffset() { return offsetof(RefCountedBuffer, buffer); }

        RefCountedBuffer(BYTE* b)
            : buffer(b), refCount(1)
        { }
    };



    class ArrayBuffer : public ArrayBufferBase
    {
    public:
        // we need to install cross-site thunk on the nested array buffer when marshaling
        // typed array.
        DEFINE_VTABLE_CTOR_ABSTRACT(ArrayBuffer, ArrayBufferBase);
    private:
        void DetachBufferFromParent(ArrayBufferParent* parent);
    public:
        template <typename FreeFN>
        class ArrayBufferDetachedState : public ArrayBufferDetachedStateBase
        {
        public:
            FreeFN* freeFunction;
            Recycler* recycler;
            ArrayBufferDetachedState(RefCountedBuffer* buffer, uint32 bufferLength, FreeFN* freeFunction, Recycler* r, ArrayBufferAllocationType allocationType)
                : ArrayBufferDetachedStateBase(TypeIds_ArrayBuffer, buffer, bufferLength, allocationType),
                recycler(r),
                freeFunction(freeFunction)
            {}

            virtual void ClearSelfOnly() override
            {
                HeapDelete(this);
            }

            virtual void DiscardState() override
            {
                DiscardStateBase(freeFunction);
            }

            virtual void Discard() override
            {
                ClearSelfOnly();
            }
        };

        template <typename Allocator>
        ArrayBuffer(DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type, Allocator allocator);

        ArrayBuffer(byte* buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type, bool isExternal = false);

        ArrayBuffer(RefCountedBuffer* buffContent, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Slice;
            static FunctionInfo IsView;
            static FunctionInfo GetterByteLength;
            static FunctionInfo GetterSymbolSpecies;
#if ENABLE_DEBUG_CONFIG_OPTIONS
            static FunctionInfo Detach;
#endif
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySlice(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsView(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterByteLength(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...);
#if ENABLE_DEBUG_CONFIG_OPTIONS
        static Var EntryDetach(RecyclableObject* function, CallInfo callInfo, ...);
#endif

        static ArrayBuffer* NewFromDetachedState(DetachedStateBase* state, JavascriptLibrary *library);

        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        ArrayBufferDetachedStateBase* DetachAndGetState(bool queueForDelayFree = true);
        virtual uint32 GetByteLength() const override;
        virtual BYTE* GetBuffer() const override;
        RefCountedBuffer *GetBufferContent() { return bufferContent;  }
        static int GetBufferContentsOffset() { return offsetof(ArrayBuffer, bufferContent); }
        static int GetByteLengthOffset() { return offsetof(ArrayBuffer, bufferLength); }

        virtual void AddParent(ArrayBufferParent* parent) override;
#if defined(TARGET_64)
        //maximum 2G -1  for amd64
        static const uint32 MaxArrayBufferLength = 0x7FFFFFFF;
#else
        // maximum 1G to avoid arithmetic overflow.
        static const uint32 MaxArrayBufferLength = 1 << 30;
#endif
        static const uint32 ParentsCleanupThreshold = 1000;

        virtual bool IsValidAsmJsBufferLength(uint length, bool forceCheck = false) { return false; }
        virtual bool IsArrayBuffer() override { return true; }
        virtual bool IsSharedArrayBuffer() override { return false; }
        virtual ArrayBuffer * GetAsArrayBuffer() override;

        virtual ArrayBufferContentForDelayedFreeBase* CopyBufferContentForDelayedFree(RefCountedBuffer * content, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength);

        static uint32 ToIndex(Var value, int32 errorCode, ScriptContext *scriptContext, uint32 MaxAllowedLength, bool checkSameValueZero = true);

    protected:
        virtual void ReportExternalMemoryFree();
        void Detach();

        typedef void __cdecl FreeFn(void* ptr);
        virtual ArrayBufferDetachedStateBase* CreateDetachedState(RefCountedBuffer * content, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) = 0;

        // This function will be called from External buffer and projection buffer as they pass the buffer
        virtual void ReleaseBufferContent();

        //In most cases, the ArrayBuffer will only have one parent
        Field(RecyclerWeakReference<ArrayBufferParent>*) primaryParent;

        struct OtherParents :public SList<RecyclerWeakReference<ArrayBufferParent>*, Recycler>
        {
            OtherParents(Recycler* recycler)
                :SList<RecyclerWeakReference<ArrayBufferParent>*, Recycler>(recycler), increasedCount(0)
            {
            }
            Field(uint) increasedCount;
        };

        Field(OtherParents*) otherParents;
        FieldNoBarrier(RefCountedBuffer *) bufferContent;
        Field(uint32) bufferLength;       // Number of bytes allocated
    };

    template <> inline bool VarIsImpl<ArrayBuffer>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_ArrayBuffer;
    }

    class ArrayBufferParent : public ArrayObject
    {
        friend ArrayBuffer;
        friend ArrayBufferBase;

    private:
        Field(ArrayBufferBase*) arrayBuffer;

    protected:
        DEFINE_VTABLE_CTOR_ABSTRACT(ArrayBufferParent, ArrayObject);

        ArrayBufferParent(DynamicType * type, uint32 length, ArrayBufferBase* arrayBuffer)
            : ArrayObject(type, /*initSlots*/true, length),
            arrayBuffer(arrayBuffer)
        {
            arrayBuffer->AddParent(this);
        }

    public:
        ArrayBufferBase* GetArrayBuffer() const
        {
            return this->arrayBuffer;
        }

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual void ProcessCorePaths() override;
#endif
    };

    // Normally we use malloc/free; for ArrayBuffer created from projection we need to use different allocator.
    class JavascriptArrayBuffer : public ArrayBuffer
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptArrayBuffer, ArrayBuffer);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptArrayBuffer);

    public:
        static JavascriptArrayBuffer* Create(DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);
        static JavascriptArrayBuffer* Create(byte* buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);
        static JavascriptArrayBuffer* Create(RefCountedBuffer* buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);
        virtual void Dispose(bool isShutdown) override;
        virtual void Finalize(bool isShutdown) override;

        static bool IsValidAsmJsBufferLengthAlgo(uint length, bool forceCheck);
        virtual bool IsValidAsmJsBufferLength(uint length, bool forceCheck = false) override;
        virtual bool IsValidVirtualBufferLength(uint length) const override;

    protected:
        JavascriptArrayBuffer(DynamicType * type);
        virtual ArrayBufferDetachedStateBase* CreateDetachedState(RefCountedBuffer * content, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) override;

        template<typename Allocator>
        JavascriptArrayBuffer(uint32 length, DynamicType * type, Allocator allocator): ArrayBuffer(length, type, allocator){}
        JavascriptArrayBuffer(uint32 length, DynamicType * type);
        JavascriptArrayBuffer(byte* buffer, uint32 length, DynamicType * type);
        JavascriptArrayBuffer(RefCountedBuffer* buffer, uint32 length, DynamicType * type);

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

#ifdef ENABLE_WASM
    class WebAssemblyArrayBuffer : public JavascriptArrayBuffer
    {
        template<typename Allocator>
        WebAssemblyArrayBuffer(uint32 length, DynamicType * type, Allocator allocator);
        WebAssemblyArrayBuffer(uint32 length, DynamicType * type);
        WebAssemblyArrayBuffer(byte* buffer, uint32 length, DynamicType * type);
    protected:
        DEFINE_VTABLE_CTOR(WebAssemblyArrayBuffer, JavascriptArrayBuffer);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(WebAssemblyArrayBuffer);
    public:
        static WebAssemblyArrayBuffer* Create(byte* buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);
        WebAssemblyArrayBuffer* GrowMemory(uint32 newBufferLength);

        virtual bool IsValidVirtualBufferLength(uint length) const override;
        virtual bool IsWebAssemblyArrayBuffer() override { return true; }

    protected:
        virtual ArrayBufferDetachedStateBase* CreateDetachedState(RefCountedBuffer * content, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) override;
    };
#endif

    // the memory must be allocated via CoTaskMemAlloc.
    class ProjectionArrayBuffer : public ArrayBuffer
    {
    protected:
        DEFINE_VTABLE_CTOR(ProjectionArrayBuffer, ArrayBuffer);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ProjectionArrayBuffer);

        typedef void __stdcall FreeFn(LPVOID ptr);
        virtual ArrayBufferDetachedStateBase* CreateDetachedState(RefCountedBuffer * content, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) override
        {
            return HeapNew(ArrayBufferDetachedState<FreeFn>, content, bufferLength, CoTaskMemFree, GetScriptContext()->GetRecycler(), ArrayBufferAllocationType::CoTask);
        }

    public:
        // Create constructor. script engine creates a buffer allocated via CoTaskMemAlloc.
        static ProjectionArrayBuffer* Create(DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);
        // take over ownership. a CoTaskMemAlloc'ed buffer passed in via projection.
        static ProjectionArrayBuffer* Create(byte* buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);
        static ProjectionArrayBuffer* Create(RefCountedBuffer* buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);

        virtual ArrayBufferContentForDelayedFreeBase* CopyBufferContentForDelayedFree(RefCountedBuffer * content, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) override;

        virtual void Dispose(bool isShutdown) override;
        virtual void Finalize(bool isShutdown) override;
    private:
        ProjectionArrayBuffer(uint32 length, DynamicType * type);
        ProjectionArrayBuffer(byte* buffer, uint32 length, DynamicType * type);
        ProjectionArrayBuffer(RefCountedBuffer* buffer, uint32 length, DynamicType * type);
    };

    // non-owning ArrayBuffer used for wrapping external data
    class ExternalArrayBuffer : public ArrayBuffer
    {
    protected:
        DEFINE_VTABLE_CTOR(ExternalArrayBuffer, ArrayBuffer);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ExternalArrayBuffer);

    public:
        ExternalArrayBuffer(byte *buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType *type);
        ExternalArrayBuffer(RefCountedBuffer *buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType *type);
        static ExternalArrayBuffer* Create(RefCountedBuffer* buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);
    protected:
        virtual ArrayBufferDetachedStateBase* CreateDetachedState(RefCountedBuffer* buffer, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) override;
        virtual void ReportExternalMemoryFree() override;

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    class ExternalArrayBufferDetachedState : public ArrayBufferDetachedStateBase
    {
    public:
        ExternalArrayBufferDetachedState(RefCountedBuffer* buffer, uint32 bufferLength);
        virtual void ClearSelfOnly() override;
        virtual void DiscardState() override;
        virtual void Discard() override;
        virtual ArrayBuffer* Create(JavascriptLibrary* library);
    };
}
