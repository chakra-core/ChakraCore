//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that circular overlapping module dependencies are all loaded before execution

WScript.RegisterModuleSource("mod0.js", "print('pass')");
WScript.RegisterModuleSource("mod1.js", "import 'mod2.js';");
WScript.RegisterModuleSource("mod2.js", "import 'mod0.js';");

WScript.LoadScriptFile("mod2.js", "module");
WScript.LoadScriptFile("mod1.js", "module");
