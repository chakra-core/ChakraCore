//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

p1 = new Promise(
    function (resolve, reject) {
    WScript.SetTimeout(
      function () {
      p1.someOtherProp = "in fullfil";
      resolve("p1 resolved");
      /**bp:evaluate('p1', 2);**/
    }, 1000);
  });
  
p1.someOtherProp = "before";

p1;
/**bp:evaluate('p1', 2);**/

p1.then(
  function (val) {
  p1.someOtherProp = "in then";
  var x = val;
  /**bp:evaluate('p1', 2);**/
})
.catch (
  function (reason) {
  p1.someOtherProp = "in catch";
  var x = reason;
  /**bp:evaluate('p1', 2);**/
});

p2 = new Promise(function (resolve, reject) {
    WScript.SetTimeout(function () {
      resolve(null);
      /**bp:evaluate('p2', 2);**/
    }, 2000);
  });

p3 = new Promise(function (resolve, reject) {
    WScript.SetTimeout(function () {
      reject(["p3", "rejected"]);
      /**bp:evaluate('p3', 2);**/
    }, 3000);
  });

Promise.all([p2, p3]).then(function (value) {
  var x = 1;
  /**bp:evaluate('p1', 2);**/
  x;
  /**bp:evaluate('p2', 2);**/
  x;
  /**bp:evaluate('p3', 2);**/
}, function (reason) {
  var x = 1;
  /**bp:evaluate('p1', 2);**/
  x;
  /**bp:evaluate('p2', 2);**/
  x;
  /**bp:evaluate('p3', 2);**/
});

WScript.Echo("pass");