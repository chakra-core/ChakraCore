/*
    Blockscope Function declaration
    Function is visible only after statement in executed
 */


var x = 1; /**bp:locals(1)**/
WScript.Echo(foo);
WScript.Echo(bar);
switch (x) {
    //function delcaration
    case 1: function foo() { return 'foo'; } break;
    case 2: function bar() { return 'bar'; } break;
}
WScript.Echo(foo);
WScript.Echo(bar);
WScript.Echo('PASSED')/**bp:locals(1)**/
