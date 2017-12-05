//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// A two digit year in the format mm/dd/yy is interpreted as a 21st century year if it is less than 50;
// otherwise it's interpreted as a 20th century year.
// See: https://github.com/Microsoft/ChakraCore/pull/4062

// 21st century years:
runTest("01/01/00", "2000-01-01T00:00:00");
runTest("01/01/00 00:00:01 am", "2000-01-01T00:00:01");
runTest("01/01/00 00:00:01 am EST", "2000-01-01T00:00:01-05:00");
runTest("11/14/17", "2017-11-14T00:00:00");
runTest("10/26/49", "2049-10-26T00:00:00");
runTest("12/31/49 11:59:59 pm", "2049-12-31T23:59:59");
runTest("12/31/49 11:59:59 pm PST", "2049-12-31T23:59:59-08:00");

// 20st century years:
runTest("01/01/50", "1950-01-01T00:00:00");
runTest("01/01/50 01:34:59", "1950-01-01T01:34:59");
runTest("09/27/70", "1970-09-27T00:00:00");
runTest("12/31/99", "1999-12-31T00:00:00");
runTest("12/31/99 11:59:59 p.m.", "1999-12-31T23:59:59");
runTest("12/31/99 11:59:59 p.m. UTC", "1999-12-31T23:59:59Z");

function runTest(dateToTest, isoDate) {
    if (isoDate === null) {
        if (isNaN(Date.parse(dateToTest))) {
            console.log("PASS");
        } else {
            console.log("Wrong date parsing result: Date.parse(\"" + dateToTest + "\") should return NaN");
        }
    } else {
        if (Date.parse(dateToTest) === Date.parse(isoDate)) {
            console.log("PASS");
        } else {
            console.log("Wrong date parsing result: Date.parse(\"" + dateToTest + "\") should equal Date.parse(\"" + isoDate + "\")");
        }
    }
}
