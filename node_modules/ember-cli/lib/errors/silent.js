'use strict';

const SilentError = require('silent-error');
const deprecate = require('../utilities/deprecate');

Object.defineProperty(module, 'exports', {
  get() {
    // Get the call stack so we can let the user know what module is using the deprecated function.
    let stack = new Error().stack;
    stack = stack.split('\n')[5];
    stack = stack.replace('    at ', '  ');

    deprecate(`\`ember-cli/lib/errors/silent.js\` is deprecated, use \`silent-error\` instead. Required here: \n${stack.toString()}`, true);
    return SilentError;
  },
});
