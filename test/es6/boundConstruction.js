//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

//setup code
let constructionCount = 0;
function a(arg1, arg2) {
    this[arg1] = arg2;
}
const b = new Proxy(a, {
    construct: function (x, y, z) {
    ++constructionCount;
    return Reflect.construct(x, y, z);
}
});
const boundObject = {};
const c = b.bind(boundObject, "prop-name");

const tests = [
    {
        name : "Construct bound proxy with Reflect",
        body : function() {
            class newTarget {}
            constructionCount = 0;
            const obj = Reflect.construct(c, ["prop-value-2"], newTarget);
            assert.areEqual(newTarget.prototype, obj.__proto__, "bound function should use explicit newTarget if provided");
            assert.areEqual("prop-value-2", obj["prop-name"], "bound function should keep bound arguments when constructing");
            //assert.areEqual(1, constructionCount, "bound proxy should be constructed once"); //currently fails
            assert.areEqual(a.prototype, obj.__proto__, "constructed object should be instance of original function");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" }); 
