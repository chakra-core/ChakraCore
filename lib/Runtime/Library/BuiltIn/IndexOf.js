//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";

(function indexOf(searchElement, fromIndex) {

    var k;

    if (this == null) {
        throw new TypeError('"this" is null or not defined');
    }

    var o = this;

    var len = o.length >>> 0;

    if (o.length <= 0) {
        return -1;
    }

    var n = fromIndex | 0;

    if (n >= len) {
        return -1;
    }

    k = Math.max(n >= 0 ? n : len - Math.abs(n), 0);

    while (k < len) {
        if (o[k] === searchElement && (o[k] !== undefined || searchElement !== undefined)) {
            return k;
        }
        k++;
    }

    return -1;
},
    function indexOf(searchElement, fromIndex) {

        var k;

        // 1. Let o be the result of calling ToObject passing
        //    the this value as the argument.
        if (this == null) {
            throw new TypeError('"this" is null or not defined');
        }

        var o = Object(this);

        // 2. Let S be ? ToString(O).
        var s = o.toString();

        // 3. Let lenValue be the result of calling the Get
        //    internal method of o with the argument "length".
        // 4. Let len be ToUint32(lenValue).
        var len = s.length >>> 0;

        // 5. If len is 0, return -1.
        if (len === 0) {
            return -1;
        }

        // 6. If argument fromIndex was passed let n be
        //    ToInteger(fromIndex); else let n be 0.
        var n = fromIndex | 0;

        // 7. If n >= len, return -1.
        if (n >= len) {
            return -1;
        }

        // 8. Let searchStr be ? ToString(searchString).
        if (searchElement === undefined) {
            searchElement = "undefined";
        }
        var searchStr = searchElement.toString();

        // 9.Let start be min(max(pos, 0), len).
        var start = Math.min(Math.max(n, 0), len);

        // 10. Let searchLen be the number of elements in searchStr.
        var searchLen = searchStr.length;

        // 11. Return the smallest possible integer k not smaller than start
        //     such that k+searchLen is not greater than len, and for all
        //     nonnegative integers j less than searchLen, the code unit at
        //     index k+j of S is the same as the code unit at index j of searchStr;
        //     but if there is no such integer k, return the value -1.
        while (start <= len - searchLen) {
            if (s.substr(start, searchLen) === searchStr) {
                return start;
            }
            start++;
        }

        return -1;
    });
