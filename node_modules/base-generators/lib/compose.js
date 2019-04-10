'use strict';

var utils = require('./utils');

/**
 * Extend the generator being invoked with settings from the instance,
 * but only if the generator is not the `default` generator.
 *
 * Also, note that this **does not add tasks** from the `default` generator
 * onto the instance.
 */

module.exports = function(app, generator, ctx) {
  var env = generator.env || {};
  var alias = env.alias;

  // update `cache.config`
  var config = utils.merge({}, ctx || app.cache.config || app.pkg.get(app._name));
  generator.set('cache.config', config);

  // set options
  utils.merge(generator.options, app.options);
  utils.merge(generator.options, config);

  // extend generator with settings from default
  if (app.generators.hasOwnProperty('default') && alias !== 'default') {
    var compose = generator
      .compose(['default'])
      .options();

    if (typeof app.data === 'function') {
      compose.data();
    }

    if (typeof app.pipeline === 'function') {
      compose.pipeline();
    }

    if (typeof app.helper === 'function') {
      compose.helpers();
      compose.engines();
      compose.views();
    }

    if (typeof app.question === 'function') {
      compose.questions();
    }
  }
};
