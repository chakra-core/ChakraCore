//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
   {
       name: "computed functions",
       body: function ()
       {
           var date = new Date(2011,10,30);
           var sym = Symbol("FAZ");
           var name0 = function() { var a = 1; var b = 2; var c = 3; return a+c+b};
           var name1 = function() { var a = 1; var b = "1"; var c = "b"; return a+b+c};
           var name2 = function() { var a = {}; var b = date; var c = {}; return a+b+c};
           var name3 = function() { return NaN;};
           var name4 = function() { return undefined;}
           var name5 = function() { return null;}
           var name6 = function() { return true;}
           var name7 = function() { return false;}
           var name8 = function() { return sym;}
           var name9 = function() { return date;}
           var qux = class {
               [name0()]() {}
               [name1()]() {}
               [name2()]() {}
               [name3()]() {}
               [name4()]() {}
               [name5()]() {}
               [name6()]() {}
               [name7()]() {}
               [name8()]() {}
               [name9()]() {}
               }
           var quxObj = new qux();
           assert.areEqual("6",           quxObj[name0()].name, "name0() adds 1+2+3, expecting 6");
           assert.areEqual("11b",         quxObj[name1()].name, "name1() returns number 1 plus string 1 and string b should evaluate to 11b");
           assert.areEqual({}+date+{}+"", quxObj[name2()].name, "name2() returns object + date + object should be toStrings of all three");
           assert.areEqual("NaN",         quxObj[name3()].name, "name3() returns NaN, which should be the name");
           assert.areEqual("undefined",   quxObj[name4()].name, "name4() returns undefined for the name");
           assert.areEqual("null",        quxObj[name5()].name, "name5() returns null as the name");
           assert.areEqual("true",        quxObj[name6()].name, "name6() returns true as the name");
           assert.areEqual("false",       quxObj[name7()].name, "name7() returns false as the name");
           //assert.areEqual(sym,         quxObj[name8()].name, "name8() returns false as the name"); // Symbol doesn't play nicely
           assert.areEqual(date+"",       quxObj[name9()].name, "name9() returns a date as the name");

           assert.areEqual("[name0()]() {}",           quxObj[name0()].toString(), "toString should be the uninterpreted original source");
           assert.areEqual("[name1()]() {}",           quxObj[name1()].toString(), "toString should be the uninterpreted original source");
           assert.areEqual("[name2()]() {}",           quxObj[name2()].toString(), "toString should be the uninterpreted original source");
           assert.areEqual("[name3()]() {}",           quxObj[name3()].toString(), "toString should be the uninterpreted original source");
           assert.areEqual("[name4()]() {}",           quxObj[name4()].toString(), "toString should be the uninterpreted original source");
           assert.areEqual("[name5()]() {}",           quxObj[name5()].toString(), "toString should be the uninterpreted original source");
           assert.areEqual("[name6()]() {}",           quxObj[name6()].toString(), "toString should be the uninterpreted original source");
           assert.areEqual("[name7()]() {}",           quxObj[name7()].toString(), "toString should be the uninterpreted original source");
           assert.areEqual("[name8()]() {}",           quxObj[name8()].toString(), "toString should be the uninterpreted original source");
           assert.areEqual("[name9()]() {}",           quxObj[name9()].toString(), "toString should be the uninterpreted original source");
       }
   },
   {
       name: "computed Values",
       body: function ()
       {
           var date = new Date(2011,10,30);
           var a = 4;
           var b = 2;
           var c = "c"
           var qux = class {
                      [1+4]() {}
                      [date]() {}
                      [a+b]() {}
                      [a+c]() {}
                      ["foo"]() {}
                      ["fo\0o"+"bar"]() {}
                    }
           var quxObj = new qux();
           assert.areEqual("5",        quxObj[5].name,     "the result of 1+4 for the name");
           assert.areEqual(date+"",    quxObj[date].name,  "date as the name");
           assert.areEqual("6",        quxObj[6].name,     "the result of 4 + 2 for the name");
           assert.areEqual("4c",       quxObj["4c"].name,  "the result of numeric value 4 plus string c");
           assert.areEqual("foo",      quxObj["foo"].name, "the result of the string foo");
           assert.areEqual("fo\0obar", quxObj["fo\0obar"].name, "the result of the concatenation of two strings");
       }
   },

   ];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
