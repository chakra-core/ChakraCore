//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
    var obj1 = {};
    var arrObj0 = obj1;
    var func0 = function(a,
      b = (Object.defineProperty(arrObj0, 'prop0', {set: function(_x) {   a; } }), (1 - {1: a }) )
    )
    {  
    };
    
    func0();
    obj1.prop0 = {};
    eval('');
  };
  
  test0();
  print("Pass");