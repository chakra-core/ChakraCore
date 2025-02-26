//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function testReconfigureAsAccessorProperty(f) {
    var length = 2;
    Object.defineProperty(f, 'length', {
      get: function () {
        return length;
      },
      set: function (v) {
        length = v;
      }
    });
  }
  (function testSetOnInstance() {
    function f() {}
    delete f.length;
    testReconfigureAsAccessorProperty(f);
    Object.defineProperty(Function.prototype, 'length', {
      writable: true
    });
    f.length = 123;
    f.length == 123 ? print("Pass") : print('fail');
  })();
  