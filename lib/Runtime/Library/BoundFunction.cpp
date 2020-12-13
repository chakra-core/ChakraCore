//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    FunctionInfo BoundFunction::functionInfo(FORCE_NO_WRITE_BARRIER_TAG(BoundFunction::NewInstance), FunctionInfo::DoNotProfile);

    BoundFunction::BoundFunction(DynamicType * type)
        : JavascriptFunction(type, &functionInfo),
        targetFunction(nullptr),
        boundThis(nullptr),
        count(0),
        boundArgs(nullptr)
    {
        // Constructor used during copy on write.
        DebugOnly(VerifyEntryPoint());
    }

    BoundFunction::BoundFunction(Arguments args, DynamicType * type)
        : JavascriptFunction(type, &functionInfo),
        count(0),
        boundArgs(nullptr)
    {
        DebugOnly(VerifyEntryPoint());
        AssertMsg(args.Info.Count > 0, "wrong number of args in BoundFunction");

        ScriptContext *scriptContext = this->GetScriptContext();
        targetFunction = VarTo<RecyclableObject>(args[0]);

        Assert(!CrossSite::NeedMarshalVar(targetFunction, scriptContext));

        // Let proto be targetFunction.[[GetPrototypeOf]]().
        RecyclableObject* proto = JavascriptOperators::GetPrototype(targetFunction);
        if (proto != type->GetPrototype())
        {
            if (type->GetIsShared())
            {
                this->ChangeType();
                type = this->GetDynamicType();
            }
            type->SetPrototype(proto);
        }

        int len = 0;
        // If targetFunction is proxy, need to make sure that traps are called in right order as per 19.2.3.2 in RC#4 dated April 3rd 2015.
        // additionally need to get the correct length value for the boundFunctions' length property
        if (JavascriptOperators::HasOwnProperty(targetFunction, PropertyIds::length, scriptContext, nullptr) == TRUE)
        {
            Var varLength;
            if (targetFunction->GetProperty(targetFunction, PropertyIds::length, &varLength, nullptr, scriptContext))
            {
                len = JavascriptConversion::ToInt32(varLength, scriptContext);
            }
        }
        GetTypeHandler()->EnsureObjectReady(this);

        if (args.Info.Count > 1)
        {
            boundThis = args[1];

            // function object and "this" arg
            const uint countAccountedFor = 2;
            count = args.Info.Count - countAccountedFor;

            // Store the args excluding function obj and "this" arg
            if (args.Info.Count > 2)
            {
                boundArgs = RecyclerNewArray(scriptContext->GetRecycler(), Field(Var), count);

                for (uint i=0; i<count; i++)
                {
                    boundArgs[i] = args[i+countAccountedFor];
                }
            }
        }
        else
        {
            // If no "this" is passed, "undefined" is used
            boundThis = scriptContext->GetLibrary()->GetUndefined();
        }

        // Reduce length number of bound args
        len = len - this->count;
        len = max(len, 0);

        SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(len), PropertyConfigurable, nullptr, PropertyOperation_None, SideEffects_None);
    }

    BoundFunction* BoundFunction::New(ScriptContext* scriptContext, ArgumentReader args)
    {
        Recycler* recycler = scriptContext->GetRecycler();

        BoundFunction* boundFunc = RecyclerNew(recycler, BoundFunction, args,
            scriptContext->GetLibrary()->GetBoundFunctionType());
        return boundFunc;
    }

    Var BoundFunction::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        RUNTIME_ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction /* TODO-ERROR: get arg name - args[0] */);
        }

        BoundFunction *boundFunction = (BoundFunction *) function;
        RecyclableObject *targetFunction = boundFunction->targetFunction;

        //
        // var o = new boundFunction()
        // a new object should be created using the actual function object
        //
        Var newVarInstance = nullptr;
        if (callInfo.Flags & CallFlags_New)
        {
            if (args.HasNewTarget())
            {
                // target has an overridden new target make a new object from the newTarget
                Var newTargetVar = args.GetNewTarget();
                AssertOrFailFastMsg(JavascriptOperators::IsConstructor(newTargetVar), "newTarget must be a constructor");
                RecyclableObject* newTarget = UnsafeVarTo<RecyclableObject>(newTargetVar);

                // Class constructors expect newTarget to be in args slot 0 (usually "this"),
                // because "this" is not constructed until we reach the most-super superclass.
                FunctionInfo* functionInfo = JavascriptOperators::GetConstructorFunctionInfo(targetFunction, scriptContext);
                if (functionInfo && functionInfo->IsClassConstructor())
                {
                    args.Values[0] = newVarInstance = newTarget;
                }
                else
                {
                    BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                    {
                        args.Values[0] = newVarInstance = JavascriptOperators::CreateFromConstructor(newTarget, scriptContext);
                    }
                    END_SAFE_REENTRANT_CALL
                }
            }
            else if (!VarIs<JavascriptProxy>(targetFunction))
            {
                // No new target and target is not a proxy can make a new object in a "normal" way.
                // NewScObjectNoCtor will either construct an object or return targetFunction depending
                // on whether targetFunction is a class constructor.
                BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                {
                    args.Values[0] = newVarInstance = JavascriptOperators::NewScObjectNoCtor(targetFunction, scriptContext);
                }
                END_SAFE_REENTRANT_CALL
            }
            else
            {
                // target is a proxy without an overridden new target
                // give nullptr - FunctionCallTrap will make a new object
                args.Values[0] = newVarInstance;
            }
        }

        Js::Arguments actualArgs = args;

        if (boundFunction->count > 0)
        {
            // OACR thinks that this can change between here and the check in the for loop below
            const unsigned int argCount = args.Info.Count;

            uint32 newArgCount = UInt32Math::Add(boundFunction->count, args.GetLargeArgCountWithExtraArgs());
            if (newArgCount > CallInfo::kMaxCountArgs)
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgListTooLarge);
            }

            Field(Var) *newValues = RecyclerNewArray(scriptContext->GetRecycler(), Field(Var), newArgCount);
            uint index = 0;

            //
            // For [[Construct]] use the newly created var instance
            // For [[Call]] use the "this" to which bind bound it.
            //
            if (callInfo.Flags & CallFlags_New)
            {
                newValues[index++] = args[0];
            }
            else
            {
                newValues[index++] = boundFunction->boundThis;
            }

            for (uint i = 0; i < boundFunction->count; i++)
            {
                newValues[index++] = boundFunction->boundArgs[i];
            }

            // Copy the extra args
            for (uint i=1; i<argCount; i++)
            {
                newValues[index++] = args[i];
            }

            if (args.HasExtraArg())
            {
                newValues[index++] = args.Values[argCount];
            }

            actualArgs = Arguments(args.Info, unsafe_write_barrier_cast<Var*>(newValues));
            actualArgs.Info.Count = boundFunction->count + argCount;

            Assert(index == actualArgs.GetLargeArgCountWithExtraArgs());
        }
        else
        {
            if (!(callInfo.Flags & CallFlags_New))
            {
                actualArgs.Values[0] = boundFunction->boundThis;
            }
        }

        Var aReturnValue = nullptr;
        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            // Number of arguments are allowed to be more than Constants::MaxAllowedArgs in runtime. Need to use the larger argcount logic for this call.
            aReturnValue = JavascriptFunction::CallFunction<true>(targetFunction, targetFunction->GetEntryPoint(), actualArgs, /* useLargeArgCount */ true);
        }
        END_SAFE_REENTRANT_CALL

        //
        // [[Construct]] and call returned a non-object
        // return the newly created var instance
        //
        if ((callInfo.Flags & CallFlags_New) && !JavascriptOperators::IsObject(aReturnValue))
        {
            aReturnValue = newVarInstance;
        }

        return aReturnValue;
    }

    JavascriptFunction * BoundFunction::GetTargetFunction() const
    {
        if (targetFunction != nullptr)
        {
            RecyclableObject* _targetFunction = targetFunction;
            while (VarIs<JavascriptProxy>(_targetFunction))
            {
                _targetFunction = VarTo<JavascriptProxy>(_targetFunction)->GetTarget();
            }

            if (VarIs<JavascriptFunction>(_targetFunction))
            {
                return VarTo<JavascriptFunction>(_targetFunction);
            }

            // targetFunction should always be a JavascriptFunction.
            Assert(FALSE);
        }
        return nullptr;
    }

    JavascriptString* BoundFunction::GetDisplayNameImpl() const
    {
        JavascriptString* displayName = GetLibrary()->GetEmptyString();
        if (targetFunction != nullptr)
        {
            Var value = JavascriptOperators::GetPropertyNoCache(targetFunction, PropertyIds::name, targetFunction->GetScriptContext());
            if (VarIs<JavascriptString>(value))
            {
                displayName = VarTo<JavascriptString>(value);
            }
        }
        return JavascriptString::Concat(GetLibrary()->GetBoundFunctionPrefixString(), displayName);
    }

    RecyclableObject* BoundFunction::GetBoundThis()
    {
        if (boundThis != nullptr && VarIs<RecyclableObject>(boundThis))
        {
            return VarTo<RecyclableObject>(boundThis);
        }
        return NULL;
    }

    inline BOOL BoundFunction::IsConstructor() const
    {
        if (this->targetFunction != nullptr)
        {
            return JavascriptOperators::IsConstructor(this->GetTargetFunction());
        }

        return false;
    }

    PropertyQueryFlags BoundFunction::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return BoundFunction::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    _Check_return_ _Success_(return) BOOL BoundFunction::GetAccessors(PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter, ScriptContext* requestContext)
    {
        return DynamicObject::GetAccessors(propertyId, getter, setter, requestContext);
    }

    DescriptorFlags BoundFunction::GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return DynamicObject::GetSetter(propertyId, setterValue, info, requestContext);
    }

    DescriptorFlags BoundFunction::GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return DynamicObject::GetSetter(propertyNameString, setterValue, info, requestContext);
    }

    BOOL BoundFunction::InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        return SetProperty(propertyId, value, PropertyOperation_None, info);
    }

    BOOL BoundFunction::HasInstance(Var instance, ScriptContext* scriptContext, IsInstInlineCache* inlineCache)
    {
        return this->targetFunction->HasInstance(instance, scriptContext, inlineCache);
    }

#if ENABLE_TTD
    void BoundFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        extractor->MarkVisitVar(this->targetFunction);

        if(this->boundThis != nullptr)
        {
            extractor->MarkVisitVar(this->boundThis);
        }

        for(uint32 i = 0; i < this->count; ++i)
        {
            extractor->MarkVisitVar(this->boundArgs[i]);
        }
    }

    void BoundFunction::ProcessCorePaths()
    {
        this->GetScriptContext()->TTDWellKnownInfo->EnqueueNewPathVarAsNeeded(this, this->targetFunction, _u("!targetFunction"));
        this->GetScriptContext()->TTDWellKnownInfo->EnqueueNewPathVarAsNeeded(this, this->boundThis, _u("!boundThis"));

        TTDAssert(this->count == 0, "Should only have empty args in core image");
    }

    TTD::NSSnapObjects::SnapObjectType BoundFunction::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapBoundFunctionObject;
    }

    void BoundFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapBoundFunctionInfo* bfi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapBoundFunctionInfo>();

        bfi->TargetFunction = TTD_CONVERT_VAR_TO_PTR_ID(static_cast<RecyclableObject*>(this->targetFunction));
        bfi->BoundThis = (this->boundThis != nullptr) ?
            TTD_CONVERT_VAR_TO_PTR_ID(static_cast<Var>(this->boundThis)) : TTD_INVALID_PTR_ID;

        bfi->ArgCount = this->count;
        bfi->ArgArray = nullptr;

        if(bfi->ArgCount > 0)
        {
            bfi->ArgArray = alloc.SlabAllocateArray<TTD::TTDVar>(bfi->ArgCount);
        }

        TTD_PTR_ID* depArray = alloc.SlabReserveArraySpace<TTD_PTR_ID>(bfi->ArgCount + 2 /*this and bound function*/);

        depArray[0] = bfi->TargetFunction;
        uint32 depCount = 1;

        if(this->boundThis != nullptr && TTD::JsSupport::IsVarComplexKind(this->boundThis))
        {
            depArray[depCount] = bfi->BoundThis;
            depCount++;
        }

        if(bfi->ArgCount > 0)
        {
            for(uint32 i = 0; i < bfi->ArgCount; ++i)
            {
                bfi->ArgArray[i] = this->boundArgs[i];

                //Primitive kinds always inflated first so we only need to deal with complex kinds as depends on
                if(TTD::JsSupport::IsVarComplexKind(this->boundArgs[i]))
                {
                    depArray[depCount] = TTD_CONVERT_VAR_TO_PTR_ID(this->boundArgs[i]);
                    depCount++;
                }
            }
        }
        alloc.SlabCommitArraySpace<TTD_PTR_ID>(depCount, depCount + bfi->ArgCount);

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapBoundFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapBoundFunctionObject>(objData, bfi, alloc, depCount, depArray);
    }

    BoundFunction* BoundFunction::InflateBoundFunction(
        ScriptContext* ctx, RecyclableObject* function, Var bThis, uint32 ct, Field(Var)* args)
    {
        BoundFunction* res = RecyclerNew(ctx->GetRecycler(), BoundFunction, ctx->GetLibrary()->GetBoundFunctionType());

        res->boundThis = bThis;
        res->count = ct;
        res->boundArgs = args;

        res->targetFunction = function;

        return res;
    }
#endif
} // namespace Js
