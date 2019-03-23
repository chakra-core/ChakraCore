'use strict';

var glob = require('./async');

module.exports = function(patterns, options) {
  return new Promise(function(resolve, reject) {
    glob(patterns, options, function(err, files) {
      if (err) {
        reject(err);
        return;
      }
      resolve(files);
    });
  });
};
