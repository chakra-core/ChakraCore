//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// Validation for the github issue 2645

WScript.LoadModule(`import * as mod1 from 'module_1_2645.js';
var ret = mod1.simpleExport();
print(ret === "exported" ? "PASS" : "FAILED");
`);
