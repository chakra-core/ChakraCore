//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{

class WebAssembly
{
#ifdef ENABLE_WASM
public:
    class EntryInfo
    {
    public:
        static FunctionInfo Compile;
        static FunctionInfo Validate;
    };
    static Var EntryCompile(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryValidate(RecyclableObject* function, CallInfo callInfo, ...);
#endif
};

}
