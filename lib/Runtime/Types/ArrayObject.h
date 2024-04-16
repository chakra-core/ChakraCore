//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
#ifdef TARGET_64
// This base class has a 4-byte length field. Change struct pack to 4 on 64bit to avoid 4 padding bytes here.
#pragma pack(push, 4)
#endif

    //
    // A base class for all array-like objects types, that can serve as an Object's internal array:
    // JavascriptArray/ES5Array or TypedArray.
    //
    class ArrayObject : public DynamicObject
    {
    protected:
        Field(uint32) length;

    protected:
        DEFINE_VTABLE_CTOR_ABSTRACT(ArrayObject, DynamicObject);

        ArrayObject(DynamicType * type, bool initSlots = true, uint32 length = 0)
            : DynamicObject(type, initSlots), length(length)
        {
        }

        ArrayObject(DynamicType * type, ScriptContext * scriptContext)
            : DynamicObject(type, scriptContext), length(0)
        {
        }

        // For boxing stack instance
        ArrayObject(ArrayObject * instance, bool deepCopy);

        void DECLSPEC_NORETURN ThrowItemNotConfigurableError(PropertyId propId = Constants::NoProperty);
        void VerifySetItemAttributes(PropertyId propId, PropertyAttributes attributes);

    public:
        uint32 GetLength() const { return length; }
        static uint32 GetOffsetOfLength() { return offsetof(ArrayObject, length); }

        // objectArray support
        virtual BOOL SetItemWithAttributes(uint32 index, Var value, PropertyAttributes attributes) = 0;
        virtual BOOL SetItemAttributes(uint32 index, PropertyAttributes attributes);
        virtual BOOL SetItemAccessors(uint32 index, Var getter, Var setter);
        virtual BOOL IsObjectArrayFrozen();
        virtual JavascriptEnumerator * GetIndexEnumerator(EnumeratorFlags flags, ScriptContext* requestContext) = 0;
    };

#ifdef TARGET_64
#pragma pack(pop)
#endif

} // namespace Js
