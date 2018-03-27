//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    // A wrapper corresponds to a named item coming from the host.
    // it maintains the IDispatch* pointer of the named item.
    // this is used in setting up the scope for scoped operations. see javascriptoperators.cpp
    class ModuleRoot : public RootObjectBase
    {
    protected:
        DEFINE_VTABLE_CTOR(ModuleRoot, RootObjectBase);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ModuleRoot);

    public:
        ModuleRoot(DynamicType * type);
        void SetHostObject(ModuleID moduleID, HostObjectBase * hostObject);

        virtual BOOL InitPropertyScoped(PropertyId propertyId, Var value) override;
        virtual BOOL InitFuncScoped(PropertyId propertyId, Var value) override;
        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId) override;
        virtual BOOL HasOwnProperty(PropertyId propertyId) override;
        virtual BOOL UseDynamicObjectForNoHostObjectAccess() override { return TRUE; }
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetAccessors(PropertyId propertyId, Var* getter, Var* setter, ScriptContext * requestContext) override;
        virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags HasItemQuery(uint32 index) override;
        virtual BOOL HasOwnItem(uint32 index) override;
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        virtual BOOL EnsureProperty(PropertyId propertyId) override sealed;

        virtual BOOL HasRootProperty(PropertyId propertyId) override;
        virtual BOOL GetRootProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetRootPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetRootProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL DeleteRootProperty(PropertyId propertyId, PropertyOperationFlags flags) override;

        ModuleID GetModuleID() { return moduleID;}
        static bool Is(Var aValue);

    protected:
        // For module binder, there is only one IDispatch* associated with the name provided
        // by the host when we can IActiveScriptSite::GetItemInfo.
        ModuleID moduleID;
    };
}
