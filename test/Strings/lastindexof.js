//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var sevenBitStr = "abcdef";
var eightBitStr = "ÄÜÖABC";
var unicodeStr = "\u0100\u0111\u0112\u0113ab";

var tests = [
  {
    name: "Empty strings",
    body: function () {
      var sevenBitStr = "aabbccddaabbccdd";
      var eightBitStr = "ÄÜÖABCÄÜÖ";
      var unicodeStr = "\u0100\u0111\u0112\u0113abc\u0100\u0111\u0112\u0113";

      assert.areEqual(0, "".lastIndexOf(""),     "Matching \"\" with \"\" at default offset returns 0");
      assert.areEqual(0, "".lastIndexOf("", 0),  "Matching \"\" with \"\" at offset 0 returns 0");
      assert.areEqual(0, "".lastIndexOf("", 1),  "Matching \"\" with \"\" at offset 1 returns 0");
      assert.areEqual(0, "".lastIndexOf("", 2),  "Matching \"\" with \"\" at offset 2 returns 0");
      assert.areEqual(0, "".lastIndexOf("", -1), "Matching \"\" with \"\" at offset -1 returns 0");
      assert.areEqual(0, "".lastIndexOf("", -2), "Matching \"\" with \"\" at offset -2 returns 0");
      
      for (var str of [new String(sevenBitStr), new String(eightBitStr), new String(unicodeStr)]) {
        assert.areEqual(str.length, str.lastIndexOf(""), "Matching \"" + str + "\" with \"\" at default offset returns the length of the string");
        for (var i = 0; i < str.length; ++i) {
          assert.areEqual(i, str.lastIndexOf("", i), "Matching \"" + str + "\" with \"\" at offset " + i + " returns the length of the string");
        }
      }
    }
  },
  {
    name: "Negative matches",
    body: function () {
      for (var str of [new String(sevenBitStr), new String(eightBitStr), new String(unicodeStr)]) {
        var doubleStr = str + str;
        assert.areEqual(-1, str.lastIndexOf(doubleStr),  "Matching \"" + str + "\" with \"" + doubleStr + "\" at default offset returns -1");
        
        for (var match of ["z", "zz", "zå", "åå", "å"]) {
          assert.areEqual(-1, str.lastIndexOf(match), "Matching \"" + str + "\" with \"" + match + "\" at default offset returns -1");
          for (var i = 0; i < str.length; ++i) {
            assert.areEqual(-1, str.lastIndexOf("z", i), "Matching \"" + str + "\" with \"" + match + "\" at offset " + i + " returns -1");
          }
        }
      }
    }
  },
  {
    name: "Single char matching",
    body: function () {
      for (var str of [new String(sevenBitStr), new String(eightBitStr), new String(unicodeStr)]) {
        for (var i = 0; i < str.length; ++i) {
          var c = str[i];

          if (i > 0) {
            assert.areEqual(-1, str.lastIndexOf(c, -2), "Matching \"" + str + "\" with \"" + c + "\" at offset -2 returns -1");
            assert.areEqual(-1, str.lastIndexOf(c, -1), "Matching \"" + str + "\" with \"" + c + "\" at offset -1 returns -1");
          }

          assert.areEqual(i, str.lastIndexOf(c), "Matching \"" + str + "\" with \"" + c + "\" at default offset returns " + i);

          assert.areEqual(i, str.lastIndexOf(c, str.length - 1), "Matching \"" + str + "\" with \"" + c + "\" at default offset returns " + i);
          assert.areEqual(i, str.lastIndexOf(c, str.length),     "Matching \"" + str + "\" with \"" + c + "\" at default offset returns " + i);
          assert.areEqual(i, str.lastIndexOf(c, str.length + 1), "Matching \"" + str + "\" with \"" + c + "\" at default offset returns " + i);

          // Match character at each possible position in the string. If we give an offset before the position of the character, we expect -1.
          for (var j = 0; j < str.length; ++j) {
            var expected = (pos) => pos < i ? -1 : i;
            assert.areEqual(expected(j), str.lastIndexOf(c, j), "Matching \"" + str + "\" with \"" + c + "\" at offset " + j + " returns " + expected(i));
          }
        }
      }
    }
  },
  {
    name: "Substring matching",
    body: function () {

      for (var str of [new String(sevenBitStr), new String(eightBitStr), new String(unicodeStr)]) {
        for (var i = 0; i < str.length; ++i) {
          var match = str.substring(i);
          assert.areEqual(i, str.lastIndexOf(match),                 "Matching \"" + str + "\" with \"" + match + "\" at default offset returns " + i);
          assert.areEqual(i, str.lastIndexOf(match, i),              "Matching \"" + str + "\" with \"" + match + "\" at offset " + i + " returns " + i);
          assert.areEqual(i, str.lastIndexOf(match, i + 1),          "Matching \"" + str + "\" with \"" + match + "\" at offset " + i + " + 1 returns " + i);
          assert.areEqual(i, str.lastIndexOf(match, i + 2),          "Matching \"" + str + "\" with \"" + match + "\" at offset " + i + " + 2 returns " + i);
          assert.areEqual(i, str.lastIndexOf(match, str.length - 1), "Matching \"" + str + "\" with \"" + match + "\" at offset " + (str.length - 1) + " returns " + i);
          assert.areEqual(i, str.lastIndexOf(match, str.length),     "Matching \"" + str + "\" with \"" + match + "\" at offset " + str.length + " returns " + i);
          assert.areEqual(i, str.lastIndexOf(match, str.length + 1), "Matching \"" + str + "\" with \"" + match + "\" at offset " + (str.length + 1) + " returns " + i);

          if (i > 0) {
            assert.areEqual(-1, str.lastIndexOf(match, i - 1), "Matching \"" + str + "\" with \"" + match + "\" at offset " + i + " - 1 returns -1");
          }
          if (i > 1) {
            assert.areEqual(-1, str.lastIndexOf(match, i - 2), "Matching \"" + str + "\" with \"" + match + "\" at offset " + i + " - 2 returns -1");
          }
        }
      }
    }
  }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });