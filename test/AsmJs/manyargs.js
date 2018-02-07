//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let str = `(function module() { "use asm";function foo(`;
// 550 double args exceeds 1 page
const totalArgs = 550
for (let i = 0; i < totalArgs; ++i)
{
    str += `arg${i},`;
}
str += `arg${totalArgs}){`;

for (let i = 0; i <= totalArgs; ++i)
{
    str += `arg${i}=+arg${i};`;
}
str += "return 10;}function bar(){return foo(";
for (let i = 0; i < totalArgs; ++i)
{
    str += "0.0,";
}
str += "1.0)|0;}"
str += "return bar})()()";

const result = eval(str);
print(result == 10 ? "Pass" : `Fail: ${result}`);