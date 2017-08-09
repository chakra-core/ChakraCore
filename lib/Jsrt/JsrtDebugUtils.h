//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

BEGIN_ENUM_UINT(JsrtDebugPropertyId)
#define DEBUGOBJECTPROPERTY(n) n,
#include "JsrtDebugPropertiesEnum.h"
END_ENUM_UINT()

BEGIN_ENUM_UINT(JsrtDebugPropertyAttribute)
    NONE = 0x1,
    HAVE_CHILDRENS = 0x2,
    READ_ONLY_VALUE = 0x4,
END_ENUM_UINT()

class JsrtDebugUtils
{
public:
    static void AddScriptIdToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo);
    static void AddFileNameOrScriptTypeToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo);
    static void AddLineColumnToObject(Js::DynamicObject* object, Js::FunctionBody* functionBody, int byteCodeOffset);
    static void AddSourceLengthAndTextToObject(Js::DynamicObject* object, Js::FunctionBody* functionBody, int byteCodeOffset);
    static void AddLineCountToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo);
    static void AddSourceToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo);
    static void AddSourceMetadataToObject(Js::DynamicObject* object, Js::Utf8SourceInfo* utf8SourceInfo);

    static void AddVarPropertyToObject(Js::DynamicObject* object, const char16* propertyName, Js::Var value, Js::ScriptContext* scriptContext);
    static void AddPropertyToObject(Js::DynamicObject* object, JsrtDebugPropertyId propertyId, double value, Js::ScriptContext* scriptContext);
    static void AddPropertyToObject(Js::DynamicObject* object, JsrtDebugPropertyId propertyId, uint32 value, Js::ScriptContext* scriptContext);
    static void AddPropertyToObject(Js::DynamicObject* object, JsrtDebugPropertyId propertyId, int32 value, Js::ScriptContext* scriptContext);
    static void AddPropertyToObject(Js::DynamicObject* object, JsrtDebugPropertyId propertyId, const char16 * value, size_t len, Js::ScriptContext* scriptContext);
    static void AddPropertyToObject(Js::DynamicObject* object, JsrtDebugPropertyId propertyId, Js::JavascriptString* jsString, Js::ScriptContext* scriptContext);
    static void AddPropertyToObject(Js::DynamicObject* object, JsrtDebugPropertyId propertyId, bool value, Js::ScriptContext* scriptContext);
    static void AddPropertyToObject(Js::DynamicObject* object, JsrtDebugPropertyId propertyId, Js::Var value, Js::ScriptContext* scriptContext);

    static void AddPropertyType(Js::DynamicObject * object, Js::IDiagObjectModelDisplay* objectDisplayRef, Js::ScriptContext * scriptContext, bool forceSetValueProp);

private:
    static void AddVarPropertyToObject(Js::DynamicObject* object, JsrtDebugPropertyId propertyId, Js::Var value, Js::ScriptContext* scriptContext);
    static bool HasProperty(Js::DynamicObject* object, JsrtDebugPropertyId propertyId, Js::ScriptContext * scriptContext);
    static const char16* GetClassName(Js::TypeId typeId);
    static const char16* GetDebugPropertyName(JsrtDebugPropertyId propertyId);
};
