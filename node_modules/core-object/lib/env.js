'use strict';

module.exports = (
  (typeof window !== 'undefined' && window) ||
  (typeof process !== 'undefined' && process.env) ||
  {}
);
