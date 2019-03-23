'use strict';

const capitalize = require('./capitalize');
const log = require('npmlog');
const castArray = require('lodash.castarray');

function Validation() {
  this.knownBrowser = null;
  this.messages = [];
  this.valid = true;
}

function warn(message) {
  log.warn('', message);
}

function hasOldBrowserArgsFormat(args) {
  return args.hasOwnProperty('mode') ||
    args.hasOwnProperty('args');
}

// Lodash's castArray function will turn `undefined` into `[undefined]`, but we want an empty array in that case.
function safeCastArray(value) {
  if (value) {
    return castArray(value);
  } else {
    return [];
  }
}

module.exports = {
  addCustomArgs(knownBrowsers, config) {
    if (!knownBrowsers || !config) { return; }

    let browserArgs = config.get('browser_args');
    let browserName;

    if (browserArgs && typeof browserArgs === 'object') {
      for (browserName in browserArgs) {
        if (browserArgs.hasOwnProperty(browserName)) {
          let args = browserArgs[browserName];

          if (typeof args === 'object' && !Array.isArray(args)) {
            if (hasOldBrowserArgsFormat(args)) {
              if (!args.mode) {
                warn('Type error: when using an object to specify browser_args for ' + browserName + ' you must specify a mode');
                continue;
              } else if (!args.args) {
                warn('Type error: when using an object to specify browser_args for ' + browserName + ' you must specify args');
                continue;
              }

              if (args.mode !== config.appMode) {
                continue;
              }

              warn('[DEPRECATION] Specifying browser_args as a hash with "mode" and "args" properties has been deprecated. Mode should be a key with its arguments as a value.');
              args = args.args;
            } else {
              args = safeCastArray(args.all).concat(safeCastArray(args[config.appMode]));
            }
          }

          this.parseArgs(capitalize(browserName), args, knownBrowsers);
        }
      }
    } else if (browserArgs !== undefined) {
      warn('Type error: browser_args should be an object');
    }

    return knownBrowsers;
  },

  createValidation() {
    return new Validation();
  },

  dedupeBrowserArgs(browserName, browserArgs) {
    if (!browserName || !Array.isArray(browserArgs)) { return; }

    let argHash = {};

    browserArgs.forEach(arg => {
      if (arg in argHash) {
        warn('Removed duplicate arg for ' + browserName + ': ' + arg);
      } else {
        argHash[arg] = null;
      }
    });

    return Object.keys(argHash);
  },

  parseArgs(browserName, browserArgs, knownBrowsers) {
    if (!browserName || !browserArgs || !knownBrowsers) { return; }

    let patchArgs;
    let self = this;
    let validation = this.validate(browserName, browserArgs, knownBrowsers);

    if (validation.valid) {
      if (typeof browserArgs === 'string') {
        browserArgs = [browserArgs];
      }

      patchArgs = validation.knownBrowser.args;

      if (Array.isArray(patchArgs)) {
        validation.knownBrowser.args = browserArgs.concat(validation.knownBrowser.args);
      } else if (typeof patchArgs === 'function') {
        validation.knownBrowser.args = function() {
          return self.dedupeBrowserArgs(browserName,
            browserArgs.concat(patchArgs.apply(this, arguments)));
        };
      } else if (patchArgs === undefined) {
        validation.knownBrowser.args = () => browserArgs;
      }
    } else {
      validation.messages.forEach(message => {
        warn(message);
      });
    }
  },

  validate(browserName, browserArgs, knownBrowsers) {
    if (!browserName || !browserArgs || !(Array.isArray(knownBrowsers)) ||
      !knownBrowsers.length) { return; }

    let i;
    let len = knownBrowsers.length;
    let validation = this.createValidation();

    this.validateBrowserArgs(browserName, browserArgs, validation);

    for (i = 0; i < len; i++) {
      if (knownBrowsers[i].name === browserName) {
        validation.knownBrowser = knownBrowsers[i];
        break;
      }
    }

    if (!validation.knownBrowser) {
      validation.messages.push('Could not find "' + browserName + '" in known browsers');
    }

    validation.valid = !validation.messages.length;

    return validation;
  },

  validateBrowserArgs(browserName, browserArgs, validation) {
    if (!browserName || !validation) { return; }

    let arg;
    let i;
    let len;

    if (typeof browserArgs !== 'string' && !(Array.isArray(browserArgs))) {
      validation.messages.push('Type error: ' + browserName +
        '\'s "args" property should be a string or an array');
    } else if (typeof browserArgs === 'string' && !browserArgs.trim()) {
      validation.messages.push('Bad value: ' + browserName +
        '\'s "args" property should not be empty');
    } else if (Array.isArray(browserArgs)) {
      len = browserArgs.length;

      if (len) {
        for (i = 0; i < len; i++) {
          arg = browserArgs[i];

          if (typeof arg !== 'string') {
            validation.messages.push('Bad value: ' + browserName +
              '\'s "args" may only contain strings');
            break;
          } else if (!arg.trim()) {
            validation.messages.push('Bad value: ' + browserName +
              '\'s "args" may not contain empty strings');
            break;
          }
        }
      } else {
        validation.messages.push('Bad value: ' + browserName + '\'s "args" property should not be empty');
      }
    }
  }
};
