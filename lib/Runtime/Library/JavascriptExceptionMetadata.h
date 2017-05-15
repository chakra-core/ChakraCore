//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#include "Types/SimplePropertyDescriptor.h"
#include "Types/SimpleTypeHandler.h"

namespace Js{
    class JavascriptExceptionMetadata {
    public:
        static Var CreateMetadataVar(ScriptContext * scriptContext);
        static void PopulateMetadataFromCompileException(Var metadata, Var exception, ScriptContext * scriptContext);
        static bool PopulateMetadataFromException(Var metadata, JavascriptExceptionObject * recordedException, ScriptContext * scriptContext);

    private:
        static SimpleTypeHandler<6> ExceptionMetadataTypeHandler;
    };
};