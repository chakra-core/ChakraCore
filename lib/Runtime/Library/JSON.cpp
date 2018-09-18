//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "Library/JSONStack.h"
#include "Library/JSONParser.h"
#include "Library/JSON.h"

using namespace Js;

// TODO: make JSON a class and use the normal EntryInfo code
namespace JSON
{
    Js::FunctionInfo EntryInfo::Stringify(FORCE_NO_WRITE_BARRIER_TAG(JSON::Stringify), Js::FunctionInfo::ErrorOnNew);
    Js::FunctionInfo EntryInfo::Parse(FORCE_NO_WRITE_BARRIER_TAG(JSON::Parse), Js::FunctionInfo::ErrorOnNew);

    Js::Var Parse(Js::JavascriptString* input, Js::RecyclableObject* reviver, Js::ScriptContext* scriptContext);

    Js::Var Parse(Js::RecyclableObject* function, Js::CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        //ES5:  parse(text [, reviver])
        ARGUMENTS(args, callInfo);
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        Js::ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("JSON.parse"));
        Assert(!(callInfo.Flags & Js::CallFlags_New));

        if(args.Info.Count < 2)
        {
            // if the text argument is missing it is assumed to be undefined.
            // ToString(undefined) returns "undefined" which is not a JSON grammar correct construct.  Shortcut and throw here
            Js::JavascriptError::ThrowSyntaxError(scriptContext, ERRsyntax);
        }

        Js::Var value = args[1];

        Js::JavascriptString * input = JavascriptOperators::TryFromVar<Js::JavascriptString>(value);
        if (!input)
        {
            input = Js::JavascriptConversion::ToString(value, scriptContext);
        }
        Js::RecyclableObject* reviver = nullptr;
        if (args.Info.Count > 2 && Js::JavascriptConversion::IsCallable(args[2]))
        {
            reviver = Js::UnsafeVarTo<Js::RecyclableObject>(args[2]);
        }

        return Parse(input, reviver, scriptContext);
    }

    Js::Var Parse(Js::JavascriptString* input, Js::RecyclableObject* reviver, Js::ScriptContext* scriptContext)
    {
        // alignment required because of the union in JSONParser::m_token
        __declspec (align(8)) JSONParser parser(scriptContext, reviver);
        Js::Var result = NULL;

        TryFinally([&]()
        {
            LazyJSONString* lazyString = JavascriptOperators::TryFromVar<LazyJSONString>(input);
            if (lazyString)
            {
                // Try to reconstruct object based on the data collected during stringify
                // In case of semantically important gap, this call may fail and we will need to do real parse
                result = lazyString->TryParse();
            }
            if (result == nullptr)
            {
                result = parser.Parse(input);
            }

    #ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            if (CONFIG_FLAG(ForceGCAfterJSONParse))
            {
                Recycler* recycler = scriptContext->GetRecycler();
                recycler->CollectNow<CollectNowForceInThread>();
            }
    #endif

            if (reviver)
            {
                Js::DynamicObject* root = scriptContext->GetLibrary()->CreateObject();
                JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(root));
                Js::PropertyId propertyId = scriptContext->GetEmptyStringPropertyId();
                Js::JavascriptOperators::InitProperty(root, propertyId, result);
                result = parser.Walk(scriptContext->GetLibrary()->GetEmptyString(), propertyId, root);
            }
        },
            [&](bool/*hasException*/)
        {
            parser.Finalizer();
        });

        return result;
    }

    Js::Var Stringify(Js::RecyclableObject* function, Js::CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        //ES5: Stringify(value, [replacer][, space]])
        ARGUMENTS(args, callInfo);
        Js::JavascriptLibrary* library = function->GetType()->GetLibrary();
        Js::ScriptContext* scriptContext = library->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("JSON.stringify"));

        Assert(!(callInfo.Flags & Js::CallFlags_New));

        if (args.Info.Count < 2)
        {
            // if value is missing it is assumed to be 'undefined'.
            // shortcut: the stringify algorithm returns undefined in this case.
            return library->GetUndefined();
        }
        Js::Var value = args[1];
        Js::Var replacerArg = args.Info.Count > 2 ? args[2] : nullptr;
        Js::Var space = args.Info.Count > 3 ? args[3] : library->GetNull();

        if (Js::JavascriptOperators::GetTypeId(value) == Js::TypeIds_HostDispatch)
        {
            // If we a remote object, we need to pull out the underlying JS object to stringify that
            Js::DynamicObject* remoteObject = Js::VarTo<Js::RecyclableObject>(value)->GetRemoteObject();
            if (remoteObject != nullptr)
            {
                AssertOrFailFast(Js::VarIsCorrectType(remoteObject));
                value = remoteObject;
            }
            else
            {
                Js::Var result;
                if (Js::VarTo<Js::RecyclableObject>(value)->InvokeBuiltInOperationRemotely(Stringify, args, &result))
                {
                    return result;
                }
            }
        }

        LazyJSONString* lazy = JSONStringifier::Stringify(scriptContext, value, replacerArg, space);
        if (!lazy)
        {
            return library->GetUndefined();
        }
        return lazy;
    }

} // namespace JSON
