'use strict';

var utils = require('./utils');

/**
 * Front matter parser
 */

var parser = module.exports;

/**
 * Parse front matter from the given string or the `contents` in the
 * given `file` and callback `next(err, file)`.
 *
 * If an object is passed, either `file.contents` or `file.content`
 * may be used (for gulp and assemble compatibility).
 *
 * ```js
 * // pass a string
 * parser.parse('---\ntitle: foo\n---\nbar', function (err, file) {
 *   //=> {content: 'bar', data: {title: 'foo'}}
 * });
 *
 * // or an object
 * var file = {contents: new Buffer('---\ntitle: foo\nbar')};
 * parser.parse(file, function(err, res) {
 *   //=> {content: 'bar', data: {title: 'foo'}}
 * });
 * ```
 * @param {String|Object} `file` The object or string to parse.
 * @param {Object|Function} `options` or `next` callback function. Options are passed to [gray-matter][].
 * @param {Function} `next` callback function.
 * @api public
 */

parser.parse = function matterParse(file, options, next) {
  if (typeof options === 'function') {
    next = options;
    options = {};
  }

  if (typeof next !== 'function') {
    throw new TypeError('expected a callback function');
  }

  try {
    next(null, parser.parseSync(file, options));
  } catch (err) {
    next(err);
  }
};

/**
 * Parse front matter from the given string or the `contents` in the
 * given `file`. If an object is passed, either `file.contents` or
 * `file.content` may be used (for gulp and assemble compatibility).
 *
 * ```js
 * // pass a string
 * var res = parser.parseSync('---\ntitle: foo\n---\nbar');
 *
 * // or an object
 * var file = {contents: new Buffer('---\ntitle: foo\nbar')};
 * var res = parser.parseSync(file);
 * //=> {content: 'bar', data: {title: 'foo'}}
 * ```
 * @param {String|Object} `file` The object or string to parse.
 * @param {Object} `options` passed to [gray-matter][].
 * @api public
 */

parser.parseSync = function matterParseSync(file, options) {
  if (typeof file === 'string') {
    file = { contents: new Buffer(file) };

  } else if (!utils.isObject(file)) {
    throw new TypeError('expected file to be a string or object');
  }

  if (file.content && !file.contents) {
    file.contents = new Buffer(file.content);
  }

  file.data = file.data || {};
  if (!utils.isValidFile(file)) {
    file.content = file.contents.toString();
    file.orig = file.content;
    return file;
  }

  try {
    var opts = utils.extend({strict: true}, options, file.options);

    // allow files to selectively disable parsing
    if (opts.frontMatter === false) {
      file.content = file.contents.toString();
      file.orig = file.content;
      return file;
    }

    var str = file.contents.toString();
    var parsed = utils.matter(str, opts);
    file.orig = parsed.orig;
    file.data = utils.merge({}, file.data, parsed.data);
    file.content = utils.trim(parsed.content);
    file.contents = new Buffer(file.content);
    return file;
  } catch (err) {
    throw err;
  }
};

parser.stringify = function stringify(file, options, next) {
  if (typeof options === 'function') {
    next = options;
    options = {};
  }

  if (typeof next !== 'function') {
    throw new TypeError('expected a callback function');
  }

  if (!utils.isValidFile(file) || !utils.hasYFM(file)) {
    next(null, file);
    return;
  }

  try {
    next(null, parser.stringifySync(file, options));
  } catch (err) {
    next(err);
  }
};

parser.stringifySync = function stringifySync(file, options) {
  if (typeof file === 'string') {
    file = { contents: new Buffer(file) };

  } else if (!utils.isObject(file)) {
    throw new TypeError('expected file to be a string or object');
  }

  if (file.content && !file.contents) {
    file.contents = new Buffer(file.content);
  }

  if (!utils.isValidFile(file) || !utils.hasYFM(file)) {
    return file;
  }

  try {
    file.content = utils.matter.stringify(file.content, file.data.yfm);
    delete file.data.yfm;
    return file;
  } catch (err) {
    throw err;
  }
};

/**
 * Decorate `.parseMatter` and `.stringifyMatter` functions onto the `file` object,
 * so front-matter can be parsed on-demand.
 */

parser.file = function(file, options) {
  file.parseMatter = function(opts) {
    return parser.parseSync(this, utils.merge({}, options, opts));
  };

  file.stringifyMatter = function(opts) {
    return parser.stringifySync(this, utils.merge({}, options, opts));
  };
};
