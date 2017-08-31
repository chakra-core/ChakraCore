//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "JsrtPch.h"
#ifdef ENABLE_SCRIPT_DEBUGGING
#include "JsrtDebugUtils.h"
#include "RuntimeDebugPch.h"
#include "screrror.h"   // For CompileScriptException

void JsrtDebugUtils::AddScriptIdToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo)
{
    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::scriptId, utf8SourceInfo->GetSourceInfoId(), utf8SourceInfo->GetScriptContext());

    Js::Utf8SourceInfo* callerUtf8SourceInfo = utf8SourceInfo->GetCallerUtf8SourceInfo();
    if (callerUtf8SourceInfo != nullptr)
    {
        JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::parentScriptId, callerUtf8SourceInfo->GetSourceInfoId(), callerUtf8SourceInfo->GetScriptContext());
    }
}

void JsrtDebugUtils::AddFileNameOrScriptTypeToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo)
{
    if (utf8SourceInfo->IsDynamic())
    {
        AssertMsg(utf8SourceInfo->GetSourceContextInfo()->url == nullptr, "How come dynamic code have a url?");

        Js::FunctionBody* anyFunctionBody = utf8SourceInfo->GetAnyParsedFunction();

        LPCWSTR sourceName = (anyFunctionBody != nullptr) ? anyFunctionBody->GetSourceName() : Js::Constants::UnknownScriptCode;

        JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::scriptType, sourceName, wcslen(sourceName), utf8SourceInfo->GetScriptContext());
    }
    else
    {
        // url can be nullptr if JsParseScript/JsRunScript didn't passed any
        const char16* url = utf8SourceInfo->GetSourceContextInfo()->url == nullptr ? _u("") : utf8SourceInfo->GetSourceContextInfo()->url;
        JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::fileName, url, wcslen(url), utf8SourceInfo->GetScriptContext());
    }
}

void JsrtDebugUtils::AddLineColumnToObject(Js::DynamicObject* object, Js::FunctionBody* functionBody, int byteCodeOffset)
{
    if (functionBody != nullptr)
    {
        ULONG line = 0;
        LONG col = 0;
        if (functionBody->GetLineCharOffset(byteCodeOffset, &line, &col, false))
        {
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::line, (uint32) line, functionBody->GetScriptContext());
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::column, (int32) col, functionBody->GetScriptContext());
        }
    }
}

void JsrtDebugUtils::AddSourceLengthAndTextToObject(Js::DynamicObject* object, Js::FunctionBody* functionBody, int byteCodeOffset)
{
    Js::FunctionBody::StatementMap* statementMap = functionBody->GetEnclosingStatementMapFromByteCode(byteCodeOffset);
    Assert(statementMap != nullptr);

    LPCUTF8 source = functionBody->GetStartOfDocument(_u("Source for debugging"));
    size_t cbLength = functionBody->GetUtf8SourceInfo()->GetCbLength();
    size_t startByte = utf8::CharacterIndexToByteIndex(source, cbLength, (const charcount_t)statementMap->sourceSpan.begin);
    size_t endByte = utf8::CharacterIndexToByteIndex(source, cbLength, (const charcount_t)statementMap->sourceSpan.end);
    int cch = statementMap->sourceSpan.end - statementMap->sourceSpan.begin;

    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::sourceLength, (double)cch, functionBody->GetScriptContext());

    AutoArrayPtr<char16> sourceContent(HeapNewNoThrowArray(char16, cch + 1), cch + 1);
    if (sourceContent != nullptr)
    {
        LPCUTF8 pbStart = source + startByte;
        LPCUTF8 pbEnd = pbStart + (endByte - startByte);
        utf8::DecodeOptions options = functionBody->GetUtf8SourceInfo()->IsCesu8() ? utf8::doAllowThreeByteSurrogates : utf8::doDefault;
        utf8::DecodeUnitsIntoAndNullTerminate(sourceContent, pbStart, pbEnd, options);
        JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::sourceText, sourceContent, cch, functionBody->GetScriptContext());
    }
    else
    {
        JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::sourceText, _u(""), 1, functionBody->GetScriptContext());
    }
}

void JsrtDebugUtils::AddLineCountToObject(Js::DynamicObject * object, Js::Utf8SourceInfo * utf8SourceInfo)
{
    utf8SourceInfo->EnsureLineOffsetCache();

    Assert(utf8SourceInfo->GetLineCount() < UINT32_MAX);
    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::lineCount, (uint32)utf8SourceInfo->GetLineCount(), utf8SourceInfo->GetScriptContext());
}

void JsrtDebugUtils::AddSourceToObject(Js::DynamicObject * object, Js::Utf8SourceInfo * utf8SourceInfo)
{
    int32 cchLength = utf8SourceInfo->GetCchLength();
    AutoArrayPtr<char16> sourceContent(HeapNewNoThrowArray(char16, cchLength + 1), cchLength + 1);
    if (sourceContent != nullptr)
    {
        LPCUTF8 source = utf8SourceInfo->GetSource();
        size_t cbLength = utf8SourceInfo->GetCbLength();
        utf8::DecodeOptions options = utf8SourceInfo->IsCesu8() ? utf8::doAllowThreeByteSurrogates : utf8::doDefault;
        utf8::DecodeUnitsIntoAndNullTerminate(sourceContent, source, source + cbLength, options);
        JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::source, sourceContent, cchLength, utf8SourceInfo->GetScriptContext());
    }
    else
    {
        JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::source, _u(""), 1, utf8SourceInfo->GetScriptContext());
    }
}

void JsrtDebugUtils::AddSourceMetadataToObject(Js::DynamicObject * object, Js::Utf8SourceInfo * utf8SourceInfo)
{
    JsrtDebugUtils::AddFileNameOrScriptTypeToObject(object, utf8SourceInfo);
    JsrtDebugUtils::AddLineCountToObject(object, utf8SourceInfo);
    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::sourceLength, utf8SourceInfo->GetCchLength(), utf8SourceInfo->GetScriptContext());

    if (utf8SourceInfo->HasDebugDocument())
    {
        // Only add the script ID in cases where a debug document exists
        JsrtDebugUtils::AddScriptIdToObject(object, utf8SourceInfo);
    }
}

void JsrtDebugUtils::AddVarPropertyToObject(Js::DynamicObject * object, const char16 * propertyName, Js::Var value, Js::ScriptContext * scriptContext)
{
    const Js::PropertyRecord* propertyRecord;

    // propertyName is the DEBUGOBJECTPROPERTY from JsrtDebugPropertiesEnum so it can't have embedded null, ok to use wcslen
    scriptContext->GetOrAddPropertyRecord(propertyName, static_cast<int>(wcslen(propertyName)), &propertyRecord);

    Js::Var marshaledObj = Js::CrossSite::MarshalVar(scriptContext, value);

    if (FALSE == Js::JavascriptOperators::InitProperty(object, propertyRecord->GetPropertyId(), marshaledObj))
    {
        Assert("Failed to add property to debugger object");
    }
}

void JsrtDebugUtils::AddPropertyType(Js::DynamicObject * object, Js::IDiagObjectModelDisplay* objectDisplayRef, Js::ScriptContext * scriptContext, bool forceSetValueProp)
{
    Assert(objectDisplayRef != nullptr);
    Assert(scriptContext != nullptr);

    bool addDisplay = false;
    bool addValue = false;

    Js::Var varValue = objectDisplayRef->GetVarValue(FALSE);

    if (varValue != nullptr)
    {
        Js::TypeId typeId = Js::JavascriptOperators::GetTypeId(varValue);

        switch (typeId)
        {
        case Js::TypeIds_Undefined:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetUndefinedDisplayString(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, scriptContext->GetLibrary()->GetUndefined(), scriptContext);
            addDisplay = true;
            break;

        case Js::TypeIds_Null:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetObjectTypeDisplayString(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, scriptContext->GetLibrary()->GetNull(), scriptContext);
            addDisplay = true;
            break;

        case Js::TypeIds_Boolean:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetBooleanTypeDisplayString(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, Js::JavascriptBoolean::FromVar(varValue)->GetValue() == TRUE ? true : false, scriptContext);
            break;

        case Js::TypeIds_Integer:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetNumberTypeDisplayString(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, Js::TaggedInt::ToDouble(varValue), scriptContext);
            break;

        case Js::TypeIds_Number:
        {
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetNumberTypeDisplayString(), scriptContext);

            double numberValue = Js::JavascriptNumber::GetValue(varValue);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, numberValue, scriptContext);

            // If number is not finite (NaN, Infinity, -Infinity) or is -0 add display as well so that we can display special strings
            if (!Js::NumberUtilities::IsFinite(numberValue) || Js::JavascriptNumber::IsNegZero(numberValue))
            {
                addDisplay = true;
            }

            break;
        }
        case Js::TypeIds_Int64Number:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetNumberTypeDisplayString(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, (double)Js::JavascriptInt64Number::FromVar(varValue)->GetValue(), scriptContext);
            break;

        case Js::TypeIds_UInt64Number:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetNumberTypeDisplayString(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, (double)Js::JavascriptUInt64Number::FromVar(varValue)->GetValue(), scriptContext);
            break;

        case Js::TypeIds_String:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetStringTypeDisplayString(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, Js::JavascriptString::FromVar(varValue), scriptContext);
            break;

        case Js::TypeIds_Symbol:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetSymbolTypeDisplayString(), scriptContext);
            addDisplay = true;
            break;

        case Js::TypeIds_Function:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetFunctionTypeDisplayString(), scriptContext);
            addDisplay = true;
            break;

#ifdef ENABLE_SIMDJS
        case Js::TypeIds_SIMDFloat32x4:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetSIMDFloat32x4DisplayString(), scriptContext);
            addDisplay = true;
            break;
        case Js::TypeIds_SIMDFloat64x2:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetSIMDFloat64x2DisplayString(), scriptContext);
            addDisplay = true;
            break;
        case Js::TypeIds_SIMDInt32x4:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetSIMDInt32x4DisplayString(), scriptContext);
            addDisplay = true;
            break;
        case Js::TypeIds_SIMDInt8x16:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetSIMDInt8x16DisplayString(), scriptContext);
            addDisplay = true;
            break;
#endif // #ifdef ENABLE_SIMDJS

        case Js::TypeIds_Enumerator:
        case Js::TypeIds_HostDispatch:
        case Js::TypeIds_WithScopeObject:
        case Js::TypeIds_UndeclBlockVar:
        case Js::TypeIds_EngineInterfaceObject:
        case Js::TypeIds_WinRTDate:
            AssertMsg(false, "Not valid types");
            break;

        case Js::TypeIds_ModuleRoot:
        case Js::TypeIds_HostObject:
        case Js::TypeIds_ActivationObject:
            AssertMsg(false, "Are these valid types for debugger?");
            break;

        case Js::TypeIds_NativeIntArray:
#if ENABLE_COPYONACCESS_ARRAY
        case Js::TypeIds_CopyOnAccessNativeIntArray:
#endif
        case Js::TypeIds_NativeFloatArray:
        case Js::TypeIds_ES5Array:
        case Js::TypeIds_CharArray:
        case Js::TypeIds_BoolArray:
        case Js::TypeIds_ArrayIterator:
        case Js::TypeIds_MapIterator:
        case Js::TypeIds_SetIterator:
        case Js::TypeIds_StringIterator:
        case Js::TypeIds_VariantDate:
        case Js::TypeIds_Object:
        case Js::TypeIds_Array:
        case Js::TypeIds_Date:
        case Js::TypeIds_RegEx:
        case Js::TypeIds_Error:
        case Js::TypeIds_BooleanObject:
        case Js::TypeIds_NumberObject:
        case Js::TypeIds_StringObject:
        case Js::TypeIds_Arguments:
        case Js::TypeIds_ArrayBuffer:
        case Js::TypeIds_Int8Array:
        case Js::TypeIds_Uint8Array:
        case Js::TypeIds_Uint8ClampedArray:
        case Js::TypeIds_Int16Array:
        case Js::TypeIds_Uint16Array:
        case Js::TypeIds_Int32Array:
        case Js::TypeIds_Uint32Array:
        case Js::TypeIds_Float32Array:
        case Js::TypeIds_Float64Array:
        case Js::TypeIds_Int64Array:
        case Js::TypeIds_Uint64Array:
        case Js::TypeIds_DataView:
        case Js::TypeIds_Map:
        case Js::TypeIds_Set:
        case Js::TypeIds_WeakMap:
        case Js::TypeIds_WeakSet:
        case Js::TypeIds_SymbolObject:
        case Js::TypeIds_Generator:
        case Js::TypeIds_Promise:
        case Js::TypeIds_GlobalObject:
        case Js::TypeIds_SpreadArgument:
#ifdef ENABLE_WASM
        case Js::TypeIds_WebAssemblyModule:
        case Js::TypeIds_WebAssemblyInstance:
        case Js::TypeIds_WebAssemblyMemory:
        case Js::TypeIds_WebAssemblyTable:
#endif

        case Js::TypeIds_Proxy:
        {
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetObjectTypeDisplayString(), scriptContext);
            addDisplay = true;
            const char16* className = JsrtDebugUtils::GetClassName(typeId);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::className, className, wcslen(className), scriptContext);
            break;
        }

        default:
            AssertMsg(false, "Unhandled type");
            break;
        }
    }
    else
    {
        if (objectDisplayRef->HasChildren())
        {
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetObjectTypeDisplayString(), scriptContext);
            addDisplay = true;
            const char16* className = JsrtDebugUtils::GetClassName(Js::TypeIds_Object);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::className, className, wcslen(className), scriptContext);
        }
        else
        {
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetStringTypeDisplayString(), scriptContext);
            addValue = true;
        }
    }

    if (addDisplay || addValue)
    {
        LPCWSTR value = nullptr;

        // Getting value might call getter which can throw so wrap in try catch
        try
        {
            value = objectDisplayRef->Value(10);
        }
        catch (const Js::JavascriptException& err)
        {
            err.GetAndClear();  // discard exception object
            value = _u("");
        }

        JsrtDebugUtils::AddPropertyToObject(object, addDisplay ? JsrtDebugPropertyId::display : JsrtDebugPropertyId::value, value, wcslen(value), scriptContext);
    }

    if (forceSetValueProp && varValue != nullptr && !JsrtDebugUtils::HasProperty(object, JsrtDebugPropertyId::value, scriptContext))
    {
        JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, varValue, scriptContext);
    }

    DBGPROP_ATTRIB_FLAGS dbPropAttrib = objectDisplayRef->GetTypeAttribute();

    JsrtDebugPropertyAttribute propertyAttributes = JsrtDebugPropertyAttribute::NONE;

    if ((dbPropAttrib & DBGPROP_ATTRIB_VALUE_READONLY) == DBGPROP_ATTRIB_VALUE_READONLY)
    {
        propertyAttributes |= JsrtDebugPropertyAttribute::READ_ONLY_VALUE;
    }

    if (objectDisplayRef->HasChildren())
    {
        propertyAttributes |= JsrtDebugPropertyAttribute::HAVE_CHILDRENS;
    }

    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::propertyAttributes, (UINT)propertyAttributes, scriptContext);
}

void JsrtDebugUtils::AddVarPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, Js::Var value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, JsrtDebugUtils::GetDebugPropertyName(propertyId), value, scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, double value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptNumber::ToVarNoCheck(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, uint32 value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptNumber::ToVarNoCheck(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, int32 value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptNumber::ToVarNoCheck(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, const char16 * value, size_t len, Js::ScriptContext * scriptContext)
{
    charcount_t charCount = static_cast<charcount_t>(len);

    Assert(charCount == len);

    if (charCount == len)
    {
        JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptString::NewCopyBuffer(value, charCount, scriptContext), scriptContext);
    }
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, Js::JavascriptString* jsString, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddPropertyToObject(object, propertyId, jsString->GetSz(), jsString->GetLength(), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, bool value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptBoolean::ToVar(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, Js::Var value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, value, scriptContext);
}

bool JsrtDebugUtils::HasProperty(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, Js::ScriptContext * scriptContext)
{
    const char16* propertyName = GetDebugPropertyName(propertyId);

    const Js::PropertyRecord* propertyRecord;
    scriptContext->FindPropertyRecord(propertyName, static_cast<int>(wcslen(propertyName)), &propertyRecord);

    if (propertyRecord == nullptr)
    {
        // No property record exists, there must be no property with that name in the script context.
        return false;
    }

    return !!Js::JavascriptOperators::HasProperty(object, propertyRecord->GetPropertyId());
}

const char16 * JsrtDebugUtils::GetClassName(Js::TypeId typeId)
{
    switch (typeId)
    {
    case Js::TypeIds_Object:
    case Js::TypeIds_ArrayIterator:
    case Js::TypeIds_MapIterator:
    case Js::TypeIds_SetIterator:
    case Js::TypeIds_StringIterator:
                                        return _u("Object");

    case Js::TypeIds_Proxy:             return _u("Proxy");
    case Js::TypeIds_Array:
    case Js::TypeIds_NativeIntArray:
#if ENABLE_COPYONACCESS_ARRAY
    case Js::TypeIds_CopyOnAccessNativeIntArray:
#endif
    case Js::TypeIds_NativeFloatArray:
    case Js::TypeIds_ES5Array:
    case Js::TypeIds_CharArray:
    case Js::TypeIds_BoolArray:
                                        return _u("Array");

    case Js::TypeIds_Date:
    case Js::TypeIds_VariantDate:
                                        return _u("Date");

    case Js::TypeIds_RegEx:             return _u("RegExp");
    case Js::TypeIds_Error:             return _u("Error");
    case Js::TypeIds_BooleanObject:     return _u("Boolean");
    case Js::TypeIds_NumberObject:      return _u("Number");
    case Js::TypeIds_StringObject:      return _u("String");
    case Js::TypeIds_Arguments:         return _u("Object");
    case Js::TypeIds_ArrayBuffer:       return _u("ArrayBuffer");
    case Js::TypeIds_Int8Array:         return _u("Int8Array");
    case Js::TypeIds_Uint8Array:        return _u("Uint8Array");
    case Js::TypeIds_Uint8ClampedArray: return _u("Uint8ClampedArray");
    case Js::TypeIds_Int16Array:        return _u("Int16Array");
    case Js::TypeIds_Uint16Array:       return _u("Uint16Array");
    case Js::TypeIds_Int32Array:        return _u("Int32Array");
    case Js::TypeIds_Uint32Array:       return _u("Uint32Array");
    case Js::TypeIds_Float32Array:      return _u("Float32Array");
    case Js::TypeIds_Float64Array:      return _u("Float64Array");
    case Js::TypeIds_Int64Array:        return _u("Int64Array");
    case Js::TypeIds_Uint64Array:       return _u("Uint64Array");
    case Js::TypeIds_DataView:          return _u("DataView");
    case Js::TypeIds_Map:               return _u("Map");
    case Js::TypeIds_Set:               return _u("Set");
    case Js::TypeIds_WeakMap:           return _u("WeakMap");
    case Js::TypeIds_WeakSet:           return _u("WeakSet");
    case Js::TypeIds_SymbolObject:      return _u("Symbol");
    case Js::TypeIds_Generator:         return _u("Generator");
    case Js::TypeIds_Promise:           return _u("Promise");
    case Js::TypeIds_GlobalObject:      return _u("Object");
    case Js::TypeIds_SpreadArgument:    return _u("Spread");
#ifdef ENABLE_WASM
    case Js::TypeIds_WebAssemblyModule:  return _u("WebAssembly.Module");
    case Js::TypeIds_WebAssemblyInstance:return _u("WebAssembly.Instance");
    case Js::TypeIds_WebAssemblyMemory:  return _u("WebAssembly.Memory");
    case Js::TypeIds_WebAssemblyTable:   return _u("WebAssembly.Table");
#endif
    default:
        Assert(false);
    }
    return _u("");
}

const char16 * JsrtDebugUtils::GetDebugPropertyName(JsrtDebugPropertyId propertyId)
{
    switch (propertyId)
    {
#define DEBUGOBJECTPROPERTY(name) case JsrtDebugPropertyId::##name: return _u(###name);
#include "JsrtDebugPropertiesEnum.h"
    }

    Assert(false);
    return _u("");
}
#endif