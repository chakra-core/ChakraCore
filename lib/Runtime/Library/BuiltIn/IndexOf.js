//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";

(function indexOf(searchElement, fromIndex) {

    var k;

    // 1. Let o be the result of calling ToObject passing
    //    the this value as the argument.
    if (this == null) {
        throw new TypeError('"this" is null or not defined');
    }

    var o = Object(this);

    // 2. Let lenValue be the result of calling the Get
    //    internal method of o with the argument "length".
    // 3. Let len be ToUint32(lenValue).
    var len = o.length >>> 0;

    // 4. If len is 0, return -1.
    if (len === 0) {
        return -1;
    }

    // 5. If argument fromIndex was passed let n be
    //    ToInteger(fromIndex); else let n be 0.
    var n = fromIndex | 0;

    // 6. If n >= len, return -1.
    if (n >= len) {
        return -1;
    }

    // 7. If n >= 0, then Let k be n.
    // 8. Else, n<0, Let k be len - abs(n).
    //    If k is less than 0, then let k be 0.
    k = Math.max(n >= 0 ? n : len - Math.abs(n), 0);

    // 9. Repeat, while k < len
    while (k < len) {
        // a. Let Pk be ToString(k).
        //   This is implicit for LHS operands of the in operator
        // b. Let kPresent be the result of calling the
        //    HasProperty internal method of o with argument Pk.
        //   This step can be combined with c
        // c. If kPresent is true, then
        //    i.  Let elementK be the result of calling the Get
        //        internal method of o with the argument ToString(k).
        //   ii.  Let same be the result of applying the
        //        Strict Equality Comparison Algorithm to
        //        searchElement and elementK.
        //  iii.  If same is true, return k.
        if (k in o && o[k] === searchElement) {
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
