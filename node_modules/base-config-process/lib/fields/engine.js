'use strict';

var engines = require('./engines');

/**
 * Alias for [engines](#engines)
 *
 * @name engines
 * @api public
 */

module.exports = function(app) {
  return engines(app);
};
