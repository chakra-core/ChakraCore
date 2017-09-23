//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var m = (function() {
  'use asm';
  function f()
  {
    return (2 >= -1 ? 0 : 1)|0;
  }
  return f;
  })()
print(m());
