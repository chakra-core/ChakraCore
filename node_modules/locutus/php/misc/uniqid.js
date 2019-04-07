'use strict';

module.exports = function uniqid(prefix, moreEntropy) {
  //  discuss at: http://locutus.io/php/uniqid/
  // original by: Kevin van Zonneveld (http://kvz.io)
  //  revised by: Kankrelune (http://www.webfaktory.info/)
  //      note 1: Uses an internal counter (in locutus global) to avoid collision
  //   example 1: var $id = uniqid()
  //   example 1: var $result = $id.length === 13
  //   returns 1: true
  //   example 2: var $id = uniqid('foo')
  //   example 2: var $result = $id.length === (13 + 'foo'.length)
  //   returns 2: true
  //   example 3: var $id = uniqid('bar', true)
  //   example 3: var $result = $id.length === (23 + 'bar'.length)
  //   returns 3: true

  if (typeof prefix === 'undefined') {
    prefix = '';
  }

  var retId;
  var _formatSeed = function _formatSeed(seed, reqWidth) {
    seed = parseInt(seed, 10).toString(16); // to hex str
    if (reqWidth < seed.length) {
      // so long we split
      return seed.slice(seed.length - reqWidth);
    }
    if (reqWidth > seed.length) {
      // so short we pad
      return Array(1 + (reqWidth - seed.length)).join('0') + seed;
    }
    return seed;
  };

  var $global = typeof window !== 'undefined' ? window : global;
  $global.$locutus = $global.$locutus || {};
  var $locutus = $global.$locutus;
  $locutus.php = $locutus.php || {};

  if (!$locutus.php.uniqidSeed) {
    // init seed with big random int
    $locutus.php.uniqidSeed = Math.floor(Math.random() * 0x75bcd15);
  }
  $locutus.php.uniqidSeed++;

  // start with prefix, add current milliseconds hex string
  retId = prefix;
  retId += _formatSeed(parseInt(new Date().getTime() / 1000, 10), 8);
  // add seed hex string
  retId += _formatSeed($locutus.php.uniqidSeed, 5);
  if (moreEntropy) {
    // for more entropy we add a float lower to 10
    retId += (Math.random() * 10).toFixed(8).toString();
  }

  return retId;
};
//# sourceMappingURL=uniqid.js.map