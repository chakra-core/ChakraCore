/*!
 * inline-source-map-comment | MIT (c) Shinnosuke Watanabe
 * https://github.com/shinnn/inline-source-map-comment
*/
'use strict';

var xtend = require('xtend');

module.exports = function inlineSourceMapComment(map, options) {
  options = options || {};

  if (typeof map === 'string') {
    map = JSON.parse(map);
  } else {
    if (arguments.length === 0) {
      throw new Error('More than one argument required.');
    }
  }

  if (!options.sourcesContent) {
    map = xtend(map);
    delete map.sourcesContent;
  }

  var sourceMapBody = module.exports.prefix + new Buffer(JSON.stringify(map)).toString('base64');

  if (options.block) {
    return '/*' + sourceMapBody + ' */';
  }
  return '//' + sourceMapBody;
};

module.exports.prefix = '# sourceMappingURL=data:application/json;base64,';
