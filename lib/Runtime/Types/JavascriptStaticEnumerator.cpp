//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"
#include "Library/JavascriptSymbol.h"

namespace Js
{

    bool JavascriptStaticEnumerator::Initialize(JavascriptEnumerator * prefixEnumerator, ArrayObject * arrayToEnumerate, 
        DynamicObject * objectToEnumerate, EnumeratorFlags flags, ScriptContext * requestContext)
    {
        this->prefixEnumerator = prefixEnumerator;
        this->arrayEnumerator = arrayToEnumerate ? arrayToEnumerate->GetIndexEnumerator(flags, requestContext) : nullptr;
        this->currentEnumerator = prefixEnumerator ? prefixEnumerator : arrayEnumerator;
        return this->propertyEnumerator.Initialize(objectToEnumerate, flags, requestContext);
    }

    void JavascriptStaticEnumerator::Clear()
    {
        this->prefixEnumerator = nullptr;
        this->arrayEnumerator = nullptr;
        this->currentEnumerator = nullptr;
        this->propertyEnumerator.Clear();
    }

    void JavascriptStaticEnumerator::Reset()
    {
        if (this->prefixEnumerator)
        {
            this->prefixEnumerator->Reset();
            this->currentEnumerator = this->prefixEnumerator;
            if (this->arrayEnumerator)
            {
                this->arrayEnumerator->Reset();
            }
        }
        else if (this->arrayEnumerator)
        {
            this->currentEnumerator = this->arrayEnumerator;
            this->arrayEnumerator->Reset();
        }
        this->propertyEnumerator.Reset();
    }

    bool JavascriptStaticEnumerator::IsNullEnumerator() const
    {
        return this->prefixEnumerator == nullptr && this->arrayEnumerator == nullptr && this->propertyEnumerator.IsNullEnumerator();
    }

    uint32 JavascriptStaticEnumerator::GetCurrentItemIndex()
    {
        if (currentEnumerator)
        {
            return currentEnumerator->GetCurrentItemIndex();
        }
        else
        {
            return JavascriptArray::InvalidIndex;
        }
    }

    Var JavascriptStaticEnumerator::MoveAndGetNextFromEnumerator(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        while (this->currentEnumerator)
        {
            Var currentIndex = this->currentEnumerator->MoveAndGetNext(propertyId, attributes);
            if (currentIndex != nullptr)
            {
                return currentIndex;
            }
            this->currentEnumerator = (this->currentEnumerator == this->prefixEnumerator) ? this->arrayEnumerator : nullptr;
        }

        return nullptr;
    }

    Var JavascriptStaticEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        Var currentIndex = MoveAndGetNextFromEnumerator(propertyId, attributes);
        if (currentIndex == nullptr)
        {
            currentIndex = propertyEnumerator.MoveAndGetNext(propertyId, attributes);
        }
        Assert(!currentIndex || !CrossSite::NeedMarshalVar(currentIndex, this->propertyEnumerator.GetRequestContext()));
        Assert(!currentIndex || JavascriptString::Is(currentIndex) || (this->propertyEnumerator.GetEnumSymbols() && JavascriptSymbol::Is(currentIndex)));
        return currentIndex;
    }
}
