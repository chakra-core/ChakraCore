//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
WScript.LoadScriptFile("..\\Common\\SpecialCasings.js");

var tests = [
    {
        name: "Edge cases",
        body() {
            assert.areEqual("\uDC37", "\uDC37".toUpperCase(), "Invalid unicode should be passed over (single character, toUpperCase)");
            assert.areEqual("\uDC37", "\uDC37".toLowerCase(), "Invalid unicode should be passed over (single character, toLowerCase)");
            assert.areEqual("ABC\uDC37DEF", "abc\uDC37def".toUpperCase(), "Invalid unicode should be passed over (mid-string, toUpperCase)");
            assert.areEqual("abc\uDC37def", "ABC\uDC37DEF".toLowerCase(), "Invalid unicode should be passed over (mid-string, toLowerCase)");
        }
    },
    {
        name: "SpecialCasing.txt",
        body() {
            const hasICU = WScript.Platform.INTL_LIBRARY === "icu";

            for (const sc of SpecialCasings) {
                // Only test length-changing strings when ICU is enabled because LCMapStringEx and PAL_tow*
                // don't handle expanding strings correctly
                if (sc.code.length === sc.lower.length || hasICU) {
                    assert.areEqual(sc.lower, sc.code.toLowerCase(), `${sc.name} - code.toLowerCase === lower`);
                }

                if (sc.code.length === sc.upper.length || hasICU) {
                    assert.areEqual(sc.upper, sc.code.toUpperCase(), `${sc.name} - code.toUpperCase === upper`);
                }
            }
        }
    },
    {
        name: "Deseret alphabet toUpperCase",
        body: function () {
            assert.areEqual("\uD801\uDC00", "\uD801\uDC28".toUpperCase(), "Expecting Deseret alphabet upper-case long I");
            assert.areEqual("\uD801\uDC01", "\uD801\uDC29".toUpperCase(), "Expecting Deseret alphabet upper-case long E");
            assert.areEqual("\uD801\uDC02", "\uD801\uDC2A".toUpperCase(), "Expecting Deseret alphabet upper-case long A");
            assert.areEqual("\uD801\uDC03", "\uD801\uDC2B".toUpperCase(), "Expecting Deseret alphabet upper-case long Ah");
            assert.areEqual("\uD801\uDC04", "\uD801\uDC2C".toUpperCase(), "Expecting Deseret alphabet upper-case long O");
            assert.areEqual("\uD801\uDC05", "\uD801\uDC2D".toUpperCase(), "Expecting Deseret alphabet upper-case long Oo");
            assert.areEqual("\uD801\uDC06", "\uD801\uDC2E".toUpperCase(), "Expecting Deseret alphabet upper-case short I");
            assert.areEqual("\uD801\uDC07", "\uD801\uDC2F".toUpperCase(), "Expecting Deseret alphabet upper-case short E");
            assert.areEqual("\uD801\uDC08", "\uD801\uDC30".toUpperCase(), "Expecting Deseret alphabet upper-case short A");
            assert.areEqual("\uD801\uDC09", "\uD801\uDC31".toUpperCase(), "Expecting Deseret alphabet upper-case short Ah");
            assert.areEqual("\uD801\uDC0A", "\uD801\uDC32".toUpperCase(), "Expecting Deseret alphabet upper-case short O");
            assert.areEqual("\uD801\uDC0B", "\uD801\uDC33".toUpperCase(), "Expecting Deseret alphabet upper-case short Oo");
            assert.areEqual("\uD801\uDC0C", "\uD801\uDC34".toUpperCase(), "Expecting Deseret alphabet upper-case Ay");
            assert.areEqual("\uD801\uDC0D", "\uD801\uDC35".toUpperCase(), "Expecting Deseret alphabet upper-case Ow");
            assert.areEqual("\uD801\uDC0E", "\uD801\uDC36".toUpperCase(), "Expecting Deseret alphabet upper-case Wu");
            assert.areEqual("\uD801\uDC0F", "\uD801\uDC37".toUpperCase(), "Expecting Deseret alphabet upper-case Yee");
            assert.areEqual("\uD801\uDC10", "\uD801\uDC38".toUpperCase(), "Expecting Deseret alphabet upper-case H");
            assert.areEqual("\uD801\uDC11", "\uD801\uDC39".toUpperCase(), "Expecting Deseret alphabet upper-case Pee");
            assert.areEqual("\uD801\uDC12", "\uD801\uDC3A".toUpperCase(), "Expecting Deseret alphabet upper-case Bee");
            assert.areEqual("\uD801\uDC13", "\uD801\uDC3B".toUpperCase(), "Expecting Deseret alphabet upper-case Tee");
            assert.areEqual("\uD801\uDC14", "\uD801\uDC3C".toUpperCase(), "Expecting Deseret alphabet upper-case Dee");
            assert.areEqual("\uD801\uDC15", "\uD801\uDC3D".toUpperCase(), "Expecting Deseret alphabet upper-case Chee");
            assert.areEqual("\uD801\uDC16", "\uD801\uDC3E".toUpperCase(), "Expecting Deseret alphabet upper-case Jee");
            assert.areEqual("\uD801\uDC17", "\uD801\uDC3F".toUpperCase(), "Expecting Deseret alphabet upper-case Kay");
            assert.areEqual("\uD801\uDC18", "\uD801\uDC40".toUpperCase(), "Expecting Deseret alphabet upper-case Gay");
            assert.areEqual("\uD801\uDC19", "\uD801\uDC41".toUpperCase(), "Expecting Deseret alphabet upper-case Ef");
            assert.areEqual("\uD801\uDC1A", "\uD801\uDC42".toUpperCase(), "Expecting Deseret alphabet upper-case Vee");
            assert.areEqual("\uD801\uDC1B", "\uD801\uDC43".toUpperCase(), "Expecting Deseret alphabet upper-case Eth");
            assert.areEqual("\uD801\uDC1C", "\uD801\uDC44".toUpperCase(), "Expecting Deseret alphabet upper-case Thee");
            assert.areEqual("\uD801\uDC1D", "\uD801\uDC45".toUpperCase(), "Expecting Deseret alphabet upper-case Es");
            assert.areEqual("\uD801\uDC1E", "\uD801\uDC46".toUpperCase(), "Expecting Deseret alphabet upper-case Zee");
            assert.areEqual("\uD801\uDC1F", "\uD801\uDC47".toUpperCase(), "Expecting Deseret alphabet upper-case Esh");
            assert.areEqual("\uD801\uDC20", "\uD801\uDC48".toUpperCase(), "Expecting Deseret alphabet upper-case Zhee");
            assert.areEqual("\uD801\uDC21", "\uD801\uDC49".toUpperCase(), "Expecting Deseret alphabet upper-case Er");
            assert.areEqual("\uD801\uDC22", "\uD801\uDC4A".toUpperCase(), "Expecting Deseret alphabet upper-case El");
            assert.areEqual("\uD801\uDC23", "\uD801\uDC4B".toUpperCase(), "Expecting Deseret alphabet upper-case Em");
            assert.areEqual("\uD801\uDC24", "\uD801\uDC4C".toUpperCase(), "Expecting Deseret alphabet upper-case En");
            assert.areEqual("\uD801\uDC25", "\uD801\uDC4D".toUpperCase(), "Expecting Deseret alphabet upper-case Eng");
            assert.areEqual("\uD801\uDC26", "\uD801\uDC4E".toUpperCase(), "Expecting Deseret alphabet upper-case Oi");
            assert.areEqual("\uD801\uDC27", "\uD801\uDC4F".toUpperCase(), "Expecting Deseret alphabet upper-case Ew");
        }
    },
    {
        name: "Deseret alphabet toLowerCase",
        body: function () {
            assert.areEqual("\uD801\uDC28", "\uD801\uDC00".toLowerCase(), "Expecting Deseret alphabet lower-case long I");
            assert.areEqual("\uD801\uDC29", "\uD801\uDC01".toLowerCase(), "Expecting Deseret alphabet lower-case long E");
            assert.areEqual("\uD801\uDC2A", "\uD801\uDC02".toLowerCase(), "Expecting Deseret alphabet lower-case long A");
            assert.areEqual("\uD801\uDC2B", "\uD801\uDC03".toLowerCase(), "Expecting Deseret alphabet lower-case long Ah");
            assert.areEqual("\uD801\uDC2C", "\uD801\uDC04".toLowerCase(), "Expecting Deseret alphabet lower-case long O");
            assert.areEqual("\uD801\uDC2D", "\uD801\uDC05".toLowerCase(), "Expecting Deseret alphabet lower-case long Oo");
            assert.areEqual("\uD801\uDC2E", "\uD801\uDC06".toLowerCase(), "Expecting Deseret alphabet lower-case short I");
            assert.areEqual("\uD801\uDC2F", "\uD801\uDC07".toLowerCase(), "Expecting Deseret alphabet lower-case short E");
            assert.areEqual("\uD801\uDC30", "\uD801\uDC08".toLowerCase(), "Expecting Deseret alphabet lower-case short A");
            assert.areEqual("\uD801\uDC31", "\uD801\uDC09".toLowerCase(), "Expecting Deseret alphabet lower-case short Ah");
            assert.areEqual("\uD801\uDC32", "\uD801\uDC0A".toLowerCase(), "Expecting Deseret alphabet lower-case short O");
            assert.areEqual("\uD801\uDC33", "\uD801\uDC0B".toLowerCase(), "Expecting Deseret alphabet lower-case short Oo");
            assert.areEqual("\uD801\uDC34", "\uD801\uDC0C".toLowerCase(), "Expecting Deseret alphabet lower-case Ay");
            assert.areEqual("\uD801\uDC35", "\uD801\uDC0D".toLowerCase(), "Expecting Deseret alphabet lower-case Ow");
            assert.areEqual("\uD801\uDC36", "\uD801\uDC0E".toLowerCase(), "Expecting Deseret alphabet lower-case Wu");
            assert.areEqual("\uD801\uDC37", "\uD801\uDC0F".toLowerCase(), "Expecting Deseret alphabet lower-case Yee");
            assert.areEqual("\uD801\uDC38", "\uD801\uDC10".toLowerCase(), "Expecting Deseret alphabet lower-case H");
            assert.areEqual("\uD801\uDC39", "\uD801\uDC11".toLowerCase(), "Expecting Deseret alphabet lower-case Pee");
            assert.areEqual("\uD801\uDC3A", "\uD801\uDC12".toLowerCase(), "Expecting Deseret alphabet lower-case Bee");
            assert.areEqual("\uD801\uDC3B", "\uD801\uDC13".toLowerCase(), "Expecting Deseret alphabet lower-case Tee");
            assert.areEqual("\uD801\uDC3C", "\uD801\uDC14".toLowerCase(), "Expecting Deseret alphabet lower-case Dee");
            assert.areEqual("\uD801\uDC3D", "\uD801\uDC15".toLowerCase(), "Expecting Deseret alphabet lower-case Chee");
            assert.areEqual("\uD801\uDC3E", "\uD801\uDC16".toLowerCase(), "Expecting Deseret alphabet lower-case Jee");
            assert.areEqual("\uD801\uDC3F", "\uD801\uDC17".toLowerCase(), "Expecting Deseret alphabet lower-case Kay");
            assert.areEqual("\uD801\uDC40", "\uD801\uDC18".toLowerCase(), "Expecting Deseret alphabet lower-case Gay");
            assert.areEqual("\uD801\uDC41", "\uD801\uDC19".toLowerCase(), "Expecting Deseret alphabet lower-case Ef");
            assert.areEqual("\uD801\uDC42", "\uD801\uDC1A".toLowerCase(), "Expecting Deseret alphabet lower-case Vee");
            assert.areEqual("\uD801\uDC43", "\uD801\uDC1B".toLowerCase(), "Expecting Deseret alphabet lower-case Eth");
            assert.areEqual("\uD801\uDC44", "\uD801\uDC1C".toLowerCase(), "Expecting Deseret alphabet lower-case Thee");
            assert.areEqual("\uD801\uDC45", "\uD801\uDC1D".toLowerCase(), "Expecting Deseret alphabet lower-case Es");
            assert.areEqual("\uD801\uDC46", "\uD801\uDC1E".toLowerCase(), "Expecting Deseret alphabet lower-case Zee");
            assert.areEqual("\uD801\uDC47", "\uD801\uDC1F".toLowerCase(), "Expecting Deseret alphabet lower-case Esh");
            assert.areEqual("\uD801\uDC48", "\uD801\uDC20".toLowerCase(), "Expecting Deseret alphabet lower-case Zhee");
            assert.areEqual("\uD801\uDC49", "\uD801\uDC21".toLowerCase(), "Expecting Deseret alphabet lower-case Er");
            assert.areEqual("\uD801\uDC4A", "\uD801\uDC22".toLowerCase(), "Expecting Deseret alphabet lower-case El");
            assert.areEqual("\uD801\uDC4B", "\uD801\uDC23".toLowerCase(), "Expecting Deseret alphabet lower-case Em");
            assert.areEqual("\uD801\uDC4C", "\uD801\uDC24".toLowerCase(), "Expecting Deseret alphabet lower-case En");
            assert.areEqual("\uD801\uDC4D", "\uD801\uDC25".toLowerCase(), "Expecting Deseret alphabet lower-case Eng");
            assert.areEqual("\uD801\uDC4E", "\uD801\uDC26".toLowerCase(), "Expecting Deseret alphabet lower-case Oi");
            assert.areEqual("\uD801\uDC4F", "\uD801\uDC27".toLowerCase(), "Expecting Deseret alphabet lower-case Ew");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
