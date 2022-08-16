//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptBuiltInFunction
    {
    public:
        enum BuiltInEnum
        {
#define BUILTIN(c, n, e, i) c ## _ ## n,
#include "JavascriptBuiltInFunctionList.h"
#undef BUILTIN
            MaxBuiltInEnum
        };
        static FunctionInfo * GetFunctionInfo(Js::LocalFunctionId builtinId);
        static bool CanChangeEntryPoint(Js::LocalFunctionId builtInId);

        static bool IsValidId(Js::LocalFunctionId builtInId);

    private:
        static FunctionInfo * const builtInFunctionInfo[MaxBuiltInEnum];
    };
};
