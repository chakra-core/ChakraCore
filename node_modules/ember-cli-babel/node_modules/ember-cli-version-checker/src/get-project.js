'use strict';

module.exports = function(input) {
  if (input.project) {
    return input.project;
  } else if (input.root && typeof input.isEmberCLIProject === 'function') {
    return input;
  } else {
    throw new Error(
      'You must provide an Addon, EmberApp/EmberAddon, or Project to check dependencies against'
    );
  }
};
