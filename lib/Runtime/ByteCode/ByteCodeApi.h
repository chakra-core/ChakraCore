//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

HRESULT GenerateByteCode(_In_ ParseNodeProg *pnode, _In_ uint32 grfscr, _In_ Js::ScriptContext* scriptContext, __inout Js::ParseableFunctionInfo ** ppRootFunc,
                         _In_ uint sourceIndex, _In_ bool forceNoNative, _In_ Parser* parser, _In_ CompileScriptException *pse, Js::ScopeInfo* parentScopeInfo = nullptr,
                         Js::ScriptFunction ** functionRef = nullptr);

//
// Output for -Trace:ByteCode
//
#if DBG_DUMP
#define TRACE_BYTECODE(fmt, ...) \
    if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::ByteCodePhase)) \
    { \
        Output::Print(fmt, __VA_ARGS__); \
    }
#else
#define TRACE_BYTECODE(fmt, ...)
#endif
