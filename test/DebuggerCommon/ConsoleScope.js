//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = 1;/**bp:evaluate("var varProp = 'varProp';let letProp = 'letProp';const constProp = 'constProp';function func() {return 0;};nonVarProp = 'nonVarProp';class class1{foo(){return 0;}};")**/

// New properties added in console eval scope shouldn't be available in user context
var propList = ["varProp", "letProp", "constProp", "func", "nonVarProp", "class1"];
for (var i = 0; i < propList.length; ++i) {
    var evalStr = "typeof " + propList[i] + " == 'undefined'";
    WScript.Echo(evalStr + " : " + eval(evalStr));
}

// New properties added in previous console eval scope should be available in subsequent console eval context
a = 2;/**bp:evaluate("WScript.Echo(varProp == 'varProp');WScript.Echo(letProp == 'letProp');WScript.Echo(constProp == 'constProp');WScript.Echo(nonVarProp == 'nonVarProp');");**/
a = 2;/**bp:evaluate("WScript.Echo(func() == 0);");**/
a = 2;/**bp:evaluate("WScript.Echo((new class1()).foo() == 0);");**/

// Same named let/const across multiple console eval invocation are treated as different but the last update wins in console scope
var a = 1;/**bp:evaluate("var varProp = 'varProp1';let letProp = 'letProp1';const constProp = 'constProp1';function func() {return 1;};nonVarProp = 'nonVarProp1';class class1{foo(){return 1;}};")**/
a = 2;/**bp:evaluate("WScript.Echo(varProp == 'varProp1');WScript.Echo(letProp == 'letProp1');WScript.Echo(constProp == 'constProp1');WScript.Echo(nonVarProp == 'nonVarProp1');");**/
a = 2;/**bp:evaluate("WScript.Echo(func() == 1);");**/
a = 2;/**bp:evaluate("WScript.Echo((new class1()).foo() == 1);");**/

// Only top level let/const should be available across multiple console eval invocation
var a = 1;/**bp:evaluate("let letProp1 = 1;const constProp1 = 1;{let letProp2 = 2;const constProp2 = 2;}")**/

propList = ["letProp2", "constProp2"];
for (var i = 0; i < propList.length; ++i) {
    var evalStr = "typeof " + propList[i] + " == 'undefined'";
    WScript.Echo(evalStr + " : " + eval(evalStr));
}

a = 1;/**bp:evaluate("WScript.Echo(typeof letProp2 == 'undefined');")**/
a = 1;/**bp:evaluate("WScript.Echo(typeof constProp2 == 'undefined');")**/


// Deleting Console scope variable should delete them
a = 2; /**bp:evaluate("delete letProp1;delete constProp1;")**/
a = 2; /**bp:evaluate("WScript.Echo(typeof letProp1 == 'undefined');")**/
a = 2; /**bp:evaluate("WScript.Echo(typeof constProp1 == 'undefined');")**/

a = 2; /**bp:evaluate("var varProp1 = 1;delete varProp1;let letProp1 = 1;delete letProp1;const constProp1 = 1;delete constProp1;")**/
a = 2; /**bp:evaluate("WScript.Echo(typeof varProp1 == 'undefined');")**/
a = 2; /**bp:evaluate("WScript.Echo(typeof letProp1 == 'undefined');")**/
a = 2; /**bp:evaluate("WScript.Echo(typeof constProp1 == 'undefined');")**/

// Redeclaration of let/const should throw redeclaration error
var a = 1;/**bp:evaluate("let letProp = 'letProp';let letProp = 'letPropReDecl';")**/
a = 2;/**bp:evaluate("const constProp = 'constProp';const constProp = 'constPropReDecl';")**/

// Same named let/const as user scope should not throw 'Use before declaration' error in console eval as it has its own scope
let userLet = 1;
a = 2;/**bp:evaluate("let userLet = 'userLetReDecl';")**/
const userConst = 1;
a = 2;/**bp:evaluate("const userConst = 'userConstReDecl';")**/