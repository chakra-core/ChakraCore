//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Resolving module2 will result in error (can't find the module file).
// This causes us to mark module3 as having an error.
// Later we resolve module0 which parses correctly.
// This causes us to notify parents which will try to instantiate module3 because
// module0 did not have an error. However, module3 has children modules which were 
// not parsed. That put module3 into error state, we should skip instantiating
// module3.

WScript.RegisterModuleSource('module0.js', `
console.log('fail');
`);

WScript.RegisterModuleSource('module3.js', `
import { default as _default } from 'module0.js';
export { } from 'module2.js';
console.log('fail');
`);

WScript.LoadScriptFile('module3.js', 'module');

console.log('pass');
