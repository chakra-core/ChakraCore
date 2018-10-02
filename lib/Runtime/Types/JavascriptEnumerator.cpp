//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js
{
    JavascriptEnumerator::JavascriptEnumerator(ScriptContext* scriptContext) : RecyclableObject(scriptContext->GetLibrary()->GetEnumeratorType())
    {
        Assert(scriptContext != NULL);
    }

    template <> bool VarIsImpl<JavascriptEnumerator>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_Enumerator;
    }
}
