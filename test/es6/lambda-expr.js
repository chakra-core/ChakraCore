//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

try
{
    // Illegal case, arrow function terminates the expression
    eval("x=>{}/print(1)/+print(2)")
}
catch (e)
{
    print(e.message);
}

// Legal case, the line feed breaks the statement
x=>{}
/print(1)/+print(2)

// Legal case, comma separate ExpressionStatement
x=>{return 2;},y=>{return 3;}

// Legal case, comma separate Expression (with paren)
var a = (x=>{return 2;},y=>{return 3;})
print(a())


try
{
    eval("var a = x=>{return 2;},y=>{return 3;}");
}
catch (e)
{
    print(e.message);
}
