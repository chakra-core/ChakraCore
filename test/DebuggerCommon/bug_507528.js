//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Endless calls to cause the stackoverflow. This is to validate the bug 507528

function foo1() {
    function bar(ab) {
        function test1(ab1)
        {
            bar1.apply({});
        }
        test1();
    }
    function bar1() {
        bar.apply({abc:1});
    }
    bar1();
}
try
{
    foo1();
}
catch(e)
{
    WScript.Echo(e.description);
}