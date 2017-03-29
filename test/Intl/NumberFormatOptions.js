//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Test Valid Options - Formatting Number with Significant Digits",
        body: function () {
            assert.areEqual(new Intl.NumberFormat("en-US", { minimumSignificantDigits: 2, maximumSignificantDigits: 2 }).format(2.131), "2.1", "max: fewer than input");
            assert.areEqual(new Intl.NumberFormat("en-US", { minimumSignificantDigits: 2, maximumSignificantDigits: 3 }).format(2.1), "2.1", "min: same as input");
            assert.areEqual(new Intl.NumberFormat("en-US", { minimumSignificantDigits: 3, maximumSignificantDigits: 3 }).format(2.1), "2.10", "min: more than input");
            assert.areEqual(new Intl.NumberFormat("en-US", { minimumSignificantDigits: 3, maximumSignificantDigits: 3 }).format(123.1), "123", "max: fewer than input - no decimal");

            assert.areEqual(new Intl.NumberFormat("en-US", { minimumSignificantDigits: 3, maximumSignificantDigits: 3, minimumIntegerDigits: 5, minimumFractionDigits: 5, maximumFractionDigits: 5 }).format(123.1), "123",
                "sigfigs(min=max=3); int(min=5); fraction(min=max=5) -- maximumSignificantDigits takes precedence over min digits");

            assert.areEqual(new Intl.NumberFormat("en-US", { minimumIntegerDigits: 5, minimumFractionDigits: 5, maximumFractionDigits: 5 }).format(123.1), "00,123.10000", "int(min=5); fraction(min=max=5)");
            assert.areEqual(new Intl.NumberFormat("en-US", { minimumIntegerDigits: 1, minimumFractionDigits: 1, maximumFractionDigits: 3 }).format(123.14444), "123.144", "int(min=1); fraction(min=1,max=3)");
        }
    },
    {
        name: "Test Invalid Options",
        body: function () {
            function verifyNFException(locale, options, expectingInvalidOption, validValuesStr) {
                try {
                    //Since minute and second aren't supported alone; doing this to prevent that exception.
                    new Intl.NumberFormat(locale, options);
                    assert.fail("Exception was expected. Option: " + expectingInvalidOption + "; options passed in: " + JSON.stringify(options));
                }
                catch (e) {
                    if (!(e instanceof RangeError || e instanceof TypeError)) {
                        assert.fail("Incorrect exception was thrown.");
                    }
                    assert.isTrue(e.message.indexOf(validValuesStr) !== -1,
                        "Exception didn't have the correct valid values when testing option:" + expectingInvalidOption +
                        ".\nMessage: " + e.message +
                        "\nSearched For:" + validValuesStr);
                }
            }

            verifyNFException("en-US", { minimumSignificantDigits: -1 }, "minimumSignificantDigits", "[1 - 21]");
            verifyNFException("en-US", { maximumSignificantDigits: -1 }, "maximumSignificantDigits", "[1 - 21]");
            verifyNFException("en-US", { minimumFractionDigits: -1 }, "minimumFractionDigits", "[0 - 20]");
            verifyNFException("en-US", { maximumFractionDigits: -1 }, "maximumFractionDigits", "[0 - 20]");
            verifyNFException("en-US", { minimumIntegerDigits: -1 }, "minimumIntegerDigits", "[1 - 21]");

            verifyNFException("en-US", { minimumSignificantDigits: 22 }, "minimumSignificantDigits", "[1 - 21]");
            verifyNFException("en-US", { maximumSignificantDigits: 22 }, "maximumSignificantDigits", "[1 - 21]");
            verifyNFException("en-US", { minimumFractionDigits: 21 }, "minimumFractionDigits", "[0 - 20]");
            verifyNFException("en-US", { maximumFractionDigits: 21 }, "maximumFractionDigits", "[0 - 20]");
            verifyNFException("en-US", { minimumIntegerDigits: 22 }, "minimumIntegerDigits", "[1 - 21]");

            verifyNFException("en-US", { minimumSignificantDigits: 5, maximumSignificantDigits: 1 }, "maximumSignificantDigits", "[5 - 21]");
            verifyNFException("en-US", { minimumFractionDigits: 5, maximumFractionDigits: 1 }, "maximumFractionDigits", "[5 - 20]");

            verifyNFException("en-US", { style: "invalid" }, "style", "['decimal', 'percent', 'currency']");
            verifyNFException("en-US", { style: "currency" }, "style", "Currency code was not specified");
            verifyNFException("en-US", { style: "currency", currency: 5 }, "currency", "Currency code '5' is invalid");
            verifyNFException("en-US", { style: "currency", currency: "USD", currencyDisplay: "invalid" }, "currencyDisplay", "['code', 'symbol', 'name']");
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
