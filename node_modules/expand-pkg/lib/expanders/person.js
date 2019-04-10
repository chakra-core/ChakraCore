'use strict';

var utils = require('../utils');
var cache = {};
var cache = {};

/**
 * Stringify a person object, or array of person objects, such as
 * `maintainer`, `collaborator`, `contributor`, and `author`.
 *
 * @param {Object|Array|String} `val` If an object is passed, it will be converted to a string. If an array of objects is passed, it will be converted to an array of strings.
 * @return {String}
 * @api public
 */

module.exports = function(val, key, config, schema) {
  if (typeof val === 'undefined') return;

  if (Array.isArray(val)) {
    var arr = [];
    val.forEach(function(person) {
      if (typeof person === 'string') {
        person = cache[person] || (cache[person] = toAuthor(person));
      }
      arr.push(person);
    });

    if (arr.length === 1) {
      val = arr[0];
    } else {
      val = arr;
    }
  }

  if (typeof val === 'string') {
    val = cache[val] || (cache[val] = toAuthor(val));
  }

  if (utils.isObject(val)) {
    username(val);
  }
  return val;
};

function toAuthor(val) {
  var author = utils.omitEmpty(utils.parseAuthor(val));
  username(author);
  return author;
}

// this is obviousy a very naive check, but I'd rather keep this
// simple since this can be overridden anyway
function username(author) {
  if (utils.isObject(author) && !author.username && author.url) {
    if (/(github|twitter)\.com/.test(author.url)) {
      author.username = author.url.split('/').pop();
    }
  }
}
