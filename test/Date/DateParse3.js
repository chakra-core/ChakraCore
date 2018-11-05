//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Test non-ISO format with milliseconds
// using colon as millisecond separator is not allowed
runTest("2011-11-08 19:48:43:", "2011-11-08T19:48:43.000");  // valid, last colon is ignored
runTest("2011-11-08 19:48:43:1", null);
runTest("2011-11-08 19:48:43:10", null);
runTest("2011-11-08 19:48:43:100", null);

// use dot as millisecond separator
runTest("2011-11-08 19:48:43.", "2011-11-08T19:48:43.000");
runTest("2011-11-08 19:48:43.1", "2011-11-08T19:48:43.100");
runTest("2011-11-08 19:48:43.1 ", "2011-11-08T19:48:43.100");
runTest("2011-11-08 19:48:43. 1", "2011-11-08T19:48:43.100");
runTest("2011-11-08 19:48:43.01", "2011-11-08T19:48:43.010");
runTest("2011-11-08 19:48:43.001", "2011-11-08T19:48:43.001");
runTest("2011-11-08 19:48:43.0001", "2011-11-08T19:48:43.000");
runTest("2011-11-08 19:48:43.00000001", null);  // having more than 7 consecutive digits causes overflow
runTest("2011-11-08 19:48:43.10", "2011-11-08T19:48:43.100");
runTest("2011-11-08 19:48:43.100", "2011-11-08T19:48:43.100");
runTest("2011-11-08 19:48:43.1000", "2011-11-08T19:48:43.100");
runTest("2011-11-08 19:48:43.12345", "2011-11-08T19:48:43.123");

// previously the '+' or '-' would be skipped and the offset interpreted as a time
runTest("2011-11-08+01:00", null);
runTest("2011-11-08-01:00", null);

function runTest(dateToTest, isoDate)
{
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
