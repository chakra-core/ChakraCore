//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// @ts-check
/// <reference path="..\UnitTestFramework\UnitTestFramework.js" />
WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const tests = [{
    name: "Clear IsInstInlineCache of 'Object.setPrototypeOf'",
    body: function () {
        // See https://github.com/chakra-core/ChakraCore/issues/5915

        function Component() { }
        function Shape() { }

        function Box() { }
        Box.prototype = Object.create(Shape.prototype);
        Box.prototype.constructor = Box;

        function checkInstanceOf(a, b) {
            return a instanceof b;
        }

        assert.isFalse(Box.prototype instanceof Component, "Box.prototype instanceof Component");
        assert.isFalse(checkInstanceOf(Box.prototype, Component), "checkInstanceOf(Box.prototype, Component)");

        Object.setPrototypeOf(Shape.prototype, Component.prototype);

        assert.isTrue(Box.prototype instanceof Component, "Box.prototype instanceof Component");
        assert.isTrue(checkInstanceOf(Box.prototype, Component), "checkInstanceOf(Box.prototype, Component)");
    }
}];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
