//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "JsrtPch.h"
#include "JsrtDebugUtils.h"
#include "RuntimeDebugPch.h"
#include "screrror.h"   // For CompileScriptException

void JsrtDebugUtils::AddScriptIdToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo)
{
    JsrtDebugUtils::AddDoublePropertyToObject(object, L"scriptId", utf8SourceInfo->GetSourceInfoId(), utf8SourceInfo->GetScriptContext());

    Js::Utf8SourceInfo* callerUtf8SourceInfo = utf8SourceInfo->GetCallerUtf8SourceInfo();
    if (callerUtf8SourceInfo != nullptr)
    {
        JsrtDebugUtils::AddDoublePropertyToObject(object, L"parentScriptId", callerUtf8SourceInfo->GetSourceInfoId(), callerUtf8SourceInfo->GetScriptContext());
    }
}

void JsrtDebugUtils::AddFileNameToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo)
{
    const wchar_t* url = utf8SourceInfo->GetSourceContextInfo()->url == nullptr ? L"" : utf8SourceInfo->GetSourceContextInfo()->url;

    JsrtDebugUtils::AddStringPropertyToObject(object, L"fileName", url, utf8SourceInfo->GetScriptContext());
}

void JsrtDebugUtils::AddErrorToObject(Js::DynamicObject* object, Js::ScriptContext* scriptContext, BSTR description)
{
    Assert(description != nullptr);

    JsrtDebugUtils::AddStringPropertyToObject(object, L"error", description, scriptContext);
}

void JsrtDebugUtils::AddLineColumnToObject(Js::DynamicObject* object, Js::FunctionBody* functionBody, int byteCodeOffset)
{
    if (functionBody != nullptr)
    {
        ULONG line = 0;
        LONG col = 0;
        if (functionBody->GetLineCharOffset(byteCodeOffset, &line, &col, false))
        {
            JsrtDebugUtils::AddDoublePropertyToObject(object, L"line", line, functionBody->GetScriptContext());
            JsrtDebugUtils::AddDoublePropertyToObject(object, L"column", col, functionBody->GetScriptContext());
        }
    }
}

void JsrtDebugUtils::AddSourceTextToObject(Js::DynamicObject* object, Js::FunctionBody* functionBody, int byteCodeOffset)
{
    Js::FunctionBody::StatementMap* statementMap = functionBody->GetEnclosingStatementMapFromByteCode(byteCodeOffset);

    LPCUTF8 source = functionBody->GetStartOfDocument(L"");
    size_t startByte = utf8::CharacterIndexToByteIndex(source, functionBody->GetUtf8SourceInfo()->GetCbLength(), (const charcount_t)statementMap->sourceSpan.begin);

    int32 byteLength = statementMap->sourceSpan.end - statementMap->sourceSpan.begin;

    AutoArrayPtr<wchar_t> sourceContent(HeapNewNoThrowArray(wchar_t, byteLength + 1), byteLength + 1);
    if (sourceContent != nullptr)
    {
        utf8::DecodeOptions options = functionBody->GetUtf8SourceInfo()->IsCesu8() ? utf8::doAllowThreeByteSurrogates : utf8::doDefault;
        utf8::DecodeIntoAndNullTerminate(sourceContent, source + startByte, byteLength, options);
    }

    JsrtDebugUtils::AddStringPropertyToObject(object, L"sourceText", sourceContent, functionBody->GetScriptContext());
}

void JsrtDebugUtils::AddSouceLengthToObject(Js::DynamicObject * object, Js::Utf8SourceInfo * utf8SourceInfo)
{
    JsrtDebugUtils::AddDoublePropertyToObject(object, L"sourceLength", utf8SourceInfo->GetCchLength(), utf8SourceInfo->GetScriptContext());
}

void JsrtDebugUtils::AddLineCountToObject(Js::DynamicObject * object, Js::Utf8SourceInfo * utf8SourceInfo)
{
    utf8SourceInfo->EnsureLineOffsetCache();

    JsrtDebugUtils::AddDoublePropertyToObject(object, L"lineCount", utf8SourceInfo->GetLineCount(), utf8SourceInfo->GetScriptContext());
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

    JsrtDebugUtils::AddStringPropertyToObject(object, L"source", sourceContent, utf8SourceInfo->GetScriptContext());
}

void JsrtDebugUtils::AddDoublePropertyToObject(Js::DynamicObject * object, const wchar_t * propertyName, double value, Js::ScriptContext* scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyName, Js::JavascriptNumber::ToVarNoCheck(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddStringPropertyToObject(Js::DynamicObject * object, const wchar_t * propertyName, const wchar_t * value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyName, Js::JavascriptString::NewCopyBuffer(value, wcslen(value), scriptContext), scriptContext);
}

void JsrtDebugUtils::AddBooleanPropertyToObject(Js::DynamicObject * object, const wchar_t * propertyName, bool value, Js::ScriptContext * scriptContext)
{
    JsrtDebugUtils::AddVarPropertyToObject(object, propertyName, Js::JavascriptBoolean::ToVar(value, scriptContext), scriptContext);
}

void JsrtDebugUtils::AddVarPropertyToObject(Js::DynamicObject * object, const wchar_t * propertyName, Js::Var value, Js::ScriptContext * scriptContext)
{
    const Js::PropertyRecord* propertyRecord;

    scriptContext->GetOrAddPropertyRecord(propertyName, wcslen(propertyName), &propertyRecord);

    if (FALSE == Js::JavascriptOperators::InitProperty(object, propertyRecord->GetPropertyId(), value))
    {
        Assert("Failed to add property to debugger object");
    }
}
