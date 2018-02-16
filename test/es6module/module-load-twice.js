//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.RegisterModuleSource('a.js', " ");
WScript.LoadScriptFile('a.js');
WScript.LoadScriptFile('a.js');
WScript.RegisterModuleSource('b.js', "import * as foo from 'a.js'");
WScript.LoadScriptFile('b.js', "module");
WScript.LoadScriptFile('b.js', "module");
print('pass');