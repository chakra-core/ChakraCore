'use strict';

var path = require('path');
var debug = require('./debug');
var utils = require('./utils');

function normalize(val, key, config, schema, prop) {
  debug.field(key, val);
  return normalize[type(val)](val, key, config, schema, prop);
}

function type(val) {
  return !utils.hasGlob(val) || val === '*' ? utils.typeOf(val) : 'glob';
}

normalize.glob = function(val, key, config, schema) {
  return normalize(utils.glob.sync(val, schema.options), key, config, schema);
};

normalize.string = function(str, key, config, schema, prop) {
  var val = normalize.module(str, key, config, schema, prop);
  var obj = {};
  var alias;

  if (typeof val === 'undefined') {
    return;
  }

  if (utils.isObject(val) && typeof val.pipe === 'function') {
    alias = toAlias(str, key);
    obj[alias] = val;
    return obj;
  }

  var res = normalize(val, key, config, schema);
  if (utils.isObject(val)) {
    return res;
  }

  if (prop) {
    obj[prop] = res;
    return obj;
  }

  alias = toAlias(str, key);
  if (alias === 'index') {
    return res;
  }

  obj[alias] = res;
  return obj;
};

normalize.function = function(fn) {
  return { fn: fn };
};

normalize.object = function(obj, key, config, schema) {
  var res = {};

  for (var prop in obj) {
    var val = obj[prop];

    if (utils.isObject(val) && val.hasOwnProperty('fn') && typeof val.fn === 'function') {
      res[prop] = val;
      continue;
    }

    var opts = {};
    var alias;

    // engine name can be defined as a wildcard
    if (prop === '*' && (key === 'engines' || key === 'engine')) {
      if (!utils.isObject(val)) {
        alias = toAlias(val, key);
        val = normalize(val, key, config, schema);
        val = val[alias];
      }
      res[prop] = val;
      continue;
    }

    if (prop === './') {
      prop = path.resolve(prop);
    }

    if (utils.isObject(val)) {
      opts = val;
      val = prop;
    }

    alias = toAlias(prop, key);
    var ele = normalize(val, key, config, schema, alias);
    if (typeof val === 'function') {
      ele.options = opts;
      res[alias] = ele;
    } else {
      if (utils.isObject(ele) && utils.isObject(ele[alias])) {
        ele[alias].options = opts;
      }
      res = utils.merge({}, res, ele);
    }
  }
  return res;
};

normalize.array = function(val, key, config, schema, prop) {
  var len = val.length;
  var idx = -1;
  var obj = {};
  while (++idx < len) {
    obj = utils.merge({}, obj, normalize(val[idx], key, config, schema, prop));
  }
  return obj;
};

normalize.module = function(name, field, config, schema) {
  var cwd = config.cwd || schema.options.cwd || schema.app.cwd || process.cwd();
  var moduleErr;

  if (path.basename(cwd) === name || schema.app.pkg.get('name') === name) {
    return require.resolve(cwd);
  }

  try {
    return require(path.resolve(cwd, 'node_modules', name));
  } catch (err) {
    moduleErr = err;
  }

  try {
    return require(path.resolve(cwd, name));
  } catch (err) {
    moduleErr = err;
  }

  try {
    var fp = utils.resolve.sync(path.basename(name), {basedir: cwd});
    return require(fp);
  } catch (err) {
    moduleErr = err;
  }

  try {
    return require(path.resolve(cwd, path.resolve(name)));
  } catch (err) {
    var e = err;
    if (name.charAt(0) !== '.') {
      e = moduleErr;
    }

    if (schema.options.strictRequire) {
      e.message = 'package.json "' + schema.app._name + '" config property "' + field + '" > ' + e.message;
      throw e;
    }
  }
};

function toAlias(name, stringToStrip) {
  if (name.length === 1) {
    return name.toLowerCase();
  }
  var fp = path.basename(name, path.extname(name));
  return utils.camelcase(fp.replace(aliasRegex(stringToStrip), ''));
}

function aliasRegex(val) {
  var names = 'assemble|verb|updater?|generate|gulp|base|engine|helper|' + inflections(val);
  return new RegExp('^(' + names + ')(?=\\W+)');
}

function inflections(str) {
  var single = utils.inflect.singularize(str);
  var plural = utils.inflect.pluralize(str);
  return [single, plural].join('|');
}

/**
 * Expose 'plugins' expander
 */

module.exports = normalize;
