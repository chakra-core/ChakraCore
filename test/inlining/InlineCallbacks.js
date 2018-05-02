//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function Dispatch(f)
{
    f();
}

// make the callsite megamorphic
Dispatch(function(){})
Dispatch(function(){})
Dispatch(function(){})
Dispatch(function(){})
Dispatch(function(){})

function Foo() { WScript.Echo("foo"); }
function DispatchFoo() { Dispatch(Foo); }

DispatchFoo();
DispatchFoo();
DispatchFoo();
