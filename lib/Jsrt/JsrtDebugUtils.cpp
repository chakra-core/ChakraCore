//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "JsrtPch.h"
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

void JsrtDebugUtils::AddFileNameToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo)
{
    const wchar_t* url = utf8SourceInfo->GetSourceContextInfo()->url == nullptr ? L"" : utf8SourceInfo->GetSourceContextInfo()->url;

    if (utf8SourceInfo->IsDynamic())
    {
        Js::FunctionBody* anyFunctionBody = utf8SourceInfo->GetAnyParsedFunction();
        JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::scriptType, anyFunctionBody->GetSourceName(), utf8SourceInfo->GetScriptContext());
    }

    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::fileName, url, utf8SourceInfo->GetScriptContext());
}

void JsrtDebugUtils::AddLineColumnToObject(Js::DynamicObject* object, Js::FunctionBody* functionBody, int byteCodeOffset)
{
    if (functionBody != nullptr)
    {
        ULONG line = 0;
        LONG col = 0;
        if (functionBody->GetLineCharOffset(byteCodeOffset, &line, &col, false))
        {
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::line, line, functionBody->GetScriptContext());
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::column, col, functionBody->GetScriptContext());
        }
    }
}

void JsrtDebugUtils::AddSourceLengthAndTextToObject(Js::DynamicObject* object, Js::FunctionBody* functionBody, int byteCodeOffset)
{
    Js::FunctionBody::StatementMap* statementMap = functionBody->GetEnclosingStatementMapFromByteCode(byteCodeOffset);

    LPCUTF8 source = functionBody->GetStartOfDocument(L"Source for debugging");
    size_t startByte = utf8::CharacterIndexToByteIndex(source, functionBody->GetUtf8SourceInfo()->GetCbLength(), (const charcount_t)statementMap->sourceSpan.begin);

    int byteLength = statementMap->sourceSpan.end - statementMap->sourceSpan.begin;

    AutoArrayPtr<wchar_t> sourceContent(HeapNewNoThrowArray(wchar_t, byteLength + 1), byteLength + 1);
    if (sourceContent != nullptr)
    {
        utf8::DecodeOptions options = functionBody->GetUtf8SourceInfo()->IsCesu8() ? utf8::doAllowThreeByteSurrogates : utf8::doDefault;
        utf8::DecodeIntoAndNullTerminate(sourceContent, source + startByte, byteLength, options);
    }
    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::sourceLength, (double)byteLength, functionBody->GetScriptContext());
    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::sourceText, sourceContent != nullptr ? sourceContent : L"", functionBody->GetScriptContext());
}

void JsrtDebugUtils::AddLineCountToObject(Js::DynamicObject * object, Js::Utf8SourceInfo * utf8SourceInfo)
{
    utf8SourceInfo->EnsureLineOffsetCache();

    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::lineCount, (ULONG)utf8SourceInfo->GetLineCount(), utf8SourceInfo->GetScriptContext());
}

void JsrtDebugUtils::AddSouceToObject(Js::DynamicObject * object, Js::Utf8SourceInfo * utf8SourceInfo)
{
    int32 cchLength = utf8SourceInfo->GetCchLength();

    AutoArrayPtr<wchar_t> sourceContent(HeapNewNoThrowArray(wchar_t, cchLength + 1), cchLength + 1);
    if (sourceContent != nullptr)
    {
        utf8::DecodeOptions options = utf8SourceInfo->IsCesu8() ? utf8::doAllowThreeByteSurrogates : utf8::doDefault;
        utf8::DecodeIntoAndNullTerminate(sourceContent, utf8SourceInfo->GetSource(), cchLength, options);
    }

    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::source, sourceContent, utf8SourceInfo->GetScriptContext());
}

void JsrtDebugUtils::AddVarPropertyToObject(Js::DynamicObject * object, const wchar_t * propertyName, Js::Var value, Js::ScriptContext * scriptContext)
{
    const Js::PropertyRecord* propertyRecord;

    scriptContext->GetOrAddPropertyRecord(propertyName, static_cast<int>(wcslen(propertyName)), &propertyRecord);

    Js::Var marshaledObj = Js::CrossSite::MarshalVar(scriptContext, value);

    if (FALSE == Js::JavascriptOperators::InitProperty(object, propertyRecord->GetPropertyId(), marshaledObj))
    {
        Assert("Failed to add property to debugger object");
    }
}

void JsrtDebugUtils::AddPropertyType(Js::DynamicObject * object, Js::IDiagObjectModelDisplay* objectDisplayRef, Js::ScriptContext * scriptContext)
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
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetUndefinedDisplayString()->GetSz(), scriptContext);
            addDisplay = true;
            break;

        case Js::TypeIds_Null:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetObjectTypeDisplayString()->GetSz(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, scriptContext->GetLibrary()->GetNull(), scriptContext);
            break;

        case Js::TypeIds_Boolean:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetBooleanTypeDisplayString()->GetSz(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, Js::JavascriptBoolean::FromVar(varValue)->GetValue() == TRUE ? true : false, scriptContext);
            break;

        case Js::TypeIds_Integer:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetNumberTypeDisplayString()->GetSz(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, Js::TaggedInt::ToDouble(varValue), scriptContext);
            break;

        case Js::TypeIds_Number:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetNumberTypeDisplayString()->GetSz(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, Js::JavascriptNumber::GetValue(varValue), scriptContext);
            break;

        case Js::TypeIds_Int64Number:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetNumberTypeDisplayString()->GetSz(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, (double)Js::JavascriptInt64Number::FromVar(varValue)->GetValue(), scriptContext);
            break;

        case Js::TypeIds_UInt64Number:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetNumberTypeDisplayString()->GetSz(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, (double)Js::JavascriptUInt64Number::FromVar(varValue)->GetValue(), scriptContext);
            break;

        case Js::TypeIds_String:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetStringTypeDisplayString()->GetSz(), scriptContext);
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::value, Js::JavascriptString::FromVar(varValue)->GetSz(), scriptContext);
            break;

        case Js::TypeIds_Symbol:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetSymbolTypeDisplayString()->GetSz(), scriptContext);
            addDisplay = true;
            break;

        case Js::TypeIds_Function:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetFunctionTypeDisplayString()->GetSz(), scriptContext);
            addDisplay = true;
            break;

        case Js::TypeIds_SIMDFloat32x4:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetSIMDFloat32x4DisplayString()->GetSz(), scriptContext);
            addDisplay = true;
            break;
        case Js::TypeIds_SIMDFloat64x2:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetSIMDFloat64x2DisplayString()->GetSz(), scriptContext);
            addDisplay = true;
            break;
        case Js::TypeIds_SIMDInt32x4:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetSIMDInt32x4DisplayString()->GetSz(), scriptContext);
            addDisplay = true;
            break;
        case Js::TypeIds_SIMDInt8x16:
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetSIMDInt8x16DisplayString()->GetSz(), scriptContext);
            addDisplay = true;
            break;

        case Js::TypeIds_Enumerator:
        case Js::TypeIds_HostDispatch:
        case Js::TypeIds_WithScopeObject:
        case Js::TypeIds_UndeclBlockVar:
        case Js::TypeIds_EngineInterfaceObject:
        case Js::TypeIds_WinRTDate:
            AssertMsg(false, "Not valid types");
            break;

        case Js::TypeIds_JavascriptEnumeratorIterator:
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

        case Js::TypeIds_Proxy:

            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetObjectTypeDisplayString()->GetSz(), scriptContext);
            addDisplay = true;
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::className, JsrtDebugUtils::GetClassName(typeId), scriptContext);
            break;

        default:
            AssertMsg(false, "Unhandled type");
            break;
        }
    }
    else
    {
        if (objectDisplayRef->HasChildren())
        {
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetObjectTypeDisplayString()->GetSz(), scriptContext);
            addDisplay = true;
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::className, JsrtDebugUtils::GetClassName(Js::TypeIds_Object), scriptContext);
        }
        else
        {
            JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::type, scriptContext->GetLibrary()->GetStringTypeDisplayString()->GetSz(), scriptContext);
            addValue = true;
        }
    }

    if (addDisplay || addValue)
    {
        LPCWSTR value = nullptr;

        // Getting value might call getter which can throw
        try
        {
            value = objectDisplayRef->Value(10);
        }
        catch (Js::JavascriptExceptionObject*)
        {
            value = L"";
        }

        JsrtDebugUtils::AddPropertyToObject(object, addDisplay ? JsrtDebugPropertyId::display : JsrtDebugPropertyId::value, value, scriptContext);
    }

    DBGPROP_ATTRIB_FLAGS dbPropAttrib = objectDisplayRef->GetTypeAttribute();

    uint propertyAttributes = PROPERTY_ATTRIBUTE_NONE;

    if ((dbPropAttrib & DBGPROP_ATTRIB_VALUE_READONLY) == DBGPROP_ATTRIB_VALUE_READONLY)
    {
        propertyAttributes |= PROPERTY_ATTRIBUTE_READ_ONLY_VALUE;
    }

    if (objectDisplayRef->HasChildren())
    {
        propertyAttributes |= PROPERTY_ATTRIBUTE_HAVE_CHILDRENS;
    }

    JsrtDebugUtils::AddPropertyToObject(object, JsrtDebugPropertyId::propertyAttributes, propertyAttributes, scriptContext);
}

void JsrtDebugUtils::AddVarPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, Js::Var value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, JsrtDebugUtils::GetDebugPropertyName(propertyId), value, scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, double value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptNumber::ToVarNoCheck(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, UINT value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptNumber::ToVarNoCheck(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, ULONG value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptNumber::ToVarNoCheck(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, LONG value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptNumber::ToVarNoCheck(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, const wchar_t * value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptString::NewCopyBuffer(value, static_cast<charcount_t>(wcslen(value)), scriptContext), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, bool value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, Js::JavascriptBoolean::ToVar(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddPropertyToObject(Js::DynamicObject * object, JsrtDebugPropertyId propertyId, Js::Var value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyId, value, scriptContext);
}

wchar_t * JsrtDebugUtils::GetClassName(Js::TypeId typeId)
{
    switch (typeId)
    {
    case Js::TypeIds_Object:
    case Js::TypeIds_ArrayIterator:
    case Js::TypeIds_MapIterator:
    case Js::TypeIds_SetIterator:
    case Js::TypeIds_StringIterator:
        return L"Object";
    case Js::TypeIds_Proxy: return L"Proxy";
    case Js::TypeIds_Array:
    case Js::TypeIds_NativeIntArray:
#if ENABLE_COPYONACCESS_ARRAY
    case Js::TypeIds_CopyOnAccessNativeIntArray:
#endif
    case Js::TypeIds_NativeFloatArray:
    case Js::TypeIds_ES5Array:
    case Js::TypeIds_CharArray:
    case Js::TypeIds_BoolArray:
        return L"Array";
    case Js::TypeIds_Date:
    case Js::TypeIds_VariantDate:
        return L"Date";
    case Js::TypeIds_RegEx: return L"RegExp";
    case Js::TypeIds_Error: return L"Error";
    case Js::TypeIds_BooleanObject: return L"Boolean";
    case Js::TypeIds_NumberObject: return L"Number";
    case Js::TypeIds_StringObject: return L"String";
    case Js::TypeIds_Arguments: return L"Object";
    case Js::TypeIds_ArrayBuffer: return L"ArrayBuffer";
    case Js::TypeIds_Int8Array: return L"Int8Array";
    case Js::TypeIds_Uint8Array: return L"Uint8Array";
    case Js::TypeIds_Uint8ClampedArray: return L"Uint8ClampedArray";
    case Js::TypeIds_Int16Array: return L"Int16Array";
    case Js::TypeIds_Uint16Array: return L"Uint16Array";
    case Js::TypeIds_Int32Array: return L"Int32Array";
    case Js::TypeIds_Uint32Array: return L"Uint32Array";
    case Js::TypeIds_Float32Array: return L"Float32Array";
    case Js::TypeIds_Float64Array: return L"Float64Array";
    case Js::TypeIds_Int64Array: return L"Int64Array";
    case Js::TypeIds_Uint64Array: return L"Uint64Array";
    case Js::TypeIds_DataView: return L"DataView";
    case Js::TypeIds_Map: return L"Map";
    case Js::TypeIds_Set: return L"Set";
    case Js::TypeIds_WeakMap: return L"WeakMap";
    case Js::TypeIds_WeakSet: return L"WeakSet";
    case Js::TypeIds_SymbolObject: return L"Symbol";
    case Js::TypeIds_Generator: return L"Generator";
    case Js::TypeIds_Promise: return L"Promise";
    case Js::TypeIds_GlobalObject: return L"Object";
    case Js::TypeIds_SpreadArgument: return L"Spread";

    default:
        Assert(false);
    }
    return L"";
}

wchar_t * JsrtDebugUtils::GetDebugPropertyName(JsrtDebugPropertyId propertyId)
{
    switch (propertyId)
    {
#define DEBUGOBJECTPROPERTY(name) case JsrtDebugPropertyId::##name: return L###name;
#include "JsrtDebugPropertiesEnum.h"
    }
    return nullptr;
}
