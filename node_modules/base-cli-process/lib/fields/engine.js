'use strict';

var fns = require('../fns');

/**
 * Alias for [engines](#engines)
 *
 * @name engines
 * @api public
 */

module.exports = function(app) {
  return fns(app, 'engines');
};
