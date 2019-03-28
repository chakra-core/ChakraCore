'use strict';

/**
 * Page constructor
 *
 * ```js
 * var page = new Page();
 * ```
 *
 * @param {Object} `page` optional page object to populate initial values.
 * @api public
 */

function Page(page) {
  if (!page) page = {};
  for (var key in page) this[key] = page[key];
  if (!this.hasOwnProperty('idx')) this.idx = 0;
  if (!this.hasOwnProperty('total')) this.total = 1;
  if (!this.hasOwnProperty('current')) {
    this.current = this.total;
  }
}

/**
 * Page getters
 */

Object.defineProperties(Page.prototype, {

  /**
   * Helper property to determine if this is the first page in a list.
   */

  isFirst: {
    configurable: true,
    enumerable: true,
    get: function() {
      return this.idx === 0;
    }
  },

  /**
   * Helper property to determine if this is the last page in a list.
   */

  isLast: {
    configurable: true,
    enumerable: true,
    get: function() {
      return this.idx === (this.total - 1);
    }
  },

  /**
   * Helper property to determine if this is there is a page before this one in a list.
   */

  hasPrevious: {
    configurable: true,
    enumerable: true,
    get: function() {
      return !this.isFirst;
    }
  },

  /**
   * Helper property to determine if this is there is a page before this one in a list.
   */

  hasPrev: {
    configurable: true,
    enumerable: true,
    get: function() {
      return !this.isFirst;
    }
  },

  /**
   * Helper property to determine if this is there is a page after this one in a list.
   */

  hasNext: {
    configurable: true,
    enumerable: true,
    get: function() {
      return !this.isLast;
    }
  }
});

/**
 * Expose `Page`
 */

module.exports = Page;
