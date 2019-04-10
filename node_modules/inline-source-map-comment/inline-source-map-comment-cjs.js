/*!
 * inline-source-map-comment | MIT (c) Shinnosuke Watanabe
 * https://github.com/shinnn/inline-source-map-comment
*/
'use strict';

function shallowCopy(obj) {
  var result = {};
  for (var key in obj) {
    if (obj.hasOwnProperty(key)) {
      result[key] = obj[key];
    }
  }
  return result;
}

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
    map = shallowCopy(map);
    delete map.sourcesContent;
  }

  var sourceMapBody = module.exports.prefix + btoa(JSON.stringify(map));

  if (options.block) {
    return '/*' + sourceMapBody + ' */';
  }
  return '//' + sourceMapBody;
};

module.exports.prefix = '# sourceMappingURL=data:application/json;base64,';
