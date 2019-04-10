/*!
 * paginationator <https://github.com/doowb/paginationator>
 *
 * Copyright (c) 2015, Brian Woodward.
 * Licensed under the MIT License.
 */

'use strict';

var Pages = require('./lib/pages');
var Page = require('./lib/page');

/**
 * Paginate an array with given options and return a `Page` object
 * containing an array of `pages` with pagination information.
 *
 * ```js
 * var pages = paginationator([1, 2, 3, 4, 5], {limit: 2});
 * //=> { pages: [
 * //=>   { idx: 0, total: 3, current: 1, items: [1, 2], first: 1, last: 3, next: 2 },
 * //=>   { idx: 1, total: 3, current: 2, items: [3, 4], first: 1, last: 3, prev: 1, next: 3 },
 * //=>   { idx: 2, total: 3, current: 3, items: [5], first: 1, last: 3, prev: 2 }
 * //=> ]}
 * ```
 *
 * @param  {Array}  `arr` Array of items to paginate
 * @param  {Object} `options` Additional options to control pagination
 * @param  {Number} `options.limit` Number of items per page (defaults to 10)
 * @return {Object} paginated pages
 * @api public
 * @name  paginationator
 */

module.exports = function paginationator(arr, options) {
  if (!Array.isArray(arr)) {
    throw new TypeError('expected arr to be an Array');
  }
  options = options || {};
  var limit = options.limit || 10;
  var total = Math.ceil(arr.length / limit);
  var i = 0;

  var pages = new Pages();
  while (i < total) {
    var page = new Page();
    var start = i * limit;
    var end = start + limit;
    page.items = arr.slice(start, end);
    pages.addPage(page);
    i++;
  }
  return pages;
};

/**
 * Expose `Pages`
 */

module.exports.Pages = Pages;

/**
 * Expose `Page`
 */

module.exports.Page = Page;
