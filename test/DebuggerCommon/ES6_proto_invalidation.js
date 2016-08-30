/*
	Invalidating proto
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
c;

//values on object index
var a1 = new String();
a1[1] = "search1";
a1[2] = "search2";
a1; /**bp:evaluate('a1',3)**/
a1.__proto__ = [];
a1; /**bp:evaluate('a1',4)**/
a1.__proto__ = c;
a1; /**bp:evaluate('a1',4)**/

//invalidating a previously set deep proto
a1.__proto__ = [];
a1; /**bp:evaluate('a1',4)**/
WScript.Echo('PASSED');