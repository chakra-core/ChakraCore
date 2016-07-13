//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ModuleNamespace : public DynamicObject
    {
        typedef JsUtil::BaseDictionary<PropertyId, ModuleNameRecord, RecyclerLeafAllocator, PowerOf2SizePolicy> UnambiguousExportMap;

    protected:
        DEFINE_VTABLE_CTOR(ModuleNamespace, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ModuleNamespace);

    protected:
        ModuleNamespace(ModuleRecordBase* moduleRecord, DynamicType * type);
        static ModuleNamespace* New(ModuleRecordBase* moduleRecord);
    public:
        static ModuleNamespace* GetModuleNamespace(ModuleRecordBase* moduleRecord);

        //virtual int GetPropertyCount() override;
        //virtual PropertyId GetPropertyId(PropertyIndex index) override;
        //virtual PropertyId GetPropertyId(BigPropertyIndex index) override;
        //virtual BOOL HasProperty(PropertyId propertyId) override;
        //virtual BOOL HasOwnProperty(PropertyId propertyId) override;
        //virtual BOOL GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        //virtual BOOL GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        //virtual BOOL GetInternalProperty(Var instance, PropertyId internalPropertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        //virtual BOOL GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        //virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        //virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override { return false; }
        //virtual BOOL SetInternalProperty(PropertyId internalPropertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override { return false; }
        //virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        //virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        //virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = nullptr) override { return false; }
        //virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override { return false; }
        //virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override;
        //virtual BOOL IsFixedProperty(PropertyId propertyId) override;
        virtual BOOL HasItem(uint32 index) override { return false; }
        virtual BOOL HasOwnItem(uint32 index) override { return false; }
        virtual BOOL GetItem(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override { return false; }
        virtual BOOL GetItemReference(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override { return false; }
        virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) override { *setterValue = nullptr; return DescriptorFlags::None; }
        //virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override;
        //virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override;
        //virtual BOOL ToPrimitive(JavascriptHint hint, Var* result, ScriptContext * requestContext) override;
        //virtual BOOL GetEnumerator(BOOL enumNonEnumerable, Var* enumerator, ScriptContext* scriptContext, bool preferSnapshotSemantics = true, bool enumSymbols = false) override;
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override { return false; }
        virtual BOOL GetAccessors(PropertyId propertyId, Var *getter, Var *setter, ScriptContext * requestContext) override { return false; }
        virtual BOOL IsWritable(PropertyId propertyId) override { return true; }
        virtual BOOL IsConfigurable(PropertyId propertyId) override { return false; }
        virtual BOOL IsEnumerable(PropertyId propertyId) override { return true; }
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override { return false; }
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override { return false; }
        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override { return false; }
        //virtual BOOL SetAttributes(PropertyId propertyId, PropertyAttributes attributes) override;
        virtual BOOL IsExtensible() override { return false; };
        virtual BOOL PreventExtensions() override { return true; }
        //virtual BOOL Seal() override;
        //virtual BOOL Freeze() override;
        virtual BOOL IsSealed() override { return true; }
        virtual BOOL IsFrozen() override { return true; }
        //virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        //virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        //virtual Var GetTypeOfString(ScriptContext * requestContext) override;

        //virtual void RemoveFromPrototype(ScriptContext * requestContext) override;
        //virtual void AddToPrototype(ScriptContext * requestContext) override;
        virtual void SetPrototype(RecyclableObject* newPrototype) override { return; }

    private:
        ModuleRecordBase* moduleRecord;
        UnambiguousExportMap* unambiguousNonLocalExports;
        // this is a special case where we can override the slot to the "export slots". 
        void SetAuxSlotsForModuleNS(Var* auxSlot) { this->auxSlots = auxSlot; }
        void AddUnambiguousNonLocalExport(PropertyId exportId, ModuleNameRecord* nonLocalExportNameRecord);
        UnambiguousExportMap* GetUnambiguousNonLocalExports() const { return unambiguousNonLocalExports; }
    };
}
