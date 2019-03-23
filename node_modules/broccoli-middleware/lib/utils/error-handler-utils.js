'use strict';

/*
 * Turns a POJO with versions into a string of the following format:
 *
 * key@value, key@value, key@value.
 *
 * Ignores `null` or `undefined` values and sorts keys alphabetically.
 *
 * @return {String}
 */
function toVersionString(versions) {
  return Object.keys(versions).sort().filter(key => versions[key]).map((key) => {
    return versions[key] ? `${key}@${versions[key]}` : '';
  }).join(', ');
}

module.exports = { toVersionString };
