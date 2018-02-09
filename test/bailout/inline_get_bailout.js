//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

if (this.WScript && this.WScript.LoadScriptFile) {
  WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
  WScript.LoadScriptFile("inline_get_bailout_helper.js");
}

let tests = [{
  name: "after inlining a getter function from another source file, we should bail out if the input data changes",
  body: function () {
    const bar = new Bar();

    function getWithGetter() {
      return bar.data;
    }

    let sum = 0;
    sum += getWithGetter();
    sum += getWithGetter();

    Object.defineProperty(bar, "data", { value: 3 });

    sum += getWithGetter();

    assert.areEqual(13, sum);
  }
}];

testRunner.run(tests, { verbose: WScript.Arguments[0] != "summary" });
