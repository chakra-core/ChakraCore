//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptRegistry : public JavascriptMap
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptRegistry, JavascriptMap);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptRegistry);

    public:
        JavascriptRegistry(DynamicType* type);

        static JavascriptRegistry* New(ScriptContext* scriptContext);

        static bool Is(Var aValue);
        static JavascriptRegistry* FromVar(Var aValue);

        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Delete;
            static FunctionInfo Get;
            static FunctionInfo Has;
            static FunctionInfo Set;
            static FunctionInfo Entries;
            static FunctionInfo Keys;
            static FunctionInfo Values;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryDelete(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGet(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryHas(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySet(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEntries(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryKeys(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryValues(RecyclableObject* function, CallInfo callInfo, ...);
    };
}
