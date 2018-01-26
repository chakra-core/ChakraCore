//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.RegisterModuleSource("mod0.js", `
    import 'mod1.js';
    import 'mod2.js';

    console.log("mod0");
`);

WScript.RegisterModuleSource("mod1.js",`
    import 'mod2.js';
    console.log("mod1");
`);

WScript.RegisterModuleSource("mod2.js",`
    import 'mod0.js';
    console.log("mod2");
`);

WScript.RegisterModuleSource("script0.js",`
    console.log("script0");
    import('mod1.js');
    import('mod2.js');
`);

WScript.LoadScriptFile("mod0.js", "module");
WScript.LoadScriptFile("script0.js");
