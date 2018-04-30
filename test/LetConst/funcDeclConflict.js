//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function f1_f(){};
var f2_v = 123;
function f3_f(){};
var f4_v = 123;

let f5_l = 123;
const f6_c = 123;

var ex = "0";

// Top level function in script does not conflict with another top level function
WScript.LoadScript("function f1_f(){};");

// Top level function in script does not conflict with same-named var
WScript.LoadScript("function f2_v(){};");

// Top level function in eval does not conflict with another top level function
eval("function f3_f(){};");

// Top level function in eval does not conflict with same-named var
eval("function f4_v(){};");

// Top level function in script conflicts with top level let
try { WScript.LoadScript("function f5_l(){};"); } catch (e) { ex = e.message }

assert.areEqual("Let/Const redeclaration", ex);
ex = "1";

// Top level function in script conflicts with top level const
try { WScript.LoadScript("function f6_c(){};"); } catch (e) { ex = e.message }

assert.areEqual("Let/Const redeclaration", ex);
ex = "2";

// Top level function in eval conflicts with top level let
try { eval("function f5_l(){};") } catch (e) { ex = e.message}

assert.areEqual("Let/Const redeclaration", ex);
ex = "3";

// Top level function in eval conflicts with top level const
try { eval("function f6_c(){};") } catch (e) { ex = e.message }

assert.areEqual("Let/Const redeclaration", ex);
ex = "4";

(function ff() {
    if (true) {
        let fo5_l = 123;

        if (true) {
            // TODO: this is blocked by https://github.com/Microsoft/ChakraCore/issues/5070
            //
            // Top level function in eval conflicts with outer function level let
            try { eval("function fo5_l(){};") } catch (e) { ex = e.message }

            assert.areEqual("4", ex);
            ex = "5";
        }
    }

    if (true) {
        // Top level function in eval conflicts with outer function level const
        try { eval("function fo6_c(){};") } catch (e) { ex = e.message }
    }

    assert.areEqual("Let/Const redeclaration", ex);
    ex = "6";

    const fo6_c = 123;
})();

(function ffs() {
    'use strict';

    let fos5_l = 123;

    // Top level function in eval conflicts with outer function level let (strict)
    eval("function fos5_l(){};");

    if (true) {
        // Top level function in eval conflicts with outer function level const (strict)
        eval("function fos6_c(){};");
    }

    const fos6_c = 123;
})();


(function ffl() {
    let fo5_l_sl = 123;

    // Top level function in eval conflicts with outer function level let
    try { eval("function fo5_l_sl(){};") } catch (e) { ex = e.message }

    assert.areEqual("Let/Const redeclaration", ex);
    ex = "7";

    // Top level function in eval conflicts with outer function level const
    try { eval("function fo6_c_sl(){};") } catch (e) { ex = e.message }

    assert.areEqual("Let/Const redeclaration", ex);
    ex = "8";

    const fo6_c_sl = 123;
})();

(function ffsl() {
    'use strict';

    let fos5_l = 123;

    // Top level function in eval conflicts with outer function level let (strict)
    WScript.LoadScript("function fos5_l_sl(){};");

    // Top level function in eval conflicts with outer function level const (strict)
    WScript.LoadScript("function fos6_c_sl(){};");

    const fos6_c_sl = 123;
})();

(function ffn() {
    let fo5_l_nf = 123;

    // Top level function in "new Function" does not conflict with outer function level let
    f = (new Function("return function fo5_l_nf(){};"))();

    // Top level function in "new Function" does not conflict with outer function level const
    f = (new Function("return function fo6_c_nf(){};"))();

    assert.areEqual("function fo6_c_nf(){}", f.toString());

    const fo6_c_nf = 123;
})();

// Top level function in eval does not conflict with top level const, in a class (since strict is assumed)    
class C1
{                
    static M()
    {
        eval("function f6_c(){};");
    }
}

C1.M();

// Top level function in eval does not conflict with class level get      
class C2
{                
    get f7_l() {return "q";};

    static M()
    {
        eval("function f7_l(){};");
    }
}

C2.M();

WScript.RegisterModuleSource("mod0.js", `
    import 'mod1.js';
    let qwer = 12;
`);

WScript.RegisterModuleSource("mod1.js",`
    // no redeclaration here since modules are not introducing global names.
    export function qwer(){};
`);

WScript.LoadScriptFile("mod0.js", "module");

WScript.Echo("PASS");
