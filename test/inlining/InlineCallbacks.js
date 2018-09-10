//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Test inlining of callback.
function Dispatch(f) { f(); }
function Foo() { WScript.Echo("foo"); }
function DispatchFoo() { Dispatch(Foo); }

// make Dispatch megamorphic
Dispatch(function(){})
Dispatch(function(){})
Dispatch(function(){})
Dispatch(function(){})
Dispatch(function(){})
DispatchFoo();
DispatchFoo();
DispatchFoo();

// Test inlining of a callback function with a callback.
function Bar() { WScript.Echo("bar"); }
function DispatchBar() { Dispatch(Bar); }
function NestedDispatch() { Dispatch(DispatchBar) };
NestedDispatch();
NestedDispatch();
NestedDispatch();

// Test inlining of callback with argument
function Dispatch2(f, arg) { f(arg); }
function Blah(arg) { WScript.Echo(arg); }
function DispatchBlah(arg) { Dispatch2(Blah, arg) }

// make dispatch2 polymorphic.
Dispatch2(function(){})
Dispatch2(function(){})
DispatchBlah("blah");
DispatchBlah("blah");
DispatchBlah("blah");

// This will fail to inline the callback because currently we track at most one callback arg per callsite
function Dispatch3(a, b) { a(); b(); }
function DispatchFooBar() { Dispatch3(Foo, Bar); }
Dispatch3(function(){}, function(){});
Dispatch3(function(){}, function(){});
DispatchFooBar();
DispatchFooBar();
DispatchFooBar();

// test inlining of callback.call
function DispatchCall(callback, thisArg) { callback.call(thisArg); }
function DispatchFooCall() { DispatchCall(Foo, {}); }
DispatchCall(function(){});
DispatchCall(function(){}, []);
DispatchFooCall();
DispatchFooCall();
DispatchFooCall();

// test inlining of callback.apply
function DispatchApply(callback, thisArg) { callback.apply(thisArg); }
function DispatchBarApply() { DispatchApply(Bar, {}); }
DispatchApply(function(){});
DispatchApply(function(){}, []);
DispatchBarApply();
DispatchBarApply();
DispatchBarApply();
