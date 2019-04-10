'use strict';

var person = require('./person');
var utils = require('../utils');

/**
 * Stringify a person object, or array of person objects, such as
 * `maintainer`, `collaborator`, `contributor`, and `author`.
 *
 * @param {Object|Array|String} `val` If an object is passed, it will be converted to a string. If an array of objects is passed, it will be converted to an array of strings.
 * @return {String}
 * @api public
 */

module.exports = function(persons, key, config, schema) {
  if (typeof persons === 'undefined') return;

  if (Array.isArray(persons)) {
    persons = persons.reduce(function(acc, val) {
      var parsed = person(val, key, config, schema);
      if (parsed) {
        acc.push(parsed);
      }
      return acc;
    }, []);
  }
  return persons;
};
