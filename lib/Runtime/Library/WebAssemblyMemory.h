//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
    class WebAssemblyMemory : public DynamicObject
    {
    protected:
        DEFINE_VTABLE_CTOR( WebAssemblyMemory, DynamicObject );
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT( WebAssemblyMemory );

#ifdef ENABLE_WASM
    public:
        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Grow;
            static FunctionInfo GetterBuffer;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGrow(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterBuffer(RecyclableObject* function, CallInfo callInfo, ...);

        static WebAssemblyMemory * CreateMemoryObject(uint32 initial, uint32 maximum, bool isShared, ScriptContext * scriptContext);
        static WebAssemblyMemory * CreateForExistingBuffer(uint32 initial, uint32 maximum, uint32 currentByteLength, ScriptContext * scriptContext);
#ifdef ENABLE_WASM_THREADS
        static WebAssemblyMemory * CreateFromSharedContents(uint32 initial, uint32 maximum, SharedContents* sharedContents, ScriptContext * scriptContext);
#endif

        ArrayBufferBase * GetBuffer() const;
        uint GetInitialLength() const;
        uint GetMaximumLength() const;
        uint GetCurrentMemoryPages() const;
#ifdef ENABLE_WASM_THREADS
        bool IsSharedMemory() const;
#endif

        int32 GrowInternal(uint32 deltaPages);
        static int32 GrowHelper(Js::WebAssemblyMemory * memory, uint32 deltaPages);

        static int GetOffsetOfArrayBuffer() { return offsetof(WebAssemblyMemory, m_buffer); }
#if DBG
        static void TraceMemWrite(WebAssemblyMemory* mem, uint32 index, uint32 offset, Js::ArrayBufferView::ViewType viewType, uint32 bytecodeOffset, ScriptContext* context);
#endif
    private:
        WebAssemblyMemory(ArrayBufferBase* buffer, uint32 initial, uint32 maximum, DynamicType * type);
        static _Must_inspect_result_ bool AreLimitsValid(uint32 initial, uint32 maximum);
        static _Must_inspect_result_ bool AreLimitsValid(uint32 initial, uint32 maximum, uint32 bufferLength);

        Field(ArrayBufferBase*) m_buffer;

        Field(uint) m_initial;
        Field(uint) m_maximum;
#endif
    };

    template <> inline bool VarIsImpl<WebAssemblyMemory>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_WebAssemblyMemory;
    }
} // namespace Js
