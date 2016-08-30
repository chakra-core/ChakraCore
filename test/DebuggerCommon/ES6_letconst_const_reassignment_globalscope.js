/*
    Const Reassignment from debugger
*/

let a = 1;
const b = 2;
a; /**bp:locals(1);evaluate('b++')**/
WScript.Echo(a);
WScript.Echo('PASSED');
