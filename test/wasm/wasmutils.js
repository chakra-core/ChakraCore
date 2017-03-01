//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function i64ToString({high, low}) {
  const convert = a => (a >>> 0).toString(16).padStart(8, "0");
  return `0x${convert(high)}${convert(low)}`;
}

function fixupI64Return(exports, fnNames) {
  if (!Array.isArray(fnNames)) {
    fnNames = [fnNames];
  }
  fnNames.forEach(name => {
    const oldI64Fn = exports[name];
    exports[name] = function(...args) {
      const val = oldI64Fn(...args);
      return i64ToString(val);
    };
  })
}
