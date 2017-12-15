//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

const tests = [
    {
        name: "Intl.DateTimeFormat.prototype.formatToParts",
        body: function () {
            const date = new Date(2000, 0, 1, 0, 0, 0);
            function test(options, key, value, message) {
                message = message || "error";
                const fmt = new Intl.DateTimeFormat("en-US", options);
                const toString = fmt.format(date);
                const toParts = fmt.formatToParts(date);

                assert.areEqual(toString, toParts.map((part) => part.value).join(""), `${message} - format() and formatToParts() returned incompatible values`);

                if (typeof key === "string") {
                    const part = toParts.find((p) => p.type === key);
                    assert.isTrue(part && typeof part.value === "string", `${message} - ${JSON.stringify(toParts)} expected to have part with type "${key}"`);
                    assert.areEqual(value, part.value, `${message} - expected ${key} to be ${value}, but was actually ${part.value}`);
                } else {
                    assert.areEqual(key.length, value.length, "test called with invalid arguments");
                    key.forEach(function (k, i) {
                        const v = value[i];
                        const part = toParts.find((p) => p.type === k);
                        assert.isTrue(part && typeof part.value === "string", `${message} - ${JSON.stringify(toParts)} expected to have part with type "${k}"`);
                        assert.areEqual(v, part.value, `${message} - expected ${k} to be ${v}, but was actually ${part.value}`);
                    });
                }
                
            }

            test(undefined, ["year", "month", "day"], ["2000", "1", "1"]);
            test({ year: "2-digit", month: "2-digit", day: "2-digit" }, ["year", "month", "day"], ["00", "01", "01"]);
            test({ hour: "numeric", minute: "numeric", second: "numeric" }, ["hour", "minute", "second"], ["12", "00", "00"]);
            test({ month: "long" }, "month", "January");

            // the "literal" tested here is the first of two literals, the second of which is a space between "12" and "AM"
            test({ hour: "numeric", weekday: "long" }, ["weekday", "literal", "hour", "dayPeriod"], ["Saturday", ", ", "12", "AM"]);
        }
    },
    {
        name: "Intl.DateTimeFormat hourCycle option",
        body: function () {
            const midnight = new Date(2000, 0, 1, 0, 0, 0);
            const noon = new Date(2000, 0, 1, 12, 0, 0);
            function test(locale, useHourField, options, expectedHC, expectedHour12) {
                options = useHourField === false ? options : Object.assign({}, { hour: "2-digit" }, options);
                const fmt = new Intl.DateTimeFormat(locale, options);
                const message = `locale: "${locale}", options: ${JSON.stringify(options)}`;
                assert.areEqual(expectedHC, fmt.resolvedOptions().hourCycle, `${message} - hourCycle is not correct`);
                assert.areEqual(expectedHour12, fmt.resolvedOptions().hour12, `${message} - hour12 is not correct`);
                if (useHourField === true) {
                    const expectedNoon = {
                        h11: "00",
                        h12: "12",
                        h23: "12",
                        h24: "12",
                    }[expectedHC];
                    const expectedMidnight = {
                        h11: "00",
                        h12: "12",
                        h23: "00",
                        h24: "24",
                    }[expectedHC];
                    assert.areEqual(expectedMidnight, fmt.formatToParts(midnight).find((p) => p.type === "hour").value, `${message} - midnight value was incorrect`);
                    assert.areEqual(expectedNoon, fmt.formatToParts(noon).find((p) => p.type === "hour").value, `${message} - noon value was incorrect`);
                } else {
                    assert.isUndefined(fmt.formatToParts(midnight).find((p) => p.type === "hour"), `${message} - unexpected hour field`);
                }
            }

            function withoutHour(locale, options) {
                test(locale, false, options, undefined, undefined);
            }

            function withHour(locale, options, expectedHC, expectedHour12) {
                test(locale, true, options, expectedHC, expectedHour12);
            }

            // ensure hourCycle and hour properties are not there when we don't ask for them
            withoutHour("en-US", undefined);
            withoutHour("en-US", { hourCycle: "h11" });
            withoutHour("en-US", { hour12: true });
            withoutHour("en-US", { hourCycle: "h11", hour12: false });
            withoutHour("en-US-u-hc-h24", undefined);
            withoutHour("en-US-u-hc-h24", { hourCycle: "h11" });
            withoutHour("en-US-u-hc-h24", { hour12: true });
            withoutHour("en-US-u-hc-h24", { hourCycle: "h11", hour12: false });

            // ensure hourCycle and hour12 properties along with hour values are correct when we do ask for hour
            withHour("en-US", undefined, "h12", true);
            withHour("en-US", { hour12: false }, "h23", false);
            withHour("en-US", { hourCycle: "h24" }, "h24", false);
            withHour("en-US", { hourCycle: "h24", hour12: true }, "h12", true);
            withHour("en-US", { hourCycle: "h11", hour12: false }, "h23", false);
            withHour("en-US-u-hc-h24", undefined, "h24", false);
            withHour("en-US-u-hc-h24", { hourCycle: "h23" }, "h23", false);
            withHour("en-US-u-hc-h24", { hourCycle: "h11" }, "h11", true);
            withHour("en-US-u-hc-h24", { hour12: false }, "h23", false);
            withHour("en-US-u-hc-h24", { hour12: true }, "h12", true);

            if (Intl.DateTimeFormat.supportedLocalesOf("en-GB").includes("en-GB")) {
                withoutHour("en-GB", undefined);

                withHour("en-GB", undefined, "h23", false);
                withHour("en-GB", { hour12: true }, "h12", true);
                withHour("en-GB-u-hc-h24", undefined, "h24", false);
            }

            if (Intl.DateTimeFormat.supportedLocalesOf("ja-JP").includes("ja-JP")) {
                withoutHour("ja-JP", undefined);

                withHour("ja-JP", undefined, "h23", false);
                withHour("ja-JP", { hour12: true }, "h11", true);
                withHour("ja-JP-u-hc-h12", undefined, "h12", true);
            }
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
