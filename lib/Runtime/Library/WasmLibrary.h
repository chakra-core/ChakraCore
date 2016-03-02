//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class WasmLibrary
    {
    public:
        class EntryInfo
        {
        public:
            static FunctionInfo instantiateModule;
        };

        static Var instantiateModule(RecyclableObject* function, CallInfo callInfo, ...);
    };
}