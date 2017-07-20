//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const [testfile, ...options] = WScript.Arguments;
if (!testfile) {
  throw new Error("Missing testfile");
}
const verbose = options.indexOf("-verbose") !== -1;

const self = this;
const setTimeout = WScript.SetTimeout;
const clearTimeout = WScript.ClearTimeout;

WScript.Flag(`-wasmMaxTableSize:${(Math.pow(2,32)-1)|0}`);

self.addEventListener = function() {};

class ConsoleTestEnvironment {
  on_tests_ready() {
    runTests();
    this.all_loaded = true;
  }

  // Invoked after setup() has been called to notify the test environment
  // of changes to the test harness properties.
  on_new_harness_properties(properties) {}

  // Should return a new unique default test name.
  next_default_test_name() {
    return this.nextName = (this.nextName | 0) + 1;
  }

  // Should return the test harness timeout duration in milliseconds.
  test_timeout() {
    return null;
  }

  add_on_loaded_callback() {}

  // Should return the global scope object.
  global_scope() {
    return self;
  }
}

function chakra_create_test_environment() {
  return new ConsoleTestEnvironment();
}

WScript.LoadScriptFile("testsuite/harness/index.js");
WScript.LoadScriptFile("testsuite/harness/wasm-constants.js");
WScript.LoadScriptFile("testsuite/harness/wasm-module-builder.js");
WScript.LoadScript(read("testsuite/harness/testharness.js")
  // Use our text environment
  .replace(" = create_test_environment", " = chakra_create_test_environment"));

function reportResult(tests, harness_status) {
  const status_text_harness = {
    [harness_status.OK]: "OK",
    [harness_status.ERROR]: "Error",
    [harness_status.TIMEOUT]: "Timeout",
  };
  const firstTest = tests[0] || {};
  const status_text = {
    [firstTest.PASS]: "Pass",
    [firstTest.FAIL]: "Fail",
    [firstTest.TIMEOUT]: "Timeout",
    [firstTest.NOTRUN]: "Not Run",
  };
  const status_number = tests.reduce((all, test) => {
    var status = status_text[test.status];
    all[status] = (all[status]|0) + 1;
    return all;
  }, {});
  function get_assertion(test) {
    if (test.properties.hasOwnProperty("assert")) {
      if (Array.isArray(test.properties.assert)) {
        return test.properties.assert.join(" ");
      }
      return test.properties.assert;
    }
    return "";
  }
  const testsReport = tests.map(test => {
    const stack = verbose ? test.stack : "";
    return `${status_text[test.status]} ${test.name} ${get_assertion(test)} ${test.message || ""}${stack ? "\n" + stack : stack}`;
  });
  console.log(`Harness Status: ${status_text_harness[harness_status.status]}
Found ${tests.length} tests: ${Object.keys(status_number).map(key => `${key} = ${status_number[key]}`).join(" ")}
${testsReport.join("\n")}`);
}

function runTests() {
  add_completion_callback((...args) => {
    try {
      reportResult(...args);
    } catch (e) {
      console.log(e);
    }
  });

  WScript.LoadScriptFile(testfile);
}
