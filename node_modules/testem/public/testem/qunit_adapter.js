/*

qunit_adapter.js
================

Testem's QUnit adapter. Works by using QUnit's hooks:

* `testStart`
* `testDone`
* `moduleStart`
* `moduleEnd`
* `done`
* `log`

*/

/* globals QUnit, emit */
/* exported qunitAdapter */
'use strict';

function qunitAdapter() {

  var results = {
    failed: 0,
    passed: 0,
    skipped: 0,
    total: 0,
    tests: []
  };
  var currentTest;
  var id = 1;

  function lineNumber(e) {
    return e.line || e.lineNumber;
  }

  function sourceFile(e) {
    return e.sourceURL || e.fileName;
  }

  function message(e) {
    var msg = (e.name && e.message) ? (e.name + ': ' + e.message) : e.toString();
    return msg;
  }

  function stacktrace(e) {
    if (e.stack) {
      return e.stack;
    }
    return undefined;
  }

  QUnit.log(function(params, e) {
    if (e) {
      currentTest.items.push({
        passed: params.result,
        line: lineNumber(e),
        file: sourceFile(e),
        stack: stacktrace(e) || params.source,
        message: message(e)
      });
    } else {
      if (params.result) {
        currentTest.items.push({
          passed: params.result,
          message: params.message
        });
      } else {
        currentTest.items.push({
          passed: params.result,
          actual: params.actual,
          expected: params.expected,
          stack: params.source,
          message: params.message,
          negative: params.negative
        });
      }

    }

  });
  QUnit.testStart(function(params) {
    currentTest = {
      id: id++,
      name: (params.module ? params.module + ': ' : '') + params.name,
      items: []
    };
    emit('tests-start', currentTest);
  });
  QUnit.testDone(function(params) {
    currentTest.failed = params.failed;
    currentTest.passed = params.passed;
    currentTest.skipped = params.skipped;
    currentTest.total = params.total;
    currentTest.runDuration = params.runtime;
    currentTest.testId = params.testId;

    results.total++;

    if (currentTest.skipped) {
      results.skipped++;
    } else if (currentTest.failed > 0) {
      results.failed++;
    } else {
      results.passed++;
    }

    results.tests.push(currentTest);

    emit('test-result', currentTest);
  });
  QUnit.done(function(params) {
    results.runDuration = params.runtime;
    emit('all-test-results');
  });

}
