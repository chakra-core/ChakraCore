//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptArrayEnumeratorBase::JavascriptArrayEnumeratorBase(
        JavascriptArray* arrayObject, ScriptContext* scriptContext, BOOL enumNonEnumerable, bool enumSymbols) :
        JavascriptEnumerator(scriptContext),
        arrayObject(arrayObject),
        enumNonEnumerable(enumNonEnumerable),
        enumSymbols(enumSymbols)
    {
    }
}
