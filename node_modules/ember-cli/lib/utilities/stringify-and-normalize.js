'use strict';

module.exports = function stringifyAndNormalize(contents) {
  return `${JSON.stringify(contents, null, 2)}\n`;
};
