'use strict';

const getConfig = require('./get-config');

let _hasWarnedLegacyViz = false;

function vizEnabled() {
  let isEnabled = process.env.BROCCOLI_VIZ === '1';
  let isLegacyEnabled = !!process.env.BROCCOLI_VIZ && !isEnabled;

  if (isLegacyEnabled && !_hasWarnedLegacyViz) {
    // TODO: this.ui
    console.warn(`Please set BROCCOLI_VIZ=1 to enable visual instrumentation, rather than '${process.env.BROCCOLI_VIZ}'`);
    _hasWarnedLegacyViz = true;
  }

  return isEnabled || isLegacyEnabled;
}

function isInstrumentationConfigEnabled(configOverride) {
  let config = getConfig(configOverride);
  return !!config.get('enableInstrumentation');
}

function instrumentationEnabled(config) {
  return vizEnabled() || process.env.EMBER_CLI_INSTRUMENTATION === '1' || isInstrumentationConfigEnabled(config);
}


module.exports = {
  vizEnabled,
  instrumentationEnabled,
};
