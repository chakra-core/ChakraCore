//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
    class WebAssemblyTable : public DynamicObject
    {
    protected:
        DEFINE_VTABLE_CTOR( WebAssemblyTable, DynamicObject );
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT( WebAssemblyTable );
#ifdef ENABLE_WASM
    public:
        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo GetterLength;
            static FunctionInfo Grow;
            static FunctionInfo Get;
            static FunctionInfo Set;
        };
        WebAssemblyTable(Var * values, uint32 currentLength, uint32 initialLength, uint32 maxLength, DynamicType * type);
        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterLength(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGrow(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGet(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySet(RecyclableObject* function, CallInfo callInfo, ...);

        static bool Is(Var aValue);
        static WebAssemblyTable * FromVar(Var aValue);

        static WebAssemblyTable * Create(uint32 initial, uint32 maximum, ScriptContext * scriptContext);

        uint32 GetCurrentLength() const;
        uint32 GetInitialLength() const;
        uint32 GetMaximumLength() const;

        void DirectSetValue(uint index, Var val);
        Var DirectGetValue(uint index) const;

        static uint32 GetOffsetOfValues() { return offsetof(WebAssemblyTable, m_values); }
        static uint32 GetOffsetOfCurrentLength() { return offsetof(WebAssemblyTable, m_currentLength); }
    private:
        Field(uint32) m_initialLength;
        Field(uint32) m_maxLength;

        Field(uint32) m_currentLength;
        Field(Field(Var)*) m_values;
#endif
    };
}
