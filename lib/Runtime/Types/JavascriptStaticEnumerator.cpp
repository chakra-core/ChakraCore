//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"
#include "Library/JavascriptSymbol.h"

namespace Js
{

    bool JavascriptStaticEnumerator::Initialize(JavascriptEnumerator * prefixEnumerator, ArrayObject * arrayToEnumerate,
        DynamicObject * objectToEnumerate, EnumeratorFlags flags, ScriptContext * requestContext, ForInCache * forInCache)
    {
        this->prefixEnumerator = prefixEnumerator;
        this->arrayEnumerator = arrayToEnumerate ? arrayToEnumerate->GetIndexEnumerator(flags, requestContext) : nullptr;
        this->currentEnumerator = prefixEnumerator ? prefixEnumerator : PointerValue(arrayEnumerator);
        return this->propertyEnumerator.Initialize(objectToEnumerate, flags, requestContext, forInCache);
    }

    void JavascriptStaticEnumerator::Clear(EnumeratorFlags flags, ScriptContext * requestContext)
    {
        this->prefixEnumerator = nullptr;
        this->arrayEnumerator = nullptr;
        this->currentEnumerator = nullptr;
        this->propertyEnumerator.Clear(flags, requestContext);
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

    bool JavascriptStaticEnumerator::CanUseJITFastPath() const
    {
        return this->propertyEnumerator.CanUseJITFastPath() && this->currentEnumerator == nullptr;
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

    JavascriptString * JavascriptStaticEnumerator::MoveAndGetNextFromEnumerator(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        while (this->currentEnumerator)
        {
            JavascriptString * currentIndex = this->currentEnumerator->MoveAndGetNext(propertyId, attributes);
            if (currentIndex != nullptr)
            {
                return currentIndex;
            }
            this->currentEnumerator = (this->currentEnumerator == this->prefixEnumerator) ? this->arrayEnumerator : nullptr;
        }

        return nullptr;
    }

    JavascriptString * JavascriptStaticEnumerator::MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes)
    {
        JavascriptString * currentIndex = MoveAndGetNextFromEnumerator(propertyId, attributes);
        if (currentIndex == nullptr)
        {
            currentIndex = propertyEnumerator.MoveAndGetNext(propertyId, attributes);
        }
        Assert(!currentIndex || !CrossSite::NeedMarshalVar(currentIndex, this->propertyEnumerator.GetScriptContext()));
        return currentIndex;
    }
}
