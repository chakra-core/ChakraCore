//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// With -useparserstatecache -force:deferparse -force:redeferral -CollectGarbage we should not hit an AV

var glo;
function test()
{
    function nested1(param2)
    {
        function nested2()
        {
            return  param2;
        }

        escape();
        return 'pass';
    }
    WScript.Echo(nested1());

    function blah() { return 'pass' }
    function escape() { glo = blah; }
}


test("test3" )
WScript.Echo(glo());

CollectGarbage();

test("test1");
