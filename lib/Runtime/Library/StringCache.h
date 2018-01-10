//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{

#define SCACHE_INIT_DEFAULT(name) __##name(nullptr)
#define DEFINE_CACHED_STRING(name, str)                   \
private:                                                  \
    Field(JavascriptString*) __##name;                    \
public:                                                   \
    JavascriptString* name() {                            \
        if (__##name == nullptr)                          \
        {                                                 \
            __##name = CreateStringFromCppLiteral(str);   \
        }                                                 \
                                                          \
        return __##name;                                  \
    }

#define DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(name, str)   \
private:                                                  \
    Field(JavascriptString*) __##name;                    \
public:                                                   \
    JavascriptString* name() {                            \
        if (__##name == nullptr)                          \
        {                                                 \
            __##name = CreatePropertyStringFromFromCppLiteral(str);   \
        }                                                 \
                                                          \
        return __##name;                                  \
    }

class StaticType;

class StringCache
{
    Field(ScriptContext*) scriptContext;
    Field(StaticType*) stringTypeStatic;
public:
    StringCache():
      SCACHE_INIT_DEFAULT(GetEmptyObjectString),
      SCACHE_INIT_DEFAULT(GetQuotesString),
      SCACHE_INIT_DEFAULT(GetWhackString),
      SCACHE_INIT_DEFAULT(GetCommaDisplayString),
      SCACHE_INIT_DEFAULT(GetCommaSpaceDisplayString),
      SCACHE_INIT_DEFAULT(GetOpenBracketString),
      SCACHE_INIT_DEFAULT(GetCloseBracketString),
      SCACHE_INIT_DEFAULT(GetOpenSBracketString),
      SCACHE_INIT_DEFAULT(GetCloseSBracketString),
      SCACHE_INIT_DEFAULT(GetEmptyArrayString),
      SCACHE_INIT_DEFAULT(GetNewLineString),
      SCACHE_INIT_DEFAULT(GetColonString),
      SCACHE_INIT_DEFAULT(GetFunctionAnonymousString),
      SCACHE_INIT_DEFAULT(GetFunctionPTRAnonymousString),
      SCACHE_INIT_DEFAULT(GetAsyncFunctionAnonymouseString),
      SCACHE_INIT_DEFAULT(GetOpenRBracketString),
      SCACHE_INIT_DEFAULT(GetNewLineCloseRBracketString),
      SCACHE_INIT_DEFAULT(GetSpaceOpenBracketString),
      SCACHE_INIT_DEFAULT(GetNewLineCloseBracketString),
      SCACHE_INIT_DEFAULT(GetFunctionPrefixString),
      SCACHE_INIT_DEFAULT(GetGeneratorFunctionPrefixString),
      SCACHE_INIT_DEFAULT(GetAsyncFunctionPrefixString),
      SCACHE_INIT_DEFAULT(GetFunctionDisplayString),
      SCACHE_INIT_DEFAULT(GetXDomainFunctionDisplayString),
      SCACHE_INIT_DEFAULT(GetInvalidDateString),
      SCACHE_INIT_DEFAULT(GetObjectDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectArgumentsDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectArrayDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectBooleanDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectDateDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectErrorDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectFunctionDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectNumberDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectRegExpDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectStringDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectNullDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectUndefinedDisplayString),
      SCACHE_INIT_DEFAULT(GetUndefinedDisplayString),
      SCACHE_INIT_DEFAULT(GetNaNDisplayString),
      SCACHE_INIT_DEFAULT(GetNullDisplayString),
      SCACHE_INIT_DEFAULT(GetUnknownDisplayString),
      SCACHE_INIT_DEFAULT(GetTrueDisplayString),
      SCACHE_INIT_DEFAULT(GetFalseDisplayString),
      SCACHE_INIT_DEFAULT(GetStringTypeDisplayString),
      SCACHE_INIT_DEFAULT(GetObjectTypeDisplayString),
      SCACHE_INIT_DEFAULT(GetFunctionTypeDisplayString),
      SCACHE_INIT_DEFAULT(GetBooleanTypeDisplayString),
      SCACHE_INIT_DEFAULT(GetNumberTypeDisplayString),
      SCACHE_INIT_DEFAULT(GetModuleTypeDisplayString),
      SCACHE_INIT_DEFAULT(GetVariantDateTypeDisplayString),
      SCACHE_INIT_DEFAULT(GetSymbolTypeDisplayString)
    {
    }

    void Initialize(ScriptContext* scriptContext, StaticType* staticString)
    {
        this->scriptContext = scriptContext;
        this->stringTypeStatic = staticString;
    }

    StaticType* GetStringTypeStatic() const
    {
        AssertMsg(stringTypeStatic, "Where's stringTypeStatic?"); return stringTypeStatic;
    }

    DEFINE_CACHED_STRING(GetEmptyObjectString, _u("{}"))
    DEFINE_CACHED_STRING(GetQuotesString, _u("\"\""))
    DEFINE_CACHED_STRING(GetWhackString, _u("/"))
    DEFINE_CACHED_STRING(GetCommaDisplayString, _u(","))
    DEFINE_CACHED_STRING(GetCommaSpaceDisplayString, _u(", "))
    DEFINE_CACHED_STRING(GetOpenBracketString, _u("{"))
    DEFINE_CACHED_STRING(GetCloseBracketString, _u("}"))
    DEFINE_CACHED_STRING(GetOpenSBracketString, _u("["))
    DEFINE_CACHED_STRING(GetCloseSBracketString, _u("]"))
    DEFINE_CACHED_STRING(GetEmptyArrayString, _u("[]"))
    DEFINE_CACHED_STRING(GetNewLineString, _u("\n"))
    DEFINE_CACHED_STRING(GetColonString, _u(":"))
    DEFINE_CACHED_STRING(GetFunctionAnonymousString, _u("function anonymous"))
    DEFINE_CACHED_STRING(GetFunctionPTRAnonymousString, _u("function* anonymous"))
    DEFINE_CACHED_STRING(GetAsyncFunctionAnonymouseString, _u("async function anonymous"))
    DEFINE_CACHED_STRING(GetOpenRBracketString, _u("("))
    DEFINE_CACHED_STRING(GetNewLineCloseRBracketString, _u("\n)"))
    DEFINE_CACHED_STRING(GetSpaceOpenBracketString, _u(" {"))
    DEFINE_CACHED_STRING(GetNewLineCloseBracketString, _u("\n}"))
    DEFINE_CACHED_STRING(GetFunctionPrefixString, _u("function "))
    DEFINE_CACHED_STRING(GetGeneratorFunctionPrefixString, _u("function* "))
    DEFINE_CACHED_STRING(GetAsyncFunctionPrefixString, _u("async function "))
    DEFINE_CACHED_STRING(GetFunctionDisplayString, JS_DISPLAY_STRING_FUNCTION_ANONYMOUS)
    DEFINE_CACHED_STRING(GetXDomainFunctionDisplayString, _u("function anonymous() {\n    [x-domain code]\n}"))
    DEFINE_CACHED_STRING(GetInvalidDateString, _u("Invalid Date"))
    DEFINE_CACHED_STRING(GetObjectDisplayString, _u("[object Object]"))
    DEFINE_CACHED_STRING(GetObjectArgumentsDisplayString, _u("[object Arguments]"))
    DEFINE_CACHED_STRING(GetObjectArrayDisplayString, _u("[object Array]"))
    DEFINE_CACHED_STRING(GetObjectBooleanDisplayString, _u("[object Boolean]"))
    DEFINE_CACHED_STRING(GetObjectDateDisplayString, _u("[object Date]"))
    DEFINE_CACHED_STRING(GetObjectErrorDisplayString, _u("[object Error]"))
    DEFINE_CACHED_STRING(GetObjectFunctionDisplayString, _u("[object Function]"))
    DEFINE_CACHED_STRING(GetObjectNumberDisplayString, _u("[object Number]"))
    DEFINE_CACHED_STRING(GetObjectRegExpDisplayString, _u("[object RegExp]"))
    DEFINE_CACHED_STRING(GetObjectStringDisplayString, _u("[object String]"))
    DEFINE_CACHED_STRING(GetObjectNullDisplayString, _u("[object Null]"))
    DEFINE_CACHED_STRING(GetObjectUndefinedDisplayString, _u("[object Undefined]"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetUndefinedDisplayString, _u("undefined"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetNaNDisplayString, _u("NaN"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetNullDisplayString, _u("null"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetUnknownDisplayString, _u("unknown"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetTrueDisplayString, _u("true"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetFalseDisplayString, _u("false"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetStringTypeDisplayString, _u("string"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetObjectTypeDisplayString, _u("object"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetFunctionTypeDisplayString, _u("function"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetBooleanTypeDisplayString, _u("boolean"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetNumberTypeDisplayString, _u("number"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetModuleTypeDisplayString, _u("Module"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetVariantDateTypeDisplayString, _u("date"))
    DEFINE_CACHED_STRING_WITH_PROPERTYSTRING(GetSymbolTypeDisplayString, _u("symbol"))

private:
  template< size_t N > JavascriptString* CreateStringFromCppLiteral(const char16(&value)[N]) const
  {
      return LiteralString::New(stringTypeStatic, value, N - 1 /*don't include terminating NUL*/, scriptContext->GetRecycler());
  }

  template< size_t N > JavascriptString* CreatePropertyStringFromFromCppLiteral(const char16(&value)[N]) const
  {
      const PropertyRecord* propertyRecord = nullptr;
      scriptContext->FindPropertyRecord(value, N - 1, &propertyRecord);
      AssertMsg(propertyRecord != nullptr, "Trying to create a propertystring for non property string?");
      Assert(IsBuiltInPropertyId(propertyRecord->GetPropertyId()));
      return scriptContext->GetPropertyString(propertyRecord->GetPropertyId());
  }

};

#undef SCACHE_INIT_DEFAULT

} // Js
