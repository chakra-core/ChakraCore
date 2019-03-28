'use strict';

var Base = require('base');
var debug = require('debug')('base:templates:group');
var utils = require('./utils');

/**
 * Expose `Group`
 */

module.exports = exports = Group;

/**
 * Create an instance of `Group` with the given `options`.
 *
 * ```js
 * var group = new Group({
 *   'foo': { items: [1,2,3] }
 * });
 * ```
 * @param {Object} `options`
 * @api public
 */

function Group(config) {
  if (!(this instanceof Group)) {
    return new Group(config);
  }

  Base.call(this, config);
  this.is('Group');
  this.use(utils.option());
  this.use(utils.plugin());
  this.init();
}

/**
 * Inherit `Base`
 */

Base.extend(Group);

/**
 * Initialize Group defaults. Makes `options` and `cache`
 * (inherited from `Base`) non-emumerable.
 */

Group.prototype.init = function() {
  debug('initializing');
  var opts = {};

  Object.defineProperty(this, 'options', {
    configurable: true,
    enumerable: false,
    set: function(val) {
      opts = val;
    },
    get: function() {
      return opts || {};
    }
  });

  this.define('cache', this.cache);
  this.define('List', this.List || require('./list'));
};

/**
 * Get a value from the group instance. If the value is an array,
 * it will be returned as a new `List`.
 */

Group.prototype.get = function() {
  var res = Base.prototype.get.apply(this, arguments);
  if (Array.isArray(res)) {
    var List = this.List;
    var list = new List();
    list.addItems(res);
    return list;
  }
  handleErrors(this, res);
  return res;
};

/**
 * When `get` returns a non-Array object, we decorate
 * noop `List` methods onto the object to throw errors when list methods
 * are used, since list array methods do not work on groups.
 *
 * @param {Object} `group`
 * @param {Object} `val` Value returned from `group.get()`
 */

function handleErrors(group, val) {
  if (utils.isObject(val)) {
    var List = group.List;
    var keys = Object.keys(List.prototype);

    keys.forEach(function(key) {
      if (typeof val[key] !== 'undefined') return;

      utils.define(val, key, function() {
        throw new Error(key + ' can only be used with an array of `List` items.');
      });
    });
  }
}
