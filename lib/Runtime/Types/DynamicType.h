//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class DynamicType : public Type
    {
#if DBG
        friend class JavascriptFunction;
#endif
        friend class DynamicObject;
        friend class DynamicTypeHandler;
        friend class CrossSite;
        friend class TypePath;
        friend class PathTypeHandlerBase;
        friend class SimplePathTypeHandler;
        friend class PathTypeHandler;
        friend class ES5ArrayType;
        friend class JavascriptOperators;

        template <typename TPropertyIndex, typename TMapKey, bool IsNotExtensibleSupported>
        friend class SimpleDictionaryTypeHandlerBase;

    private:
        Field(DynamicTypeHandler *) typeHandler;
        Field(bool) isLocked;
        Field(bool) isShared;
        Field(bool) hasNoEnumerableProperties;
#if DBG
        Field(bool) isCachedForChangePrototype;
#endif

    protected:
        DynamicType(DynamicType * type) : Type(type), typeHandler(type->typeHandler), isLocked(false), isShared(false) {}
        DynamicType(DynamicType * type, DynamicTypeHandler *typeHandler, bool isLocked, bool isShared);
        DynamicType(ScriptContext* scriptContext, TypeId typeId, RecyclableObject* prototype, JavascriptMethod entryPoint, DynamicTypeHandler * typeHandler, bool isLocked, bool isShared);

    public:
        DynamicTypeHandler * GetTypeHandler() const { return typeHandler; }

        void SetPrototype(RecyclableObject* newPrototype) { this->prototype = newPrototype; }
        bool GetIsLocked() const { return this->isLocked; }
        bool GetIsShared() const { return this->isShared; }
#if DBG
        bool GetIsCachedForChangePrototype() const { return this->isCachedForChangePrototype; }
        void SetIsCachedForChangePrototype() { this->isCachedForChangePrototype = true; }
#endif
        void SetEntryPoint(JavascriptMethod method) { entryPoint = method; }

        BOOL AllPropertiesAreEnumerable() { return typeHandler->AllPropertiesAreEnumerable(); }

        bool LockType();
        bool ShareType();
        bool LockTypeOnly();

        bool GetHasNoEnumerableProperties() const { return hasNoEnumerableProperties; }
        bool SetHasNoEnumerableProperties(bool value);
        bool PrepareForTypeSnapshotEnumeration();

        static bool Is(TypeId typeId);
        static DynamicType * New(ScriptContext* scriptContext, TypeId typeId, RecyclableObject* prototype, JavascriptMethod entryPoint, DynamicTypeHandler * typeHandler, bool isLocked = false, bool isShared = false);

        static uint32 GetOffsetOfTypeHandler() { return offsetof(DynamicType, typeHandler); }
        static uint32 GetOffsetOfIsShared() { return offsetof(DynamicType, isShared); }
        static uint32 GetOffsetOfHasNoEnumerableProperties() { return offsetof(DynamicType, hasNoEnumerableProperties); }
    private:
        void SetIsLocked() { Assert(this->GetTypeHandler()->GetIsLocked()); this->isLocked = true; }
        void SetIsShared() { Assert(this->GetIsLocked() && this->GetTypeHandler()->GetIsShared()); this->isShared = true; }
        void SetIsLockedAndShared() { SetIsLocked(); SetIsShared(); }

    };
};
