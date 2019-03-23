'use strict';

module.exports = function isAddon(keywords) {
  return Array.isArray(keywords) &&
    keywords.indexOf('ember-addon') >= 0;
};
