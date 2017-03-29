//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) {
    this.WScript.LoadScriptFile("../UnitTestFramework/TrimStackTracePath.js");
}

function Dump(output)
{
  if (this.WScript)
  {
    WScript.Echo(output);
  }
  else
  {
    alert(output);
  }
}

function a() { }
a.prototype.x = function () { WScript.Echo(1); this.y(); };
a.prototype.y = function () { WScript.Echo("a"); };
var a1 = new a();
a1.f = function () { WScript.Echo(1); this.y(); };

function b() { }
b.prototype.p = 1;
b.prototype.x = function () { WScript.Echo("b"); };
b.prototype.f = function () { WScript.Echo("b1"); };
b.prototype.y = function () { WScript.Echo("b"); };
var b1 = new b();

function pr(){};
pr.f = function() {WScript.Echo("pr")};
b.prototype = pr;
b2 = new b();

function Inheriter() {
    this.f = function () { WScript.Echo("f"); }
}

function c() { }
c.prototype.x = function () { WScript.Echo(2); };
c.prototype.q = 1;
c.prototype.p = 1;

Inheriter.prototype = a.prototype;
c.prototype = new Inheriter();

var c1 = new c();

function d() { }
d.prototype.x = function () { WScript.Echo(2); };
d.prototype.r = 1;
d.prototype.q = 1;
d.prototype.p = 1;

//Inheriter.prototype = b.prototype;
d.prototype = new Inheriter();

var d1 = new d();

function foo(func) {
    func.f();
}

foo(b1);
foo(b2);
foo(b1);
foo(d1);

foo(a1);
foo(b1);
foo(c1);
foo(d1);

var obj1 = {};
var func3 = function () {
  function v6() {
  }
  v6.prototype.method0 = function () {
  };
  var v7 = new v6();
  function v8() {
  }
  v8.method0 = function () {
  };
  v6.prototype = v8;
  var v9 = new v6();
  function v15(v16) {
    for (var v18 = 0; v18 < 2; v18++) {
      v16.method0();
    }
  }
  v15(v7);
  v15(v9);
  v15();
};

obj1.method1 = func3;
try {
  obj1.method1();
} catch(e) {
  Dump(TrimStackTracePath(e.stack));
}