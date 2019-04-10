'use strict';

var utils = require('../utils');
var debug = require('../debug');

module.exports = function(app) {
  return function(collections, prop, options, schema) {
    if (!collections || utils.isEmpty(collections)) {
      return null;
    }

    debug.field(prop, collections);
    var views = {};
    var type, msg;

    if (utils.isObject(collections)) {
      for (var key in collections) {
        var collection = collections[key];

        if (typeof collection === 'string' || Array.isArray(collection)) {
          var Loader = utils.loader;
          var loader = new Loader(app.options);
          views[key] = loader.load(collection);

        } else if (utils.isObject(collection)) {
          views[key] = collection;

        } else {
          type = utils.typeOf(collection);
          msg = 'expected views to be an object, string or array, received: "' + type + '"';
          throw new TypeError(msg);
        }
      }
    } else {
      type = utils.typeOf(collections);
      msg = 'expected collections to be an object, received: "' + type + '"';
      throw new TypeError(msg);
    }

    return views;
  };
};
