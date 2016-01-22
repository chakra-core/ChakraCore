//---------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#pragma once

class JsrtDebugUtils
{
public:
    static void AddScriptIdToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo);
    static void AddFileNameToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo);
    static void AddErrorToObject(Js::DynamicObject* object, Js::ScriptContext* scriptContext, BSTR description);
    static void AddLineColumnToObject(Js::DynamicObject* object, Js::FunctionBody* functionBody, int byteCodeOffset);
    static void AddSourceTextToObject(Js::DynamicObject* object, Js::FunctionBody* functionBody, int byteCodeOffset);
    static void AddSouceLengthToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo);
    static void AddLineCountToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo);
    static void AddSouceToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo);

    static void AddDoublePropertyToObject(Js::DynamicObject* object, const wchar_t* propertyName, double value, Js::ScriptContext* scriptContext);
    static void AddStringPropertyToObject(Js::DynamicObject* object, const wchar_t* propertyName, const wchar_t * value, Js::ScriptContext* scriptContext);
    static void AddBooleanPropertyToObject(Js::DynamicObject* object, const wchar_t* propertyName, bool value, Js::ScriptContext* scriptContext);
    static void AddVarPropertyToObject(Js::DynamicObject* object, const wchar_t* propertyName, Js::Var value, Js::ScriptContext* scriptContext);

    static void AddPropertyType(Js::DynamicObject * object, Js::IDiagObjectModelDisplay* objectDisplayRef, Js::ScriptContext * scriptContext);

private:
    static wchar_t* GetClassName(Js::TypeId typeId);
};
