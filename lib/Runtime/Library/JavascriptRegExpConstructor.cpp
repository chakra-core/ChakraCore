//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "RegexPattern.h"

namespace Js
{
    // The VS2013 linker treats this as a redefinition of an already
    // defined constant and complains. So skip the declaration if we're compiling
    // with VS2013 or below.
#if !defined(_MSC_VER) || _MSC_VER >= 1900
    const int JavascriptRegExpConstructor::NumCtorCaptures;
#endif

    JavascriptRegExpConstructor::JavascriptRegExpConstructor(DynamicType * type) :
        RuntimeFunction(type, &JavascriptRegExp::EntryInfo::NewInstance),
        reset(false),
        lastPattern(nullptr),
        lastMatch() // undefined
    {
        DebugOnly(VerifyEntryPoint());
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptString* emptyString = scriptContext->GetLibrary()->GetEmptyString();
        this->lastInput = emptyString;
        this->index = JavascriptNumber::ToVar(-1, scriptContext);
        this->lastIndex = JavascriptNumber::ToVar(-1, scriptContext);
        this->lastParen = emptyString;
        this->leftContext = emptyString;
        this->rightContext = emptyString;
        for (int i = 0; i < NumCtorCaptures; i++)
        {
            this->captures[i] = emptyString;
        }
    }

    BOOL JavascriptRegExpConstructor::GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache)
    {
        return GetEnumeratorWithPrefix(
            RecyclerNew(GetScriptContext()->GetRecycler(), JavascriptRegExpEnumerator, this, flags, requestContext),
            enumerator, flags, requestContext, forInCache);
    }

    void JavascriptRegExpConstructor::SetLastMatch(UnifiedRegex::RegexPattern* lastPattern, JavascriptString* lastInput, UnifiedRegex::GroupInfo lastMatch)
    {
        AssertMsg(!lastMatch.IsUndefined(), "SetLastMatch should only be called if there's a successful match");
        AssertMsg(lastPattern != nullptr, "lastPattern should not be null");
        AssertMsg(lastInput != nullptr, "lastInput should not be null");
        AssertMsg(JavascriptOperators::GetTypeId(lastInput) != TypeIds_Null, "lastInput should not be JavaScript null");

        this->lastPattern = lastPattern;
        this->lastInput = lastInput;
        this->lastMatch = lastMatch;
        this->reset = true;
    }

    void JavascriptRegExpConstructor::EnsureValues()
    {
        if (reset)
        {
            Assert(!lastMatch.IsUndefined());
            ScriptContext* scriptContext = this->GetScriptContext();
            UnifiedRegex::RegexPattern* pattern = lastPattern;
            JavascriptString* emptyString = scriptContext->GetLibrary()->GetEmptyString();
            const CharCount lastInputLen = lastInput->GetLength();
            // IE8 quirk: match of length 0 is regarded as length 1
            CharCount lastIndexVal = lastMatch.EndOffset();
            this->index = JavascriptNumber::ToVar(lastMatch.offset, scriptContext);
            this->lastIndex = JavascriptNumber::ToVar(lastIndexVal, scriptContext);
            this->leftContext = lastMatch.offset > 0 ? SubString::New(lastInput, 0, lastMatch.offset) : emptyString;
            this->rightContext = lastIndexVal > 0 && lastIndexVal < lastInputLen ? SubString::New(lastInput, lastIndexVal, lastInputLen - lastIndexVal) : emptyString;

            Var nonMatchValue = RegexHelper::NonMatchValue(scriptContext, true);
            captures[0] = RegexHelper::GetString(scriptContext, lastInput, nonMatchValue, lastMatch);
            int numGroups = pattern->NumGroups();
            if (numGroups > 1)
            {
                // The RegExp constructor's lastMatch holds the last *successful* match on any regular expression.
                // That regular expression may since have been used for *unsuccessful* matches, in which case
                // its groups will have been reset. Updating the RegExp constructor with the group binding after
                // every match is prohibitively slow. Instead, run the match again using the known last input string.
                if (!pattern->WasLastMatchSuccessful())
                {
                    RegexHelper::SimpleMatch(scriptContext, pattern, lastInput->GetString(), lastInputLen, lastMatch.offset);
                }
                Assert(pattern->WasLastMatchSuccessful());
                for (int groupId = 1; groupId < min(numGroups, NumCtorCaptures); groupId++)
                    captures[groupId] = RegexHelper::GetGroup(scriptContext, pattern, lastInput, nonMatchValue, groupId);

                this->lastParen = numGroups <= NumCtorCaptures ?
                    PointerValue(captures[numGroups - 1]) :
                    RegexHelper::GetGroup(scriptContext, pattern, lastInput, nonMatchValue, numGroups - 1);
            }
            else
            {
                this->lastParen = emptyString;
            }
            for (int groupId = numGroups; groupId < NumCtorCaptures; groupId++)
                captures[groupId] = emptyString;
            reset = false;
        }
    }

    /*static*/
    PropertyId const JavascriptRegExpConstructor::specialPropertyIds[] =
    {
        PropertyIds::$_,
        PropertyIds::$Ampersand,
        PropertyIds::$Plus,
        PropertyIds::$BackTick,
        PropertyIds::$Tick,
        PropertyIds::index,
    };

    PropertyId const JavascriptRegExpConstructor::specialEnumPropertyIds[] =
    {
        PropertyIds::$1,
        PropertyIds::$2,
        PropertyIds::$3,
        PropertyIds::$4,
        PropertyIds::$5,
        PropertyIds::$6,
        PropertyIds::$7,
        PropertyIds::$8,
        PropertyIds::$9,
        PropertyIds::input,
        PropertyIds::rightContext,
        PropertyIds::leftContext,
        PropertyIds::lastParen,
        PropertyIds::lastMatch,
    };

    PropertyId const JavascriptRegExpConstructor::specialnonEnumPropertyIds[] =
    {
        PropertyIds::$_,
        PropertyIds::$Ampersand,
        PropertyIds::$Plus,
        PropertyIds::$BackTick,
        PropertyIds::$Tick,
        PropertyIds::index,
    };

    PropertyQueryFlags JavascriptRegExpConstructor::HasPropertyQuery(PropertyId propertyId)
    {
        switch (propertyId)
        {
        case PropertyIds::lastMatch:
        case PropertyIds::$Ampersand:
        case PropertyIds::lastParen:
        case PropertyIds::$Plus:
        case PropertyIds::leftContext:
        case PropertyIds::$BackTick:
        case PropertyIds::rightContext:
        case PropertyIds::$Tick:
        case PropertyIds::index:
        case PropertyIds::input:
        case PropertyIds::$_:
        case PropertyIds::$1:
        case PropertyIds::$2:
        case PropertyIds::$3:
        case PropertyIds::$4:
        case PropertyIds::$5:
        case PropertyIds::$6:
        case PropertyIds::$7:
        case PropertyIds::$8:
        case PropertyIds::$9:
            return PropertyQueryFlags::Property_Found;
        default:
            return JavascriptFunction::HasPropertyQuery(propertyId);
        }
    }

    PropertyQueryFlags JavascriptRegExpConstructor::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptRegExpConstructor::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    PropertyQueryFlags JavascriptRegExpConstructor::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL result;
        if (GetPropertyBuiltIns(propertyId, value, &result))
        {
            return JavascriptConversion::BooleanToPropertyQueryFlags(result);
        }

        return JavascriptFunction::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    PropertyQueryFlags JavascriptRegExpConstructor::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL result;
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, &result))
        {
            return JavascriptConversion::BooleanToPropertyQueryFlags(result);
        }

        return JavascriptFunction::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext);
    }

    bool JavascriptRegExpConstructor::GetPropertyBuiltIns(PropertyId propertyId, Var* value, BOOL* result)
    {
        switch (propertyId)
        {
        case PropertyIds::input:
        case PropertyIds::$_:
            this->EnsureValues();
            *value = this->lastInput;
            *result = true;
            return true;
        case PropertyIds::lastMatch:
        case PropertyIds::$Ampersand:
            this->EnsureValues();
            *value = this->captures[0];
            *result = true;
            return true;
        case PropertyIds::lastParen:
        case PropertyIds::$Plus:
            this->EnsureValues();
            *value = this->lastParen;
            *result = true;
            return true;
        case PropertyIds::leftContext:
        case PropertyIds::$BackTick:
            this->EnsureValues();
            *value = this->leftContext;
            *result = true;
            return true;
        case PropertyIds::rightContext:
        case PropertyIds::$Tick:
            this->EnsureValues();
            *value = this->rightContext;
            *result = true;
            return true;
        case PropertyIds::$1:
            this->EnsureValues();
            *value = this->captures[1];
            *result = true;
            return true;
        case PropertyIds::$2:
            this->EnsureValues();
            *value = this->captures[2];
            *result = true;
            return true;
        case PropertyIds::$3:
            this->EnsureValues();
            *value = this->captures[3];
            *result = true;
            return true;
        case PropertyIds::$4:
            this->EnsureValues();
            *value = this->captures[4];
            *result = true;
            return true;
        case PropertyIds::$5:
            this->EnsureValues();
            *value = this->captures[5];
            *result = true;
            return true;
        case PropertyIds::$6:
            this->EnsureValues();
            *value = this->captures[6];
            *result = true;
            return true;
        case PropertyIds::$7:
            this->EnsureValues();
            *value = this->captures[7];
            *result = true;
            return true;
        case PropertyIds::$8:
            this->EnsureValues();
            *value = this->captures[8];
            *result = true;
            return true;
        case PropertyIds::$9:
            this->EnsureValues();
            *value = this->captures[9];
            *result = true;
            return true;
        case PropertyIds::index:
            this->EnsureValues();
            *value = this->index;
            *result = true;
            return true;
        default:
            return false;
        }
    }

    BOOL JavascriptRegExpConstructor::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        BOOL result;
        if (SetPropertyBuiltIns(propertyId, value, &result))
        {
            return result;
        }

        return JavascriptFunction::SetProperty(propertyId, value, flags, info);
    }

    BOOL JavascriptRegExpConstructor::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        BOOL result;
        PropertyRecord const * propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && SetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, &result))
        {
            return result;
        }

        return JavascriptFunction::SetProperty(propertyNameString, value, flags, info);
    }

    bool JavascriptRegExpConstructor::SetPropertyBuiltIns(PropertyId propertyId, Var value, BOOL* result)
    {
        switch (propertyId)
        {
        case PropertyIds::input:
        case PropertyIds::$_:
            //TODO: review: although the 'input' property is marked as readonly, it has a set on V5.8. There is no spec on this.
            EnsureValues(); // The last match info relies on the last input. Use it before it is changed.
            this->lastInput = JavascriptConversion::ToString(value, this->GetScriptContext());
            *result = true;
            return true;
        case PropertyIds::lastMatch:
        case PropertyIds::$Ampersand:
        case PropertyIds::lastParen:
        case PropertyIds::$Plus:
        case PropertyIds::leftContext:
        case PropertyIds::$BackTick:
        case PropertyIds::rightContext:
        case PropertyIds::$Tick:
        case PropertyIds::$1:
        case PropertyIds::$2:
        case PropertyIds::$3:
        case PropertyIds::$4:
        case PropertyIds::$5:
        case PropertyIds::$6:
        case PropertyIds::$7:
        case PropertyIds::$8:
        case PropertyIds::$9:
        case PropertyIds::index:
            *result = false;
            return true;
        default:
            return false;
        }
    }

    BOOL JavascriptRegExpConstructor::InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        return SetProperty(propertyId, value, flags, info);
    }

    BOOL JavascriptRegExpConstructor::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        switch (propertyId)
        {
            // all globals are 'fNoDelete' in V5.8
        case PropertyIds::input:
        case PropertyIds::$_:
        case PropertyIds::lastMatch:
        case PropertyIds::$Ampersand:
        case PropertyIds::lastParen:
        case PropertyIds::$Plus:
        case PropertyIds::leftContext:
        case PropertyIds::$BackTick:
        case PropertyIds::rightContext:
        case PropertyIds::$Tick:
        case PropertyIds::$1:
        case PropertyIds::$2:
        case PropertyIds::$3:
        case PropertyIds::$4:
        case PropertyIds::$5:
        case PropertyIds::$6:
        case PropertyIds::$7:
        case PropertyIds::$8:
        case PropertyIds::$9:
        case PropertyIds::index:
            JavascriptError::ThrowCantDeleteIfStrictMode(flags, GetScriptContext(), GetScriptContext()->GetPropertyName(propertyId)->GetBuffer());
            return false;

        default:
            return JavascriptFunction::DeleteProperty(propertyId, flags);
        }
    }

    BOOL JavascriptRegExpConstructor::DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags)
    {
        JsUtil::CharacterBuffer<WCHAR> propertyName(propertyNameString->GetString(), propertyNameString->GetLength());
        if (BuiltInPropertyRecords::input.Equals(propertyName)
            || BuiltInPropertyRecords::$_.Equals(propertyName)
            || BuiltInPropertyRecords::lastMatch.Equals(propertyName)
            || BuiltInPropertyRecords::$Ampersand.Equals(propertyName)
            || BuiltInPropertyRecords::lastParen.Equals(propertyName)
            || BuiltInPropertyRecords::$Plus.Equals(propertyName)
            || BuiltInPropertyRecords::leftContext.Equals(propertyName)
            || BuiltInPropertyRecords::$BackTick.Equals(propertyName)
            || BuiltInPropertyRecords::rightContext.Equals(propertyName)
            || BuiltInPropertyRecords::$Tick.Equals(propertyName)
            || BuiltInPropertyRecords::$1.Equals(propertyName)
            || BuiltInPropertyRecords::$2.Equals(propertyName)
            || BuiltInPropertyRecords::$3.Equals(propertyName)
            || BuiltInPropertyRecords::$4.Equals(propertyName)
            || BuiltInPropertyRecords::$5.Equals(propertyName)
            || BuiltInPropertyRecords::$6.Equals(propertyName)
            || BuiltInPropertyRecords::$7.Equals(propertyName)
            || BuiltInPropertyRecords::$8.Equals(propertyName)
            || BuiltInPropertyRecords::$9.Equals(propertyName)
            || BuiltInPropertyRecords::index.Equals(propertyName))
        {
            JavascriptError::ThrowCantDeleteIfStrictMode(flags, GetScriptContext(), propertyNameString->GetString());
            return false;
        }

        return JavascriptFunction::DeleteProperty(propertyNameString, flags);
    }

    BOOL JavascriptRegExpConstructor::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(JS_DIAG_VALUE_JavascriptRegExpConstructor);
        return TRUE;
    }

    BOOL JavascriptRegExpConstructor::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(JS_DIAG_TYPE_JavascriptRegExpConstructor);
        return TRUE;
    }

    BOOL JavascriptRegExpConstructor::IsEnumerable(PropertyId propertyId)
    {
        switch (propertyId)
        {
        case PropertyIds::input:
        case PropertyIds::$1:
        case PropertyIds::$2:
        case PropertyIds::$3:
        case PropertyIds::$4:
        case PropertyIds::$5:
        case PropertyIds::$6:
        case PropertyIds::$7:
        case PropertyIds::$8:
        case PropertyIds::$9:
        case PropertyIds::leftContext:
        case PropertyIds::rightContext:
        case PropertyIds::lastMatch:
        case PropertyIds::lastParen:
            return true;
        case PropertyIds::$_:
        case PropertyIds::$Ampersand:
        case PropertyIds::$Plus:
        case PropertyIds::$BackTick:
        case PropertyIds::$Tick:
        case PropertyIds::index:
            return false;
        default:
            return JavascriptFunction::IsEnumerable(propertyId);
        }
    }

    BOOL JavascriptRegExpConstructor::IsConfigurable(PropertyId propertyId)
    {
        switch (propertyId)
        {
        case PropertyIds::input:
        case PropertyIds::$_:

        case PropertyIds::lastMatch:
        case PropertyIds::$Ampersand:

        case PropertyIds::lastParen:
        case PropertyIds::$Plus:

        case PropertyIds::leftContext:
        case PropertyIds::$BackTick:

        case PropertyIds::rightContext:
        case PropertyIds::$Tick:

        case PropertyIds::$1:
        case PropertyIds::$2:
        case PropertyIds::$3:
        case PropertyIds::$4:
        case PropertyIds::$5:
        case PropertyIds::$6:
        case PropertyIds::$7:
        case PropertyIds::$8:
        case PropertyIds::$9:
        case PropertyIds::index:
            return false;
        default:
            return JavascriptFunction::IsConfigurable(propertyId);
        }
    }

    BOOL JavascriptRegExpConstructor::GetSpecialNonEnumerablePropertyName(uint32 index, Var *propertyName, ScriptContext * requestContext)
    {
        uint length = GetSpecialNonEnumerablePropertyCount();
        if (index < length)
        {
            *propertyName = requestContext->GetPropertyString(specialnonEnumPropertyIds[index]);
            return true;
        }
        return false;
    }

    // Returns the number of special non-enumerable properties this type has.
    uint JavascriptRegExpConstructor::GetSpecialNonEnumerablePropertyCount() const
    {
        return _countof(specialnonEnumPropertyIds);
    }

    // Returns the list of special properties for the type.
    PropertyId const * JavascriptRegExpConstructor::GetSpecialNonEnumerablePropertyIds() const
    {
        return specialnonEnumPropertyIds;
    }


    BOOL JavascriptRegExpConstructor::GetSpecialEnumerablePropertyName(uint32 index, JavascriptString ** propertyName, ScriptContext * requestContext)
    {
        uint length = GetSpecialEnumerablePropertyCount();
        if (index < length)
        {
            *propertyName = requestContext->GetPropertyString(specialEnumPropertyIds[index]);
            return true;
        }
        return false;
    }

    PropertyId const * JavascriptRegExpConstructor::GetSpecialEnumerablePropertyIds() const
    {
        return specialEnumPropertyIds;
    }

    // Returns the number of special non-enumerable properties this type has.
    uint JavascriptRegExpConstructor::GetSpecialEnumerablePropertyCount() const
    {
        return _countof(specialEnumPropertyIds);
    }

    // Returns the list of special properties for the type.
    PropertyId const * JavascriptRegExpConstructor::GetSpecialPropertyIds() const
    {
        return specialPropertyIds;
    }

    uint JavascriptRegExpConstructor::GetSpecialPropertyCount() const
    {
        return _countof(specialPropertyIds);
    }

    BOOL JavascriptRegExpConstructor::GetSpecialPropertyName(uint32 index, JavascriptString ** propertyName, ScriptContext * requestContext)
    {
        uint length = GetSpecialPropertyCount();
        if (index < length)
        {
            *propertyName = requestContext->GetPropertyString(specialPropertyIds[index]);
            return true;
        }
        return false;
    }

} // namespace Js
