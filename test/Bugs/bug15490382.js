//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function bar(x)
{
    if (x != x)
    {
        return;
    }
}

function foo()
{
    bar(typeof arguments[0]);
};

foo();
foo();
foo();

WScript.Echo("Passed");
