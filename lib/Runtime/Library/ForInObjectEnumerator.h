//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ForInObjectEnumerator : public WeakReferenceCache<ForInObjectEnumerator>
    {
    private:
        JavascriptStaticEnumerator enumerator;
        RecyclableObject *object;
        RecyclableObject *baseObject;
        BVSparse<Recycler>* propertyIds;
        RecyclableObject *firstPrototype;
        Var currentIndex;
        Type* baseObjectType;
        SListBase<Js::PropertyRecord const *> newPropertyStrings;
        ScriptContext * scriptContext;
        bool enumSymbols;

        BOOL TestAndSetEnumerated(PropertyId propertyId);
        BOOL InitializeCurrentEnumerator();

        // Only used by the vtable ctor for ForInObjectEnumeratorWrapper
        friend class ForInObjectEnumeratorWrapper;
        ForInObjectEnumerator() { }

    public:
        ForInObjectEnumerator(RecyclableObject* object, ScriptContext * requestContext, bool enumSymbols = false);
        ~ForInObjectEnumerator() { Clear(); }

        ScriptContext * GetScriptContext() const { return scriptContext; }
        BOOL CanBeReused();
        void Initialize(RecyclableObject* currentObject, ScriptContext * scriptContext);
        void Clear();
        Var GetCurrentIndex();
        BOOL MoveNext();
        void Reset();
        Var MoveAndGetNext(PropertyId& propertyId);

        static uint32 GetOffsetOfFirstPrototype() { return offsetof(ForInObjectEnumerator, firstPrototype); }
        static uint32 GetOffsetOfEnumeratorCurrentEnumerator() { return offsetof(ForInObjectEnumerator, enumerator) + JavascriptStaticEnumerator::GetOffsetOfCurrentEnumerator(); }
        static uint32 GetOffsetOfEnumeratorObject() { return offsetof(ForInObjectEnumerator, enumerator) + JavascriptStaticEnumerator::GetOffsetOfObject(); }
        static uint32 GetOffsetOfEnumeratorCachedDataType() { return offsetof(ForInObjectEnumerator, enumerator) + JavascriptStaticEnumerator::GetOffsetOfCachedDataType(); }
        static uint32 GetOffsetOfEnumeratorEnumeratedCount() { return offsetof(ForInObjectEnumerator, enumerator) + JavascriptStaticEnumerator::GetOffsetOfEnumeratedCount(); }
        static uint32 GetOffsetOfEnumeratorCachedData() { return offsetof(ForInObjectEnumerator, enumerator) + JavascriptStaticEnumerator::GetOffsetOfCachedData(); }     
        static uint32 GetOffsetOfEnumeratorObjectIndex() { return offsetof(ForInObjectEnumerator, enumerator) + JavascriptStaticEnumerator::GetOffsetOfObjectIndex(); }
        static RecyclableObject* GetFirstPrototypeWithEnumerableProperties(RecyclableObject* object);
    };

    // Use when we want to use the ForInObject as if they are normal javascript enumerator
    class ForInObjectEnumeratorWrapper : public JavascriptEnumerator
    {
    public:
        ForInObjectEnumeratorWrapper(RecyclableObject* object, ScriptContext * requestContext)
            : JavascriptEnumerator(requestContext), forInObjectEnumerator(object, requestContext)
        {
        }

        virtual void Reset() override { forInObjectEnumerator.Reset(); }
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr)
        {
            return forInObjectEnumerator.MoveAndGetNext(propertyId);
        }
    protected:
        DEFINE_VTABLE_CTOR(ForInObjectEnumeratorWrapper, JavascriptEnumerator);

    private:
        ForInObjectEnumerator forInObjectEnumerator;
    };
}
