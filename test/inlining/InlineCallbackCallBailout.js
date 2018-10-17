//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// test bailout from inlined callback.call
function BailOut(ary)
{
    ary[0] = 1;
}

function DispatchCallWithThis(callback, arg)
{
     callback.call(this, arg);
}

function DispatchBailout(arg)
{
    DispatchCallWithThis(BailOut, arg);
}

DispatchBailout([1]);
DispatchBailout([1]);
DispatchBailout([1.1]);

// test bail out from having a different callback function
function foo()
{
    WScript.Echo("foo");
};

function Dispatch(callback)
{
    callback.call(undefined);
}

function CallDispatch(callback)
{
    Dispatch(callback);
}

CallDispatch(foo);
CallDispatch(foo);
CallDispatch(foo);

// BailOutOnInlineFunction
CallDispatch(function() { WScript.Echo("bar"); });

CallDispatch(foo);

// tautological statement makes function.call a non-fixed method. Test CheckFixedFld bailout.
Function.prototype.call = Function.prototype.call;
CallDispatch(foo)
CallDispatch(foo)

// rejit, not inlining callback.call
CallDispatch(foo)
