/*
	Proto for user defined object
*/

var a = {
    x : 2
};

var b = {
    y : 2
};

b.__proto__ = a;

function MyObj() {
    this.z = 1;
}
var myObjInst = new MyObj();
myObjInst[1] = "MyObjIndex";
myObjInst.__proto__ = b;
WScript.Echo('PASSED'); /**bp:locals(2)**/