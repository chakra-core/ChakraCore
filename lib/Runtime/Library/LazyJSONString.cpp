//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

namespace Js
{

LazyJSONString::LazyJSONString(_In_ JSONProperty* jsonContent, charcount_t length, _In_opt_ const char16* gap, charcount_t gapLength, _In_ StaticType* type) :
    JavascriptString(type),
    jsonContent(jsonContent),
    gap(gap),
    gapLength(gapLength)
{
    // Use SetLength to ensure length is valid
    SetLength(length);
}

DynamicObject*
LazyJSONString::ParseObject(_In_ JSONObject* valueList) const
{
    const uint elementCount = valueList->Count();
    PropertyIndex requestedInlineSlotCapacity = static_cast<PropertyIndex>(elementCount);
    DynamicObject* obj = this->GetLibrary()->CreateObject(true, requestedInlineSlotCapacity);
    FOREACH_DLISTCOUNTED_ENTRY(JSONProperty, Recycler, entry, valueList)
    {
        Var propertyValue = Parse(&entry);
        PropertyValueInfo info;
        PropertyString* propertyString = PropertyString::TryFromVar(entry.propertyName);
        if (!propertyString || !propertyString->TrySetPropertyFromCache(obj, propertyValue, this->GetScriptContext(), PropertyOperation_None, &info))
        {
            JavascriptOperators::SetProperty(obj, obj, entry.propertyRecord->GetPropertyId(), propertyValue, &info, this->GetScriptContext());
        }
    }
    NEXT_DLISTCOUNTED_ENTRY;
    return obj;
}

JavascriptArray*
LazyJSONString::ParseArray(_In_ JSONArray* jsonArray) const
{
    const uint32 length = jsonArray->length;
    JavascriptArray* arr = this->GetLibrary()->CreateArrayLiteral(length);
    JSONProperty* prop = jsonArray->arr;
    for (uint i = 0; i < length; ++i)
    {
        Var element = Parse(&prop[i]);
        BOOL result = arr->SetItem(i, element, PropertyOperation_None);
        Assert(result); // Setting item in an array we just allocated should always succeed
    }
    return arr;
}

Var
LazyJSONString::Parse(_In_ JSONProperty* prop) const
{
    switch (prop->type)
    {
    case JSONContentType::Array:
        return ParseArray(prop->arr);
    case JSONContentType::Null:
        return this->GetLibrary()->GetNull();
    case JSONContentType::True:
        return this->GetLibrary()->GetTrue();
    case JSONContentType::False:
        return this->GetLibrary()->GetFalse();
    case JSONContentType::Number:
        return prop->numericValue.value;
    case JSONContentType::String:
        return prop->stringValue;
    case JSONContentType::Object:
        return ParseObject(prop->obj);
    default:
        Assume(UNREACHED);
        return nullptr;
    }
}

Var
LazyJSONString::Parse() const
{
    // If the gap is a non-space character, then parsing will be non-trivial transformation
    for (charcount_t i = 0; i < this->gapLength; ++i)
    {
        switch (this->gap[i])
        {
        // This is not exhaustive, just a useful subset of semantics preserving characters
        case _u(' '):
        case _u('\t'):
        case _u('\n'):
            continue;
        default:
            return nullptr;
        }
    }
    return Parse(this->jsonContent);
}

const char16*
LazyJSONString::GetSz()
{
    const charcount_t allocSize = SafeSzSize();

    Recycler* recycler = GetScriptContext()->GetRecycler();
    char16* target = RecyclerNewArrayLeaf(recycler, char16, allocSize);

    JSONStringBuilder builder(
        this->GetScriptContext(),
        this->jsonContent,
        target,
        allocSize,
        this->gap,
        this->gapLength);

    builder.Build();

    SetBuffer(target);
    return target;
}

// static
bool
LazyJSONString::Is(Var var)
{
    return RecyclableObject::Is(var) && VirtualTableInfo<LazyJSONString>::HasVirtualTable(RecyclableObject::FromVar(var));
}

// static
LazyJSONString*
LazyJSONString::TryFromVar(Var var)
{
    return LazyJSONString::Is(var)
        ? reinterpret_cast<LazyJSONString*>(var)
        : nullptr;
}

} // namespace Js
