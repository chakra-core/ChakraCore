'use strict';

var utils = require('./utils');

function toGenerator(name, val, options, parent) {
  var Generator = parent.constructor;
  var generator = val;

  if (utils.isValidInstance(val)) {
    generator.constructor = Generator;
  } else {
    generator = new Generator();
  }

  var fn = decorate(name, val, options, parent);
  generator.use(fn);

  // cache the generator
  parent.generators[generator.alias] = generator;
  parent.generators[generator.name] = generator;
  parent.generators[name] = generator;
  parent.emit('generator', generator);
  return generator;
}

function decorate(name, val, options, parent) {
  return function(app) {
    utils.merge(this.options, parent.options);
    this.is('generator');
    this.isApp = true;
    this.define('parent', parent);
    parent.run(this);

    this.on('error', parent.emit.bind(parent, 'error'));

    Object.defineProperty(this, 'env', {
      configurable: true,
      get: function() {
        return parent.createEnv(name, val, this.options);
      }
    });

    Object.defineProperty(this, 'alias', {
      configurable: true,
      get: function() {
        return this.env.alias;
      }
    });

    Object.defineProperty(this, 'name', {
      configurable: true,
      get: function() {
        return this.env.name;
      }
    });

    this.define('isMatch', function() {
      return this.env.isMatch.apply(this.env, arguments);
    });

    this.define('invoke', function(context, options) {
      return this.env.invoke.apply(this.env, arguments);
    });
  };
}

module.exports = toGenerator;
