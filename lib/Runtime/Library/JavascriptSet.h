//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptSet : public DynamicObject
    {
    public:
        typedef MapOrSetDataNode<Var> SetDataNode;
        typedef MapOrSetDataList<Var> SetDataList;
        typedef JsUtil::BaseDictionary<Var, SetDataNode*, Recycler, PowerOf2SizePolicy, SameValueZeroComparer> ComplexVarDataSet;
        typedef JsUtil::BaseDictionary<Var, SetDataNode*, Recycler> SimpleVarDataSet;

    private:
        enum class SetKind : uint8
        {
            // An EmptySet is a set containing no elements
            EmptySet,
            // An IntSet is a set containing only int elements
            //
            // Adding any TaggedInt or JavascriptNumber that can be represented as a TaggedInt
            // will succeed and be stored as an int32 in the set, EXCEPT for the value -1
            // Adding any other value will cause the set to be promoted to a SimpleVarSet or ComplexVarSet,
            // depending on the value being added
            //
            // Deleting any element will also cause the set to be promoted to a SimpleVarSet
            IntSet,
            // A SimpleVarSet is a set containing only Vars which are comparable by pointer, and don't require
            // value comparison
            //
            // Addition of a Var that is not comparable by pointer value causes the set to be promoted to a ComplexVarSet
            SimpleVarSet,
            // A ComplexVarSet is a set containing Vars for which we must inspect the values to do a comparison
            // This includes Strings and (sometimes) JavascriptNumbers
            ComplexVarSet
        };

        Field(SetDataList) list;
        union SetUnion
        {
            Field(SimpleVarDataSet*) simpleVarSet;
            Field(ComplexVarDataSet*) complexVarSet;
            Field(BVSparse<Recycler>*) intSet;
            SetUnion() {}
        };

        Field(SetUnion) u;

        Field(SetKind) kind = SetKind::EmptySet;

        DEFINE_VTABLE_CTOR_MEMBER_INIT(JavascriptSet, DynamicObject, list);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptSet);

        template <typename T>
        T* CreateVarSetFromList(uint initialCapacity);
        void PromoteToSimpleVarSet();
        void PromoteToComplexVarSet();

        void AddToEmptySet(Var value);
        bool TryAddToIntSet(Var value);
        bool TryAddToSimpleVarSet(Var value);
        void AddToComplexVarSet(Var value);

        bool IsInIntSet(Var value);

        template <bool isComplex>
        bool DeleteFromVarSet(Var value);
        bool DeleteFromSimpleVarSet(Var value);
    public:
        JavascriptSet(DynamicType* type);

        static JavascriptSet* New(ScriptContext* scriptContext);

        void Add(Var value);

        void Clear();

        bool Delete(Var value);
        bool Has(Var value);
        int Size();

        SetDataList::Iterator GetIterator();

        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Add;
            static FunctionInfo Clear;
            static FunctionInfo Delete;
            static FunctionInfo ForEach;
            static FunctionInfo Has;
            static FunctionInfo SizeGetter;
            static FunctionInfo Entries;
            static FunctionInfo Values;
            static FunctionInfo GetterSymbolSpecies;
        };
        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryAdd(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryClear(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryDelete(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryForEach(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryHas(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySizeGetter(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEntries(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryValues(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...);

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;

        static JavascriptSet* CreateForSnapshotRestore(ScriptContext* ctx);
#endif
    };

    template <> inline bool VarIsImpl<JavascriptSet>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_Set;
    }
}
