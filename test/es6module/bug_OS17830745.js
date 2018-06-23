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

WScript.RegisterModuleSource('module0_e0565b64-3435-42c1-ad1f-6376fb7af915.js', `
console.log('fail');`);
WScript.RegisterModuleSource('module3_65eb7cb0-2a92-4a34-a368-2cfc1d6a3768.js', `import {
  default as module3_localbinding_0,
  default as module3_localbinding_1
} from 'module0_e0565b64-3435-42c1-ad1f-6376fb7af915.js';
export { } from 'module2_894411c7-94ec-4069-b8e1-10ab0d881f6e.js';
console.log('fail');`);
WScript.LoadScriptFile('module3_65eb7cb0-2a92-4a34-a368-2cfc1d6a3768.js', 'module');
console.log('pass');
