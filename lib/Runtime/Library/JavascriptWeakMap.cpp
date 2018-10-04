//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptWeakMap::JavascriptWeakMap(DynamicType* type)
        : DynamicObject(type),
        keySet(type->GetScriptContext()->GetRecycler())
    {
    }

    JavascriptWeakMap::WeakMapKeyMap* JavascriptWeakMap::GetWeakMapKeyMapFromKey(RecyclableObject* key) const
    {
        AssertOrFailFast(DynamicType::Is(key->GetTypeId()) || JavascriptOperators::GetTypeId(key) == TypeIds_HostDispatch);

        Var weakMapKeyData = nullptr;

        if (!key->GetInternalProperty(key, InternalPropertyIds::WeakMapKeyMap, &weakMapKeyData, nullptr, key->GetScriptContext()))
        {
            return nullptr;
        }

        if (key->GetLibrary()->GetUndefined() == weakMapKeyData)
        {
            return nullptr;
        }

        return static_cast<WeakMapKeyMap*>(weakMapKeyData);
    }

    JavascriptWeakMap::WeakMapKeyMap* JavascriptWeakMap::AddWeakMapKeyMapToKey(RecyclableObject* key)
    {
        AssertOrFailFast(DynamicType::Is(key->GetTypeId()) || JavascriptOperators::GetTypeId(key) == TypeIds_HostDispatch);

        // The internal property may exist on an object that has had DynamicObject::ResetObject called on itself.
        // In that case the value stored in the property slot should be null.
        DebugOnly(Var unused = nullptr);
        Assert(!key->GetInternalProperty(key, InternalPropertyIds::WeakMapKeyMap, &unused, nullptr, key->GetScriptContext()) || unused == nullptr);

        WeakMapKeyMap* weakMapKeyData = RecyclerNew(GetScriptContext()->GetRecycler(), WeakMapKeyMap, GetScriptContext()->GetRecycler());
        BOOL success = key->SetInternalProperty(InternalPropertyIds::WeakMapKeyMap, weakMapKeyData, PropertyOperation_Force, nullptr);
        Assert(success);

        return weakMapKeyData;
    }

    bool JavascriptWeakMap::KeyMapGet(WeakMapKeyMap* map, Var* value) const
    {
        if (map->ContainsKey(GetWeakMapId()))
        {
            *value = map->Item(GetWeakMapId());
            return true;
        }

        return false;
    }

    Var JavascriptWeakMap::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        Var newTarget = args.GetNewTarget();
        bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);
        CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, WeakMap, scriptContext);

        JavascriptWeakMap* weakMapObject = nullptr;

        if (callInfo.Flags & CallFlags_New)
        {
            weakMapObject = library->CreateWeakMap();
        }
        else
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakMap"), _u("WeakMap"));
        }
        Assert(weakMapObject != nullptr);

        Var iterable = (args.Info.Count > 1) ? args[1] : library->GetUndefined();

        RecyclableObject* iter = nullptr;
        RecyclableObject* adder = nullptr;

        if (JavascriptConversion::CheckObjectCoercible(iterable, scriptContext))
        {
            iter = JavascriptOperators::GetIterator(iterable, scriptContext);
            Var adderVar = JavascriptOperators::GetProperty(weakMapObject, PropertyIds::set, scriptContext);
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

                BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                {
                    CALL_FUNCTION(scriptContext->GetThreadContext(), adder, CallInfo(CallFlags_Value, 3), weakMapObject, key, value);
                }
                END_SAFE_REENTRANT_CALL
            });
        }

#if ENABLE_TTD
        //TODO: right now we always GC before snapshots (assuming we have a weak collection)
        //      may want to optimize this and use a notify here that we have a weak container -- also update post inflate and post snap
#endif

        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), weakMapObject, nullptr, scriptContext) :
            weakMapObject;
    }

    Var JavascriptWeakMap::EntryDelete(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!VarIs<JavascriptWeakMap>(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakMap.prototype.delete"), _u("WeakMap"));
        }

        JavascriptWeakMap* weakMap = VarTo<JavascriptWeakMap>(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        bool didDelete = false;

        if (JavascriptOperators::IsObject(key))
        {
            RecyclableObject* keyObj = VarTo<RecyclableObject>(key);

            didDelete = weakMap->Delete(keyObj);
        }

#if ENABLE_TTD
        if(scriptContext->IsTTDRecordOrReplayModeEnabled())
        {
            if(scriptContext->IsTTDRecordModeEnabled())
            {
                function->GetScriptContext()->GetThreadContext()->TTDLog->RecordWeakCollectionContainsEvent(didDelete);
            }
            else
            {
                didDelete = function->GetScriptContext()->GetThreadContext()->TTDLog->ReplayWeakCollectionContainsEvent();
            }
        }
#endif

        return scriptContext->GetLibrary()->CreateBoolean(didDelete);
    }

    Var JavascriptWeakMap::EntryGet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!VarIs<JavascriptWeakMap>(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakMap.prototype.get"), _u("WeakMap"));
        }

        JavascriptWeakMap* weakMap = VarTo<JavascriptWeakMap>(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

        bool loaded = false;
        Var value = nullptr;
        if (JavascriptOperators::IsObject(key))
        {
            RecyclableObject* keyObj = VarTo<RecyclableObject>(key);
            loaded = weakMap->Get(keyObj, &value);
        }

#if ENABLE_TTD
        if(scriptContext->IsTTDRecordOrReplayModeEnabled())
        {
            if(scriptContext->IsTTDRecordModeEnabled())
            {
                function->GetScriptContext()->GetThreadContext()->TTDLog->RecordWeakCollectionContainsEvent(loaded);
            }
            else
            {
                loaded = function->GetScriptContext()->GetThreadContext()->TTDLog->ReplayWeakCollectionContainsEvent();
            }
        }
#endif

        return loaded ? CrossSite::MarshalVar(scriptContext, value) : scriptContext->GetLibrary()->GetUndefined();
    }

    Var JavascriptWeakMap::EntryHas(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!VarIs<JavascriptWeakMap>(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakMap.prototype.has"), _u("WeakMap"));
        }

        JavascriptWeakMap* weakMap = VarTo<JavascriptWeakMap>(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        bool hasValue = false;

        if (JavascriptOperators::IsObject(key))
        {
            RecyclableObject* keyObj = VarTo<RecyclableObject>(key);

            hasValue = weakMap->Has(keyObj);
        }

#if ENABLE_TTD
        if(scriptContext->IsTTDRecordOrReplayModeEnabled())
        {
            if(scriptContext->IsTTDRecordModeEnabled())
            {
                function->GetScriptContext()->GetThreadContext()->TTDLog->RecordWeakCollectionContainsEvent(hasValue);
            }
            else
            {
                hasValue = function->GetScriptContext()->GetThreadContext()->TTDLog->ReplayWeakCollectionContainsEvent();
            }
        }
#endif

        return scriptContext->GetLibrary()->CreateBoolean(hasValue);
    }

    Var JavascriptWeakMap::EntrySet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!VarIs<JavascriptWeakMap>(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakMap.prototype.set"), _u("WeakMap"));
        }

        JavascriptWeakMap* weakMap = VarTo<JavascriptWeakMap>(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        Var value = (args.Info.Count > 2) ? args[2] : scriptContext->GetLibrary()->GetUndefined();

        if (!JavascriptOperators::IsObject(key))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_WeakMapSetKeyNotAnObject, _u("WeakMap.prototype.set"));
        }

        RecyclableObject* keyObj = VarTo<RecyclableObject>(key);

#if ENABLE_TTD
        //In replay we need to pin the object (and will release at snapshot points) -- in record we don't need to do anything
        if(scriptContext->IsTTDReplayModeEnabled())
        {
            scriptContext->TTDContextInfo->TTDWeakReferencePinSet->AddNew(keyObj);
        }
#endif

        weakMap->Set(keyObj, value);

        return weakMap;
    }

    void JavascriptWeakMap::Clear()
    {
        keySet.Map([&](RecyclableObject* key, bool value, WeakType weakRef) {
            WeakMapKeyMap* keyMap = GetWeakMapKeyMapFromKey(key);

            // It may be the case that a CEO has been reset and the keyMap is now null.
            // Just ignore it in this case, the keyMap has already been collected.
            if (keyMap != nullptr)
            {
                // It may also be the case that a CEO has been reset and then added to a separate WeakMap,
                // creating a new WeakMapKeyMap on the CEO.  In this case GetWeakMapId() may not be in the
                // keyMap, so don't assert successful removal here.
                keyMap->Remove(GetWeakMapId());
            }
        });
        keySet.Clear();
    }

    bool JavascriptWeakMap::Delete(RecyclableObject* key)
    {
        WeakMapKeyMap* keyMap = GetWeakMapKeyMapFromKey(key);

        if (keyMap != nullptr)
        {
            bool unused = false;
            bool inSet = keySet.TryGetValueAndRemove(key, &unused);
            bool inData = keyMap->Remove(GetWeakMapId());
            Assert(inSet == inData);

            return inData;
        }

        return false;
    }

    bool JavascriptWeakMap::Get(RecyclableObject* key, Var* value) const
    {
        WeakMapKeyMap* keyMap = GetWeakMapKeyMapFromKey(key);

        if (keyMap != nullptr)
        {
            return KeyMapGet(keyMap, value);
        }

        return false;
    }

    bool JavascriptWeakMap::Has(RecyclableObject* key) const
    {
        WeakMapKeyMap* keyMap = GetWeakMapKeyMapFromKey(key);

        if (keyMap != nullptr)
        {
            return keyMap->ContainsKey(GetWeakMapId());
        }

        return false;
    }

    void JavascriptWeakMap::Set(RecyclableObject* key, Var value)
    {
        WeakMapKeyMap* keyMap = GetWeakMapKeyMapFromKey(key);

        if (keyMap == nullptr)
        {
            keyMap = AddWeakMapKeyMapToKey(key);
        }

        keyMap->Item(GetWeakMapId(), value);
        keySet.Item(key, true);
    }

    BOOL JavascriptWeakMap::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("WeakMap"));
        return TRUE;
    }

#if ENABLE_TTD
    void JavascriptWeakMap::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        //All weak things should be reachable from another root so no need to mark but do need to repopulate the pin sets if in replay mode
        Js::ScriptContext* scriptContext = this->GetScriptContext();
        if(scriptContext->IsTTDReplayModeEnabled())
        {
            this->Map([&](RecyclableObject* key, Js::Var value)
            {
                scriptContext->TTDContextInfo->TTDWeakReferencePinSet->AddNew(key);
            });
        }

        //Keys are weak so are always reachable from somewhere else but values are not so we must walk them
        this->Map([&](RecyclableObject* key, Js::Var value)
        {
            extractor->MarkVisitVar(value);
        });
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptWeakMap::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapMapObject;
    }

    void JavascriptWeakMap::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapMapInfo* smi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapMapInfo>();
        uint32 mapCountEst = this->Size() * 2;

        smi->MapSize = 0;
        smi->MapKeyValueArray = alloc.SlabReserveArraySpace<TTD::TTDVar>(mapCountEst + 1); //always reserve at least 1 element

        this->Map([&](RecyclableObject* key, Js::Var value)
        {
            AssertMsg(smi->MapSize + 1 < mapCountEst, "We are writing junk");

            smi->MapKeyValueArray[smi->MapSize] = key;
            smi->MapKeyValueArray[smi->MapSize + 1] = value;
            smi->MapSize += 2;
        });

        if(smi->MapSize == 0)
        {
            smi->MapKeyValueArray = nullptr;
            alloc.SlabAbortArraySpace<TTD::TTDVar>(mapCountEst + 1);
        }
        else
        {
            alloc.SlabCommitArraySpace<TTD::TTDVar>(smi->MapSize, mapCountEst + 1);
        }

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapMapInfo*, TTD::NSSnapObjects::SnapObjectType::SnapMapObject>(objData, smi);
    }
#endif
}
