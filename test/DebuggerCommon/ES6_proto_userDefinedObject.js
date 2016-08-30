//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

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