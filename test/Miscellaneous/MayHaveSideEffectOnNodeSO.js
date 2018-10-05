//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// MayHaveSideEffectOnNode() was a recursive function that could recurse enough times to cause
// an uncaught stack overflow. This was fixed by converting the recursive loop into an iterative
// loop.
//
// An example of a program that caused the stack overflow is:
//      eval("var a;a>(" + Array(2 ** 15).fill(0).join(",") + ");");
//
// MayHaveSideEffectOnNode() is originally called because the righthand side of the pNodeBin
// ">" may overwrite the lefthand side of ">". The righthand side of pNodeBin is a deep tree
// in which each pNode of the longest path is a pNodeBin(",").Since children of the original
// pNodeBin -> right are pNodeBins themselves, MayHaveSideEffectOnNode() must check all of
// their children as well.MayHaveSideEffectOnNode's original implementation was recursive and
// thus the stack would overflow while recursing through the path of pNodeBins.   

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "MayHaveSideEffectOnNode() should not cause a stack overflow nor should fail to \
terminate",
        body: function () {
            eval("var a;a>(" + Array(2 ** 15).fill(0).join(",") + ");");
            eval("var a;a===(" + Array(2 ** 15).fill(0).join(",") + ");");
        }
    }
]

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });