'use strict';

var utils = require('../utils');

/**
 * Set a value on the `related` property in package.json config.
 *
 * ```sh
 * $ --config=related.highlight:micromatch
 * //=> {verb: {related: {hightlight: 'micromatch'}}}
 *
 * $ --config=related.list:micromatch
 * //=> {verb: {related: {list: ['micromatch']}}}
 *
 * $ --config=related.list:micromatch,generate
 * //=> {verb: {related: {list: ['micromatch', 'generate']}}}
 * ```
 * @cli public
 */

module.exports = function(app) {
  return {
    type: ['array', 'object', 'string'],
    normalize: function(val, key, config, schema) {
      if (typeof val === 'undefined') {
        return;
      }

      var highlight;
      if (utils.isObject(val) && val.highlight) {
        highlight = val.highlight;

        if (utils.isObject(val.highlight)) {
          highlight = Object.keys(val.highlight)[0];
        }
      }

      if (utils.isObject(val) && val.list) {
        val.list = utils.arrayify(val.list);
        return val;
      }

      var obj = {};
      obj.list = utils.arrayify(val);
      if (utils.isString(highlight)) {
        obj.highlight = highlight;
      }

      config[key] = obj;
      return obj;
    }
  };
};

