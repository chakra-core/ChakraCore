//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace JSON
{
    class EntryInfo
    {
    public:
        static Js::FunctionInfo Stringify;
        static Js::FunctionInfo Parse;
    };

    Js::Var Stringify(Js::RecyclableObject* function, Js::CallInfo callInfo, ...);
    Js::Var Parse(Js::RecyclableObject* function, Js::CallInfo callInfo, ...);
} // namespace JSON
