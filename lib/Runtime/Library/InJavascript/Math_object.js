//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";

(function (intrinsic) {
    var platform = intrinsic.JsBuiltIn;

    __chakraLibrary.positiveInfinity = platform.POSITIVE_INFINITY;
    __chakraLibrary.negativeInfinity = platform.NEGATIVE_INFINITY;

    platform.registerFunction('min', function (value1, value2) {
        // #sec-math.min

        // If no arguments are given, the result is positive infinity
        // If any value is NaN, the result is NaN. 
        // The comparison of values to determine the smallest value is done using the Abstract Relational Comparison algorithm except that +0 is considered to be larger than -0.
        if (arguments.length === 0 ) {
            return __chakraLibrary.positiveInfinity;
        }
        
        value1 = +value1;
        if (value1 !== value1) {
            return NaN;
        }

        if (arguments.length === 1) {
            return value1;
        }

        if (arguments.length === 2) {
            value2 = +value2;
            if (value2 !== value2) {
                return NaN;
            }
            if ((value1 < value2) || (value1 === value2 && value1 === 0 && 1/value1 < 1/value2)) { // checks for -0 and +0
                return value1;
            }
            else {
                return value2;
            }
        }

        let min = value1;
        let nextVal;

        for (let i = 1; i < arguments.length; i++) {
            nextVal = +arguments[i];
            if (nextVal !== nextVal) {
                return NaN;
            }
            else if ((min > nextVal) || (min === nextVal && min === 0 && 1/min > 1/nextVal)) { // checks for -0 and +0
                min = nextVal;
            }
        }
        
        return min;
    });

    platform.registerFunction('max', function (value1, value2) {
        // #sec-math.max

        // If no arguments are given, the result is negative infinity
        // If any value is NaN, the result is NaN. 
        // The comparison of values to determine the largest value is done using the Abstract Relational Comparison algorithm except that +0 is considered to be larger than -0.
        if (arguments.length === 0) {
            return __chakraLibrary.negativeInfinity;
        }
        
        value1 = +value1;
        if (value1 !== value1) {
            return NaN;
        }

        if (arguments.length === 1) {
            return value1;
        }

        if (arguments.length === 2) {
            value2 = +value2;
            if (value2 !== value2) {
                return NaN;
            }
            if ((value1 > value2) || (value1 === value2 && value1 === 0 && 1/value1 > 1/value2)) { // checks for -0 and +0
                return value1;
            }
            else {
                return value2;
            }
        }

        let max = value1;
        let nextVal;

        for (let i = 1; i < arguments.length; i++) {
            nextVal = +arguments[i];
            if (nextVal !== nextVal) {
                return NaN;
            }
            else if ((max < nextVal) || (max === nextVal && max === 0 && 1/max < 1/nextVal)) { // checks for -0 and +0
                max = nextVal;
            } 
        }
        
        return max;
    });
});
