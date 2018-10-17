//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
struct JSONObjectProperty;
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

typedef SListCounted<JSONObjectProperty, Recycler> JSONObject;


struct JSONNumberData
{
    Field(Var) value;
    Field(JavascriptString*) string;
};


struct JSONProperty
{
    Field(JSONContentType) type;
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
    JSONProperty(const JSONProperty& other)
    {
        // Copy the full struct and use "Field(Var)" to identify write barrier
        // policy as the struct contains Vars
        CopyArray<JSONProperty, Field(Var)>(this, 1, &other, 1);
    }
};

struct JSONObjectProperty
{
    Field(JavascriptString*) propertyName;
    Field(JSONProperty) propertyValue;

    JSONObjectProperty() : propertyName(nullptr), propertyValue()
    {
    }
    JSONObjectProperty(const JSONObjectProperty& other) :
        propertyName(other.propertyName),
        propertyValue(other.propertyValue)
    {
    }
};

struct JSONArray
{
    Field(uint32) length;
    Field(JSONProperty) arr[];
};

class LazyJSONString : public JavascriptString
{
private:
    Field(charcount_t) gapLength;
    Field(JSONProperty*) jsonContent;
    Field(const char16*) gap;

    DynamicObject* ReconstructObject(_In_ JSONObject* valueList) const;
    JavascriptArray* ReconstructArray(_In_ JSONArray* valueArray) const;
    Var ReconstructVar(_In_ JSONProperty* content) const;


    static const WCHAR escapeMap[128];
public:
    static const BYTE escapeMapCount[128];

protected:
    DEFINE_VTABLE_CTOR(LazyJSONString, JavascriptString);

public:
    LazyJSONString(_In_ JSONProperty* content, charcount_t length, _In_opt_ const char16* gap, charcount_t gapLength, _In_ StaticType* type);
    Var TryParse() const;

    // Tells if the string has a gap with characters that might impact JSON.parse
    bool HasComplexGap() const;

    const char16* GetSz() override sealed;

    virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
    {
        return VTableValue::VtableLazyJSONString;
    }

}; // class LazyJSONString

template <> bool VarIsImpl<LazyJSONString>(RecyclableObject* obj);

} // namespace Js
