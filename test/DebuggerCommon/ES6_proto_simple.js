/*
	simple case of __proto__
*/
var a = {
    x : 2
};
var b = {
    y : 2
};
b.__proto__ = a;
b; /**bp:locals(2)**/
WScript.Echo('PASSED');