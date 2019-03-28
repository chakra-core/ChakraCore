'use strict';

var path = require('path');
var utils = require('../utils');

module.exports = function(app, options) {
  return function(obj, prop, config, schema) {
    if (!obj || utils.isEmpty(obj)) return null;
    if (!utils.isObject(obj)) {
      throw new TypeError('expected package.json "' + app._name + '.middleware" to be an object');
    }

    for (var key in obj) {
      var arr = obj[key];

      if (typeof arr === 'string' || utils.isObject(arr)) {
        arr = [arr];
      }

      obj[key] = arr.map(function(val) {
        var obj = {};
        if (typeof val === 'string') {
          obj.name = val;

          try {
            obj.fn = require(val);
          } catch (err) {
            try {
              obj.fn = require(path.resolve(val));
            } catch (x) {
              // throw first error, with original `val`
              throw err;
            }
          }
        }

        if (typeof obj.fn === 'function') {
          val = obj;
        }

        var opts = {};
        if (utils.isObject(val.options)) {
          opts = val.options;
          delete val.options;
        }

        if (typeof val.fn !== 'function') {
          if (typeof val.name !== 'string') {
            var keys = Object.keys(val).filter(function(k) {
              return ['fn', 'method', 'name', 'options'].indexOf(k) === -1;
            });

            var temp = val[keys[0]];
            if (keys.length === 1) {
              if (typeof val.method === 'string') {
                val.fn = temp[val.method];
              } else if (typeof temp === 'function') {
                val.fn = temp;
              } else {
                opts = utils.extend({}, opts, temp);
              }
              delete val[keys[0]];
              val.name = keys[0];
            } else {
              throw new Error('unexpected keys defined on middleware config: "' + keys.join(', ') + '"');
            }
          }
          if (typeof val.name === 'string' && typeof val.fn !== 'function') {
            var fp = utils.resolve.sync(val.name, {basedir: app.cwd});
            val.fn = require(fp);
          }
        }

        val.options = utils.extend({}, val.options, opts);
        return val;
      });
    }
    return obj;
  };
};
