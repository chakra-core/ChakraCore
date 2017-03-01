//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef ASMJS_PLAT
namespace Js
{
    class ScriptContext;
    class AsmJsCodeGenerator
    {
        ScriptContext* mScriptContext;
        InProcCodeGenAllocators* mForegroundAllocators;
        PageAllocator * mPageAllocator;
        AsmJsEncoder    mEncoder;
    public:
        AsmJsCodeGenerator( ScriptContext* scriptContext );
        void CodeGen( FunctionBody* functionBody );

    };
}
#endif
