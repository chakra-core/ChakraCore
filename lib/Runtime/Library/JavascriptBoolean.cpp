//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    Var JavascriptBoolean::OP_LdTrue(ScriptContext*scriptContext)
    {
        return scriptContext->GetLibrary()->GetTrue();
    }

    Var JavascriptBoolean::OP_LdFalse(ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->GetFalse();
    }

    Js::Var JavascriptBoolean::ToVar(BOOL fValue, ScriptContext* scriptContext)
    {
        return
            fValue ?
            scriptContext->GetLibrary()->GetTrue() :
            scriptContext->GetLibrary()->GetFalse();
    }

    Var JavascriptBoolean::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.HasArg(), "Should always have implicit 'this'");

        // SkipDefaultNewObject function flag should have prevented the default object from
        // being created, except when call true a host dispatch.
        Var newTarget = args.GetNewTarget();
        bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);

        BOOL value;

        if (args.Info.Count > 1)
        {
            value = JavascriptConversion::ToBoolean(args[1], scriptContext) ? true : false;
        }
        else
        {
            value = false;
        }

        if (callInfo.Flags & CallFlags_New)
        {
            RecyclableObject* pNew = scriptContext->GetLibrary()->CreateBooleanObject(value);
            return isCtorSuperCall ?
                JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), pNew, nullptr, scriptContext) :
                pNew;
        }

        return scriptContext->GetLibrary()->CreateBoolean(value);
    }

    // Boolean.prototype.valueOf as described in ES6 spec (draft 24) 19.3.3.3
    Var JavascriptBoolean::EntryValueOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if(VarIs<JavascriptBoolean>(args[0]))
        {
            return args[0];
        }
        else if (VarIs<JavascriptBooleanObject>(args[0]))
        {
            JavascriptBooleanObject* booleanObject = VarTo<JavascriptBooleanObject>(args[0]);
            return scriptContext->GetLibrary()->CreateBoolean(booleanObject->GetValue());
        }
        else
        {
            return TryInvokeRemotelyOrThrow(EntryValueOf, scriptContext, args, JSERR_This_NeedBoolean, _u("Boolean.prototype.valueOf"));
        }
    }

    // Boolean.prototype.toString as described in ES6 spec (draft 24) 19.3.3.2
    Var JavascriptBoolean::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        AssertMsg(args.Info.Count, "Should always have implicit 'this'.");
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        BOOL bval;
        Var aValue = args[0];
        if(VarIs<JavascriptBoolean>(aValue))
        {
            bval = VarTo<JavascriptBoolean>(aValue)->GetValue();
        }
        else if (VarIs<JavascriptBooleanObject>(aValue))
        {
            JavascriptBooleanObject* booleanObject = VarTo<JavascriptBooleanObject>(aValue);
            bval = booleanObject->GetValue();
        }
        else
        {
            return TryInvokeRemotelyOrThrow(EntryToString, scriptContext, args, JSERR_This_NeedBoolean, _u("Boolean.prototype.toString"));
        }

        return bval ? scriptContext->GetLibrary()->GetTrueDisplayString() : scriptContext->GetLibrary()->GetFalseDisplayString();
    }

    RecyclableObject * JavascriptBoolean::CloneToScriptContext(ScriptContext* requestContext)
    {
        if (this->GetValue())
        {
            return requestContext->GetLibrary()->GetTrue();
        }
        return requestContext->GetLibrary()->GetFalse();
    }

    Var JavascriptBoolean::TryInvokeRemotelyOrThrow(JavascriptMethod entryPoint, ScriptContext * scriptContext, Arguments & args, int32 errorCode, PCWSTR varName)
    {
        if (JavascriptOperators::GetTypeId(args[0]) == TypeIds_HostDispatch)
        {
            Var result;
            if (VarTo<RecyclableObject>(args[0])->InvokeBuiltInOperationRemotely(entryPoint, args, &result))
            {
                return result;
            }
        }
        // Don't error if we disabled implicit calls
        if(scriptContext->GetThreadContext()->RecordImplicitException())
        {
            JavascriptError::ThrowTypeError(scriptContext, errorCode, varName);
        }
        else
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
    }

    BOOL JavascriptBoolean::Equals(Var other, BOOL* value, ScriptContext * requestContext)
    {
        return JavascriptBoolean::Equals(this, other, value, requestContext);
    }

    BOOL JavascriptBoolean::Equals(JavascriptBoolean* left, Var right, BOOL* value, ScriptContext * requestContext)
    {
        switch (JavascriptOperators::GetTypeId(right))
        {
        case TypeIds_Integer:
            *value = left->GetValue() ? TaggedInt::ToInt32(right) == 1 : TaggedInt::ToInt32(right) == 0;
            break;
        case TypeIds_Number:
            *value = left->GetValue() ? JavascriptNumber::GetValue(right) == 1.0 : JavascriptNumber::GetValue(right) == 0.0;
            break;
        case TypeIds_Int64Number:
            *value = left->GetValue() ? VarTo<JavascriptInt64Number>(right)->GetValue() == 1 : VarTo<JavascriptInt64Number>(right)->GetValue() == 0;
            break;
        case TypeIds_UInt64Number:
            *value = left->GetValue() ? VarTo<JavascriptUInt64Number>(right)->GetValue() == 1 : VarTo<JavascriptUInt64Number>(right)->GetValue() == 0;
            break;
        case TypeIds_Boolean:
            *value = left->GetValue() == VarTo<JavascriptBoolean>(right)->GetValue();
            break;
        case TypeIds_String:
            *value = left->GetValue() ? JavascriptConversion::ToNumber(right, requestContext) == 1.0 : JavascriptConversion::ToNumber(right, requestContext) == 0.0;
            break;
        case TypeIds_Symbol:
            *value = FALSE;
            break;
        case TypeIds_VariantDate:
            // == on a variant always returns false. Putting this in a
            // switch in each .Equals to prevent a perf hit by adding an
            // if branch to JavascriptOperators::Equal_Full
            *value = FALSE;
            break;
        case TypeIds_Undefined:
        case TypeIds_Null:
        default:
            *value = JavascriptOperators::Equal_Full(left->GetValue() ? TaggedInt::ToVarUnchecked(1) : TaggedInt::ToVarUnchecked(0), right, requestContext);
            break;
        }
        return true;
    }

    BOOL JavascriptBoolean::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        if (this->GetValue())
        {
            JavascriptString* trueDisplayString = GetLibrary()->GetTrueDisplayString();
            stringBuilder->Append(trueDisplayString->GetString(), trueDisplayString->GetLength());
        }
        else
        {
            JavascriptString* falseDisplayString = GetLibrary()->GetFalseDisplayString();
            stringBuilder->Append(falseDisplayString->GetString(), falseDisplayString->GetLength());
        }
        return TRUE;
    }

    BOOL JavascriptBoolean::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Boolean"));
        return TRUE;
    }

    RecyclableObject* JavascriptBoolean::ToObject(ScriptContext * requestContext)
    {
        return requestContext->GetLibrary()->CreateBooleanObject(this->GetValue() ? true : false);
    }

    Var JavascriptBoolean::GetTypeOfString(ScriptContext * requestContext)
    {
        return requestContext->GetLibrary()->GetBooleanTypeDisplayString();
    }
} // namespace Js
