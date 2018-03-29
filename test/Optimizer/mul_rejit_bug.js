//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function factorial(n)
{
    let result = 1;
    for(let i = 0; i < n; ++i)
    {
        result *= (n - i);
    }
    return result;
}

function exponent(n)
{
    let result = 1;
    for(let i = 0; i < n; ++i)
    {
        result *= 2;
    }
    return result;
}

function testFact()
{
    let result = 0.9999;
    for (let i = 0; i < 50; ++i)
    {
        result += factorial(i);
    }
    return result;
}

function testExp()
{
    let result = 0.9999;
    for (let i = 0; i < 50; ++i)
    {
        result += exponent(i);
    }
    return result;
}

WScript.Echo(testFact());
WScript.Echo(testExp());
