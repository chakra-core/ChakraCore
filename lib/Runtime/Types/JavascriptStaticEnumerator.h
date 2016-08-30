//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    enum class EnumeratorFlags : byte
    {
        None                = 0x0,
        EnumNonEnumerable   = 0x1,
        EnumSymbols         = 0x2,
        SnapShotSemantics   = 0x4
    };
    ENUM_CLASS_HELPERS(EnumeratorFlags, byte);

    class JavascriptStaticEnumerator 
    {
    protected:
        friend class ForInObjectEnumerator;
        DynamicObjectPropertyEnumerator propertyEnumerator;
        JavascriptEnumerator* currentEnumerator;
        JavascriptEnumerator* prefixEnumerator;
        JavascriptEnumerator* arrayEnumerator;

        Var MoveAndGetNextFromEnumerator(PropertyId& propertyId, PropertyAttributes* attributes);
    public:
        JavascriptStaticEnumerator() { Clear(); }
        bool Initialize(JavascriptEnumerator * prefixEnumerator, ArrayObject * arrayToEnumerate, DynamicObject* objectToEnumerate, EnumeratorFlags flags, ScriptContext * requestContext);
        bool IsNullEnumerator() const;
        void Clear();
        void Reset();
        uint32 GetCurrentItemIndex();
        Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr);

        static uint32 GetOffsetOfCurrentEnumerator() { return offsetof(JavascriptStaticEnumerator, currentEnumerator); }

        static uint32 GetOffsetOfCachedDataType() { return offsetof(JavascriptStaticEnumerator, propertyEnumerator) + DynamicObjectPropertyEnumerator::GetOffsetOfCachedDataType(); }
        static uint32 GetOffsetOfObject() { return offsetof(JavascriptStaticEnumerator, propertyEnumerator) + DynamicObjectPropertyEnumerator::GetOffsetOfObject(); }
        static uint32 GetOffsetOfObjectIndex() { return offsetof(JavascriptStaticEnumerator, propertyEnumerator) + DynamicObjectPropertyEnumerator::GetOffsetOfObjectIndex(); }

        static uint32 GetOffsetOfEnumeratedCount() { return offsetof(JavascriptStaticEnumerator, propertyEnumerator) + DynamicObjectPropertyEnumerator::GetOffsetOfEnumeratedCount(); }
        static uint32 GetOffsetOfCachedData() { return offsetof(JavascriptStaticEnumerator, propertyEnumerator) + DynamicObjectPropertyEnumerator::GetOffsetOfCachedData(); }
    };

    

} // namespace Js
