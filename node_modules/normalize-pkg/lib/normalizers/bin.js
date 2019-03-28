'use strict';

var fs = require('fs');
var path = require('path');
var utils = require('../utils');
var merge = require('../merge');

module.exports = function(bin, key, config, schema) {
  merge(config, schema);

  if (schema.checked[key]) {
    return bin;
  }

  var opts = utils.merge({}, schema.options);
  if (opts.bin === false) {
    bin = schema.data.get('bin') || config.bin || bin;
    schema.checked[key] = true;
    return bin;
  }

  if (typeof bin === 'string') {
    schema.union('files', config, bin);
    schema.checked[key] = true;
    return bin;
  }

  if (typeof bin === 'undefined') {
    bin = {};
  }

  // ensure `name` is defined before continuing
  schema.update('name', config);

  var obj = {};
  var num = 0;

  if (utils.isObject(bin)) {
    for (var prop in bin) {
      if (utils.exists(bin[prop])) {
        obj[prop] = bin[prop];
        num++;
      }
    }
    bin = obj;
  }

  // add cli.js or "bin/" if they exist
  var dir = path.resolve(process.cwd(), 'bin');
  if (num === 0 && utils.exists(dir)) {
    fs.readdirSync(dir).forEach(function(fp) {
      bin[config.name] = path.relative(process.cwd(), path.join(dir, fp));
      num++;
    });
  }

  if (num === 0 && utils.exists(path.resolve(process.cwd(), 'cli.js'))) {
    bin[config.name] = 'cli.js';
  }

  if (!utils.isEmpty(bin)) {
    if (typeof bin === 'string') {
      schema.union('files', config, bin);
    }

    config[key] = bin;
    schema.checked[key] = true;
    return bin;
  }

  schema.omit(key);
  return;
};
