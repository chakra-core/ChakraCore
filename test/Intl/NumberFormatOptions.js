//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const tests = [
    {
        name: "Test invalid options",
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
