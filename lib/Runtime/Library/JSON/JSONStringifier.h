//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{

struct JSONObjectStack
{
    RecyclableObject* object;
    JSONObjectStack* next;

    bool Has(_In_ RecyclableObject* object)
    {
        JSONObjectStack* stack = this;
        while (stack != nullptr)
        {
            if (stack->object == object)
            {
                return true;
            }
            stack = stack->next;
        }
        return false;
    }
};


class JSONStringifier
{
private:
    // Spec defined limit on the gap length
    static const charcount_t MaxGapLength;

    struct PropertyListElement
    {
        Field(PropertyRecord const*) propertyRecord;
        Field(JavascriptString*) propertyName;

        PropertyListElement() {}
        PropertyListElement(const PropertyListElement& other)
            : propertyRecord(other.propertyRecord), propertyName(other.propertyName)
        {}
    };

    typedef SList<PropertyListElement, Recycler> PropertyList;

    ScriptContext* scriptContext;
    RecyclableObject* replacerFunction;
    PropertyList* propertyList;
    charcount_t totalStringLength;
    charcount_t indentLength;
    charcount_t gapLength;
    char16* gap;

    Var TryConvertPrimitiveObject(_In_ RecyclableObject* value);
    Var ToJSON(_In_ JavascriptString* key, _In_ RecyclableObject* valueObject);
    Var CallReplacerFunction(_In_opt_ RecyclableObject* holder, _In_ JavascriptString* key, _In_ Var value);
    _Ret_notnull_ Var ReadValue(_In_ JavascriptString* key, _In_opt_ const PropertyRecord* propertyRecord, _In_ RecyclableObject* holder);
    uint32 ReadArrayLength(_In_ RecyclableObject* value);
    JSONArray* ReadArray(_In_ RecyclableObject* arr, _In_ JSONObjectStack* objectStack);

    void AppendObjectElement(
        _In_ JavascriptString* propertyName,
        _In_ JSONObject* jsonObject,
        _In_ JSONObjectProperty* prop);

    void ReadObjectElement(
        _In_ JavascriptString* propertyName,
        _In_ uint32 numericIndex,
        _In_ RecyclableObject* obj,
        _In_ JSONObject* jsonObject,
        _In_ JSONObjectStack* objectStack);

    void ReadObjectElement(
        _In_ JavascriptString* propertyName,
        _In_opt_ PropertyRecord const* propertyRecord,
        _In_ RecyclableObject* obj,
        _In_ JSONObject* jsonObject,
        _In_ JSONObjectStack* objectStack);

    void ReadArrayElement(
        uint32 index,
        _In_ RecyclableObject* arr,
        _Out_ JSONProperty* prop,
        _In_ JSONObjectStack* objectStack);

    void CalculateStringifiedLength(uint32 propertyCount, charcount_t stepbackLength);
    void ReadProxy(_In_ JavascriptProxy* proxyObject, _In_ JSONObject* jsonObject, _In_ JSONObjectStack* stack);
    JSONObject* ReadObject(_In_ RecyclableObject* obj, _In_ JSONObjectStack* objectStack);
    void SetNullProperty(_Out_ JSONProperty* prop);
    void SetNumericProperty(double value, _In_ Var valueVar, _Out_ JSONProperty* prop);
    static charcount_t CalculateStringElementLength(_In_ JavascriptString* str);
    void ReadData(_In_ RecyclableObject* valueObj, _Out_ JSONProperty* prop);
    void ReadData(_In_ Var value, _Out_ JSONProperty* prop);
    void SetStringGap(_In_ JavascriptString* spaceString);
    void SetNumericGap(charcount_t spaceCount);
    void AddToPropertyList(_In_ Var item, _Inout_ BVSparse<Recycler>* propertyBV);
public:
    JSONStringifier(_In_ ScriptContext* scriptContext);
    void ReadSpace(_In_opt_ Var space);
    void ReadReplacer(_In_opt_ Var replacer);
    void ReadProperty(
        _In_ JavascriptString* key,
        _In_opt_ RecyclableObject* holder,
        _Out_ JSONProperty* prop,
        _In_ Var value,
        _In_ JSONObjectStack* objectStack);

    const char16* GetGap() const { return this->gap; };
    charcount_t GetGapLength() const { return this->gapLength; }
    bool HasReplacerFunction() const { return this->replacerFunction != nullptr; }

    bool HasComplexGap() const;

    static LazyJSONString* Stringify(_In_ ScriptContext* scriptContext, _In_ Var value, _In_opt_ Var replacer, _In_opt_ Var space);

}; // class JSONStringifier

} //namespace Js
