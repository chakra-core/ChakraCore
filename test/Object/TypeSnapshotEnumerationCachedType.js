//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// The following switches are needed to repro the original ExprGen bug.
// -maxinterpretcount:1 -maxsimplejitruncount:1 -off:ArrayCheckHoist
WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Snapshot enumeration works as expected when non-shared type evolves after adding to EquivalentTypeCache.",
        body: function () {
            var expected = {"Foo" : "Bar"};
            function test0() {
                var obj0 = {};
                obj0.prop1 = 1;
                obj0.prop2 = 2;
                obj0.prop3 = 3;
                obj0.prop4 = 4;

                var func2 = function () {
                    arrObj0 = obj0;
                    // Property "prop2" on arrObj0 is deleted.
                    delete arrObj0.prop2;
                    arrObj0[10] = obj0.prop3;
                };

                func2();
                var i = 1;
                for (var val in arrObj0) {
                    if (i > 4) {
                        break;
                    }

                    i++;
                    // As arrObj0.prop2 was deleted in func2() above, we should get to the arrObj0.prop4 property and set it to {} here.
                    arrObj0[val] = expected;

                    // Adding a property named "prop2" during enumeration should NOT change the values seen by the enumeration.
                    obj0.prop2 = "uniqobj19"; // SetPropertyFromDescriptor is called here. This causes TypeHandler to change.
                }

                assert.areEqual(expected, arrObj0.prop4, "Snapshot enumeration broken after non-shared type was added to EquivalentTypeCache.");  
            }

            test0();
            test0();
            test0();
            test0();
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });

