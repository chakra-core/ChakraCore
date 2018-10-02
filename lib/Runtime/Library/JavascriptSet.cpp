//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
JavascriptSet::JavascriptSet(DynamicType* type)
    : DynamicObject(type)
{
}

JavascriptSet* JavascriptSet::New(ScriptContext* scriptContext)
{
    JavascriptSet* set = scriptContext->GetLibrary()->CreateSet();

    return set;
}

JavascriptSet::SetDataList::Iterator JavascriptSet::GetIterator()
{
    return this->list.GetIterator();
}

Var JavascriptSet::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();
    JavascriptLibrary* library = scriptContext->GetLibrary();
    AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Set"));

    Var newTarget = args.GetNewTarget();
    bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);
    CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Set, scriptContext);

    JavascriptSet* setObject = nullptr;

    if (callInfo.Flags & CallFlags_New)
    {
        setObject = library->CreateSet();
    }
    else
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Set"), _u("Set"));
    }
    Assert(setObject != nullptr);

    Var iterable = (args.Info.Count > 1) ? args[1] : library->GetUndefined();

    // REVIEW: This condition seems impossible?
    if (setObject->kind != SetKind::EmptySet)
    {
        Assert(UNREACHED);
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_ObjectIsAlreadyInitialized, _u("Set"), _u("Set"));
    }

    RecyclableObject* iter = nullptr;
    RecyclableObject* adder = nullptr;

    if (JavascriptConversion::CheckObjectCoercible(iterable, scriptContext))
    {
        iter = JavascriptOperators::GetIterator(iterable, scriptContext);
        Var adderVar = JavascriptOperators::GetPropertyNoCache(setObject, PropertyIds::add, scriptContext);
        if (!JavascriptConversion::IsCallable(adderVar))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
        }
        adder = VarTo<RecyclableObject>(adderVar);
    }

    if (iter != nullptr)
    {
        JavascriptOperators::DoIteratorStepAndValue(iter, scriptContext, [&](Var nextItem) {
            BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
            {
                CALL_FUNCTION(scriptContext->GetThreadContext(), adder, CallInfo(CallFlags_Value, 2), setObject, nextItem);
            }
            END_SAFE_REENTRANT_CALL
        });
    }

    return isCtorSuperCall ?
        JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), setObject, nullptr, scriptContext) :
        setObject;
}

Var JavascriptSet::EntryAdd(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptSet* set = JavascriptOperators::TryFromVar<JavascriptSet>(args[0]);
    if (set == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Set.prototype.add"), _u("Set"));
    }

    Var value = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

    if (JavascriptNumber::Is(value) && JavascriptNumber::IsNegZero(JavascriptNumber::GetValue(value)))
    {
        // Normalize -0 to +0
        value = JavascriptNumber::New(0.0, scriptContext);
    }

    set->Add(value);

    return set;
}

Var JavascriptSet::EntryClear(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptSet* set = JavascriptOperators::TryFromVar<JavascriptSet>(args[0]);
    if (set == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Set.prototype.clear"), _u("Set"));
    }

    set->Clear();

    return scriptContext->GetLibrary()->GetUndefined();
}

Var JavascriptSet::EntryDelete(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptSet* set = JavascriptOperators::TryFromVar<JavascriptSet>(args[0]);
    if (set == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Set.prototype.delete"), _u("Set"));
    }

    Var value = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

    bool didDelete = set->Delete(value);

    return scriptContext->GetLibrary()->CreateBoolean(didDelete);
}

Var JavascriptSet::EntryForEach(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();
    AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Set.prototype.forEach"));

    JavascriptSet* set = JavascriptOperators::TryFromVar<JavascriptSet>(args[0]);
    if (set == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Set.prototype.forEach"), _u("Set"));
    }

    if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Set.prototype.forEach"));
    }
    RecyclableObject* callBackFn = VarTo<RecyclableObject>(args[1]);

    Var thisArg = (args.Info.Count > 2) ? args[2] : scriptContext->GetLibrary()->GetUndefined();

    auto iterator = set->GetIterator();

    while (iterator.Next())
    {
        Var value = iterator.Current();

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(CallFlags_Value, 4), thisArg, value, value, args[0]);
        }
        END_SAFE_REENTRANT_CALL
    }

    return scriptContext->GetLibrary()->GetUndefined();
}

Var JavascriptSet::EntryHas(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptSet* set = JavascriptOperators::TryFromVar<JavascriptSet>(args[0]);
    if (set == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Set.prototype.has"), _u("Set"));
    }

    Var value = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

    bool hasValue = set->Has(value);

    return scriptContext->GetLibrary()->CreateBoolean(hasValue);
}

Var JavascriptSet::EntrySizeGetter(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptSet* set = JavascriptOperators::TryFromVar<JavascriptSet>(args[0]);
    if (set == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Set.prototype.size"), _u("Set"));
    }

    int size = set->Size();

    return JavascriptNumber::ToVar(size, scriptContext);
}

Var JavascriptSet::EntryEntries(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptSet* set = JavascriptOperators::TryFromVar<JavascriptSet>(args[0]);
    if (set == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Set.prototype.entries"), _u("Set"));
    }

    return scriptContext->GetLibrary()->CreateSetIterator(set, JavascriptSetIteratorKind::KeyAndValue);
}

Var JavascriptSet::EntryValues(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    JavascriptSet* set = JavascriptOperators::TryFromVar<JavascriptSet>(args[0]);
    if (set == nullptr)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Set.prototype.values"), _u("Set"));
    }

    return scriptContext->GetLibrary()->CreateSetIterator(set, JavascriptSetIteratorKind::Value);
}

Var JavascriptSet::EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...)
{
    ARGUMENTS(args, callInfo);

    Assert(args.Info.Count > 0);

    return args[0];
}

template <typename T>
T*
JavascriptSet::CreateVarSetFromList(uint initialCapacity)
{
    T* varSet = RecyclerNew(this->GetRecycler(), T, this->GetRecycler(), initialCapacity);

    JavascriptSet::SetDataList::Iterator iter = this->list.GetIterator();
    // TODO: we can use a more efficient Iterator, since we know there will be no side effects
    while (iter.Next())
    {
        varSet->Add(iter.Current(), iter.CurrentNode());
    }
    return varSet;
}

void
JavascriptSet::PromoteToSimpleVarSet()
{
    AssertOrFailFast(this->kind == SetKind::IntSet);
    SimpleVarDataSet* newSet = this->CreateVarSetFromList<SimpleVarDataSet>(this->u.intSet->Count() + 1);

    this->kind = SetKind::SimpleVarSet;
    this->u.simpleVarSet = newSet;
}

void
JavascriptSet::PromoteToComplexVarSet()
{
    uint setSize = 0;
    if (this->kind == SetKind::IntSet)
    {
        setSize = this->u.intSet->Count();
    }
    else
    {
        AssertOrFailFast(this->kind == SetKind::SimpleVarSet);
        setSize = this->u.simpleVarSet->Count();
    }
    ComplexVarDataSet* newSet = this->CreateVarSetFromList<ComplexVarDataSet>(setSize + 1);

    this->kind = SetKind::ComplexVarSet;
    this->u.complexVarSet = newSet;
}

void
JavascriptSet::AddToEmptySet(Var value)
{
    Assert(this->kind == SetKind::EmptySet);
    // We cannot store an int with value -1 inside of a bit vector
    Var taggedInt = JavascriptConversion::TryCanonicalizeAsTaggedInt<false /* allowNegOne */, false /* allowLossyConversion */>(value);
    if (taggedInt)
    {
        int32 intVal = TaggedInt::ToInt32(taggedInt);

        BVSparse<Recycler>* newIntSet = RecyclerNew(this->GetRecycler(), BVSparse<Recycler>, this->GetRecycler());
        newIntSet->Set(intVal);

        this->list.Append(taggedInt, this->GetRecycler());

        this->u.intSet = newIntSet;
        this->kind = SetKind::IntSet;
        return;
    }

    Var simpleVar = JavascriptConversion::TryCanonicalizeAsSimpleVar<false /* allowLossyConversion */>(value);
    if (simpleVar)
    {
        SimpleVarDataSet* newSimpleSet = RecyclerNew(this->GetRecycler(), SimpleVarDataSet, this->GetRecycler());
        SetDataNode* node = this->list.Append(simpleVar, this->GetRecycler());

        newSimpleSet->Add(simpleVar, node);

        this->u.simpleVarSet = newSimpleSet;
        this->kind = SetKind::SimpleVarSet;
        return;
    }

    ComplexVarDataSet* newComplexSet = RecyclerNew(this->GetRecycler(), ComplexVarDataSet, this->GetRecycler());
    SetDataNode* node = this->list.Append(value, this->GetRecycler());

    newComplexSet->Add(value, node);

    this->u.complexVarSet = newComplexSet;
    this->kind = SetKind::ComplexVarSet;
}

bool
JavascriptSet::TryAddToIntSet(Var value)
{
    Assert(this->kind == SetKind::IntSet);
    Var taggedInt = JavascriptConversion::TryCanonicalizeAsTaggedInt<false /* allowNegOne */, false /* allowLossyConversion */>(value);
    if (!taggedInt)
    {
        return false;
    }

    int32 intVal = TaggedInt::ToInt32(taggedInt);
    if (!this->u.intSet->TestAndSet(intVal))
    {
        this->list.Append(taggedInt, this->GetRecycler());
    }
    return true;
}

bool
JavascriptSet::TryAddToSimpleVarSet(Var value)
{
    Assert(this->kind == SetKind::SimpleVarSet);
    Var simpleVar = JavascriptConversion::TryCanonicalizeAsSimpleVar<false /* allowLossyConversion */>(value);
    if (!simpleVar)
    {
        return false;
    }

    if (!this->u.simpleVarSet->ContainsKey(simpleVar))
    {
        SetDataNode* node = this->list.Append(simpleVar, this->GetRecycler());
        this->u.simpleVarSet->Add(simpleVar, node);
    }

    return true;
}

void
JavascriptSet::AddToComplexVarSet(Var value)
{
    Assert(this->kind == SetKind::ComplexVarSet);
    if (!this->u.complexVarSet->ContainsKey(value))
    {
        SetDataNode* node = this->list.Append(value, this->GetRecycler());
        this->u.complexVarSet->Add(value, node);
    }
}

void
JavascriptSet::Add(Var value)
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());
    switch (this->kind)
    {
    case SetKind::EmptySet:
    {
        this->AddToEmptySet(value);
        return;
    }

    case SetKind::IntSet:
    {
        if (this->TryAddToIntSet(value))
        {
            return;
        }

        Var simpleVar = JavascriptConversion::TryCanonicalizeAsSimpleVar<false /* allowLossyConversion */>(value);
        if (simpleVar)
        {
            this->PromoteToSimpleVarSet();
            this->Add(simpleVar);
            return;
        }

        this->PromoteToComplexVarSet();
        this->Add(value);
        return;
    }

    case SetKind::SimpleVarSet:
        if (this->TryAddToSimpleVarSet(value))
        {
            return;
        }

        this->PromoteToComplexVarSet();
        this->Add(value);
        return;

    case SetKind::ComplexVarSet:
        this->AddToComplexVarSet(value);
        return;

    default:
        Assume(UNREACHED);
    }
}

void JavascriptSet::Clear()
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());
    this->list.Clear();
    switch (this->kind)
    {
    case SetKind::EmptySet:
        return;
    case SetKind::IntSet:
        this->u.intSet->ClearAll();
        return;
    case SetKind::SimpleVarSet:
        this->u.simpleVarSet->Clear();
        return;
    case SetKind::ComplexVarSet:
        this->u.complexVarSet->Clear();
        return;
    default:
        Assume(UNREACHED);
    }
}

bool
JavascriptSet::IsInIntSet(Var value)
{
    Assert(this->kind == SetKind::IntSet);
    Var taggedInt = JavascriptConversion::TryCanonicalizeAsTaggedInt<false /* allowNegOne */, true /* allowLossyConversion */>(value);
    if (!taggedInt)
    {
        return false;
    }
    int32 intVal = TaggedInt::ToInt32(taggedInt);
    return this->u.intSet->Test(intVal);
}

template <bool isComplex>
bool
JavascriptSet::DeleteFromVarSet(Var value)
{
    Assert(this->kind == (isComplex ? SetKind::ComplexVarSet : SetKind::SimpleVarSet));
    SetDataNode * node = nullptr;
    if (isComplex
        ? !this->u.complexVarSet->TryGetValueAndRemove(value, &node)
        : !this->u.simpleVarSet->TryGetValueAndRemove(value, &node))
    {
        return false;
    }

    this->list.Remove(node);
    return true;
}

bool
JavascriptSet::DeleteFromSimpleVarSet(Var value)
{
    Assert(this->kind == SetKind::SimpleVarSet);
    Var simpleVar = JavascriptConversion::TryCanonicalizeAsSimpleVar<true /* allowLossyConversion */>(value);
    if (!simpleVar)
    {
        return false;
    }

    return this->DeleteFromVarSet<false>(simpleVar);
}

bool JavascriptSet::Delete(Var value)
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());
    switch (this->kind)
    {
    case SetKind::EmptySet:
        return false;

    case SetKind::IntSet:
        if (!IsInIntSet(value))
        {
            return false;
        }
        // We don't have the list node pointer readily available, so deletion from int sets would require walking the list
        // Because of this, let's just promote to a var set
        //
        // If this promotion becomes an issue, we can consider options to improve this, e.g. deferring until an iterator is requested
        this->PromoteToSimpleVarSet();
        return this->Delete(value);

    case SetKind::SimpleVarSet:
        return this->DeleteFromSimpleVarSet(value);

    case SetKind::ComplexVarSet:
        return this->DeleteFromVarSet<true /* isComplex */>(value);

    default:
        Assume(UNREACHED);
        return false;
    }
}

bool JavascriptSet::Has(Var value)
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());
    switch (this->kind)
    {
    case SetKind::EmptySet:
        return false;

    case SetKind::IntSet:
    {
        Var taggedInt = JavascriptConversion::TryCanonicalizeAsTaggedInt<false /* allowNegOne */, true /* allowLossyConversion */ >(value);
        if (!taggedInt)
        {
            return false;
        }
        int32 intVal = TaggedInt::ToInt32(taggedInt);
        return this->u.intSet->Test(intVal);
    }

    case SetKind::SimpleVarSet:
    {
        // First check if the value is in the set
        if (this->u.simpleVarSet->ContainsKey(value))
        {
            return true;
        }
        // If the value isn't in the set, check if the canonical value is
        Var simpleVar = JavascriptConversion::TryCanonicalizeAsSimpleVar<true /* allowLossyConversion */>(value);
        // If the simple value is the same as the original value, we know it isn't in the set
        if (!simpleVar || simpleVar == value)
        {
            return false;
        }

        return this->u.simpleVarSet->ContainsKey(simpleVar);
    }

    case SetKind::ComplexVarSet:
        return this->u.complexVarSet->ContainsKey(value);

    default:
        Assume(UNREACHED);
        return false;
    }
}

int JavascriptSet::Size()
{
    JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());
    switch (this->kind)
    {
    case SetKind::EmptySet:
        return 0;
    case SetKind::IntSet:
        return this->u.intSet->Count();
    case SetKind::SimpleVarSet:
        return this->u.simpleVarSet->Count();
    case SetKind::ComplexVarSet:
        return this->u.complexVarSet->Count();
    default:
        Assume(UNREACHED);
        return 0;
    }
}

BOOL JavascriptSet::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
{
    stringBuilder->AppendCppLiteral(_u("Set"));
    return TRUE;
}

#if ENABLE_TTD
void JavascriptSet::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
{
    auto iterator = this->GetIterator();
    while(iterator.Next())
    {
        extractor->MarkVisitVar(iterator.Current());
    }
}

TTD::NSSnapObjects::SnapObjectType JavascriptSet::GetSnapTag_TTD() const
{
    return TTD::NSSnapObjects::SnapObjectType::SnapSetObject;
}

void JavascriptSet::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
{
    TTD::NSSnapObjects::SnapSetInfo* ssi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapSetInfo>();
    ssi->SetSize = 0;

    if(this->Size() == 0)
    {
        ssi->SetValueArray = nullptr;
    }
    else
    {
        ssi->SetValueArray = alloc.SlabAllocateArray<TTD::TTDVar>(this->Size());

        auto iter = this->GetIterator();
        while(iter.Next())
        {
            ssi->SetValueArray[ssi->SetSize] = iter.Current();
            ssi->SetSize++;
        }
    }

    TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapSetInfo*, TTD::NSSnapObjects::SnapObjectType::SnapSetObject>(objData, ssi);
}

JavascriptSet* JavascriptSet::CreateForSnapshotRestore(ScriptContext* ctx)
{
    JavascriptSet* res = ctx->GetLibrary()->CreateSet();

    return res;
}
#endif
}
