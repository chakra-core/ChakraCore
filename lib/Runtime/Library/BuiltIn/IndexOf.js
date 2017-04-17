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

    if (n >= 0) {
        k = n;
    } else {
        k = len - Math.abs(n);
    }

    if (k < 0) {
        k = 0;
    }

    while (k < len) {
        if (o[k] === searchElement && (o[k] !== undefined || searchElement !== undefined)) {
            return k;
        }
        k++;
    }

    return -1;
});