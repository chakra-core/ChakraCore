//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptAsyncFromSyncIterator : public DynamicObject
    {
    public:
        JavascriptAsyncFromSyncIterator(DynamicType* type, RecyclableObject* syncIterator) :
            DynamicObject(type), syncIterator(syncIterator) { }

        RecyclableObject* GetSyncIterator() { return this->syncIterator; }
        RecyclableObject* JavascriptAsyncFromSyncIterator::EnsureSyncNextFunction(ScriptContext* scriptContext);

        static Var AsyncFromSyncIteratorContinuation(RecyclableObject* result, ScriptContext* scriptContext);
        static Var EntryAsyncFromSyncIteratorNext(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncFromSyncIteratorThrow(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncFromSyncIteratorReturn(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryAsyncFromSyncIteratorValueUnwrapFalseFunction(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAsyncFromSyncIteratorValueUnwrapTrueFunction(RecyclableObject* function, CallInfo callInfo, ...);

        class EntryInfo
        {
        public:
            static FunctionInfo Next;
            static FunctionInfo Return;
            static FunctionInfo Throw;
        };

    private:
        Field(RecyclableObject*) syncIterator;
        Field(RecyclableObject*) syncNextFunction;
    protected:
        DEFINE_VTABLE_CTOR(JavascriptAsyncFromSyncIterator, DynamicObject);
    };

    template <> bool VarIsImpl<JavascriptAsyncFromSyncIterator>(RecyclableObject* obj);

} // namespace Js
