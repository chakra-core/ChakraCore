'use strict';

var Promise = require('rsvp').Promise;

module.exports = {
  init: function() {},

  processString: function(ctx, contents, relativePath) {
    var string = new Promise(function(resolve) {
      resolve(ctx.processString(contents, relativePath));
    });

    return string.then(function(value) {
      var normalizedValue = value;

      if (typeof value === 'string') {
        normalizedValue = { output: value };
      }

      return new Promise(function(resolve) {
        resolve(ctx.postProcess(normalizedValue, relativePath));
      })
        .then(function(result) {
          if (result === undefined) {
            throw new Error('You must return an object from `Filter.prototype.postProcess`.');
          }

          return result.output;
        });
    });
  }
};
