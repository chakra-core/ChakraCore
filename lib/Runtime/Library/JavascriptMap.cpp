//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
JavascriptMap::JavascriptMap(DynamicType* type)
    : DynamicObject(type)
{
}

JavascriptMap* JavascriptMap::New(ScriptContext* scriptContext)
{
    JavascriptMap* map = scriptContext->GetLibrary()->CreateMap();

    return map;
}

JavascriptMap::MapDataList::Iterator JavascriptMap::GetIterator()
{
    return list.GetIterator();
}

Var JavascriptMap::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();
    JavascriptLibrary* library = scriptContext->GetLibrary();
    AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Map"));

    Var newTarget = args.GetNewTarget();
    bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);
    CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Map, scriptContext);

    JavascriptMap* mapObject = nullptr;

    if (callInfo.Flags & CallFlags_New)
    {
        mapObject = library->CreateMap();
    }
    else
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map"), _u("Map"));
    }
    Assert(mapObject != nullptr);

    Var iterable = (args.Info.Count > 1) ? args[1] : library->GetUndefined();

    // REVIEW: This condition seems impossible?
    if (mapObject->kind != MapKind::EmptyMap)
    {
        Assert(UNREACHED);
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_ObjectIsAlreadyInitialized, _u("Map"), _u("Map"));
    }

    RecyclableObject* iter = nullptr;
    RecyclableObject* adder = nullptr;

    if (JavascriptConversion::CheckObjectCoercible(iterable, scriptContext))
    {
        iter = JavascriptOperators::GetIterator(iterable, scriptContext);
        Var adderVar = JavascriptOperators::GetPropertyNoCache(mapObject, PropertyIds::set, scriptContext);
        if (!JavascriptConversion::IsCallable(adderVar))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
        }
        adder = VarTo<RecyclableObject>(adderVar);
    }

    if (iter != nullptr)
    {
        Var undefined = library->GetUndefined();

        JavascriptOperators::DoIteratorStepAndValue(iter, scriptContext, [&](Var nextItem) {
            if (!JavascriptOperators::IsObject(nextItem))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
            }

            RecyclableObject* obj = VarTo<RecyclableObject>(nextItem);

            Var key = nullptr, value = nullptr;

            if (!JavascriptOperators::GetItem(obj, 0u, &key, scriptContext))
            {
                key = undefined;
            }

            if (!JavascriptOperators::GetItem(obj, 1u, &value, scriptContext))
            {
                value = undefined;
            }

            // CONSIDER: if adder is the default built-in, fast path it and skip the JS call?
            BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
            {
                CALL_FUNCTION(scriptContext->GetThreadContext(), adder, CallInfo(CallFlags_Value, 3), mapObject, key, value);
            }
            END_SAFE_REENTRANT_CALL
        });
    }

    return isCtorSuperCall ?
        JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), mapObject, nullptr, scriptContext) :
        mapObject;
}

Var JavascriptMap::EntryClear(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptMap* map = JavascriptOperators::TryFromVar<JavascriptMap>(args[0]);
    if (map == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.clear"), _u("Map"));
    }

    map->Clear();

    return scriptContext->GetLibrary()->GetUndefined();
}

Var JavascriptMap::EntryDelete(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptMap* map = JavascriptOperators::TryFromVar<JavascriptMap>(args[0]);
    if (map == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.delete"), _u("Map"));
    }

    Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

    bool didDelete = map->Delete(key);

    return scriptContext->GetLibrary()->CreateBoolean(didDelete);
}

Var JavascriptMap::EntryForEach(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();
    AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Map.prototype.forEach"));

    JavascriptMap* map = JavascriptOperators::TryFromVar<JavascriptMap>(args[0]);
    if (map == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.forEach"), _u("Map"));
    }

    if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Map.prototype.forEach"));
    }
    RecyclableObject* callBackFn = VarTo<RecyclableObject>(args[1]);

    Var thisArg = (args.Info.Count > 2) ? args[2] : scriptContext->GetLibrary()->GetUndefined();

    auto iterator = map->GetIterator();

    while (iterator.Next())
    {
        Var key = iterator.Current().Key();
        Var value = iterator.Current().Value();

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(CallFlags_Value, 4), thisArg, value, key, map);
        }
        END_SAFE_REENTRANT_CALL
    }

    return scriptContext->GetLibrary()->GetUndefined();
}

Var JavascriptMap::EntryGet(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptMap* map = JavascriptOperators::TryFromVar<JavascriptMap>(args[0]);
    if (map == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.get"), _u("Map"));
    }

    Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
    Var value = nullptr;

    if (map->Get(key, &value))
    {
        return CrossSite::MarshalVar(scriptContext, value);
    }

    return scriptContext->GetLibrary()->GetUndefined();
}

Var JavascriptMap::EntryHas(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptMap* map = JavascriptOperators::TryFromVar<JavascriptMap>(args[0]);
    if (map == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.has"), _u("Map"));
    }

    Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

    bool hasValue = map->Has(key);

    return scriptContext->GetLibrary()->CreateBoolean(hasValue);
}

Var JavascriptMap::EntrySet(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptMap* map = JavascriptOperators::TryFromVar<JavascriptMap>(args[0]);
    if (map == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.set"), _u("Map"));
    }

    Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
    Var value = (args.Info.Count > 2) ? args[2] : scriptContext->GetLibrary()->GetUndefined();

    if (JavascriptNumber::Is(key) && JavascriptNumber::IsNegZero(JavascriptNumber::GetValue(key)))
    {
        // Normalize -0 to +0
        key = JavascriptNumber::New(0.0, scriptContext);
    }

    map->Set(key, value);

    return map;
}

Var JavascriptMap::EntrySizeGetter(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptMap* map = JavascriptOperators::TryFromVar<JavascriptMap>(args[0]);
    if (map == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.size"), _u("Map"));
    }

    int size = map->Size();

    return JavascriptNumber::ToVar(size, scriptContext);
}

Var JavascriptMap::EntryEntries(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptMap* map = JavascriptOperators::TryFromVar<JavascriptMap>(args[0]);
    if (map == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.entries"), _u("Map"));
    }

    return scriptContext->GetLibrary()->CreateMapIterator(map, JavascriptMapIteratorKind::KeyAndValue);
}

Var JavascriptMap::EntryKeys(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptMap* map = JavascriptOperators::TryFromVar<JavascriptMap>(args[0]);
    if (map == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.keys"), _u("Map"));
    }

    return scriptContext->GetLibrary()->CreateMapIterator(map, JavascriptMapIteratorKind::Key);
}

Var JavascriptMap::EntryValues(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptMap* map = JavascriptOperators::TryFromVar<JavascriptMap>(args[0]);
    if (map == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.values"), _u("Map"));
    }
    return scriptContext->GetLibrary()->CreateMapIterator(map, JavascriptMapIteratorKind::Value);
}

void
JavascriptMap::Clear()
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());

    list.Clear();

    switch (this->kind)
    {
    case MapKind::EmptyMap:
        return;
    case MapKind::SimpleVarMap:
        this->u.simpleVarMap->Clear();
        return;
    case MapKind::ComplexVarMap:
        this->u.complexVarMap->Clear();
        return;
    default:
        Assume(UNREACHED);
    }
}

template <bool isComplex>
bool
JavascriptMap::DeleteFromVarMap(Var value)
{
    Assert(this->kind == (isComplex ? MapKind::ComplexVarMap : MapKind::SimpleVarMap));

    MapDataNode * node = nullptr;
    if (isComplex
        ? !this->u.complexVarMap->TryGetValueAndRemove(value, &node)
        : !this->u.simpleVarMap->TryGetValueAndRemove(value, &node))
    {
        return false;
    }

    this->list.Remove(node);
    return true;
}

bool
JavascriptMap::DeleteFromSimpleVarMap(Var value)
{
    Assert(this->kind == MapKind::SimpleVarMap);

    Var simpleVar = JavascriptConversion::TryCanonicalizeAsSimpleVar<true /* allowLossyConversion */>(value);
    if (!simpleVar)
    {
        return false;
    }

    return this->DeleteFromVarMap<false /* isComplex */>(simpleVar);
}

bool
JavascriptMap::Delete(Var key)
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());

    switch (this->kind)
    {
    case MapKind::EmptyMap:
        return false;

    case MapKind::SimpleVarMap:
        return this->DeleteFromSimpleVarMap(key);
    case MapKind::ComplexVarMap:
        return this->DeleteFromVarMap<true>(key);
    default:
        Assume(UNREACHED);
        return false;
    }
}

bool
JavascriptMap::Get(Var key, Var* value)
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());

    switch (this->kind)
    {
    case MapKind::EmptyMap:
        return false;

    case MapKind::SimpleVarMap:
    {
        // First check if the key is in the map
        MapDataNode* node = nullptr;
        if (this->u.simpleVarMap->TryGetValue(key, &node))
        {
            *value = node->data.Value();
            return true;
        }
        // If the key isn't in the map, check if the canonical value is
        Var simpleVar = JavascriptConversion::TryCanonicalizeAsSimpleVar<true /* allowLossyConversion */>(key);
        // If the simple var is the same as the original key, we know it isn't in the map
        if (!simpleVar || simpleVar == key)
        {
            return false;
        }

        if (!this->u.simpleVarMap->TryGetValue(simpleVar, &node))
        {
            return false;
        }
        *value = node->data.Value();
        return true;
    }
    case MapKind::ComplexVarMap:
    {
        MapDataNode * node = nullptr;
        if (!this->u.complexVarMap->TryGetValue(key, &node))
        {
            return false;
        }
        *value = node->data.Value();
        return true;
    }
    default:
        Assume(UNREACHED);
        return false;
    }
}


bool
JavascriptMap::Has(Var key)
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());

    switch (this->kind)
    {
    case MapKind::EmptyMap:
        return false;

    case MapKind::SimpleVarMap:
    {
        // First check if the key is in the map
        if (this->u.simpleVarMap->ContainsKey(key))
        {
            return true;
        }
        // If the key isn't in the map, check if the canonical value is
        Var simpleVar = JavascriptConversion::TryCanonicalizeAsSimpleVar<true /* allowLossyConversion */>(key);
        // If the simple var is the same as the original key, we know it isn't in the map
        if (!simpleVar || simpleVar == key)
        {
            return false;
        }

        return this->u.simpleVarMap->ContainsKey(simpleVar);
    }
    case MapKind::ComplexVarMap:
        return this->u.complexVarMap->ContainsKey(key);

    default:
        Assume(UNREACHED);
        return false;
    }
}

void
JavascriptMap::PromoteToComplexVarMap()
{
    AssertOrFailFast(this->kind == MapKind::SimpleVarMap);

    uint newMapSize = this->u.simpleVarMap->Count() + 1;
    ComplexVarDataMap* newMap = RecyclerNew(this->GetRecycler(), ComplexVarDataMap, this->GetRecycler(), newMapSize);

    JavascriptMap::MapDataList::Iterator iter = this->list.GetIterator();
    // TODO: we can use a more efficient Iterator, since we know there will be no side effects
    while (iter.Next())
    {
        newMap->Add(iter.Current().Key(), iter.CurrentNode());
    }

    this->kind = MapKind::ComplexVarMap;
    this->u.complexVarMap = newMap;
}

void
JavascriptMap::SetOnEmptyMap(Var key, Var value)
{
    Assert(this->kind == MapKind::EmptyMap);
    Var simpleVar = JavascriptConversion::TryCanonicalizeAsSimpleVar<false /* allowLossyConversion */>(key);
    if (simpleVar)
    {
        SimpleVarDataMap* newSimpleMap = RecyclerNew(this->GetRecycler(), SimpleVarDataMap, this->GetRecycler());
        MapDataKeyValuePair simplePair(simpleVar, value);

        MapDataNode* node = this->list.Append(simplePair, this->GetRecycler());

        newSimpleMap->Add(simpleVar, node);

        this->u.simpleVarMap = newSimpleMap;
        this->kind = MapKind::SimpleVarMap;
        return;
    }

    ComplexVarDataMap* newComplexSet = RecyclerNew(this->GetRecycler(), ComplexVarDataMap, this->GetRecycler());
    MapDataKeyValuePair complexPair(key, value);

    MapDataNode* node = this->list.Append(complexPair, this->GetRecycler());

    newComplexSet->Add(key, node);

    this->u.complexVarMap = newComplexSet;
    this->kind = MapKind::ComplexVarMap;
}

bool
JavascriptMap::TrySetOnSimpleVarMap(Var key, Var value)
{
    Assert(this->kind == MapKind::SimpleVarMap);

    Var simpleVar = JavascriptConversion::TryCanonicalizeAsSimpleVar<false /* allowLossyConversion */>(key);
    if (!simpleVar)
    {
        return false;
    }

    MapDataNode* node = nullptr;
    if (this->u.simpleVarMap->TryGetValue(simpleVar, &node))
    {
        node->data = MapDataKeyValuePair(simpleVar, value);
        return true;
    }

    MapDataKeyValuePair pair(simpleVar, value);
    MapDataNode* newNode = this->list.Append(pair, this->GetRecycler());
    this->u.simpleVarMap->Add(simpleVar, newNode);
    return true;
}

void
JavascriptMap::SetOnComplexVarMap(Var key, Var value)
{
    Assert(this->kind == MapKind::ComplexVarMap);

    MapDataNode* node = nullptr;
    if (this->u.complexVarMap->TryGetValue(key, &node))
    {
        node->data = MapDataKeyValuePair(key, value);
        return;
    }

    MapDataKeyValuePair pair(key, value);
    MapDataNode* newNode = this->list.Append(pair, this->GetRecycler());
    this->u.complexVarMap->Add(key, newNode);
}

void
JavascriptMap::Set(Var key, Var value)
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());

    switch (this->kind)
    {
    case MapKind::EmptyMap:
        this->SetOnEmptyMap(key, value);
        return;

    case MapKind::SimpleVarMap:
        if (this->TrySetOnSimpleVarMap(key, value))
        {
            return;
        }
        this->PromoteToComplexVarMap();
        return this->Set(key, value);

    case MapKind::ComplexVarMap:
        this->SetOnComplexVarMap(key, value);
        return;

    default:
        Assume(UNREACHED);
    }
}

int JavascriptMap::Size()
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());

    switch (this->kind)
    {
    case MapKind::EmptyMap:
        return 0;

    case MapKind::SimpleVarMap:
        return this->u.simpleVarMap->Count();

    case MapKind::ComplexVarMap:
        return this->u.complexVarMap->Count();

    default:
        Assume(UNREACHED);
        return 0;
    }
}

BOOL JavascriptMap::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
{
    stringBuilder->AppendCppLiteral(_u("Map"));
    return TRUE;
}

Var JavascriptMap::EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...)
{
    ARGUMENTS(args, callInfo);

    Assert(args.Info.Count > 0);

    return args[0];
}

#if ENABLE_TTD
void JavascriptMap::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
{
    auto iterator = GetIterator();
    while(iterator.Next())
    {
        extractor->MarkVisitVar(iterator.Current().Key());
        extractor->MarkVisitVar(iterator.Current().Value());
    }
}

TTD::NSSnapObjects::SnapObjectType JavascriptMap::GetSnapTag_TTD() const
{
    return TTD::NSSnapObjects::SnapObjectType::SnapMapObject;
}

void JavascriptMap::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
{
    TTD::NSSnapObjects::SnapMapInfo* smi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapMapInfo>();
    smi->MapSize = 0;

    if(this->Size() == 0)
    {
        smi->MapKeyValueArray = nullptr;
    }
    else
    {
        smi->MapKeyValueArray = alloc.SlabAllocateArray<TTD::TTDVar>(this->Size() * 2);

        auto iter = this->GetIterator();
        while(iter.Next())
        {
            smi->MapKeyValueArray[smi->MapSize] = iter.Current().Key();
            smi->MapKeyValueArray[smi->MapSize + 1] = iter.Current().Value();
            smi->MapSize += 2;
        }
    }

    TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapMapInfo*, TTD::NSSnapObjects::SnapObjectType::SnapMapObject>(objData, smi);
}

JavascriptMap* JavascriptMap::CreateForSnapshotRestore(ScriptContext* ctx)
{
    JavascriptMap* res = ctx->GetLibrary()->CreateMap();

    return res;
}
#endif
}
