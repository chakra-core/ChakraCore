//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptMap : public DynamicObject
    {
    public:
        typedef JsUtil::KeyValuePair<Field(Var), Field(Var)> MapDataKeyValuePair;
        typedef MapOrSetDataNode<MapDataKeyValuePair> MapDataNode;
        typedef MapOrSetDataList<MapDataKeyValuePair> MapDataList;
        typedef JsUtil::BaseDictionary<Var, MapDataNode*, Recycler> SimpleVarDataMap;
        typedef JsUtil::BaseDictionary<Var, MapDataNode*, Recycler, PowerOf2SizePolicy, SameValueZeroComparer> ComplexVarDataMap;

    private:
        enum class MapKind : uint8
        {
            // An EmptyMap is a map containing no elements
            EmptyMap,
            // A SimpleVarMap is a map containing only Vars which are comparable by pointer, and don't require
            // value comparison
            //
            // Addition of a Var that is not comparable by pointer value causes the set to be promoted to a ComplexVarSet
            SimpleVarMap,
            // A ComplexVarMap is a map containing Vars for which we must inspect the values to do a comparison
            // This includes Strings and (sometimes) JavascriptNumbers
            ComplexVarMap
        };

        Field(MapDataList) list;

        union MapUnion
        {
            Field(SimpleVarDataMap*) simpleVarMap;
            Field(ComplexVarDataMap*) complexVarMap;
            MapUnion() {}
        };

        Field(MapUnion) u;

        Field(MapKind) kind = MapKind::EmptyMap;

        DEFINE_VTABLE_CTOR_MEMBER_INIT(JavascriptMap, DynamicObject, list);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptMap);

        template <bool isComplex>
        bool DeleteFromVarMap(Var value);
        bool DeleteFromSimpleVarMap(Var value);

        void SetOnEmptyMap(Var key, Var value);
        bool TrySetOnSimpleVarMap(Var key, Var value);
        void SetOnComplexVarMap(Var key, Var value);

        void PromoteToComplexVarMap();
    public:
        JavascriptMap(DynamicType* type);

        static JavascriptMap* New(ScriptContext* scriptContext);

        void Clear();

        bool Delete(Var key);

        bool Get(Var key, Var* value);
        bool Has(Var key);

        void Set(Var key, Var value);

        int Size();

        MapDataList::Iterator GetIterator();

        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Clear;
            static FunctionInfo Delete;
            static FunctionInfo ForEach;
            static FunctionInfo Get;
            static FunctionInfo Has;
            static FunctionInfo Set;
            static FunctionInfo SizeGetter;
            static FunctionInfo Entries;
            static FunctionInfo Keys;
            static FunctionInfo Values;
            static FunctionInfo GetterSymbolSpecies;
        };
        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryClear(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryDelete(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryForEach(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGet(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryHas(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySet(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntrySizeGetter(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEntries(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryKeys(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryValues(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...);

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;

        static JavascriptMap* CreateForSnapshotRestore(ScriptContext* ctx);
#endif
    };

    template <> inline bool VarIsImpl<JavascriptMap>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_Map;
    }
}
