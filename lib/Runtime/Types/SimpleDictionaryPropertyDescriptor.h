//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    template <typename TPropertyIndex>
    class SimpleDictionaryPropertyDescriptor
    {
    public:
        SimpleDictionaryPropertyDescriptor() :
#if ENABLE_FIXED_FIELDS
            preventFalseReference(true), isInitialized(false), isFixed(false), usedAsFixed(false),
#endif
            propertyIndex(NoSlots), Attributes(PropertyDynamicTypeDefaults) {}

        SimpleDictionaryPropertyDescriptor(TPropertyIndex inPropertyIndex) :
#if ENABLE_FIXED_FIELDS
            preventFalseReference(true), isInitialized(false), isFixed(false), usedAsFixed(false),
#endif
            propertyIndex(inPropertyIndex), Attributes(PropertyDynamicTypeDefaults) {}
            

        SimpleDictionaryPropertyDescriptor(TPropertyIndex inPropertyIndex, PropertyAttributes attributes) :
#if ENABLE_FIXED_FIELDS
            preventFalseReference(true), isInitialized(false), isFixed(false), usedAsFixed(false),
#endif
            propertyIndex(inPropertyIndex), Attributes(attributes) {}


#if ENABLE_FIXED_FIELDS
        // SimpleDictionaryPropertyDescriptor is allocated by a dictionary along with the PropertyRecord
        // so it can not allocate as leaf, tag the lower bit to prevent false reference
        bool preventFalseReference:1;
        bool isInitialized: 1;
        bool isFixed:1;
        bool usedAsFixed:1;
#endif

        PropertyAttributes Attributes;
        TPropertyIndex propertyIndex;

        bool HasNonLetConstGlobal() const
        {
            return (this->Attributes & PropertyLetConstGlobal) == 0;
        }

        bool IsOrMayBecomeFixed() const
        {
#if ENABLE_FIXED_FIELDS
            return !isInitialized || isFixed;
#else
            return false;
#endif
        }
     private:
        static const TPropertyIndex NoSlots = PropertyIndexRanges<TPropertyIndex>::NoSlots;
    };
}

namespace JsUtil
{
    template <typename TPropertyIndex>
    class ValueEntry<Js::SimpleDictionaryPropertyDescriptor<TPropertyIndex> >: public BaseValueEntry<Js::SimpleDictionaryPropertyDescriptor<TPropertyIndex>>
    {
    public:
        void Clear()
        {
            this->value = 0;
        }
    };
}
