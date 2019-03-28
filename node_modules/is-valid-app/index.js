/*!
 * is-valid-app (https://github.com/jonschlinkert/is-valid-app)
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var utils = require('./utils');

module.exports = function(app, name, types) {
  if (typeof name !== 'string') {
    throw new TypeError('expected plugin name to be a string');
  }

  // if `app` is not a valid instance of `Base`, or if `app` is a valid
  // instance of Base by not one of the given `types` return false
  if (!utils.isValid(app, types)) {
    return false;
  }

  // if the `name` has already been registered as a plugin, return false
  if (utils.isRegistered(app, name)) {
    return false;
  }

  var debug = utils.debug('base:generate:' + name);
  debug('initializing from <%s>', (module.parent && module.parent.id) || __dirname);
  return true;
};
