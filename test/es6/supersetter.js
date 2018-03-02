//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// NOTE: Correct the baseline when setter behavior with 'super' is corrected

(function() {
    function __f_2530() {};
    __f_2530.prototype = {
      mSloppy() {
          super.ownReadonlyAccessor = 25;
          this.ownReadonlyAccessor;
      }
    }
    var __v_11980 = new __f_2530();
    Object.defineProperty(__v_11980, 'ownReadonlyAccessor', { 
        get: function () { console.log('hello');
        }, set: function () { console.log('goodbye'); }
    })
    __v_11980.mSloppy()
})();

