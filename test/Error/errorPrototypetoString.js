//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Test the fix for https://github.com/microsoft/ChakraCore/issues/6372


function checkObject(object)
{
    if (object.prototype.hasOwnProperty('toString'))
    {
        throw new Error(`${object.name}.prototype should not have own property 'toString'`);
    }
    if(object.prototype.toString !== Error.prototype.toString)
    {
        throw new Error(`${object.name}.prototype.toString should === Error.prototype.toString`);
    }
}

checkObject(EvalError);
checkObject(RangeError);
checkObject(ReferenceError);
checkObject(SyntaxError);
checkObject(URIError);

if (typeof WebAssembly !== 'undefined')
{
  checkObject(WebAssembly.CompileError);
  checkObject(WebAssembly.LinkError);
  checkObject(WebAssembly.RuntimeError);
}

print('pass');
