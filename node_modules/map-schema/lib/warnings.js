'use strict';

var util = require('util');
var utils = require('./utils');

module.exports = {
  missing: function(warning) {
    return 'Required field "' + warning.prop + '" is missing';
  },
  deprecated: function(warning) {
    var msg = warning.message;
    return 'property "' + msg.actual + '" is deprecated, use "' + msg.expected + '" instead';
  },
  invalidField: function(warning) {
    return 'invalid property: "' + warning.prop + '". Since `options.knownOnly` is true, only properties with fields defined on the schema may be used.';
  },
  invalidFile: function(warning) {
    return 'file "' + warning.message.actual + '" does not exist on the file system';
  },
  invalidType: function(warning) {
    var msg = warning.message;
    return 'expected "' + warning.prop + '" to be '
      + utils.article(msg.expected) + ' but got "'
      + msg.actual + '"';
  },
  invalidValue: function(warning) {
    var actual = warning.message.actual;
    return 'invalid value defined on property "' + warning.prop + '": ' + util.inspect(actual);
  }
};
