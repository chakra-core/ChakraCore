//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let asmHeap = new ArrayBuffer(1 << 20);
let m = function (stdlib, foreign, heap) {
  'use asm';
  function f(d0) {
    d0 = +d0;
    return 1 % ~~d0 | 0;
  }
  return f;
}({}, {}, asmHeap);
print(m(4294967295));
print(m(4294967295));

m = function (stdlib, foreign, heap) {
  'use asm';
  function f(d0) {
    d0 = +d0;
    return 1 % (~~d0 >>> 0) | 0;
  }
  return f;
}({}, {}, asmHeap);
print(m(4294967295));
print(m(4294967295));