'use strict';

// Replace all `undefined` values with `null`, so that they show up in JSON output
module.exports = function undefinedToNull(obj) {
  for (const key in obj) {
    if (obj.hasOwnProperty(key) && obj[key] === undefined) {
      obj[key] = null;
    }
  }
  return obj;
};
