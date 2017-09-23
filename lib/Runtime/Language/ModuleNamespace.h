//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ModuleNamespace : public DynamicObject
    {
    public:
        friend class ModuleNamespaceEnumerator;
        typedef JsUtil::BaseDictionary<PropertyId, ModuleNameRecord, RecyclerLeafAllocator, PowerOf2SizePolicy> UnambiguousExportMap;
        typedef JsUtil::BaseDictionary<const PropertyRecord*, SimpleDictionaryPropertyDescriptor<BigPropertyIndex>, RecyclerNonLeafAllocator,
            DictionarySizePolicy<PowerOf2Policy, 1>, PropertyRecordStringHashComparer, PropertyMapKeyTraits<const PropertyRecord*>::template Entry>
            SimplePropertyDescriptorMap;
    protected:
        DEFINE_VTABLE_CTOR(ModuleNamespace, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ModuleNamespace);

    protected:
        ModuleNamespace(ModuleRecordBase* moduleRecord, DynamicType * type);
        static ModuleNamespace* New(ModuleRecordBase* moduleRecord);
    public:

        class EntryInfo
        {
        };

        static ModuleNamespace* GetModuleNamespace(ModuleRecordBase* moduleRecord);
        void Initialize();
        ListForListIterator* GetSortedExportedNames() { return this->sortedExportedNames; }
        static bool Is(Var aValue) {  return JavascriptOperators::GetTypeId(aValue) == TypeIds_ModuleNamespace; }
        static ModuleNamespace* FromVar(Var obj) { Assert(JavascriptOperators::GetTypeId(obj) == TypeIds_ModuleNamespace); return static_cast<ModuleNamespace*>(obj); }

        virtual PropertyId GetPropertyId(BigPropertyIndex index) override;
        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId) override;
        virtual BOOL HasOwnProperty(PropertyId propertyId) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetInternalProperty(Var instance, PropertyId internalPropertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override { return FALSE; }
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override { return FALSE; }
        virtual BOOL SetInternalProperty(PropertyId internalPropertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override { return FALSE; }
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override { return DescriptorFlags::None; }
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override { return DescriptorFlags::None; }
        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = nullptr) override { Assert(false); return FALSE; }
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override { return false; }
        virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags) override;
#if ENABLE_FIXED_FIELDS
        virtual BOOL IsFixedProperty(PropertyId propertyId) override { return false; }
#endif
        virtual PropertyQueryFlags HasItemQuery(uint32 index) override { return PropertyQueryFlags::Property_NotFound; }
        virtual BOOL HasOwnItem(uint32 index) override { return false; }
        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override { return PropertyQueryFlags::Property_NotFound; }
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override { return PropertyQueryFlags::Property_NotFound; }
        virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) override { *setterValue = nullptr; return DescriptorFlags::None; }
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override { return false; }
        virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override { return true; }
        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache = nullptr);
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None) override { return false; }
        virtual BOOL GetAccessors(PropertyId propertyId, Var *getter, Var *setter, ScriptContext * requestContext) override { return false; }
        virtual BOOL IsWritable(PropertyId propertyId) override;
        virtual BOOL IsConfigurable(PropertyId propertyId) override;
        virtual BOOL IsEnumerable(PropertyId propertyId) override;
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override { return false; }
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override { return false; }
        virtual BOOL IsProtoImmutable() const { return true; }
        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override { return false; }
        virtual BOOL SetAttributes(PropertyId propertyId, PropertyAttributes attributes) override { return false; }
        virtual BOOL IsExtensible() override { return false; };
        virtual BOOL PreventExtensions() override { return true; }
        virtual BOOL Seal() override { return false; }
        virtual BOOL Freeze() override { return false; }
        virtual BOOL IsSealed() override { return true; }
        virtual BOOL IsFrozen() override { return true; }
        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        virtual void RemoveFromPrototype(ScriptContext * requestContext) override { Assert(false); }
        virtual void AddToPrototype(ScriptContext * requestContext) override { Assert(false); }
        virtual void SetPrototype(RecyclableObject* newPrototype) override { Assert(false); return; }

    private:
        Field(ModuleRecordBase*) moduleRecord;
        Field(UnambiguousExportMap*) unambiguousNonLocalExports;
        Field(SimplePropertyDescriptorMap*) propertyMap;   // local exports.
        Field(ListForListIterator*) sortedExportedNames;   // sorted exported names for both local and indirect exports; excludes symbols.
        Field(Field(Var)*) nsSlots;

        void SetNSSlotsForModuleNS(Field(Var)* nsSlot) { this->nsSlots = nsSlot; }
        Var GetNSSlot(BigPropertyIndex propertyIndex);
        void AddUnambiguousNonLocalExport(PropertyId exportId, ModuleNameRecord* nonLocalExportNameRecord);
        UnambiguousExportMap* GetUnambiguousNonLocalExports() const { return unambiguousNonLocalExports; }

        // Methods used by NamespaceEnumerator;
        BOOL FindNextProperty(BigPropertyIndex& index, JavascriptString** propertyString, PropertyId* propertyId, PropertyAttributes* attributes, ScriptContext * requestContext) const;
    };
}
