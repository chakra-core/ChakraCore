//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Bug Issue 5777 https://github.com/Microsoft/ChakraCore/issues/5777
// Duplicate export names should cause an early syntax error

WScript.RegisterModuleSource("a.js",
    `export const boo = 4;
    export {bar as boo} from "b.js";
    print ("Should not be printed")`);
WScript.RegisterModuleSource("b.js","export const bar = 5;");

import("a.js").then(()=>{
    print("Failed - expected SyntaxError but no error thrown")
}).catch ((e)=>{
    if (e instanceof SyntaxError) {
        print("pass");
    } else {
        print (`Failed - threw ${e.constructor.toString()} but should have thrown SyntaxError`);
    }
});
