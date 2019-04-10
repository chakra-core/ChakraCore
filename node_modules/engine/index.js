/**
 * Copyright 2012-2015 The Dojo Foundation <http://dojofoundation.org/>
 * Based on Underscore.js 1.8.3 <http://underscorejs.org/LICENSE>
 * Copyright 2009-2015 Jeremy Ashkenas, DocumentCloud and Investigative Reporters & Editors
 * Available under MIT license <https://lodash.com/license>
 * Copyright (c) 2015 Jon Schlinkert
 */

'use strict';

var utils = require('./lib/utils');

/**
 * Create an instance of `Engine` with the given options.
 *
 * ```js
 * var Engine = require('engine');
 * var engine = new Engine();
 *
 * // or
 * var engine = require('engine')();
 * ```
 *
 * @param {Object} `options`
 * @api public
 */

function Engine(options) {
  if (!(this instanceof Engine)) {
    return new Engine(options);
  }

  this.options = options || {};
  this.init(this.options);
}

/**
 * Initialize defaults
 */

Engine.prototype.init = function(opts) {
  this.imports = opts.imports || {};

  opts.variable = '';
  this.settings = {};
  this.counter = 0;
  this.cache = {};

  // regex
  opts.escape = opts.escape || utils.reEscape;
  opts.evaluate = opts.evaluate || utils.reEvaluate;
  opts.interpolate = opts.interpolate || utils.reInterpolate;

  // register helpers
  if (opts.helpers) {
    this.helpers(opts.helpers);
  }
  // load data
  if (opts.data) {
    this.data(opts.data);
  }
};

/**
 * Register a template helper.
 *
 * ```js
 * engine.helper('upper', function(str) {
 *   return str.toUpperCase();
 * });
 *
 * engine.render('<%= upper(user) %>', {user: 'doowb'});
 * //=> 'DOOWB'
 * ```
 *
 * @param  {String} `prop`
 * @param  {Function} `fn`
 * @return {Object} Instance of `Engine` for chaining
 * @api public
 */

Engine.prototype.helper = function(prop, fn) {
  if (typeof prop === 'object') {
    this.helpers(prop);
  } else {
    utils.set(this.imports, prop, fn);
  }
  return this;
};

/**
 * Register an object of template helpers.
 *
 * ```js
 * engine.helpers({
 *  upper: function(str) {
 *    return str.toUpperCase();
 *  },
 *  lower: function(str) {
 *    return str.toLowerCase();
 *  }
 * });
 *
 * // Or, just require in `template-helpers`
 * engine.helpers(require('template-helpers')._);
 * ```
 * @param  {Object|Array} `helpers` Object or array of helper objects.
 * @return {Object} Instance of `Engine` for chaining
 * @api public
 */

Engine.prototype.helpers = function(helpers) {
  return this.visit('helper', helpers);
};

/**
 * Add data to be passed to templates as context.
 *
 * ```js
 * engine.data({first: 'Brian'});
 * engine.render('<%= last %>, <%= first %>', {last: 'Woodward'});
 * //=> 'Woodward, Brian'
 * ```
 *
 * @param  {String|Object} `key` Property key, or an object
 * @param  {any} `value` If key is a string, this can be any typeof value
 * @return {Object} Engine instance, for chaining
 * @api public
 */

Engine.prototype.data = function(prop, value) {
  this.cache.data = this.cache.data || {};
  if (typeof prop === 'object') {
    this.visit('data', prop);
  } else {
    utils.set(this.cache.data, prop, value);
  }
  return this;
};

/**
 * Generate the regex to use for matching template variables.
 * @param  {Object} `opts`
 * @return {RegExp}
 */

Engine.prototype._regex = function (opts) {
  opts = utils.assign({}, this.options, opts);
  if (!opts.interpolate && !opts.regex && !opts.escape && !opts.evaluate) {
    return utils.delimiters;
  }

  var interpolate = opts.interpolate || utils.reNoMatch;
  if (utils.typeOf(opts.regex) === 'regexp') {
    interpolate = opts.regex;
  }

  var reString = (opts.escape || utils.reNoMatch).source
    + '|' + interpolate.source

    + '|' + (interpolate === utils.reInterpolate
      ? utils.reEsTemplate
      : utils.reNoMatch).source

    + '|' + (opts.evaluate || utils.reNoMatch).source;
  return RegExp(reString + '|$', 'g');
};

/**
 * Creates a compiled template function that can interpolate data properties
 * in "interpolate" delimiters, HTML-escape interpolated data properties in
 * "escape" delimiters, and execute JavaScript in "evaluate" delimiters. Data
 * properties may be accessed as free variables in the template. If a setting
 * object is provided it takes precedence over `engine.settings` values.
 *
 * ```js
 * var fn = engine.compile('Hello, <%= user %>!');
 * //=> [function]
 *
 * fn({user: 'doowb'});
 * //=> 'Hello, doowb!'
 *
 * fn({user: 'halle'});
 * //=> 'Hello, halle!'
 * ```
 *
 * @param {string} `str` The template string.
 * @param {Object} `opts` The options object.
 * @param {RegExp} [options.escape] The HTML "escape" delimiter.
 * @param {RegExp} [options.evaluate] The "evaluate" delimiter.
 * @param {Object} [options.imports] An object to import into the template as free variables.
 * @param {RegExp} [options.interpolate] The "interpolate" delimiter.
 * @param {string} [options.sourceURL] The sourceURL of the template's compiled source.
 * @param {string} [options.variable] The data object variable name.
 * @param- {Object} [data] Enables the legacy `options` param signature.
 * @returns {Function} Returns the compiled template function.
 * @api public
 */

Engine.prototype.compile = function (str, options, settings) {
  var assign = utils.assign;
  var engine = this;

  if (!(this instanceof Engine)) {
    if (utils.typeOf(options) !== 'object') options = {};
    engine = new Engine(options);
  }

  var opts = options || {};

  settings = assign({}, engine.settings, opts.settings, settings);
  opts = assign({}, engine.options, settings, opts);
  str = String(str);

  var imports = assign({}, opts.imports, opts.helpers, settings.imports);

  imports.escape = utils.escape;
  assign(imports, utils.omit(engine.imports, 'engine'));
  assign(imports, utils.omit(engine.cache.data, 'engine'));
  imports.engine = engine;

  var keys = Object.keys(imports);
  var values = keys.map(function(key) {
    return imports[key];
  });

  var isEscaping;
  var isEvaluating;
  var idx = 0;
  var source = "__p += '";

  // Use a sourceURL for easier debugging.
  var sourceURL = '//# sourceURL=' + ('sourceURL' in opts ? opts.sourceURL : ('engine.templateSources[' + (++engine.counter) + ']')) + '\n';

  // Compile the regexp to match each delimiter.
  var re = engine._regex(opts);

  str.replace(re, function (match, esc, interp, es6, evaluate, offset) {
    if (!interp) interp = es6;

    // Escape characters that can't be included in str literals.
    source += str.slice(idx, offset).replace(utils.reUnescapedString, utils.escapeStringChar);

    // Replace delimiters with snippets.
    if (esc) {
      isEscaping = true;
      source += "' +\n__e(" + esc + ") +\n'";
    }
    if (evaluate) {
      isEvaluating = true;
      source += "';\n" + evaluate + ";\n__p += '";
    }
    if (interp) {
      source += "' +\n((__t = (" + interp + ")) == null ? '' : __t) +\n'";
    }

    idx = offset + match.length;

    // The JS engine embedded in Adobe products requires returning the `match`
    // str in order to produce the correct `offset` value.
    return match;
  });

  source += "';\n";

  // If `variable` is not specified wrap a with-statement around the generated
  // code to add the data object to the top of the scope chain.
  var variable = opts.variable;
  if (!variable) {
    source = 'with (obj) {\n' + source + '\n}\n';
  }

  // Cleanup code by stripping empty strings.
  source = (isEvaluating ? source.replace(utils.reEmptyStringLeading, '') : source)
    .replace(utils.reEmptyStringMiddle, '$1')
    .replace(utils.reEmptyStringTrailing, '$1;');

  // Frame code as the function body.
  source = 'function('
    + (variable || 'obj') + ') {\n'
    + (variable ? '' : '(obj || (obj = {}));\n')
    + 'var __t, __p = ""'
    + (isEscaping ? ', __e = escape' : '')
    + (isEvaluating ? ', __j = Array.prototype.join;\nfunction print() { __p += __j.call(arguments, "") }\n' : ';\n')
    + source + 'return __p\n}';

  var result = utils.tryCatch(function () {
    return Function(keys, sourceURL + 'return ' + source).apply(null, values);
  });

  // Provide the compiled function's source by its `toString` method or
  // the `source` property as a convenience for inlining compiled templates.
  result.source = source;
  if (result instanceof Error) {
    throw result;
  }

  if (this && this.cache && this.cache.data) {
    result = result.bind(this.cache.data);
  }
  return result;
};

/**
 * Renders templates with the given data and returns a string.
 *
 * ```js
 * engine.render('<%= user %>', {user: 'doowb'});
 * //=> 'doowb'
 * ```
 * @param  {String} `str`
 * @param  {Object} `data`
 * @return {String}
 * @api public
 */

Engine.prototype.render = function(str, data) {
  var ctx = this.cache.data || {};
  var assign = utils.assign;

  ctx = assign({}, ctx, data);
  ctx = assign({}, ctx, ctx.imports || {});
  ctx = assign({}, ctx, ctx.helpers || {});

  if (typeof str === 'function') {
    return str(ctx);
  }
  return this.compile(str)(ctx);
};

/**
 * Visit the given `method` over `val`
 * @param  {String} `method`
 * @param  {Object|Array} `val`
 */

Engine.prototype.visit = function(method, val) {
  utils.visit(this, method, val);
  return this;
};

/**
 * Expose `Engine`
 */

module.exports = Engine;

/**
 * Expose `Engine`
 */

module.exports.utils = utils;
