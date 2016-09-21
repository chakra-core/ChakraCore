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

    bool JavascriptWeakMap::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_WeakMap;
    }

    JavascriptWeakMap* JavascriptWeakMap::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptWeakMap'");

        return static_cast<JavascriptWeakMap *>(RecyclableObject::FromVar(aValue));
    }

    JavascriptWeakMap::WeakMapKeyMap* JavascriptWeakMap::GetWeakMapKeyMapFromKey(DynamicObject* key) const
    {
        Var weakMapKeyData = nullptr;
        if (!key->GetInternalProperty(key, InternalPropertyIds::WeakMapKeyMap, &weakMapKeyData, nullptr, key->GetScriptContext()))
        {
            return nullptr;
        }

        return static_cast<WeakMapKeyMap*>(weakMapKeyData);
    }

    JavascriptWeakMap::WeakMapKeyMap* JavascriptWeakMap::AddWeakMapKeyMapToKey(DynamicObject* key)
    {
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

        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
        Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr);
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(WeakMapCount);

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

                Var key, value;

                if (!JavascriptOperators::GetItem(obj, 0u, &key, scriptContext))
                {
                    key = undefined;
                }

                if (!JavascriptOperators::GetItem(obj, 1u, &value, scriptContext))
                {
                    value = undefined;
                }

                CALL_FUNCTION(adder, CallInfo(CallFlags_Value, 3), weakMapObject, key, value);
            });
        }

        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), weakMapObject, nullptr, scriptContext) :
            weakMapObject;
    }

    Var JavascriptWeakMap::EntryDelete(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptWeakMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakMap.prototype.delete"), _u("WeakMap"));
        }

        JavascriptWeakMap* weakMap = JavascriptWeakMap::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        bool didDelete = false;

        if (JavascriptOperators::IsObject(key) && JavascriptOperators::GetTypeId(key) != TypeIds_HostDispatch)
        {
            DynamicObject* keyObj = DynamicObject::FromVar(key);

            didDelete = weakMap->Delete(keyObj);
        }

        return scriptContext->GetLibrary()->CreateBoolean(didDelete);
    }

    Var JavascriptWeakMap::EntryGet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptWeakMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakMap.prototype.get"), _u("WeakMap"));
        }

        JavascriptWeakMap* weakMap = JavascriptWeakMap::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

        if (JavascriptOperators::IsObject(key) && JavascriptOperators::GetTypeId(key) != TypeIds_HostDispatch)
        {
            DynamicObject* keyObj = DynamicObject::FromVar(key);
            Var value = nullptr;

            if (weakMap->Get(keyObj, &value))
            {
                return value;
            }
        }

        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var JavascriptWeakMap::EntryHas(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptWeakMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakMap.prototype.has"), _u("WeakMap"));
        }

        JavascriptWeakMap* weakMap = JavascriptWeakMap::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        bool hasValue = false;

        if (JavascriptOperators::IsObject(key) && JavascriptOperators::GetTypeId(key) != TypeIds_HostDispatch)
        {
            DynamicObject* keyObj = DynamicObject::FromVar(key);

            hasValue = weakMap->Has(keyObj);
        }

        return scriptContext->GetLibrary()->CreateBoolean(hasValue);
    }

    Var JavascriptWeakMap::EntrySet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptWeakMap::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakMap.prototype.set"), _u("WeakMap"));
        }

        JavascriptWeakMap* weakMap = JavascriptWeakMap::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        Var value = (args.Info.Count > 2) ? args[2] : scriptContext->GetLibrary()->GetUndefined();

        if (!JavascriptOperators::IsObject(key) || JavascriptOperators::GetTypeId(key) == TypeIds_HostDispatch)
        {
            // HostDispatch can not expand so can't have internal property added to it.
            // TODO: Support HostDispatch as WeakMap key
            JavascriptError::ThrowTypeError(scriptContext, JSERR_WeakMapSetKeyNotAnObject, _u("WeakMap.prototype.set"));
        }

        DynamicObject* keyObj = DynamicObject::FromVar(key);

#if ENABLE_TTD
        //
        //TODO: This makes the map decidedly less weak -- forces it to only release when we clean the tracking set but determinizes the behavior nicely
        //      We want to improve this.
        //
        if(scriptContext->ShouldPerformWeakRefPinAction())
        {
            scriptContext->TTDContextInfo->TTDWeakReferencePinSet->Add(keyObj);
        }
#endif

        weakMap->Set(keyObj, value);

        return weakMap;
    }

    void JavascriptWeakMap::Clear()
    {
        keySet.Map([&](DynamicObject* key, bool value, const RecyclerWeakReference<DynamicObject>* weakRef) {
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

    bool JavascriptWeakMap::Delete(DynamicObject* key)
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

    bool JavascriptWeakMap::Get(DynamicObject* key, Var* value) const
    {
        WeakMapKeyMap* keyMap = GetWeakMapKeyMapFromKey(key);

        if (keyMap != nullptr)
        {
            return KeyMapGet(keyMap, value);
        }

        return false;
    }

    bool JavascriptWeakMap::Has(DynamicObject* key) const
    {
        WeakMapKeyMap* keyMap = GetWeakMapKeyMapFromKey(key);

        if (keyMap != nullptr)
        {
            return keyMap->ContainsKey(GetWeakMapId());
        }

        return false;
    }

    void JavascriptWeakMap::Set(DynamicObject* key, Var value)
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
        this->Map([&](DynamicObject* key, Js::Var value)
        {
            extractor->MarkVisitVar(key);
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

        this->Map([&](DynamicObject* key, Js::Var value)
        {
            AssertMsg(smi->MapSize + 1 < mapCountEst, "We are writting junk");

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
