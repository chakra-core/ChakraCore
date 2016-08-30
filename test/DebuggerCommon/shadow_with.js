/*
 with shadows outer variable declaration
*/

var a = 1;
with ({a: 2}) {
    a++;
    eval("a=100;");
    WScript.Echo(a);
    WScript.Echo('PASSED'); /**bp:locals(1);**/
}
WScript.Echo(a);