//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";

(function arrayIndexOf(searchElement, fromIndex) {
    // var Math = setPrototype({
    //     abs: platform.builtInMathAbs,
    //     floor: platform.builtInMathFloor,
    //     max: platform.builtInMathMax,
    //     pow: platform.builtInMathPow
    // }, null);

    function type(x) {
        switch (typeof x) {
            case "string":
                return "String";
            case "number":
                return "Number";
            case "boolean":
                return "Boolean";
            case "symbol":
                return "Symbol";
            case "undefined":
                return "Undefined";
            case "object":
                if (x === null) {
                    return "Null";
                }
                return "Object";
            case "function":
                return "Object";
        }
    };

    function isPropertyKey(argument) {
        if (type(argument) === "String") {
            return true;
        }

        if (type(argument) === "Symbol") {
            return true;
        }

        return false;
    };

    function get(O, P) {
        if (isPropertyKey(P) === false) {
            throw new Error("Specification-level assertion failure");
        }

        return O[P];
    };

    function toNumber(argument) {
        return +argument;
    };

    function toInteger(argument) {
        const number = toNumber(argument);
        if (Number.isNaN(number)) {
            return +0;
        }

        if (number === 0 || number === -Infinity || number === +Infinity) {
            return number;
        }

        return (number < 0 ? -1 : +1) * Math.floor(Math.abs(number));
    };

    function min() {
        return Math.min.apply(undefined, arguments);
    }

    function toLength(argument) {
        const len = toInteger(argument);
        if (len <= +0) {
            return +0;
        }

        return min(len, Math.pow(2, 53) - 1);
    };

    let o;
    let len;
    let isArray = Array.isArray(this);

   // if (isArray) {
   //     // fast path.
   //     o = this;
   //     len = toLength(o.length);
   // } else {
        //slow path, more spec compliant
        o = Object(this);
        len = toLength(get(o, "length"));
   // }

    if (len === 0) {
        return -1;
    }

    let n = toInteger(fromIndex);

    if (n >= len) {
        return -1;
    }

    let k;

    if (n >= 0) {
        if (len == -0) {
            k = +0;
        } else {
            k = n;
        }
    } else if (n < 0) {
        k = len + n;

        if (k < 0) {
            k = 0;
        }
    }

    // if (isArray) {
    //     // fast path
    //     while (k < len) {
    //         k.toString(); // in the spec, we call toString to times. This one is useless but make it spec compliant.
    //         let elementK = o[k.toString()];
// 
    //         if (elementK === searchElement && (elementK !== undefined || (searchElement === undefined && k in o))) {
    //             return k;
    //         }
// 
    //         k++;
    //     }
    // } else {
        // slow path, more spec compliant
        while (k < len) {
            if (k in o) {
                let elementK = o[k];

                if (elementK === searchElement) {
                    return k;
                }
            }

            k++;
        }
    //}

    return -1;
    //  if (this == null) {
    //      throw new TypeError('"this" is null or not defined');
    //  }

    //  var o;
    //  if (a instanceof Array) {
    //      o = this;
    //  } else {
    //      o = Object(this);
    //  }

    //  let len = o.length >>> 0;

    //  if (o.length <= 0) {
    //      return -1;
    //  }

    //  let n = fromIndex | 0;
    //  //n = fromIndex >>> 0;

    //  if (n >= len) {
    //      return -1;
    //  }

    //  let k;

    //  if (n >= 0) {
    //      k = n;
    //  } else {
    //      k = len - Math.abs(n);
    //  }

    //  if (k < 0) {
    //      k = 0;
    //  }

    //  while (k < len) {
    //      if (o[k] === searchElement && (o[k] !== undefined || (searchElement === undefined && o.hasOwnProperty(k)))) {
    //          return k;
    //      }
    //      k++;
    //  }

    //  return -1;
});
