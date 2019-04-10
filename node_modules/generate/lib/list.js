'use strict';

var path = require('path');
var utils = require('./utils');

module.exports = function(app) {
  function bold(str) {
    return app.log.underline(app.log.bold(str));
  }

  var list = [[bold('version'), bold('name'), bold('alias')]];
  var cache = {};

  return utils.through.obj(function(file, enc, next) {
    if (cache[file.stem]) {
      next();
      return;
    }

    cache[file.stem] = true;
    if (utils.blacklist.indexOf(file.stem) !== -1) {
      next();
      return;
    }

    var pkgPath = path.resolve(file.path, 'package.json');
    var pkg = require(pkgPath);
    list.push([app.log.gray(pkg.version), file.basename, app.log.cyan(file.alias)]);
    next();
  }, function(cb) {
    console.log();
    console.log(utils.table(list, {
      stringLength: function(str) {
        return utils.strip(str).length;
      }
    }));

    console.log();
    console.log(app.log.magenta(list.length + ' generators installed'));
    console.log();
    cb();
  });
};

