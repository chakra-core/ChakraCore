//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

namespace Js
{

const charcount_t JSONStringifier::MaxGapLength = 10;

JSONStringifier::JSONStringifier(_In_ ScriptContext* scriptContext) :
    scriptContext(scriptContext),
    replacerFunction(nullptr),
    totalStringLength(0),
    indentLength(0),
    gapLength(0),
    gap(nullptr),
    propertyList(nullptr)
{
}

void
JSONStringifier::SetStringGap(_In_ JavascriptString* spaceString)
{
    this->gapLength = min(MaxGapLength, spaceString->GetLength());
    if (this->gapLength != 0)
    {
        this->gap = RecyclerNewArrayLeaf(this->scriptContext->GetRecycler(), char16, this->gapLength);
        wmemcpy_s(gap, this->gapLength, spaceString->GetString(), this->gapLength);
    }
}

void
JSONStringifier::SetNumericGap(charcount_t spaceCount)
{
    this->gapLength = spaceCount;
    if (this->gapLength != 0)
    {
        this->gap = RecyclerNewArrayLeaf(this->scriptContext->GetRecycler(), char16, this->gapLength);
        wmemset(gap, _u(' '), this->gapLength);
    }
}

void
JSONStringifier::ReadSpace(_In_opt_ Var space)
{
    if (space != nullptr)
    {
        switch (JavascriptOperators::GetTypeId(space))
        {
        case TypeIds_Integer:
            this->SetNumericGap(static_cast<charcount_t>(max(0, min(static_cast<int>(MaxGapLength), TaggedInt::ToInt32(space)))));

            break;
        case TypeIds_Number:
        case TypeIds_NumberObject:
        case TypeIds_Int64Number:
        case TypeIds_UInt64Number:
        {
            double numericSpace = JavascriptConversion::ToInteger(space, this->scriptContext);
            if (numericSpace > 0)
            {
                SetNumericGap(numericSpace < static_cast<double>(MaxGapLength) ? static_cast<charcount_t>(numericSpace) : MaxGapLength);
            }
            break;
        }
        case TypeIds_String:
            this->SetStringGap(UnsafeVarTo<JavascriptString>(space));
            break;
        case TypeIds_StringObject:
            this->SetStringGap(JavascriptConversion::ToString(space, this->scriptContext));
            break;
        default:
            break;
        }
    }
}

void
JSONStringifier::AddToPropertyList(_In_ Var item, _Inout_ BVSparse<Recycler>* propertyBV)
{
    JavascriptString* propertyName = nullptr;
    switch (JavascriptOperators::GetTypeId(item))
    {
    case TypeIds_Integer:
        propertyName = this->scriptContext->GetIntegerString(item);
        break;
    case TypeIds_String:
        propertyName = UnsafeVarTo<JavascriptString>(item);
        break;
    case TypeIds_Number:
    case TypeIds_NumberObject:
    case TypeIds_Int64Number:
    case TypeIds_UInt64Number:
    case TypeIds_StringObject:
        propertyName = JavascriptConversion::ToString(item, this->scriptContext);
        break;
    default:
        break;
    }

    if (propertyName != nullptr)
    {
        PropertyRecord const* propertyRecord;
        scriptContext->GetOrAddPropertyRecord(propertyName, &propertyRecord);
        if(!propertyBV->TestAndSet(propertyRecord->GetPropertyId()))
        {
            PropertyListElement elem;
            elem.propertyName = propertyName;
            elem.propertyRecord = propertyRecord;
            this->propertyList->Push(elem);
        }
    }
}


void
JSONStringifier::ReadReplacer(_In_opt_ Var replacer)
{
    if (replacer != nullptr)
    {
        RecyclableObject* replacerObj = JavascriptOperators::TryFromVar<RecyclableObject>(replacer);
        if (replacerObj && JavascriptOperators::IsObject(replacerObj))
        {
            if (JavascriptConversion::IsCallable(replacerObj))
            {
                this->replacerFunction = replacerObj;
            }
            else if (JavascriptOperators::IsArray(replacerObj))
            {
                Recycler* recycler = this->scriptContext->GetRecycler();

                BVSparse<Recycler> propertyListBV(recycler);
                this->propertyList = RecyclerNew(recycler, PropertyList, recycler);
                JavascriptArray* propertyArray = JavascriptArray::TryVarToNonES5Array(replacer);
                if (propertyArray != nullptr)
                {
                    uint32 length = propertyArray->GetLength();
                    for (uint32 i = 0; i < length; i++)
                    {
                        Var item = propertyArray->DirectGetItem(i);
                        this->AddToPropertyList(item, &propertyListBV);
                    }
                }
                else
                {
                    int64 length = JavascriptConversion::ToLength(JavascriptOperators::OP_GetLength(replacerObj, this->scriptContext), this->scriptContext);
                    // ToLength always returns positive length
                    Assert(length >= 0);
                    for (uint64 i = 0; i < static_cast<uint64>(length); i++)
                    {
                        Var item = nullptr;
                        if (JavascriptOperators::GetItem(replacerObj, i, &item, scriptContext))
                        {
                            this->AddToPropertyList(item, &propertyListBV);
                        }
                    }
                }
                // PropertyList is an SList, which only has push/pop, so need to reverse list for it to be in order
                this->propertyList->Reverse();
            }
        }
    }
}

LazyJSONString*
JSONStringifier::Stringify(_In_ ScriptContext* scriptContext, _In_ Var value, _In_opt_ Var replacer, _In_opt_ Var space)
{
    Recycler* recycler = scriptContext->GetRecycler();
    JavascriptLibrary* library = scriptContext->GetLibrary();

    if (scriptContext->Cache()->toJSONCache == nullptr)
    {
        scriptContext->Cache()->toJSONCache = ScriptContextPolymorphicInlineCache::New(32, library);
    }

    JSONProperty* prop = RecyclerNewStruct(recycler, JSONProperty);
    JSONObjectStack objStack = { 0 };

    JSONStringifier stringifier(scriptContext);

    stringifier.ReadReplacer(replacer);
    stringifier.ReadSpace(space);

    DynamicObject* wrapper = nullptr;
    if (stringifier.HasReplacerFunction())
    {
        // ReplacerFunction takes wrapper object as a parameter, so we need to materialize it (otherwise it isn't needed)
        wrapper = library->CreateObject();
        PropertyId propertyId = scriptContext->GetEmptyStringPropertyId();
        JavascriptOperators::InitProperty(wrapper, propertyId, value);
    }

    stringifier.ReadProperty(
        library->GetEmptyString(),
        wrapper,
        prop,
        value,
        &objStack);

    if (prop->type == JSONContentType::Undefined)
    {
        return nullptr;
    }
    else
    {
        return RecyclerNew(
            recycler,
            LazyJSONString,
            prop,
            stringifier.totalStringLength,
            stringifier.GetGap(),
            stringifier.GetGapLength(),
            library->GetStringTypeStatic());
    }
}

_Ret_notnull_ Var
JSONStringifier::ReadValue(_In_ JavascriptString* key, _In_opt_ const PropertyRecord* propertyRecord, _In_ RecyclableObject* holder)
{
    Var value = nullptr;
    PropertyString* propertyString = JavascriptOperators::TryFromVar<PropertyString>(key);
    PropertyValueInfo info;
    if (propertyString != nullptr)
    {
        PropertyValueInfo::SetCacheInfo(&info, propertyString, propertyString->GetLdElemInlineCache(), false);
        if (propertyString->TryGetPropertyFromCache<false /* ownPropertyOnly */, false /* OutputExistence */>(holder, holder, &value, this->scriptContext, &info))
        {
            return value;
        }
    }

    if (propertyRecord == nullptr)
    {
        key->GetPropertyRecord(&propertyRecord);
    }
    JavascriptOperators::GetProperty(holder, propertyRecord->GetPropertyId(), &value, this->scriptContext, &info);
    return value;
}

// Convert NumberObject, StringObject, or BooleanObject to their primitive types,
// since stringification converts these to their primitive values.
// These types are used in cases where user does something like new Object("hello")
Var
JSONStringifier::TryConvertPrimitiveObject(_In_ RecyclableObject* value)
{
    TypeId id = JavascriptOperators::GetTypeId(value);
    if (TypeIds_NumberObject == id)
    {
        return JavascriptNumber::ToVarNoCheck(JavascriptConversion::ToNumber(value, this->scriptContext), this->scriptContext);
    }
    else if (TypeIds_StringObject == id)
    {
        return JavascriptConversion::ToString(value, this->scriptContext);
    }
    else if (TypeIds_BooleanObject == id)
    {
        return (UnsafeVarTo<JavascriptBooleanObject>(value)->GetValue() != FALSE)
            ? this->scriptContext->GetLibrary()->GetTrue()
            : this->scriptContext->GetLibrary()->GetFalse();
    }
    return nullptr;
}

Var
JSONStringifier::ToJSON(_In_ JavascriptString* key, _In_ RecyclableObject* valueObject)
{
    Var toJSON = nullptr;
    PolymorphicInlineCache* cache = this->scriptContext->Cache()->toJSONCache;
    PropertyValueInfo info;
    PropertyValueInfo::SetCacheInfo(&info, cache, false);

    // The vast majority of objects don't have custom toJSON, so check the missing cache first.
    // We can check all of the others afterward.
    if (CacheOperators::TryGetProperty<
        false,  // CheckLocal
        false,  // CheckProto
        false,  // CheckAccessor
        true,   // CheckMissing
        true,   // CheckPolymorphicInlineCache
        false,  // CheckTypePropertyCache
        false,  // IsInlineCacheAvailable
        true,   // IsPolymorphicInlineCacheAvailable
        false,  // ReturnOperationInfo
        false>  // OutputExistence
        (valueObject,
            false,
            valueObject,
            PropertyIds::toJSON,
            &toJSON,
            this->scriptContext,
            nullptr,
            &info))
    {
        // Any cache hit means the property is missing, so don't bother to do anything else
        return nullptr;
    }

    if (!CacheOperators::TryGetProperty<
        true,   // CheckLocal
        true,   // CheckProto
        true,   // CheckAccessor
        false,  // CheckMissing
        true,   // CheckPolymorphicInlineCache
        true,   // CheckTypePropertyCache
        false,  // IsInlineCacheAvailable
        true,   // IsPolymorphicInlineCacheAvailable
        false,  // ReturnOperationInfo
        false>  // OutputExistence
        (valueObject,
            false,
            valueObject,
            PropertyIds::toJSON,
            &toJSON,
            this->scriptContext,
            nullptr,
            &info))
    {
        if (!JavascriptOperators::GetProperty(valueObject, PropertyIds::toJSON, &toJSON, this->scriptContext, &info))
        {
            return nullptr;
        }
    }
    if (JavascriptConversion::IsCallable(toJSON))
    {
        RecyclableObject* func = UnsafeVarTo<RecyclableObject>(toJSON);
        Var values[2];
        Arguments args(2, values);
        args.Values[0] = valueObject;
        args.Values[1] = key;
        BEGIN_SAFE_REENTRANT_CALL(this->scriptContext->GetThreadContext())
        {
            return JavascriptFunction::CallFunction<true>(func, func->GetEntryPoint(), args);
        }
        END_SAFE_REENTRANT_CALL;
    }
    return nullptr;
}

uint32
JSONStringifier::ReadArrayLength(_In_ RecyclableObject* value)
{
    JavascriptArray* arr = JavascriptArray::TryVarToNonES5Array(value);
    if (arr != nullptr)
    {
        return arr->GetLength();
    }

    int64 len = JavascriptConversion::ToLength(JavascriptOperators::OP_GetLength(value, this->scriptContext), this->scriptContext);
    if (len >= MaxCharCount)
    {
        // If the length goes more than MaxCharCount we will eventually fail (as OOM) in ConcatStringBuilder - so failing early.
        JavascriptError::ThrowRangeError(this->scriptContext, JSERR_OutOfBoundString);
    }
    return static_cast<uint32>(len);
}

void
JSONStringifier::ReadArrayElement(uint32 index, _In_ RecyclableObject* arr, _Out_ JSONProperty* prop, _In_ JSONObjectStack* objectStack)
{
    Var value = nullptr;
    JavascriptArray* jsArray = JavascriptArray::TryVarToNonES5Array(arr);
    if (jsArray && !jsArray->IsCrossSiteObject())
    {
        value = jsArray->DirectGetItem(index);
    }
    else
    {
        JavascriptOperators::GetItem(arr, index, &value, this->scriptContext);
    }
    JavascriptString* indexString = this->scriptContext->GetIntegerString(index);
    this->ReadProperty(indexString, arr, prop, value, objectStack);
}

JSONArray*
JSONStringifier::ReadArray(_In_ RecyclableObject* arr, _In_ JSONObjectStack* objectStack)
{
    if (objectStack->Has(arr))
    {
        JavascriptError::ThrowTypeError(this->scriptContext, JSERR_JSONSerializeCircular);
    }
    JSONObjectStack stack = { 0 };
    stack.next = objectStack;
    stack.object = arr;

    // Increase indentation
    const charcount_t stepbackLength = this->indentLength;
    if (this->gapLength != 0)
    {
        this->indentLength = UInt32Math::Add(this->indentLength, this->gapLength);
    }

    const uint32 arrayLength = this->ReadArrayLength(arr);

    const size_t arraySize = AllocSizeMath::Mul(arrayLength, sizeof(JSONProperty));
    JSONArray * jsonArray = RecyclerNewPlusZ(this->scriptContext->GetRecycler(), arraySize, JSONArray);

    jsonArray->length = arrayLength;
    for (uint32 i = 0; i < arrayLength; ++i)
    {
        this->ReadArrayElement(i, arr, &jsonArray->arr[i], &stack);
        if (jsonArray->arr[i].type == JSONContentType::Undefined)
        {
            // We append null string in case of Undefined
            this->SetNullProperty(&jsonArray->arr[i]);
        }
    }

    this->CalculateStringifiedLength(arrayLength, stepbackLength);

    // Restore indentataion
    this->indentLength = stepbackLength;

    return jsonArray;
}

void
JSONStringifier::AppendObjectElement(
    _In_ JavascriptString* propertyName,
    _In_ JSONObject* jsonObject,
    _In_ JSONObjectProperty* prop)
{
    // Undefined result is not concatenated
    if (prop->propertyValue.type != JSONContentType::Undefined)
    {
        // Increase length for the name of the property
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, CalculateStringElementLength(propertyName));
        // Increment length for concatenation of ":"
        UInt32Math::Inc(this->totalStringLength);
        if (this->gapLength != 0)
        {
            // If gap is specified, a space is appended
            UInt32Math::Inc(this->totalStringLength);
        }

        jsonObject->Push(*prop);
    }
}

void
JSONStringifier::ReadObjectElement(
    _In_ JavascriptString* propertyName,
    _In_ uint32 numericIndex,
    _In_ RecyclableObject* obj,
    _In_ JSONObject* jsonObject,
    _In_ JSONObjectStack* objectStack)
{
    JSONObjectProperty prop;
    prop.propertyName = propertyName;

    Var value = JavascriptOperators::GetItem(obj, numericIndex, this->scriptContext);

    this->ReadProperty(propertyName, obj, &prop.propertyValue, value, objectStack);

    this->AppendObjectElement(propertyName, jsonObject, &prop);
}

void
JSONStringifier::ReadObjectElement(
    _In_ JavascriptString* propertyName,
    _In_opt_ PropertyRecord const* propertyRecord,
    _In_ RecyclableObject* obj,
    _In_ JSONObject* jsonObject,
    _In_ JSONObjectStack* objectStack)
{
    JSONObjectProperty prop;
    prop.propertyName = propertyName;

    Var value = this->ReadValue(propertyName, propertyRecord, obj);

    this->ReadProperty(propertyName, obj, &prop.propertyValue, value, objectStack);

    this->AppendObjectElement(propertyName, jsonObject, &prop);
}

// Calculates how many additional characters are needed for printing the Object/Array structure
// This includes commas, brackets, and gap (if any)
void
JSONStringifier::CalculateStringifiedLength(uint32 propertyCount, charcount_t stepbackLength)
{
    if (propertyCount == 0)
    {
        // Empty object is always represented as "{}" (and arrays are "[]")
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, 2);
    }
    else if (this->gapLength == 0)
    {
        // Concatenate the comma separators, one for all but the last property
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, propertyCount - 1);
        // Wrap the object with curly braces
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, 2);
    }
    else
    {
        // Separator is the concatenation of comma, line feed, and indent
        // Therefore length is 2 + indentLength
        charcount_t separatorLength = UInt32Math::Add(this->indentLength, 2);
        // Separators are inserted after all but the last property
        charcount_t totalSeparatorsLength = UInt32Math::Mul(separatorLength, propertyCount - 1);
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, totalSeparatorsLength);

        // Properties are wrapped by open bracket, line feed, indent, line feed, stepback, and close bracket
        // That amounts to an additional length of 4 + indentLength + stepbackLength
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, 4);
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, stepbackLength);
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, this->indentLength);
    }
}

void
JSONStringifier::ReadProxy(_In_ JavascriptProxy* proxyObject, _In_ JSONObject* jsonObject, _In_ JSONObjectStack* stack)
{
    JavascriptArray* ownPropertyNames = proxyObject->PropertyKeysTrap(JavascriptProxy::KeysTrapKind::GetOwnPropertyNamesKind, this->scriptContext);

    // filter enumerable keys
    uint32 resultLength = ownPropertyNames->GetLength();
    for (uint32 i = 0; i < resultLength; i++)
    {
        Var element = ownPropertyNames->DirectGetItem(i);

        // Array should only have string elements, but let's check to be safe
        JavascriptString* propertyName = JavascriptOperators::TryFromVar<JavascriptString>(element);
        Assert(propertyName);
        if (propertyName != nullptr)
        {
            PropertyDescriptor propertyDescriptor;
            PropertyRecord const* propertyRecord;
            JavascriptConversion::ToPropertyKey(propertyName, scriptContext, &propertyRecord, nullptr);
            if (JavascriptOperators::GetOwnPropertyDescriptor(proxyObject, propertyRecord->GetPropertyId(), scriptContext, &propertyDescriptor))
            {
                if (propertyDescriptor.IsEnumerable())
                {
                    this->ReadObjectElement(propertyName, propertyRecord, proxyObject, jsonObject, stack);
                }
            }
        }
    }
}

JSONObject*
JSONStringifier::ReadObject(_In_ RecyclableObject* obj, _In_ JSONObjectStack* objectStack)
{
    if (objectStack->Has(obj))
    {
        JavascriptError::ThrowTypeError(this->scriptContext, JSERR_JSONSerializeCircular);
    }
    JSONObjectStack stack = { 0 };
    stack.next = objectStack;
    stack.object = obj;

    // Increase indentation
    const charcount_t stepbackLength = this->indentLength;
    if (this->gapLength != 0)
    {
        this->indentLength = UInt32Math::Add(this->indentLength, this->gapLength);
    }

    Recycler* recycler = this->scriptContext->GetRecycler();
    JSONObject* jsonObject = RecyclerNew(recycler, JSONObject, recycler);

    // If we are given a property list, we should only read the listed properties
    if (this->propertyList != nullptr)
    {
        FOREACH_SLIST_ENTRY(PropertyListElement, entry, this->propertyList)
        {
            this->ReadObjectElement(entry.propertyName, entry.propertyRecord, obj, jsonObject, &stack);
        }
        NEXT_SLIST_ENTRY;
    }
    else
    {
        // Enumerating proxies is different than normal objects, so enumerate them separately
        JavascriptProxy* proxyObject = JavascriptOperators::TryFromVar<JavascriptProxy>(obj);
        if (proxyObject != nullptr)
        {
            this->ReadProxy(proxyObject, jsonObject, &stack);
        }
        else
        {
            JavascriptStaticEnumerator enumerator;
            EnumeratorCache* cache = this->scriptContext->GetLibrary()->GetStringifyCache(obj->GetType());
            if (obj->GetEnumerator(&enumerator, EnumeratorFlags::SnapShotSemantics | EnumeratorFlags::EphemeralReference | EnumeratorFlags::UseCache, this->scriptContext, cache))
            {
                JavascriptString* propertyName = nullptr;
                PropertyId nextKey = Constants::NoProperty;
                while ((propertyName = enumerator.MoveAndGetNext(nextKey)) != nullptr)
                {
                    const uint32 numericIndex = enumerator.GetCurrentItemIndex();
                    if (numericIndex != Constants::InvalidSourceIndex)
                    {
                        this->ReadObjectElement(propertyName, numericIndex, obj, jsonObject, &stack);
                    }
                    else
                    {
                        PropertyRecord const * propertyRecord = nullptr;
                        if (nextKey == Constants::NoProperty)
                        {
                            this->scriptContext->GetOrAddPropertyRecord(propertyName, &propertyRecord);
                            nextKey = propertyRecord->GetPropertyId();
                        }
                        this->ReadObjectElement(propertyName, propertyRecord, obj, jsonObject, &stack);
                    }
                }
            }
        }
    }
    // JSONObject is an SList, which only has push/pop, so need to reverse list for it to be in order
    jsonObject->Reverse();
    const uint propertyCount = jsonObject->Count();

    this->CalculateStringifiedLength(propertyCount, stepbackLength);

    // Restore indent level
    this->indentLength = stepbackLength;

    return jsonObject;
}

Var
JSONStringifier::CallReplacerFunction(_In_opt_ RecyclableObject* holder, _In_ JavascriptString* key, _In_ Var value)
{
    Var values[3];
    Arguments args(0, values);

    args.Info.Count = 3;
    args.Values[0] = holder;
    args.Values[1] = key;
    args.Values[2] = value;

    BEGIN_SAFE_REENTRANT_CALL(this->scriptContext->GetThreadContext())
    {
        return JavascriptFunction::CallFunction<true>(this->replacerFunction, this->replacerFunction->GetEntryPoint(), args);
    }
    END_SAFE_REENTRANT_CALL;
}

void
JSONStringifier::SetNullProperty(_Out_ JSONProperty* prop)
{
    prop->type = JSONContentType::Null;
    this->totalStringLength = UInt32Math::Add(this->totalStringLength, Constants::NullStringLength);
}

void
JSONStringifier::SetNumericProperty(double value, _In_ Var valueVar, _Out_ JSONProperty* prop)
{
    if (NumberUtilities::IsFinite(value))
    {
        prop->type = JSONContentType::Number;
        prop->numericValue.value = valueVar;
        prop->numericValue.string = JavascriptConversion::ToString(valueVar, this->scriptContext);
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, prop->numericValue.string->GetLength());
    }
    else
    {
        this->SetNullProperty(prop);
    }
}

// Strings may need to be escaped, so the length requires some calculation
charcount_t
JSONStringifier::CalculateStringElementLength(_In_ JavascriptString* str)
{
    const charcount_t strLength = str->GetLength();

    // To avoid overflow checks, use uint64 for intermediate size calculation. We cannot overflow uint64,
    // as stringification will be at most a 5x expansion of the original string (which has max length INT_MAX - 1)
    //
    // All JSON strings are enclosed in double quotes, so add 2 to the length to account for them
    uint64 escapedStrLength = strLength + 2;
    const char16* bufferStart = str->GetString();
    for (const char16* index = str->GetString(); index < bufferStart + strLength; ++index)
    {
        char16 currentCharacter = *index;

        // Some characters may require an escape sequence. We can use the escapeMapCount table
        // to determine how many extra characters are needed
        if (currentCharacter < _countof(LazyJSONString::escapeMapCount))
        {
            escapedStrLength += LazyJSONString::escapeMapCount[currentCharacter];
        }
    }
    if (escapedStrLength > UINT32_MAX)
    {
        Js::Throw::OutOfMemory();
    }
    return static_cast<charcount_t>(escapedStrLength);
}

void
JSONStringifier::ReadData(_In_ RecyclableObject* valueObj, _Out_ JSONProperty* prop)
{
    TypeId typeId = JavascriptOperators::GetTypeId(valueObj);

    switch (typeId)
    {
    case TypeIds_Null:
        this->SetNullProperty(prop);
        return;

    case TypeIds_Boolean:
        if (UnsafeVarTo<JavascriptBoolean>(valueObj)->GetValue() != FALSE)
        {
            prop->type = JSONContentType::True;
            this->totalStringLength = UInt32Math::Add(this->totalStringLength, Constants::TrueStringLength);
        }
        else
        {
            prop->type = JSONContentType::False;
            this->totalStringLength = UInt32Math::Add(this->totalStringLength, Constants::FalseStringLength);
        }
        return;

    case TypeIds_Int64Number:
        this->SetNumericProperty(static_cast<double>(UnsafeVarTo<JavascriptInt64Number>(valueObj)->GetValue()), valueObj, prop);
        return;

    case TypeIds_UInt64Number:
        this->SetNumericProperty(static_cast<double>(UnsafeVarTo<JavascriptUInt64Number>(valueObj)->GetValue()), valueObj, prop);
        return;

#if !FLOATVAR
    case TypeIds_Number:
        this->SetNumericProperty(JavascriptNumber::GetValue(valueObj), valueObj, prop);
        return;
#endif

    case TypeIds_String:
        prop->stringValue = UnsafeVarTo<JavascriptString>(valueObj);
        prop->type = JSONContentType::String;
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, CalculateStringElementLength(prop->stringValue));
        return;

    case TypeIds_Symbol:
        // Treat symbols as undefined
    case TypeIds_Undefined:
        // If undefined is in an object, it is not included in the string, and if it is an array element it will show up as "null"
        // Given that, we can't caluclate the string length here and instead we do it at the usage points
        prop->type = JSONContentType::Undefined;
        return;

    default:
        Assert(UNREACHED);
        prop->type = JSONContentType::Undefined;
        return;
    }
}

void
JSONStringifier::ReadData(_In_ Var value, _Out_ JSONProperty* prop)
{
    RecyclableObject* valueObj = JavascriptOperators::TryFromVar<RecyclableObject>(value);
    if (valueObj != nullptr)
    {
        ReadData(valueObj, prop);
        return;
    }

    // If value isn't a RecyclableObject, it must be a tagged number

    TypeId typeId = JavascriptOperators::GetTypeId(value);
    switch (typeId)
    {
    case TypeIds_Integer:
    {
        prop->type = JSONContentType::Number;
        prop->numericValue.value = value;
        prop->numericValue.string = this->scriptContext->GetIntegerString(value);
        this->totalStringLength = UInt32Math::Add(this->totalStringLength, prop->numericValue.string->GetLength());
        return;
    }
#if FLOATVAR
    case TypeIds_Number:
        this->SetNumericProperty(JavascriptNumber::GetValue(value), value, prop);
        return;
#endif
    default:
        Assume(UNREACHED);
        prop->type = JSONContentType::Undefined;
        return;
    }
}

void
RecheckValue(_In_ Var value, _Out_ RecyclableObject** valueObj, _Out_ bool* isObject)
{
    *valueObj = JavascriptOperators::TryFromVar<RecyclableObject>(value);
    if (*valueObj != nullptr)
    {
        *isObject = JavascriptOperators::IsObject(value) != FALSE;
    }
    else
    {
        *isObject = false;
    }
}

void
JSONStringifier::ReadProperty(
    _In_ JavascriptString* key,
    _In_opt_ RecyclableObject* holder,
    _Out_ JSONProperty* prop,
    _In_ Var value,
    _In_ JSONObjectStack* objectStack)
{
    PROBE_STACK(this->scriptContext, Constants::MinStackDefault);

    // Save these to avoid having to recheck conditions unless value is modified by ToJSON or a replacer function
    RecyclableObject* valueObj = JavascriptOperators::TryFromVar<RecyclableObject>(value);
    bool isObject = false;
    if (valueObj)
    {
        isObject = JavascriptOperators::IsObject(valueObj) != FALSE;
        if (isObject)
        {
            // If value is an object, we must first check if it has a ToJSON method
            Var toJSONValue = this->ToJSON(key, valueObj);
            if (toJSONValue != nullptr)
            {
                value = toJSONValue;
                RecheckValue(toJSONValue, &valueObj, &isObject);
            }
        }
    }

    // If user provided a replacer function, we need to call it
    if (this->replacerFunction != nullptr)
    {
        value = this->CallReplacerFunction(holder, key, value);
        RecheckValue(value, &valueObj, &isObject);
    }

    if (valueObj != nullptr)
    {
        // Callable JS objects are undefined, but we still stringify callable host objects
        // Host object case is for compat with old implementation, but isn't defined in the
        // spec, so we should consider removing it
        if (JavascriptConversion::IsCallable(valueObj) && JavascriptOperators::IsJsNativeObject(valueObj))
        {
            prop->type = JSONContentType::Undefined;
            return;
        }

        if (JavascriptOperators::IsArray(valueObj))
        {
            prop->type = JSONContentType::Array;
            prop->arr = this->ReadArray(valueObj, objectStack);
            return;
        }

        Var primitive = this->TryConvertPrimitiveObject(valueObj);
        if (primitive != nullptr)
        {
            value = primitive;
            RecheckValue(value, &valueObj, &isObject);
        }

        if (isObject)
        {
            prop->type = JSONContentType::Object;
            prop->obj = this->ReadObject(valueObj, objectStack);
            return;
        }

        if (valueObj != nullptr)
        {
            this->ReadData(valueObj, prop);
            return;
        }
    }
    this->ReadData(value, prop);
    return;
}

} // namespace Js
