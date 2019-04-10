'use strict';

var fs = require('fs');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 * (here, `require` is actually lazy-cache)
 */

require('define-property', 'define');
require('extend-shallow', 'extend');
require('kind-of');
require('strip-bom-buffer');
require('strip-bom-string');
require('through2', 'through');
require = fn;

/**
 * Stream byte order marks from a string
 */

utils.stripBom = function(val) {
  if (typeof val === 'string') {
    return utils.stripBomString(val);
  }
  if (utils.isBuffer(val)) {
    return utils.stripBomBuffer(val);
  }
  return val;
};

/**
 * Return true if the given value is a stream.
 */

utils.isObject = function(val) {
  return utils.kindOf(val) === 'object';
};

utils.isBuffer = function(val) {
  return utils.kindOf(val) === 'buffer';
};

utils.isStream = function(val) {
  return utils.isObject(val)
    && (typeof val.pipe === 'function')
    && (typeof val.on === 'function');
};

utils.typeOf = function(val) {
  if (utils.isStream(val)) {
    return 'stream';
  }
  return utils.kindOf(val);
};

utils.syncContent = function(file) {
  Object.defineProperty(file, 'content', {
    configurable: true,
    set: function(content) {
      syncContents(this, content);
    },
    get: function() {
      var content;
      if (typeof this._content === 'string') {
        content = this._content;
      } else if (utils.isBuffer(this.contents)) {
        content = this.contents.toString();
      } else {
        content = this.contents;
      }
      syncContents(this, content);
      return this._content;
    }
  });
};

utils.syncContents = function(file, options) {
  var opts = utils.extend({}, options, file.options);

  utils.syncContent(file);
  syncContents(file, file.contents || file.content);

  Object.defineProperty(file, 'contents', {
    configurable: true,
    set: function(contents) {
      syncContents(this, contents);
    },
    get: function fn() {
      var contents;

      if (utils.isBuffer(this._contents) || utils.isStream(this._contents)) {
        contents = this._contents;
      } else if (opts.read === false || opts.noread === true) {
        contents = null;
      } else if (opts.buffer !== false && this.stat && this.stat.isFile()) {
        contents = utils.stripBom(fs.readFileSync(this.path));
      } else if (this.stat && this.stat.isFile()) {
        contents = fs.createReadStream(this.path);
      } else {

      }

      syncContents(this, contents);
      return contents;
    }
  });
};

/**
 * Sync the _content and _contents properties on a view to ensure
 * both are set when setting either.
 *
 * @param {Object} `view` instance of a `View`
 * @param {String|Buffer|Stream|null} `contents` contents to set on both properties
 */

function syncContents(file, val) {
  switch (utils.typeOf(val)) {
    case 'buffer':
      file._contents = val;
      file._content = val.toString();
      break;
    case 'string':
      file._contents = new Buffer(val);
      file._content = val;
      break;
    case 'stream':
      file._contents = val;
      file._content = val;
      break;
    case 'undefined':
    case 'null':
    default: {
      file._contents = null;
      file._content = null;
      break;
    }
  }
}

/**
 * Expose utils
 */

module.exports = utils;
