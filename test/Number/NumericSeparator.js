//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Basic decimal support",
    body: function () {
        assert.areEqual(1234, 1_234, "1234 === 1_234");
        assert.areEqual(1234, 1_2_3_4, "1234 === 1_2_3_4");
        assert.areEqual(1234.567, 1_2_3_4.5_6_7, "1234.567 === 1_2_3_4.5_6_7");

        assert.areEqual(-1234, -1_2_34, "-1234 === -1_2_34");
        assert.areEqual(-12.34, -1_2.3_4, "-12.34 === -1_2.3_4");
    }
  },
  {
    name: "Decimal with exponent",
    body: function () {
        assert.areEqual(1e100, 1e1_00, "1e100 === 1e1_00");
        assert.areEqual(Infinity, 1e1_0_0_0, "Infinity === 1e1_0_0_0");

        assert.areEqual(123.456e23, 1_2_3.4_5_6e2_3, "123.456e23 === 1_2_3.4_5_6e2_3");
        assert.areEqual(123.456e001, 1_2_3.4_5_6e0_0_1, "123.456e001 === 1_2_3.4_5_6e0_0_1");
    }
  },
  {
    name: "Decimal bad syntax",
    body: function () {
        // Decimal left-part only with numeric separators
        assert.throws(()=>eval('1__2'), SyntaxError, "Multiple numeric separators in a row are now allowed");
        assert.throws(()=>eval('1_2____3'), SyntaxError, "Multiple numeric separators in a row are now allowed");
        assert.throws(()=>eval('1_'), SyntaxError, "Decimal may not end in a numeric separator");
        assert.throws(()=>eval('1__'), SyntaxError, "Decimal may not end in a numeric separator");
        assert.throws(()=>eval('__1'), ReferenceError, "Decimal may not begin with a numeric separator");
        assert.throws(()=>eval('_1'), ReferenceError, "Decimal may not begin with a numeric separator");

        // Decimal with right-part with numeric separators
        assert.throws(()=>eval('1.0__0'), SyntaxError, "Decimal right-part may not contain multiple contiguous numeric separators");
        assert.throws(()=>eval('1.0_0__2'), SyntaxError, "Decimal right-part may not contain multiple contiguous numeric separators");
        assert.throws(()=>eval('1._'), SyntaxError, "Decimal right-part may not be a single numeric separator");
        assert.throws(()=>eval('1.__'), SyntaxError, "Decimal right-part may not be multiple numeric separators");
        assert.throws(()=>eval('1._0'), SyntaxError, "Decimal right-part may not begin with a numeric separator");
        assert.throws(()=>eval('1.__0'), SyntaxError, "Decimal right-part may not begin with a numeric separator");
        assert.throws(()=>eval('1.0_'), SyntaxError, "Decimal right-part may not end with a numeric separator");
        assert.throws(()=>eval('1.0__'), SyntaxError, "Decimal right-part may not end with a numeric separator");

        // Decimal with both parts with numeric separators
        assert.throws(()=>eval('1_.0'), SyntaxError, "Decimal left-part may not end in numeric separator");
        assert.throws(()=>eval('1__.0'), SyntaxError, "Decimal left-part may not end in numeric separator");
        assert.throws(()=>eval('1__2.0'), SyntaxError, "Decimal left-part may not contain multiple contiguous numeric separators");

        // Decimal with exponent with numeric separators
        assert.throws(()=>eval('1_e10'), SyntaxError, "Decimal left-part may not end in numeric separator");
        assert.throws(()=>eval('1e_1'), SyntaxError, "Exponent may not begin with numeric separator");
        assert.throws(()=>eval('1e__1'), SyntaxError, "Exponent may not begin with numeric separator");
        assert.throws(()=>eval('1e1_'), SyntaxError, "Exponent may not end with numeric separator");
        assert.throws(()=>eval('1e1__'), SyntaxError, "Exponent may not end with numeric separator");
        assert.throws(()=>eval('1e1__2'), SyntaxError, "Exponent may not contain multiple contiguous numeric separators");

        // Decimal big ints with numeric separators
        assert.throws(()=>eval('1_n'), SyntaxError);
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
