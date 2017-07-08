//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

evaluate = WScript.LoadScript;

this.__defineSetter__("x", function () { });

evaluate(`
  let x = 'let';
  Object.defineProperty(this, "x", { value:
          0xdec0  })
  if (x === 'let' && this.x === 57024)
  {
    WScript.Echo('pass');
  }
`);
