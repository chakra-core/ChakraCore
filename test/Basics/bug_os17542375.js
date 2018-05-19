//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// With pageheap:2 we should not hit an AV

function foo() {
   var r = {};
   var b = {};
   var a = {};
   a.$ = r;

   function foo1() {
   }
   function foo2() {
   }
    return r.noConflict = function(b) {
        return a.$ === r && (a.$ = Wb), b && a.jQuery === r && (a.jQuery = Vb), r
    }, b || (a.jQuery = a.$ = r), r
}

foo();
console.log('pass');
