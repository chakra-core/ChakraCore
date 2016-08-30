/*
   eval has its own scope
*/

let a = 1;
eval('let a = 1; a++;');   //new scope
WScript.Echo('PASSED'); /**bp:locals(1)**/