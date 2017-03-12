//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


var a = new Object();
a.__proto__ = [1,2];

var b = [,1,2,3,]; 
b.__proto__ = new Function();

var c = function(a,b){
    this.length = 100;
}
c.prototype.length = 200;
d = new c();
c;
c; /**bp:evaluate('c.length');evaluate('c',2);evaluate('d',2)**/
c;
a;
a; /**bp:evaluate('a',2);evaluate('b',2)**/
a;
WScript.Echo('Pass');

