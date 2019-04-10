'use strict';

var generators = require('./generators');

module.exports = function(app, options) {
  return generators(app, options);
};
