//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var res = []

function test1() 
{
    [].push.apply(this, arguments);
    res.push(1);
}

test1();

function test2() 
{
    ({}).toString.apply(this, arguments);
    res.push(2);
}

test2();

var count3 = 0;
function test3() 
{
    var args = arguments;
    function test3_inner() {
        (count3 == 1 ? args : arguments).callee.apply(this, arguments);
    }

    if (++count3 == 1)
    {
        return test3_inner();
    }
    res.push(3);
}

test3();

function test4()
{
    return function() {
    try {
        throw 4;
    } catch(ex) {
        res.push(4);
        var f = arguments[0]; 
    }
    f.apply(this, arguments);
    }
}
test4()(function(){ res.push(5); });

for (var i = 0; i < 5; ++i) {
    if (res[i] !== i + 1) {
        throw "fail";
    }
}

print("pass");
