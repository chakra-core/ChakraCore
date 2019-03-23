'use strict';

var path = require('path');

var CleanCssPromise = require('clean-css-promise');
var Filter = require('broccoli-persistent-filter');
var inlineSourceMapComment = require('inline-source-map-comment');
var jsonStableStringify = require('json-stable-stringify');

function CleanCSSFilter(inputTree, options) {
  if (!(this instanceof CleanCSSFilter)) {
    return new CleanCSSFilter(inputTree, options);
  }

  this.inputTree = inputTree;

  Filter.call(this, inputTree, options);

  this.options = options || {};
  this._cleanCSS = null;
}

CleanCSSFilter.prototype = Object.create(Filter.prototype);
CleanCSSFilter.prototype.constructor = CleanCSSFilter;

CleanCSSFilter.prototype.extensions = ['css'];
CleanCSSFilter.prototype.targetExtension = 'css';

CleanCSSFilter.prototype.baseDir = function() {
  return __dirname;
};

CleanCSSFilter.prototype.optionsHash = function() {
  if (!this._optionsHash) {
    this._optionsHash = jsonStableStringify(this.options);
  }

  return this._optionsHash;
};

CleanCSSFilter.prototype.cacheKeyProcessString = function(string, relativePath) {
  return this.optionsHash() +
         Filter.prototype.cacheKeyProcessString.call(this, string, relativePath);
};

CleanCSSFilter.prototype.build = function() {
  var srcDir = this.inputPaths[0];
  var relativeTo = this.options.relativeTo;
  if (!relativeTo && relativeTo !== '' || typeof this.inputTree !== 'string') {
    this.options.relativeTo = path.resolve(srcDir, relativeTo || '.');
  }

  this._cleanCssPromise = new CleanCssPromise(this.options);

  return Filter.prototype.build.call(this);
};

CleanCSSFilter.prototype.processString = function(str) {
  return this._cleanCssPromise.minify(str).then(function(result) {
    if (result.sourceMap) {
      return result.styles + '\n' +
             inlineSourceMapComment(result.sourceMap, {block: true}) + '\n';
    }

    return result.styles;
  });
};

module.exports = CleanCSSFilter;
