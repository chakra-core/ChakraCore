'use strict';

const Promise = require('rsvp').Promise;

module.exports = {
  init() { },

  processString(ctx, contents, relativePath) {
    let string = new Promise(resolve => {
      resolve(ctx.processString(contents, relativePath));
    });

    return string.then(value => {
      let normalizedValue = value;

      if (typeof value === 'string') {
        normalizedValue = {
          output: value
        };
      }

      return new Promise(resolve => {
        resolve(ctx.postProcess(normalizedValue, relativePath));
      })
        .then(result => {
          if (result === undefined) {
            throw new Error('You must return an object from `Filter.prototype.postProcess`.');
          }

          return result.output;
        });
    });
  }
};
