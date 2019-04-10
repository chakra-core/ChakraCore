'use strict';

const CliError = require('../errors/cli');

const availableExperiments = Object.freeze([
  'PACKAGER',
  'MODULE_UNIFICATION',
  'DELAYED_TRANSPILATION',
  'BROCCOLI_2',
  'SYSTEM_TEMP',
]);

const enabledExperiments = Object.freeze([
  'BROCCOLI_2',
]);

function isExperimentEnabled(experimentName) {
  if (!availableExperiments.includes(experimentName)) {
    return false;
  }

  if (process.env.EMBER_CLI_ENABLE_ALL_EXPERIMENTS) {
    return true;
  }

  let experimentEnvironmentVariable = `EMBER_CLI_${experimentName}`;
  let experimentValue = process.env[experimentEnvironmentVariable];
  if (enabledExperiments.includes(experimentName)) {
    return experimentValue !== 'false';
  } else if (
    experimentName === 'SYSTEM_TEMP' &&
    experimentValue === undefined &&
    isExperimentEnabled('BROCCOLI_2')
  ) {
    return true;
  } else {
    return experimentValue !== undefined && experimentValue !== 'false';
  }
}

// SYSTEM_TEMP can only be used with BROCCOLI_2
if (isExperimentEnabled('SYSTEM_TEMP') && !isExperimentEnabled('BROCCOLI_2')) {
  throw new CliError('EMBER_CLI_SYSTEM_TEMP only works in combination with EMBER_CLI_BROCCOLI_2');
}

module.exports = { isExperimentEnabled };
