'use strict';

var os = require('os');
var path = require('path');
var plugins = require('./plugins');
var utils = require('./utils');

module.exports = function(Generate, app) {
  var re = /^(base|generate|helper|updater|(assemble|verb)(?:-generate)?)-/;
  app.define('aliasRegex', re);

  app.cwd = app.options.dest || app.cwd;
  app.data('dest', app.cwd);

  // custom lookup function for resolving generators
  app.option('lookup', Generate.lookup);

  // custom `toAlias` function for resolving generators by alias
  app.option('toAlias', function(key) {
    return key.replace(app.get('aliasRegex'), '');
  });

  // format help menu
  app.option('help', {
    command: 'gen',
    configname: 'generator',
    appname: 'generate'
  });

  app.define('home', function() {
    var args = [].slice.call(arguments);
    var home = path.resolve(this.options.homedir || os.homedir());
    return path.resolve.apply(path, [home].concat(args));
  });

  Object.defineProperty(app, 'common', {
    configurable: true,
    get: function() {
      return utils.common;
    }
  });

  // register async `ask` helper
  app.asyncHelper('ask', utils.ask(app));

  // load plugins
  app.use(plugins.store('generate'));
  app.use(plugins.generators());
  app.use(plugins.pipeline());
  app.use(utils.askWhen());
};
