//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{

class StaticType;

class StringCache
{
    Field(ScriptContext*) scriptContext;
    Field(StaticType*) stringTypeStatic;
public:
    StringCache() :
// NOTE: the trailing comma is important!
#define STRING(name, str) __##name(nullptr),
#define PROPERTY_STRING(name, str) STRING(name, str)
#include "StringCacheList.h"
#undef PROPERTY_STRING
#undef STRING
        scriptContext(nullptr),
        stringTypeStatic(nullptr)
    {
    }

    void Initialize(ScriptContext* scriptContext, StaticType* staticString)
    {
        this->scriptContext = scriptContext;
        this->stringTypeStatic = staticString;
    }

    StaticType* GetStringTypeStatic() const
    {
        AssertMsg(stringTypeStatic, "Where's stringTypeStatic?");
        return stringTypeStatic;
    }

#define INITIALIZE(name, str, withPropertyString)         \
private:                                                  \
    Field(JavascriptString*) __##name;                    \
public:                                                   \
    JavascriptString* Get##name() {                       \
        if (__##name == nullptr)                          \
        {                                                 \
            __##name = InitializeString<withPropertyString>(str); \
        }                                                 \
                                                          \
        return __##name;                                  \
    }
#define STRING(name, str) INITIALIZE(name, str, false)
#define PROPERTY_STRING(name, str) INITIALIZE(name, str, true)
#include "StringCacheList.h"
#undef PROPERTY_STRING
#undef STRING
#undef INITIALIZE

private:
    template <bool withPropertyString, size_t N> JavascriptString* InitializeString(const char16(&value)[N]) const
    {
        if (withPropertyString)
        {
            const PropertyRecord* propertyRecord = nullptr;
            scriptContext->FindPropertyRecord(value, N - 1, &propertyRecord);
            AssertMsg(propertyRecord != nullptr, "Trying to create a propertystring for non property string?");
            Assert(IsBuiltInPropertyId(propertyRecord->GetPropertyId()));
            return scriptContext->GetPropertyString(propertyRecord->GetPropertyId());
        }
        else
        {
            return LiteralString::New(stringTypeStatic, value, N - 1 /*don't include terminating NUL*/, scriptContext->GetRecycler());
        }
    }
};

} // Js
