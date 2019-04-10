/*!
 * common-middleware <https://github.com/jonschlinkert/common-middleware>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var debug = require('debug')('common-middleware');
var utils = require('./utils');

function middleware(options) {
  options = options || {};

  return function plugin(app) {
    if (!utils.isValid(app, 'common-middleware')) return;
    debug('initializing from <%s>', __filename);

    // we'll assume none of the stream handlers exist if `onStream` is not registered
    if (typeof this.onStream !== 'function') {
      this.handler('onStream');
      this.handler('preWrite');
      this.handler('postWrite');
    }

    var opts = utils.merge({}, this.options, options);
    var jsonRegex = opts.jsonRegex || /\.(json|jshintrc)$/;
    var escapeRegex = opts.escapeRegex || '*';
    var extRegex = opts.extRegex || '*';

    /**
     * Parses front-matter on files that match `options.extRegex` and
     * adds the resulting data object to `file.data`. This object is
     * passed as context to the template engine at render time.
     *
     * @name front matter
     * @api public
     */

    this.onLoad(extRegex, utils.mu.series([
      isHandled,
      isBinary,
      utils.matter.parse,
      escape,
      stripPrefixes,
      utils.file.renameFile(this)
    ]));

    this.onLoad(jsonRegex, parseJson);

    /**
     * Registers a `.preWrite` middleware for unescaping escaped delimiters.
     *
     * @name unescape templates
     * @api public
     */

    this.postRender(escapeRegex, utils.mu.series([
      utils.matter.stringify,
      unescape
    ]));

    this.preWrite(escapeRegex, utils.mu.series([
      utils.matter.stringify,
      unescape
    ]));

    this.preWrite(jsonRegex, updateJson);
    return plugin;
  };
};

function isHandled(file, next) {
  file.isHandled = function(name, stage) {
    var current = this.options.method;
    this.handled = this.handled || {};
    if (this.handled.hasOwnProperty(name)) {
      if (typeof stage !== 'string') return true;
      if (this.handled[name].indexOf(stage) !== -1 && stage === current) {
        return true;
      }
    }

    this.handled[name] = this.handled[name] || [];
    if (typeof stage === 'string') {
      this.handled[name].push(stage);
    }
    return false;
  };
  next(null, file);
}

/**
 * strip prefixes from dotfile and config templates
 */

function stripPrefixes(file, next) {
  if (!/templates/.test(file.dirname)) {
    next(null, file);
    return;
  }

  if (file.has('action.stripPrefixes')) {
    next(null, file);
    return;
  }

  file.set('action.stripPrefixes', true);
  file.basename = file.basename.replace(/^_/, '.');
  file.basename = file.basename.replace(/^\$/, '');
  next(null, file);
}

/**
 * Uses C-style macros to escape templates with `{%%= foo %}` or
 * `<%%= foo %>` syntax, so they will not be evaluated by a template
 * engine when `.render` is called.
 *
 * @name escape templates
 * @api public
 */

function escape(file, next) {
  if (file.isNull() || file.isBinary()) {
    next(null, file);
    return;
  }

  if (file.has('action.escaped')) {
    next(null, file);
    return;
  }

  file.set('action.escaped', true);
  var str = file.contents.toString();
  str = str.replace(/([{<])(%%[=-]?)/g, '__ESC_$1DELIM$2__');
  file.contents = new Buffer(str);
  next(null, file);
}

/**
 * Expose `unescape`
 */

function unescape(file, next) {
  if (file.isNull() || file.isBinary()) {
    next(null, file);
    return;
  }

  if (!file.has('action.escaped')) {
    next(null, file);
    return;
  }

  file.set('action.escaped', false);
  var str = file.contents.toString();
  str = str.replace(/__ESC_([{<])DELIM%(%[=-]?)__/g, '$1$2');
  file.contents = new Buffer(str);
  next();
}

/**
 * Adds a `json` property to the `file` object when the file extension
 * matches `options.jsonRegex`. This allows JSON files to be updated
 * by other middleware or pipeline plugins without having to parse and
 * stringify with each modification.
 *
 * @name JSON on-load
 * @api public
 */

function parseJson(file, next) {
  if (file.isNull() || file.isBinary()) {
    next(null, file);
    return;
  }

  var str = file.contents.toString();
  utils.define(file, 'originalContent', str);
  var json;

  Object.defineProperty(file, 'json', {
    configurable: true,
    enumerable: true,
    set: function(val) {
      json = val;
    },
    get: function() {
      return json || (json = JSON.parse(str));
    }
  });

  next(null, file);
}

/**
 * If `file.contents` has not already been updated directly, the `file.contents` property
 * is updated with stringified JSON before writing the file back to the file
 * system.
 *
 * @name JSON pre-write
 * @api public
 */

function updateJson(file, next) {
  if (file.isNull() || file.isBinary()) {
    next(null, file);
    return;
  }

  if (file.contents.toString() !== file.originalContent) {
    next(null, file);
    return;
  }
  if (!utils.isObject(file.json)) {
    next(null, file);
    return;
  }
  file.contents = new Buffer(JSON.stringify(file.json, null, 2) + '\n');
  next(null, file);
}

function isBinary(file, next) {
  file.isBinary = function() {
    if (this.hasOwnProperty('_isBinary')) {
      return this._isBinary;
    }

    if (typeof this.isStream === 'function' && this.isStream()) {
      this._isBinary = false;
      return false;
    }
    if (typeof this.isDirectory === 'function' && this.isDirectory()) {
      this._isBinary = false;
      return false;
    }
    if (this.isNull()) {
      this._isBinary = false;
      return false;
    }

    var exts = ['.png', '.jpg', '.pdf', '.gif'];
    if (exts.indexOf(this.extname) !== -1) {
      this._isBinary = true;
      return true;
    }

    var len = this.stat ? this.stat.size : this.contents.length;
    this._isBinary = utils.isBinary.sync(this.contents, len);
    return this._isBinary;
  };
  next(null, file);
}

middleware.unescape = unescape;
module.exports = middleware;
