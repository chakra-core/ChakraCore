'use strict';

var path = require('path');
var utils = require('../utils');
var merge = require('../merge');

module.exports = function(files, key, config, schema) {
  merge(config, schema);

  files = files || config[key];
  if (schema.checked[key]) {
    return files;
  }

  files = utils.arrayify(files).filter(Boolean);
  files = utils.union([], files, config.files);
  var opts = utils.merge({}, schema.options);

  var len = files.length;
  var idx = -1;
  var res = Array.isArray(opts.files) ? opts.files : [];

  while (++idx < len) {
    var file = files[idx];
    if (utils.exists(path.resolve(process.cwd(), file))) {
      utils.union(res, utils.stripSlash(file));
    }
  }

  // commond directories
  addPath(res, 'lib', opts, config);
  addPath(res, 'dist', opts, config);
  addPath(res, 'bin', opts, config);

  // common files
  addPath(res, 'README.md', opts, config);
  addPath(res, 'LICENSE', opts, config);
  addPath(res, 'index.js', opts, config);
  addPath(res, 'utils.js', opts, config);
  addPath(res, 'cli.js', opts, config);

  if (/^(generate|verb|assemble|updater)-/.test(config.name)) {
    addPath(res, 'updatefile.js', opts, config);
    addPath(res, 'generator.js', opts, config);
    addPath(res, 'templates', opts, config);
  }

  if (res.length) {
    res.sort();
    config[key] = res;
    schema.update('bin', config);
    schema.checked[key] = true;
    return res;
  }

  // don't use `omit`, so the key can be re-added if needed
  delete config[key];
  return;
};

function addPath(files, file, opts, config) {
  if (Array.isArray(opts.files) && opts.files.indexOf(file) === -1) {
    return;
  }
  if (files.indexOf(file) !== -1) {
    return;
  }
  if (utils.exists(path.resolve(process.cwd(), file))) {
    utils.union(files, file);
  }
}
