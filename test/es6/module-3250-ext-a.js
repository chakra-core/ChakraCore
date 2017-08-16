//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// module-3250-ext-a.js
import * as ns from './module-3250-ext-a.js';

export let a;
export * from './module-3250-ext-b.js';

var stringKeys = Object.getOwnPropertyNames(ns);
assert.areEqual('["a","b"]', JSON.stringify(stringKeys));
