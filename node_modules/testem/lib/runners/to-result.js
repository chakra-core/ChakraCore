'use strict';

const log = require('npmlog');

module.exports = function toResult(launcherId, err, code, runnerProcess, config, testContext) {
  let logs = [];
  let message = '';
  testContext = testContext ? testContext : {};

  if (err) {
    logs.push({
      type: 'error',
      text: err.toString()
    });

    message += err + '\n';
  }

  if (testContext.name) {
    logs.push({
      type: 'error',
      text: `Error while executing test: ${testContext.name}`
    });

    message += `Error while executing test: ${testContext.name}\n`;
  }

  if (code !== 0) {
    logs.push({
      type: 'error',
      text: 'Non-zero exit code: ' + code
    });

    message += 'Non-zero exit code: ' + code + '\n';
  }

  if (runnerProcess && runnerProcess.stderr) {
    logs.push({
      type: 'error',
      text: runnerProcess.stderr
    });

    message += 'Stderr: \n ' + runnerProcess.stderr + '\n';
  }

  if (runnerProcess && runnerProcess.stdout) {
    logs.push({
      type: 'log',
      text: runnerProcess.stdout
    });

    message += 'Stdout: \n ' + runnerProcess.stdout + '\n';
  }

  if (config && config.get('debug')) {
    log.info(runnerProcess.name + '.stdout', runnerProcess.stdout);
    log.info(runnerProcess.name + '.stderr', runnerProcess.stderr);
  }

  let result = {
    failed: code === 0 && !err ? 0 : 1,
    passed: code === 0 && !err ? 1 : 0,
    name: 'error',
    testContext: testContext,
    launcherId: launcherId,
    logs: logs
  };
  if (!result.passed) {
    result.error = {
      message: message
    };
  }

  return result;
};
