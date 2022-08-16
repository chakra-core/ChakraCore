//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
namespace Js
{
    class ES5ArrayType : public DynamicType
    {
        friend class ES5Array;

    protected:
        ES5ArrayType(DynamicType * type);
    };
}
AUTO_REGISTER_RECYCLER_OBJECT_DUMPER(Js::ES5ArrayType, &Js::RecyclableObject::DumpObjectFunction);

namespace Js
{
    //
    // ES5Array supports attribute/getter/setter for index property names.
    //
    // This implementation depends on v-table swapping so that a normal JavascriptArray instance can be
    // converted to an ES5Array at runtime when ES5 attribute/getter/setter support is needed. As a result,
    // this class can't add any new fields to JavascriptArray. The extra index attribute/getter/setter info
    // are maintained in the private ES5ArrayTypeHandler.
    //
    // ES5Array does not reimplement Array.prototype methods. It depends on JavascriptArray implementations
    // to go through generic object route as ES5Array has a new TypeId and is treated as an object.
    //
    class ES5Array : public JavascriptArray
    {
    protected:
        DEFINE_VTABLE_CTOR(ES5Array, JavascriptArray);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ES5Array);

    private:
        bool GetPropertyBuiltIns(PropertyId propertyId, Var* value, BOOL* result);
        bool SetPropertyBuiltIns(PropertyId propertyId, Var value, PropertyOperationFlags flags, BOOL* result);
        bool GetSetterBuiltIns(PropertyId propertyId, PropertyValueInfo* info, DescriptorFlags* result);

    public:
        static uint32 ToLengthValue(Var value, ScriptContext* scriptContext);
        bool IsLengthWritable() const;

        virtual DynamicType* DuplicateType() override;

        // Enumerate
        BOOL IsValidDescriptorToken(void * descriptorValidationToken) const;
        uint32 GetNextDescriptor(uint32 key, IndexPropertyDescriptor** descriptor, void ** descriptorValidationToken);
        BOOL GetDescriptor(uint32 index, Js::IndexPropertyDescriptor **ppDescriptor);

        //
        // To skip JavascriptArray overrides
        //
        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info) override;
        virtual BOOL IsWritable(PropertyId propertyId) override;
        virtual BOOL SetEnumerable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetWritable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetConfigurable(PropertyId propertyId, BOOL value) override;
        virtual BOOL SetAttributes(PropertyId propertyId, PropertyAttributes attributes) override;

        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags = PropertyOperation_None, SideEffects possibleSideEffects = SideEffects_Any) override;

        virtual PropertyQueryFlags HasItemQuery(uint32 index) override;
        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override;
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;

        virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) override;

        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags) override;
        virtual BOOL PreventExtensions() override;
        virtual BOOL Seal() override;
        virtual BOOL Freeze() override;

        virtual BOOL GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, EnumeratorCache * enumeratorCache = nullptr) override;

        // objectArray support
        virtual BOOL SetItemWithAttributes(uint32 index, Var value, PropertyAttributes attributes) override;
        virtual BOOL SetItemAttributes(uint32 index, PropertyAttributes attributes) override;
        virtual BOOL SetItemAccessors(uint32 index, Var getter, Var setter) override;
        virtual BOOL IsObjectArrayFrozen() override;
        virtual JavascriptEnumerator * GetIndexEnumerator(EnumeratorFlags flags, ScriptContext* requestContext) override;

        // for SCA
        virtual BOOL IsItemEnumerable(uint32 index) override;

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    template <> inline bool VarIsImpl<ES5Array>(RecyclableObject* instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_ES5Array;
    }
}
AUTO_REGISTER_RECYCLER_OBJECT_DUMPER(Js::ES5Array, &Js::RecyclableObject::DumpObjectFunction);
