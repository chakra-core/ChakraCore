/*
   Strict mode, eval has its own scope
*/


function Run(){
    "use strict";
    let a = 1;
    eval('let a = 1; a++;');   //new scope
    WScript.Echo(a); /**bp:locals(1);evaluate('a')**/
}


WScript.Attach(Run);