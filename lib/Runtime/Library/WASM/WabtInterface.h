//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef ENABLE_WABT
namespace Js
{
    class WabtInterface
    {
    public:
        class EntryInfo
        {
        public:
            static FunctionInfo ConvertWast2Wasm;
        };
        static Var EntryConvertWast2Wasm(RecyclableObject* function, CallInfo callInfo, ...);
    };
}
#endif
