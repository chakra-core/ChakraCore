'use strict';

var Page = require('./page');

/**
 * Pages constructor
 *
 * ```js
 * var pages = new Pages();
 * ```
 *
 * @param {Array} `pages` Optional array of pages to initialize the `pages` array.
 * @api public
 */

function Pages(pages) {
  this.pages = [];
  if (typeof pages === 'undefined') return;
  if (!Array.isArray(pages)) {
    throw new TypeError('expected pages to be an Array');
  }
  this.addPages(pages);
}

/**
 * Add a page to the list.
 *
 * ```js
 * pages.addPage({items: [1, 2, 3]});
 * ```
 *
 * @param {Object} `page` Plain object or instance of a `Page`
 * @return {Object} Returns the instance for chaining
 * @api public
 */

Pages.prototype.addPage = function(page) {
  if (!(page instanceof Page)) page = new Page(page);
  this.pages.push(decorate(this, page));
  return this;
};

/**
 * Add an array of pages to the list.
 *
 * ```js
 * pages.addPages([...]);
 * ```
 *
 * @param {Object} `pages` Array of page objects
 * @return {Object} Returns the instance for chaining
 * @api public
 */

Pages.prototype.addPages = function(pages) {
  for (var i = 0; i < pages.length; i++) {
    this.addPage(pages[i]);
  }
  return this;
};

/**
 * Decorates a page with additional properties.
 *
 * @param  {Object} `page` Instance of page to decorate
 * @return {Object} Returns the decorated page to be added to the list
 */

function decorate(pages, page) {
  Object.defineProperties(page, {
    first: {
      enumerable: true,
      set: function() {},
      get: function() {
        return pages.first && pages.first.current;
      }
    },

    current: {
      enumerable: true,
      set: function() {},
      get: function() {
        return this.idx + 1;
      }
    },

    last: {
      enumerable: true,
      set: function() {},
      get: function() {
        return pages.last && pages.last.current;
      }
    },

    total: {
      enumerable: true,
      set: function() {},
      get: function() {
        return pages.total;
      }
    }
  });

  var prev = pages.last;
  var idx = pages.total;
  page.idx = idx;
  if (prev) {
    page.prev = prev.current;
    prev.next = page.current;
  }
  return page;
}

/**
 * Getters
 */

Object.defineProperties(Pages.prototype, {

  /**
   * Helper property to calculate the total pages in the array.
   */

  total: {
    get: function() {
      return this.pages.length;
    }
  },

  /**
   * Helper property to get the first page from the array.
   */

  first: {
    get: function() {
      return this.total > 0 ? this.pages[0] : null;
    }
  },

  /**
   * Helper property to get the last page from the array.
   */

  last: {
    get: function() {
      return this.total > 0 ? this.pages[this.total - 1] : null;
    }
  }
});

/**
 * Expose `Pages`
 */

module.exports = Pages;
