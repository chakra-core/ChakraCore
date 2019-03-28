/*!
 * expand-args <https://github.com/jonschlinkert/expand-args>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var utils = require('./utils');
var sep = /[=:]/;

function expand(argv, options) {
  var opts = utils.merge({}, options);
  var res = {};
  var segs;
  var val;
  var key;

  if (Array.isArray(argv)) {
    argv = argv.reduce(function(acc, val) {
      if (typeof val === 'string' && isWindowsPath(val)) {
        res._ = res._ || [];
        res._.push(val);
        return acc;
      }
      acc.push(val);
      return acc;
    }, []);
  }

  argv = preProcess(argv, options);

  function merge(key, val) {
    if (key === '_') {
      res = utils.merge({}, res, val);
    } else {
      utils.set(res, key, val);
    }
  }

  for (key in argv) {
    if (argv.hasOwnProperty(key)) {
      val = argv[key];

      if (typeof val === 'string' && isWindowsPath(val)) {
        res[key] = val;
        continue;
      }

      // '{'a b': true}'
      if (sep.test(key) && isBoolean(val)) {
        segs = key.split(sep);
        val = segs.pop();
        key = segs.join('.');
      }

      val = toBoolean(val);

      switch (utils.typeOf(val)) {
        case 'object':
          res[key] = expand(val);
          break;
        case 'string':
          if (~val.indexOf('|')) {
            val = val.split('|');
            utils.set(res, key, expandEach(val));

          } else if (isUrl(val)) {
            utils.set(res, key, expandString(val));

          } else if (/\w,\w/.test(val)) {
            val = expandString(val);
            if (Array.isArray(val) && hasObjects(val)) {
              val.forEach(function(ele) {
                merge(key, toBoolean(ele));
              });
            } else {
              merge(key, val);
            }

          } else if (sep.test(val)) {
            if (opts.esc && ~opts.esc.indexOf(key)) {
              val = val.split('.').join('\\.');
            }
            var str = key + ':' + val;
            segs = str.split(sep);
            val = toBoolean(segs.pop());
            key = segs.join('.');
            utils.set(res, key, expandString(val));

          } else {
            res[key] = val.split('\\.').join('.');
          }
          break;
        case 'array':
          if (hasObjects(val)) {
            merge(key, expandEach(val));
          } else {
            res[key] = val.map(function(ele) {
              return toBoolean(ele);
            });
          }
          break;
        case 'boolean':
        default: {
          res[key] = val;
          break;
        }
      }
    }
  }
  return res;
}

function expandString(val) {
  if (isUrl(val)) {
    return val;
  }

  if (isPath(val)) {
    val = unescape(val);
    if (~val.indexOf(':')) {
      return toObject(val);
    }
    if (~val.indexOf(',')) {
      return val.split(',');
    }
    return toBoolean(val);
  }

  if (/^[\s\w]+$/.test(val)) {
    return val;
  }

  return utils.expand(val, {
    toBoolean: true
  });
}

function preProcess(argv, options) {
  var obj = {};

  if (Array.isArray(argv)) {
    argv = argv.reduce(function(acc, str) {
      if (!/['"]/.test(str)) {
        return acc.concat(str.split(' '));
      }
      return acc.concat(str);
    }, []);

    argv = argv.map(function(str) {
      if (isUrl(str) || /['"]/.test(str)) {
        str = str.replace(/^-+/, '');
        var m = /[=:.]/.exec(str);
        if (m) {
          var key = str.slice(0, m.index);
          var val = str.slice(m.index + 1);
          if (isUrl(val)) {
            obj[key] = val;
            return;
          }
          if (/^['"]/.test(val) && /['"]$/.test(val)) {
            obj[key] = val.slice(1, val.length - 1);
            return;
          }
        }
        return str;
      }

      if (!/=.*,/.test(str)) {
        str = str.split(':').join('=');
      }
      return str;
    });

    argv = utils.minimist(argv, options);
  }

  argv = utils.merge({}, argv, obj);
  return utils.omitEmpty(argv);
}

function expandEach(arr) {
  var len = arr.length;
  var idx = -1;
  var res = {};
  while (++idx < len) {
    utils.merge(res, expand(utils.expand(arr[idx], {toBoolean: true})));
  }
  return res;
}

function isBoolean(val) {
  return val === 'false'
    || val === 'true'
    || val === false
    || val === true;
}

function isUrl(val) {
  return /\w+:\/\/\w/.test(val);
}

function isWindowsPath(val) {
  return /^\w:[\\\/]/.test(val);
}

function isPath(val) {
  return /(?:[\\\/]|\\\.)/.test(val);
}

function unescape(val) {
  if (isString(val)) {
    return val.split('\\').join('');
  }
  if (isObject(val)) {
    for (var key in val) {
      val[key] = unescape(val[key]);
    }
    return val;
  }
  if (Array.isArray(val)) {
    return val.map(unescape);
  }
  return val;
}

function isString(val) {
  return typeof val === 'string';
}

function toBoolean(val) {
  if (val === 'true') val = true;
  if (val === 'false') val = false;
  return val;
}

function isObject(val) {
  return utils.typeOf(val) === 'object';
}

function toObject(str) {
  var res = {};
  var segs = str.split(':');
  var val = segs.pop();
  res[segs.join('.')] = val;
  return res;
}

function hasObjects(arr) {
  var len = arr.length;
  var idx = -1;

  while (++idx < len) {
    if (isObject(arr[idx]) || /[:=|,]/.test(arr[idx])) {
      return true;
    }
  }
  return false;
}

/**
 * Expose `expand`
 */

module.exports = expand;
