//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

Object.defineProperty(Array.prototype, 2, {get: function () { } });

var arr = {};
arr[0] = {};
arr.length = 10;
var protoObj = {};
Object.defineProperty(protoObj, 10, {});
arr.__proto__ = protoObj;

Array.prototype.sort.call(arr);
print('Pass');
