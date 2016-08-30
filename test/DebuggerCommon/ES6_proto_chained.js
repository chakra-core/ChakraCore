/*
	Chained proto
*/

var a = {
    x : 2
};
var b = {
    y : 2
};
b.__proto__ = a;

var c = [];
var d = new Date();
d.__proto__ = b;
a.__proto__ = [];
c.__proto__ = d;
c; /**bp:evaluate("c",5);evaluate("d",4);evaluate("b",3);evaluate("a",2)**/
WScript.Echo('PASSED');