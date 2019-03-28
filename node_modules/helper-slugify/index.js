'use strict';

var stripColor = require('strip-color');

/**
 * Slugify the given `str` with `options.`
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

module.exports = function slugify(str, options) {
  if (!isString(str)) return '';
  str = getTitle(str);
  str = stripColor(str);
  str = str.toLowerCase();
  str = str.split(/ /).join('-');
  str = str.split(/\t/).join('--');
  str = str.split(/[|$&`~=\\\/@+*!?({[\]})<>=.,;:'"^]/).join('');
  return str;
};

/**
 * Return true if `str` is a non-empty string.
 */

function isString(str) {
  return str && typeof str === 'string';
}

/**
 * Get the "title" from a markdown link
 */

function getTitle(str) {
  if (/^\[[^\]]+\]\(/.test(str)) {
    var m = /^\[([^\]]+)\]/.exec(str);
    if (m) return m[1];
  }
  return str;
}
