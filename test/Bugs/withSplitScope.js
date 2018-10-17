//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo()
{
    (function bar(a = 
        (function() 
        {
            with (1)
            {
                bar;
            }
        })()
    ){})();
}

foo();
foo();
foo();

WScript.Echo("Pass");
