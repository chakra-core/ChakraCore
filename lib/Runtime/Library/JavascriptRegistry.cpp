//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptRegistry::JavascriptRegistry(DynamicType* type)
        : JavascriptMap(type)
    {
    }

    JavascriptRegistry* JavascriptRegistry::New(ScriptContext* scriptContext)
    {
        JavascriptRegistry* registry = scriptContext->GetLibrary()->CreateRegistry();
        registry->map = RecyclerNew(scriptContext->GetRecycler(), MapDataMap, scriptContext->GetRecycler());

        return registry;
    }

    bool JavascriptRegistry::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Registry;
    }

    JavascriptRegistry* JavascriptRegistry::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptRegistry'");

        return static_cast<JavascriptRegistry*>(RecyclableObject::FromVar(aValue));
    }

    Var JavascriptRegistry::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Registry"));

        JavascriptError::ThrowTypeError(scriptContext, JSERR_CannotCallOrConstruct, _u("Registry"));
    }

    Var JavascriptRegistry::EntryDelete(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptRegistry::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Registry.prototype.delete"), _u("Registry"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

        bool didDelete = map->Delete(key);

        return scriptContext->GetLibrary()->CreateBoolean(didDelete);
    }
    
    Var JavascriptRegistry::EntryGet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptRegistry::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Registry.prototype.get"), _u("Registry"));
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

    Var JavascriptRegistry::EntryHas(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptRegistry::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Registry.prototype.has"), _u("Registry"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        Var key = (args.Info.Count > 1) ? args[1] : scriptContext->GetLibrary()->GetUndefined();

        bool hasValue = map->Has(key);

        return scriptContext->GetLibrary()->CreateBoolean(hasValue);
    }

    Var JavascriptRegistry::EntrySet(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptRegistry::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Registry.prototype.set"), _u("Registry"));
        }
        if (args.Info.Count < 3 || !JavascriptModuleStatus::Is(args[2]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_FunctionArgument_Invalid, _u("entry"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);
        Var key = args[1];
        JavascriptModuleStatus* entry = JavascriptModuleStatus::FromVar(args[2]);

        if (JavascriptNumber::Is(key) && JavascriptNumber::IsNegZero(JavascriptNumber::GetValue(key)))
        {
            // Normalize -0 to +0
            key = JavascriptNumber::New(0.0, scriptContext);
        }

        map->Set(key, entry);

        return map;
    }

    Var JavascriptRegistry::EntryEntries(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptRegistry::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Registry.prototype.entries"), _u("Registry"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        return scriptContext->GetLibrary()->CreateMapIterator(map, JavascriptMapIteratorKind::KeyAndValue);
    }

    Var JavascriptRegistry::EntryKeys(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptRegistry::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Registry.prototype.keys"), _u("Registry"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        return scriptContext->GetLibrary()->CreateMapIterator(map, JavascriptMapIteratorKind::Key);
    }

    Var JavascriptRegistry::EntryValues(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptRegistry::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Registry.prototype.values"), _u("Registry"));
        }

        JavascriptMap* map = JavascriptMap::FromVar(args[0]);

        return scriptContext->GetLibrary()->CreateMapIterator(map, JavascriptMapIteratorKind::Value);
    }
    
    BOOL JavascriptRegistry::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Registry"));
        return TRUE;
    }
}
