'use strict';

module.exports = function symbol(name) {
  let id = `EMBER_CLI${Math.floor(Math.random() * new Date())}`;
  return `__${name}__ [id=${id}]`;
};
