//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{

struct JSONProperty;
struct JSONArray;

enum class JSONContentType : uint8
{
    Array,
    Object,
    Undefined,
    Null,
    True,
    False,
    Number,
    String
};

typedef DListCounted<JSONProperty, Recycler> JSONObject;

struct JSONNumberData
{
    Field(Var) value;
    Field(JavascriptString*) string;
};

struct JSONProperty
{
    Field(JSONContentType) type;
    Field(const PropertyRecord*) propertyRecord;
    Field(JavascriptString*) propertyName;
    union
    {
        Field(JSONNumberData) numericValue;
        Field(JavascriptString*) stringValue;
        Field(JSONObject*) obj;
        Field(JSONArray*) arr;
    };

    JSONProperty()
    {
        memset(this, 0, sizeof(JSONProperty));
    }
};

struct JSONArray
{
    Field(uint32) length;
    Field(JSONProperty) arr[];
};

class LazyJSONString : JavascriptString
{
private:
    Field(charcount_t) gapLength;
    Field(JSONProperty*) jsonContent;
    Field(const char16*) gap;

    DynamicObject* ParseObject(_In_ JSONObject* valueList) const;
    JavascriptArray* ParseArray(_In_ JSONArray* valueArray) const;
    Var Parse(_In_ JSONProperty* content) const;

protected:
    DEFINE_VTABLE_CTOR(LazyJSONString, JavascriptString);

public:
    LazyJSONString(_In_ JSONProperty* content, charcount_t length, _In_opt_ const char16* gap, charcount_t gapLength, _In_ StaticType* type);
    Var Parse() const;

    const char16* GetSz() override sealed;

    static bool Is(Var var);

    static LazyJSONString* TryFromVar(Var var);

    virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableLazyJSONString;
    }
}; // class LazyJSONString

} // namespace Js
