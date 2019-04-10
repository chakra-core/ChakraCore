/*!
 * assemble-handle <https://github.com/jonschlinkert/assemble-handle>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var through = require('through2');

/**
 * Plugin for handling middleware
 *
 * @param {Object} `app` Instance of "app" (assemble, verb, etc) or a collection
 * @param {String} `stage` the middleware stage to run
 */

module.exports = create('handle');
module.exports.once = create('handleOnce');

/**
 * Create handle functions
 */

function create(prop) {
  return function handleOnce(app, stage) {
    return through.obj(function(file, enc, next) {
      if (!file.path && !file.isNull && !file.contents) {
        next();
        return;
      }

      if (file.isNull()) {
        next(null, file);
        return;
      }

      if (typeof app.handle !== 'function') {
        next(null, file);
        return;
      }

      // file.options is used for tracking middleware
      // stages during the render cycle
      if (typeof file.options === 'undefined') {
        file.options = {};
      }
      app[prop](stage, file, next);
    });
  };
}
