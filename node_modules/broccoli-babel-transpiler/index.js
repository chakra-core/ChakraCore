'use strict';

const Filter     = require('broccoli-persistent-filter');
const clone      = require('clone');
const path       = require('path');
const stringify  = require('json-stable-stringify');
const mergeTrees = require('broccoli-merge-trees');
const funnel     = require('broccoli-funnel');
const crypto     = require('crypto');
const hashForDep = require('hash-for-dep');
const transformString = require('./lib/parallel-api').transformString;
const transformIsParallelizable = require('./lib/parallel-api').transformIsParallelizable;

function getExtensionsRegex(extensions) {
  return extensions.map(extension => {
    return new RegExp('\.' + extension + '$');
  });
}

function replaceExtensions(extensionsRegex, name) {
  for (let i = 0, l = extensionsRegex.length; i < l; i++) {
    name = name.replace(extensionsRegex[i], '');
  }

  return name;
}

module.exports = Babel;
function Babel(inputTree, _options) {
  if (!(this instanceof Babel)) {
    return new Babel(inputTree, _options);
  }

  let options = _options || {};
  options.persist = 'persist' in options ? options.persist : true;
  options.async = true;

  if (options.browserPolyfill) {
    let polyfillPath = require.resolve('@babel/polyfill/package.json');
    polyfillPath = path.join(path.dirname(polyfillPath), "dist");

    let browserPolyfillPath = options.browserPolyfillPath || "browser-polyfill.js"
    let polyfill = funnel(polyfillPath, {
      files: ['polyfill.js'],
      getDestinationPath: () => browserPolyfillPath
    });
    inputTree = mergeTrees([polyfill, inputTree]);
  }
  delete options.browserPolyfill;
  delete options.browserPolyfillPath;

  Filter.call(this, inputTree, options);

  delete options.persist;
  delete options.async;
  delete options.annotation;
  delete options.description;

  this.console = options.console || console;
  this.throwUnlessParallelizable = options.throwUnlessParallelizable;

  delete options.console;
  delete options.throwUnlessParallelizable;

  this.options = options;
  this.extensions = this.options.filterExtensions || ['js'];
  this.extensionsRegex = getExtensionsRegex(this.extensions);
  this.name = 'broccoli-babel-transpiler';

  if (this.options.helperWhiteList) {
    this.helperWhiteList = this.options.helperWhiteList;
  }

  // Note, Babel does not support this option so we must save it then
  // delete it from the options hash
  delete this.options.helperWhiteList;

  let result = transformIsParallelizable(options);
  let isParallelizable = result.isParallelizable;
  let errors = result.errors;

  if ((this.throwUnlessParallelizable || process.env.THROW_UNLESS_PARALLELIZABLE) && isParallelizable === false) {
    throw new Error(this.toString() +
      ' was configured to `throwUnlessParallelizable` and was unable to parallelize a plugin. \nplugins:\n' + joinCount(errors) + '\nPlease see: https://github.com/babel/broccoli-babel-transpiler#parallel-transpilation for more details');
  }
}

function joinCount(list) {
  let summary = '';

  for (let i = 0; i < list.length; i++) {
    summary += `${i + 1}: ${list[i]}\n`
  }

  return summary;
}

Babel.prototype = Object.create(Filter.prototype);
Babel.prototype.constructor = Babel;
Babel.prototype.targetExtension = 'js';

Babel.prototype.baseDir = function() {
  return __dirname;
};

Babel.prototype.transform = function(string, options) {
  return transformString(string, options);
};

/*
 * @private
 *
 * @method optionsString
 * @returns a stringified version of the input options
 */
Babel.prototype.optionsHash = function() {
  let options = this.options;
  let hash = {};
  let key, value;

  if (!this._optionsHash) {
    for (key in options) {
      value = options[key];
      hash[key] = (typeof value === 'function') ? (value + '') : value;
    }

    if (options.plugins) {
      hash.plugins = [];

      let cacheableItems = options.plugins.slice();

      for (let i = 0; i < cacheableItems.length; i++) {
        let item = cacheableItems[i];

        let type = typeof item;
        let augmentsCacheKey = false;
        let providesBaseDir = false;
        let requiresBaseDir = true;

        if (type === 'function') {
          augmentsCacheKey = typeof item.cacheKey === 'function';
          providesBaseDir = typeof item.baseDir === 'function';

          if (augmentsCacheKey) {
            hash.plugins.push(item.cacheKey());
          }

          if (providesBaseDir) {
            let depHash = hashForDep(item.baseDir());

            hash.plugins.push(depHash);
          }

          if (!providesBaseDir && requiresBaseDir){
            // prevent caching completely if the plugin doesn't provide baseDir
            // we cannot ensure that we aren't causing invalid caching pain...
            this.console.warn('broccoli-babel-transpiler is opting out of caching due to a plugin that does not provide a caching strategy: `' + item + '`.');
            hash.plugins.push((new Date).getTime() + '|' + Math.random());
            break;
          }
        } else if (Array.isArray(item)) {
          item.forEach(part => cacheableItems.push(part));
          continue;
        } else if (type !== 'object' || item === null) {
          // handle native strings, numbers, or null (which can JSON.stringify properly)
          hash.plugins.push(item);
          continue;
        } else if (type === 'object' && (typeof item.baseDir === 'function')) {
          hash.plugins.push(hashForDep(item.baseDir()));

          if (typeof item.cacheKey === 'function') {
            hash.plugins.push(item.cacheKey());
          }
        } else if (type === 'object') {
          // iterate all keys in the item and push them into the cache
          Object.keys(item).forEach(key => {
            cacheableItems.push(key);
            cacheableItems.push(item[key]);
          });
          continue;
        } else {
          this.console.warn('broccoli-babel-transpiler is opting out of caching due to an non-cacheable item: `' + item + '` (' + type + ').');
          hash.plugins.push((new Date).getTime() + '|' + Math.random());
          break;
        }
      }
    }

    this._optionsHash = crypto.createHash('md5').update(stringify(hash), 'utf8').digest('hex');
  }

  return this._optionsHash;
};

Babel.prototype.cacheKeyProcessString = function(string, relativePath) {
  return this.optionsHash() + Filter.prototype.cacheKeyProcessString.call(this, string, relativePath);
};

Babel.prototype.processString = function(string, relativePath) {
  let options = this.copyOptions();

  options.filename = options.sourceFileName = relativePath;

  if (options.moduleId === true) {
    options.moduleId = replaceExtensions(this.extensionsRegex, options.filename);
  }

  let optionsObj = { 'babel' : options, 'cacheKey' : this._optionsHash};
  return this.transform(string, optionsObj)
    .then(transpiled => {
      if (this.helperWhiteList) {
        let invalidHelpers = transpiled.metadata.usedHelpers.filter(helper => {
          return this.helperWhiteList.indexOf(helper) === -1;
        });

        validateHelpers(invalidHelpers, relativePath);
      }

      return transpiled.code;
    });
};

Babel.prototype.copyOptions = function() {
  let cloned = clone(this.options);
  if (cloned.filterExtensions) {
    delete cloned.filterExtensions;
  }
  if (cloned.targetExtension) {
    delete cloned.targetExtension;
  }
  return cloned;
};

function validateHelpers(invalidHelpers, relativePath) {
  if (invalidHelpers.length > 0) {
    let message = relativePath + ' was transformed and relies on `' + invalidHelpers[0] + '`, which was not included in the helper whitelist. Either add this helper to the whitelist or refactor to not be dependent on this runtime helper.';

    if (invalidHelpers.length > 1) {
      let helpers = invalidHelpers.map((item, i) => {
        if (i === invalidHelpers.length - 1) {
          return '& `' + item;
        } else if (i === invalidHelpers.length - 2) {
          return item + '`, ';
        }

        return item + '`, `';
      }).join('');

      message = relativePath + ' was transformed and relies on `' + helpers + '`, which were not included in the helper whitelist. Either add these helpers to the whitelist or refactor to not be dependent on these runtime helpers.';
    }

    throw new Error(message);
  }
}
