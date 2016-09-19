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
    class ArrayBufferBase : public DynamicObject
    {
    public:
        DEFINE_VTABLE_CTOR_ABSTRACT(ArrayBufferBase, DynamicObject);

        virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext) = 0;

        ArrayBufferBase(DynamicType *type) : DynamicObject(type) { }

        virtual bool IsArrayBuffer() = 0;
        virtual bool IsSharedArrayBuffer() = 0;
        virtual ArrayBuffer * GetAsArrayBuffer() = 0;
        virtual SharedArrayBuffer * GetAsSharedArrayBuffer() { return nullptr; }
        virtual void AddParent(ArrayBufferParent* parent) { }
        virtual void RemoveParent(ArrayBufferParent* parent) { }
        virtual bool IsDetached() { return false; }
        virtual uint32 GetByteLength() const = 0;
        virtual BYTE* GetBuffer() const = 0;
        virtual bool IsValidVirtualBufferLength(uint length) { return false; }

        static bool Is(Var value);
        static ArrayBufferBase* FromVar(Var value);
    };

    class ArrayBuffer : public ArrayBufferBase
    {
    public:
        // we need to install cross-site thunk on the nested array buffer when marshaling
        // typed array.
        DEFINE_VTABLE_CTOR_ABSTRACT(ArrayBuffer, ArrayBufferBase);
#define MAX_ASMJS_ARRAYBUFFER_LENGTH 0x100000000 //4GB
    private:
        void ClearParentsLength(ArrayBufferParent* parent);
        static uint32 GetByteLengthFromVar(ScriptContext* scriptContext, Var length);
    public:
        template <typename FreeFN>
        class ArrayBufferDetachedState : public ArrayBufferDetachedStateBase
        {
        public:
            FreeFN* freeFunction;

            ArrayBufferDetachedState(BYTE* buffer, uint32 bufferLength, FreeFN* freeFunction, ArrayBufferAllocationType allocationType)
                : ArrayBufferDetachedStateBase(TypeIds_ArrayBuffer, buffer, bufferLength, allocationType),
                freeFunction(freeFunction)
            {}

            virtual void ClearSelfOnly() override
            {
                HeapDelete(this);
            }

            virtual void DiscardState() override
            {
                if (this->buffer != nullptr)
                {
                    freeFunction(this->buffer);
                    this->buffer = nullptr;
                }
                this->bufferLength = 0;
            }

            virtual void Discard() override
            {
                ClearSelfOnly();
            }
        };

        template <typename Allocator>
        ArrayBuffer(DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type, Allocator allocator);

        ArrayBuffer(byte* buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Slice;
            static FunctionInfo IsView;
            static FunctionInfo GetterByteLength;
            static FunctionInfo GetterSymbolSpecies;
            static FunctionInfo Transfer;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySlice(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsView(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterByteLength(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryTransfer(RecyclableObject* function, CallInfo callInfo, ...);

        static bool Is(Var aValue);
        static ArrayBuffer* NewFromDetachedState(DetachedStateBase* state, JavascriptLibrary *library);
        static ArrayBuffer* FromVar(Var aValue);

        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        virtual ArrayBufferDetachedStateBase* DetachAndGetState();
        virtual bool IsDetached() override { return this->isDetached; }
        void SetIsAsmJsBuffer(){ mIsAsmJsBuffer = true; }
        virtual uint32 GetByteLength() const override { return bufferLength; }
        virtual BYTE* GetBuffer() const override { return buffer; }

        static int GetByteLengthOffset() { return offsetof(ArrayBuffer, bufferLength); }
        static int GetIsDetachedOffset() { return offsetof(ArrayBuffer, isDetached); }
        static int GetBufferOffset() { return offsetof(ArrayBuffer, buffer); }

        virtual void AddParent(ArrayBufferParent* parent) override;
        virtual void RemoveParent(ArrayBufferParent* parent) override;
#if _WIN64
        //maximum 2G -1  for amd64
        static const uint32 MaxArrayBufferLength = 0x7FFFFFFF;
#else
        // maximum 1G to avoid arithmetic overflow.
        static const uint32 MaxArrayBufferLength = 1 << 30;
#endif
        virtual bool IsValidAsmJsBufferLength(uint length, bool forceCheck = false) { return false; }
        virtual bool IsArrayBuffer() override { return true; }
        virtual bool IsSharedArrayBuffer() override { return false; }
        virtual ArrayBuffer * GetAsArrayBuffer() override { return ArrayBuffer::FromVar(this); }

    protected:

        typedef void __cdecl FreeFn(void* ptr);
        virtual ArrayBufferDetachedStateBase* CreateDetachedState(BYTE* buffer, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) = 0;
        virtual ArrayBuffer * TransferInternal(DECLSPEC_GUARD_OVERFLOW uint32 newBufferLength) = 0;

        static uint32 GetIndexFromVar(Js::Var arg, uint32 length, ScriptContext* scriptContext);

        //In most cases, the ArrayBuffer will only have one parent
        RecyclerWeakReference<ArrayBufferParent>* primaryParent;
        JsUtil::List<RecyclerWeakReference<ArrayBufferParent>*>* otherParents;


        BYTE  *buffer;             // Points to a heap allocated RGBA buffer, can be null
        uint32 bufferLength;       // Number of bytes allocated

        // When an ArrayBuffer is detached, the TypedArray and DataView objects pointing to it must be made aware,
        // for this purpose the ArrayBuffer needs to hold WeakReferences to them
        bool isDetached;
        bool mIsAsmJsBuffer;
        bool isBufferCleared;

    };

    class ArrayBufferParent : public ArrayObject
    {
        friend ArrayBuffer;
        friend ArrayBufferBase;

    private:
        ArrayBufferBase* arrayBuffer;

    protected:
        DEFINE_VTABLE_CTOR_ABSTRACT(ArrayBufferParent, ArrayObject);

        ArrayBufferParent(DynamicType * type, uint32 length, ArrayBufferBase* arrayBuffer)
            : ArrayObject(type, /*initSlots*/true, length),
            arrayBuffer(arrayBuffer)
        {
            arrayBuffer->AddParent(this);
        }

        void ClearArrayBuffer()
        {
            if (this->arrayBuffer != nullptr)
            {
                this->arrayBuffer->RemoveParent(this);
                this->arrayBuffer = nullptr;
            }
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
        virtual void Dispose(bool isShutdown) override;
        virtual void Finalize(bool isShutdown) override;
        static void*__cdecl  AllocWrapper(DECLSPEC_GUARD_OVERFLOW size_t length)
        {
#if _WIN64
            LPVOID address = VirtualAlloc(nullptr, MAX_ASMJS_ARRAYBUFFER_LENGTH, MEM_RESERVE, PAGE_NOACCESS);
            //throw out of memory
            if (!address)
            {
                Js::Throw::OutOfMemory();
            }
            LPVOID arrayAddress = VirtualAlloc(address, length, MEM_COMMIT, PAGE_READWRITE);
            if (!arrayAddress)
            {
                VirtualFree(address, 0, MEM_RELEASE);
                Js::Throw::OutOfMemory();
            }
            return arrayAddress;
#else
            Assert(false);
            return nullptr;
#endif
        }

        static void FreeMemAlloc(Var ptr)
        {
            BOOL fSuccess = VirtualFree((LPVOID)ptr, 0, MEM_RELEASE);
            Assert(fSuccess);
        }

        virtual bool IsValidAsmJsBufferLength(uint length, bool forceCheck = false) override;

        virtual bool IsValidVirtualBufferLength(uint length) override;

    protected:
        JavascriptArrayBuffer(DynamicType * type);
        virtual ArrayBufferDetachedStateBase* CreateDetachedState(BYTE* buffer, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) override;
        virtual ArrayBuffer * TransferInternal(DECLSPEC_GUARD_OVERFLOW uint32 newBufferLength) override;
    private:
        JavascriptArrayBuffer(uint32 length, DynamicType * type);
        JavascriptArrayBuffer(byte* buffer, uint32 length, DynamicType * type);

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    // the memory must be allocated via CoTaskMemAlloc.
    class ProjectionArrayBuffer : public ArrayBuffer
    {
    protected:
        DEFINE_VTABLE_CTOR(ProjectionArrayBuffer, ArrayBuffer);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ProjectionArrayBuffer);

        typedef void __stdcall FreeFn(LPVOID ptr);
        virtual ArrayBufferDetachedStateBase* CreateDetachedState(BYTE* buffer, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) override
        {
            return HeapNew(ArrayBufferDetachedState<FreeFn>, buffer, bufferLength, CoTaskMemFree, ArrayBufferAllocationType::CoTask);
        }
        virtual ArrayBuffer * TransferInternal(DECLSPEC_GUARD_OVERFLOW uint32 newBufferLength) override;

    public:
        // Create constructor. script engine creates a buffer allocated via CoTaskMemAlloc.
        static ProjectionArrayBuffer* Create(DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);
        // take over ownership. a CoTaskMemAlloc'ed buffer passed in via projection.
        static ProjectionArrayBuffer* Create(byte* buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType * type);
        virtual void Dispose(bool isShutdown) override;
        virtual void Finalize(bool isShutdown) override {};
    private:
        ProjectionArrayBuffer(uint32 length, DynamicType * type);
        ProjectionArrayBuffer(byte* buffer, uint32 length, DynamicType * type);
    };

    // non-owning ArrayBuffer used for wrapping external data
    class ExternalArrayBuffer : public ArrayBuffer
    {
    protected:
        DEFINE_VTABLE_CTOR(ExternalArrayBuffer, ArrayBuffer);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ExternalArrayBuffer);
    public:
        ExternalArrayBuffer(byte *buffer, DECLSPEC_GUARD_OVERFLOW uint32 length, DynamicType *type);
    protected:
        virtual ArrayBufferDetachedStateBase* CreateDetachedState(BYTE* buffer, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) override { Assert(UNREACHED); Throw::InternalError(); };
        virtual ArrayBuffer * TransferInternal(DECLSPEC_GUARD_OVERFLOW uint32 newBufferLength) override { Assert(UNREACHED); Throw::InternalError(); };

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };
}
