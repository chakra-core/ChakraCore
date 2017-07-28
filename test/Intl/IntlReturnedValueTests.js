//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Return values from Intl function calls show the function name correctly in debugger.

/////////////////// DateFormat ////////////////////
var options = { ca: "gregory", hour12: true, timeZone:"UTC" };
var dateFormat1 = new Intl.DateTimeFormat("en-US", options);    /**bp:resume('step_over');locals();**/
WScript.Echo("");  // Dummy line to get desired debugger logging behavior. Required due to the above bug.
var date1 = new Date(2000, 1, 1);

// Bug: GitHub Issue#3439: Intl function call has 2 entries in the debugger functionCallsReturn.
// https://github.com/Microsoft/ChakraCore/issues/3439
// Actual:
    // "functionCallsReturn": {
    //   "[get format returned]": "function <large string>",
    //   "[Intl.DateTimeFormat.prototype.format returned]": "string ‎2‎/‎1‎/‎2000"
// Expected:
    // "functionCallsReturn": {
    //   "[Intl.DateTimeFormat.prototype.format returned]": "string ‎2‎/‎1‎/‎2000"
var date2 = dateFormat1.format(date1);                          /**bp:resume('step_over');locals();**/
WScript.Echo("");  // Dummy line to get desired debugger logging behavior. Required due to the above bug.
var stringDate1 = date1.toLocaleString("en-us");             /**bp:resume('step_over');locals();resume('step_over');locals();resume('step_over');locals();**/
Intl.DateTimeFormat.supportedLocalesOf(["en-US"], { localeMatcher: "best fit" });
dateFormat1.resolvedOptions();
WScript.Echo("");  // Dummy line to get desired debugger logging behavior. Required due to the above bug.

/////////////////// NumberFormat ////////////////////
var numberFormat1 = new Intl.NumberFormat();                    /**bp:resume('step_over');locals();**/
WScript.Echo("");  // Dummy line to get desired debugger logging behavior. Required due to the above bug.
// Bug: GitHub Issue#3439: Intl function call has 2 entries in the debugger functionCallsReturn.
// https://github.com/Microsoft/ChakraCore/issues/3439
// Actual:
    // "functionCallsReturn": {
    //   "[get format returned]": "function <large string>",
    //   "[Intl.DateTimeFormat.prototype.format returned]": "string ‎2‎/‎1‎/‎2000"
// Expected:
    // "functionCallsReturn": {
    //   "[Intl.DateTimeFormat.prototype.format returned]": "string ‎2‎/‎1‎/‎2000"
var formattedNumber1 = numberFormat1.format(123.456);           /**bp:resume('step_over');locals();**/
WScript.Echo("");  // Dummy line to get desired debugger logging behavior. Required due to the above bug.
Intl.NumberFormat.supportedLocalesOf(["en-US"], { localeMatcher: "lookup" });   /**bp:resume('step_over');locals();resume('step_over');locals();**/
numberFormat1.resolvedOptions();   /**bp:locals();resume('step_over');locals();**/
WScript.Echo("");  // Dummy line to get desired debugger logging behavior. Required due to the above bug.

/////////////////// Collator ////////////////////
var collator1 = Intl.Collator();                                /**bp:resume('step_over');locals();resume('step_over');locals();**/
// Bug: GitHub Issue#3439: Intl function call has 2 entries in the debugger functionCallsReturn.
// https://github.com/Microsoft/ChakraCore/issues/3439
// Actual:
    // "functionCallsReturn": {
    //   "[get compare returned]": "function <large string>",
    //   "[Intl.Collator.prototype.compare returned]": "number -1"
// Expected:
    // "functionCallsReturn": {
    //   "[Intl.Collator.prototype.compare returned]": "number -1"
var compare1 = collator1.compare('a', 'b');
WScript.Echo("");  // Dummy line to get desired debugger logging behavior. Required due to the above bug.
Intl.Collator.supportedLocalesOf(["en-US"], { localeMatcher: "best fit" });     /**bp:resume('step_over');locals();resume('step_over');locals();**/
collator1.resolvedOptions();

WScript.Echo("Pass");
