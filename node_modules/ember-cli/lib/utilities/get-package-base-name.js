'use strict';

const npa = require('npm-package-arg');


module.exports = function(name) {
  if (!name) {
    return null;
  }

  return npa(name).name;
};
