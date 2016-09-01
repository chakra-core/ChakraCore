//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class DynamicObjectPropertyEnumerator
    {
    private:
        ScriptContext * requestContext;
        DynamicObject * object;
        DynamicType * initialType;              // for snapshot enumeration
        BigPropertyIndex objectIndex;
        BigPropertyIndex initialPropertyCount;
        int enumeratedCount;
        
        EnumeratorFlags flags;

        DynamicType * cachedDataType;           // for cached type check
        struct CachedData
        {
            ScriptContext * scriptContext;
            PropertyString ** strings;
            BigPropertyIndex * indexes;
            PropertyAttributes * attributes;
            int cachedCount;
            bool completed;
            bool enumNonEnumerable;
            bool enumSymbols;
        } *cachedData;

        DynamicType * GetTypeToEnumerate() const;
        JavascriptString * MoveAndGetNextWithCache(PropertyId& propertyId, PropertyAttributes* attributes);
        JavascriptString * MoveAndGetNextNoCache(PropertyId& propertyId, PropertyAttributes * attributes);


    public:
        DynamicObject * GetObject() const { return object; }
        EnumeratorFlags GetFlags() const { return flags; }     
        bool GetEnumNonEnumerable() const;
        bool GetEnumSymbols() const;
        bool GetSnapShotSemantics() const;
        ScriptContext * GetRequestContext() { return requestContext; }

        bool Initialize(DynamicObject * object, EnumeratorFlags flags, ScriptContext * requestContext);
        bool IsNullEnumerator() const;
        void Reset();
        void Clear();
        Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes * attributes);

        static uint32 GetOffsetOfCachedDataType() { return offsetof(DynamicObjectPropertyEnumerator, cachedDataType); }
        static uint32 GetOffsetOfObject() { return offsetof(DynamicObjectPropertyEnumerator, object); }
        static uint32 GetOffsetOfObjectIndex() { return offsetof(DynamicObjectPropertyEnumerator, objectIndex); }

        static uint32 GetOffsetOfEnumeratedCount() { return offsetof(DynamicObjectPropertyEnumerator, enumeratedCount); }
        static uint32 GetOffsetOfCachedData() { return offsetof(DynamicObjectPropertyEnumerator, cachedData); }

        static uint32 GetOffsetOfCachedDataStrings() { return offsetof(CachedData, strings); }
        static uint32 GetOffsetOfCachedDataIndexes() { return offsetof(CachedData, indexes); }
        static uint32 GetOffsetOfCachedDataCachedCount() { return offsetof(CachedData, cachedCount); }
        static uint32 GetOffsetOfCachedDataPropertyAttributes() { return offsetof(CachedData, attributes); }
    };
};