//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/// <reference path="../UnitTestFramework/UnitTestFramework.js" />
if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

initializeGenerateDateStrings();
var yearDigits = 0, monthDigits = 0, dayDigits = 0, hourMinuteDigits = 0, secondDigits = 0, millisecondDigits = 0;
var tests = [{
    name: "Test if valid ISO date strings are parsed correctly",
    body: function () {
        for (yearDigits = 4; yearDigits <= 6; yearDigits += 2) {
            dayDigits = monthDigits = 0;
            runGenerateTestWithValidTime();
            monthDigits = 2;
            runGenerateTestWithValidTime();
            dayDigits = 2;
            runGenerateTestWithValidTime();
        }
    }
}];
function runGenerateTestWithValidTime() {
    millisecondDigits = secondDigits = hourMinuteDigits = 0;
    runGenerateTest();
    hourMinuteDigits = 2;
    runGenerateTest();
    secondDigits = 2;
    runGenerateTest();
    millisecondDigits = 3;
    runGenerateTest();
}

testRunner.run(tests, { verbose: WScript.Arguments[0] != "summary" });
// Test-specific helpers
function runTest(testString) {
    var dateObj = new Date(testString);
    var timeValue1;
    try {
        timeValue1 = dateObj.getTime();
    } catch (ex) {
        assert.fail("Unable to parse Date \"" + testString + "\". Error is " + ex.message);
    }
    assert.isFalse(isNaN(timeValue1), "Unable to parse Date \"" + testString + "\"");
    var ISOString = dateObj.toISOString();
    console.log(ISOString);
    var timeValue2;
    try {
        timeValue2 = new Date(ISOString).getTime();
    } catch (ex) {
        assert.fail("Unable to parse Date \"" + ISOString + "\". Error is " + ex.message);
    }
    assert.areEqual(timeValue1, timeValue2, "Date \"" + testString + "\" is not parsed correctly.");
}

function runGenerateTest() {
    var s =
        generateDateStrings(
            yearDigits,
            monthDigits,
            dayDigits,
            hourMinuteDigits,
            hourMinuteDigits,
            secondDigits,
            millisecondDigits);
    for (var i = 0; i < s.length; ++i)
        runTest(s[i]);
}

var signs, zones;
function initializeGenerateDateStrings() {
    signs = ["+", "-"];
    zones = ["", "Z"];
    var zoneDigitCombinations = ["00", "01", "10"];
    for (var i = 0; i < zoneDigitCombinations.length; ++i)
        for (var j = 0; j < zoneDigitCombinations.length; ++j)
            for (var k = 0; k < signs.length; ++k)
                zones.push(signs[k] + zoneDigitCombinations[i] + ":" + zoneDigitCombinations[j]);
}

// Generates date strings in the following format:
//     date format: "[+|-]YYYYYY[-MM[-DD]]"
//     separator:   "T| "
//     time format: "HH:mm[:ss[.sss]]"
//     time zone:   "Z|(+|-)HH:mm"
// - The separator is required only if both the date and time portions are included in the string.
// - Zero-padding is optional
// - Positive sign (+) is optional when the year is nonnegative
// - Negative sign (-) is optional when the year is zero
// - Time zone is optional
// 
// The function will return an array of strings to test against, based on the parameters.
function generateDateStrings(
    yearDigits,         // number of digits to include for the year (0-6), 0 - exclude the year (monthDigits must also be 0)
    monthDigits,        // number of digits to include for the month (0-2), 0 - exclude the month (dayDigits must also be 0)
    dayDigits,          // number of digits to include for the day (0-2), 0 - exclude the day
    hourDigits,         // number of digits to include for the hour (0-2), 0 - exclude the hour (minuteDigits must also be 0)
    minuteDigits,       // number of digits to include for the minute (0-2), 0 - exclude the minute (hourDigits and secondDigits must also be 0)
    secondDigits,       // number of digits to include for the second (0-2), 0 - exclude the second (millisecondDigits must also be 0)
    millisecondDigits)  // number of digits to include for the millisecond (0-3), 0 - exclude the millisecond
{
    if (yearDigits === 0 && monthDigits !== 0 ||
        monthDigits === 0 && dayDigits !== 0 ||
        hourDigits === 0 && minuteDigits !== 0 ||
        minuteDigits === 0 && (hourDigits !== 0 || secondDigits !== 0) ||
        secondDigits === 0 && millisecondDigits !== 0 ||
        yearDigits === 0 && (hourDigits === 0 || minuteDigits === 0))
        return [];

    var s = [""];

    if (yearDigits !== 0) {
        appendDigits(s, yearDigits, true);
        if (monthDigits !== 0) {
            append(s, ["-"]);
            appendDigits(s, monthDigits, false);
            if (dayDigits !== 0) {
                append(s, ["-"]);
                appendDigits(s, dayDigits, false);
            }
        }
    }

    if (hourDigits !== 0 && minuteDigits !== 0) {
        append(s, ["T"]);
        appendDigits(s, hourDigits, true);
        append(s, [":"]);
        appendDigits(s, minuteDigits, true);
        if (secondDigits !== 0) {
            append(s, [":"]);
            appendDigits(s, secondDigits, true);
            if (millisecondDigits !== 0) {
                append(s, ["."]);
                appendDigits(s, millisecondDigits, true);
            }
        }
    }

    if (yearDigits !== 0 && hourDigits !== 0 && minuteDigits !== 0)
        s = applyToEach(s, zones, function (str, zone) { return str + zone; });
    if (yearDigits === 6) {
        s =
            applyToEach(
                s,
                signs,
                function (str, sign) {
                    if (sign === "-" && str.length >= 6 && str.substring(0, 6) === "000000")
                        return undefined; // "-000000" is not allowed
                    return sign + str;
                });
    }

    return s;
}

// Appends interesting combinations of n digits to the string array
function appendDigits(a, n, includeZero) {
    var d = [];
    switch (n) {
        case 0:
            break;

        case 1:
            if (includeZero)
                d.push("0");
            d.push("1");
            append(a, d);
            break;

        case 3:
        case 6:
            if (n === 3)
                d.push("010");
            else
                d.push("010010");

        default:
            var z = zeroes(n - 1);
            if (includeZero)
                d.push(z + "0");
            d.push(z + "1");
            d.push("1" + z);
            append(a, d);
            break;
    }
}

// Returns a string of n zeroes
function zeroes(n) {
    var s = "";
    while (n-- > 0)
        s += "0";
    return s;
}

// Appends patterns to the string array. The array is extended to acommodate the number of patterns, and the patterns are
// repeated to acommodate the length of the array.
function append(a, p) {
    extend(a, p.length);
    for (var i = 0; i < a.length; ++i)
        a[i] += p[i % p.length];
}

// Applies the function 'f' to each combination of elements in 'a' and 'p'. 'f' will receive the element of 'a' on which it
// should apply the pattern from 'p' and it should return the modified string. The string returned by 'f' will be pushed onto a
// new array, which will be returned.
function applyToEach(a, p, f) {
    var a2 = [];
    for (var i = 0; i < a.length; ++i) {
        for (var j = 0; j < p.length; ++j) {
            var transformed = f(a[i], p[j]);
            if (transformed !== undefined)
                a2.push(transformed);
        }
    }
    return a2;
}

// Extends an array to have length n, by copying the last element as necessary
function extend(a, n) {
    var originalLength = a.length;
    for (var i = originalLength; i < n; ++i)
        a.push(a[originalLength - 1]);
}
