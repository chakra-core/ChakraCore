//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let arr = new ArrayBuffer(10);
arr.constructor = new Proxy(ArrayBuffer, {});

arr.slice(1,2);

let p = new Proxy(Object, {});
for (let  i=0; i<20000; ++i)
{
    p = new Proxy(p, {});
}
let a = null;
try
{
    // should throw stack overflow or give a new object back
    a = new p();
    print(typeof a === "object" ? "Pass" : "Fail");
}
catch(e)
{
    print("Pass")
}
