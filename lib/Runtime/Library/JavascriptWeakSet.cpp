//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptWeakSet::JavascriptWeakSet(DynamicType* type)
        : DynamicObject(type),
        keySet(type->GetScriptContext()->GetRecycler())
    {
    }

    bool JavascriptWeakSet::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_WeakSet;
    }

    JavascriptWeakSet* JavascriptWeakSet::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptWeakSet'");

        return static_cast<JavascriptWeakSet *>(RecyclableObject::FromVar(aValue));
    }

    Var JavascriptWeakSet::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
        Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr);
        CHAKRATEL_LANGSTATS_INC_DATACOUNT(ES6_WeakSet);

        JavascriptWeakSet* weakSetObject = nullptr;

        if (callInfo.Flags & CallFlags_New)
        {
             weakSetObject = library->CreateWeakSet();
        }
        else
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakSet"), _u("WeakSet"));
        }
        Assert(weakSetObject != nullptr);

        Var iterable = (args.Info.Count > 1) ? args[1] : library->GetUndefined();

        RecyclableObject* iter = nullptr;
        RecyclableObject* adder = nullptr;

        if (JavascriptConversion::CheckObjectCoercible(iterable, scriptContext))
        {
            iter = JavascriptOperators::GetIterator(iterable, scriptContext);
            Var adderVar = JavascriptOperators::GetProperty(weakSetObject, PropertyIds::add, scriptContext);
            if (!JavascriptConversion::IsCallable(adderVar))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
            }
            adder = RecyclableObject::FromVar(adderVar);
        }

        if (iter != nullptr)
        {
            JavascriptOperators::DoIteratorStepAndValue(iter, scriptContext, [&](Var nextItem) {
                CALL_FUNCTION(scriptContext->GetThreadContext(), adder, CallInfo(CallFlags_Value, 2), weakSetObject, nextItem);
            });
        }

#if ENABLE_TTD
        //TODO: right now we always GC before snapshots (assuming we have a weak collection)
        //      may want to optimize this and use a notify here that we have a weak container -- also update post inflate and post snap
#endif

        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), weakSetObject, nullptr, scriptContext) :
            weakSetObject;
    }

    Var JavascriptWeakSet::EntryAdd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptWeakSet::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakSet.prototype.add"), _u("WeakSet"));
        }

        JavascriptWeakSet* weakSet = JavascriptWeakSet::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

        if (!JavascriptOperators::IsObject(key) || JavascriptOperators::GetTypeId(key) == TypeIds_HostDispatch)
        {
            // HostDispatch is not expanded so can't have internal property added to it.
            // TODO: Support HostDispatch as WeakSet key
            JavascriptError::ThrowTypeError(scriptContext, JSERR_WeakMapSetKeyNotAnObject, _u("WeakSet.prototype.add"));
        }

        RecyclableObject* keyObj = RecyclableObject::FromVar(key);

#if ENABLE_TTD
        //In replay we need to pin the object (and will release at snapshot points) -- in record we don't need to do anything
        if(scriptContext->IsTTDReplayModeEnabled())
        {
            scriptContext->TTDContextInfo->TTDWeakReferencePinSet->AddNew(keyObj);
        }
#endif

        weakSet->Add(keyObj);

        return weakSet;
    }

    Var JavascriptWeakSet::EntryDelete(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptWeakSet::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakSet.prototype.delete"), _u("WeakSet"));
        }

        JavascriptWeakSet* weakSet = JavascriptWeakSet::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        bool didDelete = false;

        if (JavascriptOperators::IsObject(key) && JavascriptOperators::GetTypeId(key) != TypeIds_HostDispatch)
        {
            RecyclableObject* keyObj = RecyclableObject::FromVar(key);

            didDelete = weakSet->Delete(keyObj);
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

    Var JavascriptWeakSet::EntryHas(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptWeakSet::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("WeakSet.prototype.has"), _u("WeakSet"));
        }

        JavascriptWeakSet* weakSet = JavascriptWeakSet::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        bool hasValue = false;

        if (JavascriptOperators::IsObject(key) && JavascriptOperators::GetTypeId(key) != TypeIds_HostDispatch)
        {
            RecyclableObject* keyObj = RecyclableObject::FromVar(key);

            hasValue = weakSet->Has(keyObj);
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

    void JavascriptWeakSet::Add(RecyclableObject* key)
    {
        keySet.Item(key, true);
    }

    bool JavascriptWeakSet::Delete(RecyclableObject* key)
    {
        bool unused = false;
        return keySet.TryGetValueAndRemove(key, &unused);
    }

    bool JavascriptWeakSet::Has(RecyclableObject* key)
    {
        bool unused = false;
        return keySet.TryGetValue(key, &unused);
    }

    BOOL JavascriptWeakSet::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("WeakSet"));
        return TRUE;
    }

#if ENABLE_TTD
    void JavascriptWeakSet::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        //All weak things should be reachable from another root so no need to mark but do need to repopulate the pin sets if in replay mode
        Js::ScriptContext* scriptContext = this->GetScriptContext();
        if(scriptContext->IsTTDReplayModeEnabled())
        {
            this->Map([&](RecyclableObject* key)
            {
                scriptContext->TTDContextInfo->TTDWeakReferencePinSet->AddNew(key);
            });
        }
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptWeakSet::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapSetObject;
    }

    void JavascriptWeakSet::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapSetInfo* ssi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapSetInfo>();
        uint32 setCountEst = this->Size();

        ssi->SetSize = 0;
        ssi->SetValueArray = alloc.SlabReserveArraySpace<TTD::TTDVar>(setCountEst + 1); //always reserve at least 1 element

        this->Map([&](RecyclableObject* key)
        {
            AssertMsg(ssi->SetSize < setCountEst, "We are writting junk");

            ssi->SetValueArray[ssi->SetSize] = key;
            ssi->SetSize++;
        });

        if(ssi->SetSize == 0)
        {
            ssi->SetValueArray = nullptr;
            alloc.SlabAbortArraySpace<TTD::TTDVar>(setCountEst + 1);
        }
        else
        {
            alloc.SlabCommitArraySpace<TTD::TTDVar>(ssi->SetSize, setCountEst + 1);
        }

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapSetInfo*, TTD::NSSnapObjects::SnapObjectType::SnapSetObject>(objData, ssi);
    }
#endif
}
