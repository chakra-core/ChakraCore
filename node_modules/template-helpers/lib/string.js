'use strict';

var utils = require('./utils');

/**
 * camelCase the characters in `string`.
 *
 * ```js
 * <%= camelcase("foo bar baz") %>
 * //=> 'fooBarBaz'
 * ```
 *
 * @param  {String} `string` The string to camelcase.
 * @return {String}
 * @api public
 */

exports.camelcase = function camelcase(str) {
  if (!exports.isString(str)) return '';
  if (str.length === 1) {
    return str.toLowerCase();
  }

  var re = /[-_.\W\s]+(\w|$)/g;
  str = exports.chop(str).toLowerCase();
  return str.replace(re, function(_, ch) {
    return ch.toUpperCase();
  });
};

/**
 * Center align the characters in a string using
 * non-breaking spaces.
 *
 * ```js
 * <%= centerAlign("abc") %>
 * ```
 *
 * @param  {String} `str` The string to reverse.
 * @return {String} Centered string.
 * @related rightAlign
 * @api public
 */

exports.centerAlign = function centerAlign(str) {
  return exports.isString(str) ? utils.alignCenter(str, '&nbsp;') : '';
};

/**
 * Like trim, but removes both extraneous whitespace and
 * non-word characters from the beginning and end of a string.
 *
 * ```js
 * <%= chop("_ABC_") %>
 * //=> 'ABC'
 *
 * <%= chop("-ABC-") %>
 * //=> 'ABC'
 *
 * <%= chop(" ABC ") %>
 * //=> 'ABC'
 * ```
 *
 * @param  {String} `string` The string to chop.
 * @return {String}
 * @api public
 */

exports.chop = function chop(str) {
  if (!exports.isString(str)) return '';
  var re = /^[-_.\W\s]+|[-_.\W\s]+$/g;
  return str.trim().replace(re, '');
};

/**
 * Count the number of occurrances of a substring
 * within a string.
 *
 * ```js
 * <%= count("abcabcabc", "a") %>
 * //=> '3'
 * ```
 *
 * @param  {String} `string`
 * @param  {String} `substring`
 * @return {Number} The occurances of `substring` in `string`
 * @api public
 */

exports.count = function count(str, sub) {
  if (exports.isString(str)) {
    return (sub ? (str.split(sub).length - 1) : '0');
  }
  return '';
};

/**
 * dot.case the characters in `string`.
 *
 * ```js
 * <%= dotcase("a-b-c d_e") %>
 * //=> 'a.b.c.d.e'
 * ```
 *
 * @param  {String} `string`
 * @return {String}
 * @api public
 */

exports.dotcase = function dotcase(str) {
  if (!exports.isString(str)) return '';
  if (str.length === 1) { return str.toLowerCase(); }
  var re = /[-_.\W\s]+(\w|$)/g;
  return exports.chop(str).replace(re, function(_, ch) {
    return '.' + ch;
  });
};

/**
 * Truncate a string to the specified `length`, and append
 * it with an elipsis, `…`.
 *
 * ```js
 * <%= ellipsis("<span>foo bar baz</span>", 7) %>
 * //=> 'foo bar…'
 * ```
 *
 * @param  {String} `str`
 * @param  {Number} `length` The desired length of the returned string.
 * @param  {String} `ch` Optionally pass custom characters to append. Default is `…`.
 * @return {String} The truncated string.
 * @api public
 */

exports.ellipsis = function ellipsis(str, limit, ch) {
  return exports.isString(str)
    ? (exports.truncate(str, limit) + (ch || '…'))
    : '';
};

/**
 * Returns true if the value is a string.
 *
 * ```js
 * <%= isString('abc') %>
 * //=> 'true'
 *
 * <%= isString(null) %>
 * //=> 'false'
 * ```
 *
 * @param  {String} `val`
 * @return {Boolean} True if the value is a string.
 * @api public
 */

exports.isString = function isString(str) {
  return str && typeof str === 'string';
};

/**
 * Lowercase the characters in the given `string`.
 *
 * ```js
 * <%= lowercase("ABC") %>
 * //=> 'abc'
 * ```
 *
 * @param  {String} `string` The string to lowercase.
 * @return {String}
 * @api public
 */

exports.lowercase = exports.lower = function lowercase(str) {
  return exports.isString(str) ? str.toLowerCase() : '';
};

/**
 * PascalCase the characters in `string`.
 *
 * ```js
 * <%= pascalcase("foo bar baz") %>
 * //=> 'FooBarBaz'
 * ```
 *
 * @param  {String} `string`
 * @return {String}
 * @api public
 */

exports.pascalcase = function pascalcase(str) {
  if (!exports.isString(str)) return '';
  if (str.length === 1) { return str.toUpperCase(); }
  str = exports.camelcase(str);
  return str[0].toUpperCase() + str.slice(1);
};

/**
 * snake_case the characters in `string`.
 *
 * ```js
 * <%= snakecase("a-b-c d_e") %>
 * //=> 'a_b_c_d_e'
 * ```
 *
 * @param  {String} `string`
 * @return {String}
 * @api public
 */

exports.snakecase = function snakecase(str) {
  if (!exports.isString(str)) return '';
  if (str.length === 1) { return str.toLowerCase(); }
  var re = /[-_.\W\s]+(\w|$)/g;
  return exports.chop(str).replace(re, function(_, ch) {
    return '_' + ch;
  });
};

/**
 * Split `string` by the given `character`.
 *
 * ```js
 * <%= split("a,b,c", ",") %>
 * //=> ['a', 'b', 'c']
 * ```
 *
 * @param  {String} `string` The string to split.
 * @return {String} `character` Default is `,`
 * @api public
 */

exports.split = function split(str, ch) {
  return exports.isString(str) ? str.split(ch || ',') : '';
};

/**
 * Strip `substring` from the given `string`.
 *
 * ```js
 * <%= strip("foo-bar", "foo-") %>
 * //=> 'bar'
 * ```
 *
 * @param  {String|RegExp} `substring` The string or regex pattern of the substring to remove.
 * @param  {String} `string` The target string.
 * @api public
 */

exports.strip = function strip(val, str) {
  if (!exports.isString(val) && !(val instanceof RegExp)) return '';
  if (!exports.isString(str)) return '';
  return str.split(val).join('');
};

/**
 * Strip the indentation from a `string`.
 *
 * ```js
 * <%= stripIndent("  _ABC_") %>
 * //=> 'ABC'
 * ```
 *
 * @param  {String} `string` The string to strip indentation from.
 * @return {String}
 * @api public
 */

exports.stripIndent = function stripIndent(str) {
  if (!exports.isString(str)) return '';

  var matches = str.match(/^[ \t]+(?!\s)/gm);
  if (!matches) return str;

  var ws = matches.reverse().pop();
  var len = ws.length;
  if (len) {
    var re = new RegExp('^[ \\t]{' + len + '}', 'gm');
    return str.replace(re, '');
  }
  return str;
};

/**
 * Trim extraneous whitespace from the beginning and end
 * of a string.
 *
 * ```js
 * <%= trim("  ABC   ") %>
 * //=> 'ABC'
 * ```
 *
 * @param  {String} `string` The string to trim.
 * @return {String}
 * @api public
 */

exports.trim = function trim(str) {
  return exports.isString(str) ? str.trim() : '';
};

/**
 * dash-case the characters in `string`. This is similar to [slugify],
 * but [slugify] makes the string compatible to be used as a URL slug.
 *
 * ```js
 * <%= dashcase("a b.c d_e") %>
 * //=> 'a-b-c-d-e'
 * ```
 *
 * @param  {String} `string`
 * @return {String}
 * @tags dasherize, dashify, hyphenate, case
 * @api public
 */

exports.dashcase = function dashcase(str) {
  if (!exports.isString(str)) return '';
  if (str.length === 1) { return str.toLowerCase(); }
  var re = /[-_.\W\s]+(\w|$)/g;
  return exports.chop(str).replace(re, function(_, ch) {
    return '-' + ch;
  });
};

/**
 * path/case the characters in `string`.
 *
 * ```js
 * <%= pathcase("a-b-c d_e") %>
 * //=> 'a/b/c/d/e'
 * ```
 *
 * @param  {String} `string`
 * @return {String}
 * @api public
 */

exports.pathcase = function pathcase(str) {
  if (!exports.isString(str)) return '';
  if (str.length === 1) { return str.toLowerCase(); }
  var re = /[_.-\W\s]+(\w|$)/g;
  return exports.chop(str).replace(re, function(_, ch) {
    return '/' + ch;
  });
};

/**
 * Sentence-case the characters in `string`.
 *
 * ```js
 * <%= sentencecase("foo bar baz.") %>
 * //=> 'Foo bar baz.'
 * ```
 *
 * @param  {String} `string`
 * @return {String}
 * @api public
 */

exports.sentencecase = function sentencecase(str) {
  if (!exports.isString(str)) return '';
  if (str.length === 1) { return str.toUpperCase(); }
  return str.charAt(0).toUpperCase() + str.slice(1);
};

/**
 * Replace spaces in a string with hyphens. This
 *
 * ```js
 * <%= hyphenate("a b c") %>
 * //=> 'a-b-c'
 * ```
 *
 * @param  {String} `string`
 * @return {String}
 * @api public
 */

exports.hyphenate = function hyphenate(str) {
  return exports.isString(str) ? exports.chop(str).split(' ').join('-') : '';
};

/**
 * TODO: Need to use a proper slugifier for this.
 * I don't want to use `node-slug`, it's way too
 * huge for this.
 *
 * ```js
 * <%= slugify('This is a post title.') %>
 * //=> 'this-is-a-post-title'
 * ```
 *
 * @param  {String} `string` The string to slugify.
 * @return {String} Slug.
 * @api draft
 */

exports.slugify = require('helper-slugify');

/**
 * Reverse the characters in a string.
 *
 * ```js
 * <%= reverse("abc") %>
 * //=> 'cba'
 * ```
 *
 * @param  {String} `str` The string to reverse.
 * @return {String}
 * @api public
 */

exports.reverse = function reverse(str) {
  return exports.isString(str) ? str.split('').reverse().join('') : '';
};

/**
 * Right align the characters in a string using
 * non-breaking spaces.
 *
 * ```js
 * <%= rightAlign(str) %>
 * ```
 *
 * @param  {String} `str` The string to reverse.
 * @return {String} Right-aligned string.
 * @related centerAlign
 * @api public
 */

exports.rightAlign = function rightAlign(str) {
  return exports.isString(str) ? utils.alignRight(str) : '';
};

/**
 * Replace occurrences of `a` with `b`.
 *
 * ```js
 * <%= replace("abcabc", /a/, "z") %>
 * //=> 'zbczbc'
 * ```
 *
 * @param  {String} `str`
 * @param  {String|RegExp} `a` Can be a string or regexp.
 * @param  {String} `b`
 * @return {String}
 * @api public
 */

exports.replace = function replace(str, a, b) {
  if (!exports.isString(str)) return '';
  if (!a) { return str; }
  if (!b) { b = ''; }
  return str.split(a).join(b);
};

/**
 * Truncate a string by removing all HTML tags and limiting the result
 * to the specified `length`.
 *
 * ```js
 * <%= titlecase("big deal") %>
 * //=> 'foo bar'
 * ```
 *
 * @param  {String} `str`
 * @param  {Number} `length` The desired length of the returned string.
 * @return {String} The truncated string.
 * @api public
 */

exports.titlecase = exports.titleize = function titlecase(str, length) {
  return exports.isString(str) ? utils.titlecase(str) : '';
};

/**
 * Truncate a string by removing all HTML tags and limiting the result
 * to the specified `length`.
 *
 * ```js
 * <%= truncate("<span>foo bar baz</span>", 7) %>
 * //=> 'foo bar'
 * ```
 *
 * @param  {String} `str`
 * @param  {Number} `length` The desired length of the returned string.
 * @return {String} The truncated string.
 * @api public
 */

exports.truncate = function truncate(str, length) {
  return exports.isString(str) ? str.substr(0, length) : '';
};

/**
 * Uppercase the characters in a string.
 *
 * ```js
 * <%= uppercase("abc") %>
 * //=> 'ABC'
 * ```
 *
 * @param  {String} `string` The string to uppercase.
 * @return {String}
 * @api public
 */

exports.uppercase = exports.upper = function uppercase(str) {
  return exports.isString(str) ? str.toUpperCase() : '';
};
// ensure code conext picks up alias as a method
exports.upper = function upper(str) {
  return exports.uppercase.apply(null, arguments);
};

/**
 * Wrap words to a specified width using [word-wrap].
 *
 * ```js
 * <%= wordwrap("a b c d e f", {width: 5, newline: '<br>  '}) %>
 * //=> '  a b c <br>  d e f'
 * ```
 *
 * @param  {String} `string` The string with words to wrap.
 * @param  {Options} `object` Options to pass to [word-wrap]
 * @return {String} Formatted string.
 * @api public
 */

exports.wordwrap = function wordwrap(str) {
  return exports.isString(str) ? utils.wrap.apply(utils.wrap, arguments) : '';
};
