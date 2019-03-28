'use strict';

var utils = require('./utils');

/**
 * Expose `layouts`
 */

module.exports = renderLayouts;

/**
 * Cache compiled delimiter regex.
 *
 * If delimiters need to be generated, this ensures that
 * runtime compilation only happens once.
 */

var cache = {};

/**
 * Wrap one or more layouts around `string`.
 *
 * ```js
 * renderLayouts(string, layoutName, layouts, options, fn);
 * ```
 *
 * @param  {String} `string` The string to wrap with a layout.
 * @param  {String} `layoutName` The name (key) of the layout object to use.
 * @param  {Object} `layouts` Object of layout objects.
 * @param  {Object} `options` Optionally define a `defaultLayout` (string), pass custom delimiters (`layoutDelims`) to use as the placeholder for the content insertion point, or change the name of the placeholder tag with the `contentTag` option.
 * @param  {Function} `fn` Optionally pass a function to modify the context as each layout is applied.
 * @return {String} Returns the original string wrapped with one or more layouts.
 * @api public
 */

function renderLayouts(str, name, layouts, opts, fn) {
  if (isBuffer(str)) {
    str = String(str);
  }

  if (typeof str !== 'string') {
    throw new TypeError('expected content to be a string.');
  }
  if (typeof name !== 'string') {
    throw new TypeError('expected layout name to be a string.');
  }

  if (typeof opts === 'function') {
    fn = opts;
    opts = {};
  }

  opts = opts || {};
  var layout = {};
  var depth = 0;
  var prev;

  // `view` is the object we'll use to store the result
  var view = {options: {}, history: []};
  name = assertLayout(name, opts.defaultLayout);

  // recursively resolve layouts
  while (name && (prev !== name) && (layout = utils.getView(name, layouts))) {
    prev = name;

    var delims = opts.layoutDelims;

    // `data` is passed to `wrapLayout` to resolve layouts
    // to the values on the data object.
    var data = {};
    var tag = opts.contentTag || 'body';
    data[tag] = str;

    // get info about the current layout
    var obj = new Layout({
      name: name,
      layout: layout,
      before: str,
      depth: depth++
    });

    // get the delimiter regex
    var re = makeDelimiterRegex(delims, tag);

    // ensure that content is a string
    var content = (layout.content || layout.contents).toString();
    if (!re.test(content)) {
      throw error(re, name, tag);
    }

    // inject the string into the layout
    str = wrapLayout(content, data, re, name, tag);
    obj.after = str;

    // if a callback is passed, allow it modify the result
    if (typeof fn === 'function') {
      fn(obj, view, depth);
    }

    // push info about the layout onto `history`
    view.history.push(obj);

    // should we recurse again?
    // does the `layout` specify another layout?
    name = assertLayout(layout.layout, opts.defaultLayout);
  }

  if (typeof name === 'string' && prev !== name) {
    throw new Error('could not find layout "' + name + '"');
  }

  view.options = opts;
  view.result = str;
  return view;
}

/**
 * Create a new layout in the layout stack.
 *
 * @param {Object} `view` The layout view object
 * @param {Number} `depth` Current stack depth
 */

function Layout(view) {
  this.layout = view.layout;
  this.layout.name = view.name;
  this.before = view.before;
  this.depth = view.depth;
  return this;
}

/**
 * Assert whether or not a layout should be used based on
 * the given `value`.
 *
 *   - If a layout should be used, the name of the layout is returned.
 *   - If not, `null` is returned.
 *
 * @param  {*} `value`
 * @return {String|Null} Returns `true` or `null`.
 * @api private
 */

function assertLayout(value, defaultLayout) {
  if (value === false || (value && utils.isFalsey(value))) {
    return null;
  } else if (!value || value === true) {
    return defaultLayout || null;
  } else {
    return value;
  }
}

/**
 * Resolve template strings to the values on the given
 * `data` object.
 */

function wrapLayout(str, data, re, name, tag) {
  return str.replace(re, function(_, tagName) {
    var m = data[tagName.trim()];
    if (typeof m === 'undefined') {
      throw error(re, name, tag);
    }
    return m;
  });
}

/**
 * Make delimiter regex.
 *
 * @param  {Sring|Array|RegExp} `syntax`
 * @return {RegExp}
 */

function makeDelimiterRegex(syntax, tag) {
  if (!syntax) {
    syntax = makeTag(tag, ['{%', '%}']);
  }
  if (syntax instanceof RegExp) {
    return syntax;
  }
  var name = syntax + '';
  if (cache.hasOwnProperty(name)) {
    return cache[name];
  }
  if (typeof syntax === 'string') {
    return new RegExp(syntax, 'g');
  }
  return (cache[name] = utils.delims(syntax));
}

/**
 * Cast `val` to a string.
 */

function makeTag(val, delims) {
  return delims[0].trim()
    + ' ('
    + String(val).trim()
    + ') '
    + delims[1].trim();
}

/**
 * Format an error message
 */

function error(re, name, tag) {
  var delims = matchDelims(re, tag);
  return new Error('cannot find "' + delims + '" in "' + name + '"');
}

/**
 * Only used if an error is thrown. Attempts to recreate
 * delimiters for the error message.
 */

var types = {
  '{%=': function(str) {
    return '{%= ' + str + ' %}';
  },
  '{%-': function(str) {
    return '{%- ' + str + ' %}';
  },
  '{%': function(str) {
    return '{% ' + str + ' %}';
  },
  '{{': function(str) {
    return '{{ ' + str + ' }}';
  },
  '<%': function(str) {
    return '<% ' + str + ' %>';
  },
  '<%=': function(str) {
    return '<%= ' + str + ' %>';
  },
  '<%-': function(str) {
    return '<%- ' + str + ' %>';
  }
};

function matchDelims(re, str) {
  var ch = re.source.slice(0, 4);
  if (/[\\]/.test(ch.charAt(0))) {
    ch = ch.slice(1);
  }
  if (!/[-=]/.test(ch.charAt(2))) {
    ch = ch.slice(0, 2);
  } else {
    ch = ch.slice(0, 3);
  }
  return types[ch](str);
}

/**
 * Return true if the given value is a buffer
 */

function isBuffer(val) {
  if (val && val.constructor && typeof val.constructor.isBuffer === 'function') {
    return val.constructor.isBuffer(val);
  }
  return false;
}
