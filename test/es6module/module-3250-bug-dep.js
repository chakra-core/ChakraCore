//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// module-3250-bug-dep.js
import * as ns from './module-3250-bug-dep.js';

export let c_localUninit1;
export { c_localUninit1 as f_indirectUninit } from './module-3250-bug-dep2.js';

var stringKeys = Object.getOwnPropertyNames(ns);
assert.areEqual('["c_localUninit1","f_indirectUninit"]', JSON.stringify(stringKeys));
