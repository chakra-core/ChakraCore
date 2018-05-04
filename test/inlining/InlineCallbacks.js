//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

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

function Bar() { WScript.Echo("bar"); }
function DispatchBar() { Dispatch(Bar); }
function NestedDispatch() { Dispatch(DispatchBar) };
NestedDispatch();
NestedDispatch();
NestedDispatch();

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
