//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function __getProperties(obj) {
  let properties = [];
  for (let name of Object.getOwnPropertyNames(obj)) {
 properties.push(name);
  }
  return properties;
}
function __getRandomObject() {
}
function __getRandomProperty(obj, seed) {
  let properties = __getProperties(obj);
  return properties[seed % properties.length];
}
  (function __f_2672() {
      __v_13851 = {
        get p() {
        }
      }
      __v_13862 = {
        get p() {
        },
        p: 2,
        set p(__v_13863) { WScript.Echo('pass'); this.q = __v_13863},
        p:9,
        q:3,
        set p(__v_13863) { WScript.Echo('pass'); this.q = __v_13863},        
      };
      __v_13862.p = 0;
      if (__v_13862.q !== 0) WScript.Echo(__v_13862.q);
  })();
  __v_13851[__getRandomProperty(__v_13851, 483779)] = __getRandomObject();