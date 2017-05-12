//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";

(function (intrinsic) {
    var platform = intrinsic.JsBuiltIn;

    let FunctionsEnum = {
        ArrayIndexOf: { className: "Array", methodName: "indexOf", argumentsCount: 1 },
    };

    platform.registerFunction(FunctionsEnum.ArrayIndexOf, function (searchElement, fromIndex) {
        // https://www.ecma-international.org/ecma-262/7.0/index.html#sec-array.prototype.indexof

        if (this === null || this === undefined) {
            throw new TypeError("Parent object is Null or undefined.");
        }

        let o = __chakraLibrary.Object(this);
        let len = __chakraLibrary.toLength(o["length"]);

        if (len === 0) {
            return -1;
        }

        let n = __chakraLibrary.toInteger(fromIndex);

        if (n >= len) {
            return -1;
        }

        let k;

        /* We refactored the code but it still respect the spec.
           When using -0 or +0, the engine might think we are meaning
           to use floating point numbers which can hurt performance.
           So we refactored to use integers instead. */
        if (n === 0) {
            k = 0;
        }
        else if (n > 0) {
            k = n;
        } else {
            k = len + n;

            if (k < 0) {
                k = 0;
            }
        }

        while (k < len) {
            if (k in o) {
                let elementK = o[k];

                if (elementK === searchElement) {
                    return k;
                }
            }

            k++;
        }

        return -1;
    });
});

