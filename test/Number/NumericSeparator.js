//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Decimal literal support",
    body: function () {
        assert.areEqual(1234, 1_234, "1234 === 1_234");
        assert.areEqual(1234, 1_2_3_4, "1234 === 1_2_3_4");
        assert.areEqual(1234.567, 1_2_3_4.5_6_7, "1234.567 === 1_2_3_4.5_6_7");

        assert.areEqual(-1234, -1_2_34, "-1234 === -1_2_34");
        assert.areEqual(-12.34, -1_2.3_4, "-12.34 === -1_2.3_4");
    }
  },
  {
    name: "Decimal literal with exponent",
    body: function () {
        assert.areEqual(1e100, 1e1_00, "1e100 === 1e1_00");
        assert.areEqual(Infinity, 1e1_0_0_0, "Infinity === 1e1_0_0_0");

        assert.areEqual(123.456e23, 1_2_3.4_5_6e2_3, "123.456e23 === 1_2_3.4_5_6e2_3");
        assert.areEqual(123.456e001, 1_2_3.4_5_6e0_0_1, "123.456e001 === 1_2_3.4_5_6e0_0_1");
    }
  },
  {
    name: "Decimal literal bad syntax",
    body: function () {
        // Decimal left-part only with numeric separators
        assert.throws(()=>eval('1__2'), SyntaxError, "Multiple numeric separators in a row are not allowed");
        assert.throws(()=>eval('1_2____3'), SyntaxError, "Multiple numeric separators in a row are not allowed");
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
  {
    name: "Strings parsed as number do not support numeric separators for decimal literals",
    body: function () {
        assert.areEqual(NaN, Number('12_34'), "NaN === Number('12_34')");
        assert.areEqual(NaN, Number('12e3_4'), "NaN === Number('12e3_4')");
        assert.areEqual(NaN, Number('1234.45_67'), "NaN === Number('1234.45_67')");
        assert.areEqual(NaN, Number('1234.45e6_7'), "NaN === Number('1234.45e6_7')");

        assert.areEqual(1, parseInt('1_2'), "1 === parseInt('1_2')");
        assert.areEqual(1, parseInt('1e2_3'), "1 === parseInt('1e2_3')");
        assert.areEqual(12, parseInt('12.3_4'), "1 === parseInt('12.3_4')");
        assert.areEqual(12, parseInt('12.34e5_6'), "1 === parseInt('12.34e5_6')");

        assert.areEqual(1, parseFloat('1_2'), "1 === parseFloat('1_2')");
        assert.areEqual(1e2, parseFloat('1e2_3'), "1 === parseFloat('1e2_3')");
        assert.areEqual(12.3, parseFloat('12.3_4'), "1 === parseFloat('12.3_4')");
        assert.areEqual(12.34e5, parseFloat('12.34e5_6'), "1 === parseFloat('12.34e5_6')");
    }
  },
  {
    name: "Binary literal support",
    body: function () {
        assert.areEqual(0b00, 0b0_0, "0b00 === 0b0_0");
        assert.areEqual(0b11, 0b1_1, "0b11 === 0b1_1");
        assert.areEqual(0b10, 0b1_0, "0b10 === 0b1_0");
        assert.areEqual(0b01, 0b0_1, "0b01 === 0b0_1");
        assert.areEqual(0b0001, 0b000_1, "0b0001 === 0b000_1");
        assert.areEqual(0b0000, 0b000_0, "0b0000 === 0b000_0");
        assert.areEqual(0b000011110000, 0b0000_1111_0000, "0b000011110000 === 0b0000_1111_0000");
        assert.areEqual(0b000011110000, 0b0_0_0_0_1_111_00_00, "0b000011110000 === 0b0_0_0_0_1_111_00_00");
    }
  },
  {
    name: "Binary literal bad syntax",
    body: function () {
        assert.throws(()=>eval('0b_'), SyntaxError, "'_' cannot immediately follow 0b");
        assert.throws(()=>eval('0b__'), SyntaxError, "'_' cannot immediately follow 0b");
        assert.throws(()=>eval('0b_1'), SyntaxError, "Binary literal may not begin with numeric separator");
        assert.throws(()=>eval('0b_0'), SyntaxError, "Binary literal may not begin with numeric separator");
        assert.throws(()=>eval('0b__1'), SyntaxError, "Binary literal may not begin with numeric separator");
        assert.throws(()=>eval('0b__0'), SyntaxError, "Binary literal may not begin with numeric separator");
        assert.throws(()=>eval('0b1_'), SyntaxError, "Binary literal may not end with numeric separator");
        assert.throws(()=>eval('0b0_'), SyntaxError, "Binary literal may not end with numeric separator");
        assert.throws(()=>eval('0b1__'), SyntaxError, "Binary literal may not end with numeric separator");
        assert.throws(()=>eval('0b0__'), SyntaxError, "Binary literal may not end with numeric separator");
        assert.throws(()=>eval('0b1__1'), SyntaxError, "Multiple numeric separator characters may not follow each other");
        assert.throws(()=>eval('0b0__0'), SyntaxError, "Multiple numeric separator characters may not follow each other");
        assert.throws(()=>eval('0b000__1'), SyntaxError, "Multiple numeric separator characters may not follow each other");
        assert.throws(()=>eval('0b000_a'), SyntaxError, "After initial zeroes, a numeric separator followed by an invalid character");
    }
  },
  {
    name: "Strings parsed as number do not support numeric separators for binary literals",
    body: function () {
        assert.areEqual(NaN, Number('0b0_1'), "NaN === Number('0b0_1')");
        assert.areEqual(NaN, Number('0b1_0'), "NaN === Number('0b1_0')");
        assert.areEqual(NaN, Number('0b0_0'), "NaN === Number('0b0_0')");
        assert.areEqual(NaN, Number('0b1_1'), "NaN === Number('0b1_1')");

        assert.areEqual(0, parseInt('0b1_0'), "0 === parseInt('0b1_0')");

        assert.areEqual(0, parseFloat('0b1_0'), "0 === parseFloat('0b1_0')");
    }
  },
  {
    name: "Hex numeric literal support",
    body: function () {
        assert.areEqual(0x00, 0x0_0, "0x00 === 0x0_0");
        assert.areEqual(0x1f, 0x1_f, "0x1f === 0x1_f");
        assert.areEqual(0x000000Ae1, 0x00_0_000_A_e1, "0x000000Ae1 === 0x00_0_000_A_e1");
        assert.areEqual(0xaabbccdd, 0xaa_bb_cc_dd, "0xaabbccdd === 0xaa_bb_cc_dd");
    }
  },
  {
    name: "Hex numeric literal bad syntax",
    body: function () {
        assert.throws(()=>eval('0x_'), SyntaxError, "'_' cannot immediately follow 0x");
        assert.throws(()=>eval('0x__'), SyntaxError, "'_' cannot immediately follow 0x");
        assert.throws(()=>eval('0x_1'), SyntaxError, "Hex literal may not begin with numeric separator");
        assert.throws(()=>eval('0x_0'), SyntaxError, "Hex literal may not begin with numeric separator");
        assert.throws(()=>eval('0x__1'), SyntaxError, "Hex literal may not begin with numeric separator");
        assert.throws(()=>eval('0x__0'), SyntaxError, "Hex literal may not begin with numeric separator");
        assert.throws(()=>eval('0x1_'), SyntaxError, "Hex literal may not end with numeric separator");
        assert.throws(()=>eval('0x0_'), SyntaxError, "Hex literal may not end with numeric separator");
        assert.throws(()=>eval('0x1__'), SyntaxError, "Hex literal may not end with numeric separator");
        assert.throws(()=>eval('0x0__'), SyntaxError, "Hex literal may not end with numeric separator");
        assert.throws(()=>eval('0x1__1'), SyntaxError, "Multiple numeric separator characters may not follow each other");
        assert.throws(()=>eval('0x0__0'), SyntaxError, "Multiple numeric separator characters may not follow each other");
        assert.throws(()=>eval('0x000__1'), SyntaxError, "Multiple numeric separator characters may not follow each other");
        assert.throws(()=>eval('0x000_q'), SyntaxError, "After initial zeroes, a numeric separator followed by an invalid character");
    }
  },
  {
    name: "Strings parsed as number do not support numeric separators for hex literals",
    body: function () {
        assert.areEqual(NaN, Number('0x0_ff'), "NaN === Number('0x0_ff')");
        assert.areEqual(NaN, Number('0xF_F'), "NaN === Number('0xF_F')");

        assert.areEqual(0, parseInt('0x0_ff'), "0 === parseInt('0x0_ff')");
        assert.areEqual(15, parseInt('0xf_00f'), "15 === parseInt('0xf_00f')");

        assert.areEqual(0, parseFloat('0x0_ff'), "0 === parseFloat('0x0_ff')");
        assert.areEqual(0, parseFloat('0x1_fe'), "0 === parseFloat('0x1_fe')");
    }
  },
  {
    name: "Octal literal support",
    body: function () {
        assert.areEqual(0o00, 0o0_0, "0o00 === 0o0_0");
        assert.areEqual(0o11, 0o1_1, "0o11 === 0o1_1");
        assert.areEqual(0o10, 0o1_0, "0o10 === 0o1_0");
        assert.areEqual(0o01, 0o0_1, "0o01 === 0o0_1");
        assert.areEqual(0o0001, 0o000_1, "0o0001 === 0o000_1");
        assert.areEqual(0o0000, 0o000_0, "0o0000 === 0o000_0");
        assert.areEqual(0o000011110000, 0o0000_1111_0000, "0o000011110000 === 0o0000_1111_0000");
        assert.areEqual(0o000011110000, 0o0_0_0_0_1_111_00_00, "0o000011110000 === 0o0_0_0_0_1_111_00_00");
    }
  },
  {
    name: "Octal literal bad syntax",
    body: function () {
        assert.throws(()=>eval('0o_'), SyntaxError, "'_' cannot immediately follow 0o");
        assert.throws(()=>eval('0o__'), SyntaxError, "'_' cannot immediately follow 0o");
        assert.throws(()=>eval('0o_1'), SyntaxError, "Octal literal may not begin with numeric separator");
        assert.throws(()=>eval('0o_0'), SyntaxError, "Octal literal may not begin with numeric separator");
        assert.throws(()=>eval('0o__1'), SyntaxError, "Octal literal may not begin with numeric separator");
        assert.throws(()=>eval('0o__0'), SyntaxError, "Octal literal may not begin with numeric separator");
        assert.throws(()=>eval('0o1_'), SyntaxError, "Octal literal may not end with numeric separator");
        assert.throws(()=>eval('0o0_'), SyntaxError, "Octal literal may not end with numeric separator");
        assert.throws(()=>eval('0o1__'), SyntaxError, "Octal literal may not end with numeric separator");
        assert.throws(()=>eval('0o0__'), SyntaxError, "Octal literal may not end with numeric separator");
        assert.throws(()=>eval('0o1__1'), SyntaxError, "Multiple numeric separator characters may not follow each other");
        assert.throws(()=>eval('0o0__0'), SyntaxError, "Multiple numeric separator characters may not follow each other");
        assert.throws(()=>eval('0o000__1'), SyntaxError, "Multiple numeric separator characters may not follow each other");
        assert.throws(()=>eval('0o000_a'), SyntaxError, "After initial zeroes, a numeric separator followed by an invalid character");
    }
  },
  {
    name: "Strings parsed as number do not support numeric separators for octal literals",
    body: function () {
        assert.areEqual(NaN, Number('0o0_1'), "NaN === Number('0o0_1')");
        assert.areEqual(NaN, Number('0o1_0'), "NaN === Number('0o1_0')");
        assert.areEqual(NaN, Number('0o0_0'), "NaN === Number('0o0_0')");
        assert.areEqual(NaN, Number('0o1_1'), "NaN === Number('0o1_1')");

        assert.areEqual(0, parseInt('0b1_0'), "0 === parseInt('0b1_0')");

        assert.areEqual(0, parseFloat('0b1_0'), "0 === parseFloat('0b1_0')");
    }
  },
  {
    name: "Legacy octal numeric literals do not support numeric separators",
    body: function () {
        assert.throws(()=>eval('0_1'), SyntaxError, "'_' cannot immediately follow 0 in legacy octal numeric literal");
        assert.throws(()=>eval('07_7'), SyntaxError, "Legacy octal numeric literals do not support numeric separator");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
