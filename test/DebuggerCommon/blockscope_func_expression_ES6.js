/*
    Blockscope Function Expression
    ES6 verification
 */


var x = 1; /**bp:locals(1)**/
WScript.Echo(bar);
{
    //function expression
    var bar = function () { return 'bar';}
}
WScript.Echo(bar);
WScript.Echo('PASSED')/**bp:locals(1)**/
