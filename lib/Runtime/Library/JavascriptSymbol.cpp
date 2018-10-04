//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    PropertyRecordUsageCache * JavascriptSymbol::GetPropertyRecordUsageCache()
    {
        return &this->propertyRecordUsageCache;
    }

    Var JavascriptSymbol::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Symbol, scriptContext);

        // SkipDefaultNewObject function flag should have prevented the default object from
        // being created, except when call true a host dispatch.
        JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);

        if (callInfo.Flags & CallFlags_New)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ErrorOnNew, _u("Symbol"));
        }

        JavascriptString* description;

        if (args.Info.Count > 1 && !JavascriptOperators::IsUndefined(args[1]))
        {
            description = JavascriptConversion::ToString(args[1], scriptContext);
        }
        else
        {
            description = scriptContext->GetLibrary()->GetEmptyString();
        }

        return scriptContext->GetLibrary()->CreateSymbol(description);
    }

    // Symbol.prototype.valueOf as described in ES 2015
    Var JavascriptSymbol::EntryValueOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (VarIs<JavascriptSymbol>(args[0]))
        {
            return args[0];
        }
        else if (VarIs<JavascriptSymbolObject>(args[0]))
        {
            JavascriptSymbolObject* obj = VarTo<JavascriptSymbolObject>(args[0]);
            return CrossSite::MarshalVar(scriptContext, obj->Unwrap(), obj->GetScriptContext());
        }
        else
        {
            return TryInvokeRemotelyOrThrow(EntryValueOf, scriptContext, args, JSERR_This_NeedSymbol, _u("Symbol.prototype.valueOf"));
        }
    }

    // Symbol.prototype.toString as described in ES 2015
    Var JavascriptSymbol::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        AssertMsg(args.Info.Count, "Should always have implicit 'this'.");
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        const PropertyRecord* val;
        Var aValue = args[0];
        if (VarIs<JavascriptSymbol>(aValue))
        {
            val = VarTo<JavascriptSymbol>(aValue)->GetValue();
        }
        else if (VarIs<JavascriptSymbolObject>(aValue))
        {
            val = VarTo<JavascriptSymbolObject>(aValue)->GetValue();
        }
        else
        {
            return TryInvokeRemotelyOrThrow(EntryToString, scriptContext, args, JSERR_This_NeedSymbol, _u("Symbol.prototype.toString"));
        }

        return JavascriptSymbol::ToString(val, scriptContext);
    }

    // Symbol.for as described in ES 2015
    Var JavascriptSymbol::EntryFor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        AssertMsg(args.Info.Count, "Should always have implicit 'this'.");
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        Assert(!(callInfo.Flags & CallFlags_New));

        JavascriptString* key;

        if (args.Info.Count > 1)
        {
            key = JavascriptConversion::ToString(args[1], scriptContext);
        }
        else
        {
            key = library->GetUndefinedDisplayString();
        }

        // Search the global symbol registration map for a symbol with description equal to the string key.
        // The map can only have one symbol with that description so if we found a symbol, that is the registered
        // symbol for the string key.
        const Js::PropertyRecord* propertyRecord = scriptContext->GetThreadContext()->GetSymbolFromRegistrationMap(key->GetString(), key->GetLength());

        // If we didn't find a PropertyRecord in the map, we'll create a new symbol with description equal to the key string.
        // This is the only place we add new PropertyRecords to the map, so we should never have multiple PropertyRecords in the
        // map with the same string key value (since we would return the one we found above instead of creating a new one).
        if (propertyRecord == nullptr)
        {
            propertyRecord = scriptContext->GetThreadContext()->AddSymbolToRegistrationMap(key->GetString(), key->GetLength());
        }

        Assert(propertyRecord != nullptr);

        return scriptContext->GetSymbol(propertyRecord);
    }

    // Symbol.keyFor as described in ES 2015
    Var JavascriptSymbol::EntryKeyFor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        AssertMsg(args.Info.Count, "Should always have implicit 'this'.");
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count < 2 || !VarIs<JavascriptSymbol>(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSymbol, _u("Symbol.keyFor"));
        }

        JavascriptSymbol* sym = VarTo<JavascriptSymbol>(args[1]);
        const Js::PropertyRecord* symPropertyRecord = sym->GetValue();
        const char16* key = symPropertyRecord->GetBuffer();
        const charcount_t keyLength = symPropertyRecord->GetLength();

        // Search the global symbol registration map for a key equal to the description of the symbol passed into Symbol.keyFor.
        // Symbol.for creates a new symbol with description equal to the key and uses that key as a mapping to the new symbol.
        // There will only be one symbol in the map with that string key value.
        const Js::PropertyRecord* propertyRecord = scriptContext->GetThreadContext()->GetSymbolFromRegistrationMap(key, keyLength);

        // If we found a PropertyRecord in the map, make sure it is the same symbol that was passed to Symbol.keyFor.
        // If the two are different, it means the symbol passed to keyFor has the same description as a symbol registered via
        // Symbol.for _but_ is not the symbol returned from Symbol.for.
        if (propertyRecord != nullptr && propertyRecord == sym->GetValue())
        {
            return JavascriptString::NewCopyBuffer(key, sym->GetValue()->GetLength(), scriptContext);
        }

        return library->GetUndefined();
    }

    // Symbol.prototype[@@toPrimitive] as described in ES 2015
    Var JavascriptSymbol::EntrySymbolToPrimitive(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (VarIs<JavascriptSymbol>(args[0]))
        {
            return args[0];
        }
        else if (VarIs<JavascriptSymbolObject>(args[0]))
        {
            JavascriptSymbolObject* obj = VarTo<JavascriptSymbolObject>(args[0]);
            return CrossSite::MarshalVar(scriptContext, obj->Unwrap(), obj->GetScriptContext());
        }
        else
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSymbol, _u("Symbol[Symbol.toPrimitive]"));
        }
    }

    RecyclableObject * JavascriptSymbol::CloneToScriptContext(ScriptContext* requestContext)
    {
        // PropertyRecords are per-ThreadContext so we can just create a new primitive wrapper
        // around the PropertyRecord stored in this symbol via the other context library.
        return requestContext->GetSymbol(this->GetValue());
    }

    Var JavascriptSymbol::TryInvokeRemotelyOrThrow(JavascriptMethod entryPoint, ScriptContext * scriptContext, Arguments & args, int32 errorCode, PCWSTR varName)
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
        if (scriptContext->GetThreadContext()->RecordImplicitException())
        {
            JavascriptError::ThrowTypeError(scriptContext, errorCode, varName);
        }
        else
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
    }

    BOOL JavascriptSymbol::Equals(Var other, BOOL* value, ScriptContext * requestContext)
    {
        return JavascriptSymbol::Equals(this, other, value, requestContext);
    }

    BOOL JavascriptSymbol::Equals(JavascriptSymbol* left, Var right, BOOL* value, ScriptContext * requestContext)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(right);
        if (typeId != TypeIds_Symbol && typeId != TypeIds_SymbolObject)
        {
            right = JavascriptConversion::ToPrimitive<JavascriptHint::None>(right, requestContext);
            typeId = JavascriptOperators::GetTypeId(right);
        }

        switch (typeId)
        {
        case TypeIds_Symbol:
            *value = left == UnsafeVarTo<JavascriptSymbol>(right);
            Assert((left->GetValue() == UnsafeVarTo<JavascriptSymbol>(right)->GetValue()) == *value);
            break;
        case TypeIds_SymbolObject:
            *value = left == UnsafeVarTo<JavascriptSymbol>(UnsafeVarTo<JavascriptSymbolObject>(right)->Unwrap());
            Assert((left->GetValue() == UnsafeVarTo<JavascriptSymbolObject>(right)->GetValue()) == *value);
            break;
        default:
            *value = FALSE;
            break;
        }

        return TRUE;
    }

    BOOL JavascriptSymbol::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        if (this->GetValue())
        {
            stringBuilder->AppendCppLiteral(_u("Symbol("));
            stringBuilder->Append(this->GetValue()->GetBuffer(), this->GetValue()->GetLength());
            stringBuilder->Append(_u(')'));
        }
        return TRUE;
    }

    BOOL JavascriptSymbol::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Symbol"));
        return TRUE;
    }

    RecyclableObject* JavascriptSymbol::ToObject(ScriptContext * requestContext)
    {
        return requestContext->GetLibrary()->CreateSymbolObject(this);
    }

    Var JavascriptSymbol::GetTypeOfString(ScriptContext * requestContext)
    {
        return requestContext->GetLibrary()->GetSymbolTypeDisplayString();
    }

    JavascriptString* JavascriptSymbol::ToString(ScriptContext * requestContext)
    {
        if (requestContext->GetThreadContext()->RecordImplicitException())
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_ImplicitStrConv, _u("Symbol"));
        }

        return requestContext->GetLibrary()->GetEmptyString();
    }

    JavascriptString* JavascriptSymbol::ToString(const PropertyRecord* propertyRecord, ScriptContext * requestContext)
    {
        const char16* description = propertyRecord->GetBuffer();
        uint len = propertyRecord->GetLength();
        CompoundString* str = CompoundString::NewWithCharCapacity(len + _countof(_u("Symbol()")), requestContext->GetLibrary());

        str->AppendChars(_u("Symbol("), _countof(_u("Symbol(")) - 1);
        str->AppendChars(description, len);
        str->AppendChars(_u(')'));

        return str;
    }
} // namespace Js
