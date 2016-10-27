//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
    class WebAssemblyMemory : public DynamicObject
    {
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

        static bool Is(Var aValue);
        static WebAssemblyMemory * FromVar(Var aValue);

        static WebAssemblyMemory * CreateMemoryObject(uint32 initial, uint32 maximum, ScriptContext * scriptContext);

        ArrayBuffer * GetBuffer() const;

        static int GetOffsetOfArrayBuffer() { return offsetof(WebAssemblyMemory, m_buffer); }
    private:
        WebAssemblyMemory(ArrayBuffer * buffer, uint32 initial, uint32 maximum, DynamicType * type);

        ArrayBuffer * m_buffer;

        uint m_initial;
        uint m_maximum;
#endif
    };

} // namespace Js
