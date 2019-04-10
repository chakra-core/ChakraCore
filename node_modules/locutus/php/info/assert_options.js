'use strict';

module.exports = function assert_options(what, value) {
  // eslint-disable-line camelcase
  //  discuss at: http://locutus.io/php/assert_options/
  // original by: Brett Zamir (http://brett-zamir.me)
  //   example 1: assert_options('ASSERT_CALLBACK')
  //   returns 1: null

  var iniKey, defaultVal;
  switch (what) {
    case 'ASSERT_ACTIVE':
      iniKey = 'assert.active';
      defaultVal = 1;
      break;
    case 'ASSERT_WARNING':
      iniKey = 'assert.warning';
      defaultVal = 1;
      var msg = 'We have not yet implemented warnings for us to throw ';
      msg += 'in JavaScript (assert_options())';
      throw new Error(msg);
    case 'ASSERT_BAIL':
      iniKey = 'assert.bail';
      defaultVal = 0;
      break;
    case 'ASSERT_QUIET_EVAL':
      iniKey = 'assert.quiet_eval';
      defaultVal = 0;
      break;
    case 'ASSERT_CALLBACK':
      iniKey = 'assert.callback';
      defaultVal = null;
      break;
    default:
      throw new Error('Improper type for assert_options()');
  }

  // I presume this is to be the most recent value, instead of the default value
  var iniVal = (typeof require !== 'undefined' ? require('../info/ini_get')(iniKey) : undefined) || defaultVal;

  return iniVal;
};
//# sourceMappingURL=assert_options.js.map