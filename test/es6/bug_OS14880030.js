//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";
var r = delete this;

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
  {
    name: "Delete this in strict global code",
    body: function () {
         assert.isTrue(r, "We should have returned true from the global delete this above");
    }
  },
  {
    name: "Delete this in strict mode nested function",
    body: function () {
        function test() {
            "use strict";
            return delete this;
        }
        
        assert.isTrue(test(), "Delete this in strict nested function does nothing but returns true");
    }
  },
  {
    name: "Delete new.target in strict mode nested function",
    body: function () {
        function test() {
            "use strict";
            return delete new.target;
        }
        
        assert.isTrue(test(), "Delete new.target in strict nested function does nothing but returns true");
    }
  },
  {
    name: "Delete arguments in strict mode nested function",
    body: function () {
        function test() {
            "use strict";
            try {
                eval('delete arguments;');
            } catch(e) {
                return true;
            }
            return false;
        }
        
        assert.isTrue(test(), "Delete arguments in strict nested function should throw early SyntaxError");
    }
  },
  {
    name: "Delete user identifier in strict mode nested function",
    body: function () {
        function test() {
            "use strict";
            let a = 'a';
            try {
                eval('delete a;');
            } catch(e) {
                return true;
            }
            return false;
        }
        
        assert.isTrue(test(), "Delete user identifier in strict nested function should throw early SyntaxError");
    }
  },
  {
    name: "Delete user identifier in strict eval in strict mode nested function",
    body: function () {
        function test() {
            "use strict";
            try {
                eval(`
                    function test5_eval() {
                        "use strict";
                        let a = 'a';
                        delete a;
                    }
                    test5_eval();
                `);
            } catch(e) {
                return true;
            }
            return false;
        }
        
        assert.isTrue(test(), "Delete user identifier in strict nested function should throw early SyntaxError");
    }
  },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
