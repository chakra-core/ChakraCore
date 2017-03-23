//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var f1 = (a = 10, b = function () { return a; }) => { 
    if (a === 10) {
        print("PASSED");
    } else {
        print("FAILED");
    }
    var a = 20; 
    if (a === 20) {
        print("PASSED");
    } else {
        print("FAILED");
    }
    return b; 
} 
if (f1()() === 10) {
    print("PASSED");
} else {
    print("FAILED");
}

function f2(a = 10, b = function () { return a; }) { 
    if (a === 10) {
        print("PASSED");
    } else {
        print("FAILED");
    }
    a = 20; 
    if (a === 20) {
        print("PASSED");
    } else {
        print("FAILED");
    }
    return b; 
} 
if (f2()() === 20) {
    print("PASSED");
} else {
    print("FAILED");
}

function f3(a = eval("10"), b = function () { return eval("a"); }) { 
    if (a === 10) {
        print("PASSED");
    } else {
        print("FAILED");
    }
    var a = 20; 
    if (a === 20) {
        print("PASSED");
    } else {
        print("FAILED");
    }
    return b; 
} 
if (f3()() === 10) {
    print("PASSED");
} else {
    print("FAILED");
}

function f4(a = 10, b = function () { return eval("a"); }) { 
    if (a === 10) {
        print("PASSED");
    } else {
        print("FAILED");
    }
    a = 20; 
    if (a === 20) {
        print("PASSED");
    } else {
        print("FAILED");
    }
    return b; 
} 
if (f4()() === 20) {
    print("PASSED");
} else {
    print("FAILED");
}

if ((({} = eval('')) => { return 10; })(1) === 10) {
    print("PASSED");
}