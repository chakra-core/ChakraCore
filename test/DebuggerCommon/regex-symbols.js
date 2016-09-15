//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

RegExp.prototype[Symbol.search] = function search() {
    return undefined; /**bp:stack()**/
}
'string'.search(/./);

RegExp.prototype[Symbol.match] = function match() {
    return undefined; /**bp:stack()**/
}
'string'.match(/./);

RegExp.prototype[Symbol.split] = function split() {
    return undefined; /**bp:stack()**/
}
'string'.split(/./);

RegExp.prototype[Symbol.replace] = function replace() {
    return undefined; /**bp:stack()**/
}
'string'.replace(/./, '-');

print("Pass");
