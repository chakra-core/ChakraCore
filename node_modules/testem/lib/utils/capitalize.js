'use strict';

module.exports = str => {
  if (typeof str !== 'string') { return; }

  // Special case for PhantomJS
  if (str.toLowerCase() === 'phantomjs') {
    return 'PhantomJS';
  }

  return str.replace(/\w+/g, word => word.replace(/^[a-z]/, firstCharacter => firstCharacter.toUpperCase()));
};
