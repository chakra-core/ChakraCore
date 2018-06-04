//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let calledHandler = false;
var func0 = function () {
};
var func1 = function () {
  var v0 = new Proxy(func0, proxyHandler);
  new v0().a;
};
var proxyHandler = {get: function () {
    calledHandler = true;
}};
func1()
func1()
func1()

print(calledHandler ? "Fail" : "Pass");
