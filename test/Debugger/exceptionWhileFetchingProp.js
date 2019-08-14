//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var p = new Proxy({x:10}, {
    getOwnPropertyDescriptor: function (oTarget, sKey) {
        throw new Error('');
        return { configurable: true, enumerable: true, value: 5 };
    }
  });
  
  function f() {
    var j = 1; /**bp:evaluate('p',1);**/
  }
  f();
  print('Pass');
  