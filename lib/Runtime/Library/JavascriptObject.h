//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptObject : public DynamicObject
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptObject, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptObject);
    public:
        JavascriptObject(DynamicType* type)  : DynamicObject(type)
        {
        }

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo HasOwnProperty;
            static FunctionInfo PropertyIsEnumerable;
            static FunctionInfo IsPrototypeOf;
            static FunctionInfo ToLocaleString;
            static FunctionInfo ToString;
            static FunctionInfo ValueOf;
            static FunctionInfo DefineProperty;
            static FunctionInfo DefineProperties;
            static FunctionInfo Create;
            static FunctionInfo GetOwnPropertyDescriptor;
            static FunctionInfo GetOwnPropertyDescriptors;
            static FunctionInfo GetPrototypeOf;
            static FunctionInfo SetPrototypeOf;
            static FunctionInfo Keys;
            static FunctionInfo GetOwnPropertyNames;
            static FunctionInfo GetOwnPropertySymbols;
            static FunctionInfo Seal;
            static FunctionInfo Freeze;
            static FunctionInfo PreventExtensions;
            static FunctionInfo IsSealed;
            static FunctionInfo IsFrozen;
            static FunctionInfo IsExtensible;
            static FunctionInfo DefineGetter;
            static FunctionInfo DefineSetter;
            static FunctionInfo LookupGetter;
            static FunctionInfo LookupSetter;
            static FunctionInfo Is;
            static FunctionInfo Assign;
            static FunctionInfo Values;
            static FunctionInfo Entries;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryHasOwnProperty(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryPropertyIsEnumerable(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsPrototypeOf(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryToLocaleString(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryToString(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryValueOf(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryDefineProperty(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryDefineProperties(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryCreate(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetOwnPropertyDescriptor(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetOwnPropertyDescriptors(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetPrototypeOf(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySetPrototypeOf(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryKeys(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetOwnPropertyNames(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetOwnPropertySymbols(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySeal(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryFreeze(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryPreventExtensions(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsSealed(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsFrozen(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsExtensible(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryDefineGetter(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryDefineSetter(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryLookupGetter(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryLookupSetter(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIs(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAssign(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryValues(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEntries(RecyclableObject* function, CallInfo callInfo, ...);

        static Var GetPrototypeOf(RecyclableObject* obj, ScriptContext* scriptContext);
        static BOOL ChangePrototype(RecyclableObject* object, RecyclableObject* newPrototype, bool validate, ScriptContext* scriptContext);

        static JavascriptArray* CreateOwnSymbolPropertiesHelper(RecyclableObject* object, ScriptContext* scriptContext);
        static JavascriptArray* CreateOwnStringPropertiesHelper(RecyclableObject* object, ScriptContext* scriptContext);
        static JavascriptArray* CreateOwnStringSymbolPropertiesHelper(RecyclableObject* object, ScriptContext* scriptContext);
        static JavascriptArray* CreateOwnEnumerableStringPropertiesHelper(RecyclableObject* object, ScriptContext* scriptContext);
        static JavascriptArray* CreateOwnEnumerableStringSymbolPropertiesHelper(RecyclableObject* object, ScriptContext* scriptContext);

        static Var GetOwnPropertyDescriptorHelper(RecyclableObject* obj, Var propertyKey, ScriptContext* scriptContext);
        static BOOL GetOwnPropertyDescriptorHelper(RecyclableObject* obj, PropertyId propertyId, ScriptContext* scriptContext, PropertyDescriptor& propertyDescriptor);

        // Param valuesToReturn should be set to true when we are looking for values from an object otherwise entries will be returned
        static Var GetValuesOrEntries(RecyclableObject* object, bool valuesToReturn, ScriptContext* scriptContext);

        // Presently used in the projection as a mechanism of calling the general object prototype toString.
        static JavascriptString* ToStringInternal(Var thisArg, ScriptContext* scriptContext)
        {
            return static_cast<JavascriptString*>(ToStringHelper(thisArg, scriptContext));
        }

        static BOOL DefineOwnPropertyHelper(RecyclableObject* obj, PropertyId propId, const PropertyDescriptor& descriptor, ScriptContext* scriptContext, bool throwOnError = true);
        static Var ToStringHelper(Var thisArg, ScriptContext* scriptContext);
        static JavascriptString* ToStringTagHelper(Var thisArg, ScriptContext* scriptContext, TypeId type);

    private:
        static void AssignForGenericObjects(RecyclableObject* from, RecyclableObject* to, ScriptContext* scriptContext);
        static void AssignForProxyObjects(RecyclableObject* from, RecyclableObject* to, ScriptContext* scriptContext);
        static JavascriptArray* CreateKeysHelper(RecyclableObject* object, ScriptContext* scriptContext, BOOL enumNonEnumerable, bool includeSymbolProperties, bool includeStringProperties, bool includeSpecialProperties);

        static void ModifyGetterSetterFuncName(const PropertyRecord * propertyRecord, const PropertyDescriptor& descriptor, ScriptContext* scriptContext);
        static char16 * ConstructName(const PropertyRecord * propertyRecord, const char16 * getOrSetStr, ScriptContext* scriptContext);
        static Var DefinePropertiesHelper(RecyclableObject* object, RecyclableObject* properties, ScriptContext* scriptContext);
        static Var DefinePropertiesHelperForGenericObjects(RecyclableObject* object, RecyclableObject* properties, ScriptContext* scriptContext);
        static Var DefinePropertiesHelperForProxyObjects(RecyclableObject* object, RecyclableObject* properties, ScriptContext* scriptContext);
        static bool IsPrototypeOf(RecyclableObject* proto, RecyclableObject* obj, ScriptContext* scriptContext);
    };
}
