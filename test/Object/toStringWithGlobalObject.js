//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let global = new Function('return this;')();
console.log(global.toString());
console.log(global[Symbol.toStringTag]);
let descriptor = Object.getOwnPropertyDescriptor(global, Symbol.toStringTag);
console.log(descriptor.writable);
console.log(descriptor.enumerable);
console.log(descriptor.configurable);
