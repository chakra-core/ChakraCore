//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function i64ToString(val, optHigh = 0) {
  let high, low;
  if (typeof val === "object") {
    high = val.high;
    low = val.low;
  } else {
    low = val;
    high = optHigh;
  }
  const convert = (a, doPad) => {
    let s = (a >>> 0).toString(16);
    if (doPad) {
      s = s.padStart(8, "0");
    }
    return s;
  }
  if (high !== 0) {
    return `0x${convert(high)}${convert(low, true)}`;
  }
  return `0x${convert(low)}`;
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
