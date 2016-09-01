//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validating the global scopes variables, ie. glo function and global at eval code.

function foo() {
    var mm = [22, 33];
    eval(' function f1() {}; \nvar a = 10; \nvar b= {};\n a;\n b; /**bp:locals(1);evaluate("mm")**/ \n var c1 = [1]; \nc;');
    eval(' try { \n abc.def = 10;\n } catch(ex1) { \n ex1; /**bp:stack();locals(1);evaluate("ex1");evaluate("c1")**/ } \nc;');
}

foo();

function bar() { }
bar;                             /**bp:locals(1);**/

try {
    abdd.dd = 20;
}
catch (ex2) {
    ex2;                        /**bp:locals(1);**/
}

var obj = { x: 10, y: [11, 22] };

with (obj) {
    var c = x;
    c;                          /**bp:locals(1);evaluate('y')**/
}

c++;

WScript.Echo("Pass");
