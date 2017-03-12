//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var i = 0;
function incrementI() {
    return ++i; /**bp:stack();evaluate('i', 1);resume('step_out');stack()**/
}

// String templates with method invocation
WScript.Echo(`The count is ${incrementI() /**bp:resume('step_into');stack()**/}`);

while (i < 4) {
    WScript.Echo(`The count is ${incrementI() /**bp:resume('step_into');stack();evaluate('i * i', 1)**/}`);
}

// Tagged templates
function tempHandler(callsite, substitutions) {
    WScript.Echo(substitutions); /**bp:stack();evaluate('arguments', 3);resume('step_over');stack()**/
}
tempHandler`The count is
${++i /**bp:stack();evaluate('tempHandler',3)**/}
`;/**bp:resume('step_into');stack()**/

// String.raw
WScript.Echo(String.raw`The count is ${(function() {
    return ++i; /**bp:stack();evaluate('String', 2)**/
})()}`);/**bp:resume('step_into');stack();resume('step_into');stack()**/