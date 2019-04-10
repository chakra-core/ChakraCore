'use strict';

exports.configError = function configError(app, name, prop) {
  var appname = app._name + '.' + prop;
  console.error('cannot resolve ' + appname + ', in package.json ' + appname + ' config');
};

exports.moduleError = function moduleError(app, type, name) {
  return 'cannot find ' + app._name + ' ' + type + ' module ' + name + ' in local node_modules.';
};
