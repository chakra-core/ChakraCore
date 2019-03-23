'use strict';

var path = require('path');
var Base = require('base');
var util = require('expand-utils');
var utils = require('./utils');

/**
 * Create an instance of `ExpandFiles` with `options`.
 *
 * ```js
 * var config = new ExpandFiles({cwd: 'src'});
 * ```
 * @param {Object} `options`
 * @api public
 */

function ExpandFiles(options) {
  if (!(this instanceof ExpandFiles)) {
    return new ExpandFiles(options);
  }

  Base.call(this, {}, options);
  this.use(utils.plugins());
  this.is('Files');

  this.options = options || {};
  if (util.isFiles(options) || arguments.length > 1) {
    this.options = {};
    this.expand.apply(this, arguments);
    return this;
  }
}

/**
 * Inherit `Base`
 */

Base.extend(ExpandFiles);

/**
 * Normalize and expand files definitions and src-dest mappings
 * from a declarative configuration.
 *
 * ```js
 * var config = new ExpandFiles({cwd: 'src'});
 * config.expand({src: '*.hbs', dest: 'templates/'});
 * ```
 * @param {Object|String|Array} `src`
 * @param {Object|String} `dest`
 * @param {Object} `options`
 * @return {Object}
 * @api public
 */

ExpandFiles.prototype.expand = function(src, dest, options) {
  var files = utils.normalize.apply(this, arguments);
  util.run(this, 'normalized', files);
  this.emit('files', 'normalized', files);

  files = expandMapping.call(this, files, options);
  util.run(this, 'expanded', files);
  this.emit('files', 'expanded', files);

  for (var key in files) {
    if (files.hasOwnProperty(key)) {
      this[key] = files[key];
    }
  }
  return this;
};

/**
 * Iterate over a files array and expand src-dest mappings
 *
 * ```js
 * { files: [ { src: [ '*.js' ], dest: 'dist/' } ] }
 * ```
 * @param {Object} `config`
 * @param {Object} `options`
 * @return {Object}
 */

function expandMapping(config, options) {
  var len = config.files.length, i = -1;
  var res = [];

  while (++i < len) {
    var raw = config.files[i];
    var node = new RawNode(raw, config, this);
    this.emit('files', 'rawNode', node);
    this.emit('rawNode', node);
    if (node.files.length) {
      res.push.apply(res, node.files);
    }
  }
  config.files = res;
  return config;
}

/**
 * Create a `raw` node. A node represents a single element
 * in a `files` array, and "raw" means that `src` glob patterns
 * have not been expanded.
 *
 * @param {Object} `raw`
 * @param {Object} `config`
 * @return {Object}
 */

function RawNode(raw, config, app) {
  utils.define(this, 'isRawNode', true);
  util.run(config, 'rawNode', raw);
  this.files = [];
  var paths = {};

  raw.options = utils.extend({}, config.options, raw.options);
  var opts = resolvePaths(raw.options);
  var filter = filterFiles(opts);

  var srcFiles = utils.arrayify(raw.src);
  if (opts.glob !== false && utils.hasGlob(raw.src)) {
    srcFiles = utils.glob.sync(raw.src, opts);
  }

  srcFiles = srcFiles.filter(filter);

  if (config.options.mapDest) {
    var len = srcFiles.length, i = -1;
    while (++i < len) {
      var node = new FilesNode(srcFiles[i], raw, config);
      app.emit('files', 'filesNode', node);
      app.emit('filesNode', node);
      var dest = node.dest;

      if (!node.src && !node.path) {
        continue;
      }

      var src = resolveArray(node.src, opts);
      if (paths[dest]) {
        paths[dest].src = paths[dest].src.concat(src);
      } else {
        node.src = utils.arrayify(src);
        this.files.push(node);
        paths[dest] = node;
      }
    }

    if (!this.files.length) {
      node = raw;
      raw.src = [];
      this.files.push(raw);
    }

  } else {
    createNode(srcFiles, raw, this.files, config);
  }
}

/**
 * Create a new `Node` with the given `src`, `raw` node and
 * `config` object.
 *
 * @param {String|Array} `src` Glob patterns
 * @param {Object} `raw` FilesNode with un-expanded glob patterns.
 * @param {Object} `config`
 * @return {Object}
 */

function FilesNode(src, raw, config) {
  utils.define(this, 'isFilesNode', true);
  this.options = utils.omit(raw.options, ['mapDest', 'flatten', 'rename', 'filter']);
  this.src = utils.arrayify(src);

  if (this.options.resolve) {
    var cwd = path.resolve(this.options.cwd || process.cwd());
    this.src = this.src.map(function(fp) {
      return path.resolve(cwd, fp);
    });
  }

  if (raw.options.mapDest) {
    this.dest = mapDest(raw.dest, src, raw);
  } else {
    this.dest = rewriteDest(raw.dest, src, raw.options);
  }

  // copy properties to the new node
  for (var key in raw) {
    if (key !== 'src' && key !== 'dest' && key !== 'options') {
      this[key] = raw[key];
    }
  }
  util.run(config, 'filesNode', this);
}

/**
 * Create a new `Node` and push it onto the `files` array if the
 * returned node has a `path` or `src` property. In plugins, you
 * may remove the `src` and/or `path` properties to prevent nodes
 * from being pushed onto the array.
 */

function createNode(src, raw, files, config) {
  var node = new FilesNode(src, raw, config);
  if (node.path || node.src) {
    files.push(node);
  }
}

function mapDest(dest, src, node) {
  var opts = node.options;

  if (opts.rename === false) {
    return dest;
  }

  var parent = '';
  var fp = src;

  if (fp && typeof fp === 'string') {
    if (utils.hasGlob(fp)) {
      parent = utils.base(fp);
      fp = '';
    }
    var cwd = path.resolve(opts.cwd || '');
    fp = path.join(cwd, fp);
    fp = utils.relative(cwd, fp);
  } else {
    fp = '';
  }

  if (opts.flatten === true) {
    fp = path.basename(fp);
  }

  if (opts.base === true) {
    opts.base = parent;
  }

  if (opts.base) {
    dest = path.join(dest, opts.base, path.basename(fp));
  } else {
    dest = path.join(dest, fp);
  }
  return rewriteDest(dest, src, opts);
}

/**
 * Used when `mapDest` is not true
 */

function rewriteDest(dest, src, opts) {
  dest = utils.resolve(dest);
  if (opts.destBase) {
    dest = path.join(opts.destBase, dest);
  }
  if (opts.extDot || opts.hasOwnProperty('ext')) {
    dest = rewriteExt(dest, opts);
  }
  if (typeof opts.rename === 'function') {
    return opts.rename(dest, src, opts);
  }
  return dest;
}

function rewriteExt(dest, opts) {
  var re = {first: /(\.[^\/]*)?$/, last: /(\.[^\/\.]*)?$/};

  if (opts.ext === false) {
    opts.ext = '';
  }

  if (opts.ext.charAt(0) !== '.') {
    opts.ext = '.' + opts.ext;
  }

  if (typeof opts.extDot === 'undefined') {
    opts.extDot = 'first';
  }

  dest = dest.replace(re[opts.extDot], opts.ext);
  if (dest.slice(-1) === '.') {
    dest = dest.slice(0, -1);
  }
  return dest;
}

function resolvePaths(opts) {
  if (opts.destBase) {
    opts.destBase = utils.resolve(opts.destBase);
  }
  if (opts.cwd) {
    opts.cwd = utils.resolve(opts.cwd);
  }
  return opts;
}

/**
 * Default filter function.
 *
 * @param {String} `fp`
 * @param {Object} `opts`
 * @return {Boolean} Returns `true` if a file matches.
 */

function filterFiles(opts) {
  if (typeof opts.filter === 'function') {
    return opts.filter;
  }
  return function(fp) {
    return true;
  };
}

function resolveArray(files, opts) {
  if (!opts.mapDest) return files;

  return files.map(function(fp) {
    return path.join(opts.cwd || '', fp);
  });
}

/**
 * Expose expand
 */

module.exports = ExpandFiles;
