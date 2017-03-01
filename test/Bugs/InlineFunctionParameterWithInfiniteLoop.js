//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function bar(o) {if(!o)for(;;);}

function baz(a){}
function foo() {baz(bar({}))};

foo();
foo();

print("pass");

