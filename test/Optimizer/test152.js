//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function bar()
{
    return arguments.length + arguments[0];
}

function foo(a)
{
    return bar(a) + arguments;
}

foo(1)
foo(1)
foo(1)


function test4(a)
{
    return arguments.length + arguments[0];
}

function test3(a)
{
    return test4(a);
}

function test2(a)
{
    return test3(a) + arguments.length;
}

function test1(a)
{
    return test2(a)
}
test1(1)
test1(1)
test1(1)

print("passed")