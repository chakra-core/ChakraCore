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
        map->map = RecyclerNew(scriptContext->GetRecycler(), MapDataMap, scriptContext->GetRecycler());

        return map;
    }

    bool JavascriptMap::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Map;
    }

    JavascriptMap* JavascriptMap::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptMap'");

        return static_cast<JavascriptMap *>(RecyclableObject::FromVar(aValue));
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

        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
        Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr);
        CHAKRATEL_LANGSTATS_INC_DATACOUNT(ES6_Map);

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

        if (mapObject->map != nullptr)
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_ObjectIsAlreadyInitialized, _u("Map"), _u("Map"));
        }

        /* Ensure mapObject->map is created before trying to fetch the adder function. If Map.prototype.set has
           its getter set to another Map method (such as Map.prototype.get) and we try to get the function before
           the map is initialized, it will cause a null dereference. See github#2747 */
        mapObject->map = RecyclerNew(scriptContext->GetRecycler(), MapDataMap, scriptContext->GetRecycler());

        RecyclableObject* iter = nullptr;
        RecyclableObject* adder = nullptr;

        if (JavascriptConversion::CheckObjectCoercible(iterable, scriptContext))
        {
            iter = JavascriptOperators::GetIterator(iterable, scriptContext);
            Var adderVar = JavascriptOperators::GetProperty(mapObject, PropertyIds::set, scriptContext);
            if (!JavascriptConversion::IsCallable(adderVar))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
            }
            adder = RecyclableObject::FromVar(adderVar);
        }

        if (iter != nullptr)
        {
            Var undefined = library->GetUndefined();

            JavascriptOperators::DoIteratorStepAndValue(iter, scriptContext, [&](Var nextItem) {
                if (!JavascriptOperators::IsObject(nextItem))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
                }

                RecyclableObject* obj = RecyclableObject::FromVar(nextItem);

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
                CALL_FUNCTION(scriptContext->GetThreadContext(), adder, CallInfo(CallFlags_Value, 3), mapObject, key, value);
            });
        }

        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), mapObject, nullptr, scriptContext) :
            mapObject;
    }

    Var JavascriptMap::EntryClear(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.clear"), _u("Map"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        map->Clear();

        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var JavascriptMap::EntryDelete(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.delete"), _u("Map"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

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

        if (!JavascriptMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.forEach"), _u("Map"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Map.prototype.forEach"));
        }
        RecyclableObject* callBackFn = RecyclableObject::FromVar(args[1]);

        Var thisArg = (args.Info.Count > 2) ? args[2] : scriptContext->GetLibrary()->GetUndefined();

        auto iterator = map->GetIterator();

        while (iterator.Next())
        {
            Var key = iterator.Current().Key();
            Var value = iterator.Current().Value();

            CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(CallFlags_Value, 4), thisArg, value, key, map);
        }

        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var JavascriptMap::EntryGet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.get"), _u("Map"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        Var value = nullptr;

        if (map->Get(key, &value))
        {
            return value;
        }

        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var JavascriptMap::EntryHas(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.has"), _u("Map"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

        bool hasValue = map->Has(key);

        return scriptContext->GetLibrary()->CreateBoolean(hasValue);
    }

    Var JavascriptMap::EntrySet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.set"), _u("Map"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

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

        if (!JavascriptMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.size"), _u("Map"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        int size = map->Size();

        return JavascriptNumber::ToVar(size, scriptContext);
    }

    Var JavascriptMap::EntryEntries(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.entries"), _u("Map"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        return scriptContext->GetLibrary()->CreateMapIterator(map, JavascriptMapIteratorKind::KeyAndValue);
    }

    Var JavascriptMap::EntryKeys(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.keys"), _u("Map"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        return scriptContext->GetLibrary()->CreateMapIterator(map, JavascriptMapIteratorKind::Key);
    }

    Var JavascriptMap::EntryValues(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Map.prototype.values"), _u("Map"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);
        return scriptContext->GetLibrary()->CreateMapIterator(map, JavascriptMapIteratorKind::Value);
    }

    void JavascriptMap::Clear()
    {
        list.Clear();
        map->Clear();
    }

    bool JavascriptMap::Delete(Var key)
    {
        if (map->ContainsKey(key))
        {
            MapDataNode* node = map->Item(key);
            list.Remove(node);
            return map->Remove(key);
        }
        return false;
    }

    bool JavascriptMap::Get(Var key, Var* value)
    {
        if (map->ContainsKey(key))
        {
            MapDataNode* node = map->Item(key);
            *value = node->data.Value();
            return true;
        }
        return false;
    }

    bool JavascriptMap::Has(Var key)
    {
        return map->ContainsKey(key);
    }

    void JavascriptMap::Set(Var key, Var value)
    {
        if (map->ContainsKey(key))
        {
            MapDataNode* node = map->Item(key);
            node->data = MapDataKeyValuePair(key, value);
        }
        else
        {
            MapDataKeyValuePair pair(key, value);
            MapDataNode* node = list.Append(pair, GetScriptContext()->GetRecycler());
            map->Add(key, node);
        }
    }

    int JavascriptMap::Size()
    {
        return map->Count();
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
        res->map = RecyclerNew(ctx->GetRecycler(), MapDataMap, ctx->GetRecycler());

        return res;
    }
#endif
}
